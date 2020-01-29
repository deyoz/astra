#include "TotalsByDestTemplate.h"
#include "TotalsByDestElem.h"

namespace typeb_parser
{
    TbElement * TotalsByDestTemplate::parse(const std::string & text) const
    {
        return TotalsByDestElem::parse(text);
    }
} // namespace typeb_parser
