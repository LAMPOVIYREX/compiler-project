#include "utils/ErrorReporter.hpp"
#include <iomanip>

namespace minicompiler {

std::string ErrorReporter::getContext(int line) {
    auto it = sourceLines.find(line);
    if (it != sourceLines.end()) {
        return it->second;
    }
    return "";
}

std::string ErrorReporter::generatePointer(int col, int length) {
    if (col <= 0) return "";
    return std::string(col - 1, ' ') + std::string(length, '^');
}

std::string ErrorReporter::getErrorMessage(ErrorCode /*code*/, const std::string& details) {
    return details;
}

std::string ErrorReporter::getErrorSuggestion(ErrorCode code) {
    return ::minicompiler::getErrorSuggestion(code);
}

void ErrorReporter::addError(ErrorCode code, const std::string& message,
                              int line, int column, int length,
                              const std::string& context, const std::string& suggestion) {
    ErrorLocation loc(line, column, length);
    std::string ctx = context.empty() ? getContext(line) : context;
    std::string sug = suggestion.empty() ? getErrorSuggestion(code) : suggestion;
    errors.emplace_back(code, message, loc, ctx, sug, false);
}

void ErrorReporter::addWarning(ErrorCode code, const std::string& message,
                                int line, int column, int length,
                                const std::string& context) {
    ErrorLocation loc(line, column, length);
    std::string ctx = context.empty() ? getContext(line) : context;
    errors.emplace_back(code, message, loc, ctx, "", true);
}

// ============================================================================
// Лексические ошибки
// ============================================================================

void ErrorReporter::reportUnexpectedChar(int line, int col, char c) {
    std::stringstream ss;
    ss << "Недопустимый символ: '" << c << "' (код: " << (int)c << ")";
    addError(ErrorCode::LEX_UNEXPECTED_CHAR, ss.str(), line, col, 1);
}

void ErrorReporter::reportInvalidNumber(int line, int col, const std::string& num) {
    std::stringstream ss;
    ss << "Неверное целое число: '" << num << "' (выходит за допустимый диапазон)";
    addError(ErrorCode::LEX_INVALID_NUMBER, ss.str(), line, col, num.length());
}

void ErrorReporter::reportUnterminatedString(int line, int col, int startLine) {
    std::stringstream ss;
    ss << "Незакрытая строка (начата в строке " << startLine << ")";
    addError(ErrorCode::LEX_UNTERMINATED_STRING, ss.str(), line, col, 1);
}

void ErrorReporter::reportNewlineInString(int line, int col) {
    addError(ErrorCode::LEX_NEWLINE_IN_STRING, 
             "Символ новой строки внутри строки (используйте \\n)", 
             line, col, 1);
}

void ErrorReporter::reportInvalidEscape(int line, int col, char escape) {
    std::stringstream ss;
    ss << "Неверная escape-последовательность: \\" << escape;
    addError(ErrorCode::LEX_INVALID_ESCAPE, ss.str(), line, col, 2);
}

void ErrorReporter::reportUnterminatedComment(int line, int col, int startLine) {
    std::stringstream ss;
    ss << "Незакрытый комментарий (начат в строке " << startLine << ")";
    addError(ErrorCode::LEX_UNTERMINATED_COMMENT, ss.str(), line, col, 1);
}

void ErrorReporter::reportIdentifierTooLong(int line, int col, int length, int maxLength) {
    std::stringstream ss;
    ss << "Слишком длинный идентификатор (макс. " << maxLength 
       << " символов, получено " << length << ")";
    addError(ErrorCode::LEX_IDENTIFIER_TOO_LONG, ss.str(), line, col, length);
}

// ============================================================================
// Синтаксические ошибки
// ============================================================================

void ErrorReporter::reportMissingSemicolon(int line, int col, const std::string& found) {
    std::stringstream ss;
    ss << "Пропущена точка с запятой перед '" << found << "'";
    addError(ErrorCode::SYN_MISSING_SEMICOLON, ss.str(), line, col, 1);
}

void ErrorReporter::reportMissingParen(int line, int col) {
    addError(ErrorCode::SYN_MISSING_PAREN, 
             "Пропущена закрывающая скобка ')'", 
             line, col, 1);
}

void ErrorReporter::reportMissingBrace(int line, int col) {
    addError(ErrorCode::SYN_MISSING_BRACE, 
             "Пропущена закрывающая фигурная скобка '}'", 
             line, col, 1);
}

void ErrorReporter::reportMissingBracket(int line, int col) {
    addError(ErrorCode::SYN_MISSING_BRACKET, 
             "Пропущена закрывающая квадратная скобка ']'", 
             line, col, 1);
}

void ErrorReporter::reportUnexpectedToken(int line, int col, const std::string& token) {
    std::stringstream ss;
    ss << "Неожиданный токен: '" << token << "'";
    addError(ErrorCode::SYN_UNEXPECTED_TOKEN, ss.str(), line, col, token.length());
}

void ErrorReporter::reportExpectedExpression(int line, int col, const std::string& found) {
    std::stringstream ss;
    ss << "Ожидалось выражение, получен '" << found << "'";
    addError(ErrorCode::SYN_EXPECTED_EXPRESSION, ss.str(), line, col, found.length());
}

void ErrorReporter::reportExpectedIdentifier(int line, int col, const std::string& found) {
    std::stringstream ss;
    ss << "Ожидался идентификатор, получен '" << found << "'";
    addError(ErrorCode::SYN_EXPECTED_IDENTIFIER, ss.str(), line, col, found.length());
}

void ErrorReporter::reportExpectedType(int line, int col, const std::string& found) {
    std::stringstream ss;
    ss << "Ожидался тип, получен '" << found << "'";
    addError(ErrorCode::SYN_EXPECTED_TYPE, ss.str(), line, col, found.length());
}

void ErrorReporter::reportUnclosedBlock(int line, int col) {
    addError(ErrorCode::SYN_UNCLOSED_BLOCK, 
             "Незакрытый блок (ожидается '}')", 
             line, col, 1);
}

// ============================================================================
// Семантические ошибки
// ============================================================================

void ErrorReporter::reportUndeclared(int line, int col, const std::string& identifier) {
    std::stringstream ss;
    ss << "Необъявленный идентификатор: '" << identifier << "'";
    addError(ErrorCode::SEM_UNDECLARED_IDENTIFIER, ss.str(), line, col, identifier.length());
}

void ErrorReporter::reportDuplicateDeclaration(int line, int col, const std::string& name, const std::string& kind) {
    std::stringstream ss;
    ss << "Повторное объявление " << kind << ": '" << name << "'";
    addError(ErrorCode::SEM_DUPLICATE_DECLARATION, ss.str(), line, col, name.length());
}

void ErrorReporter::reportTypeMismatch(int line, int col, const std::string& expected, const std::string& got) {
    std::stringstream ss;
    ss << "Несоответствие типов: ожидался '" << expected << "', получен '" << got << "'";
    addError(ErrorCode::SEM_TYPE_MISMATCH, ss.str(), line, col, 1);
}

void ErrorReporter::reportWrongArgumentCount(int line, int col, int expected, int got, const std::string& func) {
    std::stringstream ss;
    ss << "Неверное количество аргументов функции '" << func 
       << "': ожидалось " << expected << ", получено " << got;
    addError(ErrorCode::SEM_WRONG_ARGUMENT_COUNT, ss.str(), line, col, 1);
}

void ErrorReporter::reportArgumentTypeMismatch(int line, int col, int argNum,
                                                const std::string& expected, const std::string& got) {
    std::stringstream ss;
    ss << "Несовместимый тип аргумента " << argNum 
       << ": ожидался '" << expected << "', получен '" << got << "'";
    addError(ErrorCode::SEM_ARGUMENT_TYPE_MISMATCH, ss.str(), line, col, 1);
}

void ErrorReporter::reportReturnTypeMismatch(int line, int col, const std::string& expected, const std::string& got) {
    std::stringstream ss;
    ss << "Несовместимый тип возврата: ожидался '" << expected << "', получен '" << got << "'";
    addError(ErrorCode::SEM_RETURN_TYPE_MISMATCH, ss.str(), line, col, 1);
}

void ErrorReporter::reportMissingReturn(int line, int col, const std::string& function) {
    std::stringstream ss;
    ss << "Функция '" << function << "' должна возвращать значение";
    addError(ErrorCode::SEM_MISSING_RETURN, ss.str(), line, col, 1);
}

void ErrorReporter::reportInvalidCondition(int line, int col, const std::string& type) {
    std::stringstream ss;
    ss << "Условие должно иметь тип bool, получен '" << type << "'";
    addError(ErrorCode::SEM_INVALID_CONDITION_TYPE, ss.str(), line, col, 1);
}

void ErrorReporter::reportInvalidArrayIndex(int line, int col, const std::string& type) {
    std::stringstream ss;
    ss << "Индекс массива должен быть целым числом, получен '" << type << "'";
    addError(ErrorCode::SEM_INVALID_ARRAY_INDEX, ss.str(), line, col, 1);
}

void ErrorReporter::reportNonArrayIndex(int line, int col) {
    addError(ErrorCode::SEM_NON_ARRAY_INDEX, 
             "Оператор [] можно использовать только с массивами", 
             line, col, 1);
}

void ErrorReporter::reportInvalidMemberAccess(int line, int col, const std::string& structName, const std::string& member) {
    std::stringstream ss;
    ss << "Поле '" << member << "' не найдено в структуре '" << structName << "'";
    addError(ErrorCode::SEM_INVALID_MEMBER_ACCESS, ss.str(), line, col, member.length());
}

void ErrorReporter::reportGeneralError(int line, int col, const std::string& message) {
    addError(ErrorCode::GEN_SYNTAX_ERROR, message, line, col, 1);
}

// ============================================================================
// Вывод ошибок
// ============================================================================

void ErrorReporter::printErrors() const {
    for (const auto& error : errors) {
        std::cerr << error.getFullMessage() << std::endl;
        std::cerr << "Файл:" << filename << std::endl;
        std::cerr << "Строка:" << error.location.line 
                  << ", позиция:" << error.location.column << std::endl;
        
        if (!error.context.empty()) {
            std::cerr << "Код:" << error.context << std::endl;
            
            std::string spaces(error.location.column - 1, ' ');
            std::cerr << spaces << "    ^" << std::endl;
        }
        
        if (!error.suggestion.empty()) {
            std::cerr << "Подсказка:" << error.suggestion << std::endl;
        }
        std::cerr << std::endl;
    }
    
    if (!errors.empty()) {
        std::cerr << "Всего ошибок:" << errors.size() << std::endl;
    }
}

std::string ErrorReporter::getErrorsAsString() const {
    std::stringstream ss;
    for (const auto& error : errors) {
        ss << error.getFullMessage() << "\n";
        ss << "Файл:" << filename << "\n";
        ss << "Строка:" << error.location.line 
           << ", позиция:" << error.location.column << "\n";
        
        if (!error.context.empty()) {
            ss << "Код:" << error.context << "\n";
            std::string spaces(error.location.column - 1, ' ');
            ss << spaces << "^\n";
        }
        
        if (!error.suggestion.empty()) {
            ss << "Подсказка:" << error.suggestion << "\n";
        }
        ss << "\n";
    }
    return ss.str();
}

void ErrorReporter::printStats() const {
    std::cerr << "\nСтатистика компиляции:" << std::endl;
    std::cerr << "   Всего ошибок: " << errors.size() << std::endl;
    std::cerr << "   Лексических: " << countErrorsByPrefix("LEX") << std::endl;
    std::cerr << "   Синтаксических: " << countErrorsByPrefix("SYN") << std::endl;
    std::cerr << "   Семантических: " << countErrorsByPrefix("SEM") << std::endl;
    std::cerr << "   Препроцессора: " << countErrorsByPrefix("PRE") << std::endl;
    std::cerr << "   Общих: " << countErrorsByPrefix("GEN") << std::endl;
    std::cerr << "   Попыток восстановления: " << recoveryAttempts << std::endl;
    std::cerr << "   Успешных восстановлений: " << successfulRecoveries << std::endl;
}

int ErrorReporter::countErrorsByPrefix(const std::string& prefix) const {
    return std::count_if(errors.begin(), errors.end(),
        [&](const Error& e) { 
            std::string codeStr = errorCodeToString(e.code);
            if (codeStr.length() >= 3) {
                return codeStr.substr(0, 3) == prefix;
            }
            return false;
        });
}

} // namespace minicompiler