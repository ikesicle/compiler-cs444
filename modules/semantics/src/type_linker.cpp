#include "semantics/type_linker.hpp"
#include "ast/nodes.hpp"
#include <sstream>
#include <string>

namespace semantics {

void TypeLinker::link(ASTNode* root) {
    if (root) {
        root->accept(*this);
    }
}

void TypeLinker::checkPackagePrefixes() {
    // Check that no package prefix resolves to a type
    for (const auto& pair : globalScope->tags) {
        const std::string& tag = pair.first;
        std::stringstream ss(tag);
        std::string segment;
        Scope* currentScope = nullptr;
        std::string pathSoFar = "";
        std::vector<ASTNode*> results;

        while (std::getline(ss, segment, '.')) {
            // std::cerr << "Searching for segment '" << segment << "' in scope path '" << pathSoFar << "'\n";
            currentScope = globalScope->getChildByTag(pathSoFar);
            if (currentScope == nullptr) {
                // std::cerr << "Doesn't exist as a package: " << pathSoFar << "\n";
            } else {
                // std::cerr << "Found package scope for '" << pathSoFar << "': " << currentScope << "\n";
                results = currentScope->lookupCurrentScope(segment, NodeType::TypeDeclaration);
                if (!results.empty() && !ss.eof() && !pathSoFar.empty()) { // Default package can have type name clashes
                    throw std::runtime_error("Prefix of package " + (pathSoFar + ".") + segment + " resolves to a type, which is forbidden.");
                }
                // std::cerr << "Lookup results for segment '" << segment << "' in scope path '" << pathSoFar << "': " << results.size() << " found\n";
            }
            pathSoFar += (pathSoFar.empty() ? "" : ".") + segment;
        }
    }
}

bool TypeLinker::visit(CompilationUnitNode* node) {
    currentCompilationUnit = node;
    node->importedTypes.clear();
    node->wildcardImports.clear();
    node->singleImports.clear();
    checkPackagePrefixes();
    return true;
}

bool TypeLinker::visit(ImportDeclarationNode* node) {
    std::cerr << "Processing import declaration: " << node->name->toString() << (node->isWildcard ? ".*" : "") << "\n";
    if (node->isWildcard) {
        if (globalScope->getChildByTag(node->name->toString()) == nullptr) {
            throw std::runtime_error("Wildcard import does not exist as a package: " + node->name->toString());
        }
        currentCompilationUnit->wildcardImports.insert(node->name->toString());
    } else {
        auto package = globalScope->getChildByTag(node->name->getQualifierString());
        if (!package) {
            throw std::runtime_error("Imported package does not exist: " + node->name->getQualifierString());
        }
        auto results = package->lookupCurrentScope(node->name->getSimpleName(), NodeType::TypeDeclaration);
        if (results.empty()) {
            throw std::runtime_error("Imported type does not exist: " + node->name->toString());
        }
        TypeDeclarationNode* typeDecl = static_cast<TypeDeclarationNode*>(results[0]);
        std::string simpleName = node->name->getSimpleName();
        std::string fullName = node->name->toString();
        if (currentCompilationUnit->singleImports.count(fullName)) {
            return true; // Already imported this type with a single-type import, no need to check again
        }

        if (currentCompilationUnit->importedTypes.count(simpleName) && !currentCompilationUnit->importedTypes[simpleName].isWildcard) {
            throw std::runtime_error("Duplicate single-type import for: " + simpleName);
        }

        currentCompilationUnit->importedTypes[simpleName] = {false, node->name->toString(), typeDecl};
        // std::cerr << "Added single-type import: " << simpleName << " -> " << typeDecl << "\n";
        currentCompilationUnit->singleImports.insert(fullName);
    }
    return true;
}

bool TypeLinker::visit(TypeDeclarationNode* node) {
    // Implicitly add java.lang.* to the imports of every compilation unit
    currentCompilationUnit->wildcardImports.insert("java.lang");
    // std::cerr << "Processing type declaration: " << node->name->toString() << "\n";
    // We can also add the declared type to the importedTypes map to catch clashes with imports
    for (auto& [simpleName, importInfo] : currentCompilationUnit->importedTypes) {
        if (simpleName == node->name->getSimpleName() && importInfo.resolvedDeclaration != node) {
            throw std::runtime_error("Type declaration " + node->name->toString() + " clashes with an explicitly imported type of the same name.");
        }
        if (!node->scope->declare(simpleName, importInfo.resolvedDeclaration)) {
            throw std::runtime_error("An import " + simpleName + " clashes with another imported type.");
        }
        // std::cerr << "Added imported type to scope: " << simpleName << " -> " << importInfo.resolvedDeclaration << "\n";
    }
    return true;
}

TypeDeclarationNode* TypeLinker::resolveType(NameNode* nameNode, CompilationUnitNode* compilationUnit) {
    TypeDeclarationNode* resolvedType = nullptr;
    // std::cerr << "Resolving type for name node: " << nameNode->toString() << "\n";
    // Attempt to resolve the name (e.g., "String" or "java.util.List")
    if (nameNode->getNodeType() == NodeType::SimpleName) {
        auto results = nameNode->scope->lookup(nameNode->getSimpleName(), NodeType::TypeDeclaration);
        if (!results.empty()) {
            resolvedType = static_cast<TypeDeclarationNode*>(results[0]);
        } else {
            // Attempt to resolve from wildcards
            // std::cerr << "Attempting to resolve type " << nameNode->getSimpleName() << " from wildcard imports\n";
            for (const std::string& wildcard : compilationUnit->wildcardImports) {
                // std::cerr << "Checking wildcard import: " << wildcard << ".*\n";
                auto package = globalScope->getChildByTag(wildcard);
                // std::cerr << "Looking for type " << nameNode->getSimpleName() << " in package scope " << package << "\n";
                if (package) {
                    auto results = package->lookupCurrentScope(nameNode->getSimpleName(), NodeType::TypeDeclaration);
                    if (!results.empty()) {
                        if (resolvedType != nullptr) {
                            throw std::runtime_error("Type " + nameNode->getSimpleName() + " is found in multiple wildcard imports, causing ambiguity.");
                        } else {
                            resolvedType = static_cast<TypeDeclarationNode*>(results[0]);
                            // std::cerr << "Resolved type " << nameNode->getSimpleName() << " to declaration " << resolvedType << " via wildcard import " << wildcard << ".*\n";
                        }
                    }
                } else {
                    throw std::runtime_error("Wildcard import " + wildcard + ".* does not exist as a package");
                }
            }
        }
    } else {
        Scope* currentScope = nullptr;
        std::vector<ASTNode*> results;

        QualifiedNameNode* qualifiedNameNode = static_cast<QualifiedNameNode*>(nameNode);

        if (resolveType(qualifiedNameNode->qualifier, compilationUnit) != nullptr)  { // Prefix resolves to a type
            throw std::runtime_error("Prefix of type name " + qualifiedNameNode->qualifier->toString() + " resolves to a type, which is forbidden.");
        }

        Scope* scope = globalScope->getChildByTag(qualifiedNameNode->qualifier->toString());
        if (scope != nullptr) {
            results = scope->lookupCurrentScope(qualifiedNameNode->name->getSimpleName(), NodeType::TypeDeclaration);
            if (!results.empty()) {
                resolvedType = static_cast<TypeDeclarationNode*>(results[0]);
            }
        }

        
    }
    return resolvedType;
}

bool TypeLinker::visit(ClassOrInterfaceTypeNode* node) {
    node->resolvedDeclaration = resolveType(node->name, currentCompilationUnit);
    if (!node->resolvedDeclaration) {
        throw std::runtime_error("Unresolved type: " + node->name->toString());
    }
    return true;
}

} // namespace semantics