#include "parser/lr1_parser.hpp"
#include "lexer/lexer.hpp"
#include "ast/type_manager.hpp"
#include "weeder/weeder.hpp"
#include "semantics/ast_parser.hpp"
#include "semantics/hierarchy_checker.hpp"
#include <cstdlib>
#include <fstream>
#include <vector>
#include <iostream>
#include <filesystem>
#include <memory>


#ifdef GRAMMAR_ONLY
#include "parser/generator.hpp"
#include "parser/grammar.hpp"
#endif

std::filesystem::path executablePath;
std::ifstream grammarStream;
#ifndef GRAMMAR_ONLY
std::ifstream tableStream;
parser::LR1Parser joosParser;
#else
parser::Grammar g = parser::Grammar::fromFile((executablePath / "tools" / "grammar.cfg").string());
parser::LR1Parser joosParser = parser::LR1Parser(g, parser::Generator(g).generateParseTable());
#endif

semantics::ASTParser astParser;


void joosc(const std::string& filename) {
    std::cerr << "Parsing file: " << filename << "\n";
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
#ifdef GRAMMAR_ONLY
    gen.writeParseTable("out.tbl");
#endif
    auto pt = joosParser.parse(tokens);
    if (pt) {
        // std::cerr << "Parse successful!\n";
        // pt->print();
        // std::cerr << "Weeding...\n";
        weeder::weed(*pt, filename);
        // std::cerr << "Weeding completed successfully.\n";
        // std::cerr << "Building AST...\n";
        auto& ast = astParser.parseNew(*pt, filename);
        // std::cerr << "AST construction completed successfully.\n";
    } else {
        std::cerr << "Parsing failed.\n";
        std::exit(42);
    }
}
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>\n";
        return 1;
    }
    executablePath = std::filesystem::path(argv[0]).parent_path();
    std::filesystem::path grammarPath = executablePath / "tools" / "grammar.cfg";
    std::filesystem::path tablePath = executablePath / "tools" / "grammar.tbl";
    grammarStream.open(grammarPath);
    std::cerr << "Grammar path: " << grammarPath << "\n";
#ifndef GRAMMAR_ONLY
    tableStream.open(tablePath);
    std::cerr << "Table path: " << tablePath << "\n";
#endif

    if (!grammarStream.is_open()) {
        std::cerr << "Error: Could not open grammar.cfg\n";
        return 1;
    }
    joosParser = parser::LR1Parser::fromStreams(grammarStream, tableStream);

    // Loop through all files provided in the command line
    for (int i = 1; i < argc; ++i) {
        std::string filename = argv[i];
        try {
            joosc(filename);
        } catch (const std::exception& e) {
            std::cerr << "Error processing file " << filename << ": " << e.what() << "\n";
            return 42; // Joos requirement for error
        }
    }

    try {
        astParser.linkTypes();
    } catch (const std::exception& e) {
        std::cerr << "Type linking failed: " << e.what() << "\n";
        return 42;
    }
    // std::cerr << "Type linking completed successfully.\n";
    
    ast::TypeManager typeManager;

    semantics::HierarchyChecker hierarchyChecker(astParser.getGlobalScope(), astParser.getASTRoots(), typeManager);

    try {
        hierarchyChecker.check();
    } catch (const std::exception& e) {
        std::cerr << "Hierarchy checking failed: " << e.what() << "\n";
        return 42;
    }

    std::cerr << "Hierarchy checking completed successfully.\n";

    std::cerr << "All files processed successfully.\n";

    return 0;
}
