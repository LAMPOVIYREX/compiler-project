#!/bin/bash
# ============================================
# MiniLang Codegen Integration Tests
# Sprint 5 + Sprint 6 + Float + Arrays
# ============================================

RUNTIME="src/runtime/runtime.o"
COMPILER="./minicompiler"
PASSED=0
FAILED=0
TOTAL=0

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

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
    
    rm -f /tmp/test.asm /tmp/test.o /tmp/test_prog /tmp/test_out.txt
}

# ============================================
# Sprint 5: Базовые тесты
# ============================================
echo "--- Sprint 5: Basic Tests ---"
echo ""

run_test "Simple add" "tests/codegen/valid/simple_add.mini" 42 "return 42"
run_test "Arithmetic" "tests/codegen/valid/arithmetic.mini" 42 "10 + 32 = 42"
run_test "Conditional" "tests/codegen/valid/conditional.mini" 1 "5 > 3 returns 1"

echo ""
echo "--- Sprint 5: Extended Tests ---"
echo ""

run_test "Arithmetic ops" "tests/codegen/valid/arithmetic_ops.mini" 54 "13+7+30+3+1 = 54"
run_test "Control flow" "tests/codegen/valid/control_flow.mini" 50 "x>10 && x<=20 = 50"
run_test "Recursive functions" "tests/codegen/valid/recursive_functions.mini" 126 "fact(3)=6, fact(5)=120, sum=126"
run_test "Logical ops" "tests/codegen/valid/logical_ops.mini" 1 "a>0 && b>0 = true"
run_test "Complex expressions" "tests/codegen/valid/complex_expressions.mini" 35 "(10+5)*3-10 = 35"
run_test "Integration test" "tests/codegen/valid/integration_test.mini" 42 "Full language test"

echo ""
echo "--- Sprint 5: ABI Compliance Tests ---"
echo ""

run_test "Callee-saved regs" "tests/codegen/valid/abi_callee_saved.mini" 42 "rbx, r12-r15 preserved"
run_test "Stack alignment" "tests/codegen/valid/abi_stack_align.mini" 3 "16-byte aligned calls"
run_test "6 register params" "tests/codegen/valid/abi_params.mini" 21 "rdi,rsi,rdx,rcx,r8,r9"
run_test "7+ stack params" "tests/codegen/valid/many_params.mini" 28 "stack params work"

echo ""
echo "--- Sprint 6: Control Flow Tests ---"
echo ""

run_test "Short-circuit AND" "tests/control_flow/valid/short_circuit_and.mini" 42 "right side skipped"
run_test "Short-circuit OR" "tests/control_flow/valid/short_circuit_or.mini" 42 "right side skipped"
run_test "Nested if" "tests/control_flow/valid/nested_if.mini" 50 "nested conditions"
run_test "While loop" "tests/control_flow/valid/while_loop.mini" 10 "0+1+2+3+4=10"
run_test "For loop (while)" "tests/control_flow/valid/for_loop.mini" 45 "0+1+...+9=45"
run_test "NOT operator" "tests/control_flow/valid/not_operator.mini" 42 "!0 = true"
run_test "Complex control flow" "tests/control_flow/valid/complex_control_flow.mini" 15 "1+2+3+4+5=15"

echo ""
echo "--- Sprint 6: Float Tests ---"
echo ""

run_test "Float arithmetic" "tests/control_flow/valid/float_arithmetic.mini" 42 "3.5+2.0=5.5, etc."
run_test "Float comparisons" "tests/control_flow/valid/float_comparisons.mini" 42 "> >= < <= !="
run_test "Float-int mix" "tests/control_flow/valid/float_int_mix.mini" 42 "int+float=float"

echo ""
echo "--- Sprint 6: Array Tests ---"
echo ""

run_test "Array basic" "tests/control_flow/valid/array_basic.mini" 60 "10+20+30=60"
run_test "Array loop" "tests/control_flow/valid/array_loop.mini" 40 "0+40=40"
run_test "Array conditional" "tests/control_flow/valid/array_conditional.mini" 10 "15-5=10"

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