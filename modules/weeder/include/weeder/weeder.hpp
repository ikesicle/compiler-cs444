#pragma once
#include "common/parse_tree.hpp"
#include <string>

namespace weeder {
    void weed(common::ParseTree& root, const std::string& filePath);
} // namespace weeder


