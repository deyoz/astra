#include <vector>
#include <string>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include "AsmStrings.h"
#include "AsmTypes.h"
#include "typeb/typeb_msg.h"
#define NICKNAME "ASH"
#define NICKTRACE ASH_TRACE
#include <serverlib/slogger.h>

namespace typeb_parser {

bool AsmFlightTemplate::isItYours(const std::string & text, std::string::size_type *till) const
{
    return boost::regex_match(text, boost::regex(AsmFlightString), boost::match_any);
}

TbElement * AsmFlightTemplate::parse(const std::string & text) const
{ 
    return AsmFlightElem::parse(text); 
}

AsmFlightElem * AsmFlightElem::parse(const std::string & text)
{
    LogTrace(TRACE5) << "AsmFlightElem";
    boost::smatch what;
    if (!boost::regex_match(text, what, boost::regex(AsmFlightString), boost::match_any)) {
        LogTrace(TRACE5) << "Bad format string";
        throw typeb_parse_except(STDLOG, TBMsg::MISS_NECESSARY_ELEMENT, "Wrong string format");
    }
    std::string flightDate = what.str(1),
                legChange = what.str(6),
                newFlightDate = what.str(9),
                dei1 = what.str(14),
                dei = what.str(16),
                dei6 = what.str(19),
                dei7 = what.str(22),
                dei9 = what.str(28);

    boost::trim(flightDate);
    boost::trim(legChange);
    boost::trim(newFlightDate);
    boost::trim(dei1);
    boost::trim(dei);
    boost::trim(dei6);
    boost::trim(dei7);
    boost::trim(dei9);
    
    std::vector<std::string> strs;
    if (!dei.empty())
        boost::split(strs, dei, boost::algorithm::is_any_of(" "));
    if (!dei1.empty()) {
        strs.push_back(dei1);
    }
    if (!dei6.empty()) {
        strs.push_back(dei6);
    }
    if (!dei7.empty()) {
        strs.push_back(dei7);
    }
    if (!dei9.empty()) {
        strs.push_back(dei9);
    }
    return new AsmFlightElem(flightDate, legChange, newFlightDate, strs);
}

std::ostream & operator << (std::ostream& os, const AsmFlightElem &name)
{
    os << name.flightDate() << " " << name.legChange() << " " << name.newFlightDate(); 
    for (const std::string & s:  name.dei()) {
        os << "\nDEI:" << s;
    }
    return os;
}

bool AsmRoutingTemplate::isItYours(const std::string & text, std::string::size_type *till) const
{
    return boost::regex_match(text, boost::regex(AsmRoutingString), boost::match_any);
}

TbElement * AsmRoutingTemplate::parse(const std::string & text) const
{ 
    return AsmRoutingElem::parse(text); 
}

AsmRoutingElem * AsmRoutingElem::parse(const std::string & text)
{
    LogTrace(TRACE5) << "AsmRoutingElem";
    boost::smatch what;
    if (!boost::regex_match(text, what, boost::regex(AsmRoutingString), boost::match_any)) {
        LogTrace(TRACE5) << "Bad format string";
        throw typeb_parse_except(STDLOG, TBMsg::MISS_NECESSARY_ELEMENT, "Wrong string format");
    }    //String has correct format. Initialize needed variables
    std::string dep = what.str(1),
        dateD = what.str(2),
        airSTD = what.str(3),
        pasSTD = what.str(5),
        arr = what.str(6),
        dateA = what.str(7),
        airSTA = what.str(8),
        pasSTA = what.str(10),
        dei1 = what.str(11),
        dei = what.str(13),
        dei6 = what.str(16),
        dei7 = what.str(19),
        dei9 = what.str(25);
    
    boost::trim(dei1);
    boost::trim(dei);
    boost::trim(dei6);
    boost::trim(dei7);
    boost::trim(dei9);

    LogTrace(TRACE5) << dep << "][" << dateD << "][" 
                     << airSTD << "][/" << pasSTD << "] -> ["
                     << arr << "][" << dateA << "]["
                     << airSTA << "][/" << pasSTA << "]["
                     << dei1 << "][" << dei << "][" 
                     << dei6 << "][" << dei7 << "][" << dei9;

    std::vector<std::string> strs;
    if (!dei.empty()) {
        boost::split(strs, dei, boost::algorithm::is_any_of(" "));
    }
    if (!dei1.empty()) {
        strs.push_back(dei1);
    }
    if (!dei6.empty()) {
        strs.push_back(dei6);
    }
    if (!dei7.empty()) {
        strs.push_back(dei7);
    }
    if (!dei9.empty()) {
        strs.push_back(dei9);
    }
    return new AsmRoutingElem(dep, dateD, airSTD, pasSTD, arr, dateA, airSTA, pasSTA, strs);
}

std::ostream & operator << (std::ostream& os, const AsmRoutingElem &name)
{
    os << name.dep() << " " << name.dateD() << name.airSTD() << "/" << name.pasSTD()
       << " " << name.arr() << " " << name.dateA() << name.airSTA() << "/" << name.pasSTA(); 
    for (const std::string & s:  name.dei()) {
        os << "\nDEI:" << s;
    }
    return os;
}

} //namespace typeb_parser
