#ifndef COMMON_PARSETREE_HPP
#define COMMON_PARSETREE_HPP

#include <memory>
#include <vector>
#include <string>
#include <iostream>

namespace common {

struct ParseTree {
    std::string kind;
    std::string value;
    std::vector<const ParseTree*> flattenedCache; // for memoizing flatten() results
    std::vector<std::unique_ptr<ParseTree>> children;
    ParseTree(const std::string& kind, const std::string& value)
        : kind(kind), value(value) {}
    
    void addChild(std::unique_ptr<ParseTree> child) {
        children.push_back(std::move(child));
    }

    std::string to_string() const {
        if (children.empty()) {
            return value;
        } else {
            std::string result = "";
            for (const auto& child : children) {
                result += child->to_string();
            }
            return result;
        }
    }

    const ParseTree* findOne(std::string kind = "", std::string value = "") const {
        if ((kind.empty() || this->kind == kind) &&
            (value.empty() || this->value == value)) {
            return this;
        }
        for (const auto& child : children) {
            const ParseTree* result = child->findOne(kind, value);
            if (result != nullptr) return result;
        }
        return nullptr;
    }

    std::vector<const ParseTree*> findAll(std::string kind = "", std::string value = "", bool subSearch = true) const {
        std::vector<const ParseTree*> results;
        if ((kind.empty() || this->kind == kind) &&
            (value.empty() || this->value == value)) {
            results.push_back(this);
            if (!subSearch) {
                // std::cerr << "Found node of kind '" << kind << "' with value '" << value << "'. Skipping subtree search due to subSearch=false.\n";
                return results;
            } else {
                // std::cerr << "Found node of kind '" << kind << "' with value '" << value << "'. Continuing subtree search due to subSearch=true.\n";
            }
        }
        for (const auto& child : children) {
            auto childResults = child->findAll(kind, value, subSearch);
            results.insert(results.end(), childResults.begin(), childResults.end());
        }
        return results;
    };

    std::vector<const ParseTree*>& flatten() {
        if (!flattenedCache.empty()) {
            return flattenedCache;
        }
        flattenedCache.clear();
        if (!this->value.empty()) flattenedCache.push_back(this);
        for (const auto& child : children) {
            std::vector<const ParseTree*> sub = child->flatten();
            flattenedCache.insert(flattenedCache.end(), sub.begin(), sub.end());
        }
        return flattenedCache;
    } 

    const ParseTree* operator[](size_t index) const {
        return children.at(index).get();
    }

    void print(int depth = 0) const {
        for (int i = 0; i < depth; i++) {
            std::cout << "  ";
        }
        std::cout << kind;
        if (!value.empty()) {
            std::cout << " : " << value;
        }
        std::cout << "\n";
        for (const auto& child : children) {
            child->print(depth + 1);
        }
    }

    bool isTerminal() const {
        return children.empty();
    }
};

} // namespace common


#endif // COMMON_PARSETREE_HPP