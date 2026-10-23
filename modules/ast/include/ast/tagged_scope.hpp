#ifndef AST_TAGGEDSCOPE_HPP
#define AST_TAGGEDSCOPE_HPP

#include "scope.hpp"

namespace ast {

class TaggedScope : public Scope {
public:
    std::unordered_map<std::string, Scope*> tags;
    TaggedScope(): Scope() {
    }

    ~TaggedScope() {
    }
    
    Scope* enterNamedChild(const std::string& tag) {
        if (tags.count(tag)) {
            return tags[tag];
        }
        auto child = createChild();
        tags[tag] = child;
        return child;
    }

    Scope* getChildByTag(const std::string& tag) {
        if (tags.count(tag)) return tags[tag];
        return nullptr;
    }

    std::vector<std::string> getAllTags() const {
        std::vector<std::string> result;
        for (const auto& [tag, _] : tags) {
            result.push_back(tag);
        }
        return result;
    }
};

}


#endif // SEMANTICS_TAGGEDSCOPE_HPP