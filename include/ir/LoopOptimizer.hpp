#pragma once
#include <memory>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include "ir/IR.hpp"

namespace minicompiler {

class LoopOptimizer {
public:
    LoopOptimizer();
    
    // Основной метод оптимизации циклов
    void optimize(IRProgram& program);
    
    // Настройки
    void setMoveInvariants(bool enable) { moveInvariants = enable; }
    void setOptimizeCounted(bool enable) { optimizeCounted = enable; }
    
private:
    bool moveInvariants = true;
    bool optimizeCounted = true;
    
    // Структура для хранения информации о цикле
    struct LoopInfo {
        BasicBlock* header;           // Заголовок цикла
        BasicBlock* latch;            // Последний блок (с jump назад)
        BasicBlock* exit;             // Выход из цикла
        std::unordered_set<BasicBlock*> blocks;  // Все блоки цикла
        
        // Условие цикла (для counted loop)
        IROperand* inductionVar = nullptr;   // Индукционная переменная
        int initialValue = 0;
        int stepValue = 0;
        int endValue = 0;
        bool isCounted = false;
    };
    
    // Поиск циклов в функции
    std::vector<LoopInfo> findLoops(IRFunction* func);
    
    // Вынос инвариантов
    void moveLoopInvariants(IRFunction* func, LoopInfo& loop);
    
    // Оптимизация counted loop
    void optimizeCountedLoop(IRFunction* func, LoopInfo& loop);
    
    // Проверка, является ли инструкция инвариантной в цикле
    bool isLoopInvariant(IRInstruction* instr, const LoopInfo& loop);
    
    // Проверка, является ли переменная индукционной
    bool detectInductionVariable(LoopInfo& loop);
    
    // Минимизация jump в теле цикла
    void minimizeJumps(IRFunction* func, LoopInfo& loop);
    
    // Инвертирование условия для уменьшения jump
    void invertLoopCondition(IRFunction* func, LoopInfo& loop);
};

} // namespace minicompiler