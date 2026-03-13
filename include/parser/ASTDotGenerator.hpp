#pragma once
#include "ASTVisitor.hpp"
#include <ostream>
#include <sstream>
#include <string>
#include <stack>

namespace minicompiler {

class ASTDotGenerator : public ASTVisitor {
public:
    explicit ASTDotGenerator(std::ostream& output);
    
    std::string getResult() const { return result.str(); }
    void clear();
    
    // Expression nodes
    void visit(LiteralExprNode& node) override;
    void visit(IdentifierExprNode& node) override;
    void visit(BinaryExprNode& node) override;
    void visit(UnaryExprNode& node) override;
    void visit(CallExprNode& node) override;
    void visit(IndexExprNode& node) override;
    void visit(MemberAccessExprNode& node) override;  // ДОБАВЛЕНО
    
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
    int nodeCounter;
    std::stack<int> parentStack;
    
    int createNode(const std::string& label, const std::string& shape = "box", 
                   const std::string& color = "black");
    void connectNodes(int fromId, int toId, const std::string& label = "");
    std::string escapeString(const std::string& str);
    std::string literalValueToString(const LiteralValue& value);
    std::string binaryOpToString(BinaryOp op);
    std::string unaryOpToString(UnaryOp op);
    std::string typeToString(const Type& type);
    std::string getNodeColor(ASTNode& node);
};

} // namespace minicompiler