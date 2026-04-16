#include "lexer/Scanner.hpp"
#include "utils/ErrorReporter.hpp"
#include <cctype>
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <iostream>

namespace minicompiler {

// Конструктор
Scanner::Scanner(const std::string& source, ErrorReporter& errorReporter)
    : source(source), start(0), current(0), lineStart(0), 
      line(1), errorReporter(errorReporter), recoveryEnabled(true), recoveryAttempts(0), recoveryPosition(0) {}

// Сканирование всех токенов
std::vector<Token> Scanner::scanTokens() {
    std::vector<Token> tokens;
    
    while (!isAtEnd() && !errorReporter.hasFatalError()) {
        start = current;
        Token token = scanNextToken();
        
        tokens.push_back(token);
        
        // Если это фатальная ошибка, останавливаемся
        if (token.type == TokenType::ERROR) {
            break;
        }
    }
    
    // Добавляем токен конца файла, если не было фатальной ошибки
    if (!errorReporter.hasFatalError()) {
        tokens.push_back(Token(TokenType::END_OF_FILE, "", line, getColumn()));
    }
    
    return tokens;
}

// Сканирование следующего токена
Token Scanner::scanNextToken() {
    skipWhitespace();
    start = current;
    
    if (isAtEnd()) {
        return makeToken(TokenType::END_OF_FILE);
    }
    
    char c = advance();
    
    // Комментарии
    if (c == '/') {
        if (peek() == '/') {
            // Однострочный комментарий
            skipSingleLineComment();
            return scanNextToken();
        } else if (peek() == '*') {
            // Многострочный комментарий
            skipMultiLineComment();
            return scanNextToken();
        }
        // Если это не комментарий, проверяем оператор /=
        if (peek() == '=') {
            advance(); // потребляем '='
            return makeToken(TokenType::SLASH_EQUAL);
        }
        // Иначе это просто оператор деления
        return makeToken(TokenType::SLASH);
    }
    
    // Идентификаторы и ключевые слова
    if (isAlpha(c)) {
        return scanIdentifier();
    }
    
    // Числа
    if (isDigit(c)) {
        return scanNumber();
    }
    
    // Строки
    if (c == '"') {
        return scanString();
    }
    
    // Операторы и разделители
    switch (c) {
        case '(': return makeToken(TokenType::LPAREN);
        case ')': return makeToken(TokenType::RPAREN);
        case '{': return makeToken(TokenType::LBRACE);
        case '}': return makeToken(TokenType::RBRACE);
        case '[': return makeToken(TokenType::LBRACKET);
        case ']': return makeToken(TokenType::RBRACKET);
        case ';': return makeToken(TokenType::SEMICOLON);
        case ',': return makeToken(TokenType::COMMA);
        case '.': return makeToken(TokenType::DOT);
        case ':': return makeToken(TokenType::COLON);
        
        case '+':
            if (peek() == '+') {
                advance();
                return makeToken(TokenType::PLUS_PLUS);
            }
            if (peek() == '=') {
                advance();
                return makeToken(TokenType::PLUS_EQUAL);
            }
            return makeToken(TokenType::PLUS);
            
        case '-':
            // Проверяем '->' (ARROW) в первую очередь
            if (peek() == '>') {
                advance();
                return makeToken(TokenType::ARROW);  // '->' для возвращаемого типа
            }
            if (peek() == '-') {
                advance();
                return makeToken(TokenType::MINUS_MINUS);
            }
            if (peek() == '=') {
                advance();
                return makeToken(TokenType::MINUS_EQUAL);
            }
            return makeToken(TokenType::MINUS);
            
        case '*':
            if (peek() == '=') {
                advance();
                return makeToken(TokenType::STAR_EQUAL);
            }
            return makeToken(TokenType::STAR);
            
        case '%':
            if (peek() == '=') {
                advance();
                return makeToken(TokenType::PERCENT_EQUAL);
            }
            return makeToken(TokenType::PERCENT);
            
        case '=':
            if (peek() == '=') {
                advance();
                return makeToken(TokenType::EQUAL_EQUAL);
            }
            return makeToken(TokenType::EQUAL);
            
        case '!':
            if (peek() == '=') {
                advance();
                return makeToken(TokenType::BANG_EQUAL);
            }
            return makeToken(TokenType::BANG);
            
        case '<':
            if (peek() == '=') {
                advance();
                return makeToken(TokenType::LESS_EQUAL);
            }
            return makeToken(TokenType::LESS);
            
        case '>':
            if (peek() == '=') {
                advance();
                return makeToken(TokenType::GREATER_EQUAL);
            }
            return makeToken(TokenType::GREATER);
            
        case '&':
            if (match('&')) return makeToken(TokenType::AMP_AMP);
            errorReporter.reportUnexpectedChar(line, getColumn() - 1, c);
            errorReporter.setFatalError(true);
            return Token(TokenType::ERROR, std::string(1, c), line, getColumn() - 1);
            
        case '|':
            if (match('|')) return makeToken(TokenType::PIPE_PIPE);
            errorReporter.reportUnexpectedChar(line, getColumn() - 1, c);
            errorReporter.setFatalError(true);
            return Token(TokenType::ERROR, std::string(1, c), line, getColumn() - 1);
            
        default:
            errorReporter.reportUnexpectedChar(line, getColumn() - 1, c);
            errorReporter.setFatalError(true);
            return Token(TokenType::ERROR, std::string(1, c), line, getColumn() - 1);
    }
}

// Просмотр следующего токена без продвижения
Token Scanner::peekToken() {
    size_t savedStart = start;
    size_t savedCurrent = current;
    size_t savedLineStart = lineStart;
    int savedLine = line;
    
    Token token = scanNextToken();
    
    start = savedStart;
    current = savedCurrent;
    lineStart = savedLineStart;
    line = savedLine;
    
    return token;
}

bool Scanner::isAtEnd() const {
    return current >= source.length();
}

// Вспомогательные методы
char Scanner::advance() {
    if (isAtEnd()) return '\0';
    return source[current++];
}

char Scanner::peek() const {
    if (isAtEnd()) return '\0';
    return source[current];
}

char Scanner::peekNext() const {
    if (current + 1 >= source.length()) return '\0';
    return source[current + 1];
}

bool Scanner::match(char expected) {
    if (isAtEnd()) return false;
    if (source[current] != expected) return false;
    
    current++;
    return true;
}

bool Scanner::isDigit(char c) const {
    return std::isdigit(static_cast<unsigned char>(c));
}

bool Scanner::isAlpha(char c) const {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

bool Scanner::isAlphaNumeric(char c) const {
    return isAlpha(c) || isDigit(c);
}

Token Scanner::makeToken(TokenType type) {
    std::string lexeme = source.substr(start, current - start);
    int column = (start - lineStart) + 1;
    return Token(type, lexeme, line, column);
}

Token Scanner::makeToken(TokenType type, LiteralValue value) {
    std::string lexeme = source.substr(start, current - start);
    int column = (start - lineStart) + 1;
    return Token(type, lexeme, value, line, column);
}

Token Scanner::errorToken(const std::string& message, RecoveryMode mode) {
    (void)message;
    (void)mode;
    
    std::string lexeme = source.substr(start, std::min(size_t(10), current - start));
    int column = getColumn();
    
    errorReporter.setFatalError(true);
    return Token(TokenType::ERROR, lexeme, line, column);
}

// Пропуск пробельных символов
void Scanner::skipWhitespace() {
    while (!isAtEnd()) {
        char c = peek();
        
        if (c < 32 && c != '\n' && c != '\t' && c != '\r') {
            advance();
            continue;
        }
        
        switch (c) {
            case ' ':
            case '\t':
            case '\r':
                advance();
                break;
                
            case '\n':
                line++;
                lineStart = current + 1;
                advance();
                break;
                
            default:
                return;
        }
    }
}

// Пропуск однострочного комментария
void Scanner::skipSingleLineComment() {
    while (!isAtEnd() && peek() != '\n') {
        advance();
    }
}

// Пропуск многострочного комментария
void Scanner::skipMultiLineComment() {
    int depth = 1;
    int startLine = line;
    
    while (!isAtEnd() && depth > 0) {
        if (peek() == '/' && peekNext() == '*') {
            advance(); // '/'
            advance(); // '*'
            depth++;
        } else if (peek() == '*' && peekNext() == '/') {
            advance(); // '*'
            advance(); // '/'
            depth--;
        } else {
            if (peek() == '\n') {
                line++;
                lineStart = current + 1;
            }
            advance();
        }
    }
    
    if (depth > 0) {
        errorReporter.reportUnterminatedComment(line, getColumn(), startLine);
    }
}

// Сканирование идентификатора
Token Scanner::scanIdentifier() {
    size_t idStart = start;
    
    while (isAlphaNumeric(peek())) {
        advance();
    }
    
    std::string text = source.substr(idStart, current - idStart);
    
    if (text.length() > 255) {
        errorReporter.reportIdentifierTooLong(line, getColumn() - text.length(), 
                                             text.length(), 255);
        return errorToken("", RecoveryMode::PANIC);
    }
    
    auto it = keywords.find(text);
    if (it != keywords.end()) {
        if (it->second == TokenType::KW_TRUE) {
            return makeToken(TokenType::BOOL_LITERAL, true);
        } else if (it->second == TokenType::KW_FALSE) {
            return makeToken(TokenType::BOOL_LITERAL, false);
        }
        return makeToken(it->second);
    }
    
    return makeToken(TokenType::IDENTIFIER);
}

// Сканирование числа
Token Scanner::scanNumber() {
    bool isFloat = false;
    size_t numberStart = start;
    
    // Целая часть
    while (isDigit(peek())) {
        advance();
    }
    
    // Дробная часть
    if (peek() == '.' && isDigit(peekNext())) {
        isFloat = true;
        advance();
        while (isDigit(peek())) {
            advance();
        }
    }
    
    // Научная нотация
    if (peek() == 'e' || peek() == 'E') {
        isFloat = true;
        advance();
        if (peek() == '+' || peek() == '-') {
            advance();
        }
        if (!isDigit(peek())) {
            errorReporter.reportInvalidNumber(line, getColumn(), 
                source.substr(numberStart, current - numberStart));
            return errorToken("", RecoveryMode::PANIC);
        }
        while (isDigit(peek())) {
            advance();
        }
    }
    
    std::string numberStr = source.substr(numberStart, current - numberStart);
    
    // Проверка на буквы после числа
    if (isAlpha(peek())) {
        errorReporter.reportInvalidNumber(line, getColumn() - numberStr.length(), numberStr);
        return errorToken("", RecoveryMode::PANIC);
    }
    
    try {
        if (isFloat) {
            double value = std::stod(numberStr);
            return makeToken(TokenType::FLOAT_LITERAL, value);
        } else {
            long value = std::stol(numberStr);
            if (value < INT_MIN || value > INT_MAX) {
                errorReporter.reportInvalidNumber(line, getColumn() - numberStr.length(), numberStr);
                return errorToken("", RecoveryMode::PANIC);
            }
            return makeToken(TokenType::INT_LITERAL, static_cast<int>(value));
        }
    } catch (const std::exception&) {
        errorReporter.reportInvalidNumber(line, getColumn() - numberStr.length(), numberStr);
        return errorToken("", RecoveryMode::PANIC);
    }
}

// Сканирование строки
Token Scanner::scanString() {
    std::string value;
    int startLine = line;
    
    while (!isAtEnd() && peek() != '"') {
        char c = advance();
        
        if (c == '\\') {
            if (isAtEnd()) {
                errorReporter.reportUnterminatedString(line, getColumn(), startLine);
                return errorToken("", RecoveryMode::PANIC);
            }
            
            char escape = advance();
            switch (escape) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case 'r': value += '\r'; break;
                case '"': value += '"'; break;
                case '\\': value += '\\'; break;
                default:
                    errorReporter.reportInvalidEscape(line, getColumn() - 1, escape);
                    return errorToken("", RecoveryMode::PANIC);
            }
        } else {
            if (c == '\n') {
                errorReporter.reportNewlineInString(line, getColumn());
                return errorToken("", RecoveryMode::PANIC);
            }
            value += c;
        }
    }
    
    if (isAtEnd()) {
        errorReporter.reportUnterminatedString(line, getColumn(), startLine);
        return errorToken("", RecoveryMode::PANIC);
    }
    
    advance(); // Потребляем закрывающую кавычку
    return makeToken(TokenType::STRING_LITERAL, value);
}

// Recovery methods
Token Scanner::recoverFromError(const std::string& message, RecoveryMode mode) {
    (void)message;
    recoveryAttempts++;
    bool success = false;
    
    switch (mode) {
        case RecoveryMode::PANIC:
            // Пропускаем до конца строки или следующей ';'
            while (!isAtEnd() && peek() != '\n' && peek() != ';') {
                advance();
            }
            if (peek() == ';') {
                advance();
            }
            success = true;
            break;
            
        case RecoveryMode::INSERT:
            // Вставляем ожидаемый токен - просто продолжаем
            success = true;
            break;
            
        case RecoveryMode::DELETE:
            // Удаляем текущий символ
            if (!isAtEnd()) {
                advance();
                success = true;
            }
            break;
            
        case RecoveryMode::SYNC:
            // Синхронизируемся на следующем ключевом слове
            while (!isAtEnd()) {
                // Следим за переводом строки
                if (peek() == '\n') {
                    line++;
                    lineStart = current + 1;
                }
                
                // Проверяем, не начинается ли ключевое слово
                if (isAlpha(peek())) {
                    size_t temp = current;
                    std::string word;
                    while (isAlphaNumeric(peek())) {
                        word += advance();
                    }
                    if (isKeyword(word)) {
                        current = temp;
                        success = true;
                        break;
                    }
                    current = temp;
                }
                advance();
            }
            break;
    }
    
    errorReporter.recoveryAttempt(success);
    return scanNextToken();
}

void Scanner::syncToNextStatement() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ';' || c == '}' || c == '\n') {
            if (c == ';' || c == '}') advance();
            break;
        }
        if (isAlpha(c)) {
            size_t temp = current;
            std::string word;
            while (isAlphaNumeric(peek())) {
                word += advance();
            }
            if (isKeyword(word)) {
                current = temp;
                break;
            }
            current = temp;
        }
        advance();
    }
}

Token Scanner::insertToken(TokenType type, const std::string& lexeme) {
    return Token(type, lexeme, line, getColumn());
}

void Scanner::deleteCurrentChar() {
    if (!isAtEnd()) {
        advance();
    }
}

bool Scanner::isKeyword(const std::string& text) {
    return keywords.find(text) != keywords.end();
}

bool Scanner::isStatementBoundary(char c) {
    return c == ';' || c == '}' || c == '{' || c == '\n';
}

} // namespace minicompiler