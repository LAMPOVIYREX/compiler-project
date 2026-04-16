#pragma once
#include <string>
#include <vector>
#include "Token.hpp"
#include "../utils/ErrorReporter.hpp"

namespace minicompiler {

enum class RecoveryMode {
    PANIC,      // Skip to next statement boundary
    INSERT,     // Insert expected token and continue
    DELETE,     // Delete current token and continue
    SYNC        // Sync to next keyword
};

struct RecoveryStrategy {
    RecoveryMode mode;
    std::string expected;
};

class Scanner {
public:
    explicit Scanner(const std::string& source, ErrorReporter& errorReporter);
    
    std::vector<Token> scanTokens();
    Token scanNextToken();
    Token peekToken();
    bool isAtEnd() const;
    
    int getLine() const { return line; }
    int getColumn() const { return (current - lineStart) + 1; }
    
    // Recovery methods
    void setRecoveryMode(bool enabled) { recoveryEnabled = enabled; }
    Token recoverFromError(const std::string& message, RecoveryMode mode = RecoveryMode::PANIC);
    
private:
    std::string source;
    size_t start;
    size_t current;
    size_t lineStart;
    int line;
    
    ErrorReporter& errorReporter;
    bool recoveryEnabled = true;
    int recoveryAttempts = 0;
    size_t recoveryPosition = 0;
    
    // Recovery state
    std::vector<Token> recoveredTokens;
    
    char advance();
    char peek() const;
    char peekNext() const;
    bool match(char expected);
    bool isDigit(char c) const;
    bool isAlpha(char c) const;
    bool isAlphaNumeric(char c) const;
    
    Token makeToken(TokenType type);
    Token makeToken(TokenType type, LiteralValue value);
    Token errorToken(const std::string& message, RecoveryMode mode = RecoveryMode::PANIC);
    
    void skipWhitespace();
    void skipSingleLineComment();
    void skipMultiLineComment();
    
    Token scanIdentifier();
    Token scanNumber();
    Token scanString();
    Token scanOperator();
    
    // Recovery helpers
    void syncToNextStatement();
    Token insertToken(TokenType type, const std::string& lexeme);
    void deleteCurrentChar();
    bool isKeyword(const std::string& text);
    bool isStatementBoundary(char c);
};

} // namespace minicompiler