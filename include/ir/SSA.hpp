#pragma once
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <stack>
#include <queue>
#include <functional>
#include "ir/IR.hpp"

namespace minicompiler {

// ============================================================================
// SSA Value - значение в SSA форме
// ============================================================================

class SSAValue {
public:
    enum class Kind {
        CONSTANT,       // Константа
        VARIABLE,       // Переменная (с версией)
        TEMPORARY,      // Временная переменная
        PHI             // φ-функция
    };
    
    Kind kind;
    std::string name;
    int version;        // Версия для SSA (x1, x2, x3...)
    IRType type;
    
    // Для констант
    union {
        int intValue;
        double floatValue;
        bool boolValue;
    };
    std::string stringValue;
    
    // Для PHI
    std::vector<std::pair<SSAValue*, class SSABasicBlock*>> phiOperands;
    
    SSAValue() : kind(Kind::VARIABLE), version(0), type(IRType::VOID), intValue(0) {}
    
    std::string toString() const;
    static SSAValue* createConstant(int value);
    static SSAValue* createConstant(double value);
    static SSAValue* createConstant(bool value);
    static SSAValue* createVariable(const std::string& name, int version, IRType type);
    static SSAValue* createTemp(int id, IRType type);
    static SSAValue* createPhi(IRType type);
};

// Forward declaration
class SSABasicBlock;

// ============================================================================
// SSA Instruction - инструкция в SSA форме
// ============================================================================

class SSAInstruction {
public:
    IROpcode opcode;
    SSAValue* dest;
    SSAValue* src1;
    SSAValue* src2;
    std::string comment;
    
    SSAInstruction(IROpcode op, SSAValue* d = nullptr, 
                   SSAValue* s1 = nullptr, SSAValue* s2 = nullptr);
    
    std::string toString() const;
    bool isPhi() const { return opcode == IROpcode::PHI; }
    bool isControlFlow() const;
};

// ============================================================================
// SSA Basic Block - базовый блок в SSA форме
// ============================================================================

class SSABasicBlock {
public:
    std::string name;
    std::vector<std::unique_ptr<SSAInstruction>> instructions;
    std::vector<SSABasicBlock*> predecessors;
    std::vector<SSABasicBlock*> successors;
    
    // Для SSA построения
    std::unordered_set<SSABasicBlock*> dominanceFrontier;
    std::unordered_set<SSABasicBlock*> children;  // в дереве доминаторов
    SSABasicBlock* immediateDominator = nullptr;
    int domLevel = 0;  // уровень в дереве доминаторов
    
    // PHI функции размещенные в этом блоке
    std::unordered_map<std::string, SSAValue*> phiFunctions;
    
    explicit SSABasicBlock(const std::string& n) : name(n) {}
    
    void addInstruction(std::unique_ptr<SSAInstruction> instr);
    void addPhiFunction(const std::string& varName, SSAValue* phi);
    std::string toString() const;
};

// ============================================================================
// SSA Function - функция в SSA форме
// ============================================================================

class SSAFunction {
public:
    std::string name;
    IRType returnType;
    std::vector<std::pair<std::string, IRType>> parameters;
    std::vector<std::unique_ptr<SSABasicBlock>> blocks;
    std::unordered_map<std::string, SSABasicBlock*> blockMap;
    
    // Управление версиями переменных
    std::unordered_map<std::string, int> varVersions;
    std::unordered_map<std::string, std::stack<SSAValue*>> varStacks;
    
    SSAFunction(const std::string& n, IRType rt) : name(n), returnType(rt) {}
    
    SSABasicBlock* createBlock(const std::string& name);
    SSABasicBlock* getBlock(const std::string& name);
    
    int getNextVersion(const std::string& varName);
    void pushVar(const std::string& varName, SSAValue* value);
    SSAValue* getCurrentVar(const std::string& varName);
    void popVar(const std::string& varName);
    
    std::string toString() const;
};

// ============================================================================
// SSA Program - программа в SSA форме
// ============================================================================

class SSAProgram {
public:
    std::vector<std::unique_ptr<SSAFunction>> functions;
    std::unordered_map<std::string, SSAFunction*> functionMap;
    
    SSAFunction* createFunction(const std::string& name, IRType returnType);
    SSAFunction* getFunction(const std::string& name);
    std::string toString() const;
};

} // namespace minicompiler