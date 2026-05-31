# MiniCompiler Architecture

## Overview
MiniCompiler transforms MiniLang source code into x86-64 Linux executables in a multi-stage pipeline.

## Pipeline Stages
1. **Preprocessor** (`src/preprocessor/`) – handles `#define`, `#ifdef`, `#if`, `#endif`.
2. **Lexer** (`src/lexer/`) – tokenizes source into `Token` stream.
3. **Parser** (`src/parser/`) – builds AST using recursive descent, LL(1) grammar.
4. **Semantic Analyzer** (`src/semantic/`) – populates symbol table, checks types, scopes.
5. **IR Generator** (`src/ir/`) – lowers AST into three-address IR with basic blocks.
6. **Optimizer** (`src/ir/Optimizer.cpp`) – constant folding, propagation, DCE, algebraic simplifications.
7. **SSA Builder** (`src/ir/SSABuilder.cpp`) – SSA transformation with variable renaming.
8. **Code Generator** (`src/codegen/`) – translates IR to NASM x86-64 assembly (System V AMD64 ABI).

## Key Modules
| Module | Files | Description |
|--------|-------|-------------|
| Preprocessor | `Preprocessor.cpp`, `PreprocessorFrontend.cpp` | Macro expansion, conditional compilation |
| Lexer | `Scanner.cpp`, `Token.cpp`, `TokenType.hpp` | Token types, scanning |
| Parser | `Parser.cpp`, `AST.cpp`, `AST*.cpp` | Recursive descent, AST nodes, visitors |
| Semantic | `SemanticAnalyzer.cpp`, `SymbolTable.cpp` | Type checking, scope management |
| IR | `IR.cpp`, `IRGenerator.cpp` | Three-address code, basic blocks |
| Optimizer | `Optimizer.cpp` | Constant folding, propagation, DCE, algebraic |
| Codegen | `X86Generator.cpp`, `StackFrame.cpp` | x86-64 instruction selection, stack frames |
| Runtime | `runtime.asm` | `_start`, `printInt`, `readInt`, `exit` |

## Adding a New Language Feature
1. Update `TokenType.hpp` and `Scanner.cpp` for new tokens.
2. Modify `Parser.cpp` to parse the new syntax.
3. Add AST node in `AST.hpp`, implement visitor methods.
4. Update `SemanticAnalyzer.cpp` for type checking.
5. Extend `IRGenerator.cpp` to emit IR instructions.
6. Handle in `X86Generator.cpp` if needed.

## Debugging
- Use `--verbose` to see tokens, AST, symbol table, memory layout.
- Use `--stats` for compilation statistics.
- Use `--optimize --stats` to see optimization counts.
- Output IR with `./minicompiler ir file.mini`.
- Output SSA with `./minicompiler ssa file.mini`.
