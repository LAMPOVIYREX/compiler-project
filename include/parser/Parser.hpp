#pragma once
#include <vector>
#include <memory>
#include <string>
#include "lexer/Token.hpp"
#include "parser/AST.hpp"
#include "utils/ErrorReporter.hpp"

namespace minicompiler {

enum class ASTFormat {
    TEXT,
    DOT,
    JSON
};



class Parser {
public:
    Parser(const std::vector<Token>& tokens, ErrorReporter& errorReporter);
    
    // Основной метод парсинга
    std::unique_ptr<ProgramNode> parse();
    
    // Вспомогательные методы для отладки
    bool hadError() const { return errorReporter.hasErrors(); }
    const std::vector<Token>& getTokens() const { return tokens; }
    
    // Статический метод для парсинга из строки (для тестов)
    static std::unique_ptr<ProgramNode> parseFromString(
        const std::string& source, 
        ErrorReporter& errorReporter
    );

private:
    std::vector<Token> tokens;
    size_t current;
    ErrorReporter& errorReporter;
    
    // Основные методы разбора
    std::unique_ptr<ProgramNode> parseProgram();
    std::unique_ptr<DeclarationNode> parseDeclaration();
    std::unique_ptr<FunctionDeclNode> parseFunctionDeclaration();
    std::unique_ptr<StructDeclNode> parseStructDeclaration();
    std::unique_ptr<VarDeclStmtNode> parseVariableDeclaration(bool requireSemicolon = true);
    
    std::unique_ptr<StatementNode> parseStatement();
    std::unique_ptr<BlockStmtNode> parseBlock();
    std::unique_ptr<IfStmtNode> parseIfStatement();
    std::unique_ptr<WhileStmtNode> parseWhileStatement();
    std::unique_ptr<ForStmtNode> parseForStatement();
    std::unique_ptr<ReturnStmtNode> parseReturnStatement();
    std::unique_ptr<ExprStmtNode> parseExpressionStatement();
    
    std::unique_ptr<ExpressionNode> parseExpression();
    std::unique_ptr<ExpressionNode> parseAssignment();
    std::unique_ptr<ExpressionNode> parseLogicalOr();
    std::unique_ptr<ExpressionNode> parseLogicalAnd();
    std::unique_ptr<ExpressionNode> parseEquality();
    std::unique_ptr<ExpressionNode> parseRelational();
    std::unique_ptr<ExpressionNode> parseAdditive();
    std::unique_ptr<ExpressionNode> parseMultiplicative();
    std::unique_ptr<ExpressionNode> parseUnary();
    std::unique_ptr<ExpressionNode> parsePostfix();
    std::unique_ptr<ExpressionNode> parsePrimary();
    std::unique_ptr<CallExprNode> parseCall(const std::string& callee);
    
    Type parseType();
    
    // Вспомогательные методы
    Token peek() const;
    Token previous() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool isAtEnd() const;
    
    Token consume(TokenType type, const std::string& message);
    void synchronize();
    
    // ДОБАВЛЕНО: метод для проверки конца блока
    bool isAtBlockEnd() const;
    
    // Обработка ошибок 
    void error(const Token& token, const std::string& message);
    void error(const std::string& message);
};

} // namespace minicompiler