#include "EndTemplate.h"
#include "EndElem.h"

namespace typeb_parser
{
    TbElement * EndTemplate::parse(const std::string & text) const
    {
        return EndElem::parse(text);
    }
} // namespace typeb_parser
