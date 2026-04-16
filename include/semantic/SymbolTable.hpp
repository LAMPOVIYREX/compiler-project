#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>
#include "parser/AST.hpp"

namespace minicompiler {

// Типы символов
enum class SymbolKind {
    VARIABLE,
    PARAMETER,
    FUNCTION,
    STRUCT,
    FIELD
};

// Структура для хранения информации о символе
struct SymbolInfo {
    std::string name;
    Type type;
    SymbolKind kind;
    int line;
    int column;
    
    // Для функций
    std::optional<Type> returnType;
    std::vector<Type> parameterTypes;
    std::vector<std::string> parameterNames;
    
    // Для структур
    std::unordered_map<std::string, Type> fields;
    
    // Для Memory Layout 
    int stackOffset = 0;
    int size = 0;
    bool initialized = false;
    
    // Конструктор 
    SymbolInfo(const std::string& n, const Type& t, SymbolKind k, int l, int c)
        : name(n), type(t), kind(k), line(l), column(c), size(t.getSize()) {}
    
    std::string toString() const {
        std::string result = name + " : " + type.toString();
        if (kind == SymbolKind::FUNCTION) {
            result += " -> " + (returnType ? returnType->toString() : "void");
        }
        return result;
    }
};

// Класс таблицы символов
class SymbolTable {
public:
    SymbolTable();
    
    // Управление областями видимости
    void enterScope();
    void exitScope();
    int getCurrentScopeDepth() const { return scopes.size(); }
    
    // Вставка символов
    bool insert(const std::string& name, std::shared_ptr<SymbolInfo> symbol);
    bool insert(const std::string& name, const Type& type, SymbolKind kind, int line, int column);
    
    // Поиск символов
    std::shared_ptr<SymbolInfo> lookup(const std::string& name) const;
    std::shared_ptr<SymbolInfo> lookupLocal(const std::string& name) const;
    bool isDeclaredInCurrentScope(const std::string& name) const;
    
    // Для Memory Layout
    const std::unordered_map<std::string, std::shared_ptr<SymbolInfo>>& getCurrentScope() const {
        if (scopes.empty()) {
            static std::unordered_map<std::string, std::shared_ptr<SymbolInfo>> empty;
            return empty;
        }
        return scopes.back();
    }
    
    // Для отладки
    void dump() const;
    std::string toString() const;
    std::string getMemoryLayoutString() const;

private:
    // Стек областей видимости
    std::vector<std::unordered_map<std::string, std::shared_ptr<SymbolInfo>>> scopes;
    
    bool addToScope(std::unordered_map<std::string, std::shared_ptr<SymbolInfo>>& scope,
                    const std::string& name, std::shared_ptr<SymbolInfo> symbol);
};

} // namespace minicompiler