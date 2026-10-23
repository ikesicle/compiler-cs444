#ifndef AST_AST_HPP
#define AST_AST_HPP

#include "ast_node.hpp"
#include <vector>
#include <memory>
#include <type_traits>
#include <concepts>

namespace ast {

// Concept: Constrains template parameter to types derived from ASTNode
template<typename T>
concept DerivedFromASTNode = std::is_base_of_v<ASTNode, T>;

// AST Factory and Owner of all AST nodes. This class can be used to create and manage the lifecycle of AST nodes.
class AST {
    std::string name; // Optional name for debugging purposes
    ASTNode* root;
    std::vector<std::unique_ptr<ASTNode>> nodes; // Owns all AST nodes to manage their lifetimes
public:
    AST(const std::string& name = "") : name(name), root(nullptr) {}
    void setRoot(ASTNode* root) { this->root = root; }
    ASTNode* getRoot() const { return root; }
    std::string getName() const { return name; }
    
    // Templated function to create and store a node of the given ASTNode-derived type
    template <DerivedFromASTNode T, typename... Args>
    T* create(Args&&... args) {
        auto node = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = node.get();
        nodes.push_back(std::move(node));
        return ptr;
    }
};
    
}

#endif // SEMANTICS_AST_HPP