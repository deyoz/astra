/*
*  C++ Implementation: typeb_mini_parser
*
* Description: Разбор Recloc элемента телеграммы
*
*
* Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2006
*
*/
#include <iostream>
#include "typeb_msg.h"
#include <serverlib/helpcpp.h>
#include <serverlib/str_utils.h>
#include <etick/ticket.h>
#include "recloc_element.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/test.h>

namespace typeb_parser{
using namespace std;
using namespace HelpCpp;
using boost::posix_time::ptime;
using boost::gregorian::date;
using namespace boost::gregorian;
using namespace boost::posix_time;
using namespace boost;
using namespace Ticketing;

//   HDQRMBA
//   .HDQRM1P 041595
//   ETE
//   HDQ1P ABCDEF
//   SSR TKNX 1252420123999 O

void RecordLocatorElem::Trace(int level, const char *nick, const char *file, int line,
                              const RecordLocatorElem &si)
{
    ProgTrace(level, nick, file, line, "Recloc info: %s:%s. CityPort:%s, Awk:%s",
              si.predP().c_str(), si.recloc().c_str(),
              si.locationPoint().c_str(),
              si.airline().c_str());
}

RecordLocatorElem * RecordLocatorElem::parse(const std::string & rl_elem)
{
    string point, airl;
    string recloc;
    bool long_airiline;
    std::string::size_type pos;
    if((pos = rl_elem.find('/'))!=string::npos){
        if(pos!=3){
            throw typeb_parse_except(STDLOG,TBMsg::INV_RL_ELEMENT,
                               "Invalid recloc element: '/' position must be 3");
        }
        long_airiline = true;
    } else {
        long_airiline = false;
    }
    point = rl_elem.substr(0,3);
    airl  = rl_elem.substr(long_airiline?4:3,long_airiline?3:2);
    recloc = rl_elem.substr(long_airiline?7:6);

    recloc = StrUtils::rtrim(StrUtils::ltrim(recloc));
    if(recloc.size()>BaseResContrInfo::MaxRemoteRlLen)
    {
        throw typeb_parse_except(STDLOG, TBMsg::INV_RL_ELEMENT,
                           "Too long recloc, " +
                                   string_cast(BaseResContrInfo::MaxRemoteRlLen)+
                                   " Max!");
    }

    return new RecordLocatorElem(point+airl,
                                 point,
                                 airl,
                                 recloc);
}

}


