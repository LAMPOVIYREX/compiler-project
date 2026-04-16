#include "preprocessor/Preprocessor.hpp"
#include <sstream>
#include <cctype>
#include <algorithm>
#include <iostream>
#include <regex>
#include <cstdlib>
#include <cstring>

namespace minicompiler {

Preprocessor::Preprocessor(const std::string& source) : source(source) {
    // Добавляем встроенные макросы
    define("__LINE__", "0");
    define("__FILE__", "\"\"");
    define("__VERSION__", "\"1.0.0\"");
}

std::string Preprocessor::process() {
    std::istringstream stream(source);
    std::string line;
    std::ostringstream result;
    int originalLineNum = 1;
    int processedLineNum = 1;
    
    // Очищаем состояние
    macros.clear();
    ifStack.clear();
    originalToProcessed.clear();
    processedToOriginal.clear();
    
    // Добавляем встроенные макросы
    define("__LINE__", "0");
    define("__FILE__", "\"\"");
    define("__VERSION__", "\"1.0.0\"");
    
    while (std::getline(stream, line)) {
        // Сохраняем маппинг строк
        originalToProcessed.push_back(processedLineNum);
        
        // Обрабатываем строку
        std::string processed = processLine(line, originalLineNum);
        
        if (!processed.empty()) {
            result << processed << '\n';
            
            // Обновляем маппинг для обработанных строк
            ptrdiff_t newlineCount = std::count(processed.begin(), processed.end(), '\n');
            for (ptrdiff_t i = 0; i < newlineCount + 1; i++) {
                processedToOriginal.push_back(originalLineNum);
            }
            processedLineNum++;
        }
        
        originalLineNum++;
    }
    
    // Проверяем незакрытые директивы
    if (!ifStack.empty()) {
        addError(ifStack.back().line, "Unterminated #if/#ifdef/#ifndef");
    }
    
    processedSource = result.str();
    return processedSource;
}

void Preprocessor::define(const std::string& name, const std::string& value) {
    macros[name] = {name, value, -1};
}

void Preprocessor::undefine(const std::string& name) {
    macros.erase(name);
}

bool Preprocessor::isDefined(const std::string& name) const {
    return macros.find(name) != macros.end();
}

int Preprocessor::getOriginalLine(int processedLine) const {
    if (processedLine > 0 && processedLine <= static_cast<int>(processedToOriginal.size())) {
        return processedToOriginal[processedLine - 1];
    }
    return -1;
}

int Preprocessor::getProcessedLine(int originalLine) const {
    if (originalLine > 0 && originalLine <= static_cast<int>(originalToProcessed.size())) {
        return originalToProcessed[originalLine - 1];
    }
    return -1;
}

void Preprocessor::addError(int line, const std::string& message) {
    std::ostringstream oss;
    oss << "Preprocessor error at line " << line << ": " << message;
    errors.push_back(oss.str());
}

bool Preprocessor::isInActiveRegion() const {
    if (ifStack.empty()) return true;
    return ifStack.back().isActive;
}

// Оценка выражения #if
bool Preprocessor::evaluateIfExpression(const std::string& expr) {
    std::string trimmed = trim(expr);
    
    // Пустое выражение = false
    if (trimmed.empty()) return false;
    
    // Поддержка defined(NAME)
    std::regex definedRegex(R"(defined\s*\(\s*(\w+)\s*\))");
    std::smatch match;
    std::string working = trimmed;
    
    // Заменяем все defined(NAME) на 1 или 0
    while (std::regex_search(working, match, definedRegex)) {
        std::string name = match[1];
        std::string replacement = isDefined(name) ? "1" : "0";
        working.replace(match.position(0), match.length(0), replacement);
    }
    
    // Поддержка числовых сравнений
    std::regex compareRegex(R"((\w+)\s*(==|!=|<=|>=|<|>)\s*(\w+))");
    if (std::regex_search(working, match, compareRegex)) {
        std::string left = match[1];
        std::string op = match[2];
        std::string right = match[3];
        
        // Получаем значения
        long leftVal = getValueAsNumber(left);
        long rightVal = getValueAsNumber(right);
        
        if (op == "==") return leftVal == rightVal;
        if (op == "!=") return leftVal != rightVal;
        if (op == "<") return leftVal < rightVal;
        if (op == "<=") return leftVal <= rightVal;
        if (op == ">") return leftVal > rightVal;
        if (op == ">=") return leftVal >= rightVal;
    }
    
    // Простая поддержка операторов
    // Убираем пробелы
    working.erase(std::remove_if(working.begin(), working.end(), ::isspace), working.end());
    
    // Проверяем на !NAME
    if (!working.empty() && working[0] == '!') {
        std::string name = working.substr(1);
        return !evaluateSimpleExpression(name);
    }
    
    // Проверяем на NAME && NAME
    size_t andPos = working.find("&&");
    if (andPos != std::string::npos) {
        std::string left = working.substr(0, andPos);
        std::string right = working.substr(andPos + 2);
        return evaluateSimpleExpression(left) && evaluateSimpleExpression(right);
    }
    
    // Проверяем на NAME || NAME
    size_t orPos = working.find("||");
    if (orPos != std::string::npos) {
        std::string left = working.substr(0, orPos);
        std::string right = working.substr(orPos + 2);
        return evaluateSimpleExpression(left) || evaluateSimpleExpression(right);
    }
    
    // Простое имя или число
    return evaluateSimpleExpression(working);
}

// Получить значение как число
long Preprocessor::getValueAsNumber(const std::string& name) {
    // Проверяем, является ли это числом
    char* endptr;
    long num = std::strtol(name.c_str(), &endptr, 10);
    if (*endptr == '\0') {
        return num;
    }
    
    // Иначе это имя макроса
    if (isDefined(name)) {
        std::string val = macros[name].value;
        // Убираем кавычки если есть
        if (val.size() >= 2 && val[0] == '"' && val[val.size()-1] == '"') {
            val = val.substr(1, val.size() - 2);
        }
        long numVal = std::strtol(val.c_str(), &endptr, 10);
        if (*endptr == '\0') {
            return numVal;
        }
    }
    
    return 0; // По умолчанию
}

// Оценка простого выражения (имя или число)
bool Preprocessor::evaluateSimpleExpression(const std::string& expr) {
    // Проверяем, является ли это числом
    char* endptr;
    long num = std::strtol(expr.c_str(), &endptr, 10);
    if (*endptr == '\0') {
        return num != 0; // 0 = false, всё остальное = true
    }
    
    // Иначе это имя макроса
    if (isDefined(expr)) {
        std::string val = macros[expr].value;
        // Убираем кавычки если есть
        if (val.size() >= 2 && val[0] == '"' && val[val.size()-1] == '"') {
            return true; // Строковый макрос считается true
        }
        // Пробуем интерпретировать значение как число
        long numVal = std::strtol(val.c_str(), &endptr, 10);
        if (*endptr == '\0') {
            return numVal != 0;
        }
        // Не число, но определено = true
        return true;
    }
    
    return false; // Неопределенный макрос = false
}

std::string Preprocessor::processLine(const std::string& line, int lineNum) {
    std::string trimmed = trim(line);
    
    // Пустая строка
    if (trimmed.empty()) {
        return "";
    }
    
    // Директива препроцессора
    if (trimmed[0] == '#') {
        auto [directive, args] = splitDirective(trimmed);
        
        // Раскрываем макросы в аргументах директивы
        std::string expandedArgs = expandMacros(args, false);
        
        return handleDirective(directive, expandedArgs, lineNum);
    }
    
    // Обычная строка - обрабатываем, только если мы в активной области
    if (isInActiveRegion()) {
        // Заменяем макрос __LINE__
        std::string lineWithMacro = line;
        size_t pos = 0;
        while ((pos = lineWithMacro.find("__LINE__", pos)) != std::string::npos) {
            lineWithMacro.replace(pos, 8, std::to_string(lineNum));
            pos += std::to_string(lineNum).length();
        }
        
        // Раскрываем остальные макросы
        return expandMacros(lineWithMacro, true);
    }
    
    return "";  // Строка в неактивной области игнорируется
}

std::string Preprocessor::expandMacros(const std::string& text, bool /*inCode*/) {
    std::string result = text;
    
    // Продолжаем раскрывать, пока есть изменения
    bool changed;
    int maxIterations = 100;
    int iter = 0;
    
    do {
        changed = false;
        iter++;
        if (iter > maxIterations) break;
        
        for (const auto& [name, macro] : macros) {
            if (name == "__LINE__") continue;
            
            size_t pos = 0;
            while ((pos = result.find(name, pos)) != std::string::npos) {
                // Проверяем границы слова
                bool isWordStart = (pos == 0);
                if (!isWordStart) {
                    char prev = result[pos - 1];
                    isWordStart = !isalnum(prev) && prev != '_';
                }
                
                bool isWordEnd = (pos + name.length() >= result.length());
                if (!isWordEnd) {
                    char next = result[pos + name.length()];
                    isWordEnd = !isalnum(next) && next != '_';
                }
                
                if (isWordStart && isWordEnd) {
                    result.replace(pos, name.length(), macro.value);
                    changed = true;
                    break;
                }
                pos += name.length();
            }
            if (changed) break;
        }
    } while (changed);
    
    return result;
}

std::string Preprocessor::handleDirective(const std::string& directive, 
                                          const std::string& args, 
                                          int lineNum) {
    if (directive == "define") {
        // #define NAME VALUE
        std::vector<std::string> tokens = tokenize(args);
        if (tokens.size() >= 2) {
            std::string name = tokens[0];
            std::string value;
            for (size_t i = 1; i < tokens.size(); i++) {
                if (i > 1) value += " ";
                value += tokens[i];
            }
            macros[name] = {name, value, lineNum};
        } else {
            addError(lineNum, "Invalid #define syntax");
        }
        return "";  // Директива не попадает в вывод
    }
    else if (directive == "undef") {
        // #undef NAME
        std::string name = trim(args);
        if (!name.empty()) {
            undefine(name);
        } else {
            addError(lineNum, "Invalid #undef syntax");
        }
        return "";
    }
    else if (directive == "ifdef") {
        // #ifdef NAME
        std::string name = trim(args);
        bool defined = isDefined(name);
        IfState state{defined, false, lineNum};
        ifStack.push_back(state);
        return "";
    }
    else if (directive == "ifndef") {
        // #ifndef NAME
        std::string name = trim(args);
        bool defined = !isDefined(name);
        IfState state{defined, false, lineNum};
        ifStack.push_back(state);
        return "";
    }
    else if (directive == "if") {
        // #if expression
        bool result = evaluateIfExpression(args);
        IfState state{result, false, lineNum};
        ifStack.push_back(state);
        return "";
    }
    else if (directive == "elif") {
        // #elif expression
        if (ifStack.empty()) {
            addError(lineNum, "#elif without #if");
            return "";
        }
        
        IfState& state = ifStack.back();
        if (state.hasBeenTrue) {
            state.isActive = false;
        } else {
            bool result = evaluateIfExpression(args);
            state.isActive = result;
            if (result) {
                state.hasBeenTrue = true;
            }
        }
        return "";
    }
    else if (directive == "else") {
        // #else
        if (ifStack.empty()) {
            addError(lineNum, "#else without #if/#ifdef/#ifndef");
            return "";
        }
        
        IfState& state = ifStack.back();
        if (state.hasBeenTrue) {
            state.isActive = false;
        } else {
            state.isActive = !state.isActive;
            state.hasBeenTrue = true;
        }
        return "";
    }
    else if (directive == "endif") {
        // #endif
        if (ifStack.empty()) {
            addError(lineNum, "#endif without #if/#ifdef/#ifndef");
            return "";
        }
        ifStack.pop_back();
        return "";
    }
    else if (directive == "error") {
        // #error message
        addError(lineNum, "#error: " + args);
        return "";
    }
    else if (directive == "warning") {
        // #warning message
        std::cerr << "Warning at line " << lineNum << ": " << args << std::endl;
        return "";
    }
    else if (directive == "include") {
        // #include "file" - заглушка для будущей реализации
        return "";
    }
    else {
        addError(lineNum, "Unknown preprocessor directive: #" + directive);
        return "";
    }
}

std::pair<std::string, std::string> Preprocessor::splitDirective(const std::string& line) {
    std::string trimmed = trim(line);
    if (trimmed.empty() || trimmed[0] != '#') {
        return {"", ""};
    }
    
    // Убираем '#'
    std::string withoutHash = trim(trimmed.substr(1));
    
    // Разделяем на директиву и аргументы
    size_t spacePos = withoutHash.find_first_of(" \t");
    if (spacePos == std::string::npos) {
        return {withoutHash, ""};
    }
    
    std::string directive = withoutHash.substr(0, spacePos);
    std::string args = trim(withoutHash.substr(spacePos + 1));
    
    return {directive, args};
}

std::string Preprocessor::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

std::vector<std::string> Preprocessor::tokenize(const std::string& str) {
    std::vector<std::string> tokens;
    std::string current;
    bool inString = false;
    
    for (size_t i = 0; i < str.length(); i++) {
        char c = str[i];
        
        if (c == '"') {
            inString = !inString;
            current += c;
        }
        else if (inString) {
            current += c;
        }
        else if (std::isspace(static_cast<unsigned char>(c))) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        }
        else {
            current += c;
        }
    }
    
    if (!current.empty()) {
        tokens.push_back(current);
    }
    
    return tokens;
}

} // namespace minicompiler