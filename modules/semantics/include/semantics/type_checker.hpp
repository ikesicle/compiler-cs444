#ifndef SEMANTICS_TYPECHECKER_HPP
#define SEMANTICS_TYPECHECKER_HPP

#include "ast/ast_node.hpp"
#include "ast/ast_printer.hpp"
#include "ast/ast_visitor.hpp"
#include "ast/java_type.hpp"
#include "ast/nodes.hpp"
#include "ast/type_manager.hpp"
#include <unordered_map>

namespace semantics {

using namespace ast;

class TypeChecker: public ASTVisitor {
    TypeManager& typeManager;
    TypeDeclarationNode* currentType;
    MethodInfo currentMethod;
    std::unordered_map<VariableDeclarationFragmentNode*, JavaType*> localVarTypes; // For type checking variables
    
    void die(const std::string& message) {
        std::cerr << "Current type: " << currentType->resolvedType->name << "\n";
        if (currentMethod.returnType) {
            std::cerr << "Current method: " << currentMethod.returnType->name << " " << currentMethod.signature.toString() << "\n";
        } else {
            std::cerr << "Currently in a field." << "\n";
        }
        throw std::runtime_error(message);
    }
public:

    TypeChecker(TypeManager& tm): typeManager(tm) {}

    void run(CompilationUnitNode* compilationUnit) {
        currentType = nullptr;
        currentMethod = {};
        localVarTypes.clear();
        compilationUnit->accept(*this);
    }

    JavaType* resolveType(VariableDeclarationFragmentNode* varDecl) {
        if (localVarTypes.find(varDecl) != localVarTypes.end()) {
            return localVarTypes[varDecl];
        }
        switch (varDecl->parent->getNodeType()) {
            case NodeType::FieldDeclaration: {
                FieldDeclarationNode* fieldDecl = static_cast<FieldDeclarationNode*>(varDecl->parent);
                auto* classType = typeManager.getTypeFromNode(fieldDecl->type);
                localVarTypes[varDecl] = classType;
                return classType;
            }
            case NodeType::VariableDeclarationStatement: {
                VariableDeclarationStatementNode* varDeclStat = static_cast<VariableDeclarationStatementNode*>(varDecl->parent);
                auto* classType = typeManager.getTypeFromNode(varDeclStat->type);
                localVarTypes[varDecl] = classType;
                return classType;
                break;
            }
            case NodeType::VariableDeclarationExpression: {
                VariableDeclarationExpressionNode* varDeclExpr = static_cast<VariableDeclarationExpressionNode*>(varDecl->parent);
                auto* classType = typeManager.getTypeFromNode(varDeclExpr->type);
                localVarTypes[varDecl] = classType;
                return classType;
                break;
            }
            default:
                die("Variable declaration fragment has unrecognized parent node type");
        }
        return nullptr; // Unreachable, but silences compiler warning
    }

    JavaType* resolveType(SingleVariableDeclarationNode* paramDecl) {
        return typeManager.getTypeFromNode(paramDecl->type);
    }

    bool isNonStaticField(VariableDeclarationFragmentNode* varDecl) {
        switch (varDecl->parent->getNodeType()) {
            case NodeType::FieldDeclaration: {
                FieldDeclarationNode* fdn = static_cast<FieldDeclarationNode*>(varDecl->parent);
                JavaType* classType = typeManager.getTypeFromNode(fdn->type);
                bool isStatic = false;
                for (auto* modifier : fdn->modifiers) {
                    if (modifier->kind == ModifierNode::Static) {
                        isStatic = true;
                        break;
                    }
                }
                return !isStatic;
            }
            case NodeType::VariableDeclarationStatement:
            case NodeType::VariableDeclarationExpression: {
                return false; // Local variables can't be static
            }
            default:
                die("Variable declaration fragment has unrecognized parent node type");
        }
        return false; // Unreachable, but silences compiler warning
    }

    bool isAssignable(VariableDeclarationFragmentNode* varDecl) {
        switch (varDecl->parent->getNodeType()) {
            case NodeType::FieldDeclaration: {
                FieldDeclarationNode* fdn = static_cast<FieldDeclarationNode*>(varDecl->parent);
                JavaType* classType = typeManager.getTypeFromNode(fdn->type);
                bool isFinal = false;
                for (auto* modifier : fdn->modifiers) {
                    if (modifier->kind == ModifierNode::Final) {
                        isFinal = true;
                        break;
                    }
                }
                return !isFinal;
            }
            case NodeType::VariableDeclarationStatement:
            case NodeType::VariableDeclarationExpression: {
                return true; // Local variables are always assignable
            }
            default:
                die("Variable declaration fragment has unrecognized parent node type");
        }
        return false; // Unreachable, but silences compiler warning
    }

    bool visit(ImportDeclarationNode* node) override {
        return false; // Don't need to search inside import declarations
    }

    bool visit(PackageDeclarationNode* node) override {
        return false; // Don't need to search inside package declarations
    }

    bool visit(TypeDeclarationNode* node) override;
    void leave(TypeDeclarationNode* node) override;
    bool visit(MethodDeclarationNode* node) override;
    void leave(MethodDeclarationNode* node) override;
    bool visit(ConstructorDeclarationNode* node) override;
    void leave(ConstructorDeclarationNode* node) override;
    bool visit(FieldDeclarationNode* node) override;
    void leave(FieldDeclarationNode* node) override;
    void leave(IntegerLiteralNode* node) override;
    void leave(BooleanLiteralNode* node) override;
    void leave(StringLiteralNode* node) override;
    void leave(CharacterLiteralNode* node) override;
    void leave(NullLiteralNode* node) override;
    // Negating a bool leaves a bool
    void leave(PrefixExpressionNode* node) override;
    // Handles +, -, *, /, %, and also the comparison ops (also string concat)
    void leave(InfixExpressionNode* node) override;
    bool visit(SimpleNameNode* node) override;
    void leave(SimpleNameNode* node) override;
    bool visit(QualifiedNameNode* node) override;
    void leave(QualifiedNameNode* node) override;
    bool visit(VariableDeclarationFragmentNode* node) override;
    void leave(AssignmentExpressionNode* node) override;
    void leave(MethodInvocationExpressionNode* node) override;
    bool visit(FieldAccessExpressionNode* node) override;
    void leave(FieldAccessExpressionNode* node) override;
    void leave(CastExpressionNode* node) override;
    void leave(InstanceofExpressionNode* node) override;
    void leave(ParenthesizedExpressionNode* node) override;
    void leave(ReturnStatementNode* node) override;
    void leave(IfStatementNode* node) override;
    void leave(WhileStatementNode* node) override;
    void leave(ForStatementNode* node) override;
    void leave(ArrayAccessExpressionNode* node) override;
    void leave(InstanceCreationExpressionNode* node) override;
    void leave(ThisExpressionNode* node) override;
    void leave(ArrayCreationExpressionNode* node) override;
    void leave(SingleVariableDeclarationNode* node) override;
    void leave(VariableDeclarationStatementNode* node) override;
    void leave(VariableDeclarationExpressionNode* node) override;

};

}

#endif // SEMANTICS_TYPECHECKER_HPP