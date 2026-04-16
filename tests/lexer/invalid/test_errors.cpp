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
    int expectedCount;  // Добавляем явное ожидаемое количество ошибок
};

bool runErrorTest(const ErrorTestCase& test) {
    std::cout << "Error Test: " << test.name << "... ";
    
    ErrorReporter errorReporter;
    Scanner scanner(test.input, errorReporter);
    auto tokens = scanner.scanTokens();  // Остановится при первой ошибке
    
    const auto& errors = errorReporter.getErrors();
    
    if (errors.size() != test.expectedCount) {
        std::cout << "FAILED (error count mismatch)" << std::endl;
        std::cout << "  Expected: " << test.expectedCount 
                  << " errors, Got: " << errors.size() << std::endl;
        if (errors.empty()) {
            std::cout << "  No errors were reported" << std::endl;
        } else {
            std::cout << "  Got errors:" << std::endl;
            for (const auto& error : errors) {
                std::cout << "    " << error.message << std::endl;
            }
        }
        return false;
    }
    
    for (size_t i = 0; i < errors.size(); i++) {
        bool found = false;
        for (const auto& expected : test.expectedErrors) {
            if (errors[i].message.find(expected) != std::string::npos) {
                found = true;
                break;
            }
        }
        if (!found) {
            std::cout << "FAILED" << std::endl;
            std::cout << "  Expected error containing one of:" << std::endl;
            for (const auto& expected : test.expectedErrors) {
                std::cout << "    " << expected << std::endl;
            }
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
            {"Недопустимый символ: '@'"},
            1
        },
        {
            "Invalid number",
            "int x = 123abc;",
            {"Неверное целое число"},
            1
        },
        {
            "Unterminated string",
            "string s = \"hello;",
            {"Незакрытая строка"},
            1
        },
        {
            "Newline in string",
            "string s = \"hello\nworld\";",
            {"Символ новой строки внутри строки"},
            1
        },
        {
            "Invalid escape sequence",
            "string s = \"invalid\\x\";",
            {"Неверная escape-последовательность"},
            1
        },
        {
            "Unterminated comment",
            "int x = 5; /* comment",
            {"Незакрытый комментарий"},
            1
        },
        {
            "Identifier too long",
            std::string("int ") + std::string(300, 'a') + " = 5;",
            {"Слишком длинный идентификатор"},
            1
        },
        {
            "Integer out of range",
            "int x = 2147483648;",
            {"Неверное целое число"},
            1
        },
        {
            "Multiple errors",
            "int @ = 123abc; string s = \"unclosed;",
            {"Недопустимый символ: '@'"},
            1
        },
        {
            "Invalid operator",
            "int x = 5 & 3;",
            {"Недопустимый символ: '&'"},
            1
        },
        {
            "Empty file",
            "",
            {},
            0
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