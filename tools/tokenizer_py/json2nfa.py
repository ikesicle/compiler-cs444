import json
from nfa import NFA, NFAState
from dfa import DFA, DFAState
"""
Convert a JSON filled with regexes into a Nondeterministic Finite Automaton (NFA).
"""


class NFABuilder:
    def __init__(self):
        pass

    def makeEpsilon(self):
        nfa = NFA()
        nfa.start_state.is_accepting = True
        nfa.accepting_states.add(nfa.start_state)
        return nfa

    def makeSymbol(self, symbol):
        nfa = NFA()
        accept_state = NFAState()
        accept_state.is_accepting = True
        nfa.accepting_states.add(accept_state)
        nfa.start_state.transitions[symbol] = accept_state
        nfa.alphabet.add(symbol)
        nfa.states.add(accept_state)
        return nfa
    
    def makeSymbolGroup(self, symbols):
        nfa = NFA()
        accept_state = NFAState()
        accept_state.is_accepting = True
        nfa.accepting_states.add(accept_state)
        for symbol in symbols:
            nfa.start_state.transitions[symbol] = accept_state
            nfa.alphabet.add(symbol)
        nfa.states.add(accept_state)
        return nfa
    
    """Concatenate nfa2 onto nfa1."""
    def concatenate(self, nfa1, nfa2):
        for state in nfa1.accepting_states:
            state.is_accepting = False
            state.epsilon_transitions.add(nfa2.start_state)
        nfa1.states.update(nfa2.states)
        nfa1.accepting_states = nfa2.accepting_states
        nfa1.alphabet.update(nfa2.alphabet)
        return nfa1
    
    """Union of a list of NFAs."""
    def union(self, *nfas):
        new_nfa = NFA()
        for nfa in nfas:
            new_nfa.start_state.epsilon_transitions.add(nfa.start_state)
            new_nfa.states.update(nfa.states)
            new_nfa.accepting_states.update(nfa.accepting_states)
            new_nfa.alphabet.update(nfa.alphabet)
        return new_nfa

    """Kleene star of nfa."""
    def kleeneStar(self, nfa):
        for state in nfa.accepting_states:
            state.epsilon_transitions.add(nfa.start_state)
            state.is_accepting = False
        nfa.start_state.is_accepting = True
        nfa.accepting_states = {nfa.start_state}
        return nfa

class StringIterator:
    def __init__(self, string):
        self.string = string
        self.position = 0

    def peek(self):
        if self.position < len(self.string):
            return self.string[self.position]
        return None

    def next(self):
        if self.position < len(self.string):
            char = self.string[self.position]
            self.position += 1
            return char
        return None

    def hasNext(self):
        return self.position < len(self.string)

class RegexToDFAConverter:
    """
    Initialize regex to DFA converter with JSON regex data. Regexes should be listed in order of priority.
    Format:
    {
        "regexes": [
            {"label": "TOKEN1", "pattern": "a(b|c)*"},
            ...
        ]
    }
    """
    def __init__(self, regex_json):
        with open(regex_json, 'r') as regex_json_file:
            self.regex_data = json.load(regex_json_file)
        self.builder = NFABuilder()
        self.nfa_list = []
        self.labelPriority = []
    
    def buildNFAList(self):
        self.nfa_list = []
        for entry in self.regex_data["regexes"]:
            label = entry["label"]
            pattern = entry["pattern"]
            it = StringIterator(pattern)
            print(f"Building NFA for label: {label}, pattern: {pattern}")
            nfa = self.convertRegexToNFA(it)
            for state in nfa.accepting_states:
                state.label = label
            self.nfa_list.append(nfa)
            self.labelPriority.append(label)
        return self.nfa_list
    
    def readCharacterOrEscape(self, patternIterator: StringIterator) -> str:
        char = patternIterator.next()
        if char == "\\":
            next_char = patternIterator.next()
            if next_char is None:
                raise ValueError("Unexpected end of pattern after escape character")
            if next_char == "n":
                return "\n"
            elif next_char == "r":
                return "\r"
            elif next_char == "t":
                return "\t"
            elif next_char == "f":
                return "\f"
            else:
                return next_char
        return char

    def convertRegexToNFA(self, patternIterator: StringIterator) -> NFA:
        """
        Need to handle the following:
        - Single symbols
        - Character classes
        - Concatenation
        - Union (should only be handled at the end of a block)
        - Kleene star
        - Parentheses for grouping
        """
        unionComponents = []
        confirmed = self.builder.makeEpsilon()
        lastNFA = None
        while patternIterator.hasNext():
            match patternIterator.peek():
                case "(":
                    # Handle inner expression
                    patternIterator.next()  # consume '('
                    print("Starting inner expression")
                    nfa = self.convertRegexToNFA(patternIterator)
                    print("Finished inner expression")
                    if patternIterator.peek() == ")":
                        patternIterator.next()  # consume ')'
                    else:
                        raise ValueError("Unmatched parenthesis")
                    if lastNFA:
                        confirmed = self.builder.concatenate(confirmed, lastNFA)
                    lastNFA = nfa
                case ")":
                    # End of current group
                    break
                case "[":
                    # Character class
                    patternIterator.next()  # consume '['
                    char_set = []
                    while patternIterator.peek() != "]":
                        char_set.append(self.readCharacterOrEscape(patternIterator))
                        if patternIterator.peek() == "-":
                            patternIterator.next()  # consume '-'
                            start_char = char_set.pop()
                            end_char = patternIterator.next()
                            if end_char is None or end_char == "]":
                                raise ValueError("Unexpected end to character range")
                            for c in range(ord(start_char), ord(end_char) + 1):
                                char_set.append(chr(c))
                    patternIterator.next()  # consume ']'
                    print(f"Symbol group consisting of characters: {char_set}")
                    nfa = self.builder.makeSymbolGroup(char_set)
                    if lastNFA:
                        confirmed = self.builder.concatenate(confirmed, lastNFA)
                    lastNFA = nfa
                case "*":
                    patternIterator.next()  # consume '*'
                    if lastNFA is None:
                        raise ValueError("Kleene star applied to nothing")
                    lastNFA = self.builder.kleeneStar(lastNFA)
                case "|":
                    patternIterator.next()  # consume '|'
                    if lastNFA:
                        confirmed = self.builder.concatenate(confirmed, lastNFA)
                    lastNFA = None
                    unionComponents.append(confirmed)
                    confirmed = self.builder.makeEpsilon()
                case _:
                    # Single symbol
                    symbol = self.readCharacterOrEscape(patternIterator)
                    print(f"Read char/escape: {symbol} (value: {ord(symbol)})")
                    nfa = self.builder.makeSymbol(symbol)
                    if lastNFA:
                        confirmed = self.builder.concatenate(confirmed, lastNFA)
                    lastNFA = nfa
        if lastNFA:
            confirmed = self.builder.concatenate(confirmed, lastNFA)
        unionComponents.append(confirmed)
        # Fold union components
        return self.builder.union(*unionComponents)
    
    def buildCombinedDFA(self) -> DFA:
        combined_nfa = self.builder.union(*self.nfa_list)
        # Convert combined NFA to DFA (subset construction)
        dfa = DFA()
        start_set = frozenset(combined_nfa.start_state.epsilon_closure())
        # print(f"Start set has {len(start_set)} NFA states")
        state_map = {start_set: dfa.create_state("START")}
        unmarked_states = [start_set]
        while unmarked_states:
            # print(f"Processing state {state_map[unmarked_states[0]]} with {len(unmarked_states[0])} NFA states")
            current_set = unmarked_states.pop()
            current_state_id = state_map[current_set]
            dfa_state = dfa.get_state(current_state_id)
            # Check if any NFA states in current_set are accepting
            accepting_labels = {state.label for state in current_set if state.is_accepting}
            if accepting_labels:
                dfa_state.is_accepting = True
                dfa_state.token_type = sorted(accepting_labels, key=lambda x: self.labelPriority.index(x))[0]  # Choose one label (priority)
            # Find transitions for each symbol
            symbol_map = {}
            for nfa_state in current_set:
                for symbol, target in nfa_state.transitions.items():
                    if symbol not in symbol_map:
                        symbol_map[symbol] = set()
                    symbol_map[symbol].add(target)
            # print(symbol_map)
            # symbol_map now maps symbols to sets of NFA states
            for symbol, target_states in symbol_map.items():
                closure_set = frozenset(set().union(*(state.epsilon_closure() for state in target_states)))
                if closure_set not in state_map:
                    state_map[closure_set] = dfa.create_state(f"STATE_{len(state_map)}")
                    unmarked_states.append(closure_set)
                next_state_id = state_map[closure_set]
                dfa_state.add_transition(symbol, next_state_id)
        print(f"Construction finished. Stats: {len(dfa.states)} DFA states created")
        return dfa
    

    
