#include "AutomPNRAddrTemplate.h"
#include "AutomPNRAddr.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

namespace typeb_parser
{

TbElement * AutomPNRAddrTemplate::parse(const std::string & text) const
{
    return AutomPNRAddr::parse(text);
}

} // namespace typeb_parser
