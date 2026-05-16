#include "ir/LoopOptimizer.hpp"
#include <algorithm>

namespace minicompiler {

LoopOptimizer::LoopOptimizer() {}

void LoopOptimizer::optimize(IRProgram& program) {
    for (auto& func : program.functions) {
        auto loops = findLoops(func.get());
        
        for (auto& loop : loops) {
            if (moveInvariants) {
                moveLoopInvariants(func.get(), loop);
            }
            
            if (optimizeCounted && detectInductionVariable(loop)) {
                optimizeCountedLoop(func.get(), loop);
            }
            
            minimizeJumps(func.get(), loop);
        }
    }
}

std::vector<LoopOptimizer::LoopInfo> LoopOptimizer::findLoops(IRFunction* func) {
    std::vector<LoopInfo> loops;
    
    // Ищем блоки с обратными ребрами (back edges)
    for (auto& block : func->blocks) {
        for (auto succ : block->successors) {
            // Если successor доминирует над block — это обратное ребро
            // Упрощенно: ищем jump на блок с меткой "loop_*"
            for (auto& instr : block->instructions) {
                if (instr->opcode == IROpcode::JUMP) {
                    std::string target = instr->src1.name;
                    if (target.find("loop_") != std::string::npos) {
                        // Нашли цикл
                        LoopInfo info;
                        info.latch = block.get();
                        info.header = func->getBlock(target);
                        
                        if (info.header) {
                            // Находим exit
                            for (auto& hdrInstr : info.header->instructions) {
                                if (hdrInstr->opcode == IROpcode::JUMP_IF_NOT) {
                                    std::string exitName = hdrInstr->src2.name;
                                    info.exit = func->getBlock(exitName);
                                }
                            }
                            
                            // Собираем блоки цикла (упрощенно)
                            info.blocks.insert(info.header);
                            info.blocks.insert(info.latch);
                            if (info.exit) info.blocks.insert(info.exit);
                            
                            // Находим промежуточные блоки
                            for (auto& b : func->blocks) {
                                if (b.get() == info.header || b.get() == info.latch) continue;
                                // Если блок между header и latch
                                for (auto pred : b->predecessors) {
                                    if (info.blocks.count(pred)) {
                                        info.blocks.insert(b.get());
                                        break;
                                    }
                                }
                            }
                            
                            loops.push_back(info);
                        }
                    }
                }
            }
        }
    }
    
    return loops;
}

bool LoopOptimizer::isLoopInvariant(IRInstruction* instr, const LoopInfo& loop) {
    // Инструкция инвариантна если все её операнды:
    // - константы
    // - переменные, определенные вне цикла
    // - другие инвариантные инструкции
    
    if (instr->opcode == IROpcode::JUMP || 
        instr->opcode == IROpcode::JUMP_IF ||
        instr->opcode == IROpcode::JUMP_IF_NOT ||
        instr->opcode == IROpcode::RETURN ||
        instr->opcode == IROpcode::LABEL) {
        return false; // Управляющие инструкции не двигаем
    }
    
    // Проверяем src1
    if (instr->src1.kind == IROperand::Kind::VARIABLE) {
        // Если переменная определена в цикле — не инвариант
        for (auto block : loop.blocks) {
            for (auto& bi : block->instructions) {
                if (bi->dest.kind == IROperand::Kind::VARIABLE && 
                    bi->dest.name == instr->src1.name) {
                    return false;
                }
            }
        }
    }
    
    // Проверяем src2
    if (instr->src2.kind == IROperand::Kind::VARIABLE) {
        for (auto block : loop.blocks) {
            for (auto& bi : block->instructions) {
                if (bi->dest.kind == IROperand::Kind::VARIABLE && 
                    bi->dest.name == instr->src2.name) {
                    return false;
                }
            }
        }
    }
    
    return true;
}

void LoopOptimizer::moveLoopInvariants(IRFunction* func, LoopInfo& loop) {
    // Создаем preheader блок если нужно
    BasicBlock* preheader = func->createBlock(loop.header->name + "_preheader");
    
    // Переносим инвариантные инструкции в preheader
    for (auto block : loop.blocks) {
        auto& instrs = block->instructions;
        std::vector<std::unique_ptr<IRInstruction>> invariants;
        
        for (auto it = instrs.begin(); it != instrs.end(); ) {
            if (isLoopInvariant(it->get(), loop)) {
                invariants.push_back(std::move(*it));
                it = instrs.erase(it);
            } else {
                ++it;
            }
        }
        
        // Добавляем инварианты в preheader
        for (auto& inv : invariants) {
            preheader->addInstruction(std::move(inv));
        }
    }
    
    // Вставляем preheader перед header
    for (auto pred : loop.header->predecessors) {
        if (!loop.blocks.count(pred)) {
            // Заменяем переходы на preheader
            for (auto& instr : pred->instructions) {
                if (instr->opcode == IROpcode::JUMP && 
                    instr->src1.name == loop.header->name) {
                    instr->src1.name = preheader->name;
                }
            }
        }
    }
    
    // Добавляем jump из preheader в header
    auto jumpInstr = std::make_unique<IRInstruction>(
        IROpcode::JUMP, IROperand(), IROperand::label(loop.header->name));
    preheader->addInstruction(std::move(jumpInstr));
}

bool LoopOptimizer::detectInductionVariable(LoopInfo& loop) {
    // Ищем паттерн: i = i + 1 в latch блоке
    for (auto& instr : loop.latch->instructions) {
        if (instr->opcode == IROpcode::ADD &&
            instr->src1.kind == IROperand::Kind::VARIABLE &&
            instr->src2.kind == IROperand::Kind::LITERAL &&
            instr->src2.intValue > 0) {
            
            loop.inductionVar = &instr->src1;
            loop.stepValue = instr->src2.intValue;
            loop.isCounted = true;
            return true;
        }
    }
    
    return false;
}

void LoopOptimizer::optimizeCountedLoop(IRFunction* func, LoopInfo& loop) {
    if (!loop.isCounted) return;
    
    // Для counted loop: if (i < N) → можно заменить на цикл с счетчиком
    // Проверяем условие в header
    for (auto& instr : loop.header->instructions) {
        if (instr->opcode == IROpcode::CMP_LT || instr->opcode == IROpcode::CMP_LE) {
            if (instr->src1.kind == IROperand::Kind::VARIABLE &&
                instr->src2.kind == IROperand::Kind::LITERAL) {
                loop.endValue = instr->src2.intValue;
                // Оптимизация для counted loop уже при генерации кода
                break;
            }
        }
    }
}

void LoopOptimizer::minimizeJumps(IRFunction* func, LoopInfo& loop) {
    // Убираем лишний jump в конце latch если за ним следует header
    if (!loop.latch->instructions.empty()) {
        auto& lastInstr = loop.latch->instructions.back();
        if (lastInstr->opcode == IROpcode::JUMP && 
            lastInstr->src1.name == loop.header->name) {
            // Проверяем, можно ли инвертировать условие
            invertLoopCondition(func, loop);
        }
    }
}

void LoopOptimizer::invertLoopCondition(IRFunction* func, LoopInfo& loop) {
    // Находим условие в header
    for (auto& instr : loop.header->instructions) {
        if (instr->opcode == IROpcode::JUMP_IF_NOT) {
            // Инвертируем: JUMP_IF_NOT → JUMP_IF (меняем exit и body местами)
            // Делаем тело цикла fall-through, выход по условию
            std::string exitName = instr->src2.name;
            
            // Создаем новую инструкцию
            instr->opcode = IROpcode::JUMP_IF;
            // Теперь прыгаем на выход если условие истинно
            // (было: прыгаем на выход если условие ложно)
            
            break;
        }
    }
}

} // namespace minicompiler