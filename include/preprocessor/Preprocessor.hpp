#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace minicompiler {

// Структура для хранения макроса
struct Macro {
    std::string name;
    std::string value;
    int line;           // Строка определения (для ошибок)
};

// Состояние условной компиляции
struct IfState {
    bool isActive;      // Активна ли текущая ветка
    bool hasBeenTrue;   // Была ли уже истинная ветка
    int line;           // Строка начала (для ошибок)
};

class Preprocessor {
public:
    // Конструктор
    explicit Preprocessor(const std::string& source);
    
    // Основной метод: обработка исходного кода
    std::string process();
    
    // Методы для управления макросами
    void define(const std::string& name, const std::string& value);
    void undefine(const std::string& name);
    bool isDefined(const std::string& name) const;
    
    // Получить обработанный исходный код
    const std::string& getProcessedSource() const { return processedSource; }
    
    // Получить сообщения об ошибках
    const std::vector<std::string>& getErrors() const { return errors; }
    bool hasErrors() const { return !errors.empty(); }
    
    // Маппинг строк (для сохранения позиций в ошибках)
    int getOriginalLine(int processedLine) const;
    int getProcessedLine(int originalLine) const;

private:
    // Исходный и обработанный код
    std::string source;
    std::string processedSource;
    
    // Таблица макросов
    std::unordered_map<std::string, Macro> macros;
    
    // Состояние условной компиляции
    std::vector<IfState> ifStack;
    bool isInActiveRegion() const;
    
    // Ошибки
    std::vector<std::string> errors;
    
    // Маппинг строк (оригинальная -> обработанная и наоборот)
    std::vector<int> originalToProcessed;
    std::vector<int> processedToOriginal;
    
    // Вспомогательные методы
    void addError(int line, const std::string& message);
    std::string processLine(const std::string& line, int lineNum);
    std::string expandMacros(const std::string& text, bool inCode = true);
    std::string handleDirective(const std::string& directive, const std::string& args, int lineNum);
    
    // Методы для оценки выражений
    bool evaluateIfExpression(const std::string& expr);
    bool evaluateSimpleExpression(const std::string& expr);
    long getValueAsNumber(const std::string& name);
    
    // Разбор строки
    std::pair<std::string, std::string> splitDirective(const std::string& line);
    std::string trim(const std::string& str);
    std::vector<std::string> tokenize(const std::string& str);
};

} // namespace minicompiler