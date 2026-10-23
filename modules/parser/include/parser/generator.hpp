/*
generator.hpp - LR(1) Parse Table Generator
Written by Hypatia
Ported by Ike
*/

#ifndef PARSER_GENERATOR_HPP
#define PARSER_GENERATOR_HPP

#include "lr1_item.hpp"
#include "grammar.hpp"
#include "state.hpp"
#include <map>
#include <set>

namespace parser {

class Generator {
    int MAX_SYMBOLS = 1024;
    int MAX_STATES = 2048;

public:
    Grammar grammar;
    std::map<std::string, int> symbolIDs;
    std::map<int, std::string> idToSymbol;
    std::vector<std::vector<int>> dependents, transitions;
    std::vector<std::set<int>> FIRST;
    std::vector<ItemSet> states;

    Generator(const Grammar& g) : grammar(g) {
        computeFIRST();
        buildCanonicalCollection();
    }

    ItemSet getClosure(const ItemSet& I);
    ItemSet getGOTO(const ItemSet& I, const std::string& X);
    void buildCanonicalCollection();
    int getSymbolID(const std::string& symbol);
    void buildFIRSTDependencies();
    void computeFIRST();
    std::set<std::string> getSeqFIRST(const std::vector<std::string>& seq);

    std::vector<std::vector<Action>> generateParseTable();
    void writeParseTable(const std::string& filename);
    void writeEmbeddedTransitionTable(const std::string& filename);

};

}

#endif // PARSER_GENERATOR_HPP