#ifndef PARSER_LR1PARSER_HPP
#define PARSER_LR1PARSER_HPP

#include "common/parse_tree.hpp"
#include "common/token.hpp"
#include "state.hpp"
#include "grammar.hpp"
#include <vector>

namespace parser {

class LR1Parser {
    std::vector<std::vector<Action>> transitions;
    Grammar grammar;
public:
    LR1Parser(const Grammar& g, const std::vector<std::vector<Action>>& table)
        : grammar(g), transitions(table) {}

    LR1Parser() = default;

    static LR1Parser fromFiles(const std::string& grammarFile, const std::string& tableFile);
    static LR1Parser fromStreams(std::istream& grammarStream, std::istream& tableStream);

    std::unique_ptr<common::ParseTree> parse(const std::vector<common::Token>& tokens);
    std::unique_ptr<common::ParseTree> parse(const std::vector<std::pair<std::string, std::string>>& rawTokens);
};

} // namespace parser

#endif // PARSER_LR1PARSER_HPP