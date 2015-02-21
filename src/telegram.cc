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
#include "alarms.h"
#include "salons.h"
#include "qrys.h"
#include "serverlib/logger.h"
#include "serverlib/posthooks.h"
#include "TypeBHelpMng.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

#define ENDL "\015\012"

using namespace std;
using namespace ASTRA;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace boost::local_time;

TTlgOutPartInfo::TTlgOutPartInfo (const TypeB::TDetailCreateInfo &info)
{
  id=NoExists;
  num = 1;
  tlg_type = info.get_tlg_type();
  point_id = info.point_id;
  pr_lat = info.get_options().is_lat;
  for(int i=0; i<=1; i++)
  {
    string lang=(i==0?LANG_RU:LANG_EN);
    localizedstream s(lang);
    extra[lang]=info.get_options().extraStr(s).str();
  };
  addr = info.addrs;
  time_create = info.time_create;
  time_send_scd = NoExists;
  originator_id = info.originator.id;
  airline_mark = info.airline_mark();
  manual_creation = info.manual_creation;
};
void TTlgOutPartInfo::addToFileParams(map<string, string> &params) const
{
  params[FILE_PARAM_POINT_ID] = point_id==NoExists?"":IntToString(point_id);
  params[FILE_PARAM_TLG_TYPE] = tlg_type;
  params[FILE_PARAM_ORIGIN] = origin;
  params[FILE_PARAM_HEADING] = heading;
  params[FILE_PARAM_ENDING] = ending;
  params[FILE_PARAM_PR_LAT] = IntToString((int)pr_lat);
  params[FILE_PARAM_TIME_CREATE] = time_create==NoExists?"":DateTimeToStr(time_create, ServerFormatDateTimeAsString);
  params[FILE_PARAM_ORIGINATOR_ID] = originator_id==NoExists?"":IntToString(originator_id);
  params[FILE_PARAM_AIRLINE_MARK] = airline_mark;
  params[FILE_PARAM_MANUAL_CREATION] = IntToString((int)manual_creation);
  for(map<string/*lang*/, string>::const_iterator i=extra.begin(); i!=extra.end(); ++i)
    params[FILE_PARAM_EXTRA+i->first] = i->second;
};
void TTlgOutPartInfo::addFromFileParams(const map<string, string> &params)
{
  clear();

  map<string, string>::const_iterator p;
  p=params.find(FILE_PARAM_POINT_ID);
  if (p!=params.end())
    point_id = p->second.empty()?NoExists:ToInt(p->second);

  p=params.find(FILE_PARAM_TLG_TYPE);
  if (p!=params.end())
    tlg_type = p->second;

  p=params.find(FILE_PARAM_ORIGIN);
  if (p!=params.end())
    origin = p->second;

  p=params.find(FILE_PARAM_HEADING);
  if (p!=params.end())
    heading = p->second;

  p=params.find(FILE_PARAM_ENDING);
  if (p!=params.end())
    ending = p->second;

  p=params.find(FILE_PARAM_PR_LAT);
  if (p!=params.end())
    pr_lat = ToInt(p->second)!=0;

  p=params.find(FILE_PARAM_TIME_CREATE);
  if (p!=params.end())
    p->second.empty()?time_create=NoExists:
                      StrToDateTime(p->second.c_str(), ServerFormatDateTimeAsString, time_create);

  p=params.find(FILE_PARAM_ORIGINATOR_ID);
  if (p!=params.end())
    originator_id = p->second.empty()?NoExists:ToInt(p->second);

  p=params.find(FILE_PARAM_AIRLINE_MARK);
  if (p!=params.end())
    airline_mark = p->second;

  p=params.find(FILE_PARAM_MANUAL_CREATION);
  if (p!=params.end())
    manual_creation = ToInt(p->second)!=0;

  for(int i=0; i<=1; i++)
  {
    string lang=(i==0?LANG_RU:LANG_EN);
    p=params.find(FILE_PARAM_EXTRA+lang);
    if (p!=params.end())
      extra[lang] = p->second;
    else
      extra[lang] = "";
  };
};

void TTlgOutPartInfo::getExtra()
{
    extra.clear();
    QParams QryParams;
    QryParams << QParam("tlg_id", otInteger, id);
    TCachedQuery Qry("select * from typeb_out_extra where tlg_id = :tlg_id", QryParams);
    Qry.get().Execute();
    for(; !Qry.get().Eof; Qry.get().Next())
        extra.insert(make_pair(Qry.get().FieldAsString("lang"), Qry.get().FieldAsString("text")));
}

TTlgOutPartInfo& TTlgOutPartInfo::fromDB(TQuery &Qry)
{
  clear();
  id=Qry.FieldAsInteger("id");
  num=Qry.FieldAsInteger("num");
  point_id=Qry.FieldIsNULL("point_id")?NoExists:Qry.FieldAsInteger("point_id");
  tlg_type=Qry.FieldAsString("type");
  addr=Qry.FieldAsString("addr");
  origin=Qry.FieldAsString("origin");
  heading=Qry.FieldAsString("heading");
  body=Qry.FieldAsString("body");
  ending=Qry.FieldAsString("ending");
  pr_lat=Qry.FieldAsInteger("pr_lat")!=0;
  time_create=Qry.FieldAsDateTime("time_create");
  time_send_scd=Qry.FieldIsNULL("time_send_scd")?NoExists:Qry.FieldAsDateTime("time_send_scd");
  originator_id=Qry.FieldAsInteger("originator_id");
  airline_mark=Qry.FieldAsString("airline_mark");
  manual_creation=Qry.FieldAsInteger("manual_creation")!=0;
  getExtra();
  return *this;
};

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
    double tlg_num;
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
    set<int> typeb_in_ids; //не передается с клиента, вспомогательное
    set<int> tlgs_ids; //не передается с клиента, вспомогательное
    void get(xmlNodePtr reqNode);
    void dump() const;
    TTlgSearchParams():
        err_cls(0),
        tlg_id(NoExists),
        tlg_num(NoExists),
        flt_no(NoExists),
        pr_time_create(false),
        pr_time_receive(false),
        TimeCreateFrom(0),
        TimeCreateTo(0),
        TimeReceiveFrom(0),
        TimeReceiveTo(0)
    {};
};

void TTlgSearchParams::dump() const
{
    ProgTrace(TRACE5, "err_cls: %d", err_cls);
    if (tlg_id!=NoExists)
        ProgTrace(TRACE5, "tlg_id: %d", tlg_id);
    else
        ProgTrace(TRACE5, "tlg_id: NoExists");
    if (tlg_num!=NoExists)
        ProgTrace(TRACE5, "tlg_num: %-15.0f", tlg_num);
    else
        ProgTrace(TRACE5, "tlg_num: NoExists");
    ProgTrace(TRACE5, "tlg_type: '%s'", tlg_type.c_str());
    ProgTrace(TRACE5, "airline: '%s'", airline.c_str());
    ProgTrace(TRACE5, "airp: '%s'", airp.c_str());
    if (flt_no!=NoExists)
        ProgTrace(TRACE5, "flt_no: %d", flt_no);
    else
        ProgTrace(TRACE5, "flt_no: NoExists");
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
    tlg_num = NodeAsFloatFast("tlg_num", currNode, NoExists);
    tlg_type = NodeAsStringFast("tlg_type", currNode, "");
    airline = NodeAsStringFast("airline", currNode, "");
    airp = NodeAsStringFast("airp", currNode, "");
    flt_no = NodeAsIntegerFast("flt_no", currNode, NoExists);
    suffix = NodeAsStringFast("suffix", currNode, "");
    pr_time_create = NodeAsIntegerFast("pr_time_create", currNode, 0) != 0;
    pr_time_receive = NodeAsIntegerFast("pr_time_receive", currNode, 0) != 0;
    if (!pr_time_create && !pr_time_receive && tlg_id==NoExists && tlg_num==NoExists)
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

void set_ids_search_params(const set<int> &ids, ostringstream &sql, TQuery &Qry)
{
  int i=1;
  for(set<int>::const_iterator id=ids.begin(); id!=ids.end(); ++id)
  {
    string param;
    param="id" + IntToString(i);

    if (id==ids.begin())
      sql << ":" << param;
    else
      sql << ", :" << param;
    Qry.CreateVariable(param, otInteger, *id);
    i++;
  };
}

void set_tlgs_in_search_params(const TTlgSearchParams &search_params, ostringstream &sql, TQuery &Qry)
{
    bool filtered=false;

    if(search_params.pr_time_create) {
        sql << " AND tlgs_in.time_create >= :TimeCreateFrom AND tlgs_in.time_create < :TimeCreateTo \n";
        Qry.CreateVariable("TimeCreateFrom", otDate, search_params.TimeCreateFrom);
        Qry.CreateVariable("TimeCreateTo", otDate, search_params.TimeCreateTo);
        filtered=true;
    }

    if(search_params.pr_time_receive) {
        sql << " AND tlgs_in.time_receive >= :TimeReceiveFrom AND tlgs_in.time_receive < :TimeReceiveTo \n";
        Qry.CreateVariable("TimeReceiveFrom", otDate, search_params.TimeReceiveFrom);
        Qry.CreateVariable("TimeReceiveTo", otDate, search_params.TimeReceiveTo);
        filtered=true;
    }

    if(!search_params.tlg_type.empty()) {
        sql << " AND tlgs_in.type = :tlg_type \n";
        Qry.CreateVariable("tlg_type", otString, search_params.tlg_type);
        filtered=true;
    }

    if (!search_params.typeb_in_ids.empty())
    {
        sql << " AND tlgs_in.id IN (";
        set_ids_search_params(search_params.typeb_in_ids, sql, Qry);
        sql << ") \n";
        filtered=true;
    };

    if (!filtered) throw Exception("set_tlgs_in_search_params: bad situation!");
}

void set_tlg_trips_search_params(const TTlgSearchParams &search_params, ostringstream &sql, TQuery &Qry)
{
    TReqInfo &info = *(TReqInfo::Instance());

    if (!info.user.access.airlines.empty()) {
        if (info.user.access.airlines_permit)
            sql << " AND tlg_trips.airline IN " << GetSQLEnum(info.user.access.airlines) << " \n";
        else
            sql << " AND tlg_trips.airline NOT IN " << GetSQLEnum(info.user.access.airlines) << " \n";
    }

    if (!info.user.access.airps.empty()) {
        if (info.user.access.airps_permit)
            sql << " AND (tlg_trips.airp_dep IS NULL AND tlg_trips.airp_arv IS NULL OR \n"
                << "      tlg_trips.airp_dep IN " << GetSQLEnum(info.user.access.airps) << " OR \n"
                << "      tlg_trips.airp_arv IN " << GetSQLEnum(info.user.access.airps) << ") \n" ;
        else
            sql << " AND (tlg_trips.airp_dep IS NULL AND tlg_trips.airp_arv IS NULL OR \n"
                << "      tlg_trips.airp_dep NOT IN " << GetSQLEnum(info.user.access.airps) << " OR \n"
                << "      tlg_trips.airp_arv NOT IN " << GetSQLEnum(info.user.access.airps) << ") \n" ;
    }

    if(!search_params.airline.empty()) {
        sql << " AND tlg_trips.airline = :airline \n";
        Qry.CreateVariable("airline", otString, search_params.airline);
    }

    if(!search_params.airp.empty()) {
        sql << " AND tlg_trips.airp_dep = :airp \n";
        Qry.CreateVariable("airp", otString, search_params.airp);
    }

    if(search_params.flt_no != NoExists) {
        sql << " AND tlg_trips.flt_no = :flt_no \n";
        Qry.CreateVariable("flt_no", otInteger, search_params.flt_no);
        if(!search_params.suffix.empty()) {
            sql << " AND tlg_trips.suffix = :suffix \n";
            Qry.CreateVariable("suffix", otString, search_params.suffix);
        }
    }
};

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

struct TTlgInPart {
    int id;
    int num;
    string type;
    TypeB::TDraftPart draft;
    TDateTime time_receive;
    bool is_final_part;
    bool is_history;
    TTlgInPart():
        id(NoExists),
        num(NoExists),
        time_receive(NoExists),
        is_final_part(false),
        is_history(false)
    {}
};

void TelegramInterface::GetTlgIn2(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo &info = *(TReqInfo::Instance());
    xmlNodePtr tlgsNode = NewTextChild( resNode, "tlgs" );
    if ((info.user.access.airps_permit && info.user.access.airps.empty()) ||
        (info.user.access.airlines_permit && info.user.access.airlines.empty()) ) return;

    TTlgSearchParams search_params;
    search_params.get(reqNode);
    search_params.dump();

    TQuery Qry(&OraSession);
    if (search_params.tlg_id!=NoExists)
    {
      search_params.typeb_in_ids.insert(search_params.tlg_id);
      Qry.Clear();
      Qry.SQLText="SELECT id, typeb_tlg_id FROM tlgs WHERE id=:tlg_id";
      Qry.CreateVariable("tlg_id", otInteger, search_params.tlg_id);
      Qry.Execute();
      if (!Qry.Eof)
      {
        if (!Qry.FieldIsNULL("typeb_tlg_id"))
          search_params.typeb_in_ids.insert(Qry.FieldAsInteger("typeb_tlg_id"));
        else
          search_params.tlgs_ids.insert(Qry.FieldAsInteger("id"));
      };
    };
    if (search_params.tlg_num!=NoExists)
    {
      Qry.Clear();
      Qry.SQLText="SELECT id, typeb_tlg_id FROM tlgs WHERE tlg_num=:tlg_num";
      Qry.CreateVariable("tlg_num", otFloat, search_params.tlg_num);
      Qry.Execute();
      if (!Qry.Eof)
      {
        if (!Qry.FieldIsNULL("typeb_tlg_id"))
          search_params.typeb_in_ids.insert(Qry.FieldAsInteger("typeb_tlg_id"));
        else
          search_params.tlgs_ids.insert(Qry.FieldAsInteger("id"));
      };
    };


    bool limited_access=!info.user.access.airlines.empty() ||
                        !info.user.access.airps.empty();

    bool need_check_tlg_trips = !search_params.airline.empty() ||
                                !search_params.airp.empty() ||
                                search_params.flt_no != NoExists ||
                                !info.user.access.airlines.empty() ||
                                !info.user.access.airps.empty();


    string tz_region =  info.desk.tz_region;

    vector<TTlgInPart> tlgs;
    for(int pass=1; pass<=2; pass++)
    {
      if (search_params.tlg_id!=NoExists || search_params.tlg_num!=NoExists)
      {
        if (pass==1 && search_params.typeb_in_ids.empty()) continue;
        if (pass==2 && search_params.tlgs_ids.empty()) continue;
      };

      Qry.Clear();
      ostringstream sql;
      if (pass==1)
      {
        sql << "SELECT \n"
               "   tlgs_in.id, \n"
               "   tlgs_in.num, \n"
               "   tlgs_in.type, \n"
               "   tlgs_in.addr, \n"
               "   tlgs_in.heading, \n"
               "   tlgs_in.body, \n"
               "   tlgs_in.ending, \n"
               "   tlgs_in.time_receive, \n"
               "   tlgs_in.is_final_part, \n"
               "   typeb_in_history.prev_tlg_id \n"
               "FROM tlgs_in, typeb_in_history, \n"
               "( \n";
        if(search_params.err_cls == 1)
        {
            //неразобранные (содержащие ошибки)
            if (!limited_access)
            {
              sql << " SELECT DISTINCT tlgs_in.id \n"
                     " FROM tlgs_in, tlg_source \n"
                     " WHERE tlgs_in.id=tlg_source.tlg_id(+) AND tlg_source.tlg_id IS NULL \n";
              set_tlgs_in_search_params(search_params, sql, Qry);
              sql << " UNION \n";
            };

            if (need_check_tlg_trips)
            {
              sql << " SELECT DISTINCT ids.id \n"
                     " FROM tlg_trips, (\n";
            };

            sql << " SELECT DISTINCT tlgs_in.id"
                << (need_check_tlg_trips?", tlg_source.point_id_tlg \n":" \n")
                << " FROM tlgs_in, tlg_source \n"
                   " WHERE tlgs_in.id=tlg_source.tlg_id AND \n"
                   "       NVL(tlg_source.has_errors,0)<>0 \n";
            set_tlgs_in_search_params(search_params, sql, Qry);

            if (need_check_tlg_trips)
            {
              sql << " ORDER BY tlgs_in.id) ids \n"
                     " WHERE ids.point_id_tlg=tlg_trips.point_id \n";
              set_tlg_trips_search_params(search_params, sql, Qry);
            };
        }
        else
        {
            //непривязанные (в том числе и содержащие ошибки)
            if (need_check_tlg_trips)
            {
              sql << " SELECT DISTINCT ids.id \n"
                     " FROM tlg_trips, (\n";
            };

            sql << " SELECT DISTINCT tlgs_in.id"
                << (need_check_tlg_trips?", tlg_source.point_id_tlg \n":" \n")
                << " FROM tlgs_in, tlg_source, tlg_binding \n"
                   " WHERE tlgs_in.id=tlg_source.tlg_id AND \n"
                   "       tlg_source.point_id_tlg=tlg_binding.point_id_tlg(+) AND \n"
                   "       tlg_binding.point_id_tlg IS NULL \n";
            set_tlgs_in_search_params(search_params, sql, Qry);

            if (need_check_tlg_trips)
            {
              sql << " ORDER BY tlgs_in.id) ids \n"
                     " WHERE ids.point_id_tlg=tlg_trips.point_id \n";
              set_tlg_trips_search_params(search_params, sql, Qry);
            };
        }

        sql << ") ids \n"
               "WHERE tlgs_in.id=ids.id and tlgs_in.id = typeb_in_history.prev_tlg_id(+) \n"
               "ORDER BY id,num \n";
      }
      else
      {
        if (search_params.err_cls == 1 &&
            !limited_access &&
            !search_params.tlgs_ids.empty())
        {
          //неразобранные (содержащие ошибки)
          sql << "SELECT \n"
                 "   tlgs.id, \n"
                 "   1 AS num, \n"
                 "   NULL AS type, \n"
                 "   NULL AS addr, \n"
                 "   NULL AS heading, \n"
                 "   tlg_text, \n"
                 "   NULL AS ending, \n"
                 "   time AS time_receive \n"
                 "FROM tlgs \n"
                 "WHERE type IN ('INA','INB') AND \n"
                 "      id IN (";
          set_ids_search_params(search_params.tlgs_ids, sql, Qry);
          sql << ") \n";
        }
        else continue;
      };

      ProgTrace(TRACE5, "sql: %s", sql.str().c_str());
      Qry.SQLText=sql.str();
      Qry.Execute();

      if(!Qry.Eof) {
          int col_type = Qry.FieldIndex("type");
          int col_id = Qry.FieldIndex("id");
          int col_num = Qry.FieldIndex("num");
          int col_addr = Qry.FieldIndex("addr");
          int col_heading = Qry.FieldIndex("heading");
          int col_ending = Qry.FieldIndex("ending");
          int col_time_receive = Qry.FieldIndex("time_receive");

          for(;!Qry.Eof;Qry.Next()) {
              TTlgInPart tlg;
              tlg.id = Qry.FieldAsInteger(col_id);
              tlg.num = Qry.FieldAsInteger(col_num);
              tlg.type = Qry.FieldAsString(col_type);
              tlg.draft.addr = Qry.FieldAsString(col_addr);
              tlg.draft.heading = Qry.FieldAsString(col_heading);
              tlg.draft.ending = Qry.FieldAsString(col_ending);
              tlg.time_receive = UTCToClient( Qry.FieldAsDateTime(col_time_receive), tz_region );
              if (pass==1)
              {
                tlg.is_final_part = Qry.FieldAsInteger("is_final_part")!=0;
                tlg.is_history = not Qry.FieldIsNULL("prev_tlg_id");
                tlg.draft.body = getTypeBBody(tlg.id, tlg.num, Qry);
              }
              else
              {
                tlg.is_final_part = false;
                tlg.is_history = false;
                tlg.draft.body = getTlgText(tlg.id, Qry);
              };
              tlgs.push_back(tlg);
          };
          if(tlgs.size() >= 4000)
              throw AstraLocale::UserException("MSG.TOO_MANY_DATA.ADJUST_SEARCH_PARAMS");
      }
    };

  TypeB::TErrLst err_lst(TypeB::tioIn);
  xmlNodePtr node;
  for(vector<TTlgInPart>::iterator iv = tlgs.begin(); iv != tlgs.end(); iv++)
  {
      node = NewTextChild( tlgsNode, "tlg" );

      try {
          err_lst.fromDB(iv->id, iv->num);
          if(TReqInfo::Instance()->desk.compatible(TLG_ERR_BROWSE_VERSION))
          {
            bool is_first_part = iv == tlgs.begin() or (iv - 1)->id != iv->id;
            bool is_last_part = (iv + 1 == tlgs.end() or (iv + 1)->id != iv->id);
            err_lst.toXML(node, iv->draft, is_first_part, is_last_part, TReqInfo::Instance()->desk.lang);
          }
          else
          {
            if (!err_lst.empty()) iv->type=!iv->type.empty()?"!"+iv->type+"!":"!";
          };
      } catch(Exception &E) {
          ProgError(STDLOG, "ErrLst: tlg_id = %d, num = %d; %s", iv->id, iv->num, E.what());
      } catch(...) {
          ProgError(STDLOG, "ErrLst: tlg_id = %d, num = %d; unknown exception", iv->id, iv->num);
      }

      NewTextChild( node, "id", iv->id);
      NewTextChild( node, "num", iv->num);
      NewTextChild( node, "type", iv->type);
      NewTextChild( node, "addr", GetValidXMLString(iv->draft.addr));
      NewTextChild( node, "heading", GetValidXMLString(iv->draft.heading));
      NewTextChild( node, "body", GetValidXMLString(iv->draft.body));
      NewTextChild( node, "ending", GetValidXMLString(iv->draft.ending));
      NewTextChild( node, "time_receive", DateTimeToStr( iv->time_receive ) );
      NewTextChild( node, "is_final_part", (int)iv->is_final_part, (int)false);
      NewTextChild( node, "is_history", (int)iv->is_history);
  };
}

void TelegramInterface::GetTlgIn(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo &info = *(TReqInfo::Instance());
  xmlNodePtr tlgsNode = NewTextChild( resNode, "tlgs" );
  if ((info.user.access.airps_permit && info.user.access.airps.empty()) ||
      (info.user.access.airlines_permit && info.user.access.airlines.empty()) ) return;

  int point_id = NodeAsInteger( "point_id", reqNode );

  TQuery RegionQry(&OraSession);
  RegionQry.SQLText="SELECT airp FROM points WHERE point_id=:point_id AND pr_del>=0";
  RegionQry.CreateVariable("point_id",otInteger,point_id);
  RegionQry.Execute();
  if (RegionQry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");
  string tz_region = AirpTZRegion(RegionQry.FieldAsString("airp"));

  TQuery Qry(&OraSession);
  Qry.SQLText=
    "SELECT "
    " tlgs_in.id, "
    " tlgs_in.num, "
    " tlgs_in.type, "
    " tlgs_in.addr, "
    " tlgs_in.heading, "
    " tlgs_in.body, "
    " tlgs_in.ending, "
    " tlgs_in.time_receive, "
    " tlgs_in.is_final_part, "
    " typeb_in_history.prev_tlg_id "
    "FROM tlgs_in, typeb_in_history, "
    "     (SELECT DISTINCT tlg_source.tlg_id AS id "
    "      FROM tlg_source,tlg_binding "
    "      WHERE tlg_source.point_id_tlg=tlg_binding.point_id_tlg AND "
    "            tlg_binding.point_id_spp=:point_id) ids "
    "WHERE tlgs_in.id=ids.id AND tlgs_in.id = typeb_in_history.prev_tlg_id(+) "
    "ORDER BY id,num ";
  Qry.CreateVariable("point_id",otInteger,point_id);
  Qry.Execute();

  vector<TTlgInPart> tlgs;
  for(;!Qry.Eof;Qry.Next()) {
      TTlgInPart tlg;
      tlg.id = Qry.FieldAsInteger("id");
      tlg.num = Qry.FieldAsInteger("num");
      tlg.type = Qry.FieldAsString("type");
      tlg.draft.addr = Qry.FieldAsString("addr");
      tlg.draft.heading = Qry.FieldAsString("heading");
      tlg.draft.ending = Qry.FieldAsString("ending");
      tlg.time_receive = UTCToClient( Qry.FieldAsDateTime("time_receive"), tz_region );
      tlg.is_final_part = Qry.FieldAsInteger("is_final_part")!=0;
      tlg.is_history = not Qry.FieldIsNULL("prev_tlg_id");
      tlg.draft.body = getTypeBBody(tlg.id, tlg.num, Qry);
      tlgs.push_back(tlg);
  };

  TypeB::TErrLst err_lst(TypeB::tioIn);
  xmlNodePtr node;
  for(vector<TTlgInPart>::iterator iv = tlgs.begin(); iv != tlgs.end(); iv++)
  {
      node = NewTextChild( tlgsNode, "tlg" );

      try {
          err_lst.fromDB(iv->id, iv->num);
          if(TReqInfo::Instance()->desk.compatible(TLG_ERR_BROWSE_VERSION))
          {
            bool is_first_part = iv == tlgs.begin() or (iv - 1)->id != iv->id;
            bool is_last_part = (iv + 1 == tlgs.end() or (iv + 1)->id != iv->id);
            err_lst.toXML(node, iv->draft, is_first_part, is_last_part, TReqInfo::Instance()->desk.lang);
          }
          else
          {
            if (!err_lst.empty()) iv->type=!iv->type.empty()?"!"+iv->type+"!":"!";
          };
      } catch(Exception &E) {
          ProgError(STDLOG, "ErrLst: tlg_id = %d, num = %d; %s", iv->id, iv->num, E.what());
      } catch(...) {
          ProgError(STDLOG, "ErrLst: tlg_id = %d, num = %d; unknown exception", iv->id, iv->num);
      }

      NewTextChild( node, "id", iv->id);
      NewTextChild( node, "num", iv->num);
      NewTextChild( node, "type", iv->type);
      NewTextChild( node, "addr", GetValidXMLString(iv->draft.addr));
      NewTextChild( node, "heading", GetValidXMLString(iv->draft.heading));
      NewTextChild( node, "body", GetValidXMLString(iv->draft.body));
      NewTextChild( node, "ending", GetValidXMLString(iv->draft.ending));
      NewTextChild( node, "time_receive", DateTimeToStr( iv->time_receive ) );
      NewTextChild( node, "is_final_part", (int)iv->is_final_part, (int)false);
      NewTextChild( node, "is_history", (int)iv->is_history);
  };
};

struct TTlgOutPart {
    string tlg_type;
    int id;
    int num;
    TypeB::TDraftPart draft;
    bool completed;
    bool has_errors;
    string extra;
    bool pr_lat;
    TDateTime time_create;
    TDateTime time_send_scd;
    TDateTime time_send_act;
    TTlgOutPart():
        id(NoExists),
        num(NoExists),
        completed(false),
        has_errors(false),
        pr_lat(false),
        time_create(NoExists),
        time_send_scd(NoExists),
        time_send_act(NoExists)
    {}
};

void TelegramInterface::GetTlgOut(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  xmlNodePtr node = GetNode( "point_id", reqNode );
  int point_id;
  TQuery Qry(&OraSession);
  string tz_region;
  ostringstream sql;
  sql << "SELECT point_id,id,num,addr,origin,heading,body,ending, "
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

  vector<TTlgOutPart> tlgs;
  for(;!Qry.Eof;Qry.Next())
  {
      TTlgOutPart tlg;
      tlg.tlg_type = Qry.FieldAsString("tlg_type");
      tlg.id = Qry.FieldAsInteger("id");
      tlg.num = Qry.FieldAsInteger("num");

      tlg.draft.addr = Qry.FieldAsString("addr");
      tlg.draft.origin = Qry.FieldAsString("origin");
      tlg.draft.heading = Qry.FieldAsString("heading");
      tlg.draft.body = Qry.FieldAsString("body");
      tlg.draft.ending = Qry.FieldAsString("ending");

      tlg.completed = Qry.FieldAsInteger("completed") != 0;
      tlg.has_errors = Qry.FieldAsInteger("has_errors") != 0;
      tlg.extra = Qry.FieldAsString("extra");
      tlg.pr_lat = Qry.FieldAsInteger("pr_lat") != 0;
      tlg.time_create = Qry.FieldAsDateTime("time_create");
      if(not Qry.FieldIsNULL("time_send_scd"))
          tlg.time_send_scd = Qry.FieldAsDateTime("time_send_scd");
      if(not Qry.FieldIsNULL("time_send_act"))
          tlg.time_send_act = Qry.FieldAsDateTime("time_send_act");
      tlgs.push_back(tlg);
  }

  ostringstream endl_stream;
  endl_stream << endl;
  TypeB::TErrLst err_lst(TypeB::tioOut);

  for(vector<TTlgOutPart>::iterator iv = tlgs.begin(); iv != tlgs.end(); iv++)
  {
    node = NewTextChild( tlgsNode, "tlg" );

    TTypeBTypesRow& row = (TTypeBTypesRow&)(base_tables.get("typeb_types").get_row("code",iv->tlg_type));
    string basic_type = row.basic_type;

    bool is_first_part = iv == tlgs.begin() or (iv - 1)->id != iv->id;
    bool is_last_part = (iv + 1 == tlgs.end() or (iv + 1)->id != iv->id);

    err_lst.fromDB(iv->id, iv->num);
    if(TReqInfo::Instance()->desk.compatible(TLG_ERR_BROWSE_VERSION)) {
        err_lst.toXML(node, iv->draft, is_first_part, is_last_part, TReqInfo::Instance()->desk.lang);
    } else
        err_lst.unpack(iv->draft, is_first_part, is_last_part);

    NewTextChild( node, "id", iv->id );
    NewTextChild( node, "num", iv->num);
    NewTextChild( node, "tlg_type", iv->tlg_type, basic_type );
    NewTextChild( node, "tlg_short_name", ElemIdToNameShort(etTypeBType, iv->tlg_type), basic_type );
    NewTextChild( node, "basic_type", basic_type );
    bool editable = row.editable;
    bool completed = iv->completed;
    bool has_errors = iv->has_errors;
    if(editable)
        editable = not has_errors;
    if(completed)
        completed = not has_errors;
    NewTextChild( node, "editable", editable, false );

    NewTextChild( node, "addr", iv->draft.addr );
    NewTextChild( node, "heading", iv->draft.origin + iv->draft.heading );
    NewTextChild( node, "body", iv->draft.body );
    NewTextChild( node, "ending", iv->draft.ending );
    NewTextChild( node, "is_final_part", (int)is_last_part, (int)false);
    if(TReqInfo::Instance()->desk.compatible(CACHE_CHILD_VERSION))
      NewTextChild( node, "extra", iv->extra, "" );
    else
      NewTextChild( node, "extra", CharReplace(iv->extra,endl_stream.str().c_str()," "), "" );

    NewTextChild( node, "pr_lat", iv->pr_lat);
    NewTextChild( node, "completed", completed, true );

    TDateTime time_create = UTCToClient( iv->time_create, tz_region );
    NewTextChild( node, "time_create", DateTimeToStr( time_create ) );

    if (iv->time_send_scd != NoExists)
    {
      TDateTime time_send_scd = UTCToClient( iv->time_send_scd, tz_region );
      NewTextChild( node, "time_send_scd", DateTimeToStr( time_send_scd ) );
    }
    else
      NewTextChild( node, "time_send_scd" );

    if (iv->time_send_act != NoExists)
    {
      TDateTime time_send_act = UTCToClient( iv->time_send_act, tz_region );
      NewTextChild( node, "time_send_act", DateTimeToStr( time_send_act ) );
    }
    else
      NewTextChild( node, "time_send_act" );
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

void TelegramInterface::SaveInTlg(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  string text = NodeAsString("tlg_text",reqNode);
  if (text.empty()) throw AstraLocale::UserException("MSG.TLG.EMPTY");
  int tlg_id = NodeAsInteger("tlg_id", reqNode);
  int num = NodeAsInteger("num", reqNode);
  if(num != 0) throw AstraLocale::UserException("MSG.TLG.CANT_SAVE_SEPARATE_PART");
  bool hist_uniq_error;
  loadTlg(text, tlg_id, hist_uniq_error);
  if (hist_uniq_error)
      throw UserException("MSG.TLG.CHANGED.REFRESH_DATA");
  registerHookAfter(sendCmdTypeBHandler);
  AstraLocale::showMessage("MSG.TLG.LOADED");
}

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
    "SELECT tlg_out.type AS code, "
    "       point_id, "
    "       NVL(LENGTH(addr),0)+ "
    "       NVL(LENGTH(origin),0)+ "
    "       NVL(LENGTH(heading),0)+ "
    "       NVL(LENGTH(ending),0) AS len "
    "FROM tlg_out "
    "WHERE id=:id AND num=1 FOR UPDATE";
  Qry.CreateVariable( "id", otInteger, tlg_id);
  Qry.Execute();
  if (Qry.Eof) throw AstraLocale::UserException("MSG.TLG.NOT_FOUND.REFRESH_DATA");

  if (tlg_body.size()+Qry.FieldAsInteger("len") > PART_SIZE)
    throw AstraLocale::UserException("MSG.TLG.MAX_LENGTH", LParams() << LParam("count", (int)PART_SIZE));

  string tlg_code=Qry.FieldAsString("code");
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
  Qry.CreateVariable( "body", otString, tlg_body );
  Qry.Execute();

  check_tlg_out_alarm(point_id);

  TReqInfo::Instance()->LocaleToLog("EVT.TLG.MODIFIED", LEvntPrms() << PrmElem<std::string>("tlg_name", etTypeBType, tlg_code, efmtNameShort)
                                    << PrmSmpl<int>("tlg_id", tlg_id), evtTlg, point_id, tlg_id);
  AstraLocale::showMessage("MSG.TLG.SAVED");
};

namespace TypeB
{
enum TTransportType { ttBagMessage, ttHttp, ttSirena, ttNone };

class IataAddrOwnerItem
{
  public:
    string addr;
    TTransportType transport_type;
    string transport_addr;
    string country;
    map<string, string> transport_params;
    IataAddrOwnerItem()
    {
      clear();
    }
    void clear()
    {
      addr.clear();
      transport_type=ttNone;
      transport_addr.clear();
      country.clear();
      transport_params.clear();
    }
    void fromDB(const string &iata_addr)
    {
      clear();

      if (iata_addr.size()!=7)
        throw AstraLocale::UserException("MSG.TLG.INVALID_SITA_ADDR", LParams() << LParam("addr", iata_addr));
      for(string::const_iterator p=iata_addr.begin(); p!=iata_addr.end(); ++p)
        if (!((IsUpperLetter(*p)&&IsAscii7(*p))||IsDigit(*p)))
          throw AstraLocale::UserException("MSG.TLG.INVALID_SITA_ADDR", LParams() << LParam("addr", iata_addr));

      QParams QryParams;
      QryParams << QParam("addr", otString, iata_addr);
      TCachedQuery Qry("SELECT * FROM typeb_addr_owners WHERE addr=:addr", QryParams);
      Qry.get().Execute();
      if (Qry.get().Eof) throw AstraLocale::UserException("MSG.TLG.SITA.CANON_ADDR_UNDEFINED", LParams() << LParam("addr", iata_addr));

      string transport=Qry.get().FieldAsString("transport_type");
      if (transport=="BAG_MESSAGE") transport_type=ttBagMessage;
      else if (transport=="HTTP_TYPEB") transport_type=ttHttp;
      else if (transport=="SIRENA_TYPEB") transport_type=ttSirena;

      if (transport_type==ttNone)
        throw Exception("Unknown transport type %s for %s", transport.c_str(), iata_addr.c_str());

      addr=Qry.get().FieldAsString("addr");
      transport_addr=Qry.get().FieldAsString("transport_addr");
      country=Qry.get().FieldAsString("country");

      QParams PQryParams;
      PQryParams << QParam("addr", otString, iata_addr);
      TCachedQuery PQry("SELECT param_name, param_value FROM typeb_addr_trans_params WHERE addr=:addr", PQryParams);
      PQry.get().Execute();
      for(; !PQry.get().Eof; PQry.get().Next())
        transport_params.insert(make_pair(PQry.get().FieldAsString("param_name"), PQry.get().FieldAsString("param_value")));
    }
    bool operator == ( const IataAddrOwnerItem& item ) const
    {
      if (transport_type!=item.transport_type) return false;
      if (transport_addr!=item.transport_addr) return false;
      if (transport_type!=ttSirena)
      {
        if (transport_params!=item.transport_params) return false;
      };
      return true;
    }
};

}

string GetOutputGateway(const string &orig_canon_name,
                        const string &dest_canon_name)
{
  QParams QryParams;
  QryParams << QParam("orig", otString, orig_canon_name)
            << QParam("dest", otString, dest_canon_name);
  TCachedQuery Qry("SELECT out_canon_name FROM output_gateways WHERE orig_canon_name=:orig AND dest_canon_name=:dest", QryParams);
  Qry.get().Execute();
  if (Qry.get().Eof) return dest_canon_name;
  return Qry.get().FieldAsString("out_canon_name");
};

void TelegramInterface::SendTlg(int tlg_id)
{
  try
  {
    TQuery TlgQry(&OraSession);
    TlgQry.Clear();
    TlgQry.SQLText=
      "SELECT * FROM tlg_out WHERE id=:id FOR UPDATE";
    TlgQry.CreateVariable( "id", otInteger, tlg_id);
    TlgQry.Execute();
    if (TlgQry.Eof) throw AstraLocale::UserException("MSG.TLG.NOT_FOUND.REFRESH_DATA");
    if (TlgQry.FieldAsInteger("completed")==0)
      throw AstraLocale::UserException("MSG.TLG.MANUAL_EDIT");
    if (TlgQry.FieldAsInteger("has_errors")==1)
      throw AstraLocale::UserException("MSG.TLG.HAS_ERRORS.UNABLE_SEND");

    TTlgOutPartInfo tlg;
    tlg.fromDB(TlgQry);

    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText="SELECT id, addr, double_sign, descr FROM typeb_originators WHERE id=:originator_id";
    Qry.CreateVariable( "originator_id", otInteger, tlg.originator_id );
    Qry.Execute();
    if (Qry.Eof) throw AstraLocale::UserException("MSG.TLG.NOT_FOUND.REFRESH_DATA");
    TypeB::TOriginatorInfo originator;
    originator.fromDB(Qry);

    TypeB::IataAddrOwnerItem orig;
    orig.fromDB(originator.addr);

    string tlg_basic_type;
    try
    {
      const TTypeBTypesRow& row = (TTypeBTypesRow&)(base_tables.get("typeb_types").get_row("code",tlg.tlg_type));
      tlg_basic_type=row.basic_type;
    }
    catch(EBaseTableError)
    {
      throw AstraLocale::UserException("MSG.TLG.NOT_FOUND.REFRESH_DATA");
    };

    TTripInfo fltInfo;
    if (tlg.point_id!=NoExists)
    {
      Qry.Clear();
      Qry.SQLText="SELECT airline, flt_no, suffix, airp, scd_out FROM points WHERE point_id=:point_id";
      Qry.CreateVariable("point_id", otInteger, tlg.point_id);
      Qry.Execute();
      if (Qry.Eof) throw AstraLocale::UserException("MSG.TLG.NOT_FOUND.REFRESH_DATA");
      fltInfo.Init(Qry);
    };

    string old_addrs;
    list< list<TypeB::IataAddrOwnerItem> > recvs;
    set<string> iata_addrs;
    TypeB::TTlgParser parser;
    const char *addrs,*line_p;

    for(;!TlgQry.Eof;TlgQry.Next())
    {
      tlg.fromDB(TlgQry);
      if (tlg.addr!=old_addrs)
      {
        recvs.clear();
        line_p=tlg.addr.c_str();
        try
        {
          do
          {
            addrs=parser.GetLexeme(line_p);
            while (addrs!=NULL)
            {
              if (iata_addrs.insert(parser.lex).second)  //проверим на уникальность адреса IATA
              {
                TypeB::IataAddrOwnerItem dest;
                dest.fromDB(parser.lex);
                if (orig.transport_type==TypeB::ttSirena &&
                    dest.transport_type==TypeB::ttSirena)
                {
                  dest.transport_addr=GetOutputGateway(orig.transport_addr, dest.transport_addr);
                };

                list< list<TypeB::IataAddrOwnerItem> >::iterator r=recvs.begin();
                for(; r!=recvs.end(); ++r)
                {
                  if (r->empty()) continue;
                  if (*(r->begin())==dest) break;
                };
                if (r==recvs.end()) r=recvs.insert(recvs.end(), list<TypeB::IataAddrOwnerItem>());
                if (r==recvs.end()) throw Exception("%s: r==recvs.end()", __FUNCTION__);

                r->push_back(dest);
              };
              addrs=parser.GetLexeme(addrs);
            };
          }
          while ((line_p=parser.NextLine(line_p))!=NULL);
        }
        catch(TypeB::ETlgError)
        {
          throw AstraLocale::UserException("MSG.WRONG_ADDR_LINE");
        };
        old_addrs=tlg.addr;
      };
      if (recvs.empty()) throw AstraLocale::UserException("MSG.TLG.DST_ADDRS_NOT_SET");

      //формируем телеграмму
      for(list< list<TypeB::IataAddrOwnerItem> >::const_iterator i=recvs.begin();i!=recvs.end();++i)
      {
        if (i->empty()) throw Exception("%s: i->empty()", __FUNCTION__);

        string addrs;
        for(list<TypeB::IataAddrOwnerItem>::const_iterator j=i->begin(); j!=i->end(); ++j)
        {
          if (!addrs.empty()) addrs+=" ";
          addrs+=j->addr;
        };
/*
        //трассировка маршрутизации
        if (i == recvs.begin())
        {
          LogTrace(TRACE5) << __FUNCTION__ << ": "
                           << std::left << setw(15) << "transport_type"
                           << std::left << setw(15) << "transport_addr"
                           << std::left << setw(10) << "iata_addr";
        };

        string transport_type_str;
        if (i->begin()->transport_type==TypeB::ttSirena) transport_type_str="ttSirena";
        if (i->begin()->transport_type==TypeB::ttBagMessage) transport_type_str="ttBagMessage";
        if (i->begin()->transport_type==TypeB::ttHttp) transport_type_str="ttHttp";

        LogTrace(TRACE5) << __FUNCTION__ << ": "
                         << std::left << setw(15) << transport_type_str
                         << std::left << setw(15) << i->begin()->transport_addr
                         << std::left << setw(10) << addrs;
*/
        addrs=TypeB::format_addr_line(addrs);

        if (i->begin()->transport_type==TypeB::ttSirena)
        {
          string tlg_text=addrs+tlg.origin+tlg.heading+tlg.body+tlg.ending;
          if (OWN_CANON_NAME()==i->begin()->transport_addr)
          {
            // сразу помещаем во входную очередь
            loadTlg(tlg_text);
            registerHookAfter(sendCmdTypeBHandler);
          }
          else
          {
            int queue_tlg_id=sendTlg(i->begin()->transport_addr.c_str(),OWN_CANON_NAME(),qpOutB,0,tlg_text,tlg_id,tlg.num);
            for(list<TypeB::IataAddrOwnerItem>::const_iterator j=i->begin(); j!=i->end(); ++j)
            {
              TTlgStat().putTypeBOut(queue_tlg_id,
                                     tlg_id,
                                     tlg.num,
                                     TTlgStatPoint(originator.addr, OWN_CANON_NAME(), OWN_CANON_NAME(), ""),
                                     TTlgStatPoint(j->addr, j->transport_addr, j->transport_addr, j->country),
                                     tlg.time_create,
                                     tlg_basic_type,
                                     tlg_text.size(),
                                     fltInfo,
                                     tlg.airline_mark,
                                     originator.descr);
            };
          };
        }
        else if (i->begin()->transport_type==TypeB::ttBagMessage)
        {
          //это передача телеграмм в BagMessage
          //без addr и origin
          map<string, string> params;
          params=i->begin()->transport_params;
          params[ PARAM_CANON_NAME ] = i->begin()->transport_addr;
          params[ NS_PARAM_EVENT_TYPE ] = EncodeEventType( ASTRA::evtTlg );
          params[ NS_PARAM_EVENT_ID1 ] = IntToString( tlg.point_id );
          params[ NS_PARAM_EVENT_ID2 ] = IntToString( tlg_id );
          TFileQueue::putFile(i->begin()->transport_addr,
                              OWN_POINT_ADDR(),
                              FILE_BAG_MESSAGE_TYPE,
                              params,
                              tlg.heading+tlg.body+tlg.ending);
        }
        else if (i->begin()->transport_type==TypeB::ttHttp)
        {
          //это передача телеграмм по HTTP
          map<string, string> params;
          params=i->begin()->transport_params;
          tlg.addToFileParams(params);
          TFileQueue::putFile(i->begin()->transport_addr,
                              OWN_POINT_ADDR(),
                              FILE_HTTP_TYPEB_TYPE,
                              params,
                              tlg.body);
          registerHookAfter(sendCmdTlgHttpSnd);
        };

        if (i == recvs.begin()) // ignore same tlg for different receivers
          putUTG(tlg_id, tlg.num, tlg_basic_type, fltInfo, tlg.heading+tlg.body+tlg.ending, tlg.extra);
      };
    };
    markTlgAsSent(tlg_id);
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

void markTlgAsSent(int tlg_id)
{
    QParams QryParams;
    QryParams
        << QParam("tlg_type", otString)
        << QParam("point_id", otInteger)
        << QParam("id", otInteger, tlg_id);
    TCachedQuery Qry(
            "declare "
            "  curRow tlg_out%rowtype; "
            "begin "
            "  select * into curRow from tlg_out where id = :id and rownum < 2; "
            "  :tlg_type := curRow.type; "
            "  :point_id := curRow.point_id; "
            "  UPDATE tlg_out SET time_send_act=system.UTCSYSDATE WHERE id=:id; "
            "end; ",
            QryParams);
    Qry.get().Execute();
    TReqInfo::Instance()->LocaleToLog("EVT.TLG.SENT", LEvntPrms()
            << PrmElem<std::string>("tlg_name", etTypeBType, Qry.get().GetVariableAsString("tlg_type"), efmtNameShort)
            << PrmSmpl<int>("tlg_id", tlg_id), evtTlg, Qry.get().GetVariableAsInteger("point_id"), tlg_id);
}

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
        "SELECT tlg_out.type AS code, "
        "       point_id "
        "FROM tlg_out "
        "WHERE id=:id AND num=1 FOR UPDATE";
    Qry.CreateVariable( "id", otInteger, tlg_id);
    Qry.Execute();
    if (Qry.Eof) throw AstraLocale::UserException("MSG.TLG.NOT_FOUND.REFRESH_DATA");

    string tlg_code=Qry.FieldAsString("code");
    int point_id=Qry.FieldAsInteger("point_id");

    Qry.Clear();
    Qry.SQLText=
        "DELETE FROM tlg_out WHERE id=:id AND time_send_act IS NULL ";
    Qry.CreateVariable( "id", otInteger, tlg_id);
    Qry.Execute();
    if (Qry.RowsProcessed()>0)
    {
        Qry.SQLText=
            "begin "
            "   DELETE FROM typeb_out_extra WHERE tlg_id=:id; "
            "   delete from typeb_out_errors where tlg_id = :id; "
            "end;";
        Qry.Execute();
        TReqInfo::Instance()->LocaleToLog("EVT.TLG.DELETED", LEvntPrms()
                                          << PrmElem<std::string>("tlg_name", etTypeBType, tlg_code, efmtNameShort)
                                          << PrmSmpl<int>("tlg_id", tlg_id), evtTlg, point_id, tlg_id);
        AstraLocale::showMessage("MSG.TLG.DELETED");
    };
    check_tlg_out_alarm(point_id);
    GetTlgOut(ctxt,reqNode,resNode);
};

void TelegramInterface::SendTlg(const vector<TypeB::TCreateInfo> &info, int typeb_in_id) 
{
  for(vector<TypeB::TCreateInfo>::const_iterator i=info.begin(); i!=info.end(); ++i)
  {
    try
    {
        int typeb_out_id=NoExists;
        TTypeBTypesRow tlgTypeInfo;
        string lexema_id;
        LEvntPrms params;
        try
        {
            time_t time_start=time(NULL);
            typeb_out_id = create_tlg( *i, tlgTypeInfo, false);

            time_t time_end=time(NULL);
            if (time_end-time_start>1)
                ProgTrace(TRACE5,"Attention! c++ create_tlg execute time: %ld secs, type=%s, point_id=%d",
                        time_end-time_start,
                        i->get_tlg_type().c_str(),
                        i->point_id);

            if (typeb_out_id!=NoExists) //телеграмма создалась
            {
              lexema_id = "EVT.TLG.CREATED";
              params << PrmElem<std::string>("name", etTypeBType, i->get_tlg_type(), efmtNameShort)
                     << PrmSmpl<int>("id", typeb_out_id) << PrmBool("lat", i->get_options().is_lat);
              TReqInfo::Instance()->LocaleToLog(lexema_id, params, evtTlg, i->point_id, typeb_out_id);
            };
        }
        catch(AstraLocale::UserException &E)
        {
            lexema_id = "EVT.TLG.CREATE_ERROR";
            string err_id;
            LEvntPrms err_prms;
            E.getAdvParams(err_id, err_prms);

            params << PrmElem<std::string>("name", etTypeBType, i->get_tlg_type(), efmtNameShort)
                   << PrmBool("lat", i->get_options().is_lat) << PrmLexema("what", err_id, err_prms);
            TReqInfo::Instance()->LocaleToLog(lexema_id, params, evtTlg, i->point_id, typeb_out_id);
        }

        if (typeb_out_id!=NoExists)
        {
            time_t time_start=time(NULL);
            if(not TypeBHelpMng::notify(typeb_in_id, typeb_out_id)) // Если небыло процесса для отвисания, действуем как обычно
                try
                {
                    SendTlg(typeb_out_id);
                }
            catch(AstraLocale::UserException &E)
            {
                string err_id;
                LEvntPrms err_prms;
                E.getAdvParams(err_id, err_prms);

                params << PrmElem<std::string>("name", etTypeBType, i->get_tlg_type(), efmtNameShort)
                    << PrmSmpl<int>("id", typeb_out_id) << PrmLexema("what", err_id, err_prms);

                TReqInfo::Instance()->LocaleToLog("EVT.TLG.SEND_ERROR", params, evtTlg, i->point_id, typeb_out_id);
            };
            time_t time_end=time(NULL);
            if (time_end-time_start>1)
                ProgTrace(TRACE5,"Attention! SendTlg execute time: %ld secs, typeb_out_id=%d",
                        time_end-time_start,typeb_out_id);
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
  con.OutCls=Qry.FieldAsString("class");

  int point_id=Qry.FieldAsInteger("point_id");
  int point_num=Qry.FieldAsInteger("point_num");
  int first_point=Qry.FieldIsNULL("first_point")?NoExists:Qry.FieldAsInteger("first_point");
  bool pr_tranzit=Qry.FieldAsInteger("pr_tranzit")!=0;

  bool pr_unaccomp=con.OutCls.empty();

  //читаем OutFlt и OnwardFlt
  if (!con.OnwardFlt.GetRoute(grp_id, trtWithFirstSeg)) return;
  if (con.OnwardFlt.empty()) return;
  con.OutFlt=*con.OnwardFlt.begin();
  con.OnwardFlt.erase(con.OnwardFlt.begin());

  if (!pr_unaccomp)
  {
    vector<TTlgCompLayer> complayers;
    if(not SALONS2::isFreeSeating(point_id) and not SALONS2::isEmptySalons(point_id)) {
      getSalonLayers( point_id, point_num, first_point, pr_tranzit, complayers, false );
    }

    Qry.Clear();
    Qry.SQLText=
      "SELECT DISTINCT transfer_subcls.transfer_num, classes.code, classes.priority "
      "FROM pax, transfer_subcls, subcls, classes "
      "WHERE pax.pax_id=transfer_subcls.pax_id AND "
      "      transfer_subcls.subclass=subcls.code AND "
      "      subcls.class=classes.code AND "
      "      pax.grp_id=:grp_id AND transfer_subcls.transfer_num>0 "
      "ORDER BY transfer_num, priority ";
    Qry.CreateVariable("grp_id",otInteger,grp_id);
    Qry.Execute();
    int num=0;
    for(;!Qry.Eof;Qry.Next())
    {
      if (num!=Qry.FieldAsInteger("transfer_num"))
      {
        num=Qry.FieldAsInteger("transfer_num");
        con.OnwardCls.push_back(list<string>());
      };
      con.OnwardCls.back().push_back(Qry.FieldAsString("code"));
    };

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
  conADD.OutCls=con2.OutCls;
  conADD.OnwardCls=con2.OnwardCls;

  conCHG.indicator=TypeB::CHG;
  conCHG.OutFlt=con2.OutFlt;
  conCHG.OnwardFlt=con2.OnwardFlt;
  conCHG.pr_lat_seat=con2.pr_lat_seat;
  conCHG.OutCls=con2.OutCls;
  conCHG.OnwardCls=con2.OnwardCls;

  conDEL.indicator=TypeB::DEL;
  conDEL.OutFlt=con1.OutFlt;
  conDEL.OnwardFlt=con1.OnwardFlt;
  conDEL.pr_lat_seat=con1.pr_lat_seat;
  conDEL.OutCls=con1.OutCls;
  conDEL.OnwardCls=con1.OnwardCls;


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

void CreateTlgBody(const TTlgContent& con, const TypeB::TCreateInfo &createInfo, TTlgOutPartInfo &partInfo)
{
  const TypeB::TBSMOptions &options=*(createInfo.optionsAs<TypeB::TBSMOptions>());

  partInfo.pr_lat=options.is_lat;

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


  ostringstream heading;
  heading << "BSM" << ENDL;
  partInfo.heading = heading.str();

  ostringstream body;

  body.setf(ios::fixed);

  switch(con.indicator)
  {
    case TypeB::CHG: body << "CHG" << ENDL;
                     break;
    case TypeB::DEL: body << "DEL" << ENDL;
                     break;
     default: ;
  };

  body << ".V/1L" << TlgElemIdToElem(etAirp, con.OutFlt.operFlt.airp, options.is_lat) << ENDL;

  TDateTime scd_out;
  if(con.OutFlt.operFlt.airp == "АЯТ")
      scd_out = con.OutFlt.operFlt.real_out;
  else
      scd_out = con.OutFlt.operFlt.scd_out;

  body << ".F/"
       << TlgElemIdToElem(etAirline, con.OutFlt.operFlt.airline, options.is_lat)
       << setw(3) << setfill('0') << con.OutFlt.operFlt.flt_no
       << (con.OutFlt.operFlt.suffix.empty() ? "" : TlgElemIdToElem(etSuffix, con.OutFlt.operFlt.suffix, options.is_lat)) << '/'
       << DateTimeToStr( scd_out, "ddmmm", options.is_lat) << '/'
       << TlgElemIdToElem(etAirp, con.OutFlt.airp_arv, options.is_lat);
  if (options.class_of_travel && !con.OutCls.empty())
    body << '/' << TlgElemIdToElem(etClass, con.OutCls, options.is_lat);
  body << ENDL;

  vector<list<string> >::const_iterator j=con.OnwardCls.begin();
  for(TTrferRoute::const_iterator i=con.OnwardFlt.begin(); i!=con.OnwardFlt.end(); ++i)
  {
    body << ".O/"
         << TlgElemIdToElem(etAirline, i->operFlt.airline, options.is_lat)
         << setw(3) << setfill('0') << i->operFlt.flt_no
         << (i->operFlt.suffix.empty() ? "" : TlgElemIdToElem(etSuffix, i->operFlt.suffix, options.is_lat)) << '/'
         << DateTimeToStr( i->operFlt.scd_out, "ddmmm", options.is_lat) << '/'
         << TlgElemIdToElem(etAirp, i->airp_arv, options.is_lat);
    if (j!=con.OnwardCls.end())
    {
      if (!j->empty() && !j->begin()->empty() && options.class_of_travel && !con.OutCls.empty())
        body << '/' << TlgElemIdToElem(etClass, *(j->begin()), options.is_lat);
      ++j;
    };
    body << ENDL;
  };

  map<int, pair<TPaxItem, vector<CheckIn::TTagItem> > >::const_iterator p=tmpPax.begin();
  for(;p!=tmpPax.end();++p)
  {
    const vector<CheckIn::TTagItem> &tmpTags=p->second.second;
    if (tmpTags.empty()) throw Exception("BSM::CreateTlgBody: tmpTags empty");
    double first_no = 0.;
    int num = 0;
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
           << p->second.first.seat_no.get_seat_one(con.pr_lat_seat || options.is_lat) << '/'
           << p->second.first.status << '/'
           << setw(3) << setfill('0') << p->second.first.reg_no;
    body << ENDL;

    body << ".W/K/" << p->second.first.bag_amount << '/' << p->second.first.bag_weight; //всегда пишем
/*    if (p->second.first.rk_weight!=0)
      body << '/' << p->second.first.rk_weight;*/
    body << ENDL;

    body << ".P/" << transliter(p->second.first.surname,1,options.is_lat);
    if (!p->second.first.name.empty())
      body << '/' << transliter(p->second.first.name,1,options.is_lat);
    body  << ENDL;

    if (!p->second.first.pnr_addr.empty())
      body << ".L/" << convert_pnr_addr(p->second.first.pnr_addr,options.is_lat) << ENDL;
  };
  partInfo.body = body.str();

  ostringstream ending;
  ending << "ENDBSM" << ENDL;
  partInfo.ending = ending.str();
};

bool IsSend( const TAdvTripInfo &fltInfo, TBSMAddrs &addrs )
{
    TypeB::TCreator creator(fltInfo);
    creator << "BSM";
    creator.getInfo(addrs.createInfo);

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

    for(vector<TTlgContent>::iterator i=bsms.begin();i!=bsms.end();++i)
    {
      if(not addrs.empty()) {
          TypeB::TOriginatorInfo originator=TypeB::getOriginator(i->OutFlt.operFlt.airline,
                                                                 i->OutFlt.operFlt.airp,
                                                                 p.tlg_type, p.time_create, true);
          p.originator_id=originator.id;
          p.origin=originator.originSection(p.time_create, ENDL);
      }

      for(vector<TypeB::TCreateInfo>::const_iterator j=addrs.createInfo.begin();j!=addrs.createInfo.end();++j)
      {
        p.id=NoExists;
        p.num=1;
        p.addr=TypeB::format_addr_line(j->get_addrs());
        CreateTlgBody(*i, *j, p);
        TelegramInterface::SaveTlgOutPart(p, true, false);
        TelegramInterface::SendTlg(p.id);
      };
    };

    check_tlg_out_alarm(point_dep);
};

};

void TelegramInterface::SaveTlgOutPart( TTlgOutPartInfo &info, bool completed, bool has_errors )
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
    "INSERT INTO tlg_out(id,num,type,point_id,addr,origin,heading,body,ending,pr_lat, "
    "  completed,has_errors,time_create,time_send_scd,time_send_act, "
    "  originator_id,airline_mark,manual_creation) "
    "VALUES(:id,:num,:type,:point_id,:addr,:origin,:heading,:body,:ending,:pr_lat, "
    "  :completed,:has_errors,NVL(:time_create,system.UTCSYSDATE),:time_send_scd,NULL, "
    "  :originator_id,:airline_mark,:manual_creation)";

  /*
  ProgTrace(TRACE5, "-------SaveTlgOutPart--------");
  ProgTrace(TRACE5, "id: %d", info.id);
  ProgTrace(TRACE5, "num: %d", info.num);
  ProgTrace(TRACE5, "point_id: %d", info.point_id);
  ProgTrace(TRACE5, "addr: %s", info.addr.c_str());
  ProgTrace(TRACE5, "origin: %s", info.origin.c_str());
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
  Qry.CreateVariable("origin",otString,info.origin);
  Qry.CreateVariable("heading",otString,info.heading);
  Qry.CreateVariable("body",otString,info.body);
  Qry.CreateVariable("ending",otString,info.ending);
  Qry.CreateVariable("pr_lat",otInteger,(int)info.pr_lat);
  Qry.CreateVariable("completed",otInteger,(int)completed);
  Qry.CreateVariable("has_errors",otInteger,(int)has_errors);

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
  Qry.CreateVariable("manual_creation",otInteger,(int)info.manual_creation);
  Qry.Execute();
  ProgTrace(TRACE5, "Qry.Execute() throw");

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
                           const int typeb_tlg_id,
                           const int typeb_tlg_num,
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
    "INSERT INTO tlg_stat(queue_tlg_id, typeb_tlg_id, typeb_tlg_num, "
    "  sender_sita_addr, sender_canon_name, sender_descr, sender_country, "
    "  receiver_sita_addr, receiver_canon_name, receiver_descr, receiver_country, "
    "  time_create, time_send, time_receive, tlg_type, tlg_len, "
    "  airline, flt_no, suffix, scd_local_date, airp_dep, airline_mark, extra) "
    "VALUES(:queue_tlg_id, :typeb_tlg_id, :typeb_tlg_num, "
    "  :sender_sita_addr, :sender_canon_name, :sender_descr, :sender_country, "
    "  :receiver_sita_addr, :receiver_canon_name, :receiver_descr, :receiver_country, "
    "  :time_create, NULL, NULL, :tlg_type, :tlg_len, "
    "  :airline, :flt_no, :suffix, :scd_local_date, :airp_dep, :airline_mark, :extra) ";
  Qry.CreateVariable("queue_tlg_id", otInteger, queue_tlg_id);
  Qry.CreateVariable("typeb_tlg_id", otInteger, typeb_tlg_id);
  Qry.CreateVariable("typeb_tlg_num", otInteger, typeb_tlg_num);
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

