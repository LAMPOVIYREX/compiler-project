#pragma once
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <iostream>
#include <algorithm>
#include "ErrorCodes.hpp"

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
    ErrorCode code;
    std::string message;
    ErrorLocation location;
    std::string context;
    std::string suggestion;
    bool isWarning;
    
    Error(ErrorCode c, const std::string& msg, const ErrorLocation& loc,
          const std::string& ctx = "", const std::string& sug = "", bool warn = false)
        : code(c), message(msg), location(loc), context(ctx), suggestion(sug), isWarning(warn) {}
    
    std::string getFullMessage() const {
        std::stringstream ss;
        if (isWarning) {
            ss << "ПРЕДУПРЕЖДЕНИЕ";
        } else {
            ss << "ОШИБКА";
        }
        ss << " [" << errorCodeToString(code) << "]: " << message;
        return ss.str();
    }
};

class ErrorReporter {
public:
    ErrorReporter() : fatalError(false), recoveryAttempts(0), successfulRecoveries(0) {}
    
    void setFilename(const std::string& name) { filename = name; }
    void setSourceLine(int lineNum, const std::string& line) {
        sourceLines[lineNum] = line;
    }
    
    bool hasFatalError() const { return fatalError; }
    void setFatalError(bool fatal) { fatalError = fatal; }
    
    // Основной метод добавления ошибки
    void addError(ErrorCode code, const std::string& message,
                  int line, int column, int length = 1,
                  const std::string& context = "", const std::string& suggestion = "");
    
    void addWarning(ErrorCode code, const std::string& message,
                    int line, int column, int length = 1,
                    const std::string& context = "");
    
    // Лексические ошибки
    void reportUnexpectedChar(int line, int col, char c);
    void reportInvalidNumber(int line, int col, const std::string& num);
    void reportUnterminatedString(int line, int col, int startLine);
    void reportNewlineInString(int line, int col);
    void reportInvalidEscape(int line, int col, char escape);
    void reportUnterminatedComment(int line, int col, int startLine);
    void reportIdentifierTooLong(int line, int col, int length, int maxLength);
    
    // Синтаксические ошибки
    void reportMissingSemicolon(int line, int col, const std::string& found);
    void reportMissingParen(int line, int col);
    void reportMissingBrace(int line, int col);
    void reportMissingBracket(int line, int col);
    void reportUnexpectedToken(int line, int col, const std::string& token);
    void reportExpectedExpression(int line, int col, const std::string& found);
    void reportExpectedIdentifier(int line, int col, const std::string& found);
    void reportExpectedType(int line, int col, const std::string& found);
    void reportUnclosedBlock(int line, int col);
    
    // Семантические ошибки
    void reportUndeclared(int line, int col, const std::string& identifier);
    void reportDuplicateDeclaration(int line, int col, const std::string& name, const std::string& kind);
    void reportTypeMismatch(int line, int col, const std::string& expected, const std::string& got);
    void reportWrongArgumentCount(int line, int col, int expected, int got, const std::string& func);
    void reportArgumentTypeMismatch(int line, int col, int argNum, 
                                    const std::string& expected, const std::string& got);
    void reportReturnTypeMismatch(int line, int col, const std::string& expected, const std::string& got);
    void reportMissingReturn(int line, int col, const std::string& function);
    void reportInvalidCondition(int line, int col, const std::string& type);
    void reportInvalidArrayIndex(int line, int col, const std::string& type);
    void reportNonArrayIndex(int line, int col);
    void reportInvalidMemberAccess(int line, int col, const std::string& structName, const std::string& member);
    
    // Общий метод
    void reportGeneralError(int line, int col, const std::string& message);
    
    // Вывод ошибок (только объявление)
    void printErrors() const;
    std::string getErrorsAsString() const;
    
    bool hasErrors() const { return !errors.empty(); }
    void clear() { errors.clear(); fatalError = false; recoveryAttempts = 0; successfulRecoveries = 0; }
    
    const std::vector<Error>& getErrors() const { return errors; }
    
    // Recovery tracking
    void recoveryAttempt(bool success) {
        recoveryAttempts++;
        if (success) successfulRecoveries++;
    }
    
    void printStats() const;

private:
    std::string filename;
    std::vector<Error> errors;
    std::map<int, std::string> sourceLines;
    bool fatalError;
    int recoveryAttempts;
    int successfulRecoveries;
    
    std::string getContext(int line);
    std::string generatePointer(int col, int length);
    std::string getErrorMessage(ErrorCode code, const std::string& details);
    std::string getErrorSuggestion(ErrorCode code);
    
    int countErrorsByPrefix(const std::string& prefix) const;
};

} // namespace minicompiler