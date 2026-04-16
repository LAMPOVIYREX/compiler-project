# IR_README.md — Документация генерации промежуточного представления и SSA формы

---

## Быстрый старт

### Сборка проекта
```bash
# Очистка и сборка
make clean
make

# Проверка работы IR-генератора
./minicompiler ir examples/factorial.src

# Проверка работы SSA-генератора
./minicompiler ssa examples/factorial.src
```

### Базовое использование
```bash
# Генерация трёхадресного кода (IR)
./minicompiler ir <filename>

# Генерация SSA формы
./minicompiler ssa <filename>

# Сохранить результат в файл
./minicompiler ir examples/factorial.src --output factorial.ir

# Подробный вывод (включая токены, AST, таблицу символов)
./minicompiler ir examples/factorial.src --verbose

# Показать статистику
./minicompiler ir examples/factorial.src --stats
```

---

## Команды программы

| Команда | Описание | Пример |
|---------|----------|---------|
| `ir <file>` | Генерация трёхадресного кода | `./minicompiler ir factorial.src` |
| `ssa <file>` | Генерация SSA формы | `./minicompiler ssa factorial.src` |
| `ir <file> --output <file>` | Сохранить IR в файл | `./minicompiler ir factorial.src --output out.ir` |
| `ir <file> --verbose` | Подробный вывод всех этапов | `./minicompiler ir factorial.src --verbose` |
| `ir <file> --stats` | Статистика генерации | `./minicompiler ir factorial.src --stats` |

---

## Архитектура IR

### Компоненты

| Файл | Назначение |
|------|------------|
| `include/ir/IR.hpp` | Определение IR-инструкций, операндов, базовых блоков |
| `src/ir/IR.cpp` | Реализация IR-структур и методов вывода |
| `include/ir/IRGenerator.hpp` | Интерфейс генератора IR из AST |
| `src/ir/IRGenerator.cpp` | Реализация трансляции AST → IR |
| `include/ir/SSA.hpp` | Определение SSA-структур (значения, инструкции, блоки) |
| `src/ir/SSA.cpp` | Реализация SSA-структур |
| `include/ir/SSABuilder.hpp` | Интерфейс построителя SSA формы |
| `src/ir/SSABuilder.cpp` | Реализация алгоритма SSA-трансформации |

### Иерархия IR

```
IRProgram
└── IRFunction
    ├── Параметры
    ├── Локальные переменные
    └── BasicBlock[]
        └── IRInstruction[]
```

### Типы операндов IR

| Тип | Описание | Пример |
|-----|----------|--------|
| `TEMP` | Временная переменная | `t0`, `t1`, `t2` |
| `VARIABLE` | Именованная переменная | `[x]`, `[result]` |
| `LITERAL` | Константа | `42`, `3.14`, `true` |
| `LABEL` | Метка базового блока | `L1`, `entry`, `exit` |

---

## IR Инструкции

### Арифметические операции
| Инструкция | Описание | Пример |
|------------|----------|--------|
| `ADD` | Сложение | `t0 = ADD a, b` |
| `SUB` | Вычитание | `t0 = SUB a, b` |
| `MUL` | Умножение | `t0 = MUL a, b` |
| `DIV` | Деление | `t0 = DIV a, b` |
| `MOD` | Остаток от деления | `t0 = MOD a, b` |
| `NEG` | Унарный минус | `t0 = NEG a` |

### Логические операции и сравнения
| Инструкция | Описание | Пример |
|------------|----------|--------|
| `AND` | Логическое И | `t0 = AND a, b` |
| `OR` | Логическое ИЛИ | `t0 = OR a, b` |
| `NOT` | Логическое НЕ | `t0 = NOT a` |
| `CMP_EQ` | Равно | `t0 = CMP_EQ a, b` |
| `CMP_NE` | Не равно | `t0 = CMP_NE a, b` |
| `CMP_LT` | Меньше | `t0 = CMP_LT a, b` |
| `CMP_LE` | Меньше или равно | `t0 = CMP_LE a, b` |
| `CMP_GT` | Больше | `t0 = CMP_GT a, b` |
| `CMP_GE` | Больше или равно | `t0 = CMP_GE a, b` |

### Работа с памятью
| Инструкция | Описание | Пример |
|------------|----------|--------|
| `LOAD` | Загрузка из памяти | `t0 = LOAD [addr]` |
| `STORE` | Сохранение в память | `STORE [addr], value` |
| `MOVE` | Копирование значения | `[x] = MOVE t0` |

### Управление потоком
| Инструкция | Описание | Пример |
|------------|----------|--------|
| `JUMP` | Безусловный переход | `JUMP L1` |
| `JUMP_IF` | Условный переход (если истина) | `JUMP_IF t0, L1` |
| `JUMP_IF_NOT` | Условный переход (если ложь) | `JUMP_IF_NOT t0, L2` |
| `LABEL` | Метка базового блока | `L1:` |
| `RETURN` | Возврат из функции | `RETURN t0` |

### Вызовы функций
| Инструкция | Описание | Пример |
|------------|----------|--------|
| `PARAM` | Передача параметра | `PARAM t0` |
| `CALL` | Вызов функции | `t0 = CALL factorial` |

---

## SSA форма (Static Single Assignment)

### Что такое SSA?

SSA — это промежуточное представление, в котором каждая переменная присваивается **ровно один раз**. Для объединения значений из разных ветвей используются **φ-функции (phi-функции)**.

### Структуры SSA

```
SSAProgram
└── SSAFunction
    ├── Параметры
    └── SSABasicBlock[]
        ├── φ-функции
        └── SSAInstruction[]
```

### Типы SSA значений

| Тип | Описание | Пример |
|-----|----------|--------|
| `CONSTANT` | Константа | `42`, `true` |
| `VARIABLE` | Переменная с версией | `x0`, `x1`, `y0` |
| `TEMPORARY` | Временная переменная | `t0`, `t1` |
| `PHI` | φ-функция | `x2 = φ(x0:else, x1:then)` |

### Алгоритм построения SSA

1. **Построение CFG** — графа потока управления
2. **Вычисление доминаторов** — определение отношений доминирования между блоками
3. **Вычисление границ доминирования** (dominance frontiers)
4. **Размещение φ-функций** — в узлах границ доминирования
5. **Переименование переменных** — присвоение уникальных версий

---

## Примеры вывода

### Пример 1: Простая арифметика

**Исходный код:**
```c
fn main() -> int {
    int x = 2 + 3 * 4;
    return x;
}
```

**IR вывод:**
```
# MiniLang IR Program

function main: int ()

entry:
  t0 = MUL 3, 4
  t1 = ADD 2, t0
  [x] = MOVE t1
  RETURN [x]
```

### Пример 2: Условный оператор

**Исходный код:**
```c
fn main() -> int {
    int x = 10;
    if (x > 5) {
        x = x + 1;
    }
    return x;
}
```

**IR вывод:**
```
# MiniLang IR Program

function main: int ()

entry:
  [x] = MOVE 10
  t0 = CMP_GT [x], 5
  JUMP_IF t0, then_1
  JUMP else_2

then_1:
  t3 = ADD [x], 1
  [x] = MOVE t3
  JUMP endif_3

else_2:
  JUMP endif_3

endif_3:
  RETURN [x]
```

**SSA вывод:**
```
# MiniLang SSA IR Program

function main: int ()

entry:
  x0 = MOVE 10
  t0 = CMP_GT x0, 5
  JUMP_IF t0, then_1
  JUMP else_2

then_1:
  t3 = ADD x0, 1
  x1 = MOVE t3
  JUMP endif_3

else_2:
  JUMP endif_3

endif_3:
  t4 = ADD x1, y0
  RETURN t4
```

### Пример 3: Цикл while

**Исходный код:**
```c
fn main() -> int {
    int i = 0;
    int sum = 0;
    while (i < 10) {
        sum = sum + i;
        i = i + 1;
    }
    return sum;
}
```

**IR вывод:**
```
# MiniLang IR Program

function main: int ()

entry:
  [i] = MOVE 0
  [sum] = MOVE 0

loop_0:
  t0 = CMP_LT [i], 10
  JUMP_IF_NOT t0, exit_1

loop_0_body:
  t1 = ADD [sum], [i]
  [sum] = MOVE t1
  t2 = ADD [i], 1
  [i] = MOVE t2
  JUMP loop_0

exit_1:
  RETURN [sum]
```

**SSA вывод:**
```
# MiniLang SSA IR Program

function main: int ()

entry:
  i0 = MOVE 0
  sum0 = MOVE 0
  JUMP loop_0

loop_0:
  t0 = CMP_LT i0, 10
  JUMP_IF_NOT t0, exit_1

loop_0_body:
  t1 = ADD sum0, i0
  sum1 = MOVE t1
  t2 = ADD i0, 1
  i1 = MOVE t2
  JUMP loop_0

exit_1:
  RETURN sum1
```

### Пример 4: Рекурсивный факториал

**Исходный код:**
```c
fn factorial(int n) -> int {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}
```

**IR вывод:**
```
# MiniLang IR Program

function factorial: int ([n])

entry:
  t0 = CMP_LE [n], 1
  JUMP_IF t0, then_1
  JUMP else_2

then_1:
  RETURN 1

else_2:
  JUMP endif_3

endif_3:
  t3 = SUB [n], 1
  PARAM t3
  t4 = CALL factorial
  t5 = MUL [n], t4
  RETURN t5
```

---

## Тестирование

### Запуск тестов IR
```bash
# Запустить все тесты IR
make test-ir

# Запустить тесты SSA
make test-ssa

# Запустить все тесты (лексер + парсер + семантика + IR + SSA)
make test-all
```

### Структура тестов
```
tests/ir/
├── valid/
│   ├── simple_arith.mini
│   ├── simple_if.mini
│   ├── simple_while.mini
│   ├── arithmetic.mini
│   ├── comparisons.mini
│   ├── while.mini
│   ├── if_else.mini
│   ├── nested_functions.mini
│   ├── logical_operators.mini
│   ├── factorial_recursive.mini
│   ├── constant_folding.mini
│   └── dead_code.mini
├── invalid/
│   ├── wrong_operator_types.mini
│   ├── undeclared_variable.mini
│   └── type_mismatch.mini
├── ssa/
│   └── valid/
│       ├── simple_ssa.mini
│       └── loop_ssa.mini
└── ir_test_runner.cpp
```

### Статистика тестов
| Категория | Количество | Статус |
|-----------|------------|--------|
| IR валидные | 12 | ✅ |
| IR с ошибками | 3 | ✅ |
| SSA валидные | 2 | ✅ |
| **Всего IR тестов** | **17** | ✅ |

---

## Оптимизации IR

### Constant Folding
Свёртка константных выражений на этапе компиляции:
```
До: t0 = ADD 2, 3
После: t0 = MOVE 5
```

### Dead Code Elimination
Удаление неиспользуемого кода:
```
До:
  t0 = ADD a, b    # t0 не используется
  RETURN c
После:
  RETURN c
```

### Peephole оптимизации
Локальные оптимизации в пределах базового блока:
```
До:   t0 = ADD x, 0
После: t0 = MOVE x

До:   t0 = MUL x, 1
После: t0 = MOVE x
```

---

## Отладка

### Режим verbose
```bash
# Показать все этапы трансляции
./minicompiler ir examples/factorial.src --verbose
```

### Статистика генерации
```bash
./minicompiler ir examples/factorial.src --stats

Compilation Statistics:
   Всего инструкций IR: 15
   Базовых блоков: 4
   Временных переменных: 6
   Вызовов функций: 1
```

### Сравнение IR и SSA
```bash
# Сгенерировать обычный IR
./minicompiler ir factorial.src --output factorial.ir

# Сгенерировать SSA форму
./minicompiler ssa factorial.src --output factorial.ssa

# Сравнить
diff -u factorial.ir factorial.ssa
```

---



