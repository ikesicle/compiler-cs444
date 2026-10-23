#ifndef SEMANTICS_FORWARD_DECLARE_CHECKER_HPP
#define SEMANTICS_FORWARD_DECLARE_CHECKER_HPP

#include "ast/ast_visitor.hpp"
#include "ast/nodes.hpp"
#include "ast/scope.hpp"
#include "ast/ast_printer.hpp"

namespace semantics {
using namespace ast;

class ForwardDeclareChecker : public ASTVisitor {
    std::unordered_map<VariableDeclarationFragmentNode*, int> order;
    VariableDeclarationFragmentNode* currentDeclContext = nullptr, *found = nullptr;

    static bool isStatic(FieldDeclarationNode* fd) {
        if (!fd) return false;
        for (auto* m : fd->modifiers) {
            if (m && m->kind == ModifierNode::Static) return true;
        }
        return false;
    }

public:
    void run(CompilationUnitNode* u);

    bool visit(TypeDeclarationNode* n) override {
        order.clear();
        currentDeclContext = nullptr;

        int i = 0;
        for (auto* bd : n->bodyDeclarations) {
            if (bd->getNodeType() != NodeType::FieldDeclaration) continue;
            auto* fd = static_cast<FieldDeclarationNode*>(bd);
            if (!fd) continue;
            for (auto* f : fd->fragments) {
                if (f) order[f] = i++;
            }
        }
        return true;
    }

    bool visit(FieldDeclarationNode* n) override {
        for (auto* f : n->fragments) {
            currentDeclContext = f;
            found = nullptr;
            if (f->initializer) f->initializer->accept(*this);
            // std::cerr << "Finished checking initializer for field declaration: " << f->name->getSimpleName() << "\n";
            if (found) {
                // f->accept(ASTPrinter::Instance);
                throw std::runtime_error("Illegal forward reference: " + found->name->getSimpleName());
            }
            currentDeclContext = nullptr;
        }
        return false;
    }

    bool visit(SimpleNameNode* n) override {
        if (!currentDeclContext || n->variant != NameNode::Expression) return false; // Not inside a field declaration, so no forward ref possible
        auto result = n->scope->lookupCurrentScope(n->getSimpleName(), NodeType::VariableDeclarationFragment);
        if (result.empty()) return false; // No local field decl
        auto* decl = static_cast<VariableDeclarationFragmentNode*>(result[0]);
        if (decl->parent->getNodeType() != NodeType::FieldDeclaration) {
            std::cerr << "Warning: Forward reference to non-field declaration: " << n->parent->getNodeType() << std::endl;
            return false; // Not a field declaration fragment, so what is it?
        } 
        if (isStatic(static_cast<FieldDeclarationNode*>(decl->parent))) {
            std::cerr << "Skipping static field reference: " << n->getSimpleName() << std::endl;
            return false; // Non-static fields can be forward referenced, but will be null at runtime
        }
        
        auto itDecl = order.find(decl);
        if (itDecl == order.end()) return false; // Not a field declaration fragment, so no forward ref possible
        if (itDecl->second >= order.find(currentDeclContext)->second) { // Cant read itself or a later declaration
            found = decl;
        }
        return false;
    }

    bool visit(AssignmentExpressionNode* n) override {
        if (!currentDeclContext) return false;
        found = nullptr;
        n->lhs->accept(*this); // Only check innermost name for fwd ref
        found = nullptr;
        n->rhs->accept(*this); // Only check innermost name for fwd ref
        if (found) {
            // n->accept(ASTPrinter::Instance);
            throw std::runtime_error("Illegal forward reference in RHS of assignment: " + found->name->getSimpleName());
        }
        return false;
    }

    bool visit(QualifiedNameNode* n) override {
        if (!currentDeclContext) return false;
        n->qualifier->accept(*this); // Only check innermost name for fwd ref
        return false;
    }

    bool visit(FieldAccessExpressionNode* n) override {
        if (!currentDeclContext) return false;
        found = nullptr;
        n->expression->accept(*this); // Only check innermost name for fwd ref
        if (found) {
            // n->accept(ASTPrinter::Instance);
            throw std::runtime_error("Illegal forward reference in field access: " + found->name->getSimpleName());
        }
        return false;
    }

    bool visit(MethodDeclarationNode* n) override {
        currentDeclContext = nullptr;
        return false;
    }

    bool visit(ConstructorDeclarationNode* n) override {
        currentDeclContext = nullptr;
        return false;
    }
};

}

#endif