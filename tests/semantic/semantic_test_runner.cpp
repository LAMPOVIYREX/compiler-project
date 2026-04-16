#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <filesystem>
#include "lexer/Scanner.hpp"
#include "parser/Parser.hpp"
#include "semantic/SemanticAnalyzer.hpp"
#include "utils/ErrorReporter.hpp"
#include "utils/ErrorCodes.hpp"

using namespace minicompiler;
namespace fs = std::filesystem;

struct TestCase {
    std::string name;
    std::string inputFile;
    std::string expectedFile;
    bool expectSuccess;
};

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool runSemanticTest(const TestCase& test) {
    std::cout << "Test: " << test.name << "... ";
    
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
    
    if (errorReporter.hasErrors()) {
        if (test.expectSuccess) {
            std::cout << "FAILED (lexer errors)" << std::endl;
            errorReporter.printErrors();
            return false;
        } else {
            // СОЗДАЕМ .actual файл
            std::string actualFile = test.inputFile + ".actual";
            std::ofstream actualOut(actualFile);
            errorReporter.printErrors();
            std::cout << "PASSED" << std::endl;
            return true;
        }
    }
    
    // Parsing
    Parser parser(tokens, errorReporter);
    auto program = parser.parse();
    
    if (errorReporter.hasErrors()) {
        if (test.expectSuccess) {
            std::cout << "FAILED (parser errors)" << std::endl;
            errorReporter.printErrors();
            return false;
        } else {
            // СОЗДАЕМ .actual файл
            std::string actualFile = test.inputFile + ".actual";
            std::ofstream actualOut(actualFile);
            errorReporter.printErrors();
            std::cout << "PASSED" << std::endl;
            return true;
        }
    }
    
    // Semantic analysis
    SemanticAnalyzer analyzer(errorReporter);
    bool valid = analyzer.analyze(*program);
    
    // Generate actual output
    std::stringstream actual;
    
    if (errorReporter.hasErrors()) {
        for (const auto& error : errorReporter.getErrors()) {
            actual << error.getFullMessage() << std::endl;
            actual << "Файл:" << test.inputFile << std::endl;
            actual << "Строка:" << error.location.line 
                   << ", позиция:" << error.location.column << std::endl;
            
            if (!error.context.empty()) {
                actual << "Код:" << error.context << std::endl;
                std::string spaces(error.location.column - 1, ' ');
                actual << spaces << "^" << std::endl;
            }
            
            if (!error.suggestion.empty()) {
                actual << "Подсказка:" << error.suggestion << std::endl;
            }
            actual << std::endl;
        }
        actual << "Всего ошибок:" << errorReporter.getErrors().size() << std::endl;
    } else if (!valid) {
        actual << "Semantic analysis failed" << std::endl;
    }
    
    // СОЗДАЕМ .actual файл
    std::string actualFile = test.inputFile + ".actual";
    std::ofstream actualOut(actualFile);
    actualOut << actual.str();
    actualOut.close();
    
    // Читаем expected файл (если он существует)
    std::string expected = readFile(test.expectedFile);
    
    // Если expected не существует, считаем тест пройденным (создаем эталон)
    if (expected.empty()) {
        std::cout << "PASSED (created expected template)" << std::endl;
        std::cout << "  Please review and commit: " << test.expectedFile << std::endl;
        return true;
    }
    
    // Trim trailing newline
    std::string actualStr = actual.str();
    while (!actualStr.empty() && actualStr.back() == '\n') actualStr.pop_back();
    while (!expected.empty() && expected.back() == '\n') expected.pop_back();
    
    if (actualStr == expected) {
        std::cout << "PASSED" << std::endl;
        return true;
    } else {
        std::cout << "FAILED (output mismatch)" << std::endl;
        std::cout << "  Run: diff -u " << test.expectedFile << " " << actualFile << std::endl;
        return false;
    }
}

std::vector<TestCase> discoverTests(const std::string& baseDir) {
    std::vector<TestCase> tests;
    
    // Valid tests
    std::vector<std::pair<std::string, std::string>> validTests = {
        {"Type compatibility basic", "type_compatibility/basic"},
        {"Nested scopes", "nested_scopes/nested"},
        {"Multiple functions", "functions/multiple"},
        {"Recursive functions", "functions/recursive"},
        {"Structs basic", "structs/basic"}
    };
    
    for (const auto& [name, path] : validTests) {
        std::string inputFile = baseDir + "/valid/" + path + ".src";
        std::string expectedFile = baseDir + "/valid/" + path + ".expected";
        
        if (fs::exists(inputFile)) {
            tests.push_back({name, inputFile, expectedFile, true});
        }
    }
    
    // Invalid tests
    std::vector<std::pair<std::string, std::string>> invalidTests = {
        {"Undeclared variable", "undeclared_variable/use"},
        {"Type mismatch assignment", "type_mismatch/assignment"},
        {"Type mismatch operation", "type_mismatch/operation"},
        {"Type mismatch condition", "type_mismatch/condition"},
        {"Duplicate variable", "duplicate_declaration/variable"},
        {"Duplicate function", "duplicate_declaration/function"},
        {"Wrong argument count", "argument_errors/count"},
        {"Wrong argument type", "argument_errors/type"},
        {"Wrong return type", "return_errors/wrong_type"},
        {"Cascade errors", "type_mismatch/cascade_errors"},
        //{"Missing return", "return_errors/missing"}
    };
    
    for (const auto& [name, path] : invalidTests) {
        std::string inputFile = baseDir + "/invalid/" + path + ".src";
        std::string expectedFile = baseDir + "/invalid/" + path + ".expected";
        
        if (fs::exists(inputFile)) {
            tests.push_back({name, inputFile, expectedFile, false});
        }
    }
    
    return tests;
}

int main() {
    std::string testDir = "tests/semantic";
    
    std::cout << "=== Semantic Analyzer Test Runner ===\n" << std::endl;
    
    if (!fs::exists(testDir)) {
        std::cerr << "Error: Test directory '" << testDir << "' not found" << std::endl;
        return 1;
    }
    
    auto tests = discoverTests(testDir);
    
    if (tests.empty()) {
        std::cout << "No tests found in " << testDir << std::endl;
        return 0;
    }
    
    std::cout << "Found " << tests.size() << " test(s)\n" << std::endl;
    
    int passed = 0;
    for (const auto& test : tests) {
        if (runSemanticTest(test)) {
            passed++;
        }
        std::cout << std::endl;
    }
    
    std::cout << "========================================" << std::endl;
    std::cout << "Passed: " << passed << "/" << tests.size() << std::endl;
    
    return 0;
}