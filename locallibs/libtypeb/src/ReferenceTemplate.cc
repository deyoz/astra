#include <boost/regex.hpp>
#include "ReferenceTemplate.h"
#include "ReferenceElem.h"
#include "SsmStrings.h"
#define NICKNAME "ASH"
#define NICKTRACE ASH_TRACE
#include <serverlib/slogger.h>

namespace typeb_parser
{

bool ReferenceTemplate::isItYours(const std::string &text, std::string::size_type *till) const
{
   boost::smatch what;
   return boost::regex_match(text, what, boost::regex(SsmReferenceString), boost::match_any);
}

TbElement * ReferenceTemplate::parse(const std::string & text) const
{
    return ReferenceElem::parse(text);
}

} // namespace typeb_parser
