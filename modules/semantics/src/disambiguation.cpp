#include "semantics/disambiguation.hpp"
#include "ast/nodes.hpp"
#include "ast/scope.hpp"
#include <stdexcept>

namespace semantics {

void Disambiguation::run(CompilationUnitNode* u) {
    if (u) u->accept(*this);
}

bool Disambiguation::visit(SimpleNameNode* n) {
    if (n->variant != NameNode::Ambiguous && n->variant != NameNode::Expression) return true; // Search deeper
    /*
    If the Identifier appears within the scope (§6.3) of a local variable declaration
     (§14.4) or parameter declaration (§8.4.1, §8.8.1, §14.19) or field declaration 
     (§8.3) with that name, then the AmbiguousName is reclassified as an 
     ExpressionName.
    */
    auto variable = n->scope->lookup(n->getSimpleName());
    if (!variable.empty()) {
        for (ASTNode* node : variable) {
            if (node->getNodeType() != NodeType::SingleVariableDeclaration && 
                node->getNodeType() != NodeType::VariableDeclarationFragment) {
                continue;
            }
            n->variant = NameNode::Expression;
            n->resolvedReference = node;
            return false; // Don't need to search deeper since we found a match
        }
    }

    // Look up in type (scope doesn't capture inherited types)
    auto fieldIt = currentType->fields.find(n->getSimpleName());
    if (fieldIt != currentType->fields.end()) {
        n->variant = NameNode::Expression;
        n->resolvedReference = fieldIt->second.decl;
        return false; // Don't need to search deeper since we found a match
    }

    if (n->variant == NameNode::Expression && n->resolvedReference == nullptr) {
        throw std::runtime_error("Failed to resolve expression name " + n->getSimpleName());
    }

    if (n->variant != NameNode::Ambiguous) return true;
    /*
    Otherwise, if the Identifier appears within the scope (§6.3) of a local class 
    declaration (§14.3) or member type declaration (§8.5, §9.5) with that name, then 
    the AmbiguousName is reclassified as a TypeName.

    Doesn't apply because we don't have local classes or member types.
    */
    auto type = n->scope->lookup(n->getSimpleName(), NodeType::TypeDeclaration);
    if (!type.empty()) {
        n->variant = NameNode::Type;
        n->resolvedReference = type[0];
        return false; // Don't need to search deeper since we found a match
    }

    auto onDemandImportedTyped = typeLinker.resolveType(n, currentCompilationUnit);
    if (onDemandImportedTyped) {
        n->variant = NameNode::Type;
        n->resolvedReference = onDemandImportedTyped;
        return false; // Don't need to search deeper since we found a match
    }

    n->variant = NameNode::Package; // If it doesn't resolve to a variable or type, it has to be a package name
    n->resolvedReference = nullptr; // We don't resolve package names since they don't point to any specific ASTnode
    return false;
}

bool Disambiguation::visit(QualifiedNameNode* n) {
    if (!n || n->variant != NameNode::Ambiguous) return true; // Search deeper
    n->qualifier->accept(*this);
    if (n->qualifier->variant == NameNode::Ambiguous) {
        // Can't resolve because it's still ambiguous, so just return and hope that it gets resolved later when we visit the qualifier
        return false;
    } else if (n->qualifier->variant == NameNode::Package) {
        auto pkg = typeLinker.globalScope->getChildByTag(n->getQualifierString());
        if (pkg == nullptr) {
            n->variant = NameNode::Package;
            // Assume this is just an extended package name and try to resolve it later down the line
            return false;
        } else {
            auto type = pkg->lookupCurrentScope(n->name->getSimpleName(), NodeType::TypeDeclaration);
            if (!type.empty()) {
                n->variant = NameNode::Type;
                n->resolvedReference = type[0];
                return false;
            } else {
                n->variant = NameNode::Package;
                return false;
            }
        }
    } else if (n->qualifier->variant == NameNode::Type) {
        TypeDeclarationNode* typeDecl = static_cast<TypeDeclarationNode*>(n->qualifier->resolvedReference);
        JavaType* type = typeDecl->resolvedType;
        auto field = type->fields.find(n->name->getSimpleName());
        if (field == type->fields.end() || !field->second.stat) {
             throw std::runtime_error("No static field named " + n->name->getSimpleName() + " found in type " + type->name);
        }
        n->variant = NameNode::Expression;
        n->resolvedReference = field->second.decl;
        std::cerr << "Resolved qualified name " << n->name->toString() << " to static field " << field->second.decl->name->getSimpleName() << " in type " << type->name << ", with ref type " << n->resolvedReference->getNodeType() << "\n";
        return false;
    } else if (n->qualifier->variant == NameNode::Expression) {
        // Can't resolve non-static method or field accesses at this stage
        return false;
    } else {
        throw std::runtime_error("Unexpected qualifier variant");
    }

    return false;
}

}