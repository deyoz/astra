#include "RvrTemplate.h"
#include "RvrElem.h"
#define NICKNAME "ASH"
#define NICKTRACE ASH_TRACE
#include <serverlib/slogger.h>

namespace typeb_parser
{

bool RvrTemplate::isItYours(const std::string &name_str, std::string::size_type *till) const
{
    //can copy regex here, but why?
    return true;
}

TbElement * RvrTemplate::parse(const std::string & text) const
{
    return RvrElem::parse(text);
}

} // namespace typeb_parser
