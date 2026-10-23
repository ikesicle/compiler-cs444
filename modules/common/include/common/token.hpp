/*
token.hpp
Written by Ike Yang
*/

#ifndef COMMON_TOKEN_HPP
#define COMMON_TOKEN_HPP

#include "token_type.hpp"
#include <string>

namespace common {

struct Token {
    TokenType type;
    std::string lexeme;
};

} // namespace common


#endif