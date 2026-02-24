#include "lexer/Scanner.hpp"
#include "utils/ErrorReporter.hpp"
#include <cctype>
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <cstring>

namespace minicompiler {

// Исправленный конструктор с удалением BOM
// Исправленный конструктор с правильным порядком инициализации
Scanner::Scanner(const std::string& source, ErrorReporter& errorReporter)
    : source(source), 
      start(0), 
      current(0), 
      lineStart(0),
      line(1),
      errorReporter(errorReporter) {
    
    // Удаляем UTF-8 BOM если есть (0xEF 0xBB 0xBF)
    if (this->source.size() >= 3 && 
        static_cast<unsigned char>(this->source[0]) == 0xEF &&
        static_cast<unsigned char>(this->source[1]) == 0xBB &&
        static_cast<unsigned char>(this->source[2]) == 0xBF) {
        this->source = this->source.substr(3);
    }
}

// Сканирование всех токенов
std::vector<Token> Scanner::scanTokens() {
    std::vector<Token> tokens;
    
    while (!isAtEnd()) {
        start = current;
        Token token = scanNextToken();
        if (token.type != TokenType::ERROR) {
            tokens.push_back(token);
        }
    }
    
    // Добавляем токен конца файла
    tokens.push_back(Token(TokenType::END_OF_FILE, "", line, getColumn()));
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
            return scanNextToken(); // Рекурсивно получаем следующий токен
        } else if (peek() == '*') {
            // Многострочный комментарий
            skipMultiLineComment();
            return scanNextToken(); // Рекурсивно получаем следующий токен
        }
        // Если не комментарий, то это оператор деления
        current = start + 1; // Откатываемся к '/'
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
        
        default:
            // Для операторов вызываем отдельный метод
            current = start;
            return scanOperator();
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
    int column = (start - lineStart) + 1; // Исправляем вычисление столбца
    return Token(type, lexeme, line, column);
}

Token Scanner::makeToken(TokenType type, LiteralValue value) {
    std::string lexeme = source.substr(start, current - start);
    int column = (start - lineStart) + 1; // Исправляем вычисление столбца
    return Token(type, lexeme, value, line, column);
}

Token Scanner::errorToken(const std::string& message) {
    std::string lexeme = source.substr(start, std::min(size_t(10), current - start));
    int column = (start - lineStart) + 1; // Исправляем вычисление столбца
    errorReporter.report(line, column, message);
    return Token(TokenType::ERROR, lexeme, line, column);
}

// Пропуск пробельных символов
void Scanner::skipWhitespace() {
    while (!isAtEnd()) {
        char c = peek();
        
        // Пропускаем непечатаемые ASCII символы (кроме табуляции и новой строки)
        if (c >= 0 && c <= 31 && c != '\n' && c != '\t' && c != '\r') {
            advance(); // Пропускаем непечатаемый символ
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
    // Пропускаем все до конца строки
    while (!isAtEnd() && peek() != '\n') {
        advance();
    }
}

// Пропуск многострочного комментария
void Scanner::skipMultiLineComment() {
    int depth = 1;
    
    while (!isAtEnd() && depth > 0) {
        if (peek() == '/' && peekNext() == '*') {
            // Начало вложенного комментария
            advance(); // '/'
            advance(); // '*'
            depth++;
        } else if (peek() == '*' && peekNext() == '/') {
            // Конец комментария
            advance(); // '*'
            advance(); // '/'
            depth--;
        } else if (peek() == '\n') {
            line++;
            lineStart = current + 1;
            advance();
        } else {
            advance();
        }
    }
    
    if (depth > 0) {
        errorReporter.report(line, getColumn(), "Unterminated multi-line comment");
    }
}

// Сканирование идентификатора или ключевого слова
Token Scanner::scanIdentifier() {
    while (isAlphaNumeric(peek())) {
        advance();
    }
    
    std::string text = source.substr(start, current - start);
    
    // Проверка максимальной длины
    if (text.length() > 255) {
        return errorToken("Identifier too long (max 255 characters)");
    }
    
    // Проверка ключевых слов
    auto it = keywords.find(text);
    if (it != keywords.end()) {
        // Для true/false создаем литералы
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
    bool hasError = false;
    
    // Целая часть
    while (isDigit(peek())) {
        advance();
    }
    
    // Дробная часть
    if (peek() == '.' && isDigit(peekNext())) {
        isFloat = true;
        advance(); // Потребляем точку
        
        while (isDigit(peek())) {
            advance();
        }
    }
    
    // Научная нотация (опционально)
    if (peek() == 'e' || peek() == 'E') {
        isFloat = true;
        advance(); // Потребляем 'e' или 'E'
        
        if (peek() == '+' || peek() == '-') {
            advance(); // Потребляем знак
        }
        
        if (!isDigit(peek())) {
            return errorToken("Invalid scientific notation");
        }
        
        while (isDigit(peek())) {
            advance();
        }
    }
    
    std::string numberStr = source.substr(start, current - start);
    
    try {
        if (isFloat) {
            // Для float используем stod
            char* endptr;
            double value = std::strtod(numberStr.c_str(), &endptr);
            
            if (*endptr != '\0') {
                hasError = true;
            } else {
                return makeToken(TokenType::FLOAT_LITERAL, value);
            }
        } else {
            // Для int используем stol с проверкой диапазона
            char* endptr;
            long value = std::strtol(numberStr.c_str(), &endptr, 10);
            
            if (*endptr != '\0') {
                hasError = true;
            } else if (value < INT_MIN || value > INT_MAX) {
                return errorToken("Integer literal out of range (must be between " + 
                                 std::to_string(INT_MIN) + " and " + 
                                 std::to_string(INT_MAX) + ")");
            } else {
                return makeToken(TokenType::INT_LITERAL, static_cast<int>(value));
            }
        }
    } catch (const std::exception&) {
        hasError = true;
    }
    
    if (hasError) {
        return errorToken("Invalid number literal: '" + numberStr + "'");
    }
    
    return errorToken("Failed to parse number");
}

// Сканирование строки
Token Scanner::scanString() {
    std::string value;
    
    while (!isAtEnd() && peek() != '"') {
        char c = advance();
        
        // Обработка escape-последовательностей
        if (c == '\\') {
            if (isAtEnd()) {
                return errorToken("Unterminated string");
            }
            
            char escape = advance();
            switch (escape) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case 'r': value += '\r'; break;
                case 'b': value += '\b'; break;   // Backspace
                case 'f': value += '\f'; break;   // Form feed
                case 'v': value += '\v'; break;   // Vertical tab
                case '0': value += '\0'; break;   // Null character
                case '"': value += '"'; break;
                case '\\': value += '\\'; break;
                case '\'': value += '\''; break;  // Single quote
                default:
                    return errorToken("Invalid escape sequence: \\" + std::string(1, escape));
            }
        } else {
            // Запрещаем реальные символы новой строки в строке
            if (c == '\n') {
                return errorToken("Newline character in string literal (use \\n)");
            }
            value += c;
        }
    }
    
    if (isAtEnd()) {
        return errorToken("Unterminated string");
    }
    
    // Потребляем закрывающую кавычку
    advance();
    
    return makeToken(TokenType::STRING_LITERAL, value);
}

// Сканирование оператора
Token Scanner::scanOperator() {
    char c = advance();
    
    switch (c) {
        case '+':
            if (match('+')) return makeToken(TokenType::PLUS_PLUS);
            if (match('=')) return makeToken(TokenType::PLUS_EQUAL);
            return makeToken(TokenType::PLUS);
            
        case '-':
            if (match('-')) return makeToken(TokenType::MINUS_MINUS);
            if (match('=')) return makeToken(TokenType::MINUS_EQUAL);
            return makeToken(TokenType::MINUS);
            
        case '*':
            if (match('=')) return makeToken(TokenType::STAR_EQUAL);
            return makeToken(TokenType::STAR);
            
        case '/':
            if (match('=')) return makeToken(TokenType::SLASH_EQUAL);
            return makeToken(TokenType::SLASH);
            
        case '%':
            if (match('=')) return makeToken(TokenType::PERCENT_EQUAL);
            return makeToken(TokenType::PERCENT);
            
        case '=':
            if (match('=')) return makeToken(TokenType::EQUAL_EQUAL);
            return makeToken(TokenType::EQUAL);
            
        case '!':
            if (match('=')) return makeToken(TokenType::BANG_EQUAL);
            return makeToken(TokenType::BANG);
            
        case '<':
            if (match('=')) return makeToken(TokenType::LESS_EQUAL);
            return makeToken(TokenType::LESS);
            
        case '>':
            if (match('=')) return makeToken(TokenType::GREATER_EQUAL);
            return makeToken(TokenType::GREATER);
            
        case '&':
            if (match('&')) return makeToken(TokenType::AMP_AMP);
            return errorToken("Unexpected character '&' (did you mean '&&'?)");
            
        case '|':
            if (match('|')) return makeToken(TokenType::PIPE_PIPE);
            return errorToken("Unexpected character '|' (did you mean '||'?)");
            
        default:
            return errorToken("Unexpected character: '" + std::string(1, c) + "'");
    }
}

} // namespace minicompiler