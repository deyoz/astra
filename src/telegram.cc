#include <vector>
#include <utility>
#include <boost/date_time/local_time/local_time.hpp>
#include "telegram.h"
#include "xml_unit.h"
#include "oralib.h"
#include "exceptions.h"
#include "misc.h"
#include "astra_utils.h"
#include "tlg/tlg.h"
#include "tlg/tlg_parser.h"
#include "base_tables.h"
#include "astra_misc.h"
#include "astra_service.h"
#include "file_queue.h"
#include "http_io.h"
#include "typeb_utils.h"
#include "term_version.h"
#include "serverlib/logger.h"
#include "serverlib/posthooks.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

#include "alarms.h"

#define ENDL "\015\012"

using namespace std;
using namespace ASTRA;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace boost::local_time;

const std::string PARAM_TLG_TYPE = "TLG_TYPE";

void TelegramInterface::readTripData( int point_id, xmlNodePtr dataNode )
{
  xmlNodePtr tripdataNode = NewTextChild( dataNode, "tripdata" );
  xmlNodePtr node;

  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText =
    "SELECT airline,flt_no,suffix,airp,scd_out, "
    "       point_num, first_point, pr_tranzit "
    "FROM points WHERE point_id=:point_id AND pr_del>=0";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();
  if (Qry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");

  TTripInfo fltInfo(Qry);
  TTripRoute route;

  route.GetRouteAfter(NoExists,
                      point_id,
                      Qry.FieldAsInteger("point_num"),
                      Qry.FieldIsNULL("first_point")?NoExists:Qry.FieldAsInteger("first_point"),
                      Qry.FieldAsInteger("pr_tranzit")!=0,
                      trtNotCurrent,trtNotCancelled);

  node = NewTextChild( tripdataNode, "airps" );
  vector<string> airps;
  vector<string>::iterator i;
  for(TTripRoute::iterator r=route.begin();r!=route.end();r++)
  {
    //проверим на дублирование кодов аэропортов в рамках одного рейса
    for(i=airps.begin();i!=airps.end();i++)
      if (*i==r->airp) break;
    if (i!=airps.end()) continue;

    NewTextChild( node, "airp", r->airp );

    airps.push_back(r->airp);
  };

  vector<TTripInfo> markFltInfo;
  GetMktFlights(fltInfo,markFltInfo);
  if (!markFltInfo.empty())
  {
    node = NewTextChild( tripdataNode, "mark_flights" );
    for(vector<TTripInfo>::iterator f=markFltInfo.begin();f!=markFltInfo.end();f++)
    {
      xmlNodePtr fltNode=NewTextChild(node,"flight");
      NewTextChild(fltNode,"airline",f->airline);
      NewTextChild(fltNode,"flt_no",f->flt_no);
      NewTextChild(fltNode,"suffix",f->suffix);
      ostringstream flt_str;
      flt_str << ElemIdToCodeNative(etAirline, f->airline)
              << setw(3) << setfill('0') << f->flt_no
              << ElemIdToCodeNative(etSuffix, f->suffix);
      NewTextChild(fltNode,"flt_str",flt_str.str());
    };
  };

  //зачитаем все источники PNL на данный рейс
  vector<string> crs;
  GetCrsList(point_id,crs);
  if (!crs.empty())
  {
    node = NewTextChild( tripdataNode, "crs_list" );
    for(vector<string>::iterator c=crs.begin();c!=crs.end();c++)
      NewTextChild(node,"crs",*c);
  };
};

struct TTlgSearchParams {
    int err_cls;
    int tlg_id;
    string tlg_type;
    string airline;
    string airp;
    int flt_no;
    string suffix;
    bool pr_time_create;
    bool pr_time_receive;
    TDateTime TimeCreateFrom;
    TDateTime TimeCreateTo;
    TDateTime TimeReceiveFrom;
    TDateTime TimeReceiveTo;
    void get(xmlNodePtr reqNode);
    void dump();
    TTlgSearchParams():
        err_cls(0),
        flt_no(NoExists),
        pr_time_create(false),
        pr_time_receive(false),
        TimeCreateFrom(0),
        TimeCreateTo(0),
        TimeReceiveFrom(0),
        TimeReceiveTo(0)
    {};
};

void TTlgSearchParams::dump()
{
    ProgTrace(TRACE5, "err_cls: %d", err_cls);
    ProgTrace(TRACE5, "tlg_id: %d", tlg_id);
    ProgTrace(TRACE5, "tlg_type: '%s'", tlg_type.c_str());
    ProgTrace(TRACE5, "airline: '%s'", airline.c_str());
    ProgTrace(TRACE5, "airp: '%s'", airp.c_str());
    if(flt_no == NoExists)
        ProgTrace(TRACE5, "flt_no: NoExists");
    else
        ProgTrace(TRACE5, "flt_no: %d", flt_no);
    ProgTrace(TRACE5, "suffix: '%s'", suffix.c_str());
    ProgTrace(TRACE5, "pr_time_create: %s", pr_time_create ? "true" : "false");
    ProgTrace(TRACE5, "pr_time_receive: %s", pr_time_receive ? "true" : "false");
    if(pr_time_create) {
        ProgTrace(TRACE5, "TimeCreateFrom: %s", DateTimeToStr(TimeCreateFrom, ServerFormatDateTimeAsString).c_str());
        ProgTrace(TRACE5, "TimeCreateTo: %s", DateTimeToStr(TimeCreateTo, ServerFormatDateTimeAsString).c_str());
    }
    if(pr_time_receive) {
        ProgTrace(TRACE5, "TimeReceiveFrom: %s", DateTimeToStr(TimeReceiveFrom, ServerFormatDateTimeAsString).c_str());
        ProgTrace(TRACE5, "TimeReceiveTo: %s", DateTimeToStr(TimeReceiveTo, ServerFormatDateTimeAsString).c_str());
    }
}

void TTlgSearchParams::get(xmlNodePtr reqNode)
{
    xmlNodePtr currNode = reqNode->children;
    if(currNode == NULL) return;

    err_cls = NodeAsIntegerFast("err_cls", currNode, 0);
    tlg_id = NodeAsIntegerFast("tlg_id", currNode, NoExists);
    tlg_type = NodeAsStringFast("tlg_type", currNode, "");
    airline = NodeAsStringFast("airline", currNode, "");
    airp = NodeAsStringFast("airp", currNode, "");
    flt_no = NodeAsIntegerFast("flt_no", currNode, NoExists);
    suffix = NodeAsStringFast("suffix", currNode, "");
    pr_time_create = NodeAsIntegerFast("pr_time_create", currNode, 0) != 0;
    pr_time_receive = NodeAsIntegerFast("pr_time_receive", currNode, 0) != 0;
    if (!pr_time_create && !pr_time_receive && tlg_id==NoExists)
      throw AstraLocale::UserException("MSG.NOT_SET_RANGE_OR_TLG_ID");
    
    if(pr_time_create) {
        TimeCreateFrom = NodeAsDateTimeFast("TimeCreateFrom", currNode);
        TimeCreateTo = NodeAsDateTimeFast("TimeCreateTo", currNode);
        if(TimeCreateFrom + 7 < TimeCreateTo)
            throw AstraLocale::UserException("MSG.SEARCH_PERIOD_MAX_N_DAYS", LParams()<<LParam("days", 7));
    }
    if(pr_time_receive) {
        TimeReceiveFrom = NodeAsDateTimeFast("TimeReceiveFrom", currNode);
        TimeReceiveTo = NodeAsDateTimeFast("TimeReceiveTo", currNode);
        if(TimeReceiveFrom + 7 < TimeReceiveTo)
            throw AstraLocale::UserException("MSG.SEARCH_PERIOD_MAX_N_DAYS", LParams()<<LParam("days", 7));
    }
}

void set_tlgs_in_search_params(TTlgSearchParams &search_params, string &sql, TQuery &Qry)
{
    if(search_params.pr_time_create) {
        sql +=
            " AND tlgs_in.time_create >= :TimeCreateFrom AND tlgs_in.time_create < :TimeCreateTo \n";
        Qry.CreateVariable("TimeCreateFrom", otDate, search_params.TimeCreateFrom);
        Qry.CreateVariable("TimeCreateTo", otDate, search_params.TimeCreateTo);
    }

    if(search_params.pr_time_receive) {
        sql +=
            " AND tlgs_in.time_receive >= :TimeReceiveFrom AND tlgs_in.time_receive < :TimeReceiveTo \n";
        Qry.CreateVariable("TimeReceiveFrom", otDate, search_params.TimeReceiveFrom);
        Qry.CreateVariable("TimeReceiveTo", otDate, search_params.TimeReceiveTo);
    }
    
    if(!search_params.tlg_type.empty()) {
        sql +=
            " AND tlgs_in.type = :tlg_type \n";
        Qry.CreateVariable("tlg_type", otString, search_params.tlg_type);
    }
    
    if(search_params.tlg_id!=NoExists) {
        sql +=
            " AND tlgs_in.id = :tlg_id \n";
        Qry.CreateVariable("tlg_id", otInteger, search_params.tlg_id);
    }

}

string GetValidXMLString(const std::string& str)
{
  ostringstream result;
  for(string::const_iterator i=str.begin(); i!=str.end(); i++)
  {
    if (!ValidXMLChar(*i))
    {
      result << '$'
             << setw(2) << setfill('0') << setbase(16) << (int)((unsigned char)(*i)); //можно и так
      continue;
    };
    result << *i;
  };
  return result.str();
};


void TelegramInterface::GetTlgIn2(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo &info = *(TReqInfo::Instance());
    TTlgSearchParams search_params;
    search_params.get(reqNode);
    search_params.dump();
    bool is_trip_info =
        not search_params.airline.empty() or
        not search_params.airp.empty() or
        search_params.flt_no != NoExists;
    TQuery Qry(&OraSession);
    string tz_region =  info.desk.tz_region;
    string sql="SELECT tlgs_in.id,num,type,addr,heading,body,ending,time_receive \n"
        "FROM tlgs_in, \n";
    sql+="( \n";
    if(search_params.err_cls == 1) {
        sql+=
            " SELECT DISTINCT tlgs_in.id \n"
            " FROM tlgs_in,tlg_source \n"
            " WHERE tlgs_in.id=tlg_source.tlg_id(+) AND tlg_source.tlg_id IS NULL \n";
        set_tlgs_in_search_params(search_params, sql, Qry);
    } else {
        if (!info.user.access.airlines.empty()||
                !info.user.access.airps.empty() ||
                is_trip_info
           )
        {
            sql+="SELECT DISTINCT ids.id \n"
                "FROM tlg_trips \n";
            sql+=",( \n";

        };
        sql+="SELECT DISTINCT tlgs_in.id \n";
        if (!info.user.access.airlines.empty()||
                !info.user.access.airps.empty() ||
                is_trip_info
           )
            sql+=",tlg_source.point_id_tlg \n";
        sql+="FROM tlgs_in,tlg_source,tlg_binding \n"
            "WHERE tlgs_in.id=tlg_source.tlg_id AND \n"
            "      tlg_source.point_id_tlg=tlg_binding.point_id_tlg(+) AND \n"
            "      tlg_binding.point_id_tlg IS NULL \n";
        set_tlgs_in_search_params(search_params, sql, Qry);
        if (!info.user.access.airlines.empty()||
                !info.user.access.airps.empty() ||
                is_trip_info
           )
        {
            sql+="ORDER BY tlgs_in.id) ids \n"
                "WHERE ids.point_id_tlg=tlg_trips.point_id \n";
            if (!info.user.access.airlines.empty())
            {
                if (info.user.access.airlines_permit)
                    sql+="AND tlg_trips.airline IN "+GetSQLEnum(info.user.access.airlines);
                else
                    sql+="AND tlg_trips.airline NOT IN "+GetSQLEnum(info.user.access.airlines);
            };
            if (!info.user.access.airps.empty())
            {
                if (info.user.access.airps_permit)
                    sql+="AND (tlg_trips.airp_dep IS NULL AND tlg_trips.airp_arv IS NULL OR \n"
                        "     tlg_trips.airp_dep IN "+GetSQLEnum(info.user.access.airps)+" OR \n"+
                        "     tlg_trips.airp_arv IN "+GetSQLEnum(info.user.access.airps)+") \n" ;
                else
                    sql+="AND (tlg_trips.airp_dep IS NULL AND tlg_trips.airp_arv IS NULL OR \n"
                        "     tlg_trips.airp_dep NOT IN "+GetSQLEnum(info.user.access.airps)+" OR \n"+
                        "     tlg_trips.airp_arv NOT IN "+GetSQLEnum(info.user.access.airps)+")\n" ;
            };
            if(is_trip_info) {
                if(not search_params.airline.empty()) {
                    sql +=
                        "and tlg_trips.airline = :airline \n";
                    Qry.CreateVariable("airline", otString, search_params.airline);
                }
                if(not search_params.airp.empty()) {
                    sql +=
                        "and tlg_trips.airp_dep = :airp \n";
                    Qry.CreateVariable("airp", otString, search_params.airp);
                }
                if(search_params.flt_no != NoExists) {
                    sql +=
                        "and tlg_trips.flt_no = :flt_no \n";
                    Qry.CreateVariable("flt_no", otInteger, search_params.flt_no);
                    if(not search_params.suffix.empty()) {
                        sql +=
                            "and tlg_trips.suffix = :suffix \n";
                        Qry.CreateVariable("suffix", otString, search_params.suffix);
                    }
                }
            }
        };
    }

    sql+=
        ") ids \n";
    sql+="WHERE tlgs_in.id=ids.id \n"
         "ORDER BY id,num \n";

    xmlNodePtr tlgsNode = NewTextChild( resNode, "tlgs" );
    if ((info.user.access.airps_permit && info.user.access.airps.empty()) ||
        (info.user.access.airlines_permit && info.user.access.airlines.empty()) ) return;

    ProgTrace(TRACE5, "sql: %s", sql.c_str());
    Qry.SQLText=sql;
    Qry.Execute();
    xmlNodePtr node;
    int len,bufLen=0;
    char *ph,*buf=NULL;
    try
    {
        int rowcount = 0;
        for(int pass=1; pass<=2; pass++)
        {
          if (pass==2)
          {
            if (search_params.err_cls == 1 && search_params.tlg_id!=NoExists)
            {
              Qry.Clear();
              Qry.SQLText=
                "SELECT id, 1 AS num, NULL AS type, NULL AS addr, NULL AS heading, \n"
                "       tlg_text AS body, NULL AS ending, time AS time_receive \n"
                "FROM tlgs \n"
                "WHERE id=:tlg_id AND error IS NOT NULL AND type IN ('INA','INB') \n";
              Qry.CreateVariable("tlg_id", otInteger, search_params.tlg_id);
              Qry.Execute();
            }
            else break;
          };
        
          if(!Qry.Eof) {
              int col_type = Qry.FieldIndex("type");
              int col_id = Qry.FieldIndex("id");
              int col_num = Qry.FieldIndex("num");
              int col_addr = Qry.FieldIndex("addr");
              int col_heading = Qry.FieldIndex("heading");
              int col_ending = Qry.FieldIndex("ending");
              int col_time_receive = Qry.FieldIndex("time_receive");
              int col_body = Qry.FieldIndex("body");

              for(;!Qry.Eof;Qry.Next())
              {
                  node = NewTextChild( tlgsNode, "tlg" );
                  NewTextChild( node, "id", Qry.FieldAsInteger(col_id) );
                  NewTextChild( node, "num", Qry.FieldAsInteger(col_num) );
                  NewTextChild( node, "type", Qry.FieldAsString(col_type));
                  NewTextChild( node, "addr", GetValidXMLString(Qry.FieldAsString(col_addr)) );
                  NewTextChild( node, "heading", GetValidXMLString(Qry.FieldAsString(col_heading)) );
                  NewTextChild( node, "ending", GetValidXMLString(Qry.FieldAsString(col_ending)) );
                  TDateTime time_receive = UTCToClient( Qry.FieldAsDateTime(col_time_receive), tz_region );
                  NewTextChild( node, "time_receive", DateTimeToStr( time_receive ) );

                  len=Qry.GetSizeLongField(col_body)+1;
                  if (len>bufLen)
                  {
                      if (buf==NULL)
                          ph=(char*)malloc(len);
                      else
                          ph=(char*)realloc(buf,len);
                      if (ph==NULL) throw EMemoryError("Out of memory");
                      buf=ph;
                      bufLen=len;
                  };
                  Qry.FieldAsLong("body",buf);
                  buf[len-1]=0;

                  string body(buf,len-1);
                  NewTextChild( node, "body", GetValidXMLString(body) );
                  rowcount++;
              };
              if(rowcount >= 4000)
                  throw AstraLocale::UserException("MSG.TOO_MANY_DATA.ADJUST_SEARCH_PARAMS");
          }
        };
        if (buf!=NULL) free(buf);
    }
    catch(...)
    {
        if (buf!=NULL) free(buf);
        throw;
    };
}

void TelegramInterface::GetTlgIn(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo &info = *(TReqInfo::Instance());
  int point_id = NodeAsInteger( "point_id", reqNode );

  TQuery Qry(&OraSession);
  string tz_region =  info.desk.tz_region;
  string sql="SELECT tlgs_in.id,num,type,addr,heading,body,ending,time_receive \n"
             "FROM tlgs_in, \n";
  if (point_id!=-1)
  {
    TQuery RegionQry(&OraSession);
    RegionQry.SQLText="SELECT airp FROM points WHERE point_id=:point_id AND pr_del>=0";
    RegionQry.CreateVariable("point_id",otInteger,point_id);
    RegionQry.Execute();
    if (RegionQry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");
    tz_region = AirpTZRegion(RegionQry.FieldAsString("airp"));

    sql+="(SELECT DISTINCT tlg_source.tlg_id AS id  \n"
         " FROM tlg_source,tlg_binding  \n"
         " WHERE tlg_source.point_id_tlg=tlg_binding.point_id_tlg AND  \n"
         "       tlg_binding.point_id_spp=:point_id) ids  \n";
    Qry.CreateVariable("point_id",otInteger,point_id);
  }
  else
  {
    sql+="( \n";
    if (!info.user.access.airlines.empty()||
        !info.user.access.airps.empty())
    {
      sql+="SELECT DISTINCT ids.id \n"
           "FROM tlg_trips \n";
      sql+=",( \n";

    };
    sql+="SELECT DISTINCT tlgs_in.id \n";
    if (!info.user.access.airlines.empty()||
        !info.user.access.airps.empty()) sql+=",tlg_source.point_id_tlg \n";
    sql+="FROM tlgs_in,tlg_source,tlg_binding \n"
         "WHERE tlgs_in.id=tlg_source.tlg_id AND \n"
         "      tlg_source.point_id_tlg=tlg_binding.point_id_tlg(+) AND \n"
         "      tlg_binding.point_id_tlg IS NULL AND \n"
         "      time_receive>=TRUNC(system.UTCSYSDATE)-2 \n";
    if (!info.user.access.airlines.empty()||
        !info.user.access.airps.empty())
    {
      sql+="ORDER BY tlgs_in.id) ids \n"
           "WHERE ids.point_id_tlg=tlg_trips.point_id \n";
      if (!info.user.access.airlines.empty())
      {
        if (info.user.access.airlines_permit)
          sql+="AND tlg_trips.airline IN "+GetSQLEnum(info.user.access.airlines);
        else
          sql+="AND tlg_trips.airline NOT IN "+GetSQLEnum(info.user.access.airlines);
      };
      if (!info.user.access.airps.empty())
      {
        if (info.user.access.airps_permit)
          sql+="AND (tlg_trips.airp_dep IS NULL AND tlg_trips.airp_arv IS NULL OR \n"
               "     tlg_trips.airp_dep IN "+GetSQLEnum(info.user.access.airps)+" OR \n"+
               "     tlg_trips.airp_arv IN "+GetSQLEnum(info.user.access.airps)+") \n" ;
        else
          sql+="AND (tlg_trips.airp_dep IS NULL AND tlg_trips.airp_arv IS NULL OR \n"
               "     tlg_trips.airp_dep NOT IN "+GetSQLEnum(info.user.access.airps)+" OR \n"+
               "     tlg_trips.airp_arv NOT IN "+GetSQLEnum(info.user.access.airps)+")\n" ;
      };
    };
    sql+=" UNION \n"
         " SELECT DISTINCT tlgs_in.id \n"
         " FROM tlgs_in,tlg_source \n"
         " WHERE tlgs_in.id=tlg_source.tlg_id(+) AND tlg_source.tlg_id IS NULL AND \n"
         "       time_receive>=TRUNC(system.UTCSYSDATE)-2 \n"
         ") ids \n";
  };
  sql+="WHERE tlgs_in.id=ids.id \n"
       "ORDER BY id,num \n";

  xmlNodePtr tlgsNode = NewTextChild( resNode, "tlgs" );
  if ((info.user.access.airps_permit && info.user.access.airps.empty()) ||
      (info.user.access.airlines_permit && info.user.access.airlines.empty()) ) return;

  ProgTrace(TRACE5, "sql: %s", sql.c_str());
  Qry.SQLText=sql;
  Qry.Execute();
  xmlNodePtr node;
  int len,bufLen=0;
  char *ph,*buf=NULL;
  try
  {
    for(;!Qry.Eof;Qry.Next())
    {
      node = NewTextChild( tlgsNode, "tlg" );
      NewTextChild( node, "id", Qry.FieldAsInteger("id") );
      NewTextChild( node, "num", Qry.FieldAsInteger("num") );
      NewTextChild( node, "type", Qry.FieldAsString("type") );
      NewTextChild( node, "addr", GetValidXMLString(Qry.FieldAsString("addr")) );
      NewTextChild( node, "heading", GetValidXMLString(Qry.FieldAsString("heading")) );
      NewTextChild( node, "ending", GetValidXMLString(Qry.FieldAsString("ending")) );
      TDateTime time_receive = UTCToClient( Qry.FieldAsDateTime("time_receive"), tz_region );
      NewTextChild( node, "time_receive", DateTimeToStr( time_receive ) );

      len=Qry.GetSizeLongField("body")+1;
      if (len>bufLen)
      {
        if (buf==NULL)
          ph=(char*)malloc(len);
        else
          ph=(char*)realloc(buf,len);
        if (ph==NULL) throw EMemoryError("Out of memory");
        buf=ph;
        bufLen=len;
      };
      Qry.FieldAsLong("body",buf);
      buf[len-1]=0;
      string body(buf,len-1);
      NewTextChild( node, "body", GetValidXMLString(body) );
    };
    if (buf!=NULL) free(buf);
  }
  catch(...)
  {
    if (buf!=NULL) free(buf);
    throw;
  };
};

void TelegramInterface::GetTlgOut(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  xmlNodePtr node = GetNode( "point_id", reqNode );
  int point_id;
  TQuery Qry(&OraSession);
  string tz_region;
  ostringstream sql;
  sql << "SELECT point_id,id,num,addr,heading,body,ending, "
         "       pr_lat,completed,has_errors,time_create,time_send_scd,time_send_act, "
         "       type AS tlg_type, "
         "       typeb_out_extra.text AS extra "
         "FROM tlg_out, typeb_out_extra "
         "WHERE tlg_out.id=typeb_out_extra.tlg_id(+) AND "
         "      typeb_out_extra.lang(+)=:lang ";
  Qry.CreateVariable("lang", otString, TReqInfo::Instance()->desk.lang);
  if (node==NULL)
  {
    int tlg_id = NodeAsInteger( "tlg_id", reqNode );
    sql << " AND id=:tlg_id ";
    Qry.CreateVariable("tlg_id",otInteger,tlg_id);
  }
  else
  {
    point_id = NodeAsInteger( node );
    if (point_id!=-1)
    {
      sql << " AND point_id=:point_id ";
      Qry.CreateVariable("point_id",otInteger,point_id);
    }
    else
    {
      sql << " AND point_id IS NULL AND time_create>=TRUNC(system.UTCSYSDATE)-2 ";
    };

  };
  sql << "ORDER BY id,num";

  Qry.SQLText=sql.str();
  Qry.Execute();
  xmlNodePtr tlgsNode = NewTextChild( resNode, "tlgs" );

  if (!Qry.Eof)
  {
    if (!Qry.FieldIsNULL("point_id"))
    {
      point_id = Qry.FieldAsInteger("point_id");
      TQuery RegionQry(&OraSession);
      RegionQry.SQLText="SELECT airp FROM points WHERE point_id=:point_id AND pr_del>=0";
      RegionQry.CreateVariable("point_id",otInteger,point_id);
      RegionQry.Execute();
      if (RegionQry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");
      tz_region = AirpTZRegion(RegionQry.FieldAsString("airp"));
    }
    else
    {
      point_id = -1;
      tz_region =  TReqInfo::Instance()->desk.tz_region;
    };
  };

  ostringstream endl_stream;
  endl_stream << endl;
  for(;!Qry.Eof;Qry.Next())
  {
    node = NewTextChild( tlgsNode, "tlg" );
    string tlg_type = Qry.FieldAsString("tlg_type");

    TTypeBTypesRow& row = (TTypeBTypesRow&)(base_tables.get("typeb_types").get_row("code",tlg_type));
    string basic_type = row.basic_type;

    NewTextChild( node, "id", Qry.FieldAsInteger("id") );
    NewTextChild( node, "num", Qry.FieldAsInteger("num") );
    NewTextChild( node, "tlg_type", tlg_type, basic_type );
    NewTextChild( node, "tlg_short_name", ElemIdToNameShort(etTypeBType, tlg_type), basic_type );
    NewTextChild( node, "basic_type", basic_type );
    bool editable = row.editable;
    bool completed = Qry.FieldAsInteger("completed") != 0;
    bool has_errors = Qry.FieldAsInteger("has_errors") != 0;
    if(editable)
        editable = not has_errors;
    if(completed)
        completed = not has_errors;
    NewTextChild( node, "editable", editable, false );

    NewTextChild( node, "addr", Qry.FieldAsString("addr") );
    NewTextChild( node, "heading", Qry.FieldAsString("heading") );
    NewTextChild( node, "ending", Qry.FieldAsString("ending") );
    if(TReqInfo::Instance()->desk.compatible(CACHE_CHILD_VERSION))
      NewTextChild( node, "extra", Qry.FieldAsString("extra"), "" );
    else
      NewTextChild( node, "extra", CharReplace(Qry.FieldAsString("extra"),endl_stream.str().c_str()," "), "" );

    NewTextChild( node, "pr_lat", (int)(Qry.FieldAsInteger("pr_lat")!=0) );
    NewTextChild( node, "completed", completed, true );

    TDateTime time_create = UTCToClient( Qry.FieldAsDateTime("time_create"), tz_region );
    NewTextChild( node, "time_create", DateTimeToStr( time_create ) );

    if (!Qry.FieldIsNULL("time_send_scd"))
    {
      TDateTime time_send_scd = UTCToClient( Qry.FieldAsDateTime("time_send_scd"), tz_region );
      NewTextChild( node, "time_send_scd", DateTimeToStr( time_send_scd ) );
    }
    else
      NewTextChild( node, "time_send_scd" );

    if (!Qry.FieldIsNULL("time_send_act"))
    {
      TDateTime time_send_act = UTCToClient( Qry.FieldAsDateTime("time_send_act"), tz_region );
      NewTextChild( node, "time_send_act", DateTimeToStr( time_send_act ) );
    }
    else
      NewTextChild( node, "time_send_act" );

    NewTextChild( node, "body", Qry.FieldAsString("body") );
  };
};

void TelegramInterface::GetAddrs(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TypeB::TAddrInfo info;
  info.fromXML(reqNode);
  string addrs;

  if (info.sendInfo.point_id!=NoExists)
  {
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText=
      "SELECT airline,flt_no,airp,point_num,first_point,pr_tranzit "
      "FROM points WHERE point_id=:point_id AND pr_del>=0";
    Qry.CreateVariable("point_id",otInteger,info.sendInfo.point_id);
    Qry.Execute();
    if (Qry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");

    info.sendInfo.airline=Qry.FieldAsString("airline");
    info.sendInfo.flt_no=Qry.FieldAsInteger("flt_no");
    info.sendInfo.airp_dep=Qry.FieldAsString("airp");
    info.sendInfo.point_num=Qry.FieldAsInteger("point_num");
    info.sendInfo.first_point=Qry.FieldIsNULL("first_point")?NoExists:Qry.FieldAsInteger("first_point");
    info.sendInfo.pr_tranzit=Qry.FieldAsInteger("pr_tranzit")!=0;
    addrs=info.getAddrs();
  };

  NewTextChild(resNode,"addrs",addrs);
  return;
};

void TelegramInterface::LoadTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  string text = NodeAsString("tlg_text",reqNode);
  if (text.empty()) throw AstraLocale::UserException("MSG.TLG.EMPTY");
  loadTlg(text);
  registerHookAfter(sendCmdTypeBHandler);
  AstraLocale::showMessage("MSG.TLG.LOADED");
};

void TelegramInterface::SaveTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int tlg_id = NodeAsInteger( "tlg_id", reqNode );
  string tlg_body = NodeAsString( "tlg_body", reqNode );
  if (tlg_body.size()>PART_SIZE)
    throw AstraLocale::UserException("MSG.TLG.MAX_LENGTH", LParams() << LParam("count", (int)PART_SIZE));
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT typeb_types.short_name, "
    "       point_id, NVL(LENGTH(addr),0)+NVL(LENGTH(heading),0)+NVL(LENGTH(ending),0) AS len "
    "FROM tlg_out,typeb_types "
    "WHERE tlg_out.type=typeb_types.code AND id=:id AND num=1 FOR UPDATE";
  Qry.CreateVariable( "id", otInteger, tlg_id);
  Qry.Execute();
  if (Qry.Eof) throw AstraLocale::UserException("MSG.TLG.NOT_FOUND.REFRESH_DATA");
  
  if (tlg_body.size()+Qry.FieldAsInteger("len") > PART_SIZE)
    throw AstraLocale::UserException("MSG.TLG.MAX_LENGTH", LParams() << LParam("count", (int)PART_SIZE));

  string tlg_short_name=Qry.FieldAsString("short_name");
  int point_id=Qry.FieldAsInteger("point_id");

  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  DELETE FROM tlg_out WHERE id=:id AND num<>1; "
    "  UPDATE tlg_out SET body=:body,completed=1 WHERE id=:id; "
    "END;";
  Qry.CreateVariable( "id", otInteger, tlg_id);
  // Если в конце телеграммы нет перевода строки, добавим его
  if(tlg_body.size() > 2 and tlg_body.substr(tlg_body.size() - 2) != "\xd\xa")
      tlg_body += "\xd\xa";
  ProgTrace(TRACE5, "tlg_body: %s", tlg_body.c_str());
  Qry.CreateVariable( "body", otString, tlg_body );
  Qry.Execute();

  check_tlg_out_alarm(point_id);

  ostringstream msg;
  msg << "Телеграмма " << tlg_short_name << " (ид=" << tlg_id << ") изменена";
  TReqInfo::Instance()->MsgToLog(msg.str(),evtTlg,point_id,tlg_id);
  AstraLocale::showMessage("MSG.TLG.SAVED");
};

void putUTG(int id, const string &basic_type, const TTripInfo &flt, const string &data)
{
    map<string, string> file_params;
    TFileQueue::add_sets_params( flt.airp,
                                 flt.airline,
                                 IntToString(flt.flt_no),
                                 OWN_POINT_ADDR(),
                                 FILE_UTG_TYPE,
                                 1,
                                 file_params );
    if(not file_params.empty() and (file_params[PARAM_TLG_TYPE].find(basic_type) != string::npos)) {
        string encoding=TFileQueue::getEncoding(FILE_UTG_TYPE, OWN_POINT_ADDR(), true);
        if (encoding.empty()) encoding="CP866";

        TDateTime now_utc = NowUTC();
        double days;
        int msecs = (int)(modf(now_utc, &days) * MSecsPerDay) % 1000;
        ostringstream file_name;
        file_name
            << DateTimeToStr(now_utc, "yyyy_mm_dd_hh_nn_ss_")
            << setw(3) << setfill('0') << msecs
            << "." << setw(9) << setfill('0') << id
            << "." << basic_type
            << "." << BSM::TlgElemIdToElem(etAirline, flt.airline, true)
                   << setw(3) << setfill('0') << flt.flt_no << flt.suffix
            << "." << DateTimeToStr(flt.scd_out, "dd.mm");
        file_params[PARAM_FILE_NAME] = file_name.str();
        TFileQueue::putFile( OWN_POINT_ADDR(),
                             OWN_POINT_ADDR(),
                             FILE_UTG_TYPE,
                             file_params,
                             (encoding == "CP866" ? data : ConvertCodepage(data, "CP866", encoding)));
    }
}

void TelegramInterface::SendTlg(int tlg_id)
{
  try
  {
    TQuery TlgQry(&OraSession);
    TlgQry.Clear();
    TlgQry.SQLText=
      "SELECT id, num, type, point_id, addr AS addrs, heading, body, ending, "
      "       completed, has_errors, time_create, originator_id, airline_mark "
      "FROM tlg_out "
      "WHERE id=:id FOR UPDATE";
    TlgQry.CreateVariable( "id", otInteger, tlg_id);
    TlgQry.Execute();
    if (TlgQry.Eof) throw AstraLocale::UserException("MSG.TLG.NOT_FOUND.REFRESH_DATA");
    if (TlgQry.FieldAsInteger("completed")==0)
      throw AstraLocale::UserException("MSG.TLG.MANUAL_EDIT");
    if (TlgQry.FieldAsInteger("has_errors")==1)
      throw AstraLocale::UserException("MSG.TLG.HAS_ERRORS.UNABLE_SEND");
    int point_id=TlgQry.FieldIsNULL("point_id")?NoExists:TlgQry.FieldAsInteger("point_id");

    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText="SELECT addr, descr FROM typeb_originators WHERE id=:originator_id";
    Qry.CreateVariable( "originator_id", otInteger, TlgQry.FieldAsInteger("originator_id") );
    Qry.Execute();
    if (Qry.Eof) throw AstraLocale::UserException("MSG.TLG.NOT_FOUND.REFRESH_DATA");
    string originator_addr=Qry.FieldAsString("addr");
    string originator_descr=Qry.FieldAsString("descr");

    TTlgStatPoint sender(originator_addr,
                         OWN_CANON_NAME(),
                         OWN_CANON_NAME(),
                         "");

    string tlg_basic_type;
    string tlg_short_name;
    try
    {
      const TTypeBTypesRow& row = (TTypeBTypesRow&)(base_tables.get("typeb_types").get_row("code",TlgQry.FieldAsString("type")));
      tlg_basic_type=row.basic_type;
      tlg_short_name=row.short_name;
    }
    catch(EBaseTableError)
    {
      throw AstraLocale::UserException("MSG.TLG.NOT_FOUND.REFRESH_DATA");
    };

    TTripInfo fltInfo;
    if (point_id!=NoExists)
    {
      Qry.Clear();
      Qry.SQLText="SELECT airline, flt_no, suffix, airp, scd_out FROM points WHERE point_id=:point_id";
      Qry.CreateVariable("point_id", otInteger, point_id);
      Qry.Execute();
      if (Qry.Eof) throw AstraLocale::UserException("MSG.TLG.NOT_FOUND.REFRESH_DATA");
      fltInfo.Init(Qry);
    };

    Qry.Clear();
    Qry.SQLText="SELECT canon_name, country FROM addrs WHERE addr=:addr";
    Qry.DeclareVariable("addr",otString);

    string old_addrs;
    map<string, vector<TTlgStatPoint> > recvs;
    TypeB::TTlgParser tlg;
    char *addrs,*line_p;

    for(;!TlgQry.Eof;TlgQry.Next())
    {
      if (TlgQry.FieldAsString("addrs")!=old_addrs)
      {
        recvs.clear();
        line_p=TlgQry.FieldAsString("addrs");
        try
        {
          do
          {
            addrs=tlg.GetLexeme(line_p);
            while (addrs!=NULL)
            {
              string canon_name,country;
              if (strlen(tlg.lex)!=7)
                throw AstraLocale::UserException("MSG.TLG.INVALID_SITA_ADDR", LParams() << LParam("addr", tlg.lex));
              for(char *p=tlg.lex;*p!=0;p++)
                if (!((IsUpperLetter(*p)&&IsAscii7(*p))||IsDigit(*p)))
                    throw AstraLocale::UserException("MSG.TLG.INVALID_SITA_ADDR", LParams() << LParam("addr", tlg.lex));
              char addr[8];
              strcpy(addr,tlg.lex);
              Qry.SetVariable("addr",addr);
              Qry.Execute();
              if (!Qry.Eof)
              {
                canon_name=Qry.FieldAsString("canon_name");
                country=Qry.FieldAsString("country");
              }
              else
              {
                addr[5]=0; //обрезаем до 5-ти символов
                Qry.SetVariable("addr",addr);
                Qry.Execute();
                if (!Qry.Eof)
                {
                  canon_name=Qry.FieldAsString("canon_name");
                  country=Qry.FieldAsString("country");
                }
                else
                {
                  if (*(DEF_CANON_NAME())!=0)
                  {
                    canon_name=DEF_CANON_NAME();
                    country="";
                  }
                  else
                    throw AstraLocale::UserException("MSG.TLG.SITA.CANON_ADDR_UNDEFINED", LParams() << LParam("addr", tlg.lex));
                };
              };
              recvs[canon_name].push_back(TTlgStatPoint(tlg.lex,
                                                        canon_name,
                                                        canon_name,
                                                        country));
              addrs=tlg.GetLexeme(addrs);
            };
          }
          while ((line_p=tlg.NextLine(line_p))!=NULL);
        }
        catch(TypeB::ETlgError)
        {
          throw AstraLocale::UserException("MSG.WRONG_ADDR_LINE");
        };
        old_addrs=TlgQry.FieldAsString("addrs");
      };
      if (recvs.empty()) throw AstraLocale::UserException("MSG.TLG.DST_ADDRS_NOT_SET");

      //формируем телеграмму
      string tlg_text=(string)TlgQry.FieldAsString("heading")+
                              TlgQry.FieldAsString("body")+
                              TlgQry.FieldAsString("ending");

      for(map<string, vector<TTlgStatPoint> >::const_iterator i=recvs.begin();i!=recvs.end();++i)
      {
        string addrs;
        for(vector<TTlgStatPoint>::const_iterator j=i->second.begin(); j!=i->second.end(); ++j)
        {
          if (!addrs.empty()) addrs+=" ";
          addrs+=j->sita_addr;
        };
      
        addrs=TypeB::format_addr_line(addrs);
      	if (i->first.size()<=5)
      	{
          if (OWN_CANON_NAME()==i->first)
          {
            /* сразу помещаем во входную очередь */
            loadTlg(addrs+tlg_text);
            registerHookAfter(sendCmdTypeBHandler);
          }
          else
          {
            int queue_tlg_id=sendTlg(i->first.c_str(),OWN_CANON_NAME(),qpOutB,0,addrs+tlg_text);
            for(vector<TTlgStatPoint>::const_iterator j=i->second.begin(); j!=i->second.end(); ++j)
            {
              TTlgStat().putTypeBOut(queue_tlg_id,
                                     tlg_id,
                                     TlgQry.FieldAsInteger("num"),
                                     sender,
                                     *j,
                                     TlgQry.FieldAsDateTime("time_create"),
                                     tlg_basic_type,
                                     addrs.size()+tlg_text.size(),
                                     fltInfo,
                                     TlgQry.FieldAsString("airline_mark"),
                                     originator_descr);
            };
            registerHookAfter(sendCmdTlgSnd);
          };
        }
        else
        {
          //это передача файлов
          //string data=addrs+tlg_text; без заголовка
          string data=TlgQry.FieldAsString("body");
          map<string,string> params;
          params[PARAM_CANON_NAME] = i->first;
          params[ NS_PARAM_EVENT_TYPE ] = EncodeEventType( ASTRA::evtTlg );
          params[ NS_PARAM_EVENT_ID1 ] = IntToString( point_id );
          params[ NS_PARAM_EVENT_ID2 ] = IntToString( tlg_id );

          TFileQueue::putFile(i->first,OWN_POINT_ADDR(),TlgQry.FieldAsString("type"),params,data);
        };
        putUTG(tlg_id, tlg_basic_type, fltInfo, addrs + tlg_text);
      };
    };

    TlgQry.Clear();
    TlgQry.SQLText=
      "UPDATE tlg_out SET time_send_act=system.UTCSYSDATE WHERE id=:id";
    TlgQry.CreateVariable( "id", otInteger, tlg_id);
    TlgQry.Execute();
    ostringstream msg;
    msg << "Телеграмма " << tlg_short_name << " (ид=" << tlg_id << ") создана";
    TReqInfo::Instance()->MsgToLog(msg.str(),evtTlg,point_id,tlg_id);
  }
  catch(EOracleError &E)
  {
    if ( E.Code >= 20000 )
    {
      string str = E.what();
      throw AstraLocale::UserException(EOracleError2UserException(str));
    }
    else
      throw;
  };
};

void TelegramInterface::SendTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  SendTlg(NodeAsInteger( "tlg_id", reqNode ));
  AstraLocale::showMessage("MSG.TLG.SEND");
  GetTlgOut(ctxt,reqNode,resNode);
}

void TelegramInterface::DeleteTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int tlg_id = NodeAsInteger( "tlg_id", reqNode );
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText=
        "SELECT typeb_types.basic_type, typeb_types.short_name,point_id FROM tlg_out,typeb_types "
        "WHERE tlg_out.type=typeb_types.code AND id=:id AND num=1 FOR UPDATE";
    Qry.CreateVariable( "id", otInteger, tlg_id);
    Qry.Execute();
    if (Qry.Eof) throw AstraLocale::UserException("MSG.TLG.NOT_FOUND.REFRESH_DATA");
    string tlg_short_name=Qry.FieldAsString("short_name");
    string tlg_basic_type=Qry.FieldAsString("basic_type");
    int point_id=Qry.FieldAsInteger("point_id");

    Qry.Clear();
    Qry.SQLText=
        "DELETE FROM tlg_out WHERE id=:id AND time_send_act IS NULL ";
    Qry.CreateVariable( "id", otInteger, tlg_id);
    Qry.Execute();
    if (Qry.RowsProcessed()>0)
    {
        Qry.SQLText="DELETE FROM typeb_out_extra WHERE tlg_id=:id";
        Qry.Execute();
        ostringstream msg;
        msg << "Телеграмма " << tlg_short_name << " (ид=" << tlg_id << ") удалена";
        TReqInfo::Instance()->MsgToLog(msg.str(),evtTlg,point_id,tlg_id);
        AstraLocale::showMessage("MSG.TLG.DELETED");
    };
    check_tlg_out_alarm(point_id);
    GetTlgOut(ctxt,reqNode,resNode);
};

void TelegramInterface::SendTlg(const vector<TypeB::TCreateInfo> &info)
{
  for(vector<TypeB::TCreateInfo>::const_iterator i=info.begin(); i!=info.end(); ++i)
  {
    try
    {
        int tlg_id=NoExists;
        TTypeBTypesRow tlgTypeInfo;
        localizedstream msg(LANG_RU);;
        try
        {
            time_t time_start=time(NULL);
            tlg_id = create_tlg( *i, tlgTypeInfo );

            time_t time_end=time(NULL);
            if (time_end-time_start>1)
                ProgTrace(TRACE5,"Attention! c++ create_tlg execute time: %ld secs, type=%s, point_id=%d",
                        time_end-time_start,
                        i->get_tlg_type().c_str(),
                        i->point_id);

            msg << "Телеграмма " << tlgTypeInfo.short_name
                << " (ид=" << tlg_id << ") сформирована: ";
        }
        catch(AstraLocale::UserException &E)
        {
            msg << "Ошибка формирования телеграммы "
                << (tlgTypeInfo.short_name.empty()?i->get_tlg_type():tlgTypeInfo.short_name)
                << ": " << getLocaleText(E.getLexemaData()) << ", ";
        }

        TReqInfo::Instance()->MsgToLog(i->get_options().logStr(msg).str(),evtTlg,i->point_id,tlg_id);

        if (tlg_id!=NoExists)
        {
            time_t time_start=time(NULL);
            try
            {
                SendTlg(tlg_id);
            }
            catch(AstraLocale::UserException &E)
            {
                msg.str("");
                msg << "Ошибка отправки телеграммы "
                    << (tlgTypeInfo.short_name.empty()?i->get_tlg_type():tlgTypeInfo.short_name)
                    << " (ид=" << tlg_id << ")"
                    << ": " << getLocaleText(E.getLexemaData());
                TReqInfo::Instance()->MsgToLog(msg.str(),evtTlg,i->point_id,tlg_id);
            };
            time_t time_end=time(NULL);
            if (time_end-time_start>1)
                ProgTrace(TRACE5,"Attention! SendTlg execute time: %ld secs, tlg_id=%d",
                        time_end-time_start,tlg_id);
        };
    }
    catch( Exception &E )
    {
      ProgError(STDLOG,"SendTlg (point_id=%d, type=%s): %s",i->point_id,i->get_tlg_type().c_str(),E.what());
    }
    catch(...)
    {
      ProgError(STDLOG,"SendTlg (point_id=%d, type=%s): unknown error",i->point_id,i->get_tlg_type().c_str());
    };
  };
};

namespace BSM
{

bool TTlgContent::addTag(double no, const TTlgContent& src)
{
  map<double, CheckIn::TTagItem>::const_iterator srcTag=src.tags.find(no);
  if (srcTag==src.tags.end()) return false;
  map<int, CheckIn::TBagItem>::const_iterator srcBag=src.bags.find(srcTag->second.bag_num);
  if (srcBag==src.bags.end()) return false;
  map<int, TPaxItem>::const_iterator srcPax=src.pax.find(srcBag->second.bag_pool_num);
  if (srcPax==src.pax.end()) return false;

  map<int, TPaxItem>::iterator destPax=pax.find(srcPax->second.bag_pool_num);
  if (destPax==pax.end())
  {
    TPaxItem tmpPax=srcPax->second;
    tmpPax.bag_amount=0;
    tmpPax.bag_weight=0;
      
    pax[tmpPax.bag_pool_num]=tmpPax;
  };

  map<int, CheckIn::TBagItem>::const_iterator destBag=bags.find(srcBag->second.num);
  if (destBag==bags.end())
  {
    if (!addBag(srcBag->second)) return false;
  };

  map<double, CheckIn::TTagItem>::const_iterator destTag=tags.find(srcTag->second.no);
  if (destTag==tags.end())
  {
    if (!addTag(srcTag->second)) return false;
  };
  return true;
};

bool TTlgContent::addTag(const CheckIn::TTagItem &tag)
{
  if (tag.bag_num==ASTRA::NoExists ||
      bags.find(tag.bag_num)==bags.end()) return false;
  tags[tag.no]=tag;
  return true;
};

bool TTlgContent::addBag(const CheckIn::TBagItem &bag)
{
  if (bag.bag_pool_num==ASTRA::NoExists) return false;
  std::map<int, TPaxItem>::iterator iPax=pax.find(bag.bag_pool_num);
  if (iPax==pax.end()) return false;
  if (!bag.pr_cabin)
  {
    iPax->second.bag_amount+=bag.amount;
    iPax->second.bag_weight+=bag.weight;
  }
  else
  {
    iPax->second.rk_weight+=bag.weight;
  };
  bags[bag.num]=bag;
  return true;
};

void LoadContent(int grp_id, TTlgContent& con)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT points.point_id, points.point_num, points.first_point, points.pr_tranzit, "
    "       NVL(trip_sets.pr_lat_seat,1) AS pr_lat_seat, pax_grp.class "
    "FROM points,pax_grp,trip_sets "
    "WHERE points.point_id=pax_grp.point_dep AND points.pr_del>=0 AND "
    "      points.point_id=trip_sets.point_id(+) AND "
    "      grp_id=:grp_id AND bag_refuse=0";
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.Execute();
  if (Qry.Eof) return;
  con.pr_lat_seat=Qry.FieldAsInteger("pr_lat_seat")!=0;

  int point_id=Qry.FieldAsInteger("point_id");
  int point_num=Qry.FieldAsInteger("point_num");
  int first_point=Qry.FieldIsNULL("first_point")?NoExists:Qry.FieldAsInteger("first_point");
  bool pr_tranzit=Qry.FieldAsInteger("pr_tranzit")!=0;

  bool pr_unaccomp=Qry.FieldIsNULL("class");
  
  //читаем OutFlt и OnwardFlt
  if (!con.OnwardFlt.GetRoute(grp_id, trtWithFirstSeg)) return;
  if (con.OnwardFlt.empty()) return;
  con.OutFlt=*con.OnwardFlt.begin();
  con.OnwardFlt.erase(con.OnwardFlt.begin());

  if (!pr_unaccomp)
  {
    vector<TTlgCompLayer> complayers;
    ReadSalons( point_id, point_num, first_point, pr_tranzit, complayers );
  
    Qry.Clear();
    Qry.SQLText=
      "SELECT pax_id, bag_pool_num, reg_no, surname, name,  "
      "       DECODE(pr_brd,NULL,'N',0,'C','B') AS status "
      "FROM pax "
      "WHERE grp_id=:grp_id AND bag_pool_num IS NOT NULL AND "
      "      pax_id=ckin.get_bag_pool_pax_id(grp_id,bag_pool_num,0)";
    Qry.CreateVariable("grp_id",otInteger,grp_id);
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
    {
      TPaxItem pax;
      int pax_id=Qry.FieldAsInteger("pax_id");
      pax.reg_no=Qry.FieldAsInteger("reg_no");
      pax.surname=Qry.FieldAsString("surname");
      pax.name=Qry.FieldAsString("name");
      pax.status=Qry.FieldAsString("status");
      pax.seat_no.add_seats(pax_id, complayers);
      vector<TPnrAddrItem> pnrs;
      pax.pnr_addr=GetPaxPnrAddr(pax_id,pnrs);
      pax.bag_pool_num=Qry.FieldAsInteger("bag_pool_num");
      con.pax[pax.bag_pool_num]=pax;
    };
  }
  else
  {
    TPaxItem pax;
    pax.surname="UNACCOMPANIED";
    pax.bag_pool_num=1;
    con.pax[pax.bag_pool_num]=pax;
  };
  
  Qry.Clear();
  Qry.SQLText=
    "SELECT * FROM bag2 WHERE grp_id=:grp_id";
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
    con.addBag(CheckIn::TBagItem().fromDB(Qry));
  
  Qry.Clear();
  Qry.SQLText=
    "SELECT * FROM bag_tags WHERE grp_id=:grp_id";
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
    con.addTag(CheckIn::TTagItem().fromDB(Qry));
};

void CompareContent(const TTlgContent& con1, const TTlgContent& con2, vector<TTlgContent>& bsms)
{
  bsms.clear();

  TTlgContent conADD,conCHG,conDEL;
  conADD.indicator=TypeB::None;
  conADD.OutFlt=con2.OutFlt;
  conADD.OnwardFlt=con2.OnwardFlt;
  conADD.pr_lat_seat=con2.pr_lat_seat;

  conCHG.indicator=TypeB::CHG;
  conCHG.OutFlt=con2.OutFlt;
  conCHG.OnwardFlt=con2.OnwardFlt;
  conCHG.pr_lat_seat=con2.pr_lat_seat;

  conDEL.indicator=TypeB::DEL;
  conDEL.OutFlt=con1.OutFlt;
  conDEL.OnwardFlt=con1.OnwardFlt;
  conDEL.pr_lat_seat=con1.pr_lat_seat;


  //проверяем рейс
  if (con1.OutFlt.operFlt.airline==con2.OutFlt.operFlt.airline &&
      con1.OutFlt.operFlt.flt_no==con2.OutFlt.operFlt.flt_no &&
      con1.OutFlt.operFlt.suffix==con2.OutFlt.operFlt.suffix &&
      con1.OutFlt.operFlt.scd_out==con2.OutFlt.operFlt.scd_out &&
      con1.OutFlt.operFlt.airp==con2.OutFlt.operFlt.airp)
  {
    bool pr_chd=!(con1.OutFlt.airp_arv==con2.OutFlt.airp_arv);
    if (!pr_chd)
    {
      //придется проверить изменения в стыковочных рейсах
      TTrferRoute::const_iterator i1=con1.OnwardFlt.begin();
      TTrferRoute::const_iterator i2=con2.OnwardFlt.begin();
      for(;i1!=con1.OnwardFlt.end()&&i2!=con2.OnwardFlt.end();++i1,++i2)
      {
        if (!(i1->operFlt.airline==i2->operFlt.airline &&
              i1->operFlt.flt_no==i2->operFlt.flt_no &&
              i1->operFlt.suffix==i2->operFlt.suffix &&
              i1->operFlt.scd_out==i2->operFlt.scd_out &&
              i1->operFlt.airp==i2->operFlt.airp &&
              i1->airp_arv==i2->airp_arv)) break;
      };
      pr_chd= i1!=con1.OnwardFlt.end() || i2!=con2.OnwardFlt.end();
    };

    map<double, CheckIn::TTagItem>::const_iterator i1=con1.tags.begin();
    map<double, CheckIn::TTagItem>::const_iterator i2=con2.tags.begin();
    int res;
    for(;i1!=con1.tags.end() || i2!=con2.tags.end();)
    {
      res=0;
      if (i1==con1.tags.end() ||
          (i2!=con2.tags.end() && i1->first>i2->first)) res=-1;
      if (i2==con2.tags.end() ||
          (i1!=con1.tags.end() && i1->first<i2->first)) res=1;

      if (res>0) conDEL.addTag(i1->first, con1);

      if (res<0) conADD.addTag(i2->first, con2);

      if (res==0)
      {
        if (!pr_chd)
        {
          std::map<int, CheckIn::TBagItem>::const_iterator bag1=con1.bags.find(i1->second.bag_num);
          std::map<int, CheckIn::TBagItem>::const_iterator bag2=con2.bags.find(i2->second.bag_num);
          if (bag1!=con1.bags.end() && bag2!=con2.bags.end())
          {
            std::map<int, TPaxItem>::const_iterator pax1=con1.pax.find(bag1->second.bag_pool_num);
            std::map<int, TPaxItem>::const_iterator pax2=con2.pax.find(bag2->second.bag_pool_num);
            if (pax1!=con1.pax.end() && pax2!=con2.pax.end())
            {
              if (pax1->second.surname != pax2->second.surname ||
                  pax1->second.name != pax2->second.name ||
                  pax1->second.status != pax2->second.status ||
                  pax1->second.pnr_addr != pax2->second.pnr_addr ||
                  pax1->second.seat_no.get_seat_one(con1.pr_lat_seat) !=
                  pax2->second.seat_no.get_seat_one(con2.pr_lat_seat) ||
                  pax1->second.reg_no != pax2->second.reg_no ||
                  bag1->second.amount != bag2->second.amount ||
                  bag1->second.weight != bag2->second.weight)
              {
                conCHG.addTag(i2->first, con2);
              };
            };
          };
        }
        else conCHG.addTag(i2->first, con2);
      };
      if (res>=0) ++i1;
      if (res<=0) ++i2;
    };
  }
  else
  {
    conDEL.tags=con1.tags;
    conDEL.bags=con1.bags;
    conDEL.pax=con1.pax;
    conADD.tags=con2.tags;
    conADD.bags=con2.bags;
    conADD.pax=con2.pax;
  };

  if (!conADD.tags.empty()) bsms.push_back(conADD);
  if (!conCHG.tags.empty()) bsms.push_back(conCHG);
  if (!conDEL.tags.empty()) bsms.push_back(conDEL);
};

std::string TlgElemIdToElem(TElemType type, int id, bool pr_lat)
{
    TElemFmt fmt=prLatToElemFmt(efmtCodeNative, pr_lat);
    try {
        return TypeB::TlgElemIdToElem(type, id, fmt, AstraLocale::LANG_RU);
    } catch(UserException &E) {
        ProgTrace(TRACE5, "BSM::TlgElemIdToElem: elem_type: %s, fmt: %s, what: %s", EncodeElemType(type), EncodeElemFmt(fmt), E.what());
        return "";
    } catch(exception &E) {
        ProgError(STDLOG, "BSM::TlgElemIdToElem: elem_type: %s, fmt: %s, what: %s", EncodeElemType(type), EncodeElemFmt(fmt), E.what());
        return "";
    } catch(...) {
        ProgError(STDLOG, "BSM::TlgElemIdToElem: unknown except caught. elem_type: %s, fmt: %s", EncodeElemType(type), EncodeElemFmt(fmt));
        return "";
    }
};

std::string TlgElemIdToElem(TElemType type, std::string id, bool pr_lat)
{
    TElemFmt fmt=prLatToElemFmt(efmtCodeNative, pr_lat);
    try {
        return TypeB::TlgElemIdToElem(type, id, fmt, AstraLocale::LANG_RU);
    } catch(UserException &E) {
        ProgTrace(TRACE5, "BSM::TlgElemIdToElem: elem_type: %s, fmt: %s, what: %s", EncodeElemType(type), EncodeElemFmt(fmt), E.what());
        return "";
    } catch(exception &E) {
        ProgError(STDLOG, "BSM::TlgElemIdToElem: elem_type: %s, fmt: %s, what: %s", EncodeElemType(type), EncodeElemFmt(fmt), E.what());
        return "";
    } catch(...) {
        ProgError(STDLOG, "BSM::TlgElemIdToElem: unknown except caught. elem_type: %s, fmt: %s", EncodeElemType(type), EncodeElemFmt(fmt));
        return "";
    }
};

string CreateTlgBody(const TTlgContent& con, bool pr_lat)
{
  map<int/*reg_no*/, pair<TPaxItem, vector<CheckIn::TTagItem> > > tmpPax;
  for(map<double, CheckIn::TTagItem>::const_iterator iTag=con.tags.begin();iTag!=con.tags.end();++iTag)
  {
    map<int, CheckIn::TBagItem>::const_iterator iBag=con.bags.find(iTag->second.bag_num);
    if (iBag==con.bags.end()) continue;
    map<int, TPaxItem>::const_iterator iPax=con.pax.find(iBag->second.bag_pool_num);
    if (iPax==con.pax.end()) continue;
    
    map<int, pair<TPaxItem, vector<CheckIn::TTagItem> > >::iterator p=tmpPax.find(iPax->second.reg_no);
    if (p==tmpPax.end())
      tmpPax[iPax->second.reg_no]=make_pair(iPax->second, vector<CheckIn::TTagItem>(1,iTag->second));
    else
      p->second.second.push_back(iTag->second);
  };
  if (tmpPax.empty()) throw Exception("BSM::CreateTlgBody: tmpPax empty");
  

  ostringstream body;

  body.setf(ios::fixed);

  body << "BSM" << ENDL;

  switch(con.indicator)
  {
    case TypeB::CHG: body << "CHG" << ENDL;
                     break;
    case TypeB::DEL: body << "DEL" << ENDL;
                     break;
     default: ;
  };

  body << ".V/1L" << TlgElemIdToElem(etAirp, con.OutFlt.operFlt.airp, pr_lat) << ENDL;

  TDateTime scd_out;
  if(con.OutFlt.operFlt.airp == "АЯТ")
      scd_out = con.OutFlt.operFlt.real_out;
  else
      scd_out = con.OutFlt.operFlt.scd_out;

  body << ".F/"
       << TlgElemIdToElem(etAirline, con.OutFlt.operFlt.airline, pr_lat)
       << setw(3) << setfill('0') << con.OutFlt.operFlt.flt_no
       << (con.OutFlt.operFlt.suffix.empty() ? "" : TlgElemIdToElem(etSuffix, con.OutFlt.operFlt.suffix, pr_lat)) << '/'
       << DateTimeToStr( scd_out, "ddmmm", pr_lat) << '/'
       << TlgElemIdToElem(etAirp, con.OutFlt.airp_arv, pr_lat) << ENDL;

  for(TTrferRoute::const_iterator i=con.OnwardFlt.begin();i!=con.OnwardFlt.end();++i)
  {
    body << ".O/"
         << TlgElemIdToElem(etAirline, i->operFlt.airline, pr_lat)
         << setw(3) << setfill('0') << i->operFlt.flt_no
         << (i->operFlt.suffix.empty() ? "" : TlgElemIdToElem(etSuffix, i->operFlt.suffix, pr_lat)) << '/'
         << DateTimeToStr( i->operFlt.scd_out, "ddmmm", pr_lat) << '/'
         << TlgElemIdToElem(etAirp, i->airp_arv, pr_lat) << ENDL;
  };
  
  map<int, pair<TPaxItem, vector<CheckIn::TTagItem> > >::const_iterator p=tmpPax.begin();
  for(;p!=tmpPax.end();++p)
  {
    const vector<CheckIn::TTagItem> &tmpTags=p->second.second;
    if (tmpTags.empty()) throw Exception("BSM::CreateTlgBody: tmpTags empty");
    double first_no;
    int num;
    vector<CheckIn::TTagItem>::const_iterator i=tmpTags.begin();
    while(true)
    {
      if (i!=tmpTags.begin() &&
          (i==tmpTags.end() || i->no!=first_no+num))
      {
        body << ".N/"
             << setw(10) << setfill('0') << setprecision(0) << first_no
             << setw(3) << setfill('0') << num << ENDL;
      };
      if (i==tmpTags.end()) break;
      if (i==tmpTags.begin() || i->no!=first_no+num)
      {
        first_no=i->no;
        num=1;
      }
      else num++;
      ++i;
    };
    body << ".S/"
         << (con.indicator==TypeB::DEL?'N':'Y');
    if (p->second.first.reg_no!=ASTRA::NoExists)
      body << '/'
           << p->second.first.seat_no.get_seat_one(con.pr_lat_seat || pr_lat) << '/'
           << p->second.first.status << '/'
           << setw(3) << setfill('0') << p->second.first.reg_no;
    body << ENDL;

    body << ".W/K/" << p->second.first.bag_amount << '/' << p->second.first.bag_weight; //всегда пишем
/*    if (p->second.first.rk_weight!=0)
      body << '/' << p->second.first.rk_weight;*/
    body << ENDL;
    
    body << ".P/" << transliter(p->second.first.surname,1,pr_lat);
    if (!p->second.first.name.empty())
      body << '/' << transliter(p->second.first.name,1,pr_lat);
    body  << ENDL;
    
    if (!p->second.first.pnr_addr.empty())
      body << ".L/" << convert_pnr_addr(p->second.first.pnr_addr,pr_lat) << ENDL;
  };

  body << "ENDBSM" << ENDL;

  return body.str();
};

bool IsSend( const TAdvTripInfo &fltInfo, TBSMAddrs &addrs )
{
    TypeB::TCreator creator(fltInfo);
    creator << "BSM";
    creator.getInfo(addrs.createInfo);

    TFileQueue::add_sets_params(
                                 fltInfo.airp,
                                 fltInfo.airline,
                                 IntToString(fltInfo.flt_no),
                                 OWN_POINT_ADDR(),
                                 FILE_HTTP_TYPEB_TYPE,
                                 1,
                                 addrs.HTTP_TYPEBparams );

    return !addrs.empty();
};

void Send( int point_dep, int grp_id, const TTlgContent &con1, const TBSMAddrs &addrs )
{
    TTlgContent con2;
    LoadContent(grp_id,con2);
    vector<TTlgContent> bsms;
    CompareContent(con1,con2,bsms);
    TTlgOutPartInfo p;
    p.tlg_type="BSM";
    p.point_id=point_dep;
    p.time_create=NowUTC();

    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText="UPDATE tlg_out SET completed=1 WHERE id=:id";
    Qry.DeclareVariable("id",otInteger);

    for(vector<TTlgContent>::iterator i=bsms.begin();i!=bsms.end();++i)
    {
      if(not addrs.empty()) {
          TypeB::TOriginatorInfo originator=TypeB::getOriginator(i->OutFlt.operFlt.airline,
                                                                 i->OutFlt.operFlt.airp,
                                                                 p.tlg_type, p.time_create, true);
          p.originator_id=originator.id;
          ostringstream heading;
          heading << '.' << originator.addr
                  << ' ' << DateTimeToStr(p.time_create,"ddhhnn") << ENDL;
          p.heading=heading.str();
      }

      for(vector<TypeB::TCreateInfo>::const_iterator j=addrs.createInfo.begin();j!=addrs.createInfo.end();++j)
      {
        p.id=NoExists;
        p.num=1;
        p.pr_lat=j->get_options().is_lat;
        p.addr=TypeB::format_addr_line(j->get_addrs());
        p.body=CreateTlgBody(*i,p.pr_lat);
        TelegramInterface::SaveTlgOutPart(p);
        Qry.SetVariable("id",p.id);
        Qry.Execute();
        TelegramInterface::SendTlg(p.id);
      };
      if(not addrs.HTTP_TYPEBparams.empty()) {
          map<string, string> params = addrs.HTTP_TYPEBparams;

          p.addToFileParams(params);

          TFileQueue::putFile(OWN_POINT_ADDR(),
                              OWN_POINT_ADDR(),
                              FILE_HTTP_TYPEB_TYPE,
                              params,
                              CreateTlgBody(*i, true));
      }
    };
    if(not addrs.HTTP_TYPEBparams.empty())
        registerHookAfter(sendCmdTlgHttpSnd);

    check_tlg_out_alarm(point_dep);
};

};

void TelegramInterface::SaveTlgOutPart( TTlgOutPartInfo &info )
{
  TQuery Qry(&OraSession);

  if (info.id==NoExists)
  {
    Qry.Clear();
    Qry.SQLText=
      "SELECT tlg_in_out__seq.nextval AS id FROM dual";
    Qry.Execute();
    if (Qry.Eof) return;
    info.id=Qry.FieldAsInteger("id");
  };

  Qry.Clear();
  Qry.SQLText=
    "INSERT INTO tlg_out(id,num,type,point_id,addr,heading,body,ending,pr_lat, "
    "                    completed,has_errors,time_create,time_send_scd,time_send_act,originator_id,airline_mark) "
    "VALUES(:id,:num,:type,:point_id,:addr,:heading,:body,:ending,:pr_lat, "
    "       0,0,NVL(:time_create,system.UTCSYSDATE),:time_send_scd,NULL,:originator_id,:airline_mark)";

  /*
  ProgTrace(TRACE5, "-------SaveTlgOutPart--------");
  ProgTrace(TRACE5, "id: %d", info.id);
  ProgTrace(TRACE5, "num: %d", info.num);
  ProgTrace(TRACE5, "point_id: %d", info.point_id);
  ProgTrace(TRACE5, "addr: %s", info.addr.c_str());
  ProgTrace(TRACE5, "heading: %s", info.heading.c_str());
  ProgTrace(TRACE5, "body: %s, size: %zu", info.body.c_str(), info.body.size());
  ProgTrace(TRACE5, "ending: %s", info.ending.c_str());
  //ProgTrace(TRACE5, "extra: %s", info.extra.c_str());
  */

  Qry.CreateVariable("id",otInteger,info.id);
  Qry.CreateVariable("num",otInteger,info.num);
  Qry.CreateVariable("type",otString,info.tlg_type);
  if (info.point_id!=NoExists)
    Qry.CreateVariable("point_id",otInteger,info.point_id);
  else
    Qry.CreateVariable("point_id",otInteger,FNull);
  Qry.CreateVariable("addr",otString,info.addr);
  Qry.CreateVariable("heading",otString,info.heading);
  Qry.CreateVariable("body",otString,info.body);
  Qry.CreateVariable("ending",otString,info.ending);
  Qry.CreateVariable("pr_lat",otInteger,(int)info.pr_lat);
  if (info.time_create!=NoExists)
    Qry.CreateVariable("time_create",otDate,info.time_create);
  else
    Qry.CreateVariable("time_create",otDate,FNull);
  if (info.time_send_scd!=NoExists)
    Qry.CreateVariable("time_send_scd",otDate,info.time_send_scd);
  else
    Qry.CreateVariable("time_send_scd",otDate,FNull);
  if (info.originator_id!=NoExists)
    Qry.CreateVariable("originator_id",otInteger,info.originator_id);
  else
    throw Exception("SaveTlgOutPart: info.originator_id=NoExists");
  Qry.CreateVariable("airline_mark",otString,info.airline_mark);
  Qry.Execute();

  if (info.num==1)
  {
    Qry.Clear();
    Qry.SQLText=
      "INSERT INTO typeb_out_extra(tlg_id, lang, text) "
      "VALUES(:tlg_id, :lang, :text)";
    Qry.CreateVariable("tlg_id", otInteger, info.id);
    Qry.DeclareVariable("lang", otString);
    Qry.DeclareVariable("text", otString);
    for(map<string,string>::const_iterator e=info.extra.begin(); e!=info.extra.end(); ++e)
    {
      if (e->second.empty()) continue;
      Qry.SetVariable("lang", e->first);
      Qry.SetVariable("text", e->second.substr(0,250));
      Qry.Execute();
    };
  };

  info.num++;
};

void TelegramInterface::TestSeatRanges(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  vector<TSeatRange> ranges;
  try
  {
    TypeB::ParseSeatRange(NodeAsString("lexeme",reqNode),ranges,true);

    xmlNodePtr rangesNode,rangeNode;
    rangesNode=NewTextChild(resNode,"ranges");

    sort(ranges.begin(),ranges.end());

    if (ranges.begin()!=ranges.end())
    {
      TSeatRange range=*(ranges.begin());
      TSeat seat=range.first;
      do
      {
        TSeatRange range2;
        range2.first=seat;
        range2.second=seat;
        ranges.push_back(range2);
      }
      while (NextSeatInRange(range,seat));
    };

    for(vector<TSeatRange>::iterator i=ranges.begin();i!=ranges.end();i++)
    {
      rangeNode=NewTextChild(rangesNode,"range");
      NewTextChild(rangeNode,"first_row",i->first.row);
      NewTextChild(rangeNode,"first_line",i->first.line);
      NewTextChild(rangeNode,"second_row",i->second.row);
      NewTextChild(rangeNode,"second_line",i->second.line);
    };
  }
  catch(Exception &e)
  {
    throw UserException(e.what());
  };
};

void send_tlg_help(const char *name)
{
  printf("  %-15.15s ", name);
  puts("<tlg_id>");
};

int send_tlg(int argc,char **argv)
{
    try {
        if(argc != 2)
            throw Exception("wrong arg count");
        int tlg_id;
        if(StrToInt(argv[1], tlg_id) == EOF)
            throw Exception("tlg_id must be number");
        TelegramInterface::SendTlg(tlg_id);
    } catch (Exception &E) {
        printf("Error: %s\n", E.what());
        puts("Usage:");
        send_tlg_help(argv[0]);
        puts("Example:");
        printf("  %s 1234\n",argv[0]);
        return 1;
    }
    return 0;
}

void TTlgStat::putTypeBOut(const int queue_tlg_id,
                           const int tlg_id,
                           const int tlg_num,
                           const TTlgStatPoint &sender,
                           const TTlgStatPoint &receiver,
                           const BASIC::TDateTime time_create,
                           const std::string &tlg_type,
                           const int tlg_len,
                           const TTripInfo &fltInfo,
                           const std::string &airline_mark,
                           const std::string &extra)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "INSERT INTO tlg_stat(queue_tlg_id, tlg_id, tlg_num, "
    "  sender_sita_addr, sender_canon_name, sender_descr, sender_country, "
    "  receiver_sita_addr, receiver_canon_name, receiver_descr, receiver_country, "
    "  time_create, time_send, time_receive, tlg_type, tlg_len, "
    "  airline, flt_no, suffix, scd_local_date, airp_dep, airline_mark, extra) "
    "VALUES(:queue_tlg_id, :tlg_id, :tlg_num, "
    "  :sender_sita_addr, :sender_canon_name, :sender_descr, :sender_country, "
    "  :receiver_sita_addr, :receiver_canon_name, :receiver_descr, :receiver_country, "
    "  :time_create, NULL, NULL, :tlg_type, :tlg_len, "
    "  :airline, :flt_no, :suffix, :scd_local_date, :airp_dep, :airline_mark, :extra) ";
  Qry.CreateVariable("queue_tlg_id", otInteger, queue_tlg_id);
  Qry.CreateVariable("tlg_id", otInteger, tlg_id);
  Qry.CreateVariable("tlg_num", otInteger, tlg_num);
  Qry.CreateVariable("sender_sita_addr", otString, sender.sita_addr);
  Qry.CreateVariable("sender_canon_name", otString, sender.canon_name);
  Qry.CreateVariable("sender_descr", otString, sender.descr);
  Qry.CreateVariable("sender_country", otString, sender.country);
  Qry.CreateVariable("receiver_sita_addr", otString, receiver.sita_addr);
  Qry.CreateVariable("receiver_canon_name", otString, receiver.canon_name);
  Qry.CreateVariable("receiver_descr", otString, receiver.descr);
  Qry.CreateVariable("receiver_country", otString, receiver.country);
  Qry.CreateVariable("time_create", otDate, time_create);
  Qry.CreateVariable("tlg_type", otString, tlg_type);
  Qry.CreateVariable("tlg_len", otInteger, tlg_len);
  if (!fltInfo.airline.empty())
  {
    TDateTime scd_local_date=UTCToLocal(fltInfo.scd_out, AirpTZRegion(fltInfo.airp));
    modf(scd_local_date,&scd_local_date);
    Qry.CreateVariable("airline", otString, fltInfo.airline);
    Qry.CreateVariable("flt_no", otInteger, fltInfo.flt_no);
    Qry.CreateVariable("suffix", otString, fltInfo.suffix);
    Qry.CreateVariable("scd_local_date", otDate, scd_local_date);
    Qry.CreateVariable("airp_dep", otString, fltInfo.airp);
  }
  else
  {
    Qry.CreateVariable("airline", otString, FNull);
    Qry.CreateVariable("flt_no", otInteger, FNull);
    Qry.CreateVariable("suffix", otString, FNull);
    Qry.CreateVariable("scd_local_date", otDate, FNull);
    Qry.CreateVariable("airp_dep", otString, FNull);
  };
  Qry.CreateVariable("airline_mark", otString, airline_mark);
  Qry.CreateVariable("extra", otString, extra);
  Qry.Execute();
};

