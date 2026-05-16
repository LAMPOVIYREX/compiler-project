#include "codegen/StackFrame.hpp"
#include <algorithm>

namespace minicompiler {

void StackFrame::addSlot(const std::string& name, int size, int alignment) {
    if (slots.find(name) != slots.end()) return;
    
    int offset = totalSize;
    offset = alignTo(offset, alignment);
    offset += size;
    totalSize = offset;
    
    slots[name] = {name, -(offset + (isLeaf ? RED_ZONE_SIZE : 0)), size, IRType::INT};
}

void StackFrame::addParameter(const std::string& name, int index) {
    addSlot(name, 8, 8);
    if (index < MAX_PARAM_REGS) {
        paramRegs[name] = PARAM_REGS[index];
    }
}

std::string StackFrame::getSlot(const std::string& name) const {
    auto it = slots.find(name);
    if (it != slots.end()) {
        return "qword [rbp" + std::to_string(it->second.offset) + "]";
    }
    return "qword [rbp-8]";
}

std::string StackFrame::getParamReg(int index) const {
    return (index < MAX_PARAM_REGS) ? PARAM_REGS[index] : "rax";
}

void StackFrame::clear() {
    slots.clear();
    paramRegs.clear();
    totalSize = 0;
    isLeaf = false;
}

int StackFrame::alignTo(int value, int alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

} // namespace minicompiler