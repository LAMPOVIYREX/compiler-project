#pragma once
#include <string>
#include <unordered_map>

namespace minicompiler {

// Типы токенов
enum class TokenType {
    // Конец файла
    END_OF_FILE,
    
    // Ключевые слова
    KW_FN, KW_IF, KW_ELSE, KW_WHILE, KW_FOR, KW_RETURN,
    KW_INT, KW_FLOAT, KW_BOOL, KW_VOID, KW_TRUE, KW_FALSE,
    KW_STRUCT, KW_CONST,
    
    // Идентификаторы и литералы
    IDENTIFIER,
    INT_LITERAL, FLOAT_LITERAL, STRING_LITERAL, BOOL_LITERAL,
    
    // Операторы
    PLUS, MINUS, STAR, SLASH, PERCENT,
    PLUS_PLUS, MINUS_MINUS,
    EQUAL_EQUAL, BANG_EQUAL,
    LESS, LESS_EQUAL, GREATER, GREATER_EQUAL,
    AMP_AMP, PIPE_PIPE, BANG,
    EQUAL, PLUS_EQUAL, MINUS_EQUAL, STAR_EQUAL, SLASH_EQUAL, PERCENT_EQUAL,
    
    // Разделители
    LPAREN, RPAREN,          // ( )
    LBRACE, RBRACE,          // { }
    LBRACKET, RBRACKET,      // [ ]
    SEMICOLON, COMMA, DOT, COLON,
    
    // Ошибка
    ERROR
};

// Сопоставление типов токенов с их строковыми представлениями
extern const std::unordered_map<TokenType, std::string> tokenTypeToString;
extern const std::unordered_map<std::string, TokenType> keywords;

// Получить строковое представление типа токена
std::string tokenTypeToStringFunc(TokenType type);
// Проверить, является ли строка ключевым словом
bool isKeyword(const std::string& text);

} // namespace minicompiler