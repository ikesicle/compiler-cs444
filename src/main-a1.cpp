#include "parser/lr1_parser.hpp"
#include "lexer/lexer.hpp"
#include "weeder/weeder.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>

#ifdef GRAMMAR_ONLY
#include "parser/generator.hpp"
#include "parser/grammar.hpp"
#endif

std::filesystem::path executablePath;

void joosc(const std::string& filename) {
    
    std::vector<common::Token> tokens;
    std::ifstream infile(filename);
    if (!infile) {
        std::cerr << "Error: Could not open file " << filename << "\n";
        std::exit(42);
    }
    bool tokensuccess = lexer::Lexer::execute(infile, tokens);
    if (!tokensuccess) {
        std::cerr << "Lexing failed.\n";
        std::exit(42);
    }
    std::ifstream grammarStream(executablePath / "tools" / "grammar.cfg");
#ifndef GRAMMAR_ONLY
    std::ifstream tableStream(executablePath / "tools" / "grammar.tbl");
    parser::LR1Parser parser = parser::LR1Parser::fromStreams(grammarStream, tableStream);
#else
    parser::Grammar g = parser::Grammar::fromFile((executablePath / "tools" / "grammar.cfg").string());
    parser::Generator gen(g);
    parser::LR1Parser parser = parser::LR1Parser(g, gen.generateParseTable());
    gen.writeParseTable((executablePath / "tools" / "grammar.tbl").string());
#endif
    try {
        auto pt = parser.parse(tokens);
        if (pt) {
            std::cerr << "Parse successful!\n";
            pt->print();
            std::cerr << "Weeding...\n";
            weeder::weed(*pt, filename);
            std::cerr << "Weeding completed successfully.\n";
        } else {
            std::cerr << "Parsing failed.\n";
            std::exit(42);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error during parsing: " << e.what() << "\n";
        std::exit(42);
    }
}
int main(int argc, char* argv[]) {
    executablePath = std::filesystem::path(argv[0]).parent_path();
    if (!std::filesystem::exists(executablePath / "tools" / "grammar.cfg")) {
        std::cerr << "Error: tools/grammar.cfg not found\n";
        return 1;
    }
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>\n";
        return 1;
    }
    joosc(argv[1]);
    return 0;
}