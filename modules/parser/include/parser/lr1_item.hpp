/*
lr1_item.hpp - LR(1) Item Definition
Written by Hypatia
Ported by Ike
*/

#ifndef PARSER_LR1ITEM_HPP
#define PARSER_LR1ITEM_HPP

#include <string>
#include <unordered_set>
#include "production.hpp"

#define EPSILON_SYMBOL "EPSILON"
#define START_OF_FILE_SYMBOL "START_FILE"
#define END_OF_FILE_SYMBOL "END_FILE"
#define START_SYMBOL "Start"

namespace parser {
    
struct LR1Item {
    const Production* production;
    int dotPosition;
    std::string lookahead;

    bool operator==(const LR1Item& other) const {
        return production == other.production &&
               dotPosition == other.dotPosition &&
               lookahead == other.lookahead;
    }

    bool isAccepting() const {
        if (production == nullptr) return false;
        return dotPosition == production->rhs.size() && production->lhs == START_SYMBOL && lookahead == END_OF_FILE_SYMBOL;
    }

    std::string toString() const {
        std::string repr = production->lhs + " -> ";
        for (int i = 0; i < production->rhs.size(); i++) {
            if (i == dotPosition) {
                repr += "• ";
            }
            repr += production->rhs[i] + " ";
        }
        if (dotPosition == production->rhs.size()) {
            repr += "• ";
        }
        repr += " [" + lookahead + "]";
        return repr;
    }
};

inline void hash_combine(std::size_t& seed, std::size_t value) noexcept {
    seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
}

typedef std::unordered_set<LR1Item> ItemSet;

}

namespace std {
template<>
struct hash<parser::LR1Item> {
    size_t operator()(const parser::LR1Item& item) const noexcept {
        size_t seed = 0;

        // hash pointer identity
        parser::hash_combine(seed, std::hash<const parser::Production*>{}(item.production));

        // hash dot position
        parser::hash_combine(seed, std::hash<int>{}(item.dotPosition));

        // hash lookahead
        parser::hash_combine(seed, std::hash<std::string>{}(item.lookahead));

        return seed;
    }
};
}

#endif // PARSER_LR1ITEM_HPP