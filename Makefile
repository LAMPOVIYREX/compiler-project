# ============================================
# MiniCompiler Makefile
# ============================================

# Компилятор и флаги
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -Iinclude
LDFLAGS = 

# Исходные файлы
LEXER_SRCS = src/lexer/Scanner.cpp src/lexer/Token.cpp
UTILS_SRCS = src/utils/ErrorReporter.cpp src/utils/ErrorCodes.cpp
PREPROC_SRCS = src/preprocessor/Preprocessor.cpp src/preprocessor/PreprocessorFrontend.cpp
MAIN_SRC = src/main.cpp

# Исходные файлы парсера
PARSER_SRCS = src/parser/Parser.cpp \
              src/parser/AST.cpp \
              src/parser/ASTPrettyPrinter.cpp \
              src/parser/ASTDotGenerator.cpp \
              src/parser/ASTJsonGenerator.cpp 

# Исходные файлы семантического анализа
SEMANTIC_SRCS = src/semantic/SymbolTable.cpp src/semantic/SemanticAnalyzer.cpp

# Исходные файлы IR
IR_SRCS = src/ir/IR.cpp src/ir/IRGenerator.cpp

# Исходные файлы SSA
SSA_SRCS = src/ir/SSA.cpp src/ir/SSABuilder.cpp

# Тесты
TEST_FINAL_SRC = tests/test_runner_final.cpp
ERROR_TEST_SRC = tests/lexer/invalid/test_errors.cpp
PARSER_TEST_SRC = tests/parser/parser_test_runner.cpp
PARSER_ERROR_TEST_SRC = tests/parser/invalid/parser_error_test_runner.cpp
SEMANTIC_TEST_SRC = tests/semantic/semantic_test_runner.cpp
IR_TEST_SRC = tests/ir/ir_test_runner.cpp

# Объектные файлы
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

# Исполняемые файлы
TARGET = minicompiler
TEST_FINAL_TARGET = test_runner_final
ERROR_TEST_TARGET = test_errors
PARSER_TEST_TARGET = parser_test_runner
PARSER_ERROR_TEST_TARGET = parser_error_test_runner
SEMANTIC_TEST_TARGET = semantic_test_runner
IR_TEST_TARGET = ir_test_runner

# ============================================
# Цели по умолчанию
# ============================================

.DEFAULT_GOAL := all

all: $(TARGET)

# ============================================
# Компиляция основной программы
# ============================================

$(TARGET): $(MAIN_OBJ) $(LEXER_OBJS) $(UTILS_OBJS) $(PREPROC_OBJS) $(PARSER_OBJS) $(SEMANTIC_OBJS) $(ALL_IR_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

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
	rm -f *.o *~ core *.dot *.png *.svg *.pdf *.jpg
	@echo "✅ Очистка завершена"

clean-actual:
	find tests -name "*.actual" -delete
	@echo "✅ Файлы .actual удалены"

clean-all: clean clean-actual
	@echo "✅ Полная очистка завершена"

# ============================================
# Тесты
# ============================================

test-lexer: $(TEST_FINAL_TARGET) $(ERROR_TEST_TARGET)
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
	@echo "=== ТЕСТЫ СЕМАНТИКИ ==="
	./$(SEMANTIC_TEST_TARGET)

test-ir: $(IR_TEST_TARGET)
	@echo "=== ТЕСТЫ IR ==="
	./$(IR_TEST_TARGET)

test-ssa: $(TARGET)
	@echo "=== ТЕСТЫ SSA ==="
	@if [ -d "tests/ir/ssa/valid" ]; then \
		for file in tests/ir/ssa/valid/*.mini; do \
			if [ -f "$$file" ]; then \
				echo "Testing $$file..."; \
				./$(TARGET) ssa "$$file"; \
				echo "---"; \
			fi; \
		done; \
	else \
		echo "No SSA tests found in tests/ir/ssa/valid/"; \
	fi
	@echo "=== SSA TESTS COMPLETE ==="

test-all: build-tests test-lexer test-parser-all test-semantic test-ir test-ssa
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

check-all: check-lexer check-parser check-semantic check-ir check-ssa
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

# ============================================
# Справка
# ============================================

help:
	@echo "============================================"
	@echo "MiniCompiler - Доступные команды"
	@echo "============================================"
	@echo ""
	@echo "СБОРКА:"
	@echo "  make all              - Собрать компилятор"
	@echo "  make build-tests      - Собрать все тестовые раннеры"
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
	@echo "  make test-all         - ВСЕ ТЕСТЫ"
	@echo ""
	@echo "БЫСТРЫЕ ПРОВЕРКИ:"
	@echo "  make check-lexer      - Проверить примеры лексером"
	@echo "  make check-parser     - Проверить примеры парсером"
	@echo "  make check-semantic   - Проверить примеры семантикой"
	@echo "  make check-ir         - Проверить примеры IR"
	@echo "  make check-ssa        - Проверить примеры SSA"
	@echo "  make check-all        - Все проверки"
	@echo ""
	@echo "ЗАПУСК:"
	@echo "  make run-lexer FILE=x.mini   - Запустить лексер"
	@echo "  make run-parser FILE=x.mini  - Запустить парсер"
	@echo "  make run-check FILE=x.mini   - Запустить семантику"
	@echo "  make run-ir FILE=x.mini      - Запустить IR"
	@echo "  make run-ssa FILE=x.mini     - Запустить SSA"
	@echo ""
	@echo "ВИЗУАЛИЗАЦИЯ AST:"
	@echo "  make ast-file FILE=x.mini    - Создать PNG с AST"
	@echo ""
	@echo "ПРИМЕРЫ:"
	@echo "  make test-all"
	@echo "  make run-check FILE=examples/factorial.src"
	@echo "  make run-ir FILE=tests/ir/valid/simple_arith.mini"
	@echo "  make run-ssa FILE=tests/ir/ssa/valid/simple_ssa.mini"
	@echo "============================================"

.PHONY: all build-tests clean clean-actual clean-all \
        test-lexer test-parser test-parser-errors test-parser-all \
        test-semantic test-ir test-ssa test-all \
        check-lexer check-parser check-semantic check-ir check-ssa check-all \
        check-graphviz ast-file \
        run-lexer run-parser run-check run-ir run-ssa \
        help