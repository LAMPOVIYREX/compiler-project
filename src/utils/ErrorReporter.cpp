#include "utils/ErrorReporter.hpp"
#include <iostream>

namespace minicompiler {

void ErrorReporter::report(int line, int column, const std::string& message) {
    errors.emplace_back(message, line, column);
}

void ErrorReporter::printErrors() const {
    for (const auto& error : errors) {
        std::cerr << "[Line " << error.line << ", Col " << error.column 
                  << "] Error: " << error.message << std::endl;
    }
}

} // namespace minicompiler