#include "SeatConfigTemplate.h"
#include "SeatConfigElem.h"

namespace typeb_parser
{
    TbElement * SeatConfigTemplate::parse(const std::string & text) const
    {
        return SeatConfigElem::parse(text);
    }
} // namespace typeb_parser
