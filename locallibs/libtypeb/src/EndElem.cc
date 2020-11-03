#include <boost/regex.hpp>

#include "EndElem.h"
#include "typeb_msg.h"
#include "typeb_parse_exceptions.h"

#define NICKNAME "NONSTOP"
#include <serverlib/slogger.h>

namespace typeb_parser
{

EndElem * EndElem::parse (const std::string &txt)
{
    LogTrace(TRACE3) << "text = " << txt;

    static boost::regex re1(" *[A-Z]+ *");
    if (boost::regex_match(txt, re1)) {
        LogTrace(TRACE3) << "TRUE";
        return new EndElem(true);
    }
    static boost::regex re2(" *(END)? *PART[0-9]{1,2} *");
    if (boost::regex_match(txt, re2)) {
        LogTrace(TRACE3) << "FALSE";
        return new EndElem(false);
    }
    throw typeb_lz_parse_except(STDLOG, TBMsg::INV_FORMAT_z1z) <<
            "End element";
}


} // typeb_parser

