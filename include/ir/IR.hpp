#pragma once
#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <iostream>
#include "parser/AST.hpp"

namespace minicompiler {

//=============================================================================
// Типы операндов IR
//=============================================================================

enum class IRType {
    VOID,
    INT,
    FLOAT,
    BOOL,
    STRING,
    LABEL
};

struct IROperand {
    enum class Kind {
        TEMP,       // t1, t2, t3...
        VARIABLE,   // x, y, result...
        LITERAL,    // 42, 3.14, true, "hello"
        LABEL       // L1, L2, entry, exit...
    };
    
    Kind kind;
    std::string name;
    IRType type;
    int intValue;
    double floatValue;
    bool boolValue;
    std::string stringValue;
    
    // Конструкторы
    IROperand() : kind(Kind::TEMP), type(IRType::VOID), intValue(0), floatValue(0.0), boolValue(false) {}
    
    static IROperand temp(int id, IRType type = IRType::INT) {
        IROperand op;
        op.kind = Kind::TEMP;
        op.name = "t" + std::to_string(id);
        op.type = type;
        return op;
    }
    
    static IROperand variable(const std::string& name, IRType type = IRType::INT) {
        IROperand op;
        op.kind = Kind::VARIABLE;
        op.name = name;
        op.type = type;
        return op;
    }
    
    static IROperand literal(int value) {
        IROperand op;
        op.kind = Kind::LITERAL;
        op.type = IRType::INT;
        op.intValue = value;
        return op;
    }
    
    static IROperand literal(double value) {
        IROperand op;
        op.kind = Kind::LITERAL;
        op.type = IRType::FLOAT;
        op.floatValue = value;
        return op;
    }
    
    static IROperand literal(bool value) {
        IROperand op;
        op.kind = Kind::LITERAL;
        op.type = IRType::BOOL;
        op.boolValue = value;
        return op;
    }
    
    static IROperand literal(const std::string& value) {
        IROperand op;
        op.kind = Kind::LITERAL;
        op.type = IRType::STRING;
        op.stringValue = value;
        return op;
    }
    
    static IROperand label(const std::string& name) {
        IROperand op;
        op.kind = Kind::LABEL;
        op.name = name;
        op.type = IRType::LABEL;
        return op;
    }
    
    std::string toString() const {
        switch (kind) {
            case Kind::TEMP: return name;
            case Kind::VARIABLE: return "[" + name + "]";
            case Kind::LITERAL:
                switch (type) {
                    case IRType::INT: return std::to_string(intValue);
                    case IRType::FLOAT: return std::to_string(floatValue);
                    case IRType::BOOL: return boolValue ? "true" : "false";
                    case IRType::STRING: return "\"" + stringValue + "\"";
                    default: return "?";
                }
            case Kind::LABEL: return name;
        }
        return "?";
    }
    
    bool isConstant() const {
        return kind == Kind::LITERAL;
    }
    
    int getConstantValue() const {
        return intValue;
    }
};

//=============================================================================
// IR Инструкции
//=============================================================================

enum class IROpcode {
    // Арифметические
    ADD, SUB, MUL, DIV, MOD, NEG,
    // Логические
    AND, OR, NOT, XOR,
    // Сравнения
    CMP_EQ, CMP_NE, CMP_LT, CMP_LE, CMP_GT, CMP_GE,
    // Память
    LOAD, STORE, ALLOCA,
    // Управление потоком
    JUMP, JUMP_IF, JUMP_IF_NOT, LABEL, PHI,
    // Функции
    CALL, RETURN, PARAM,
    // Данные
    MOVE, COPY
};

std::string opcodeToString(IROpcode op);

class IRInstruction {
public:
    IRInstruction(IROpcode op, const IROperand& dest = IROperand(),
                  const IROperand& src1 = IROperand(),
                  const IROperand& src2 = IROperand());
    
    IROpcode opcode;
    IROperand dest;
    IROperand src1;
    IROperand src2;
    std::string comment;
    
    std::string toString() const;
    bool isControlFlow() const;
    bool isTerminator() const;
};

//=============================================================================
// Базовый блок
//=============================================================================

class BasicBlock {
public:
    BasicBlock(const std::string& name);
    
    std::string name;
    std::vector<std::unique_ptr<IRInstruction>> instructions;
    std::vector<BasicBlock*> predecessors;
    std::vector<BasicBlock*> successors;
    
    void addInstruction(std::unique_ptr<IRInstruction> instr);
    void addSuccessor(BasicBlock* block);
    void addPredecessor(BasicBlock* block);
    
    std::string toString() const;
    bool isEmpty() const { return instructions.empty(); }
    bool isTerminated() const;
};

//=============================================================================
// IR Функция
//=============================================================================

class IRFunction {
public:
    IRFunction(const std::string& name, IRType returnType);
    
    std::string name;
    IRType returnType;
    std::vector<IROperand> parameters;
    std::vector<std::unique_ptr<BasicBlock>> blocks;
    std::unordered_map<std::string, BasicBlock*> blockMap;
    int tempCounter;
    
    BasicBlock* createBlock(const std::string& name);
    BasicBlock* getBlock(const std::string& name);
    IROperand newTemp(IRType type = IRType::INT);
    
    void addParameter(const IROperand& param);
    
    std::string toString() const;
    void verify() const;
};

//=============================================================================
// IR Программа
//=============================================================================

class IRProgram {
public:
    IRProgram();
    
    std::vector<std::unique_ptr<IRFunction>> functions;
    std::unordered_map<std::string, IRFunction*> functionMap;
    
    IRFunction* createFunction(const std::string& name, IRType returnType);
    IRFunction* getFunction(const std::string& name);
    
    void addFunction(std::unique_ptr<IRFunction> func);
    
    std::string toString() const;
    void verify() const;
};

} // namespace minicompiler