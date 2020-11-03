#include <list>
#include <vector>
#include <regex>
#include <boost/algorithm/string.hpp>
#include "typeb/AvsElem.h"
#include "typeb/typeb_msg.h"
#include <serverlib/a_ya_a_z_0_9.h>
#define NICKNAME "ASH"
#define NICKTRACE ASH_TRACE
#include <serverlib/slogger.h>
#include <serverlib/str_utils.h>

namespace typeb_parser
{

AvsElem * AvsElem::parse(const std::string & text) 
{
    static std::regex regexp(
        //reid + rbd. 
        "^([" ZERO_NINE A_Z A_YA "]{2}[" A_Z A_YA "]?)\\s*([" ZERO_NINE "]{3,4})([" A_Z A_YA "]?)([/|" A_Z A_YA "])\\s*" 
        //date  
        "([" ZERO_NINE "]{2}[" A_Z A_YA "]{3}(?:[" ZERO_NINE "]{2})?)\\s*"
        //status code
        "(A[" ZERO_NINE "]|AS|CR|CL|CC|CN|CS|LR|LL|LC|LA|LN|L[" ZERO_NINE "])\\s*"
        //leg
        "([" A_Z A_YA "]{6})?\\s*$"
    );
    std::smatch what;
    if (!std::regex_match(text, what, regexp, std::regex_constants::match_any)) {
        throw typeb_parse_except(STDLOG, TBMsg::MISS_NECESSARY_ELEMENT, "Wrong string format");
    }
    //String has correct format. Initialize needed variables
    std::string airline(what.str(1));
    unsigned int fl = std::stoi(what[2]);
    char suffix = what.str(3)[0];
    char rbd = what.str(4)[0];
    std::string d(what.str(5));
    std::string status(what.str(6));
    std::string leg(what.str(7));

    return new AvsElem(airline, fl, suffix, rbd, rbd == '/', d, status, leg);
}

std::ostream & operator <<(std::ostream & os, const AvsElem & av)
{
    os  << "Parsed awk[" << av.airline()
        << "] flight[" << av.flight()
        << "] suf[" << (av.suffix() != 0 ? av.suffix() : ' ')
        << "] date[" << av.date1()
        << "] st[" << av.status()
        << "] leg[" << av.leg()
        << "] subcls[" << av.rbd() << "]" 
        << "] forAll[" << (av.statusForAllRbd() ? "Y]" : "N]"); 
    return os;
}

} //namespace typeb_parser
