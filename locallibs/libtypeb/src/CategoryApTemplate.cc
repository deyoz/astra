#include <boost/regex.hpp>
#include "CategoryApTemplate.h"
#include "CategoryApElem.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

using namespace boost;
namespace typeb_parser
{

    bool CategoryApTemplate::isItYours(const std::string &text, std::string::size_type *till) const
    {
        if (!regex_match(text, regex("^-[A-Z€-Ÿð]{3}$"), match_any))
            return false;
        else
            return true;
    }

    TbElement * CategoryApTemplate::parse(const std::string & text) const
    {
        return CategoryApElement::parse(text);
    }

} // namespace typeb_parser
