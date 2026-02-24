#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include "lexer/Scanner.hpp"
#include "utils/ErrorReporter.hpp"
#include "preprocessor/PreprocessorFrontend.hpp"

using namespace minicompiler;

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " <command> [options] <file>" << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  lex <file>           Tokenize source file" << std::endl;
    std::cout << "  preprocess <file>    Run preprocessor only" << std::endl;
    std::cout << "  compile <file>       Run preprocessor + lexer" << std::endl;
    std::cout << "  help                 Show this help message" << std::endl;
}

std::string cleanSource(const std::string& source) {
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
    
    for (size_t i = start; i < source.size(); i++) {
        unsigned char ch = static_cast<unsigned char>(source[i]);
        if (ch >= 32 || ch == '\n' || ch == '\t' || ch == '\r' || ch == '\0') {
            cleaned += source[i];
        } else {
            cleaned += ' ';
        }
    }
    
    return cleaned;
}

std::string readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return cleanSource(buffer.str());
}

void runLexer(const std::string& filename, bool usePreprocessor = false) {
    try {
        ErrorReporter errorReporter;
        
        if (usePreprocessor) {
            // Используем препроцессор + лексер
            PreprocessorFrontend frontend(errorReporter);
            auto tokens = frontend.tokenizeWithPreprocessor(filename);
            
            // Вывод токенов
            for (const auto& token : tokens) {
                std::cout << token.toString() << std::endl;
            }
        } else {
            // Только лексер
            std::string source = readFile(filename);
            Scanner scanner(source, errorReporter);
            auto tokens = scanner.scanTokens();
            
            for (const auto& token : tokens) {
                std::cout << token.toString() << std::endl;
            }
        }
        
        // Вывод ошибок, если есть
        if (errorReporter.hasErrors()) {
            std::cout << "\nErrors found:" << std::endl;
            errorReporter.printErrors();
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void runPreprocessor(const std::string& filename) {
    try {
        ErrorReporter errorReporter;
        PreprocessorFrontend frontend(errorReporter);
        
        std::string preprocessed = frontend.preprocessFile(filename);
        
        if (!preprocessed.empty()) {
            std::cout << "=== Preprocessed Output ===" << std::endl;
            std::cout << preprocessed << std::endl;
        }
        
        if (errorReporter.hasErrors()) {
            std::cout << "\nErrors found:" << std::endl;
            errorReporter.printErrors();
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string command = argv[1];
    
    if (command == "lex") {
        if (argc < 3) {
            std::cerr << "Error: Missing filename" << std::endl;
            printUsage(argv[0]);
            return 1;
        }
        runLexer(argv[2], false);
    }
    else if (command == "preprocess") {
        if (argc < 3) {
            std::cerr << "Error: Missing filename" << std::endl;
            printUsage(argv[0]);
            return 1;
        }
        runPreprocessor(argv[2]);
    }
    else if (command == "compile") {
        if (argc < 3) {
            std::cerr << "Error: Missing filename" << std::endl;
            printUsage(argv[0]);
            return 1;
        }
        runLexer(argv[2], true);
    }
    else if (command == "help") {
        printUsage(argv[0]);
    }
    else {
        std::cerr << "Error: Unknown command '" << command << "'" << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    return 0;
}