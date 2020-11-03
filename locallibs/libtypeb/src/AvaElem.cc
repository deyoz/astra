#include <list>
#include <vector>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include "AvaElem.h"
#include "typeb_msg.h"
#include <serverlib/a_ya_a_z_0_9.h>
#define NICKNAME "ASH"
#define NICKTRACE ASH_TRACE
#include <serverlib/slogger.h>
#include <serverlib/str_utils.h>

namespace typeb_parser
{

AvaElem * AvaElem::parseUsual(const std::string & text) 
{
    std::string regexp = 
        //reid + comparment
        "^([" ZERO_NINE A_Z A_YA "]{2})([" ZERO_NINE "]{3,4})([" A_Z A_YA "]{1,2})\\s*" 
        //date and RBD count 
        "([" ZERO_NINE "]{2}[" A_Z A_YA "]{3}[" ZERO_NINE "]{2})\\s*[" ZERO_NINE "]{2}\\s*"
        //leg and RBDs
        "([" A_Z A_YA "]{6})(\\s*[" A_Z A_YA "][" ZERO_NINE "]{2})*$"; //RBDs
    boost::smatch what;
    if (!boost::regex_match(text, what, boost::regex(regexp), boost::match_any)) {
        throw typeb_parse_except(STDLOG, TBMsg::MISS_NECESSARY_ELEMENT, "Wrong string format");
    }
    //String has correct format. Initialize needed variables
    std::string airline(what.str(1));
    unsigned int fl = boost::lexical_cast<unsigned int>(what[2]);
    char comp, suffix = 0;
    //usually there is only one letter after flight number - compartment
    if(what.length(3) == 1) {
        comp = what.str(3)[0];
        //but if there are two of them, first must be flight suffix
    } else {
        suffix = what.str(3)[0];
        comp = what.str(3)[1];
    }
    std::string d(what.str(4));
    std::string leg(what.str(5));
    
    //now let's handle list of rbds availbilities
    std::string rbd_substr = std::string(what[5].second, what.suffix().first);
    std::vector< std::string > rbds;
    boost::trim(rbd_substr);
    boost::split(rbds, rbd_substr, boost::is_any_of("\t "));
    std::list< RawRbd > lst;
    
    for(std::vector<std::string>::const_iterator it = rbds.begin(); it != rbds.end(); ++it) {
        RawRbd tmp;
        //first symbol(letter) is RBD
        //2-digit number after it is availability status
        tmp.rbd = (it->c_str())[0];
        tmp.avail = boost::lexical_cast<int>(it->substr(1)); 
        lst.push_back(tmp);
    }
    return new AvaElem(airline, fl, suffix, comp, d, leg, lst);
}

AvaElem * AvaElem::parseCancellation(const std::string & text)
{
    std::string regexp = 
        //reid and slash
        "^([" ZERO_NINE A_Z A_YA "]{2})([" ZERO_NINE "]{3,4})([" A_Z A_YA "]{0,1})/" 
        //date, CN and leg
        "([" ZERO_NINE "]{2}[" A_Z A_YA "]{3}[" ZERO_NINE "]{2})\\s*CN\\s*([" A_Z A_YA "]{6})";
    boost::smatch what;
    if (!boost::regex_match(text, what, boost::regex(regexp), boost::match_any)) {
        throw typeb_parse_except(STDLOG, TBMsg::MISS_NECESSARY_ELEMENT, "Wrong string format");
    }

    //String has correct format. Initialize needed variables
    std::string airline(what.str(1));
    unsigned int fl = boost::lexical_cast<unsigned int>(what[2]);
    char comp = 0; //there is no compartment in such a string
    char suffix = 0;
    //usually there is no letter after flight number - suffix is 0
    if(what.length(3) == 1) {
        suffix = what.str(3)[0];
    }
    std::string d(what.str(4));
    std::string leg(what.str(5));
    std::list< RawRbd > lst; //not used in this type of element
    return new AvaElem(airline, fl, suffix, comp, d, leg, lst, true);
}

AvaElem * AvaElem::parse(const std::string & text) 
{
    LogTrace(TRACE5) << text;
    if(text.find("/") != std::string::npos)
        return parseCancellation(text);
    else
        return parseUsual(text);
}

std::ostream & operator <<(std::ostream & os, const AvaElem & ava)
{
    os  << "Parsed awk[" << ava.airline()
        << "] flight[" << ava.flight()
        << "] suf[" << (ava.suffix() != 0 ? ava.suffix() : ' ')
        << "] date[" << ava.date1()
        << "] leg [" << ava.leg()
        << "] cancel[" << (ava.isCancellation() ? "yes] " : "no] ") 
        << "] comp[" << (ava.isCancellation() ? ' ' : ava.compartment()) 
        << "]" << std::endl;
    std::list< RawRbd >::const_iterator it;
    for(it = ava.rbds().begin(); it != ava.rbds().end(); ++it){
        os << "Subcls " << it->rbd << " avail " << it->avail << " pushed" << std::endl;
    }
    return os;
}

} //namespace typeb_parser
