#ifndef SEMANTICS_TYPE_LINKER_HPP
#define SEMANTICS_TYPE_LINKER_HPP

#include "ast/ast_visitor.hpp"
#include "ast/nodes.hpp"
#include "ast/ast_node.hpp"
#include "ast/tagged_scope.hpp"
#include <unordered_map>
#include <unordered_set>

namespace semantics {

using namespace ast;

class TypeLinker : public ASTVisitor {
public:
    TaggedScope* globalScope; // Reference to the global scope for resolving types
    
    CompilationUnitNode* currentCompilationUnit = nullptr; // Track the current compilation unit for resolving imports

    explicit TypeLinker(TaggedScope* globalScope) : globalScope(globalScope) {};

    // Entry point for the linking pass
    void link(ASTNode* root);

    void checkPackagePrefixes();
    
    TypeDeclarationNode* resolveType(NameNode* nameNode, CompilationUnitNode* compilationUnit);

    // We only need to visit nodes that contain types to be resolved
    bool visit(CompilationUnitNode* node) override;
    bool visit(ImportDeclarationNode* node) override;
    bool visit(TypeDeclarationNode* node) override;
    bool visit(ClassOrInterfaceTypeNode* node) override;
};

} // namespace semantics

#endif