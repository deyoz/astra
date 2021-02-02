#pragma once

#ifdef XP_TESTING
#include <string>

namespace xp_testing { namespace tscript {
    bool PrintDiff(const std::string& oldText, const std::string& newText);
}}

#endif // XP_TESTING
