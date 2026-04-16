#pragma once
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <queue>
#include <functional>
#include <algorithm>
#include "ir/IR.hpp"
#include "ir/SSA.hpp"

namespace minicompiler {

class SSABuilder {
public:
    SSABuilder();
    
    std::unique_ptr<SSAProgram> buildSSA(IRProgram& irProgram);
    
    void setOptimizeConstants(bool enable) { optimizeConstants = enable; }
    void setEliminateDeadCode(bool enable) { eliminateDeadCode = enable; }
    
private:
    std::unique_ptr<SSAProgram> ssaProgram;
    bool optimizeConstants = true;
    bool eliminateDeadCode = true;
    
    struct DomInfo {
        std::unordered_map<SSABasicBlock*, SSABasicBlock*> idom;
        std::unordered_map<SSABasicBlock*, std::unordered_set<SSABasicBlock*>> domFrontier;
        std::vector<SSABasicBlock*> reversePostOrder;
    };
    
    struct VarInfo {
        std::unordered_set<SSABasicBlock*> defBlocks;
        std::unordered_set<SSABasicBlock*> phiBlocks;
    };
    
    struct RenameState {
        std::unordered_map<std::string, std::stack<SSAValue*>> varStacks;
        std::unordered_map<std::string, int> varCounters;
    };
    
    // Вспомогательный метод для конвертации с переименованием
    SSAValue* convertOperandWithRename(const IROperand& op, 
                                        const std::unordered_map<std::string, std::string>& currentVar);
    
    // Заглушки для полной SSA (будут реализованы позже)
    DomInfo computeDominators(SSAFunction* func);
    void computeDominanceFrontier(SSAFunction* func, DomInfo& info);
    std::vector<SSABasicBlock*> computeReversePostOrder(SSABasicBlock* entry);
    void buildDominatorTree(SSAFunction* func, const DomInfo& info);
    std::unordered_map<std::string, VarInfo> collectDefSites(SSAFunction* func);
    void placePhiFunctions(SSAFunction* func, const DomInfo& domInfo);
    void renameVariables(SSAFunction* func, const DomInfo& domInfo);
    void renameBlock(SSABasicBlock* block, RenameState& state);
    SSAValue* createNewSSAValue(const std::string& name, IRType type, RenameState& state);
    void optimizeConstantsSSA(SSAFunction* func);
    bool foldConstant(SSAInstruction* instr);
    void eliminateDeadCodeSSA(SSAFunction* func);
    SSAValue* convertOperand(const IROperand& op);
    SSABasicBlock* findSSABlock(SSAFunction* func, const std::string& name);
};

} // namespace minicompiler