#pragma once
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

namespace minicompiler {

struct ErrorLocation {
    std::string filename;
    int line;
    int column;
    int length;
    
    ErrorLocation() : line(0), column(0), length(1) {}
    ErrorLocation(int l, int c) : line(l), column(c), length(1) {}
    ErrorLocation(int l, int c, int len) : line(l), column(c), length(len) {}
};

struct Error {
    std::string code;        // e.g., "LEX-001"
    std::string message;
    ErrorLocation location;
    std::string context;      // The source line
    std::string pointer;      // Pointer to error location
    
    Error(const std::string& c, const std::string& msg, 
          const ErrorLocation& loc, const std::string& ctx = "", 
          const std::string& ptr = "")
        : code(c), message(msg), location(loc), context(ctx), pointer(ptr) {}
};

class ErrorReporter {
public:
    ErrorReporter() : fatalError(false) {}
    
    void setFilename(const std::string& name) { filename = name; }
    
    void setSourceLine(int lineNum, const std::string& line) {
        sourceLines[lineNum] = line;
    }
    
    bool hasFatalError() const { return fatalError; }
    void setFatalError(bool fatal) { fatalError = fatal; }
    
    // Лексические ошибки (LEX-001 до LEX-008)
    void reportLexical(int line, int col, const std::string& msg, char c) {
        std::stringstream ss;
        ss << "Unexpected character: '" << c << "' (ASCII: " << (int)c << ")";
        ErrorLocation loc(line, col, 1);
        addError("LEX-001", ss.str(), loc, getContext(line), generatePointer(col, 1));
    }
    
    void reportInvalidNumber(int line, int col, const std::string& num) {
        std::stringstream ss;
        ss << "Invalid integer literal: '" << num << "' (value out of range)";
        ErrorLocation loc(line, col, static_cast<int>(num.length()));
        addError("LEX-002", ss.str(), loc, getContext(line), generatePointer(col, num.length()));
    }
    
    void reportInvalidFloat(int line, int col, const std::string& num) {
        std::stringstream ss;
        ss << "Invalid float literal: '" << num << "'";
        ErrorLocation loc(line, col, static_cast<int>(num.length()));
        addError("LEX-003", ss.str(), loc, getContext(line), generatePointer(col, num.length()));
    }
    
    void reportUnterminatedString(int line, int col, int startLine) {
        std::stringstream ss;
        ss << "Unterminated string literal (starting at line " << startLine << ")";
        ErrorLocation loc(line, col, 1);
        addError("LEX-004", ss.str(), loc, getContext(line), generatePointer(col, 1));
    }
    
    void reportNewlineInString(int line, int col) {
        ErrorLocation loc(line, col, 1);
        addError("LEX-005", "Newline in string literal (use \\n)", 
                loc, getContext(line), generatePointer(col, 1));
    }
    
    void reportInvalidEscape(int line, int col, char escape) {
        std::stringstream ss;
        ss << "Invalid escape sequence: \\" << escape;
        ErrorLocation loc(line, col, 2);
        addError("LEX-006", ss.str(), loc, getContext(line), generatePointer(col, 2));
    }
    
    void reportUnterminatedComment(int line, int col, int startLine) {
        std::stringstream ss;
        ss << "Unterminated multi-line comment (started at line " << startLine << ")";
        ErrorLocation loc(line, col, 1);
        addError("LEX-007", ss.str(), loc, getContext(line), generatePointer(col, 1));
    }
    
    void reportIdentifierTooLong(int line, int col, int length, int maxLength) {
        std::stringstream ss;
        ss << "Identifier too long (max " << maxLength << " characters, got " << length << ")";
        ErrorLocation loc(line, col, length);
        addError("LEX-008", ss.str(), loc, getContext(line), generatePointer(col, length));
    }
    
    // Синтаксические ошибки (SYN-001 до SYN-008)
    void reportSyntax(int line, int col, const std::string& expected, 
                      const std::string& found) {
        std::stringstream ss;
        ss << "Expected '" << expected << "' but found '" << found << "'";
        ErrorLocation loc(line, col, static_cast<int>(found.length()));
        addError("SYN-001", ss.str(), loc, getContext(line), generatePointer(col, found.length()));
    }
    
    void reportUnexpectedToken(int line, int col, const std::string& token) {
        std::stringstream ss;
        ss << "Unexpected token '" << token << "'";
        ErrorLocation loc(line, col, static_cast<int>(token.length()));
        addError("SYN-002", ss.str(), loc, getContext(line), generatePointer(col, token.length()));
    }
    
    void reportMissingToken(int line, int col, const std::string& token, const std::string& context) {
        std::stringstream ss;
        ss << "Missing '" << token << "' after " << context;
        ErrorLocation loc(line, col, 1);
        addError("SYN-003", ss.str(), loc, getContext(line), generatePointer(col, 1));
    }
    
    void reportInvalidExpression(int line, int col, const std::string& expr) {
        std::stringstream ss;
        ss << "Invalid expression: " << expr;
        ErrorLocation loc(line, col, static_cast<int>(expr.length()));
        addError("SYN-004", ss.str(), loc, getContext(line), generatePointer(col, expr.length()));
    }
    
    void reportExpectedIdentifier(int line, int col, const std::string& found) {
        std::stringstream ss;
        ss << "Expected identifier but found '" << found << "'";
        ErrorLocation loc(line, col, static_cast<int>(found.length()));
        addError("SYN-005", ss.str(), loc, getContext(line), generatePointer(col, found.length()));
    }
    
    void reportEmptyStatement(int line, int col) {
        ErrorLocation loc(line, col, 1);
        addError("SYN-006", "Empty statement", loc, getContext(line), generatePointer(col, 1));
    }
    
    void reportUnclosedBlock(int line, int col) {
        ErrorLocation loc(line, col, 1);
        addError("SYN-007", "Unclosed block (expected '}' at end of file)", 
                loc, getContext(line), generatePointer(col, 1));
    }
    
    void reportInvalidAssignmentTarget(int line, int col, const std::string& target) {
        std::stringstream ss;
        ss << "Invalid assignment target: '" << target << "'";
        ErrorLocation loc(line, col, static_cast<int>(target.length()));
        addError("SYN-008", ss.str(), loc, getContext(line), generatePointer(col, target.length()));
    }
    
    // Семантические ошибки (SEM-001 до SEM-010)
    void reportUndeclared(int line, int col, const std::string& identifier) {
        std::stringstream ss;
        ss << "Undeclared identifier: '" << identifier << "'";
        ErrorLocation loc(line, col, static_cast<int>(identifier.length()));
        addError("SEM-001", ss.str(), loc, getContext(line), generatePointer(col, identifier.length()));
    }
    
    void reportRedeclaration(int line, int col, const std::string& identifier) {
        std::stringstream ss;
        ss << "Redeclaration of identifier: '" << identifier << "'";
        ErrorLocation loc(line, col, static_cast<int>(identifier.length()));
        addError("SEM-002", ss.str(), loc, getContext(line), generatePointer(col, identifier.length()));
    }
    
    void reportTypeMismatch(int line, int col, const std::string& expected, 
                           const std::string& got) {
        std::stringstream ss;
        ss << "Type mismatch: cannot assign " << got << " to " << expected;
        ErrorLocation loc(line, col, 1);
        addError("SEM-003", ss.str(), loc, getContext(line), generatePointer(col, 1));
    }
    
    void reportUndeclaredFunction(int line, int col, const std::string& function) {
        std::stringstream ss;
        ss << "Function '" << function << "' not declared";
        ErrorLocation loc(line, col, static_cast<int>(function.length()));
        addError("SEM-004", ss.str(), loc, getContext(line), generatePointer(col, function.length()));
    }
    
    void reportWrongArgumentCount(int line, int col, int expected, int got) {
        std::stringstream ss;
        ss << "Wrong number of arguments: expected " << expected << ", got " << got;
        ErrorLocation loc(line, col, 1);
        addError("SEM-005", ss.str(), loc, getContext(line), generatePointer(col, 1));
    }
    
    void reportArgumentTypeMismatch(int line, int col, const std::string& expected, 
                                   const std::string& got) {
        std::stringstream ss;
        ss << "Argument type mismatch: expected " << expected << ", got " << got;
        ErrorLocation loc(line, col, 1);
        addError("SEM-006", ss.str(), loc, getContext(line), generatePointer(col, 1));
    }
    
    void reportNonArrayIndex(int line, int col, const std::string& type) {
        std::stringstream ss;
        ss << "Cannot index non-array type '" << type << "'";
        ErrorLocation loc(line, col, 1);
        addError("SEM-007", ss.str(), loc, getContext(line), generatePointer(col, 1));
    }
    
    void reportInvalidIndexType(int line, int col, const std::string& type) {
        std::stringstream ss;
        ss << "Array index must be integer, got " << type;
        ErrorLocation loc(line, col, 1);
        addError("SEM-008", ss.str(), loc, getContext(line), generatePointer(col, 1));
    }
    
    void reportReturnTypeMismatch(int line, int col, const std::string& expected, 
                                 const std::string& got) {
        std::stringstream ss;
        ss << "Return type mismatch: expected " << expected << ", got " << got;
        ErrorLocation loc(line, col, 1);
        addError("SEM-009", ss.str(), loc, getContext(line), generatePointer(col, 1));
    }
    
    void reportMissingReturn(int line, int col, const std::string& function) {
        std::stringstream ss;
        ss << "Missing return statement in function '" << function << "'";
        ErrorLocation loc(line, col, 1);
        addError("SEM-010", ss.str(), loc, getContext(line), generatePointer(col, 1));
    }
    
    // Общая ошибка
    void reportGeneralError(int line, int col, const std::string& message) {
        ErrorLocation loc(line, col, 1);
        addError("GEN-001", message, loc, getContext(line), generatePointer(col, 1));
    }
    
    // Ошибка препроцессора
    void reportPreprocessorError(int line, int col, const std::string& message) {
        ErrorLocation loc(line, col, 1);
        addError("PRE-001", message, loc, getContext(line), generatePointer(col, 1));
    }
    
    // Печать всех ошибок
    void printErrors() const {
        for (const auto& error : errors) {
            std::cerr << "[ERROR] " << filename << ":" 
                      << error.location.line << ":" 
                      << error.location.column << ": "
                      << error.code << ": " << error.message << std::endl;
            
            if (!error.context.empty()) {
                std::cerr << "[CONTEXT] " << error.context << std::endl;
            }
            if (!error.pointer.empty()) {
                std::cerr << "[POINTER] " << error.pointer << std::endl;
            }
            std::cerr << std::endl;
        }
        
        if (!errors.empty()) {
            std::cerr << "Found " << errors.size() << " error(s)" << std::endl;
        }
    }
    
    // Печать ошибок в JSON формате (для интеграции с IDE)
    std::string printErrorsJSON() const {
        std::stringstream ss;
        ss << "{\n  \"errors\": [\n";
        
        for (size_t i = 0; i < errors.size(); i++) {
            const auto& error = errors[i];
            ss << "    {\n";
            ss << "      \"code\": \"" << error.code << "\",\n";
            ss << "      \"message\": \"" << error.message << "\",\n";
            ss << "      \"location\": {\n";
            ss << "        \"file\": \"" << filename << "\",\n";
            ss << "        \"line\": " << error.location.line << ",\n";
            ss << "        \"column\": " << error.location.column << ",\n";
            ss << "        \"length\": " << error.location.length << "\n";
            ss << "      },\n";
            ss << "      \"context\": \"" << error.context << "\",\n";
            ss << "      \"pointer\": \"" << error.pointer << "\"\n";
            ss << "    }";
            if (i < errors.size() - 1) ss << ",";
            ss << "\n";
        }
        
        ss << "  ]\n}\n";
        return ss.str();
    }
    
    bool hasErrors() const { return !errors.empty(); }
    
    void clear() { 
        errors.clear(); 
        recoveryAttempts = 0;
        successfulRecoveries = 0;
        fatalError = false;
    }
    
    // Recovery tracking
    void recoveryAttempt(bool success) {
        recoveryAttempts++;
        if (success) successfulRecoveries++;
    }
    
    void printStats() const {
        std::cerr << "\nCompilation Statistics:" << std::endl;
        std::cerr << "- Total errors: " << errors.size() << std::endl;
        std::cerr << "  - Lexical errors: " << countErrorsByPrefix("LEX") << std::endl;
        std::cerr << "  - Syntax errors: " << countErrorsByPrefix("SYN") << std::endl;
        std::cerr << "  - Semantic errors: " << countErrorsByPrefix("SEM") << std::endl;
        std::cerr << "  - Preprocessor errors: " << countErrorsByPrefix("PRE") << std::endl;
        std::cerr << "  - General errors: " << countErrorsByPrefix("GEN") << std::endl;
        std::cerr << "- Recovery attempts: " << recoveryAttempts << std::endl;
        std::cerr << "  - Successful: " << successfulRecoveries << std::endl;
        std::cerr << "  - Failed: " << (recoveryAttempts - successfulRecoveries) << std::endl;
    }
    
    const std::vector<Error>& getErrors() const { return errors; }

private:
    std::string filename;
    std::vector<Error> errors;
    std::map<int, std::string> sourceLines;
    int recoveryAttempts = 0;
    int successfulRecoveries = 0;
    bool fatalError;
    
    void addError(const std::string& code, const std::string& message,
                  const ErrorLocation& loc, const std::string& context = "",
                  const std::string& pointer = "") {
        errors.emplace_back(code, message, loc, context, pointer);
    }
    
    std::string getContext(int line) {
        auto it = sourceLines.find(line);
        if (it != sourceLines.end()) {
            return it->second;
        }
        return "";
    }
    
    std::string generatePointer(int col, int length) {
        if (col <= 0) return "";
        return std::string(col - 1, ' ') + std::string(length, '^');
    }
    
    int countErrorsByPrefix(const std::string& prefix) const {
        return std::count_if(errors.begin(), errors.end(),
            [&](const Error& e) { 
                return e.code.length() >= 3 && e.code.substr(0, 3) == prefix; 
            });
    }
};

} // namespace minicompiler