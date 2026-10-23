#include "ast/nodes.hpp"
#include "ast/ast_visitor.hpp"

namespace ast {

void CompilationUnitNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
        if (packageDeclaration) packageDeclaration->accept(visitor);
        
        for (auto* imp : importDeclarations) {
            imp->accept(visitor);
        }
        
        for (auto* decl : typeDeclarations) {
            decl->accept(visitor);
        }
    }
    visitor.leave(this);
}

void PackageDeclarationNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
        if (name) name->accept(visitor);
    }
    visitor.leave(this);
}

void ImportDeclarationNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
        if (name) name->accept(visitor);
    }
    visitor.leave(this);
}

void TypeDeclarationNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
        for (auto modifier : modifiers) modifier->accept(visitor);
        if (name) name->accept(visitor);
        for (auto ext : extends) ext->accept(visitor);
        for (auto impl : implements) impl->accept(visitor);
        for (auto decl : bodyDeclarations) decl->accept(visitor);
    }
    visitor.leave(this);
}

void BodyDeclarationNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
    }
    visitor.leave(this);
}

void FieldDeclarationNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
        for (auto modifier : modifiers) modifier->accept(visitor);
        if (type) type->accept(visitor);
        for (auto fragment : fragments) fragment->accept(visitor);
    }
    visitor.leave(this);
}

void MethodDeclarationNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
        for (auto modifier : modifiers) modifier->accept(visitor);
        if (returnType) returnType->accept(visitor);
        if (name) name->accept(visitor);
        for (auto param : parameters) param->accept(visitor);
        if (body) body->accept(visitor);
    }
    visitor.leave(this);
}

void ConstructorDeclarationNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
        for (auto modifier : modifiers) modifier->accept(visitor);
        if (name) name->accept(visitor);
        for (auto param : parameters) param->accept(visitor);
        if (body) body->accept(visitor);
    }
    visitor.leave(this);
}

void VariableDeclarationNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
        if (name) name->accept(visitor);
        if (initializer) initializer->accept(visitor);
    }
    visitor.leave(this);
}

void SingleVariableDeclarationNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
        if (type) type->accept(visitor);
        if (name) name->accept(visitor);
        if (initializer) initializer->accept(visitor);
    }
    visitor.leave(this);
}

void VariableDeclarationFragmentNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
        if (name) name->accept(visitor);
        if (initializer) initializer->accept(visitor);
    }
    visitor.leave(this);
}

void StatementNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
    }
    visitor.leave(this);
}

void BlockNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
        for (auto statement : statements) statement->accept(visitor);
    }
    visitor.leave(this);
}

void EmptyStatementNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
    }
    visitor.leave(this);
}

void ExpressionStatementNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
        if (expression) expression->accept(visitor);
    }
    visitor.leave(this);
}

void ReturnStatementNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
        if (expression) expression->accept(visitor);
    }
    visitor.leave(this);
}

void IfStatementNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
        if (condition) condition->accept(visitor);
        if (thenBranch) thenBranch->accept(visitor);
        if (elseBranch) elseBranch->accept(visitor);
    }
    visitor.leave(this);
}

void WhileStatementNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
        if (condition) condition->accept(visitor);
        if (body) body->accept(visitor);
    }
    visitor.leave(this);
}

void ForStatementNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
        for (auto init : initializers) init->accept(visitor);
        if (condition) condition->accept(visitor);
        for (auto update : updates) update->accept(visitor);
        if (body) body->accept(visitor);
    }
    visitor.leave(this);
}

void VariableDeclarationStatementNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
        if (type) type->accept(visitor);
        for (auto fragment : fragments) fragment->accept(visitor);
    }
    visitor.leave(this);
}

void ExpressionNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
    }
    visitor.leave(this);
}

void AssignmentExpressionNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
        if (lhs) lhs->accept(visitor);
        if (rhs) rhs->accept(visitor);
    }
    visitor.leave(this);
}

void InfixExpressionNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
        if (lhs) lhs->accept(visitor);
        if (rhs) rhs->accept(visitor);
    }
    visitor.leave(this);
}

void InstanceofExpressionNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
        if (expression) expression->accept(visitor);
        if (type) type->accept(visitor);
    }
    visitor.leave(this);
}

void PrefixExpressionNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
        if (operand) operand->accept(visitor);
    }
    visitor.leave(this);
}

void CastExpressionNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
        if (type) type->accept(visitor);
        if (expression) expression->accept(visitor);
    }
    visitor.leave(this);
}

void VariableDeclarationExpressionNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
        if (type) type->accept(visitor);
        for (auto fragment : fragments) fragment->accept(visitor);
    }
    visitor.leave(this);
}

void TypeNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
    }
    visitor.leave(this);
}

void PrimitiveTypeNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
    }
    visitor.leave(this);
}

void ClassOrInterfaceTypeNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
        if (name) name->accept(visitor);
    }
    visitor.leave(this);
}

void ArrayTypeNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
        if (componentType) componentType->accept(visitor);
    }
    visitor.leave(this);
}

void ArrayCreationExpressionNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
        if (type) type->accept(visitor);
        if (size) size->accept(visitor);
    }
    visitor.leave(this);
}

void IntegerLiteralNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
    }
    visitor.leave(this);
}

void BooleanLiteralNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
    }
    visitor.leave(this);
}

void CharacterLiteralNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
    }
    visitor.leave(this);
}

void StringLiteralNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
    }
    visitor.leave(this);
}

void NullLiteralNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
    }
    visitor.leave(this);
}

void ThisExpressionNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
    }
    visitor.leave(this);
}

void ParenthesizedExpressionNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
        if (expression) expression->accept(visitor);
    }
    visitor.leave(this);
}

void FieldAccessExpressionNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
        if (expression) expression->accept(visitor);
        if (fieldName) fieldName->accept(visitor);
    }
    visitor.leave(this);
}

void MethodInvocationExpressionNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
        if (expression) expression->accept(visitor);
        if (methodName) methodName->accept(visitor);
        for (auto arg : arguments) arg->accept(visitor);
    }
    visitor.leave(this);
}

void InstanceCreationExpressionNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
        if (type) type->accept(visitor);
        for (auto arg : arguments) arg->accept(visitor);
    }
    visitor.leave(this);
}

void ArrayAccessExpressionNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
        if (array) array->accept(visitor);
        if (index) index->accept(visitor);
    }
    visitor.leave(this);
}

void NameNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
    }
    visitor.leave(this);
}

void SimpleNameNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
    }
    visitor.leave(this);
}

void QualifiedNameNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
        if (qualifier) qualifier->accept(visitor);
    }
    visitor.leave(this);
}

void ModifierNode::accept(ASTVisitor& visitor) {
    if (visitor.visit(this)) {
    }
    visitor.leave(this);
}

}

std::ostream& operator<<(std::ostream &os, const ast::NodeType &nodeType) {
    switch (nodeType) {
        case ast::NodeType::CompilationUnit: return os << "CompilationUnit";
        case ast::NodeType::PackageDeclaration: return os << "PackageDeclaration";
        case ast::NodeType::ImportDeclaration: return os << "ImportDeclaration";
        case ast::NodeType::TypeDeclaration: return os << "TypeDeclaration";
        case ast::NodeType::BodyDeclaration: return os << "BodyDeclaration";
        case ast::NodeType::FieldDeclaration: return os << "FieldDeclaration";
        case ast::NodeType::MethodDeclaration: return os << "MethodDeclaration";
        case ast::NodeType::ConstructorDeclaration: return os << "ConstructorDeclaration";
        case ast::NodeType::VariableDeclaration: return os << "VariableDeclaration";
        case ast::NodeType::SingleVariableDeclaration: return os << "SingleVariableDeclaration";
        case ast::NodeType::VariableDeclarationFragment: return os << "VariableDeclarationFragment";
        case ast::NodeType::Statement: return os << "Statement";
        case ast::NodeType::Block: return os << "Block";
        case ast::NodeType::EmptyStatement: return os << "EmptyStatement";
        case ast::NodeType::ExpressionStatement: return os << "ExpressionStatement";
        case ast::NodeType::ReturnStatement: return os << "ReturnStatement";
        case ast::NodeType::IfStatement: return os << "IfStatement";
        case ast::NodeType::WhileStatement: return os << "WhileStatement";
        case ast::NodeType::ForStatement: return os << "ForStatement";
        case ast::NodeType::VariableDeclarationStatement: return os << "VariableDeclarationStatement";
        case ast::NodeType::Expression: return os << "Expression";
        case ast::NodeType::AssignmentExpression: return os << "AssignmentExpression";
        case ast::NodeType::InfixExpression: return os << "InfixExpression";
        case ast::NodeType::InstanceofExpression: return os << "InstanceofExpression";
        case ast::NodeType::PrefixExpression: return os << "PrefixExpression";
        case ast::NodeType::CastExpression: return os << "CastExpression";
        case ast::NodeType::VariableDeclarationExpression: return os << "VariableDeclarationExpression";
        case ast::NodeType::Name: return os << "Name";
        case ast::NodeType::SimpleName: return os << "SimpleName";
        case ast::NodeType::QualifiedName: return os << "QualifiedName";
        case ast::NodeType::Modifier: return os << "Modifier";
        case ast::NodeType::PrimitiveType: return os << "PrimitiveType";
        case ast::NodeType::ClassOrInterfaceType: return os << "ClassOrInterfaceType";
        case ast::NodeType::ArrayType: return os << "ArrayType";
        case ast::NodeType::ArrayCreationExpression: return os << "ArrayCreationExpression";
        case ast::NodeType::IntegerLiteral: return os << "IntegerLiteral";
        case ast::NodeType::BooleanLiteral: return os << "BooleanLiteral";
        case ast::NodeType::CharacterLiteral: return os << "CharacterLiteral";
        case ast::NodeType::StringLiteral: return os << "StringLiteral";
        case ast::NodeType::NullLiteral: return os << "NullLiteral";
        case ast::NodeType::ThisExpression: return os << "ThisExpression";
        case ast::NodeType::ParenthesizedExpression: return os << "ParenthesizedExpression";
        case ast::NodeType::FieldAccessExpression: return os << "FieldAccessExpression";
        case ast::NodeType::MethodInvocationExpression: return os << "MethodInvocationExpression";
        case ast::NodeType::InstanceCreationExpression: return os << "InstanceCreationExpression";
        case ast::NodeType::ArrayAccessExpression: return os << "ArrayAccessExpression";
        default: return os << "UnknownNodeType (" << static_cast<int>(nodeType) << ")";
    }
}

std::ostream& operator<<(std::ostream &os, const ast::ModifierNode::Kind &modifierKind) {
    switch (modifierKind) {
        case ast::ModifierNode::Kind::Public: return os << "public";
        case ast::ModifierNode::Kind::Protected: return os << "protected";
        case ast::ModifierNode::Kind::Static: return os << "static";
        case ast::ModifierNode::Kind::Abstract: return os << "abstract";
        case ast::ModifierNode::Kind::Final: return os << "final";
        case ast::ModifierNode::Kind::Native: return os << "native";
        default: return os << "UnknownModifierKind (" << static_cast<int>(modifierKind) << ")";
    }
}

std::ostream& operator<<(std::ostream &os, const ast::PrimitiveTypeNode::Kind &primitiveTypeKind) {
    switch (primitiveTypeKind) {
        case ast::PrimitiveTypeNode::Kind::Byte: return os << "byte";
        case ast::PrimitiveTypeNode::Kind::Short: return os << "short";
        case ast::PrimitiveTypeNode::Kind::Int: return os << "int";
        case ast::PrimitiveTypeNode::Kind::Boolean: return os << "boolean";
        case ast::PrimitiveTypeNode::Kind::Char: return os << "char";
        default: return os << "UnknownPrimitiveTypeKind (" << static_cast<int>(primitiveTypeKind) << ")";
    }
}

std::ostream& operator<<(std::ostream &os, const ast::InfixExpressionNode::Operator &infixOperator) {
    switch (infixOperator) {
        case ast::InfixExpressionNode::Operator::Add: return os << "+";
        case ast::InfixExpressionNode::Operator::Subtract: return os << "-";
        case ast::InfixExpressionNode::Operator::Multiply: return os << "*";
        case ast::InfixExpressionNode::Operator::Divide: return os << "/";
        case ast::InfixExpressionNode::Operator::Remainder: return os << "%";
        case ast::InfixExpressionNode::Operator::Additive: return os << "(+ or -)";
        case ast::InfixExpressionNode::Operator::Multiplicative: return os << "(*, /, or %)";
        case ast::InfixExpressionNode::Operator::ConditionalAnd: return os << "&&";
        case ast::InfixExpressionNode::Operator::ConditionalOr: return os << "||";
        case ast::InfixExpressionNode::Operator::Equality: return os << "==";
        case ast::InfixExpressionNode::Operator::NotEqual: return os << "!=";
        case ast::InfixExpressionNode::Operator::Relational: return os << "(<, >, <=, or >=)";
        case ast::InfixExpressionNode::Operator::LessThan: return os << "<";
        case ast::InfixExpressionNode::Operator::GreaterThan: return os << ">";
        case ast::InfixExpressionNode::Operator::LessThanOrEqual: return os << "<=";
        case ast::InfixExpressionNode::Operator::GreaterThanOrEqual: return os << ">=";
        case ast::InfixExpressionNode::Operator::InclusiveOr: return os << "|";
        case ast::InfixExpressionNode::Operator::And: return os << "&";
        default: return os << "UnknownInfixOperator (" << static_cast<int>(infixOperator) << ")";
    }
}

std::ostream& operator<<(std::ostream &os, const ast::PrefixExpressionNode::Operator &prefixOperator) {
    switch (prefixOperator) {
        case ast::PrefixExpressionNode::Operator::Negate: return os << "- (prefix)";
        case ast::PrefixExpressionNode::Operator::Not: return os << "! (prefix)";
        default: return os << "UnknownPrefixOperator (" << static_cast<int>(prefixOperator) << ")";
    }
}

std::ostream& operator<<(std::ostream &os, const ast::NameNode::Variant &nameVariant) {
    switch (nameVariant) {
        case ast::NameNode::Variant::Package: return os << "Package";
        case ast::NameNode::Variant::Type: return os << "Type";
        case ast::NameNode::Variant::Expression: return os << "Expression";
        case ast::NameNode::Variant::Method: return os << "Method";
        case ast::NameNode::Variant::Ambiguous: return os << "Ambiguous";
        case ast::NameNode::Variant::RawIdentifier: return os << "RawIdentifier";
        default: return os << "UnknownNameVariant (" << static_cast<int>(nameVariant) << ")";
    }
}