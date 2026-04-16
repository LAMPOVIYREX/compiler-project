#include "parser/ASTPrettyPrinter.hpp"
#include <iomanip>
#include <sstream>

namespace minicompiler {

ASTPrettyPrinter::ASTPrettyPrinter(std::ostream& output, int indentSize)
    : output(output), indentLevel(0), indentSize(indentSize) {}

void ASTPrettyPrinter::indent() {
    for (int i = 0; i < indentLevel * indentSize; ++i) {
        result << ' ';
    }
}

std::string ASTPrettyPrinter::nodeLocation(ASTNode& node) {
    return "[line " + std::to_string(node.getLine()) + "]";
}

std::string ASTPrettyPrinter::binaryOpToString(BinaryOp op) {
    return ::minicompiler::binaryOpToString(op);
}

std::string ASTPrettyPrinter::unaryOpToString(UnaryOp op) {
    return ::minicompiler::unaryOpToString(op);
}

std::string ASTPrettyPrinter::typeToString(const Type& type) {
    return type.toString();
}

std::string ASTPrettyPrinter::literalValueToString(const LiteralValue& value) {
    if (std::holds_alternative<int>(value)) {
        return std::to_string(std::get<int>(value));
    } else if (std::holds_alternative<double>(value)) {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << std::get<double>(value);
        return ss.str();
    } else if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value) ? "true" : "false";
    } else if (std::holds_alternative<std::string>(value)) {
        return "\"" + std::get<std::string>(value) + "\"";
    }
    return "null";
}

//=============================================================================
// Expression Nodes
//=============================================================================

void ASTPrettyPrinter::visit(LiteralExprNode& node) {
    indent();
    result << "Literal: " << literalValueToString(node.value) 
           << " " << nodeLocation(node) << "\n";
}

void ASTPrettyPrinter::visit(IdentifierExprNode& node) {
    indent();
    result << "Identifier: " << node.name << " " << nodeLocation(node) << "\n";
}

void ASTPrettyPrinter::visit(BinaryExprNode& node) {
    indent();
    result << "BinaryOp: " << binaryOpToString(node.op) 
           << " " << nodeLocation(node) << "\n";
    increaseIndent();
    node.left->accept(*this);
    node.right->accept(*this);
    decreaseIndent();
}

void ASTPrettyPrinter::visit(UnaryExprNode& node) {
    indent();
    result << "UnaryOp: " << unaryOpToString(node.op) 
           << " " << nodeLocation(node) << "\n";
    increaseIndent();
    node.operand->accept(*this);
    decreaseIndent();
}

void ASTPrettyPrinter::visit(CallExprNode& node) {
    indent();
    result << "Call: " << node.callee << " " << nodeLocation(node) << "\n";
    if (!node.arguments.empty()) {
        increaseIndent();
        indent();
        result << "Arguments:\n";
        increaseIndent();
        for (auto& arg : node.arguments) {
            arg->accept(*this);
        }
        decreaseIndent();
        decreaseIndent();
    }
}

void ASTPrettyPrinter::visit(IndexExprNode& node) {
    indent();
    result << "IndexOp " << nodeLocation(node) << "\n";
    increaseIndent();
    indent();
    result << "Array:\n";
    increaseIndent();
    node.array->accept(*this);
    decreaseIndent();
    indent();
    result << "Index:\n";
    increaseIndent();
    node.index->accept(*this);
    decreaseIndent();
    decreaseIndent();
}

// ДОБАВЛЕНО: реализация для MemberAccessExprNode
void ASTPrettyPrinter::visit(MemberAccessExprNode& node) {
    indent();
    result << "MemberAccess: ." << node.member << " " << nodeLocation(node) << "\n";
    increaseIndent();
    node.object->accept(*this);
    decreaseIndent();
}

//=============================================================================
// Statement Nodes
//=============================================================================

void ASTPrettyPrinter::visit(BlockStmtNode& node) {
    indent();
    result << "Block " << nodeLocation(node) << "\n";
    increaseIndent();
    for (auto& stmt : node.statements) {
        stmt->accept(*this);
    }
    decreaseIndent();
}

void ASTPrettyPrinter::visit(ExprStmtNode& node) {
    indent();
    result << "ExprStmt " << nodeLocation(node) << "\n";
    increaseIndent();
    node.expression->accept(*this);
    decreaseIndent();
}

void ASTPrettyPrinter::visit(VarDeclStmtNode& node) {
    indent();
    result << "VarDecl: " << typeToString(node.varType) << " " << node.name 
           << " " << nodeLocation(node) << "\n";
    if (node.initializer) {
        increaseIndent();
        indent();
        result << "Initializer:\n";
        increaseIndent();
        node.initializer->accept(*this);
        decreaseIndent();
        decreaseIndent();
    }
}

void ASTPrettyPrinter::visit(IfStmtNode& node) {
    indent();
    result << "IfStmt " << nodeLocation(node) << "\n";
    increaseIndent();
    indent();
    result << "Condition:\n";
    increaseIndent();
    node.condition->accept(*this);
    decreaseIndent();
    indent();
    result << "Then:\n";
    increaseIndent();
    node.thenBranch->accept(*this);
    decreaseIndent();
    if (node.elseBranch) {
        indent();
        result << "Else:\n";
        increaseIndent();
        node.elseBranch->accept(*this);
        decreaseIndent();
    }
    decreaseIndent();
}

void ASTPrettyPrinter::visit(WhileStmtNode& node) {
    indent();
    result << "WhileStmt " << nodeLocation(node) << "\n";
    increaseIndent();
    indent();
    result << "Condition:\n";
    increaseIndent();
    node.condition->accept(*this);
    decreaseIndent();
    indent();
    result << "Body:\n";
    increaseIndent();
    node.body->accept(*this);
    decreaseIndent();
    decreaseIndent();
}

void ASTPrettyPrinter::visit(ForStmtNode& node) {
    indent();
    result << "ForStmt " << nodeLocation(node) << "\n";
    increaseIndent();
    
    if (node.init) {
        indent();
        result << "Init:\n";
        increaseIndent();
        node.init->accept(*this);
        decreaseIndent();
    }
    
    if (node.condition) {
        indent();
        result << "Condition:\n";
        increaseIndent();
        node.condition->accept(*this);
        decreaseIndent();
    }
    
    if (node.update) {
        indent();
        result << "Update:\n";
        increaseIndent();
        node.update->accept(*this);
        decreaseIndent();
    }
    
    indent();
    result << "Body:\n";
    increaseIndent();
    node.body->accept(*this);
    decreaseIndent();
    
    decreaseIndent();
}

void ASTPrettyPrinter::visit(ReturnStmtNode& node) {
    indent();
    result << "ReturnStmt " << nodeLocation(node) << "\n";
    if (node.value) {
        increaseIndent();
        node.value->accept(*this);
        decreaseIndent();
    }
}

//=============================================================================
// Declaration Nodes
//=============================================================================

void ASTPrettyPrinter::visit(FunctionDeclNode& node) {
    indent();
    result << "FunctionDecl: " << typeToString(node.returnType) << " " << node.name 
           << " " << nodeLocation(node) << "\n";
    
    increaseIndent();
    if (!node.parameters.empty()) {
        indent();
        result << "Parameters:\n";
        increaseIndent();
        for (const auto& param : node.parameters) {
            indent();
            result << typeToString(param.first) << " " << param.second << "\n";
        }
        decreaseIndent();
    }
    
    if (node.body) {
        node.body->accept(*this);
    }
    decreaseIndent();
}

void ASTPrettyPrinter::visit(StructDeclNode& node) {
    indent();
    result << "StructDecl: " << node.name << " " << nodeLocation(node) << "\n";
    
    if (!node.fields.empty()) {
        increaseIndent();
        indent();
        result << "Fields:\n";
        increaseIndent();
        for (auto& field : node.fields) {
            field->accept(*this);
        }
        decreaseIndent();
        decreaseIndent();
    }
}

void ASTPrettyPrinter::visit(ProgramNode& node) {
    result << "Program " << nodeLocation(node) << "\n";
    increaseIndent();
    for (auto& decl : node.declarations) {
        decl->accept(*this);
    }
    decreaseIndent();
    
    output << result.str();
}

} // namespace minicompiler