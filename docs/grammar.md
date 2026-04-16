# Формальная LL(1)-грамматика MiniLang

## Обозначения
- Терминалы в кавычках `"if"`
- `*` - 0 или более повторений
- `|` - или (альтернатива)
- `[ ]` - опционально
- `{ }` - группа


## Терминалы

```
Ключевые слова:
  "fn" | "if" | "else" | "while" | "for" | "return"
  | "int" | "float" | "bool" | "string" | "struct" | "const"
  | "true" | "false"

Идентификаторы и литералы:
  identifier        : letter (letter | digit | "_")*
  integer_literal   : digit+
  float_literal     : digit+ "." digit+ [ ("e" | "E") [ "+" | "-" ] digit+ ]
  string_literal    : '"' (character | escape_sequence)* '"'
  boolean_literal   : "true" | "false"

Математические операции:
  math_add = "+"
  math_sub = "-"
  math_mul = "*"
  math_div = "/"
  math_mod = "%"

Операции сравнения:
  cmp_eq  = "=="
  cmp_ne = "!="
  cmp_lt  = "<"
  cmp_gt  = ">"
  cmp_le = "<="
  cmp_ge = ">="

Логические операции:
  logic_and = "&&"
  logic_or  = "||"
  logic_not = "!"

Операции присваивания:
  assign_simple      = "="
  assign_add  = "+="
  assign_sub  = "-="
  assign_mul  = "*="
  assign_div  = "/="
  assign_mod  = "%="

Инкремент и декремент:
  inc_op = "++"
  dec_op = "--"

Стрелка для возвращаемого типа:
  return_arrow = "->"

Разделители:
  ";" | "," | "." | ":" | "(" | ")" | "{" | "}" | "[" | "]"

Escape-последовательности:
  escape_seq ::= '\' ( 'n' | 't' | 'r' | 'b' | 'f' | 'v' | '0' | '"' | '\\' | '\'' )

EOF : маркер конца файла
```

## Таблица приоритетов операций
| Уровень | Категория | Операции | Ассоциативность |
|---------|-----------|----------|-----------------|
| 1 | Базовые элементы | литералы, переменные, `(выражение)` | - |
| 2 | Постфиксные | `()` `[]` `.` `++` `--` | Левая |
| 3 | Префиксные унарные | `-` `!` `++` `--` | Правая |
| 4 | Мультипликативные | `*` `/` `%` | Левая |
| 5 | Аддитивные | `+` `-` | Левая |
| 6 | Сравнения | `<` `>` `<=` `>=` | Левая |
| 7 | Проверки на равенство | `==` `!=` | Левая |
| 8 | Логическое И | `&&` | Левая |
| 9 | Логическое ИЛИ | `\|\|` | Левая |
| 10 | Присваивания | `=` `+=` `-=` `*=` `/=` `%=` | Правая |

## Грамматика языка

### Корневой элемент
```
Program ::= DeclarationList EOF

DeclarationList ::= Declaration DeclarationList
                  | ε   (пустая последовательность)
```

### Описания
```
Declaration ::= FunctionDecl
              | StructDecl

FunctionDecl ::= "fn" identifier "(" ParamList ")" ReturnType Block

ReturnType ::= "->" Type
             | ε   (функция ничего не возвращает)

ParamList ::= Param ParamTail
            | ε

ParamTail ::= "," Param ParamTail
            | ε

Param ::= Type identifier

StructDecl ::= "struct" identifier "{" MemberList "}"

MemberList ::= Member MemberList
             | ε

Member ::= Type identifier ";"
```

### Система типов
```
Type ::= "int"
       | "float"
       | "bool"
       | "string"
       | identifier   
```

### Инструкции
```
Statement ::= CompoundStmt
            | ConditionalStmt
            | LoopWhileStmt
            | LoopForStmt
            | ReturnStmt
            | ExpressionStmt
            | VarDecl
            | EmptyStmt

CompoundStmt ::= "{" StatementSequence "}"

ConditionalStmt ::= "if" "(" Expression ")" Statement ElseBranch

ElseBranch ::= "else" Statement
             | ε

LoopWhileStmt ::= "while" "(" Expression ")" Statement

LoopForStmt ::= "for" "(" InitPart ";" ConditionPart ";" UpdatePart ")" Statement

InitPart ::= VarDecl
           | ExpressionStmt
           | ";"

ConditionPart ::= Expression
                | ε

UpdatePart ::= Expression
             | ε

ReturnStmt ::= "return" ReturnValue ";"

ReturnValue ::= Expression
              | ε

ExpressionStmt ::= Expression ";"

EmptyStmt ::= ";"

StatementSequence ::= Statement StatementSequence
                    | ε
```

### Объявления переменных
```
VarDecl ::= Type identifier Initializer ";"

Initializer ::= "=" Expression
              | ε
```

### Система выражений
```
Expression ::= AssignmentExpr

AssignmentExpr ::= LogicalExpr AssignSuffix

AssignSuffix ::= AssignOp AssignmentExpr
               | ε

AssignOp ::= "="
           | "+="
           | "-="
           | "*="
           | "/="
           | "%="

LogicalExpr ::= AndExpr OrSuffix

OrSuffix ::= "||" AndExpr OrSuffix
           | ε

AndExpr ::= EqualityExpr AndSuffix

AndSuffix ::= "&&" EqualityExpr AndSuffix
            | ε

EqualityExpr ::= RelationalExpr EqualitySuffix

EqualitySuffix ::= EqualityOp RelationalExpr EqualitySuffix
                 | ε

EqualityOp ::= "=="
             | "!="

RelationalExpr ::= AdditiveExpr RelationalSuffix

RelationalSuffix ::= RelationalOp AdditiveExpr RelationalSuffix
                   | ε

RelationalOp ::= "<"
               | ">"
               | "<="
               | ">="

AdditiveExpr ::= MultiplicativeExpr AdditiveSuffix

AdditiveSuffix ::= AddOp MultiplicativeExpr AdditiveSuffix
                 | ε

AddOp ::= "+"
        | "-"

MultiplicativeExpr ::= UnaryExpr MultiplicativeSuffix

MultiplicativeSuffix ::= MulOp UnaryExpr MultiplicativeSuffix
                       | ε

MulOp ::= "*"
        | "/"
        | "%"

UnaryExpr ::= UnaryOp UnaryExpr
            | PostfixExpr

UnaryOp ::= "-"
          | "!"
          | "++"
          | "--"

PostfixExpr ::= PrimaryExpr PostfixSuffix

PostfixSuffix ::= PostfixOp PostfixSuffix
                | ε

PostfixOp ::= CallSuffix
            | IndexSuffix
            | MemberAccess
            | "++"
            | "--"

CallSuffix ::= "(" ArgList ")"

IndexSuffix ::= "[" Expression "]"

MemberAccess ::= "." identifier

PrimaryExpr ::= Literal
              | identifier
              | "(" Expression ")"

Literal ::= integer_literal
          | float_literal
          | string_literal
          | boolean_literal

ArgList ::= Expression ArgTail
          | ε

ArgTail ::= "," Expression ArgTail
          | ε
```

## Анализ FIRST и FOLLOW множеств

### Множества FIRST (начальные символы)
```
FIRST(Program)           = { "fn", "struct", ε }
FIRST(Declaration)       = { "fn", "struct" }
FIRST(FunctionDecl)      = { "fn" }
FIRST(StructDecl)        = { "struct" }
FIRST(Type)              = { "int", "float", "bool", "string", identifier }
FIRST(Statement)         = { "{", "if", "while", "for", "return", "int", "float", 
                              "bool", "string", identifier, ";", FIRST(Expression) }
FIRST(CompoundStmt)      = { "{" }
FIRST(ConditionalStmt)   = { "if" }
FIRST(LoopWhileStmt)     = { "while" }
FIRST(LoopForStmt)       = { "for" }
FIRST(ReturnStmt)        = { "return" }
FIRST(ExpressionStmt)    = FIRST(Expression)
FIRST(VarDecl)           = { "int", "float", "bool", "string", identifier }
FIRST(EmptyStmt)         = { ";" }
FIRST(Expression)        = { identifier, integer_literal, float_literal, 
                              string_literal, boolean_literal, "(", 
                              "-", "!", "++", "--" }
FIRST(AssignmentExpr)    = FIRST(LogicalExpr)
FIRST(LogicalExpr)       = FIRST(AndExpr)
FIRST(AndExpr)           = FIRST(EqualityExpr)
FIRST(EqualityExpr)      = FIRST(RelationalExpr)
FIRST(RelationalExpr)    = FIRST(AdditiveExpr)
FIRST(AdditiveExpr)      = FIRST(MultiplicativeExpr)
FIRST(MultiplicativeExpr) = FIRST(UnaryExpr)
FIRST(UnaryExpr)         = { "-", "!", "++", "--" } ∪ FIRST(PostfixExpr)
FIRST(PostfixExpr)       = FIRST(PrimaryExpr)
FIRST(PrimaryExpr)       = { identifier, integer_literal, float_literal, 
                              string_literal, boolean_literal, "(" }
FIRST(Literal)           = { integer_literal, float_literal, 
                              string_literal, boolean_literal }
```

### Множества FOLLOW (допустимые следующие символы)
```
FOLLOW(Program)           = { EOF }
FOLLOW(Declaration)       = { "fn", "struct", EOF }
FOLLOW(FunctionDecl)      = { "fn", "struct", EOF }
FOLLOW(StructDecl)        = { "fn", "struct", EOF }
FOLLOW(Type)              = { identifier, ";", ",", ")" }
FOLLOW(Statement)         = { "}", "else", EOF }
FOLLOW(CompoundStmt)      = { "}", "else", EOF }
FOLLOW(ConditionalStmt)   = { "}", "else", EOF }
FOLLOW(LoopWhileStmt)     = { "}", "else", EOF }
FOLLOW(LoopForStmt)       = { "}", "else", EOF }
FOLLOW(ReturnStmt)        = { "}", "else", EOF }
FOLLOW(ExpressionStmt)    = { "}", "else", EOF }
FOLLOW(VarDecl)           = { "}", "else", EOF }
FOLLOW(EmptyStmt)         = { "}", "else", EOF }
FOLLOW(Expression)        = { ";", ",", ")", "]", "}", "else", 
                              "=", "+=", "-=", "*=", "/=", "%=",
                              "||", "&&", "==", "!=", "<", ">", "<=", ">=",
                              "+", "-", "*", "/", "%" }
FOLLOW(AssignmentExpr)    = FOLLOW(Expression)
FOLLOW(LogicalExpr)       = { "=", "+=", "-=", "*=", "/=", "%=" } ∪ FOLLOW(Expression)
FOLLOW(AndExpr)           = { "||" } ∪ FOLLOW(LogicalExpr)
FOLLOW(EqualityExpr)      = { "&&" } ∪ FOLLOW(AndExpr)
FOLLOW(RelationalExpr)    = { "==", "!=" } ∪ FOLLOW(EqualityExpr)
FOLLOW(AdditiveExpr)      = { "<", ">", "<=", ">=" } ∪ FOLLOW(RelationalExpr)
FOLLOW(MultiplicativeExpr) = { "+", "-" } ∪ FOLLOW(AdditiveExpr)
FOLLOW(UnaryExpr)         = { "*", "/", "%" } ∪ FOLLOW(MultiplicativeExpr)
FOLLOW(PostfixExpr)       = FOLLOW(UnaryExpr)
FOLLOW(PrimaryExpr)       = FOLLOW(PostfixExpr)
```

## LL(1)-свойств

✅ **Отсутствие левой рекурсии** - все рекурсивные правила используют правостороннюю рекурсию

✅ **Левая факторизация выполнена** - общие префиксы вынесены в отдельные правила (например, `AssignSuffix`, `OrSuffix`)

✅ **Непересекающиеся FIRST множества** - для каждого нетерминала альтернативные правила имеют различные начальные символы

✅ **Пригодность для рекурсивного спуска** - грамматика может быть реализована с просмотром на один токен вперед