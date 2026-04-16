#include "ir/SSABuilder.hpp"
#include <algorithm>
#include <queue>
#include <iostream>
#include <functional>
#include <sstream>

namespace minicompiler {

SSABuilder::SSABuilder() {}

std::unique_ptr<SSAProgram> SSABuilder::buildSSA(IRProgram& irProgram) {
    ssaProgram = std::make_unique<SSAProgram>();
    
    // Преобразуем каждую функцию
    for (auto& func : irProgram.functions) {
        // Создаем SSA функцию
        auto ssaFunc = ssaProgram->createFunction(func->name, func->returnType);
        
        // Копируем параметры
        for (const auto& param : func->parameters) {
            ssaFunc->parameters.push_back({param.name, param.type});
        }
        
        // Копируем базовые блоки
        for (auto& block : func->blocks) {
            ssaFunc->createBlock(block->name);
        }
        
        // Копируем связи между блоками
        for (auto& block : func->blocks) {
            auto ssaBlock = ssaFunc->getBlock(block->name);
            for (auto succ : block->successors) {
                auto ssaSucc = ssaFunc->getBlock(succ->name);
                if (ssaSucc) {
                    ssaBlock->successors.push_back(ssaSucc);
                    ssaSucc->predecessors.push_back(ssaBlock);
                }
            }
        }
        
        // Просто копируем инструкции с переименованием переменных
        std::unordered_map<std::string, int> varCounter;
        std::unordered_map<std::string, std::string> currentVar;
        
        // Инициализация параметров
        for (const auto& param : func->parameters) {
            std::string varName = param.name;
            varCounter[varName] = 0;
            currentVar[varName] = varName + "0";
        }
        
        // Копируем инструкции с SSA-переименованием
        for (auto& block : func->blocks) {
            auto ssaBlock = ssaFunc->getBlock(block->name);
            
            for (auto& instr : block->instructions) {
                // Пропускаем LABEL инструкции
                if (instr->opcode == IROpcode::LABEL) {
                    continue;
                }
                
                // Конвертируем операнды с переименованием
                SSAValue* dest = nullptr;
                SSAValue* src1 = nullptr;
                SSAValue* src2 = nullptr;
                
                // Обрабатываем dest
                if (instr->dest.kind != IROperand::Kind::TEMP || !instr->dest.name.empty()) {
                    std::string destName = instr->dest.name;
                    if (instr->dest.kind == IROperand::Kind::VARIABLE) {
                        // Увеличиваем версию переменной
                        int version = varCounter[destName]++;
                        destName = destName + std::to_string(version);
                        currentVar[instr->dest.name] = destName;
                        dest = SSAValue::createVariable(destName, 0, instr->dest.type);
                    } else if (instr->dest.kind == IROperand::Kind::TEMP) {
                        dest = SSAValue::createTemp(0, instr->dest.type);
                        dest->name = destName;
                    }
                }
                
                // Обрабатываем src1
                if (instr->src1.kind != IROperand::Kind::TEMP || !instr->src1.name.empty()) {
                    src1 = convertOperandWithRename(instr->src1, currentVar);
                }
                
                // Обрабатываем src2
                if (instr->src2.kind != IROperand::Kind::TEMP || !instr->src2.name.empty()) {
                    src2 = convertOperandWithRename(instr->src2, currentVar);
                }
                
                auto ssaInstr = std::make_unique<SSAInstruction>(
                    instr->opcode, dest, src1, src2);
                ssaInstr->comment = instr->comment;
                ssaBlock->addInstruction(std::move(ssaInstr));
            }
        }
    }
    
    return std::move(ssaProgram);
}

SSAValue* SSABuilder::convertOperandWithRename(const IROperand& op, 
                                                const std::unordered_map<std::string, std::string>& currentVar) {
    switch (op.kind) {
        case IROperand::Kind::LITERAL:
            if (op.type == IRType::INT) return SSAValue::createConstant(op.intValue);
            if (op.type == IRType::FLOAT) return SSAValue::createConstant(op.floatValue);
            if (op.type == IRType::BOOL) return SSAValue::createConstant(op.boolValue);
            if (op.type == IRType::STRING) {
                auto val = SSAValue::createConstant(0);
                val->type = IRType::STRING;
                val->stringValue = op.stringValue;
                return val;
            }
            break;
        case IROperand::Kind::VARIABLE: {
            auto it = currentVar.find(op.name);
            std::string ssaName = (it != currentVar.end()) ? it->second : (op.name + "0");
            return SSAValue::createVariable(ssaName, 0, op.type);
        }
        case IROperand::Kind::TEMP: {
            auto val = SSAValue::createTemp(0, op.type);
            val->name = op.name;
            return val;
        }
        case IROperand::Kind::LABEL: {
            auto val = SSAValue::createTemp(0, IRType::LABEL);
            val->name = op.name;
            return val;
        }
        default:
            break;
    }
    return nullptr;
}

SSAValue* SSABuilder::convertOperand(const IROperand& op) {
    static std::unordered_map<std::string, std::string> emptyMap;
    return convertOperandWithRename(op, emptyMap);
}

// Заглушки для остальных методов
SSABuilder::DomInfo SSABuilder::computeDominators(SSAFunction* func) {
    (void)func;
    return DomInfo();
}

void SSABuilder::computeDominanceFrontier(SSAFunction* func, DomInfo& info) {
    (void)func;
    (void)info;
}

std::vector<SSABasicBlock*> SSABuilder::computeReversePostOrder(SSABasicBlock* entry) {
    (void)entry;
    return {};
}

void SSABuilder::buildDominatorTree(SSAFunction* func, const DomInfo& info) {
    (void)func;
    (void)info;
}

std::unordered_map<std::string, SSABuilder::VarInfo> 
SSABuilder::collectDefSites(SSAFunction* func) {
    (void)func;
    return {};
}

void SSABuilder::placePhiFunctions(SSAFunction* func, const DomInfo& domInfo) {
    (void)func;
    (void)domInfo;
}

void SSABuilder::renameVariables(SSAFunction* func, const DomInfo& domInfo) {
    (void)func;
    (void)domInfo;
}

void SSABuilder::renameBlock(SSABasicBlock* block, RenameState& state) {
    (void)block;
    (void)state;
}

SSAValue* SSABuilder::createNewSSAValue(const std::string& name, IRType type, RenameState& state) {
    (void)name;
    (void)type;
    (void)state;
    return nullptr;
}

void SSABuilder::optimizeConstantsSSA(SSAFunction* func) {
    (void)func;
}

bool SSABuilder::foldConstant(SSAInstruction* instr) {
    (void)instr;
    return false;
}

void SSABuilder::eliminateDeadCodeSSA(SSAFunction* func) {
    (void)func;
}

SSABasicBlock* SSABuilder::findSSABlock(SSAFunction* func, const std::string& name) {
    return func->getBlock(name);
}

} // namespace minicompiler