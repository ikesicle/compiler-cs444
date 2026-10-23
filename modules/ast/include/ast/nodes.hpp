#ifndef AST_NODES_HPP   
#define AST_NODES_HPP

#include "ast_node.hpp"
#include "java_type.hpp"
#include <ostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace ast {
// ============================================================================
// Forward Declarations
// ============================================================================

class CompilationUnitNode;
class PackageDeclarationNode;
class ImportDeclarationNode;
class TypeDeclarationNode;
class BodyDeclarationNode;
class FieldDeclarationNode;
class MethodDeclarationNode;
class ConstructorDeclarationNode;
class VariableDeclarationNode;
class SingleVariableDeclarationNode;
class VariableDeclarationFragmentNode;
class StatementNode;
class BlockNode;
class EmptyStatementNode;
class ExpressionStatementNode;
class ReturnStatementNode;
class IfStatementNode;
class WhileStatementNode;
class ForStatementNode;
class ExpressionNode;
class AssignmentExpressionNode;
class InfixExpressionNode;
class InstanceofExpressionNode;
class PrefixExpressionNode;
class CastExpressionNode;
class ArrayCreationExpressionNode;
class IntegerLiteralNode;
class BooleanLiteralNode;
class CharacterLiteralNode;
class StringLiteralNode;
class NullLiteralNode;
class ThisExpressionNode;
class ParenthesizedExpressionNode;
class FieldAccessExpressionNode;
class MethodInvocationExpressionNode;
class InstanceCreationExpressionNode;
class ArrayAccessExpressionNode;
class NameNode;
class SimpleNameNode;
class QualifiedNameNode;
class TypeNode;
class PrimitiveTypeNode;
class ClassOrInterfaceTypeNode;
class ArrayTypeNode;
class ModifierNode;

// ============================================================================
// Compilation Unit and Import Declarations
// ============================================================================

struct TypeImportInfo {
    bool isWildcard;
    std::string fullName; // Only for non-wildcard imports
    TypeDeclarationNode* resolvedDeclaration = nullptr; // Resolved declaration for this import
};

class CompilationUnitNode : public ASTNode {
public:
    PackageDeclarationNode* packageDeclaration = nullptr;
    std::vector<ImportDeclarationNode*> importDeclarations;
    std::vector<TypeDeclarationNode*> typeDeclarations;
    std::unordered_map<std::string, TypeImportInfo> importedTypes; // Map from simple name to import info
    std::unordered_set<std::string> wildcardImports, singleImports; // Set of package names for wildcard imports

    explicit CompilationUnitNode() {  }
    NodeType getNodeType() const override { return NodeType::CompilationUnit; }
    void accept(ASTVisitor& visitor) override;
};

class PackageDeclarationNode : public ASTNode {
public:
    NameNode* name = nullptr;
    explicit PackageDeclarationNode() {  }
    NodeType getNodeType() const override { return NodeType::PackageDeclaration; }
    void accept(ASTVisitor& visitor) override;
};

class ImportDeclarationNode : public ASTNode {
public:
    NameNode* name;
    bool isWildcard; // Ends with *

    explicit ImportDeclarationNode() {  }
    NodeType getNodeType() const override { return NodeType::ImportDeclaration; }
    void accept(ASTVisitor& visitor) override;
};

class TypeDeclarationNode : public ASTNode {
public:
    std::vector<ModifierNode*> modifiers;
    bool isInterface; // true for interface, false for class
    SimpleNameNode* name;
    std::vector<ClassOrInterfaceTypeNode*> extends;
    std::vector<ClassOrInterfaceTypeNode*> implements;
    std::vector<BodyDeclarationNode*> bodyDeclarations;
    Scope* bodyScope; // Scope for body declarations, used for resolving member accesses
    JavaType* resolvedType = nullptr; // Resolved JavaType for this type declaration

    explicit TypeDeclarationNode() {  }
    NodeType getNodeType() const override { return NodeType::TypeDeclaration; }
    void accept(ASTVisitor& visitor) override;
};

class ModifierNode : public ASTNode {
public:
    enum Kind {
        Public,
        Protected,
        Static,
        Abstract,
        Final,
        Native
    };
    Kind kind;
    explicit ModifierNode() {  }
    NodeType getNodeType() const override { return NodeType::Modifier; }
    void accept(ASTVisitor& visitor) override;
};

class BodyDeclarationNode : public ASTNode {
public:
    std::vector<ModifierNode*> modifiers;
    explicit BodyDeclarationNode() {  }
    NodeType getNodeType() const override { return NodeType::BodyDeclaration; }
    void accept(ASTVisitor& visitor) override;
    bool isAbstract() const {
        for (ModifierNode* m : modifiers) {
            if (m->kind == ModifierNode::Kind::Abstract) return true;
        }
        return false;
    }
    bool isNative() const {
        for (ModifierNode* m : modifiers) {
            if (m->kind == ModifierNode::Kind::Native) return true;
        }
        return false;
    }
    bool isStatic() const {
        for (ModifierNode* m : modifiers) {
            if (m->kind == ModifierNode::Kind::Static) return true;
        }
        return false;
    }
    bool isPublic() const {
        for (ModifierNode* m : modifiers) {
            if (m->kind == ModifierNode::Kind::Public) return true;
        }
        return false;
    }
    bool isProtected() const {
        for (ModifierNode* m : modifiers) {
            if (m->kind == ModifierNode::Kind::Protected) return true;
        }
        return false;
    }
    bool isFinal() const {
        for (ModifierNode* m : modifiers) {
            if (m->kind == ModifierNode::Kind::Final) return true;
        }
        return false;
    }
};

class FieldDeclarationNode : public BodyDeclarationNode {
public:
    TypeNode* type;
    std::vector<VariableDeclarationFragmentNode*> fragments;

    explicit FieldDeclarationNode() {  }
    NodeType getNodeType() const override { return NodeType::FieldDeclaration; }
    void accept(ASTVisitor& visitor) override;
};

class MethodDeclarationNode : public BodyDeclarationNode {
public:
    TypeNode* returnType; // null if return type is void
    SimpleNameNode* name;
    std::vector<SingleVariableDeclarationNode*> parameters;
    BlockNode* body; // null if method is bodyless (e.g., abstract method)

    explicit MethodDeclarationNode() {  }
    NodeType getNodeType() const override { return NodeType::MethodDeclaration; }
    void accept(ASTVisitor& visitor) override;
};

class ConstructorDeclarationNode : public BodyDeclarationNode {
public:
    SimpleNameNode* name;
    std::vector<SingleVariableDeclarationNode*> parameters;
    BlockNode* body; // NOTE: constructor body cannot be null

    explicit ConstructorDeclarationNode() {  }
    NodeType getNodeType() const override { return NodeType::ConstructorDeclaration; }
    void accept(ASTVisitor& visitor) override;
};

class VariableDeclarationNode : public ASTNode {
public:
    SimpleNameNode* name;
    ExpressionNode* initializer; // null if no initializer
    explicit VariableDeclarationNode() {  }
    NodeType getNodeType() const override { return NodeType::VariableDeclaration; }
    void accept(ASTVisitor& visitor) override;
};

class SingleVariableDeclarationNode : public VariableDeclarationNode {
public:
    TypeNode* type;

    explicit SingleVariableDeclarationNode() {  }
    NodeType getNodeType() const override { return NodeType::SingleVariableDeclaration; }
    void accept(ASTVisitor& visitor) override;
};

class VariableDeclarationFragmentNode : public VariableDeclarationNode {
public:
    explicit VariableDeclarationFragmentNode() {  }
    NodeType getNodeType() const override { return NodeType::VariableDeclarationFragment; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================================
// Statements
// ============================================================================

class StatementNode : public ASTNode {
public:
    explicit StatementNode() {  }
    NodeType getNodeType() const override { return NodeType::Statement; }
    void accept(ASTVisitor& visitor) override;
};

class BlockNode : public StatementNode {
public:
    std::vector<StatementNode*> statements;
    explicit BlockNode() {  }
    NodeType getNodeType() const override { return NodeType::Block; }
    void accept(ASTVisitor& visitor) override;
};

class EmptyStatementNode : public StatementNode {
public:
    explicit EmptyStatementNode() {  }
    NodeType getNodeType() const override { return NodeType::EmptyStatement; }
    void accept(ASTVisitor& visitor) override;
};

class ExpressionStatementNode : public StatementNode {
public:
    ExpressionNode* expression;
    explicit ExpressionStatementNode() {  }
    NodeType getNodeType() const override { return NodeType::ExpressionStatement; }
    void accept(ASTVisitor& visitor) override;
};

class ReturnStatementNode : public StatementNode {
public:
    ExpressionNode* expression;
    explicit ReturnStatementNode() {  }
    NodeType getNodeType() const override { return NodeType::ReturnStatement; }
    void accept(ASTVisitor& visitor) override;
};

class IfStatementNode : public StatementNode {
public:
    ExpressionNode* condition;
    StatementNode* thenBranch;
    StatementNode* elseBranch; // null if no else branch

    explicit IfStatementNode() {  }
    NodeType getNodeType() const override { return NodeType::IfStatement; }
    void accept(ASTVisitor& visitor) override;
};

class WhileStatementNode : public StatementNode {
public:
    ExpressionNode* condition;
    StatementNode* body;
    explicit WhileStatementNode() {  }
    NodeType getNodeType() const override { return NodeType::WhileStatement; }
    void accept(ASTVisitor& visitor) override;
};

class ForStatementNode : public StatementNode {
public:
    std::vector<ExpressionNode*> initializers;
    ExpressionNode* condition; // null if no condition
    std::vector<ExpressionNode*> updates; 
    StatementNode* body;
    explicit ForStatementNode() {  }
    NodeType getNodeType() const override { return NodeType::ForStatement; }
    void accept(ASTVisitor& visitor) override;
};


class VariableDeclarationStatementNode : public StatementNode {
public:
    TypeNode* type;
    std::vector<VariableDeclarationFragmentNode*> fragments;
    explicit VariableDeclarationStatementNode() {  }
    NodeType getNodeType() const override { return NodeType::VariableDeclarationStatement; }
    void accept(ASTVisitor& visitor) override;
};


// ============================================================================
// Expressions
// ============================================================================

class ExpressionNode : public ASTNode {
public:
    JavaType* resolvedType = nullptr; // Resolved type of this expression, set during type checking
    bool assignable = false;
    bool isLValue = false; // Whether this expression can appear on the left-hand side of an assignment
    explicit ExpressionNode() {  }
    NodeType getNodeType() const override { return NodeType::Expression; }
    void accept(ASTVisitor& visitor) override;
};

class AssignmentExpressionNode : public ExpressionNode {
public:
    ExpressionNode* lhs;
    ExpressionNode* rhs;

    explicit AssignmentExpressionNode() {  }
    NodeType getNodeType() const override { return NodeType::AssignmentExpression; }
    void accept(ASTVisitor& visitor) override;
};

class InfixExpressionNode : public ExpressionNode {
public:
    enum Operator {
        ConditionalOr,
        ConditionalAnd,
        InclusiveOr,
        And,
        Equality,
        NotEqual,

        Relational,
        LessThan,
        GreaterThan,
        LessThanOrEqual,
        GreaterThanOrEqual,

        Additive,
        Add,
        Subtract,

        Multiplicative,
        Multiply,
        Divide,
        Remainder
    };
    ExpressionNode* lhs;
    Operator op;
    ExpressionNode* rhs;
    explicit InfixExpressionNode() {  }
    NodeType getNodeType() const override { return NodeType::InfixExpression; }
    void accept(ASTVisitor& visitor) override;
};

class InstanceofExpressionNode : public ExpressionNode {
public:
    ExpressionNode* expression;
    TypeNode* type;
    JavaType* resolvedCheckType = nullptr; // Resolved type of the instanceof check, set during type checking
    explicit InstanceofExpressionNode() {  }
    NodeType getNodeType() const override { return NodeType::InstanceofExpression; }
    void accept(ASTVisitor& visitor) override;
};

class PrefixExpressionNode : public ExpressionNode {
public:
    enum Operator {
        Negate, // -
        Not // !
    };
    Operator op;
    ExpressionNode* operand;
    explicit PrefixExpressionNode() {  }
    NodeType getNodeType() const override { return NodeType::PrefixExpression; }
    void accept(ASTVisitor& visitor) override;
};

class CastExpressionNode : public ExpressionNode {
public:
    TypeNode* type;
    ExpressionNode* expression;
    explicit CastExpressionNode() {  }
    NodeType getNodeType() const override { return NodeType::CastExpression; }
    void accept(ASTVisitor& visitor) override;
};

class VariableDeclarationExpressionNode : public ExpressionNode {
public:
    TypeNode* type;
    std::vector<VariableDeclarationFragmentNode*> fragments;
    explicit VariableDeclarationExpressionNode() {  }
    NodeType getNodeType() const override { return NodeType::VariableDeclarationExpression; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================================
// Types
// ============================================================================

class TypeNode : public ASTNode {
public:
    explicit TypeNode() {  }
    NodeType getNodeType() const override { return NodeType::Type; }
    void accept(ASTVisitor& visitor) override;
};

class PrimitiveTypeNode : public TypeNode {
public:
    enum Kind {
        Boolean,
        Byte,
        Short,
        Int,
        Char
    };
    Kind kind;
    explicit PrimitiveTypeNode() {  }
    NodeType getNodeType() const override { return NodeType::PrimitiveType; }
    void accept(ASTVisitor& visitor) override;
};

class ClassOrInterfaceTypeNode : public TypeNode {
public:
    NameNode* name; // Simple or Qualified name
    TypeDeclarationNode* resolvedDeclaration = nullptr;
    explicit ClassOrInterfaceTypeNode() {  }
    NodeType getNodeType() const override { return NodeType::ClassOrInterfaceType; }
    void accept(ASTVisitor& visitor) override;
};

class ArrayTypeNode : public TypeNode {
public:
    TypeNode* componentType; // Can't do 2D arrays, so if there's an ArrayTypeNode here it's an error
    explicit ArrayTypeNode() {  }
    NodeType getNodeType() const override { return NodeType::ArrayType; }
    void accept(ASTVisitor& visitor) override;
};

// ============================================================================
// Literals and Other Expressions
// ============================================================================

class ArrayCreationExpressionNode : public ExpressionNode {
public:
    TypeNode* type;
    ExpressionNode* size;
    explicit ArrayCreationExpressionNode() {  }
    NodeType getNodeType() const override { return NodeType::ArrayCreationExpression; }
    void accept(ASTVisitor& visitor) override;
};

class IntegerLiteralNode : public ExpressionNode {
public:
    std::string rawValue; // The original string value from the source code, since we need to handle the negative sign being out of range.
    int value;
    explicit IntegerLiteralNode() {  }
    NodeType getNodeType() const override { return NodeType::IntegerLiteral; }
    void accept(ASTVisitor& visitor) override;
};

class BooleanLiteralNode : public ExpressionNode {
public:
    bool value;
    explicit BooleanLiteralNode() {  }
    NodeType getNodeType() const override { return NodeType::BooleanLiteral; }
    void accept(ASTVisitor& visitor) override;
};

class CharacterLiteralNode : public ExpressionNode {
public:
    char value;
    explicit CharacterLiteralNode() {  }
    NodeType getNodeType() const override { return NodeType::CharacterLiteral; }
    void accept(ASTVisitor& visitor) override;
};

class StringLiteralNode : public ExpressionNode {
public:
    std::vector<unsigned char> value;
    explicit StringLiteralNode() {  }
    NodeType getNodeType() const override { return NodeType::StringLiteral; }
    void accept(ASTVisitor& visitor) override;
};

class NullLiteralNode : public ExpressionNode {
public:
    explicit NullLiteralNode() {  }
    NodeType getNodeType() const override { return NodeType::NullLiteral; }
    void accept(ASTVisitor& visitor) override;
};

class ThisExpressionNode : public ExpressionNode {
public:
    explicit ThisExpressionNode() {  }
    NodeType getNodeType() const override { return NodeType::ThisExpression; }
    void accept(ASTVisitor& visitor) override;
};

class ParenthesizedExpressionNode : public ExpressionNode {
public:
    ExpressionNode* expression;
    explicit ParenthesizedExpressionNode() {  }
    NodeType getNodeType() const override { return NodeType::ParenthesizedExpression; }
    void accept(ASTVisitor& visitor) override;
};

class FieldAccessExpressionNode : public ExpressionNode {
public:
    ExpressionNode* expression; // null if simple name
    SimpleNameNode* fieldName;
    
    explicit FieldAccessExpressionNode() { assignable = true; isLValue = true; }
    NodeType getNodeType() const override { return NodeType::FieldAccessExpression; }
    void accept(ASTVisitor& visitor) override;
};

class MethodInvocationExpressionNode : public ExpressionNode {
public:
    ExpressionNode* expression; // null if simple name
    SimpleNameNode* methodName;
    MethodDeclarationNode* resolvedDeclaration = nullptr;
    std::vector<ExpressionNode*> arguments;

    explicit MethodInvocationExpressionNode() {  }
    NodeType getNodeType() const override { return NodeType::MethodInvocationExpression; }
    void accept(ASTVisitor& visitor) override;
};

class InstanceCreationExpressionNode : public ExpressionNode {
public:
    ClassOrInterfaceTypeNode* type;
    std::vector<ExpressionNode*> arguments;

    explicit InstanceCreationExpressionNode() {  }
    NodeType getNodeType() const override { return NodeType::InstanceCreationExpression; }
    void accept(ASTVisitor& visitor) override;
};

class ArrayAccessExpressionNode : public ExpressionNode {
public:
    ExpressionNode* array; // The array being accessed
    ExpressionNode* index;
    explicit ArrayAccessExpressionNode() { assignable = true; isLValue = true; }
    NodeType getNodeType() const override { return NodeType::ArrayAccessExpression; }
    void accept(ASTVisitor& visitor) override;
};

class NameNode : public ExpressionNode {
public:
    enum Variant {
        Package,
        Type,
        Expression,
        Method,
        Ambiguous,
        RawIdentifier // In declarations
    };
    ASTNode* resolvedReference = nullptr;
    Variant variant = Ambiguous; // Determined during name resolution, or optionally passed in
    
    explicit NameNode() { assignable = true; }
    NodeType getNodeType() const override { return NodeType::Name; }
    void accept(ASTVisitor& visitor) override;
    virtual std::string toString() const { throw std::runtime_error("toString not implemented for base NameNode"); }
    virtual std::string getSimpleName() const { throw std::runtime_error("getSimpleName not implemented for base NameNode"); }
    virtual std::string getQualifierString() const { throw std::runtime_error("getQualifierString not implemented for base NameNode"); }
};

class SimpleNameNode : public NameNode {
public:
    std::string value;
    explicit SimpleNameNode() { assignable = true; isLValue = true; }
    NodeType getNodeType() const override { return NodeType::SimpleName; }
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override  { return value; }
    std::string getSimpleName() const override { return value; }
    std::string getQualifierString() const override { return ""; }
};

class QualifiedNameNode : public NameNode {
public:
    NameNode* qualifier; // null if simple name
    SimpleNameNode* name;
    explicit QualifiedNameNode() { assignable = true; isLValue = true; }
    NodeType getNodeType() const override { return NodeType::QualifiedName; }
    void accept(ASTVisitor& visitor) override;
    std::string toString() const override {
        if (qualifier) {
            return qualifier->toString() + "." + name->toString();
        } else {
            return name->toString();
        }
    }
    std::string getSimpleName() const override { return name->getSimpleName(); }
    std::string getQualifierString() const override { return qualifier ? qualifier->toString() : ""; }
};

}  // namespace ast

std::ostream& operator<<(std::ostream &os, const ast::NodeType &nodeType);

std::ostream& operator<<(std::ostream &os, const ast::ModifierNode::Kind &modifierKind);

std::ostream& operator<<(std::ostream &os, const ast::PrimitiveTypeNode::Kind &primitiveTypeKind);

std::ostream& operator<<(std::ostream &os, const ast::InfixExpressionNode::Operator &infixOperator);

std::ostream& operator<<(std::ostream &os, const ast::PrefixExpressionNode::Operator &prefixOperator);

std::ostream& operator<<(std::ostream &os, const ast::NameNode::Variant &nameVariant);

#endif  // AST_NODES_HPP
