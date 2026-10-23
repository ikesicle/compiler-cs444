#ifndef PARSER_STATE_HPP
#define PARSER_STATE_HPP

namespace parser {

enum class ActionType {
    SHIFT,
    REDUCE,
    GOTO,
    ACCEPT,
    ERROR
};

struct Action {
    ActionType type;
    int target; // Could be state number or production number based on action type
};

} // namespace parser


#endif // PARSER_STATE_HPP