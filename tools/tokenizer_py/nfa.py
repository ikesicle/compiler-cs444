class NFAState:
    def __init__(self):
        self.label = None
        self.transitions = {}
        self.epsilon_transitions = set()
        self.is_accepting = False
    
    def epsilon_closure(self):
        closure = set()
        stack = [self]
        while stack:
            state = stack.pop()
            if state not in closure:
                closure.add(state)
                for next_state in state.epsilon_transitions:
                    stack.append(next_state)
        return closure

class NFA:
    def __init__(self):
        self.start_state = NFAState()
        self.states = {self.start_state}
        self.accepting_states = set()
        self.alphabet = set()
