#include "lexer/Token.hpp"
#include <sstream>
#include <iomanip>

namespace minicompiler {

// Глобальные словари
const std::unordered_map<TokenType, std::string> tokenTypeToString = {
    {TokenType::END_OF_FILE, "END_OF_FILE"},
    
    // Ключевые слова
    {TokenType::KW_FN, "KW_FN"},
    {TokenType::KW_IF, "KW_IF"},
    {TokenType::KW_ELSE, "KW_ELSE"},
    {TokenType::KW_WHILE, "KW_WHILE"},
    {TokenType::KW_FOR, "KW_FOR"},
    {TokenType::KW_RETURN, "KW_RETURN"},
    {TokenType::KW_INT, "KW_INT"},
    {TokenType::KW_FLOAT, "KW_FLOAT"},
    {TokenType::KW_BOOL, "KW_BOOL"},
    {TokenType::KW_VOID, "KW_VOID"},
    {TokenType::KW_TRUE, "KW_TRUE"},
    {TokenType::KW_FALSE, "KW_FALSE"},
    {TokenType::KW_STRUCT, "KW_STRUCT"},
    {TokenType::KW_CONST, "KW_CONST"},
    
    // Идентификаторы и литералы
    {TokenType::IDENTIFIER, "IDENTIFIER"},
    {TokenType::INT_LITERAL, "INT_LITERAL"},
    {TokenType::FLOAT_LITERAL, "FLOAT_LITERAL"},
    {TokenType::STRING_LITERAL, "STRING_LITERAL"},
    {TokenType::BOOL_LITERAL, "BOOL_LITERAL"},
    
    // Операторы
    {TokenType::PLUS, "PLUS"},
    {TokenType::MINUS, "MINUS"},
    {TokenType::STAR, "STAR"},
    {TokenType::SLASH, "SLASH"},
    {TokenType::PERCENT, "PERCENT"},
    {TokenType::PLUS_PLUS, "PLUS_PLUS"},
    {TokenType::MINUS_MINUS, "MINUS_MINUS"},
    {TokenType::EQUAL_EQUAL, "EQUAL_EQUAL"},
    {TokenType::BANG_EQUAL, "BANG_EQUAL"},
    {TokenType::LESS, "LESS"},
    {TokenType::LESS_EQUAL, "LESS_EQUAL"},
    {TokenType::GREATER, "GREATER"},
    {TokenType::GREATER_EQUAL, "GREATER_EQUAL"},
    {TokenType::AMP_AMP, "AMP_AMP"},
    {TokenType::PIPE_PIPE, "PIPE_PIPE"},
    {TokenType::BANG, "BANG"},
    {TokenType::EQUAL, "EQUAL"},
    {TokenType::PLUS_EQUAL, "PLUS_EQUAL"},
    {TokenType::MINUS_EQUAL, "MINUS_EQUAL"},
    {TokenType::STAR_EQUAL, "STAR_EQUAL"},
    {TokenType::SLASH_EQUAL, "SLASH_EQUAL"},
    {TokenType::PERCENT_EQUAL, "PERCENT_EQUAL"},
    
    // Разделители
    {TokenType::LPAREN, "LPAREN"},
    {TokenType::RPAREN, "RPAREN"},
    {TokenType::LBRACE, "LBRACE"},
    {TokenType::RBRACE, "RBRACE"},
    {TokenType::LBRACKET, "LBRACKET"},
    {TokenType::RBRACKET, "RBRACKET"},
    {TokenType::SEMICOLON, "SEMICOLON"},
    {TokenType::COMMA, "COMMA"},
    {TokenType::DOT, "DOT"},
    {TokenType::COLON, "COLON"},
    
    // Ошибка
    {TokenType::ERROR, "ERROR"}
};

const std::unordered_map<std::string, TokenType> keywords = {
    {"fn", TokenType::KW_FN},
    {"if", TokenType::KW_IF},
    {"else", TokenType::KW_ELSE},
    {"while", TokenType::KW_WHILE},
    {"for", TokenType::KW_FOR},
    {"return", TokenType::KW_RETURN},
    {"int", TokenType::KW_INT},
    {"float", TokenType::KW_FLOAT},
    {"bool", TokenType::KW_BOOL},
    {"void", TokenType::KW_VOID},
    {"true", TokenType::KW_TRUE},
    {"false", TokenType::KW_FALSE},
    {"struct", TokenType::KW_STRUCT},
    {"const", TokenType::KW_CONST}
};

std::string tokenTypeToStringFunc(TokenType type) {
    auto it = tokenTypeToString.find(type);
    if (it != tokenTypeToString.end()) {
        return it->second;
    }
    return "UNKNOWN";
}

bool isKeyword(const std::string& text) {
    return keywords.find(text) != keywords.end();
}

// Реализация методов Token
Token::Token(TokenType type, const std::string& lexeme, int line, int column)
    : type(type), lexeme(lexeme), value(std::monostate{}), 
      line(line), column(column) {}

Token::Token(TokenType type, const std::string& lexeme, LiteralValue value, 
             int line, int column)
    : type(type), lexeme(lexeme), value(value), 
      line(line), column(column) {}

std::string Token::toString() const {
    std::ostringstream oss;
    oss << line << ":" << column << " " 
        << tokenTypeToStringFunc(type) << " \"" << lexeme << "\"";
    
    if (!std::holds_alternative<std::monostate>(value)) {
        oss << " [" << valueToString() << "]";
    }
    
    return oss.str();
}

std::string Token::valueToString() const {
    if (std::holds_alternative<int>(value)) {
        return std::to_string(std::get<int>(value));
    } else if (std::holds_alternative<double>(value)) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(6) << std::get<double>(value);
        return oss.str();
    } else if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value) ? "true" : "false";
    } else if (std::holds_alternative<std::string>(value)) {
        return "\"" + std::get<std::string>(value) + "\"";
    }
    return "";
}

} // namespace minicompiler