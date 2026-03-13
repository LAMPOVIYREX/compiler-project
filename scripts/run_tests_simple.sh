#!/bin/bash
echo "=== МиниКомпилятор - Простая сборка ==="
echo

# Сборка проекта
echo "1. Сборка проекта..."
make clean
make

if [ $? -ne 0 ]; then
    echo "Ошибка сборки!"
    exit 1
fi

echo "✓ Сборка завершена"
echo

# Запуск тестов
echo "2. Запуск тестов..."
./test_runner

if [ $? -ne 0 ]; then
    echo "✗ Тесты провалились!"
    exit 1
fi

echo
echo "✓ Тесты пройдены"
echo

# Запуск примера
echo "3. Тестирование на примере..."
echo "=== Токены из examples/hello.src ==="
./minicompiler lex examples/hello.src

if [ $? -ne 0 ]; then
    echo "✗ Ошибка при обработке примера!"
    exit 1
fi

echo
echo "=== Все проверки пройдены! ==="
echo "Компилятор готов к работе."