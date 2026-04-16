#include "semantic/SymbolTable.hpp"
#include <iostream>
#include <sstream>

namespace minicompiler {

SymbolTable::SymbolTable() {
    // Создаем глобальную область видимости
    scopes.push_back({});
}

void SymbolTable::enterScope() {
    scopes.push_back({});
}

void SymbolTable::exitScope() {
    if (scopes.size() > 1) {
        scopes.pop_back();
    }
}

bool SymbolTable::addToScope(std::unordered_map<std::string, std::shared_ptr<SymbolInfo>>& scope,
                              const std::string& name, std::shared_ptr<SymbolInfo> symbol) {
    if (scope.find(name) != scope.end()) {
        return false; // Уже существует
    }
    scope[name] = symbol;
    return true;
}

bool SymbolTable::insert(const std::string& name, std::shared_ptr<SymbolInfo> symbol) {
    return addToScope(scopes.back(), name, symbol);
}

bool SymbolTable::insert(const std::string& name, const Type& type, SymbolKind kind, int line, int column) {
    auto symbol = std::make_shared<SymbolInfo>(name, type, kind, line, column);
    return insert(name, symbol);
}

std::shared_ptr<SymbolInfo> SymbolTable::lookup(const std::string& name) const {
    // Ищем от самой внутренней к внешней
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return found->second;
        }
    }
    return nullptr;
}

std::shared_ptr<SymbolInfo> SymbolTable::lookupLocal(const std::string& name) const {
    if (!scopes.empty()) {
        auto found = scopes.back().find(name);
        if (found != scopes.back().end()) {
            return found->second;
        }
    }
    return nullptr;
}

bool SymbolTable::isDeclaredInCurrentScope(const std::string& name) const {
    return lookupLocal(name) != nullptr;
}

void SymbolTable::dump() const {
    std::cout << toString() << std::endl;
}

std::string SymbolTable::toString() const {
    std::stringstream ss;
    ss << "Symbol Table:" << std::endl;
    
    int scopeLevel = 0;
    for (const auto& scope : scopes) {
        std::string indent(scopeLevel * 2, ' ');
        ss << indent << "Scope level " << scopeLevel << ":" << std::endl;
        
        for (const auto& [name, symbol] : scope) {
            ss << indent << "  " << symbol->toString() << std::endl;
        }
        scopeLevel++;
    }
    
    return ss.str();
}

std::string SymbolTable::getMemoryLayoutString() const {
    std::stringstream ss;
    
    int scopeLevel = 0;
    for (const auto& scope : scopes) {
        std::string indent(scopeLevel * 2, ' ');
        ss << indent << "Scope level " << scopeLevel << ":" << std::endl;
        
        for (const auto& [name, symbol] : scope) {
            ss << indent << "  " << name;
            ss << " [offset=" << symbol->stackOffset;
            ss << ", size=" << symbol->size;
            ss << ", type=" << symbol->type.toString();
            ss << "]" << std::endl;
        }
        scopeLevel++;
    }
    
    return ss.str();
}

} // namespace minicompiler