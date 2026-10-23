# compiler-cs444

# Assignment 3

Error: Test Je_8_DefiniteAssignment_ComplexInitializer.java expected to fail but passed (exit code 0)
Error: Test Je_8_DefiniteAssignment_FieldWithSameName.java expected to fail but passed (exit code 0)
Error: Test Je_8_DefiniteAssignment_InitToItself.java expected to fail but passed (exit code 0)

# Assignment 2

# Assignment 1

- [ ] **Scanner** to split up input into tokens using Maximal Munch parsing with Backtracking
    - [X] Requires regular expression scanner to identify regexes for tokens and convert them into NFAs, tagging accept states of regexes with their corresponding tokens (Need to test)
    - [X] Convert NFAs into DFAs to use in scanning
    - [X] Save DFAs to a persistent file format for reuse
    - [X] Scanner must output scanned tokens into a persistent format which we can use for parsing
    - [X] Grammar to tokenize JOOS input (located in `src/tokenizer/lex.json`)
- [X] **Parser** to parse the tokenized input according to a CFG using LALR(1) parsing

## CFG File Spec
1. Terminals (count, then followed by all terminal token names. MUST MATCH in `modules/common`)
2. Non-terminals (count, then followed by all nonterminal names)
3. Productions (count, then followed by all productions where)
```
[1] [2] [3] ...
-- corresponds to --
[1] -> [2][3]...
```