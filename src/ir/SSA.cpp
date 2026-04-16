#include "ir/SSA.hpp"
#include <sstream>

namespace minicompiler {

// ============================================================================
// SSAValue implementation
// ============================================================================

std::string SSAValue::toString() const {
    switch (kind) {
        case Kind::CONSTANT:
            if (type == IRType::INT) return std::to_string(intValue);
            if (type == IRType::FLOAT) return std::to_string(floatValue);
            if (type == IRType::BOOL) return boolValue ? "true" : "false";
            if (type == IRType::STRING) return "\"" + stringValue + "\"";
            return "?";
        case Kind::VARIABLE:
            return name + std::to_string(version);
        case Kind::TEMPORARY:
            return name;
        case Kind::PHI:
            return "φ";
    }
    return "?";
}

SSAValue* SSAValue::createConstant(int value) {
    SSAValue* v = new SSAValue();
    v->kind = Kind::CONSTANT;
    v->type = IRType::INT;
    v->intValue = value;
    return v;
}

SSAValue* SSAValue::createConstant(double value) {
    SSAValue* v = new SSAValue();
    v->kind = Kind::CONSTANT;
    v->type = IRType::FLOAT;
    v->floatValue = value;
    return v;
}

SSAValue* SSAValue::createConstant(bool value) {
    SSAValue* v = new SSAValue();
    v->kind = Kind::CONSTANT;
    v->type = IRType::BOOL;
    v->boolValue = value;
    return v;
}

SSAValue* SSAValue::createVariable(const std::string& name, int version, IRType type) {
    SSAValue* v = new SSAValue();
    v->kind = Kind::VARIABLE;
    v->name = name;
    v->version = version;
    v->type = type;
    return v;
}

SSAValue* SSAValue::createTemp(int id, IRType type) {
    SSAValue* v = new SSAValue();
    v->kind = Kind::TEMPORARY;
    v->name = "t" + std::to_string(id);
    v->type = type;
    return v;
}

SSAValue* SSAValue::createPhi(IRType type) {
    SSAValue* v = new SSAValue();
    v->kind = Kind::PHI;
    v->type = type;
    return v;
}

// ============================================================================
// SSAInstruction implementation
// ============================================================================

SSAInstruction::SSAInstruction(IROpcode op, SSAValue* d, SSAValue* s1, SSAValue* s2)
    : opcode(op), dest(d), src1(s1), src2(s2) {}

std::string SSAInstruction::toString() const {
    std::stringstream ss;
    
    if (opcode == IROpcode::LABEL) {
        return "";
    }
    
    ss << "  ";
    
    // Для jump инструкций
    if (opcode == IROpcode::JUMP) {
        ss << "JUMP ";
        if (src1) ss << src1->name;
        return ss.str();
    }
    
    if (opcode == IROpcode::JUMP_IF) {
        ss << "JUMP_IF ";
        if (src1) ss << src1->toString() << ", ";
        if (src2) ss << src2->name;
        return ss.str();
    }
    
    if (opcode == IROpcode::JUMP_IF_NOT) {
        ss << "JUMP_IF_NOT ";
        if (src1) ss << src1->toString() << ", ";
        if (src2) ss << src2->name;
        return ss.str();
    }
    
    if (opcode == IROpcode::RETURN) {
        ss << "RETURN";
        if (src1) ss << " " << src1->toString();
        return ss.str();
    }
    
    if (opcode == IROpcode::PARAM) {
        ss << "PARAM";
        if (src1) ss << " " << src1->toString();
        return ss.str();
    }
    
    if (opcode == IROpcode::CALL) {
        if (dest) ss << dest->toString() << " = ";
        ss << "CALL " << src1->name;
        return ss.str();
    }
    
    // Для обычных инструкций
    if (dest) ss << dest->toString() << " = ";
    ss << opcodeToString(opcode);
    if (src1) ss << " " << src1->toString();
    if (src2) ss << ", " << src2->toString();
    
    if (!comment.empty()) ss << "  # " << comment;
    return ss.str();
}

bool SSAInstruction::isControlFlow() const {
    return opcode == IROpcode::JUMP || 
           opcode == IROpcode::JUMP_IF || 
           opcode == IROpcode::JUMP_IF_NOT ||
           opcode == IROpcode::RETURN;
}

// ============================================================================
// SSABasicBlock implementation
// ============================================================================

void SSABasicBlock::addInstruction(std::unique_ptr<SSAInstruction> instr) {
    instructions.push_back(std::move(instr));
}

void SSABasicBlock::addPhiFunction(const std::string& varName, SSAValue* phi) {
    phiFunctions[varName] = phi;
}

std::string SSABasicBlock::toString() const {
    std::stringstream ss;
    ss << name << ":\n";
    
    // Сначала выводим φ-функции
    for (const auto& [varName, phi] : phiFunctions) {
        ss << "  " << varName << " = φ(";
        for (size_t i = 0; i < phi->phiOperands.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << phi->phiOperands[i].first->toString() 
               << ":" << phi->phiOperands[i].second->name;
        }
        ss << ")\n";
    }
    
    // Затем обычные инструкции (пропускаем LABEL)
    for (const auto& instr : instructions) {
        if (instr->opcode != IROpcode::LABEL && instr->opcode != IROpcode::PHI) {
            std::string instrStr = instr->toString();
            if (!instrStr.empty()) {
                ss << instrStr << "\n";
            }
        }
    }
    return ss.str();
}

// ============================================================================
// SSAFunction implementation
// ============================================================================

SSABasicBlock* SSAFunction::createBlock(const std::string& name) {
    auto block = std::make_unique<SSABasicBlock>(name);
    auto ptr = block.get();
    blocks.push_back(std::move(block));
    blockMap[name] = ptr;
    return ptr;
}

SSABasicBlock* SSAFunction::getBlock(const std::string& name) {
    auto it = blockMap.find(name);
    return it != blockMap.end() ? it->second : nullptr;
}

int SSAFunction::getNextVersion(const std::string& varName) {
    return varVersions[varName]++;
}

void SSAFunction::pushVar(const std::string& varName, SSAValue* value) {
    varStacks[varName].push(value);
}

SSAValue* SSAFunction::getCurrentVar(const std::string& varName) {
    auto it = varStacks.find(varName);
    if (it != varStacks.end() && !it->second.empty()) {
        return it->second.top();
    }
    return nullptr;
}

void SSAFunction::popVar(const std::string& varName) {
    auto it = varStacks.find(varName);
    if (it != varStacks.end() && !it->second.empty()) {
        it->second.pop();
    }
}

std::string SSAFunction::toString() const {
    std::stringstream ss;
    ss << "function " << name << ": ";
    
    switch (returnType) {
        case IRType::INT: ss << "int"; break;
        case IRType::FLOAT: ss << "float"; break;
        case IRType::BOOL: ss << "bool"; break;
        case IRType::VOID: ss << "void"; break;
        default: ss << "?";
    }
    
    ss << " (";
    for (size_t i = 0; i < parameters.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << parameters[i].first;
    }
    ss << ")\n";
    
    for (const auto& block : blocks) {
        ss << "\n" << block->toString();
    }
    
    return ss.str();
}

// ============================================================================
// SSAProgram implementation
// ============================================================================

SSAFunction* SSAProgram::createFunction(const std::string& name, IRType returnType) {
    auto func = std::make_unique<SSAFunction>(name, returnType);
    auto ptr = func.get();
    functions.push_back(std::move(func));
    functionMap[name] = ptr;
    return ptr;
}

SSAFunction* SSAProgram::getFunction(const std::string& name) {
    auto it = functionMap.find(name);
    return it != functionMap.end() ? it->second : nullptr;
}

std::string SSAProgram::toString() const {
    std::stringstream ss;
    ss << "# MiniLang SSA IR Program\n\n";
    for (const auto& func : functions) {
        ss << func->toString() << "\n";
    }
    return ss.str();
}

} // namespace minicompiler