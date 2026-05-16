#include "ir/IRGenerator.hpp"
#include <sstream>
#include <iostream>

namespace minicompiler {

IRGenerator::IRGenerator(SymbolTable& symbolTable, ErrorReporter& errorReporter)
    : symbolTable(symbolTable), errorReporter(errorReporter), 
      currentFunction(nullptr), currentBlock(nullptr), errorCount(0) {
    program = std::make_unique<IRProgram>();
}

std::unique_ptr<IRProgram> IRGenerator::generate(ProgramNode& programNode) {
    programNode.accept(*this);
    return std::move(this->program);
}

void IRGenerator::reportError(int line, int column, const std::string& message) {
    errorCount++;
    errorReporter.reportGeneralError(line, column, "IR generation error: " + message);
}

//=============================================================================
// Генерация выражений
//=============================================================================

IROperand IRGenerator::generateExpression(ExpressionNode* expr) {
    expr->accept(*this);
    if (valueStack.empty()) {
        return IROperand();
    }
    IROperand result = valueStack.top();
    valueStack.pop();
    return result;
}

//=============================================================================
// Управление потоком
//=============================================================================

void IRGenerator::emitJump(const std::string& target) {
    emit(IROpcode::JUMP, IROperand(), IROperand::label(target));
}

void IRGenerator::emitCondJump(const IROperand& cond, const std::string& trueTarget, 
                                const std::string& falseTarget) {
    emit(IROpcode::JUMP_IF, IROperand(), cond, IROperand::label(trueTarget));
    emitJump(falseTarget);
}

void IRGenerator::emitLabel(const std::string& name) {
    if (currentBlock && currentBlock->name == name) {
        return;
    }
    currentBlock = currentFunction->createBlock(name);
}

void IRGenerator::emit(IROpcode op, const IROperand& dest,
                       const IROperand& src1, const IROperand& src2,
                       const std::string& comment) {
    if (!currentBlock) {
        reportError(0, 0, "No current block");
        return;
    }
    
    auto instr = std::make_unique<IRInstruction>(op, dest, src1, src2);
    instr->comment = comment;
    currentBlock->addInstruction(std::move(instr));
}

IROperand IRGenerator::newTemp(IRType type) {
    if (!currentFunction) {
        return IROperand();
    }
    return currentFunction->newTemp(type);
}

//=============================================================================
// Конвертация типов
//=============================================================================

IRType IRGenerator::convertType(const Type& type) {
    switch (type.kind) {
        case TypeKind::INT: return IRType::INT;
        case TypeKind::FLOAT: return IRType::FLOAT;
        case TypeKind::BOOL: return IRType::BOOL;
        case TypeKind::STRING: return IRType::STRING;
        default: return IRType::VOID;
    }
}

//=============================================================================
// AST Visitor методы
//=============================================================================

void IRGenerator::visit(ProgramNode& node) {
    for (auto& decl : node.declarations) {
        decl->accept(*this);
    }
}

void IRGenerator::visit(FunctionDeclNode& node) {
    varMap.clear();
    
    IRType returnType = convertType(node.returnType);
    currentFunction = program->createFunction(node.name, returnType);
    
    // Добавляем параметры
    for (const auto& param : node.parameters) {
        IROperand paramOp = IROperand::variable(param.second, convertType(param.first));
        currentFunction->addParameter(paramOp);
        varMap[param.second] = paramOp;
    }
    
    // Создаем entry блок
    currentBlock = currentFunction->createBlock("entry");
    
    // Генерируем тело функции
    if (node.body) {
        node.body->accept(*this);
    }
    
    // Если функция void и нет return, добавляем
    if (returnType == IRType::VOID && !currentBlock->isTerminated()) {
        emit(IROpcode::RETURN);
    }
    
    currentFunction = nullptr;
    currentBlock = nullptr;
}

void IRGenerator::visit(StructDeclNode& node) {
    (void)node;
    // Структуры пока не генерируем в IR
}

void IRGenerator::visit(BlockStmtNode& node) {
    for (auto& stmt : node.statements) {
        stmt->accept(*this);
    }
}

void IRGenerator::visit(VarDeclStmtNode& node) {
    IRType varType = convertType(node.varType);
    IROperand var = IROperand::variable(node.name, varType);
    varMap[node.name] = var;
    
    // Для массивов выделяем память
    if (node.varType.isArray()) {
        int arraySize = node.varType.arraySize;
        if (arraySize > 0) {
            emit(IROpcode::ALLOCA, var, IROperand::literal(arraySize * 8), IROperand(),
                 "allocate array " + node.name + "[" + std::to_string(arraySize) + "]");
        }
        return;
    }
    
    if (node.initializer) {
        IROperand init = generateExpression(node.initializer.get());
        init = convertOperand(init, varType);
        emit(IROpcode::MOVE, var, init);
    }
}



void IRGenerator::visit(IfStmtNode& node) {
    IROperand cond = generateExpression(node.condition.get());
    
    std::string thenLabel = "then_" + std::to_string(currentFunction->tempCounter++);
    std::string elseLabel = "else_" + std::to_string(currentFunction->tempCounter++);
    std::string endLabel = "endif_" + std::to_string(currentFunction->tempCounter++);
    
    // Если есть else ветка
    if (node.elseBranch) {
        emit(IROpcode::JUMP_IF_NOT, IROperand(), cond, IROperand::label(elseLabel));
    } else {
        emit(IROpcode::JUMP_IF_NOT, IROperand(), cond, IROperand::label(endLabel));
    }
    
    // Then branch
    emitLabel(thenLabel);
    node.thenBranch->accept(*this);
    if (!currentBlock->isTerminated()) {
        emitJump(endLabel);
    }
    
    // Else branch
    if (node.elseBranch) {
        emitLabel(elseLabel);
        node.elseBranch->accept(*this);
        if (!currentBlock->isTerminated()) {
            emitJump(endLabel);
        }
    }
    
    // End label
    emitLabel(endLabel);
}

void IRGenerator::visit(WhileStmtNode& node) {
    std::string loopLabel = "loop_" + std::to_string(currentFunction->tempCounter++);
    std::string bodyLabel = loopLabel + "_body";
    std::string exitLabel = "exit_" + std::to_string(currentFunction->tempCounter++);
    
    // Переход на проверку условия
    emitJump(loopLabel);
    
    // Метка условия
    emitLabel(loopLabel);
    
    IROperand cond = generateExpression(node.condition.get());
    emit(IROpcode::JUMP_IF_NOT, IROperand(), cond, IROperand::label(exitLabel));
    
    // Тело цикла
    emitLabel(bodyLabel);
    node.body->accept(*this);
    if (!currentBlock->isTerminated()) {
        emitJump(loopLabel);
    }
    
    // Выход из цикла
    emitLabel(exitLabel);
}

void IRGenerator::visit(ForStmtNode& node) {
    // For разворачивается в while:
    // init; while (condition) { body; update; }
    
    // Init
    if (node.init) {
        node.init->accept(*this);
    }
    
    std::string loopLabel = "loop_" + std::to_string(currentFunction->tempCounter++);
    std::string bodyLabel = loopLabel + "_body";
    std::string exitLabel = "exit_" + std::to_string(currentFunction->tempCounter++);
    
    // Переход на проверку условия
    emitJump(loopLabel);
    
    // Метка условия
    emitLabel(loopLabel);
    
    // Condition
    if (node.condition) {
        IROperand cond = generateExpression(node.condition.get());
        emit(IROpcode::JUMP_IF_NOT, IROperand(), cond, IROperand::label(exitLabel));
    }
    
    // Тело
    emitLabel(bodyLabel);
    if (node.body) {
        node.body->accept(*this);
    }
    
    // Update
    if (node.update) {
        node.update->accept(*this);
    }
    
    if (!currentBlock->isTerminated()) {
        emitJump(loopLabel);
    }
    
    // Выход
    emitLabel(exitLabel);
}

void IRGenerator::visit(ReturnStmtNode& node) {
    if (node.value) {
        IROperand value = generateExpression(node.value.get());
        emit(IROpcode::RETURN, IROperand(), value);
    } else {
        emit(IROpcode::RETURN);
    }
}

void IRGenerator::visit(ExprStmtNode& node) {
    if (node.expression) {
        if (auto call = dynamic_cast<CallExprNode*>(node.expression.get())) {
            for (size_t i = 0; i < call->arguments.size(); i++) {
                IROperand arg = generateExpression(call->arguments[i].get());
                emit(IROpcode::PARAM, IROperand(), arg);
            }
            IROperand result = newTemp();
            emit(IROpcode::CALL, result, IROperand::label(call->callee));
        } else {
            generateExpression(node.expression.get());
        }
        while (!valueStack.empty()) {
            valueStack.pop();
        }
    }
}

void IRGenerator::visit(LiteralExprNode& node) {
    IROperand result;
    
    if (std::holds_alternative<int>(node.value)) {
        result = IROperand::literal(std::get<int>(node.value));
    } else if (std::holds_alternative<double>(node.value)) {
        result = IROperand::literal(std::get<double>(node.value));
    } else if (std::holds_alternative<bool>(node.value)) {
        result = IROperand::literal(std::get<bool>(node.value));
    } else if (std::holds_alternative<std::string>(node.value)) {
        result = IROperand::literal(std::get<std::string>(node.value));
    }
    
    valueStack.push(result);
}

void IRGenerator::visit(IdentifierExprNode& node) {
    auto it = varMap.find(node.name);
    if (it != varMap.end()) {
        valueStack.push(it->second);
    } else {
        reportError(node.getLine(), node.getColumn(), "Undeclared variable: " + node.name);
        valueStack.push(newTemp());
    }
}

void IRGenerator::visit(BinaryExprNode& node) {
    // ============================================================
    // SHORT-CIRCUIT EVALUATION FOR LOGICAL AND (&&)
    // ============================================================
    if (node.op == BinaryOp::AND) {
        std::string scTrueLabel = "sc_true_" + std::to_string(currentFunction->tempCounter++);
        std::string scFalseLabel = "sc_false_" + std::to_string(currentFunction->tempCounter++);
        std::string scEndLabel = "sc_end_" + std::to_string(currentFunction->tempCounter++);
        
        IROperand result = newTemp(IRType::BOOL);
        
        // Вычисляем левую часть
        IROperand left = generateExpression(node.left.get());
        
        // Если левая часть false — переходим на false метку (короткое замыкание)
        emit(IROpcode::JUMP_IF_NOT, IROperand(), left, IROperand::label(scFalseLabel));
        
        // Иначе вычисляем правую часть
        IROperand right = generateExpression(node.right.get());
        emit(IROpcode::MOVE, result, right);
        emitJump(scEndLabel);
        
        // False: результат = 0
        emitLabel(scFalseLabel);
        emit(IROpcode::MOVE, result, IROperand::literal(0));
        
        emitLabel(scEndLabel);
        valueStack.push(result);
        return;
    }
    
    // ============================================================
    // SHORT-CIRCUIT EVALUATION FOR LOGICAL OR (||)
    // ============================================================
    if (node.op == BinaryOp::OR) {
        std::string scTrueLabel = "sc_true_" + std::to_string(currentFunction->tempCounter++);
        std::string scEndLabel = "sc_end_" + std::to_string(currentFunction->tempCounter++);
        
        IROperand result = newTemp(IRType::BOOL);
        
        // Вычисляем левую часть
        IROperand left = generateExpression(node.left.get());
        
        // Если левая часть true — переходим на true метку (короткое замыкание)
        emit(IROpcode::JUMP_IF, IROperand(), left, IROperand::label(scTrueLabel));
        
        // Иначе вычисляем правую часть
        IROperand right = generateExpression(node.right.get());
        emit(IROpcode::MOVE, result, right);
        emitJump(scEndLabel);
        
        // True: результат = 1
        emitLabel(scTrueLabel);
        emit(IROpcode::MOVE, result, IROperand::literal(1));
        
        emitLabel(scEndLabel);
        valueStack.push(result);
        return;
    }
    
    // ============================================================
    // Присваивание
    // ============================================================
    if (node.op == BinaryOp::ASSIGN) {
        IROperand right = generateExpression(node.right.get());
        
        // ============================================================
        // Присваивание в элемент массива: arr[i] = value
        // ============================================================
        // В IRGenerator::visit(BinaryExprNode&) для ASSIGN с IndexExprNode:
        if (auto indexExpr = dynamic_cast<IndexExprNode*>(node.left.get())) {
            // 1. Вычисляем значение
            IROperand rightVal = generateExpression(node.right.get());
            IROperand savedValue = IROperand::variable("__array_val", IRType::INT);
            emit(IROpcode::MOVE, savedValue, rightVal);
            
            // 2. Вычисляем адрес и сохраняем в переменную
            IROperand base = generateExpression(indexExpr->array.get());
            IROperand index = generateExpression(indexExpr->index.get());
            IROperand savedAddr = IROperand::variable("__array_addr", IRType::INT);
            IROperand offset = newTemp();
            IROperand addr = newTemp();
            emit(IROpcode::MUL, offset, index, IROperand::literal(8));
            emit(IROpcode::ADD, addr, base, offset);
            emit(IROpcode::MOVE, savedAddr, addr);  // сохраняем адрес
            
            // 3. STORE использует переменные (не TEMP)
            emit(IROpcode::STORE, savedAddr, savedValue);
            valueStack.push(rightVal);
            return;
        }
        
        // ============================================================
        // Присваивание в поле структуры: obj.field = value
        // ============================================================
        if (auto memberExpr = dynamic_cast<MemberAccessExprNode*>(node.left.get())) {
            IROperand addr = generateExpression(memberExpr);
            emit(IROpcode::STORE, addr, right, IROperand(), "store to struct field");
            valueStack.push(right);
            return;
        }
        
        // ============================================================
        // Присваивание в переменную
        // ============================================================
        IROperand left;
        if (auto id = dynamic_cast<IdentifierExprNode*>(node.left.get())) {
            auto it = varMap.find(id->name);
            if (it != varMap.end()) {
                left = it->second;
            } else {
                reportError(node.getLine(), node.getColumn(), "Undeclared variable: " + id->name);
                valueStack.push(right);
                return;
            }
        } else {
            left = generateExpression(node.left.get());
        }
        
        right = convertOperand(right, left.type);
        emit(IROpcode::MOVE, left, right);
        valueStack.push(left);
        return;
    }
    
    // ============================================================
    // Обычные бинарные операции
    // ============================================================
    IROperand left = generateExpression(node.left.get());
    IROperand right = generateExpression(node.right.get());
    
    // Приведение типов
    if (left.type != right.type) {
        if (left.type == IRType::INT && right.type == IRType::FLOAT) {
            left = convertOperand(left, IRType::FLOAT);
        } else if (left.type == IRType::FLOAT && right.type == IRType::INT) {
            right = convertOperand(right, IRType::FLOAT);
        }
    }
    
    IROperand result = newTemp(left.type);
    IROpcode irOp;
    
    switch (node.op) {
        case BinaryOp::ADD: irOp = IROpcode::ADD; break;
        case BinaryOp::SUB: irOp = IROpcode::SUB; break;
        case BinaryOp::MUL: irOp = IROpcode::MUL; break;
        case BinaryOp::DIV: irOp = IROpcode::DIV; break;
        case BinaryOp::MOD: irOp = IROpcode::MOD; break;
        case BinaryOp::EQ: irOp = IROpcode::CMP_EQ; break;
        case BinaryOp::NE: irOp = IROpcode::CMP_NE; break;
        case BinaryOp::LT: irOp = IROpcode::CMP_LT; break;
        case BinaryOp::LE: irOp = IROpcode::CMP_LE; break;
        case BinaryOp::GT: irOp = IROpcode::CMP_GT; break;
        case BinaryOp::GE: irOp = IROpcode::CMP_GE; break;
        default:
            reportError(node.getLine(), node.getColumn(), "Unknown binary operator");
            valueStack.push(result);
            return;
    }
    
    emit(irOp, result, left, right);
    valueStack.push(result);
}

void IRGenerator::visit(UnaryExprNode& node) {
    IROperand operand = generateExpression(node.operand.get());
    IROperand result = newTemp(IRType::BOOL);
    
    switch (node.op) {
        case UnaryOp::NEG:
            emit(IROpcode::NEG, result, operand);
            break;
        case UnaryOp::NOT:
            emit(IROpcode::NOT, result, operand);
            break;
        default:
            result = operand;
            break;
    }
    
    valueStack.push(result);
}

void IRGenerator::visit(CallExprNode& node) {
    // Передаем параметры
    for (size_t i = 0; i < node.arguments.size(); i++) {
        IROperand arg = generateExpression(node.arguments[i].get());
        emit(IROpcode::PARAM, IROperand(), arg);
    }
    
    // Вызываем функцию
    IROperand result = newTemp();
    emit(IROpcode::CALL, result, IROperand::label(node.callee));
    
    valueStack.push(result);
}

void IRGenerator::visit(IndexExprNode& node) {
    IROperand base = generateExpression(node.array.get());
    IROperand index = generateExpression(node.index.get());
    
    IROperand addr = newTemp();
    IROperand offset = newTemp();
    
    emit(IROpcode::MUL, offset, index, IROperand::literal(8), "offset = index * 8");
    emit(IROpcode::ADD, addr, base, offset, "address = base + offset");
    
    // ВСЕГДА делаем LOAD — возвращаем ЗНАЧЕНИЕ, не адрес
    IROperand value = newTemp();
    emit(IROpcode::LOAD, value, addr, IROperand(), "load from array");
    
    valueStack.push(value);  // возвращаем значение
}

void IRGenerator::visit(MemberAccessExprNode& node) {
    // Заглушка для структур
    reportError(node.getLine(), node.getColumn(), "Structs not yet supported in IR");
    valueStack.push(newTemp());
}

void IRGenerator::dumpIR() {
    std::cout << program->toString() << std::endl;
}

IROperand IRGenerator::convertOperand(const IROperand& operand, IRType targetType) {
    if (operand.type == targetType) {
        return operand;
    }
    
    if (operand.type == IRType::INT && targetType == IRType::FLOAT) {
        IROperand result = newTemp(targetType);
        emit(IROpcode::COPY, result, operand);
        return result;
    }
    
    if (operand.type == IRType::FLOAT && targetType == IRType::INT) {
        IROperand result = newTemp(targetType);
        emit(IROpcode::COPY, result, operand);
        return result;
    }
    
    return operand;
}

IROperand IRGenerator::generateBinaryOp(BinaryOp op, const IROperand& left, 
                                         const IROperand& right, int line) {
    IROperand result = newTemp();
    IROpcode irOp;
    
    switch (op) {
        case BinaryOp::ADD: irOp = IROpcode::ADD; break;
        case BinaryOp::SUB: irOp = IROpcode::SUB; break;
        case BinaryOp::MUL: irOp = IROpcode::MUL; break;
        case BinaryOp::DIV: irOp = IROpcode::DIV; break;
        case BinaryOp::MOD: irOp = IROpcode::MOD; break;
        case BinaryOp::EQ: irOp = IROpcode::CMP_EQ; break;
        case BinaryOp::NE: irOp = IROpcode::CMP_NE; break;
        case BinaryOp::LT: irOp = IROpcode::CMP_LT; break;
        case BinaryOp::LE: irOp = IROpcode::CMP_LE; break;
        case BinaryOp::GT: irOp = IROpcode::CMP_GT; break;
        case BinaryOp::GE: irOp = IROpcode::CMP_GE; break;
        case BinaryOp::AND: irOp = IROpcode::AND; break;
        case BinaryOp::OR: irOp = IROpcode::OR; break;
        default:
            reportError(line, 0, "Unknown binary operator");
            return result;
    }
    
    emit(irOp, result, left, right);
    return result;
}

IROperand IRGenerator::generateUnaryOp(UnaryOp op, const IROperand& operand, int line) {
    IROperand result = newTemp();
    
    switch (op) {
        case UnaryOp::NEG:
            emit(IROpcode::NEG, result, operand);
            break;
        case UnaryOp::NOT:
            emit(IROpcode::NOT, result, operand);
            break;
        default:
            reportError(line, 0, "Unknown unary operator");
            break;
    }
    
    return result;
}

} // namespace minicompiler