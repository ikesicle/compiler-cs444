#include "common/token_type.hpp"
#include "lexer/lexer.hpp"
#include <fstream>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }

    std::ifstream inputFile(argv[1]);
    if (!inputFile.is_open()) {
        std::cerr << "Error opening file: " << argv[1] << std::endl;
        return 1;
    }

    std::vector<common::Token> tokens;
    lexer::Lexer::execute(inputFile, tokens);
    for (const auto& token : tokens) {
        std::cout << TOKEN_TYPE_NAMES[(int)token.type] << " " << token.lexeme << std::endl;
    }
    return 0;
}