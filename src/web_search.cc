#include "web_search.h"
#include "date_time.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "checkin.h"
#include "passenger.h"
#include "baggage.h"
#include "baggage_calc.h"
#include "xml_unit.h"
#include "misc.h"
#include "dev_utils.h"
#include "astra_elem_utils.h"
#include "sopp.h"
#include "salons.h"

#define NICKNAME "VLAD"
#include "serverlib/slogger.h"

using namespace std;
using namespace ASTRA;
using namespace BASIC::date_time;
using namespace AstraLocale;

namespace WebSearch
{

int TIMEOUT()       //миллисекунды
{
  static boost::optional<int> value;
  if (!value)
    value=getTCLParam("WEB_SEARCH_TIMEOUT",1,10000,1000);
  return value.get();
}

void TTestPaxInfo::trace( TRACE_SIGNATURE ) const
{
  ProgTrace(TRACE_PARAMS, "============ TTestPaxInfo ============");
  ProgTrace(TRACE_PARAMS, "airlines: %s", airline.c_str());
  ProgTrace(TRACE_PARAMS, "subcls: %s", subcls.c_str());
  ProgTrace(TRACE_PARAMS, "pnr_addr: %s", pnr_addr.str(TPnrAddrInfo::AddrAndAirline).c_str());
  ProgTrace(TRACE_PARAMS, "pax_id: %d", pax_id);
  ProgTrace(TRACE_PARAMS, "surname: %s", surname.c_str());
  ProgTrace(TRACE_PARAMS, "name: %s", name.c_str());
  ProgTrace(TRACE_PARAMS, "ticket_no: %s", ticket_no.c_str());
  ProgTrace(TRACE_PARAMS, "document: %s", document.c_str());
  ProgTrace(TRACE_PARAMS, "reg_no: %d", reg_no);
  ProgTrace(TRACE_PARAMS, "^^^^^^^^^^^^ TTestPaxInfo ^^^^^^^^^^^^");
}

TPNRFilter& TPNRFilter::fromXML(xmlNodePtr fltParentNode, xmlNodePtr paxParentNode)
{
  clear();
  if (fltParentNode==nullptr || paxParentNode==nullptr) return *this;
  xmlNodePtr node2=fltParentNode->children;
  string str;

  for(int pass=0; pass<2; pass++)
  {
    string airline;
    xmlNodePtr alNode=NULL;
    if (pass==0)
    {
      alNode=GetNodeFast("airlines", node2);
      if (alNode!=NULL) alNode=alNode->children;
    }
    if (pass==1)
    {
      alNode=GetNodeFast("airline", node2);
    };

    for(; alNode!=NULL; alNode=alNode->next)
    {
      airline = airl_fromXML(alNode, (pass==0?cfTraceIfEmpty:cfNothingIfEmpty), __FUNCTION__);
      if (!airline.empty())
        airlines.insert(airline);
      if (pass==1) break;
    };
  };

  flt_no = flt_no_fromXML(NodeAsStringFast("flt_no", node2, ""), cfNothingIfEmpty);
  suffix = suffix_fromXML(NodeAsStringFast("suffix", node2, ""));

  for(int pass=0; pass<3; pass++)
  {
    pair<TDateTime, TDateTime> scd_out_range(NoExists, NoExists);
    if (pass==0)
    {
      str=NodeAsStringFast("scd_out_min", node2, "");
      TrimString(str);
      if (!str.empty())
      {
        if ( StrToDateTime( str.c_str(), "dd.mm.yyyy hh:nn:ss", scd_out_range.first ) == EOF )
        {
          TReqInfo::Instance()->traceToMonitor(TRACE5, "TPNRFilter::fromXML: invalid <scd_out_min> %s", str.c_str());
              throw UserException( "MSG.FLIGHT_DATE.INVALID",
                                     LParams()<<LParam("scd_out", str) );
            };
        };
            str=NodeAsStringFast("scd_out_max", node2, "");
      TrimString(str);
      if (!str.empty())
      {
        if ( StrToDateTime( str.c_str(), "dd.mm.yyyy hh:nn:ss", scd_out_range.second ) == EOF )
        {
          TReqInfo::Instance()->traceToMonitor(TRACE5, "TPNRFilter::fromXML: invalid <scd_out_max> %s", str.c_str());
              throw UserException( "MSG.FLIGHT_DATE.INVALID",
                                     LParams()<<LParam("scd_out", str) );
            };
      };
      if (scd_out_range.first==NoExists && scd_out_range.second!=NoExists)
      {
        TReqInfo::Instance()->traceToMonitor(TRACE5, "TPNRFilter::fromXML: <scd_out_min> not defined");
        throw UserException("MSG.INVALID_RANGE");
      };
      if (scd_out_range.first!=NoExists && scd_out_range.second==NoExists)
      {
        TReqInfo::Instance()->traceToMonitor(TRACE5, "TPNRFilter::fromXML: <scd_out_max> not defined");
        throw UserException("MSG.INVALID_RANGE");
      };
    };
    if (pass==1)
    {
      scd_out_range.first = scd_out_fromXML(NodeAsStringFast("scd_out", node2, ""), "dd.mm.yyyy hh:nn:ss");
      if (scd_out_range.first!=NoExists)
        scd_out_range.second=scd_out_range.first+1.0; //1 сутки
    };
    if (pass==2)
    {
      int scd_out_shift;
      str=NodeAsStringFast("scd_out_shift", node2, "");
      TrimString(str);
      if (!str.empty())
      {
        if ( StrToInt( str.c_str(), scd_out_shift ) == EOF )
        {
          TReqInfo::Instance()->traceToMonitor(TRACE5, "TPNRFilter::fromXML: invalid <scd_out_shift> %s", str.c_str());
          throw UserException("MSG.INVALID_RANGE");
        };
        scd_out_range.first=NowUTC();
        scd_out_range.second=scd_out_range.first+(double)scd_out_shift/1440.0;
      };
    };
    if (scd_out_range.first!=NoExists && scd_out_range.second!=NoExists)
    {
      if (scd_out_range.first>=scd_out_range.second)
      {
        TReqInfo::Instance()->traceToMonitor(TRACE5, "TPNRFilter::fromXML: invalid search period");
        throw UserException("MSG.INVALID_RANGE");
      };
      if (pass==0 || pass==1)
        MergeSortedRanges(scd_out_local_ranges, scd_out_range);
      if (pass==2)
        MergeSortedRanges(scd_out_utc_ranges, scd_out_range);
    };
  };

  double local_days=0;
  for(vector< pair<TDateTime, TDateTime> >::const_iterator r=scd_out_local_ranges.begin();
                                                           r!=scd_out_local_ranges.end(); ++r )
    local_days+=(r->second-r->first);
  if (local_days>5.0)
  {
    TReqInfo::Instance()->traceToMonitor(TRACE5, "TPNRFilter::fromXML: search period too large");
    throw UserException("MSG.SEARCH_PERIOD_MAX_N_DAYS", LParams()<<LParam("days", 5));
  };
  double utc_days=0;
  for(vector< pair<TDateTime, TDateTime> >::const_iterator r=scd_out_utc_ranges.begin();
                                                           r!=scd_out_utc_ranges.end(); ++r )
    utc_days+=(r->second-r->first);
  if (utc_days>5.0)
  {
    TReqInfo::Instance()->traceToMonitor(TRACE5, "TPNRFilter::fromXML: search period too large");
    throw UserException("MSG.SEARCH_PERIOD_MAX_N_DAYS", LParams()<<LParam("days", 5));
  };

  node2=paxParentNode->children;

  surname=NodeAsStringFast("surname", node2, "");
  TrimString(surname);
  if (surname.empty())
  {
    TReqInfo::Instance()->traceToMonitor(TRACE5, "TPNRFilter::fromXML: <surname> not defined");
    throw UserException("MSG.PASSENGER.NOT_SET.SURNAME");
  };
  surname=upperc(surname);

  name=NodeAsStringFast("name", node2, "");
  TrimString(name);
  name=upperc(name);
  name.erase(find(name.begin(), name.end(), ' '), name.end()); //оставляем часть до пробела

  str=NodeAsStringFast("pnr_addr", node2, "");
  TrimString(str);
  pnr_addr_normal=convert_pnr_addr(upperc(str) , true);

  ticket_no=NodeAsStringFast("ticket_no", node2, "");
  TrimString(ticket_no);
  ticket_no=upperc(ticket_no);

  document=NodeAsStringFast("document", node2, "");
  TrimString(document);
  document=upperc(document);

  str=NodeAsStringFast("reg_no", node2, "");
  TrimString(str);
  if (!str.empty())
  {
    if ( StrToInt( str.c_str(), reg_no ) == EOF ||
             reg_no > 999 || reg_no <= 0 )
        {
      TReqInfo::Instance()->traceToMonitor(TRACE5, "TPNRFilter::fromXML: invalid <reg_no> %s", str.c_str());
      throw UserException("MSG.INVALID_REG_NO");
    };
  };

  return *this;
};

bool SurnameFilter::validForSearch() const
{
  return !surname.empty();
}

void SurnameFilter::addSQLTablesForSearch(const PaxOrigin& origin, std::list<std::string>& tables) const
{
}

void SurnameFilter::addSQLConditionsForSearch(const PaxOrigin& origin, std::list<std::string>& conditions) const
{
  std::string field_name;

  switch(origin)
  {
    case paxCheckIn:
      field_name="pax.surname";
      break;
    case paxPnl:
      field_name="crs_pax.surname";
      break;
    case paxTest:
      field_name="test_pax.surname";
      break;
  }
  ostringstream sql;
  sql << (checkSurnameEqualBeginning?"system.transliter_equal_begin(":
                                     "system.transliter_equal(")
      << field_name << ", :surname)<>0";

  conditions.push_back(sql.str());
}

void SurnameFilter::addSQLParamsForSearch(QParams& params) const
{
  params << QParam("surname", otString, surname);
}

string TPNRFilter::getSurnameSQLFilter(const string &field_name, TQuery &Qry) const
{
  ostringstream sql;
  sql << (checkSurnameEqualBeginning?"system.transliter_equal_begin(":
                                     "system.transliter_equal(")
      << field_name << ", :surname)<>0";

  Qry.CreateVariable("surname", otString, surname);
  return sql.str();
};

bool TPNRFilter::isEqualPnrAddr(const TPnrAddrs &pnr_addrs) const
{
  if (!pnr_addr_normal.empty())
  {
    //найдем есть ли среди адресов искомый
    TPnrAddrs::const_iterator a=pnr_addrs.begin();
    for(; a!=pnr_addrs.end(); ++a)
      if (convert_pnr_addr(a->addr, true)==pnr_addr_normal) break;
    if (a==pnr_addrs.end()) return false;
  };
  return true;
}

bool TPNRFilter::isEqualSurname(const string &pax_surname) const
{
  if (!surname.empty())
  {
    if (!(checkSurnameEqualBeginning?transliter_equal_begin(pax_surname, surname):
                                     transliter_equal(pax_surname, surname))) return false;
  }
  return true;
}

bool TPNRFilter::isEqualName(const string &pax_name) const
{
  if (!name.empty())
  {
    string pax_name_normal=pax_name;
    //оставляем часть до пробела
    pax_name_normal.erase(find(pax_name_normal.begin(), pax_name_normal.end(), ' '), pax_name_normal.end());
    //проверим совпадение имени
    if (!(checkNameEqualBeginning?transliter_equal_begin(pax_name_normal, name):
                                  transliter_equal(pax_name_normal, name))) return false;
  };
  return true;
};

bool TPNRFilter::isEqualTkn(const string &pax_ticket_no) const
{
  if (!ticket_no.empty())
  {
    //проверим совпадение билета
    if (pax_ticket_no!=ticket_no)
    {
      if (ticket_no.size() != 14) return false;
      if (pax_ticket_no!=ticket_no.substr(0,13)) return false;
    };
  };
  return true;
};

bool TPNRFilter::isEqualDoc(const string &pax_document) const
{
  if (!document.empty())
  {
    //проверим совпадение документа
    if (pax_document!=document) return false;
  };
  return true;
};

bool TPNRFilter::isEqualRegNo(const int &pax_reg_no) const
{
  if (reg_no!=NoExists)
  {
    //проверим совпадение рег. номера
    if (pax_reg_no!=reg_no) return false;
  };
  return true;
};

TPNRFilter& TPNRFilter::testPaxFromDB()
{
  if (surname.empty())
    throw EXCEPTIONS::Exception("TPNRFilter::testPaxFromDB: surname not defined");

  test_paxs.clear();

  TQuery Qry(&OraSession);
  Qry.Clear();
  ostringstream sql;
  sql << "SELECT id, airline, surname, name, subclass, doc_no, tkn_no, "
         "       pnr_airline, pnr_addr, reg_no FROM test_pax "
      << "WHERE " << getSurnameSQLFilter("test_pax.surname", Qry);

  if (!airlines.empty())
    sql << " AND (airline IN " << GetSQLEnum(airlines) << " OR airline IS NULL) ";

  Qry.SQLText=sql.str().c_str();
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    TTestPaxInfo pax;
    pax.airline=Qry.FieldAsString("airline");
    pax.subcls=Qry.FieldAsString("subclass");
    pax.pnr_addr.addr=Qry.FieldAsString("pnr_addr");
    if (!pax.pnr_addr.addr.empty())
    {
      pax.pnr_addr.airline=Qry.FieldAsString("pnr_airline");
      if (pax.pnr_addr.airline.empty()) pax.pnr_addr.airline=pax.airline;
      if (pax.pnr_addr.airline.empty()) pax.pnr_addr.clear();
    };

    pax.pax_id=Qry.FieldAsInteger("id");
    pax.surname=Qry.FieldAsString("surname");
    pax.name=Qry.FieldAsString("name");
    pax.ticket_no=Qry.FieldAsString("tkn_no");
    pax.document=Qry.FieldAsString("doc_no");
    pax.reg_no=Qry.FieldAsInteger("reg_no");

    if (!isTestPaxId(pax.pax_id)) continue;
    if (!isEqualName(pax.name)) continue;
    if (!isEqualTkn(pax.ticket_no)) continue;
    if (!isEqualDoc(pax.document)) continue;
    TPnrAddrs pnrAddrs;
    pnrAddrs.emplace_back(pax.pnr_addr);
    if (!isEqualPnrAddr(pnrAddrs)) continue;
    if (!isEqualRegNo(pax.reg_no)) continue;
    test_paxs.push_back(pax);
  };

  return *this;
};

void TPNRFilter::trace( TRACE_SIGNATURE ) const
{
  ProgTrace(TRACE_PARAMS, "============ TPNRFilter ============");
  ostringstream str;
  str.str("");
  for(set<string>::const_iterator i=airlines.begin(); i!=airlines.end(); ++i)
  {
    if (i!=airlines.begin()) str << ", ";
    str << *i;
  };
  ProgTrace(TRACE_PARAMS, "airlines: %s", str.str().c_str());
  ProgTrace(TRACE_PARAMS, "flt_no: %s", flt_no==NoExists?"":IntToString(flt_no).c_str());
  ProgTrace(TRACE_PARAMS, "suffix: %s", suffix.c_str());
  for(int range_pass=0; range_pass<2; range_pass++)
  {
    const vector< pair<TDateTime, TDateTime> > &scd_out_ranges=range_pass==0?scd_out_local_ranges:scd_out_utc_ranges;
    if (!scd_out_ranges.empty())
    {
      for(vector< pair<TDateTime, TDateTime> >::const_iterator i=scd_out_ranges.begin();
                                                               i!=scd_out_ranges.end(); ++i)
      {
        str.str("");
        str << "[" << DateTimeToStr(i->first, "dd.mm.yy hh:nn:ss") << ", "
                   << DateTimeToStr(i->second, "dd.mm.yy hh:nn:ss") << ")";
        if (i==scd_out_ranges.begin())
          ProgTrace(TRACE_PARAMS, "%s: %s", range_pass==0?"scd_out_local":"scd_out_utc", str.str().c_str());
        else
          ProgTrace(TRACE_PARAMS, "%s  %s", range_pass==0?"             ":"           ", str.str().c_str());
      };
    }
    else ProgTrace(TRACE_PARAMS, "%s: %s", range_pass==0?"scd_out_local":"scd_out_utc", "");
  };
  ProgTrace(TRACE_PARAMS, "surname: %s", surname.c_str());
  ProgTrace(TRACE_PARAMS, "name: %s", name.c_str());
  ProgTrace(TRACE_PARAMS, "pnr_addr_normal: %s", pnr_addr_normal.c_str());
  ProgTrace(TRACE_PARAMS, "ticket_no: %s", ticket_no.c_str());
  ProgTrace(TRACE_PARAMS, "document: %s", document.c_str());
  ProgTrace(TRACE_PARAMS, "reg_no: %s", reg_no==NoExists?"":IntToString(reg_no).c_str());
  ProgTrace(TRACE_PARAMS, " ");
  ProgTrace(TRACE_PARAMS, "airp_dep: %s", airp_dep.c_str());
  ProgTrace(TRACE_PARAMS, "airp_arv: %s", airp_arv.c_str());
  ProgTrace(TRACE_PARAMS, "checkSurnameEqualBeginning: %s", checkSurnameEqualBeginning?"true":"false");
  ProgTrace(TRACE_PARAMS, "checkNameEqualBeginning: %s", checkNameEqualBeginning?"true":"false");
  ProgTrace(TRACE_PARAMS, "from_scan_code: %s", from_scan_code?"true":"false");
  ProgTrace(TRACE_PARAMS, "^^^^^^^^^^^^ TPNRFilter ^^^^^^^^^^^^");
};

TPNRFilters& TPNRFilters::getBCBPSections(const std::string &bcbp, BCBPSections &sections)
{
  sections.clear();
  if (bcbp.empty())
  {
    TReqInfo::Instance()->traceToMonitor(TRACE5, "TPNRFilters::getBCBPSections: empty bcbp");
    throw UserException("MSG.SCAN_CODE.NOT_SET");
  };

  try
  {
    BCBPSections::get(bcbp, 0, bcbp.size(), sections, false);
  }
  catch(EXCEPTIONS::EConvertError &e)
  {
    LogTrace(TRACE5) << __FUNCTION__ << ": bcbp=" << bcbp;
    LogTrace(TRACE5) << '\n' << sections;
    TReqInfo::Instance()->traceToMonitor(TRACE5, "TPNRFilters::getBCBPSections: %s", e.what());
    throw UserException("MSG.SCAN_CODE.UNKNOWN_FORMAT");
  };

  return *this;
}

TPNRFilters& TPNRFilters::fromBCBPSections(const BCBPSections &sections)
{
  clear();
  try
  {
    TElemFmt fmt;

    //фамилия/имя пассажира
    pair<string, string> name_pair=sections.unique.passengerName();

    for(vector<BCBPRepeatedSections>::const_iterator s=sections.repeated.begin(); s!=sections.repeated.end(); ++s)
    {
      TPNRFilter filter;

      filter.from_scan_code=true;
      filter.surname=name_pair.first;
      filter.name=name_pair.second;

      if (filter.surname.size()+filter.name.size()+1 >= 20)
      {
        filter.checkSurnameEqualBeginning=true;
        if (!filter.name.empty())
          filter.checkNameEqualBeginning=true;
      };
      filter.name.erase(find(filter.name.begin(), filter.name.end(), ' '), filter.name.end()); //оставляем часть до пробела

      //номер PNR
      const BCBPRepeatedSections &repeated=*s;
      filter.pnr_addr_normal=convert_pnr_addr(repeated.operatingCarrierPNRCode(), true);

      //аэропорт вылета, а надо бы еще и город анализировать!!!
      filter.airp_dep = ElemToElemId( etAirp, repeated.fromCityAirpCode() , fmt );
      if (fmt==efmtUnknown)
        throw EXCEPTIONS::EConvertError("unknown item 26 <From City Airport Code> %s", repeated.fromCityAirpCode().c_str());

      //аэропорт прилета, а надо бы еще и город анализировать!!!
      filter.airp_arv = ElemToElemId( etAirp, repeated.toCityAirpCode(), fmt );
      if (fmt==efmtUnknown)
        throw EXCEPTIONS::EConvertError("unknown item 38 <To City Airport Code> %s", repeated.toCityAirpCode().c_str());

      //авиакомпания
      string airline = ElemToElemId( etAirline, repeated.operatingCarrierDesignator(), fmt );
      if (fmt==efmtUnknown)
        throw EXCEPTIONS::EConvertError("unknown item 42 <Operating carrier Designator> %s", repeated.operatingCarrierDesignator().c_str());
      filter.airlines.insert(airline);

      //номер рейса + суффих
      pair<int, string> flt_no_pair=repeated.flightNumber();
      if (!flt_no_pair.second.empty())
      {
        filter.suffix = ElemToElemId( etSuffix, flt_no_pair.second, fmt );
        if (fmt==efmtUnknown)
          throw EXCEPTIONS::EConvertError("unknown item 43 <Flight Number> suffix %s", flt_no_pair.second.c_str());
      };

      filter.flt_no=flt_no_pair.first;

      //дата рейса (julian format)
      int julian_date=repeated.dateOfFlight();
      try
      {
        JulianDate scd_out_local(julian_date, NowUTC(), JulianDate::everywhere);
        scd_out_local.trace(__FUNCTION__);
        MergeSortedRanges(filter.scd_out_local_ranges, make_pair(scd_out_local.getDateTime(), scd_out_local.getDateTime()+1.0));
      }
      catch(EXCEPTIONS::EConvertError)
      {
        throw EXCEPTIONS::EConvertError("unknown item 46 <Date of Flight (Julian Date)> %d", julian_date);
      };

      //рег. номер
      filter.reg_no=repeated.checkinSeqNumber().first;

      segs.push_back(filter);
    };
  }
  catch(EXCEPTIONS::EConvertError &e)
  {
    LogTrace(TRACE5) << '\n' << sections;
    TReqInfo::Instance()->traceToMonitor(TRACE5, "TPNRFilters::fromBCBPSections: %s", e.what());
    throw UserException("MSG.SCAN_CODE.UNKNOWN_DATA");
  };

  return *this;
}

TPNRFilters& TPNRFilters::fromBCBP_M(const std::string &bcbp)
{
  BCBPSections sections;
  getBCBPSections(bcbp, sections);
  fromBCBPSections(sections);

  return *this;
};

TPNRFilters& TPNRFilters::fromXML(xmlNodePtr fltParentNode, xmlNodePtr paxParentNode)
{
  clear();
  if (fltParentNode==nullptr || paxParentNode==nullptr) return *this;
  segs.push_back(TPNRFilter().fromXML(fltParentNode, paxParentNode));
  return *this;
};

TMultiPNRFilters& TMultiPNRFilters::fromXML(xmlNodePtr fltParentNode, xmlNodePtr paxParentNode)
{
  clear();
  if (fltParentNode==nullptr || paxParentNode==nullptr) return *this;
  if (string((const char*)paxParentNode->name)=="group")
  {
    group_id=NodeAsInteger("@id", paxParentNode);
    is_main=NodeAsBoolean("@main", paxParentNode, false);
  }

  xmlNodePtr scanCodeNode=GetNode("scan_code", paxParentNode);
  if (scanCodeNode!=NULL)
    TPNRFilters::fromBCBP_M(NodeAsString(scanCodeNode));
  else
    TPNRFilters::fromXML(fltParentNode, paxParentNode);

  return *this;
}

TMultiPNRFiltersList& TMultiPNRFiltersList::fromXML(xmlNodePtr reqNode)
{
  clear();

  if (reqNode==nullptr) return *this;

  xmlNodePtr fltParentNode=reqNode;

  if (trueMultiRequest(reqNode))
  {
    for(xmlNodePtr grpNode=NodeAsNode("groups/group", reqNode); grpNode!=nullptr; grpNode=grpNode->next)
      if (string((const char*)grpNode->name)=="group")
        push_back(WebSearch::TMultiPNRFilters().fromXML(fltParentNode, grpNode));
  }
  else push_back(WebSearch::TMultiPNRFilters().fromXML(fltParentNode, fltParentNode));

  return *this;
}

bool TDestInfo::fromDB(int point_id, bool pr_throw)
{
  if (point_id==NoExists)
    throw EXCEPTIONS::Exception("TDestInfo::fromDB: point_id not defined");
  clear();
  try
  {
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText=
      "SELECT scd_in, est_in, act_in, airp FROM points WHERE point_id=:point_id AND pr_del=0";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.Execute();
    if (Qry.Eof) throw UserException( "MSG.FLIGHT.NOT_FOUND" );
    point_arv=point_id;
    airp_arv=Qry.FieldAsString("airp");
    string region=AirpTZRegion(airp_arv);
    TDateTime scd_in=Qry.FieldIsNULL("scd_in")?NoExists:Qry.FieldAsDateTime("scd_in");
    scd_in_local=scd_in==NoExists?NoExists:UTCToLocal(scd_in, region);
    est_in_local=Qry.FieldIsNULL("est_in")?NoExists:UTCToLocal(Qry.FieldAsDateTime("est_in"), region);
    act_in_local=Qry.FieldIsNULL("act_in")?NoExists:UTCToLocal(Qry.FieldAsDateTime("act_in"), region);

    city_arv = ((const TAirpsRow&)base_tables.get( "airps" ).get_row( "code", airp_arv, true )).city;
    if (scd_in!=NoExists && scd_in_local!=NoExists)
      arv_utc_offset = (int)round((scd_in_local - scd_in) * 1440);
    else
      arv_utc_offset = NoExists;
  }
  catch(UserException &E)
  {
    ProgTrace(TRACE5, ">>>> %s", getLocaleText(E.getLexemaData()).c_str());
    if ( pr_throw )
      throw;
    else
      return false;
  };
  return true;
};

void TDestInfo::toXML(xmlNodePtr node, XMLStyle xmlStyle) const
{
/*
  <dest>
    <point_arv> ид. пункта прилета
    <scd_in>DD.MM.YYYY HH24:MI:SS плановое время прилета
    <est_in>DD.MM.YYYY HH24:MI:SS расчетное время прилета
    <act_in>DD.MM.YYYY HH24:MI:SS фактическое время прилета
    <airp_arv> порт прилета
    <city_arv> город прилета
    <arr_utc_offset> смещение локального времени пункта прилета относительно UTC. Указывается в минутах!
  </dest>
*/
  if (node==NULL) return;
  if (xmlStyle==xmlSearchPNRs)
  {
    point_arv==NoExists?NewTextChild(node, "point_arv"):
                        NewTextChild(node, "point_arv", point_arv);
  };
  NewTextChild(node, "scd_in", scd_in_local==NoExists?"":DateTimeToStr(scd_in_local, ServerFormatDateTimeAsString));
  NewTextChild(node, "est_in", est_in_local==NoExists?"":DateTimeToStr(est_in_local, ServerFormatDateTimeAsString));
  NewTextChild(node, "act_in", act_in_local==NoExists?"":DateTimeToStr(act_in_local, ServerFormatDateTimeAsString));
  NewTextChild(node, "airp_arv", airpToPrefferedCode(airp_arv, AstraLocale::OutputLang()));
  NewTextChild(node, "city_arv", ElemIdToCodeNative(etCity, city_arv));
  arv_utc_offset==NoExists?NewTextChild(node, "arr_utc_offset"):
                           NewTextChild(node, "arr_utc_offset", arv_utc_offset);
};

void TFlightInfo::set(const TAdvTripInfo& fltInfo)
{
  clear();
  oper=fltInfo;

  string region=AirpTZRegion(oper.airp);
  scd_out_local=oper.scd_out!=NoExists?UTCToLocal(oper.scd_out, region):NoExists;
  est_out_local=oper.est_out_exists()?UTCToLocal(oper.est_out.get(), region):NoExists;
  act_out_local=oper.act_out_exists()?UTCToLocal(oper.act_out.get(), region):NoExists;

  city_dep = ((const TAirpsRow&)base_tables.get( "airps" ).get_row( "code", oper.airp, true )).city;
  if (oper.scd_out!=NoExists && scd_out_local!=NoExists)
    dep_utc_offset = (int)round((scd_out_local - oper.scd_out) * 1440);
  else
    dep_utc_offset = NoExists;
}

bool TFlightInfo::fromDB(TQuery &Qry)
{
  clear();
  if (Qry.Eof) return false;
  set(TAdvTripInfo(Qry));
  return true;
}

#if 0
void TFlightInfo::isSelfCheckInPossibleTest()
{
  for(const ASTRA::TClientType& client_type : {ctWeb, ctKiosk, ctMobile})
    for(const bool& first_segment : {true, false})
      for(const bool& notRefusalExists : {true, false})
        for(const bool& refusalExists : {true, false})
          for(const TStage& checkInStage : {sRemovalGangWay, sNoActive, sOpenCheckIn, sCloseCheckIn, sTakeoff})
            for(const TStage& cancelStage : {sRemovalGangWay, sNoActive, sOpenWEBCheckIn, sCloseWEBCancel, sTakeoff})
            {
              if (client_type==ctKiosk && cancelStage!=sRemovalGangWay) continue;
              if (client_type!=ctKiosk && checkInStage==sNoActive && cancelStage!=sNoActive) continue;
              if (client_type!=ctKiosk && checkInStage==sOpenCheckIn && cancelStage!=sOpenWEBCheckIn && cancelStage!=sCloseWEBCancel) continue;
              if (client_type!=ctKiosk && checkInStage==sCloseCheckIn && cancelStage!=sOpenWEBCheckIn && cancelStage!=sCloseWEBCancel) continue;
              if (client_type!=ctKiosk && checkInStage==sTakeoff && cancelStage!=sTakeoff) continue;

              std::string error;
              boost::optional<TStage> checkInStageOpt(checkInStage==sRemovalGangWay?boost::none:boost::optional<TStage>(checkInStage));
              boost::optional<TStage> cancelStageOpt(cancelStage==sRemovalGangWay?boost::none:boost::optional<TStage>(cancelStage));
              if (checkInStage==sOpenCheckIn)
                checkInStageOpt=client_type==ctKiosk?sOpenKIOSKCheckIn:sOpenWEBCheckIn;
              if (checkInStage==sCloseCheckIn)
                checkInStageOpt=client_type==ctKiosk?sCloseKIOSKCheckIn:sCloseWEBCheckIn;

              int priority=getStagePriority(client_type,
                                            checkInStageOpt,
                                            cancelStageOpt);

              try
              {

                isSelfCheckInPossible(client_type,
                                      checkInStageOpt,
                                      cancelStageOpt,
                                      first_segment,
                                      notRefusalExists,
                                      refusalExists);
              }
              catch(UserException &e)
              {
                error=e.getLexemaData().lexema_id;
              }

              if (checkInStageOpt && (checkInStageOpt.get()==sOpenKIOSKCheckIn || checkInStageOpt.get()==sOpenWEBCheckIn))
                checkInStageOpt=sOpenCheckIn;
              if (checkInStageOpt && (checkInStageOpt.get()==sCloseKIOSKCheckIn || checkInStageOpt.get()==sCloseWEBCheckIn))
                checkInStageOpt=sCloseCheckIn;


              LogTrace(TRACE5) << setw(6) << EncodeClientType(client_type)
                               << setw(6) << (first_segment?"first":"")
                               << setw(11) << (notRefusalExists?"notRefusal":"-")
                               << setw(8) << (refusalExists?"refusal":"-")
                               << setw(15) << (checkInStageOpt?EncodeStage(checkInStageOpt.get()):"")
                               << setw(15) << (cancelStageOpt?EncodeStage(cancelStageOpt.get()):"")
                               << setw(2) << priority
                               << "  " << error;
            }
}
#endif

int TFlightInfo::getStagePriority(const ASTRA::TClientType& client_type,
                                  const boost::optional<TStage>& checkInStage,
                                  const boost::optional<TStage>& cancelStage)
{
  if (!checkInStage) return 5;

  if (checkInStage.get()==sTakeoff) return 4;

  if (checkInStage.get() == sOpenKIOSKCheckIn ||
      checkInStage.get() == sOpenWEBCheckIn ||
      (client_type != ctKiosk &&
       cancelStage && cancelStage.get() == sOpenWEBCheckIn))
    //разрешена регистрация или отмена регистрации
    return 1;

  if (checkInStage.get() == sNoActive)
    return 2;

  if (checkInStage.get() == sCloseWEBCheckIn ||
      checkInStage.get() == sCloseKIOSKCheckIn)
    return 3;

  return 5;
}

int TFlightInfo::getStagePriority() const
{
  return getStagePriority(TReqInfo::Instance()->client_type,
                          stage(),
                          stage(stWEBCancel));
}

void TFlightInfo::isSelfCheckInPossible(const ASTRA::TClientType& client_type,
                                        const boost::optional<TStage>& checkInStage,
                                        const boost::optional<TStage>& cancelStage,
                                        bool first_segment,
                                        bool notRefusalExists,
                                        bool refusalExists)
{
  if (!notRefusalExists && !refusalExists) return;

  //проверяем на сегменте вылет рейса и состояние соответствующего этапа
  if (!checkInStage)
    throw UserException( "MSG.CHECKIN.CLOSED_OR_DENIAL" );

  if (checkInStage.get()==sTakeoff)
    throw UserException( "MSG.FLIGHT.TAKEOFF" );

  if (notRefusalExists || (refusalExists && client_type == ctKiosk))
  {
    if (!(checkInStage.get() == sOpenKIOSKCheckIn ||
          checkInStage.get() == sOpenWEBCheckIn ||
          (checkInStage.get() == sNoActive && !first_segment)))
    {
      if (checkInStage.get() == sNoActive)
        throw UserException( "MSG.CHECKIN.NOT_OPEN" );
      else
        throw UserException( "MSG.CHECKIN.CLOSED_OR_DENIAL" );
    }
  }

  if (refusalExists && client_type != ctKiosk)
  {
    if (!cancelStage)
      throw UserException( "MSG.CHECKIN.CLOSED_OR_DENIAL" );
    if (!(cancelStage.get() == sOpenWEBCheckIn ||
          (cancelStage.get() == sNoActive && !first_segment)))
    {
      if (cancelStage.get() == sNoActive)
        throw UserException( "MSG.CHECKIN.NOT_OPEN" );
      else if (cancelStage.get() == sCloseWEBCancel)
        throw UserException( "MSG.PASSENGER.UNREGISTRATION_DENIAL" );
      else
        throw UserException( "MSG.CHECKIN.CLOSED_OR_DENIAL" );
    }
  }
}

void TFlightInfo::isSelfCheckInPossible(bool first_segment, bool notRefusalExists, bool refusalExists) const
{
  TFlightInfo::isSelfCheckInPossible(TReqInfo::Instance()->client_type,
                                     stage(),
                                     stage(stWEBCancel),
                                     first_segment,
                                     notRefusalExists,
                                     refusalExists);
}

bool TFlightInfo::fromDB(int point_id, bool pr_throw)
{
  if (point_id==NoExists)
    throw EXCEPTIONS::Exception("TFlightInfo::fromDB: point_id not defined");
  clear();
  try
  {
    TAdvTripInfo fltInfo;
    if (!fltInfo.getByPointId(point_id))
      throw UserException( "MSG.FLIGHT.NOT_FOUND" );

    if (fltInfo.pr_del!=0)
      throw UserException( "MSG.FLIGHT.CANCELED" );

    if (!fltInfo.pr_reg)
      throw UserException( "MSG.FLIGHT.CHECKIN_CANCELED" );

    if (fltInfo.scd_out==NoExists) throw UserException( "MSG.FLIGHT_DATE.NOT_SET" );

    set(fltInfo);
  }
  catch(UserException &E)
  {
    ProgTrace(TRACE5, ">>>> %s", getLocaleText(E.getLexemaData()).c_str());
    if ( pr_throw )
      throw;
    else
      return false;
  };
    return true;
}

bool TFlightInfo::fromDBadditional(bool first_segment, bool pr_throw)
{
  if (oper.point_id==NoExists)
    throw EXCEPTIONS::Exception("TFlightInfo::fromDBadditional: point_dep not defined");
  try
  {
    GetMktFlights(oper, mark);
    stage_statuses.clear();
    TTripStages tripStages( oper.point_id );
    stage_statuses[stWEBCheckIn]=tripStages.getStage( stWEBCheckIn );
    stage_statuses[stWEBCancel]=tripStages.getStage( stWEBCancel );
    stage_statuses[stKIOSKCheckIn]=tripStages.getStage( stKIOSKCheckIn );
    stage_statuses[stCheckIn]=tripStages.getStage( stCheckIn );
    stage_statuses[stBoarding]=tripStages.getStage( stBoarding );

    TQuery Qry(&OraSession);
    TReqInfo *reqInfo = TReqInfo::Instance();
    //std::map<TStage, TDateTime> stages
    stage_times.clear();
    TStagesRules *sr = TStagesRules::Instance();
    TCkinClients ckin_clients;
    TTripStages::ReadCkinClients( oper.point_id, ckin_clients );
    TStage stage;
    for(int pass=0; pass<7; pass++)
    {
      switch(pass)
      {
        case 0: if ( reqInfo->client_type == ctKiosk )
                  stage=sOpenKIOSKCheckIn;
                else
                  stage=sOpenWEBCheckIn;
                break;
        case 1: if ( reqInfo->client_type == ctKiosk )
                  stage=sCloseKIOSKCheckIn;
                else
                  stage=sCloseWEBCheckIn;
                break;
        case 2: if ( reqInfo->client_type == ctKiosk )
                  continue;
                else
                  stage=sCloseWEBCancel;
                break;
        case 3: stage=sOpenCheckIn;
                break;
        case 4: stage=sCloseCheckIn;
                break;
        case 5: stage=sOpenBoarding;
                break;
        case 6: stage=sCloseBoarding;
                break;
      };

      if ( reqInfo->client_type == ctKiosk && first_segment )
      {
        //проверяем возможность регистрации для киоска только на первом сегменте
        TCkinClients::iterator iClient=find(ckin_clients.begin(),ckin_clients.end(),EncodeClientType(reqInfo->client_type));
        if (iClient!=ckin_clients.end())
        {
          Qry.Clear();
          Qry.SQLText=
            "SELECT pr_permit FROM trip_ckin_client "
            "WHERE point_id=:point_id AND client_type=:client_type AND desk_grp_id=:desk_grp_id";
          Qry.CreateVariable("point_id", otInteger, oper.point_id);
          Qry.CreateVariable("client_type", otString, EncodeClientType(reqInfo->client_type));
          Qry.CreateVariable("desk_grp_id", otInteger, reqInfo->desk.grp_id);
          Qry.Execute();
          if (Qry.Eof || Qry.FieldAsInteger("pr_permit")==0)
          {
            ckin_clients.erase(iClient);
            if (stage_statuses[stKIOSKCheckIn]==sOpenKIOSKCheckIn ||
                stage_statuses[stKIOSKCheckIn]==sCloseKIOSKCheckIn)
              stage_statuses[stKIOSKCheckIn]=sNoActive;
          };
        };
      };

      if (!( sr->isClientStage( (int)stage ) && !sr->canClientStage( ckin_clients, (int)stage ) ))
      {
        TTripStageTimes local_times=tripStages.getStageTimes(stage);
        if (local_times.scd!=NoExists) local_times.scd = UTCToLocal( local_times.scd, AirpTZRegion(oper.airp) );
        if (local_times.est!=NoExists) local_times.est = UTCToLocal( local_times.est, AirpTZRegion(oper.airp) );
        if (local_times.act!=NoExists) local_times.act = UTCToLocal( local_times.act, AirpTZRegion(oper.airp) );
        stage_times.insert( make_pair(stage, local_times ) );
      };
    };

    Qry.Clear();
    Qry.SQLText =
      "SELECT trip_paid_ckin.pr_permit AS pr_paid_ckin "
      "FROM trip_paid_ckin "
      "WHERE trip_paid_ckin.point_id=:point_id";
    Qry.CreateVariable( "point_id", otInteger, oper.point_id );
    Qry.Execute();
    pr_paid_ckin = false;
    if (!Qry.Eof)
      pr_paid_ckin = Qry.FieldAsInteger("pr_paid_ckin")!=0;
    free_seating=SALONS2::isFreeSeating(oper.point_id);
    if (reqInfo->isSelfCkinClientType())
      have_to_select_seats = GetSelfCkinSets(tsRegWithSeatChoice, oper, reqInfo->client_type);
  }
  catch(UserException &E)
  {
    ProgTrace(TRACE5, ">>>> %s", getLocaleText(E.getLexemaData()).c_str());
    if ( pr_throw )
      throw;
    else
      return false;
  };
  return true;
};

bool TPNRSegInfo::fromTestPax(int point_id, const TTripRoute &route, const TTestPaxInfo &pax)
{
  clear();
  TTripRoute::const_iterator r=route.begin();
  if (r==route.end()) return false;

  point_dep=point_id;
  point_arv=r->point_id;
  pnr_id=pax.pax_id;
  try
  {
    cls=((const TSubclsRow&)base_tables.get( "subcls" ).get_row( "code", pax.subcls, true )).cl;
  }
  catch(EBaseTableError)
  {
    return false;
  };
  subcls=pax.subcls;
  if (!pax.pnr_addr.airline.empty() && !pax.pnr_addr.addr.empty())
    pnr_addrs.push_back(pax.pnr_addr);
  return true;
};

void TFlightInfo::add(const TDestInfo &dest)
{
  if (dest.point_arv==NoExists) throw EXCEPTIONS::Exception("TFlightInfo::add: dest.point_arv not defined");
  dests.insert(dest);
};

const TDestInfo& TFlightInfo::getDestInfo(int point_arv) const
{
  std::set<WebSearch::TDestInfo>::const_iterator iDest=dests.find(WebSearch::TDestInfo(point_arv));
  if (iDest==dests.end())
    throw EXCEPTIONS::Exception("TFlightInfo::getDestInfo: dest not found (point_arv=%d)", point_arv);
  return *iDest;
}

boost::optional<TStage> TFlightInfo::stage(const TStage_Type& type) const
{
  map<TStage_Type, TStage>::const_iterator iStatus=stage_statuses.find(type);
  if (iStatus!=stage_statuses.end())
    return iStatus->second;

  return boost::none;
}

boost::optional<TStage> TFlightInfo::stage() const
{
  if ( act_out_local != NoExists )
    return sTakeoff;

  return stage(TReqInfo::Instance()->client_type == ctKiosk?stKIOSKCheckIn:stWEBCheckIn);
}

void TFlightInfo::toXMLsimple(xmlNodePtr node, XMLStyle xmlStyle) const
{
  if (node==NULL) return;
  oper.point_id==NoExists?NewTextChild(node, xmlStyle==xmlSearchPNRs?"point_dep":"point_id"):
                          NewTextChild(node, xmlStyle==xmlSearchPNRs?"point_dep":"point_id", oper.point_id);
  NewTextChild(node, "airline", airlineToPrefferedCode(oper.airline, AstraLocale::OutputLang()));
  oper.flt_no==NoExists?NewTextChild(node, "flt_no"):
                        NewTextChild(node, "flt_no", oper.flt_no);
  NewTextChild(node, "suffix", ElemIdToCodeNative(etSuffix, oper.suffix));
}

void TFlightInfo::toXML(xmlNodePtr node, XMLStyle xmlStyle) const
{
/*
  <point_dep> ид. пункта вылета = ид. рейса
  <airline>
  <flt_no>
  <suffix>
  <craft> тип ВС
  <scd_out>DD.MM.YYYY HH24:MI:SS плановое время вылета
  <est_out>DD.MM.YYYY HH24:MI:SS расчетное время вылета
  <act_out>DD.MM.YYYY HH24:MI:SS фактическое время вылета
  <airp_dep> порт вылета
  <city_dep> город вылета
  <dep_utc_offset> смещение локального времени пункта вылета относительно UTC. Указывается в минутах!
  <dests>
    <dest>
      ...
    </dest>
    ...
  </dests>
  <status> текущий статус саморегистрации
           для web-регистрации: sNoActive/sOpenWEBCheckIn/sCloseWEBCheckIn/sTakeoff
           для киосков:         sNoActive/sOpenKIOSKCheckIn/sCloseKIOSKCheckIn/sTakeoff
  <stages>
    для web-регистрации:
      <stage type='sOpenWEBCheckIn'>DD.MM.YYYY HH24:MI:SS начало WEB-регистрации
      <stage type='sCloseWEBCheckIn'>                     окончание WEB-регистрации
      <stage type="sCloseWEBCancel">                      запрет отмены WEB-регистрации
    для киосков:
      <stage type='sOpenKIOSKCheckIn'>  начало регистрации через киоски
      <stage type='sCloseKIOSKCheckIn'> окончание регистрации через киоски
    <stage type='sOpenCheckIn'>     начало обычной регистрации
    <stage type='sCloseCheckIn'>    окончание обычной регистрации
    <stage type='sOpenBoarding'>    начало посадки
    <stage type='sCloseBoarding'>   окончание посадки
  </stages>
  <semaphors>
    для web-регистрации:
      <web_checkin>   0/1 признак активноcти web-регистрации
      <web_cancel>    0/1 признак активности отмены web-регистрации с сайта
    для киосков:
      <kiosk_checkin>   0/1 признак активноcти регистрации через киоски
    <term_checkin>  0/1 признак активноcти обычной регистрации
    <term_brd>      0/1 признак активноcти посадки
  </semaphors>
  <paid_checkin>  0/1 признак платной регистрации на рейсе
  <free_seating>  0/1 признак свободной рассадки на рейсе (не назначен салон)
  <have_to_select_seats>  0/1 признак требуется ли выбирать место для регистрации
  <mark_flights> секция коммерческих рейсов, связанных с оперирующим
    <flight>
      <airline>
      <flt_no>
      <suffix>
    </flight>
    ...
  </mark_flights>
*/
  if (node==NULL) return;
  toXMLsimple(node, xmlStyle);
  NewTextChild(node, "craft", craftToPrefferedCode(oper.craft, AstraLocale::OutputLang()));
  NewTextChild(node, "scd_out", scd_out_local==NoExists?"":DateTimeToStr(scd_out_local, ServerFormatDateTimeAsString));
  NewTextChild(node, "est_out", est_out_local==NoExists?"":DateTimeToStr(est_out_local, ServerFormatDateTimeAsString));
  NewTextChild(node, "act_out", act_out_local==NoExists?"":DateTimeToStr(act_out_local, ServerFormatDateTimeAsString));
  NewTextChild(node, "airp_dep", airpToPrefferedCode(oper.airp, AstraLocale::OutputLang()));
  NewTextChild(node, "city_dep", ElemIdToCodeNative(etCity, city_dep));
  dep_utc_offset==NoExists?NewTextChild(node, "dep_utc_offset"):
                           NewTextChild(node, "dep_utc_offset", dep_utc_offset);
  if (xmlStyle==xmlSearchPNRs)
  {
    xmlNodePtr destsNode=NewTextChild(node, "dests");
    for(std::set<TDestInfo>::const_iterator i=dests.begin();i!=dests.end();++i)
      i->toXML(NewTextChild(destsNode, "dest"), xmlStyle);
  };

  if (boost::optional<TStage> opt_stage=stage())
  {
    switch ( opt_stage.get() ) {
      case sNoActive:
        NewTextChild( node, "status", "sNoActive" );
        break;
      case sOpenWEBCheckIn:
        NewTextChild( node, "status", "sOpenWEBCheckIn" );
        break;
      case sOpenKIOSKCheckIn:
        NewTextChild( node, "status", "sOpenKIOSKCheckIn" );
        break;
      case sCloseWEBCheckIn:
        NewTextChild( node, "status", "sCloseWEBCheckIn" );
        break;
      case sCloseKIOSKCheckIn:
        NewTextChild( node, "status", "sCloseKIOSKCheckIn" );
        break;
      case sTakeoff:
        NewTextChild( node, "status", "sTakeoff" );
        break;
      default:
        NewTextChild( node, "status" );
        break;
    };
  }
  else NewTextChild( node, "status" );

  TReqInfo *reqInfo = TReqInfo::Instance();
  xmlNodePtr stagesNode = NewTextChild( node, "stages" );
  xmlNodePtr stageNode;
  TStage stage;
  string stage_name;
  for(int pass=0; pass<7; pass++)
  {
    switch(pass)
    {
      case 0: if ( reqInfo->client_type == ctKiosk )
              {
                stage=sOpenKIOSKCheckIn;
                stage_name="sOpenKIOSKCheckIn";
              }
              else
              {
                stage=sOpenWEBCheckIn;
                stage_name="sOpenWEBCheckIn";
              };
              break;
      case 1: if ( reqInfo->client_type == ctKiosk )
              {
                stage=sCloseKIOSKCheckIn;
                stage_name="sCloseKIOSKCheckIn";
              }
              else
              {
                stage=sCloseWEBCheckIn;
                stage_name="sCloseWEBCheckIn";
              };
              break;
      case 2: if ( reqInfo->client_type == ctKiosk )
                continue;
              else
              {
                stage=sCloseWEBCancel;
                stage_name="sCloseWEBCancel";
              };
              break;
      case 3: stage=sOpenCheckIn;
              stage_name="sOpenCheckIn";
              break;
      case 4: stage=sCloseCheckIn;
              stage_name="sCloseCheckIn";
              break;
      case 5: stage=sOpenBoarding;
              stage_name="sOpenBoarding";
              break;
      case 6: stage=sCloseBoarding;
              stage_name="sCloseBoarding";
              break;
    };

    TTripStageTimes t;
    map<TStage, TTripStageTimes>::const_iterator iStage=stage_times.find(stage);
    if (iStage!=stage_times.end()) t=iStage->second;

    stageNode = NewTextChild( stagesNode, "stage", (t.time()!=NoExists?DateTimeToStr( t.time(), ServerFormatDateTimeAsString ):"") );

    SetProp( stageNode, "type", stage_name );
    SetProp( stageNode, "scd", (t.scd!=NoExists?DateTimeToStr( t.scd, ServerFormatDateTimeAsString ):"") );
    SetProp( stageNode, "est", (t.est!=NoExists?DateTimeToStr( t.est, ServerFormatDateTimeAsString ):"") );
    SetProp( stageNode, "act", (t.act!=NoExists?DateTimeToStr( t.act, ServerFormatDateTimeAsString ):"") );
  };

  xmlNodePtr semNode = NewTextChild( node, "semaphors" );
  TStage_Type stage_type;
  string sem_name;
  for(int pass=0; pass<4; pass++)
  {
    switch(pass)
    {
      case 0: if ( reqInfo->client_type == ctKiosk )
              {
                stage_type=stKIOSKCheckIn;
                stage=sOpenKIOSKCheckIn;
                sem_name="kiosk_checkin";
              }
              else
              {
                stage_type=stWEBCheckIn;
                stage=sOpenWEBCheckIn;
                sem_name="web_checkin";
              };
              break;
      case 1: if ( reqInfo->client_type == ctKiosk )
                continue;
              else
              {
                stage_type=stWEBCancel;
                stage=sOpenWEBCheckIn;
                sem_name="web_cancel";
              };
              break;
      case 2: stage_type=stCheckIn;
              stage=sOpenCheckIn;
              sem_name="term_checkin";
              break;
      case 3: stage_type=stBoarding;
              stage=sOpenBoarding;
              sem_name="term_brd";
              break;
    };
    map<TStage_Type, TStage>::const_iterator iStatus=stage_statuses.find(stage_type);
    bool sem= act_out_local==NoExists &&
              iStatus!=stage_statuses.end() &&
              iStatus->second==stage;
    NewTextChild( semNode, sem_name.c_str(), (int)sem );
  };

  NewTextChild( node, "paid_checkin", (int)pr_paid_ckin );
  NewTextChild( node, "free_seating", (int)free_seating );
  NewTextChild( node, "have_to_select_seats", (int)have_to_select_seats );

  xmlNodePtr fltsNode = NewTextChild( node, "mark_flights" );
  for(const TSimpleMktFlight& m : mark)
    m.toXML(NewTextChild( fltsNode, "flight" ), AstraLocale::OutputLang());
};

bool TPNRSegInfo::fromDB(int point_id, const TTripRoute &route, TQuery &Qry)
{
  clear();
  if (Qry.Eof) return false;
  boost::optional<TTripRouteItem> arv=route.findFirstAirp(Qry.FieldAsString("airp_arv"));
  if (!arv) return false;

  point_dep=point_id;
  point_arv=arv.get().point_id;
  pnr_id=Qry.FieldAsInteger("pnr_id");
  cls=Qry.FieldAsString("class");
  subcls=Qry.FieldAsString("subclass");

  pnr_addrs.getByPnrIdFast(pnr_id);

  mktFlight=TMktFlight();
  mktFlight.get().getByPnrId(pnr_id);
  if (mktFlight.get().empty())
    throw EXCEPTIONS::Exception("TPNRSegInfo::fromDB: mktFlight.get().empty() (pnr_id=%d)",pnr_id);

  return true;
};

bool TPNRSegInfo::filterFromDB(const TPNRFilter &filter)
{
  return filter.isEqualPnrAddr(pnr_addrs);
};

bool TFlightInfo::setIfSuitable(const TPNRFilter &filter,
                                const TAdvTripInfo& oper,
                                const TSimpleMktFlight &mark)
{
  clear();

  if (!filter.isEqualFlight(oper, mark)) return false;

  set(oper);
  return true;
}

bool TPNRSegInfo::setIfSuitable(const TPNRFilter &filter,
                                const TAdvTripInfo& flt,
                                const CheckIn::TSimplePaxItem& pax)
{
  clear();

  bool checked=pax.grp_id!=ASTRA::NoExists;

  TPnrAddrs pnrAddrs;
  pnrAddrs.getByPaxIdFast(pax.id);

  if (!filter.isEqualPnrAddr(pnrAddrs)) return false;

  CheckIn::TSimplePnrItem pnr;
  if (!pnr.getByPaxId(pax.id)) return false;

  CheckIn::TSimplePaxGrpItem grp;

  if (checked)
  {
    if (!grp.getByGrpId(pax.grp_id)) return false;
  }

  TTripRoute route;
  route.GetRouteAfter( NoExists,
                       flt.point_id,
                       flt.point_num,
                       flt.first_point,
                       flt.pr_tranzit,
                       trtNotCurrent,
                       trtNotCancelled );

  boost::optional<TTripRouteItem> arv=route.findFirstAirp(checked?grp.airp_arv:pnr.airp_arv);
  if (!arv) return false;

  TMktFlight markFlt;
  checked?markFlt.getByPaxId(pax.id):markFlt.getByPnrId(pnr.id);
  if (markFlt.empty()) return false;

  point_dep=flt.point_id;
  point_arv=arv.get().point_id;
  pnr_id=pnr.pnrId();
  cls=checked?grp.cl:pnr.cl;
  subcls=pax.subcl;
  pnr_addrs=pnrAddrs;
  mktFlight=markFlt;

  return true;
}

void TPNRSegInfo::toXML(xmlNodePtr node, XMLStyle xmlStyle) const
{
/*
  <point_dep>   ид пункта вылета из секции oper_flights/flight
  <point_arv>   ид пункта прилета из секции oper_flights/flight/dests/dest
  <pnr_id>     ид PNR
  <subclass>   подкласс на сегменте
  <pnr_addrs>  список номеров PNR
    <pnr_addr>
      <airline> авиакомпания PNR
      <addr>    собственно номер PNR
    </pnr_addr>
    ...
  </pnr_addrs>
*/
  if (node==NULL) return;
  if (xmlStyle==xmlSearchPNRs)
  {
    point_dep==NoExists?NewTextChild(node, "point_dep"):
                        NewTextChild(node, "point_dep", point_dep);
    point_arv==NoExists?NewTextChild(node, "point_arv"):
                        NewTextChild(node, "point_arv", point_arv);
  };

  if (xmlStyle!=xmlSearchFltMulti)
  {
    pnr_id==NoExists?NewTextChild(node, "pnr_id"):
                     NewTextChild(node, "pnr_id", pnr_id);
    NewTextChild(node, "subclass", ElemIdToCodeNative(etSubcls, subcls));
    pnr_addrs.toXML(node, AstraLocale::OutputLang());
  }

  xmlNodePtr fltNode=GetNode("pnr_mark_flight", node);
  if (fltNode==nullptr)
  {
    fltNode=NewTextChild(node, "pnr_mark_flight");
    if (mktFlight)
      mktFlight.get().toXML(fltNode, AstraLocale::OutputLang());
  };

  if (xmlStyle==xmlSearchFltMulti)
  {
    xmlNodePtr idsNode=GetNode("crs_pnr_ids", node);
    if (idsNode!=nullptr)
    {
      set<int> crs_pnr_ids;
      for(xmlNodePtr idNode=idsNode->children; idNode!=nullptr; idNode=idNode->next)
        if (string((const char*)idNode->name)=="crs_pnr_id") crs_pnr_ids.insert(NodeAsInteger(idNode));

      if (crs_pnr_ids.insert(pnr_id).second)
        NewTextChild(idsNode, "crs_pnr_id", pnr_id);
    }
  };
}

bool TPaxInfo::fromTestPax(const TTestPaxInfo &pax)
{
  clear();

  pax_id=pax.pax_id;
  surname=pax.surname;
  name=pax.name;
  ticket_no=pax.ticket_no;
  document=pax.document;
  reg_no=NoExists; //не pax.reg_no, так как считаем что изначально тестовый пассажир не зарегистрирован
  return true;
};

bool TPaxInfo::filterFromDB(const TPNRFilter &filter, TQuery &Qry, bool ignore_reg_no)
{
  clear();
  if (Qry.Eof) return false;
  pax_id=Qry.FieldAsInteger("pax_id");
  surname=Qry.FieldAsString("surname");
  name=Qry.FieldAsString("name");
  if (!filter.isEqualName(name)) return false;

  CheckIn::TPaxTknItem tkn;
  LoadCrsPaxTkn(pax_id, tkn);
  ticket_no=tkn.no;
  if (!filter.isEqualTkn(ticket_no)) return false;

  CheckIn::TPaxDocItem doc;
  LoadCrsPaxDoc(pax_id, doc);
  document=doc.no;
  if (!filter.isEqualDoc(document)) return false;

  TQuery Qry1(&OraSession);
  Qry1.Clear();
  Qry1.SQLText="SELECT reg_no FROM pax WHERE pax_id=:pax_id";
  Qry1.CreateVariable("pax_id", otInteger, pax_id);
  Qry1.Execute();
  if (!Qry1.Eof) reg_no=Qry1.FieldAsInteger("reg_no");
  if (!ignore_reg_no &&
      !filter.isEqualRegNo(reg_no)) return false;

  return true;
}

bool TPaxInfo::setIfSuitable(const TPNRFilter& filter, const CheckIn::TSimplePaxItem& pax, bool ignore_reg_no)
{
  clear();
  if (!filter.isEqualSurname(pax.surname)) return false;
  if (!filter.isEqualName(pax.name)) return false;
  if (!ignore_reg_no &&
      !filter.isEqualRegNo(pax.reg_no)) return false;

  CheckIn::TPaxTknItem tkn(pax.tkn);
  if (!pax.TknExists)
    pax.grp_id!=ASTRA::NoExists?LoadPaxTkn(pax.id, tkn):
                                LoadCrsPaxTkn(pax.id, tkn);

  if (!filter.isEqualTkn(tkn.no)) return false;

  CheckIn::TPaxDocItem doc;
  pax.grp_id!=ASTRA::NoExists?LoadPaxDoc(pax.id, doc):
                              LoadCrsPaxDoc(pax.id, doc);

  if (!filter.isEqualDoc(doc.no)) return false;

  pax_id=pax.id;
  surname=pax.surname;
  name=pax.name;
  ticket_no=tkn.no;
  document=doc.no;
  reg_no=pax.reg_no;
  return true;
}

void TPaxInfo::toXML(xmlNodePtr node) const
{
/*
  <pax>
    <surname>   //фамилия
    <name>      //имя
    <ticket_no> //номер билета
    <document>  //номер документа
    <reg_no>    //рег. номер
  </pax>
*/
  if (node==NULL) return;
  NewTextChild(node, "surname", surname);
  NewTextChild(node, "name", name);
  NewTextChild(node, "ticket_no", ticket_no);
  NewTextChild(node, "document", document);
  reg_no==NoExists?NewTextChild(node, "reg_no"):
                   NewTextChild(node, "reg_no", reg_no);
};

void checkPnrData(const TFlightInfo &flt, const TDestInfo &dest, const TPNRSegInfo &seg, const string &where)
{
  if (flt.oper.point_id==NoExists || flt.oper.point_id!=seg.point_dep)
    throw EXCEPTIONS::Exception("%s: wrong flt.point_dep %d", where.c_str(), flt.oper.point_id);
  if (dest.point_arv==NoExists || dest.point_arv!=seg.point_arv)
    throw EXCEPTIONS::Exception("%s: wrong dest.point_arv %d", where.c_str(), dest.point_arv);
  if (seg.pnr_id==NoExists)
    throw EXCEPTIONS::Exception("%s: pnr_id not defined", where.c_str());
};

void TPNRSegInfo::getMarkFlt(const TFlightInfo &flt, TTripInfo &mark) const
{
  mark.Clear();
  if (mktFlight)
  {
    //коммерческий рейс PNR
    mark.airline=mktFlight.get().airline;
    mark.flt_no=mktFlight.get().flt_no;
    mark.suffix=mktFlight.get().suffix;
    mark.airp=mktFlight.get().airp_dep;
    mark.scd_out=mktFlight.get().scd_date_local;
  }
  else
  {
    mark.airline=flt.oper.airline;
    mark.flt_no=flt.oper.flt_no;
    mark.suffix=flt.oper.suffix;
    mark.airp=flt.oper.airp;
    mark.scd_out=UTCToLocal(flt.oper.scd_out, AirpTZRegion(flt.oper.airp));
  };

};

bool TPNRInfo::fromDBadditional(const TFlightInfo &flt, const TDestInfo &dest)
{
  if (segs.empty())
    throw EXCEPTIONS::Exception("TPNRInfo::fromDBadditional: empty segs");
  checkPnrData(flt, dest, segs.begin()->second, "TPNRInfo::fromDBadditional");

  TTripInfo pnrMarkFlt;
  segs.begin()->second.getMarkFlt(flt, pnrMarkFlt);
  TCodeShareSets codeshareSets;
  codeshareSets.get(flt.oper,pnrMarkFlt);

  bag_norm = NoExists; //ничего не выводим в качестве нормы
  if (!TTripSetList().fromDB(flt.oper.point_id).value(tsPieceConcept, true)) //не разрешаем расчет по кол-ву мест
  {
    WeightConcept::TNormFltInfo fltInfo;
    fltInfo.point_id=flt.oper.point_id;
    fltInfo.use_mark_flt=codeshareSets.pr_mark_norms;
    fltInfo.airline_mark=pnrMarkFlt.airline;
    fltInfo.flt_no_mark=pnrMarkFlt.flt_no;
    fltInfo.use_mixed_norms=GetTripSets(tsMixedNorms,flt.oper);
    WeightConcept::TPaxInfo paxInfo;
    paxInfo.target=dest.city_arv;
    paxInfo.final_target=""; //трансфер пока не анализируем
    paxInfo.subcl=segs.begin()->second.subcls;
    paxInfo.cl=segs.begin()->second.cls;

    WeightConcept::TBagNormInfo norm;
    WeightConcept::CheckOrGetPaxBagNorm(fltInfo, paxInfo, false, WeightConcept::REGULAR_BAG_TYPE, WeightConcept::TPaxNormItem(), norm); //обычный багаж
    if (!norm.empty())
      bag_norm=norm.weight;
  };

  return true;
};

void TPNRInfo::add(const TPaxInfo &pax)
{
  if (pax.pax_id==NoExists) throw EXCEPTIONS::Exception("TPNRInfo::add: pax.pax_id not defined");
  paxs.insert(pax);
};

void TPNRInfo::toXML(xmlNodePtr node, XMLStyle xmlStyle) const
{
/*
  <pnr>
    <segments> сегменты трансфера
      <segment>
        ...
      </segment>
      ...
    </segments>
    <bag_norm>   //норма одного пассажира для обычного багажа или р/к (без учета категории) для первого сегмента
    <passengers> //список пассажиров, подходящих под критерии поиска, для первого сегмента
      <pax>
        ...
      </pax>
      ...
    </passengers>
  </pnr>
*/

  if (node==NULL) return;
  if (xmlStyle==xmlSearchPNRs)
  {
    xmlNodePtr segsNode=NewTextChild(node, "segments");
    for(map< int/*num*/, TPNRSegInfo >::const_iterator i=segs.begin(); i!=segs.end(); ++i)
    {
      xmlNodePtr segNode=NewTextChild(segsNode, "segment");
      i->second.toXML(segNode, xmlStyle);
    }
  };

  if (xmlStyle!=xmlSearchFltMulti)
  {
    bag_norm==NoExists?NewTextChild(node, "bag_norm"):
                       NewTextChild(node, "bag_norm", bag_norm);
  }
  else
  {
    xmlNodePtr idsNode=GetNode("crs_pax_ids", node);
    if (idsNode!=nullptr)
    {
      set<int> crs_pax_ids;
      for(xmlNodePtr idNode=idsNode->children; idNode!=nullptr; idNode=idNode->next)
        if (string((const char*)idNode->name)=="crs_pax_id") crs_pax_ids.insert(NodeAsInteger(idNode));

      for(const TPaxInfo& pax : paxs)
        if (crs_pax_ids.insert(pax.pax_id).second)
          NewTextChild(idsNode, "crs_pax_id", pax.pax_id);
    };
  }

  if (xmlStyle==xmlSearchPNRs)
  {
    xmlNodePtr paxsNode=NewTextChild(node, "passengers");
    for(set<TPaxInfo>::const_iterator i=paxs.begin(); i!=paxs.end(); ++i)
      i->toXML(NewTextChild(paxsNode, "pax"));
  };
};

int TPNRInfo::getFirstPointDep() const
{
  if (segs.empty())
    throw EXCEPTIONS::Exception("TPNRInfo::getFirstPointDep: empty segs");
  return segs.begin()->second.point_dep;
}

bool TPNRs::add(const TFlightInfo &flt, const TPNRSegInfo &seg, const TPaxInfo &pax, bool is_test)
{
  if (flt.oper.point_id==NoExists) throw EXCEPTIONS::Exception("TPNRs::add: flt.point_dep not defined");
  if (seg.point_arv==NoExists) throw EXCEPTIONS::Exception("TPNRs::add: seg.point_arv not defined");
  if (seg.pnr_id==NoExists) throw EXCEPTIONS::Exception("TPNRs::add: seg.pnr_id not defined");

  map< int/*pnr_id*/, TPNRInfo >::iterator iPNR=pnrs.find(seg.pnr_id);
  if (iPNR!=pnrs.end())
  {
    //такой PNR уже есть в PNRs - просто добавим пассажира
    iPNR->second.add(pax);
    return true;
  };

  TPnrData first;
  first.flt=flt;
  first.dest.point_arv=seg.point_arv;
  first.seg=seg;

  set<TFlightInfo>::const_iterator iFlt=flights.find(first.flt);
  if (iFlt==flights.end())
  {
    //рейса нет
    if (!first.flt.fromDBadditional(true,false)) return false;
    if (!first.dest.fromDB(seg.point_arv,false)) return false;
  }
  else
  {
    set<TDestInfo>::const_iterator iDest=iFlt->dests.find(first.dest);
    if (iDest!=iFlt->dests.end())
      first.dest=*iDest;
    else
      if (!first.dest.fromDB(seg.point_arv,false)) return false;
  };

  //здесь имеем валидные first.flt, first.dest, first.seg

  TPNRInfo pnr;
  pnr.segs[1]=first.seg;
  pnr.add(pax);
  if (!pnr.fromDBadditional(first.flt, first.dest)) return false;

  vector<TPnrData> other;
  getTCkinData(first, is_test, other);

  //здесь имеем валидное other

  int s=1;
  for(vector<TPnrData>::const_iterator i=other.begin(); s==1 || i!=other.end(); s++)
  {
    const TPnrData &d=(s==1?first:*i);

    ((TFlightInfo&)*flights.insert(d.flt).first).add(d.dest);

    if (s!=1)
    {
      pnr.segs[s]=d.seg;
      ++i;
    };
  };

  pnrs[seg.pnr_id] = pnr;

  return true;
};

const TFlightInfo& TPNRs::getFlightInfo(int point_dep) const
{
  set<WebSearch::TFlightInfo>::const_iterator iFlt=flights.find(WebSearch::TFlightInfo(point_dep));
  if (iFlt==flights.end())
    throw EXCEPTIONS::Exception("TPNRs::getFlightInfo: flight not found (point_dep=%d)", point_dep);
  return *iFlt;
}

const TPNRInfo& TPNRs::getPNRInfo(int pnr_id) const
{
  map< int/*pnr_id*/, TPNRInfo >::const_iterator iPnr=pnrs.find(pnr_id);
  if (iPnr==pnrs.end())
    throw EXCEPTIONS::Exception("TPNRs::getPNRInfo: pnr not found (pnr_id=%d)", pnr_id);
  return iPnr->second;
}

int TPNRs::getFirstPnrId() const
{
  if (pnrs.empty())
    throw EXCEPTIONS::Exception("TPNRs::getFirstPnrId: empty pnrs");
  return pnrs.begin()->first;
}

const TPNRInfo& TPNRs::getFirstPNRInfo() const
{
  if (pnrs.empty())
    throw EXCEPTIONS::Exception("TPNRs::getFirstPNRInfo: empty pnrs");
  return pnrs.begin()->second;
}

TPNRInfo& TPNRs::getFirstPNRInfo()
{
  if (pnrs.empty())
    throw EXCEPTIONS::Exception("TPNRs::getFirstPNRInfo: empty pnrs");
  return pnrs.begin()->second;
}

int TPNRs::calcStagePriority(int pnr_id) const
{
  return getFlightInfo(getPNRInfo(pnr_id).getFirstPointDep()).getStagePriority();
}

boost::optional<TDateTime> TPNRs::getFirstSegTime() const
{
  const TFlightInfo& info=getFlightInfo(getFirstPNRInfo().getFirstPointDep());
  if (info.oper.scd_out!=NoExists)
    return info.oper.scd_out;
  return boost::none;
}

bool TPNRsSortOrder::operator () (const TPNRs &item1, const TPNRs &item2) const
{
  boost::optional<TDateTime> time1=item1.getFirstSegTime();
  boost::optional<TDateTime> time2=item2.getFirstSegTime();
  if ((time1==boost::none)!=(time2==boost::none))
    return time1!=boost::none;

  if (time1 && time2)
  {
    if (_timePoint!=NoExists)
    {
      bool past1=time1.get()<_timePoint;
      bool past2=time2.get()<_timePoint;
      if (past1!=past2)
        return past1<past2;

      double timeAbsDistance1=abs(_timePoint-time1.get());
      double timeAbsDistance2=abs(_timePoint-time2.get());

      if (timeAbsDistance1!=timeAbsDistance2)
        return timeAbsDistance1<timeAbsDistance2;
    }
    else
    {
      if (time1.get() != time2.get())
        return time1.get() < time2.get();
    }
  }

  return false;
}

void TPNRs::trace(XMLStyle xmlStyle) const
{
  xmlNodePtr rootNode;
  XMLDoc doc("PNRs", rootNode, "TPNRs::trace");
  toXML(rootNode, true, xmlStyle);
  LogTrace(TRACE5) << doc.text();
}

void TPNRs::toXML(xmlNodePtr node, bool is_primary, XMLStyle xmlStyle) const
{
/*
  секция оперирующих рейсов:
  <oper_flights>
    <flight>
      ...
    </flight>
  </oper_flights>

  секция PNRs и пассажиров:
  <pnrs>
    <pnr>
      ...
    <pnr>
    ...
  </pnrs>
*/
  if (node==NULL) return;
  if (flights.empty() && pnrs.empty()) return;

  if (xmlStyle==xmlSearchPNRs)
  {
    xmlNodePtr fltsNode=NewTextChild(node, "oper_flights");
    for(set<TFlightInfo>::const_iterator i=flights.begin(); i!=flights.end(); ++i)
      i->toXML(NewTextChild(fltsNode, "flight"), xmlStyle);
    xmlNodePtr pnrsNode=NewTextChild(node, "pnrs");
    for(map< int/*pnr_id*/, TPNRInfo >::const_iterator i=pnrs.begin(); i!=pnrs.end(); ++i)
      i->second.toXML(NewTextChild(pnrsNode, "pnr"), xmlStyle);
  }
  else
  {
    const TPNRInfo& pnrInfo=getFirstPNRInfo();

    xmlNodePtr segsNode=GetNode("segments", node);
    if (is_primary)
    {
      if (segsNode!=nullptr)
        throw EXCEPTIONS::Exception("TPNRs::toXML: segsNode!=nullptr!");
      xmlNodePtr segsNode=NewTextChild(node, "segments");
      for(map< int/*num*/, TPNRSegInfo >::const_iterator iSeg=pnrInfo.segs.begin(); iSeg!=pnrInfo.segs.end(); ++iSeg)
      {
        const TPNRSegInfo& pnrSegInfo=iSeg->second;

        xmlNodePtr segNode=NewTextChild(segsNode, "segment");
        getFlightInfo(pnrSegInfo.point_dep).toXML(segNode, xmlStyle);
        getFlightInfo(pnrSegInfo.point_dep).getDestInfo(pnrSegInfo.point_arv).toXML(segNode, xmlStyle);

        if (xmlStyle==xmlSearchFltMulti)
        {
          if (iSeg==pnrInfo.segs.begin())
            NewTextChild(segNode, "crs_pax_ids");
          else
            NewTextChild(segNode, "crs_pnr_ids");
        }

        pnrSegInfo.toXML(segNode, xmlStyle);
        pnrInfo.toXML(segNode, xmlStyle);
      }
    }
    else
    {
      if (segsNode!=nullptr)
      {
        map< int/*num*/, TPNRSegInfo >::const_iterator iSeg=pnrInfo.segs.begin();
        for(xmlNodePtr segNode=segsNode->children; segNode!=nullptr && iSeg!=pnrInfo.segs.end(); segNode=segNode->next, ++iSeg)
        {
          const TPNRSegInfo& pnrSegInfo=iSeg->second;

          pnrSegInfo.toXML(segNode, xmlStyle);
          pnrInfo.toXML(segNode, xmlStyle);
        }
      }
    }
  }
}

void TMultiPNRs::toXML(xmlNodePtr segsParentNode, xmlNodePtr grpsParentNode, bool is_primary, XMLStyle xmlStyle) const
{
  if (segsParentNode==nullptr || grpsParentNode==nullptr) return;

  if (errors.empty())
    TPNRs::toXML(segsParentNode, is_primary, xmlStyle);

  if (!errors.empty() || !warnings.empty())
  {
    for(int pass=0; pass<2; pass++)
    {
      if (!(is_primary && !errors.empty()) && pass==0) continue;
      xmlNodePtr grpNode=grpsParentNode;
      if (group_id && pass!=0)
      {
        xmlNodePtr grpsNode=GetNode("groups", grpsParentNode);
        if (grpsNode==nullptr)
          grpsNode=NewTextChild(grpsParentNode, "groups");
        grpNode=NewTextChild(grpsNode, "group");
        SetProp(grpNode, "id", group_id.get());
      };

      string text, master_lexema_id;
      if (!errors.empty())
      {
        getLexemaText( errors.front(), text, master_lexema_id );
        ReplaceTextChild(grpNode, "error_code", master_lexema_id);
        ReplaceTextChild(grpNode, "error_message", text);
      }
      else if (!warnings.empty())
      {
        getLexemaText( warnings.front(), text, master_lexema_id );
        ReplaceTextChild(grpNode, "warning_code", master_lexema_id);
        ReplaceTextChild(grpNode, "warning_message", text);
      }
    }
  }
}

void TMultiPNRsList::toXML(xmlNodePtr resNode) const
{
  if (primary.empty())
    throw EXCEPTIONS::Exception("TMultiPNRsList::toXML: primary.empty()!");
  const TMultiPNRs& primaryPNRs=primary.front();

  xmlNodePtr segsParentNode=resNode;
  xmlNodePtr grpsParentNode=resNode;

  if (trueMultiResponse(resNode))
  {
    primaryPNRs.toXML(segsParentNode, grpsParentNode, true, xmlSearchFltMulti);
    for(const TMultiPNRs& secondaryPNRs : secondary)
      secondaryPNRs.toXML(segsParentNode, grpsParentNode, false, xmlSearchFltMulti);
  }
  else
    primaryPNRs.toXML(segsParentNode, grpsParentNode, true, xmlSearchFlt);
}

void TMultiPNRsList::checkGroups()
{
  if (primary.empty() && secondary.size()==1)
  {
    primary.emplace_back(secondary.front());
    secondary.pop_front();
  }

  if (primary.empty())
    throw EXCEPTIONS::Exception("%s: main group not defined", __FUNCTION__);

  if (primary.size()>1)
    throw EXCEPTIONS::Exception("%s: more than one main group found", __FUNCTION__);

  //проверим на уникальность group_id
  set<int> group_ids;
  for(int pass=0; pass<2; pass++)
    for(TMultiPNRs& multiPNRs : (pass==0?primary:secondary))
      if (multiPNRs.group_id && !group_ids.insert(multiPNRs.group_id.get()).second)
        throw EXCEPTIONS::Exception("%s: duplicate group id=%d", __FUNCTION__, multiPNRs.group_id.get());
}

bool TPNRSegInfo::isJointCheckInPossible(const TPNRSegInfo& seg1,
                                         const TPNRSegInfo& seg2,
                                         const TFlightRbd& rbds,
                                         std::list<AstraLocale::LexemaData>& errors)
{
  errors.clear();
  if (seg1.point_dep!=seg2.point_dep ||
      seg1.point_arv!=seg2.point_arv)
    errors.emplace_back("MSG.INCOMPATIBLE_SEGMENTS");
  if (seg1.cls!=seg2.cls)
    errors.emplace_back("MSG.DIFFERENT_CLASS");
  if (!(seg1.mktFlight==seg2.mktFlight))
    errors.emplace_back("MSG.DIFFERENT_MARK_FLIGHT");
  boost::optional<TSubclassGroup> subclass_grp1=rbds.getSubclassGroup(seg1.subcls, seg1.cls);
  boost::optional<TSubclassGroup> subclass_grp2=rbds.getSubclassGroup(seg2.subcls, seg2.cls);
  if (subclass_grp1!=subclass_grp2)
    errors.emplace_back("MSG.INCOMPATIBLE_FARE_CLASS");

  return errors.empty();
}

void TMultiPNRs::truncateSegments(int numOfSegs)
{
  if (pnrs.empty()) return;
  auto& segs=getFirstPNRInfo().segs;
  auto s=segs.begin();
  for(int i=0; s!=segs.end() && i<numOfSegs; ++s, i++);
  if (s!=segs.end())
  {
    warnings.emplace_back("MSG.SEGMENTS_TRUNCATED");
    segs.erase(s, segs.end());
  }
}

int TMultiPNRs::numberOfCompatibleSegments(const TMultiPNRs& PNRs1,
                                           const TMultiPNRs& PNRs2,
                                           std::map<int/*point_id*/, TFlightRbd>& flightRbdMap,
                                           std::list<AstraLocale::LexemaData>& errors)
{
  errors.clear();

  int result=0;
  if (PNRs1.pnrs.empty() || PNRs2.pnrs.empty())
  {
    errors.emplace_back("MSG.INCOMPATIBLE_SEGMENTS");
    return result;
  }

  const auto& segs1=PNRs1.getFirstPNRInfo().segs;
  const auto& segs2=PNRs2.getFirstPNRInfo().segs;

  auto s1=segs1.begin();
  auto s2=segs2.begin();
  for(; s1!=segs1.end() && s2!=segs2.end(); ++s1, ++s2, result++)
  {
    auto iRbds=flightRbdMap.find(s1->second.point_dep);
    if (iRbds==flightRbdMap.end())
      iRbds=flightRbdMap.emplace(s1->second.point_dep, PNRs1.getFlightInfo(s1->second.point_dep).oper).first;
    if (iRbds==flightRbdMap.end())
      throw EXCEPTIONS::Exception("TMultiPNRs::numberOfCompatibleSegments: iRbds==flightRbdMap.end()");

    if (!TPNRSegInfo::isJointCheckInPossible(s1->second, s2->second, iRbds->second, errors)) break;
  }

  return result;
}

void TMultiPNRsList::checkSegmentCompatibility()
{
  if (primary.empty())
    throw EXCEPTIONS::Exception("TMultiPNRsList::checkSegmentCompatibility: primary.empty()!");

  TMultiPNRs& primaryPNRs=primary.front();
  if (!primaryPNRs.errors.empty()) return; //есть ошибка главной группы

  std::map<int/*point_id*/, TFlightRbd> flightRbdMap;
  std::list<AstraLocale::LexemaData> errors;
  int numOfSegsMin=ASTRA::NoExists;

  for(TMultiPNRs& secondaryPNRs : secondary)
  {
    if (!secondaryPNRs.errors.empty()) continue;
    int numOfSegs=TMultiPNRs::numberOfCompatibleSegments(primaryPNRs, secondaryPNRs, flightRbdMap, errors);
    if (numOfSegs==0)
    {
      //вообще даже первый сегмент не подходит
      secondaryPNRs.errors.insert(secondaryPNRs.errors.end(), errors.begin(), errors.end());
      continue;
    };
    if (numOfSegsMin==ASTRA::NoExists || numOfSegsMin>numOfSegs) numOfSegsMin=numOfSegs;
  }

  if (numOfSegsMin==ASTRA::NoExists) return; //нет обрезания сегментов

  primaryPNRs.truncateSegments(numOfSegsMin);
  for(TMultiPNRs& secondaryPNRs : secondary)
  {
    if (!secondaryPNRs.errors.empty()) continue;
    secondaryPNRs.truncateSegments(numOfSegsMin);
  }
}

void findPNRs(const TPNRFilter &filter, TPNRs &PNRs, bool ignore_reg_no)
{
  if (filter.flt_no==NoExists) return;
  if (filter.scd_out_local_ranges.empty() &&
      filter.scd_out_utc_ranges.empty()) return;

  if (filter.surname.empty())
    throw EXCEPTIONS::Exception("findPNRs: filter.surname not defined");

  TReqInfo *reqInfo = TReqInfo::Instance();

  TQuery PointsQry(&OraSession);
  TQuery PaxQry(&OraSession);

    ostringstream sql;
    sql.str("");
    sql << "SELECT " << TAdvTripInfo::selectedFields("points")
        << "FROM points "
           "WHERE ";
    if (!filter.airlines.empty())
      sql << "      points.airline IN " << GetSQLEnum(filter.airlines) << " AND ";
    sql << "      points.flt_no=:flt_no AND "
           "      ( :suffix IS NULL OR points.suffix=:suffix ) AND "
           "      ( :airp_dep IS NULL OR points.airp=:airp_dep ) AND "
           "      points.scd_out >= :first_date AND points.scd_out < :last_date AND "
           "      points.pr_del=0 AND points.pr_reg<>0";

    PointsQry.Clear();
    PointsQry.SQLText= sql.str().c_str();
    PointsQry.CreateVariable("flt_no", otInteger, filter.flt_no);
    PointsQry.CreateVariable("suffix", otString, filter.suffix);
    PointsQry.CreateVariable("airp_dep", otString, filter.airp_dep);
    PointsQry.DeclareVariable("first_date", otDate);
    PointsQry.DeclareVariable("last_date", otDate);


    PaxQry.Clear();
    sql.str("");
    sql << "SELECT crs_pnr.pnr_id, "
           "       crs_pnr.airp_arv, "
           "       crs_pnr.class, "
           "       crs_pnr.subclass, "
           "       crs_pax.pax_id, "
           "       crs_pax.surname, "
           "       crs_pax.name "
           "FROM tlg_binding,crs_pnr,crs_pax "
           "WHERE tlg_binding.point_id_tlg=crs_pnr.point_id AND "
           "      crs_pax.pnr_id=crs_pnr.pnr_id AND "
           "      tlg_binding.point_id_spp=:point_id AND "
           "      crs_pnr.system='CRS' AND "
           "      (:airp_arv IS NULL OR crs_pnr.airp_arv=:airp_arv) AND "
        << filter.getSurnameSQLFilter("crs_pax.surname", PaxQry) << " AND "
        << "      crs_pax.pr_del=0 "
           "ORDER BY crs_pnr.pnr_id";

    PaxQry.SQLText= sql.str().c_str();
    PaxQry.DeclareVariable("point_id", otInteger);
    PaxQry.CreateVariable("airp_arv", otString, filter.airp_arv);

    map< TFlightInfo, map<int, string> > flights;
    for(int range_pass=0; range_pass<2; range_pass++)
    {
      const vector< pair<TDateTime, TDateTime> > &scd_out_ranges=range_pass==0?
            filter.scd_out_local_ranges:
            filter.scd_out_utc_ranges;

      //цикл по диапазонам дат
      for(vector< pair<TDateTime, TDateTime> >::const_iterator r=scd_out_ranges.begin();
          r!=scd_out_ranges.end(); ++r )
      {
        pair<TDateTime, TDateTime> date_range;
        //points.scd_out
        if (range_pass==0)
        {
          date_range.first=r->first-1.0;
          date_range.second=r->second+1.0;
        }
        else
        {
          date_range.first=r->first;
          date_range.second=r->second;
        };

        PointsQry.SetVariable("first_date", date_range.first);
        PointsQry.SetVariable("last_date", date_range.second);
        PointsQry.Execute();
        for(;!PointsQry.Eof;PointsQry.Next())
        {
          TFlightInfo flt;
          if (!flt.fromDB(PointsQry)) continue;
          if ( !reqInfo->user.access.airlines().permitted(flt.oper.airline) ||
               !reqInfo->user.access.airps().permitted(flt.oper.airp) ||
               (range_pass==0?flt.scd_out_local:flt.oper.scd_out)==NoExists ||
               (range_pass==0?flt.scd_out_local:flt.oper.scd_out)<r->first ||
               (range_pass==0?flt.scd_out_local:flt.oper.scd_out)>=r->second) continue;

          flights[flt].insert(make_pair(PointsQry.FieldAsInteger("point_id"),
                                        PointsQry.FieldAsString("airline")));
        };
      };
    };

    for(map< TFlightInfo, map<int, string> >::const_iterator iFlt=flights.begin(); iFlt!=flights.end(); ++iFlt)
    {
      TTripRoute route;
      route.GetRouteAfter( NoExists,
                           iFlt->first.oper.point_id,
                           iFlt->first.oper.point_num,
                           iFlt->first.oper.first_point,
                           iFlt->first.oper.pr_tranzit,
                           trtNotCurrent,
                           trtNotCancelled );

      if (filter.test_paxs.empty())
      {
        for(map<int, string>::const_iterator i=iFlt->second.begin(); i!=iFlt->second.end(); ++i)
        {
          PaxQry.SetVariable("point_id", i->first);
          PaxQry.Execute();
          if (!PaxQry.Eof)
          {
            int pnr_id=NoExists;
            bool pnr_filter=false;
            TPNRSegInfo seg;
            for(;!PaxQry.Eof;PaxQry.Next())
            {
              if (pnr_id==NoExists || pnr_id!=PaxQry.FieldAsInteger("pnr_id"))
              {
                pnr_id=PaxQry.FieldAsInteger("pnr_id");
                pnr_filter=seg.fromDB(iFlt->first.oper.point_id, route, PaxQry);
                pnr_filter=pnr_filter && seg.filterFromDB(filter);
              };
              if (!pnr_filter) continue;
              TPaxInfo pax;
              if (!pax.filterFromDB(filter, PaxQry, ignore_reg_no)) continue;
              PNRs.add(iFlt->first, seg, pax, false);
            };
          };
        };
      }
      else
      {
        for(vector<TTestPaxInfo>::const_iterator p=filter.test_paxs.begin();
            p!=filter.test_paxs.end(); ++p)
        {
          if (!p->airline.empty())
          {
            map<int, string>::const_iterator i=iFlt->second.begin();
            for(; i!=iFlt->second.end(); ++i)
              if (i->second==p->airline) break;
            if (i==iFlt->second.end()) continue;
          };

          TPNRSegInfo seg;
          if (!seg.fromTestPax(iFlt->first.oper.point_id, route, *p)) continue;
          TPaxInfo pax;
          if (!pax.fromTestPax(*p)) continue;
          PNRs.add(iFlt->first, seg, pax, true);
        };
      };
    };
};

bool TPNRFilter::userAccessIsAllowed(const TAdvTripInfo &oper, const boost::optional<TSimpleMktFlight>& mark)
{
  const TAccess& access=TReqInfo::Instance()->user.access;

  return (access.airlines().permitted(oper.airline) ||
          (mark && access.airlines().permitted(mark.get().airline))) &&
         access.airps().permitted(oper.airp);
}

bool TPNRFilter::isEqualFlight(const TAdvTripInfo& oper, const TSimpleMktFlight& mark) const
{
  if (!userAccessIsAllowed(oper, mark)) return false;

  if ((!airlines.empty() && airlines.find(oper.airline)==airlines.end()) ||
      (flt_no!=NoExists && oper.flt_no!=flt_no) ||
      (!suffix.empty() && oper.suffix!=suffix))
  {
    //фактический не подходит, проверим коммерческий
    if ((!airlines.empty() && airlines.find(mark.airline)==airlines.end()) ||
        (flt_no!=NoExists && mark.flt_no!=flt_no) ||
        (!suffix.empty() && mark.suffix!=suffix)) return false;
  }

  if (!airp_dep.empty() && oper.airp!=airp_dep) return false;

  if (!scd_out_local_ranges.empty())
  {
    if (oper.scd_out==NoExists) return false;

    TDateTime scd_out_local=UTCToLocal(oper.scd_out, AirpTZRegion(oper.airp));
    for(const auto& range : scd_out_local_ranges)
      if (scd_out_local < range.first ||
          scd_out_local >= range.second) return false;
  }

  if (!scd_out_utc_ranges.empty())
  {
    if (oper.scd_out==NoExists) return false;

    for(const auto& range : scd_out_utc_ranges)
      if (oper.scd_out < range.first ||
          oper.scd_out >= range.second) return false;
  }

  return true;
}

void getTCkinData( const TFlightInfo& first_flt,
                   const TDestInfo& first_dest,
                   const TPNRSegInfo& first_seg,
                   bool is_test,
                   vector<TPnrData> &other)
{
  other.clear();
  checkPnrData(first_flt, first_dest, first_seg, "getTCkinData");

  if (is_test) return;

  TReqInfo *reqInfo = TReqInfo::Instance();

  //поиск стыковочных сегментов (возвращаем вектор point_id)

  TQuery Qry(&OraSession);
  map<int, CheckIn::TTransferItem> crs_trfer;
  CheckInInterface::GetOnwardCrsTransfer(first_seg.pnr_id, Qry, first_flt.oper, first_dest.airp_arv, crs_trfer);
  if (!crs_trfer.empty())
  {
    //проверяем разрешение сквозной регистрации для данного типа клиента
    Qry.Clear();
    if (reqInfo->client_type==ctKiosk)
    {
      Qry.SQLText=
        "SELECT pr_tckin FROM trip_ckin_client "
        "WHERE point_id=:point_id AND client_type=:client_type AND desk_grp_id=:desk_grp_id";
      Qry.CreateVariable("desk_grp_id", otInteger, reqInfo->desk.grp_id);
      Qry.CreateVariable("client_type", otString, EncodeClientType(reqInfo->client_type));
    }
    else
    {
      Qry.SQLText=
        "SELECT pr_tckin FROM trip_ckin_client "
        "WHERE point_id=:point_id AND client_type=:client_type AND desk_grp_id IS NULL";
      Qry.CreateVariable("client_type", otString, EncodeClientType(ctWeb)); //!!!ctMobile
    };
    Qry.CreateVariable("point_id", otInteger, first_flt.oper.point_id);
    Qry.Execute();
    if (Qry.Eof || Qry.FieldAsInteger("pr_tckin")==0)
    {
      //сквозная регистрация запрещена
      ProgTrace(TRACE5, ">>>> Through check-in not permitted (point_id=%d, client_type=%s, desk_grp_id=%d)",
                        first_flt.oper.point_id, EncodeClientType(reqInfo->client_type), reqInfo->desk.grp_id);
      return;
    };

    map<int, pair<TCkinSegFlts, TTrferSetsInfo> > trfer_segs;
    traceTrfer(TRACE5, "getTCkinData: crs_trfer", crs_trfer);
    CheckInInterface::GetTrferSets(first_flt.oper,
                                   first_dest.airp_arv,
                                   "",
                                   crs_trfer,
                                   false,
                                   trfer_segs);
    traceTrfer(TRACE5, "getTCkinData: trfer_segs", trfer_segs);
    if (crs_trfer.size()!=trfer_segs.size())
      throw EXCEPTIONS::Exception("getTCkinData: different array sizes "
                                  "(crs_trfer.size()=%zu, trfer_segs.size()=%zu)",
                                  crs_trfer.size(),trfer_segs.size());

    int seg_no=1;
    try
    {
      //цикл по стыковочным сегментам и по трансферным рейсам
      map<int, pair<TCkinSegFlts, TTrferSetsInfo> >::const_iterator s=trfer_segs.begin();
      map<int, CheckIn::TTransferItem>::const_iterator f=crs_trfer.begin();
      for(;s!=trfer_segs.end() && f!=crs_trfer.end();++s,++f)
      {
        seg_no++;
        if (s->second.first.is_edi)
          throw "Flight from the other DCS";

        if (s->second.first.flts.empty())
          throw "Flight not found";

        if (s->second.first.flts.size()>1)
          throw "More than one flight found";

        if (!s->second.second.tckin_permit)
          throw "Check-in not permitted";

        const TSegInfo &currSeg=*(s->second.first.flts.begin());

        if (currSeg.fltInfo.pr_del!=0)
          throw "Flight canceled";

        if (currSeg.point_arv==ASTRA::NoExists)
          throw "Destination not found";

        if (TCFG(currSeg.point_dep).empty())
          throw "Configuration of the flight not assigned";

        TPnrData pnrData;
        if (!pnrData.flt.fromDB(currSeg.point_dep, false))
          throw "Error in TFlightInfo::fromDB";
        if (!pnrData.flt.fromDBadditional(false, false))
          throw "Error in TFlightInfo::fromDBadditional";

        if (reqInfo->client_type==ctKiosk)
        {
          if ( pnrData.flt.stage_times.find( sOpenKIOSKCheckIn ) == pnrData.flt.stage_times.end() ||
               pnrData.flt.stage_times.find( sCloseKIOSKCheckIn ) == pnrData.flt.stage_times.end() )
            throw "Stage of kiosk check-in not found";
        }
        else
        {
          if ( pnrData.flt.stage_times.find( sOpenWEBCheckIn ) == pnrData.flt.stage_times.end() ||
               pnrData.flt.stage_times.find( sCloseWEBCheckIn ) == pnrData.flt.stage_times.end() ||
               pnrData.flt.stage_times.find( sCloseWEBCancel ) == pnrData.flt.stage_times.end())
            throw "Stage of web check-in not found";
        };

        if (first_seg.pnr_addrs.empty())
          throw "PNR not defined";

        //ищем PNR по номеру
        Qry.Clear();
        Qry.SQLText=
          "SELECT DISTINCT crs_pnr.pnr_id, crs_pnr.airp_arv, "
          "                crs_pnr.class, crs_pnr.subclass "
          "FROM tlg_binding, crs_pnr, crs_pax "
          "WHERE crs_pnr.point_id=tlg_binding.point_id_tlg AND "
          "      crs_pax.pnr_id=crs_pnr.pnr_id AND "
          "      tlg_binding.point_id_spp=:point_id AND "
          "      crs_pnr.system='CRS' AND "
          "      crs_pnr.airp_arv=:airp_arv AND "
          "      crs_pnr.subclass=:subclass AND "
          "      crs_pax.pr_del=0";
        Qry.CreateVariable("point_id", otInteger, currSeg.point_dep);
        Qry.CreateVariable("airp_arv", otString, currSeg.airp_arv);  //идет проверка совпадения а/п назначения из трансферного маршрута
        Qry.CreateVariable("subclass", otString, f->second.subclass);//идет проверка совпадения подкласса из трансферного маршрута
        Qry.Execute();
        if (!Qry.Eof)
        {
          TTripRoute route;
          route.GetRouteAfter( NoExists,
                               pnrData.flt.oper.point_id,
                               pnrData.flt.oper.point_num,
                               pnrData.flt.oper.first_point,
                               pnrData.flt.oper.pr_tranzit,
                               trtNotCurrent,
                               trtNotCancelled );

          for(;!Qry.Eof;Qry.Next())
          {
            TPNRSegInfo seg;
            if (!seg.fromDB(pnrData.flt.oper.point_id, route, Qry)) continue;
            if (!seg.pnr_addrs.equalPnrExists(first_seg.pnr_addrs)) continue;
            if (pnrData.seg.pnr_id!=NoExists) //дубль PNR
              throw "More than one PNR found";
            pnrData.seg=seg;
          };
        };

        if (pnrData.seg.pnr_id==NoExists)
          throw "PNR not found";

        if (!pnrData.dest.fromDB(pnrData.seg.point_arv, false))
          throw "Error in TDestInfo::fromDB";

        other.push_back(pnrData);
      };
    }
    catch(const char* error)
    {
      ProgTrace(TRACE5, ">>>> seg_no=%d: %s ", seg_no, error);
      traceTrfer(TRACE5, "crs_trfer", crs_trfer);
      traceTrfer(TRACE5, "trfer_segs", trfer_segs);
    };
  };
}

void getTCkinData( const TPnrData &first,
                   bool is_test,
                   vector<TPnrData> &other)
{
  getTCkinData( first.flt, first.dest, first.seg, is_test, other );
}

} // namespace WebSearch

