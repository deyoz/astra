#include <vector>
#include <string>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include "SsmElem.h"
#include "SsmStrings.h"
#include "typeb/typeb_msg.h"
#define NICKNAME "ASH"
#define NICKTRACE ASH_TRACE
#include <serverlib/slogger.h>

namespace typeb_parser
{

SsmFlightElem * SsmFlightElem::parse(const std::string & text) 
{
    static const boost::regex rx(SsmFlightString);

    LogTrace(TRACE5) << "SsmFlightElem";
    boost::smatch what;
    if (!boost::regex_match(text, what, rx, boost::match_any)) {
        tst();
        throw typeb_parse_except(STDLOG, TBMsg::MISS_NECESSARY_ELEMENT, "Wrong string format");
    }
    std::string fl = what.str(1), 
                dei1 = what.str(2);
    
    std::vector< std::string > strs;
    std::string d = what.str(4);
    //dei -- old code handles only 2/ and 9/ type.
    boost::trim(d); //remove first whitespace
    boost::trim(dei1);
    LogTrace(TRACE5) << fl << "][" << d;
    if (!d.empty()) {
        boost::split(strs, d, boost::algorithm::is_from_range(' ',' '));
    }
    if (!dei1.empty())
        strs.push_back(dei1);
    return new SsmFlightElem(fl, strs);
}

SsmRevFlightElem * SsmRevFlightElem::parse(const std::string & text)
{
    static const boost::regex rx(SsmRevFlightString);

    LogTrace(TRACE5) << "SsmRevFlightElem";
    boost::smatch what;
    if (!boost::regex_match(text, what, rx, boost::match_any)) {
        tst();
        throw typeb_parse_except(STDLOG, TBMsg::MISS_NECESSARY_ELEMENT, "Wrong string format");
    }
    //String has correct format. Initialize needed variables
    std::string fl = what.str(1);
    std::string dt1 = what.str(2), 
                dt2 = what.str(4), 
                freq = what.str(6), 
                freqRate = what.str(7);

    boost::trim(freqRate); //remove leading whitesp
    LogTrace(TRACE5) << fl << "][" << dt1 << "][" << dt2 << "][" << freq << "][" << freqRate;  
    return new SsmRevFlightElem(fl, dt1, dt2, freq, freqRate);
}

SsmLongPeriodElem * SsmLongPeriodElem::parse(const std::string & text) 
{
    static const boost::regex rx(SsmLongPeriodString);
    LogTrace(TRACE5) << "SsmLongPeriodElem";
    boost::smatch what;
    if (!boost::regex_match(text, what, rx, boost::match_any)) {
        tst();
        throw typeb_parse_except(STDLOG, TBMsg::MISS_NECESSARY_ELEMENT, "Wrong string format");
    }
    //String has correct format. Initialize needed variables
    std::string dt1 = what.str(1),
                dt2 = what.str(3), 
                freq = what.str(5),
                freqRate = what.str(6),
                dei1 = what.str(7),
                dei6 = what.str(9);
    std::string d = what.str(12);
    
    boost::trim(dei1);
    boost::trim(dei6);
    boost::trim(freqRate); //remove leading whitesp
    boost::trim(d); //remove first whitespace
    
    std::vector< std::string > strs;
    LogTrace(TRACE5) << dt1 << "][" << dt2 << "][" << freq << "]["
                     << freqRate << "][" << dei1 << "][" << dei6 << "][" << d;
    if (!d.empty()) {
        boost::split(strs, d, boost::algorithm::is_from_range(' ', ' '));
    }
    if (!dei1.empty())
        strs.push_back(dei1);
    if (!dei6.empty())
        strs.push_back(dei6);
                      
    return new SsmLongPeriodElem(dt1, dt2, freq, freqRate, strs);
}

SsmSkdPeriodElem * SsmSkdPeriodElem::parse(const std::string & text)
{
    static const boost::regex rx(SsmSkdPeriodString);

    LogTrace(TRACE5) << "SsmSkdPeriodElem";
    boost::smatch what;
    if (!boost::regex_match(text, what, rx, boost::match_any)) {
        tst();
        throw typeb_parse_except(STDLOG, TBMsg::MISS_NECESSARY_ELEMENT, "Wrong string format");
    }
    //String has correct format. Initialize needed variables
    std::string eD = what.str(1);
    std::string dD= what.str(3);
    boost::trim(dD);
    return new SsmSkdPeriodElem(eD, dD);
}

SsmShortFlightElem * SsmShortFlightElem::parse(const std::string & text) 
{
    static const boost::regex rx(SsmShortFlightString);

    LogTrace(TRACE5) << "SsmShortFlightElem";
    boost::smatch what;
    if (!boost::regex_match(text, what, rx, boost::match_any)) {
        tst();
        throw typeb_parse_except(STDLOG, TBMsg::MISS_NECESSARY_ELEMENT, "Wrong string format");
    }
    //String has correct format. Initialize needed variables
    return new SsmShortFlightElem(what.str(1));
}

SsmEquipmentElem * SsmEquipmentElem::parse(const std::string & text)
{
    static const boost::regex rx(SsmEquipmentString);

    LogTrace(TRACE5) << "SsmEquipmentElem";
    boost::smatch what;
    if (!boost::regex_match(text, what, rx, boost::match_any)) {
        tst();
        throw typeb_parse_except(STDLOG, TBMsg::MISS_NECESSARY_ELEMENT, "Wrong string format");
    }
    //String has correct format. Initialize needed variables
    char sType = what.str(1)[0];
    std::string craftType = what.str(2),
        airConfig = what.str(3),
        airReg = what.str(8),
        deiF = what.str(9), //dei -- old code handles only 2/ and 9/ type.
        dei6 = what.str(12),
        dei9 = what.str(15);
    boost::trim(airReg);
    boost::trim(deiF);
    boost::trim(dei6);
    boost::trim(dei9);

    LogTrace(TRACE5) << sType << "][" << craftType << "][" 
                     << airConfig << "][" << deiF << "]["
                     << dei6 << "][" << dei9 << ']';
    std::vector< std::string > strs;
    if (!deiF.empty()) {
        boost::split(strs, deiF, boost::algorithm::is_any_of(" "));
    }
    if (!dei6.empty()) {
        strs.push_back(dei6);
    }
    if (!dei9.empty()) {
        strs.push_back(dei9);
    }
    return new SsmEquipmentElem(sType, craftType, airConfig, airReg, strs);
}

SsmRoutingElem * SsmRoutingElem::parse(const std::string & text)
{
    static const boost::regex rx(SsmRoutingString);

    LogTrace(TRACE5) << "SsmRoutingElem";
    boost::smatch what;
    if (!boost::regex_match(text, what, rx, boost::match_any)) {
        tst();
        throw typeb_parse_except(STDLOG, TBMsg::MISS_NECESSARY_ELEMENT, "Wrong string format");
    }
    //String has correct format. Initialize needed variables
    std::string dep = what.str(1),
        airSTD = what.str(2),
        varSTD = what.str(4),
        pasSTD = what.str(6),
        arr = what.str(7),
        airSTA = what.str(8),
        varSTA = what.str(10),
        pasSTA = what.str(12),
        dei1 = what.str(13),
        dei = what.str(15),
        dei6 = what.str(18),
        dei7 = what.str(21),
        dei9 = what.str(27);
    boost::trim(dei1);
    boost::trim(dei);
    boost::trim(dei6);
    boost::trim(dei7);
    boost::trim(dei9);

    LogTrace(TRACE5) << dep << "][" << airSTD << "][" 
                     << varSTD << "][" << pasSTD << "]["
                     << arr << "][" << airSTA << "]["
                     << varSTA << "][" << pasSTA << "]["
                     << dei1 << "][" << dei << "][" 
                     << dei6 << "][" << dei7 << "][" << dei9;

    std::vector< std::string > strs;
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

    return new SsmRoutingElem(dep, airSTD, varSTD, pasSTD, arr, airSTA, varSTA, pasSTA, strs);
}

SsmLegChangeElem * SsmLegChangeElem::parse(const std::string & text)
{
    static const boost::regex rx(SsmLegChangeString);

    LogTrace(TRACE5) << "SsmLegChangeElem";
    boost::smatch what;
    if (!boost::regex_match(text, what, rx, boost::match_any)) {
        tst();
        throw typeb_parse_except(STDLOG, TBMsg::MISS_NECESSARY_ELEMENT, "Wrong string format");
    }

    std::string legsChange = what.str(1),
        dei1 = what.str(3),
        dei = what.str(5),
        dei6 = what.str(8),
        dei7 = what.str(11),
        dei9 = what.str(17);
    boost::trim(dei1);
    boost::trim(dei);
    boost::trim(dei6);
    boost::trim(dei7);
    boost::trim(dei9);

    std::vector< std::string > strs;
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

    return new SsmLegChangeElem(legsChange, strs);
}

SsmSegmentElem * SsmSegmentElem::parse(const std::string & text)
{
    static const boost::regex rx(SsmSegmentString);

    LogTrace(TRACE5) << "SsmSegmentElem";
    boost::smatch what;
    if (!boost::regex_match(text, what, rx, boost::match_any)) {
        throw typeb_parse_except(STDLOG, TBMsg::MISS_NECESSARY_ELEMENT, "Wrong string format");
    }
    //String has correct format. Initialize needed variables
    std::string seg = what.str(1), dei = what.str(2);
    return new SsmSegmentElem(seg, dei);
}

SsmSupplInfoElem * SsmSupplInfoElem::parse(const std::string & text)
{
    static const boost::regex rx(SsmSupplInfoString);

    LogTrace(TRACE5) << "SsmSupplInfoElem";
    boost::smatch what;
    if (!boost::regex_match(text, what, rx, boost::match_any)) {
        tst();
        throw typeb_parse_except(STDLOG, TBMsg::MISS_NECESSARY_ELEMENT, "Wrong string format");
    }
    //String has correct format. Initialize needed variables
    std::string txt = what.str(1);
    return new SsmSupplInfoElem(txt);
}

SsmDontCareElem * SsmDontCareElem::parse(const std::string & text)
{
    LogTrace(TRACE5) << "SsmDontCareElem";
    //old code just yells "our tlg was rejected!" and returns.
    //so i suppose existence is just fine
    return new SsmDontCareElem();
}

SsmRejectInfoElem * SsmRejectInfoElem::parse(const std::string & text)
{
    static const boost::regex rx(SsmRejectInfoString);

    LogTrace(TRACE5) << "SsmRejectInfoElem";
    boost::smatch what;
    if (!boost::regex_match(text, what, rx, boost::match_any)) {
        throw typeb_parse_except(STDLOG, TBMsg::MISS_NECESSARY_ELEMENT, "Wrong string format");
    }
    //String has correct format. Initialize needed variables
    std::string txt = what.str(0);
    return new SsmRejectInfoElem(txt);
}

SsmEmptyElem * SsmEmptyElem::parse(const std::string & text)
{
    static const boost::regex rx(SsmEmptyString);

    LogTrace(TRACE5) << "SsmEmptyElem";
    boost::smatch what;
    if (!boost::regex_match(text, what, rx, boost::match_any)) {
        throw typeb_parse_except(STDLOG, TBMsg::MISS_NECESSARY_ELEMENT, "Wrong string format");
    }
    //String has correct format. Initialize needed variables
    return new SsmEmptyElem();
}

std::ostream & operator << (std::ostream& os, const SsmRevFlightElem &name)
{
    os << " fl[" << name.flight() << "]"
       << " dt1[" << name.date1() << "]"
       << " dt2[" << name.date2() << "]"
       << " freq[" << name.freq() << "]"
       << " rate["<< name.freqRate() << "]";
    return os;
}

std::ostream & operator << (std::ostream& os, const SsmFlightElem &name)
{
    os << " fl[" << name.flight() << "]"; 
    for (const std::string & s:  name.dei()) {
        os << "\nDEI:" << s;
    }
    return os;
}

std::ostream & operator << (std::ostream& os, const SsmLongPeriodElem &name)
{
    os << " dt1[" << name.date1() << "]" 
       << " dt2[" << name.date2() << "]" 
       << " freq[" << name.freq() << "]" 
       << " rate["<< name.freqRate() << "]";
    for (const std::string & s:  name.dei()) {
        os << "\nDEI:" << s;
    }
    return os;
}

std::ostream & operator << (std::ostream& os, const SsmSkdPeriodElem &name)
{
    os << "effect[" << name.effectDt() << "]" 
       << " discont[" << name.discontDt() << "]";
    return os;
}

std::ostream & operator << (std::ostream& os, const SsmShortFlightElem &name)
{
    os << "fl[" << name.flight() << "]";
    return os;
}

std::ostream & operator << (std::ostream& os, const SsmEquipmentElem &name)
{
    os << "service[" << name.serviceType() << "]"
        <<" craftType[" << name.craftType() << "]"
        <<" config[" << name.aircraftConfig() << "]";
    for (const std::string & s:  name.dei()) {
        os << "\nDEI:" << s;
    }
    return os;
}

std::ostream & operator << (std::ostream& os, const SsmRoutingElem &name)
{
    os << " dep [" << name.dep() << "]"
        << " airSTD [" << name.airSTD() << "]"
        << " varSTD [" << name.varSTD() << "]"
        << " pasSTD [" << name.pasSTD() << "]"
        << " arr [" << name.arr() << "]"
        << " airSTA [" << name.airSTA() << "]"
        << " varSTA [" << name.varSTA() << "]"
        << " pasSTA [" << name.pasSTA() << "]";
    for (const std::string & s:  name.dei()) {
        os << "\nDEI:" << s;
    }
    return os;
}

std::ostream & operator << (std::ostream & os, const SsmLegChangeElem & name)
{
    os << " legsChange [" << name.legsChange() << "]";
    for (const std::string & s:  name.dei()) {
        os << "\nDEI:" << s;
    }
    return os;
}

std::ostream & operator << (std::ostream& os, const SsmSegmentElem &name)
{
    os << " seg [" << name.seg() << "]"
        << " dei [" << name.dei() << "]";
    return os;
}

std::ostream & operator << (std::ostream& os, const SsmSupplInfoElem &name)
{
    return os << name.txt();
}

std::ostream & operator << (std::ostream& os, const SsmRejectInfoElem &name)
{
    return os << name.txt();
}

std::ostream & operator << (std::ostream& os, const SsmDontCareElem &name)
{
    return os;
}

std::ostream & operator << (std::ostream& os, const SsmEmptyElem &name)
{
    return os;
}

} //namespace typeb_parser
