#ifndef REACHABILITY_REACHABILITY_HPP
#define REACHABILITY_REACHABILITY_HPP

#include "ast/ast_visitor.hpp"
#include "ast/nodes.hpp"
#include <unordered_map>

namespace reachability {

using namespace ast;
    
class ReachabilityVisitor : public ast::ASTVisitor {
private:
    std::unordered_map<ASTNode*, bool> canStart;
    std::unordered_map<ASTNode*, bool> canEnd;
    
    const static bool No = false;
    const static bool Maybe = true;

    typedef std::pair<bool, int> IntConstantValue;

    bool isConstantTrue(ExpressionNode* expr);
    bool isConstantFalse(ExpressionNode* expr);
    IntConstantValue evaluateNumericConstant(ExpressionNode* expr);

public:
    
    ReachabilityVisitor() = default;

    // Top-down traversal (in)
    bool visit(MethodDeclarationNode* node) override;
    bool visit(ConstructorDeclarationNode* node) override;
    bool visit(BlockNode* node) override;
    bool visit(IfStatementNode* node) override;
    bool visit(WhileStatementNode* node) override;
    bool visit(ForStatementNode* node) override;

    // Bottom-up traversal (out) 
    void leave(EmptyStatementNode* node) override;
    void leave(ExpressionStatementNode* node) override;
    void leave(VariableDeclarationStatementNode* node) override;
    void leave(ReturnStatementNode* node) override;
};

} // namespace reachability

#endif // REACHABILITY_REACHABILITY_HPP