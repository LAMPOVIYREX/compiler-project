#pragma once
#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <iostream>
#include "lexer/Token.hpp"

namespace minicompiler {

// Forward declarations
class ASTVisitor;

//=============================================================================
// Base AST Node
//=============================================================================

class ASTNode {
public:
    ASTNode(int line, int column) : line(line), column(column) {}
    virtual ~ASTNode() = default;
    
    virtual void accept(ASTVisitor& visitor) = 0;
    virtual std::string toString() const = 0;
    
    int getLine() const { return line; }
    int getColumn() const { return column; }
    
protected:
    int line;
    int column;
};

//=============================================================================
// Types
//=============================================================================

enum class TypeKind {
    INT,
    FLOAT,
    BOOL,
    STRING,
    STRUCT,
    VOID  // For function return types only
};

struct Type {
    TypeKind kind;
    std::string structName;  // For STRUCT kind
    
    Type(TypeKind k) : kind(k) {}
    Type(const std::string& name) : kind(TypeKind::STRUCT), structName(name) {}
    
    std::string toString() const {
        switch (kind) {
            case TypeKind::INT: return "int";
            case TypeKind::FLOAT: return "float";
            case TypeKind::BOOL: return "bool";
            case TypeKind::STRING: return "string";
            case TypeKind::VOID: return "void";
            case TypeKind::STRUCT: return "struct " + structName;
        }
        return "unknown";
    }
    
    bool operator==(const Type& other) const {
        if (kind != other.kind) return false;
        if (kind == TypeKind::STRUCT) return structName == other.structName;
        return true;
    }
    
    bool operator!=(const Type& other) const {
        return !(*this == other);
    }
};

//=============================================================================
// Expression Nodes
//=============================================================================

class ExpressionNode : public ASTNode {
public:
    ExpressionNode(int line, int column) : ASTNode(line, column) {}
};

class LiteralExprNode : public ExpressionNode {
public:
    LiteralValue value;
    
    LiteralExprNode(int line, int column, const LiteralValue& val)
        : ExpressionNode(line, column), value(val) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class IdentifierExprNode : public ExpressionNode {
public:
    std::string name;
    
    IdentifierExprNode(int line, int column, const std::string& n)
        : ExpressionNode(line, column), name(n) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

enum class BinaryOp {
    ADD, SUB, MUL, DIV, MOD,
    EQ, NE, LT, LE, GT, GE,
    AND, OR,
    ASSIGN, ADD_ASSIGN, SUB_ASSIGN, MUL_ASSIGN, DIV_ASSIGN, MOD_ASSIGN
};

std::string binaryOpToString(BinaryOp op);

class BinaryExprNode : public ExpressionNode {
public:
    std::unique_ptr<ExpressionNode> left;
    BinaryOp op;
    std::unique_ptr<ExpressionNode> right;
    
    BinaryExprNode(int line, int column, 
                   std::unique_ptr<ExpressionNode> l, BinaryOp o, 
                   std::unique_ptr<ExpressionNode> r)
        : ExpressionNode(line, column), left(std::move(l)), op(o), right(std::move(r)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

enum class UnaryOp {
    NEG, NOT, PRE_INC, PRE_DEC, POST_INC, POST_DEC
};

std::string unaryOpToString(UnaryOp op);

class UnaryExprNode : public ExpressionNode {
public:
    UnaryOp op;
    std::unique_ptr<ExpressionNode> operand;
    
    UnaryExprNode(int line, int column, UnaryOp o, std::unique_ptr<ExpressionNode> opnd)
        : ExpressionNode(line, column), op(o), operand(std::move(opnd)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class CallExprNode : public ExpressionNode {
public:
    std::string callee;
    std::vector<std::unique_ptr<ExpressionNode>> arguments;
    
    CallExprNode(int line, int column, const std::string& c)
        : ExpressionNode(line, column), callee(c) {}
    
    void addArgument(std::unique_ptr<ExpressionNode> arg) {
        arguments.push_back(std::move(arg));
    }
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class IndexExprNode : public ExpressionNode {
public:
    std::unique_ptr<ExpressionNode> array;
    std::unique_ptr<ExpressionNode> index;
    
    IndexExprNode(int line, int column, 
                  std::unique_ptr<ExpressionNode> arr,
                  std::unique_ptr<ExpressionNode> idx)
        : ExpressionNode(line, column), array(std::move(arr)), index(std::move(idx)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

// Добавить после IndexExprNode
class MemberAccessExprNode : public ExpressionNode {
public:
    std::unique_ptr<ExpressionNode> object;
    std::string member;
    
    MemberAccessExprNode(int line, int column, 
                        std::unique_ptr<ExpressionNode> obj,
                        const std::string& mem)
        : ExpressionNode(line, column), object(std::move(obj)), member(mem) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

//=============================================================================
// Statement Nodes
//=============================================================================

class StatementNode : public ASTNode {
public:
    StatementNode(int line, int column) : ASTNode(line, column) {}
};

class BlockStmtNode : public StatementNode {
public:
    std::vector<std::unique_ptr<StatementNode>> statements;
    
    BlockStmtNode(int line, int column) : StatementNode(line, column) {}
    
    void addStatement(std::unique_ptr<StatementNode> stmt) {
        statements.push_back(std::move(stmt));
    }
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class ExprStmtNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> expression;
    
    ExprStmtNode(int line, int column, std::unique_ptr<ExpressionNode> expr)
        : StatementNode(line, column), expression(std::move(expr)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class VarDeclStmtNode : public StatementNode {
public:
    Type varType;
    std::string name;
    std::unique_ptr<ExpressionNode> initializer;
    
    VarDeclStmtNode(int line, int column, const Type& t, const std::string& n)
        : StatementNode(line, column), varType(t), name(n) {}
    
    VarDeclStmtNode(int line, int column, const Type& t, const std::string& n,
                    std::unique_ptr<ExpressionNode> init)
        : StatementNode(line, column), varType(t), name(n), initializer(std::move(init)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class IfStmtNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> condition;
    std::unique_ptr<StatementNode> thenBranch;
    std::unique_ptr<StatementNode> elseBranch;
    
    IfStmtNode(int line, int column,
               std::unique_ptr<ExpressionNode> cond,
               std::unique_ptr<StatementNode> thenStmt)
        : StatementNode(line, column), condition(std::move(cond)), 
          thenBranch(std::move(thenStmt)) {}
    
    void setElseBranch(std::unique_ptr<StatementNode> elseStmt) {
        elseBranch = std::move(elseStmt);
    }
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class WhileStmtNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> condition;
    std::unique_ptr<StatementNode> body;
    
    WhileStmtNode(int line, int column,
                  std::unique_ptr<ExpressionNode> cond,
                  std::unique_ptr<StatementNode> b)
        : StatementNode(line, column), condition(std::move(cond)), body(std::move(b)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class ForStmtNode : public StatementNode {
public:
    std::unique_ptr<StatementNode> init;
    std::unique_ptr<ExpressionNode> condition;
    std::unique_ptr<ExpressionNode> update;
    std::unique_ptr<StatementNode> body;
    
    ForStmtNode(int line, int column)
        : StatementNode(line, column) {}
    
    void setInit(std::unique_ptr<StatementNode> i) { init = std::move(i); }
    void setCondition(std::unique_ptr<ExpressionNode> c) { condition = std::move(c); }
    void setUpdate(std::unique_ptr<ExpressionNode> u) { update = std::move(u); }
    void setBody(std::unique_ptr<StatementNode> b) { body = std::move(b); }
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class ReturnStmtNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> value;
    
    ReturnStmtNode(int line, int column) : StatementNode(line, column) {}
    ReturnStmtNode(int line, int column, std::unique_ptr<ExpressionNode> val)
        : StatementNode(line, column), value(std::move(val)) {}
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

//=============================================================================
// Declaration Nodes
//=============================================================================

class DeclarationNode : public ASTNode {
public:
    DeclarationNode(int line, int column) : ASTNode(line, column) {}
};

class FunctionDeclNode : public DeclarationNode {
public:
    Type returnType;
    std::string name;
    std::vector<std::pair<Type, std::string>> parameters;
    std::unique_ptr<BlockStmtNode> body;
    
    FunctionDeclNode(int line, int column, const Type& retType, const std::string& n)
        : DeclarationNode(line, column), returnType(retType), name(n) {}
    
    void addParameter(const Type& type, const std::string& paramName) {
        parameters.emplace_back(type, paramName);
    }
    
    void setBody(std::unique_ptr<BlockStmtNode> b) {
        body = std::move(b);
    }
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class StructDeclNode : public DeclarationNode {
public:
    std::string name;
    std::vector<std::unique_ptr<VarDeclStmtNode>> fields;
    
    StructDeclNode(int line, int column, const std::string& n)
        : DeclarationNode(line, column), name(n) {}
    
    void addField(std::unique_ptr<VarDeclStmtNode> field) {
        fields.push_back(std::move(field));
    }
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

class ProgramNode : public ASTNode {
public:
    std::vector<std::unique_ptr<DeclarationNode>> declarations;
    
    ProgramNode(int line, int column) : ASTNode(line, column) {}
    
    void addDeclaration(std::unique_ptr<DeclarationNode> decl) {
        declarations.push_back(std::move(decl));
    }
    
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override;
};

} // namespace minicompiler