#pragma once
#include <string>
#include <sstream>

namespace minicompiler {

class LabelManager {
public:
    LabelManager() : counter(0) {}
    
    std::string newLabel(const std::string& prefix = ".L") {
        return prefix + std::to_string(counter++);
    }
    
    std::string newStringLabel() {
        return ".L.str" + std::to_string(stringCounter++);
    }
    
    std::string newBlockLabel(const std::string& funcName, const std::string& blockName) {
        return funcName + "_" + blockName;
    }
    
    std::string newExitLabel(const std::string& funcName) {
        return funcName + "_exit";
    }
    
    void reset() { counter = 0; stringCounter = 0; }
    
private:
    int counter;
    int stringCounter;
};

} // namespace minicompiler