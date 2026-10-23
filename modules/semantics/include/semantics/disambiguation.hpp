#ifndef SEMANTICS_DISAMBIGUATION_HPP
#define SEMANTICS_DISAMBIGUATION_HPP

#include "ast/ast_visitor.hpp"
#include "ast/nodes.hpp"
#include "type_linker.hpp"
#include "ast/java_type.hpp"

namespace semantics {

using namespace ast;

class Disambiguation : public ASTVisitor {
    TypeLinker& typeLinker;
    JavaType* currentType;
    CompilationUnitNode* currentCompilationUnit;

public:

    explicit Disambiguation(TypeLinker& typeLinker) : typeLinker(typeLinker) {}
    void run(CompilationUnitNode* u);

    bool visit(TypeDeclarationNode* n) override {
        currentType = n->resolvedType;
        currentCompilationUnit = static_cast<CompilationUnitNode*>(n->parent);
        return true;
    }

    void leave(TypeDeclarationNode* n) override {
        currentType = nullptr;
    }

    bool visit(SimpleNameNode* n) override;
    bool visit(QualifiedNameNode* n) override;
};

}

#endif