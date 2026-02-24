#include <iostream>
#include <vector>
#include <string>
#include "lexer/Scanner.hpp"
#include "utils/ErrorReporter.hpp"

using namespace minicompiler;

struct ErrorTestCase {
    std::string name;
    std::string input;
    std::vector<std::string> expectedErrors;
};

bool runErrorTest(const ErrorTestCase& test) {
    std::cout << "Error Test: " << test.name << "... ";
    
    ErrorReporter errorReporter;
    Scanner scanner(test.input, errorReporter);
    auto tokens = scanner.scanTokens();  // Игнорируем токены, важны ошибки
    
    const auto& errors = errorReporter.getErrors();
    
    if (errors.size() != test.expectedErrors.size()) {
        std::cout << "FAILED (error count mismatch)" << std::endl;
        std::cout << "  Expected: " << test.expectedErrors.size() 
                  << " errors, Got: " << errors.size() << std::endl;
        return false;
    }
    
    for (size_t i = 0; i < errors.size(); i++) {
        if (errors[i].message.find(test.expectedErrors[i]) == std::string::npos) {
            std::cout << "FAILED" << std::endl;
            std::cout << "  Expected error containing: " << test.expectedErrors[i] << std::endl;
            std::cout << "  Got: " << errors[i].message << std::endl;
            return false;
        }
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

int main() {
    std::vector<ErrorTestCase> tests = {
        {
            "Invalid character",
            "int x = @;",
            {"Unexpected character: '@'"}
        },
        {
            "Invalid number",
            "int x = 123abc;",
            {"Invalid number literal"}
        },
        {
            "Unterminated string",
            "string s = \"hello;",
            {"Unterminated string"}
        },
        {
            "Newline in string",
            "string s = \"hello\nworld\";",
            {"Newline character in string literal"}
        },
        {
            "Invalid escape sequence",
            "string s = \"invalid\\x\";",
            {"Invalid escape sequence"}
        },
        {
            "Unterminated comment",
            "int x = 5; /* comment",
            {"Unterminated comment"}
        },
        {
            "Identifier too long",
            std::string("int ") + std::string(300, 'a') + " = 5;",
            {"Identifier too long"}
        },
        {
            "Integer out of range",
            "int x = 2147483648;",
            {"Integer literal out of range"}
        },
        {
            "Multiple errors",
            "int @ = 123abc; string s = \"unclosed;",
            {
                "Unexpected character: '@'",
                "Invalid number literal",
                "Unterminated string"
            }
        },
        {
            "Invalid operator",
            "int x = 5 & 3;",
            {"Unexpected character '&'"}
        },
        {
            "Empty file",
            "",
            {}
        }
    };
    
    std::cout << "=== Error Tests (11 TESTS) ===" << std::endl;
    
    int passed = 0;
    for (const auto& test : tests) {
        if (runErrorTest(test)) {
            passed++;
        }
        std::cout << std::endl;
    }
    
    std::cout << "========================================" << std::endl;
    std::cout << "Total error tests: " << tests.size() << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << (tests.size() - passed) << std::endl;
    
    return 0;
}