#include <boost/regex.hpp>
#include "NumericsByDestTemplate.h"
#include "NumericsByDestElem.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

using namespace boost;
namespace typeb_parser
{

bool NumericsByDestTemplate::check_nextline(const std::string &line1, const std::string &currline,
                                            size_t *start, size_t *till) const
{
// VKO 001/002/000/000/001/000/000/000/000/000/002/000/000/000/
//     001/002/073/005/000/000/000 PAD 000/000/000/000/000/000/
//     000/000/000/000/000/000/000/000/000/000/000/000/000/000/
//     000

    LogTrace(TRACE3) << "line1:" << line1 << " currline:" << currline;

    if(line1.empty() || line1[line1.length()-1] != '/')
        return false;
    else
        return true;
}

bool NumericsByDestTemplate::isItYours(const std::string &nums_str, std::string::size_type *till) const
{
    LogTrace(TRACE3) << "NumericsByDestTemplate::isItYours? {" << nums_str << "}" ;
    if (!regex_match(nums_str, regex("^[A-Z€-Ÿð]{3}\\s*(\\d{1,3}/?\\s*)+.*$"), match_any))
        return false;
        else
            return true;
}

TbElement * NumericsByDestTemplate::parse(const std::string & text) const
{
    return NumericsByDestElement::parse(text);
}

} // namespace typeb_parser
