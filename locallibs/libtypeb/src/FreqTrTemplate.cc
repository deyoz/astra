#include "FreqTrTemplate.h"
#include "FreqTrElem.h"

namespace typeb_parser
{
    TbElement * FreqTrTemplate::parse(const std::string & text) const
    {
        return FreqTrElem::parse(text);
    }
} // namespace typeb_parser
