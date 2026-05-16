# ============================================
# MiniCompiler Makefile - All Sprints (1-5)
# ============================================

# Компилятор и флаги
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -Iinclude
LDFLAGS = 

# ============================================
# Исходные файлы (Спринт 1 - Лексер)
# ============================================
LEXER_SRCS = src/lexer/Scanner.cpp src/lexer/Token.cpp
UTILS_SRCS = src/utils/ErrorReporter.cpp src/utils/ErrorCodes.cpp
PREPROC_SRCS = src/preprocessor/Preprocessor.cpp src/preprocessor/PreprocessorFrontend.cpp
MAIN_SRC = src/main.cpp

# ============================================
# Исходные файлы (Спринт 2 - Парсер)
# ============================================
PARSER_SRCS = src/parser/Parser.cpp \
              src/parser/AST.cpp \
              src/parser/ASTPrettyPrinter.cpp \
              src/parser/ASTDotGenerator.cpp \
              src/parser/ASTJsonGenerator.cpp 

# ============================================
# Исходные файлы (Спринт 3 - Семантический анализ)
# ============================================
SEMANTIC_SRCS = src/semantic/SymbolTable.cpp src/semantic/SemanticAnalyzer.cpp

# ============================================
# Исходные файлы (Спринт 4 - IR и SSA)
# ============================================
# Добавить в IR_SRCS:
IR_SRCS = src/ir/IR.cpp src/ir/IRGenerator.cpp src/ir/LoopOptimizer.cpp
SSA_SRCS = src/ir/SSA.cpp src/ir/SSABuilder.cpp

# ============================================
# Исходные файлы (Спринт 5 - Кодогенерация x86-64)
# ============================================
# Исходные файлы кодогенерации (Sprint 5-6)
CODEGEN_SRCS = src/codegen/X86Generator.cpp \
               src/codegen/LabelManager.cpp \
               src/codegen/StackFrame.cpp \
               src/codegen/AssemblyEmitter.cpp
CODEGEN_OBJS = $(CODEGEN_SRCS:.cpp=.o)

# ============================================
# Тесты
# ============================================
TEST_FINAL_SRC = tests/test_runner_final.cpp
ERROR_TEST_SRC = tests/lexer/invalid/test_errors.cpp
PARSER_TEST_SRC = tests/parser/parser_test_runner.cpp
PARSER_ERROR_TEST_SRC = tests/parser/invalid/parser_error_test_runner.cpp
SEMANTIC_TEST_SRC = tests/semantic/semantic_test_runner.cpp
IR_TEST_SRC = tests/ir/ir_test_runner.cpp

# ============================================
# Объектные файлы
# ============================================
LEXER_OBJS = $(LEXER_SRCS:.cpp=.o)
UTILS_OBJS = $(UTILS_SRCS:.cpp=.o)
PREPROC_OBJS = $(PREPROC_SRCS:.cpp=.o)
PARSER_OBJS = $(PARSER_SRCS:.cpp=.o)
SEMANTIC_OBJS = $(SEMANTIC_SRCS:.cpp=.o)
IR_OBJS = $(IR_SRCS:.cpp=.o)
SSA_OBJS = $(SSA_SRCS:.cpp=.o)
MAIN_OBJ = $(MAIN_SRC:.cpp=.o)

# Все объектные файлы IR + SSA
ALL_IR_OBJS = $(IR_OBJS) $(SSA_OBJS)

# ============================================
# Исполняемые файлы
# ============================================
TARGET = minicompiler
TEST_FINAL_TARGET = test_runner_final
ERROR_TEST_TARGET = test_errors
PARSER_TEST_TARGET = parser_test_runner
PARSER_ERROR_TEST_TARGET = parser_error_test_runner
SEMANTIC_TEST_TARGET = semantic_test_runner
IR_TEST_TARGET = ir_test_runner
CODEGEN_TEST_TARGET = codegen_test_runner

# ============================================
# Цели по умолчанию
# ============================================

.DEFAULT_GOAL := all

all: $(TARGET) runtime

# ============================================
# Компиляция основной программы (все спринты)
# ============================================

$(TARGET): $(MAIN_OBJ) $(LEXER_OBJS) $(UTILS_OBJS) $(PREPROC_OBJS) $(PARSER_OBJS) $(SEMANTIC_OBJS) $(ALL_IR_OBJS) $(CODEGEN_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✅ Компилятор собран успешно"

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ============================================
# Runtime библиотека
# ============================================

RUNTIME_SRC = src/runtime/runtime.asm
RUNTIME_OBJ = src/runtime/runtime.o

runtime: $(RUNTIME_OBJ)
	@echo "✅ Runtime библиотека собрана"

$(RUNTIME_OBJ): $(RUNTIME_SRC)
	nasm -f elf64 -o $@ $<

# ============================================
# Компиляция тестовых раннеров
# ============================================

$(TEST_FINAL_TARGET): tests/test_runner_final.o $(LEXER_OBJS) $(UTILS_OBJS) $(PREPROC_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

tests/test_runner_final.o: $(TEST_FINAL_SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(ERROR_TEST_TARGET): tests/lexer/invalid/test_errors.o $(LEXER_OBJS) $(UTILS_OBJS) $(PREPROC_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

tests/lexer/invalid/test_errors.o: $(ERROR_TEST_SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(PARSER_TEST_TARGET): tests/parser/parser_test_runner.o $(LEXER_OBJS) $(UTILS_OBJS) $(PARSER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

tests/parser/parser_test_runner.o: $(PARSER_TEST_SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(PARSER_ERROR_TEST_TARGET): tests/parser/invalid/parser_error_test_runner.o $(LEXER_OBJS) $(UTILS_OBJS) $(PARSER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

tests/parser/invalid/parser_error_test_runner.o: $(PARSER_ERROR_TEST_SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(SEMANTIC_TEST_TARGET): tests/semantic/semantic_test_runner.o $(LEXER_OBJS) $(UTILS_OBJS) $(PARSER_OBJS) $(SEMANTIC_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

tests/semantic/semantic_test_runner.o: $(SEMANTIC_TEST_SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(IR_TEST_TARGET): tests/ir/ir_test_runner.o $(LEXER_OBJS) $(UTILS_OBJS) $(PARSER_OBJS) $(SEMANTIC_OBJS) $(ALL_IR_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

tests/ir/ir_test_runner.o: $(IR_TEST_SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ============================================
# Сборка всех тестов
# ============================================

build-tests: $(TEST_FINAL_TARGET) $(ERROR_TEST_TARGET) $(PARSER_TEST_TARGET) $(PARSER_ERROR_TEST_TARGET) $(SEMANTIC_TEST_TARGET) $(IR_TEST_TARGET)
	@echo "✅ Все тестовые раннеры собраны"

# ============================================
# Очистка
# ============================================

clean:
	rm -f $(TARGET) $(TEST_FINAL_TARGET) $(ERROR_TEST_TARGET) $(PARSER_TEST_TARGET) $(PARSER_ERROR_TEST_TARGET) $(SEMANTIC_TEST_TARGET) $(IR_TEST_TARGET)
	rm -f src/*.o src/*/*.o tests/*.o tests/*/*.o tests/*/*/*.o tests/*/*/*/*.o
	rm -f *.o *~ core *.dot *.png *.svg *.pdf *.jpg *.asm
	@echo "✅ Очистка завершена"

clean-actual:
	find tests -name "*.actual" -delete
	rm -f tests/codegen/*/*.asm tests/codegen/*/*.o tests/codegen/*/*.out
	@echo "✅ Файлы .actual удалены"

clean-all: clean clean-actual
	@echo "✅ Полная очистка завершена"

# ============================================
# Тесты (все спринты)
# ============================================

test-lexer: $(TEST_FINAL_TARGET) $(ERROR_TEST_TARGET)
	@echo "========================================"
	@echo "=== ТЕСТЫ ЛЕКСЕРА (Спринт 1) ==="
	@echo "========================================"
	@echo ""
	@echo "=== ВАЛИДНЫЕ ТЕСТЫ ЛЕКСЕРА ==="
	./$(TEST_FINAL_TARGET)
	@echo ""
	@echo "=== ТЕСТЫ ЛЕКСЕРА С ОШИБКАМИ ==="
	./$(ERROR_TEST_TARGET)

test-parser: $(PARSER_TEST_TARGET)
	@echo "=== ТЕСТЫ ПАРСЕРА (ВАЛИДНЫЕ) ==="
	./$(PARSER_TEST_TARGET)

test-parser-errors: $(PARSER_ERROR_TEST_TARGET)
	@echo "=== ТЕСТЫ ПАРСЕРА С ОШИБКАМИ ==="
	./$(PARSER_ERROR_TEST_TARGET)

test-parser-all: test-parser test-parser-errors
	@echo "=== ВСЕ ТЕСТЫ ПАРСЕРА ПРОЙДЕНЫ ==="

test-semantic: $(SEMANTIC_TEST_TARGET)
	@echo "=== ТЕСТЫ СЕМАНТИКИ (Спринт 3) ==="
	./$(SEMANTIC_TEST_TARGET)

test-ir: $(IR_TEST_TARGET)
	@echo "=== ТЕСТЫ IR (Спринт 4) ==="
	./$(IR_TEST_TARGET)

test-ssa: $(TARGET)
	@echo "=== ТЕСТЫ SSA ==="
	@if [ -d "tests/ir/ssa/valid" ]; then \
		for file in tests/ir/ssa/valid/*.mini; do \
			if [ -f "$$file" ]; then \
				echo "Testing $$file..."; \
				./$(TARGET) ssa "$$file" > /dev/null 2>&1 && echo "  ✅ PASSED" || echo "  ❌ FAILED"; \
			fi; \
		done; \
	fi
	@echo "=== SSA TESTS COMPLETE ==="

# ============================================
# Тесты кодогенерации (Спринт 5)
# ============================================

test-codegen: $(TARGET) $(RUNTIME_OBJ)
	@echo "========================================"
	@echo "=== ТЕСТЫ КОДОГЕНЕРАЦИИ (Спринт 5) ==="
	@echo "========================================"
	@echo ""
	@if [ -f "tests/codegen/test_codegen.sh" ]; then \
		chmod +x tests/codegen/test_codegen.sh; \
		./tests/codegen/test_codegen.sh; \
	else \
		echo "❌ test_codegen.sh not found"; \
	fi


test-all: build-tests test-lexer test-parser-all test-semantic test-ir test-ssa test-codegen
	@echo ""
	@echo "=========================================="
	@echo "=== ВСЕ ТЕСТЫ ПРОЙДЕНЫ! ==="
	@echo "=========================================="

# ============================================
# Быстрые проверки
# ============================================

check-lexer: $(TARGET)
	@echo "=== ПРОВЕРКА ЛЕКСЕРА ==="
	@for file in examples/*.src; do \
		echo "Проверка $$file..."; \
		./$(TARGET) lex "$$file" > /dev/null 2>&1 && echo "  ✅ OK" || echo "  ❌ FAILED"; \
	done

check-parser: $(TARGET)
	@echo "=== ПРОВЕРКА ПАРСЕРА ==="
	@for file in examples/*.src; do \
		echo "Проверка $$file..."; \
		./$(TARGET) parse "$$file" > /dev/null 2>&1 && echo "  ✅ OK" || echo "  ❌ FAILED"; \
	done

check-semantic: $(TARGET)
	@echo "=== ПРОВЕРКА СЕМАНТИКИ ==="
	@for file in examples/*.src; do \
		echo "Проверка $$file..."; \
		./$(TARGET) check "$$file" > /dev/null 2>&1 && echo "  ✅ OK" || echo "  ❌ FAILED"; \
	done

check-ir: $(TARGET)
	@echo "=== ПРОВЕРКА IR ==="
	@for file in examples/*.src; do \
		echo "Проверка $$file..."; \
		./$(TARGET) ir "$$file" > /dev/null 2>&1 && echo "  ✅ OK" || echo "  ❌ FAILED"; \
	done

check-ssa: $(TARGET)
	@echo "=== ПРОВЕРКА SSA ==="
	@for file in examples/*.src; do \
		echo "Проверка $$file..."; \
		./$(TARGET) ssa "$$file" > /dev/null 2>&1 && echo "  ✅ OK" || echo "  ❌ FAILED"; \
	done

check-codegen: $(TARGET) $(RUNTIME_OBJ)
	@echo "=== ПРОВЕРКА КОДОГЕНЕРАЦИИ ==="
	@for file in examples/*.src; do \
		name=$$(basename "$$file" .src); \
		echo "Проверка $$file..."; \
		./$(TARGET) codegen "$$file" --output "/tmp/$$name.asm" 2>/dev/null && \
		nasm -f elf64 "/tmp/$$name.asm" -o "/tmp/$$name.o" 2>/dev/null && \
		ld -o "/tmp/$$name.out" $(RUNTIME_OBJ) "/tmp/$$name.o" 2>/dev/null && \
		echo "  ✅ OK (собирается)" || echo "  ❌ FAILED"; \
		rm -f "/tmp/$$name.asm" "/tmp/$$name.o" "/tmp/$$name.out"; \
	done

check-all: check-lexer check-parser check-semantic check-ir check-ssa check-codegen
	@echo "=== ВСЕ ПРОВЕРКИ ПРОЙДЕНЫ ==="

# ============================================
# Визуализация AST
# ============================================

check-graphviz:
	@command -v dot >/dev/null 2>&1 || { echo "❌ Graphviz не установлен. Установите: sudo apt install graphviz"; exit 1; }

ast-file: $(TARGET) check-graphviz
	@if [ -z "$(FILE)" ]; then \
		echo "❌ Укажите файл: make ast-file FILE=yourfile.mini"; \
		exit 1; \
	fi
	@BASENAME=$$(basename "$(FILE)" .mini); \
	./$(TARGET) parse "$(FILE)" --format dot --output $${BASENAME}.dot; \
	dot -Tpng $${BASENAME}.dot -o $${BASENAME}.png; \
	echo "✅ Создан $${BASENAME}.png"

# ============================================
# Запуск программы
# ============================================

run-lexer: $(TARGET)
	@if [ -z "$(FILE)" ]; then \
		echo "❌ Укажите файл: make run-lexer FILE=yourfile.mini"; \
		exit 1; \
	fi
	./$(TARGET) lex "$(FILE)"

run-parser: $(TARGET)
	@if [ -z "$(FILE)" ]; then \
		echo "❌ Укажите файл: make run-parser FILE=yourfile.mini"; \
		exit 1; \
	fi
	./$(TARGET) parse "$(FILE)" --verbose

run-check: $(TARGET)
	@if [ -z "$(FILE)" ]; then \
		echo "❌ Укажите файл: make run-check FILE=yourfile.mini"; \
		exit 1; \
	fi
	./$(TARGET) check "$(FILE)" --verbose

run-ir: $(TARGET)
	@if [ -z "$(FILE)" ]; then \
		echo "❌ Укажите файл: make run-ir FILE=yourfile.mini"; \
		exit 1; \
	fi
	./$(TARGET) ir "$(FILE)" --verbose

run-ssa: $(TARGET)
	@if [ -z "$(FILE)" ]; then \
		echo "❌ Укажите файл: make run-ssa FILE=yourfile.mini"; \
		exit 1; \
	fi
	./$(TARGET) ssa "$(FILE)" --verbose

run-codegen: $(TARGET)
	@if [ -z "$(FILE)" ]; then \
		echo "❌ Укажите файл: make run-codegen FILE=yourfile.mini"; \
		exit 1; \
	fi
	./$(TARGET) codegen "$(FILE)" --verbose

build-and-run: $(TARGET) $(RUNTIME_OBJ)
	@if [ -z "$(FILE)" ]; then \
		echo "❌ Укажите файл: make build-and-run FILE=yourfile.mini"; \
		exit 1; \
	fi
	@name=$$(basename "$(FILE)" .mini); \
	echo "Компиляция $(FILE)..."; \
	./$(TARGET) codegen "$(FILE)" > "/tmp/$$name.asm" 2>&1; \
	if [ $$? -ne 0 ]; then \
		echo "❌ Compilation failed"; \
		cat "/tmp/$$name.asm"; \
		rm -f "/tmp/$$name.asm"; \
		exit 1; \
	fi; \
	echo "Ассемблирование..."; \
	nasm -f elf64 "/tmp/$$name.asm" -o "/tmp/$$name.o" 2>&1; \
	if [ $$? -ne 0 ]; then \
		echo "❌ Assembly failed"; \
		cat "/tmp/$$name.asm"; \
		rm -f "/tmp/$$name.asm" "/tmp/$$name.o"; \
		exit 1; \
	fi; \
	echo "Линковка..."; \
	ld -o "/tmp/$$name.out" $(RUNTIME_OBJ) "/tmp/$$name.o" 2>&1; \
	if [ $$? -ne 0 ]; then \
		echo "❌ Link failed"; \
		rm -f "/tmp/$$name.asm" "/tmp/$$name.o"; \
		exit 1; \
	fi; \
	echo "Запуск..."; \
	"/tmp/$$name.out"; \
	echo "Exit code: $$?"; \
	rm -f "/tmp/$$name.asm" "/tmp/$$name.o" "/tmp/$$name.out"	
# ============================================
# Справка
# ============================================

help:
	@echo "============================================"
	@echo "MiniCompiler - Доступные команды"
	@echo "============================================"
	@echo ""
	@echo "СБОРКА:"
	@echo "  make all              - Собрать компилятор + runtime"
	@echo "  make build-tests      - Собрать все тестовые раннеры"
	@echo "  make runtime          - Собрать runtime библиотеку"
	@echo "  make clean            - Очистить объектные файлы"
	@echo "  make clean-actual     - Удалить .actual файлы тестов"
	@echo "  make clean-all        - Полная очистка"
	@echo ""
	@echo "ТЕСТЫ:"
	@echo "  make test-lexer       - Тесты лексера (Спринт 1)"
	@echo "  make test-parser      - Валидные тесты парсера"
	@echo "  make test-parser-errors - Тесты парсера с ошибками"
	@echo "  make test-parser-all  - Все тесты парсера (Спринт 2)"
	@echo "  make test-semantic    - Тесты семантики (Спринт 3)"
	@echo "  make test-ir          - Тесты IR (Спринт 4)"
	@echo "  make test-ssa         - Тесты SSA формы"
	@echo "  make test-codegen     - Тесты кодогенерации (Спринт 5)"
	@echo "  make test-all         - ВСЕ ТЕСТЫ (Спринты 1-5)"
	@echo ""
	@echo "БЫСТРЫЕ ПРОВЕРКИ:"
	@echo "  make check-lexer      - Проверить примеры лексером"
	@echo "  make check-parser     - Проверить примеры парсером"
	@echo "  make check-semantic   - Проверить примеры семантикой"
	@echo "  make check-ir         - Проверить примеры IR"
	@echo "  make check-ssa        - Проверить примеры SSA"
	@echo "  make check-codegen    - Проверить примеры кодогенерацией"
	@echo "  make check-all        - Все проверки"
	@echo ""
	@echo "ЗАПУСК:"
	@echo "  make run-lexer FILE=x.mini   - Запустить лексер"
	@echo "  make run-parser FILE=x.mini  - Запустить парсер"
	@echo "  make run-check FILE=x.mini   - Запустить семантику"
	@echo "  make run-ir FILE=x.mini      - Запустить IR"
	@echo "  make run-ssa FILE=x.mini     - Запустить SSA"
	@echo "  make run-codegen FILE=x.mini - Запустить кодогенерацию"
	@echo "  make build-and-run FILE=x.mini - Скомпилировать и запустить"
	@echo ""
	@echo "ВИЗУАЛИЗАЦИЯ AST:"
	@echo "  make ast-file FILE=x.mini    - Создать PNG с AST"
	@echo ""
	@echo "ПРИМЕРЫ:"
	@echo "  make test-all"
	@echo "  make run-codegen FILE=tests/codegen/valid/simple_add.mini"
	@echo "  make build-and-run FILE=tests/codegen/valid/simple_add.mini"
	@echo "  make check-all"
	@echo "============================================"

.PHONY: all build-tests runtime clean clean-actual clean-all \
        test-lexer test-parser test-parser-errors test-parser-all \
        test-semantic test-ir test-ssa test-codegen test-all \
        check-lexer check-parser check-semantic check-ir check-ssa check-codegen check-all \
        check-graphviz ast-file \
        run-lexer run-parser run-check run-ir run-ssa run-codegen build-and-run \
        help