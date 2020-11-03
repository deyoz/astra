#include "NameTemplate.h"
#include <serverlib/isdigit.h>
#include "NameElem.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

namespace typeb_parser
{

bool NameTemplate::isItYours(const std::string &name_str, std::string::size_type *till) const
{
//     LogTrace(TRACE5) << "NameTemplate::isItYours? {" << name_str << "}" ;
    if(!name_str.empty() && ISDIGIT(name_str[0]) )
        return true;
    else
        return false;
}

TbElement * NameTemplate::parse(const std::string & text) const
{
    return NameElem::parse(text);
}

} // namespace typeb_parser
