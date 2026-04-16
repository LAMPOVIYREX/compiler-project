#pragma once
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include "parser/AST.hpp"
#include "parser/ASTVisitor.hpp"
#include "semantic/SymbolTable.hpp"
#include "utils/ErrorReporter.hpp"
#include "utils/ErrorCodes.hpp"

namespace minicompiler {

// Тип для результатов проверки выражений
struct TypeCheckResult {
    Type type;
    bool success;
    std::string error;
    
    TypeCheckResult() : type(Type(TypeKind::VOID)), success(true) {}
    TypeCheckResult(const Type& t) : type(t), success(false) {}
    TypeCheckResult(const std::string& err) : type(Type(TypeKind::VOID)), success(false), error(err) {}
};

class SemanticAnalyzer : public ASTVisitor {
public:
    SemanticAnalyzer(ErrorReporter& errorReporter);
    
    // Основной метод анализа
    bool analyze(ProgramNode& program);
    
    // Получение результатов
    SymbolTable& getSymbolTable() { return symbolTable; }
    bool hasErrors() const { return errorCount > 0; }
    int getErrorCount() const { return errorCount; }
    
    // Memory Layout
    void printMemoryLayout() const;
    
    // Получение типов выражений
    const std::unordered_map<ASTNode*, Type>& getExprTypes() const { return exprTypes; }
    
    // ТОЛЬКО ОДИН РАЗ! Удалите дубликат, если он есть
    static Type getErrorType() {
        return Type(TypeKind::ERROR);
    }
    
    // AST Visitor методы
    void visit(ProgramNode& node) override;
    void visit(FunctionDeclNode& node) override;
    void visit(StructDeclNode& node) override;
    void visit(BlockStmtNode& node) override;
    void visit(VarDeclStmtNode& node) override;
    void visit(IfStmtNode& node) override;
    void visit(WhileStmtNode& node) override;
    void visit(ForStmtNode& node) override;
    void visit(ReturnStmtNode& node) override;
    void visit(ExprStmtNode& node) override;
    
    void visit(LiteralExprNode& node) override;
    void visit(IdentifierExprNode& node) override;
    void visit(BinaryExprNode& node) override;
    void visit(UnaryExprNode& node) override;
    void visit(CallExprNode& node) override;
    void visit(IndexExprNode& node) override;
    void visit(MemberAccessExprNode& node) override;
    
private:
    ErrorReporter& errorReporter;
    SymbolTable symbolTable;
    int errorCount;
    
    // Контекст текущей функции
    std::string currentFunctionName;
    Type currentFunctionReturnType;
    bool inFunction;
    
    // Текущая область видимости
    std::vector<std::string> scopeStack;
    
    // Типы выражений (для декорирования AST)
    std::unordered_map<ASTNode*, Type> exprTypes;
    
    // Memory Layout
    int currentStackOffset = 0;
    std::vector<int> stackOffsetStack;
    
    // Вспомогательные методы
    void reportError(int line, int column, const std::string& message);
    void reportError(const std::string& message);
    void reportError(int line, int column, const std::string& message, ErrorCode code);
    
    // Проверка типов
    TypeCheckResult checkBinaryOp(BinaryOp op, const Type& left, const Type& right, int line, int col);
    TypeCheckResult checkUnaryOp(UnaryOp op, const Type& operand, int line, int col);
    TypeCheckResult checkArrayAccess(const Type& arrayType, const Type& indexType, int line, int col);
    bool isTypeCompatible(const Type& expected, const Type& actual);
    bool isAssignable(const Type& target, const Type& source);
    
    // Вычисление типов
    Type getExpressionType(ExpressionNode* expr);
    void setExpressionType(ExpressionNode* expr, const Type& type);
    Type getArrayElementType(const Type& arrayType);
    
    // Memory Layout методы
    int align(int offset, int alignment);
    void pushStackOffset();
    void popStackOffset();
    void allocateVariable(const std::string& name, int size, int alignment);
    void calculateStackOffsets(BlockStmtNode& node);
    
    // Проверка семантики
    void checkFunctionCall(CallExprNode& node);
    void checkVariableDeclaration(VarDeclStmtNode& node);
    void checkAssignment(BinaryExprNode& node);
};

} // namespace minicompiler