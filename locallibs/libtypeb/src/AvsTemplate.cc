#include "typeb/AvsTemplate.h"
#include "typeb/AvsElem.h"
#define NICKNAME "ASH"
#define NICKTRACE ASH_TRACE
#include <serverlib/slogger.h>

namespace typeb_parser
{

bool AvsTemplate::isItYours(const std::string &name_str, std::string::size_type *till) const
{
    //can copy regex here, but why?
    return true;
}

TbElement * AvsTemplate::parse(const std::string & text) const
{
    return AvsElem::parse(text);
}

} // namespace typeb_parser
