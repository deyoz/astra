#include <list>
#include <vector>
#include <boost/regex.hpp>
#include "typeb/RasElem.h"
#include "typeb/typeb_msg.h"
#include <serverlib/a_ya_a_z_0_9.h>

#define NICKNAME "DAG"
#include <serverlib/slogger.h>
#include <serverlib/str_utils.h>

namespace typeb_parser {

RasElem::RasElem(const std::string& flt, const std::string& startDt, const std::string& endDt, const std::string& seg, bool closed)
    : flight_(flt), startDate_(startDt), endDate_(endDt), segment_(seg), closedOnly_(closed)
{}

RasElem * RasElem::parse(const std::string & text)
{
    static boost::regex rx(
        //flight
        "^([" ZERO_NINE A_Z A_YA "]{2}[" A_Z A_YA "]?)[/]?([" ZERO_NINE "]{3,4})([" A_Z A_YA "]?)/"
        //start date
        "(\\d{2}[" A_Z A_YA "]{3}(?:\\d{2})?)/"
        //optional end date
        "(\\d{2}[" A_Z A_YA "]{3}(?:\\d{2})?)?/"
        //segment
        "([" A_Z A_YA "]{6})"
        //option
        "(?:/([C]))?"
        "\\s*$"
    );

    LogTrace(TRACE5) << "Parse: [" << text << "]";
    boost::smatch m;
    if (!boost::regex_match(text, m, rx)) {
        throw typeb_parse_except(STDLOG, TBMsg::MISS_NECESSARY_ELEMENT, "Wrong string format");
    }
    return new RasElem(
        m.str(1) + m.str(2) + m.str(3),
        m[4], m[5], m[6], m[7].matched
    );
}

std::ostream & operator <<(std::ostream & os, const RasElem & re)
{
    return os << "flight[" << re.flight()
       << "] dt1[" << re.startDate()
       << "] dt2[" << re.endDate()
       << "] seg[" << re.segment()
       << "] option [" << re.isClosedOnly() << "]";
}

} //namespace typeb_parser
