# ============================================
# MiniCompiler Makefile
# ============================================

# Компилятор и флаги
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -Iinclude
LDFLAGS = 

# Исходные файлы
LEXER_SRCS = src/lexer/Scanner.cpp src/lexer/Token.cpp
UTILS_SRCS = src/utils/ErrorReporter.cpp
PREPROC_SRCS = src/preprocessor/Preprocessor.cpp src/preprocessor/PreprocessorFrontend.cpp
MAIN_SRC = src/main.cpp
TEST_FINAL_SRC = tests/test_runner_final.cpp
ERROR_TEST_SRC = tests/lexer/invalid/test_errors.cpp

# Объектные файлы
LEXER_OBJS = $(LEXER_SRCS:.cpp=.o)
UTILS_OBJS = $(UTILS_SRCS:.cpp=.o)
PREPROC_OBJS = $(PREPROC_SRCS:.cpp=.o)
MAIN_OBJ = $(MAIN_SRC:.cpp=.o)
TEST_FINAL_OBJ = $(TEST_FINAL_SRC:.cpp=.o)
ERROR_TEST_OBJ = $(ERROR_TEST_SRC:.cpp=.o)

# Исполняемые файлы
TARGET = minicompiler
TEST_FINAL_TARGET = test_runner_final
ERROR_TEST_TARGET = test_errors

# Цели по умолчанию
.DEFAULT_GOAL := all

all: $(TARGET) $(TEST_FINAL_TARGET) $(ERROR_TEST_TARGET)

# Основная программа
$(TARGET): $(MAIN_OBJ) $(LEXER_OBJS) $(UTILS_OBJS) $(PREPROC_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Тестовый раннер (финальный)
$(TEST_FINAL_TARGET): $(TEST_FINAL_OBJ) $(LEXER_OBJS) $(UTILS_OBJS) $(PREPROC_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Тесты с ошибками
$(ERROR_TEST_TARGET): $(ERROR_TEST_OBJ) $(LEXER_OBJS) $(UTILS_OBJS) $(PREPROC_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Компиляция .cpp в .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Очистка
clean:
	rm -f $(TARGET) $(TEST_FINAL_TARGET) $(ERROR_TEST_TARGET) \
	      $(MAIN_OBJ) $(LEXER_OBJS) $(UTILS_OBJS) $(PREPROC_OBJS) \
	      $(TEST_FINAL_OBJ) $(ERROR_TEST_OBJ) tests/lexer/invalid/*.o
	rm -f *.o *~ core
	@echo "Очистка завершена"

# Запуск всех тестов
test-all: $(TEST_FINAL_TARGET) $(ERROR_TEST_TARGET)
	@echo "=== ВАЛИДНЫЕ ТЕСТЫ ==="
	./$(TEST_FINAL_TARGET)
	@echo ""
	@echo "=== ТЕСТЫ С ОШИБКАМИ ==="
	./$(ERROR_TEST_TARGET)

# Запуск валидных тестов
test-valid: $(TEST_FINAL_TARGET)
	@echo "=== ВАЛИДНЫЕ ТЕСТЫ ==="
	./$(TEST_FINAL_TARGET)

# Запуск тестов с ошибками
test-errors: $(ERROR_TEST_TARGET)
	@echo "=== ТЕСТЫ С ОШИБКАМИ ==="
	./$(ERROR_TEST_TARGET)

# Проверка препроцессора
test-preprocess: $(TARGET)
	@echo "=== ТЕСТ ПРЕПРОЦЕССОРА ==="
	./$(TARGET) preprocess examples/preprocess_test.src

# Проверка компиляции
test-compile: $(TARGET)
	@echo "=== ТЕСТ КОМПИЛЯЦИИ ==="
	./$(TARGET) compile examples/hello.src

# Полная проверка
check: test-all test-preprocess test-compile
	@echo ""
	@echo "=== ВСЕ ПРОВЕРКИ ПРОЙДЕНЫ! ==="

# Справка
help:
	@echo "Доступные команды:"
	@echo "  make all              - Собрать все"
	@echo "  make clean            - Очистить"
	@echo "  make test-valid       - Запустить валидные тесты"
	@echo "  make test-errors      - Запустить тесты с ошибками"
	@echo "  make test-all         - Запустить все тесты"
	@echo "  make test-preprocess  - Тест препроцессора"
	@echo "  make test-compile     - Тест компиляции"
	@echo "  make check            - Выполнить все проверки"
	@echo ""
	@echo "Программа:"
	@echo "  ./minicompiler lex <file>        - Только лексер"
	@echo "  ./minicompiler preprocess <file> - Только препроцессор"
	@echo "  ./minicompiler compile <file>    - Препроцессор + лексер"

.PHONY: all clean test-valid test-errors test-all test-preprocess test-compile check help