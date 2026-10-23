/*
lexer.hpp - Joos Lexer
Written by Ike Yang
*/

#ifndef LEXER_LEXER_HPP
#define LEXER_LEXER_HPP

#include "checkpoint_stream.hpp"
#include <ios>
#include <iostream>
#include <vector>
#include "lexer_tables.hpp"
#include "common/token.hpp"

using namespace common;

namespace lexer {

class Lexer {
public:
    static bool execute(std::istream& input, std::vector<Token>& out) {
        CheckpointStream cpstream(input);
        TokenType lastValidType;
        int state = START_STATE;
        std::string cLexeme = "", lastValidLexeme = "";
        cpstream.SetCheckpoint();
        int ic;
        char c;
        out.clear();
        input >> std::noskipws;
        while (cpstream.ok()) {
            cpstream >> ic;
            if (ic == EOF) {
                c = '\n';
            } else {
                c = static_cast<char>(ic);
            }
            state = DFA_TABLE[state][c];
            // std::cerr << "Read char: '" << c << "' (" << (int)ic << "), now in state " << state << " corresponding to '" << (state == -1 ? "INVALID" : TOKEN_TYPE_NAMES[(int)TOKEN_TYPES[state]]) << "'\n";
            cLexeme += c;
            if (state == -1) { // Reject
                cpstream.GotoCheckpoint();
                if (lastValidType == TokenType::NONE) {
                    std::cerr << "Lexical Error: Unrecognized token '" << cLexeme << "'\n";
                    return false;
                }
                if (lastValidType != TokenType::WHITESPACE && lastValidType != TokenType::COMMENTLINE && lastValidType != TokenType::COMMENTBLOCK) {
                    out.push_back({
                        .type = lastValidType,
                        .lexeme = lastValidLexeme
                    });
                }
                // std::cerr << TOKEN_TYPE_NAMES[(int)lastValidType] << " " << lastValidLexeme << "\n";
                lastValidType = TokenType::NONE;
                lastValidLexeme = "";
                cLexeme = "";
                state = START_STATE;
            } else if (IS_ACCEPTING[state]) {
                lastValidLexeme = cLexeme;
                lastValidType = TOKEN_TYPES[state];
                cpstream.SetCheckpoint();
            }
        }
        out.push_back({
            .type = TokenType::END_FILE,
            .lexeme = ""
        });
        return true;
    }

};

}; // namespace lexer


#endif