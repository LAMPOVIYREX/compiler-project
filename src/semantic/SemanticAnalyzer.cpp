#include "semantic/SemanticAnalyzer.hpp"
#include <sstream>
#include <iostream>

namespace minicompiler {

// Вспомогательные функции
int SemanticAnalyzer::align(int offset, int alignment) {
    return (offset + alignment - 1) & ~(alignment - 1);
}

void SemanticAnalyzer::pushStackOffset() {
    stackOffsetStack.push_back(currentStackOffset);
}

void SemanticAnalyzer::popStackOffset() {
    if (!stackOffsetStack.empty()) {
        currentStackOffset = stackOffsetStack.back();
        stackOffsetStack.pop_back();
    }
}

void SemanticAnalyzer::allocateVariable(const std::string& name, int size, int alignment) {
    currentStackOffset = align(currentStackOffset, alignment);
    
    auto symbol = symbolTable.lookupLocal(name);
    if (symbol) {
        symbol->stackOffset = currentStackOffset;
        symbol->size = size;
    }
    
    currentStackOffset += size;
}

void SemanticAnalyzer::calculateStackOffsets(BlockStmtNode& node) {
    pushStackOffset();
    
    for (auto& stmt : node.statements) {
        if (auto varDecl = dynamic_cast<VarDeclStmtNode*>(stmt.get())) {
            int size = varDecl->varType.getSize();
            int alignment = varDecl->varType.getAlignment();
            allocateVariable(varDecl->name, size, alignment);
        }
    }
    
    for (auto& stmt : node.statements) {
        if (auto block = dynamic_cast<BlockStmtNode*>(stmt.get())) {
            calculateStackOffsets(*block);
        }
    }
    
    popStackOffset();
}

void SemanticAnalyzer::printMemoryLayout() const {
    std::cerr << "\nMemory Layout (Stack Offsets):" << std::endl;
    std::cerr << "=================================" << std::endl;
    std::cerr << symbolTable.getMemoryLayoutString();
}

SemanticAnalyzer::SemanticAnalyzer(ErrorReporter& errorReporter)
    : errorReporter(errorReporter), errorCount(0), inFunction(false) {}

bool SemanticAnalyzer::analyze(ProgramNode& program) {
    errorCount = 0;
    program.accept(*this);
    return errorCount == 0;
}

void SemanticAnalyzer::reportError(int line, int column, const std::string& message) {
    errorCount++;
    errorReporter.reportGeneralError(line, column, message);
}

void SemanticAnalyzer::reportError(const std::string& message) {
    errorCount++;
    errorReporter.reportGeneralError(0, 0, message);
}

void SemanticAnalyzer::reportError(int line, int column, const std::string& message, ErrorCode code) {
    errorCount++;
    errorReporter.addError(code, message, line, column, 1);
}

Type SemanticAnalyzer::getExpressionType(ExpressionNode* expr) {
    auto it = exprTypes.find(expr);
    if (it != exprTypes.end()) {
        return it->second;
    }
    return Type(TypeKind::ERROR);
}

void SemanticAnalyzer::setExpressionType(ExpressionNode* expr, const Type& type) {
    exprTypes[expr] = type;
}

bool SemanticAnalyzer::isTypeCompatible(const Type& expected, const Type& actual) {
    // Если один из типов ERROR, считаем несовместимыми
    if (expected.isError() || actual.isError()) return false;
    
    // Одинаковые типы всегда совместимы
    if (expected.kind == actual.kind) {
        if (expected.kind == TypeKind::STRUCT) {
            return expected.structName == actual.structName;
        }
        return true;
    }
    
    // int -> float разрешено
    if (expected.kind == TypeKind::FLOAT && actual.kind == TypeKind::INT) {
        return true;
    }
    
    return false;
}

bool SemanticAnalyzer::isAssignable(const Type& target, const Type& source) {
    if (source.isError()) return false;
    if (target.isVoid() || source.isVoid()) return false;
    return isTypeCompatible(target, source);
}

TypeCheckResult SemanticAnalyzer::checkBinaryOp(BinaryOp op, const Type& left, 
                                                 const Type& right, int line, int col) {
    // Если один из операндов - ERROR, возвращаем ERROR
    if (left.isError() || right.isError()) {
        return TypeCheckResult(Type(TypeKind::ERROR));
    }
    
    // Логические операторы
    if (op == BinaryOp::AND || op == BinaryOp::OR) {
        if (left.kind != TypeKind::BOOL || right.kind != TypeKind::BOOL) {
            std::stringstream ss;
            ss << "Логический оператор требует bool, получены: " 
               << left.toString() << " и " << right.toString();
            reportError(line, col, ss.str());
            return TypeCheckResult(Type(TypeKind::ERROR));
        }
        return Type(TypeKind::BOOL);
    }
    
    // Операторы сравнения
    if (op == BinaryOp::EQ || op == BinaryOp::NE || 
        op == BinaryOp::LT || op == BinaryOp::LE || 
        op == BinaryOp::GT || op == BinaryOp::GE) {
        
        if (!isTypeCompatible(left, right) && !isTypeCompatible(right, left)) {
            std::stringstream ss;
            ss << "Несовместимые типы в сравнении: " 
               << left.toString() << " и " << right.toString();
            reportError(line, col, ss.str());
            return TypeCheckResult(Type(TypeKind::ERROR));
        }
        return Type(TypeKind::BOOL);
    }
    
    // Арифметические операторы
    if (op == BinaryOp::ADD || op == BinaryOp::SUB || 
        op == BinaryOp::MUL || op == BinaryOp::DIV || op == BinaryOp::MOD) {
        
        if ((left.kind != TypeKind::INT && left.kind != TypeKind::FLOAT) ||
            (right.kind != TypeKind::INT && right.kind != TypeKind::FLOAT)) {
            std::stringstream ss;
            ss << "Арифметический оператор требует числовые типы, получены: " 
               << left.toString() << " и " << right.toString();
            reportError(line, col, ss.str());
            return TypeCheckResult(Type(TypeKind::ERROR));
        }
        
        if (left.kind == TypeKind::FLOAT || right.kind == TypeKind::FLOAT) {
            return Type(TypeKind::FLOAT);
        }
        return Type(TypeKind::INT);
    }
    
    // Операторы присваивания
    if (op == BinaryOp::ASSIGN || op == BinaryOp::ADD_ASSIGN || 
        op == BinaryOp::SUB_ASSIGN || op == BinaryOp::MUL_ASSIGN ||
        op == BinaryOp::DIV_ASSIGN || op == BinaryOp::MOD_ASSIGN) {
        
        if (!isAssignable(left, right)) {
            std::stringstream ss;
            ss << "Несовместимые типы в присваивании: " 
               << left.toString() << " = " << right.toString();
            reportError(line, col, ss.str());
            return TypeCheckResult(Type(TypeKind::ERROR));
        }
        return left;
    }
    
    return Type(TypeKind::ERROR);
}

TypeCheckResult SemanticAnalyzer::checkUnaryOp(UnaryOp op, const Type& operand, int line, int col) {
    if (operand.isError()) {
        return TypeCheckResult(Type(TypeKind::ERROR));
    }
    
    if (op == UnaryOp::NOT) {
        if (operand.kind != TypeKind::BOOL) {
            std::stringstream ss;
            ss << "Оператор ! требует bool, получен: " << operand.toString();
            reportError(line, col, ss.str());
            return TypeCheckResult(Type(TypeKind::ERROR));
        }
        return Type(TypeKind::BOOL);
    }
    
    if (op == UnaryOp::NEG) {
        if (!operand.isArithmetic()) {
            std::stringstream ss;
            ss << "Унарный минус требует числовой тип, получен: " << operand.toString();
            reportError(line, col, ss.str());
            return TypeCheckResult(Type(TypeKind::ERROR));
        }
        return operand;
    }
    
    return operand;
}

//=============================================================================
// Обход AST узлов
//=============================================================================

void SemanticAnalyzer::visit(ProgramNode& node) {
    // 🔑 ПЕРВЫЙ ПРОХОД: Собираем ВСЕ объявления функций и структур в глобальную область
    for (auto& decl : node.declarations) {
        if (auto funcDecl = dynamic_cast<FunctionDeclNode*>(decl.get())) {
            // Проверяем дублирование
            if (symbolTable.lookup(funcDecl->name)) {
                reportError(funcDecl->getLine(), funcDecl->getColumn(),
                    "Повторное объявление функции '" + funcDecl->name + "'");
                continue;
            }
            
            // Создаем символ функции
            auto symbol = std::make_shared<SymbolInfo>(
                funcDecl->name, funcDecl->returnType, SymbolKind::FUNCTION,
                funcDecl->getLine(), funcDecl->getColumn());
            symbol->returnType = funcDecl->returnType;
            
            for (const auto& param : funcDecl->parameters) {
                symbol->parameterTypes.push_back(param.first);
                symbol->parameterNames.push_back(param.second);
            }
            
            // Добавляем в глобальную таблицу символов
            symbolTable.insert(funcDecl->name, symbol);
        }
        else if (auto structDecl = dynamic_cast<StructDeclNode*>(decl.get())) {
            if (symbolTable.lookup(structDecl->name)) {
                reportError(structDecl->getLine(), structDecl->getColumn(),
                    "Повторное объявление структуры '" + structDecl->name + "'");
                continue;
            }
            
            auto symbol = std::make_shared<SymbolInfo>(
                structDecl->name, Type(structDecl->name), SymbolKind::STRUCT,
                structDecl->getLine(), structDecl->getColumn());
            
            // Собираем поля структуры
            for (const auto& field : structDecl->fields) {
                // Проверяем дублирование полей
                if (symbol->fields.find(field->name) != symbol->fields.end()) {
                    reportError(field->getLine(), field->getColumn(),
                        "Повторное объявление поля '" + field->name + "' в структуре '" + structDecl->name + "'");
                }
                symbol->fields[field->name] = field->varType;
            }
            
            symbolTable.insert(structDecl->name, symbol);
        }
    }
    
    // 🔑 ВТОРОЙ ПРОХОД: Анализируем тела функций
    for (auto& decl : node.declarations) {
        if (auto funcDecl = dynamic_cast<FunctionDeclNode*>(decl.get())) {
            funcDecl->accept(*this);
        }
        else if (auto structDecl = dynamic_cast<StructDeclNode*>(decl.get())) {
            structDecl->accept(*this);
        }
    }
}

void SemanticAnalyzer::visit(FunctionDeclNode& node) {
    inFunction = true;
    currentFunctionName = node.name;
    currentFunctionReturnType = node.returnType;
    
    symbolTable.enterScope();
    
    // Сбрасываем смещение стека для функции
    currentStackOffset = 0;
    
    // Добавляем параметры в область видимости функции
    for (size_t i = 0; i < node.parameters.size(); i++) {
        const auto& param = node.parameters[i];
        const std::string& paramName = param.second;
        const Type& paramType = param.first;
        
        if (symbolTable.lookupLocal(paramName)) {
            reportError(node.getLine(), node.getColumn(),
                "Повторное объявление параметра '" + paramName + "'");
            continue;
        }
        
        auto symbol = std::make_shared<SymbolInfo>(
            paramName, paramType, SymbolKind::PARAMETER,
            node.getLine(), node.getColumn());
        
        // Выделяем место для параметра в стеке
        int size = paramType.getSize();
        int alignment = paramType.getAlignment();
        allocateVariable(paramName, size, alignment);
        
        symbolTable.insert(paramName, symbol);
    }
    
    if (node.body) {
        node.body->accept(*this);
        calculateStackOffsets(*node.body);
    }
    
    symbolTable.exitScope();
    inFunction = false;
    currentFunctionName = "";
}

void SemanticAnalyzer::visit(StructDeclNode& node) {
    (void)node;  
    symbolTable.enterScope();
    
    symbolTable.exitScope();
}

void SemanticAnalyzer::visit(BlockStmtNode& node) {
    symbolTable.enterScope();
    
    for (auto& stmt : node.statements) {
        stmt->accept(*this);
    }
    
    symbolTable.exitScope();
}

void SemanticAnalyzer::visit(VarDeclStmtNode& node) {
    if (symbolTable.lookupLocal(node.name)) {
        reportError(node.getLine(), node.getColumn(),
            "Повторное объявление переменной '" + node.name + "'");
        return;
    }
    
    auto symbol = std::make_shared<SymbolInfo>(
        node.name, node.varType, SymbolKind::VARIABLE,
        node.getLine(), node.getColumn());
    
    if (node.initializer) {
        node.initializer->accept(*this);
        Type initType = getExpressionType(node.initializer.get());
        
        if (!isAssignable(node.varType, initType)) {
            reportError(node.getLine(), node.getColumn(),
                "Несовместимые типы при инициализации: '" +
                node.varType.toString() + " = " + initType.toString() + "'");
            symbol->type = Type(TypeKind::ERROR);
        } else {
            symbol->initialized = true;
        }
    }
    
    symbolTable.insert(node.name, symbol);
}

void SemanticAnalyzer::visit(IfStmtNode& node) {
    node.condition->accept(*this);
    Type condType = getExpressionType(node.condition.get());
    
    if (!condType.isError() && condType.kind != TypeKind::BOOL) {
        reportError(node.condition->getLine(), node.condition->getColumn(),
            "Условие if должно иметь тип bool, получен: " + condType.toString());
    }
    
    node.thenBranch->accept(*this);
    
    if (node.elseBranch) {
        node.elseBranch->accept(*this);
    }
}

void SemanticAnalyzer::visit(WhileStmtNode& node) {
    node.condition->accept(*this);
    Type condType = getExpressionType(node.condition.get());
    
    if (!condType.isError() && condType.kind != TypeKind::BOOL) {
        reportError(node.condition->getLine(), node.condition->getColumn(),
            "Условие while должно иметь тип bool, получен: " + condType.toString());
    }
    
    node.body->accept(*this);
}

void SemanticAnalyzer::visit(ForStmtNode& node) {
    symbolTable.enterScope();
    
    if (node.init) {
        node.init->accept(*this);
    }
    
    if (node.condition) {
        node.condition->accept(*this);
        Type condType = getExpressionType(node.condition.get());
        
        if (!condType.isError() && condType.kind != TypeKind::BOOL) {
            reportError(node.condition->getLine(), node.condition->getColumn(),
                "Условие for должно иметь тип bool, получен: " + condType.toString());
        }
    }
    
    if (node.update) {
        node.update->accept(*this);
    }
    
    if (node.body) {
        node.body->accept(*this);
    }
    
    symbolTable.exitScope();
}

void SemanticAnalyzer::visit(ReturnStmtNode& node) {
    if (!inFunction) {
        reportError(node.getLine(), node.getColumn(),
            "Оператор return вне функции");
        return;
    }
    
    if (node.value) {
        node.value->accept(*this);
        Type returnType = getExpressionType(node.value.get());
        
        if (returnType.isError()) {
            return;
        }
        
        if (!isTypeCompatible(currentFunctionReturnType, returnType)) {
            reportError(node.getLine(), node.getColumn(),
                "Несовместимый тип возврата: ожидался '" +
                currentFunctionReturnType.toString() + "', получен '" +
                returnType.toString() + "'");
        }
    } else {
        if (currentFunctionReturnType.kind != TypeKind::VOID) {
            reportError(node.getLine(), node.getColumn(),
                "Функция должна возвращать значение типа '" +
                currentFunctionReturnType.toString() + "'");
        }
    }
}

void SemanticAnalyzer::visit(ExprStmtNode& node) {
    if (node.expression) {
        node.expression->accept(*this);
    }
}

void SemanticAnalyzer::visit(LiteralExprNode& node) {
    Type resultType;
    
    if (std::holds_alternative<int>(node.value)) {
        resultType = Type(TypeKind::INT);
    } else if (std::holds_alternative<double>(node.value)) {
        resultType = Type(TypeKind::FLOAT);
    } else if (std::holds_alternative<bool>(node.value)) {
        resultType = Type(TypeKind::BOOL);
    } else if (std::holds_alternative<std::string>(node.value)) {
        resultType = Type(TypeKind::STRING);
    } else {
        resultType = Type(TypeKind::ERROR);
    }
    
    setExpressionType(&node, resultType);
}

void SemanticAnalyzer::visit(IdentifierExprNode& node) {
    auto symbol = symbolTable.lookup(node.name);
    
    if (!symbol) {
        reportError(node.getLine(), node.getColumn(),
            "Необъявленный идентификатор '" + node.name + "'");
        setExpressionType(&node, Type(TypeKind::ERROR));
        return;
    }
    
    setExpressionType(&node, symbol->type);
}

void SemanticAnalyzer::visit(BinaryExprNode& node) {
    node.left->accept(*this);
    node.right->accept(*this);
    
    Type leftType = getExpressionType(node.left.get());
    Type rightType = getExpressionType(node.right.get());
    
    // Для оператора присваивания проверяем, что левая часть - переменная
    if (node.op == BinaryOp::ASSIGN) {
        if (!dynamic_cast<IdentifierExprNode*>(node.left.get()) &&
            !dynamic_cast<MemberAccessExprNode*>(node.left.get()) &&
            !dynamic_cast<IndexExprNode*>(node.left.get())) {
            reportError(node.getLine(), node.getColumn(),
                "Левая часть присваивания должна быть переменной, полем или элементом массива");
            setExpressionType(&node, Type(TypeKind::ERROR));
            return;
        }
    }
    
    auto result = checkBinaryOp(node.op, leftType, rightType, node.getLine(), node.getColumn());
    setExpressionType(&node, result.type);
}

void SemanticAnalyzer::visit(UnaryExprNode& node) {
    node.operand->accept(*this);
    Type operandType = getExpressionType(node.operand.get());
    
    auto result = checkUnaryOp(node.op, operandType, node.getLine(), node.getColumn());
    setExpressionType(&node, result.type);
}

void SemanticAnalyzer::visit(CallExprNode& node) {
    auto symbol = symbolTable.lookup(node.callee);
    
    if (!symbol) {
        reportError(node.getLine(), node.getColumn(),
            "Необъявленная функция '" + node.callee + "'",
            ErrorCode::SEM_UNDECLARED_FUNCTION);
        setExpressionType(&node, Type(TypeKind::ERROR));
        
        // Проверяем аргументы даже при ошибке
        for (auto& arg : node.arguments) {
            arg->accept(*this);
        }
        return;
    }
    
    if (symbol->kind != SymbolKind::FUNCTION) {
        reportError(node.getLine(), node.getColumn(),
            "'" + node.callee + "' не является функцией");
        setExpressionType(&node, Type(TypeKind::ERROR));
        return;
    }
    
    // Проверяем количество аргументов
    if (node.arguments.size() != symbol->parameterTypes.size()) {
        reportError(node.getLine(), node.getColumn(),
            "Неверное количество аргументов: ожидалось " +
            std::to_string(symbol->parameterTypes.size()) +
            ", получено " + std::to_string(node.arguments.size()));
    }
    
    // Проверяем типы аргументов
    size_t minArgs = std::min(node.arguments.size(), symbol->parameterTypes.size());
    for (size_t i = 0; i < minArgs; i++) {
        node.arguments[i]->accept(*this);
        Type argType = getExpressionType(node.arguments[i].get());
        
        if (!argType.isError() && !isTypeCompatible(symbol->parameterTypes[i], argType)) {
            reportError(node.arguments[i]->getLine(), node.arguments[i]->getColumn(),
                "Несовместимый тип аргумента " + std::to_string(i + 1) +
                ": ожидался '" + symbol->parameterTypes[i].toString() +
                "', получен '" + argType.toString() + "'");
        }
    }
    
    // Проверяем оставшиеся аргументы (если их больше)
    for (size_t i = minArgs; i < node.arguments.size(); i++) {
        node.arguments[i]->accept(*this);
    }
    
    Type returnType = symbol->returnType.value_or(Type(TypeKind::VOID));
    setExpressionType(&node, returnType);
}

void SemanticAnalyzer::visit(IndexExprNode& node) {
    node.array->accept(*this);
    node.index->accept(*this);
    
    Type arrayType = getExpressionType(node.array.get());
    Type indexType = getExpressionType(node.index.get());
    
    // Упрощенная проверка для индексации
    if (!arrayType.isError() && arrayType.kind != TypeKind::ARRAY) {
        reportError(node.getLine(), node.getColumn(),
            "Индексация применима только к массивам");
        setExpressionType(&node, Type(TypeKind::ERROR));
        return;
    }
    
    if (!indexType.isError() && indexType.kind != TypeKind::INT) {
        reportError(node.getLine(), node.getColumn(),
            "Индекс массива должен быть целым числом");
    }
    
    // Возвращаем тип элемента массива
    if (arrayType.kind == TypeKind::ARRAY && arrayType.elementType) {
        setExpressionType(&node, *arrayType.elementType);
    } else {
        setExpressionType(&node, Type(TypeKind::ERROR));
    }
}

void SemanticAnalyzer::visit(MemberAccessExprNode& node) {
    node.object->accept(*this);
    Type objectType = getExpressionType(node.object.get());
    
    if (objectType.isError()) {
        setExpressionType(&node, Type(TypeKind::ERROR));
        return;
    }
    
    if (objectType.kind != TypeKind::STRUCT) {
        reportError(node.getLine(), node.getColumn(),
            "Доступ к полям возможен только для структур, получен тип: " + objectType.toString());
        setExpressionType(&node, Type(TypeKind::ERROR));
        return;
    }
    
    auto structSymbol = symbolTable.lookup(objectType.structName);
    
    if (!structSymbol) {
        reportError(node.getLine(), node.getColumn(),
            "Структура '" + objectType.structName + "' не объявлена");
        setExpressionType(&node, Type(TypeKind::ERROR));
        return;
    }
    
    auto fieldIt = structSymbol->fields.find(node.member);
    if (fieldIt == structSymbol->fields.end()) {
        reportError(node.getLine(), node.getColumn(),
            "Поле '" + node.member + "' не найдено в структуре '" + objectType.structName + "'");
        setExpressionType(&node, Type(TypeKind::ERROR));
        return;
    }
    
    setExpressionType(&node, fieldIt->second);
}

Type SemanticAnalyzer::getArrayElementType(const Type& arrayType) {
    if (arrayType.kind == TypeKind::ARRAY && arrayType.elementType) {
        return *arrayType.elementType;
    }
    return Type(TypeKind::ERROR);
}

TypeCheckResult SemanticAnalyzer::checkArrayAccess(const Type& arrayType, const Type& indexType, int line, int col) {
    if (arrayType.isError() || indexType.isError()) {
        return TypeCheckResult(Type(TypeKind::ERROR));
    }
    
    if (arrayType.kind != TypeKind::ARRAY) {
        std::stringstream ss;
        ss << "Индексация применима только к массивам, получен тип: " << arrayType.toString();
        reportError(line, col, ss.str());
        return TypeCheckResult(Type(TypeKind::ERROR));
    }
    
    if (indexType.kind != TypeKind::INT) {
        std::stringstream ss;
        ss << "Индекс массива должен быть целым числом, получен: " << indexType.toString();
        reportError(line, col, ss.str());
        return TypeCheckResult(Type(TypeKind::ERROR));
    }
    
    Type elementType = arrayType.getElementType();
    return TypeCheckResult(elementType);
}

} // namespace minicompiler