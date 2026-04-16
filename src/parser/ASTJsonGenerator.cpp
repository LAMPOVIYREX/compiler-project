#include "parser/ASTJsonGenerator.hpp"
#include <iomanip>

namespace minicompiler {

ASTJsonGenerator::ASTJsonGenerator(std::ostream& output)
    : output(output), indentLevel(0), firstInObject(true), firstInArray(true) {}

void ASTJsonGenerator::clear() {
    result.str("");
    indentLevel = 0;
    firstInObject = true;
    firstInArray = true;
}

void ASTJsonGenerator::indent() {
    for (int i = 0; i < indentLevel; ++i) {
        result << "  ";
    }
}

void ASTJsonGenerator::beginObject() {
    if (!firstInObject) {
        result << ",";
    }
    result << "\n";
    indent();
    result << "{";
    indentLevel++;
    firstInObject = true;
}

void ASTJsonGenerator::endObject() {
    indentLevel--;
    result << "\n";
    indent();
    result << "}";
    firstInObject = false;
}

void ASTJsonGenerator::beginArray() {
    if (!firstInArray) {
        result << ",";
    }
    result << "\n";
    indent();
    result << "[";
    indentLevel++;
    firstInArray = true;
}

void ASTJsonGenerator::endArray() {
    indentLevel--;
    result << "\n";
    indent();
    result << "]";
    firstInArray = false;
}

void ASTJsonGenerator::addKey(const std::string& key) {
    if (!firstInObject) {
        result << ",";
    }
    result << "\n";
    indent();
    result << "\"" << escapeString(key) << "\": ";
    firstInObject = false;
}

void ASTJsonGenerator::addValue(const std::string& value) {
    if (!firstInArray) {
        result << ",";
    }
    result << "\n";
    indent();
    result << "\"" << escapeString(value) << "\"";
    firstInArray = false;
}

void ASTJsonGenerator::addNumber(const std::string& value) {
    if (!firstInArray) {
        result << ",";
    }
    result << "\n";
    indent();
    result << value;
    firstInArray = false;
}

void ASTJsonGenerator::addBool(bool value) {
    if (!firstInArray) {
        result << ",";
    }
    result << "\n";
    indent();
    result << (value ? "true" : "false");
    firstInArray = false;
}

void ASTJsonGenerator::addNull() {
    if (!firstInArray) {
        result << ",";
    }
    result << "\n";
    indent();
    result << "null";
    firstInArray = false;
}

std::string ASTJsonGenerator::escapeString(const std::string& str) {
    std::string escaped;
    for (char c : str) {
        switch (c) {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += c; break;
        }
    }
    return escaped;
}

std::string ASTJsonGenerator::literalValueToString(const LiteralValue& value) {
    if (std::holds_alternative<int>(value)) {
        return std::to_string(std::get<int>(value));
    } else if (std::holds_alternative<double>(value)) {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << std::get<double>(value);
        return ss.str();
    } else if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value) ? "true" : "false";
    } else if (std::holds_alternative<std::string>(value)) {
        return "\"" + escapeString(std::get<std::string>(value)) + "\"";
    }
    return "null";
}

std::string ASTJsonGenerator::binaryOpToString(BinaryOp op) {
    switch (op) {
        case BinaryOp::ADD: return "+";
        case BinaryOp::SUB: return "-";
        case BinaryOp::MUL: return "*";
        case BinaryOp::DIV: return "/";
        case BinaryOp::MOD: return "%";
        case BinaryOp::EQ: return "==";
        case BinaryOp::NE: return "!=";
        case BinaryOp::LT: return "<";
        case BinaryOp::LE: return "<=";
        case BinaryOp::GT: return ">";
        case BinaryOp::GE: return ">=";
        case BinaryOp::AND: return "&&";
        case BinaryOp::OR: return "||";
        case BinaryOp::ASSIGN: return "=";
        case BinaryOp::ADD_ASSIGN: return "+=";
        case BinaryOp::SUB_ASSIGN: return "-=";
        case BinaryOp::MUL_ASSIGN: return "*=";
        case BinaryOp::DIV_ASSIGN: return "/=";
        case BinaryOp::MOD_ASSIGN: return "%=";
        default: return "?";
    }
}

std::string ASTJsonGenerator::unaryOpToString(UnaryOp op) {
    switch (op) {
        case UnaryOp::NEG: return "-";
        case UnaryOp::NOT: return "!";
        case UnaryOp::PRE_INC: return "++";
        case UnaryOp::PRE_DEC: return "--";
        case UnaryOp::POST_INC: return "++ (post)";
        case UnaryOp::POST_DEC: return "-- (post)";
        default: return "?";
    }
}

std::string ASTJsonGenerator::typeToString(const Type& type) {
    return type.toString();
}

std::string ASTJsonGenerator::nodeLocation(ASTNode& node) {
    std::ostringstream ss;
    ss << node.getLine() << ":" << node.getColumn();
    return ss.str();
}

//=============================================================================
// Expression Nodes
//=============================================================================

void ASTJsonGenerator::visit(LiteralExprNode& node) {
    beginObject();
    addKey("type");
    addValue("Literal");
    addKey("value");
    addValue(literalValueToString(node.value));
    addKey("location");
    addValue(nodeLocation(node));
    endObject();
}

void ASTJsonGenerator::visit(IdentifierExprNode& node) {
    beginObject();
    addKey("type");
    addValue("Identifier");
    addKey("name");
    addValue(node.name);
    addKey("location");
    addValue(nodeLocation(node));
    endObject();
}

void ASTJsonGenerator::visit(BinaryExprNode& node) {
    beginObject();
    addKey("type");
    addValue("BinaryOp");
    addKey("operator");
    addValue(binaryOpToString(node.op));
    addKey("left");
    node.left->accept(*this);
    addKey("right");
    node.right->accept(*this);
    addKey("location");
    addValue(nodeLocation(node));
    endObject();
}

void ASTJsonGenerator::visit(UnaryExprNode& node) {
    beginObject();
    addKey("type");
    addValue("UnaryOp");
    addKey("operator");
    addValue(unaryOpToString(node.op));
    addKey("operand");
    node.operand->accept(*this);
    addKey("location");
    addValue(nodeLocation(node));
    endObject();
}

void ASTJsonGenerator::visit(CallExprNode& node) {
    beginObject();
    addKey("type");
    addValue("Call");
    addKey("callee");
    addValue(node.callee);
    addKey("arguments");
    beginArray();
    for (auto& arg : node.arguments) {
        arg->accept(*this);
    }
    endArray();
    addKey("location");
    addValue(nodeLocation(node));
    endObject();
}

void ASTJsonGenerator::visit(IndexExprNode& node) {
    beginObject();
    addKey("type");
    addValue("Index");
    addKey("array");
    node.array->accept(*this);
    addKey("index");
    node.index->accept(*this);
    addKey("location");
    addValue(nodeLocation(node));
    endObject();
}

void ASTJsonGenerator::visit(MemberAccessExprNode& node) {
    beginObject();
    addKey("type");
    addValue("MemberAccess");
    addKey("member");
    addValue(node.member);
    addKey("object");
    node.object->accept(*this);
    addKey("location");
    addValue(nodeLocation(node));
    endObject();
}

//=============================================================================
// Statement Nodes
//=============================================================================

void ASTJsonGenerator::visit(BlockStmtNode& node) {
    beginObject();
    addKey("type");
    addValue("Block");
    addKey("statements");
    beginArray();
    for (auto& stmt : node.statements) {
        stmt->accept(*this);
    }
    endArray();
    addKey("location");
    addValue(nodeLocation(node));
    endObject();
}

void ASTJsonGenerator::visit(ExprStmtNode& node) {
    beginObject();
    addKey("type");
    addValue("ExpressionStatement");
    addKey("expression");
    node.expression->accept(*this);
    addKey("location");
    addValue(nodeLocation(node));
    endObject();
}

void ASTJsonGenerator::visit(VarDeclStmtNode& node) {
    beginObject();
    addKey("type");
    addValue("VariableDeclaration");
    addKey("varType");
    addValue(typeToString(node.varType));
    addKey("name");
    addValue(node.name);
    if (node.initializer) {
        addKey("initializer");
        node.initializer->accept(*this);
    }
    addKey("location");
    addValue(nodeLocation(node));
    endObject();
}

void ASTJsonGenerator::visit(IfStmtNode& node) {
    beginObject();
    addKey("type");
    addValue("If");
    addKey("condition");
    node.condition->accept(*this);
    addKey("then");
    node.thenBranch->accept(*this);
    if (node.elseBranch) {
        addKey("else");
        node.elseBranch->accept(*this);
    }
    addKey("location");
    addValue(nodeLocation(node));
    endObject();
}

void ASTJsonGenerator::visit(WhileStmtNode& node) {
    beginObject();
    addKey("type");
    addValue("While");
    addKey("condition");
    node.condition->accept(*this);
    addKey("body");
    node.body->accept(*this);
    addKey("location");
    addValue(nodeLocation(node));
    endObject();
}

void ASTJsonGenerator::visit(ForStmtNode& node) {
    beginObject();
    addKey("type");
    addValue("For");
    if (node.init) {
        addKey("init");
        node.init->accept(*this);
    }
    if (node.condition) {
        addKey("condition");
        node.condition->accept(*this);
    }
    if (node.update) {
        addKey("update");
        node.update->accept(*this);
    }
    addKey("body");
    node.body->accept(*this);
    addKey("location");
    addValue(nodeLocation(node));
    endObject();
}

void ASTJsonGenerator::visit(ReturnStmtNode& node) {
    beginObject();
    addKey("type");
    addValue("Return");
    if (node.value) {
        addKey("value");
        node.value->accept(*this);
    }
    addKey("location");
    addValue(nodeLocation(node));
    endObject();
}

//=============================================================================
// Declaration Nodes
//=============================================================================

void ASTJsonGenerator::visit(FunctionDeclNode& node) {
    beginObject();
    addKey("type");
    addValue("Function");
    addKey("name");
    addValue(node.name);
    addKey("returnType");
    addValue(typeToString(node.returnType));
    addKey("parameters");
    beginArray();
    for (const auto& param : node.parameters) {
        beginObject();
        addKey("type");
        addValue(typeToString(param.first));
        addKey("name");
        addValue(param.second);
        endObject();
    }
    endArray();
    if (node.body) {
        addKey("body");
        node.body->accept(*this);
    }
    addKey("location");
    addValue(nodeLocation(node));
    endObject();
}

void ASTJsonGenerator::visit(StructDeclNode& node) {
    beginObject();
    addKey("type");
    addValue("Struct");
    addKey("name");
    addValue(node.name);
    addKey("fields");
    beginArray();
    for (auto& field : node.fields) {
        field->accept(*this);
    }
    endArray();
    addKey("location");
    addValue(nodeLocation(node));
    endObject();
}

void ASTJsonGenerator::visit(ProgramNode& node) {
    result << "{\n";
    indentLevel = 0;
    firstInObject = true;
    beginObject();
    addKey("program");
    beginObject();
    addKey("declarations");
    beginArray();
    for (auto& decl : node.declarations) {
        decl->accept(*this);
    }
    endArray();
    endObject();
    endObject();
    result << "\n}\n";
    output << result.str();
}

} // namespace minicompiler