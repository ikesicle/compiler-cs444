#include "parser/lr1_parser.hpp"
#include "common/parse_tree.hpp"
#include "parser/state.hpp"
#include "parser/production.hpp"
#include "common/token_type.hpp"
#include <fstream>
#include <iostream>
#include <istream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace parser {

LR1Parser LR1Parser::fromStreams(std::istream& grammarStream, std::istream& tableStream) {
    Grammar g = Grammar::fromStream(grammarStream);
    std::istream& infile = tableStream;
    int state_num;
    std::string value;
    infile >> state_num;
    // std::cerr << "Total table states: " << state_num << ", reading from " << tableFile << "\n";
    std::vector<std::vector<Action>> transitions(state_num, std::vector<Action>(g.symbolCount(), {ActionType::ERROR, -1}));
    
    while (value != ".TRANSITIONS") {
        infile >> value;
        // std::cerr << value << "\n";
    }
    std::stringstream ss;
    int src, target;
    std::string token, action;
    // std::cerr << "Reading parse table from " << tableFile << "\n";
    while (std::getline(infile, value)) {
        // std::cerr << "Read line: " << value << "\n";
        if (value.empty()) continue;
        ss.str(value);
        ss.clear();
        ss >> src >> token >> action >> target;
        // std::cerr << "Parsed entry: " << src << " " << token << " " << action << " " << target << "\n";
        if (action == "shift") {
            transitions[src][g.symbolIndexMap[token]] = {ActionType::SHIFT, target};
            // std::cerr << "SHIFT action added: state " << src << ", token " << token << " -> state " << target << "\n";
        } else if (action == "reduce") {
            transitions[src][g.symbolIndexMap[token]] = {ActionType::REDUCE, target};
            // std::cerr << "REDUCE action added: state " << src << ", token " << token << " -> reduce using rule " << target << "\n";
        } else if (action == "goto") {
            transitions[src][g.symbolIndexMap[token]] = {ActionType::GOTO, target};
            // std::cerr << "GOTO action added: state " << src << ", token " << token << " -> state " << target << "\n";
        } else if (action == "accept") {
            transitions[src][g.symbolIndexMap[token]] = {ActionType::ACCEPT, 0};
            // std::cerr << "ACCEPT action added: state " << src << ", token " << token << " -> accept\n";
        } else {
            throw std::runtime_error("Invalid action type in parse table: " + action);
        }
    }
    return LR1Parser(g, transitions);
}

LR1Parser LR1Parser::fromFiles(const std::string& grammarFile, const std::string& tableFile) {
    std::ifstream grammarStream(grammarFile);
    if (!grammarStream) {
        throw std::runtime_error("Failed to open grammar file: " + grammarFile);
    }
    std::ifstream tableStream(tableFile);
    if (!tableStream) {
        throw std::runtime_error("Failed to open table file: " + tableFile);
    }
    LR1Parser ret = fromStreams(grammarStream, tableStream);
    grammarStream.close();
    tableStream.close();
    return ret;
}

std::unique_ptr<common::ParseTree> LR1Parser::parse(const std::vector<std::pair<std::string, std::string>>& rawTokens) {
    std::vector<int> stateStack;
    std::vector<std::unique_ptr<common::ParseTree>> parseStack;
    stateStack.push_back(0); // initial state
    size_t inputIndex = 0;
    try {
        for (std::pair<std::string, std::string> token : rawTokens) {
            std::unique_ptr<common::ParseTree> tokentree = std::make_unique<common::ParseTree>(token.first, token.second);

#ifdef DEBUG_PARSER
            std::cerr << "[ ";
            for (auto& e : parseStack) {
                std::cerr << e->kind << " ";
            }
            std::cerr << "] Next token: " << tokentree->kind << " ( " << tokentree->value << " ) -- ";
#endif
            
            Action next = transitions[stateStack.back()][grammar.symbolIndexMap.at(tokentree->kind)];
            while (next.type == ActionType::REDUCE || next.type == ActionType::ACCEPT) {
                const Production& p = grammar.productions[next.target];
                std::unique_ptr<common::ParseTree> newParent = std::make_unique<common::ParseTree>(p.lhs, "");
                newParent->children.resize(p.rhs.size());
                for (int i = p.rhs.size()-1; i >= 0; i--) {
                    newParent->children[i] = std::move(parseStack.back());
                    parseStack.pop_back();
                    stateStack.pop_back();
                }
                parseStack.push_back(std::move(newParent));
#ifdef DEBUG_PARSER
                std::cerr << "REDUCE\nprod: " << p.lhs << " ->";
                for (const std::string& sym : p.rhs) {
                    std::cerr << " " << sym;
                }
                std::cerr << "\n";
#endif
                if (next.type == ActionType::ACCEPT) {
                    if (parseStack.size() != 1) {
                        throw std::runtime_error("Parsing error: multiple items on stack at ACCEPT");
                    }
                    return std::move(parseStack.back());
                }

                next = transitions[stateStack.back()][grammar.symbolIndexMap.at(p.lhs)];
                if (next.type != ActionType::GOTO) {
                    std::cerr << "Expected GOTO action after reduction, but got: " << (int)next.type << "\n";
                    throw std::runtime_error("Invalid GOTO action during reduction");
                }
                stateStack.push_back(next.target);
#ifdef DEBUG_PARSER
                std::cerr << "[ ";
                for (auto& e : parseStack) {
                    std::cerr << e->kind << " ";
                }
                std::cerr << "] Next token: " << tokentree->kind << " ( " << tokentree->value << " ) -- ";
#endif
                next = transitions[stateStack.back()][grammar.symbolIndexMap.at(tokentree->kind)];
            }
            if (next.type == ActionType::ERROR) {
                throw std::runtime_error("Parsing error: unexpected token " + tokentree->kind + " at state " + std::to_string(stateStack.back()));
            }
            if (next.type == ActionType::SHIFT) {
#ifdef DEBUG_PARSER
                std::cerr << "SHIFT\n";
#endif
                parseStack.push_back(std::move(tokentree));
                stateStack.push_back(next.target);
                inputIndex++;
            } else {
                throw std::runtime_error("Parsing error: invalid action type " + std::to_string((int)next.type));
            }
        }
    } catch (const std::out_of_range& e) {
        std::cerr << "Range error (token outside of grammar?) : " << e.what() << "\n";
        return nullptr;
    } catch (const std::runtime_error& e) {
        std::cerr << "Runtime error: " <<e.what() << "\n";
        return nullptr;
    }
    std::cerr << "Parsing error: reached end of input without ACCEPT\n";
    return nullptr;
}

std::unique_ptr<common::ParseTree> LR1Parser::parse(const std::vector<common::Token>& tokens) {
    std::vector<std::pair<std::string, std::string>> equivalentTokens;
    for (const common::Token& token : tokens) {
        equivalentTokens.emplace_back(common::TOKEN_TYPE_NAMES[(int)token.type], token.lexeme);
    }
    return parse(equivalentTokens);
}

} // namespace parser