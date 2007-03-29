
#include "astra_misc.h"
#include <string>
#include <vector>
#include "basic.h"
#include "exceptions.h"
#include "oralib.h"
#include "tlg/tlg_parser.h"

using namespace BASIC;
using namespace EXCEPTIONS;
using namespace std;

string GetPnrAddr(int pnr_id, vector<TPnrAddrItem> &pnrs, string &airline)
{
  pnrs.clear();
  TQuery Qry(&OraSession);
  if (airline.empty())
  {
    Qry.Clear();
    Qry.SQLText=
      "SELECT airline "
      "FROM crs_pnr,tlg_trips "
      "WHERE crs_pnr.point_id=tlg_trips.point_id AND pnr_id=:pnr_id";
    Qry.CreateVariable("pnr_id",otInteger,pnr_id);
    Qry.Execute();
    if (!Qry.Eof) airline=Qry.FieldAsString("airline");
  };

  Qry.Clear();
  Qry.SQLText=
    "SELECT airline,addr FROM pnr_addrs "
    "WHERE pnr_id=:pnr_id ORDER BY DECODE(airline,:airline,0,1),airline";
  Qry.CreateVariable("pnr_id",otInteger,pnr_id);
  Qry.CreateVariable("airline",otString,airline);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    TPnrAddrItem pnr;
    strcpy(pnr.airline,Qry.FieldAsString("airline"));
    strcpy(pnr.addr,Qry.FieldAsString("addr"));
    pnrs.push_back(pnr);
  };
  if (!pnrs.empty() && pnrs[0].airline==airline)
    return pnrs[0].addr;
  else
    return "";
};

string GetPaxPnrAddr(int pax_id, vector<TPnrAddrItem> &pnrs, string &airline)
{
  pnrs.clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT pnr_id FROM crs_pax WHERE pax_id=:pax_id";
  Qry.CreateVariable("pax_id",otInteger,pax_id);
  Qry.Execute();
  if (!Qry.Eof)
    return GetPnrAddr(Qry.FieldAsInteger("pnr_id"),pnrs,airline);
  else
    return "";
};

TDateTime DayToDate(int day, TDateTime base_date)
{
  if (day<1 || day>31) throw EConvertError("DayToDate: wrong day");
  modf(base_date,&base_date);
  int iDay,iMonth,iYear;
  DecodeDate(base_date,iYear,iMonth,iDay);
  TDateTime result;
  do
  {
    try
    {
      EncodeDate(iYear,iMonth,day,result);
    }
    catch(EConvertError) {};
    if (iMonth==12)
    {
      iMonth=1;
      iYear++;
    }
    else iMonth++;
  }
  while(result<base_date);
  return result;
};

