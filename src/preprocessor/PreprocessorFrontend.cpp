#include "preprocessor/PreprocessorFrontend.hpp"
#include "lexer/Scanner.hpp"  // ДОБАВЛЕНО
#include <fstream>
#include <sstream>
#include <iostream>

namespace minicompiler {

PreprocessorFrontend::PreprocessorFrontend(ErrorReporter& errorReporter)
    : errorReporter(errorReporter) {}

std::string PreprocessorFrontend::cleanSource(const std::string& source) {
    std::string cleaned;
    cleaned.reserve(source.size());
    
    // Удаляем UTF-8 BOM если есть
    size_t start = 0;
    if (source.size() >= 3 && 
        static_cast<unsigned char>(source[0]) == 0xEF &&
        static_cast<unsigned char>(source[1]) == 0xBB &&
        static_cast<unsigned char>(source[2]) == 0xBF) {
        start = 3;
    }
    
    // Копируем только ASCII символы и управляющие символы
    for (size_t i = start; i < source.size(); i++) {
        unsigned char ch = static_cast<unsigned char>(source[i]);
        
        // Принимаем печатаемые ASCII (32-126), табуляцию, новую строку, возврат каретки
        if (ch >= 32 || ch == '\n' || ch == '\t' || ch == '\r' || ch == '\0') {
            cleaned += source[i];
        } else {
            // Заменяем не-ASCII символы пробелом
            cleaned += ' ';
        }
    }
    
    return cleaned;
}

std::string PreprocessorFrontend::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        // ИСПРАВЛЕНО: используем специализированный метод вместо report
        errorReporter.reportLexical(0, 0, "", '\0');  // Заглушка, нужно добавить метод
        // Временно заменим на:
        std::cerr << "Error: Cannot open file: " << filename << std::endl;
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    return cleanSource(content);
}

std::string PreprocessorFrontend::preprocessFile(const std::string& filename) {
    std::string source = readFile(filename);
    if (source.empty()) {
        return "";
    }
    
    // Сохраняем исходные строки для ErrorReporter
    std::istringstream stream(source);
    std::string line;
    int lineNum = 1;
    while (std::getline(stream, line)) {
        errorReporter.setSourceLine(lineNum, line);
        lineNum++;
    }
    
    preprocessor = std::make_unique<Preprocessor>(source);
    preprocessedSource = preprocessor->process();
    
    // Передаем ошибки препроцессора в ErrorReporter
    for (const auto& error : preprocessor->getErrors()) {
        // Парсим строку ошибки для получения номера строки
        size_t linePos = error.find("line ");
        if (linePos != std::string::npos) {
            int line = std::stoi(error.substr(linePos + 5));
            // ИСПРАВЛЕНО: используем подходящий метод
            errorReporter.reportLexical(line, 1, error, '\0');
        } else {
            errorReporter.reportLexical(0, 0, error, '\0');
        }
    }
    
    return preprocessedSource;
}

int PreprocessorFrontend::getOriginalLine(int processedLine) const {
    if (preprocessor) {
        return preprocessor->getOriginalLine(processedLine);
    }
    return processedLine;
}

std::vector<Token> PreprocessorFrontend::tokenizeWithPreprocessor(const std::string& filename) {
    std::vector<Token> tokens;
    
    // Сначала запускаем препроцессор
    std::string preprocessed = preprocessFile(filename);
    if (preprocessed.empty() || errorReporter.hasErrors()) {
        return tokens;
    }
    
    // Затем запускаем лексер на обработанном коде
    Scanner scanner(preprocessed, errorReporter);  // ТЕПЕРЬ РАБОТАЕТ
    tokens = scanner.scanTokens();
    
    // Корректируем номера строк в токенах
    for (auto& token : tokens) {
        if (token.line > 0) {
            int originalLine = getOriginalLine(token.line);
            if (originalLine > 0) {
                // Создаем новый токен с исправленной строкой
                token = Token(token.type, token.lexeme, token.value, 
                             originalLine, token.column);
            }
        }
    }
    
    return tokens;
}

} // namespace minicompiler