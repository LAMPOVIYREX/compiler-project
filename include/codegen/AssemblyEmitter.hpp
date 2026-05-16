#pragma once
#include <string>
#include <sstream>

namespace minicompiler {

class AssemblyEmitter {
public:
    AssemblyEmitter() {}
    
    void emit(const std::string& line, const std::string& comment = "");
    void emitLabel(const std::string& name);
    void emitBlank();
    void emitComment(const std::string& text);
    void emitSection(const std::string& name);
    void emitGlobal(const std::string& name);
    void emitExtern(const std::string& name);
    
    std::string getOutput() const { return output.str(); }
    
private:
    std::stringstream output;
};

} // namespace minicompiler