#include "RemarkTemplate.h"
#include <serverlib/isdigit.h>
#include "ssr_parser.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

namespace typeb_parser
{
    TbElement * RemarkTemplate::parse(const std::string & text) const
    {
        return RemarkParsers::parse(Remark::RemTag, text);
    }

} // namespace typeb_parser
