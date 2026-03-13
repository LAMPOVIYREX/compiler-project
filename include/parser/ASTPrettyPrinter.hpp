#pragma once
#include "ASTVisitor.hpp"
#include <ostream>
#include <sstream>
#include <string>

namespace minicompiler {

class ASTPrettyPrinter : public ASTVisitor {
public:
    explicit ASTPrettyPrinter(std::ostream& output, int indentSize = 2);
    
    std::string getResult() const { return result.str(); }
    void clear();
    
    // Expression nodes
    void visit(LiteralExprNode& node) override;
    void visit(IdentifierExprNode& node) override;
    void visit(BinaryExprNode& node) override;
    void visit(UnaryExprNode& node) override;
    void visit(CallExprNode& node) override;
    void visit(IndexExprNode& node) override;
    void visit(MemberAccessExprNode& node) override;
    
    // Statement nodes
    void visit(BlockStmtNode& node) override;
    void visit(ExprStmtNode& node) override;
    void visit(VarDeclStmtNode& node) override;
    void visit(IfStmtNode& node) override;
    void visit(WhileStmtNode& node) override;
    void visit(ForStmtNode& node) override;
    void visit(ReturnStmtNode& node) override;
    
    // Declaration nodes
    void visit(FunctionDeclNode& node) override;
    void visit(StructDeclNode& node) override;
    void visit(ProgramNode& node) override;

private:
    std::ostream& output;
    std::stringstream result;
    int indentLevel;
    int indentSize;
    
    void indent();
    void increaseIndent() { indentLevel++; }
    void decreaseIndent() { indentLevel--; }
    
    std::string nodeLocation(ASTNode& node);
    std::string binaryOpToString(BinaryOp op);
    std::string unaryOpToString(UnaryOp op);
    std::string typeToString(const Type& type);
    std::string literalValueToString(const LiteralValue& value);
};

} // namespace minicompiler