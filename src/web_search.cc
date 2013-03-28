#include "web_search.h"
#include "basic.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "checkin.h"
#include "passenger.h"
#include "baggage.h"
#include "xml_unit.h"
#include "misc.h"
#include "dev_utils.h"

#define NICKNAME "VLAD"
#include "serverlib/test.h"

using namespace std;
using namespace ASTRA;
using namespace BASIC;
using namespace AstraLocale;

namespace WebSearch
{

TPNRFilter& TPNRFilter::fromBCBP_M(const std::string bcbp)
{
  clear();
  if (bcbp.empty())
  {
    ProgError(STDLOG, "TPNRFilter::fromBCBP_M: empty bcbp");
    throw UserException("MSG.SCAN_CODE.NOT_SET");
  };

  try
  {
    string::size_type airline_use_begin_idx, airline_use_end_idx;
    checkBCBP_M(bcbp, 0, airline_use_begin_idx, airline_use_end_idx);
  }
  catch(EXCEPTIONS::EConvertError &e)
  {
    ProgError(STDLOG, "TPNRFilter::fromBCBP_M: %s", e.what());
    throw UserException("MSG.SCAN_CODE.UNKNOWN_FORMAT");
  };
    
  try
  {
    string str;
    TElemFmt fmt;

    //фамилия/имя пассажира
    surname=upperc(bcbp.substr(2,20));
    TrimString(surname);
    if (surname.empty())
    {
      throw EXCEPTIONS::EConvertError("empty <Passenger Name>");
    };
    string::size_type pos=surname.find('/');
    if (pos != string::npos)
    {
      name=surname.substr(pos+1);
      surname=surname.substr(0,pos);
    };
    TrimString(surname);
    if (surname.empty())
    {
      throw EXCEPTIONS::EConvertError("invalid <Passenger Name>");
    };

    TrimString(name);
    
    if (surname.size()+name.size()+1 >= 20)
    {
      surname_equal_len=surname.size();
      if (!name.empty())
        name_equal_len=name.size();
    };
    
    name.erase(find(name.begin(), name.end(), ' '), name.end()); //оставляем часть до пробела

    str=bcbp.substr(23,7);
    TrimString(str);
    pnr_addr_normal=convert_pnr_addr(upperc(str) , true);

    str=bcbp.substr(30,3);
    TrimString(str);
    if (!str.empty())
    {
      airp_dep = ElemToElemId( etAirp, upperc(str), fmt );
      if (fmt==efmtUnknown)
      {
        throw EXCEPTIONS::EConvertError("unknown <From City Airport Code> %s", str.c_str());
    	};
    }
    else
    {
      throw EXCEPTIONS::EConvertError("empty <From City Airport Code>");
    };

    str=bcbp.substr(33,3);
    TrimString(str);
    if (!str.empty())
    {
      airp_arv = ElemToElemId( etAirp, upperc(str), fmt );
      if (fmt==efmtUnknown)
      {
        throw EXCEPTIONS::EConvertError("unknown <To City Airport Code> %s", str.c_str());
    	};
    }
    else
    {
      throw EXCEPTIONS::EConvertError("empty <To City Airport Code>");
    };

    str=bcbp.substr(36,3);
    TrimString(str);
    if (!str.empty())
    {
      string airline = ElemToElemId( etAirline, upperc(str), fmt );
      if (fmt==efmtUnknown)
      {
        throw EXCEPTIONS::EConvertError("unknown <Operating carrier Designator> %s", str.c_str());
      };
      airlines.insert(airline);
    }
    else
    {
      throw EXCEPTIONS::EConvertError("empty <Operating carrier Designator>");
    };

    str=bcbp.substr(39,5);
    TrimString(str);
    if (!str.empty())
    {
      char last_char=str[str.size()-1];
      if (IsLetter(last_char))
      {
        //проверяем суффикс
        suffix = ElemToElemId( etSuffix, upperc(string(1,last_char)), fmt );
        if (fmt==efmtUnknown)
        {
          throw EXCEPTIONS::EConvertError("unknown <Flight Number> suffix %c", last_char);
      	};
      	str.erase(str.size()-1);
      };
    };

  	if (!str.empty())
    {
      if ( StrToInt( str.c_str(), flt_no ) == EOF ||
  		     flt_no > 99999 || flt_no <= 0 )
      {
        throw EXCEPTIONS::EConvertError("invalid <Flight Number> %s", str.c_str());
      };
  	}
    else
    {
      throw EXCEPTIONS::EConvertError("empty <Flight Number>");
    };

    str=bcbp.substr(44,3);
    TrimString(str);
    if (!str.empty())
    {
      int julian_date;
      if ( StrToInt( str.c_str(), julian_date ) == EOF ||
  		     julian_date > 366 || julian_date <= 0 )
      {
        throw EXCEPTIONS::EConvertError("invalid <Date of Flight> %s", str.c_str());
      };

      int Year, Month, Day;
      TDateTime utc_date=NowUTC(), scd_out_local=NoExists;
      DecodeDate(utc_date, Year, Month, Day);
      for(int y=Year-1; y<=Year+1; y++)
      {
        try
        {
          TDateTime d=JulianDateToDateTime(julian_date, y);
          if (scd_out_local==NoExists ||
              fabs(scd_out_local-utc_date)>fabs(d-utc_date))
            scd_out_local=d;
        }
        catch(EXCEPTIONS::EConvertError) {};
      };
      if (scd_out_local==NoExists)
      {
        throw EXCEPTIONS::EConvertError("invalid <Date of Flight> %s", str.c_str());
      };

      MergeSortedRanges(scd_out_local_ranges, make_pair(scd_out_local, scd_out_local+1.0));
    }
    else
    {
      throw EXCEPTIONS::EConvertError("empty <Date of Flight>");
    };

    str=bcbp.substr(52,5);
    TrimString(str);
    if (!str.empty())
    {
      if ( StrToInt( str.c_str(), reg_no ) == EOF ||
  		     reg_no > 999 || reg_no <= 0 )
  		{
    	  throw EXCEPTIONS::EConvertError("invalid <Check-In Sequence Number> %s", str.c_str());
    	};
  	};
  }
  catch(EXCEPTIONS::EConvertError &e)
  {
    ProgError(STDLOG, "TPNRFilter::fromBCBP_M: %s", e.what());
    throw UserException("MSG.SCAN_CODE.UNKNOWN_DATA");
  };

  return *this;
};

TPNRFilter& TPNRFilter::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  string str;
  TElemFmt fmt;

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
      str=NodeAsString(alNode);
      TrimString(str);
      if (!str.empty())
      {
        airline = ElemToElemId( etAirline, upperc(str), fmt );
        if (fmt==efmtUnknown)
        {
          ProgError(STDLOG, "TPNRFilter::fromXML: unknown <airline> %s", str.c_str());
        	throw UserException( "MSG.AIRLINE.INVALID",
        		                   LParams()<<LParam("airline", str ) );
        };
        airlines.insert(airline);
      }
      else
      {
        if (pass==0)
          ProgError(STDLOG, "TPNRFilter::fromXML: empty <airline>");
      };
      if (pass==1) break;
    };
  };
  
  str=NodeAsStringFast("flt_no", node2, "");
  TrimString(str);
	if (!str.empty())
  {
    if ( StrToInt( str.c_str(), flt_no ) == EOF ||
		     flt_no > 99999 || flt_no <= 0 )
    {
      ProgError(STDLOG, "TPNRFilter::fromXML: invalid <flt_no> %s", str.c_str());
      throw UserException( "MSG.FLT_NO.INVALID",
		                       LParams()<<LParam("flt_no", str) );
    };
	}
  else
  {
    ProgError(STDLOG, "TPNRFilter::fromXML: <flt_no> not defined");
    throw UserException( "MSG.CHECK_FLIGHT.NOT_SET_FLT_NO" );
  };
  
  str=NodeAsStringFast("suffix", node2, "");
  TrimString(str);
  if (!str.empty())
  {
    suffix = ElemToElemId( etSuffix, upperc(str), fmt );
    if (fmt==efmtUnknown)
    {
      ProgError(STDLOG, "TPNRFilter::fromXML: unknown <suffix> %s", str.c_str());
  		throw UserException( "MSG.SUFFIX.INVALID",
  			                   LParams()<<LParam("suffix", str) );
  	};
  };
  
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
          ProgError(STDLOG, "TPNRFilter::fromXML: invalid <scd_out_min> %s", str.c_str());
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
          ProgError(STDLOG, "TPNRFilter::fromXML: invalid <scd_out_max> %s", str.c_str());
  			  throw UserException( "MSG.FLIGHT_DATE.INVALID",
  				                     LParams()<<LParam("scd_out", str) );
  			};
  	  };
      if (scd_out_range.first==NoExists && scd_out_range.second!=NoExists)
      {
        ProgError(STDLOG, "TPNRFilter::fromXML: <scd_out_min> not defined");
        throw UserException("MSG.INVALID_RANGE");
      };
      if (scd_out_range.first!=NoExists && scd_out_range.second==NoExists)
      {
        ProgError(STDLOG, "TPNRFilter::fromXML: <scd_out_max> not defined");
        throw UserException("MSG.INVALID_RANGE");
      };
    };
    if (pass==1)
    {
      str=NodeAsStringFast("scd_out", node2, "");
      TrimString(str);
      if (!str.empty())
      {
        if ( StrToDateTime( str.c_str(), "dd.mm.yyyy hh:nn:ss", scd_out_range.first ) == EOF )
        {
          ProgError(STDLOG, "TPNRFilter::fromXML: invalid <scd_out> %s", str.c_str());
    		  throw UserException( "MSG.FLIGHT_DATE.INVALID",
    			                     LParams()<<LParam("scd_out", str) );
    		};
    		scd_out_range.second=scd_out_range.first+1.0; //1 сутки
    	};
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
          ProgError(STDLOG, "TPNRFilter::fromXML: invalid <scd_out_shift> %s", str.c_str());
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
        ProgError(STDLOG, "TPNRFilter::fromXML: invalid search period");
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
    ProgError(STDLOG, "TPNRFilter::fromXML: search period too large");
    throw UserException("MSG.SEARCH_PERIOD_MAX_N_DAYS", LParams()<<LParam("days", 5));
  };
  double utc_days=0;
  for(vector< pair<TDateTime, TDateTime> >::const_iterator r=scd_out_utc_ranges.begin();
                                                           r!=scd_out_utc_ranges.end(); ++r )
    utc_days+=(r->second-r->first);
  if (utc_days>5.0)
  {
    ProgError(STDLOG, "TPNRFilter::fromXML: search period too large");
    throw UserException("MSG.SEARCH_PERIOD_MAX_N_DAYS", LParams()<<LParam("days", 5));
  };
  
  if (local_days<=0 && utc_days<=0)
  {
    ProgError(STDLOG, "TPNRFilter::fromXML: empty search period");
    throw UserException("MSG.NOT_SET_RANGE");
  };

  surname=NodeAsStringFast("surname", node2, "");
  TrimString(surname);
  if (surname.empty())
  {
    ProgError(STDLOG, "TPNRFilter::fromXML: <surname> not defined");
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
  	  ProgError(STDLOG, "TPNRFilter::fromXML: invalid <reg_no> %s", str.c_str());
      throw UserException("MSG.INVALID_REG_NO");
  	};
  };

  return *this;
};

TPNRFilter& TPNRFilter::testPaxFromDB()
{
  if (surname.empty())
    throw EXCEPTIONS::Exception("TPNRFilter::testPaxFromDB: surname not defined");

  test_paxs.clear();

  TQuery Qry(&OraSession);
  Qry.Clear();
  ostringstream sql;
  sql <<
    "SELECT id, airline, surname, name, subclass, doc_no, tkn_no, "
    "       pnr_airline, pnr_addr, reg_no FROM test_pax ";
  if (surname_equal_len!=NoExists)
  {
    sql << "WHERE system.transliter_equal(SUBSTR(surname,1,:surname_equal_len),:surname)<>0 ";
    Qry.CreateVariable("surname", otString, surname.substr(0, surname_equal_len));
    Qry.CreateVariable("surname_equal_len", otInteger, surname_equal_len);
  }
  else
  {
    sql << "WHERE system.transliter_equal(surname,:surname)<>0 ";
    Qry.CreateVariable("surname", otString, surname);
  };
  if (!airlines.empty())
    sql << "AND (airline IN " << GetSQLEnum(airlines) << " OR airline IS NULL) ";

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

    if (!name.empty())
    {
      string pax_name_normal=pax.name;
      //оставляем часть до пробела
      pax_name_normal.erase(find(pax_name_normal.begin(), pax_name_normal.end(), ' '), pax_name_normal.end());
      //проверим совпадение имени
      if (name_equal_len!=NoExists)
      {
        if (!transliter_equal(pax_name_normal.substr(0, name_equal_len),
                                         name.substr(0, name_equal_len))) continue;
      }
      else
      {
        if (!transliter_equal(pax_name_normal, name)) continue;
      };
    };

    if (!document.empty() &&
        document!=pax.document) continue;
    if (!ticket_no.empty() &&
        ticket_no!=pax.ticket_no) continue;
    if (!pnr_addr_normal.empty() &&
        pnr_addr_normal!=convert_pnr_addr(pax.pnr_addr.addr, true)) continue;
    if (reg_no!=NoExists &&
        reg_no!=pax.reg_no) continue;
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
  ProgTrace(TRACE_PARAMS, "surname_equal_len: %s", surname_equal_len==NoExists?"":IntToString(surname_equal_len).c_str());
  ProgTrace(TRACE_PARAMS, "name_equal_len: %s", name_equal_len==NoExists?"":IntToString(name_equal_len).c_str());
  ProgTrace(TRACE_PARAMS, "^^^^^^^^^^^^ TPNRFilter ^^^^^^^^^^^^");
};

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

    city_arv = ((TAirpsRow&)base_tables.get( "airps" ).get_row( "code", airp_arv, true )).city;
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

void TDestInfo::toXML(xmlNodePtr node, bool old_style) const
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
  if (!old_style)
  {
    point_arv==NoExists?NewTextChild(node, "point_arv"):
                        NewTextChild(node, "point_arv", point_arv);
  };
  NewTextChild(node, "scd_in", scd_in_local==NoExists?"":DateTimeToStr(scd_in_local, ServerFormatDateTimeAsString));
  NewTextChild(node, "est_in", est_in_local==NoExists?"":DateTimeToStr(est_in_local, ServerFormatDateTimeAsString));
  NewTextChild(node, "act_in", act_in_local==NoExists?"":DateTimeToStr(act_in_local, ServerFormatDateTimeAsString));
  NewTextChild(node, "airp_arv", airp_arv);
  NewTextChild(node, "city_arv", city_arv);
  arv_utc_offset==NoExists?NewTextChild(node, "arr_utc_offset"):
                           NewTextChild(node, "arr_utc_offset", arv_utc_offset);
};

bool TFlightInfo::fromDB(TQuery &Qry)
{
  clear();
  if (Qry.Eof) return false;
  point_dep = Qry.FieldAsInteger("point_id");
  point_num = Qry.FieldAsInteger("point_num");
  first_point = Qry.FieldIsNULL("first_point")?NoExists:Qry.FieldAsInteger("first_point");
  pr_tranzit = Qry.FieldAsInteger("pr_tranzit")!=0;
  oper.Init(Qry);
  craft=Qry.FieldAsString("craft");
  craft_fmt=(TElemFmt)Qry.FieldAsInteger("craft_fmt");
  string region=AirpTZRegion(oper.airp);
  scd_out_local=oper.scd_out==NoExists?NoExists:UTCToLocal(oper.scd_out, region);
  est_out_local=Qry.FieldIsNULL("est_out")?NoExists:UTCToLocal(Qry.FieldAsDateTime("est_out"), region);
  act_out_local=Qry.FieldIsNULL("act_out")?NoExists:UTCToLocal(Qry.FieldAsDateTime("act_out"), region);

  city_dep = ((TAirpsRow&)base_tables.get( "airps" ).get_row( "code", oper.airp, true )).city;
  if (oper.scd_out!=NoExists && scd_out_local!=NoExists)
    dep_utc_offset = (int)round((scd_out_local - oper.scd_out) * 1440);
  else
    dep_utc_offset = NoExists;
  return true;
};

bool TFlightInfo::fromDB(int point_id, bool first_segment, bool pr_throw)
{
  if (point_id==NoExists)
    throw EXCEPTIONS::Exception("TFlightInfo::fromDB: point_id not defined");
  clear();
  try
  {
    TReqInfo *reqInfo = TReqInfo::Instance();
  
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText=
      "SELECT points.point_id, points.point_num, points.first_point, points.pr_tranzit, "
      "       points.pr_del, points.pr_reg, "
      "       points.airline, points.flt_no, points.suffix, points.airp, "
      "       points.scd_out, points.est_out, points.act_out, points.craft, "
      "       points.airline_fmt, points.suffix_fmt, points.airp_fmt, points.craft_fmt "
      "FROM points "
      "WHERE points.point_id=:point_id";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.Execute();
    if (Qry.Eof) throw UserException( "MSG.FLIGHT.NOT_FOUND" );
    
  	if (!fromDB(Qry)) throw UserException( "MSG.FLIGHT.NOT_FOUND" );
  	
  	if ( Qry.FieldAsInteger( "pr_del" ) < 0 )
  		throw UserException( "MSG.FLIGHT.DELETED" );
  	  
  	if (first_segment)
    {
      //необходимо соблюдать доступ только к первому сегменту
      if ( !reqInfo->CheckAirline(oper.airline) ||
           !reqInfo->CheckAirp(oper.airp) )
        throw UserException( "MSG.FLIGHT.ACCESS_DENIED" );
    };
    
    if ( Qry.FieldAsInteger( "pr_del" ) > 0 )
  		throw UserException( "MSG.FLIGHT.CANCELED" );
    
    if ( Qry.FieldAsInteger( "pr_reg" ) == 0 )
  		throw UserException( "MSG.FLIGHT.CHECKIN_CANCELED" );
  		
  	if (oper.scd_out==NoExists) throw UserException( "MSG.FLIGHT_DATE.NOT_SET" );
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

bool TFlightInfo::fromDBadditional(bool first_segment, bool pr_throw)
{
  if (point_dep==NoExists)
    throw EXCEPTIONS::Exception("TFlightInfo::fromDBadditional: point_dep not defined");
  try
  {
    GetMktFlights(oper, mark);
    //std::map<TStage_Type, TStage> stage_statuses
    stage_statuses.clear();
    TTripStages tripStages( point_dep );
    stage_statuses[stWEBCheckIn]=tripStages.getStage( stWEBCheckIn );
    stage_statuses[stWEBCancel]=tripStages.getStage( stWEBCancel );
    stage_statuses[stKIOSKCheckIn]=tripStages.getStage( stKIOSKCheckIn );
    stage_statuses[stCheckIn]=tripStages.getStage( stCheckIn );
    stage_statuses[stBoarding]=tripStages.getStage( stBoarding );

    TQuery Qry(&OraSession);
    TReqInfo *reqInfo = TReqInfo::Instance();
    //std::map<TStage, BASIC::TDateTime> stages
    stages.clear();
    TStagesRules *sr = TStagesRules::Instance();
  	TCkinClients ckin_clients;
  	TTripStages::ReadCkinClients( point_dep, ckin_clients );
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
          Qry.CreateVariable("point_id", otInteger, point_dep);
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

      if ( sr->isClientStage( (int)stage ) && !sr->canClientStage( ckin_clients, (int)stage ) )
      	stages.insert( make_pair(stage, NoExists) );
      else
      	stages.insert( make_pair(stage, UTCToLocal( tripStages.time(stage), AirpTZRegion(oper.airp) ) ) );
  	};

    Qry.Clear();
    Qry.SQLText =
      "SELECT trip_paid_ckin.pr_permit AS pr_paid_ckin "
      "FROM trip_paid_ckin "
      "WHERE trip_paid_ckin.point_id=:point_id";
    Qry.CreateVariable( "point_id", otInteger, point_dep );
    Qry.Execute();
    pr_paid_ckin = false;
    if (!Qry.Eof)
      pr_paid_ckin = Qry.FieldAsInteger("pr_paid_ckin")!=0;
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
    cls=((TSubclsRow&)base_tables.get( "subcls" ).get_row( "code", pax.subcls, true )).cl;
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

void TFlightInfo::toXML(xmlNodePtr node, bool old_style) const
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
  point_dep==NoExists?NewTextChild(node, old_style?"point_id":"point_dep"):
                      NewTextChild(node, old_style?"point_id":"point_dep", point_dep);
  NewTextChild(node, "airline", oper.airline);
  oper.flt_no==NoExists?NewTextChild(node, "flt_no"):
                        NewTextChild(node, "flt_no", oper.flt_no);
  NewTextChild(node, "suffix", oper.suffix);
  NewTextChild(node, "craft", craft);
  NewTextChild(node, "scd_out", scd_out_local==NoExists?"":DateTimeToStr(scd_out_local, ServerFormatDateTimeAsString));
  NewTextChild(node, "est_out", est_out_local==NoExists?"":DateTimeToStr(est_out_local, ServerFormatDateTimeAsString));
  NewTextChild(node, "act_out", act_out_local==NoExists?"":DateTimeToStr(act_out_local, ServerFormatDateTimeAsString));
  NewTextChild(node, "airp_dep", oper.airp);
  NewTextChild(node, "city_dep", city_dep);
  dep_utc_offset==NoExists?NewTextChild(node, "dep_utc_offset"):
                           NewTextChild(node, "dep_utc_offset", dep_utc_offset);
  if (!old_style)
  {
    xmlNodePtr destsNode=NewTextChild(node, "dests");
    for(set<TDestInfo>::const_iterator i=dests.begin();i!=dests.end();++i)
      i->toXML(NewTextChild(destsNode, "dest"));
  };

  TReqInfo *reqInfo = TReqInfo::Instance();
  map<TStage_Type, TStage>::const_iterator iStatus;
  if ( act_out_local != NoExists )
  	NewTextChild( node, "status", "sTakeoff" );
  else
  {
    if ( reqInfo->client_type == ctKiosk )
      iStatus=stage_statuses.find(stKIOSKCheckIn);
    else
      iStatus=stage_statuses.find(stWEBCheckIn);
    if (iStatus!=stage_statuses.end())
    {
      switch ( iStatus->second ) {
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
    else
      NewTextChild( node, "status" );
  };

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

    map<TStage, TDateTime>::const_iterator iStage=stages.find(stage);
    if (iStage!=stages.end() && iStage->second!=NoExists)
      stageNode = NewTextChild( stagesNode, "stage", DateTimeToStr( iStage->second, ServerFormatDateTimeAsString ) );
    else
    	stageNode = NewTextChild( stagesNode, "stage" );
    SetProp( stageNode, "type", stage_name );
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
    iStatus=stage_statuses.find(stage_type);
    bool sem= act_out_local==NoExists &&
              iStatus!=stage_statuses.end() &&
              iStatus->second==stage;
    NewTextChild( semNode, sem_name.c_str(), (int)sem );
  };

  NewTextChild( node, "paid_checkin", (int)pr_paid_ckin );

  xmlNodePtr fltsNode = NewTextChild( node, "mark_flights" );
  for(vector<TTripInfo>::const_iterator m=mark.begin();
                                        m!=mark.end();++m)
  {
    xmlNodePtr fltNode=NewTextChild( fltsNode, "flight" );
    NewTextChild( fltNode, "airline", m->airline );
    NewTextChild( fltNode, "flt_no", m->flt_no );
    NewTextChild( fltNode, "suffix", m->suffix );
  };
};

bool TPNRSegInfo::fromDB(int point_id, const TTripRoute &route, TQuery &Qry)
{
  clear();
  if (Qry.Eof) return false;
  string airp_arv=Qry.FieldAsString("airp_arv");
  TTripRoute::const_iterator r=route.begin();
  for(; r!=route.end(); ++r)
    if (r->airp == airp_arv) break;
  if (r==route.end()) return false;
  
  point_dep=point_id;
  point_arv=r->point_id;
  pnr_id=Qry.FieldAsInteger("pnr_id");
  cls=Qry.FieldAsString("class");
  subcls=Qry.FieldAsString("subclass");
  
  TQuery AddrQry(&OraSession);
  AddrQry.Clear();
  AddrQry.SQLText="SELECT airline, addr FROM pnr_addrs WHERE pnr_id=:pnr_id";
  AddrQry.CreateVariable( "pnr_id", otInteger, pnr_id );
  AddrQry.Execute();
  for(;!AddrQry.Eof;AddrQry.Next())
  {
    TPNRAddrInfo pnr_addr;
    pnr_addr.airline=AddrQry.FieldAsString("airline");
    pnr_addr.addr=AddrQry.FieldAsString("addr");
    pnr_addrs.push_back(pnr_addr);
  };
  
  return true;
};

bool TPNRSegInfo::filterFromDB(const TPNRFilter &filter)
{
  if (!filter.pnr_addr_normal.empty())
  {
    //найдем есть ли среди адресов искомый
    vector<TPNRAddrInfo>::const_iterator a=pnr_addrs.begin();
    for(; a!=pnr_addrs.end(); ++a)
      if (convert_pnr_addr(a->addr, true)==filter.pnr_addr_normal) break;
    if (a==pnr_addrs.end()) return false;
  };

  return true;
};

bool TPNRSegInfo::filterFromDB(const std::vector<TPNRAddrInfo> &filter)
{
  for(vector<TPNRAddrInfo>::const_iterator i=filter.begin(); i!=filter.end(); ++i)
    if (find(pnr_addrs.begin(), pnr_addrs.end(), *i)!=pnr_addrs.end()) return true;
  return false;
};

void TPNRSegInfo::toXML(xmlNodePtr node, bool old_style) const
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
  if (!old_style)
  {
    point_dep==NoExists?NewTextChild(node, "point_dep"):
                        NewTextChild(node, "point_dep", point_dep);
    point_arv==NoExists?NewTextChild(node, "point_arv"):
                        NewTextChild(node, "point_arv", point_arv);
  };
  pnr_id==NoExists?NewTextChild(node, "pnr_id"):
                   NewTextChild(node, "pnr_id", pnr_id);
  NewTextChild(node, "subclass", subcls);
  xmlNodePtr addrsNode=NewTextChild(node, "pnr_addrs");
  for(vector<TPNRAddrInfo>::const_iterator i=pnr_addrs.begin(); i!=pnr_addrs.end(); ++i)
  {
    xmlNodePtr addrNode = NewTextChild(addrsNode, "pnr_addr");
    NewTextChild(addrNode, "airline", i->airline);
    NewTextChild(addrNode, "addr", i->addr);
  };
};

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

bool TPaxInfo::filterFromDB(const TPNRFilter &filter, TQuery &Qry)
{
  clear();
  if (Qry.Eof) return false;
  pax_id=Qry.FieldAsInteger("pax_id");
  surname=Qry.FieldAsString("surname");
  name=Qry.FieldAsString("name");
  if (!filter.name.empty())
  {
    string pax_name_normal=name;
    //оставляем часть до пробела
    pax_name_normal.erase(find(pax_name_normal.begin(), pax_name_normal.end(), ' '), pax_name_normal.end());
    //проверим совпадение имени
    if (filter.name_equal_len!=NoExists)
    {
      if (!transliter_equal(pax_name_normal.substr(0, filter.name_equal_len),
                                filter.name.substr(0, filter.name_equal_len))) return false;
    }
    else
    {
      if (!transliter_equal(pax_name_normal, filter.name)) return false;
    };
    
  };
  
  TQuery Qry1(&OraSession);
  TQuery Qry2(&OraSession);
  CheckIn::TPaxTknItem tkn;
  LoadCrsPaxTkn(pax_id, tkn, Qry1, Qry2);
  ticket_no=tkn.no;
  if (!filter.ticket_no.empty())
  {
    //проверим совпадение билета
    if (ticket_no!=filter.ticket_no)
    {
       if (filter.ticket_no.size() != 14) return false;
       if (ticket_no!=filter.ticket_no.substr(0,13)) return false;
    };
  };
        	
  CheckIn::TPaxDocItem doc;
  LoadCrsPaxDoc(pax_id, doc, Qry1, Qry2);
  document=doc.no;
  if (!filter.document.empty())
  {
    //проверим совпадение документа
    if (document!=filter.document) return false;
  };
  
  Qry1.Clear();
  Qry1.SQLText="SELECT reg_no FROM pax WHERE pax_id=:pax_id";
  Qry1.CreateVariable("pax_id", otInteger, pax_id);
  Qry1.Execute();
  if (!Qry1.Eof) reg_no=Qry1.FieldAsInteger("reg_no");
  
  if (filter.reg_no!=NoExists)
  {
    //проверим совпадение рег. номера
    if (reg_no!=filter.reg_no) return false;
  };
  
  return true;
};

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
  if (flt.point_dep==NoExists || flt.point_dep!=seg.point_dep)
    throw EXCEPTIONS::Exception("%s: wrong flt.point_dep %d", where.c_str(), flt.point_dep);
  if (dest.point_arv==NoExists || dest.point_arv!=seg.point_arv)
    throw EXCEPTIONS::Exception("%s: wrong dest.point_arv %d", where.c_str(), dest.point_arv);
  if (seg.pnr_id==NoExists)
    throw EXCEPTIONS::Exception("%s: pnr_id not defined", where.c_str());
};

void TPNRSegInfo::getMarkFlt(const TFlightInfo &flt, bool is_test, TTripInfo &mark) const
{
  mark.Clear();
  if (!is_test)
  {
    //коммерческий рейс PNR
    TMktFlight mktFlt;
    mktFlt.getByPnrId(pnr_id);
    if (mktFlt.IsNULL())
      throw EXCEPTIONS::Exception("TPNRSegInfo::getMarkFlt: empty mktFlt (pnr_id=%d)",pnr_id);

    mark.airline=mktFlt.airline;
    mark.flt_no=mktFlt.flt_no;
    mark.suffix=mktFlt.suffix;
    mark.airp=mktFlt.airp_dep;
    mark.scd_out=mktFlt.scd_date_local;
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

bool TPNRInfo::fromDBadditional(const TFlightInfo &flt, const TDestInfo &dest, bool is_test)
{
  if (segs.empty())
    throw EXCEPTIONS::Exception("TPNRInfo::fromDBadditional: empty segs");
  checkPnrData(flt, dest, segs.begin()->second, "TPNRInfo::fromDBadditional");

  TTripInfo pnrMarkFlt;
  segs.begin()->second.getMarkFlt(flt, is_test, pnrMarkFlt);
  TCodeShareSets codeshareSets;
  codeshareSets.get(flt.oper,pnrMarkFlt);

  TQuery BagNormsQry(&OraSession);
  BagNormsQry.Clear();
  BagNormsQry.SQLText=
   "SELECT point_id, :use_mark_flt AS use_mark_flt, "
   "       :airline_mark AS airline_mark, :flt_no_mark AS flt_no_mark, "
   "       id,bag_norms.airline,pr_trfer,city_dep,city_arv,pax_cat, "
   "       subclass,class,bag_norms.flt_no,bag_norms.craft,bag_norms.trip_type, "
   "       first_date,last_date-1/86400 AS last_date,"
   "       bag_type,amount,weight,per_unit,norm_type,extra,bag_norms.tid "
   "FROM bag_norms, "
   "     (SELECT point_id,airps.city, "
   "             DECODE(:use_mark_flt,0,airline,:airline_mark) AS airline, "
   "             DECODE(:use_mark_flt,0,flt_no,:flt_no_mark) AS flt_no, "
   "             craft,NVL(est_out,scd_out) AS scd, "
   "             trip_type, point_num, DECODE(pr_tranzit,0,point_id,first_point) AS first_point "
   "      FROM points,airps WHERE points.airp=airps.code AND point_id=:point_id) p "
   "WHERE (bag_norms.airline IS NULL OR bag_norms.airline=p.airline) AND "
   "      (bag_norms.city_dep IS NULL OR bag_norms.city_dep=p.city) AND "
   "      (bag_norms.city_arv IS NULL OR "
   "       bag_norms.pr_trfer IS NULL OR bag_norms.pr_trfer<>0 OR "
   "       bag_norms.city_arv IN "
   "        (SELECT city FROM points p2,airps "
   "         WHERE p2.first_point=p.first_point AND p2.point_num>p.point_num AND p2.pr_del=0 AND "
   "               p2.airp=airps.code)) AND "
   "      (bag_norms.flt_no IS NULL OR bag_norms.flt_no=p.flt_no) AND "
   "      (bag_norms.craft IS NULL OR bag_norms.craft=p.craft) AND "
   "      (bag_norms.trip_type IS NULL OR bag_norms.trip_type=p.trip_type) AND "
   "      first_date<=scd AND (last_date IS NULL OR last_date>scd) AND "
   "      bag_norms.pr_del=0 AND "
   "      bag_norms.norm_type=:norm_type "
   "ORDER BY airline,DECODE(pr_trfer,0,0,NULL,0,1),id";
  BagNormsQry.CreateVariable("use_mark_flt",otInteger,codeshareSets.pr_mark_norms);
  BagNormsQry.CreateVariable("airline_mark",otString,pnrMarkFlt.airline);
  BagNormsQry.CreateVariable("flt_no_mark",otInteger,pnrMarkFlt.flt_no);
  BagNormsQry.CreateVariable("point_id",otInteger,flt.point_dep);
  BagNormsQry.CreateVariable("norm_type",otString,EncodeBagNormType(bntFreeExcess));
  BagNormsQry.Execute();

  bool use_mixed_norms=GetTripSets(tsMixedNorms,flt.oper);
  BagPayment::TPaxInfo pax;
  pax.pax_cat="";
  pax.target=dest.city_arv;
  pax.final_target=""; //трансфер пока не анализируем
  pax.subcl=segs.begin()->second.subcls;
  pax.cl=segs.begin()->second.cls;
  pair<CheckIn::TPaxNormItem, CheckIn::TNormItem> norm;

  BagPayment::GetPaxBagNorm(BagNormsQry, use_mixed_norms, pax, false, norm);
  bag_norm = norm.second.weight; //NoExists, если не найдена
  
  return true;
};

void TPNRInfo::add(const TPaxInfo &pax)
{
  if (pax.pax_id==NoExists) throw EXCEPTIONS::Exception("TPNRInfo::add: pax.pax_id not defined");
  paxs.insert(pax);
};

void TPNRInfo::toXML(xmlNodePtr node, bool old_style) const
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
  if (!old_style)
  {
    xmlNodePtr segsNode=NewTextChild(node, "segments");
    for(map< int/*num*/, TPNRSegInfo >::const_iterator i=segs.begin(); i!=segs.end(); ++i)
      i->second.toXML(NewTextChild(segsNode, "segment"));
  };
  bag_norm==NoExists?NewTextChild(node, "bag_norm"):
                     NewTextChild(node, "bag_norm", bag_norm);
  if (!old_style)
  {
    xmlNodePtr paxsNode=NewTextChild(node, "passengers");
    for(set<TPaxInfo>::const_iterator i=paxs.begin(); i!=paxs.end(); ++i)
      i->toXML(NewTextChild(paxsNode, "pax"));
  };
};

bool TPNRs::add(const TFlightInfo &flt, const TPNRSegInfo &seg, const TPaxInfo &pax, bool is_test)
{
  if (flt.point_dep==NoExists) throw EXCEPTIONS::Exception("TPNRs::add: flt.point_dep not defined");
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
  if (!pnr.fromDBadditional(first.flt, first.dest, is_test)) return false;

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

void TPNRs::toXML(xmlNodePtr node) const
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
  
  xmlNodePtr fltsNode=NewTextChild(node, "oper_flights");
  for(set<TFlightInfo>::const_iterator i=flights.begin(); i!=flights.end(); ++i)
    i->toXML(NewTextChild(fltsNode, "flight"));
  xmlNodePtr pnrsNode=NewTextChild(node, "pnrs");
  for(map< int/*pnr_id*/, TPNRInfo >::const_iterator i=pnrs.begin(); i!=pnrs.end(); ++i)
    i->second.toXML(NewTextChild(pnrsNode, "pnr"));
};

void findPNRs(const TPNRFilter &filter, TPNRs &PNRs, int pass)
{
  if (filter.flt_no==NoExists)
    throw EXCEPTIONS::Exception("findPNRs: filter.flt_no not defined");
  if (filter.surname.empty())
    throw EXCEPTIONS::Exception("findPNRs: filter.surname not defined");
    
  TReqInfo *reqInfo = TReqInfo::Instance();
  
  TQuery PointsQry(&OraSession);
  TQuery PaxQry(&OraSession);
	if (pass==1 || pass==2)
	{
	  ostringstream sql;
	  sql.str("");
	  sql << "SELECT points.point_id, points.point_num, points.first_point, points.pr_tranzit, "
           "       points.airline, points.flt_no, points.suffix, points.airp, "
           "       points.scd_out, points.est_out, points.act_out, points.craft, "
           "       points.airline_fmt, points.suffix_fmt, points.airp_fmt, points.craft_fmt ";
    if (pass==1)
    {
      //ищем фактический рейс
      sql << "FROM points "
        	   "WHERE ";
      if (!filter.airlines.empty())
        sql << "      points.airline IN " << GetSQLEnum(filter.airlines) << " AND ";
      sql << "      points.flt_no=:flt_no AND "
          	 "      ( :suffix IS NULL OR points.suffix=:suffix ) AND "
          	 "      ( :airp_dep IS NULL OR points.airp=:airp_dep ) AND "
          	 "      points.scd_out >= :first_date AND points.scd_out < :last_date AND "
          	 "      points.pr_del=0 AND points.pr_reg<>0";
    };
    if (pass==2)
    {
      //ищем среди телеграммных рейсов
      sql << "      ,tlg_trips.point_id AS point_id_tlg, tlg_trips.airline AS airline_tlg "
             "FROM tlg_trips, tlg_binding, points "
             "WHERE tlg_binding.point_id_tlg=tlg_trips.point_id AND "
             "      points.point_id=tlg_binding.point_id_spp AND "
             "      tlg_trips.scd >= :first_date AND tlg_trips.scd <= :last_date AND "  //'<=' это не ошибка
             "      tlg_trips.pr_utc=0 AND ";
      if (!filter.airlines.empty())
        sql << "      tlg_trips.airline IN " << GetSQLEnum(filter.airlines) << " AND ";
      sql << "      tlg_trips.flt_no=:flt_no AND "
          	 "      ( :suffix IS NULL OR tlg_trips.suffix=:suffix ) AND "
          	 "      ( :airp_dep IS NULL OR tlg_trips.airp_dep=:airp_dep ) AND "
             "      points.pr_del=0 AND points.pr_reg<>0 ";
    };

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
      	   "       crs_pax.name ";
    if (pass==1)
    {
      sql << "FROM tlg_binding,crs_pnr,crs_pax "
             "WHERE tlg_binding.point_id_tlg=crs_pnr.point_id AND "
             "      crs_pax.pnr_id=crs_pnr.pnr_id AND "
             "      tlg_binding.point_id_spp=:point_id AND ";
    };
    if (pass==2)
    {
      sql << "FROM crs_pnr,crs_pax,pnr_market_flt "
           	 "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
           	 "      crs_pnr.pnr_id=pnr_market_flt.pnr_id(+) AND "
           	 "      crs_pnr.point_id=:point_id AND "
             "      pnr_market_flt.pnr_id IS NULL AND ";
    };
    
    sql << "      crs_pnr.system='CRS' AND "
           "      (:airp_arv IS NULL OR crs_pnr.airp_arv=:airp_arv) AND ";
           
    if (filter.surname_equal_len!=NoExists)
    {
      sql << "      system.transliter_equal(SUBSTR(crs_pax.surname,1,:surname_equal_len),:surname)<>0 AND ";
      PaxQry.CreateVariable("surname", otString, filter.surname.substr(0, filter.surname_equal_len));
      PaxQry.CreateVariable("surname_equal_len", otInteger, filter.surname_equal_len);
    }
    else
    {
      sql << "      system.transliter_equal(crs_pax.surname,:surname)<>0 AND ";
      PaxQry.CreateVariable("surname", otString, filter.surname);
    };
           
    sql << "      crs_pax.pr_del=0 "
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
        if (pass==1)
        {
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
        };
        if (pass==2)
        {
          //tlg_trips.scd
          if (range_pass==0)
          {
            modf(r->first, &date_range.first);
            modf(r->second-1.0/(1440.0*60.0), &date_range.second);
          }
          else
          {
            modf(r->first-1.0, &date_range.first);
            modf(r->second+1.0-1.0/(1440.0*60.0), &date_range.second);
          };
        };
        PointsQry.SetVariable("first_date", date_range.first);
        PointsQry.SetVariable("last_date", date_range.second);
        PointsQry.Execute();
        for(;!PointsQry.Eof;PointsQry.Next())
        {
          TFlightInfo flt;
  	      if (!flt.fromDB(PointsQry)) continue;
          if ( !reqInfo->CheckAirline(flt.oper.airline) ||
               !reqInfo->CheckAirp(flt.oper.airp) ||
               (range_pass==0?flt.scd_out_local:flt.oper.scd_out)==NoExists ||
               (range_pass==0?flt.scd_out_local:flt.oper.scd_out)<r->first ||
               (range_pass==0?flt.scd_out_local:flt.oper.scd_out)>=r->second) continue;

          if (pass==1)
            flights[flt].insert(make_pair(PointsQry.FieldAsInteger("point_id"),
                                          PointsQry.FieldAsString("airline")));
          if (pass==2)
            flights[flt].insert(make_pair(PointsQry.FieldAsInteger("point_id_tlg"),
                                          PointsQry.FieldAsString("airline_tlg")));
        };
      };
    };

    for(map< TFlightInfo, map<int, string> >::const_iterator iFlt=flights.begin(); iFlt!=flights.end(); ++iFlt)
    {
      TTripRoute route;
      route.GetRouteAfter( NoExists,
                           iFlt->first.point_dep,
                           iFlt->first.point_num,
                           iFlt->first.first_point,
                           iFlt->first.pr_tranzit,
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
                pnr_filter=seg.fromDB(iFlt->first.point_dep, route, PaxQry);
                pnr_filter=pnr_filter && seg.filterFromDB(filter);
              };
              if (!pnr_filter) continue;
              TPaxInfo pax;
              if (!pax.filterFromDB(filter, PaxQry)) continue;
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
          if (!seg.fromTestPax(iFlt->first.point_dep, route, *p)) continue;
          TPaxInfo pax;
          if (!pax.fromTestPax(*p)) continue;
          PNRs.add(iFlt->first, seg, pax, true);
        };
      };
    };

	}; //pass==1 || pass==2
	
  if (pass==3 && filter.test_paxs.empty())
	{
    //ищем среди эл-тов .M из PNL
  	PointsQry.Clear();
  	PointsQry.SQLText=
  	  "SELECT points.point_id, points.point_num, points.first_point, points.pr_tranzit, "
      "       points.airline, points.flt_no, points.suffix, points.airp, "
      "       points.scd_out, points.est_out, points.act_out, points.craft, "
      "       points.airline_fmt, points.suffix_fmt, points.airp_fmt, points.craft_fmt "
  	  "FROM points,tlg_binding "
  	  "WHERE points.point_id=tlg_binding.point_id_spp AND "
  	  "      tlg_binding.point_id_tlg=:point_id_tlg AND "
  	  "      points.pr_del=0 AND points.pr_reg<>0 ";
  	PointsQry.DeclareVariable("point_id_tlg", otInteger);

    PaxQry.Clear();
    ostringstream sql;
	  sql.str("");
	  sql <<
	    "SELECT tlg_trips.point_id AS point_id_tlg, "
	    "       tlg_trips.scd, "
	    "       crs_pnr.pnr_id, "
	    "       crs_pnr.airp_arv, "
	    "       crs_pnr.class, "
	    "       crs_pnr.subclass, "
	    "       crs_pax.pax_id, "
      "       crs_pax.surname, "
  	  "       crs_pax.name "
 	    "FROM tlg_trips,crs_pnr,crs_pax,pnr_market_flt "
 	    "WHERE tlg_trips.point_id=crs_pnr.point_id AND "
 	    "      crs_pnr.pnr_id=crs_pax.pnr_id AND "
 	    "      crs_pnr.pnr_id=pnr_market_flt.pnr_id AND "
 	    "      pnr_market_flt.local_date>=:first_day AND pnr_market_flt.local_date<=:last_day AND ";
    if (!filter.airlines.empty())
      sql << "      pnr_market_flt.airline IN " << GetSQLEnum(filter.airlines) << " AND ";
    sql <<
      "      pnr_market_flt.flt_no=:flt_no AND "
 	    "      (:suffix IS NULL OR pnr_market_flt.suffix=:suffix) AND "
 	    "      (:airp_dep IS NULL OR tlg_trips.airp_dep=:airp_dep) AND " // tlg_trips.airp_dep - это не ошибка
 	    "      crs_pnr.system='CRS' AND "
 	    "      (:airp_arv IS NULL OR crs_pnr.airp_arv=:airp_arv) AND ";
 	    
 	  if (filter.surname_equal_len!=NoExists)
    {
      sql << "      system.transliter_equal(SUBSTR(crs_pax.surname,1,:surname_equal_len),:surname)<>0 AND ";
      PaxQry.CreateVariable("surname", otString, filter.surname.substr(0, filter.surname_equal_len));
      PaxQry.CreateVariable("surname_equal_len", otInteger, filter.surname_equal_len);
    }
    else
    {
      sql << "      system.transliter_equal(crs_pax.surname,:surname)<>0 AND ";
      PaxQry.CreateVariable("surname", otString, filter.surname);
    };
    sql <<
      "      crs_pax.pr_del=0 "
      "ORDER BY tlg_trips.point_id, crs_pnr.pnr_id ";

	  PaxQry.SQLText= sql.str().c_str();
    PaxQry.DeclareVariable("first_day", otInteger);
    PaxQry.DeclareVariable("last_day", otInteger);
    PaxQry.CreateVariable("flt_no", otInteger, filter.flt_no);
    PaxQry.CreateVariable("suffix", otString, filter.suffix);
    PaxQry.CreateVariable("airp_dep", otString, filter.airp_dep);
    PaxQry.CreateVariable("airp_arv", otString, filter.airp_arv);
    PaxQry.Execute();

    for(int range_pass=0; range_pass<2; range_pass++)
    {
      const vector< pair<TDateTime, TDateTime> > &scd_out_ranges=range_pass==0?
                                                                 filter.scd_out_local_ranges:
                                                                 filter.scd_out_utc_ranges;

      //цикл по диапазонам дат
      for(vector< pair<TDateTime, TDateTime> >::const_iterator r=scd_out_ranges.begin();
                                                               r!=scd_out_ranges.end(); ++r )
      {
        pair<int, int> day_range;
        int year,month;
        if (range_pass==0)
        {
          DecodeDate(r->first,year,month,day_range.first);
          DecodeDate(r->second-1.0/(1440.0*60.0),year,month,day_range.second);
        }
        else
        {
          DecodeDate(r->first-1.0,year,month,day_range.first);
          DecodeDate(r->second+1.0-1.0/(1440.0*60.0),year,month,day_range.second);
        };
        PaxQry.SetVariable("first_day", day_range.first);
        PaxQry.SetVariable("last_day", day_range.second);
        PaxQry.Execute();
        int point_id_tlg=NoExists;
        vector< pair<TFlightInfo, TTripRoute> > flts;
        int pnr_id=NoExists;
        vector< pair<TFlightInfo, TPNRSegInfo> > segs;
        for(;!PaxQry.Eof;PaxQry.Next())
        {
          if (point_id_tlg==NoExists || point_id_tlg!=PaxQry.FieldAsInteger("point_id_tlg"))
          {
            point_id_tlg=PaxQry.FieldAsInteger("point_id_tlg");

            PointsQry.SetVariable("point_id_tlg",point_id_tlg);
            PointsQry.Execute();
            flts.clear();
            for(;!PointsQry.Eof;PointsQry.Next())
            {
              TFlightInfo flt;
        	    flt.fromDB(PointsQry);
              if ( !reqInfo->CheckAirline(flt.oper.airline) ||
                   !reqInfo->CheckAirp(flt.oper.airp) ||
                   (range_pass==0?flt.scd_out_local:flt.oper.scd_out)==NoExists ||
                   (range_pass==0?flt.scd_out_local:flt.oper.scd_out)<r->first ||
                   (range_pass==0?flt.scd_out_local:flt.oper.scd_out)>=r->second) continue;

              TTripRoute route;
              route.GetRouteAfter( NoExists,
                                   flt.point_dep,
                                   flt.point_num,
                                   flt.first_point,
                                   flt.pr_tranzit,
                                   trtNotCurrent,
                                   trtNotCancelled );

              flts.push_back( make_pair(flt, route) );
            };
          };
          if (flts.empty()) continue;

          if (pnr_id==NoExists || pnr_id!=PaxQry.FieldAsInteger("pnr_id"))
          {
            pnr_id=PaxQry.FieldAsInteger("pnr_id");
            segs.clear();
            for(vector< pair<TFlightInfo, TTripRoute> >::const_iterator i=flts.begin(); i!=flts.end(); ++i)
            {
              TPNRSegInfo seg;
              bool pnr_filter=seg.fromDB(i->first.point_dep, i->second, PaxQry);
              pnr_filter=pnr_filter && seg.filterFromDB(filter);
              if (pnr_filter)
                segs.push_back( make_pair(i->first, seg) );
            };
          };
          if (segs.empty()) continue;
          TPaxInfo pax;
          if (!pax.filterFromDB(filter, PaxQry)) continue;
          for(vector< pair<TFlightInfo, TPNRSegInfo> >::const_iterator i=segs.begin(); i!=segs.end(); ++i)
            PNRs.add(i->first, i->second, pax, false);

        };
      };
    }; //range_pass
  }; //pass==3 && filter.test_paxs.empty()
};

void getTCkinData( const TPnrData &first,
                   bool is_test,
                   vector<TPnrData> &other)
{
  other.clear();
  checkPnrData(first.flt, first.dest, first.seg, "getTCkinData");
  
  if (is_test) return;

  TReqInfo *reqInfo = TReqInfo::Instance();

  //поиск стыковочных сегментов (возвращаем вектор point_id)

  TQuery Qry(&OraSession);
  map<int, CheckIn::TTransferItem> crs_trfer;
  CheckInInterface::GetOnwardCrsTransfer(first.seg.pnr_id, Qry, first.flt.oper, first.dest.airp_arv, crs_trfer);
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
    }
    else
    {
      Qry.SQLText=
        "SELECT pr_tckin FROM trip_ckin_client "
        "WHERE point_id=:point_id AND client_type=:client_type AND desk_grp_id IS NULL";
    };
    Qry.CreateVariable("point_id", otInteger, first.flt.point_dep);
    Qry.CreateVariable("client_type", otString, EncodeClientType(reqInfo->client_type));
    Qry.Execute();
    if (Qry.Eof || Qry.FieldAsInteger("pr_tckin")==0)
    {
      //сквозная регистрация запрещена
      ProgTrace(TRACE5, ">>>> Through check-in not permitted (point_id=%d, client_type=%s, desk_grp_id=%d)",
                        first.flt.point_dep, EncodeClientType(reqInfo->client_type), reqInfo->desk.grp_id);
      return;
    };

    map<int, pair<TCkinSegFlts, TTrferSetsInfo> > trfer_segs;
    traceTrfer(TRACE5, "getTCkinData: crs_trfer", crs_trfer);
    CheckInInterface::GetTrferSets(first.flt.oper,
                                   first.dest.airp_arv,
                                   "",
                                   crs_trfer,
                                   false,
                                   trfer_segs);
    traceTrfer(TRACE5, "getTCkinData: trfer_segs", trfer_segs);
    if (crs_trfer.size()!=trfer_segs.size())
      throw EXCEPTIONS::Exception("getTCkinData: different array sizes "
                                  "(crs_trfer.size()=%d, trfer_segs.size()=%d)",
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

        Qry.Clear();
        Qry.SQLText="SELECT COUNT(*) AS num FROM trip_classes WHERE point_id=:point_id";
        Qry.CreateVariable("point_id",otInteger,currSeg.point_dep);
        Qry.Execute();
        if (Qry.Eof || Qry.FieldAsInteger("num")==0)
          throw "Configuration of the flight not assigned";
          
        TPnrData pnrData;
        if (!pnrData.flt.fromDB(currSeg.point_dep, false, false))
          throw "Error in TFlightInfo::fromDB";
        if (!pnrData.flt.fromDBadditional(false, false))
          throw "Error in TFlightInfo::fromDBadditional";

        if (reqInfo->client_type==ctKiosk)
        {
          if ( pnrData.flt.stages[ sOpenKIOSKCheckIn ] == NoExists ||
               pnrData.flt.stages[ sCloseKIOSKCheckIn ] == NoExists )
            throw "Stage of kiosk check-in not found";
        }
        else
        {
          if ( pnrData.flt.stages[ sOpenWEBCheckIn ] == NoExists ||
               pnrData.flt.stages[ sCloseWEBCheckIn ] == NoExists ||
               pnrData.flt.stages[ sCloseWEBCancel ] == NoExists)
            throw "Stage of web check-in not found";
        };

        if (first.seg.pnr_addrs.empty())
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
                               pnrData.flt.point_dep,
                               pnrData.flt.point_num,
                               pnrData.flt.first_point,
                               pnrData.flt.pr_tranzit,
                               trtNotCurrent,
                               trtNotCancelled );

          for(;!Qry.Eof;Qry.Next())
          {
            TPNRSegInfo seg;
            if (!seg.fromDB(pnrData.flt.point_dep, route, Qry)) continue;
            if (!seg.filterFromDB(first.seg.pnr_addrs)) continue;
            if (pnrData.seg.pnr_id!=NoExists) //дубль PNR
              throw "More than one PNR found";
            pnrData.seg=seg;
          };
          if (!Qry.Eof) ;
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
};

} // namespace WebSearch

