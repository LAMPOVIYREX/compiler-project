#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include "lexer/Scanner.hpp"
#include "utils/ErrorReporter.hpp"

using namespace minicompiler;

struct TestCase {
    std::string name;
    std::string input;
    std::vector<std::string> expected;
};

bool runTest(const TestCase& test) {
    std::cout << "Test: " << test.name << "... ";
    
    ErrorReporter errorReporter;
    Scanner scanner(test.input, errorReporter);
    std::vector<Token> tokens = scanner.scanTokens();
    
    if (tokens.size() != test.expected.size()) {
        std::cout << "FAILED (token count mismatch)" << std::endl;
        std::cout << "  Expected: " << test.expected.size() 
                  << ", Got: " << tokens.size() << std::endl;
        
        std::cout << "  Actual tokens:" << std::endl;
        for (const auto& token : tokens) {
            std::cout << "    " << token.toString() << std::endl;
        }
        return false;
    }
    
    bool allMatch = true;
    for (size_t i = 0; i < tokens.size(); i++) {
        std::string tokenStr = tokens[i].toString();
        if (tokenStr != test.expected[i]) {
            if (allMatch) {
                std::cout << "FAILED" << std::endl;
                allMatch = false;
            }
            std::cout << "  Token mismatch at index " << i << ":" << std::endl;
            std::cout << "    Expected: " << test.expected[i] << std::endl;
            std::cout << "    Got:      " << tokenStr << std::endl;
        }
    }
    
    if (!allMatch) {
        return false;
    }
    
    if (errorReporter.hasErrors()) {
        if (test.name == "Integer literals - boundaries") {
            const auto& errors = errorReporter.getErrors();
            if (errors.size() == 1 && 
                errors[0].message.find("Invalid integer literal") != std::string::npos) {
                // Ожидаемая ошибка - пропускаем
            } else {
                std::cout << "FAILED (unexpected errors)" << std::endl;
                errorReporter.printErrors();
                return false;
            }
        } else {
            std::cout << "FAILED (unexpected errors)" << std::endl;
            errorReporter.printErrors();
            return false;
        }
    }
    
    std::cout << "PASSED" << std::endl;
    return true;
}

int main() {
    std::vector<TestCase> tests = {
        // Тест 1: Базовые идентификаторы
        {
            "Basic identifiers",
            "x y z _tmp var123",
            {
                "1:1 IDENTIFIER \"x\"",
                "1:3 IDENTIFIER \"y\"",
                "1:5 IDENTIFIER \"z\"",
                "1:7 IDENTIFIER \"_tmp\"",
                "1:12 IDENTIFIER \"var123\"",
                "1:18 END_OF_FILE \"\""
            }
        },
        
        // Тест 2: Длинные идентификаторы
        {
            "Long identifiers",
            "very_long_identifier_name_123 test",
            {
                "1:1 IDENTIFIER \"very_long_identifier_name_123\"",
                "1:31 IDENTIFIER \"test\"",
                "1:35 END_OF_FILE \"\""
            }
        },
        
        // Тест 3: Все ключевые слова (БЕЗ VOID)
        {
            "All keywords",
            "fn if else while for return int float bool true false struct const",
            {
                "1:1 KW_FN \"fn\"",
                "1:4 KW_IF \"if\"",
                "1:7 KW_ELSE \"else\"",
                "1:12 KW_WHILE \"while\"",
                "1:18 KW_FOR \"for\"",
                "1:22 KW_RETURN \"return\"",
                "1:29 KW_INT \"int\"",
                "1:33 KW_FLOAT \"float\"",
                "1:39 KW_BOOL \"bool\"",
                "1:44 BOOL_LITERAL \"true\" [true]",
                "1:49 BOOL_LITERAL \"false\" [false]",
                "1:55 KW_STRUCT \"struct\"",
                "1:62 KW_CONST \"const\"",
                "1:67 END_OF_FILE \"\""
            }
        },
        
        // Тест 4: Целые числа (граничные значения) - С ОЖИДАЕМОЙ ОШИБКОЙ
        {
            "Integer literals - boundaries",
            "0 1 42 2147483647 -2147483648",
            {
                "1:1 INT_LITERAL \"0\" [0]",
                "1:3 INT_LITERAL \"1\" [1]",
                "1:5 INT_LITERAL \"42\" [42]",
                "1:8 INT_LITERAL \"2147483647\" [2147483647]",
                "1:19 MINUS \"-\"",
                "1:20 ERROR \"2147483648\"",
                "1:30 END_OF_FILE \"\""
            }
        },
        
        // Тест 5: Числа с плавающей точкой
        {
            "Float literals",
            "0.0 3.14 0.5 123.456 1.0",
            {
                "1:1 FLOAT_LITERAL \"0.0\" [0.000000]",
                "1:5 FLOAT_LITERAL \"3.14\" [3.140000]",
                "1:10 FLOAT_LITERAL \"0.5\" [0.500000]",
                "1:14 FLOAT_LITERAL \"123.456\" [123.456000]",
                "1:22 FLOAT_LITERAL \"1.0\" [1.000000]",
                "1:25 END_OF_FILE \"\""
            }
        },
        
        // Тест 6: Научная нотация
        {
            "Scientific notation",
            "1e10 2.5e-3 1E+6",
            {
                "1:1 FLOAT_LITERAL \"1e10\" [10000000000.000000]",
                "1:6 FLOAT_LITERAL \"2.5e-3\" [0.002500]",
                "1:13 FLOAT_LITERAL \"1E+6\" [1000000.000000]",
                "1:17 END_OF_FILE \"\""
            }
        },
        
        // Тест 7: Арифметические операторы
        {
            "Arithmetic operators",
            "+ - * / %",
            {
                "1:1 PLUS \"+\"",
                "1:3 MINUS \"-\"",
                "1:5 STAR \"*\"",
                "1:7 SLASH \"/\"",
                "1:9 PERCENT \"%\"",
                "1:10 END_OF_FILE \"\""
            }
        },
        
        // Тест 8: Операторы сравнения
        {
            "Comparison operators",
            "== != < <= > >=",
            {
                "1:1 EQUAL_EQUAL \"==\"",
                "1:4 BANG_EQUAL \"!=\"",
                "1:7 LESS \"<\"",
                "1:9 LESS_EQUAL \"<=\"",
                "1:12 GREATER \">\"",
                "1:14 GREATER_EQUAL \">=\"",
                "1:16 END_OF_FILE \"\""
            }
        },
        
        // Тест 9: Логические операторы
        {
            "Logical operators",
            "&& || !",
            {
                "1:1 AMP_AMP \"&&\"",
                "1:4 PIPE_PIPE \"||\"",
                "1:7 BANG \"!\"",
                "1:8 END_OF_FILE \"\""
            }
        },
        
        // Тест 10: Операторы присваивания
        {
            "Assignment operators",
            "= += -= *= /= %=",
            {
                "1:1 EQUAL \"=\"",
                "1:3 PLUS_EQUAL \"+=\"",
                "1:6 MINUS_EQUAL \"-=\"",
                "1:9 STAR_EQUAL \"*=\"",
                "1:12 SLASH_EQUAL \"/=\"",
                "1:15 PERCENT_EQUAL \"%=\"",
                "1:17 END_OF_FILE \"\""
            }
        },
        
        // Тест 11: Инкремент/декремент
        {
            "Increment/Decrement",
            "++ --",
            {
                "1:1 PLUS_PLUS \"++\"",
                "1:4 MINUS_MINUS \"--\"",
                "1:6 END_OF_FILE \"\""
            }
        },
        
        // Тест 12: Разделители
        {
            "Delimiters",
            "( ) { } [ ] ; , . :",
            {
                "1:1 LPAREN \"(\"",
                "1:3 RPAREN \")\"",
                "1:5 LBRACE \"{\"",
                "1:7 RBRACE \"}\"",
                "1:9 LBRACKET \"[\"",
                "1:11 RBRACKET \"]\"",
                "1:13 SEMICOLON \";\"",
                "1:15 COMMA \",\"",
                "1:17 DOT \".\"",
                "1:19 COLON \":\"",
                "1:20 END_OF_FILE \"\""
            }
        },
        
        // Тест 13: Пустые строки
        {
            "Empty strings",
            "\"\" \" \" \"\\\"\\\"\"",
            {
                "1:1 STRING_LITERAL \"\"\"\" [\"\"]",
                "1:4 STRING_LITERAL \"\" \"\" [\" \"]",
                "1:8 STRING_LITERAL \"\"\\\"\\\"\"\" [\"\\\"\\\"\"]",
                "1:14 END_OF_FILE \"\""
            }
        },
        
        // Тест 14: Строки с escape-последовательностями
        {
            "String escapes",
            "\"\\n\" \"\\t\" \"\\r\" \"\\\\\" \"\\\"\"",
            {
                "1:1 STRING_LITERAL \"\"\\n\"\" [\"\\n\"]",
                "1:6 STRING_LITERAL \"\"\\t\"\" [\"\\t\"]",
                "1:11 STRING_LITERAL \"\"\\r\"\" [\"\\r\"]",
                "1:16 STRING_LITERAL \"\"\\\\\"\" [\"\\\\\"]",
                "1:21 STRING_LITERAL \"\"\\\"\"\" [\"\\\"\"]",
                "1:25 END_OF_FILE \"\""
            }
        },
        
        // Тест 15: Текстовые строки
        {
            "Text strings",
            "\"Hello\" \"World\" \"Test 123\"",
            {
                "1:1 STRING_LITERAL \"\"Hello\"\" [\"Hello\"]",
                "1:9 STRING_LITERAL \"\"World\"\" [\"World\"]",
                "1:17 STRING_LITERAL \"\"Test 123\"\" [\"Test 123\"]",
                "1:27 END_OF_FILE \"\""
            }
        },
        
        // Тест 16: Логические литералы
        {
            "Boolean literals",
            "true false",
            {
                "1:1 BOOL_LITERAL \"true\" [true]",
                "1:6 BOOL_LITERAL \"false\" [false]",
                "1:11 END_OF_FILE \"\""
            }
        },
        
        // Тест 17: Однострочные комментарии
        {
            "Single-line comments",
            "x = 5 // comment\ny = 10\n// pure comment\nz = 15",
            {
                "1:1 IDENTIFIER \"x\"",
                "1:3 EQUAL \"=\"",
                "1:5 INT_LITERAL \"5\" [5]",
                "2:1 IDENTIFIER \"y\"",
                "2:3 EQUAL \"=\"",
                "2:5 INT_LITERAL \"10\" [10]",
                "4:1 IDENTIFIER \"z\"",
                "4:3 EQUAL \"=\"",
                "4:5 INT_LITERAL \"15\" [15]",
                "4:7 END_OF_FILE \"\""
            }
        },
        
        // Тест 18: Многострочные комментарии
        {
            "Multi-line comments",
            "x = /* multi\nline\ncomment */ 5",
            {
                "1:1 IDENTIFIER \"x\"",
                "1:3 EQUAL \"=\"",
                "3:12 INT_LITERAL \"5\" [5]",
                "3:13 END_OF_FILE \"\""
            }
        },
        
        // Тест 19: Вложенные комментарии
        {
            "Nested comments",
            "/* outer /* inner */ outer */ x = 5",
            {
                "1:31 IDENTIFIER \"x\"",
                "1:33 EQUAL \"=\"",
                "1:35 INT_LITERAL \"5\" [5]",
                "1:36 END_OF_FILE \"\""
            }
        },
        
        // Тест 20: Смешанные пробелы
        {
            "Mixed whitespace",
            "x\t=\n5\r\ny = 10",
            {
                "1:1 IDENTIFIER \"x\"",
                "1:3 EQUAL \"=\"",
                "2:1 INT_LITERAL \"5\" [5]",
                "3:1 IDENTIFIER \"y\"",
                "3:3 EQUAL \"=\"",
                "3:5 INT_LITERAL \"10\" [10]",
                "3:7 END_OF_FILE \"\""
            }
        },
        
        // Тест 21: Комплексное выражение
        {
            "Complex expression",
            "result = (x + y) * 2 / 3.5;",
            {
                "1:1 IDENTIFIER \"result\"",
                "1:8 EQUAL \"=\"",
                "1:10 LPAREN \"(\"",
                "1:11 IDENTIFIER \"x\"",
                "1:13 PLUS \"+\"",
                "1:15 IDENTIFIER \"y\"",
                "1:16 RPAREN \")\"",
                "1:18 STAR \"*\"",
                "1:20 INT_LITERAL \"2\" [2]",
                "1:22 SLASH \"/\"",
                "1:24 FLOAT_LITERAL \"3.5\" [3.500000]",
                "1:27 SEMICOLON \";\"",
                "1:28 END_OF_FILE \"\""
            }
        },
        
        // Тест 22: Функция с параметрами
        {
            "Function declaration",
            "fn add(int a, int b) { return a + b; }",
            {
                "1:1 KW_FN \"fn\"",
                "1:4 IDENTIFIER \"add\"",
                "1:7 LPAREN \"(\"",
                "1:8 KW_INT \"int\"",
                "1:12 IDENTIFIER \"a\"",
                "1:13 COMMA \",\"",
                "1:15 KW_INT \"int\"",
                "1:19 IDENTIFIER \"b\"",
                "1:20 RPAREN \")\"",
                "1:22 LBRACE \"{\"",
                "1:24 KW_RETURN \"return\"",
                "1:31 IDENTIFIER \"a\"",
                "1:33 PLUS \"+\"",
                "1:35 IDENTIFIER \"b\"",
                "1:36 SEMICOLON \";\"",
                "1:38 RBRACE \"}\"",
                "1:39 END_OF_FILE \"\""
            }
        },
        
        // Тест 23: Массив
        {
            "Array declaration",
            "int arr[10]; arr[0] = 42;",
            {
                "1:1 KW_INT \"int\"",
                "1:5 IDENTIFIER \"arr\"",
                "1:8 LBRACKET \"[\"",
                "1:9 INT_LITERAL \"10\" [10]",
                "1:11 RBRACKET \"]\"",
                "1:12 SEMICOLON \";\"",
                "1:14 IDENTIFIER \"arr\"",
                "1:17 LBRACKET \"[\"",
                "1:18 INT_LITERAL \"0\" [0]",
                "1:19 RBRACKET \"]\"",
                "1:21 EQUAL \"=\"",
                "1:23 INT_LITERAL \"42\" [42]",
                "1:25 SEMICOLON \";\"",
                "1:26 END_OF_FILE \"\""
            }
        },
        
        // Тест 24: If-else конструкция
        {
            "If-else statement",
            "if (x > 0) { y = 1; } else { y = 2; }",
            {
                "1:1 KW_IF \"if\"",
                "1:4 LPAREN \"(\"",
                "1:5 IDENTIFIER \"x\"",
                "1:7 GREATER \">\"",
                "1:9 INT_LITERAL \"0\" [0]",
                "1:10 RPAREN \")\"",
                "1:12 LBRACE \"{\"",
                "1:14 IDENTIFIER \"y\"",
                "1:16 EQUAL \"=\"",
                "1:18 INT_LITERAL \"1\" [1]",
                "1:19 SEMICOLON \";\"",
                "1:21 RBRACE \"}\"",
                "1:23 KW_ELSE \"else\"",
                "1:28 LBRACE \"{\"",
                "1:30 IDENTIFIER \"y\"",
                "1:32 EQUAL \"=\"",
                "1:34 INT_LITERAL \"2\" [2]",
                "1:35 SEMICOLON \";\"",
                "1:37 RBRACE \"}\"",
                "1:38 END_OF_FILE \"\""
            }
        },
        
        // Тест 25: While цикл
        {
            "While loop",
            "while (i < 10) { i++; }",
            {
                "1:1 KW_WHILE \"while\"",
                "1:7 LPAREN \"(\"",
                "1:8 IDENTIFIER \"i\"",
                "1:10 LESS \"<\"",
                "1:12 INT_LITERAL \"10\" [10]",
                "1:14 RPAREN \")\"",
                "1:16 LBRACE \"{\"",
                "1:18 IDENTIFIER \"i\"",
                "1:19 PLUS_PLUS \"++\"",
                "1:21 SEMICOLON \";\"",
                "1:23 RBRACE \"}\"",
                "1:24 END_OF_FILE \"\""
            }
        }
    };
    
    std::cout << "=== MiniCompiler Test Runner (25 VALID TESTS) ===" << std::endl;
    std::cout << "=================================================" << std::endl;
    
    int passed = 0;
    int total = tests.size();
    
    for (const auto& test : tests) {
        if (runTest(test)) {
            passed++;
        }
        std::cout << std::endl;
    }
    
    std::cout << "========================================" << std::endl;
    std::cout << "Total tests: " << total << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << (total - passed) << std::endl;
    std::cout << "========================================" << std::endl;
    
    if (passed == total) {
        std::cout << "✅ ALL 25 TESTS PASSED!" << std::endl;
    } else {
        std::cout << "❌ Some tests failed!" << std::endl;
    }
    
    return 0;
}