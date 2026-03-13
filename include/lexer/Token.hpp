#pragma once
#include <string>
#include <variant>
#include "TokenType.hpp"

namespace minicompiler {

// Значение литерала (разные типы)
using LiteralValue = std::variant<
    std::monostate,      // Нет значения
    int,                 // Целое число
    double,              // Число с плавающей точкой
    bool,                // Логическое значение
    std::string          // Строка
>;

// Структура токена
struct Token {
    TokenType type;          // Тип токена
    std::string lexeme;      // Исходная строка
    LiteralValue value;      // Значение (для литералов)
    int line;               // Номер строки (1-based)
    int column;             // Номер столбца (1-based)
    
    // Конструкторы
    Token(TokenType type, const std::string& lexeme, int line, int column);
    Token(TokenType type, const std::string& lexeme, LiteralValue value, 
          int line, int column);
    
    // Получить строковое представление токена
    std::string toString() const;
    
    // Получить строковое представление значения
    std::string valueToString() const;
};

} // namespace minicompiler