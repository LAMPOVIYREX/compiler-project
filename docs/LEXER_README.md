# **LEXER README - Документация лексического анализатора**

## **📖 О проекте**

Лексический анализатор (лексер) - первый компонент компилятора MiniLang, отвечающий за преобразование исходного кода в поток токенов. Это основа для всех последующих этапов компиляции.

---

## **🚀 Быстрый старт**

### **Сборка проекта**
```bash
# Очистка и сборка
make clean
make

# Проверка, что лексер работает
./minicompiler lex examples/hello.src
```

### **Базовое использование**
```bash
# Токенизация файла
./minicompiler lex <filename>

# Пример с выводом в файл
./minicompiler lex examples/hello.src > tokens.txt

# Просмотр первых 10 токенов
./minicompiler lex examples/hello.src | head -10
```

---

## **🔧 Команды программы**

| Команда | Описание | Пример |
|---------|----------|---------|
| `lex <file>` | Только лексер (без препроцессора) | `./minicompiler lex hello.src` |
| `preprocess <file>` | Только препроцессор | `./minicompiler preprocess test.src` |
| `compile <file>` | Препроцессор + лексер | `./minicompiler compile program.src` |
| `help` | Показать справку | `./minicompiler help` |

---

## **📋 Формат вывода**

### **Формат токена**
```
СТРОКА:КОЛОНКА ТИП_ТОКЕНА "ЛЕКСЕМА" [ЗНАЧЕНИЕ]
```

### **Пример вывода**
```
1:1 KW_FN "fn"
1:4 IDENTIFIER "main"
1:8 LPAREN "("
1:9 RPAREN ")"
1:10 LBRACE "{"
2:5 KW_INT "int"
2:9 IDENTIFIER "x"
2:11 EQUAL "="
2:13 INT_LITERAL "42" [42]
2:15 SEMICOLON ";"
3:1 RBRACE "}"
4:1 END_OF_FILE ""
```

---

## **🎯 Типы токенов**

### **Ключевые слова**
| Токен | Лексемы |
|-------|---------|
| `KW_FN` | `fn` |
| `KW_IF` | `if` |
| `KW_ELSE` | `else` |
| `KW_WHILE` | `while` |
| `KW_FOR` | `for` |
| `KW_RETURN` | `return` |
| `KW_INT` | `int` |
| `KW_FLOAT` | `float` |
| `KW_BOOL` | `bool` |
| `KW_VOID` | `void` |
| `KW_STRUCT` | `struct` |
| `KW_CONST` | `const` |

### **Литералы**
| Токен | Описание | Пример | Значение |
|-------|----------|---------|----------|
| `INT_LITERAL` | Целое число | `42` | `[42]` |
| `FLOAT_LITERAL` | Число с плавающей точкой | `3.14` | `[3.140000]` |
| `STRING_LITERAL` | Строка | `"hello"` | `["hello"]` |
| `BOOL_LITERAL` | Логическое значение | `true` | `[true]` |

### **Идентификаторы**
| Токен | Описание | Пример |
|-------|----------|---------|
| `IDENTIFIER` | Имя переменной/функции | `counter`, `_temp`, `var123` |

### **Арифметические операторы**
| Токен | Лексемы |
|-------|---------|
| `PLUS` | `+` |
| `MINUS` | `-` |
| `STAR` | `*` |
| `SLASH` | `/` |
| `PERCENT` | `%` |
| `PLUS_PLUS` | `++` |
| `MINUS_MINUS` | `--` |

### **Операторы сравнения**
| Токен | Лексемы |
|-------|---------|
| `EQUAL_EQUAL` | `==` |
| `BANG_EQUAL` | `!=` |
| `LESS` | `<` |
| `LESS_EQUAL` | `<=` |
| `GREATER` | `>` |
| `GREATER_EQUAL` | `>=` |

### **Логические операторы**
| Токен | Лексемы |
|-------|---------|
| `AMP_AMP` | `&&` |
| `PIPE_PIPE` | `\|\|` |
| `BANG` | `!` |

### **Операторы присваивания**
| Токен | Лексемы |
|-------|---------|
| `EQUAL` | `=` |
| `PLUS_EQUAL` | `+=` |
| `MINUS_EQUAL` | `-=` |
| `STAR_EQUAL` | `*=` |
| `SLASH_EQUAL` | `/=` |
| `PERCENT_EQUAL` | `%=` |

### **Разделители**
| Токен | Лексемы |
|-------|---------|
| `LPAREN` | `(` |
| `RPAREN` | `)` |
| `LBRACE` | `{` |
| `RBRACE` | `}` |
| `LBRACKET` | `[` |
| `RBRACKET` | `]` |
| `SEMICOLON` | `;` |
| `COMMA` | `,` |
| `DOT` | `.` |
| `COLON` | `:` |

### **Специальные токены**
| Токен | Описание |
|-------|----------|
| `END_OF_FILE` | Конец файла |
| `ERROR` | Ошибочный токен |

---

## **📚 Примеры использования**

### **Пример 1: Простая программа**
```bash
$ cat simple.src
fn main() {
    return 42;
}

$ ./minicompiler lex simple.src
1:1 KW_FN "fn"
1:4 IDENTIFIER "main"
1:8 LPAREN "("
1:9 RPAREN ")"
1:11 LBRACE "{"
2:5 KW_RETURN "return"
2:12 INT_LITERAL "42" [42]
2:14 SEMICOLON ";"
3:1 RBRACE "}"
4:1 END_OF_FILE ""
```

### **Пример 2: Строки с escape-последовательностями**
```bash
$ cat strings.src
string s = "Hello\nWorld\t!";

$ ./minicompiler lex strings.src
1:1 IDENTIFIER "string"
1:8 IDENTIFIER "s"
1:10 EQUAL "="
1:12 STRING_LITERAL "\"Hello\\nWorld\\t!\"" ["Hello\nWorld\t!"]
1:30 SEMICOLON ";"
2:1 END_OF_FILE ""
```

### **Пример 3: Комментарии**
```bash
$ cat comments.src
// Это комментарий
x = 5; // комментарий после кода
/* Многострочный
   комментарий */
y = 10;

$ ./minicompiler lex comments.src
2:1 IDENTIFIER "x"
2:3 EQUAL "="
2:5 INT_LITERAL "5" [5]
2:6 SEMICOLON ";"
5:1 IDENTIFIER "y"
5:3 EQUAL "="
5:5 INT_LITERAL "10" [10]
5:7 SEMICOLON ";"
6:1 END_OF_FILE ""
```

### **Пример 4: Операторы**
```bash
$ cat operators.src
x += 5 * 2;
y = (x > 10) && flag;

$ ./minicompiler lex operators.src
1:1 IDENTIFIER "x"
1:3 PLUS_EQUAL "+="
1:6 INT_LITERAL "5" [5]
1:8 STAR "*"
1:10 INT_LITERAL "2" [2]
1:11 SEMICOLON ";"
2:1 IDENTIFIER "y"
2:3 EQUAL "="
2:5 LPAREN "("
2:6 IDENTIFIER "x"
2:8 GREATER ">"
2:10 INT_LITERAL "10" [10]
2:12 RPAREN ")"
2:14 AMP_AMP "&&"
2:17 IDENTIFIER "flag"
2:21 SEMICOLON ";"
3:1 END_OF_FILE ""
```

---

## **⚠️ Обработка ошибок**

### **Типы ошибок**
```bash
# Неизвестный символ
$ echo "int @ = 5;" | ./minicompiler lex
[Line 1, Col 5] Error: Unexpected character: '@'

# Незакрытая строка
$ echo "string s = \"hello;" | ./minicompiler lex
[Line 1, Col 12] Error: Unterminated string

# Неправильная escape-последовательность
$ echo "\"invalid\\x\"" | ./minicompiler lex
[Line 1, Col 9] Error: Invalid escape sequence: \x

# Новая строка в строковом литерале
$ echo "\"hello\nworld\"" | ./minicompiler lex
[Line 1, Col 7] Error: Newline character in string literal (use \n)

# Незакрытый комментарий
$ echo "/* незакрытый комментарий" | ./minicompiler lex
[Line 1, Col 27] Error: Unterminated multi-line comment

# Число вне диапазона
$ echo "int x = 2147483648;" | ./minicompiler lex
[Line 1, Col 9] Error: Integer literal out of range
```

### **Восстановление после ошибок**
Лексер продолжает сканирование после ошибок:
```bash
$ cat errors.src
int x = 123abc;
string s = "unclosed;
int y = 5;

$ ./minicompiler lex errors.src
[Line 1, Col 9] Error: Invalid number literal: '123abc'
[Line 2, Col 11] Error: Unterminated string
3:1 KW_INT "int"
3:5 IDENTIFIER "y"
3:7 EQUAL "="
3:9 INT_LITERAL "5" [5]
3:10 SEMICOLON ";"
4:1 END_OF_FILE ""
```

---

## **🧪 Тестирование**

### **Запуск тестов**
```bash
# Все тесты одной командой
make test-all

# Или по отдельности:
make test-valid      # 25 валидных тестов
make test-errors     # 11 тестов с ошибками
make test-preprocess # тест препроцессора
make test-compile    # тест компиляции
make check          # полная проверка
```

### **Статистика тестов**
- **Валидные тесты:** 25
- **Тесты с ошибками:** 11
- **Общее покрытие:** ~3000 строк кода
- **Производительность:** O(n), файлы до 1 МБ

### **Пример ручного тестирования**
```bash
# Создать тестовый файл
cat > test.mini << 'EOF'
#define MAX 100
fn test() {
    int arr[MAX];
    return 0;
}
EOF

# Проверить препроцессор
./minicompiler preprocess test.mini

# Проверить лексер
./minicompiler compile test.mini
```

---

## **🏗️ Архитектура**

### **Структура проекта**
```
compiler-project/
├── include/
│   ├── lexer/
│   │   ├── Scanner.hpp      # Интерфейс сканера
│   │   ├── Token.hpp        # Структура токена
│   │   └── TokenType.hpp    # Перечисление типов
│   └── utils/
│       └── ErrorReporter.hpp # Обработка ошибок
├── src/
│   ├── lexer/
│   │   ├── Scanner.cpp      # Реализация сканера
│   │   └── Token.cpp        # Реализация токена
│   └── utils/
│       └── ErrorReporter.cpp
└── tests/
    ├── test_runner_final.cpp # Валидные тесты
    └── lexer/
        └── invalid/
            └── test_errors.cpp # Тесты с ошибками
```

### **Компоненты**
1. **Scanner** - основной класс, выполняющий токенизацию
2. **Token** - структура данных для хранения токена
3. **TokenType** - перечисление всех возможных типов
4. **ErrorReporter** - централизованная обработка ошибок

---

## **⚙️ Настройка производительности**

### **Оптимизация**
- Лексер работает за O(n) времени
- Минимальное копирование строк
- Эффективное использование памяти
- Поддержка файлов до 1 МБ

### **Ограничения**
- Максимальная длина идентификатора: 255 символов
- Диапазон целых чисел: 32-bit signed
- Поддержка только ASCII (не-ASCII символы заменяются пробелами)

---

## **🐛 Отладка**

### **Режим подробного вывода**
```bash
# Показать все токены с подробностями
./minicompiler lex -v examples/hello.src

# Сохранить вывод в файл для анализа
./minicompiler lex examples/hello.src > debug.log

# Посмотреть прогресс сканирования
./minicompiler lex examples/hello.src | grep -n "IDENTIFIER"
```

### **Проверка конкретных конструкций**
```bash
# Проверить только числа
echo "42 3.14 0.5" | ./minicompiler lex

# Проверить только строки
echo "\"hello\" \"world\"" | ./minicompiler lex

# Проверить только операторы
echo "+ - * / % == != < >" | ./minicompiler lex
```

---

## **📈 Планы развития**

### **Спринт 2 (Следующий этап)**
- Синтаксический анализатор (парсер)
- Построение AST
- Проверка грамматики

### **Будущие улучшения**
- Поддержка Unicode
- Расширенные escape-последовательности
- Оптимизация производительности
- Интеграция с IDE

---

## **🤝 Участие в разработке**

### **Как добавить новый тип токена**
1. Добавить в `TokenType.hpp` новое значение
2. Добавить в `tokenTypeToString` в `Token.cpp`
3. Добавить распознавание в `Scanner.cpp`
4. Добавить тесты в `test_runner_final.cpp`

### **Как добавить тест**
```cpp
// В test_runner_final.cpp добавить:
{
    "New test name",
    "input code here",
    {
        "1:1 TOKEN_TYPE \"lexeme\"",
        "1:5 END_OF_FILE \"\""
    }
}
```

---

## **📚 Дополнительные ресурсы**

- [Спецификация языка](docs/language_spec.md)
- [Техническое задание Спринта 1](docs/sprint1_requirements.md)
- [Документация препроцессора](docs/preprocessor.md)
- [System V ABI (для будущих спринтов)](https://wiki.osdev.org/System_V_ABI)

---

## **📊 Статус реализации**

| Компонент | Статус | Покрытие тестами |
|-----------|--------|------------------|
| Базовые токены | ✅ 100% | 25 тестов |
| Идентификаторы | ✅ 100% | 3 теста |
| Литералы | ✅ 100% | 6 тестов |
| Операторы | ✅ 100% | 5 тестов |
| Комментарии | ✅ 100% | 3 теста |
| Обработка ошибок | ✅ 100% | 11 тестов |
| Препроцессор | ✅ 100% | 5 примеров |

---

## **📝 Лицензия**

Учебный проект. Свободное использование для образовательных целей.

---

**Версия документа:** 1.1.0  
**Последнее обновление:** 2024  
**Автор:** Команда MiniCompiler

---

*Данный документ соответствует требованиям Спринта 1 и описывает текущее состояние лексического анализатора.*