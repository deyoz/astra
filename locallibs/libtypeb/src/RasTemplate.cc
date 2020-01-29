#include "typeb/RasTemplate.h"
#include "typeb/RasElem.h"

namespace typeb_parser {

bool RasTemplate::isItYours(const std::string &name_str, std::string::size_type *till) const
{
    //can copy regex here, but why?
    return true;
}

TbElement * RasTemplate::parse(const std::string & text) const
{
    return RasElem::parse(text);
}

} // namespace typeb_parser
