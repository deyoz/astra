#include "AvaTemplate.h"
#include "AvaElem.h"
#define NICKNAME "ASH"
#define NICKTRACE ASH_TRACE
#include <serverlib/slogger.h>

namespace typeb_parser
{

bool AvaTemplate::isItYours(const std::string &name_str, std::string::size_type *till) const
{
    //can copy regex here, but why?
    return true;
}

TbElement * AvaTemplate::parse(const std::string & text) const
{
    return AvaElem::parse(text);
}

} // namespace typeb_parser
