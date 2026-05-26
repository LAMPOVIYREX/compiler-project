# MiniCompiler — Учебный компилятор для MiniLang

### Ключевые особенности

#### Спринт 1: Лексический анализатор
- Полноценный лексический анализатор с поддержкой всех типов токенов
- Препроцессор с поддержкой макросов и условной компиляции (`#define`, `#ifdef`, `#if`, `#elif`, `#else`, `#endif`)
- Подробная обработка ошибок с указанием позиции и кода ошибки
- 36 автоматических тестов (25 валидных + 11 с ошибками)
- Производительность O(n), поддержка файлов до 1 МБ
- Поддержка escape-последовательностей в строках (`\n`, `\t`, `\r`, `\"`, `\\`)
- Вложенные комментарии `/* /* */ */`
- Восстановление после ошибок (panic mode)

#### Спринт 2: Синтаксический анализатор
- Парсер рекурсивного спуска с поддержкой LL(1) грамматики
- Полная иерархия узлов AST (Abstract Syntax Tree)
- Поддержка всех операторов с правильным приоритетом и ассоциативностью
- Объявления функций, структур и переменных
- Управляющие конструкции: `if-else`, `while`, `for`, `return`
- Вызовы функций с аргументами
- Индексация массивов `arr[10]`
- Доступ к полям структур `p.x`
- Постфиксные операторы `x++`, `x--`
- Составные присваивания `+=`, `-=`, `*=`, `/=`, `%=`
- Визуализация AST в текстовом формате, Graphviz DOT и JSON
- Интеграция с лексером и препроцессором
- 16 тестов парсера (6 валидных + 10 с синтаксическими ошибками)

#### Спринт 3: Семантический анализ
- Таблица символов с поддержкой вложенных областей видимости
- Проверка объявлений (дублирование, корректность типов)
- Проверка типов выражений и операций
- Проверка вызовов функций (количество и типы аргументов)
- Проверка оператора return (соответствие типу функции)
- Проверка условий в управляющих конструкциях (if, while, for)
- Проверка доступа к массивам и структурам
- **Memory Layout** — вычисление смещений переменных в стеке
- Подробные сообщения об ошибках на русском языке с подсказками по исправлению
- 15 тестов семантического анализа (5 валидных + 10 с ошибками)

#### Спринт 4: Промежуточное представление (IR)
- Генерация трёхадресного кода из декорированного AST
- Поддержка арифметических, логических операций и сравнений
- Инструкции управления потоком (JUMP, JUMP_IF, JUMP_IF_NOT)
- Вызовы функций (CALL, RETURN, PARAM)
- Базовые блоки и граф потока управления (CFG)
- **SSA форма** (Static Single Assignment) с переименованием переменных
- Constant folding и Dead code elimination (базовая реализация)
- Текстовый дампер IR для отладки
- 17 тестов IR (12 валидных + 3 с ошибками) + 2 теста SSA

#### Спринт 5: Генерация кода x86-64
- Генерация нативного ассемблерного кода x86-64 (NASM синтаксис)
- Соблюдение **System V AMD64 ABI** (передача параметров, возврат значений)
- Управление стековым фреймом (пролог/эпилог функций)
- Выделение памяти под локальные переменные с корректным выравниванием
- Трансляция IR-инструкций в x86-64 (ADD, SUB, MUL, DIV, CMP, JMP и др.)
- **Runtime библиотека** на ассемблере (`printInt`, `printString`, `readInt`, `readString`, `exit`, `_start`)
- Системные вызовы Linux x86-64 (syscall)
- Статическая линковка с рантайм-библиотекой
- Полный цикл: MiniLang → AST → IR → SSA → x86-64 → исполняемый файл
- 13 тестов кодогенерации с полным циклом компиляции и запуска

#### Спринт 6: Управляющие конструкции и оптимизации
- **Short-circuit evaluation** для логических операторов (`&&`, `||`)
- Оператор `!` через эффективный `xor rax, 1` без ветвления
- Вложенные условные операторы с уникальными метками
- Циклы `while` и `for` с правильной структурой переходов
- **Полная поддержка float** (арифметика, сравнения, int↔float через SSE)
- **Поддержка массивов** (ALLOCA, LOAD, STORE с индексными вычислениями)
- **Loop optimization** (вынос инвариантов, counted loops, минимизация jump)
- Сложные булевы выражения с сохранением short-circuit семантики
- Type promotion для смешанных выражений (int + float → float)
- 26 тестов: control flow (7), logical (3), float (3), arrays (3), integration (10)

#### Спринт 7: Продвинутые возможности и внешние вызовы
- **Расширенные оптимизации IR**:
  - Constant Folding (свёртка констант) — до 3 проходов
  - Constant Propagation (распространение констант) — замена переменных известными значениями
  - Dead Code Elimination (удаление мёртвого кода) — код после `return`/`jump`
  - Algebraic Simplification (алгебраические упрощения) — `x+0→x`, `x*1→x`, `x*0→0` и др.
  - Итеративный pipeline: оптимизации применяются циклически до стабилизации
- **Поддержка внешних вызовов (External Functions)**:
  - Объявление `extern`-функций с проверкой типов
  - Корректная обработка variadic-функций (`printf`, `scanf`)
  - Полная интеграция с libc: `printf`, `puts`, `malloc`, `free`, `memcpy`, `memset`, `strlen`, `strcmp`, `strcpy`, `sqrtf`, `sinf`, `cosf`
  - Соблюдение System V AMD64 ABI для внешних вызовов (регистры, выравнивание стека, `AL=0` для variadic)
- **Демонстрационная программа** `tests/demo/extern_demo.mini` — показывает работу 10 внешних функций одновременно
- **Статистика оптимизаций** выводится при флаге `--stats`
- **Флаг `--optimize`** для включения всех оптимизаций
- 41 тест кодогенерации (включая массивы, float, внешние вызовы, оптимизации)

---

## Быстрый старт

### Установка и сборка
```bash
# Клонирование репозитория
cd minicompiler

# Установка зависимостей (Ubuntu/Debian)
sudo apt install g++ nasm

# Сборка проекта
make clean
make

# Проверка работы
./minicompiler help
```

### Первая программа
Создайте файл `hello.mini`:
```c
fn main() -> int {
    return 42;
}
```

Запустите компиляцию и выполнение:
```bash
# Все этапы по отдельности
./minicompiler lex hello.mini          # Токены
./minicompiler parse hello.mini        # AST
./minicompiler check hello.mini        # Семантика
./minicompiler ir hello.mini           # IR
./minicompiler ssa hello.mini          # SSA
./minicompiler codegen hello.mini      # x86-64 ассемблер

# Сквозная компиляция и запуск одной командой
make build-and-run FILE=hello.mini
```

---

## Команды и использование

### Основные команды
```bash
# Сборка проекта
make all

# Очистка
make clean

# Токенизация файла (только лексер)
./minicompiler lex <file>

# Только препроцессор
./minicompiler preprocess <file>

# Парсинг и построение AST
./minicompiler parse <file> [options]

# Семантический анализ
./minicompiler check <file> [options]

# Генерация IR (трёхадресный код)
./minicompiler ir <file>

# Генерация SSA формы
./minicompiler ssa <file>

# Генерация x86-64 ассемблера
./minicompiler codegen <file>

# Полная компиляция (препроцессор + лексер + парсер + семантика)
./minicompiler compile <file>

# Справка
./minicompiler help
```

### Опции для команд parse/check/ir/codegen
```bash
--format <text|dot|json>   # Формат вывода AST (по умолчанию: text)
--output <file>            # Выходной файл (по умолчанию: stdout)
--verbose                  # Показать токены, AST, таблицу символов и Memory Layout
--stats                    # Показать статистику компиляции
--optimize                 # Включить оптимизации (только для codegen)
```

### Makefile цели
| Команда | Описание |
|---------|----------|
| `make all` | Собрать компилятор и runtime библиотеку |
| `make clean` | Очистить объектные файлы |
| `make runtime` | Собрать runtime библиотеку |
| `make test-lexer` | Запустить тесты лексера (Спринт 1) |
| `make test-parser` | Запустить валидные тесты парсера (Спринт 2) |
| `make test-parser-errors` | Запустить тесты парсера с ошибками |
| `make test-semantic` | Запустить тесты семантики (Спринт 3) |
| `make test-ir` | Запустить тесты IR (Спринт 4) |
| `make test-ssa` | Запустить тесты SSA формы |
| `make test-codegen` | Запустить все тесты кодогенерации (Спринт 5-7) |
| `make test-all` | Запустить все тесты (127+ тестов) |
| `make check-all` | Проверить все примеры на всех этапах |
| `make build-and-run FILE=x.mini` | Скомпилировать и запустить программу |
| `make build-and-run-extern FILE=x.mini` | Скомпилировать с libc и запустить (для extern-программ) |
| `make ast-file FILE=x` | Визуализация AST для указанного файла |
| `make help` | Показать справку |

---

## x86-64 Генерация кода

### System V AMD64 ABI

Кодогенератор соблюдает соглашения о вызовах Linux x86-64:

```
Передача параметров:
  Целочисленные: RDI, RSI, RDX, RCX, R8, R9
  С плавающей точкой: XMM0-XMM7
  Дополнительные: в стеке (справа налево)

Возврат значений:
  Целочисленные: RAX
  64-битные: RAX:RDX
  С плавающей точкой: XMM0

Сохранение регистров:
  Caller-saved: RAX, RCX, RDX, RSI, RDI, R8-R11
  Callee-saved: RBX, RSP, RBP, R12-R15

Выравнивание стека: 16 байт перед CALL
```

### Стековый фрейм
```
Высокие адреса
+-----------------+
| Аргумент 7+     |  [rbp+32] (если есть)
+-----------------+
| Аргумент 6      |  [rbp+24] (на стеке)
+-----------------+
| Адрес возврата  |  [rbp+16]
+-----------------+
| Сохраненный RBP |  [rbp]  ← RBP указывает сюда
+-----------------+
| Локальная 1     |  [rbp-8]
+-----------------+
| Локальная 2     |  [rbp-16]
+-----------------+
| ...             |
+-----------------+
|                 |  ← RSP указывает сюда
Низкие адреса
```

### Примеры генерации кода

**Арифметика:**
```c
fn main() -> int {
    int a = 10;
    int b = 32;
    return a + b;
}
```
```asm
main:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    mov qword [rbp-8], 10
    mov qword [rbp-16], 32
    mov rax, qword [rbp-8]
    add rax, qword [rbp-16]
    jmp main_exit
main_exit:
    mov rsp, rbp
    pop rbp
    ret
```

**Условный оператор с short-circuit:**
```c
fn main() -> int {
    int x = 0;
    if (x != 0) {     // false → short-circuit
        return 100;
    }
    return 42;
}
```
```asm
    mov qword [rbp-8], 0
    mov rax, qword [rbp-8]
    cmp rax, 0
    setne al
    movzx rax, al
    cmp rax, 0
    je main_else_2       ; короткое замыкание
main_then_1:
    mov rax, 100
    jmp main_exit
main_else_2:
    jmp main_endif_3
main_endif_3:
    mov rax, 42
    jmp main_exit
```

**Float операции:**
```c
fn main() -> int {
    float a = 3.5;
    float b = 2.0;
    float c = a + b;
    if (c > 5.0) { return 42; }
    return 0;
}
```
```asm
    mov eax, __float32__(3.500000)
    movd xmm0, eax
    movss dword [rbp-X], xmm0
    movss xmm0, dword [rbp-X]
    movss xmm1, dword [rbp-Y]
    addss xmm0, xmm1
    ucomiss xmm0, [5.0]
    seta al
    movzx rax, al
```

**Массивы:**
```c
fn main() -> int {
    int arr[5];
    arr[0] = 10;
    arr[1] = 20;
    return arr[0] + arr[1];
}
```
```asm
    sub rsp, 40              ; alloca arr[5]
    mov qword [rbp-X], rsp
    mov rax, 10
    mov rbx, qword [rbp-X]  ; адрес arr[0]
    mov [rbx], rax
    mov rax, 20
    mov rbx, qword [rbp-X]
    add rbx, 8              ; адрес arr[1]
    mov [rbx], rax
    mov rax, qword [rbp-X]
    mov rax, [rax]          ; load arr[0]
    ; ...
```

**Внешние вызовы (Extern):**
```c
extern printf(string format, ...) -> int;
extern malloc(int size) -> int;
extern free(int ptr) -> int;

fn main() -> int {
    printf("Hello from MiniLang!\n");
    int ptr = malloc(100);
    if (ptr != 0) {
        free(ptr);
        printf("malloc+free works!\n");
    }
    return 42;
}
```
```asm
    extern printf
    extern malloc
    extern free
    ...
    lea rdi, [rel L.str0]    ; param 1: string
    xor eax, eax             ; variadic: AL = 0
    call printf
    mov rdi, 100             ; param 1: size
    call malloc
    ; ...
    call free
```

### Runtime библиотека

Библиотека `src/runtime/runtime.asm` предоставляет:

| Функция | Описание | Сигнатура |
|---------|----------|-----------|
| `_start` | Точка входа, вызывает `main` | `void _start()` |
| `exit` | Завершение программы | `void exit(int code)` |
| `printInt` | Вывод целого числа | `void printInt(int n)` |
| `printString` | Вывод строки | `void printString(char* s)` |
| `printChar` | Вывод символа | `void printChar(char c)` |
| `readInt` | Чтение целого числа | `int readInt()` |
| `readString` | Чтение строки | `int readString(char* buf, int size)` |
| `readChar` | Чтение символа | `char readChar()` |
| `strlen` | Длина строки | `int strlen(char* s)` |
| `strcmp` | Сравнение строк | `int strcmp(char* s1, char* s2)` |
| `strcpy` | Копирование строки | `char* strcpy(char* dst, char* src)` |
| `memset` | Заполнение памяти | `void* memset(void* ptr, int val, int n)` |
| `memcpy` | Копирование памяти | `void* memcpy(void* dst, void* src, int n)` |

### Сборка и запуск

```bash
# 1. Компиляция MiniLang → ассемблер
./minicompiler codegen program.mini --optimize --output program.asm

# 2. Ассемблирование
nasm -f elf64 program.asm -o program.o

# 3. Линковка с рантаймом (для программ без extern)
ld -o program src/runtime/runtime.o program.o

# 4. Линковка с libc (для программ с extern-функциями)
gcc -no-pie -o program program.o -lm

# 5. Запуск
./program
echo $?  # Код возврата

# Или одной командой:
make build-and-run FILE=program.mini
make build-and-run-extern FILE=program.mini  # для extern-программ
```

---

## Структура проекта

```
minicompiler/
├── docs/                      # Документация
│   ├── grammar.md            # Формальная LL(1)-грамматика
│   ├── language_spec.md      # Спецификация языка
│   ├── LEXER_README.md       # Документация лексера
│   ├── PARSER_README.md      # Документация парсера
│   ├── SEMANTIC_README.md    # Документация семантического анализатора
│   └── IR_README.md          # Документация IR и SSA
├── examples/                  # Примеры кода
│   ├── factorial.src         # Рекурсивный и итеративный факториал
│   ├── hello.src             # Базовый пример
│   └── preprocess_test.src   # Тест препроцессора
├── include/                   # Заголовочные файлы
│   ├── lexer/                # Лексический анализатор
│   ├── parser/               # Синтаксический анализатор
│   ├── preprocessor/         # Препроцессор
│   ├── semantic/             # Семантический анализатор
│   ├── ir/                   # Промежуточное представление + оптимизации
│   │   ├── IR.hpp
│   │   ├── IRGenerator.hpp
│   │   ├── SSA.hpp
│   │   ├── SSABuilder.hpp
│   │   ├── Optimizer.hpp     # Основные оптимизации IR
│   │   └── LoopOptimizer.hpp # Оптимизация циклов (Sprint 6)
│   ├── codegen/              # Генерация кода x86-64
│   │   ├── X86Generator.hpp
│   │   ├── LabelManager.hpp
│   │   ├── StackFrame.hpp
│   │   └── AssemblyEmitter.hpp
│   └── utils/                # Утилиты
├── src/                       # Исходный код
│   ├── lexer/
│   ├── parser/
│   ├── preprocessor/
│   ├── semantic/
│   ├── ir/
│   │   ├── Optimizer.cpp     # Constant Folding, Propagation, DCE, Algebraic Simplification
│   │   └── LoopOptimizer.cpp
│   ├── codegen/
│   │   ├── X86Generator.cpp
│   │   ├── LabelManager.cpp
│   │   ├── StackFrame.cpp
│   │   └── AssemblyEmitter.cpp
│   ├── runtime/
│   │   └── runtime.asm
│   ├── utils/
│   └── main.cpp
├── tests/                      # Тесты
│   ├── lexer/                  # 36 тестов (Спринт 1)
│   ├── parser/                 # 16 тестов (Спринт 2)
│   ├── semantic/               # 17 тестов (Спринт 3)
│   ├── ir/                     # 17 тестов (Спринт 4)
│   ├── codegen/                # Скрипт и тесты кодогенерации (Спринт 5)
│   ├── control_flow/           # 7 тестов (Спринт 6)
│   ├── optimization/           # Тесты оптимизаций (Спринт 7)
│   │   ├── valid/              # Внешние вызовы, математика, строки
│   │   └── demo/               # Демонстрационные примеры всех оптимизаций
│   └── demo/                   # Демо-программы
├── Makefile
├── README.md
└── .gitignore
```

---

## Дорожная карта

### Спринт 1 (Завершен)
- [x] Лексический анализатор
- [x] Препроцессор с макросами
- [x] 36 автоматических тестов
- [x] Обработка ошибок с восстановлением

### Спринт 2 (Завершен)
- [x] Формальная LL(1)-грамматика
- [x] Парсер рекурсивного спуска
- [x] Полная иерархия AST узлов
- [x] Поддержка структур и массивов
- [x] Визуализация AST (текст, DOT, JSON)
- [x] 16 тестов парсера

### Спринт 3 (Завершен)
- [x] Таблица символов с областями видимости
- [x] Проверка типов и объявлений
- [x] Проверка вызовов функций
- [x] Проверка оператора return
- [x] Memory Layout (stack offsets)
- [x] 15 тестов семантического анализа

### Спринт 4 (Завершен)
- [x] Промежуточное представление (IR) — трёхадресный код
- [x] SSA форма с переименованием переменных
- [x] Базовые блоки и граф потока управления
- [x] Constant folding (базовая реализация)
- [x] Dead code elimination (базовая реализация)
- [x] 17 тестов IR + SSA

### Спринт 5 (Завершен)
- [x] Генерация кода x86-64 (NASM синтаксис)
- [x] System V AMD64 ABI
- [x] Стековые фреймы (пролог/эпилог)
- [x] Локальные переменные на стеке
- [x] Runtime библиотека (print, read, exit, _start)
- [x] Системные вызовы Linux
- [x] Полный цикл: исходный код → исполняемый файл
- [x] 13 тестов кодогенерации

### Спринт 6 (Завершен)
- [x] Short-circuit evaluation (&&, ||)
- [x] NOT operator через xor
- [x] Вложенные условные операторы
- [x] Циклы while и for
- [x] Полная поддержка float (SSE: addss, subss, mulss, divss, ucomiss)
- [x] Поддержка массивов (ALLOCA, LOAD, STORE)
- [x] Loop optimization (инварианты, counted loops, минимизация jump)
- [x] 26 тестов control flow/float/arrays

### Спринт 7 (Завершен)
- [x] Constant Folding (свёртка констант) — до 3 проходов
- [x] Constant Propagation (распространение констант) — замена переменных
- [x] Dead Code Elimination (удаление мёртвого кода)
- [x] Algebraic Simplification (алгебраические упрощения): `x+0→x`, `x*1→x`, `x*0→0`
- [x] Итеративный оптимизационный pipeline с флагом `--optimize`
- [x] Внешние вызовы (`extern`) с поддержкой variadic-функций
- [x] Интеграция с libc: `printf`, `puts`, `malloc`, `free`, `memcpy`, `memset`, `strlen`, `strcmp`, `strcpy`, `sqrtf`, `sinf`, `cosf`
- [x] Демонстрационная программа с 10 extern-функциями
- [x] Статистика оптимизаций при `--stats`
- [x] 41 тест кодогенерации (включая extern и оптимизации)

### Спринт 8 (Планируется)
- [ ] Консолидированный вывод ошибок с кодами
- [ ] Система предупреждений (`-Wall`, `-Werror`)
- [ ] Финальная документация и туториал
- [ ] Презентационные материалы

---

## Примеры программ

### Пример 1: Простая программа
```c
fn main() -> int {
    return 42;
}
```

### Пример 2: Арифметика и условия
```c
fn main() -> int {
    int a = 10;
    int b = 32;
    if (a + b == 42) {
        return 1;
    }
    return 0;
}
```

### Пример 3: Цикл while
```c
fn main() -> int {
    int i = 0;
    int sum = 0;
    while (i < 10) {
        sum = sum + i;
        i = i + 1;
    }
    return sum;  // 45
}
```

### Пример 4: Float операции
```c
fn main() -> int {
    float a = 3.5;
    float b = 2.0;
    float c = a + b;    // 5.5
    float d = a * b;    // 7.0
    if (c > 5.0) {
        return 42;
    }
    return 0;
}
```

### Пример 5: Массивы
```c
fn main() -> int {
    int arr[3];
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;
    return arr[0] + arr[1] + arr[2];  // 60
}
```

### Пример 6: Short-circuit
```c
fn main() -> int {
    int a = 0;
    // Правая часть не вычисляется из-за short-circuit
    if (a != 0) {
        return 100;  // недостижимо
    }
    return 42;
}
```

### Пример 7: Рекурсия
```c
fn factorial(int n) -> int {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

fn main() -> int {
    return factorial(5);  // 120
}
```

### Пример 8: Внешние вызовы (Extern)
```c
extern printf(string format, ...) -> int;
extern malloc(int size) -> int;
extern free(int ptr) -> int;
extern strlen(string s) -> int;
extern strcmp(string s1, string s2) -> int;
extern strcpy(int dest, string src) -> int;
extern memcpy(int dest, int src, int n) -> int;
extern memset(int ptr, int value, int n) -> int;

fn main() -> int {
    printf("=== MiniLang External Functions Demo ===\n");
    
    int p1 = malloc(100);
    int p2 = malloc(200);
    memset(p1, 0, 100);
    memcpy(p2, p1, 100);
    free(p1);
    free(p2);
    
    int len = strlen("MiniLang");
    printf("strlen = %d\n", len);
    
    int cmp = strcmp("abc", "abc");
    if (cmp == 0) {
        printf("strcmp works!\n");
    }
    
    return 42;
}
```

---

## Тестирование

### Запуск всех тестов
```bash
make test-all
```

### Статистика тестов
| Категория | Количество | Статус |
|-----------|------------|--------|
| Лексер (валидные) | 25 | ✅ |
| Лексер (ошибки) | 11 | ✅ |
| Парсер (валидные) | 6 | ✅ |
| Парсер (ошибки) | 10 | ✅ |
| Семантика (валидные) | 6 | ✅ |
| Семантика (ошибки) | 11 | ✅ |
| IR (валидные) | 12 | ✅ |
| IR (ошибки) | 3 | ✅ |
| SSA | 2 | ✅ |
| Кодогенерация (Sprint 5-7) | 41 | ✅ |
| **Всего** | **127** | ✅ |

---

## Документация
- [Спецификация языка MiniLang](docs/language_spec.md)
- [Документация лексера](docs/LEXER_README.md)
- [Документация парсера](docs/PARSER_README.md)
- [Документация семантического анализатора](docs/SEMANTIC_README.md)
- [Документация IR и SSA](docs/IR_README.md)
- [Грамматика языка](docs/grammar.md)
