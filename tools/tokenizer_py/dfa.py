from typing import List, Tuple, Optional, Dict


class DFAState:
    """Represents a state in a DFA."""
    
    def __init__(self, state_id: int, is_accepting: bool = False, token_type: Optional[str] = None):
        """
        Initialize a DFA state.
        
        Args:
            state_id: Unique identifier for this state
            is_accepting: Whether this is an accepting/final state
            token_type: The token type if this is an accepting state
        """
        self.state_id = state_id
        self.is_accepting = is_accepting
        self.token_type = token_type
        self.transitions: List[Tuple[str, int]] = []
    
    def add_transition(self, token: str, next_state: int):
        """
        Add a transition from this state.
        
        Args:
            token: The input symbol/token that triggers this transition
            next_state: The ID of the state to transition to
        """
        self.transitions.append((token, next_state))
    
    def get_next_state(self, token: str) -> Optional[int]:
        """
        Get the next state for a given input token.
        
        Args:
            token: The input symbol/token
            
        Returns:
            The next state ID, or None if no transition exists
        """
        for trans_token, next_state in self.transitions:
            if trans_token == token:
                return next_state
        return None
    
    def __repr__(self):
        return f"DFAState(id={self.state_id}, accepting={self.is_accepting}, transitions={self.transitions})"


class DFA:
    """Represents a Deterministic Finite Automaton."""
    
    def __init__(self, start_state: int = 0):
        """
        Initialize a DFA.
        
        Args:
            start_state: The ID of the start state (default: 0)
        """
        self.states: List[DFAState] = []
        self.state_name_map: Dict[int, str] = {}
        self.start_state = start_state
    
    def create_state(self, name: str) -> int:
        """
        Add a state to the DFA.
        
        Args:
            name: The name of the state to add
        """
        self.states.append(DFAState(len(self.states)))
        self.state_name_map[len(self.states) - 1] = name
        return len(self.states) - 1
    
    def get_state(self, state_id: int) -> Optional[DFAState]:
        """
        Get a state by its ID.
        
        Args:
            state_id: The ID of the state to retrieve
            
        Returns:
            The DFAState object, or None if not found
        """
        return self.states[state_id] if 0 <= state_id < len(self.states) else None

    def get_state_by_name(self, name: str) -> Optional[DFAState]:
        """
        Get a state by its ID.
        
        Args:
            state_id: The ID of the state to retrieve
            
        Returns:
            The DFAState object, or None if not found
        """
        state_id = self.state_name_map.get(name)
        return self.states[state_id] if state_id is not None and 0 <= state_id < len(self.states) else None

    def try_run(self, token: str):
        """
        Try to run the DFA on a given input token from the start state. 
        
        Args:
            token: The input symbol/token
            
        Returns:
            The next state ID if a transition exists, otherwise None
        """
        current_state = self.get_state(self.start_state)
        for trans_chr in token:
            if current_state is None:
                print(f"Failed while processing {trans_chr}")
                return None
            print(f"At state {current_state.state_id} -> {trans_chr} (accepting={current_state.is_accepting}, token_type={current_state.token_type})")
            next_state_id = current_state.get_next_state(trans_chr)
            if next_state_id is None:
                print(f"Failed while processing {trans_chr}")
                return None
            current_state = self.get_state(next_state_id)
        print(f"Ended in state with token type {current_state.token_type if current_state else 'None'} (accepting={current_state.is_accepting if current_state else 'N/A'})")
    
    def to_cpp(self, outfile, namespace: str = "lexer", error_state: int = -1):
        """
        Export the DFA as C++ code with static transition tables.
        
        Args:
            namespace: C++ namespace to use (default: "lexer")
            error_state: Value to use for invalid transitions (default: -1)
            
        Returns:
            C++ code as a string
        """
        num_states = len(self.states)
        ascii_size = 128
        
        # Build transition table
        table = [[error_state] * ascii_size for _ in range(num_states)]
        for state in self.states:
            for symbol, next_state in state.transitions:
                if len(symbol) == 1 and ord(symbol) < ascii_size:
                    table[state.state_id][ord(symbol)] = next_state
        
        # Build accepting states and token types arrays
        accepting = [state.is_accepting for state in self.states]
        token_types = [state.token_type if state.token_type else "" for state in self.states]
        
        # Generate unique token type enum
        unique_tokens = sorted(set(tt for tt in token_types if tt))
        
        # Generate C++ code
        cpp = []
        cpp.append(f"// Auto-generated DFA transition table")
        cpp.append(f"// States: {num_states}, Alphabet: ASCII (0-{ascii_size-1})")
        cpp.append(f"")
        cpp.append(f"namespace {namespace} {{")
        cpp.append(f"")
        
        # Token type enum
        cpp.append(f"enum class TokenType {{")
        cpp.append(f"    NONE,")
        for token in unique_tokens:
            cpp.append(f"    {token},")
        cpp.append(f"}};")
        cpp.append(f"")
        
        # Transition table
        cpp.append(f"// DFA transition table[state][char] = next_state")
        cpp.append(f"// {error_state} indicates no transition (error state)")
        cpp.append(f"static const int DFA_TABLE[{num_states}][{ascii_size}] = {{")
        for i, row in enumerate(table):
            cpp.append(f"    {{ " + ", ".join(f"{val:3d}" for val in row) + " }" + ("," if i < num_states - 1 else ""))
        cpp.append("};")
        cpp.append(f"")
        
        # Accepting states
        cpp.append(f"// Accepting states")
        cpp.append(f"static const bool IS_ACCEPTING[{num_states}] = {{")
        cpp.append(f"    " + ", ".join("true" if acc else "false" for acc in accepting))
        cpp.append(f"}};")
        cpp.append(f"")
        
        # Token types
        cpp.append(f"// Token type for each accepting state")
        cpp.append(f"static const TokenType TOKEN_TYPES[{num_states}] = {{")
        token_enum_map = {token: f"TokenType::{token}" for token in unique_tokens}
        token_enum_map[""] = "TokenType::NONE"
        cpp.append(f"    " + ", ".join(token_enum_map[tt] for tt in token_types))
        cpp.append(f"}};")
        cpp.append(f"")
        
        # Start state
        cpp.append(f"// Start state")
        cpp.append(f"static const int START_STATE = {self.start_state};")
        cpp.append(f"")

        # Token type to string mapping
        cpp.append(f"// String mappings for debugging")
        cpp.append(f"static const char* TOKEN_TYPE_NAMES[] = {{")
        cpp.append(f'    "NONE",')
        for token in unique_tokens:
            cpp.append(f'    "{token}",')
        cpp.append(f"}};")
        cpp.append(f"")
        
        cpp.append(f"}} // namespace {namespace}")
        
        outstr = "\n".join(cpp)
        with open(outfile, "w") as f:
            f.write(outstr)