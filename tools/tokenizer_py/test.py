from json2nfa import *

c = RegexToDFAConverter('lex.json')
c.buildNFAList()
dfa = c.buildCombinedDFA()
# dfa.to_cpp('lexer_tables.hpp')