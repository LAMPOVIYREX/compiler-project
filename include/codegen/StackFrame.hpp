#pragma once
#include <string>
#include <unordered_map>
#include "ir/IR.hpp"

namespace minicompiler {

struct StackSlot {
    std::string name;
    int offset;
    int size;
    IRType type;
};

class StackFrame {
public:
    StackFrame() : totalSize(0), isLeaf(false) {}
    
    void addSlot(const std::string& name, int size, int alignment);
    void addParameter(const std::string& name, int index);
    void setLeaf(bool leaf) { isLeaf = leaf; }
    bool getIsLeaf() const { return isLeaf; }
    
    int getTotalSize() const { return totalSize; }
    std::string getSlot(const std::string& name) const;
    std::string getParamReg(int index) const;
    
    void clear();
    
    static int alignTo(int value, int alignment);
    
    static constexpr const char* PARAM_REGS[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
    static constexpr int MAX_PARAM_REGS = 6;
    static constexpr int RED_ZONE_SIZE = 128;
    static constexpr int STACK_ALIGNMENT = 16;
    
private:
    std::unordered_map<std::string, StackSlot> slots;
    std::unordered_map<std::string, std::string> paramRegs;
    int totalSize;
    bool isLeaf;
};

} // namespace minicompiler