#pragma once

#include <string>
#include <vector>

namespace ampersand::scripting {

/**
 * @brief Holds the scripts of an entity.
 */
struct Scripts {
    /** @brief The list of scripts to execute. */
    std::vector<std::string> scripts;
};

}  // namespace ampersand::scripting
