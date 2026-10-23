/*
grammar.cpp - Grammar Implementation
Written by Ike
*/

#include "parser/grammar.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

namespace parser {

Grammar Grammar::fromStream(std::istream& grammarStream) {
    Grammar g;
    int symbolIndex = 0;
    
    std::istream& in = grammarStream;
    if (!in) {
        throw std::runtime_error("Failed to read from grammar stream");
    }

    int terminal_count, nonterminal_count, production_count;
    std::string value;
    in >> value; // Read .TERMINALS
    if (value != ".TERMINALS") {
        std::cerr << "Expected .TERMINALS, got: '" << value << "'\n";
        throw std::runtime_error("Expected .TERMINALS section");
    }
    while (in >> value && value != ".NONTERMINALS") {
        g.terminals.push_back(value);
        g.symbolIndexMap[value] = symbolIndex++;
    }

    // Now value should be .NONTERMINALS
    if (value != ".NONTERMINALS") {
        throw std::runtime_error("Expected .NONTERMINALS section");
    }
    while (in >> value && value != ".PRODUCTIONS") {
        g.nonterminals.push_back(value);
        g.symbolIndexMap[value] = symbolIndex++;
    }

    if (value != ".PRODUCTIONS") {
        throw std::runtime_error("Expected .PRODUCTIONS section");
    }
    std::stringstream ss;
    std::string line, lhs, rhsItem;
    std::unordered_set<std::string> productionSet; // To check for duplicates
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        // Assuming a production rule consists of a left-hand side and a right-hand side
        ss.str(line);
        ss.clear();
        ss >> lhs;
        if (lhs == "") continue;
        std::vector<std::string> rhs;
        while (ss >> rhsItem) {
            rhs.push_back(rhsItem);
        }
        g.productions.emplace_back(lhs, rhs);
        auto v = productionSet.emplace(g.productions.back().toString());
        if (!v.second) {
            std::cerr << "Duplicate production detected: " << g.productions.back().toString() << "\n";
        }
        // std::cerr << "Loaded production: " << lhs << " -> ";
        // for (const auto& sym : rhs) {
        //     std::cerr << sym << " ";
        // }
        // std::cerr << "\n";
    }
    // in.close();
    // std::cerr << "Loaded grammar from " << filename << " with "
    //           << g.terminals.size() << " terminals, "
    //           << g.nonterminals.size() << " nonterminals, and "
    //           << g.productions.size() << " productions.\n";
    return g;
}

Grammar Grammar::fromFile(const std::string &filename) {
    std::ifstream infile(filename);
    if (!infile) {
        throw std::runtime_error("Failed to open grammar file: " + filename);
    }
    Grammar grammar = fromStream(infile);
    infile.close();
    return grammar;
}

} // namespace parser