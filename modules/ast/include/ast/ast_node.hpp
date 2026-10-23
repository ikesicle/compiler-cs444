#ifndef AST_ASTNODE_HPP
#define AST_ASTNODE_HPP

#include <string>
namespace ast {

enum NodeType {
    CompilationUnit,
    PackageDeclaration,
    ImportDeclaration,
    TypeDeclaration, // Class/Interface, Body, Extend and Implements, Constructor

    BodyDeclaration,
    FieldDeclaration, // Modifiers, Type, VariableDeclartionFragments
    MethodDeclaration, // Modifiers, Type, Parameters, Body
    ConstructorDeclaration, // M
    VariableDeclaration,
    SingleVariableDeclaration,
    VariableDeclarationFragment,

    Statement, // Block, If, While, For, Return, ExpressionStatement
    Block, // List of Statements
    EmptyStatement,
    ExpressionStatement,
    ReturnStatement,
    IfStatement,
    WhileStatement,
    ForStatement,
    VariableDeclarationStatement,

    Expression,
    AssignmentExpression,
    InfixExpression, // Or, And, BitOr, BitAnd, Equality, Relational, Additive, Multiplicative
    InstanceofExpression,
    PrefixExpression, // -, !
    CastExpression,

    Type,
    PrimitiveType,
    ClassOrInterfaceType,
    ArrayType,

    ArrayCreationExpression,
    IntegerLiteral,
    BooleanLiteral,
    CharacterLiteral,
    StringLiteral,
    NullLiteral,
    ThisExpression,
    ParenthesizedExpression,
    FieldAccessExpression,
    MethodInvocationExpression,
    InstanceCreationExpression,
    ArrayAccessExpression,
    VariableDeclarationExpression,

    Name,
    SimpleName,
    QualifiedName,
    Modifier
};

static std::string nodeTypeToString(NodeType type) {
    std::string result;
    switch (type) {
        case CompilationUnit: result = "CompilationUnit"; break;
        case PackageDeclaration: result = "PackageDeclaration"; break;
        case ImportDeclaration: result = "ImportDeclaration"; break;
        case TypeDeclaration: result = "TypeDeclaration"; break;
        case BodyDeclaration: result = "BodyDeclaration"; break;
        case FieldDeclaration: result = "FieldDeclaration"; break;
        case MethodDeclaration: result = "MethodDeclaration"; break;
        case ConstructorDeclaration: result = "ConstructorDeclaration"; break;
        case VariableDeclaration: result = "VariableDeclaration"; break;
        case SingleVariableDeclaration: result = "SingleVariableDeclaration"; break;
        case VariableDeclarationFragment: result = "VariableDeclarationFragment"; break;
        case Statement: result = "Statement"; break;
        case Block: result = "Block"; break;
        case EmptyStatement: result = "EmptyStatement"; break;
        case ExpressionStatement: result = "ExpressionStatement"; break;
        case ReturnStatement: result = "ReturnStatement"; break;
        case IfStatement: result = "IfStatement"; break;
        case WhileStatement: result = "WhileStatement"; break;
        case ForStatement: result = "ForStatement"; break;
        case VariableDeclarationStatement: result = "VariableDeclarationStatement"; break;
        case Expression: result = "Expression"; break;
        case AssignmentExpression: result = "AssignmentExpression"; break;
        case InfixExpression: result = "InfixExpression"; break;
        case InstanceofExpression: result = "InstanceofExpression"; break;
        case PrefixExpression: result = "PrefixExpression"; break;
        case CastExpression: result = "CastExpression"; break;
        case Type: result = "Type"; break;
        case PrimitiveType: result = "PrimitiveType"; break;
        case ClassOrInterfaceType: result = "ClassOrInterfaceType"; break;
        case ArrayType: result = "ArrayType"; break;
        case ArrayCreationExpression: result = "ArrayCreationExpression"; break;
        case IntegerLiteral: result = "IntegerLiteral"; break;
        case BooleanLiteral: result = "BooleanLiteral"; break;
        case CharacterLiteral: result = "CharacterLiteral"; break;
        case StringLiteral: result = "StringLiteral"; break;
        case NullLiteral: result = "NullLiteral"; break;
        case ThisExpression: result = "ThisExpression"; break;
        case ParenthesizedExpression: result = "ParenthesizedExpression"; break;
        case FieldAccessExpression: result = "FieldAccessExpression"; break;
        case MethodInvocationExpression: result = "MethodInvocationExpression"; break;
        case InstanceCreationExpression: result = "InstanceCreationExpression"; break;
        case ArrayAccessExpression: result = "ArrayAccessExpression"; break;
        case VariableDeclarationExpression: result = "VariableDeclarationExpression"; break;
        case Name: result = "Name"; break;
        case SimpleName: result = "SimpleName"; break;
        case QualifiedName: result = "QualifiedName"; break;
        case Modifier: result = "Modifier"; break;
        default: result = "Unknown"; break;
    }
    return result;
}

class Scope; // Forward declaration

// AST Factory and Owner of all AST nodes. This class can be used to create and manage the lifecycle of AST nodes.
class ASTNode {
public:
    ASTNode* parent = nullptr;
    Scope* scope = nullptr;
    virtual ~ASTNode() = default;
    virtual NodeType getNodeType() const = 0;
    virtual void accept(class ASTVisitor& visitor) = 0;
};
    
}

#endif // SEMANTICS_ASTNODE_HPP