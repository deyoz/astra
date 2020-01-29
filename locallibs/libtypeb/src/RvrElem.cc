#include <list>
#include <vector>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include "RvrElem.h"
#include "typeb_msg.h"
#include <serverlib/a_ya_a_z_0_9.h>
#define NICKNAME "ASH"
#define NICKTRACE ASH_TRACE
#include <serverlib/slogger.h>
#include <serverlib/str_utils.h>

namespace typeb_parser
{

RvrElem * RvrElem::parse(const std::string & text) 
{
    std::string regexp = 
        //reid
        "^([" ZERO_NINE A_Z A_YA "]{2})([" ZERO_NINE "]{3,4})([" A_Z A_YA "]?)/\\s*" 
        //first date
        "([" ZERO_NINE "]{2}[" A_Z A_YA "]{3}[" ZERO_NINE "]{2})\\s*-\\s*"
        //second date
        "([" ZERO_NINE "]{2}[" A_Z A_YA "]{3}[" ZERO_NINE "]{2})\\s*/?\\s*"
        //frequency
        "([" ZERO_NINE "]{0,7})\\s*$";
    boost::smatch what;
    if (!boost::regex_match(text, what, boost::regex(regexp), boost::match_any)) {
        throw typeb_parse_except(STDLOG, TBMsg::MISS_NECESSARY_ELEMENT, "Wrong string format");
    }
    //String has correct format. Initialize needed variables
    std::string airline(what.str(1));
    unsigned int fl = std::stoi(what[2]);
    char suffix = 0;
    if(what.length(3) == 1) {
        suffix = what.str(3)[0];
    }
    std::string d1(what.str(4));
    std::string d2(what.str(5));
    std::string freq(what.str(6));
    return new RvrElem(airline, fl, suffix, d1, d2, freq);
}

std::ostream & operator <<(std::ostream & os, const RvrElem & rvr)
{
    os  << "Parsed awk[" << rvr.airline()
        << "] flight[" << rvr.flight()
        << "] suf[" << (rvr.suffix() != 0 ? rvr.suffix() : ' ')
        << "] date[" << rvr.firstDate()
        << "-" << rvr.secondDate()
        << "] freq[" << rvr.freq() 
        << "]" << std::endl;
    return os;
}

} //namespace typeb_parser
