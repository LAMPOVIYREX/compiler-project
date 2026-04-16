#include "ir/IR.hpp"
#include <sstream>
#include <algorithm>
#include <iostream>

namespace minicompiler {

std::string opcodeToString(IROpcode op) {
    switch (op) {
        case IROpcode::ADD: return "ADD";
        case IROpcode::SUB: return "SUB";
        case IROpcode::MUL: return "MUL";
        case IROpcode::DIV: return "DIV";
        case IROpcode::MOD: return "MOD";
        case IROpcode::NEG: return "NEG";
        case IROpcode::AND: return "AND";
        case IROpcode::OR: return "OR";
        case IROpcode::NOT: return "NOT";
        case IROpcode::XOR: return "XOR";
        case IROpcode::CMP_EQ: return "CMP_EQ";
        case IROpcode::CMP_NE: return "CMP_NE";
        case IROpcode::CMP_LT: return "CMP_LT";
        case IROpcode::CMP_LE: return "CMP_LE";
        case IROpcode::CMP_GT: return "CMP_GT";
        case IROpcode::CMP_GE: return "CMP_GE";
        case IROpcode::LOAD: return "LOAD";
        case IROpcode::STORE: return "STORE";
        case IROpcode::ALLOCA: return "ALLOCA";
        case IROpcode::JUMP: return "JUMP";
        case IROpcode::JUMP_IF: return "JUMP_IF";
        case IROpcode::JUMP_IF_NOT: return "JUMP_IF_NOT";
        case IROpcode::LABEL: return "LABEL";
        case IROpcode::PHI: return "PHI";
        case IROpcode::CALL: return "CALL";
        case IROpcode::RETURN: return "RETURN";
        case IROpcode::PARAM: return "PARAM";
        case IROpcode::MOVE: return "MOVE";
        case IROpcode::COPY: return "COPY";
    }
    return "UNKNOWN";
}

//=============================================================================
// IRInstruction
//=============================================================================

IRInstruction::IRInstruction(IROpcode op, const IROperand& dest,
                             const IROperand& src1, const IROperand& src2)
    : opcode(op), dest(dest), src1(src1), src2(src2) {}

std::string IRInstruction::toString() const {
    std::stringstream ss;
    
    if (opcode == IROpcode::LABEL) {
        return "";  // Пропускаем LABEL, так как имя блока уже выводится
    }
    
    ss << "  ";
    
    // Для инструкций без результата (JUMP, RETURN и т.д.)
    if (opcode == IROpcode::JUMP || opcode == IROpcode::JUMP_IF || 
        opcode == IROpcode::JUMP_IF_NOT || opcode == IROpcode::RETURN ||
        opcode == IROpcode::PARAM || opcode == IROpcode::CALL) {
        
        ss << opcodeToString(opcode);
        
        if (opcode == IROpcode::CALL) {
            ss << " " << src1.toString();
        } else if (opcode == IROpcode::JUMP || opcode == IROpcode::JUMP_IF || 
                   opcode == IROpcode::JUMP_IF_NOT) {
            if (src1.kind != IROperand::Kind::TEMP || src1.name != "") {
                ss << " " << src1.toString();
            }
            if (src2.kind != IROperand::Kind::TEMP || src2.name != "") {
                ss << ", " << src2.toString();
            }
        } else if (opcode == IROpcode::PARAM) {
            if (src1.kind != IROperand::Kind::TEMP || src1.name != "") {
                ss << " " << src1.toString();
            }
        } else if (opcode == IROpcode::RETURN) {
            if (src1.kind != IROperand::Kind::TEMP || src1.name != "") {
                ss << " " << src1.toString();
            }
        }
    } 
    // Для инструкций с результатом
    else {
        ss << dest.toString() << " = " << opcodeToString(opcode);
        if (src1.kind != IROperand::Kind::TEMP || src1.name != "") {
            ss << " " << src1.toString();
        }
        if (src2.kind != IROperand::Kind::TEMP || src2.name != "") {
            ss << ", " << src2.toString();
        }
    }
    
    if (!comment.empty()) {
        ss << "  # " << comment;
    }
    
    return ss.str();
}

bool IRInstruction::isControlFlow() const {
    return opcode == IROpcode::JUMP || 
           opcode == IROpcode::JUMP_IF || 
           opcode == IROpcode::JUMP_IF_NOT ||
           opcode == IROpcode::RETURN;
}

bool IRInstruction::isTerminator() const {
    return isControlFlow();
}

//=============================================================================
// BasicBlock
//=============================================================================

BasicBlock::BasicBlock(const std::string& name) : name(name) {}

void BasicBlock::addInstruction(std::unique_ptr<IRInstruction> instr) {
    instructions.push_back(std::move(instr));
}

void BasicBlock::addSuccessor(BasicBlock* block) {
    if (std::find(successors.begin(), successors.end(), block) == successors.end()) {
        successors.push_back(block);
    }
}

void BasicBlock::addPredecessor(BasicBlock* block) {
    if (std::find(predecessors.begin(), predecessors.end(), block) == predecessors.end()) {
        predecessors.push_back(block);
    }
}

std::string BasicBlock::toString() const {
    std::stringstream ss;
    ss << name << ":\n";
    for (const auto& instr : instructions) {
        std::string instrStr = instr->toString();
        
        if (!instrStr.empty() && instrStr.find("LABEL") == std::string::npos) {
            ss << instrStr << "\n";
        }
    }
    return ss.str();
}

bool BasicBlock::isTerminated() const {
    if (instructions.empty()) return false;
    return instructions.back()->isTerminator();
}

//=============================================================================
// IRFunction
//=============================================================================

IRFunction::IRFunction(const std::string& name, IRType returnType)
    : name(name), returnType(returnType), tempCounter(0) {}

BasicBlock* IRFunction::createBlock(const std::string& name) {
    auto block = std::make_unique<BasicBlock>(name);
    auto ptr = block.get();
    blocks.push_back(std::move(block));
    blockMap[name] = ptr;
    return ptr;
}

BasicBlock* IRFunction::getBlock(const std::string& name) {
    auto it = blockMap.find(name);
    if (it != blockMap.end()) {
        return it->second;
    }
    return nullptr;
}

IROperand IRFunction::newTemp(IRType type) {
    return IROperand::temp(tempCounter++, type);
}

void IRFunction::addParameter(const IROperand& param) {
    parameters.push_back(param);
}

std::string IRFunction::toString() const {
    std::stringstream ss;
    ss << "function " << name << ": ";
    
    switch (returnType) {
        case IRType::INT: ss << "int"; break;
        case IRType::FLOAT: ss << "float"; break;
        case IRType::BOOL: ss << "bool"; break;
        case IRType::STRING: ss << "string"; break;
        case IRType::VOID: ss << "void"; break;
        default: ss << "unknown";
    }
    
    ss << " (";
    for (size_t i = 0; i < parameters.size(); i++) {
        if (i > 0) ss << ", ";
        ss << parameters[i].toString();
    }
    ss << ")\n";
    
    for (const auto& block : blocks) {
        ss << "\n" << block->toString();
    }
    
    return ss.str();
}

void IRFunction::verify() const {
    for (const auto& block : blocks) {
        if (!block->isTerminated() && block != blocks.back()) {
            std::cerr << "Warning: Block " << block->name 
                      << " is not terminated but not last block\n";
        }
    }
}

//=============================================================================
// IRProgram
//=============================================================================

IRProgram::IRProgram() {}

IRFunction* IRProgram::createFunction(const std::string& name, IRType returnType) {
    auto func = std::make_unique<IRFunction>(name, returnType);
    auto ptr = func.get();
    functions.push_back(std::move(func));
    functionMap[name] = ptr;
    return ptr;
}

IRFunction* IRProgram::getFunction(const std::string& name) {
    auto it = functionMap.find(name);
    if (it != functionMap.end()) {
        return it->second;
    }
    return nullptr;
}

void IRProgram::addFunction(std::unique_ptr<IRFunction> func) {
    functionMap[func->name] = func.get();
    functions.push_back(std::move(func));
}

std::string IRProgram::toString() const {
    std::stringstream ss;
    ss << "# MiniLang IR Program\n\n";
    for (const auto& func : functions) {
        ss << func->toString() << "\n";
    }
    return ss.str();
}

void IRProgram::verify() const {
    for (const auto& func : functions) {
        func->verify();
    }
}

} // namespace minicompiler