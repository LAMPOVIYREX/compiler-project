#pragma once
#include <string>
#include <vector>

namespace minicompiler {

struct Error {
    std::string message;
    int line;
    int column;
    
    Error(const std::string& msg, int ln, int col)
        : message(msg), line(ln), column(col) {}
};

class ErrorReporter {
public:
    // Сообщить об ошибке
    void report(int line, int column, const std::string& message);
    
    // Проверить, были ли ошибки
    bool hasErrors() const { return !errors.empty(); }
    
    // Получить все ошибки
    const std::vector<Error>& getErrors() const { return errors; }
    
    // Вывести все ошибки в stderr
    void printErrors() const;
    
    // Очистить список ошибок
    void clear() { errors.clear(); }
    
private:
    std::vector<Error> errors;
};

} // namespace minicompiler