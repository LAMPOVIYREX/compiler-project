#include "parser/ASTDotGenerator.hpp"
#include <iomanip>
#include <sstream>

namespace minicompiler {

ASTDotGenerator::ASTDotGenerator(std::ostream& output)
    : output(output), nodeCounter(0) {
    result << "digraph AST {\n";
    result << "  node [shape=box, style=filled, fontname=\"Courier\"];\n";
    result << "  edge [fontname=\"Courier\"];\n";
}

void ASTDotGenerator::clear() {
    result.str("");
    result << "digraph AST {\n";
    result << "  node [shape=box, style=filled, fontname=\"Courier\"];\n";
    result << "  edge [fontname=\"Courier\"];\n";
    nodeCounter = 0;
    while (!parentStack.empty()) parentStack.pop();
}

int ASTDotGenerator::createNode(const std::string& label, const std::string& shape, 
                                 const std::string& color) {
    int id = nodeCounter++;
    result << "  node" << id << " [label=\"" << escapeString(label) 
           << "\", shape=" << shape << ", color=" << color << "];\n";
    return id;
}

void ASTDotGenerator::connectNodes(int fromId, int toId, const std::string& label) {
    if (!label.empty()) {
        result << "  node" << fromId << " -> node" << toId 
               << " [label=\"" << escapeString(label) << "\"];\n";
    } else {
        result << "  node" << fromId << " -> node" << toId << ";\n";
    }
}

std::string ASTDotGenerator::escapeString(const std::string& str) {
    std::string escaped;
    for (char c : str) {
        switch (c) {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\n': escaped += "\\n"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += c; break;
        }
    }
    return escaped;
}

std::string ASTDotGenerator::literalValueToString(const LiteralValue& value) {
    if (std::holds_alternative<int>(value)) {
        return std::to_string(std::get<int>(value));
    } else if (std::holds_alternative<double>(value)) {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << std::get<double>(value);
        return ss.str();
    } else if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value) ? "true" : "false";
    } else if (std::holds_alternative<std::string>(value)) {
        return std::get<std::string>(value);
    }
    return "null";
}

std::string ASTDotGenerator::binaryOpToString(BinaryOp op) {
    return ::minicompiler::binaryOpToString(op);
}

std::string ASTDotGenerator::unaryOpToString(UnaryOp op) {
    return ::minicompiler::unaryOpToString(op);
}

std::string ASTDotGenerator::typeToString(const Type& type) {
    return type.toString();
}

std::string ASTDotGenerator::getNodeColor(ASTNode& node) {
    if (dynamic_cast<LiteralExprNode*>(&node)) return "lightblue";
    if (dynamic_cast<IdentifierExprNode*>(&node)) return "lightcyan";
    if (dynamic_cast<BinaryExprNode*>(&node)) return "lightgreen";
    if (dynamic_cast<UnaryExprNode*>(&node)) return "lightgreen";
    if (dynamic_cast<CallExprNode*>(&node)) return "lightyellow";
    if (dynamic_cast<IndexExprNode*>(&node)) return "lightyellow";
    if (dynamic_cast<MemberAccessExprNode*>(&node)) return "lightyellow";  // ДОБАВЛЕНО
    if (dynamic_cast<BlockStmtNode*>(&node)) return "lightgrey";
    if (dynamic_cast<ExprStmtNode*>(&node)) return "lightgrey";
    if (dynamic_cast<VarDeclStmtNode*>(&node)) return "lightpink";
    if (dynamic_cast<IfStmtNode*>(&node)) return "lightsalmon";
    if (dynamic_cast<WhileStmtNode*>(&node)) return "lightsalmon";
    if (dynamic_cast<ForStmtNode*>(&node)) return "lightsalmon";
    if (dynamic_cast<ReturnStmtNode*>(&node)) return "lightpink";
    if (dynamic_cast<FunctionDeclNode*>(&node)) return "lightcoral";
    if (dynamic_cast<StructDeclNode*>(&node)) return "lightcoral";
    if (dynamic_cast<ProgramNode*>(&node)) return "lightgoldenrodyellow";
    return "white";
}

//=============================================================================
// Expression Nodes
//=============================================================================

void ASTDotGenerator::visit(LiteralExprNode& node) {
    std::string label = "Literal\\n" + literalValueToString(node.value);
    int nodeId = createNode(label, "box", "lightblue");
    
    if (!parentStack.empty()) {
        connectNodes(parentStack.top(), nodeId);
    }
}

void ASTDotGenerator::visit(IdentifierExprNode& node) {
    std::string label = "Identifier\\n" + node.name;
    int nodeId = createNode(label, "box", "lightcyan");
    
    if (!parentStack.empty()) {
        connectNodes(parentStack.top(), nodeId);
    }
}

void ASTDotGenerator::visit(BinaryExprNode& node) {
    std::string label = "BinaryOp\\n" + binaryOpToString(node.op);
    int nodeId = createNode(label, "box", "lightgreen");
    
    if (!parentStack.empty()) {
        connectNodes(parentStack.top(), nodeId);
    }
    
    parentStack.push(nodeId);
    node.left->accept(*this);
    node.right->accept(*this);
    parentStack.pop();
}

void ASTDotGenerator::visit(UnaryExprNode& node) {
    std::string label = "UnaryOp\\n" + unaryOpToString(node.op);
    int nodeId = createNode(label, "box", "lightgreen");
    
    if (!parentStack.empty()) {
        connectNodes(parentStack.top(), nodeId);
    }
    
    parentStack.push(nodeId);
    node.operand->accept(*this);
    parentStack.pop();
}

void ASTDotGenerator::visit(CallExprNode& node) {
    std::string label = "Call\\n" + node.callee;
    int nodeId = createNode(label, "box", "lightyellow");
    
    if (!parentStack.empty()) {
        connectNodes(parentStack.top(), nodeId);
    }
    
    if (!node.arguments.empty()) {
        parentStack.push(nodeId);
        int argsId = createNode("Arguments", "box", "lightyellow");
        connectNodes(nodeId, argsId);
        parentStack.push(argsId);
        for (auto& arg : node.arguments) {
            arg->accept(*this);
        }
        parentStack.pop();
        parentStack.pop();
    }
}

void ASTDotGenerator::visit(IndexExprNode& node) {
    int nodeId = createNode("IndexOp", "box", "lightyellow");
    
    if (!parentStack.empty()) {
        connectNodes(parentStack.top(), nodeId);
    }
    
    parentStack.push(nodeId);
    
    int arrayId = createNode("Array", "box", "lightyellow");
    connectNodes(nodeId, arrayId);
    parentStack.push(arrayId);
    node.array->accept(*this);
    parentStack.pop();
    
    int indexId = createNode("Index", "box", "lightyellow");
    connectNodes(nodeId, indexId);
    parentStack.push(indexId);
    node.index->accept(*this);
    parentStack.pop();
    
    parentStack.pop();
}

// ДОБАВЛЕНО: реализация для MemberAccessExprNode
void ASTDotGenerator::visit(MemberAccessExprNode& node) {
    std::string label = "MemberAccess\\n." + node.member;
    int nodeId = createNode(label, "box", "lightyellow");
    
    if (!parentStack.empty()) {
        connectNodes(parentStack.top(), nodeId);
    }
    
    parentStack.push(nodeId);
    node.object->accept(*this);
    parentStack.pop();
}

//=============================================================================
// Statement Nodes
//=============================================================================

void ASTDotGenerator::visit(BlockStmtNode& node) {
    int nodeId = createNode("Block", "box", "lightgrey");
    
    if (!parentStack.empty()) {
        connectNodes(parentStack.top(), nodeId);
    }
    
    if (!node.statements.empty()) {
        parentStack.push(nodeId);
        for (auto& stmt : node.statements) {
            stmt->accept(*this);
        }
        parentStack.pop();
    }
}

void ASTDotGenerator::visit(ExprStmtNode& node) {
    int nodeId = createNode("ExprStmt", "box", "lightgrey");
    
    if (!parentStack.empty()) {
        connectNodes(parentStack.top(), nodeId);
    }
    
    parentStack.push(nodeId);
    node.expression->accept(*this);
    parentStack.pop();
}

void ASTDotGenerator::visit(VarDeclStmtNode& node) {
    std::string label = "VarDecl\\n" + typeToString(node.varType) + " " + node.name;
    int nodeId = createNode(label, "box", "lightpink");
    
    if (!parentStack.empty()) {
        connectNodes(parentStack.top(), nodeId);
    }
    
    if (node.initializer) {
        parentStack.push(nodeId);
        int initId = createNode("Initializer", "box", "lightpink");
        connectNodes(nodeId, initId);
        parentStack.push(initId);
        node.initializer->accept(*this);
        parentStack.pop();
        parentStack.pop();
    }
}

void ASTDotGenerator::visit(IfStmtNode& node) {
    int nodeId = createNode("IfStmt", "box", "lightsalmon");
    
    if (!parentStack.empty()) {
        connectNodes(parentStack.top(), nodeId);
    }
    
    parentStack.push(nodeId);
    
    int condId = createNode("Condition", "box", "lightsalmon");
    connectNodes(nodeId, condId);
    parentStack.push(condId);
    node.condition->accept(*this);
    parentStack.pop();
    
    int thenId = createNode("Then", "box", "lightsalmon");
    connectNodes(nodeId, thenId);
    parentStack.push(thenId);
    node.thenBranch->accept(*this);
    parentStack.pop();
    
    if (node.elseBranch) {
        int elseId = createNode("Else", "box", "lightsalmon");
        connectNodes(nodeId, elseId);
        parentStack.push(elseId);
        node.elseBranch->accept(*this);
        parentStack.pop();
    }
    
    parentStack.pop();
}

void ASTDotGenerator::visit(WhileStmtNode& node) {
    int nodeId = createNode("WhileStmt", "box", "lightsalmon");
    
    if (!parentStack.empty()) {
        connectNodes(parentStack.top(), nodeId);
    }
    
    parentStack.push(nodeId);
    
    int condId = createNode("Condition", "box", "lightsalmon");
    connectNodes(nodeId, condId);
    parentStack.push(condId);
    node.condition->accept(*this);
    parentStack.pop();
    
    int bodyId = createNode("Body", "box", "lightsalmon");
    connectNodes(nodeId, bodyId);
    parentStack.push(bodyId);
    node.body->accept(*this);
    parentStack.pop();
    
    parentStack.pop();
}

void ASTDotGenerator::visit(ForStmtNode& node) {
    int nodeId = createNode("ForStmt", "box", "lightsalmon");
    
    if (!parentStack.empty()) {
        connectNodes(parentStack.top(), nodeId);
    }
    
    parentStack.push(nodeId);
    
    if (node.init) {
        int initId = createNode("Init", "box", "lightsalmon");
        connectNodes(nodeId, initId);
        parentStack.push(initId);
        node.init->accept(*this);
        parentStack.pop();
    }
    
    if (node.condition) {
        int condId = createNode("Condition", "box", "lightsalmon");
        connectNodes(nodeId, condId);
        parentStack.push(condId);
        node.condition->accept(*this);
        parentStack.pop();
    }
    
    if (node.update) {
        int updateId = createNode("Update", "box", "lightsalmon");
        connectNodes(nodeId, updateId);
        parentStack.push(updateId);
        node.update->accept(*this);
        parentStack.pop();
    }
    
    int bodyId = createNode("Body", "box", "lightsalmon");
    connectNodes(nodeId, bodyId);
    parentStack.push(bodyId);
    node.body->accept(*this);
    parentStack.pop();
    
    parentStack.pop();
}

void ASTDotGenerator::visit(ReturnStmtNode& node) {
    int nodeId = createNode("ReturnStmt", "box", "lightpink");
    
    if (!parentStack.empty()) {
        connectNodes(parentStack.top(), nodeId);
    }
    
    if (node.value) {
        parentStack.push(nodeId);
        node.value->accept(*this);
        parentStack.pop();
    }
}

//=============================================================================
// Declaration Nodes
//=============================================================================

void ASTDotGenerator::visit(FunctionDeclNode& node) {
    std::string label = "Function\\n" + typeToString(node.returnType) + " " + node.name;
    int nodeId = createNode(label, "box", "lightcoral");
    
    if (!parentStack.empty()) {
        connectNodes(parentStack.top(), nodeId);
    }
    
    parentStack.push(nodeId);
    
    if (!node.parameters.empty()) {
        int paramsId = createNode("Parameters", "box", "lightcoral");
        connectNodes(nodeId, paramsId);
        parentStack.push(paramsId);
        for (const auto& param : node.parameters) {
            std::string paramLabel = typeToString(param.first) + " " + param.second;
            int paramId = createNode(paramLabel, "box", "lightcoral");
            connectNodes(paramsId, paramId);
        }
        parentStack.pop();
    }
    
    if (node.body) {
        node.body->accept(*this);
    }
    
    parentStack.pop();
}

void ASTDotGenerator::visit(StructDeclNode& node) {
    std::string label = "Struct\\n" + node.name;
    int nodeId = createNode(label, "box", "lightcoral");
    
    if (!parentStack.empty()) {
        connectNodes(parentStack.top(), nodeId);
    }
    
    if (!node.fields.empty()) {
        parentStack.push(nodeId);
        int fieldsId = createNode("Fields", "box", "lightcoral");
        connectNodes(nodeId, fieldsId);
        parentStack.push(fieldsId);
        for (auto& field : node.fields) {
            field->accept(*this);
        }
        parentStack.pop();
        parentStack.pop();
    }
}

void ASTDotGenerator::visit(ProgramNode& node) {
    int nodeId = createNode("Program", "box", "lightgoldenrodyellow");
    parentStack.push(nodeId);
    
    for (auto& decl : node.declarations) {
        decl->accept(*this);
    }
    
    parentStack.pop();
    result << "}\n";
    output << result.str();
}

} // namespace minicompiler