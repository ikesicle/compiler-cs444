/*
production.hpp - Grammar Production
Written by Hypatia
Ported by Ike
*/

#ifndef PARSER_PRODUCTION_HPP
#define PARSER_PRODUCTION_HPP

#include <string>
#include <vector>

namespace parser {

class Production {
public:
    std::string lhs;
    std::vector<std::string> rhs;

    inline std::string toString() const {
        std::string out = lhs + " ->";
        for (const std::string& sym : rhs) {
            out += " " + sym;
        }
        return out;
    }

    inline bool operator==(const Production& other) const {
        return lhs == other.lhs && rhs == other.rhs;
    }
};

}

#endif // PARSER_PRODUCTION_HPP