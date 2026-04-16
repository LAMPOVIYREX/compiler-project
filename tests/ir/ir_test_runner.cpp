// tests/ir/ir_test_runner.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <filesystem>
#include "lexer/Scanner.hpp"
#include "parser/Parser.hpp"
#include "semantic/SemanticAnalyzer.hpp"
#include "ir/IRGenerator.hpp"
#include "utils/ErrorReporter.hpp"

using namespace minicompiler;
namespace fs = std::filesystem;

struct IRTestCase {
    std::string name;
    std::string inputFile;
    std::string expectedFile;
    bool shouldFail;  // true для invalid тестов
};

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool runIRTest(const IRTestCase& test) {
    if (test.shouldFail) {
        std::cout << "IR Invalid Test: " << test.name << "... ";
    } else {
        std::cout << "IR Test: " << test.name << "... ";
    }
    std::cout.flush();
    
    ErrorReporter errorReporter;
    errorReporter.setFilename(test.inputFile);
    
    // Read source
    std::string source = readFile(test.inputFile);
    if (source.empty()) {
        std::cout << "FAILED (cannot read input)" << std::endl;
        return false;
    }
    
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
    
    // Semantic analysis
    SemanticAnalyzer analyzer(errorReporter);
    bool semanticValid = analyzer.analyze(*program);
    
    // Для invalid тестов ожидаем ошибки
    if (test.shouldFail) {
        if (!semanticValid || errorReporter.hasErrors()) {
            // Сохраняем ошибки в actual файл
            std::string actualFile = test.inputFile + ".actual";
            std::ofstream actualOut(actualFile);
            actualOut << errorReporter.getErrorsAsString();
            actualOut.close();
            
            // Проверяем expected
            if (!fs::exists(test.expectedFile)) {
                std::cout << "CREATED (expected file created)" << std::endl;
                std::ofstream expectedOut(test.expectedFile);
                expectedOut << errorReporter.getErrorsAsString();
                expectedOut.close();
                std::cout << "  Please review: " << test.expectedFile << std::endl;
                return true;
            }
            
            std::string expected = readFile(test.expectedFile);
            std::string actual = errorReporter.getErrorsAsString();
            
            if (actual == expected) {
                std::cout << "PASSED" << std::endl;
                return true;
            } else {
                std::cout << "FAILED (error mismatch)" << std::endl;
                std::cout << "  Run: diff -u " << test.expectedFile << " " << actualFile << std::endl;
                return false;
            }
        } else {
            std::cout << "FAILED (expected errors but got none)" << std::endl;
            return false;
        }
    }
    
    // Для валидных тестов ошибок быть не должно
    if (errorReporter.hasErrors()) {
        std::cout << "FAILED (unexpected errors)" << std::endl;
        errorReporter.printErrors();
        return false;
    }
    
    // IR Generation
    IRGenerator irGen(analyzer.getSymbolTable(), errorReporter);
    auto irProgram = irGen.generate(*program);
    
    if (irGen.hasErrors() || errorReporter.hasErrors()) {
        std::cout << "FAILED (IR generation errors)" << std::endl;
        errorReporter.printErrors();
        return false;
    }
    
    // Get actual IR output
    std::string actual = irProgram->toString();
    
    // Remove trailing newlines
    while (!actual.empty() && actual.back() == '\n') {
        actual.pop_back();
    }
    
    // Check expected file
    if (!fs::exists(test.expectedFile)) {
        std::cout << "CREATED (expected file created)" << std::endl;
        std::ofstream expectedOut(test.expectedFile);
        expectedOut << actual;
        expectedOut.close();
        std::cout << "  Please review: " << test.expectedFile << std::endl;
        return true;
    }
    
    std::string expected = readFile(test.expectedFile);
    while (!expected.empty() && expected.back() == '\n') {
        expected.pop_back();
    }
    
    if (actual == expected) {
        std::cout << "PASSED" << std::endl;
        return true;
    } else {
        std::cout << "FAILED (output mismatch)" << std::endl;
        
        std::string actualFile = test.inputFile + ".actual";
        std::ofstream actualOut(actualFile);
        actualOut << actual;
        actualOut.close();
        
        std::cout << "  Expected: " << test.expectedFile << std::endl;
        std::cout << "  Actual:   " << actualFile << std::endl;
        std::cout << "  Run: diff -u " << test.expectedFile << " " << actualFile << std::endl;
        
        return false;
    }
}

std::vector<IRTestCase> discoverIRTests(const std::string& baseDir) {
    std::vector<IRTestCase> tests;
    
    // Valid tests
    std::string validDir = baseDir + "/valid";
    if (fs::exists(validDir)) {
        for (const auto& entry : fs::recursive_directory_iterator(validDir)) {
            if (entry.path().extension() == ".mini") {
                std::string inputFile = entry.path().string();
                std::string expectedFile = inputFile + ".expected";
                std::string name = entry.path().stem().string();
                tests.push_back({name, inputFile, expectedFile, false});
            }
        }
    }
    
    // Invalid tests
    std::string invalidDir = baseDir + "/invalid";
    if (fs::exists(invalidDir)) {
        for (const auto& entry : fs::recursive_directory_iterator(invalidDir)) {
            if (entry.path().extension() == ".mini") {
                std::string inputFile = entry.path().string();
                std::string expectedFile = inputFile + ".expected";
                std::string name = entry.path().stem().string();
                tests.push_back({name, inputFile, expectedFile, true});
            }
        }
    }
    
    return tests;
}

int main() {
    std::string testDir = "tests/ir";
    
    std::cout << "=== MiniCompiler IR Test Runner ===\n" << std::endl;
    
    auto tests = discoverIRTests(testDir);
    
    if (tests.empty()) {
        std::cout << "No IR tests found!" << std::endl;
        return 0;
    }
    
    // Сортируем: сначала валидные, потом невалидные
    std::sort(tests.begin(), tests.end(), [](const IRTestCase& a, const IRTestCase& b) {
        if (a.shouldFail != b.shouldFail) {
            return !a.shouldFail;  // valid first
        }
        return a.name < b.name;
    });
    
    std::cout << "Found " << tests.size() << " test(s)\n" << std::endl;
    
    int passed = 0;
    int total = tests.size();
    
    for (const auto& test : tests) {
        if (runIRTest(test)) {
            passed++;
        }
        std::cout << std::endl;
    }
    
    std::cout << "========================================" << std::endl;
    std::cout << "IR Tests: " << passed << "/" << total << " passed" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return (passed == total) ? 0 : 1;
}