#pragma once
#include <memory>
#include <string>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "ir/IR.hpp"
#include "semantic/SymbolTable.hpp"
#include "utils/ErrorReporter.hpp"

namespace minicompiler {

struct StackSlot {
    std::string name;
    int offset;
    int size;
    IRType type;
};

struct GlobalVar {
    std::string name;
    std::string value;
    bool isInitialized;
};

class X86Generator {
public:
    X86Generator(SymbolTable& symbolTable, ErrorReporter& errorReporter);
    std::string generate(IRProgram& irProgram);
    
    void setSyntaxNASM() { useNasmSyntax = true; }
    void setSyntaxGAS() { useNasmSyntax = false; }
    
private:
    SymbolTable& symbolTable;
    ErrorReporter& errorReporter;
    std::stringstream output;

    bool isLeafFunction = false;
    void analyzeLeafFunction(IRFunction& func);
    
    bool useNasmSyntax = true;
    
    IRFunction* currentFunction = nullptr;
    int currentStackSize = 0;
    std::unordered_map<std::string, StackSlot> stackSlots;
    std::unordered_map<std::string, std::string> varToReg;
    
    std::vector<GlobalVar> globalVars;
    std::vector<std::string> stringLiterals;
    std::unordered_set<std::string> externFunctions;
    
    int labelCounter = 0;
    int stringLabelCounter = 0;
    
    void generateFunctionHeader(IRFunction& func);
    void generateFunctionFooter(IRFunction& func);
    void generateBasicBlock(BasicBlock& block);
    void generateInstruction(IRInstruction& instr);
    
    void emitBinaryOp(IROpcode op, const IROperand& dest, const IROperand& src1, const IROperand& src2);
    void emitDivMod(IROpcode op, const IROperand& dest, const IROperand& src1, const IROperand& src2);
    void emitUnaryOp(const std::string& opcode, const IROperand& dest, const IROperand& src);
    void genComparison(IROpcode op, const IROperand& dest, const IROperand& src1, const IROperand& src2);
    void genJumpIf(const IROperand& cond, const IROperand& target, bool invert);
    void genMove(const IROperand& dest, const IROperand& src);
    void genLoad(const IROperand& dest, const IROperand& src);
    void genStore(const IROperand& dest, const IROperand& src);
    void genReturn(const IROperand& value);
    void genCall(const IROperand& dest, const IROperand& func);
    void genParam(const IROperand& value);
    void saveResultToDest(const IROperand& dest);
    
    void computeStackLayout(IRFunction& func);
    void emitPrologue(IRFunction& func);
    void emitEpilogue(IRFunction& func);
    int alignTo(int value, int alignment);
    
    std::string getOperandString(const IROperand& op);
    std::string getRegister(int paramIndex);
    std::string getStackSlot(const std::string& varName);
    std::string newLabel();
    std::string newStringLabel();
    
    void emit(const std::string& line, const std::string& comment = "");
    void emitLabel(const std::string& name);
    void emitBlank();

    
    std::string getFloatOperand(const IROperand& op);
    void saveFloatResult(const IROperand& dest);
    
    static constexpr const char* PARAM_REGS[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
    static constexpr const char* CALLER_SAVED[] = {"rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11"};
    static constexpr const char* CALLEE_SAVED[] = {"rbx", "rsp", "rbp", "r12", "r13", "r14", "r15"};
    static constexpr int MAX_PARAM_REGS = 6;
    static constexpr int STACK_ALIGNMENT = 16;
    static constexpr int RED_ZONE_SIZE = 128;
};

} // namespace minicompiler