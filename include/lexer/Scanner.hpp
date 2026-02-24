#pragma once
#include <string>
#include <vector>
#include "Token.hpp"

namespace minicompiler {

class ErrorReporter;

class Scanner {
public:
    // Конструктор принимает исходный код
    explicit Scanner(const std::string& source, ErrorReporter& errorReporter);
    
    // Основной метод: сканирование всех токенов
    std::vector<Token> scanTokens();
    
    // Построчное сканирование (для отладки)
    Token scanNextToken();
    
    // Просмотр следующего токена без продвижения
    Token peekToken();
    
    // Проверка конца файла
    bool isAtEnd() const;
    
    // Получить текущую позицию
    int getLine() const { return line; }
    int getColumn() const { return (current - lineStart) + 1; }
    
private:
    // Исходный код (очищенный от BOM)
    std::string source;
    
    // Позиции в исходном коде
    size_t start;           // Начало текущей лексемы
    size_t current;         // Текущая позиция
    size_t lineStart;       // Начало текущей строки
    int line;              // Номер текущей строки
    
    // Обработчик ошибок
    ErrorReporter& errorReporter;
    
    // Вспомогательные методы
    char advance();                    // Следующий символ
    char peek() const;                 // Текущий символ без продвижения
    char peekNext() const;             // Следующий символ
    bool match(char expected);         // Проверка и потребление символа
    bool isDigit(char c) const;        // Проверка цифры
    bool isAlpha(char c) const;        // Проверка буквы
    bool isAlphaNumeric(char c) const; // Проверка буквы или цифры
    
    // Методы для сканирования конкретных токенов
    Token makeToken(TokenType type);
    Token makeToken(TokenType type, LiteralValue value);
    Token errorToken(const std::string& message);
    
    // Методы пропуска
    void skipWhitespace();             // Пропуск пробелов
    void skipSingleLineComment();      // Пропуск однострочного комментария
    void skipMultiLineComment();       // Пропуск многострочного комментария
    
    // Методы сканирования
    Token scanIdentifier();            // Сканирование идентификатора/ключа
    Token scanNumber();                // Сканирование числа
    Token scanString();                // Сканирование строки
    Token scanOperator();              // Сканирование оператора
};

} // namespace minicompiler