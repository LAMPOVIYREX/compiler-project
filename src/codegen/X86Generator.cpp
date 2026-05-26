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
    
    // 1. Сначала section .text (NASM требует директиву в начале)
    output << "section .text\n\n";
    
    emit("; ============================================");
    emit("; MiniLang Compiler - x86-64 Assembly Output");
    emit("; Target: Linux x86-64, System V AMD64 ABI");
    emit("; Assembler: NASM (nasm -f elf64)");
    emit("; ============================================");
    emitBlank();
    
    // 2. Собираем строковые литералы
    rodataStrings.clear();
    for (auto& func : irProgram.functions) {
        for (auto& block : func->blocks) {
            for (auto& instr : block->instructions) {
                if (instr->opcode == IROpcode::PARAM && 
                    instr->src1.kind == IROperand::Kind::LITERAL &&
                    instr->src1.type == IRType::STRING) {
                    std::string str = instr->src1.stringValue;
                    if (str.size() >= 2 && str.front() == '"' && str.back() == '"') {
                        str = str.substr(1, str.size() - 2);
                    }
                    bool found = false;
                    for (auto& s : rodataStrings) {
                        if (s == str) { found = true; break; }
                    }
                    if (!found) rodataStrings.push_back(str);
                }
            }
        }
    }
    
    // ============ ЗАПОЛНЯЕМ ИМЕНА ГЛОБАЛЬНЫХ МАССИВОВ ДО ГЕНЕРАЦИИ ФУНКЦИЙ ============
    globalArrayNames.clear();
    for (auto& [name, size] : irProgram.globalArrays) {
        globalArrayNames.insert(name);
    }
 

    // 3. Определяем внешние функции
    externFunctions.clear();
    std::unordered_set<std::string> definedFuncs;
    for (auto& func : irProgram.functions) {
        // Только функции с телом считаются определёнными
        if (!func->blocks.empty()) {
            definedFuncs.insert(func->name);
        }
    }
    for (auto& func : irProgram.functions) {
        for (auto& block : func->blocks) {
            for (auto& instr : block->instructions) {
                if (instr->opcode == IROpcode::CALL) {
                    std::string callee = instr->src1.name;
                    if (definedFuncs.find(callee) == definedFuncs.end()) {
                        externFunctions[callee] = externFunctions.size();
                    }
                }
            }
        }
    }
    
    for (auto& [name, idx] : externFunctions) {
        std::cerr << name << " ";
    }
    std::cerr << std::endl;
    if (!externFunctions.empty()) emitBlank();
    
    // 4. Генерируем функции
    for (auto& func : irProgram.functions) {
        currentFunction = func.get();
        
        // extern функции без тела
        bool hasBody = false;
        for (auto& block : func->blocks) {
            if (!block->instructions.empty()) { hasBody = true; break; }
        }
        if (!hasBody) {
            emit("extern " + func->name);
            emitBlank();
            continue;
        }
        
        stackSlots.clear();
        paramCounter = 0;
        computeStackLayout(*func);
        generateFunctionHeader(*func);
        for (auto& block : func->blocks) {
            generateBasicBlock(*block);
        }
        generateFunctionFooter(*func);
        emitBlank();
    }
    
    // 5. Секция .rodata (после .text)
    if (!rodataStrings.empty()) {
        output << "section .rodata\n";
        for (size_t i = 0; i < rodataStrings.size(); i++) {
            emitLabel("L.str" + std::to_string(i) + ":");
            std::string escaped;
            for (size_t j = 0; j < rodataStrings[i].size(); j++) {
                char c = rodataStrings[i][j];
                if (c == '\\' && j + 1 < rodataStrings[i].size()) {
                    char next = rodataStrings[i][j + 1];
                    if (next == 'n') { escaped += "\\n"; j++; continue; }
                    if (next == 't') { escaped += "\\t"; j++; continue; }
                    if (next == 'r') { escaped += "\\r"; j++; continue; }
                    if (next == '"') { escaped += "\\\""; j++; continue; }
                    if (next == '\\') { escaped += "\\\\"; j++; continue; }
                }
                if (c == '\n') { escaped += "\\n"; continue; }
                if (c == '\t') { escaped += "\\t"; continue; }
                if (c == '\r') { escaped += "\\r"; continue; }
                if (c == '\"') { escaped += "\\\""; continue; }
                escaped += c;
            }
            emit("db `" + escaped + "`, 0");
        }
    }
    
    // 6. Секция .bss (после .rodata)
    if (!irProgram.globalArrays.empty()) {
        output << "\nsection .bss\n";
        for (auto& [name, size] : irProgram.globalArrays) {
            output << name << ": resq " << size << "\n";
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
    
    offset = alignTo(offset, 8);
    offset += 8;
    stackSlots["__tmp2"] = {"__tmp2", -(offset + redZoneOffset), 8, IRType::INT};
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

void X86Generator::generateExternCall(IRInstruction& instr) {
    std::string funcName = instr.src1.name;
        
    // Для variadic функций (printf, scanf) нужно установить AL = 0
    if (funcName == "printf" || funcName == "scanf" || funcName == "fprintf" ||
        funcName == "sprintf" || funcName == "snprintf") {
        emit("xor eax, eax", "; variadic: AL = 0 (no SSE registers used)");
    }
    
    // Вызываем функцию
    emit("call " + funcName);
    
    // Сохраняем результат в зависимости от возвращаемого типа
    // Определяем возвращаемый тип по IR-инструкции
    if (instr.dest.type == IRType::FLOAT) {
        // Float-функции возвращают результат в xmm0
        saveFloatResult(instr.dest);
    } else {
        // Int/pointer функции возвращают результат в rax
        emit("mov " + getStackSlot("__tmp") + ", rax", "; save call result");
    }
    paramCounter = 0;
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
        
        // ============================================================
        // CALL — проверяем внешняя функция или внутренняя
        // ============================================================
        case IROpcode::CALL:
            if (externFunctions.find(instr.src1.name) != externFunctions.end()) {
                generateExternCall(instr);
            } else {
                genCall(instr.dest, instr.src1);
            }
            break;
            
        case IROpcode::RETURN: genReturn(instr.src1); break;
        case IROpcode::PARAM: genParam(instr.src1); break;
        case IROpcode::COPY:
            if (instr.src1.type == IRType::INT && instr.dest.type == IRType::FLOAT) {
                std::string opStr = getOperandString(instr.src1);
                if (opStr.size() > 5 && opStr.substr(0, 5) == "qword") {
                    opStr = "dword" + opStr.substr(5);
                }
                if (instr.src1.kind == IROperand::Kind::LITERAL) {
                    emit("mov eax, " + opStr, "; constant to reg");
                    emit("cvtsi2ss xmm0, eax", "; int to float");
                } else {
                    emit("cvtsi2ss xmm0, " + opStr, "; int to float");
                }
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
            emitAddressLoad(instr.src1);                    
            emit("mov rax, [rax]", "; load value from memory");
            saveResultToDest(instr.dest);
            break;
        case IROpcode::STORE:
            // Значение сохраняем через __tmp2 (чтобы не перезаписать адрес)
            if (instr.src1.kind == IROperand::Kind::TEMP) {
                emit("mov rax, " + getStackSlot("__tmp2"), "; load value from tmp2");
            } else {
                emit("mov rax, " + getOperandString(instr.src1), "; value to store");
            }
            // Адрес загружаем из __tmp
            if (instr.dest.kind == IROperand::Kind::TEMP) {
                emit("mov rbx, " + getStackSlot("__tmp"), "; load address from tmp");
            } else if (instr.dest.kind == IROperand::Kind::VARIABLE && globalArrayNames.count(instr.dest.name)) {
                emit("lea rbx, [" + getOperandString(instr.dest) + "]", "; global array address");
            } else {
                emit("mov rbx, " + getOperandString(instr.dest), "; destination address");
            }
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
            if (src1.kind == IROperand::Kind::LITERAL) {
                emit("mov eax, " + opStr);
                emit("cvtsi2ss xmm0, eax");
            } else {
                emit("cvtsi2ss xmm0, " + opStr);
            }
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
            if (src2.kind == IROperand::Kind::LITERAL) {
                emit("mov eax, " + opStr);
                emit("cvtsi2ss xmm1, eax");
            } else {
                emit("cvtsi2ss xmm1, " + opStr);
            }
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
            if (src1.kind == IROperand::Kind::LITERAL) {
                emit("mov eax, " + opStr);
                emit("cvtsi2ss xmm0, eax");
            } else {
                emit("cvtsi2ss xmm0, " + opStr);
            }
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
            if (src2.kind == IROperand::Kind::LITERAL) {
                emit("mov eax, " + opStr);
                emit("cvtsi2ss xmm1, eax");
            } else {
                emit("cvtsi2ss xmm1, " + opStr);
            }
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
            if (src1.kind == IROperand::Kind::LITERAL) {
                emit("mov eax, " + opStr, "; constant to reg");
                emit("cvtsi2ss xmm0, eax", "; int to float");
            } else {
                emit("cvtsi2ss xmm0, " + opStr, "; int to float");
            }
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
            if (src2.kind == IROperand::Kind::LITERAL) {
                emit("mov eax, " + opStr, "; constant to reg");
                emit("cvtsi2ss xmm1, eax", "; int to float");
            } else {
                emit("cvtsi2ss xmm1, " + opStr, "; int to float");
            }
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
        std::string srcStr = getOperandString(src);
        
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
                std::string srcStr2 = getOperandString(src);
                if (srcStr2[0] == 'q') {
                    emit("movq xmm0, " + srcStr2, "; load temp as float");
                } else {
                    emit("movss xmm0, " + srcStr2, "; load float");
                }
            }
            emit("movss " + destStr + ", xmm0", "; save float");
            return;
        }
        
        if (srcStr[0] == 'q' && destStr[0] == 'q') {
            emit("mov rax, " + srcStr);
            emit("mov " + destStr + ", rax");
        } else if (destStr != srcStr) {
            emit("mov " + destStr + ", " + srcStr);
        }
    }
    // Для TEMP с именем (например __tmp2 = MOVE value)
    else if (dest.kind == IROperand::Kind::TEMP && !dest.name.empty()) {
        std::string srcStr = getOperandString(src);
        emit("mov rax, " + srcStr);
        emit("mov " + getStackSlot(dest.name) + ", rax", "; save to " + dest.name);
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
    
    if (value.kind == IROperand::Kind::LITERAL && value.type == IRType::STRING) {
        std::string str = value.stringValue;
        if (str.size() >= 2 && str.front() == '"' && str.back() == '"') {
            str = str.substr(1, str.size() - 2);
        }
        int strIdx = -1;
        for (size_t i = 0; i < rodataStrings.size(); i++) {
            if (rodataStrings[i] == str) {
                strIdx = i;
                break;
            }
        }
        
        if (strIdx >= 0) {
            valStr = "L.str" + std::to_string(strIdx);
            if (paramCounter < MAX_PARAM_REGS) {
                emit("lea " + std::string(PARAM_REGS[paramCounter]) + ", [rel " + valStr + "]",
                     "; param " + std::to_string(paramCounter + 1) + ": string");
            } else {
                emit("lea rax, [rel " + valStr + "]", "; load string address");
                emit("push rax", "; param " + std::to_string(paramCounter + 1) + " on stack");
            }
            paramCounter++;
            return;
        }
    }
    
    // Обычные параметры
    if (paramCounter < MAX_PARAM_REGS) {
        if (value.type == IRType::FLOAT) {
            // Для float используем 32-битный регистр (edi, esi, ...)
            std::string reg32 = PARAM_REGS[paramCounter];
            // Заменяем 64-битный регистр на 32-битный: rdi -> edi, rsi -> esi и т.д.
            if (reg32 == "rdi") reg32 = "edi";
            else if (reg32 == "rsi") reg32 = "esi";
            else if (reg32 == "rdx") reg32 = "edx";
            else if (reg32 == "rcx") reg32 = "ecx";
            else if (reg32 == "r8") reg32 = "r8d";
            else if (reg32 == "r9") reg32 = "r9d";
            emit("mov " + reg32 + ", " + valStr,
                 "; param " + std::to_string(paramCounter + 1) + ": float");
        } else {
            emit("mov " + std::string(PARAM_REGS[paramCounter]) + ", " + valStr,
                 "; param " + std::to_string(paramCounter + 1));
        }
    } else {
        emit("push " + valStr, "; param " + std::to_string(paramCounter + 1) + " on stack");
    }
    paramCounter++;
}

void X86Generator::saveResultToDest(const IROperand& dest) {
    if (dest.kind == IROperand::Kind::VARIABLE) {
        emit("mov " + getStackSlot(dest.name) + ", rax", "; save to " + dest.name);
    } else if (dest.kind == IROperand::Kind::TEMP) {
        emit("mov " + getStackSlot("__tmp") + ", rax", "; save temp");
    }
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
            // Для глобального массива возвращаем просто имя метки (адрес)
            if (globalArrayNames.count(op.name)) {
                return op.name;
            }
            if (stackSlots.find(op.name) == stackSlots.end()) {
                return "qword [" + op.name + "]";
            }
            return getStackSlot(op.name);
        case IROperand::Kind::TEMP:
            return getStackSlot("__tmp");
        case IROperand::Kind::LABEL:
            return op.name;
    }
    return "0";
}

void X86Generator::emitAddressLoad(const IROperand& addr) {
    std::string opStr = getOperandString(addr);
    if (addr.kind == IROperand::Kind::VARIABLE && globalArrayNames.count(addr.name)) {
        emit("lea rax, [" + opStr + "]");
    } else {
        emit("mov rax, " + opStr);
    }
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
std::string X86Generator::newStringLabel() { return "L.str" + std::to_string(stringLabelCounter++); }

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