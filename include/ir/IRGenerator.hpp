#pragma once
#include <memory>
#include <unordered_map>
#include <stack>
#include "ir/IR.hpp"
#include "parser/AST.hpp"
#include "parser/ASTVisitor.hpp"  // ВАЖНО!
#include "semantic/SymbolTable.hpp"
#include "utils/ErrorReporter.hpp"

namespace minicompiler {

class IRGenerator : public ASTVisitor {
public:
    IRGenerator(SymbolTable& symbolTable, ErrorReporter& errorReporter);
    
    std::unique_ptr<IRProgram> generate(ProgramNode& program);
    
    IRProgram* getProgram() const { return program.get(); }
    bool hasErrors() const { return errorCount > 0; }
    
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
    SymbolTable& symbolTable;
    ErrorReporter& errorReporter;
    std::unique_ptr<IRProgram> program;
    IRFunction* currentFunction;
    BasicBlock* currentBlock;
    int errorCount;

    
    std::unordered_map<std::string, IROperand> varMap;
    std::stack<IROperand> valueStack;
    
    void reportError(int line, int column, const std::string& message);
    
    IROperand generateExpression(ExpressionNode* expr);
    IROperand generateBinaryOp(BinaryOp op, const IROperand& left, const IROperand& right, int line);
    IROperand generateUnaryOp(UnaryOp op, const IROperand& operand, int line);
    
    void emitJump(const std::string& target);
    void emitCondJump(const IROperand& cond, const std::string& trueTarget, const std::string& falseTarget);
    void emitLabel(const std::string& name);
    
    void emit(IROpcode op, const IROperand& dest = IROperand(),
              const IROperand& src1 = IROperand(),
              const IROperand& src2 = IROperand(),
              const std::string& comment = "");
    
    IROperand newTemp(IRType type = IRType::INT);
    
    IRType convertType(const Type& type);
    IROperand convertOperand(const IROperand& operand, IRType targetType);
    
    void dumpIR();

    Type getExpressionType(ExpressionNode* expr) {
        (void)expr;  
        return Type(TypeKind::INT);  
    }   
};

} // namespace minicompiler