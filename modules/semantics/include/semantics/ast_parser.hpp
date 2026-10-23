#ifndef JOOSAST_ASTPARSER_HPP
#define JOOSAST_ASTPARSER_HPP

#include "common/parse_tree.hpp"
#include "ast/ast.hpp"
#include "ast/nodes.hpp"
#include "ast/scope.hpp"
#include "ast/tagged_scope.hpp"
#include "type_linker.hpp"
#include <string>
#include <vector>

namespace semantics {

using namespace ast;

class ASTParser {
    std::vector<AST> astUnits; // List of all parsed ASTs (one per compilation unit)
    AST* ast; // Reference to the current AST being constructed
    TaggedScope globalScope; // Global scope for symbol resolution during parsing
    Scope* currentScope = &globalScope; // Pointer to the current scope during parsing
    TypeLinker typeLinker{&globalScope}; // Type linker for resolving types during parsing

    CompilationUnitNode* parseCompilationUnit(const common::ParseTree& node);
    PackageDeclarationNode* parsePackageDeclaration(const common::ParseTree& node);
    ImportDeclarationNode* parseImportDeclaration(const common::ParseTree& node);
    TypeDeclarationNode* parseTypeDeclaration(const common::ParseTree& node);
    BodyDeclarationNode* parseBodyDeclaration(const common::ParseTree& node);
    FieldDeclarationNode* parseFieldDeclaration(const common::ParseTree& node);
    MethodDeclarationNode* parseMethodDeclaration(const common::ParseTree& node);
    ConstructorDeclarationNode* parseConstructorDeclaration(const common::ParseTree& node);
    SingleVariableDeclarationNode* parseSingleVariableDeclaration(const common::ParseTree& node);
    VariableDeclarationFragmentNode* parseVariableDeclarationFragment(const common::ParseTree& node, bool inMethod = false);

    StatementNode* parseStatement(const common::ParseTree& node, int& newScopeCount);
    StatementNode* parseStatementNoShortIf(const common::ParseTree& node);
    BlockNode* parseBlock(const common::ParseTree& node);
    EmptyStatementNode* parseEmptyStatement(const common::ParseTree& node);
    ExpressionStatementNode* parseExpressionStatement(const common::ParseTree& node);
    ReturnStatementNode* parseReturnStatement(const common::ParseTree& node);
    IfStatementNode* parseIfStatement(const common::ParseTree& node);
    WhileStatementNode* parseWhileStatement(const common::ParseTree& node);
    ForStatementNode* parseForStatement(const common::ParseTree& node);
    VariableDeclarationStatementNode* parseVariableDeclarationStatement(const common::ParseTree& node, int& newScopeCount);
    
    ExpressionNode* parseExpression(const common::ParseTree& node);
    AssignmentExpressionNode* parseAssignmentExpression(const common::ParseTree& node);
    ExpressionNode* parseInfixExpression(const common::ParseTree& node, InfixExpressionNode::Operator parentOp = InfixExpressionNode::ConditionalOr);
    InstanceofExpressionNode* parseInstanceofExpression(const common::ParseTree& node);
    ExpressionNode* parsePrefixExpression(const common::ParseTree& node, PrefixExpressionNode::Operator parentOp = PrefixExpressionNode::Negate);

    ExpressionNode* parsePrimaryExpression(const common::ParseTree& node);
    ExpressionNode* parsePrimaryNoArrayExpression(const common::ParseTree& node);

    CastExpressionNode* parseCastExpression(const common::ParseTree& node);
    VariableDeclarationExpressionNode* parseVariableDeclarationExpression(const common::ParseTree& node, int& newScopeCount);

    ArrayCreationExpressionNode* parseArrayCreationExpression(const common::ParseTree& node);
    IntegerLiteralNode* parseIntegerLiteral(const common::ParseTree& node);
    BooleanLiteralNode* parseBooleanLiteral(const common::ParseTree& node);
    CharacterLiteralNode* parseCharacterLiteral(const common::ParseTree& node);
    StringLiteralNode* parseStringLiteral(const common::ParseTree& node);
    NullLiteralNode* parseNullLiteral(const common::ParseTree& node);
    ThisExpressionNode* parseThisExpression(const common::ParseTree& node);
    FieldAccessExpressionNode* parseFieldAccessExpression(const common::ParseTree& node);
    MethodInvocationExpressionNode* parseMethodInvocationExpression(const common::ParseTree& node);
    InstanceCreationExpressionNode* parseInstanceCreationExpression(const common::ParseTree& node);
    ArrayAccessExpressionNode* parseArrayAccessExpression(const common::ParseTree& node);

    SimpleNameNode* parseSimpleName(const common::ParseTree& node);
    NameNode* parseName(const common::ParseTree& node, NameNode::Variant variant = NameNode::Ambiguous);
    PrimitiveTypeNode* parsePrimitiveType(const common::ParseTree& node);
    TypeNode* parseReferenceType(const common::ParseTree& node);
    ClassOrInterfaceTypeNode* parseClassOrInterfaceType(const common::ParseTree& node);
    ArrayTypeNode* parseArrayType(const common::ParseTree& node);
    TypeNode* parseType(const common::ParseTree& node);
    ModifierNode* parseModifier(const common::ParseTree& node);

public:

    const std::vector<CompilationUnitNode*> getASTRoots() const {
        std::vector<CompilationUnitNode*> roots;
        for (const auto& ast : astUnits) {
            if (!ast.getRoot() || ast.getRoot()->getNodeType() != NodeType::CompilationUnit) {
                throw std::runtime_error("Expected root of AST to be CompilationUnitNode");
            }
            roots.push_back(static_cast<CompilationUnitNode*>(ast.getRoot()));
        }
        return roots;
    }

    CompilationUnitNode* getMainRoot() const {
        if (astUnits.empty()) {
            throw std::runtime_error("No ASTs parsed");
        }
        if (!astUnits[0].getRoot() || astUnits[0].getRoot()->getNodeType() != NodeType::CompilationUnit) {
            throw std::runtime_error("Expected root of AST to be CompilationUnitNode");
        }
        return static_cast<CompilationUnitNode*>(astUnits[0].getRoot());
    }

    ASTParser() = default;
    AST& parseNew(const common::ParseTree& parseTreeRoot, std::string astName = "");
    Scope* enterScope() {
        // std::cerr << "Entering new scope from scope " << currentScope << "\n";
        currentScope = currentScope->createChild();
        return currentScope;
    }
    
    Scope* exitScope() {
        if (currentScope->parent) {
            currentScope = currentScope->parent;
            // std::cerr << "Exiting to parent scope: " << currentScope << "\n";
        } else {
            throw std::runtime_error("Attempted to exit global scope");
        }
        return currentScope;
    }

    Scope* enterNamedGlobalScope(const NameNode* nameNode) {
        if (nameNode == nullptr) {
            currentScope = globalScope.enterNamedChild(""); // Default unnamed global scope for classes without package
            return currentScope;
        }
         if (nameNode->getNodeType() == NodeType::QualifiedName) {
            const QualifiedNameNode* qualifiedNameNode = static_cast<const QualifiedNameNode*>(nameNode);
            if (globalScope.getChildByTag(qualifiedNameNode->qualifier->toString()) == nullptr) {
                // Automatically create package scope for qualifier if it doesn't exist
                enterNamedGlobalScope(qualifiedNameNode->qualifier);
            }
        }
        currentScope = globalScope.enterNamedChild(nameNode->toString());
        return currentScope;
    }

    TaggedScope* getGlobalScope() {
        return &globalScope;
    }

    TypeLinker& getTypeLinker() {
        return typeLinker;
    }

    void linkTypes() {
        // Package validation - No package names or prefixes of package names of declared packages, 
        // single-type-import declarations or import-on-demand declarations that are 
        // used may resolve to types, except for types in the default package.

        for (const auto& [packageName, packageScope] : globalScope.tags) {
            size_t lastDot = packageName.rfind('.');
            std::string qualifier = (lastDot != std::string::npos) ? packageName.substr(0, lastDot) : "";
            std::string simpleName = (lastDot != std::string::npos) ? packageName.substr(lastDot + 1) : packageName;
            if  (qualifier.empty()) continue; // Default package can have name conflicts with types, so we skip the check for it
            auto searchScope = globalScope.getChildByTag(qualifier);
            if (searchScope) {
                auto type = searchScope->lookupCurrentScope(simpleName, NodeType::TypeDeclaration);
                if (!type.empty()) {
                    throw std::runtime_error("Package name '" + packageName + "' conflicts with type '" + simpleName + "' in package '" + qualifier + "'");
                }
            }
        }

        for (auto& ast : astUnits) {
            // std::cerr << "Linking types for AST: " << ast.getName() << "\n";
            typeLinker.link(ast.getRoot());
        }
    }
};

}

#endif // SEMANTICS_ASTPARSER_HPP