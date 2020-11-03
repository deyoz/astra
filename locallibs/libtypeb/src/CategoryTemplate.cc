#include <boost/regex.hpp>
#include "CategoryTemplate.h"
#include "CategoryElem.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

using namespace boost;
namespace typeb_parser
{

    bool CategoryTemplate::isItYours(const std::string &text, std::string::size_type *till) const
    {
        if (!regex_match(text, regex("^[A-Z€-Ÿð]{5}\\s*\\d{1,3}[A-ZA-Ÿð]{1}$"), match_any))
            return false;
        else
            return true;
    }

    TbElement * CategoryTemplate::parse(const std::string & text) const
    {
        return CategoryElement::parse(text);
    }

} // namespace typeb_parser
