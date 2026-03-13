# ============================================
# MiniCompiler Makefile
# ============================================

# Компилятор и флаги
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -Iinclude
LDFLAGS = 

# Исходные файлы (Спринт 1)
LEXER_SRCS = src/lexer/Scanner.cpp src/lexer/Token.cpp
UTILS_SRCS = src/utils/ErrorReporter.cpp
PREPROC_SRCS = src/preprocessor/Preprocessor.cpp src/preprocessor/PreprocessorFrontend.cpp
MAIN_SRC = src/main.cpp

# Исходные файлы парсера (Спринт 2)
PARSER_SRCS = src/parser/Parser.cpp \
              src/parser/AST.cpp \
              src/parser/ASTPrettyPrinter.cpp \
              src/parser/ASTDotGenerator.cpp

# Тесты
TEST_FINAL_SRC = tests/test_runner_final.cpp
ERROR_TEST_SRC = tests/lexer/invalid/test_errors.cpp
PARSER_TEST_SRC = tests/parser/parser_test_runner.cpp

# Объектные файлы
LEXER_OBJS = $(LEXER_SRCS:.cpp=.o)
UTILS_OBJS = $(UTILS_SRCS:.cpp=.o)
PREPROC_OBJS = $(PREPROC_SRCS:.cpp=.o)
PARSER_OBJS = $(PARSER_SRCS:.cpp=.o)
MAIN_OBJ = $(MAIN_SRC:.cpp=.o)
TEST_FINAL_OBJ = $(TEST_FINAL_SRC:.cpp=.o)
ERROR_TEST_OBJ = $(ERROR_TEST_SRC:.cpp=.o)
PARSER_TEST_OBJ = $(PARSER_TEST_SRC:.cpp=.o)

# Исполняемые файлы
TARGET = minicompiler
TEST_FINAL_TARGET = test_runner_final
ERROR_TEST_TARGET = test_errors
PARSER_TEST_TARGET = parser_test_runner

# ============================================
# Цели по умолчанию
# ============================================

.DEFAULT_GOAL := all

all: $(TARGET) $(TEST_FINAL_TARGET) $(ERROR_TEST_TARGET) $(PARSER_TEST_TARGET)

# ============================================
# Компиляция
# ============================================

# Основная программа
$(TARGET): $(MAIN_OBJ) $(LEXER_OBJS) $(UTILS_OBJS) $(PREPROC_OBJS) $(PARSER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Тесты лексера
$(TEST_FINAL_TARGET): $(TEST_FINAL_OBJ) $(LEXER_OBJS) $(UTILS_OBJS) $(PREPROC_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(ERROR_TEST_TARGET): $(ERROR_TEST_OBJ) $(LEXER_OBJS) $(UTILS_OBJS) $(PREPROC_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Тесты парсера
$(PARSER_TEST_TARGET): $(PARSER_TEST_OBJ) $(LEXER_OBJS) $(UTILS_OBJS) $(PARSER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Компиляция .cpp в .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ============================================
# Очистка
# ============================================

clean:
	rm -f $(TARGET) $(TEST_FINAL_TARGET) $(ERROR_TEST_TARGET) $(PARSER_TEST_TARGET) \
	      $(MAIN_OBJ) $(LEXER_OBJS) $(UTILS_OBJS) $(PREPROC_OBJS) $(PARSER_OBJS) \
	      $(TEST_FINAL_OBJ) $(ERROR_TEST_OBJ) $(PARSER_TEST_OBJ) tests/lexer/invalid/*.o
	rm -f *.o *~ core *.dot *.png *.svg *.pdf *.jpg
	@echo "✅ Очистка завершена"

# ============================================
# Тесты (Спринт 1)
# ============================================

test-lexer: $(TEST_FINAL_TARGET) $(ERROR_TEST_TARGET)
	@echo "=== ВАЛИДНЫЕ ТЕСТЫ ЛЕКСЕРА ==="
	./$(TEST_FINAL_TARGET)
	@echo ""
	@echo "=== ТЕСТЫ ЛЕКСЕРА С ОШИБКАМИ ==="
	./$(ERROR_TEST_TARGET)

# ============================================
# Тесты (Спринт 2)
# ============================================

test-parser: $(PARSER_TEST_TARGET)
	@echo "=== ТЕСТЫ ПАРСЕРА ==="
	./$(PARSER_TEST_TARGET)

test-integration: $(TARGET)
	@echo "=== ТЕСТЫ ИНТЕГРАЦИИ ==="
	@for file in examples/*.src; do \
		echo "Тестирование $$file:"; \
		./$(TARGET) parse "$$file" > /dev/null 2>&1 && echo "  ✅ OK" || echo "  ❌ FAILED"; \
	done

test-all: test-lexer test-parser test-integration
	@echo ""
	@echo "=== ВСЕ ТЕСТЫ ПРОЙДЕНЫ! ==="

# ============================================
# Препроцессор
# ============================================

test-preprocess: $(TARGET)
	@echo "=== ТЕСТ ПРЕПРОЦЕССОРА ==="
	./$(TARGET) preprocess examples/preprocess_test.src

# ============================================
# ВИЗУАЛИЗАЦИЯ AST (НОВЫЕ ФУНКЦИИ)
# ============================================

# Вспомогательная функция для проверки наличия Graphviz
check-graphviz:
	@command -v dot >/dev/null 2>&1 || { echo "❌ Graphviz не установлен. Установите: sudo apt install graphviz"; exit 1; }

# Генерация AST для примера factorial
ast-factorial: $(TARGET) check-graphviz
	@echo "=== Генерация AST для factorial ==="
	./$(TARGET) parse examples/factorial.src --format dot --output factorial.dot
	dot -Tpng factorial.dot -o factorial.png
	@echo "✅ Создан factorial.png"
	@echo "   Открыть: eog factorial.png"

# Генерация AST для примера hello
ast-hello: $(TARGET) check-graphviz
	@echo "=== Генерация AST для hello ==="
	./$(TARGET) parse examples/hello.src --format dot --output hello.dot
	dot -Tpng hello.dot -o hello.png
	@echo "✅ Создан hello.png"

# Генерация AST для теста функций
ast-functions: $(TARGET) check-graphviz
	@echo "=== Генерация AST для функций ==="
	./$(TARGET) parse test_functions.mini --format dot --output functions.dot
	dot -Tpng functions.dot -o functions.png
	@echo "✅ Создан functions.png"

# Генерация AST для теста структур
ast-structs: $(TARGET) check-graphviz
	@echo "=== Генерация AST для структур ==="
	./$(TARGET) parse test_structs.mini --format dot --output structs.dot
	dot -Tpng structs.dot -o structs.png
	@echo "✅ Создан structs.png"

# Генерация AST для пользовательского файла
ast-file: $(TARGET) check-graphviz
	@if [ -z "$(FILE)" ]; then \
		echo "❌ Укажите файл: make ast-file FILE=yourfile.mini"; \
		exit 1; \
	fi
	@echo "=== Генерация AST для $(FILE) ==="
	@BASENAME=$$(basename "$(FILE)" .mini); \
	./$(TARGET) parse "$(FILE)" --format dot --output $${BASENAME}.dot; \
	dot -Tpng $${BASENAME}.dot -o $${BASENAME}.png; \
	echo "✅ Создан $${BASENAME}.png"

# Генерация всех AST
ast-all: ast-factorial ast-hello ast-functions ast-structs
	@echo "=== Все AST визуализации созданы ==="

# ============================================
# РАЗЛИЧНЫЕ ФОРМАТЫ ВИЗУАЛИЗАЦИИ
# ============================================

# SVG формат (векторный)
ast-svg: $(TARGET) check-graphviz
	@if [ -z "$(FILE)" ]; then \
		echo "❌ Укажите файл: make ast-svg FILE=yourfile.mini"; \
		exit 1; \
	fi
	@echo "=== Генерация SVG для $(FILE) ==="
	@BASENAME=$$(basename "$(FILE)" .mini); \
	./$(TARGET) parse "$(FILE)" --format dot --output $${BASENAME}.dot; \
	dot -Tsvg $${BASENAME}.dot -o $${BASENAME}.svg; \
	echo "✅ Создан $${BASENAME}.svg"

# PDF формат
ast-pdf: $(TARGET) check-graphviz
	@if [ -z "$(FILE)" ]; then \
		echo "❌ Укажите файл: make ast-pdf FILE=yourfile.mini"; \
		exit 1; \
	fi
	@echo "=== Генерация PDF для $(FILE) ==="
	@BASENAME=$$(basename "$(FILE)" .mini); \
	./$(TARGET) parse "$(FILE)" --format dot --output $${BASENAME}.dot; \
	dot -Tpdf $${BASENAME}.dot -o $${BASENAME}.pdf; \
	echo "✅ Создан $${BASENAME}.pdf"

# JPG формат
ast-jpg: $(TARGET) check-graphviz
	@if [ -z "$(FILE)" ]; then \
		echo "❌ Укажите файл: make ast-jpg FILE=yourfile.mini"; \
		exit 1; \
	fi
	@echo "=== Генерация JPG для $(FILE) ==="
	@BASENAME=$$(basename "$(FILE)" .mini); \
	./$(TARGET) parse "$(FILE)" --format dot --output $${BASENAME}.dot; \
	dot -Tjpg $${BASENAME}.dot -o $${BASENAME}.jpg; \
	echo "✅ Создан $${BASENAME}.jpg"

# ============================================
# РАСШИРЕННЫЕ ОПЦИИ ВИЗУАЛИЗАЦИИ
# ============================================

# Горизонтальная ориентация (слева направо)
ast-horizontal: $(TARGET) check-graphviz
	@if [ -z "$(FILE)" ]; then \
		echo "❌ Укажите файл: make ast-horizontal FILE=yourfile.mini"; \
		exit 1; \
	fi
	@echo "=== Генерация горизонтального AST для $(FILE) ==="
	@BASENAME=$$(basename "$(FILE)" .mini); \
	./$(TARGET) parse "$(FILE)" --format dot --output $${BASENAME}.dot; \
	dot -Tpng -Grankdir=LR $${BASENAME}.dot -o $${BASENAME}_horizontal.png; \
	echo "✅ Создан $${BASENAME}_horizontal.png"

# Высокое разрешение
ast-highres: $(TARGET) check-graphviz
	@if [ -z "$(FILE)" ]; then \
		echo "❌ Укажите файл: make ast-highres FILE=yourfile.mini"; \
		exit 1; \
	fi
	@echo "=== Генерация AST с высоким разрешением для $(FILE) ==="
	@BASENAME=$$(basename "$(FILE)" .mini); \
	./$(TARGET) parse "$(FILE)" --format dot --output $${BASENAME}.dot; \
	dot -Tpng -Gdpi=300 $${BASENAME}.dot -o $${BASENAME}_highres.png; \
	echo "✅ Создан $${BASENAME}_highres.png"

# Пакетная обработка всех .mini файлов
ast-batch: $(TARGET) check-graphviz
	@echo "=== Пакетная визуализация всех .mini файлов ==="
	@for file in *.mini; do \
		if [ -f "$$file" ]; then \
			BASENAME=$$(basename "$$file" .mini); \
			echo "  Обработка $$file..."; \
			./$(TARGET) parse "$$file" --format dot --output $${BASENAME}.dot; \
			dot -Tpng $${BASENAME}.dot -o $${BASENAME}.png; \
		fi \
	done
	@echo "✅ Пакетная обработка завершена"

# ============================================
# ОТКРЫТИЕ ИЗОБРАЖЕНИЙ
# ============================================

# Открыть последнее сгенерированное изображение
open-last:
	@LATEST_PNG=$$(ls -t *.png 2>/dev/null | head -1); \
	if [ -n "$$LATEST_PNG" ]; then \
		echo "Открываю $$LATEST_PNG"; \
		eog "$$LATEST_PNG" 2>/dev/null || xdg-open "$$LATEST_PNG" 2>/dev/null || echo "Не удалось открыть изображение"; \
	else \
		echo "❌ Нет PNG файлов"; \
	fi

# Открыть конкретный файл
open:
	@if [ -z "$(FILE)" ]; then \
		echo "❌ Укажите файл: make open FILE=filename.png"; \
		exit 1; \
	fi
	@if [ -f "$(FILE)" ]; then \
		eog "$(FILE)" 2>/dev/null || xdg-open "$(FILE)" 2>/dev/null; \
	else \
		echo "❌ Файл $(FILE) не найден"; \
	fi

# ============================================
# ОЧИСТКА ФАЙЛОВ ВИЗУАЛИЗАЦИИ
# ============================================

clean-ast:
	rm -f *.dot *.png *.svg *.pdf *.jpg
	@echo "✅ Файлы визуализации удалены"

# ============================================
# Полная проверка
# ============================================

check: test-all test-preprocess ast-all
	@echo ""
	@echo "=== ВСЕ ПРОВЕРКИ ПРОЙДЕНЫ! ==="

# ============================================
# Справка
# ============================================

help:
	@echo "============================================"
	@echo "MiniCompiler - Доступные команды"
	@echo "============================================"
	@echo ""
	@echo "СБОРКА:"
	@echo "  make all              - Собрать все"
	@echo "  make clean            - Очистить"
	@echo ""
	@echo "ТЕСТЫ:"
	@echo "  make test-lexer       - Тесты лексера (Спринт 1)"
	@echo "  make test-parser      - Тесты парсера (Спринт 2)"
	@echo "  make test-integration - Тесты интеграции"
	@echo "  make test-all         - Все тесты"
	@echo "  make test-preprocess  - Тест препроцессора"
	@echo ""
	@echo "ВИЗУАЛИЗАЦИЯ AST (НОВОЕ):"
	@echo "  make ast-factorial    - Визуализация factorial.src"
	@echo "  make ast-hello        - Визуализация hello.src"
	@echo "  make ast-functions    - Визуализация test_functions.mini"
	@echo "  make ast-structs      - Визуализация test_structs.mini"
	@echo "  make ast-all          - Все визуализации"
	@echo "  make ast-file FILE=x  - Визуализация указанного файла"
	@echo ""
	@echo "ФОРМАТЫ ВИЗУАЛИЗАЦИИ:"
	@echo "  make ast-svg FILE=x   - SVG формат"
	@echo "  make ast-pdf FILE=x   - PDF формат"
	@echo "  make ast-jpg FILE=x   - JPG формат"
	@echo ""
	@echo "РАСШИРЕННЫЕ ОПЦИИ:"
	@echo "  make ast-horizontal FILE=x - Горизонтальная ориентация"
	@echo "  make ast-highres FILE=x    - Высокое разрешение"
	@echo "  make ast-batch              - Все .mini файлы"
	@echo ""
	@echo "ПРОСМОТР:"
	@echo "  make open-last        - Открыть последний PNG"
	@echo "  make open FILE=x.png  - Открыть указанный файл"
	@echo "  make clean-ast        - Удалить файлы визуализации"
	@echo ""
	@echo "ПРОГРАММА:"
	@echo "  ./minicompiler lex <file>        - Только лексер"
	@echo "  ./minicompiler preprocess <file> - Только препроцессор"
	@echo "  ./minicompiler parse <file>      - Парсер + AST"
	@echo "  ./minicompiler compile <file>    - Полная компиляция"
	@echo ""
	@echo "ПРИМЕРЫ:"
	@echo "  make ast-factorial"
	@echo "  eog factorial.png"
	@echo "============================================"

.PHONY: all clean test-lexer test-parser test-integration test-all \
        test-preprocess check-graphviz \
        ast-factorial ast-hello ast-functions ast-structs ast-all ast-file \
        ast-svg ast-pdf ast-jpg ast-horizontal ast-highres ast-batch \
        open-last open clean-ast check help