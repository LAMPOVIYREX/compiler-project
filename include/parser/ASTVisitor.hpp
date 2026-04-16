#pragma once
#include "AST.hpp"

namespace minicompiler {

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    
    // Expression nodes
    virtual void visit(LiteralExprNode& node) = 0;
    virtual void visit(IdentifierExprNode& node) = 0;
    virtual void visit(BinaryExprNode& node) = 0;
    virtual void visit(UnaryExprNode& node) = 0;
    virtual void visit(CallExprNode& node) = 0;
    virtual void visit(IndexExprNode& node) = 0;           
    virtual void visit(MemberAccessExprNode& node) = 0; 
    
    // Statement nodes
    virtual void visit(BlockStmtNode& node) = 0;
    virtual void visit(ExprStmtNode& node) = 0;
    virtual void visit(VarDeclStmtNode& node) = 0;
    virtual void visit(IfStmtNode& node) = 0;
    virtual void visit(WhileStmtNode& node) = 0;
    virtual void visit(ForStmtNode& node) = 0;
    virtual void visit(ReturnStmtNode& node) = 0;
    
    // Declaration nodes
    virtual void visit(FunctionDeclNode& node) = 0;
    virtual void visit(StructDeclNode& node) = 0;
    virtual void visit(ProgramNode& node) = 0;
};

} // namespace minicompiler