#include "utils/ErrorCodes.hpp"
#include <sstream>

namespace minicompiler {

std::string errorCodeToString(ErrorCode code) {
    int num = static_cast<int>(code);
    std::string category;
    
    if (num < 100) {
        category = "LEX";
    } else if (num < 200) {
        category = "SYN";
    } else if (num < 300) {
        category = "SEM";
    } else if (num < 400) {
        category = "PRE";
    } else {
        category = "GEN";
    }
    
    std::stringstream ss;
    ss << category << "-";
    if (num < 10) {
        ss << "00" << num;
    } else if (num < 100) {
        ss << "0" << num;
    } else {
        ss << num;
    }
    
    return ss.str();
}

std::string errorCodeToCategory(ErrorCode code) {
    int num = static_cast<int>(code);
    if (num < 100) return "Лексическая ошибка";
    if (num < 200) return "Синтаксическая ошибка";
    if (num < 300) return "Семантическая ошибка";
    if (num < 400) return "Ошибка препроцессора";
    return "Общая ошибка";
}

std::string getErrorSuggestion(ErrorCode code) {
    switch (code) {
        case ErrorCode::LEX_UNEXPECTED_CHAR:
            return "Удалите или замените недопустимый символ";
        case ErrorCode::LEX_INVALID_NUMBER:
            return "Используйте только цифры для целых чисел";
        case ErrorCode::LEX_UNTERMINATED_STRING:
            return "Добавьте закрывающую кавычку \" в конце строки";
        case ErrorCode::LEX_NEWLINE_IN_STRING:
            return "Используйте \\n для переноса строки внутри строки";
        case ErrorCode::LEX_INVALID_ESCAPE:
            return "Допустимые escape-последовательности: \\n \\t \\r \\\" \\\\";
        case ErrorCode::LEX_IDENTIFIER_TOO_LONG:
            return "Сократите имя идентификатора до 255 символов";
            
        case ErrorCode::SYN_MISSING_SEMICOLON:
            return "Добавьте ';' в конце оператора";
        case ErrorCode::SYN_MISSING_PAREN:
            return "Добавьте закрывающую скобку ')'";
        case ErrorCode::SYN_MISSING_BRACE:
            return "Добавьте закрывающую фигурную скобку '}'";
        case ErrorCode::SYN_MISSING_BRACKET:
            return "Добавьте закрывающую квадратную скобку ']'";
        case ErrorCode::SYN_UNEXPECTED_TOKEN:
            return "Проверьте синтаксис вокруг этого токена";
        case ErrorCode::SYN_EXPECTED_IDENTIFIER:
            return "Укажите имя переменной или функции";
        case ErrorCode::SYN_EXPECTED_TYPE:
            return "Укажите тип (int, float, bool, string) или имя структуры";
        case ErrorCode::SYN_UNCLOSED_BLOCK:
            return "Проверьте, что все блоки кода закрыты '}'";
            
        case ErrorCode::SEM_UNDECLARED_IDENTIFIER:
            return "Объявите переменную перед использованием";
        case ErrorCode::SEM_DUPLICATE_DECLARATION:
            return "Используйте другое имя или удалите предыдущее объявление";
        case ErrorCode::SEM_TYPE_MISMATCH:
            return "Проверьте соответствие типов в выражении";
        case ErrorCode::SEM_WRONG_ARGUMENT_COUNT:
            return "Проверьте количество аргументов при вызове функции";
        case ErrorCode::SEM_RETURN_TYPE_MISMATCH:
            return "Функция должна возвращать значение указанного типа";
        case ErrorCode::SEM_MISSING_RETURN:
            return "Добавьте оператор return в функцию";
        case ErrorCode::SEM_INVALID_ARRAY_INDEX:
            return "Индекс массива должен быть целым числом";
        case ErrorCode::SEM_NON_ARRAY_INDEX:
            return "Оператор [] можно использовать только с массивами";
            
        default:
            return "Проверьте синтаксис кода";
    }
}

} // namespace minicompiler