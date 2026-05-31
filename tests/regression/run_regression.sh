#!/bin/bash
# Regression tests for previously fixed bugs

RUNTIME="src/runtime/runtime.o"
COMPILER="./minicompiler"
PASSED=0
FAILED=0
TOTAL=0

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

echo "=========================================="
echo "  MiniLang Regression Tests"
echo "=========================================="
echo ""

run_reg_test() {
    local name="$1"
    local file="$2"
    local expected="$3"
    local desc="$4"
    TOTAL=$((TOTAL + 1))
    echo -n "Test $TOTAL: $name... "
    $COMPILER codegen "$file" --optimize > /tmp/reg_test.asm 2>/dev/null
    if [ $? -ne 0 ]; then
        echo -e "${RED}FAILED${NC} (compilation error)"
        FAILED=$((FAILED + 1))
        return
    fi
    nasm -f elf64 /tmp/reg_test.asm -o /tmp/reg_test.o 2>/dev/null
    if [ $? -ne 0 ]; then
        echo -e "${RED}FAILED${NC} (assembly error)"
        FAILED=$((FAILED + 1))
        return
    fi
    # Link with runtime if not using libc
    if echo "$file" | grep -q "heap\|extern\|malloc\|printf\|puts"; then
        gcc -no-pie -o /tmp/reg_test_prog /tmp/reg_test.o -lm 2>/dev/null
    else
        ld -o /tmp/reg_test_prog $RUNTIME /tmp/reg_test.o 2>/dev/null
    fi
    if [ $? -ne 0 ]; then
        echo -e "${RED}FAILED${NC} (link error)"
        FAILED=$((FAILED + 1))
        return
    fi
    /tmp/reg_test_prog
    local result=$?
    if [ "$result" -eq "$expected" ]; then
        echo -e "${GREEN}PASSED${NC} (exit: $result) - $desc"
        PASSED=$((PASSED + 1))
    else
        echo -e "${RED}FAILED${NC} (expected $expected, got $result)"
        FAILED=$((FAILED + 1))
    fi
    rm -f /tmp/reg_test.asm /tmp/reg_test.o /tmp/reg_test_prog
}

# Test 1: Global array (segfault fix)
run_reg_test "Global array" "tests/control_flow/valid/global_array.mini" 60 "global array in .bss"

# Test 2: Heap array (malloc/free, fix r12 save/restore)
run_reg_test "Heap array" "tests/control_flow/valid/heap_array.mini" 42 "heap int arr[10]"

# Test 3: Variadic extern (printf multiple args)
run_reg_test "Variadic extern" "tests/optimization/valid/printf_test.mini" 42 "printf with format string"

# Test 4: Extern type check (semantic test, just parse+check should pass)
echo -n "Test $((TOTAL+1)): Extern type check... "
$COMPILER check tests/semantic/valid/functions/extern_basic.src > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo -e "${GREEN}PASSED${NC} (no errors)"
    PASSED=$((PASSED + 1))
else
    echo -e "${RED}FAILED${NC} (unexpected errors)"
    FAILED=$((FAILED + 1))
fi
TOTAL=$((TOTAL + 1))

# Test 5: Semantic error for wrong extern type
echo -n "Test $((TOTAL+1)): Wrong extern type... "
$COMPILER check tests/semantic/invalid/extern_errors/wrong_type.src > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo -e "${GREEN}PASSED${NC} (error detected)"
    PASSED=$((PASSED + 1))
else
    echo -e "${RED}FAILED${NC} (expected error)"
    FAILED=$((FAILED + 1))
fi
TOTAL=$((TOTAL + 1))

echo ""
echo "=========================================="
echo "  Results: $PASSED/$TOTAL passed"
echo "=========================================="

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}✅ All regression tests passed!${NC}"
    exit 0
else
    echo -e "${RED}❌ $FAILED regression test(s) failed${NC}"
    exit 1
fi
