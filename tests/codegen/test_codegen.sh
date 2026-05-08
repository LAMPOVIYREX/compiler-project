#!/bin/bash
# ============================================
# Codegen Test Runner Script
# ============================================

RUNTIME="src/runtime/runtime.o"
COMPILER="./minicompiler"
PASSED=0
FAILED=0
TOTAL=0

# Цвета для вывода
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "=========================================="
echo "  MiniLang Codegen Integration Tests"
echo "=========================================="
echo ""

run_test() {
    local test_name="$1"
    local test_file="$2"
    local expected="$3"
    local description="$4"
    
    TOTAL=$((TOTAL + 1))
    echo -n "Test $TOTAL: $test_name... "
    
    # Компиляция
    $COMPILER codegen "$test_file" > /tmp/test.asm 2>/dev/null
    if [ $? -ne 0 ]; then
        echo -e "${RED}FAILED${NC} (compilation error)"
        FAILED=$((FAILED + 1))
        return 1
    fi
    
    # Ассемблирование
    nasm -f elf64 /tmp/test.asm -o /tmp/test.o 2>/dev/null
    if [ $? -ne 0 ]; then
        echo -e "${RED}FAILED${NC} (assembly error)"
        FAILED=$((FAILED + 1))
        return 1
    fi
    
    # Линковка
    ld -o /tmp/test_prog $RUNTIME /tmp/test.o 2>/dev/null
    if [ $? -ne 0 ]; then
        echo -e "${RED}FAILED${NC} (link error)"
        FAILED=$((FAILED + 1))
        return 1
    fi
    
    # Выполнение
    /tmp/test_prog > /tmp/test_out.txt 2>/dev/null
    local result=$?
    
    # Проверка результата
    if [ "$result" -eq "$expected" ]; then
        echo -e "${GREEN}PASSED${NC} (exit code: $result)"
        if [ -n "$description" ]; then
            echo "        $description"
        fi
        PASSED=$((PASSED + 1))
        return 0
    else
        echo -e "${RED}FAILED${NC} (expected: $expected, got: $result)"
        FAILED=$((FAILED + 1))
        return 1
    fi
    
    # Очистка
    rm -f /tmp/test.asm /tmp/test.o /tmp/test_prog /tmp/test_out.txt
}

# ============================================
# Запуск тестов
# ============================================

echo "--- Базовые тесты ---"
echo ""

run_test "Simple add" "tests/codegen/valid/simple_add.mini" 42 "return 42"
run_test "Arithmetic" "tests/codegen/valid/arithmetic.mini" 42 "10 + 32 = 42"
run_test "Conditional" "tests/codegen/valid/conditional.mini" 1 "5 > 3 returns 1"

echo ""
echo "--- Расширенные тесты ---"
echo ""

run_test "Arithmetic ops" "tests/codegen/valid/arithmetic_ops.mini" 54 "13+7+30+3+1 = 54"
run_test "Control flow" "tests/codegen/valid/control_flow.mini" 50 "x>10 && x<=20 = 50"
run_test "Recursive functions" "tests/codegen/valid/recursive_functions.mini" 126 "fact(3)=6, fact(5)=120, sum=126"
run_test "Logical ops" "tests/codegen/valid/logical_ops.mini" 1 "a>0 && b>0 = true"
run_test "Complex expressions" "tests/codegen/valid/complex_expressions.mini" 35 "(10+5)*3-10 = 35"

echo ""
echo "--- Интеграционный тест ---"
echo ""

echo ""
echo "--- SHOULD: ABI Compliance Tests ---"
echo ""


run_test "Callee-saved regs" "tests/codegen/valid/abi_callee_saved.mini" 42 "rbx, r12-r15 preserved"
run_test "Stack alignment" "tests/codegen/valid/abi_stack_align.mini" 3 "16-byte aligned calls"
run_test "6 register params" "tests/codegen/valid/abi_params.mini" 21 "rdi,rsi,rdx,rcx,r8,r9"
run_test "7+ stack params" "tests/codegen/valid/many_params.mini" 28 "stack params work"
run_test "Integration test" "tests/codegen/valid/integration_test.mini" 42 "Full language test"

echo ""
echo "=========================================="
echo "  Results: $PASSED/$TOTAL passed"
echo "=========================================="

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}✅ ALL TESTS PASSED!${NC}"
    exit 0
else
    echo -e "${RED}❌ $FAILED test(s) failed${NC}"
    exit 1
fi
