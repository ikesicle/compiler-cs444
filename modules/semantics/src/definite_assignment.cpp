#include "semantics/definite_assignment.hpp"
#include "ast/ast_node.hpp"
#include "ast/nodes.hpp"
#include <stdexcept>

namespace semantics {

bool DefiniteAssignment::visit(SingleVariableDeclarationNode* node) {
    currentVarDeclContext = node;
    return true;
};

bool DefiniteAssignment::visit(VariableDeclarationStatementNode* node) {
    for (auto fragment : node->fragments) {
        currentVarDeclContext = fragment;
        fragment->accept(*this);
        currentVarDeclContext = nullptr;
    }
    return false;
};

bool DefiniteAssignment::visit(VariableDeclarationExpressionNode* node) {
    for (auto fragment : node->fragments) {
        currentVarDeclContext = fragment;
        fragment->accept(*this);
        currentVarDeclContext = nullptr;
    }
    return false;
};

bool DefiniteAssignment::visit(SimpleNameNode* node) {
    if (currentVarDeclContext == nullptr || node->variant != NameNode::Variant::Expression) {
        return true; // Not a variable reference, ignore
    }
    if (node->resolvedReference == currentVarDeclContext) {
        // This is a reference to the variable currently being declared, which is not definitely assigned
        throw std::runtime_error("Variable declaration might not have been initialized before use");
    }
    return true;
};
void DefiniteAssignment::leave(SingleVariableDeclarationNode* node) {
    currentVarDeclContext = nullptr;
};
void DefiniteAssignment::leave(VariableDeclarationStatementNode* node) {
    currentVarDeclContext = nullptr;
};
void DefiniteAssignment::leave(VariableDeclarationExpressionNode* node) {
    currentVarDeclContext = nullptr;
};

}