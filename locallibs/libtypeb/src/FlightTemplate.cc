#include "FlightTemplate.h"
#include "FlightElem.h"

namespace typeb_parser
{
    TbElement * FlightTemplate::parse(const std::string & text) const
    {
        return FlightElem::parse(text);
    }
} // namespace typeb_parser
