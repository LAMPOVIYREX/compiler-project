#include "codegen/X86Generator.hpp"
#include <algorithm>
#include <iostream>

namespace minicompiler {

constexpr const char* X86Generator::PARAM_REGS[];
constexpr const char* X86Generator::CALLER_SAVED[];
constexpr const char* X86Generator::CALLEE_SAVED[];

static int paramCounter = 0;

X86Generator::X86Generator(SymbolTable& symbolTable, ErrorReporter& errorReporter)
    : symbolTable(symbolTable), errorReporter(errorReporter) {}

std::string X86Generator::generate(IRProgram& irProgram) {
    output.str("");
    output.clear();
    
    emit("; ============================================");
    emit("; MiniLang Compiler - x86-64 Assembly Output");
    emit("; Target: Linux x86-64, System V AMD64 ABI");
    emit("; Assembler: NASM (nasm -f elf64)");
    emit("; ============================================");
    emitBlank();
    
    // Extern declarations
    externFunctions.clear();
    for (auto& func : irProgram.functions) {
        for (auto& block : func->blocks) {
            for (auto& instr : block->instructions) {
                if (instr->opcode == IROpcode::CALL) {
                    std::string callee = instr->src1.name;
                    bool found = false;
                    for (auto& f : irProgram.functions) {
                        if (f->name == callee) { found = true; break; }
                    }
                    if (!found) externFunctions.insert(callee);
                }
            }
        }
    }
    
    for (auto& ext : externFunctions) {
        emit("extern " + ext);
    }
    if (!externFunctions.empty()) emitBlank();
    
    emit("section .text");
    emitBlank();
    
    for (auto& func : irProgram.functions) {
        currentFunction = func.get();
        stackSlots.clear();
        varToReg.clear();
        paramCounter = 0;
        
        computeStackLayout(*func);
        generateFunctionHeader(*func);
        
        for (auto& block : func->blocks) {
            generateBasicBlock(*block);
        }
        
        generateFunctionFooter(*func);
        emitBlank();
    }
    
    // Секции данных
    if (!stringLiterals.empty()) {
        emitBlank();
        emit("section .rodata");
        for (size_t i = 0; i < stringLiterals.size(); i++) {
            emitLabel(".L.str" + std::to_string(i) + ":");
            emit("db " + stringLiterals[i] + ", 0");
        }
    }
    
    if (!globalVars.empty()) {
        emitBlank();
        emit("section .data");
        for (auto& gv : globalVars) {
            if (gv.isInitialized) {
                emitLabel(gv.name + ":");
                emit("dq " + gv.value);
            }
        }
    }
    
    return output.str();
}

void X86Generator::analyzeLeafFunction(IRFunction& func) {
    isLeafFunction = true;
    for (auto& block : func.blocks) {
        for (auto& instr : block->instructions) {
            if (instr->opcode == IROpcode::CALL) {
                isLeafFunction = false;
                return;
            }
        }
    }
}

void X86Generator::computeStackLayout(IRFunction& func) {
    analyzeLeafFunction(func);
    
    int offset = 0;
    int redZoneOffset = isLeafFunction ? RED_ZONE_SIZE : 0;
    
    // Временный слот
    offset = alignTo(offset, 8);
    offset += 8;
    stackSlots["__tmp"] = {"__tmp", -(offset + redZoneOffset), 8, IRType::INT};
    
    // Параметры
    for (size_t i = 0; i < func.parameters.size(); i++) {
        std::string paramName = func.parameters[i].name;
        offset = alignTo(offset, 8);
        offset += 8;
        stackSlots[paramName] = {paramName, -(offset + redZoneOffset), 8, IRType::INT};
        if (i < MAX_PARAM_REGS) {
            varToReg[paramName] = PARAM_REGS[i];
        }
    }
    
    // Локальные переменные - всегда 8 байт для простоты
    for (auto& block : func.blocks) {
        for (auto& instr : block->instructions) {
            if (instr->opcode == IROpcode::MOVE && 
                instr->dest.kind == IROperand::Kind::VARIABLE) {
                std::string varName = instr->dest.name;
                if (stackSlots.find(varName) != stackSlots.end()) continue;
                offset = alignTo(offset, 8);
                offset += 8;
                stackSlots[varName] = {varName, -(offset + redZoneOffset), 8, IRType::INT};
            }
        }
    }
    
    currentStackSize = isLeafFunction ? 0 : alignTo(offset + 8, STACK_ALIGNMENT);
}

void X86Generator::generateFunctionHeader(IRFunction& func) {
    emit("global " + func.name);
    emitBlank();
    emitLabel(func.name + ":");
    emitPrologue(func);
}

void X86Generator::emitPrologue(IRFunction& func) {
    emit("push rbp", "; save base pointer");
    emit("mov rbp, rsp", "; set new base pointer");
    
    if (isLeafFunction && currentStackSize <= RED_ZONE_SIZE) {
        emit("; leaf function using red zone");
    } else if (currentStackSize > 0) {
        emit("sub rsp, " + std::to_string(currentStackSize), "; allocate stack frame");
    }
    
    for (size_t i = 0; i < func.parameters.size(); i++) {
        auto& param = func.parameters[i];
        std::string slot = getStackSlot(param.name);
        
        if (i < MAX_PARAM_REGS) {
            emit("mov " + slot + ", " + PARAM_REGS[i], "; save param: " + param.name);
        } else {
            int stackOffset = 16 + (int)(i - MAX_PARAM_REGS) * 8;
            emit("mov rax, qword [rbp+" + std::to_string(stackOffset) + "]",
                 "; load param " + param.name);
            emit("mov " + slot + ", rax", "; save param");
        }
    }
    
    emitBlank();
}

void X86Generator::emitEpilogue(IRFunction& func) {
    (void)func;
    emitLabel(func.name + "_exit:");
    if (!isLeafFunction || currentStackSize > RED_ZONE_SIZE) {
        emit("mov rsp, rbp", "; restore stack pointer");
    }
    emit("pop rbp", "; restore base pointer");
    emit("ret", "; return to caller");
}

void X86Generator::generateFunctionFooter(IRFunction& func) {
    emitBlank();
    emitEpilogue(func);
}

void X86Generator::generateBasicBlock(BasicBlock& block) {
    if (block.name != "entry") {
        emitLabel(currentFunction->name + "_" + block.name + ":");
    }
    for (auto& instr : block.instructions) {
        generateInstruction(*instr);
    }
}

void X86Generator::generateInstruction(IRInstruction& instr) {
    switch (instr.opcode) {
        case IROpcode::ADD: case IROpcode::SUB:
        case IROpcode::MUL: case IROpcode::AND:
        case IROpcode::OR:  case IROpcode::XOR:
            emitBinaryOp(instr.opcode, instr.dest, instr.src1, instr.src2); break;
        case IROpcode::DIV: case IROpcode::MOD:
            emitDivMod(instr.opcode, instr.dest, instr.src1, instr.src2); break;
        case IROpcode::NEG: emitUnaryOp("neg", instr.dest, instr.src1); break;
        case IROpcode::NOT: emitUnaryOp("not", instr.dest, instr.src1); break;
        case IROpcode::CMP_EQ: case IROpcode::CMP_NE:
        case IROpcode::CMP_LT: case IROpcode::CMP_LE:
        case IROpcode::CMP_GT: case IROpcode::CMP_GE:
            genComparison(instr.opcode, instr.dest, instr.src1, instr.src2); break;
        case IROpcode::JUMP:
            emit("jmp " + currentFunction->name + "_" + instr.src1.name); break;
        case IROpcode::JUMP_IF:
            emit("cmp " + getOperandString(instr.src1) + ", 0");
            emit("jne " + currentFunction->name + "_" + instr.src2.name); break;
        case IROpcode::JUMP_IF_NOT:
            emit("cmp " + getOperandString(instr.src1) + ", 0");
            emit("je " + currentFunction->name + "_" + instr.src2.name); break;
        case IROpcode::MOVE: genMove(instr.dest, instr.src1); break;
        case IROpcode::CALL: genCall(instr.dest, instr.src1); break;
        case IROpcode::RETURN: genReturn(instr.src1); break;
        case IROpcode::PARAM: genParam(instr.src1); break;
        default: break;
    }
}

void X86Generator::emitBinaryOp(IROpcode op, const IROperand& dest,
                                 const IROperand& src1, const IROperand& src2) {
    emit("mov rax, " + getOperandString(src1), "; load left operand");
    switch (op) {
        case IROpcode::ADD: emit("add rax, " + getOperandString(src2), "; add"); break;
        case IROpcode::SUB: emit("sub rax, " + getOperandString(src2), "; sub"); break;
        case IROpcode::MUL: emit("imul rax, " + getOperandString(src2), "; mul"); break;
        case IROpcode::AND: emit("and rax, " + getOperandString(src2), "; and"); break;
        case IROpcode::OR:  emit("or rax, " + getOperandString(src2), "; or"); break;
        case IROpcode::XOR: emit("xor rax, " + getOperandString(src2), "; xor"); break;
        default: break;
    }
    saveResultToDest(dest);
}

void X86Generator::emitDivMod(IROpcode op, const IROperand& dest,
                               const IROperand& src1, const IROperand& src2) {
    emit("mov rax, " + getOperandString(src1));
    emit("xor rdx, rdx", "; clear rdx for div");
    if (src2.kind == IROperand::Kind::LITERAL) {
        emit("mov rbx, " + getOperandString(src2));
        emit("idiv rbx");
    } else {
        emit("idiv " + getOperandString(src2));
    }
    if (op == IROpcode::MOD) emit("mov rax, rdx", "; get remainder");
    saveResultToDest(dest);
}

void X86Generator::emitUnaryOp(const std::string& opcode, const IROperand& dest,
                                const IROperand& src) {
    emit("mov rax, " + getOperandString(src));
    emit(opcode + " rax");
    saveResultToDest(dest);
}

void X86Generator::genComparison(IROpcode op, const IROperand& dest,
                                  const IROperand& src1, const IROperand& src2) {
    emit("mov rax, " + getOperandString(src1));
    emit("cmp rax, " + getOperandString(src2));
    switch (op) {
        case IROpcode::CMP_EQ: emit("sete al"); break;
        case IROpcode::CMP_NE: emit("setne al"); break;
        case IROpcode::CMP_LT: emit("setl al"); break;
        case IROpcode::CMP_LE: emit("setle al"); break;
        case IROpcode::CMP_GT: emit("setg al"); break;
        case IROpcode::CMP_GE: emit("setge al"); break;
        default: break;
    }
    emit("movzx rax, al");
    saveResultToDest(dest);
}

void X86Generator::genMove(const IROperand& dest, const IROperand& src) {
    if (dest.kind == IROperand::Kind::VARIABLE) {
        std::string destStr = getStackSlot(dest.name);
        std::string srcStr = getOperandString(src);
        if (srcStr[0] == 'q' && destStr[0] == 'q') {
            emit("mov rax, " + srcStr);
            emit("mov " + destStr + ", rax");
        } else if (destStr != srcStr) {
            emit("mov " + destStr + ", " + srcStr);
        }
    }
}

void X86Generator::genReturn(const IROperand& value) {
    if (value.kind == IROperand::Kind::LITERAL)
        emit("mov rax, " + std::to_string(value.intValue));
    else if (value.kind == IROperand::Kind::VARIABLE)
        emit("mov rax, " + getStackSlot(value.name));
    else if (value.kind == IROperand::Kind::TEMP)
        emit("mov rax, " + getStackSlot("__tmp"));
    emit("jmp " + currentFunction->name + "_exit");
}

void X86Generator::genCall(const IROperand& dest, const IROperand& func) {
    (void)dest;
    emit("call " + func.name);
    if (paramCounter > MAX_PARAM_REGS) {
        int stackParams = paramCounter - MAX_PARAM_REGS;
        emit("add rsp, " + std::to_string(stackParams * 8), "; clean stack params");
    }
    emit("mov " + getStackSlot("__tmp") + ", rax", "; save call result");
    paramCounter = 0;
}

void X86Generator::genParam(const IROperand& value) {
    std::string valStr = getOperandString(value);
    if (paramCounter < MAX_PARAM_REGS)
        emit("mov " + std::string(PARAM_REGS[paramCounter]) + ", " + valStr,
             "; param " + std::to_string(paramCounter + 1));
    else
        emit("push " + valStr, "; param " + std::to_string(paramCounter + 1) + " on stack");
    paramCounter++;
}

void X86Generator::saveResultToDest(const IROperand& dest) {
    if (dest.kind == IROperand::Kind::VARIABLE)
        emit("mov " + getStackSlot(dest.name) + ", rax", "; save to " + dest.name);
    else if (dest.kind == IROperand::Kind::TEMP)
        emit("mov " + getStackSlot("__tmp") + ", rax", "; save temp");
}

std::string X86Generator::getOperandString(const IROperand& op) {
    switch (op.kind) {
        case IROperand::Kind::LITERAL: return std::to_string(op.intValue);
        case IROperand::Kind::VARIABLE: return getStackSlot(op.name);
        case IROperand::Kind::TEMP: return getStackSlot("__tmp");
        case IROperand::Kind::LABEL: return op.name;
    }
    return "0";
}

std::string X86Generator::getStackSlot(const std::string& varName) {
    auto it = stackSlots.find(varName);
    if (it != stackSlots.end()) {
        
        (void)it->second.size;  
        return "qword [rbp" + std::to_string(it->second.offset) + "]";
    }
    return "qword [rbp-8]";
}

std::string X86Generator::getRegister(int paramIndex) {
    return (paramIndex < MAX_PARAM_REGS) ? PARAM_REGS[paramIndex] : "rax";
}

std::string X86Generator::newLabel() { return ".LBB" + std::to_string(labelCounter++); }
std::string X86Generator::newStringLabel() { return ".L.str" + std::to_string(stringLabelCounter++); }

void X86Generator::emit(const std::string& line, const std::string& comment) {
    output << "    " << line;
    if (!comment.empty()) output << "    " << comment;
    output << "\n";
}
void X86Generator::emitLabel(const std::string& name) { output << name << "\n"; }
void X86Generator::emitBlank() { output << "\n"; }

int X86Generator::alignTo(int value, int alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

void X86Generator::genLoad(const IROperand& dest, const IROperand& src) { (void)dest; (void)src; }
void X86Generator::genStore(const IROperand& dest, const IROperand& src) { (void)dest; (void)src; }
void X86Generator::genJumpIf(const IROperand& cond, const IROperand& target, bool invert) {
    emit("cmp " + getOperandString(cond) + ", 0");
    emit(invert ? "je " : "jne " + currentFunction->name + "_" + target.name);
}

} // namespace minicompiler