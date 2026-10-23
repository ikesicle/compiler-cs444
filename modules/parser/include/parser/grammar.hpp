/*
grammar.hpp - Grammar Definition
Written by Hypatia
Ported by Ike
*/

#ifndef PARSER_GRAMMAR_HPP
#define PARSER_GRAMMAR_HPP

#include <vector>
#include <unordered_map>
#include <string>
#include "lr1_item.hpp"
#include "production.hpp"

namespace parser {

class Grammar {
public:
    std::vector<std::string> terminals;
    std::vector<std::string> nonterminals;
    std::vector<Production> productions;
    std::unordered_map<std::string, int> symbolIndexMap;
    std::string startSymbol = START_SYMBOL;
    int startProductionIndex = 0;

    Grammar() {}

    static Grammar fromFile(const std::string& filename);

    static Grammar fromStream(std::istream& grammarStream);

    inline int symbolCount() const {
        return terminals.size() + nonterminals.size();
    }

    inline std::string operator[](int id) const {
        int tcount = terminals.size();
        if (id < tcount) {
            auto it = terminals.begin();
            std::advance(it, id);
            return *it;
        } else {
            auto it = nonterminals.begin();
            std::advance(it, id - tcount);
            return *it;
        }
    }
};

}


#endif // PARSER_GRAMMAR_HPP