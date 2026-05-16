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
            if (instr->opcode == IROpcode::CALL || instr->opcode == IROpcode::ALLOCA) {
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
        stackSlots[paramName] = {paramName, -(offset + redZoneOffset), 8, func.parameters[i].type};
        if (i < MAX_PARAM_REGS) {
            varToReg[paramName] = PARAM_REGS[i];
        }
    }
    
    // Локальные переменные (MOVE)
    for (auto& block : func.blocks) {
        for (auto& instr : block->instructions) {
            if (instr->opcode == IROpcode::MOVE && 
                instr->dest.kind == IROperand::Kind::VARIABLE) {
                std::string varName = instr->dest.name;
                if (stackSlots.find(varName) != stackSlots.end()) continue;
                offset = alignTo(offset, 8);
                offset += 8;
                IRType slotType = instr->dest.type;
                stackSlots[varName] = {varName, -(offset + redZoneOffset), 8, slotType};
            }
        }
    }
    
    // Локальные переменные (ALLOCA — массивы)
    for (auto& block : func.blocks) {
        for (auto& instr : block->instructions) {
            if (instr->opcode == IROpcode::ALLOCA && 
                instr->dest.kind == IROperand::Kind::VARIABLE) {
                std::string varName = instr->dest.name;
                if (stackSlots.find(varName) == stackSlots.end()) {
                    offset = alignTo(offset, 8);
                    offset += 8;
                    stackSlots[varName] = {varName, -(offset + redZoneOffset), 8, IRType::INT};
                }
            }
        }
    }
    
    // Слоты для временного хранения при работе с массивами
    bool needsArrayVal = false;
    for (auto& block : func.blocks) {
        for (auto& instr : block->instructions) {
            if (instr->opcode == IROpcode::STORE) {
                needsArrayVal = true;
                break;
            }
        }
        if (needsArrayVal) break;
    }
    if (needsArrayVal) {
        if (stackSlots.find("__array_val") == stackSlots.end()) {
            offset = alignTo(offset, 8);
            offset += 8;
            stackSlots["__array_val"] = {"__array_val", -(offset + redZoneOffset), 8, IRType::INT};
        }
        if (stackSlots.find("__array_addr") == stackSlots.end()) {
            offset = alignTo(offset, 8);
            offset += 8;
            stackSlots["__array_addr"] = {"__array_addr", -(offset + redZoneOffset), 8, IRType::INT};
        }
    }
    
    // Проверяем, есть ли ALLOCA в функции
    bool hasAlloca = false;
    for (auto& block : func.blocks) {
        for (auto& instr : block->instructions) {
            if (instr->opcode == IROpcode::ALLOCA) {
                hasAlloca = true;
                break;
            }
        }
        if (hasAlloca) break;
    }
    
    if (hasAlloca) {
        isLeafFunction = false;
        offset += 256;
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
        case IROpcode::NOT:
            emit("mov rax, " + getOperandString(instr.src1));
            emit("xor rax, 1", "; logical NOT");
            saveResultToDest(instr.dest);
            break;
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
        case IROpcode::COPY:
            if (instr.src1.type == IRType::INT && instr.dest.type == IRType::FLOAT) {
                std::string opStr = getOperandString(instr.src1);
                if (opStr.size() > 5 && opStr.substr(0, 5) == "qword") {
                    opStr = "dword" + opStr.substr(5);
                }
                emit("cvtsi2ss xmm0, " + opStr, "; int to float");
                saveFloatResult(instr.dest);
            } else if (instr.src1.type == IRType::FLOAT && instr.dest.type == IRType::INT) {
                emit("cvttss2si rax, " + getOperandString(instr.src1), "; float to int");
                saveResultToDest(instr.dest);
            } else {
                emit("mov rax, " + getOperandString(instr.src1));
                saveResultToDest(instr.dest);
            }
            break;
        case IROpcode::ALLOCA:
            emit("sub rsp, " + std::to_string(instr.src1.intValue), "; alloca " + instr.dest.name);
            emit("mov " + getStackSlot(instr.dest.name) + ", rsp", "; save array pointer");
            break;
        case IROpcode::LOAD:
            emit("mov rax, " + getOperandString(instr.src1), "; load address");
            emit("mov rax, [rax]", "; load value from memory");
            saveResultToDest(instr.dest);
            break;
        case IROpcode::STORE:
            emit("mov rax, " + getOperandString(instr.src1), "; value to store");
            emit("mov rbx, " + getOperandString(instr.dest), "; destination address");
            emit("mov [rbx], rax", "; store to memory");
            break;
        default: break;
    }
}

void X86Generator::emitBinaryOp(IROpcode op, const IROperand& dest,
                                 const IROperand& src1, const IROperand& src2) {
    if (dest.type == IRType::FLOAT || src1.type == IRType::FLOAT) {
        if (src1.kind == IROperand::Kind::LITERAL && src1.type == IRType::FLOAT) {
            emit("mov eax, __float32__(" + std::to_string(src1.floatValue) + ")", "; float constant");
            emit("movd xmm0, eax", "; move to xmm");
        } else if (src1.type == IRType::INT) {
            std::string opStr = getOperandString(src1);
            if (opStr.size() > 5 && opStr.substr(0, 5) == "qword") {
                opStr = "dword" + opStr.substr(5);
            }
            emit("cvtsi2ss xmm0, " + opStr);
        } else {
            std::string opStr = getOperandString(src1);
            if (opStr[0] == 'q') {
                emit("movq xmm0, " + opStr, "; load temp as float");
            } else {
                emit("movss xmm0, " + opStr, "; load float");
            }
        }
        
        if (src2.kind == IROperand::Kind::LITERAL && src2.type == IRType::FLOAT) {
            emit("mov eax, __float32__(" + std::to_string(src2.floatValue) + ")", "; float constant");
            emit("movd xmm1, eax", "; move to xmm");
        } else if (src2.type == IRType::INT) {
            std::string opStr = getOperandString(src2);
            if (opStr.size() > 5 && opStr.substr(0, 5) == "qword") {
                opStr = "dword" + opStr.substr(5);
            }
            emit("cvtsi2ss xmm1, " + opStr);
        } else {
            std::string opStr = getOperandString(src2);
            if (opStr[0] == 'q') {
                emit("movq xmm1, " + opStr, "; load temp as float");
            } else {
                emit("movss xmm1, " + opStr, "; load float");
            }
        }
        
        switch (op) {
            case IROpcode::ADD: emit("addss xmm0, xmm1", "; float add"); break;
            case IROpcode::SUB: emit("subss xmm0, xmm1", "; float sub"); break;
            case IROpcode::MUL: emit("mulss xmm0, xmm1", "; float mul"); break;
            case IROpcode::DIV: emit("divss xmm0, xmm1", "; float div"); break;
            default: break;
        }
        
        saveFloatResult(dest);
        return;
    }
    
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
    // Float деление
    if (dest.type == IRType::FLOAT || src1.type == IRType::FLOAT) {
        // Загружаем операнды как float
        if (src1.kind == IROperand::Kind::LITERAL && src1.type == IRType::FLOAT) {
            emit("mov eax, __float32__(" + std::to_string(src1.floatValue) + ")");
            emit("movd xmm0, eax");
        } else if (src1.type == IRType::INT) {
            std::string opStr = getOperandString(src1);
            if (opStr.size() > 5 && opStr.substr(0, 5) == "qword") {
                opStr = "dword" + opStr.substr(5);
            }
            emit("cvtsi2ss xmm0, " + opStr);
        } else {
            std::string opStr = getOperandString(src1);
            if (opStr[0] == 'q') {
                emit("movq xmm0, " + opStr);
            } else {
                emit("movss xmm0, " + opStr);
            }
        }
        
        if (src2.kind == IROperand::Kind::LITERAL && src2.type == IRType::FLOAT) {
            emit("mov eax, __float32__(" + std::to_string(src2.floatValue) + ")");
            emit("movd xmm1, eax");
        } else if (src2.type == IRType::INT) {
            std::string opStr = getOperandString(src2);
            if (opStr.size() > 5 && opStr.substr(0, 5) == "qword") {
                opStr = "dword" + opStr.substr(5);
            }
            emit("cvtsi2ss xmm1, " + opStr);
        } else {
            std::string opStr = getOperandString(src2);
            if (opStr[0] == 'q') {
                emit("movq xmm1, " + opStr);
            } else {
                emit("movss xmm1, " + opStr);
            }
        }
        
        emit("divss xmm0, xmm1", "; float div");
        saveFloatResult(dest);
        return;
    }
    
    // Integer деление (существующий код)
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
    if (src1.type == IRType::FLOAT || src2.type == IRType::FLOAT) {
        if (src1.kind == IROperand::Kind::LITERAL && src1.type == IRType::FLOAT) {
            emit("mov eax, __float32__(" + std::to_string(src1.floatValue) + ")");
            emit("movd xmm0, eax");
        } else if (src1.type == IRType::INT) {
            std::string opStr = getOperandString(src1);
            if (opStr.size() > 5 && opStr.substr(0, 5) == "qword") {
                opStr = "dword" + opStr.substr(5);
            }
            emit("cvtsi2ss xmm0, " + opStr, "; int to float");
        } else {
            std::string opStr = getOperandString(src1);
            if (opStr[0] == 'q') {
                emit("movq xmm0, " + opStr);
            } else {
                emit("movss xmm0, " + opStr);
            }
        }
        
        if (src2.kind == IROperand::Kind::LITERAL && src2.type == IRType::FLOAT) {
            emit("mov eax, __float32__(" + std::to_string(src2.floatValue) + ")");
            emit("movd xmm1, eax");
        } else if (src2.type == IRType::INT) {
            std::string opStr = getOperandString(src2);
            if (opStr.size() > 5 && opStr.substr(0, 5) == "qword") {
                opStr = "dword" + opStr.substr(5);
            }
            emit("cvtsi2ss xmm1, " + opStr, "; int to float");
        } else {
            std::string opStr = getOperandString(src2);
            if (opStr[0] == 'q') {
                emit("movq xmm1, " + opStr);
            } else {
                emit("movss xmm1, " + opStr);
            }
        }
        
        emit("ucomiss xmm0, xmm1", "; float compare");
        
        switch (op) {
            case IROpcode::CMP_EQ: emit("sete al"); emit("setnp cl"); emit("and al, cl"); break;
            case IROpcode::CMP_NE: emit("setne al"); emit("setp cl"); emit("or al, cl"); break;
            case IROpcode::CMP_LT: emit("setb al"); break;
            case IROpcode::CMP_LE: emit("setbe al"); break;
            case IROpcode::CMP_GT: emit("seta al"); break;
            case IROpcode::CMP_GE: emit("setae al"); break;
            default: break;
        }
        
        emit("movzx rax, al");
        saveResultToDest(dest);
        return;
    }
    
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
        
        // Float dest
        if (destStr[0] == 'd') {
            if (src.kind == IROperand::Kind::LITERAL && src.type == IRType::FLOAT) {
                emit("mov eax, __float32__(" + std::to_string(src.floatValue) + ")", "; float constant");
                emit("movd xmm0, eax");
            } else if (src.type == IRType::INT) {
                std::string opStr = getOperandString(src);
                if (opStr.size() > 5 && opStr.substr(0, 5) == "qword") {
                    opStr = "dword" + opStr.substr(5);
                }
                emit("cvtsi2ss xmm0, " + opStr, "; int to float");
            } else {
                std::string srcStr = getOperandString(src);
                if (srcStr[0] == 'q') {
                    emit("movq xmm0, " + srcStr, "; load temp as float");
                } else {
                    emit("movss xmm0, " + srcStr, "; load float");
                }
            }
            emit("movss " + destStr + ", xmm0", "; save float");
            return;
        }
        
        // Integer dest
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

void X86Generator::saveFloatResult(const IROperand& dest) {
    if (dest.kind == IROperand::Kind::VARIABLE) {
        std::string slot = getStackSlot(dest.name);
        emit("movss " + slot + ", xmm0", "; save float to " + dest.name);
    } else if (dest.kind == IROperand::Kind::TEMP) {
        emit("movq " + getStackSlot("__tmp") + ", xmm0", "; save float temp");
    }
}

std::string X86Generator::getFloatOperand(const IROperand& op) {
    return getOperandString(op);
}

std::string X86Generator::getOperandString(const IROperand& op) {
    switch (op.kind) {
        case IROperand::Kind::LITERAL:
            if (op.type == IRType::FLOAT) {
                return "__float32__(" + std::to_string(op.floatValue) + ")";
            }
            return std::to_string(op.intValue);
        case IROperand::Kind::VARIABLE:
            return getStackSlot(op.name);
        case IROperand::Kind::TEMP:
            return getStackSlot("__tmp");
        case IROperand::Kind::LABEL:
            return op.name;
    }
    return "0";
}

std::string X86Generator::getStackSlot(const std::string& varName) {
    auto it = stackSlots.find(varName);
    if (it != stackSlots.end()) {
        if (it->second.type == IRType::FLOAT) {
            return "dword [rbp" + std::to_string(it->second.offset) + "]";
        }
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