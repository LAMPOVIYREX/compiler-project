#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <filesystem>
#include "lexer/Scanner.hpp"
#include "parser/Parser.hpp"
#include "parser/ASTPrettyPrinter.hpp"
#include "utils/ErrorReporter.hpp"

using namespace minicompiler;

namespace fs = std::filesystem;

struct TestCase {
    std::string name;
    std::string inputFile;
    std::string expectedFile;
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

bool runParserTest(const TestCase& test) {
    std::cout << "Test: " << test.name << "... ";
    
    ErrorReporter errorReporter;
    
    // Read input source
    std::string source = readFile(test.inputFile);
    if (source.empty()) {
        std::cout << "FAILED (cannot read input file: " << test.inputFile << ")" << std::endl;
        return false;
    }
    
    // Set source lines for error reporting
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
        std::cout << "FAILED (lexer errors)" << std::endl;
        errorReporter.printErrors();
        return false;
    }
    
    // Parsing
    Parser parser(tokens, errorReporter);
    auto program = parser.parse();
    
    if (errorReporter.hasErrors()) {
        std::cout << "FAILED (parser errors)" << std::endl;
        errorReporter.printErrors();
        return false;
    }
    
    if (!program) {
        std::cout << "FAILED (null program)" << std::endl;
        return false;
    }
    
    // Generate AST output
    std::stringstream astOutput;
    ASTPrettyPrinter printer(astOutput);
    program->accept(printer);
    std::string actual = astOutput.str();
    
    // Read expected output
    std::string expected = readFile(test.expectedFile);
    if (expected.empty()) {
        std::cout << "FAILED (cannot read expected file: " << test.expectedFile << ")" << std::endl;
        return false;
    }
    
    // Compare
    if (actual == expected) {
        std::cout << "PASSED" << std::endl;
        return true;
    } else {
        std::cout << "FAILED (output mismatch)" << std::endl;
        
        // Write actual output to temp file for diff
        std::string tempFile = test.inputFile + ".actual";
        std::ofstream actualOut(tempFile);
        actualOut << actual;
        actualOut.close();
        
        std::cout << "  Expected: " << test.expectedFile << std::endl;
        std::cout << "  Actual:   " << tempFile << std::endl;
        std::cout << "  Run: diff -u " << test.expectedFile << " " << tempFile << std::endl;
        
        return false;
    }
}

std::vector<TestCase> discoverTests(const std::string& baseDir) {
    std::vector<TestCase> tests;
    
    std::cout << "Looking for tests in: " << baseDir << std::endl;
    
    // Check if directory exists
    if (!fs::exists(baseDir)) {
        std::cout << "Directory does not exist: " << baseDir << std::endl;
        return tests;
    }
    
    // Expressions tests
    std::vector<std::pair<std::string, std::string>> exprTests = {
        // expressions
        {"Arithmetic expressions", "expressions/arithmetic"},
        {"Comparison expressions", "expressions/comparison"},
        {"Logical expressions", "expressions/logical"},
        {"Increment addition", "expressions/increment_addition"},  // NEW!
    };
    
    for (const auto& [name, path] : exprTests) {
        std::string inputFile = baseDir + "/valid/expressions/" + path + ".src";
        std::string expectedFile = baseDir + "/valid/expressions/" + path + ".expected";
        
        if (fs::exists(inputFile)) {
            if (fs::exists(expectedFile)) {
                tests.push_back({name, inputFile, expectedFile});
                std::cout << "Found test: " << name << std::endl;
            } else {
                std::cout << "Warning: Input file exists but expected file missing: " << expectedFile << std::endl;
            }
        }
    }
    
    // Statements tests
    std::vector<std::pair<std::string, std::string>> stmtTests = {
        {"If statement", "if"},
        {"While statement", "while"},
        {"For statement", "for"},
        {"Return statement", "return"},
        {"Variable declaration", "var_decl"},
        {"Block statement", "block"}
    };
    
    for (const auto& [name, path] : stmtTests) {
        std::string inputFile = baseDir + "/valid/statements/" + path + ".src";
        std::string expectedFile = baseDir + "/valid/statements/" + path + ".expected";
        
        if (fs::exists(inputFile)) {
            if (fs::exists(expectedFile)) {
                tests.push_back({name, inputFile, expectedFile});
                std::cout << "Found test: " << name << std::endl;
            }
        }
    }
    
    // Declarations tests
    std::vector<std::pair<std::string, std::string>> declTests = {
        {"Function declaration", "functions"},
        {"Struct declaration", "structs"},
        {"Multiple functions", "multiple_functions"}
    };
    
    for (const auto& [name, path] : declTests) {
        std::string inputFile = baseDir + "/valid/declarations/" + path + ".src";
        std::string expectedFile = baseDir + "/valid/declarations/" + path + ".expected";
        
        if (fs::exists(inputFile)) {
            if (fs::exists(expectedFile)) {
                tests.push_back({name, inputFile, expectedFile});
                std::cout << "Found test: " << name << std::endl;
            }
        }
    }
    
    // Full programs tests
    std::vector<std::pair<std::string, std::string>> progTests = {
        {"Factorial program", "factorial"},
        {"Fibonacci program", "fibonacci"},
        {"Struct program", "structs"},
        {"Nested loops", "nested_loops"}
    };
    
    for (const auto& [name, path] : progTests) {
        std::string inputFile = baseDir + "/valid/full_programs/" + path + ".src";
        std::string expectedFile = baseDir + "/valid/full_programs/" + path + ".expected";
        
        if (fs::exists(inputFile)) {
            if (fs::exists(expectedFile)) {
                tests.push_back({name, inputFile, expectedFile});
                std::cout << "Found test: " << name << std::endl;
            }
        }
    }
    
    return tests;
}

void runSyntaxErrorTests(const std::string& baseDir) {
    std::cout << "\n=== SYNTAX ERROR TESTS ===\n" << std::endl;
    
    std::vector<std::pair<std::string, std::string>> errorTests = {
        {"Missing semicolon", "missing_semicolon"},
        {"Missing parenthesis", "missing_paren"},
        {"Unclosed brace", "unclosed_brace"},
        {"Unexpected token", "unexpected_token"},
        {"Invalid expression", "invalid_expression"}
    };
    
    int passed = 0;
    int total = 0;
    
    for (const auto& [name, path] : errorTests) {
        total++;
        std::cout << "Error Test: " << name << "... ";
        
        std::string inputFile = baseDir + "/invalid/syntax_errors/" + path + ".src";
        
        if (!fs::exists(inputFile)) {
            std::cout << "SKIPPED (file not found: " << inputFile << ")" << std::endl;
            continue;
        }
        
        std::string source = readFile(inputFile);
        
        ErrorReporter errorReporter;
        
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
        
        if (errorReporter.hasErrors()) {
            std::cout << "PASSED (got errors as expected)" << std::endl;
            passed++;
        } else {
            std::cout << "FAILED (expected syntax errors but got none)" << std::endl;
        }
    }
    
    std::cout << "\nSyntax Error Tests: " << passed << "/" << total << " passed" << std::endl;
}

int main() {
    std::string testDir = "tests/parser";
    
    std::cout << "=== MiniCompiler Parser Test Runner ===\n" << std::endl;
    
    // Check if test directory exists
    if (!fs::exists(testDir)) {
        std::cerr << "Error: Test directory '" << testDir << "' not found" << std::endl;
        return 1;
    }
    
    // Discover and run valid tests
    auto tests = discoverTests(testDir);
    
    if (tests.empty()) {
        std::cout << "\nNo valid tests found in " << testDir << "/valid/" << std::endl;
    } else {
        std::cout << "\nFound " << tests.size() << " valid test(s)\n" << std::endl;
    }
    
    int passed = 0;
    int total = tests.size();
    
    for (const auto& test : tests) {
        if (runParserTest(test)) {
            passed++;
        }
        std::cout << std::endl;
    }
    
    std::cout << "========================================" << std::endl;
    std::cout << "Valid Tests: " << passed << "/" << total << " passed" << std::endl;
    
    // Run syntax error tests
    runSyntaxErrorTests(testDir);
    
    std::cout << "\n========================================" << std::endl;
    
    return 0;
}