#ifndef AST_SCOPE_HPP
#define AST_SCOPE_HPP

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "ast_node.hpp"

namespace ast {

class Scope {
protected:
    std::unordered_map<std::string, std::vector<ASTNode*>> table;
    std::vector<std::unique_ptr<Scope>> children;
public:
    virtual ~Scope() {
    }
    Scope* parent = nullptr;

    Scope(): table(), children(), parent(nullptr) {}

    // Adds a symbol, returns false if it's a non-method duplicate declaration in the same scope
    bool declare(const std::string& name, ASTNode* node) {
        if (table.count(name) == 0) {
            table[name] = {};
        }
        for (ASTNode* existingNode : table[name]) {
            if (existingNode->getNodeType() == node->getNodeType() && existingNode->getNodeType() != NodeType::MethodDeclaration) {
                return false; // Duplicate declaration of the same name and type in the same scope (except for method overloading)
            }
        }
        // std::cerr << "Declaring symbol: " << name << " in scope " << this << "\n";
        table[name].push_back(node);
        return true;
    }

    std::vector<ASTNode*> lookupCurrentScope(const std::string& name) {
        if (table.count(name) == 0) {
            table[name] = {};
        }
        return table[name];
    }

    std::vector<ASTNode*> lookupCurrentScope(const std::string& name, NodeType expectedType) {
        std::vector<ASTNode*> results;
        if (table.count(name) == 0) {
            return results;
        }
        for (ASTNode* node : table[name]) {
            if (node->getNodeType() == expectedType) {
                results.push_back(node);
            }
        }
        return results;
    }

    std::vector<ASTNode*> lookupCurrentScope(NodeType expectedType) {
        std::vector<ASTNode*> results;
        for (const auto& pair : table) {
            for (ASTNode* node : pair.second) {
                if (node->getNodeType() == expectedType) {
                    results.push_back(node);
                }
            }
        }
        return results;
    }

    // Recursively looks up a name in this scope or any parent scope
    std::vector<ASTNode*> lookup(const std::string& name) {
        std::vector<ASTNode*> results;
        if (table.count(name) > 0) {
            results.insert(results.end(), table[name].begin(), table[name].end());
        }
        if (parent) {
            auto parentResults = parent->lookup(name);
            results.insert(results.end(), parentResults.begin(), parentResults.end());
        }
        return results;
    }

    // Looks up a name that must be attached to a specific AST node type
    std::vector<ASTNode*> lookup(const std::string& name, NodeType expectedType) {
        // std::cerr << "Looking up symbol: " << name << " in scope " << this << " with expected type " << expectedType << "\n";
        std::vector<ASTNode*> results;
        if (table.count(name) > 0) {
            for (ASTNode* node : table[name]) {
                if (node->getNodeType() == expectedType) {
                    results.push_back(node);
                }
            }
        }
        if (parent) {
            auto parentResults = parent->lookup(name, expectedType);
            results.insert(results.end(), parentResults.begin(), parentResults.end());
        }
        return results;
    }

    virtual Scope* createChild() {
        children.push_back(std::make_unique<Scope>());
        children.back()->parent = this;
        return children.back().get();
    }
};

}
#endif