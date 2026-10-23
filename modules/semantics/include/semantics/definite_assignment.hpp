#ifndef SEMANTICS_DEFINITE_ASSIGNMENT_HPP
#define SEMANTICS_DEFINITE_ASSIGNMENT_HPP

#include "ast/ast_visitor.hpp"
#include "ast/nodes.hpp"
#include "ast/ast_node.hpp"

namespace semantics {

using namespace ast;
class DefiniteAssignment: public ASTVisitor {
    VariableDeclarationNode* currentVarDeclContext = nullptr;
public:
    bool visit(SingleVariableDeclarationNode* node) override;
    bool visit(VariableDeclarationStatementNode* node) override;
    bool visit(VariableDeclarationExpressionNode* node) override;
    bool visit(SimpleNameNode* node) override;
    void leave(SingleVariableDeclarationNode* node) override;
    void leave(VariableDeclarationStatementNode* node) override;
    void leave (VariableDeclarationExpressionNode* node) override;


};

}

#endif // SEMANTICS_DEFINITE_ASSIGNMENT_HPP