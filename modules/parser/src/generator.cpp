/*
generator.cpp - LR(1) Parse Table Generator
Written by Hypatia
Ported by Ike
*/

#include "parser/generator.hpp"
#include "parser/lr1_item.hpp"
#include <stdexcept>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

namespace parser {

ItemSet Generator::getClosure(const ItemSet& I) {

    int changed = 1;
    ItemSet result = I;
    while (changed) {
        changed = 0;

        for (auto it = result.begin(); it != result.end(); ++it) {
            const LR1Item& item = *it;
            const Production* p = item.production;

            if (p == nullptr)
                throw std::runtime_error("Null production in LR(1) item");

            /* Dot at end: nothing to expand */
            if (item.dotPosition >= p->rhs.size())
                continue;

            const std::string& B = p->rhs[item.dotPosition];

            /* Only expand nonterminals */
            if (std::find(grammar.nonterminals.begin(), grammar.nonterminals.end(), B) == grammar.nonterminals.end())
                continue;

            /* Build β */
            std::vector<std::string> beta;
            int beta_len = 0;
            for (int k = item.dotPosition + 1; k < p->rhs.size(); k++) {
                beta.push_back(p->rhs[k]);
            }

            /* Compute FIRST(βa) */
            beta.push_back(item.lookahead);
            std::set<std::string> lookaheads = getSeqFIRST(beta);
            beta.pop_back();

            /* For each production B → γ */
            for (const Production& prod : grammar.productions) {
                if (prod.lhs != B)
                    continue;
                for (const std::string& lookahead : lookaheads) {
                    if (lookahead == EPSILON_SYMBOL)
                        continue;

                    LR1Item newItem;
                    newItem.production = &prod;
                    newItem.dotPosition = 0;
                    newItem.lookahead = lookahead;

                    if (!result.contains(newItem)) {
                        result.insert(newItem);
                        changed = 1;
                    }
                }
            }
        }
    }
    return result;
}

ItemSet Generator::getGOTO(const ItemSet& I, const std::string& X) {
    ItemSet result;

    for (auto it = I.begin(); it != I.end(); ++it) {
        const LR1Item& item = *it;
        const Production* p = item.production;

        if (item.dotPosition < p->rhs.size() &&
            p->rhs[item.dotPosition] == X) {

            LR1Item moved;
            moved.production = item.production;
            moved.dotPosition = item.dotPosition + 1;
            moved.lookahead = item.lookahead;

            if (!result.contains(moved)) {
                result.insert(moved);
            }
        }
    }

    if (!result.empty())
        result = getClosure(result);
    return result;
}

void Generator::buildCanonicalCollection() {
     // Initialize transition table 
    transitions.clear();
    transitions.resize(MAX_STATES, std::vector<int>(MAX_SYMBOLS, -1));

    // Initial state: [S' → • Start, $] 
    ItemSet start;
    start.insert({
        .production = &grammar.productions[grammar.startProductionIndex], // augmented production
        .dotPosition = 0,
        .lookahead = "END_FILE"
    });

    start = getClosure(start);

    states.clear();
    states.push_back(start);
    char anim[4] = {'|', '/', '-', '\\'};

    // BFS-style expansion 
    std::cout << "Building DFA states...\n";
    for (int i = 0; i < states.size(); i++) {
        std::cout << "\r" << anim[i%4] << " Processing state " << i <<  "..." << std::flush;
        for (int s = 0; s < grammar.symbolCount(); s++) {
            ItemSet next = getGOTO(states[i], grammar[s]);

            if (next.size() == 0)
                continue;

            int target = -1;
            for (int j = 0; j < states.size(); j++) {
                if (states[j] == next) {
                    target = j;
                    break;
                }
            }

            if (target == -1) {
                if (states.size() >= MAX_STATES) {
                    std::cerr << "Error: Too many LR(1) states\n";
                    exit(1);
                }
                target = states.size();
                states.push_back(next);
            }
            transitions[i][s] = target;
            // std::cerr << "State " << i <<"  On symbol '" << grammar[s] << "' -> state " << target << "\n";
        }
    }
#ifdef DEBUG_GENERATOR_STATES
    for (int i = 0; i < states.size(); i++) {
        std::cout << "\nState " << i << ":\n";
        for (const LR1Item& item : states[i]) {
            std::cout << "  " << item.toString() << "\n";
        }
    }
#endif
    std::cout << "\nDFA build complete. " << states.size() << " states generated.\n";
}

int Generator::getSymbolID(const std::string& symbol) {
    if (symbol == EPSILON_SYMBOL) {
        return -1;
    }
    try {
        int v = grammar.symbolIndexMap.at(symbol);
        // std::cerr << "Symbol '" << symbol << "' has ID " << v << "\n";
        return v;
    } catch (const std::out_of_range& e) {
        std::cerr << "Symbol not found in grammar: '" << symbol << "'" << std::endl;
        throw;
    }
}

void Generator::computeFIRST() {
    // Initialize FIRST sets
    FIRST.clear();
    for (int i = 0; i < grammar.symbolCount(); i++)
        FIRST.push_back(std::set<int>());

    // FIRST(t) = {t}
    for (std::string terminal : grammar.terminals) {
        int id = getSymbolID(terminal);
        FIRST[id].insert(id);
    }
    
    int epsilon = getSymbolID(EPSILON_SYMBOL);
    bool changed = true;
    while (changed) {
        changed = false;
        for (const Production& p : grammar.productions) {
            // std::cerr << "Processing production: " << p.lhs << " -> ";
            // for (const auto& sym : p.rhs) {
            //     std::cerr << sym << " ";
            // }
            // std::cerr << "\n";
            int A = getSymbolID(p.lhs);
            bool nullable = true;
            
            

            for (std::string symbol : p.rhs) {
                int Y = getSymbolID(symbol);
                
                if (Y == epsilon) {
                    continue;
                }

                for (int sym : FIRST[Y]) {
                    if (sym != epsilon) {
                        // std::cerr << "Adding symbol " << grammar[sym] << " to FIRST(" << grammar[A] << ")\n";
                        changed = changed || FIRST[A].insert(sym).second;
                    }
                }

                if (!FIRST[Y].contains(epsilon)) {
                    nullable = false;
                    break;
                }
            }

            if (nullable) {
                // std::cerr << "Symbol " << grammar[A] << " is nullable, stopping.\n";
                changed = changed || FIRST[A].insert(epsilon).second;
            }
        }
    }
    // for (int i = 0; i < FIRST.size(); i++) {
    //     std::cerr << "FIRST(" << grammar[i] << ") = { ";
    //     for (int sym : FIRST[i]) {
    //         if (sym != epsilon) {
    //             std::cerr << grammar[sym] << " ";
    //         } else {
    //             std::cerr << "EPSILON ";
    //         }
    //     }
    //     std::cerr << "}\n";
    // }
}

std::set<std::string> Generator::getSeqFIRST(const std::vector<std::string>& seq) {
    std::set<std::string> out;
    int nullable = 1;
    int epsilon = getSymbolID(EPSILON_SYMBOL);
    for (std::string symbol : seq) {
        int X = getSymbolID(symbol);

        for (int sym : FIRST[X]) {
            if (sym != epsilon) {
                out.insert(grammar[sym]);
            }
        }

        if (!FIRST[X].contains(epsilon)) {
            nullable = 0;
            break;
        }
    }

    return out;
}

std::vector<std::vector<Action>> Generator::generateParseTable() {
    std::vector<std::vector<Action>> parseTable;
    parseTable.resize(states.size(), std::vector<Action>(grammar.symbolCount(), {ActionType::ERROR, -1}));
    /* Shift rules (from DFA transitions) */
    for (int i = 0; i < states.size(); i++) {
        for (std::string terminal : grammar.terminals) {
            int sym_index = getSymbolID(terminal);
            int target = transitions[i][sym_index];

            if (target != -1) {
                parseTable[i][sym_index] = {ActionType::SHIFT, target};
            }
        }
    }

    /* Goto rules (from DFA transitions) */
    for (int i = 0; i < states.size(); i++) {
        for (std::string nonterminal : grammar.nonterminals) {
            int sym_index = getSymbolID(nonterminal);
            int target = transitions[i][sym_index];
            // std::cerr << "GOTO from state " << i << " on nonterminal '" << nonterminal << "' -> " << target << "\n";
            if (target != -1) {
                parseTable[i][sym_index] = {ActionType::GOTO, target};
            }
        }
    }

    /* Reduce rules (accept is a special case) */
    for (int i = 0; i < states.size(); i++) {
        for (const LR1Item& item : states[i]) {
            const Production* p = item.production;

            if (item.dotPosition == p->rhs.size()) {
                if (item.isAccepting()) {
                    parseTable[i][getSymbolID(item.lookahead)] = {ActionType::ACCEPT, 0};
                } else {
                    if (parseTable[i][getSymbolID(item.lookahead)].type == ActionType::REDUCE) {
                        std::cerr << "Grammar error: Conflict between states at state " << i << " on lookahead '" << item.lookahead << "':\n";
                        int existing = parseTable[i][getSymbolID(item.lookahead)].target;
                        int newItem = item.production - &grammar.productions[0];
                        std::cerr << "  Existing action: REDUCE by production " << existing << " (" << grammar.productions[existing].toString() << ")\n";
                        std::cerr << "  New action: REDUCE by production " << newItem << " (" << grammar.productions[newItem].toString() << ")\n";
                        std::cerr << "Items in state " << i << ":\n";
                        for (const LR1Item& it2 : states[i]) {
                            if (it2.lookahead == item.lookahead) {
                                std::cerr << it2.toString() << "\n";
                            }
                        }
                        std::exit(43);
                    }
                    if (parseTable[i][getSymbolID(item.lookahead)].type == ActionType::SHIFT) {
                        std::cerr << "Grammar error: Shift/Reduce conflict at state " << i << " on lookahead '" << item.lookahead << "'\n";
                        int existing = parseTable[i][getSymbolID(item.lookahead)].target;
                        std::cerr << "  Existing action: SHIFT to state " << existing << "\n";
                        // std::cerr << "Items in state " << existing << ":\n";
                        // for (const LR1Item& it2 : states[existing]) {
                        //     std::cerr << it2.toString() << "\n";
                        // }
                        int newItem = item.production - &grammar.productions[0];
                        std::cerr << "  New action: REDUCE by production " << newItem << " (" << grammar.productions[newItem].toString() << ")\n";
                        std::cerr << "Items in state " << i << ":\n";
                        for (const LR1Item& it2 : states[i]) {
                            if (it2.lookahead == item.lookahead) {
                                std::cerr << it2.toString() << "\n";
                            }
                        }
                        std::exit(44);
                    }
                    parseTable[i][getSymbolID(item.lookahead)] = {ActionType::REDUCE, static_cast<int>((item.production - &grammar.productions[0]))};
                }
            }
        }
    }

    return parseTable;
}

void Generator::writeParseTable(const std::string& filename) {
    std::ofstream out(filename);
    if (!out) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        throw std::runtime_error("File open error");
    }

    std::vector<std::vector<Action>> parseTable = generateParseTable();
    
    /* DFA states */
    out << states.size() << "\n.TRANSITIONS\n";

    /* Parse table */
    for (int i = 0; i < parseTable.size(); i++) {
        for (int j = 0; j < parseTable[i].size(); j++) {
            Action action = parseTable[i][j];
            std::string actionRep = "error";
            switch (action.type) {
                case ActionType::SHIFT:
                    actionRep = "shift";
                    break;
                case ActionType::REDUCE:
                    actionRep = "reduce";
                    break;
                case ActionType::GOTO:
                    actionRep = "goto";
                    break;
                case ActionType::ACCEPT:
                    actionRep = "accept";
                    break;
                default:
                    break;
            }
            if (action.type != ActionType::ERROR) {
                out << i << " " << grammar[j] << " " << actionRep << " " << action.target << "\n";
            }
        }
    }

    out.close();
}

} // namespace parser