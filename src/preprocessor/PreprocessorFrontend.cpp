#include "preprocessor/PreprocessorFrontend.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>  

namespace minicompiler {

PreprocessorFrontend::PreprocessorFrontend(ErrorReporter& errorReporter)
    : errorReporter(errorReporter) {}

std::string PreprocessorFrontend::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        errorReporter.report(0, 0, "Cannot open file: " + filename);
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    // Удаляем UTF-8 BOM если есть
    if (content.size() >= 3 && 
        static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF) {
        content = content.substr(3);
    }
    
    return content;
}

std::string PreprocessorFrontend::preprocessFile(const std::string& filename) {
    std::string source = readFile(filename);
    if (source.empty()) {
        return "";
    }
    
    preprocessor = std::make_unique<Preprocessor>(source);
    preprocessedSource = preprocessor->process();
    
    // Передаем ошибки препроцессора в ErrorReporter
    for (const auto& error : preprocessor->getErrors()) {
        // Парсим строку ошибки для получения номера строки
        size_t linePos = error.find("line ");
        if (linePos != std::string::npos) {
            int line = std::stoi(error.substr(linePos + 5));
            errorReporter.report(line, 1, error);
        } else {
            errorReporter.report(0, 0, error);
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
    Scanner scanner(preprocessed, errorReporter);
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