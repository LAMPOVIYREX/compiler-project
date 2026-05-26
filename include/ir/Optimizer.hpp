#pragma once
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include "ir/IR.hpp"

namespace minicompiler {

class IROptimizer {
public:
    IROptimizer();
    
    void optimize(IRProgram& program);
    
    void setConstantFolding(bool enable) { doConstantFolding = enable; }
    void setConstantPropagation(bool enable) { doConstantPropagation = enable; }
    void setDeadCodeElimination(bool enable) { doDeadCodeElim = enable; }
    
    struct Stats {
        int foldedConstants = 0;
        int propagatedConstants = 0;
        int deadInstructions = 0;
        int algebraicSimplifications = 0;    
        int totalBefore = 0;
        int totalAfter = 0;
    };
    
    Stats getStats() const { return stats; }
    
private:
    bool doConstantFolding = true;
    bool doConstantPropagation = true;
    bool doDeadCodeElim = true;
    Stats stats;
    
    // Проходы оптимизации
    bool foldConstants(IRFunction* func);
    bool propagateConstants(IRFunction* func);
    bool eliminateDeadCode(IRFunction* func);
    
    // Вспомогательные
    int countInstructions(IRFunction* func);
    bool isConstant(const IROperand& op) const;
    int evaluateConstant(IROpcode op, int left, int right) const;
    bool algebraicSimplification(IRFunction* func);
};

} // namespace minicompiler