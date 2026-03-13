#include "parser/AST.hpp"
#include "parser/ASTVisitor.hpp"
#include <sstream>
#include <iomanip>

namespace minicompiler {

//=============================================================================
// Вспомогательные функции
//=============================================================================

std::string binaryOpToString(BinaryOp op) {
    switch (op) {
        case BinaryOp::ADD: return "+";
        case BinaryOp::SUB: return "-";
        case BinaryOp::MUL: return "*";
        case BinaryOp::DIV: return "/";
        case BinaryOp::MOD: return "%";
        case BinaryOp::EQ: return "==";
        case BinaryOp::NE: return "!=";
        case BinaryOp::LT: return "<";
        case BinaryOp::LE: return "<=";
        case BinaryOp::GT: return ">";
        case BinaryOp::GE: return ">=";
        case BinaryOp::AND: return "&&";
        case BinaryOp::OR: return "||";
        case BinaryOp::ASSIGN: return "=";
        case BinaryOp::ADD_ASSIGN: return "+=";
        case BinaryOp::SUB_ASSIGN: return "-=";
        case BinaryOp::MUL_ASSIGN: return "*=";
        case BinaryOp::DIV_ASSIGN: return "/=";
        case BinaryOp::MOD_ASSIGN: return "%=";
    }
    return "unknown";
}

std::string unaryOpToString(UnaryOp op) {
    switch (op) {
        case UnaryOp::NEG: return "-";
        case UnaryOp::NOT: return "!";
        case UnaryOp::PRE_INC: return "++";
        case UnaryOp::PRE_DEC: return "--";
        case UnaryOp::POST_INC: return "++ (post)";
        case UnaryOp::POST_DEC: return "-- (post)";
    }
    return "unknown";
}

//=============================================================================
// Expression Nodes
//=============================================================================

void LiteralExprNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string LiteralExprNode::toString() const {
    std::stringstream ss;
    if (std::holds_alternative<int>(value)) {
        ss << "Literal(" << std::get<int>(value) << ")";
    } else if (std::holds_alternative<double>(value)) {
        ss << "Literal(" << std::fixed << std::setprecision(6) 
           << std::get<double>(value) << ")";
    } else if (std::holds_alternative<bool>(value)) {
        ss << "Literal(" << (std::get<bool>(value) ? "true" : "false") << ")";
    } else if (std::holds_alternative<std::string>(value)) {
        ss << "Literal(\"" << std::get<std::string>(value) << "\")";
    } else {
        ss << "Literal(null)";
    }
    return ss.str();
}

void IdentifierExprNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string IdentifierExprNode::toString() const {
    return "Identifier(" + name + ")";
}

void BinaryExprNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string BinaryExprNode::toString() const {
    return "BinaryOp(" + binaryOpToString(op) + ")";
}

void UnaryExprNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string UnaryExprNode::toString() const {
    return "UnaryOp(" + unaryOpToString(op) + ")";
}

void CallExprNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string CallExprNode::toString() const {
    return "Call(" + callee + ")";
}

void IndexExprNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string IndexExprNode::toString() const {
    return "IndexOp";
}

//=============================================================================
// Statement Nodes
//=============================================================================

void BlockStmtNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string BlockStmtNode::toString() const {
    return "Block";
}

void ExprStmtNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string ExprStmtNode::toString() const {
    return "ExprStmt";
}

void VarDeclStmtNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string VarDeclStmtNode::toString() const {
    return "VarDecl(" + varType.toString() + " " + name + ")";
}

void IfStmtNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string IfStmtNode::toString() const {
    if (elseBranch) {
        return "IfStmt (with else)";
    }
    return "IfStmt";
}

void WhileStmtNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string WhileStmtNode::toString() const {
    return "WhileStmt";
}

void ForStmtNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string ForStmtNode::toString() const {
    return "ForStmt";
}

void ReturnStmtNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string ReturnStmtNode::toString() const {
    if (value) {
        return "ReturnStmt";
    }
    return "ReturnStmt (void)";
}

//=============================================================================
// Declaration Nodes
//=============================================================================

void FunctionDeclNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string FunctionDeclNode::toString() const {
    return "Function(" + returnType.toString() + " " + name + ")";
}

void StructDeclNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string StructDeclNode::toString() const {
    return "Struct(" + name + ")";
}

void ProgramNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void MemberAccessExprNode::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

std::string MemberAccessExprNode::toString() const {
    return "MemberAccess(." + member + ")";
}

std::string ProgramNode::toString() const {
    return "Program";
}

} // namespace minicompiler