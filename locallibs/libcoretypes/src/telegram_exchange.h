#pragma once

#include <string>
#include <iosfwd>

namespace ct
{

enum class SysRole { ETS, INV, DISTR, EMS, SMP, PAO };
inline std::ostream& operator<<(std::ostream& out, const SysRole& role)
{
    switch (role)
    {
        case SysRole::ETS:      return out << "ETS";
        case SysRole::INV:      return out << "INV";
        case SysRole::DISTR:    return out << "DISTR";
        case SysRole::EMS:      return out << "EMS";
        case SysRole::SMP:      return out << "SMP";
        case SysRole::PAO:      return out << "PAO";
    }
    return out;//can't be here
}

} /* namespace ct */

