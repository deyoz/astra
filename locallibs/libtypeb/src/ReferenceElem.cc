#include <list>
#include <vector>
#include <boost/regex.hpp>
#include "SsmStrings.h"
#include "ReferenceElem.h"
#include "typeb_msg.h"
#define NICKNAME "ASH"
#define NICKTRACE ASH_TRACE
#include <serverlib/slogger.h>
#include <serverlib/str_utils.h>

namespace typeb_parser
{

ReferenceElem * ReferenceElem::parse(const std::string & text) 
{
    LogTrace(TRACE5) << "ReferenceElem";
    boost::smatch what;
    if (!boost::regex_match(text, what, boost::regex(SsmReferenceString), boost::match_any)) {
        throw typeb_parse_except(STDLOG, TBMsg::MISS_NECESSARY_ELEMENT, "Wrong string format");
    }

    //String has correct format. Initialize needed variables
    std::string date(what.str(1));
    unsigned int gid = std::stoi(what[2]);
    bool last = ((what.str(3)[0] == '…' || what.str(3)[0] == 'E') ? true : false);
    unsigned int mid = std::stoi(what[4]);
    std::string creatorRef(what.str(5));
    
    return new ReferenceElem(date, gid, last, mid, creatorRef);
}

std::ostream & operator <<(std::ostream & os, const ReferenceElem & te)
{
    os  << "date[" << te.date() << "] gid[" << te.gid() << "] last[" 
        << (te.isLast() ?"y":"n") << "] mid[" << te.mid() << "] ref[" 
        << te.creatorRef() << "]";
    return os;
}

} //namespace typeb_parser
