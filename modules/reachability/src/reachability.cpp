#include "reachability/reachability.hpp"
#include "ast/ast_printer.hpp"
#include "ast/nodes.hpp"
#include <iostream>

namespace reachability {

// --- Top-Down Traversals (Computing 'in' and controlling children) ---

bool ReachabilityVisitor::visit(MethodDeclarationNode* node) {
    if (node->body) {
        // Method body can start (in[L] = maybe)
        canStart[node->body] = Maybe;
        
        // Traverse the body
        node->body->accept(*this);
        
        // out[non-void method body] = maybe is an error
        bool isVoid = (node->returnType == nullptr); 
        if (!isVoid && canEnd[node->body] == Maybe) {
            throw std::runtime_error("Reachability Error: Non-void method might not return a value.");
        }
    }
    return false; // Stop automatic traversal; we manually visited the body
}

bool ReachabilityVisitor::visit(ConstructorDeclarationNode* node) {
    if (node->body) {
        // Constructor body can start (in[L] = maybe)
        canStart[node->body] = Maybe;
        
        // Traverse the body
        node->body->accept(*this);
    }
    return false; // Stop automatic traversal; we manually visited the body
}

bool ReachabilityVisitor::visit(BlockNode* node) {
    // For sequential statements L: { S1; S2; ... Sn }
    bool currentReachability = canStart[node];
    
    for (auto* stmt : node->statements) { // Assuming BlockNode has a 'statements' vector
        canStart[stmt] = currentReachability;
        
        if (canStart[stmt] == No) {
            stmt->accept(ASTPrinter::Instance); // Visit block itself first if needed (e.g., for variable declarations)
            throw std::runtime_error("Reachability Error: Unreachable code detected in block at this point");
        }
        stmt->accept(*this);
        currentReachability = canEnd[stmt]; // in[S2] = out[S1]
    }
    // std::cerr << "ending block with current reachability: " << (currentReachability ? "Maybe" : "No") << "\n";
    canEnd[node] = currentReachability; // out[L] = out[Sn]
    return false; // Children manually visited
}

bool ReachabilityVisitor::visit(IfStatementNode* node) {
    // We assume the expression can evaluate to either true or false
    if (node->thenBranch) {
        canStart[node->thenBranch] = canStart[node];
        node->thenBranch->accept(*this);
    }
    
    if (node->elseBranch) {
        // For statement L: if(E) S1 else S2
        canStart[node->elseBranch] = canStart[node];
        node->elseBranch->accept(*this);
        
        bool thenEnd = node->thenBranch ? canEnd[node->thenBranch] : canStart[node];
        canEnd[node] = thenEnd || canEnd[node->elseBranch];
    } else {
        // For statement L: if(E) S
        bool thenEnd = node->thenBranch ? canEnd[node->thenBranch] : canStart[node];
        canEnd[node] = canStart[node] || thenEnd;
    }
    return false; 
}

bool ReachabilityVisitor::visit(WhileStatementNode* node) {
    bool constTrue = isConstantTrue(node->condition);
    bool constFalse = isConstantFalse(node->condition);

    if (constFalse) {
        canStart[node->body] = No;
        throw std::runtime_error("Reachability Error: Unreachable code in while body (condition is constant false).");
    } else {
        canStart[node->body] = canStart[node];
    }

    if (canStart[node->body] == Maybe) {
        node->body->accept(*this);
    }

    if (constTrue) {
        canEnd[node] = No;
    } else if (constFalse) {
        canEnd[node] = canStart[node];
    } else {
        canEnd[node] = canStart[node] || canEnd[node->body];
    }

    return false;
}

bool ReachabilityVisitor::visit(ForStatementNode* node) {
    // JLS rules for 'for' loops are nearly identical to 'while' loops regarding conditions.
    bool constTrue = (node->condition == nullptr) || isConstantTrue(node->condition);
    bool constFalse = node->condition != nullptr && isConstantFalse(node->condition);

    if (constFalse && node->body) {
        canStart[node->body] = No;
        throw std::runtime_error("Reachability Error: Unreachable code in for loop body.");
    } else if (node->body) {
        canStart[node->body] = canStart[node];
        node->body->accept(*this);
    }

    if (constTrue) {
        canEnd[node] = No;
    } else if (constFalse) {
        canEnd[node] = canStart[node];
    } else {
        canEnd[node] = canStart[node] || (node->body ? canEnd[node->body] : Maybe);
    }

    return false;
}

// --- Bottom-Up Traversals (Computing 'out' for simple statements) 

void ReachabilityVisitor::leave(EmptyStatementNode* node) {
    canEnd[node] = canStart[node];
}

void ReachabilityVisitor::leave(ExpressionStatementNode* node) {
    canEnd[node] = canStart[node];
}

void ReachabilityVisitor::leave(VariableDeclarationStatementNode* node) {
    canEnd[node] = canStart[node];
}

void ReachabilityVisitor::leave(ReturnStatementNode* node) {
    // out[return] = no; out[return E] = no;
    canEnd[node] = No;
}


// --- Constant Expression Evaluation Helpers ---

bool ReachabilityVisitor::isConstantTrue(ExpressionNode* expr) {
    if (!expr) return false;
    switch(expr->getNodeType()) {
        case NodeType::BooleanLiteral:
            return static_cast<BooleanLiteralNode*>(expr)->value == true;
        case NodeType::InfixExpression: {
            auto* infix = static_cast<InfixExpressionNode*>(expr);
            if (infix->op == InfixExpressionNode::ConditionalAnd) {
                return isConstantTrue(infix->lhs) && isConstantTrue(infix->rhs);
            } else if (infix->op == InfixExpressionNode::ConditionalOr) {
                return isConstantTrue(infix->lhs) || isConstantTrue(infix->rhs);
            } else if (infix->op == InfixExpressionNode::Equality) {
                if (infix->lhs->resolvedType == &JavaType::Boolean && infix->rhs->resolvedType == &JavaType::Boolean) {
                    return isConstantTrue(infix->lhs) == isConstantTrue(infix->rhs);
                }
                if (infix->lhs->resolvedType == &JavaType::Int && infix->rhs->resolvedType == &JavaType::Int) {
                    IntConstantValue leftVal = evaluateNumericConstant(infix->lhs);
                    IntConstantValue rightVal = evaluateNumericConstant(infix->rhs);
                    if (leftVal.first && rightVal.first) {
                        return leftVal.second == rightVal.second;
                    }
                }
                return false;
            } else {
                return false;
            }
        }
        case NodeType::PrefixExpression: {
            auto* prefix = static_cast<PrefixExpressionNode*>(expr);
            if (prefix->op == PrefixExpressionNode::Not) {
                return isConstantFalse(prefix->operand);
            } else {
                return false;
            }
        }
        case NodeType::ParenthesizedExpression: {
            auto* parenExpr = static_cast<ParenthesizedExpressionNode*>(expr);
            return isConstantTrue(parenExpr->expression);
        }
        case NodeType::AssignmentExpression: {
            auto* assignExpr = static_cast<AssignmentExpressionNode*>(expr);
            return isConstantTrue(assignExpr->rhs);
        }

        default:
            return false;
    }
}

bool ReachabilityVisitor::isConstantFalse(ExpressionNode* expr) {
    if (!expr) return false;
    switch(expr->getNodeType()) {
        case NodeType::BooleanLiteral:
            return static_cast<BooleanLiteralNode*>(expr)->value == false;
        case NodeType::InfixExpression: {
            auto* infix = static_cast<InfixExpressionNode*>(expr);
            bool equalityReturn = false;
            switch (infix->op) {
                case InfixExpressionNode::ConditionalAnd:
                    return isConstantFalse(infix->lhs) || isConstantFalse(infix->rhs);
                case InfixExpressionNode::ConditionalOr:
                    return isConstantFalse(infix->lhs) && isConstantFalse(infix->rhs);
                case InfixExpressionNode::Equality:
                    if (infix->lhs->resolvedType == &JavaType::Boolean && infix->rhs->resolvedType == &JavaType::Boolean) {
                        return
                            (isConstantFalse(infix->lhs) && isConstantTrue(infix->rhs)) ||
                            (isConstantTrue(infix->lhs) && isConstantFalse(infix->rhs));
                    }
                    if (infix->lhs->resolvedType == &JavaType::Int && infix->rhs->resolvedType == &JavaType::Int) {
                        IntConstantValue leftVal = evaluateNumericConstant(infix->lhs);
                        IntConstantValue rightVal = evaluateNumericConstant(infix->rhs);
                        return (leftVal.first && rightVal.first) && (leftVal.second != rightVal.second);
                    }
                    return false;
                case InfixExpressionNode::NotEqual:
                    if (infix->lhs->resolvedType == &JavaType::Boolean && infix->rhs->resolvedType == &JavaType::Boolean) {
                        return
                            (isConstantFalse(infix->lhs) && isConstantFalse(infix->rhs)) ||
                            (isConstantTrue(infix->lhs) && isConstantTrue(infix->rhs));
                    }
                    if (infix->lhs->resolvedType == &JavaType::Int && infix->rhs->resolvedType == &JavaType::Int) {
                        IntConstantValue leftVal = evaluateNumericConstant(infix->lhs);
                        IntConstantValue rightVal = evaluateNumericConstant(infix->rhs);
                        return (leftVal.first && rightVal.first) && (leftVal.second == rightVal.second);
                    }
                    return false;
                default:
                    return false;
            }
        }
        case NodeType::PrefixExpression: {
            auto* prefix = static_cast<PrefixExpressionNode*>(expr);
            if (prefix->op == PrefixExpressionNode::Not) {
                return isConstantTrue(prefix->operand);
            } else {
                return false;
            }
        }
        case NodeType::ParenthesizedExpression: {
            auto* parenExpr = static_cast<ParenthesizedExpressionNode*>(expr);
            return isConstantFalse(parenExpr->expression);
        }
        case NodeType::AssignmentExpression: {
            auto* assignExpr = static_cast<AssignmentExpressionNode*>(expr);
            return isConstantFalse(assignExpr->rhs);
        }

        default:
            return false;
    }
}

ReachabilityVisitor::IntConstantValue ReachabilityVisitor::evaluateNumericConstant(ExpressionNode* expr) {
    if (!expr) return {false, 0};
    switch (expr->getNodeType()) {
        case NodeType::IntegerLiteral: {
            auto* intLit = static_cast<IntegerLiteralNode*>(expr);
            return {true, intLit->value};
        }
        case NodeType::InfixExpression: {
            auto* infix = static_cast<InfixExpressionNode*>(expr);
            auto leftVal = evaluateNumericConstant(infix->lhs);
            auto rightVal = evaluateNumericConstant(infix->rhs);
            switch (infix->op) {
                case InfixExpressionNode::Add:
                    return {leftVal.first && rightVal.first, leftVal.second + rightVal.second};
                    break;
                case InfixExpressionNode::Subtract:
                    return {leftVal.first && rightVal.first, leftVal.second - rightVal.second};
                    break;
                case InfixExpressionNode::Multiply:
                    return {leftVal.first && rightVal.first, leftVal.second * rightVal.second};
                    break;
                case InfixExpressionNode::Divide:
                    if (rightVal.first && rightVal.second != 0) {
                        return {true, leftVal.second / rightVal.second};
                    } else {
                        return {false, 0};
                    }
                case InfixExpressionNode::Remainder:
                    if (rightVal.first && rightVal.second != 0) {
                        return {true, leftVal.second % rightVal.second};
                    } else {
                        return {false, 0};
                    }
                    break;
                default:
                    return {false, 0};
            }
        }
        case NodeType::PrefixExpression: {
            auto* prefix = static_cast<PrefixExpressionNode*>(expr);
            auto operandVal = evaluateNumericConstant(prefix->operand);
            if (prefix->op == PrefixExpressionNode::Negate) {
                return {operandVal.first, -operandVal.second};
            } else {
                return {false, 0};
            }
        }
        case NodeType::ParenthesizedExpression: {
            auto* parenExpr = static_cast<ParenthesizedExpressionNode*>(expr);
            return evaluateNumericConstant(parenExpr->expression);
        }
        case NodeType::AssignmentExpression: {
            auto* assignExpr = static_cast<AssignmentExpressionNode*>(expr);
            return evaluateNumericConstant(assignExpr->rhs);
        }
        default:
            return {false, 0};

    }
    return {false, 0};
}

}