#include "tlg.h"

#include <regex>
#include <serverlib/str_utils.h>
#include <serverlib/a_ya_a_z_0_9.h>

namespace ct
{

bool typebAddressValidator(const std::string& str)
{
    static const std::regex validator("^[0-9A-Z" A_YA "]{7,8}$");
    if (str.empty()) {
        return false;
    }
    if (!std::regex_match(str, validator)) {
        return false;
    }
    return true;
}

bool ediAddressValidator(const std::string& str)
{
    const auto l = str.size();
    if (l < 1 || l > 35) {
        return false;
    }
    for (char c : str) {
        if (!isalnum(c) && !StrUtils::isalnum_rus(c)) {
            return false;
        }
    }
    return true;
}


} // ct
