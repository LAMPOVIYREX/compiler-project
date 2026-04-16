#include "parser/Parser.hpp"
#include "lexer/Scanner.hpp"
#include <iostream>
#include <sstream>

namespace minicompiler {

Parser::Parser(const std::vector<Token>& tokens, ErrorReporter& errorReporter)
    : tokens(tokens), current(0), errorReporter(errorReporter) {}

//=============================================================================
// Статический метод для тестирования
//=============================================================================

std::unique_ptr<ProgramNode> Parser::parseFromString(
    const std::string& source, 
    ErrorReporter& errorReporter) {
    
    Scanner scanner(source, errorReporter);
    auto tokens = scanner.scanTokens();
    
    if (errorReporter.hasErrors()) {
        return nullptr;
    }
    
    Parser parser(tokens, errorReporter);
    return parser.parse();
}

//=============================================================================
// Обработка ошибок
//=============================================================================

void Parser::error(const Token& token, const std::string& message) {
    errorReporter.reportGeneralError(token.line, token.column, message);
}

void Parser::error(const std::string& message) {
    Token token = peek();
    error(token, message);
}

//=============================================================================
// Основные методы парсинга
//=============================================================================

std::unique_ptr<ProgramNode> Parser::parse() {
    auto program = std::make_unique<ProgramNode>(1, 1);
    int maxIterations = 10000;
    int iterations = 0;
    
    while (!isAtEnd() && iterations < maxIterations) {
        iterations++;
        
        // Пропускаем лишние токены (например, пустые строки)
        while (!isAtEnd() && (peek().type == TokenType::SEMICOLON || 
                              peek().type == TokenType::END_OF_FILE)) {
            advance();
        }
        
        if (isAtEnd()) break;
        
        size_t oldPos = current;
        auto decl = parseDeclaration();
        
        if (decl) {
            program->addDeclaration(std::move(decl));
        } else {
            // Если не удалось распарсить, пропускаем один токен
            if (oldPos == current && !isAtEnd()) {
                advance();
            }
        }
    }
    
    return program;
}

std::unique_ptr<DeclarationNode> Parser::parseDeclaration() {
    if (match(TokenType::KW_FN)) {
        return parseFunctionDeclaration();
    }
    if (match(TokenType::KW_STRUCT)) {
        return parseStructDeclaration();
    }
    
    
    synchronize();
    return nullptr;
}

std::unique_ptr<FunctionDeclNode> Parser::parseFunctionDeclaration() {
    Token fnToken = previous();
    
    // Имя функции
    if (!check(TokenType::IDENTIFIER)) {
        errorReporter.reportExpectedIdentifier(peek().line, peek().column, peek().lexeme);
        return nullptr;
    }
    Token nameToken = advance();
    std::string funcName = nameToken.lexeme;
    
    // Открывающая скобка
    if (!match(TokenType::LPAREN)) {
        errorReporter.reportMissingParen(peek().line, peek().column);
        return nullptr;
    }
    
    auto funcDecl = std::make_unique<FunctionDeclNode>(
        fnToken.line, fnToken.column,
        Type(TypeKind::VOID),
        funcName
    );
    
    // Параметры
    if (!check(TokenType::RPAREN)) {
        do {
            Type paramType = parseType();
            if (paramType.kind == TypeKind::VOID) {
                errorReporter.reportGeneralError(peek().line, peek().column, 
                    "Параметр не может иметь тип void");
                break;
            }
            
            if (!check(TokenType::IDENTIFIER)) {
                errorReporter.reportExpectedIdentifier(peek().line, peek().column, peek().lexeme);
                break;
            }
            Token paramName = advance();
            
            funcDecl->addParameter(paramType, paramName.lexeme);
            
        } while (match(TokenType::COMMA) && !isAtEnd());
    }
    
    // Закрывающая скобка
    if (!match(TokenType::RPAREN)) {
        errorReporter.reportMissingParen(peek().line, peek().column);
        while (!isAtEnd() && !check(TokenType::LBRACE) && !check(TokenType::ARROW)) {
            advance();
        }
    }
    
    // Возвращаемый тип
    if (match(TokenType::ARROW)) {
        Type returnType = parseType();
        funcDecl->returnType = returnType;
    }
    
    // Тело функции
    if (!check(TokenType::LBRACE)) {
        errorReporter.reportMissingBrace(peek().line, peek().column);
        return funcDecl;
    }
    
    auto body = parseBlock();
    if (body) {
        funcDecl->setBody(std::move(body));
    }
    
    return funcDecl;
}

std::unique_ptr<StructDeclNode> Parser::parseStructDeclaration() {
    Token structToken = previous();
    
    if (!check(TokenType::IDENTIFIER)) {
        errorReporter.reportExpectedIdentifier(peek().line, peek().column, peek().lexeme);
        return nullptr;
    }
    Token nameToken = advance();
    std::string structName = nameToken.lexeme;
    
    if (!match(TokenType::LBRACE)) {
        errorReporter.reportMissingBrace(peek().line, peek().column);
        return nullptr;
    }
    
    auto structDecl = std::make_unique<StructDeclNode>(
        structToken.line, structToken.column,
        structName
    );
    
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        auto field = parseVariableDeclaration(true);
        if (field) {
            structDecl->addField(std::move(field));
        }
    }
    
    if (!match(TokenType::RBRACE)) {
        errorReporter.reportUnclosedBlock(structToken.line, structToken.column);
    }
    
    return structDecl;
}

std::unique_ptr<VarDeclStmtNode> Parser::parseVariableDeclaration(bool requireSemicolon) {
    Type varType = parseType();
    
    if (!check(TokenType::IDENTIFIER)) {
        errorReporter.reportExpectedIdentifier(peek().line, peek().column, peek().lexeme);
        return nullptr;
    }
    Token nameToken = advance();
    
    auto varDecl = std::make_unique<VarDeclStmtNode>(
        nameToken.line, nameToken.column,
        varType,
        nameToken.lexeme
    );
    
    if (match(TokenType::EQUAL)) {
        varDecl->initializer = parseExpression();
    }
    
    if (requireSemicolon) {
        if (!match(TokenType::SEMICOLON)) {
            errorReporter.reportMissingSemicolon(peek().line, peek().column, peek().lexeme);
        }
    }
    
    return varDecl;
}

std::unique_ptr<ExprStmtNode> Parser::parseExpressionStatement() {
    auto expr = parseExpression();
    if (!expr) {
        errorReporter.reportExpectedExpression(peek().line, peek().column, peek().lexeme);
        return nullptr;
    }
    
    if (!match(TokenType::SEMICOLON)) {
        errorReporter.reportMissingSemicolon(peek().line, peek().column, peek().lexeme);
    }
    
    return std::make_unique<ExprStmtNode>(
        expr->getLine(), expr->getColumn(), std::move(expr)
    );
}



//=============================================================================
// Парсинг операторов
//=============================================================================

bool Parser::isAtBlockEnd() const {
    if (isAtEnd()) return true;
    TokenType type = peek().type;
    return type == TokenType::RBRACE || 
           type == TokenType::END_OF_FILE;
}

std::unique_ptr<StatementNode> Parser::parseStatement() {
    if (isAtEnd()) {
        return nullptr;
    }
    
    if (check(TokenType::LBRACE)) {
        return parseBlock();
    }
    
    if (match(TokenType::KW_IF)) {
        return parseIfStatement();
    }
    
    if (match(TokenType::KW_WHILE)) {
        return parseWhileStatement();
    }
    
    if (match(TokenType::KW_FOR)) {
        return parseForStatement();
    }
    
    if (match(TokenType::KW_RETURN)) {
        return parseReturnStatement();
    }
    
    Token peekToken = peek();
    if (peekToken.type == TokenType::KW_INT || 
        peekToken.type == TokenType::KW_FLOAT ||
        peekToken.type == TokenType::KW_BOOL ||
        peekToken.type == TokenType::KW_STRING) {
        return parseVariableDeclaration(true);
    }
    
    // Объявление структурной переменной
    if (peekToken.type == TokenType::IDENTIFIER) {
        size_t savePos = current;
        Type possibleType = parseType();
        if (possibleType.kind == TypeKind::STRUCT && check(TokenType::IDENTIFIER)) {
            current = savePos;
            return parseVariableDeclaration(true);
        }
        current = savePos;
    }
    
    auto expr = parseExpression();
    if (!expr) {
        errorReporter.reportExpectedExpression(peekToken.line, peekToken.column, peekToken.lexeme);
        return nullptr;
    }
    
    if (!match(TokenType::SEMICOLON)) {
        errorReporter.reportMissingSemicolon(peek().line, peek().column, peek().lexeme);
    }
    
    return std::make_unique<ExprStmtNode>(
        expr->getLine(), expr->getColumn(), std::move(expr)
    );
}

std::unique_ptr<BlockStmtNode> Parser::parseBlock() {
    if (!check(TokenType::LBRACE)) {
        errorReporter.reportMissingBrace(peek().line, peek().column);
        return nullptr;
    }
    
    Token lbrace = advance();
    auto block = std::make_unique<BlockStmtNode>(lbrace.line, lbrace.column);
    
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        auto stmt = parseStatement();
        if (stmt) {
            block->addStatement(std::move(stmt));
        } else {
            if (!isAtEnd()) {
                advance();
            }
        }
    }
    
    if (!match(TokenType::RBRACE)) {
        errorReporter.reportUnclosedBlock(lbrace.line, lbrace.column);
    }
    
    return block;
}

std::unique_ptr<IfStmtNode> Parser::parseIfStatement() {
    Token ifToken = previous();
    
    if (!match(TokenType::LPAREN)) {
        errorReporter.reportMissingParen(peek().line, peek().column);
        return nullptr;
    }
    
    auto condition = parseExpression();
    if (!condition) {
        errorReporter.reportExpectedExpression(peek().line, peek().column, peek().lexeme);
    }
    
    if (!match(TokenType::RPAREN)) {
        errorReporter.reportMissingParen(peek().line, peek().column);
    }
    
    auto thenBranch = parseStatement();
    
    std::unique_ptr<StatementNode> elseBranch = nullptr;
    if (match(TokenType::KW_ELSE)) {
        elseBranch = parseStatement();
    }
    
    auto ifStmt = std::make_unique<IfStmtNode>(
        ifToken.line, ifToken.column,
        std::move(condition), std::move(thenBranch)
    );
    
    if (elseBranch) {
        ifStmt->setElseBranch(std::move(elseBranch));
    }
    
    return ifStmt;
}

std::unique_ptr<WhileStmtNode> Parser::parseWhileStatement() {
    Token whileToken = previous();
    
    if (!match(TokenType::LPAREN)) {
        errorReporter.reportMissingParen(peek().line, peek().column);
        return nullptr;
    }
    
    auto condition = parseExpression();
    if (!condition) {
        errorReporter.reportExpectedExpression(peek().line, peek().column, peek().lexeme);
    }
    
    if (!match(TokenType::RPAREN)) {
        errorReporter.reportMissingParen(peek().line, peek().column);
    }
    
    auto body = parseStatement();
    
    return std::make_unique<WhileStmtNode>(
        whileToken.line, whileToken.column,
        std::move(condition), std::move(body)
    );
}

std::unique_ptr<ForStmtNode> Parser::parseForStatement() {
    Token forToken = previous();
    
    if (!match(TokenType::LPAREN)) {
        errorReporter.reportMissingParen(peek().line, peek().column);
        return nullptr;
    }
    
    auto forStmt = std::make_unique<ForStmtNode>(forToken.line, forToken.column);
    
    // Init
    if (!match(TokenType::SEMICOLON)) {
        Token peekToken = this->peek();
        if (peekToken.type == TokenType::KW_INT || 
            peekToken.type == TokenType::KW_FLOAT ||
            peekToken.type == TokenType::KW_BOOL ||
            peekToken.type == TokenType::KW_STRING) {
            forStmt->setInit(parseVariableDeclaration(false));
        } else {
            auto expr = parseExpression();
            if (expr) {
                forStmt->setInit(std::make_unique<ExprStmtNode>(
                    expr->getLine(), expr->getColumn(), std::move(expr)
                ));
            }
        }
        if (!match(TokenType::SEMICOLON)) {
            errorReporter.reportMissingSemicolon(peek().line, peek().column, peek().lexeme);
        }
    }
    
    // Condition
    if (!match(TokenType::SEMICOLON)) {
        forStmt->setCondition(parseExpression());
        if (!match(TokenType::SEMICOLON)) {
            errorReporter.reportMissingSemicolon(peek().line, peek().column, peek().lexeme);
        }
    }
    
    // Update
    if (!match(TokenType::RPAREN)) {
        forStmt->setUpdate(parseExpression());
        if (!match(TokenType::RPAREN)) {
            errorReporter.reportMissingParen(peek().line, peek().column);
        }
    }
    
    // Body
    forStmt->setBody(parseStatement());
    
    return forStmt;
}

std::unique_ptr<ReturnStmtNode> Parser::parseReturnStatement() {
    Token returnToken = previous();
    
    if (match(TokenType::SEMICOLON)) {
        return std::make_unique<ReturnStmtNode>(returnToken.line, returnToken.column);
    }
    
    auto value = parseExpression();
    if (!value) {
        errorReporter.reportExpectedExpression(peek().line, peek().column, peek().lexeme);
    }
    
    if (!match(TokenType::SEMICOLON)) {
        errorReporter.reportMissingSemicolon(peek().line, peek().column, peek().lexeme);
    }
    
    return std::make_unique<ReturnStmtNode>(
        returnToken.line, returnToken.column,
        std::move(value)
    );
}

//=============================================================================
// Парсинг выражений
//=============================================================================

std::unique_ptr<ExpressionNode> Parser::parseExpression() {
    return parseAssignment();
}

std::unique_ptr<ExpressionNode> Parser::parseAssignment() {
    auto expr = parseLogicalOr();
    
    if (match(TokenType::EQUAL) ||
        match(TokenType::PLUS_EQUAL) ||
        match(TokenType::MINUS_EQUAL) ||
        match(TokenType::STAR_EQUAL) ||
        match(TokenType::SLASH_EQUAL) ||
        match(TokenType::PERCENT_EQUAL)) {
        
        Token opToken = previous();
        BinaryOp op;
        
        switch (opToken.type) {
            case TokenType::EQUAL: op = BinaryOp::ASSIGN; break;
            case TokenType::PLUS_EQUAL: op = BinaryOp::ADD_ASSIGN; break;
            case TokenType::MINUS_EQUAL: op = BinaryOp::SUB_ASSIGN; break;
            case TokenType::STAR_EQUAL: op = BinaryOp::MUL_ASSIGN; break;
            case TokenType::SLASH_EQUAL: op = BinaryOp::DIV_ASSIGN; break;
            case TokenType::PERCENT_EQUAL: op = BinaryOp::MOD_ASSIGN; break;
            default: op = BinaryOp::ASSIGN;
        }
        
        // Проверка на допустимый левый операнд
        if (!dynamic_cast<IdentifierExprNode*>(expr.get()) && 
            !dynamic_cast<MemberAccessExprNode*>(expr.get()) &&
            !dynamic_cast<IndexExprNode*>(expr.get())) {
            errorReporter.reportGeneralError(opToken.line, opToken.column,
                "Неверная цель присваивания. Слева от '=' может быть только переменная, "
                "поле структуры или элемент массива");
        }
        
        auto right = parseAssignment();
        return std::make_unique<BinaryExprNode>(
            opToken.line, opToken.column,
            std::move(expr), op, std::move(right)
        );
    }
    
    return expr;
}

std::unique_ptr<ExpressionNode> Parser::parseLogicalOr() {
    auto expr = parseLogicalAnd();
    
    while (match(TokenType::PIPE_PIPE)) {
        Token opToken = previous();
        auto right = parseLogicalAnd();
        expr = std::make_unique<BinaryExprNode>(
            opToken.line, opToken.column,
            std::move(expr), BinaryOp::OR, std::move(right)
        );
    }
    
    return expr;
}

std::unique_ptr<ExpressionNode> Parser::parseLogicalAnd() {
    auto expr = parseEquality();
    
    while (match(TokenType::AMP_AMP)) {
        Token opToken = previous();
        auto right = parseEquality();
        expr = std::make_unique<BinaryExprNode>(
            opToken.line, opToken.column,
            std::move(expr), BinaryOp::AND, std::move(right)
        );
    }
    
    return expr;
}

std::unique_ptr<ExpressionNode> Parser::parseEquality() {
    auto expr = parseRelational();
    
    while (match(TokenType::EQUAL_EQUAL) || match(TokenType::BANG_EQUAL)) {
        Token opToken = previous();
        BinaryOp op = (opToken.type == TokenType::EQUAL_EQUAL) ? BinaryOp::EQ : BinaryOp::NE;
        auto right = parseRelational();
        expr = std::make_unique<BinaryExprNode>(
            opToken.line, opToken.column,
            std::move(expr), op, std::move(right)
        );
    }
    
    return expr;
}

std::unique_ptr<ExpressionNode> Parser::parseRelational() {
    auto expr = parseAdditive();
    
    while (match(TokenType::LESS) || match(TokenType::LESS_EQUAL) ||
           match(TokenType::GREATER) || match(TokenType::GREATER_EQUAL)) {
        Token opToken = previous();
        BinaryOp op;
        switch (opToken.type) {
            case TokenType::LESS: op = BinaryOp::LT; break;
            case TokenType::LESS_EQUAL: op = BinaryOp::LE; break;
            case TokenType::GREATER: op = BinaryOp::GT; break;
            case TokenType::GREATER_EQUAL: op = BinaryOp::GE; break;
            default: op = BinaryOp::EQ;
        }
        auto right = parseAdditive();
        expr = std::make_unique<BinaryExprNode>(
            opToken.line, opToken.column,
            std::move(expr), op, std::move(right)
        );
    }
    
    return expr;
}

std::unique_ptr<ExpressionNode> Parser::parseAdditive() {
    auto expr = parseMultiplicative();
    
    while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
        Token opToken = previous();
        BinaryOp op = (opToken.type == TokenType::PLUS) ? BinaryOp::ADD : BinaryOp::SUB;
        auto right = parseMultiplicative();
        expr = std::make_unique<BinaryExprNode>(
            opToken.line, opToken.column,
            std::move(expr), op, std::move(right)
        );
    }
    
    return expr;
}

std::unique_ptr<ExpressionNode> Parser::parseMultiplicative() {
    auto expr = parseUnary();
    
    while (match(TokenType::STAR) || match(TokenType::SLASH) || match(TokenType::PERCENT)) {
        Token opToken = previous();
        BinaryOp op;
        switch (opToken.type) {
            case TokenType::STAR: op = BinaryOp::MUL; break;
            case TokenType::SLASH: op = BinaryOp::DIV; break;
            case TokenType::PERCENT: op = BinaryOp::MOD; break;
            default: op = BinaryOp::MUL;
        }
        auto right = parseUnary();
        expr = std::make_unique<BinaryExprNode>(
            opToken.line, opToken.column,
            std::move(expr), op, std::move(right)
        );
    }
    
    return expr;
}

std::unique_ptr<ExpressionNode> Parser::parseUnary() {
    if (match(TokenType::MINUS) || match(TokenType::BANG) ||
        match(TokenType::PLUS_PLUS) || match(TokenType::MINUS_MINUS)) {
        
        Token opToken = previous();
        UnaryOp op;
        
        switch (opToken.type) {
            case TokenType::MINUS: op = UnaryOp::NEG; break;
            case TokenType::BANG: op = UnaryOp::NOT; break;
            case TokenType::PLUS_PLUS: op = UnaryOp::PRE_INC; break;
            case TokenType::MINUS_MINUS: op = UnaryOp::PRE_DEC; break;
            default: op = UnaryOp::NEG;
        }
        
        auto operand = parseUnary();
        return std::make_unique<UnaryExprNode>(
            opToken.line, opToken.column,
            op, std::move(operand)
        );
    }
    
    return parsePostfix();
}


std::unique_ptr<ExpressionNode> Parser::parsePrimary() {
    if (match(TokenType::INT_LITERAL)) {
        Token token = previous();
        int value = std::stoi(token.lexeme);
        return std::make_unique<LiteralExprNode>(
            token.line, token.column, LiteralValue(value)
        );
    }
    
    if (match(TokenType::FLOAT_LITERAL)) {
        Token token = previous();
        double value = std::stod(token.lexeme);
        return std::make_unique<LiteralExprNode>(
            token.line, token.column, LiteralValue(value)
        );
    }
    
    if (match(TokenType::STRING_LITERAL)) {
        Token token = previous();
        return std::make_unique<LiteralExprNode>(
            token.line, token.column, LiteralValue(token.lexeme)
        );
    }
    
    if (match(TokenType::BOOL_LITERAL)) {
        Token token = previous();
        bool value = (token.lexeme == "true");
        return std::make_unique<LiteralExprNode>(
            token.line, token.column, LiteralValue(value)
        );
    }
    
    if (match(TokenType::IDENTIFIER)) {
        Token token = previous();
        
        // Вызов функции
        if (match(TokenType::LPAREN)) {
            auto call = std::make_unique<CallExprNode>(
                token.line, token.column, token.lexeme
            );
            
            if (!check(TokenType::RPAREN)) {
                do {
                    auto arg = parseExpression();
                    if (arg) {
                        call->addArgument(std::move(arg));
                    }
                } while (match(TokenType::COMMA));
            }
            
            if (!match(TokenType::RPAREN)) {
                errorReporter.reportMissingParen(peek().line, peek().column);
            }
            return call;
        }
        
        return std::make_unique<IdentifierExprNode>(
            token.line, token.column, token.lexeme
        );
    }
    
    if (match(TokenType::LPAREN)) {
        auto expr = parseExpression();
        if (!match(TokenType::RPAREN)) {
            errorReporter.reportMissingParen(peek().line, peek().column);
        }
        return expr;
    }
    
    if (!isAtEnd()) {
        errorReporter.reportExpectedExpression(peek().line, peek().column, peek().lexeme);
    }
    return nullptr;
}

std::unique_ptr<ExpressionNode> Parser::parsePostfix() {
    auto expr = parsePrimary();
    
    while (true) {
        if (match(TokenType::PLUS_PLUS)) {
            Token opToken = previous();
            expr = std::make_unique<UnaryExprNode>(
                opToken.line, opToken.column,
                UnaryOp::POST_INC, std::move(expr)
            );
        } else if (match(TokenType::MINUS_MINUS)) {
            Token opToken = previous();
            expr = std::make_unique<UnaryExprNode>(
                opToken.line, opToken.column,
                UnaryOp::POST_DEC, std::move(expr)
            );
        } else if (match(TokenType::LBRACKET)) {
            auto index = parseExpression();
            if (!index) {
                errorReporter.reportExpectedExpression(peek().line, peek().column, peek().lexeme);
            }
            if (!match(TokenType::RBRACKET)) {
                errorReporter.reportMissingBracket(peek().line, peek().column);
            }
            expr = std::make_unique<IndexExprNode>(
                expr->getLine(), expr->getColumn(),
                std::move(expr), std::move(index)
            );
        } else if (match(TokenType::DOT)) {
            Token dotToken = previous();
            if (!check(TokenType::IDENTIFIER)) {
                errorReporter.reportExpectedIdentifier(peek().line, peek().column, peek().lexeme);
                break;
            }
            Token memberToken = advance();
            expr = std::make_unique<MemberAccessExprNode>(
                dotToken.line, dotToken.column,
                std::move(expr), memberToken.lexeme
            );
        } else {
            break;
        }
    }
    
    return expr;
}

Type Parser::parseType() {
    if (match(TokenType::KW_INT)) {
        return Type(TypeKind::INT);
    }
    if (match(TokenType::KW_FLOAT)) {
        return Type(TypeKind::FLOAT);
    }
    if (match(TokenType::KW_BOOL)) {
        return Type(TypeKind::BOOL);
    }
    if (match(TokenType::KW_STRING)) {
        return Type(TypeKind::STRING);
    }
    if (match(TokenType::IDENTIFIER)) {
        Token typeToken = previous();
        return Type(typeToken.lexeme);
    }
    
    if (!isAtEnd()) {
        errorReporter.reportExpectedType(peek().line, peek().column, peek().lexeme);
    }
    return Type(TypeKind::VOID);
}

//=============================================================================
// Вспомогательные методы
//=============================================================================

Token Parser::peek() const {
    if (isAtEnd()) return Token(TokenType::END_OF_FILE, "", 0, 0);
    return tokens[current];
}

Token Parser::previous() const {
    if (current == 0) return Token(TokenType::END_OF_FILE, "", 0, 0);
    return tokens[current - 1];
}

Token Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::isAtEnd() const {
    return current >= tokens.size() || tokens[current].type == TokenType::END_OF_FILE;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    
    Token current = peek();
    errorReporter.reportGeneralError(current.line, current.column, message);
    return current;
}

void Parser::synchronize() {
    int maxIterations = 1000;
    int iterations = 0;
    
    while (!isAtEnd() && iterations < maxIterations) {
        iterations++;
        
        if (previous().type == TokenType::SEMICOLON) return;
        
        switch (peek().type) {
            case TokenType::KW_FN:
            case TokenType::KW_STRUCT:
            case TokenType::KW_IF:
            case TokenType::KW_WHILE:
            case TokenType::KW_FOR:
            case TokenType::KW_RETURN:
            case TokenType::KW_INT:
            case TokenType::KW_FLOAT:
            case TokenType::KW_BOOL:
            case TokenType::KW_STRING:
                return;
            default:
                advance();
        }
    }
}

std::unique_ptr<CallExprNode> Parser::parseCall(const std::string& callee) {
    (void)callee;
    errorReporter.reportGeneralError(peek().line, peek().column, 
        "Внутренняя ошибка: parseCall не должен вызываться напрямую");
    return nullptr;
}

} // namespace minicompiler