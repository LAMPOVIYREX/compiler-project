#include "ir/IRGenerator.hpp"
#include <sstream>
#include <iostream>

namespace minicompiler {

IRGenerator::IRGenerator(SymbolTable& symbolTable, ErrorReporter& errorReporter)
    : symbolTable(symbolTable), errorReporter(errorReporter), 
      currentFunction(nullptr), currentBlock(nullptr), errorCount(0) {
    program = std::make_unique<IRProgram>();
}

std::unique_ptr<IRProgram> IRGenerator::generate(ProgramNode& program) {
    program.accept(*this);
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
        case BinaryOp::ASSIGN:
        case BinaryOp::ADD_ASSIGN:
        case BinaryOp::SUB_ASSIGN:
        case BinaryOp::MUL_ASSIGN:
        case BinaryOp::DIV_ASSIGN:
        case BinaryOp::MOD_ASSIGN:
            return left;
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
    
    // Для MOVE с временной переменной и литералом
    if (op == IROpcode::MOVE && src1.isConstant()) {
        // Если присваивание константы переменной
        auto instr = std::make_unique<IRInstruction>(op, dest, src1, src2);
        instr->comment = comment;
        currentBlock->addInstruction(std::move(instr));
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

IROperand IRGenerator::convertOperand(const IROperand& operand, IRType targetType) {
    if (operand.type == targetType) {
        return operand;
    }
    
    // int → float (расширение)
    if (operand.type == IRType::INT && targetType == IRType::FLOAT) {
        IROperand result = newTemp(targetType);
        emit(IROpcode::COPY, result, operand);
        return result;
    }
    
    // float → int (сужение)
    if (operand.type == IRType::FLOAT && targetType == IRType::INT) {
        IROperand result = newTemp(targetType);
        emit(IROpcode::COPY, result, operand);
        return result;
    }
    
    return operand;
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
    
    // Создаем entry блок (без лишней LABEL инструкции)
    currentBlock = currentFunction->createBlock("entry");
    
    // Генерируем тело функции
    if (node.body) {
        node.body->accept(*this);
    }
    
    // Если функция void, добавляем return в конце
    if (returnType == IRType::VOID && !currentBlock->isTerminated()) {
        emit(IROpcode::RETURN);
    }
    
    currentFunction = nullptr;
    currentBlock = nullptr;
}

// В src/ir/IRGenerator.cpp:
void IRGenerator::visit(StructDeclNode& node) {
    (void)node;  
   
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
    
    if (node.initializer) {
        if (auto call = dynamic_cast<CallExprNode*>(node.initializer.get())) {
            // Вызов функции для инициализации переменной
            for (size_t i = 0; i < call->arguments.size(); i++) {
                IROperand arg = generateExpression(call->arguments[i].get());
                emit(IROpcode::PARAM, IROperand(), arg);
            }
            IROperand result = newTemp();
            emit(IROpcode::CALL, result, IROperand::label(call->callee));
            emit(IROpcode::MOVE, var, result);
        } else {
            IROperand init = generateExpression(node.initializer.get());
            init = convertOperand(init, varType);
            emit(IROpcode::MOVE, var, init);
        }
    }
}

void IRGenerator::visit(IfStmtNode& node) {
    IROperand cond = generateExpression(node.condition.get());
    cond = convertOperand(cond, IRType::BOOL);
    
    std::string thenLabel = "then_" + std::to_string(currentFunction->tempCounter++);
    std::string elseLabel = "else_" + std::to_string(currentFunction->tempCounter++);
    std::string endLabel = "endif_" + std::to_string(currentFunction->tempCounter++);
    
    emitCondJump(cond, thenLabel, elseLabel);
    
    // Then branch
    emitLabel(thenLabel);
    node.thenBranch->accept(*this);
    if (!currentBlock->isTerminated()) {
        emitJump(endLabel);
    }
    
    // Else branch (if exists)
    emitLabel(elseLabel);
    if (node.elseBranch) {
        node.elseBranch->accept(*this);
    }
    if (!currentBlock->isTerminated()) {
        emitJump(endLabel);
    }
    
    // End label
    emitLabel(endLabel);
}

void IRGenerator::visit(WhileStmtNode& node) {
    std::string loopLabel = "loop_" + std::to_string(currentFunction->tempCounter++);
    std::string bodyLabel = loopLabel + "_body";
    std::string exitLabel = "exit_" + std::to_string(currentFunction->tempCounter++);
    
    currentBlock = currentFunction->createBlock(loopLabel);
    
    IROperand cond = generateExpression(node.condition.get());
    cond = convertOperand(cond, IRType::BOOL);
    
    // Если условие ложно, переходим на выход
    emit(IROpcode::JUMP_IF_NOT, IROperand(), cond, IROperand::label(exitLabel));
    
    currentBlock = currentFunction->createBlock(bodyLabel);
    node.body->accept(*this);
    if (!currentBlock->isTerminated()) {
        emitJump(loopLabel);
    }
    
    currentBlock = currentFunction->createBlock(exitLabel);
}

void IRGenerator::visit(ForStmtNode& node) {
    // Инициализация
    if (node.init) {
        node.init->accept(*this);
    }
    
    std::string loopLabel = "loop_" + std::to_string(currentFunction->tempCounter++);
    std::string bodyLabel = loopLabel + "_body";
    std::string exitLabel = "exit_" + std::to_string(currentFunction->tempCounter++);
    
    currentBlock = currentFunction->createBlock(loopLabel);
    
    // Условие
    if (node.condition) {
        IROperand cond = generateExpression(node.condition.get());
        cond = convertOperand(cond, IRType::BOOL);
        IROperand temp = newTemp(IRType::BOOL);
        emit(IROpcode::CMP_NE, temp, cond, IROperand::literal(0));
        emit(IROpcode::JUMP_IF, IROperand(), temp, IROperand::label(bodyLabel));
        emitJump(exitLabel);
    }
    
    // Тело
    currentBlock = currentFunction->createBlock(bodyLabel);
    if (node.body) {
        node.body->accept(*this);
    }
    
    // Обновление
    if (node.update) {
        node.update->accept(*this);
    }
    
    if (!currentBlock->isTerminated()) {
        emitJump(loopLabel);
    }
    
    currentBlock = currentFunction->createBlock(exitLabel);
}

void IRGenerator::visit(ReturnStmtNode& node) {
    if (node.value) {
        IROperand value = generateExpression(node.value.get());
        value = convertOperand(value, currentFunction->returnType);
        emit(IROpcode::RETURN, IROperand(), value);
    } else {
        emit(IROpcode::RETURN);
    }
}

void IRGenerator::visit(ExprStmtNode& node) {
    if (node.expression) {
        // Генерируем выражение, но результат нам не нужен
        if (auto call = dynamic_cast<CallExprNode*>(node.expression.get())) {
            // Для вызова функции без сохранения результата
            for (size_t i = 0; i < call->arguments.size(); i++) {
                IROperand arg = generateExpression(call->arguments[i].get());
                emit(IROpcode::PARAM, IROperand(), arg);
            }
            IROperand result = newTemp();
            emit(IROpcode::CALL, result, IROperand::label(call->callee));
            // Результат игнорируем
        } else {
            generateExpression(node.expression.get());
        }
        // Очищаем стек
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
        // Просто передаем переменную, не загружая её
        valueStack.push(it->second);
    } else {
        reportError(node.getLine(), node.getColumn(), "Undeclared variable: " + node.name);
        valueStack.push(newTemp());
    }
}

void IRGenerator::visit(BinaryExprNode& node) {
    // Присваивание
    if (node.op == BinaryOp::ASSIGN) {
        IROperand right = generateExpression(node.right.get());
        
        IROperand left;
        if (auto id = dynamic_cast<IdentifierExprNode*>(node.left.get())) {
            auto it = varMap.find(id->name);
            if (it != varMap.end()) {
                left = it->second;
            }
        } else {
            left = generateExpression(node.left.get());
        }
        
        right = convertOperand(right, left.type);
        emit(IROpcode::MOVE, left, right);
        valueStack.push(left);
        return;
    }
    
    // Обычные бинарные операции
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
        case BinaryOp::AND: irOp = IROpcode::AND; break;
        case BinaryOp::OR: irOp = IROpcode::OR; break;
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
    IROperand result = generateUnaryOp(node.op, operand, node.getLine());
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
    
    // Сохраняем результат на стеке
    valueStack.push(result);
}

void IRGenerator::visit(IndexExprNode& node) {
    // Генерируем базовый адрес
    IROperand array = generateExpression(node.array.get());
    
    // Генерируем индекс
    IROperand index = generateExpression(node.index.get());
    
    // Создаем временную переменную для адреса элемента
    IROperand elementAddr = newTemp(IRType::INT);
    IROperand elementSize = IROperand::literal(4); // размер int
    
    // Вычисляем смещение: index * elementSize
    IROperand offset = newTemp(IRType::INT);
    emit(IROpcode::MUL, offset, index, elementSize, "смещение элемента массива");
    
    // Вычисляем адрес: array + offset
    emit(IROpcode::ADD, elementAddr, array, offset, "адрес элемента массива");
    
    // Загружаем значение
    IROperand result = newTemp(IRType::INT);
    emit(IROpcode::LOAD, result, elementAddr, IROperand(), "загрузка из массива");
    
    valueStack.push(result);
}

void IRGenerator::visit(MemberAccessExprNode& node) {
    // Генерируем адрес объекта
    IROperand object = generateExpression(node.object.get());
    
    // Создаем временную переменную для адреса поля
    IROperand fieldAddr = newTemp(IRType::INT);
    
    // Временное решение - просто копируем адрес
    // TODO: Добавить реальное вычисление смещения из SymbolTable
    emit(IROpcode::MOVE, fieldAddr, object, IROperand(), "доступ к полю " + node.member);
    
    // Загружаем значение
    IROperand result = newTemp(IRType::INT);
    emit(IROpcode::LOAD, result, fieldAddr, IROperand(), "загрузка поля " + node.member);
    
    valueStack.push(result);
}

void IRGenerator::dumpIR() {
    std::cout << program->toString() << std::endl;
}

} // namespace minicompiler