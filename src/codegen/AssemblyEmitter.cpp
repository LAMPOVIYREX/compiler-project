#include "codegen/AssemblyEmitter.hpp"

namespace minicompiler {

void AssemblyEmitter::emit(const std::string& line, const std::string& comment) {
    output << "    " << line;
    if (!comment.empty()) output << "    " << comment;
    output << "\n";
}

void AssemblyEmitter::emitLabel(const std::string& name) {
    output << name << "\n";
}

void AssemblyEmitter::emitBlank() {
    output << "\n";
}

void AssemblyEmitter::emitComment(const std::string& text) {
    output << "; " << text << "\n";
}

void AssemblyEmitter::emitSection(const std::string& name) {
    output << "section " << name << "\n";
}

void AssemblyEmitter::emitGlobal(const std::string& name) {
    output << "global " << name << "\n";
}

void AssemblyEmitter::emitExtern(const std::string& name) {
    output << "extern " << name << "\n";
}

} // namespace minicompiler