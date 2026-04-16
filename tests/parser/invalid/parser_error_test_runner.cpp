#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <filesystem>
#include "lexer/Scanner.hpp"
#include "parser/Parser.hpp"
#include "utils/ErrorReporter.hpp"
#include "utils/ErrorCodes.hpp"

using namespace minicompiler;
namespace fs = std::filesystem;

struct ErrorTestCase {
    std::string name;
    std::string inputFile;
    std::string expectedFile;
};

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool runErrorTest(const ErrorTestCase& test) {
    std::cout << "Error Test: " << test.name << "... ";
    
    std::string source = readFile(test.inputFile);
    if (source.empty()) {
        std::cout << "FAILED (cannot read input)" << std::endl;
        return false;
    }
    
    ErrorReporter errorReporter;
    errorReporter.setFilename(test.inputFile);
    
    // Set source lines
    std::istringstream stream(source);
    std::string line;
    int lineNum = 1;
    while (std::getline(stream, line)) {
        errorReporter.setSourceLine(lineNum, line);
        lineNum++;
    }
    
    // Lexical analysis
    Scanner scanner(source, errorReporter);
    auto tokens = scanner.scanTokens();
    
    // Parsing
    Parser parser(tokens, errorReporter);
    auto program = parser.parse();
    
    // Get errors
    const auto& errors = errorReporter.getErrors();
    
    // Read expected output
    std::string expected = readFile(test.expectedFile);
    if (expected.empty()) {
        std::cout << "FAILED (cannot read expected)" << std::endl;
        return false;
    }
    
    // Generate actual error output
    std::stringstream actual;
    
    for (const auto& error : errors) {
        actual << "ОШИБКА [" << errorCodeToString(error.code) << "]: " << error.message << std::endl;
        actual << "Файл:" << test.inputFile << std::endl;
        actual << "Строка:" << error.location.line << ", позиция:" << error.location.column << std::endl;
        
        if (!error.context.empty()) {
            actual << "Код:" << error.context << std::endl;
            std::string spaces(error.location.column - 1, ' ');
            actual << spaces << "    ^" << std::endl;
        }
        
        if (!error.suggestion.empty()) {
            actual << "Подсказка:" << error.suggestion << std::endl;
        }
        actual << std::endl;
    }
    
    if (!errors.empty()) {
        actual << "Всего ошибок: " << errors.size() << std::endl;
    }
    
    // Compare
    std::string actualStr = actual.str();
    // Удаляем последний перевод строки для сравнения
    while (!actualStr.empty() && actualStr.back() == '\n') actualStr.pop_back();
    while (!expected.empty() && expected.back() == '\n') expected.pop_back();
    
    if (actualStr == expected) {
        std::cout << "PASSED" << std::endl;
        return true;
    } else {
        std::cout << "FAILED (output mismatch)" << std::endl;
        
        // Save actual output
        std::string tempFile = test.inputFile + ".actual";
        std::ofstream actualOut(tempFile);
        actualOut << actualStr;
        actualOut.close();
        
        std::cout << "  Expected: " << test.expectedFile << std::endl;
        std::cout << "  Actual:   " << tempFile << std::endl;
        std::cout << "  Run: diff -u " << test.expectedFile << " " << tempFile << std::endl;
        
        return false;
    }
}

int main() {
    std::vector<ErrorTestCase> tests = {
        {
            "Missing semicolon",
            "tests/parser/invalid/syntax_errors/missing_semicolon.src",
            "tests/parser/invalid/expected/missing_semicolon.expected"
        },
        {
            "Missing parenthesis",
            "tests/parser/invalid/syntax_errors/missing_paren.src",
            "tests/parser/invalid/expected/missing_paren.expected"
        },
        {
            "Unclosed brace",
            "tests/parser/invalid/syntax_errors/unclosed_brace.src",
            "tests/parser/invalid/expected/unclosed_brace.expected"
        },
        {
            "Unexpected token",
            "tests/parser/invalid/syntax_errors/unexpected_token.src",
            "tests/parser/invalid/expected/unexpected_token.expected"
        },
        {
            "Invalid expression",
            "tests/parser/invalid/syntax_errors/invalid_expression.src",
            "tests/parser/invalid/expected/invalid_expression.expected"
        },
        {
            "Empty statement",
            "tests/parser/invalid/syntax_errors/empty_statement.src",
            "tests/parser/invalid/expected/empty_statement.expected"
        },
        {
            "Missing identifier",
            "tests/parser/invalid/syntax_errors/missing_identifier.src",
            "tests/parser/invalid/expected/missing_identifier.expected"
        },
        {
            "Missing parenthesis custom",
            "tests/parser/invalid/syntax_errors/missing_paren_custom.src",
            "tests/parser/invalid/expected/missing_paren_custom.expected"
        },
        {
            "Double operator invalid",
            "tests/parser/invalid/syntax_errors/double_operator_invalid.src",
            "tests/parser/invalid/expected/double_operator_invalid.expected"
        },
        {
            "BBB",
            "tests/parser/invalid/syntax_errors/bbb.src",
            "tests/parser/invalid/expected/bbb.expected"
        }
    };
    
    std::cout << "=== Parser Error Tests (" << tests.size() << " tests) ===\n" << std::endl;
    
    int passed = 0;
    for (const auto& test : tests) {
        if (runErrorTest(test)) {
            passed++;
        }
        std::cout << std::endl;
    }
    
    std::cout << "========================================" << std::endl;
    std::cout << "Passed: " << passed << "/" << tests.size() << std::endl;
    
    return 0;
}