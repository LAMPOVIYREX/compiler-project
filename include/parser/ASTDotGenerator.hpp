#pragma once
#include "ASTVisitor.hpp"
#include <ostream>
#include <sstream>
#include <string>
#include <stack>
#include <unordered_map>

namespace minicompiler {

class ASTDotGenerator : public ASTVisitor {
public:
    explicit ASTDotGenerator(std::ostream& output, bool showTypes = true);
    
    std::string getResult() const { return result.str(); }
    void clear();
    void setShowTypes(bool show) { showTypes = show; }
    
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

    void setNodeTypes(const std::unordered_map<ASTNode*, Type>& types) {
        nodeTypes = types;
    }

private:
    std::ostream& output;
    std::stringstream result;
    int nodeCounter;
    std::stack<int> parentStack;
    bool showTypes;
    std::unordered_map<ASTNode*, Type> nodeTypes;  
    
    int createNode(const std::string& label, const std::string& shape = "box", 
                   const std::string& color = "black");
    void connectNodes(int fromId, int toId, const std::string& label = "");
    std::string escapeString(const std::string& str);
    std::string literalValueToString(const LiteralValue& value);
    std::string binaryOpToString(BinaryOp op);
    std::string unaryOpToString(UnaryOp op);
    std::string typeToString(const Type& type);
    std::string getNodeColor(ASTNode& node);
    std::string getTypeLabel(const Type& type);
};

} // namespace minicompiler