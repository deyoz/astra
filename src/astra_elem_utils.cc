#include "astra_elem_utils.h"
#include "astra_elems.h"
#include "astra_utils.h"
#include "astra_locale.h"
#include "basic.h"
#include "xml_unit.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

using namespace std;
using namespace AstraLocale;
using namespace BASIC;

string airl_fromXML(xmlNodePtr node, TCheckFieldFromXML check_type, const string &trace_info)
{
  if (node==NULL) throw EXCEPTIONS::Exception("%s: airl_fromXML(node==NULL)!", trace_info.c_str());
  return airl_fromXML(NodeAsString(node), check_type, trace_info, (char*)(node->name));
}

string airl_fromXML(const string &value, TCheckFieldFromXML check_type, const string &trace_info, const string &node_name)
{
  string str(value);
  string airline;
  TElemFmt fmt;
  TrimString(str);
  if (!str.empty())
  {
    airline = ElemToElemId( etAirline, upperc(str), fmt );
    if (fmt==efmtUnknown)
    {
      TReqInfo::Instance()->traceToMonitor(TRACE5, "%s: unknown <%s> %s", trace_info.c_str(), node_name.c_str(), str.c_str());
      throw UserException( "MSG.AIRLINE.INVALID",
                           LEvntPrms() << PrmSmpl<string>("airline", str ) );
    };
  }
  else
  {
    if (check_type==cfTraceIfEmpty ||
        check_type==cfErrorIfEmpty)
      TReqInfo::Instance()->traceToMonitor(TRACE5, "%s: empty <%s>", trace_info.c_str(), node_name.c_str());
    if (check_type==cfErrorIfEmpty)
      throw UserException( "MSG.AIRLINE.NOT_SET" );

  };
  return airline;
}

string airp_fromXML(xmlNodePtr node, TCheckFieldFromXML check_type, const string &trace_info)
{
  if (node==NULL) throw EXCEPTIONS::Exception("%s: airp_fromXML(node==NULL)!", trace_info.c_str());
  return airp_fromXML(NodeAsString(node), check_type, trace_info, (char*)(node->name));
}

string airp_fromXML(const string &value, TCheckFieldFromXML check_type, const string &trace_info, const string &node_name)
{
  string str(value);
  string airp;
  TElemFmt fmt;
  TrimString(str);
  if (!str.empty())
  {
    airp = ElemToElemId( etAirp, upperc(str), fmt );
    if (fmt==efmtUnknown)
    {
      TReqInfo::Instance()->traceToMonitor(TRACE5, "%s: unknown <%s> %s", trace_info.c_str(), node_name.c_str(), str.c_str());
      throw UserException( "MSG.AIRPORT.INVALID",
                           LEvntPrms() << PrmSmpl<string>("airp", str ) );
    };
  }
  else
  {
    if (check_type==cfTraceIfEmpty ||
        check_type==cfErrorIfEmpty)
      TReqInfo::Instance()->traceToMonitor(TRACE5, "%s: empty <%s>", trace_info.c_str(), node_name.c_str());
    if (check_type==cfErrorIfEmpty)
      throw UserException( "MSG.AIRP.NOT_SET" );
  };
  return airp;
}

int flt_no_fromXML(string str)
{
    int flt_no;
    TrimString(str);
    if (!str.empty())
    {
        if ( StrToInt( str.c_str(), flt_no ) == EOF ||
             flt_no > 99999 || flt_no <= 0 )
        {
            TReqInfo::Instance()->traceToMonitor(TRACE5, "flt_no_fromXML: invalid <flt_no> %s", str.c_str());
            throw UserException( "MSG.FLT_NO.INVALID",
                                 LEvntPrms() << PrmSmpl<string>("flt_no", str) );
        };
    }
    else
    {
        TReqInfo::Instance()->traceToMonitor(TRACE5, "flt_no_fromXML: <flt_no> not defined");
        throw UserException( "MSG.CHECK_FLIGHT.NOT_SET_FLT_NO" );
    };
    return flt_no;
}

string suffix_fromXML(string str)
{
    string suffix;
    TElemFmt fmt;
    TrimString(str);
    if (!str.empty())
    {
        suffix = ElemToElemId( etSuffix, upperc(str), fmt );
        if (fmt==efmtUnknown)
        {
            TReqInfo::Instance()->traceToMonitor(TRACE5, "suffix_fromXML: unknown <suffix> %s", str.c_str());
            throw UserException( "MSG.SUFFIX.INVALID",
                                 LEvntPrms() << PrmSmpl<string>("suffix", str) );
        };
    };
    return suffix;
}

TDateTime scd_out_fromXML(string str, const char* fmt)
{
    TDateTime scd_out;
    TrimString(str);
    if (!str.empty())
    {
        if ( StrToDateTime( str.c_str(), fmt, scd_out ) == EOF )
        {
            TReqInfo::Instance()->traceToMonitor(TRACE5, "scd_out_fromXML: invalid <scd_out> %s", str.c_str());
            throw UserException( "MSG.FLIGHT_DATE.INVALID",
                                 LEvntPrms() << PrmSmpl<string>("scd_out", str) );
        };
    };
    return scd_out;
}

TDateTime date_fromXML(string str)
{
    TDateTime date;
    TrimString(str);
    if (!str.empty())
    {
        if ( StrToDateTime( str.c_str(), "dd.mm.yyyy hh:nn:ss", date ) == EOF )
        {
            if ( StrToDateTime( str.c_str(), "dd.mm.yyyy", date ) == EOF )
            {
                TReqInfo::Instance()->traceToMonitor(TRACE5, "date_fromXML: invalid <date> %s", str.c_str());
                throw UserException( "Date is invalid" );
            }
        };
    };
    return date;
}
