#include "ir/Optimizer.hpp"
#include <algorithm>
#include <iostream>
#include <unordered_set>

namespace minicompiler {

IROptimizer::IROptimizer() {}

void IROptimizer::optimize(IRProgram& program) {
    stats = Stats{};
    
    for (auto& func : program.functions) {
        stats.totalBefore += countInstructions(func.get());
        
        bool changed = true;
        int iterations = 0;
        
        // Итеративно применяем оптимизации пока есть изменения
        while (changed && iterations < 10) {
            changed = false;
            iterations++;
            
            if (doConstantFolding) {
                changed |= algebraicSimplification(func.get());
                changed |= foldConstants(func.get());
            }
            if (doConstantPropagation) {
                changed |= propagateConstants(func.get());
            }
            if (doDeadCodeElim) {
                changed |= eliminateDeadCode(func.get());
            }
        }
        
        stats.totalAfter += countInstructions(func.get());
    }
}

bool IROptimizer::foldConstants(IRFunction* func) {
    bool changed = false;
    
    for (auto& block : func->blocks) {
        for (auto& instr : block->instructions) {
            // Пропускаем не-арифметические инструкции и не-сравнения
            if (instr->opcode != IROpcode::ADD &&
                instr->opcode != IROpcode::SUB &&
                instr->opcode != IROpcode::MUL &&
                instr->opcode != IROpcode::DIV &&
                instr->opcode != IROpcode::MOD &&
                instr->opcode != IROpcode::AND &&
                instr->opcode != IROpcode::OR  && 
                instr->opcode != IROpcode::XOR &&
                instr->opcode != IROpcode::CMP_EQ && 
                instr->opcode != IROpcode::CMP_NE &&
                instr->opcode != IROpcode::CMP_LT && 
                instr->opcode != IROpcode::CMP_LE &&
                instr->opcode != IROpcode::CMP_GT && 
                instr->opcode != IROpcode::CMP_GE) {
                continue;
            }
            
            // Пропускаем если операнды — переменные (указатели на массивы)
            if (instr->src1.kind == IROperand::Kind::VARIABLE ||
                instr->src2.kind == IROperand::Kind::VARIABLE) {
                continue;
            }
            
            // Пропускаем float операции
            if (instr->src1.type == IRType::FLOAT || instr->src2.type == IRType::FLOAT ||
                instr->dest.type == IRType::FLOAT) {
                continue;
            }
            
            // Проверяем оба операнда на константность
            if (isConstant(instr->src1) && isConstant(instr->src2)) {
                int left = instr->src1.intValue;
                int right = instr->src2.intValue;
                
                int result = evaluateConstant(instr->opcode, left, right);
                
                // Заменяем инструкцию на MOVE константы
                instr->opcode = IROpcode::MOVE;
                instr->src1 = IROperand::literal(result);
                instr->src2 = IROperand();
                instr->comment = "folded constant";
                
                stats.foldedConstants++;
                changed = true;
            }
        }
    }
    
    return changed;
}

bool IROptimizer::propagateConstants(IRFunction* func) {
    bool changed = false;
    
    // Для каждого базового блока отдельно
    for (auto& block : func->blocks) {
        std::unordered_map<std::string, int> blockConsts;
        std::unordered_set<std::string> blockModified;
        
        // Первый проход: собираем константные присваивания в блоке
        for (auto& instr : block->instructions) {
            // MOVE константы в переменную или temp
            if (instr->opcode == IROpcode::MOVE && 
                instr->src1.kind == IROperand::Kind::LITERAL &&
                instr->src1.type == IRType::INT) {
                
                std::string name = instr->dest.name;
                // Только если ещё не изменялась в этом блоке
                if (blockModified.find(name) == blockModified.end()) {
                    blockConsts[name] = instr->src1.intValue;
                }
            }
            
            // Если переменная или temp используется как цель не-MOVE инструкции — она изменяется
            if (instr->dest.kind == IROperand::Kind::VARIABLE ||
                instr->dest.kind == IROperand::Kind::TEMP) {
                if (instr->opcode != IROpcode::MOVE ||
                    instr->src1.kind != IROperand::Kind::LITERAL) {
                    blockModified.insert(instr->dest.name);
                    blockConsts.erase(instr->dest.name);
                }
            }
        }
        
        // Второй проход: заменяем использования констант
        for (auto& instr : block->instructions) {
            if (instr->opcode == IROpcode::ALLOCA) continue;
            if (instr->src1.kind == IROperand::Kind::VARIABLE && instr->src1.type != IRType::INT) continue;
            if (instr->src2.kind == IROperand::Kind::VARIABLE && instr->src2.type != IRType::INT) continue;
            if (instr->opcode == IROpcode::STORE) continue;
            if (instr->opcode == IROpcode::LOAD) continue;
            if (instr->opcode == IROpcode::JUMP) continue;
            if (instr->opcode == IROpcode::JUMP_IF) continue;
            if (instr->opcode == IROpcode::JUMP_IF_NOT) continue;
            
            if (instr->opcode == IROpcode::CMP_EQ || instr->opcode == IROpcode::CMP_NE ||
                instr->opcode == IROpcode::CMP_LT || instr->opcode == IROpcode::CMP_LE ||
                instr->opcode == IROpcode::CMP_GT || instr->opcode == IROpcode::CMP_GE) continue;

            if (instr->src1.kind == IROperand::Kind::VARIABLE ||
                instr->src1.kind == IROperand::Kind::TEMP) {
                auto it = blockConsts.find(instr->src1.name);
                if (it != blockConsts.end() && 
                    blockModified.find(instr->src1.name) == blockModified.end()) {
                    instr->src1 = IROperand::literal(it->second);
                    stats.propagatedConstants++;
                    changed = true;
                }
            }
            
            if (instr->src2.kind == IROperand::Kind::VARIABLE ||
                instr->src2.kind == IROperand::Kind::TEMP) {
                auto it = blockConsts.find(instr->src2.name);
                if (it != blockConsts.end() && 
                    blockModified.find(instr->src2.name) == blockModified.end()) {
                    instr->src2 = IROperand::literal(it->second);
                    stats.propagatedConstants++;
                    changed = true;
                }
            }

        }
    }
    
    return changed;
}

bool IROptimizer::eliminateDeadCode(IRFunction* func) {
    bool changed = false;
    
    for (auto& block : func->blocks) {
        auto& instrs = block->instructions;
        
        for (auto it = instrs.begin(); it != instrs.end(); ) {
            bool removed = false;
            
            // Удаляем инструкции после RETURN в том же блоке
            if ((*it)->opcode == IROpcode::RETURN) {
                auto next = std::next(it);
                size_t removed_count = 0;
                while (next != instrs.end()) {
                    // НЕ удаляем LABEL (это цель перехода из другого блока)
                    if ((*next)->opcode == IROpcode::LABEL) break;
                    next = instrs.erase(next);
                    removed_count++;
                }
                if (removed_count > 0) {
                    stats.deadInstructions += removed_count;
                    changed = true;
                }
                break; // После RETURN в этом блоке больше нечего проверять
            }
            
            // Удаляем инструкции после безусловного JUMP (но перед LABEL)
            if ((*it)->opcode == IROpcode::JUMP) {
                auto next = std::next(it);
                size_t removed_count = 0;
                while (next != instrs.end()) {
                    if ((*next)->opcode == IROpcode::LABEL) break;
                    if ((*next)->opcode == IROpcode::JUMP) break; // ещё один jump
                    next = instrs.erase(next);
                    removed_count++;
                }
                if (removed_count > 0) {
                    stats.deadInstructions += removed_count;
                    changed = true;
                }
                break;
            }
            
            ++it;
        }
    }
    
    return changed;
}

int IROptimizer::countInstructions(IRFunction* func) {
    int count = 0;
    for (auto& block : func->blocks) {
        count += block->instructions.size();
    }
    return count;
}

bool IROptimizer::isConstant(const IROperand& op) const {
    return op.kind == IROperand::Kind::LITERAL;
}

int IROptimizer::evaluateConstant(IROpcode op, int left, int right) const {
    switch (op) {
        case IROpcode::ADD: return left + right;
        case IROpcode::SUB: return left - right;
        case IROpcode::MUL: return left * right;
        case IROpcode::DIV: return (right != 0) ? left / right : 0;
        case IROpcode::MOD: return (right != 0) ? left % right : 0;
        case IROpcode::AND: return left && right;
        case IROpcode::OR:  return left || right;
        case IROpcode::XOR: return (left != 0) ^ (right != 0);
        case IROpcode::CMP_EQ: return left == right;
        case IROpcode::CMP_NE: return left != right;
        case IROpcode::CMP_LT: return left < right;
        case IROpcode::CMP_LE: return left <= right;
        case IROpcode::CMP_GT: return left > right;
        case IROpcode::CMP_GE: return left >= right;
        default: return 0;
    }
}

bool IROptimizer::algebraicSimplification(IRFunction* func) {
    bool changed = false;
    
    for (auto& block : func->blocks) {
        for (auto& instr : block->instructions) {
            IROpcode op = instr->opcode;
            
            // НЕ упрощаем ADD/SUB, где один из операндов — переменная
            // (это может быть вычисление адреса массива)
            bool hasVariable = (instr->src1.kind == IROperand::Kind::VARIABLE ||
                               instr->src2.kind == IROperand::Kind::VARIABLE);
            
            // Правило: ADD x, 0 → x  (только если x не VARIABLE)
            if (op == IROpcode::ADD && !hasVariable && 
                instr->src2.isConstant() && instr->src2.intValue == 0) {
                instr->opcode = IROpcode::MOVE;
                instr->src2 = IROperand();
                instr->comment = "simplified: x + 0 = x";
                stats.algebraicSimplifications++;
                changed = true;
            }
            // Правило: SUB x, 0 → x  (только если x не VARIABLE)
            else if (op == IROpcode::SUB && !hasVariable && 
                     instr->src2.isConstant() && instr->src2.intValue == 0) {
                instr->opcode = IROpcode::MOVE;
                instr->src2 = IROperand();
                instr->comment = "simplified: x - 0 = x";
                stats.algebraicSimplifications++;
                changed = true;
            }
            // Правило: MUL x, 1 → x
            else if (op == IROpcode::MUL && 
                     instr->src2.isConstant() && instr->src2.intValue == 1) {
                instr->opcode = IROpcode::MOVE;
                instr->src2 = IROperand();
                instr->comment = "simplified: x * 1 = x";
                stats.algebraicSimplifications++;
                changed = true;
            }
            // Правило: MUL x, 0 → 0
            else if (op == IROpcode::MUL && 
                     instr->src2.isConstant() && instr->src2.intValue == 0) {
                instr->opcode = IROpcode::MOVE;
                instr->src1 = IROperand::literal(0);
                instr->src2 = IROperand();
                instr->comment = "simplified: x * 0 = 0";
                stats.algebraicSimplifications++;
                changed = true;
            }
            // Правило: ADD 0, x → x  (только если x не VARIABLE)
            else if (op == IROpcode::ADD && !hasVariable && 
                     instr->src1.isConstant() && instr->src1.intValue == 0) {
                instr->opcode = IROpcode::MOVE;
                instr->src1 = instr->src2;
                instr->src2 = IROperand();
                instr->comment = "simplified: 0 + x = x";
                stats.algebraicSimplifications++;
                changed = true;
            }
            // Правило: MUL 1, x → x
            else if (op == IROpcode::MUL && 
                     instr->src1.isConstant() && instr->src1.intValue == 1) {
                instr->opcode = IROpcode::MOVE;
                instr->src1 = instr->src2;
                instr->src2 = IROperand();
                instr->comment = "simplified: 1 * x = x";
                stats.algebraicSimplifications++;
                changed = true;
            }
        }
    }
    
    return changed;
}

} // namespace minicompiler