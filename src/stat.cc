#include <fstream>
#include "stat.h"
#include "oralib.h"
#include "cache.h"
#include "xml_unit.h"
#include "exceptions.h"
#include "stl_utils.h"
#include "misc.h"
#include "docs.h"
#include "base_tables.h"
#include "astra_utils.h"
#include "astra_date_time.h"
#include "term_version.h"
#include "passenger.h"
#include "points.h"
#include "telegram.h"
#include "qrys.h"
#include "tlg/tlg.h"
#include "astra_elem_utils.h"
#include "astra_elems.h"
#include "serverlib/xml_stuff.h"
#include "astra_misc.h"
#include "file_queue.h"
#include "jxtlib/zip.h"
#include "astra_context.h"
#include "serverlib/str_utils.h"
#include <boost/filesystem.hpp>
#include <boost/shared_array.hpp>
#include <boost/crc.hpp>
#include "md5_sum.h"
#include "payment_base.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

#define WITHOUT_TOTAL_WHEN_PROBLEM false

using namespace std;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace BASIC::date_time;
using namespace ASTRA::date_time;

int MAX_STAT_ROWS()
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("MAX_STAT_ROWS",NoExists,NoExists,2000);
  return VAR;
};

const string SYSTEM_USER = "Система";

const int arx_trip_date_range=5;

enum TOrderStatus {
    stReady,
    stRunning,
    stCorrupted,
    stError,
    stUnknown,
    stNum
};

const char *TOrderStatusS[] = {
    "Готов",
    "Выполняется",
    "Поврежден",
    "Ошибка",
    "?"
};

TOrderStatus DecodeOrderStatus( const string &os )
{
  int i;
  for( i=0; i<(int)stNum; i++ )
    if ( os == TOrderStatusS[ i ] )
      break;
  if ( i == stNum )
    return stUnknown;
  else
    return (TOrderStatus)i;
}

const string EncodeOrderStatus(TOrderStatus s)
{
  return TOrderStatusS[s];
};

enum TOrderSource {
    osSTAT,
    osUnknown,
    osNum
};

const char *TOrderSourceS[] = {
    "STAT",
    "?"
};

TOrderSource DecodeOrderSource( const string &os )
{
  int i;
  for( i=0; i<(int)osNum; i++ )
    if ( os == TOrderSourceS[ i ] )
      break;
  if ( i == osNum )
    return osUnknown;
  else
    return (TOrderSource)i;
}

const string EncodeOrderSource(TOrderSource s)
{
  return TOrderSourceS[s];
};


const string AIRP_PERIODS =
"(SELECT DECODE(SIGN(period_first_date-:FirstDate),1,period_first_date,:FirstDate) AS period_first_date, \n"
"         DECODE(SIGN(period_last_date- :LastDate),-1,period_last_date, :LastDate) AS period_last_date \n"
"  FROM \n"
"   (SELECT distinct change_point AS period_first_date, \n"
"           report.get_airp_period_last_date(:ap,change_point) AS period_last_date \n"
"    FROM pacts, \n"
"     (SELECT first_date AS change_point \n"
"      FROM pacts \n"
"      WHERE airp=:ap \n"
"      UNION \n"
"      SELECT last_date   \n"
"      FROM pacts \n"
"      WHERE airp=:ap AND last_date IS NOT NULL) a \n"
"    WHERE airp=:ap AND  \n"
"          change_point>=first_date AND  \n"
"          (last_date IS NULL OR change_point<last_date) AND \n"
"          airline IS NULL) periods \n"
"  WHERE (period_last_date IS NULL OR period_last_date>:FirstDate) AND period_first_date<:LastDate \n"
" ) periods \n";
const string AIRLINE_PERIODS =
"(SELECT DECODE(SIGN(period_first_date-:FirstDate),1,period_first_date,:FirstDate) AS period_first_date, \n"
"         DECODE(SIGN(period_last_date- :LastDate),-1,period_last_date, :LastDate) AS period_last_date \n"
"  FROM \n"
"   (SELECT distinct change_point AS period_first_date, \n"
"           report.get_airline_period_last_date(:ak,change_point) AS period_last_date \n"
"    FROM pacts,  \n"
"     (SELECT first_date AS change_point \n"
"      FROM pacts \n"
"      WHERE airline=:ak \n"
"      UNION \n"
"      SELECT last_date   \n"
"      FROM pacts \n"
"      WHERE airline=:ak AND last_date IS NOT NULL) a \n"
"    WHERE airline=:ak AND  \n"
"          change_point>=first_date AND  \n"
"          (last_date IS NULL OR change_point<last_date)) periods \n"
"  WHERE (period_last_date IS NULL OR period_last_date>:FirstDate) AND period_first_date<:LastDate \n"
" ) periods \n";

const string AIRLINE_LIST =
" (SELECT airline  \n"
"  FROM pacts  \n"
"  WHERE airp=:ap AND \n"
"        first_date<=periods.period_first_date AND  \n"
"        (last_date IS NULL OR periods.period_first_date<last_date) AND \n"
"        airline IS NOT NULL) \n";

const string AIRP_LIST =
" (SELECT airp  \n"
"  FROM pacts  \n"
"  WHERE airline=:ak AND \n"
"        first_date<=periods.period_first_date AND  \n"
"        (last_date IS NULL OR periods.period_first_date<last_date) ) \n";

void GetSystemLogAgentSQL(TQuery &Qry);
void GetSystemLogStationSQL(TQuery &Qry);
void GetSystemLogModuleSQL(TQuery &Qry);

enum TScreenState {ssNone,ssLog,ssPaxList,ssFltLog,ssFltTaskLog,ssSystemLog,ssPaxSrc};
enum TColumnSortType {sortString,
                      sortInteger,
                      sortFloat,
                      sortDate,
                      sortDateTime,
                      sortTime,
                      sortIntegerSlashInteger,
                      sortSlashedInt}; // любая последовательность, вида N.../N.../N...

void GetSystemLogAgentSQL(TQuery &Qry)
{
    string SQLText =
        "select -1, null agent, -1 view_order from dual "
        "union "
        "select 0, '";
    SQLText += getLocaleText(SYSTEM_USER);
    SQLText +=
        "' agent, 0 view_order from dual "
        "union "
        "select 1, descr agent, 1 view_order from users2 where "
        "  (adm.check_user_access(user_id,:SYS_user_id)<>0 or user_id=:SYS_user_id) "
        "order by "
        "   view_order, agent";
    Qry.SQLText = SQLText;
    Qry.CreateVariable("SYS_user_id", otInteger, TReqInfo::Instance()->user.user_id);
}

void GetSystemLogStationSQL(TQuery &Qry)
{
    string SQLText =
        "select -1, null station, -1 view_order from dual "
        "union "
        "select 0, '";
    SQLText += getLocaleText(SYSTEM_USER);
    SQLText +=
        "' station, 0 view_order from dual "
        "union "
        "select 1, code, 1 from desks where "
        "   adm.check_desk_view_access(code, :SYS_user_id) <> 0 "
        "order by "
        "   view_order, station";
    Qry.SQLText = SQLText;
    Qry.CreateVariable("SYS_user_id", otInteger, TReqInfo::Instance()->user.user_id);
}

void GetSystemLogModuleSQL(TQuery &Qry)
{
    string SQLText =
        "select -1, null module, -1 view_order from dual "
        "union "
        "select 0, '";
    SQLText += getLocaleText(SYSTEM_USER);
    SQLText +=
        "' module, 0 view_order from dual "
        "union "
        "select id, name, view_order from screen where view_order is not null order by view_order";
    Qry.SQLText = SQLText;
}


class TPointsRow
{
  public:
    TDateTime part_key, real_out_client;
    string airline, suffix, name;
    int point_id, flt_no, move_id, point_num;
    bool operator == (const TPointsRow &item) const
    {
      return part_key==item.part_key &&
             name==item.name &&
             point_id==item.point_id;
    };
} ;

bool lessPointsRow(const TPointsRow& item1,const TPointsRow& item2)
{
    bool result;
    if(item1.real_out_client == item2.real_out_client) {
        if(item1.flt_no == item2.flt_no) {
            if(item1.airline == item2.airline) {
                if(item1.suffix == item2.suffix) {
                    if(item1.move_id == item2.move_id) {
                        result = item1.point_num < item2.point_num;
                    } else
                        result = item1.move_id < item2.move_id;
                } else
                    result = item1.suffix < item2.suffix;
            } else
                result = item1.airline < item2.airline;
        } else
            result = item1.flt_no < item2.flt_no;
    } else
        result = item1.real_out_client > item2.real_out_client;
    return result;
};

void GetFltCBoxList(TScreenState scr, TDateTime first_date, TDateTime last_date, bool pr_show_del, vector<TPointsRow> &points)
{
    TReqInfo &reqInfo = *(TReqInfo::Instance());
    TQuery Qry(&OraSession);
    Qry.CreateVariable("FirstDate", otDate, first_date);
    Qry.CreateVariable("LastDate", otDate, last_date);
    string trip_name;
    TPerfTimer tm;
    tm.Init();
    int count = 0;
    if (!reqInfo.user.access.totally_not_permitted())
    {
        for(int pass=0; pass<=2; pass++)
        {
            ostringstream sql;
            if (pass==0)
                sql << "SELECT \n"
                    "    NULL part_key, \n"
                    "    move_id, \n";
            else
                sql << "SELECT \n"
                    "    arx_points.part_key, \n"
                    "    arx_points.move_id, \n";

            sql << "    point_id, airp, airline, flt_no, \n"
                "    airline_fmt, airp_fmt, suffix_fmt, \n"
                "    suffix, \n"
                "    scd_out, trunc(NVL(act_out,NVL(est_out,scd_out))) AS real_out, \n"
                "    pr_del, point_num \n";
            if (pass==0)
                sql << "FROM points \n"
                    "WHERE points.scd_out >= :FirstDate AND points.scd_out < :LastDate \n";
            if (pass==1)
                sql << "FROM arx_points \n"
                    "WHERE arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate AND \n"
                    "      arx_points.part_key >= :FirstDate and arx_points.part_key < :LastDate + :arx_trip_date_range \n";
            if (pass==2)
                sql << "FROM arx_points, \n"
                    "     (SELECT part_key, move_id FROM move_arx_ext \n"
                    "      WHERE part_key >= :LastDate + :arx_trip_date_range AND part_key <= :LastDate + date_range) arx_ext \n"
                    "WHERE arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate AND \n"
                    "      arx_points.part_key=arx_ext.part_key AND arx_points.move_id=arx_ext.move_id \n";

            if(scr == ssPaxList)
                sql << " AND pr_del = 0 \n";
            if((scr == ssFltLog or scr == ssFltTaskLog) and !pr_show_del)
                sql << " AND pr_del >= 0 \n";
            if (!reqInfo.user.access.airlines().elems().empty()) {
                if (reqInfo.user.access.airlines().elems_permit())
                    sql << " AND airline IN " << GetSQLEnum(reqInfo.user.access.airlines().elems()) << "\n";
                else
                    sql << " AND airline NOT IN " << GetSQLEnum(reqInfo.user.access.airlines().elems()) << "\n";
            }
            if (!reqInfo.user.access.airps().elems().empty()) {
                if (reqInfo.user.access.airps().elems_permit())
                {
                    sql << "AND (airp IN " << GetSQLEnum(reqInfo.user.access.airps().elems()) << " OR \n";
                    if (pass==0)
                        sql << "ckin.next_airp(DECODE(pr_tranzit,0,point_id,first_point), point_num) IN \n";
                    else
                        sql << "arch.next_airp(arx_points.part_key, DECODE(pr_tranzit,0,point_id,first_point), point_num) IN \n";
                    sql << GetSQLEnum(reqInfo.user.access.airps().elems()) << ") \n";
                }
                else
                {
                    sql << "AND (airp NOT IN " << GetSQLEnum(reqInfo.user.access.airps().elems()) << " OR \n";
                    if (pass==0)
                        sql << "ckin.next_airp(DECODE(pr_tranzit,0,point_id,first_point), point_num) NOT IN \n";
                    else
                        sql << "arch.next_airp(arx_points.part_key, DECODE(pr_tranzit,0,point_id,first_point), point_num) NOT IN \n";
                    sql << GetSQLEnum(reqInfo.user.access.airps().elems()) << ") \n";
                };
            };

            if (pass!=0)
                Qry.CreateVariable("arx_trip_date_range", otInteger, ARX_TRIP_DATE_RANGE());

            //ProgTrace(TRACE5, "FltCBoxDropDown: pass=%d SQL=\n%s", pass, sql.str().c_str());
            Qry.SQLText = sql.str().c_str();
            try {
                Qry.Execute();
            } catch (EOracleError &E) {
                if(E.Code == 376)
                    throw AstraLocale::UserException("MSG.ONE_OF_DB_FILES_UNAVAILABLE.CALL_ADMIN");
                else
                    throw;
            }
            if(!Qry.Eof) {
                int col_move_id=Qry.FieldIndex("move_id");
                int col_point_num=Qry.FieldIndex("point_num");
                int col_point_id=Qry.FieldIndex("point_id");
                int col_part_key=Qry.FieldIndex("part_key");
                for( ; !Qry.Eof; Qry.Next()) {
                    TTripInfo tripInfo(Qry);
                    try
                    {
                        trip_name = GetTripName(tripInfo,ecCkin,false,true);
                    }
                    catch(AstraLocale::UserException &E)
                    {
                        AstraLocale::showErrorMessage("MSG.ERR_MSG.NOT_ALL_FLIGHTS_ARE_SHOWN", LParams() << LParam("msg", getLocaleText(E.getLexemaData())));
                        continue;
                    };
                    TPointsRow pointsRow;
                    TDateTime scd_out_client;
                    if(Qry.FieldIsNULL(col_part_key))
                        pointsRow.part_key = NoExists;
                    else
                        pointsRow.part_key = Qry.FieldAsDateTime(col_part_key);
                    pointsRow.point_id = Qry.FieldAsInteger(col_point_id);
                    tripInfo.get_client_dates(scd_out_client, pointsRow.real_out_client);
                    pointsRow.airline = tripInfo.airline;
                    pointsRow.suffix = tripInfo.suffix;
                    pointsRow.name = trip_name;
                    pointsRow.flt_no = tripInfo.flt_no;
                    pointsRow.move_id = Qry.FieldAsInteger(col_move_id);
                    pointsRow.point_num = Qry.FieldAsInteger(col_point_num);
                    points.push_back(pointsRow);

                    count++;
                    if(count >= MAX_STAT_ROWS()) {
                        AstraLocale::showErrorMessage("MSG.TOO_MANY_FLIGHTS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_SEARCH",
                                LParams() << LParam("num", MAX_STAT_ROWS()));
                        break;
                    }
                }
            }
        }
    };
    ProgTrace(TRACE5, "FltCBoxDropDown EXEC QRY: %s", tm.PrintWithMessage().c_str());
    if(count == 0)
        throw AstraLocale::UserException("MSG.FLIGHTS_NOT_FOUND");
    tm.Init();
    sort(points.begin(), points.end(), lessPointsRow);
    ProgTrace(TRACE5, "FltCBoxDropDown SORT: %s", tm.PrintWithMessage().c_str());
};

namespace STAT
{

void TMoveIds::get_for_airp(TDateTime first_date, TDateTime last_date, const std::string& airp)
{
  clear();

  TReqInfo &reqInfo = *(TReqInfo::Instance());
  reqInfo.user.access.set_total_permit();
  reqInfo.user.access.merge_airps(TAccessElems<std::string>(airp, true));

  vector<TPointsRow> points;
  GetFltCBoxList(ssPaxList, first_date, last_date, false, points);
  for(vector<TPointsRow>::const_iterator i=points.begin(); i!=points.end(); ++i)
    insert(make_pair(i->part_key, i->move_id));
}

}

void GetMinMaxPartKey(const string &where, TDateTime &min_part_key, TDateTime &max_part_key)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT TRUNC(MIN(part_key)) AS min_part_key FROM arx_points";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("min_part_key"))
    min_part_key=NoExists;
  else
    min_part_key=Qry.FieldAsDateTime("min_part_key");

  Qry.SQLText="SELECT MAX(part_key) AS max_part_key FROM arx_points";
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("max_part_key"))
    max_part_key=NoExists;
  else
    max_part_key=Qry.FieldAsDateTime("max_part_key");

  ProgTrace(TRACE5, "%s: min_part_key=%s max_part_key=%s",
                    where.c_str(),
                    DateTimeToStr(min_part_key, ServerFormatDateTimeAsString).c_str(),
                    DateTimeToStr(max_part_key, ServerFormatDateTimeAsString).c_str());
};

template <class T>
bool EqualCollections(const string &where, const T &c1, const T &c2, const string &err1, const string &err2,
                      pair<typename T::const_iterator, typename T::const_iterator> &diff)
{
  diff.first=c1.end();
  diff.second=c2.end();
  if (!err1.empty() || !err2.empty())
  {
    if (err1!=err2)
    {
      ProgError(STDLOG, "%s: err1=%s err2=%s", where.c_str(), err1.c_str(), err2.c_str());
      return false;
    };
  }
  else
  {
    //ошибок нет
    if (c1.size() != c2.size())
    {
      ProgError(STDLOG, "%s: c1.size()=%zu c2.size()=%zu", where.c_str(), c1.size(), c2.size());
      diff.first=c1.begin();
      diff.second=c2.begin();
      for(;diff.first!=c1.end() && diff.second!=c2.end(); diff.first++,diff.second++)
      {
        if (*(diff.first)==*(diff.second)) continue;
        break;
      };
      return false;
    }
    else
    {
      //размер векторов совпадает
      if (!c1.empty() && !c2.empty())
      {
        diff=mismatch(c1.begin(),c1.end(),c2.begin());
        if (diff.first!=c1.end() || diff.second!=c2.end()) return false;
      };
    };
  };
  return true;
};

void StatInterface::FltCBoxDropDown(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    bool pr_show_del = false;
    xmlNodePtr prDelNode = GetNode("pr_del", reqNode);
    if(prDelNode)
        pr_show_del = NodeAsInteger(prDelNode) == 1;
    TScreenState scr = TScreenState(NodeAsInteger("scr", reqNode));
    TDateTime first_date=NodeAsDateTime("FirstDate", reqNode);
    TDateTime last_date=NodeAsDateTime("LastDate", reqNode);
    vector<TPointsRow> points;
    GetFltCBoxList(scr, first_date, last_date, pr_show_del, points);

    TPerfTimer tm;
    tm.Init();
    xmlNodePtr cboxNode = NewTextChild(resNode, "cbox");
    for(vector<TPointsRow>::const_iterator iv = points.begin(); iv != points.end(); iv++) {
        xmlNodePtr fNode = NewTextChild(cboxNode, "f");
        NewTextChild( fNode, "name", iv->name);
        NewTextChild(fNode, "point_id", iv->point_id);
        if(iv->part_key != NoExists)
            NewTextChild(fNode, "part_key", DateTimeToStr(iv->part_key, ServerFormatDateTimeAsString));
    }
    ProgTrace(TRACE5, "FltCBoxDropDown XML: %s", tm.PrintWithMessage().c_str());
}

void StatInterface::CommonCBoxDropDown(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    string cbox = NodeAsString("cbox", reqNode);
    TQuery Qry(&OraSession);
    if(cbox == "AgentCBox")
        GetSystemLogAgentSQL(Qry);
    else if(cbox == "StationCBox")
        GetSystemLogStationSQL(Qry);
    else if(cbox == "ModuleCBox")
        GetSystemLogModuleSQL(Qry);
    else
        throw Exception("StatInterface::CommonCBoxDropDown: unknown cbox: %s", cbox.c_str());

    try {
        Qry.Execute();
    } catch (EOracleError &E) {
        if(E.Code == 376)
            throw AstraLocale::UserException("MSG.ONE_OF_DB_FILES_UNAVAILABLE.CALL_ADMIN");
        else
            throw;
    }
    xmlNodePtr cboxNode = NewTextChild(resNode, "cbox");
    while(!Qry.Eof) {
        xmlNodePtr fNode = NewTextChild(cboxNode, "f");
        NewTextChild(fNode, "key", Qry.FieldAsInteger(0));
        NewTextChild(fNode, "value", getLocaleText(Qry.FieldAsString(1)));
        Qry.Next();
    }
}

void StatInterface::FltTaskLogRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqInfo = TReqInfo::Instance();
    if (!reqInfo->user.access.rights().permitted(651))
        throw AstraLocale::UserException("MSG.FLT_TASK_LOG.VIEW_DENIED");
    xmlNodePtr paramNode = reqNode->children;
    int point_id = NodeAsIntegerFast("point_id", paramNode);
    TDateTime part_key;
    xmlNodePtr partKeyNode = GetNodeFast("part_key", paramNode);
    if(partKeyNode == NULL)
        part_key = NoExists;
    else
        part_key = NodeAsDateTime(partKeyNode);
    get_compatible_report_form("FltTaskLog", reqNode, resNode);
    STAT::set_variables(resNode);
    xmlNodePtr variablesNode = GetNode("form_data/variables", resNode);
    NewTextChild(variablesNode, "report_title", getLocaleText("Журнал задач рейса"));
    TQuery Qry(&OraSession);
    int count = 0;

    xmlNodePtr paxLogNode = NewTextChild(resNode, "PaxLog");
    xmlNodePtr headerNode = NewTextChild(paxLogNode, "header");
    NewTextChild(headerNode, "col", "Агент"); // для совместимости со старой версией терминала

    Qry.Clear();
    string SQLQuery;
    string airline;
    if (part_key == NoExists) {
        {
            TQuery Qry(&OraSession);
            Qry.SQLText = "select airline from points where point_id = :point_id "; // pr_del>=0 - не надо т.к. можно просматривать удаленные рейсы
            Qry.CreateVariable("point_id", otInteger, point_id);
            Qry.Execute();
            if(Qry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.MOVED_TO_ARX_OR_DEL.SELECT_AGAIN");
            airline = Qry.FieldAsString("airline");
        }
        SQLQuery =
            "SELECT msg, time,  "
            "       id1 AS point_id,  "
            "       events_bilingual.screen,  "
            "       ev_user, station, ev_order, NVL(part_num, 1) AS part_num "
            "FROM events_bilingual  "
            "WHERE "
            "   events_bilingual.lang = :lang AND  "
            "   events_bilingual.type = :evtFltTask AND  "
            "   events_bilingual.id1=:point_id  ";
    } else {
        if(ARX_EVENTS_DISABLED())
            throw UserException("MSG.ERR_MSG.ARX_EVENTS_DISABLED");
        {
            TQuery Qry(&OraSession);
            Qry.SQLText =
                "select airline from arx_points "
                "where part_key = :part_key and point_id = :point_id "; // pr_del >= 0 - не надо, т.к. в архиве нет удаленных рейсов
            Qry.CreateVariable("part_key", otDate, part_key);
            Qry.CreateVariable("point_id", otInteger, point_id);
            Qry.Execute();
            if(Qry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND");
            airline = Qry.FieldAsString("airline");
        }
        SQLQuery =
            "SELECT msg, time,  "
            "       id1 AS point_id,  "
            "       arx_events.screen,  "
            "       ev_user, station, ev_order, NVL(part_num, 1) AS part_num  "
            "FROM arx_events  "
            "WHERE "
            "   arx_events.part_key = :part_key and "
            "   (arx_events.lang = :lang OR arx_events.lang = :lang_undef) AND "
            "   arx_events.type = :evtFltTask AND  "
            "   arx_events.id1=:point_id  ";
    }
    NewTextChild(resNode, "airline", airline);

    TPerfTimer tm;
    tm.Init();
    xmlNodePtr rowsNode = NULL;
    Qry.Clear();
    Qry.SQLText = SQLQuery;
    Qry.CreateVariable("lang", otString, TReqInfo::Instance()->desk.lang);
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("evtFltTask",otString,EncodeEventType(ASTRA::evtFltTask));
    if(part_key != NoExists) {
        Qry.CreateVariable("part_key", otDate, part_key);
        Qry.CreateVariable("lang_undef", otString, "ZZ");
    }
    try {
        Qry.Execute();
    } catch (EOracleError &E) {
        if(E.Code == 376)
            throw AstraLocale::UserException("MSG.ONE_OF_DB_FILES_UNAVAILABLE.CALL_ADMIN");
        else
            throw;
    }

    if(Qry.Eof && part_key == NoExists) {
        TQuery Qry(&OraSession);
        Qry.SQLText = "select point_id from points where point_id = :point_id";
        Qry.CreateVariable("point_id", otInteger, point_id);
        Qry.Execute();
        if(Qry.Eof)
            throw AstraLocale::UserException("MSG.FLIGHT.MOVED_TO_ARX_OR_DEL.SELECT_AGAIN");
    }

    typedef map<string, string> TScreenMap;
    TScreenMap screen_map;
    if(!Qry.Eof) {
        int col_point_id=Qry.FieldIndex("point_id");
        int col_ev_user=Qry.FieldIndex("ev_user");
        int col_station=Qry.FieldIndex("station");
        int col_time=Qry.FieldIndex("time");
        int col_msg=Qry.FieldIndex("msg");
        int col_ev_order=Qry.FieldIndex("ev_order");
        int col_part_num=Qry.FieldIndex("part_num");
        int col_screen=Qry.FieldIndex("screen");

        if(!rowsNode)
            rowsNode = NewTextChild(paxLogNode, "rows");
        for( ; !Qry.Eof; Qry.Next()) {
            string ev_user = Qry.FieldAsString(col_ev_user);
            string station = Qry.FieldAsString(col_station);

            xmlNodePtr rowNode = NewTextChild(rowsNode, "row");
            NewTextChild(rowNode, "point_id", Qry.FieldAsInteger(col_point_id));
            NewTextChild( rowNode, "time",
                    DateTimeToStr(
                        UTCToClient( Qry.FieldAsDateTime(col_time), reqInfo->desk.tz_region),
                        ServerFormatDateTimeAsString
                        )
                    );
            NewTextChild(rowNode, "msg", Qry.FieldAsString(col_msg));
            NewTextChild(rowNode, "ev_order", Qry.FieldAsInteger(col_ev_order));
            NewTextChild(rowNode, "part_num", Qry.FieldAsInteger(col_part_num), 1);
            NewTextChild(rowNode, "ev_user", ev_user, "");
            NewTextChild(rowNode, "station", station, "");
            string screen = Qry.FieldAsString(col_screen);
            if(screen.size()) {
                if(screen_map.find(screen) == screen_map.end()) {
                    TQuery Qry(&OraSession);
                    Qry.SQLText = "select name from screen where exe = :exe";
                    Qry.CreateVariable("exe", otString, screen);
                    Qry.Execute();
                    if(Qry.Eof) throw Exception("FltLogRun: screen name fetch failed for " + screen);
                    screen_map[screen] = getLocaleText(Qry.FieldAsString(0));
                }
                screen = screen_map[screen];
            }
            NewTextChild(rowNode, "screen", screen, "");

            count++;
        }
    }
    if(!count)
        throw AstraLocale::UserException("MSG.OPERATIONS_NOT_FOUND");
}

void StatInterface::FltLogRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqInfo = TReqInfo::Instance();
    if (!reqInfo->user.access.rights().permitted(650))
        throw AstraLocale::UserException("MSG.FLT_LOG.VIEW_DENIED");
    xmlNodePtr paramNode = reqNode->children;
    int point_id = NodeAsIntegerFast("point_id", paramNode);
    TDateTime part_key;
    xmlNodePtr partKeyNode = GetNodeFast("part_key", paramNode);
    if(partKeyNode == NULL)
        part_key = NoExists;
    else
        part_key = NodeAsDateTime(partKeyNode);
    xmlNodePtr client_with_trip_col_in_SysLogNode = GetNodeFast("client_with_trip_col_in_SysLog", paramNode);
    if(client_with_trip_col_in_SysLogNode == NULL)
        get_compatible_report_form("ArxPaxLog", reqNode, resNode);
    else
        get_compatible_report_form("FltLog", reqNode, resNode);
    STAT::set_variables(resNode);
    xmlNodePtr variablesNode = GetNode("form_data/variables", resNode);
    NewTextChild(variablesNode, "report_title", getLocaleText("Журнал операций рейса"));
    TQuery Qry(&OraSession);
    int count = 0;

    xmlNodePtr paxLogNode = NewTextChild(resNode, "PaxLog");
    xmlNodePtr headerNode = NewTextChild(paxLogNode, "header");
    NewTextChild(headerNode, "col", "Агент"); // для совместимости со старой версией терминала

    Qry.Clear();
    string qry1, qry2;
    int move_id = 0;
    string airline;
    if (part_key == NoExists) {
        {
            TQuery Qry(&OraSession);
            Qry.SQLText = "select move_id, airline from points where point_id = :point_id "; // pr_del>=0 - не надо т.к. можно просматривать удаленные рейсы
            Qry.CreateVariable("point_id", otInteger, point_id);
            Qry.Execute();
            if(Qry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.MOVED_TO_ARX_OR_DEL.SELECT_AGAIN");
            move_id = Qry.FieldAsInteger("move_id");
            airline = Qry.FieldAsString("airline");
        }
        qry1 =
            "SELECT msg, time,  "
            "       id1 AS point_id,  "
            "       events_bilingual.screen,  "
            "       DECODE(type,:evtPax,id2,:evtPay,id2,NULL) AS reg_no,  "
            "       DECODE(type,:evtPax,id3,:evtPay,id3,NULL) AS grp_id,  "
            "       ev_user, station, ev_order, NVL(part_num, 1) AS part_num  "
            "FROM events_bilingual  "
            "WHERE "
            "   events_bilingual.lang = :lang AND  "
            "   events_bilingual.type IN (:evtFlt,:evtGraph,:evtPax,:evtPay,:evtTlg,:evtPrn) AND  "
            "   events_bilingual.id1=:point_id  ";
        qry2 =
            "SELECT msg, time,  "
            "       id2 AS point_id,  "
            "       events_bilingual.screen,  "
            "       NULL AS reg_no,  "
            "       NULL AS grp_id,  "
            "       ev_user, station, ev_order, NVL(part_num, 1) AS part_num  "
            "FROM events_bilingual  "
            "WHERE "
            "   events_bilingual.lang = :lang AND  "
            "   events_bilingual.type IN (:evtDisp) AND  "
            "   events_bilingual.id1=:move_id  ";
    } else {
        if(ARX_EVENTS_DISABLED())
            throw UserException("MSG.ERR_MSG.ARX_EVENTS_DISABLED");
        {
            TQuery Qry(&OraSession);
            Qry.SQLText =
                "select move_id, airline from arx_points "
                "where part_key = :part_key and point_id = :point_id "; // pr_del >= 0 - не надо, т.к. в архиве нет удаленных рейсов
            Qry.CreateVariable("part_key", otDate, part_key);
            Qry.CreateVariable("point_id", otInteger, point_id);
            Qry.Execute();
            if(Qry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND");
            move_id = Qry.FieldAsInteger("move_id");
            airline = Qry.FieldAsString("airline");
        }
        qry1 =
            "SELECT msg, time,  "
            "       id1 AS point_id,  "
            "       arx_events.screen,  "
            "       DECODE(type,:evtPax,id2,:evtPay,id2,NULL) AS reg_no,  "
            "       DECODE(type,:evtPax,id3,:evtPay,id3,NULL) AS grp_id,  "
            "       ev_user, station, ev_order, NVL(part_num, 1) AS part_num  "
            "FROM arx_events  "
            "WHERE "
            "   arx_events.part_key = :part_key and "
            "   (arx_events.lang = :lang OR arx_events.lang = :lang_undef) and "
            "   arx_events.type IN (:evtFlt,:evtGraph,:evtPax,:evtPay,:evtTlg,:evtPrn) AND  "
            "   arx_events.id1=:point_id  ";
        qry2 =
            "SELECT msg, time,  "
            "       id2 AS point_id,  "
            "       arx_events.screen,  "
            "       NULL AS reg_no,  "
            "       NULL AS grp_id,  "
            "       ev_user, station, ev_order, NVL(part_num, 1) AS part_num  "
            "FROM arx_events  "
            "WHERE "
            "      arx_events.part_key = :part_key and "
            "      (arx_events.lang = :lang OR arx_events.lang = :lang_undef) and "
            "      arx_events.type IN (:evtDisp) AND "
            "      arx_events.id1=:move_id  ";
    }
    NewTextChild(resNode, "airline", airline);

    TPerfTimer tm;
    tm.Init();
    xmlNodePtr rowsNode = NULL;
    for(int i = 0; i < 2; i++) {
        Qry.Clear();
        if(i == 0) {
            Qry.SQLText = qry1;
            Qry.CreateVariable("point_id", otInteger, point_id);
            Qry.CreateVariable("evtFlt",otString,EncodeEventType(ASTRA::evtFlt));
            Qry.CreateVariable("evtGraph",otString,EncodeEventType(ASTRA::evtGraph));
            Qry.CreateVariable("evtPax",otString,EncodeEventType(ASTRA::evtPax));
            Qry.CreateVariable("evtPay",otString,EncodeEventType(ASTRA::evtPay));
            Qry.CreateVariable("evtTlg",otString,EncodeEventType(ASTRA::evtTlg));
            Qry.CreateVariable("evtPrn",otString,EncodeEventType(ASTRA::evtPrn));
        } else {
            Qry.SQLText = qry2;
            Qry.CreateVariable("move_id", otInteger, move_id);
            Qry.CreateVariable("evtDisp",otString,EncodeEventType(ASTRA::evtDisp));
        }
        Qry.CreateVariable("lang", otString, TReqInfo::Instance()->desk.lang);
        if(part_key != NoExists) {
            Qry.CreateVariable("part_key", otDate, part_key);
            Qry.CreateVariable("lang_undef", otString, "ZZ");
        }
        try {
            Qry.Execute();
        } catch (EOracleError &E) {
            if(E.Code == 376)
                throw AstraLocale::UserException("MSG.ONE_OF_DB_FILES_UNAVAILABLE.CALL_ADMIN");
            else
                throw;
        }

        if(Qry.Eof && part_key == NoExists) {
            TQuery Qry(&OraSession);
            Qry.SQLText = "select point_id from points where point_id = :point_id";
            Qry.CreateVariable("point_id", otInteger, point_id);
            Qry.Execute();
            if(Qry.Eof)
                throw AstraLocale::UserException("MSG.FLIGHT.MOVED_TO_ARX_OR_DEL.SELECT_AGAIN");
        }

        typedef map<string, string> TScreenMap;
        TScreenMap screen_map;
        if(!Qry.Eof) {
            int col_point_id=Qry.FieldIndex("point_id");
            int col_ev_user=Qry.FieldIndex("ev_user");
            int col_station=Qry.FieldIndex("station");
            int col_time=Qry.FieldIndex("time");
            int col_grp_id=Qry.FieldIndex("grp_id");
            int col_reg_no=Qry.FieldIndex("reg_no");
            int col_msg=Qry.FieldIndex("msg");
            int col_ev_order=Qry.FieldIndex("ev_order");
            int col_part_num=Qry.FieldIndex("part_num");
            int col_screen=Qry.FieldIndex("screen");

            if(!rowsNode)
                rowsNode = NewTextChild(paxLogNode, "rows");
            for( ; !Qry.Eof; Qry.Next()) {
                string ev_user = Qry.FieldAsString(col_ev_user);
                string station = Qry.FieldAsString(col_station);

                xmlNodePtr rowNode = NewTextChild(rowsNode, "row");
                NewTextChild(rowNode, "point_id", Qry.FieldAsInteger(col_point_id));
                NewTextChild( rowNode, "time",
                        DateTimeToStr(
                            UTCToClient( Qry.FieldAsDateTime(col_time), reqInfo->desk.tz_region),
                            ServerFormatDateTimeAsString
                            )
                        );
                NewTextChild(rowNode, "msg", Qry.FieldAsString(col_msg));
                NewTextChild(rowNode, "ev_order", Qry.FieldAsInteger(col_ev_order));
                NewTextChild(rowNode, "part_num", Qry.FieldAsInteger(col_part_num), 1);
                if(!Qry.FieldIsNULL(col_grp_id))
                    NewTextChild(rowNode, "grp_id", Qry.FieldAsInteger(col_grp_id));
                if(!Qry.FieldIsNULL(col_reg_no))
                    NewTextChild(rowNode, "reg_no", Qry.FieldAsInteger(col_reg_no));
                NewTextChild(rowNode, "ev_user", ev_user, "");
                NewTextChild(rowNode, "station", station, "");
                string screen = Qry.FieldAsString(col_screen);
                if(screen.size()) {
                    if(screen_map.find(screen) == screen_map.end()) {
                        TQuery Qry(&OraSession);
                        Qry.SQLText = "select name from screen where exe = :exe";
                        Qry.CreateVariable("exe", otString, screen);
                        Qry.Execute();
                        if(Qry.Eof) throw Exception("FltLogRun: screen name fetch failed for " + screen);
                        screen_map[screen] = getLocaleText(Qry.FieldAsString(0));
                    }
                    screen = screen_map[screen];
                }
                NewTextChild(rowNode, "screen", screen, "");

                count++;
            }
        }
    }
    if(!count)
        throw AstraLocale::UserException("MSG.OPERATIONS_NOT_FOUND");
}

void StatInterface::LogRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr paramNode = reqNode->children;
    int point_id = NodeAsIntegerFast("point_id", paramNode);
    int reg_no = NodeAsIntegerFast("reg_no", paramNode);
    int grp_id = NodeAsIntegerFast("grp_id", paramNode);
    TDateTime part_key;
    xmlNodePtr partKeyNode = GetNodeFast("part_key", paramNode);
    if(partKeyNode == NULL)
        part_key = NoExists;
    else
        part_key = NodeAsDateTime(partKeyNode);

    xmlNodePtr client_with_trip_col_in_SysLogNode = GetNode("client_with_trip_col_in_SysLog", reqNode);
    if(client_with_trip_col_in_SysLogNode == NULL)
        get_compatible_report_form("ArxPaxLog", reqNode, resNode);
    else
        get_compatible_report_form("FltLog", reqNode, resNode);
    STAT::set_variables(resNode);
    xmlNodePtr variablesNode = GetNode("form_data/variables", resNode);
    NewTextChild(variablesNode, "report_title", getLocaleText("Операции по пассажиру"));
    TReqInfo *reqInfo = TReqInfo::Instance();
    TQuery Qry(&OraSession);
    int count = 0;

    xmlNodePtr paxLogNode = NewTextChild(resNode, "PaxLog");
    xmlNodePtr headerNode = NewTextChild(paxLogNode, "header");
    NewTextChild(headerNode, "col", "Агент"); // Для совместимости со старой версией терминала
    Qry.Clear();
    TQuery AirlineQry(&OraSession);
    AirlineQry.CreateVariable("point_id", otInteger, point_id);
    if (part_key == NoExists) {
        AirlineQry.SQLText = "select airline from points where point_id = :point_id and pr_del >= 0";
        Qry.SQLText =
            "SELECT msg, time, id1 AS point_id, null as screen, id2 AS reg_no, id3 AS grp_id, "
            "       ev_user, station, ev_order, NVL(part_num, 1) AS part_num "
            "FROM events_bilingual "
            "WHERE "
            "      lang = :lang AND "
            "      type IN (:evtPax,:evtPay) AND "
            "      screen <> 'ASTRASERV.EXE' and "
            "      id1=:point_id AND "
            "      (id2 IS NULL OR id2=:reg_no) AND "
            "      (id3 IS NULL OR id3=:grp_id) ";
    } else {
        if(ARX_EVENTS_DISABLED())
            throw UserException("MSG.ERR_MSG.ARX_EVENTS_DISABLED");
        AirlineQry.SQLText = "select airline from arx_points where point_id = :point_id and part_key = :part_key and pr_del >= 0";
        AirlineQry.CreateVariable("part_key", otDate, part_key);
        Qry.SQLText =
            "SELECT msg, time, id1 AS point_id, null as screen, id2 AS reg_no, id3 AS grp_id, "
            "       ev_user, station, ev_order, NVL(part_num, 1) AS part_num "
            "FROM arx_events "
            "WHERE part_key = :part_key AND "
            "      (lang = :lang OR lang = :lang_undef) AND "
            "      type IN (:evtPax,:evtPay) AND "
            "      screen <> 'ASTRASERV.EXE' and "
            "      id1=:point_id AND "
            "      (id2 IS NULL OR id2=:reg_no) AND "
            "      (id3 IS NULL OR id3=:grp_id) ";
        Qry.CreateVariable("part_key", otDate, part_key);
        Qry.CreateVariable("lang_undef", otString, "ZZ");
    }

    Qry.CreateVariable("lang", otString, TReqInfo::Instance()->desk.lang);
    Qry.CreateVariable("evtPax",otString,EncodeEventType(ASTRA::evtPax));
    Qry.CreateVariable("evtPay",otString,EncodeEventType(ASTRA::evtPay));
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("reg_no", otInteger, reg_no);
    Qry.CreateVariable("grp_id", otInteger, grp_id);

    TPerfTimer tm;
    tm.Init();
    try {
        Qry.Execute();
    } catch (EOracleError &E) {
        if(E.Code == 376)
            throw AstraLocale::UserException("MSG.ONE_OF_DB_FILES_UNAVAILABLE.CALL_ADMIN");
        else
            throw;
    }

    if(Qry.Eof && part_key == NoExists) {
        TQuery Qry(&OraSession);
        Qry.SQLText = "select point_id from points where point_id = :point_id and pr_del >= 0";
        Qry.CreateVariable("point_id", otInteger, point_id);
        Qry.Execute();
        if(Qry.Eof)
            throw AstraLocale::UserException("MSG.FLIGHT.MOVED_TO_ARX_OR_DEL.SELECT_AGAIN");
    }

    AirlineQry.Execute();
    if(AirlineQry.Eof)
        throw Exception("Cannot fetch airline");
    NewTextChild(resNode, "airline", AirlineQry.FieldAsString("airline"));

    if(!Qry.Eof) {
        int col_point_id=Qry.FieldIndex("point_id");
        int col_ev_user=Qry.FieldIndex("ev_user");
        int col_station=Qry.FieldIndex("station");
        int col_time=Qry.FieldIndex("time");
        int col_grp_id=Qry.FieldIndex("grp_id");
        int col_reg_no=Qry.FieldIndex("reg_no");
        int col_msg=Qry.FieldIndex("msg");
        int col_ev_order=Qry.FieldIndex("ev_order");
        int col_part_num=Qry.FieldIndex("part_num");
        int col_screen=Qry.FieldIndex("screen");

        xmlNodePtr rowsNode = NewTextChild(paxLogNode, "rows");
        for( ; !Qry.Eof; Qry.Next()) {
            string ev_user = Qry.FieldAsString(col_ev_user);
            string station = Qry.FieldAsString(col_station);

            xmlNodePtr rowNode = NewTextChild(rowsNode, "row");
            NewTextChild(rowNode, "point_id", Qry.FieldAsInteger(col_point_id));
            NewTextChild( rowNode, "time",
                    DateTimeToStr(
                        UTCToClient( Qry.FieldAsDateTime(col_time), reqInfo->desk.tz_region),
                        ServerFormatDateTimeAsString
                        )
                    );
            NewTextChild(rowNode, "msg", Qry.FieldAsString(col_msg));
            NewTextChild(rowNode, "ev_order", Qry.FieldAsInteger(col_ev_order));
            NewTextChild(rowNode, "part_num", Qry.FieldAsInteger(col_part_num), 1);
            if(!Qry.FieldIsNULL(col_grp_id))
                NewTextChild(rowNode, "grp_id", Qry.FieldAsInteger(col_grp_id));
            if(!Qry.FieldIsNULL(col_reg_no))
                NewTextChild(rowNode, "reg_no", Qry.FieldAsInteger(col_reg_no));
            NewTextChild(rowNode, "ev_user", ev_user, "");
            NewTextChild(rowNode, "station", station, "");
            NewTextChild(rowNode, "screen", Qry.FieldAsString(col_screen), "");

            count++;
            if(count >= MAX_STAT_ROWS()) {
                AstraLocale::showErrorMessage("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_SEARCH",
                        LParams() << LParam("num", MAX_STAT_ROWS()));
                break;
            }
        }
    }
    if(!count)
        throw AstraLocale::UserException("MSG.OPERATIONS_NOT_FOUND");
}

typedef struct {
    string trip, scd_out;
} TTripItem;

void StatInterface::SystemLogRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqInfo = TReqInfo::Instance();
    if (!reqInfo->user.access.rights().permitted(655))
        throw AstraLocale::UserException("MSG.SYS_LOG.VIEW_DENIED");

    if(NodeAsDateTime("FirstDate", reqNode)+1 < NodeAsDateTime("LastDate", reqNode))
      throw AstraLocale::UserException("MSG.SEARCH_PERIOD_SHOULD_NOT_EXCEED_ONE_DAY");

    xmlNodePtr client_with_trip_col_in_SysLogNode = GetNode("client_with_trip_col_in_SysLog", reqNode);
    if(client_with_trip_col_in_SysLogNode == NULL)
        get_compatible_report_form("ArxPaxLog", reqNode, resNode);
    else
        get_compatible_report_form("SystemLog", reqNode, resNode);
    STAT::set_variables(resNode);
    xmlNodePtr variablesNode = GetNode("form_data/variables", resNode);
    NewTextChild(variablesNode, "report_title", getLocaleText("Операции в системе"));
    string module;

    TQuery Qry(&OraSession);
    xmlNodePtr moduleNode = GetNode("module", reqNode);
    if(not moduleNode)
        ;
    else if(NodeIsNULL(moduleNode))
        module = SYSTEM_USER;
    else {
        Qry.SQLText = "select exe from screen where id = :module";
        Qry.CreateVariable("module", otInteger, NodeAsInteger(moduleNode));
        Qry.Execute();
        if(!Qry.Eof) module = Qry.FieldAsString("exe");
    }

    string agent, station;
    xmlNodePtr agentNode = GetNode("agent", reqNode);
    if(not agentNode)
        ;
    else if(NodeIsNULL(agentNode))
        agent = SYSTEM_USER;
    else
        agent = NodeAsString(agentNode);

    xmlNodePtr stationNode = GetNode("station", reqNode);
    if(not stationNode)
        ;
    else if(NodeIsNULL(stationNode))
        station = SYSTEM_USER;
    else
        station = NodeAsString(stationNode);

    ProgTrace(TRACE5, "module: '%s'", module.c_str());
    ProgTrace(TRACE5, "agent: '%s'", agent.c_str());
    ProgTrace(TRACE5, "station: '%s'", station.c_str());

    int count = 0;

    xmlNodePtr paxLogNode = NewTextChild(resNode, "PaxLog");
    xmlNodePtr headerNode = NewTextChild(paxLogNode, "header");
    NewTextChild(headerNode, "col", "Агент"); // для совместимости со старой версией терминала

    map<int, string> TripItems;
    xmlNodePtr rowsNode = NULL;
    for(int j = 0; j < 2; j++) {
        Qry.Clear();
        if (j==0) {
            Qry.SQLText =
                "SELECT msg, time, "
                "       DECODE(type, :evtFlt, id1, :evtFltTask, id1, :evtPax, id1, :evtPay, id1, :evtGraph, id1, :evtTlg, id1, "
                "                    :evtDisp, id2, NULL) AS point_id, "
                "       screen, "
                "       DECODE(type,:evtPax,id2,:evtPay,id2,NULL) AS reg_no, "
                "       DECODE(type,:evtPax,id3,:evtPay,id3,NULL) AS grp_id, "
                "  ev_user, station, ev_order, NVL(part_num, 1) AS part_num, null part_key "
                "FROM "
                "  events_bilingual "
                "WHERE "
                "  events_bilingual.lang = :lang AND "
                "  events_bilingual.time >= :FirstDate and "
                "  events_bilingual.time < :LastDate and "
                "  (:agent is null or nvl(ev_user, :system_user) = :agent) and "
                "  (:module is null or nvl(screen, :system_user) = :module) and "
                "  (:station is null or nvl(station, :system_user) = :station) and "
                "  events_bilingual.type IN ( "
                "    :evtFlt, "
                "    :evtFltTask, "
                "    :evtPax, "
                "    :evtPay, "
                "    :evtGraph, "
                "    :evtTlg, "
                "    :evtComp, "
                "    :evtAccess, "
                "    :evtSystem, "
                "    :evtCodif, "
                "    :evtSeason, "
                "    :evtDisp, "
                "    :evtPeriod "
                "          ) ";
        } else {
            if(ARX_EVENTS_DISABLED())
                throw UserException("MSG.ERR_MSG.ARX_EVENTS_DISABLED");
            Qry.SQLText =
                "SELECT msg, time, "
                "       DECODE(type, :evtFlt, id1, :evtFltTask, id1, :evtPax, id1, :evtPay, id1, :evtGraph, id1, :evtTlg, id1, "
                "                    :evtDisp, id2, NULL) AS point_id, "
                "       screen, "
                "       DECODE(type,:evtPax,id2,:evtPay,id2,NULL) AS reg_no, "
                "       DECODE(type,:evtPax,id3,:evtPay,id3,NULL) AS grp_id, "
                "  ev_user, station, ev_order, NVL(part_num, 1) AS part_num, part_key "
                "FROM "
                "   arx_events "
                "WHERE "
                "  arx_events.part_key >= :FirstDate - 10 and " // time и part_key не совпадают для
                "  arx_events.part_key < :LastDate + 10 and "   // разных типов событий
                "  (arx_events.lang = :lang OR arx_events.lang = :lang_undef) and "
                "  arx_events.time >= :FirstDate and "         // поэтому для part_key берем больший диапазон time
                "  arx_events.time < :LastDate and "
                "  (:agent is null or nvl(ev_user, :system_user) = :agent) and "
                "  (:module is null or nvl(screen, :system_user) = :module) and "
                "  (:station is null or nvl(station, :system_user) = :station) and "
                "  arx_events.type IN ( "
                "    :evtFlt, "
                "    :evtFltTask, "
                "    :evtPax, "
                "    :evtPay, "
                "    :evtGraph, "
                "    :evtTlg, "
                "    :evtComp, "
                "    :evtAccess, "
                "    :evtSystem, "
                "    :evtCodif, "
                "    :evtSeason, "
                "    :evtDisp, "
                "    :evtPeriod "
                "          ) ";
            Qry.CreateVariable("lang_undef", otString, "ZZ");
        }

        Qry.CreateVariable("lang", otString, TReqInfo::Instance()->desk.lang);
        Qry.CreateVariable("evtFlt", otString, NodeAsString("evtFlt", reqNode));
        Qry.CreateVariable("evtPax", otString, NodeAsString("evtPax", reqNode));
        Qry.CreateVariable("system_user", otString, SYSTEM_USER);
        {
            xmlNodePtr node = GetNode("evtFltTask", reqNode);
            string evtFltTask;
            if(node)
                evtFltTask = NodeAsString(node);
            Qry.CreateVariable("evtFltTask", otString, evtFltTask);
        }
        {
            xmlNodePtr node = GetNode("evtPay", reqNode);
            string evtPay;
            if(node)
                evtPay = NodeAsString(node);
            Qry.CreateVariable("evtPay", otString, evtPay);
        }
        {
            xmlNodePtr node = GetNode("evtDisp", reqNode);
            string evtDisp;
            if(node)
                evtDisp = NodeAsString(node);
            Qry.CreateVariable("evtDisp", otString, evtDisp);
        }
        {
            xmlNodePtr node = GetNode("evtSeason", reqNode);
            string evtSeason;
            if(node)
                evtSeason = NodeAsString(node);
            Qry.CreateVariable("evtSeason", otString, evtSeason);
        }
        Qry.CreateVariable("evtGraph", otString, NodeAsString("evtGraph", reqNode));
        Qry.CreateVariable("evtTlg", otString, NodeAsString("evtTlg", reqNode));
        Qry.CreateVariable("evtComp", otString, NodeAsString("evtComp", reqNode));
        Qry.CreateVariable("evtAccess", otString, NodeAsString("evtAccess", reqNode));
        Qry.CreateVariable("evtSystem", otString, NodeAsString("evtSystem", reqNode));
        Qry.CreateVariable("evtCodif", otString, NodeAsString("evtCodif", reqNode));
        Qry.CreateVariable("evtPeriod", otString, NodeAsString("evtPeriod", reqNode));

        Qry.CreateVariable("FirstDate", otDate, NodeAsDateTime("FirstDate", reqNode));
        Qry.CreateVariable("LastDate", otDate, NodeAsDateTime("LastDate", reqNode));
        Qry.CreateVariable("agent", otString, agent);
        Qry.CreateVariable("station", otString, station);
        Qry.CreateVariable("module", otString, module);

        TPerfTimer tm;
        tm.Init();
        try {
            Qry.Execute();
        } catch (EOracleError &E) {
            if(E.Code == 376)
                throw AstraLocale::UserException("MSG.ONE_OF_DB_FILES_UNAVAILABLE.CALL_ADMIN");
            else
                throw;
        }
        ProgTrace(TRACE5, "SystemLogRun%d EXEC QRY: %s, Qry.Eof: %d", j, tm.PrintWithMessage().c_str(), Qry.Eof);

        typedef map<string, bool> TAccessMap;
        TAccessMap user_access;
        TAccessMap desk_access;
        if(!Qry.Eof) {
            int col_point_id=Qry.FieldIndex("point_id");
            int col_ev_user=Qry.FieldIndex("ev_user");
            int col_station=Qry.FieldIndex("station");
            int col_time=Qry.FieldIndex("time");
            int col_grp_id=Qry.FieldIndex("grp_id");
            int col_reg_no=Qry.FieldIndex("reg_no");
            int col_msg=Qry.FieldIndex("msg");
            int col_ev_order=Qry.FieldIndex("ev_order");
            int col_part_num=Qry.FieldIndex("part_num");
            int col_screen=Qry.FieldIndex("screen");
            int col_part_key=Qry.FieldIndex("part_key");

            if(not rowsNode)
                rowsNode = NewTextChild(paxLogNode, "rows");
            for( ; !Qry.Eof; Qry.Next()) {
                string ev_user = Qry.FieldAsString(col_ev_user);
                string station = Qry.FieldAsString(col_station);

                if(ev_user != "") {
                    if(user_access.find(ev_user) == user_access.end()) {
                        TQuery Qry(&OraSession);
                        Qry.SQLText =
                            "select descr from users2 where "
                            "   (user_id = :SYS_user_id or adm.check_user_access(user_id,:SYS_user_id)<>0) and "
                            "   descr = :ev_user";
                        Qry.CreateVariable("ev_user", otString, ev_user);
                        Qry.CreateVariable("SYS_user_id", otInteger, reqInfo->user.user_id);
                        Qry.Execute();
                        user_access[ev_user] = !Qry.Eof;
                    }
                    if(!user_access[ev_user]) continue;
                }

                if(station != "") {
                    if(desk_access.find(station) == desk_access.end()) {
                        TQuery Qry(&OraSession);
                        Qry.SQLText =
                            "select code from desks where "
                            "   adm.check_desk_view_access(code, :SYS_user_id) <> 0 and "
                            "   code = :station ";
                        Qry.CreateVariable("station", otString, station);
                        Qry.CreateVariable("SYS_user_id", otInteger, reqInfo->user.user_id);
                        Qry.Execute();
                        desk_access[station] = !Qry.Eof;
                    }
                    if(!desk_access[station]) continue;
                }

                xmlNodePtr rowNode = NewTextChild(rowsNode, "row");
                NewTextChild(rowNode, "point_id", Qry.FieldAsInteger(col_point_id));
                NewTextChild( rowNode, "time",
                        DateTimeToStr(
                            UTCToClient( Qry.FieldAsDateTime(col_time), reqInfo->desk.tz_region),
                            ServerFormatDateTimeAsString
                            )
                        );
                NewTextChild(rowNode, "msg", Qry.FieldAsString(col_msg));
                NewTextChild(rowNode, "ev_order", Qry.FieldAsInteger(col_ev_order));
                NewTextChild(rowNode, "part_num", Qry.FieldAsInteger(col_part_num), 1);
                if(!Qry.FieldIsNULL(col_point_id)) {
                    int point_id = Qry.FieldAsInteger(col_point_id);
                    if(TripItems.find(point_id) == TripItems.end()) {
                        TQuery tripQry(&OraSession);
                        string SQLText =
                            "select "
                            "   airline, "
                            "   flt_no, "
                            "   suffix, "
                            "   airp, "
                            "   scd_out, "
                            "   airp_fmt, "
                            "   airline_fmt, "
                            "   suffix_fmt, "
                            "   NVL(act_out,NVL(est_out,scd_out)) AS real_out, "
                            "   pr_del "
                            "from ";
                        SQLText += (j == 0 ? "points" : "arx_points");
                        SQLText +=
                            " where "
                            "   point_id = :point_id ";
                        if(j == 1) {
                            SQLText += " and part_key = :part_key ";
                            tripQry.CreateVariable("part_key", otDate,  Qry.FieldAsDateTime(col_part_key));
                        }
                        tripQry.SQLText = SQLText;
                        tripQry.CreateVariable("point_id", otInteger,  point_id);
                        tripQry.Execute();
                        if(tripQry.Eof)
                            TripItems[point_id]; // записываем пустую строку для данного point_id
                        else {
                            if(tripQry.FieldIsNULL("flt_no")) { // если нет инфы по номеру рейса, ничего не выводим
                                TripItems[point_id]; // записываем пустую строку для данного point_id
                            } else {
                                TTripInfo trip_info(tripQry);
                                TripItems[point_id] = GetTripName(trip_info, ecCkin);
                            }
                        }
                    }
                    NewTextChild(rowNode, "trip", TripItems[point_id]);
                }
                if(!Qry.FieldIsNULL(col_grp_id))
                    NewTextChild(rowNode, "grp_id", Qry.FieldAsInteger(col_grp_id));
                if(!Qry.FieldIsNULL(col_reg_no))
                    NewTextChild(rowNode, "reg_no", Qry.FieldAsInteger(col_reg_no));
                NewTextChild(rowNode, "ev_user", ev_user, "");
                NewTextChild(rowNode, "station", station, "");
                NewTextChild(rowNode, "screen", Qry.FieldAsString(col_screen), "");

                count++;
                if(count >= MAX_STAT_ROWS()) {
                    AstraLocale::showErrorMessage("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_SEARCH",
                            LParams() << LParam("num", MAX_STAT_ROWS()));
                    break;
                }
            }
        }
        ProgTrace(TRACE5, "FORM XML2: %s", tm.PrintWithMessage().c_str());
    }
    if(!count)
        throw AstraLocale::UserException("MSG.OPERATIONS_NOT_FOUND");
}

struct THallItem {
    int id;
    string name;
};

void UnaccompListToXML(TQuery &Qry, xmlNodePtr resNode, TExcessNodeList &excessNodeList, bool isPaxSearch, int pass, int &count)
{
  if(Qry.Eof) return;

  xmlNodePtr paxListNode = GetNode("paxList", resNode);
  if (paxListNode==NULL)
    paxListNode = NewTextChild(resNode, "paxList");

  xmlNodePtr rowsNode = GetNode("rows", paxListNode);
  if (rowsNode==NULL)
    rowsNode = NewTextChild(paxListNode, "rows");

  int col_point_id = Qry.FieldIndex("point_id");
  int col_scd_out = Qry.FieldIndex("scd_out");
  int col_bag_amount = Qry.FieldIndex("bag_amount");
  int col_bag_weight = Qry.FieldIndex("bag_weight");
  int col_rk_weight = Qry.FieldIndex("rk_weight");
  int col_excess = Qry.FieldIndex("excess");
  int col_piece_concept = Qry.FieldIndex("piece_concept");
  int col_grp_id = Qry.FieldIndex("grp_id");
  int col_airp_arv = Qry.FieldIndex("airp_arv");
  int col_tags = Qry.FieldIndex("tags");
  int col_hall = Qry.FieldIndex("hall");
  int col_part_key=Qry.FieldIndex("part_key");

  map<int, TTripItem> TripItems;
  TPerfTimer tm;
  tm.Init();
  for(;!Qry.Eof;Qry.Next())
  {
      xmlNodePtr paxNode=NewTextChild(rowsNode,"pax");

      int point_id = Qry.FieldAsInteger(col_point_id);
      TDateTime part_key=NoExists;
      if(!Qry.FieldIsNULL(col_part_key)) part_key=Qry.FieldAsDateTime(col_part_key);

      if(part_key!=NoExists)
          NewTextChild(paxNode, "part_key",
                  DateTimeToStr(part_key, ServerFormatDateTimeAsString));
      NewTextChild(paxNode, "point_id", point_id);
      NewTextChild(paxNode, "airline");
      NewTextChild(paxNode, "flt_no", 0);
      NewTextChild(paxNode, "suffix");
      map<int, TTripItem>::iterator i=TripItems.find(point_id);
      if(i == TripItems.end())
      {
          TTripInfo trip_info(Qry);
          TTripItem trip_item;
          trip_item.trip = GetTripName(trip_info, ecCkin);
          trip_item.scd_out =
              DateTimeToStr(
                      UTCToClient( Qry.FieldAsDateTime(col_scd_out), TReqInfo::Instance()->desk.tz_region),
                      ServerFormatDateTimeAsString
                      );
          i=TripItems.insert(make_pair(point_id, trip_item)).first;
      };
      if(i == TripItems.end())
        throw Exception("UnaccompListToXML: i == TripItems.end()");

      NewTextChild(paxNode, "trip", i->second.trip);
      NewTextChild(paxNode, "scd_out", i->second.scd_out);
      NewTextChild(paxNode, "reg_no", 0);
      NewTextChild(paxNode, "full_name", getLocaleText("Багаж без сопровождения"));
      NewTextChild(paxNode, "bag_amount", Qry.FieldAsInteger(col_bag_amount));
      NewTextChild(paxNode, "bag_weight", Qry.FieldAsInteger(col_bag_weight));
      NewTextChild(paxNode, "rk_weight", Qry.FieldAsInteger(col_rk_weight));

      xmlNodePtr excessNode =  NewTextChild(paxNode, "excess", Qry.FieldAsInteger(col_excess));;
      excessNodeList.set_concept(excessNode,  Qry.FieldAsInteger(col_piece_concept));

      NewTextChild(paxNode, "grp_id", Qry.FieldAsInteger(col_grp_id));
      NewTextChild(paxNode, "airp_arv", ElemIdToCodeNative(etAirp, Qry.FieldAsString(col_airp_arv)));
      NewTextChild(paxNode, "tags", Qry.FieldAsString(col_tags));
      NewTextChild(paxNode, "status");
      NewTextChild(paxNode, "class");
      NewTextChild(paxNode, "seat_no");
      NewTextChild(paxNode, "document");
      NewTextChild(paxNode, "ticket_no");
      if(Qry.FieldIsNULL(col_hall))
          NewTextChild(paxNode, "hall");
      else
          NewTextChild(paxNode, "hall", ElemIdToNameLong(etHall, Qry.FieldAsInteger(col_hall)));

      if (isPaxSearch)
      {
        count++;
        if(count >= MAX_STAT_ROWS()) {
            AstraLocale::showErrorMessage("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_SEARCH",
                    LParams() << LParam("num", MAX_STAT_ROWS()));
            break;
        }
      };
  }
  ProgTrace(TRACE5, "XML%d: %s", pass, tm.PrintWithMessage().c_str());
};

void PaxListToXML(TQuery &Qry, xmlNodePtr resNode, TExcessNodeList& excessNodeList, bool isPaxSearch, int pass, int &count)
{
  if(Qry.Eof) return;

  xmlNodePtr paxListNode = GetNode("paxList", resNode);
  if (paxListNode==NULL)
    paxListNode = NewTextChild(resNode, "paxList");

  xmlNodePtr rowsNode = GetNode("rows", paxListNode);
  if (rowsNode==NULL)
    rowsNode = NewTextChild(paxListNode, "rows");

  int col_point_id = Qry.FieldIndex("point_id");
  int col_airline = Qry.FieldIndex("airline");
  int col_flt_no = Qry.FieldIndex("flt_no");
  int col_suffix = Qry.FieldIndex("suffix");
  int col_scd_out = Qry.FieldIndex("scd_out");
  int col_reg_no = Qry.FieldIndex("reg_no");
  int col_full_name = Qry.FieldIndex("full_name");
  int col_bag_amount = Qry.FieldIndex("bag_amount");
  int col_bag_weight = Qry.FieldIndex("bag_weight");
  int col_rk_weight = Qry.FieldIndex("rk_weight");
  int col_excess = Qry.FieldIndex("excess");
  int col_piece_concept = Qry.FieldIndex("piece_concept");
  int col_grp_id = Qry.FieldIndex("grp_id");
  int col_airp_arv = Qry.FieldIndex("airp_arv");
  int col_tags = Qry.FieldIndex("tags");
  int col_pr_brd = Qry.FieldIndex("pr_brd");
  int col_refuse = Qry.FieldIndex("refuse");
  int col_class_grp = Qry.FieldIndex("class_grp");
  int col_seat_no = Qry.FieldIndex("seat_no");
  int col_ticket_no = Qry.FieldIndex("ticket_no");
  int col_hall = Qry.FieldIndex("hall");
  int col_part_key=Qry.FieldIndex("part_key");
  int col_pax_id=Qry.FieldIndex("pax_id");
  int col_status=Qry.FieldIndex("status");

  map<int, TTripItem> TripItems;

  TPerfTimer tm;
  tm.Init();
  for( ; !Qry.Eof; Qry.Next()) {
      xmlNodePtr paxNode = NewTextChild(rowsNode, "pax");

      int point_id = Qry.FieldAsInteger(col_point_id);
      TDateTime part_key=NoExists;
      if(!Qry.FieldIsNULL(col_part_key)) part_key=Qry.FieldAsDateTime(col_part_key);

      if(part_key!=NoExists)
          NewTextChild(paxNode, "part_key",
                  DateTimeToStr(part_key, ServerFormatDateTimeAsString));
      NewTextChild(paxNode, "point_id", point_id);
      NewTextChild(paxNode, "airline", Qry.FieldAsString(col_airline));
      NewTextChild(paxNode, "flt_no", Qry.FieldAsInteger(col_flt_no));
      NewTextChild(paxNode, "suffix", Qry.FieldAsString(col_suffix));
      map<int, TTripItem>::iterator i=TripItems.find(point_id);
      if(i == TripItems.end())
      {
          TTripInfo trip_info(Qry);
          TTripItem trip_item;
          trip_item.trip = GetTripName(trip_info, ecCkin);
          trip_item.scd_out =
              DateTimeToStr(
                      UTCToClient( Qry.FieldAsDateTime(col_scd_out), TReqInfo::Instance()->desk.tz_region),
                      ServerFormatDateTimeAsString
                      );
          i=TripItems.insert(make_pair(point_id, trip_item)).first;
      };
      if(i == TripItems.end())
        throw Exception("PaxListToXML: i == TripItems.end()");

      NewTextChild(paxNode, "trip", i->second.trip);
      NewTextChild(paxNode, "scd_out", i->second.scd_out);
      NewTextChild(paxNode, "reg_no", Qry.FieldAsInteger(col_reg_no));
      NewTextChild(paxNode, "full_name", Qry.FieldAsString(col_full_name));
      NewTextChild(paxNode, "bag_amount", Qry.FieldAsInteger(col_bag_amount));
      NewTextChild(paxNode, "bag_weight", Qry.FieldAsInteger(col_bag_weight));
      NewTextChild(paxNode, "rk_weight", Qry.FieldAsInteger(col_rk_weight));
      xmlNodePtr excessNode = NewTextChild(paxNode, "excess", Qry.FieldAsInteger(col_excess));
      excessNodeList.set_concept(excessNode,  Qry.FieldAsInteger(col_piece_concept));

      NewTextChild(paxNode, "grp_id", Qry.FieldAsInteger(col_grp_id));
      NewTextChild(paxNode, "airp_arv", ElemIdToCodeNative(etAirp, Qry.FieldAsString(col_airp_arv)));
      NewTextChild(paxNode, "tags", Qry.FieldAsString(col_tags));
      string status;
      if (DecodePaxStatus(Qry.FieldAsString(col_status))!=psCrew)
      {
        if(Qry.FieldIsNULL(col_refuse))
            status = getLocaleText(Qry.FieldAsInteger(col_pr_brd) == 0 ? "Зарег." : "Посаж.");
      }
      else
      {
        status = getLocaleText("Экипаж");
        if(!Qry.FieldIsNULL(col_refuse)) status += ". ";
      };

      if(!Qry.FieldIsNULL(col_refuse))
          status+= getLocaleText("MSG.CANCEL_REG.REFUSAL",
                   LParams() << LParam("refusal", ElemIdToCodeNative(etRefusalType, Qry.FieldAsString(col_refuse))));
      NewTextChild(paxNode, "status", status);
      NewTextChild(paxNode, "class", ElemIdToCodeNative(etClsGrp, Qry.FieldAsInteger(col_class_grp)));
      NewTextChild(paxNode, "seat_no", Qry.FieldAsString(col_seat_no));
      NewTextChild(paxNode, "document", CheckIn::GetPaxDocStr(part_key,
                                                              Qry.FieldAsInteger(col_pax_id),
                                                              true));
      NewTextChild(paxNode, "ticket_no", Qry.FieldAsString(col_ticket_no));
      if(Qry.FieldIsNULL(col_hall))
          NewTextChild(paxNode, "hall");
      else
          NewTextChild(paxNode, "hall", ElemIdToNameLong(etHall, Qry.FieldAsInteger(col_hall)));

      if (isPaxSearch)
      {
        count++;
        if(count >= MAX_STAT_ROWS()) {
            AstraLocale::showErrorMessage("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_SEARCH",
                    LParams() << LParam("num", MAX_STAT_ROWS()));
            break;
        }
      };
  }
  ProgTrace(TRACE5, "XML%d: %s", pass, tm.PrintWithMessage().c_str());
};

void StatInterface::PaxListRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo &info = *(TReqInfo::Instance());
    if (!info.user.access.rights().permitted(630))
        throw AstraLocale::UserException("MSG.PAX_LIST.VIEW_DENIED");
    if (info.user.access.totally_not_permitted())
        throw AstraLocale::UserException("MSG.PASSENGERS.NOT_FOUND");
    xmlNodePtr paramNode = reqNode->children;

    int point_id = NodeAsIntegerFast("point_id", paramNode);
    TDateTime part_key;
    xmlNodePtr partKeyNode = GetNodeFast("part_key", paramNode);
    if(partKeyNode == NULL)
        part_key = NoExists;
    else
        part_key = NodeAsDateTime(partKeyNode);
    get_compatible_report_form("ArxPaxList", reqNode, resNode);
    {
        TQuery Qry(&OraSession);
        string SQLText;
        if(part_key == NoExists)  {
            SQLText =
                "SELECT "
                "   null part_key, "
                "   pax_grp.point_dep point_id, "
                "   points.airline, "
                "   points.flt_no, "
                "   points.suffix, "
                "   points.airline_fmt, "
                "   points.airp_fmt, "
                "   points.suffix_fmt, "
                "   points.airp, "
                "   points.scd_out, "
                "   NVL(points.act_out,NVL(points.est_out,points.scd_out)) AS real_out, "
                "   pax.reg_no, "
                "   pax_grp.airp_arv, "
                "   pax.surname||' '||pax.name full_name, "
                "   NVL(ckin.get_bagAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) bag_amount, "
                "   NVL(ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) bag_weight, "
                "   NVL(ckin.get_rkWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) rk_weight, "
                "   NVL(ckin.get_excess(pax.grp_id,pax.pax_id),0) excess, "
                "   pax_grp.piece_concept, "
                "   pax_grp.grp_id, "
                "   ckin.get_birks2(pax.grp_id,pax.pax_id,pax.bag_pool_num,:pr_lat) tags, "
                "   pax.refuse, "
                "   pax.pr_brd, "
                "   pax_grp.class_grp, "
                "   salons.get_seat_no(pax.pax_id, pax.seats, pax.is_jmp, pax_grp.status, pax_grp.point_dep, 'seats', rownum) seat_no, "
                "   pax_grp.hall, "
                "   pax.ticket_no, "
                "   pax.pax_id, "
                "   pax_grp.status "
                "FROM  pax_grp,pax, points "
                "WHERE "
                "   points.point_id = :point_id and points.pr_del>=0 and "
                "   points.point_id = pax_grp.point_dep and "
                "   pax_grp.grp_id=pax.grp_id ";
            if (!info.user.access.airps().elems().empty()) {
                if (info.user.access.airps().elems_permit())
                    SQLText += " AND points.airp IN "+GetSQLEnum(info.user.access.airps().elems());
                else
                    SQLText += " AND points.airp NOT IN "+GetSQLEnum(info.user.access.airps().elems());
            }
            if (!info.user.access.airlines().elems().empty()) {
                if (info.user.access.airlines().elems_permit())
                    SQLText += " AND points.airline IN "+GetSQLEnum(info.user.access.airlines().elems());
                else
                    SQLText += " AND points.airline NOT IN "+GetSQLEnum(info.user.access.airlines().elems());
            }
        } else {
            SQLText =
                "SELECT "
                "   arx_points.part_key, "
                "   arx_pax_grp.point_dep point_id, "
                "   arx_points.airline, "
                "   arx_points.flt_no, "
                "   arx_points.suffix, "
                "   arx_points.airp, "
                "   arx_points.scd_out, "
                "   arx_points.airline_fmt, "
                "   arx_points.airp_fmt, "
                "   arx_points.suffix_fmt, "
                "   NVL(arx_points.act_out,NVL(arx_points.est_out,arx_points.scd_out)) AS real_out, "
                "   arx_pax.reg_no, "
                "   arx_pax_grp.airp_arv, "
                "   arx_pax.surname||' '||arx_pax.name full_name, "
                "   NVL(arch.get_bagAmount2(arx_pax.part_key,arx_pax.grp_id,arx_pax.pax_id,arx_pax.bag_pool_num,rownum),0) bag_amount, "
                "   NVL(arch.get_bagWeight2(arx_pax.part_key,arx_pax.grp_id,arx_pax.pax_id,arx_pax.bag_pool_num,rownum),0) bag_weight, "
                "   NVL(arch.get_rkWeight2(arx_pax.part_key,arx_pax.grp_id,arx_pax.pax_id,arx_pax.bag_pool_num,rownum),0) rk_weight, "
                "   NVL(arch.get_excess(arx_pax.part_key,arx_pax.grp_id,arx_pax.pax_id),0) excess, "
                "   arx_pax_grp.piece_concept, "
                "   arx_pax_grp.grp_id, "
                "   arch.get_birks2(arx_pax.part_key,arx_pax.grp_id,arx_pax.pax_id,arx_pax.bag_pool_num,:pr_lat) tags, "
                "   arx_pax.refuse, "
                "   arx_pax.pr_brd, "
                "   arx_pax_grp.class_grp, "
                "   LPAD(seat_no,3,'0')|| "
                "       DECODE(SIGN(1-seats),-1,'+'||TO_CHAR(seats-1),'') seat_no, "
                "   arx_pax_grp.hall, "
                "   arx_pax.ticket_no, "
                "   arx_pax.pax_id, "
                "   arx_pax_grp.status "
                "FROM  arx_pax_grp,arx_pax, arx_points "
                "WHERE "
                "   arx_points.point_id = :point_id and arx_points.pr_del>=0 and "
                "   arx_points.part_key = arx_pax_grp.part_key and "
                "   arx_points.point_id = arx_pax_grp.point_dep and "
                "   arx_pax_grp.part_key=arx_pax.part_key AND "
                "   arx_pax_grp.grp_id=arx_pax.grp_id AND "
                "   arx_points.part_key = :part_key and "
                "   pr_brd IS NOT NULL  ";
            if (!info.user.access.airps().elems().empty()) {
                if (info.user.access.airps().elems_permit())
                    SQLText += " AND arx_points.airp IN "+GetSQLEnum(info.user.access.airps().elems());
                else
                    SQLText += " AND arx_points.airp NOT IN "+GetSQLEnum(info.user.access.airps().elems());
            }
            if (!info.user.access.airlines().elems().empty()) {
                if (info.user.access.airlines().elems_permit())
                    SQLText += " AND arx_points.airline IN "+GetSQLEnum(info.user.access.airlines().elems());
                else
                    SQLText += " AND arx_points.airline NOT IN "+GetSQLEnum(info.user.access.airlines().elems());
            }
            Qry.CreateVariable("part_key", otDate, part_key);
        }

        Qry.SQLText = SQLText;

        Qry.CreateVariable("point_id", otInteger, point_id);
        Qry.CreateVariable("pr_lat", otInteger, TReqInfo::Instance()->desk.lang!=AstraLocale::LANG_RU);

        TPerfTimer tm;
        tm.Init();
        Qry.Execute();
        ProgTrace(TRACE5, "Qry.Execute: %s", tm.PrintWithMessage().c_str());

        int count=0;
        TExcessNodeList excessNodeList;
        PaxListToXML(Qry, resNode, excessNodeList, false, 0, count);

        ProgTrace(TRACE5, "XML: %s", tm.PrintWithMessage().c_str());

        //несопровождаемый багаж
        if(part_key == NoExists)  {
            Qry.SQLText=
                "SELECT "
                "   null part_key, "
                "   pax_grp.point_dep point_id, "
                "   points.airline, "
                "   points.flt_no, "
                "   points.suffix, "
                "   points.airline_fmt, "
                "   points.airp_fmt, "
                "   points.suffix_fmt, "
                "   points.airp, "
                "   points.scd_out, "
                "   NVL(points.act_out,NVL(points.est_out,points.scd_out)) AS real_out, "
                "   pax_grp.airp_arv, "
                "   ckin.get_bagAmount2(pax_grp.grp_id,NULL,NULL) AS bag_amount, "
                "   ckin.get_bagWeight2(pax_grp.grp_id,NULL,NULL) AS bag_weight, "
                "   ckin.get_rkWeight2(pax_grp.grp_id,NULL,NULL) AS rk_weight, "
                "   ckin.get_excess(pax_grp.grp_id,NULL) AS excess, "
                "   pax_grp.piece_concept, "
                "   ckin.get_birks2(pax_grp.grp_id,NULL,NULL,:pr_lat) AS tags, "
                "   pax_grp.grp_id, "
                "   pax_grp.hall "
                "FROM pax_grp, points "
                "WHERE "
                "   pax_grp.point_dep=:point_id AND "
                "   pax_grp.class IS NULL and "
                "   pax_grp.status NOT IN ('E') AND "
                "   pax_grp.point_dep = points.point_id and points.pr_del>=0 ";
            if (!info.user.access.airps().elems().empty()) {
                if (info.user.access.airps().elems_permit())
                    SQLText += " AND points.airp IN "+GetSQLEnum(info.user.access.airps().elems());
                else
                    SQLText += " AND points.airp NOT IN "+GetSQLEnum(info.user.access.airps().elems());
            }
            if (!info.user.access.airlines().elems().empty()) {
                if (info.user.access.airlines().elems_permit())
                    SQLText += " AND points.airline IN "+GetSQLEnum(info.user.access.airlines().elems());
                else
                    SQLText += " AND points.airline NOT IN "+GetSQLEnum(info.user.access.airlines().elems());
            }
        } else {
            Qry.SQLText=
                "SELECT "
                "  arx_pax_grp.part_key, "
                "  arx_pax_grp.point_dep point_id, "
                "  arx_points.airline, "
                "  arx_points.flt_no, "
                "  arx_points.suffix, "
                "  arx_points.airline_fmt, "
                "  arx_points.airp_fmt, "
                "  arx_points.suffix_fmt, "
                "  arx_points.airp, "
                "  arx_points.scd_out, "
                "  NVL(arx_points.act_out,NVL(arx_points.est_out,arx_points.scd_out)) AS real_out, "
                "  arx_pax_grp.airp_arv, "
                "  arch.get_bagAmount2(arx_pax_grp.part_key,arx_pax_grp.grp_id,NULL,NULL) AS bag_amount, "
                "  arch.get_bagWeight2(arx_pax_grp.part_key,arx_pax_grp.grp_id,NULL,NULL) AS bag_weight, "
                "  arch.get_rkWeight2(arx_pax_grp.part_key,arx_pax_grp.grp_id,NULL,NULL) AS rk_weight, "
                "  arch.get_excess(arx_pax_grp.part_key,arx_pax_grp.grp_id,NULL) AS excess, "
                "  arx_pax_grp.piece_concept, "
                "  arch.get_birks2(arx_pax_grp.part_key,arx_pax_grp.grp_id,NULL,NULL,:pr_lat) AS tags, "
                "  arx_pax_grp.grp_id, "
                "  arx_pax_grp.hall "
                "FROM arx_pax_grp, arx_points "
                "WHERE point_dep=:point_id AND class IS NULL and "
                "   arx_pax_grp.status NOT IN ('E') AND "
                "   arx_pax_grp.part_key = arx_points.part_key and "
                "   arx_pax_grp.point_dep = arx_points.point_id and arx_points.pr_del>=0 and "
                "   arx_pax_grp.part_key = :part_key ";
            if (!info.user.access.airps().elems().empty()) {
                if (info.user.access.airps().elems_permit())
                    SQLText += " AND arx_points.airp IN "+GetSQLEnum(info.user.access.airps().elems());
                else
                    SQLText += " AND arx_points.airp NOT IN "+GetSQLEnum(info.user.access.airps().elems());
            }
            if (!info.user.access.airlines().elems().empty()) {
                if (info.user.access.airlines().elems_permit())
                    SQLText += " AND arx_points.airline IN "+GetSQLEnum(info.user.access.airlines().elems());
                else
                    SQLText += " AND arx_points.airline NOT IN "+GetSQLEnum(info.user.access.airlines().elems());
            }
            Qry.CreateVariable("part_key", otDate, part_key);
        }

        Qry.Execute();

        UnaccompListToXML(Qry, resNode, excessNodeList, false, 0, count);


        xmlNodePtr paxListNode = GetNode("paxList", resNode);
        if(paxListNode!=NULL) { // для совместимости со старой версией терминала
            xmlNodePtr headerNode = NewTextChild(paxListNode, "header");
            NewTextChild(headerNode, "col", "Рейс");
        } else
            throw AstraLocale::UserException("MSG.PASSENGERS.NOT_FOUND");
        tm.Init();
        STAT::set_variables(resNode);
        ProgTrace(TRACE5, "set_variables: %s", tm.PrintWithMessage().c_str());
        tm.Init();

        return;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

xmlNodePtr STAT::getVariablesNode(xmlNodePtr resNode)
{
    xmlNodePtr formDataNode = GetNode("form_data", resNode);
    if(!formDataNode)
        formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr variablesNode = GetNode("variables", formDataNode);
    if(!variablesNode)
        variablesNode = NewTextChild(formDataNode, "variables");
    return variablesNode;
}

xmlNodePtr STAT::set_variables(xmlNodePtr resNode, string lang)
{
    if(lang.empty())
        lang = TReqInfo::Instance()->desk.lang;

    xmlNodePtr variablesNode = getVariablesNode(resNode);

    TReqInfo *reqInfo = TReqInfo::Instance();
    TDateTime issued = NowUTC();
    string tz;
    if(reqInfo->user.sets.time == ustTimeUTC)
        tz = "(GMT)";
    else if(
            reqInfo->user.sets.time == ustTimeLocalDesk ||
            reqInfo->user.sets.time == ustTimeLocalAirp
           ) {
        issued = UTCToLocal(issued,reqInfo->desk.tz_region);
        tz = "(" + ElemIdToCodeNative(etCity, reqInfo->desk.city) + ")";
    }

    NewTextChild(variablesNode, "print_date",
            DateTimeToStr(issued, "dd.mm.yyyy hh:nn:ss ") + tz);
    NewTextChild(variablesNode, "print_oper", reqInfo->user.login);
    NewTextChild(variablesNode, "print_term", reqInfo->desk.code);
    NewTextChild(variablesNode, "use_seances", false); //!!!потом убрать
    NewTextChild(variablesNode, "test_server", bad_client_img_version() ? 2 : get_test_server());
    if(bad_client_img_version())
        NewTextChild(variablesNode, "doc_cap_test", " ");
    NewTextChild(variablesNode, "cap_test", getLocaleText("CAP.TEST", lang));
    NewTextChild(variablesNode, "page_number_fmt", getLocaleText("CAP.PAGE_NUMBER_FMT", lang));
    NewTextChild(variablesNode, "short_page_number_fmt", getLocaleText("CAP.SHORT_PAGE_NUMBER_FMT", lang));
    NewTextChild(variablesNode, "oper_info", getLocaleText("CAP.DOC.OPER_INFO", LParams()
                << LParam("date", DateTimeToStr(issued, "dd.mm.yyyy hh:nn:ss ") + tz)
                << LParam("oper", reqInfo->user.login)
                << LParam("term", reqInfo->desk.code),
                lang
                ));
    NewTextChild(variablesNode, "skip_header", 0);
    return variablesNode;
}

enum TStatType {
    statTrferFull,
    statFull,
    statShort,
    statDetail,
    statSelfCkinFull,
    statSelfCkinShort,
    statSelfCkinDetail,
    statAgentFull,
    statAgentShort,
    statAgentTotal,
    statTlgOutFull,
    statTlgOutShort,
    statTlgOutDetail,
    statPactShort,
    statRFISC,
    statService,
    statLimitedCapab,
    statUnaccBag,
    statAnnulBT,
    statPFSShort,
    statPFSFull,
    statTrferPax,
    statNum
};

const char *TStatTypeS[statNum] = {
    "statTrferFull",
    "statFull",
    "statShort",
    "statDetail",
    "statSelfCkinFull",
    "statSelfCkinShort",
    "statSelfCkinDetail",
    "statAgentFull",
    "statAgentShort",
    "statAgentTotal",
    "statTlgOutFull",
    "statTlgOutShort",
    "statTlgOutDetail",
    "statPactShort",
    "statRFISC",
    "statService",
    "statLimitedCapab",
    "statUnaccBag",
    "statAnnulBT",
    "statPFSShort",
    "statPFSFull",
    "statTrferPax"
};

TStatType DecodeStatType( const string stat_type )
{
    int i;
    for( i=0; i<(int)statNum; i++ )
        if ( stat_type == TStatTypeS[ i ] )
            break;
    return (TStatType)i;
}

string EncodeStatType(const TStatType stat_type)
{
    return (
            stat_type >= statNum or stat_type < 0
            ? string() : TStatTypeS[ stat_type ]
           );
}

enum TSeanceType { seanceAirline, seanceAirport, seanceAll };

static const string PARAM_SEANCE_TYPE           = "seance_type";
static const string PARAM_DESK_CITY             = "desk_city";
static const string PARAM_DESK_LANG             = "desk_lang";
static const string PARAM_NAME                  = "name";
static const string PARAM_TYPE                  = "type";
static const string PARAM_STAT_TYPE             = "stat_type";
static const string PARAM_AIRLINES_PREFIX       = "airlines.";
static const string PARAM_AIRLINES_PERMIT       = "airlines.permit";
static const string PARAM_AIRPS_PREFIX          = "airps.";
static const string PARAM_AIRPS_PERMIT          = "airps.permit";
static const string PARAM_AIRP_COLUMN_FIRST     = "airp_column_first";
static const string PARAM_FIRSTDATE             = "FirstDate";
static const string PARAM_LASTDATE              = "LastDate";
static const string PARAM_FLT_NO                = "flt_no";
static const string PARAM_DESK                  = "desk";
static const string PARAM_USER_ID               = "user_id";
static const string PARAM_USER_LOGIN            = "user_login";
static const string PARAM_TYPEB_TYPE            = "typeb_type";
static const string PARAM_SENDER_ADDR           = "sender_addr";
static const string PARAM_RECEIVER_DESCR        = "receiver_descr";
static const string PARAM_REG_TYPE              = "reg_type";
static const string PARAM_SKIP_ROWS             = "skip_rows";
static const string PARAM_ORDER_SOURCE          = "order_source";
static const string PARAM_PR_PACTS              = "pr_pacts";
static const string PARAM_TRFER_AIRP            = "trfer_airp";
static const string PARAM_TRFER_AIRLINE         = "trfer_airline";
static const string PARAM_SEG_CATEGORY          = "seg_category";

class TSegCategories {
    public:
        enum Enum
        {
            IntInt, // Internal - Internal
            ForFor, // Foreign - Foreign
            ForInt, // Foreign -Internal
            IntFor,  // Internal - Foreign
            Unknown
        };

        static const std::list< std::pair<Enum, std::string> >& pairsCodes()
        {
            static std::list< std::pair<Enum, std::string> > l;
            if (l.empty())
            {
                l.push_back(std::make_pair(IntInt,  "ВВЛ-ВВЛ"));
                l.push_back(std::make_pair(ForFor,  "МВЛ-МВЛ"));
                l.push_back(std::make_pair(ForInt,  "МВЛ-ВВЛ"));
                l.push_back(std::make_pair(IntFor,  "ВВЛ-МВЛ"));
                l.push_back(std::make_pair(Unknown, ""));
            }
            return l;
        }
};

class TSegCategory : public ASTRA::PairList<TSegCategories::Enum, std::string>
{
  private:
    virtual std::string className() const { return "TSegCategory"; }
  public:
    TSegCategory() : ASTRA::PairList<TSegCategories::Enum, std::string>(TSegCategories::pairsCodes(),
                                                                            boost::none,
                                                                            boost::none) {}
};

struct TStatParams {
    string desk_city; // Используется в TReqInfo::Initialize(city) в фоновой статистике
    string desk_lang;
    string name, type;
    TStatType statType;
    TAccessElems<string> airlines, airps;
    bool airp_column_first;
    TSeanceType seance;
    TDateTime FirstDate, LastDate;
    int flt_no;
    string desk;
    int user_id;
    string user_login;
    string typeb_type;
    string sender_addr;
    string receiver_descr;
    string reg_type;
    bool order;
    bool skip_rows;
    bool pr_pacts;
    TSegCategories::Enum seg_category;
    string trfer_airp;
    string trfer_airline;
    void get(xmlNodePtr resNode);
    void toFileParams(map<string, string> &file_params) const;
    void fromFileParams(map<string, string> &file_params);
    void AccessClause(string &SQLText) const;
};

void TStatParams::fromFileParams(map<string, string> &file_params)
{
    trfer_airp = file_params[PARAM_TRFER_AIRP];
    trfer_airline = file_params[PARAM_TRFER_AIRLINE];
    seg_category = TSegCategory().decode(file_params[PARAM_SEG_CATEGORY]);
    seance = (TSeanceType)ToInt(file_params[PARAM_SEANCE_TYPE]);
    desk_city = file_params[PARAM_DESK_CITY];
    desk_lang = file_params[PARAM_DESK_LANG];
    name = file_params[PARAM_NAME];
    type = file_params[PARAM_TYPE];
    statType = DecodeStatType(file_params[PARAM_STAT_TYPE]);
    for(map<string, string>::iterator i = file_params.begin(); i != file_params.end(); i++) {
        if(i->first.substr(0, PARAM_AIRLINES_PREFIX.size()) == PARAM_AIRLINES_PREFIX)
            airlines.add_elem(i->second);
        if(i->first.substr(0, PARAM_AIRPS_PREFIX.size()) == PARAM_AIRPS_PREFIX)
            airlines.add_elem(i->second);
    }
    airlines.set_elems_permit(ToInt(file_params[PARAM_AIRLINES_PERMIT]));
    airps.set_elems_permit(ToInt(file_params[PARAM_AIRPS_PERMIT]));
    airp_column_first = ToInt(file_params[PARAM_AIRP_COLUMN_FIRST]);
    StrToDateTime(file_params[PARAM_FIRSTDATE].c_str(), ServerFormatDateTimeAsString, FirstDate);
    StrToDateTime(file_params[PARAM_LASTDATE].c_str(), ServerFormatDateTimeAsString, LastDate);
    flt_no = ToInt(file_params[PARAM_FLT_NO]);
    desk = file_params[PARAM_DESK];
    user_id = ToInt(file_params[PARAM_USER_ID]);
    user_login = file_params[PARAM_USER_LOGIN];
    typeb_type = file_params[PARAM_TYPEB_TYPE];
    sender_addr = file_params[PARAM_SENDER_ADDR];
    receiver_descr = file_params[PARAM_RECEIVER_DESCR];
    reg_type = file_params[PARAM_REG_TYPE];
    skip_rows = ToInt(file_params[PARAM_SKIP_ROWS]);
    pr_pacts = ToInt(file_params[PARAM_PR_PACTS]) != 0;
}

void TStatParams::toFileParams(map<string, string> &file_params) const
{
    file_params[PARAM_TRFER_AIRP] = trfer_airp;
    file_params[PARAM_TRFER_AIRLINE] = trfer_airline;
    file_params[PARAM_SEG_CATEGORY] = TSegCategory().encode(seg_category);
    file_params[PARAM_SEANCE_TYPE] = IntToString(seance);
    file_params[PARAM_DESK_CITY] = TReqInfo::Instance()->desk.city;
    file_params[PARAM_DESK_LANG] = TReqInfo::Instance()->desk.lang;
    file_params[PARAM_NAME] = name;
    file_params[PARAM_TYPE] = type;
    file_params[PARAM_STAT_TYPE] = EncodeStatType(statType);
    file_params[PARAM_STAT_TYPE] = EncodeStatType(statType);
    int idx = 0;
    for(set<string>::iterator i = airlines.elems().begin(); i != airlines.elems().end(); i++, idx++) {
        file_params[PARAM_AIRLINES_PREFIX + IntToString(idx)] = *i;
    }
    file_params[PARAM_AIRLINES_PERMIT] = IntToString(airlines.elems_permit());

    idx = 0;
    for(set<string>::iterator i = airps.elems().begin(); i != airps.elems().end(); i++, idx++) {
        file_params[PARAM_AIRPS_PREFIX + IntToString(idx)] = *i;
    }
    file_params[PARAM_AIRPS_PERMIT] = IntToString(airps.elems_permit());
    file_params[PARAM_AIRP_COLUMN_FIRST] = IntToString(airp_column_first);
    file_params[PARAM_FIRSTDATE] = DateTimeToStr(FirstDate, ServerFormatDateTimeAsString);
    file_params[PARAM_LASTDATE] = DateTimeToStr(LastDate, ServerFormatDateTimeAsString);
    file_params[PARAM_FLT_NO] = IntToString(flt_no);
    file_params[PARAM_DESK] = desk;
    file_params[PARAM_USER_ID] = IntToString(user_id);
    file_params[PARAM_USER_LOGIN] = user_login;
    file_params[PARAM_TYPEB_TYPE] = typeb_type;
    file_params[PARAM_SENDER_ADDR] = sender_addr;
    file_params[PARAM_RECEIVER_DESCR] = receiver_descr;
    file_params[PARAM_REG_TYPE] = reg_type;
    file_params[PARAM_SKIP_ROWS] = IntToString(skip_rows);

    file_params[PARAM_ORDER_SOURCE] = EncodeOrderSource(osSTAT);
    file_params[PARAM_PR_PACTS] = IntToString(pr_pacts);
}

string GetStatSQLText(const TStatParams &params, int pass)
{
    static const string sum_pax_by_client_type =
        " SUM(DECODE(client_type, :web, adult + child + baby, 0)) web, \n"
        " SUM(DECODE(client_type, :web, term_bag, 0)) web_bag, \n"
        " SUM(DECODE(client_type, :web, term_bp, 0)) web_bp, \n"

        " SUM(DECODE(client_type, :kiosk, adult + child + baby, 0)) kiosk, \n"
        " SUM(DECODE(client_type, :kiosk, term_bag, 0)) kiosk_bag, \n"
        " SUM(DECODE(client_type, :kiosk, term_bp, 0)) kiosk_bp, \n"

        " SUM(DECODE(client_type, :mobile, adult + child + baby, 0)) mobile, \n"
        " SUM(DECODE(client_type, :mobile, term_bp, 0)) mobile_bp, \n"
        " SUM(DECODE(client_type, :mobile, term_bag, 0)) mobile_bag \n";

    static const string sum_pax_by_class =
        " sum(f) f, \n"
        " sum(c) c, \n"
        " sum(y) y, \n";

  ostringstream sql;
  sql << "SELECT \n";

  if (params.statType==statTrferFull)
  {
    sql << " points.airp, \n"
           " points.airline, \n"
           " points.flt_no, \n"
           " points.scd_out, \n"
           " trfer_stat.point_id, \n"
           " trfer_stat.trfer_route places, \n"
           " adult + child + baby pax_amount, \n"
           " adult, \n"
           " child, \n"
           " baby, \n"
           " unchecked rk_weight, \n"
           " pcs bag_amount, \n"
           " weight bag_weight, \n"
           " excess, \n"
           " nvl(excess_pc,0) excess_pc\n";
  };
  if (params.statType==statFull)
  {
    sql << " points.airp, \n"
           " points.airline, \n"
           " points.flt_no, \n"
           " points.scd_out, \n";
    if (pass!=0)
      sql << " stat.part_key, \n";
    sql << " stat.point_id, \n"
           " SUM(adult + child + baby) pax_amount, \n" <<
           sum_pax_by_class <<
           sum_pax_by_client_type <<
           ", SUM(adult) adult, \n"
           " SUM(child) child, \n"
           " SUM(baby) baby, \n"
           " SUM(unchecked) rk_weight, \n"
           " SUM(pcs) bag_amount, \n"
           " SUM(weight) bag_weight, \n"
           " SUM(excess) excess, \n"
           " SUM(nvl(excess_pc,0)) excess_pc \n";
  };
  if (params.statType==statShort)
  {
    if(params.airp_column_first)
      sql << " points.airp, \n";
    else
      sql << " points.airline, \n";
    sql << " COUNT(distinct stat.point_id) flt_amount, \n"
           " SUM(adult + child + baby) pax_amount, \n" <<
           sum_pax_by_class <<
           sum_pax_by_client_type;
  };
  if (params.statType==statDetail)
  {
    sql << " points.airp, \n"
           " points.airline, \n"
           " COUNT(distinct stat.point_id) flt_amount, \n"
           " SUM(adult + child + baby) pax_amount, \n" <<
           sum_pax_by_class <<
           sum_pax_by_client_type;
  };
  sql << "FROM \n";
  if (pass==0)
    sql << " points, \n";
  else
    sql << " arx_points points, \n";
  if (params.statType==statTrferFull)
  {
    if (pass==0)
      sql << " trfer_stat \n";
    else
      sql << " arx_trfer_stat trfer_stat \n";
  };
  if (params.statType==statFull || params.statType==statShort || params.statType==statDetail)
  {
    if (pass==0)
      sql << " stat \n";
    else
      sql << " arx_stat stat \n";
  };

  if (params.seance==seanceAirport ||
      params.seance==seanceAirline)
  {
    if (pass!=2)
    {
      if(params.seance==seanceAirport)
        sql << "," << AIRP_PERIODS;
      if(params.seance==seanceAirline)
        sql << "," << AIRLINE_PERIODS;
    }
    else
    {
      sql << ", (SELECT arx_points.part_key, arx_points.point_id \n"
             "   FROM move_arx_ext, arx_points \n";
      if(params.seance==seanceAirport)
        sql << "," << AIRP_PERIODS;
      if(params.seance==seanceAirline)
        sql << "," << AIRLINE_PERIODS;
      sql << "   WHERE move_arx_ext.part_key >= periods.period_last_date + :arx_trip_date_range AND \n"
             "         move_arx_ext.part_key <= periods.period_last_date + move_arx_ext.date_range AND \n"
             "         move_arx_ext.part_key = arx_points.part_key AND move_arx_ext.move_id = arx_points.move_id AND \n"
             "         arx_points.scd_out >= periods.period_first_date AND arx_points.scd_out < periods.period_last_date AND \n";
      if(params.seance==seanceAirport)
        sql << "         arx_points.airline NOT IN \n" << AIRLINE_LIST;
      if(params.seance==seanceAirline)
        sql << "         arx_points.airp IN \n" << AIRP_LIST;
      sql << "  ) arx_ext \n";
    };
  }
  else
  {
    if (pass==2)
      sql << ", (SELECT part_key, move_id FROM move_arx_ext \n"
             "   WHERE part_key >= :LastDate+:arx_trip_date_range AND part_key <= :LastDate+date_range) arx_ext \n";
  };

  sql << "WHERE \n";
  if(params.flt_no != NoExists)
    sql << " points.flt_no = :flt_no AND \n";
  if (params.statType==statTrferFull)
  {
    if (pass!=0)
      sql << " points.part_key = trfer_stat.part_key AND \n";
    sql << " points.point_id = trfer_stat.point_id AND \n";
  };
  if (params.statType==statFull || params.statType==statShort || params.statType==statDetail)
  {
    if (pass!=0)
      sql << " points.part_key = stat.part_key AND \n";
    sql << " points.point_id = stat.point_id AND \n";
  };

  if (params.seance==seanceAirport ||
      params.seance==seanceAirline)
  {
    if (pass!=2)
    {
      if (pass!=0)
        sql << " points.part_key >= periods.period_first_date AND points.part_key < periods.period_last_date + :arx_trip_date_range AND \n";
      sql << " points.scd_out >= periods.period_first_date AND points.scd_out < periods.period_last_date AND \n";
    }
    else
      sql << " points.part_key=arx_ext.part_key AND points.point_id=arx_ext.point_id AND \n";
  }
  else
  {
    if (pass==1)
      sql << " points.part_key >= :FirstDate AND points.part_key < :LastDate + :arx_trip_date_range AND \n";
    if (pass==2)
      sql << " points.part_key=arx_ext.part_key AND points.move_id=arx_ext.move_id AND \n";
    sql << " points.scd_out >= :FirstDate AND points.scd_out < :LastDate AND \n";
  };

  sql << " points.pr_del>=0 \n";
  if (params.seance==seanceAirport)
  {
    sql << " AND points.airp = :ap \n";
  }
  else
  {
    if (!params.airps.elems().empty()) {
      if (params.airps.elems_permit())
        sql << " AND points.airp IN " << GetSQLEnum(params.airps.elems()) << "\n";
      else
        sql << " AND points.airp NOT IN " << GetSQLEnum(params.airps.elems()) << "\n";
    };
  };

  if (params.seance==seanceAirline)
  {
    sql << " AND points.airline = :ak \n";
  }
  else
  {
    if (!params.airlines.elems().empty()) {
      if (params.airlines.elems_permit())
        sql << " AND points.airline IN " << GetSQLEnum(params.airlines.elems()) << "\n";
      else
        sql << " AND points.airline NOT IN " << GetSQLEnum(params.airlines.elems()) << "\n";
    }
  };

  if (pass!=2)
  {
    if(params.seance==seanceAirport)
      sql << " AND points.airline NOT IN \n" << AIRLINE_LIST;
    if(params.seance==seanceAirline)
      sql << " AND points.airp IN \n" << AIRP_LIST;
  };

  if (params.statType==statFull)
  {
    sql << "GROUP BY \n"
           " points.airp, \n"
           " points.airline, \n"
           " points.flt_no, \n"
           " points.scd_out, \n";
    if (pass!=0)
      sql << " stat.part_key, \n";
    sql << " stat.point_id \n";
  };
  if (params.statType==statShort)
  {
    sql << "GROUP BY \n";
    if(params.airp_column_first)
      sql << " points.airp \n";
    else
      sql << " points.airline \n";
  };
  if (params.statType==statDetail)
  {
    sql << "GROUP BY \n"
           " points.airp, \n"
           " points.airline \n";
  };
  return sql.str();
}

struct TPrintAirline {
    private:
        string val;
        bool multi_airlines;
    public:
        TPrintAirline(): multi_airlines(false) {};
        void check(string val);
        string get() const;
};

string TPrintAirline::get() const
{
    if(multi_airlines)
        return "";
    else
        return val;
}

void TPrintAirline::check(string val)
{
    if(this->val.empty())
        this->val = val;
    else if(this->val != val)
        multi_airlines = true;
}

/* GRISHA */
void TStatParams::get(xmlNodePtr reqNode)
{
    name = NodeAsString("stat_mode", reqNode);
    type = NodeAsString("stat_type", reqNode, "Общая");

    if(type == "Трансфер") {
        if(name == "Общая") statType = statTrferFull;
        else if(name == "Подробная") statType=statTrferPax;
        else throw Exception("Unknown stat mode " + name);
    } else if(type == "Общая") {
        if(name == "Подробная") statType=statFull;
        else if(name == "Общая") statType=statShort;
        else if(name == "Детализированная") statType=statDetail;
        else throw Exception("Unknown stat mode " + name);
    } else if(type ==
            ((TReqInfo::Instance()->client_type==ctHTTP ||
              TReqInfo::Instance()->desk.compatible(SELF_CKIN_STAT_VERSION)) ?
             "Саморегистрация" :
             "По киоскам"
            )
            ) {
        if(name == "Подробная")
            statType=statSelfCkinFull;
        else if(name == "Общая")
            statType=statSelfCkinShort;
        else if(name == "Детализированная")
            statType=statSelfCkinDetail;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "По агентам") {
        if(name == "Подробная")
            statType=statAgentFull;
        else if(name == "Общая")
            statType=statAgentShort;
        else if(name == "Итого")
            statType=statAgentTotal;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "Отпр. телеграммы") {
        if(name == "Подробная")
            statType=statTlgOutFull;
        else if(name == "Общая")
            statType=statTlgOutShort;
        else if(name == "Детализированная")
            statType=statTlgOutDetail;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "Договор") {
        if(name == "Общая")
            statType=statPactShort;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "Багажные RFISC") {
        if(name == "Подробная")
            statType=statRFISC;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "Услуги") {
        if(name == "Подробная")
            statType=statService;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "Огр. возмож.") {
        if(name == "Подробная")
            statType=statLimitedCapab;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "Несопр. багаж") {
        if(name == "Подробная")
            statType=statUnaccBag;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "Аннул. бирки") {
        if(name == "Подробная")
            statType=statAnnulBT;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "PFS") {
        if(name == "Подробная")
            statType = statPFSFull;
        else if(name == "Общая")
            statType = statPFSShort;
        else
            throw Exception("Unknown stat mode " + name);
    } else
        throw Exception("Unknown stat type " + type);

    FirstDate = NodeAsDateTime("FirstDate", reqNode);
    LastDate = NodeAsDateTime("LastDate", reqNode);
    TReqInfo &info = *(TReqInfo::Instance());

    xmlNodePtr curNode = reqNode->children;

    string ak = NodeAsStringFast("ak", curNode, "");
    string ap = NodeAsStringFast("ap", curNode, "");
    if (!NodeIsNULLFast("flt_no", curNode, true))
      flt_no = NodeAsIntegerFast("flt_no", curNode, NoExists);
    else
      flt_no = NoExists;
    desk = NodeAsStringFast("desk", curNode, NodeAsStringFast("kiosk", curNode, ""));
    if (!NodeIsNULLFast("user", curNode, true))
      user_id = NodeAsIntegerFast("user", curNode, NoExists);
    else
      user_id = NoExists;
    user_login = NodeAsStringFast("user_login", curNode, "");
    typeb_type = NodeAsStringFast("typeb_type", curNode, "");
    sender_addr = NodeAsStringFast("sender_addr", curNode, "");
    receiver_descr = NodeAsStringFast("receiver_descr", curNode, "");
    reg_type = NodeAsStringFast("reg_type", curNode, "");
    order = NodeAsStringFast("Order", curNode, 0) != 0;
    seg_category = TSegCategory().decode(NodeAsStringFast("SegCategory", curNode, ""));
    TElemFmt fmt;
    trfer_airp = ElemToElemId(etAirp, NodeAsStringFast("trfer_airp", curNode, ""), fmt);
    trfer_airline = ElemToElemId(etAirline, NodeAsStringFast("trfer_airline", curNode, ""), fmt);

    ProgTrace(TRACE5, "ak: %s", ak.c_str());
    ProgTrace(TRACE5, "ap: %s", ap.c_str());

    airlines=info.user.access.airlines();
    if (!ak.empty())
      airlines.merge(TAccessElems<string>(ak, true));
    airps=info.user.access.airps();
    if (!ap.empty())
      airps.merge(TAccessElems<string>(ap, true));

    if (airlines.totally_not_permitted() ||
        airps.totally_not_permitted())
      throw AstraLocale::UserException("MSG.NO_ACCESS");

    airp_column_first = (info.user.user_type == utAirport);

    //сеансы (договоры)
    string seance_str = NodeAsStringFast("seance", curNode, "");
    seance = seanceAll;
    if (seance_str=="АК") seance=seanceAirline;
    if (seance_str=="АП") seance=seanceAirport;

    bool all_seances_permit = info.user.access.rights().permitted(615);

    if (info.user.user_type != utSupport && !all_seances_permit)
    {
      if (info.user.user_type == utAirline)
        seance=seanceAirline;
      else
        seance=seanceAirport;
    };

    if (seance==seanceAirline)
    {
        if (!airlines.elems_permit()) throw UserException("MSG.NEED_SET_CODE_AIRLINE");
    };

    if (seance==seanceAirport)
    {
        if (!airps.elems_permit()) throw UserException("MSG.NEED_SET_CODE_AIRP");
    };

    skip_rows =
        info.user.user_type == utAirline and
        statType == statTrferFull and
        ak.empty() and
        ap.empty() and
        flt_no == NoExists and
        seance == seanceAll;
    pr_pacts = false;
};

struct TInetStat {
    int web;
    int kiosk;
    int mobile;
    int web_bp, kiosk_bp;   // Кол-во пассажиров из веб и киоска, которые распечатали посадочный на стойке
    int web_bag, kiosk_bag; //        ___,,____                 , которые зарег. багаж на стойке
    int mobile_bp, mobile_bag;

    TInetStat():
        web(0),
        kiosk(0),
        mobile(0),
        web_bp(0), kiosk_bp(0),
        web_bag(0), kiosk_bag(0),
        mobile_bp(0), mobile_bag(0)
    {}

    bool operator == (const TInetStat &item) const
    {
        return
            web == item.web &&
            kiosk == item.kiosk &&
            mobile == item.mobile &&
            web_bp == item.web_bp &&
            web_bag == item.web_bag &&
            kiosk_bp == item.kiosk_bp &&
            kiosk_bag == item.kiosk_bag &&
            mobile_bp == item.mobile_bp &&
            mobile_bag == item.mobile_bag;
    }

    void operator += (const TInetStat &item)
    {
        web += item.web;
        kiosk += item.kiosk;
        mobile += item.mobile;
        web_bp += item.web_bp;
        web_bag += item.web_bag;
        kiosk_bp += item.kiosk_bp;
        kiosk_bag += item.kiosk_bag;
        mobile_bp += item.mobile_bp;
        mobile_bag += item.mobile_bag;
    }

    void toXML(xmlNodePtr headerNode, xmlNodePtr rowNode) {
        //Web
        xmlNodePtr colNode = NewTextChild(headerNode, "col", getLocaleText("Web"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", web);

        colNode = NewTextChild(headerNode, "col", getLocaleText("БГ"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", web_bag);

        colNode = NewTextChild(headerNode, "col", getLocaleText("ПТ"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", web_bp);

        // Киоски
        colNode = NewTextChild(headerNode, "col", getLocaleText("Киоски"));
        SetProp(colNode, "width", 40);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", kiosk);

        colNode = NewTextChild(headerNode, "col", getLocaleText("БГ"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", kiosk_bag);

        colNode = NewTextChild(headerNode, "col", getLocaleText("ПТ"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", kiosk_bp);

        // Моб.
        colNode = NewTextChild(headerNode, "col", getLocaleText("Моб."));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", mobile);

        colNode = NewTextChild(headerNode, "col", getLocaleText("БГ"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", mobile_bag);

        colNode = NewTextChild(headerNode, "col", getLocaleText("ПТ"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", mobile_bp);
    }
};

struct TDetailStatRow {
    int flt_amount, pax_amount;
    int f, c, y;
    TInetStat i_stat;
    set<int> flts;
    TDetailStatRow():
        flt_amount(0),
        pax_amount(0),
        f(0), c(0), y(0)
    {};
    bool operator == (const TDetailStatRow &item) const
    {
        return flt_amount == item.flt_amount &&
               pax_amount == item.pax_amount &&
               i_stat == item.i_stat &&
               f == item.f &&
               c == item.c &&
               y == item.y &&
               flts.size() == item.flts.size();
    };
    void operator += (const TDetailStatRow &item)
    {
        flt_amount += item.flt_amount;
        pax_amount += item.pax_amount;
        i_stat += item.i_stat;
        f += item.f;
        c += item.c;
        y += item.y;
        flts.insert(item.flts.begin(),item.flts.end());
    };
};

struct TDetailStatKey {
    string pact_descr, col1, col2;
    bool operator == (const TDetailStatKey &item) const
    {
        return pact_descr == item.pact_descr &&
               col1 == item.col1 &&
               col2 == item.col2;
    };
};
struct TDetailCmp {
    bool operator() (const TDetailStatKey &lr, const TDetailStatKey &rr) const
    {
      if(lr.col1 == rr.col1)
        if(lr.col2 == rr.col2)
          return lr.pact_descr < rr.pact_descr;
        else
          return lr.col2 < rr.col2;
      else
        return lr.col1 < rr.col1;
    };
};
typedef map<TDetailStatKey, TDetailStatRow, TDetailCmp> TDetailStat;

struct TOrderStatItem {
    static const char delim = ';';
    virtual void add_header(ostringstream &buf) const = 0;
    virtual void add_data(ostringstream &buf) const = 0;
    virtual ~TOrderStatItem() {}
};

struct TRFISCStatRow:public TOrderStatItem {
    string rfisc;
    int point_id;
    int point_num;
    int pr_trfer;
    TDateTime scd_out;
    int flt_no;
    string suffix;
    string airp;
    string airp_arv;
    string craft;
    TDateTime travel_time;
    int trfer_flt_no;
    string trfer_suffix;
    string trfer_airp_arv;
    string desk;
    string user_login;
    string user_descr;
    TDateTime time_create;
    double tag_no;
    string fqt_no;
    int excess;
    int paid;

    TRFISCStatRow():
        point_id(NoExists),
        point_num(NoExists),
        pr_trfer(NoExists),
        scd_out(NoExists),
        flt_no(NoExists),
        travel_time(NoExists),
        trfer_flt_no(NoExists),
        time_create(NoExists),
        tag_no(NoExists),
        excess(NoExists),
        paid(NoExists)
    {}

    bool operator < (const TRFISCStatRow &val) const
    {
        if(point_id != val.point_id)
            return point_id < val.point_id;
        if(point_num != val.point_num)
            return point_num < val.point_num;
        if(pr_trfer != val.pr_trfer)
            return pr_trfer > val.pr_trfer;
        if(trfer_airp_arv != val.trfer_airp_arv)
            return trfer_airp_arv < val.trfer_airp_arv;
        return tag_no < val.tag_no;
    }

    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void TRFISCStatRow::add_data(ostringstream &buf) const
{
    //RFISC
    buf << rfisc << delim;
    // Платн.
    if(excess != NoExists)
        buf << excess;
    buf << delim;
    // Опл.
    if(paid != NoExists)
        buf << paid;
    buf
        << delim
        // Бирка
        << fixed << setprecision(0) << setw(10) << setfill('0') << tag_no << delim
        // SPEQ
        << delim
        // FQTV
        << fqt_no << delim
        // Дата вылета
        << DateTimeToStr(scd_out, "dd.mm.yyyy") << delim
        // Рейс
        << flt_no << suffix << delim
        // От
        << delim
        // До
        << airp_arv << delim
        // Тип ВС
        << craft << delim;
    // Время в пути
    if(travel_time != NoExists)
        buf << DateTimeToStr(travel_time, "hh:nn");
    buf << delim;

    // Трансфер на
    ostringstream trfer_flt_no;
    string trfer_airp_dep;
    string trfer_airp_arv;
    if(this->trfer_flt_no) {
        trfer_flt_no << setw(3) << setfill('0') << this->trfer_flt_no << ElemIdToCodeNative(etSuffix, trfer_suffix);
        trfer_airp_dep = airp;
        trfer_airp_arv = trfer_airp_arv;
    }
    // Рейс
    buf << trfer_flt_no.str() << delim;
    // От (совпадает с От рейса)
    buf << ElemIdToCodeNative(etAirp, trfer_airp_dep) << delim;
    // До
    buf << ElemIdToCodeNative(etAirp, trfer_airp_arv) << delim;

    // Информация об агенте
    // АП рег. (совпадает с От рейса)
    buf << ElemIdToCodeNative(etAirp, airp) << delim;
    // Стойка
    buf << desk << delim;
    // LOGIN
    buf << user_login << delim;
    // Агент
    buf << user_descr << delim;
    // Дата оформления
    if(time_create != NoExists)
        buf << DateTimeToStr(time_create, "dd.mm.yyyy");
    buf << endl;
}

void TRFISCStatRow::add_header(ostringstream &buf) const
{
    buf
        << "RFISC" << delim
        << "Платн." << delim
        << "Опл." << delim
        << "Бирка" << delim
        << "SPEC" << delim
        << "FQTV" << delim
        << "Дата вылета" << delim
        << "Рейс" << delim
        << "От" << delim
        << "До" << delim
        << "Тип ВС" << delim
        << "Время в пути" << delim
        << "Трфр.рейс" << delim
        << "От" << delim
        << "До" << delim
        << "АП рег." << delim
        << "Стойка" << delim
        << "LOGIN" << delim
        << "Агент" << delim
        << "Дата оформ."
        << endl;
}

typedef set<TRFISCStatRow> TRFISCStat;

struct TPaid {
    int excess;
    int excess_pcs;

    TPaid(int aexcess, int aexcess_pcs):
        excess(aexcess),
        excess_pcs(aexcess_pcs)
    {}

    /*
    string toString() const
    {
        ostringstream result;
        if(excess == 0 and excess_pcs == 0)
            result << 0;
        else {
            if(excess != 0)
                result << excess << getLocaleText("кг");
            if(excess_pcs != 0) {
                if(not result.str().empty())
                    result << " + ";
                result << excess_pcs << getLocaleText("м");
            }
        }
        return result.str();
    }
    */

    void set(int aexcess, int aexcess_pcs)
    {
        excess = aexcess;
        excess_pcs = aexcess_pcs;
    }

    bool operator == (const TPaid &val) const
    {
        return
            excess == val.excess and
            excess_pcs == val.excess_pcs;
    }

    void operator += (const TPaid &val)
    {
        excess += val.excess;
        excess_pcs += val.excess_pcs;
    }
};

struct TFullStatRow {
    int pax_amount;
    TInetStat i_stat;
    int adult;
    int child;
    int baby;
    int rk_weight;
    int bag_amount;
    int bag_weight;
    TPaid paid;
    TFullStatRow():
        pax_amount(0),
        adult(0),
        child(0),
        baby(0),
        rk_weight(0),
        bag_amount(0),
        bag_weight(0),
        paid(0, 0)
    {}
    bool operator == (const TFullStatRow &item) const
    {
        return pax_amount == item.pax_amount &&
            i_stat == item.i_stat &&
            adult == item.adult &&
            child == item.child &&
            baby == item.baby &&
            rk_weight == item.rk_weight &&
            bag_amount == item.bag_amount &&
            bag_weight == item.bag_weight &&
            paid == item.paid;
    };
    void operator += (const TFullStatRow &item)
    {
        pax_amount += item.pax_amount;
        i_stat += item.i_stat;
        adult += item.adult;
        child += item.child;
        baby += item.baby;
        rk_weight += item.rk_weight;
        bag_amount += item.bag_amount;
        bag_weight += item.bag_weight;
        paid += item.paid;
    };
};

struct TStatPlaces {
    private:
        string result;
    public:
        void set(string aval, bool pr_locale);
        string get() const;
};

void TStatPlaces::set(string aval, bool pr_locale)
{
    if(not result.empty())
        throw Exception("TStatPlaces::set(): already set");
    if(pr_locale) {
        vector<string> tokens;
        while(true) {
            size_t idx = aval.find('-');
            if(idx == string::npos) break;
            tokens.push_back(aval.substr(0, idx));
            aval.erase(0, idx + 1);
        }
        tokens.push_back(aval);
        for(vector<string>::iterator is = tokens.begin(); is != tokens.end(); is++)
            result += (result.empty() ? "" : "-") + ElemIdToCodeNative(etAirp, *is);
    } else
        result = aval;
}

string TStatPlaces::get() const
{
    return result;
}

struct TFullStatKey {
    string col1, col2;
    string airp; // в сортировке не участвует, нужен для AirpTZRegion
    int flt_no;
    TDateTime scd_out;
    int point_id;
    TStatPlaces places;
    TFullStatKey():
        flt_no(NoExists),
        scd_out(NoExists),
        point_id(NoExists)
    {}
    bool operator == (const TFullStatKey &item) const
    {
        return col1 == item.col1 &&
               col2 == item.col2 &&
               airp == item.airp &&
               flt_no == item.flt_no &&
               scd_out == item.scd_out &&
               point_id == item.point_id &&
               places.get() == item.places.get();
    };
};
struct TFullCmp {
    bool operator() (const TFullStatKey &lr, const TFullStatKey &rr) const
    {
      if(lr.col1 == rr.col1)
        if(lr.col2 == rr.col2)
          if(lr.flt_no == rr.flt_no)
            if(lr.scd_out == rr.scd_out)
              if(lr.point_id == rr.point_id)
                return lr.places.get() < rr.places.get();
              else
                return lr.point_id < rr.point_id;
            else
              return lr.scd_out < rr.scd_out;
          else
            return lr.flt_no < rr.flt_no;
        else
          return lr.col2 < rr.col2;
      else
        return lr.col1 < rr.col1;
    };
};
typedef map<TFullStatKey, TFullStatRow, TFullCmp> TFullStat;

namespace AstraLocale {
class MaxStatRowsException: public UserException
{
    public: MaxStatRowsException(const std::string &vlexema, const LParams &aparams): UserException(vlexema, aparams) {}
};
}

// TODO переименовать здесь и всюду bool full в что-то типа override_MAX_STAT_ROWS()
template <class keyClass, class rowClass, class cmpClass>
void AddStatRow(const keyClass &key, const rowClass &row, map<keyClass, rowClass, cmpClass> &stat, bool full = false)
{
  typename map< keyClass, rowClass, cmpClass >::iterator i = stat.find(key);
  if (i!=stat.end())
    i->second+=row;
  else
  {
    if ((not full) and (stat.size() > (size_t)MAX_STAT_ROWS()))
      throw MaxStatRowsException("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH", LParams() << LParam("num", MAX_STAT_ROWS()));
    if (full or stat.size()<=(size_t)MAX_STAT_ROWS())
      stat.insert(make_pair(key,row));
  };
};

void GetDetailStat(const TStatParams &params, TQuery &Qry,
                   TDetailStat &DetailStat, TDetailStatRow &DetailStatTotal,
                   TPrintAirline &airline, string pact_descr = "", bool full = false)
{
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next())
  {
    TDetailStatRow row;
    row.flt_amount = Qry.FieldAsInteger("flt_amount");
    row.pax_amount = Qry.FieldAsInteger("pax_amount");

    row.i_stat.web = Qry.FieldAsInteger("web");
    row.i_stat.web_bag = Qry.FieldAsInteger("web_bag");
    row.i_stat.web_bp = Qry.FieldAsInteger("web_bp");

    row.i_stat.kiosk = Qry.FieldAsInteger("kiosk");
    row.i_stat.kiosk_bag = Qry.FieldAsInteger("kiosk_bag");
    row.i_stat.kiosk_bp = Qry.FieldAsInteger("kiosk_bp");

    row.i_stat.mobile = Qry.FieldAsInteger("mobile");
    row.i_stat.mobile_bag = Qry.FieldAsInteger("mobile_bag");
    row.i_stat.mobile_bp = Qry.FieldAsInteger("mobile_bp");
    row.f = Qry.FieldAsInteger("f");
    row.c = Qry.FieldAsInteger("c");
    row.y = Qry.FieldAsInteger("y");

    if (!params.skip_rows)
    {
      TDetailStatKey key;
      key.pact_descr = pact_descr;
      if(params.airp_column_first) {
          key.col1 = ElemIdToCodeNative(etAirp, Qry.FieldAsString("airp"));
          if (params.statType==statDetail)
          {
            key.col2 = ElemIdToCodeNative(etAirline, Qry.FieldAsString("airline"));
            airline.check(key.col2);
          };
      } else {
          key.col1 = ElemIdToCodeNative(etAirline, Qry.FieldAsString("airline"));
          if (params.statType==statDetail)
          {
            key.col2 = ElemIdToCodeNative(etAirp, Qry.FieldAsString("airp"));
          };
          airline.check(key.col1);
      }

      AddStatRow(key, row, DetailStat, full);
    }
    else
    {
      DetailStatTotal+=row;
    };
  }
};

void GetFullStat(const TStatParams &params, TQuery &Qry,
                 TFullStat &FullStat, TFullStatRow &FullStatTotal,
                 TPrintAirline &airline, bool full = false)
{
  Qry.Execute();
  if(!Qry.Eof) {
      int col_point_id = Qry.FieldIndex("point_id");
      int col_airp = Qry.FieldIndex("airp");
      int col_airline = Qry.FieldIndex("airline");
      int col_pax_amount = Qry.FieldIndex("pax_amount");

      int col_web = -1;
      int col_web_bag = -1;
      int col_web_bp = -1;

      int col_kiosk = -1;
      int col_kiosk_bag = -1;
      int col_kiosk_bp = -1;

      int col_mobile = -1;
      int col_mobile_bag = -1;
      int col_mobile_bp = -1;

      if (params.statType==statFull)
      {
        col_web = Qry.FieldIndex("web");
        col_web_bag = Qry.FieldIndex("web_bag");
        col_web_bp = Qry.FieldIndex("web_bp");

        col_kiosk = Qry.FieldIndex("kiosk");
        col_kiosk_bag = Qry.FieldIndex("kiosk_bag");
        col_kiosk_bp = Qry.FieldIndex("kiosk_bp");

        col_mobile = Qry.FieldIndex("mobile");
        col_mobile_bag = Qry.FieldIndex("mobile_bag");
        col_mobile_bp = Qry.FieldIndex("mobile_bp");
      };
      int col_adult = Qry.FieldIndex("adult");
      int col_child = Qry.FieldIndex("child");
      int col_baby = Qry.FieldIndex("baby");
      int col_rk_weight = Qry.FieldIndex("rk_weight");
      int col_bag_amount = Qry.FieldIndex("bag_amount");
      int col_bag_weight = Qry.FieldIndex("bag_weight");
      int col_excess = Qry.FieldIndex("excess");
      int col_excess_pc = Qry.FieldIndex("excess_pc");
      int col_flt_no = Qry.FieldIndex("flt_no");
      int col_scd_out = Qry.FieldIndex("scd_out");
      int col_places = Qry.GetFieldIndex("places");
      int col_part_key = Qry.GetFieldIndex("part_key");
      for(; !Qry.Eof; Qry.Next())
      {
        TFullStatRow row;
        row.pax_amount = Qry.FieldAsInteger(col_pax_amount);
        if (params.statType==statFull)
        {
          row.i_stat.web = Qry.FieldAsInteger(col_web);
          row.i_stat.web_bag = Qry.FieldAsInteger(col_web_bag);
          row.i_stat.web_bp = Qry.FieldAsInteger(col_web_bp);

          row.i_stat.kiosk = Qry.FieldAsInteger(col_kiosk);
          row.i_stat.kiosk_bag = Qry.FieldAsInteger(col_kiosk_bag);
          row.i_stat.kiosk_bp = Qry.FieldAsInteger(col_kiosk_bp);

          row.i_stat.mobile = Qry.FieldAsInteger(col_mobile);
          row.i_stat.mobile_bag = Qry.FieldAsInteger(col_mobile_bag);
          row.i_stat.mobile_bp = Qry.FieldAsInteger(col_mobile_bp);
        };
        row.adult = Qry.FieldAsInteger(col_adult);
        row.child = Qry.FieldAsInteger(col_child);
        row.baby = Qry.FieldAsInteger(col_baby);
        row.rk_weight = Qry.FieldAsInteger(col_rk_weight);
        row.bag_amount = Qry.FieldAsInteger(col_bag_amount);
        row.bag_weight = Qry.FieldAsInteger(col_bag_weight);
        row.paid.set(Qry.FieldAsInteger(col_excess), Qry.FieldAsInteger(col_excess_pc));
        if (!params.skip_rows)
        {
          TFullStatKey key;
          key.airp = Qry.FieldAsString(col_airp);
          if(params.airp_column_first) {
              key.col1 = ElemIdToCodeNative(etAirp, Qry.FieldAsString(col_airp));
              key.col2 = ElemIdToCodeNative(etAirline, Qry.FieldAsString(col_airline));
              airline.check(key.col2);
          } else {
              key.col1 = ElemIdToCodeNative(etAirline, Qry.FieldAsString(col_airline));
              key.col2 = ElemIdToCodeNative(etAirp, Qry.FieldAsString(col_airp));
              airline.check(key.col1);
          }
          key.flt_no = Qry.FieldAsInteger(col_flt_no);
          key.scd_out = Qry.FieldAsDateTime(col_scd_out);
          key.point_id = Qry.FieldAsInteger(col_point_id);
          if (params.statType==statTrferFull)
            key.places.set(Qry.FieldAsString(col_places), true);
          else
            key.places.set(GetRouteAfterStr( col_part_key>=0?Qry.FieldAsDateTime(col_part_key):NoExists,
                                             Qry.FieldAsInteger(col_point_id),
                                             trtNotCurrent,
                                             trtNotCancelled),
                           false);
          AddStatRow(key, row, FullStat, full);
        }
        else
        {
          FullStatTotal+=row;
        };
      }
  }
};

struct TPact {
    string descr, airline, airp;
    vector<string> airlines; // Список ак, который исключается из ап договора.
    TDateTime first_date, last_date;
    TPact(string vdescr, string vairline, string vairp, TDateTime vfirst_date, TDateTime vlast_date):
        descr(vdescr),
        airline(vairline),
        airp(vairp),
        first_date(vfirst_date),
        last_date(vlast_date)
    {};
    void dump();
};

void TPact::dump()
{
    ProgTrace(TRACE5, "------TPact-------");
    ProgTrace(TRACE5, "descr: %s", descr.c_str());
    ProgTrace(TRACE5, "airline: %s", airline.c_str());
    ProgTrace(TRACE5, "airp: %s", airp.c_str());
    string buf;
    for(vector<string>::iterator iv = airlines.begin(); iv != airlines.end(); iv++) {
        if(not buf.empty())
            buf += ", ";
        buf += *iv;
    }
    ProgTrace(TRACE5, "airlines: %s", buf.c_str());
    ProgTrace(TRACE5, "first_date: %s", DateTimeToStr(first_date, "ddmmyy").c_str());
    ProgTrace(TRACE5, "last_date: %s", DateTimeToStr(last_date, "ddmmyy").c_str());
}

void correct_airp_pacts(vector<TPact> &airp_pacts, TPact &airline_pact)
{
    if(airp_pacts.empty())
        return;
    vector<TPact> added_pacts;
    for(vector<TPact>::iterator iv = airp_pacts.begin(); iv != airp_pacts.end(); iv++) {
        if(
                airline_pact.first_date >= iv->first_date and
                airline_pact.first_date < iv->last_date and
                airline_pact.last_date >= iv->last_date
          ) {// Левый край ак договора попал, разбиваем ап договор на 2 части и добавляем ак в список исключенных для этого ап договора
            TPact new_pact = *iv;
            new_pact.first_date = airline_pact.first_date;
            iv->last_date = airline_pact.first_date;
            new_pact.airlines.push_back(airline_pact.airline);
            added_pacts.push_back(new_pact);
        } if(
                airline_pact.last_date >= iv->first_date and
                airline_pact.last_date < iv->last_date and
                airline_pact.first_date < iv->first_date
          ) { // правый край ак договора попал
            TPact new_pact = *iv;
            new_pact.last_date = airline_pact.last_date;
            iv->first_date = airline_pact.last_date;
            new_pact.airlines.push_back(airline_pact.airline);
            added_pacts.push_back(new_pact);
        } if(
                airline_pact.first_date >= iv->first_date and
                airline_pact.last_date < iv->last_date
          ) { // ак договор целиком внутри периода ап договора, разбиваем ап договор на 3 части.
            TPact new_pact = *iv;
            TPact new_pact1 = *iv;
            new_pact.first_date = airline_pact.first_date;
            new_pact.last_date = airline_pact.last_date;
            new_pact1.first_date = airline_pact.last_date;
            iv->last_date = airline_pact.first_date;
            new_pact.airlines.push_back(airline_pact.airline);
            added_pacts.push_back(new_pact);
            added_pacts.push_back(new_pact1);
        }
        if(
                airline_pact.first_date < iv->first_date and
                airline_pact.last_date >= iv->last_date
          ) { // ап договор целиком внутри ак договора, просто добавляем в список исключаемых ак этого дог. текущую.
            iv->airlines.push_back(airline_pact.airline);
        }
    }
    airp_pacts.insert(airp_pacts.end(), added_pacts.begin(), added_pacts.end());
    while(true) {
        vector<TPact>::iterator iv = airp_pacts.begin();
        for(; iv != airp_pacts.end(); iv++) {
            if(iv->descr.empty() or iv->first_date == iv->last_date)
                break;
        }
        if(iv == airp_pacts.end())
            break;
        airp_pacts.erase(iv);
    }
}

void setClientTypeCaps(xmlNodePtr variablesNode)
{
    NewTextChild(variablesNode, "kiosks", getLocaleText("CAP.KIOSKS"));
    NewTextChild(variablesNode, "pax", getLocaleText("CAP.PAX"));
    NewTextChild(variablesNode, "mob", getLocaleText("CAP.MOB"));
    NewTextChild(variablesNode, "mobile_devices", getLocaleText("CAP.MOBILE_DEVICES"));
}

void createXMLDetailStat(const TStatParams &params, bool pr_pact,
                         const TDetailStat &DetailStat, const TDetailStatRow &DetailStatTotal,
                         const TPrintAirline &airline, xmlNodePtr resNode)
{
    if(DetailStat.empty() && DetailStatTotal==TDetailStatRow())
      throw AstraLocale::UserException("MSG.NOT_DATA");

    NewTextChild(resNode, "airline", airline.get(), "");
    xmlNodePtr grdNode = NewTextChild(resNode, "grd");
    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    TDetailStatRow total;
    bool showTotal=true;
    if (!params.skip_rows)
    {
      int rows = 0;
      for(TDetailStat::const_iterator si = DetailStat.begin(); si != DetailStat.end(); ++si, rows++)
      {
          if(rows >= MAX_STAT_ROWS()) {
              throw MaxStatRowsException("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH", LParams() << LParam("num", MAX_STAT_ROWS()));
              /*AstraLocale::showErrorMessage("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH",
                      LParams() << LParam("num", MAX_STAT_ROWS()));
              if (WITHOUT_TOTAL_WHEN_PROBLEM) showTotal=false; //не будем показывать итоговую строку дабы не ввести в заблуждение
              break;*/
          }

          rowNode = NewTextChild(rowsNode, "row");
          if(params.statType != statPactShort)
              NewTextChild(rowNode, "col", si->first.col1);
          if (params.statType==statDetail)
              NewTextChild(rowNode, "col", si->first.col2);

          if(params.statType != statPactShort)
              NewTextChild(rowNode, "col", (int)(pr_pact?si->second.flts.size():si->second.flt_amount));
          if(params.statType == statPactShort)
              NewTextChild(rowNode, "col", si->first.pact_descr);
          NewTextChild(rowNode, "col", si->second.pax_amount);
          if(params.statType != statPactShort) {
              NewTextChild(rowNode, "col", si->second.f);
              NewTextChild(rowNode, "col", si->second.c);
              NewTextChild(rowNode, "col", si->second.y);

              NewTextChild(rowNode, "col", si->second.i_stat.web);
              NewTextChild(rowNode, "col", si->second.i_stat.web_bag);
              NewTextChild(rowNode, "col", si->second.i_stat.web_bp);

              NewTextChild(rowNode, "col", si->second.i_stat.kiosk);
              NewTextChild(rowNode, "col", si->second.i_stat.kiosk_bag);
              NewTextChild(rowNode, "col", si->second.i_stat.kiosk_bp);

              NewTextChild(rowNode, "col", si->second.i_stat.mobile);
              NewTextChild(rowNode, "col", si->second.i_stat.mobile_bag);
              NewTextChild(rowNode, "col", si->second.i_stat.mobile_bp);
          }
          if(pr_pact and params.statType != statPactShort)
              NewTextChild(rowNode, "col", si->first.pact_descr);

          total += si->second;
      };
    }
    else total=DetailStatTotal;

    rowNode = NewTextChild(rowsNode, "row");

    xmlNodePtr colNode;
    if(params.statType != statPactShort) {
        if(params.airp_column_first) {
            colNode = NewTextChild(headerNode, "col", getLocaleText("Код а/п"));
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", TAlignment::LeftJustify);
            SetProp(colNode, "sort", sortString);
            NewTextChild(rowNode, "col", getLocaleText("Итого:"));
            if (params.statType==statDetail)
            {
                colNode = NewTextChild(headerNode, "col", getLocaleText("Код а/к"));
                SetProp(colNode, "width", 50);
                SetProp(colNode, "align", TAlignment::LeftJustify);
                SetProp(colNode, "sort", sortString);
                NewTextChild(rowNode, "col");
            };
        } else {
            colNode = NewTextChild(headerNode, "col", getLocaleText("Код а/к"));
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", TAlignment::LeftJustify);
            SetProp(colNode, "sort", sortString);
            NewTextChild(rowNode, "col", getLocaleText("Итого:"));
            if (params.statType==statDetail)
            {
                colNode = NewTextChild(headerNode, "col", getLocaleText("Код а/п"));
                SetProp(colNode, "width", 50);
                SetProp(colNode, "align", TAlignment::LeftJustify);
                SetProp(colNode, "sort", sortString);
                NewTextChild(rowNode, "col");
            };
        }
    }

    if(params.statType != statPactShort) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("Кол-во рейсов"));
        SetProp(colNode, "width", 85);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", (int)(pr_pact?total.flts.size():total.flt_amount));
    }

    if(params.statType == statPactShort)
    {
        colNode = NewTextChild(headerNode, "col", getLocaleText("№ договора"));
        SetProp(colNode, "width", 230);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col", getLocaleText("Итого:"));
    }

    colNode = NewTextChild(headerNode, "col", getLocaleText("Кол-во пасс."));
    SetProp(colNode, "width", 85);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.pax_amount);

    if(params.statType != statPactShort) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("П"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.f);

        colNode = NewTextChild(headerNode, "col", getLocaleText("Б"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.c);

        colNode = NewTextChild(headerNode, "col", getLocaleText("Э"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.y);

        total.i_stat.toXML(headerNode, rowNode);
    }

    if(pr_pact and params.statType != statPactShort)
    {
        colNode = NewTextChild(headerNode, "col", getLocaleText("№ договора"));
        SetProp(colNode, "width", 230);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col");
    }

    if (!showTotal)
    {
      xmlUnlinkNode(rowNode);
      xmlFreeNode(rowNode);
    };

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    setClientTypeCaps(variablesNode);
    NewTextChild(variablesNode, "pr_pact", pr_pact);
    if(params.statType == statPactShort) {
        NewTextChild(variablesNode, "stat_type", params.statType);
        NewTextChild(variablesNode, "stat_mode", getLocaleText("Договоры на использование DCS АСТРА"));
        string stat_type_caption;
        switch(params.statType) {
            case statPactShort:
                stat_type_caption = getLocaleText("Общая");
                break;
            default:
                throw Exception("createXMLDetailStat: unexpected statType %d", params.statType);
                break;
        }
        NewTextChild(variablesNode, "stat_type_caption", stat_type_caption);
    }
};

void createXMLFullStat(const TStatParams &params,
                       const TFullStat &FullStat, const TFullStatRow &FullStatTotal,
                       const TPrintAirline &airline, xmlNodePtr resNode)
{
    if(FullStat.empty() && FullStatTotal==TFullStatRow())
      throw AstraLocale::UserException("MSG.NOT_DATA");

    NewTextChild(resNode, "airline", airline.get(), "");
    xmlNodePtr grdNode = NewTextChild(resNode, "grd");
    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    TFullStatRow total;
    bool showTotal=true;
    if (!params.skip_rows)
    {
      int rows = 0;
      for(TFullStat::const_iterator im = FullStat.begin(); im != FullStat.end(); ++im, rows++)
      {
          if(rows >= MAX_STAT_ROWS()) {
              throw MaxStatRowsException("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH", LParams() << LParam("num", MAX_STAT_ROWS()));
              /*AstraLocale::showErrorMessage("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH",
                                            LParams() << LParam("num", MAX_STAT_ROWS()));
              if (WITHOUT_TOTAL_WHEN_PROBLEM) showTotal=false; //не будем показывать итоговую строку дабы не ввести в заблуждение
              break;*/
          }
          //region обязательно в начале цикла, иначе будет испорчен xml
          string region;
          try
          {
              region = AirpTZRegion(im->first.airp);
          }
          catch(AstraLocale::UserException &E)
          {
              AstraLocale::showErrorMessage("MSG.ERR_MSG.NOT_ALL_FLIGHTS_ARE_SHOWN", LParams() << LParam("msg", getLocaleText(E.getLexemaData())));
              if (WITHOUT_TOTAL_WHEN_PROBLEM) showTotal=false; //не будем показывать итоговую строку дабы не ввести в заблуждение
              continue;
          };
          rowNode = NewTextChild(rowsNode, "row");
          NewTextChild(rowNode, "col", im->first.col1);
          NewTextChild(rowNode, "col", im->first.col2);
          NewTextChild(rowNode, "col", im->first.flt_no);
          NewTextChild(rowNode, "col", DateTimeToStr(
                      UTCToClient(im->first.scd_out, region), "dd.mm.yy")
                  );
          NewTextChild(rowNode, "col", im->first.places.get());
          NewTextChild(rowNode, "col", im->second.pax_amount);
          if (params.statType==statFull)
          {
              NewTextChild(rowNode, "col", im->second.i_stat.web);
              NewTextChild(rowNode, "col", im->second.i_stat.web_bag);
              NewTextChild(rowNode, "col", im->second.i_stat.web_bp);

              NewTextChild(rowNode, "col", im->second.i_stat.kiosk);
              NewTextChild(rowNode, "col", im->second.i_stat.kiosk_bag);
              NewTextChild(rowNode, "col", im->second.i_stat.kiosk_bp);

              NewTextChild(rowNode, "col", im->second.i_stat.mobile);
              NewTextChild(rowNode, "col", im->second.i_stat.mobile_bag);
              NewTextChild(rowNode, "col", im->second.i_stat.mobile_bp);
          };
          NewTextChild(rowNode, "col", im->second.adult);
          NewTextChild(rowNode, "col", im->second.child);
          NewTextChild(rowNode, "col", im->second.baby);
          NewTextChild(rowNode, "col", im->second.rk_weight);

          NewTextChild(rowNode, "col", im->second.bag_amount);
          NewTextChild(rowNode, "col", im->second.bag_weight);

          NewTextChild(rowNode, "col", im->second.paid.excess_pcs);
          NewTextChild(rowNode, "col", im->second.paid.excess);
          if (params.statType==statTrferFull)
              NewTextChild(rowNode, "col", im->first.point_id);

          total += im->second;
      };
    }
    else total=FullStatTotal;

    rowNode = NewTextChild(rowsNode, "row");

    xmlNodePtr colNode;
    if(params.airp_column_first) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("Код а/п"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col", getLocaleText("Итого:"));

        colNode = NewTextChild(headerNode, "col", getLocaleText("Код а/к"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col");
    } else {
        colNode = NewTextChild(headerNode, "col", getLocaleText("Код а/к"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col", getLocaleText("Итого:"));

        colNode = NewTextChild(headerNode, "col", getLocaleText("Код а/п"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col");
    }
    colNode = NewTextChild(headerNode, "col", getLocaleText("Номер рейса"));
    SetProp(colNode, "width", 75);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col");

    colNode = NewTextChild(headerNode, "col", getLocaleText("Дата"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDate);
    NewTextChild(rowNode, "col");

    colNode = NewTextChild(headerNode, "col", getLocaleText("Направление"));
    SetProp(colNode, "width", 90);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col");

    colNode = NewTextChild(headerNode, "col", getLocaleText("Кол-во пасс."));
    SetProp(colNode, "width", 75);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.pax_amount);

    if (params.statType==statFull)
        total.i_stat.toXML(headerNode, rowNode);

    colNode = NewTextChild(headerNode, "col", getLocaleText("ВЗ"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.adult);

    colNode = NewTextChild(headerNode, "col", getLocaleText("РБ"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.child);

    colNode = NewTextChild(headerNode, "col", getLocaleText("РМ"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.baby);

    colNode = NewTextChild(headerNode, "col", getLocaleText("Р/кладь (вес)"));
    SetProp(colNode, "width", 80);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.rk_weight);

    colNode = NewTextChild(headerNode, "col", getLocaleText("БГ мест"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortIntegerSlashInteger);
    NewTextChild(rowNode, "col", total.bag_amount);

    colNode = NewTextChild(headerNode, "col", getLocaleText("БГ вес"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortIntegerSlashInteger);
    NewTextChild(rowNode, "col", total.bag_weight);

    colNode = NewTextChild(headerNode, "col", getLocaleText("Пл.м"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col",  total.paid.excess_pcs);

    colNode = NewTextChild(headerNode, "col", getLocaleText("Пл.вес"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col",  total.paid.excess);

    if (!showTotal)
    {
      xmlUnlinkNode(rowNode);
      xmlFreeNode(rowNode);
    };

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    setClientTypeCaps(variablesNode);
    if (params.statType==statFull)
      NewTextChild(variablesNode, "caption", getLocaleText("Подробная сводка"));
    else
      NewTextChild(variablesNode, "caption", getLocaleText("Трансферная сводка"));
};

void RunPactDetailStat(const TStatParams &params,
                       TDetailStat &DetailStat, TDetailStatRow &DetailStatTotal,
                       TPrintAirline &prn_airline, bool full = false)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select\n"
        "    descr,\n"
        "    airline,\n"
        "    airp,\n"
        "    DECODE(SIGN(first_date-:FirstDate),1,first_date,:FirstDate) AS first_date,\n"
        "    DECODE(SIGN(last_date- :LastDate),-1,last_date, :LastDate) AS last_date\n"
        "from pacts where\n"
        "  first_date >= :FirstDate and first_date < :LastDate or\n"
        "  last_date >= :FirstDate and last_date < :LastDate or\n"
        "  first_date < :FirstDate and (last_date >= :LastDate or last_date is null)\n"
        "order by \n"
        "  airline nulls first \n";

    Qry.CreateVariable("FirstDate", otDate, params.FirstDate);
    Qry.CreateVariable("LastDate", otDate, params.LastDate);
    Qry.Execute();
    map<string, vector<TPact> > airp_pacts;
    vector<TPact> result_pacts;
    for(; !Qry.Eof; Qry.Next()) {
        TPact pact(
                Qry.FieldAsString("descr"),
                Qry.FieldAsString("airline"),
                Qry.FieldAsString("airp"),
                Qry.FieldAsDateTime("first_date"),
                Qry.FieldAsDateTime("last_date")
                );
        if(pact.airline.empty())
            airp_pacts[pact.airp].push_back(pact);
        else {
            /*
            ProgTrace(TRACE5, "before correct_airp_pacts:");
            ProgTrace(TRACE5, "descr: %s, first_date: %s, last_date: %s",
                    pact.descr.c_str(),
                    DateTimeToStr(pact.first_date, "ddmmyy").c_str(),
                    DateTimeToStr(pact.last_date, "ddmmyy").c_str()
                    );*/
            /*
            for(map<string, vector<TPact> >::iterator im = airp_pacts.begin(); im != airp_pacts.end(); im++)
                correct_airp_pacts(im->second, pact);
                */
            correct_airp_pacts(airp_pacts[pact.airp], pact);

            result_pacts.push_back(pact);
        }
    }
    /*
    ProgTrace(TRACE5, "AIRP_PACTS:");
    for(map<string, vector<TPact> >::iterator im = airp_pacts.begin(); im != airp_pacts.end(); im++) {
        ProgTrace(TRACE5, "airp: %s", im->first.c_str());
        ProgTrace(TRACE5, "periods:");
        for(vector<TPact>::iterator iv = im->second.begin(); iv != im->second.end(); iv++) {
            ProgTrace(TRACE5, "first_date: %s, last_date: %s",
                    DateTimeToStr(iv->first_date, "ddmmyy").c_str(),
                    DateTimeToStr(iv->last_date, "ddmmyy").c_str()
                    );
            string denied;
            for(set<string>::iterator is = iv->airlines.begin(); is != iv->airlines.end(); is++) {
                if(not denied.empty())
                    denied += ", ";
                denied += *is;
            }
            ProgTrace(TRACE5, "denied airlines: %s", denied.c_str());
        }
    }
    */
    for(map<string, vector<TPact> >::iterator im = airp_pacts.begin(); im != airp_pacts.end(); im++) {
        if(im->second.empty())
            continue;
        result_pacts.insert(result_pacts.end(), im->second.begin(), im->second.end());
    }

    Qry.Clear();
    for(int pass = 0; pass <= 2; pass++) {
        ostringstream sql;
        sql << "SELECT \n"
            " points.airline, \n"
            " points.airp, \n"
            " points.scd_out, \n"
            " stat.point_id, \n"
            " adult, \n"
            " child, \n"
            " baby, \n"
            " term_bp, \n"
            " term_bag, \n"
            " f, c, y, \n"
            " client_type \n"
            "FROM \n";
        if (pass!=0)
        {
            sql << " arx_points points, \n"
                " arx_stat stat \n";
            if (pass==2)
                sql << ",(SELECT part_key, move_id FROM move_arx_ext \n"
                    "  WHERE part_key >= :last_date+:arx_trip_date_range AND part_key <= :last_date+date_range) arx_ext \n";
        }
        else
            sql << " points, \n"
                " stat \n";
        sql << "WHERE \n";
        if (pass==1)
            sql << " points.part_key >= :first_date AND points.part_key < :last_date + :arx_trip_date_range AND \n";
        if (pass==2)
            sql << " points.part_key=arx_ext.part_key AND points.move_id=arx_ext.move_id AND \n";
        if (pass!=0)
            sql << " stat.part_key = points.part_key AND \n";
        sql << " stat.point_id = points.point_id AND \n"
            " points.pr_del>=0 AND \n"
            " points.scd_out >= :first_date AND points.scd_out < :last_date \n";

        if(params.flt_no != NoExists) {
            sql << " AND points.flt_no = :flt_no \n";
            Qry.CreateVariable("flt_no", otInteger, params.flt_no);
        }
        if (!params.airps.elems().empty()) {
            if (params.airps.elems_permit())
                sql << " AND points.airp IN " << GetSQLEnum(params.airps.elems()) << "\n";
            else
                sql << " AND points.airp NOT IN " << GetSQLEnum(params.airps.elems()) << "\n";
        };
        if (!params.airlines.elems().empty()) {
            if (params.airlines.elems_permit())
                sql << " AND points.airline IN " << GetSQLEnum(params.airlines.elems()) << "\n";
            else
                sql << " AND points.airline NOT IN " << GetSQLEnum(params.airlines.elems()) << "\n";
        };

        //ProgTrace(TRACE5, "RunPactDetailStat: pass=%d SQL=\n%s", pass, sql.str().c_str());
        Qry.SQLText = sql.str().c_str();
        if (pass!=0)
            Qry.CreateVariable("arx_trip_date_range", otInteger, ARX_TRIP_DATE_RANGE());

        Qry.CreateVariable("first_date", otDate, params.FirstDate);
        Qry.CreateVariable("last_date", otDate, params.LastDate);
        Qry.Execute();
        if(not Qry.Eof) {
            int col_airline = Qry.FieldIndex("airline");
            int col_airp = Qry.FieldIndex("airp");
            int col_scd_out = Qry.FieldIndex("scd_out");
            int col_point_id = Qry.FieldIndex("point_id");
            int col_adult = Qry.FieldIndex("adult");
            int col_child = Qry.FieldIndex("child");
            int col_baby = Qry.FieldIndex("baby");
            int col_term_bp = Qry.FieldIndex("term_bp");
            int col_term_bag = Qry.FieldIndex("term_bag");
            int col_f = Qry.FieldIndex("f");
            int col_c = Qry.FieldIndex("c");
            int col_y = Qry.FieldIndex("y");
            int col_client_type = Qry.FieldIndex("client_type");
            for(; not Qry.Eof; Qry.Next())
            {
                TDetailStatRow row;
                TClientType client_type = DecodeClientType(Qry.FieldAsString(col_client_type));
                int pax_amount=Qry.FieldAsInteger(col_adult) +
                    Qry.FieldAsInteger(col_child) +
                    Qry.FieldAsInteger(col_baby);

                row.flts.insert(Qry.FieldAsInteger(col_point_id));
                row.pax_amount = pax_amount;

                row.i_stat.web = (client_type == ctWeb ? pax_amount : 0);
                row.i_stat.kiosk = (client_type == ctKiosk ? pax_amount : 0);
                row.i_stat.mobile = (client_type == ctMobile ? pax_amount : 0);

                row.f = Qry.FieldAsInteger(col_f);
                row.c = Qry.FieldAsInteger(col_c);
                row.y = Qry.FieldAsInteger(col_y);

                int term_bp = Qry.FieldAsInteger(col_term_bp);
                int term_bag = Qry.FieldAsInteger(col_term_bag);

                row.i_stat.web_bag = (client_type == ctWeb ? term_bag : 0);
                row.i_stat.web_bp = (client_type == ctWeb ? term_bp : 0);

                row.i_stat.kiosk_bag = (client_type == ctKiosk ? term_bag : 0);
                row.i_stat.kiosk_bp = (client_type == ctKiosk ? term_bp : 0);

                row.i_stat.mobile_bag = (client_type == ctMobile ? term_bag : 0);
                row.i_stat.mobile_bp = (client_type == ctMobile ? term_bp : 0);

                if (!params.skip_rows)
                {
                    string airline = Qry.FieldAsString(col_airline);
                    string airp = Qry.FieldAsString(col_airp);
                    TDateTime scd_out = Qry.FieldAsDateTime(col_scd_out);

                    vector<TPact>::iterator iv = result_pacts.begin();
                    for(; iv != result_pacts.end(); iv++) {
                        if(
                                scd_out >= iv->first_date and
                                scd_out < iv->last_date and
                                airp == iv->airp and
                                (iv->airline.empty() or iv->airline == airline) and
                                (iv->airlines.empty() or find(iv->airlines.begin(), iv->airlines.end(), airline) == iv->airlines.end())
                          )
                        {
                            break;
                        }
                    }
                    TDetailStatKey key;
                    if(iv != result_pacts.end())
                        key.pact_descr = iv->descr;
                    if(
                            params.statType == statDetail or
                            params.statType == statShort
                      ) {
                        if(params.airp_column_first) {
                            key.col1 = ElemIdToCodeNative(etAirp, airp);
                            if (params.statType==statDetail)
                            {
                                key.col2 = ElemIdToCodeNative(etAirline, airline);
                                prn_airline.check(key.col2);
                            };
                        } else {
                            key.col1 = ElemIdToCodeNative(etAirline, airline);
                            if (params.statType==statDetail)
                            {
                                key.col2 = ElemIdToCodeNative(etAirp, airp);
                            };
                            prn_airline.check(key.col1);
                        }
                    }

                    AddStatRow(key, row, DetailStat, full);
                }
                else
                {
                    DetailStatTotal+=row;
                };
            }
        }
    }
}

void RunDetailStat(const TStatParams &params,
                   TDetailStat &DetailStat, TDetailStatRow &DetailStatTotal,
                   TPrintAirline &airline, bool full = false)
{
    TQuery Qry(&OraSession);
    Qry.CreateVariable("FirstDate", otDate, params.FirstDate);
    Qry.CreateVariable("LastDate", otDate, params.LastDate);
    Qry.CreateVariable("web", otString, EncodeClientType(ctWeb));
    Qry.CreateVariable("kiosk", otString, EncodeClientType(ctKiosk));
    Qry.CreateVariable("mobile", otString, EncodeClientType(ctMobile));
    if (params.seance==seanceAirline) Qry.DeclareVariable("ak",otString);
    if (params.seance==seanceAirport) Qry.DeclareVariable("ap",otString);
    if(params.flt_no != NoExists)
        Qry.CreateVariable("flt_no", otString, params.flt_no);

    for(int pass = 0; pass <= 2; pass++) {
          Qry.SQLText = GetStatSQLText(params,pass).c_str();
        if (pass!=0)
          Qry.CreateVariable("arx_trip_date_range", otInteger, ARX_TRIP_DATE_RANGE());

        if (params.seance==seanceAirline)
        {
          //цикл по компаниям
          if (params.airlines.elems_permit())
          {
            for(set<string>::const_iterator i=params.airlines.elems().begin();
                                            i!=params.airlines.elems().end(); i++)
            {
              Qry.SetVariable("ak",*i);
              GetDetailStat(params, Qry, DetailStat, DetailStatTotal, airline, "", full);
            };
          };
          continue;
        };

        if (params.seance==seanceAirport)
        {
          //цикл по портам
          if (params.airps.elems_permit())
          {
            for(set<string>::const_iterator i=params.airps.elems().begin();
                                            i!=params.airps.elems().end(); i++)
            {
              Qry.SetVariable("ap",*i);
              GetDetailStat(params, Qry, DetailStat, DetailStatTotal, airline, "", full);
            };
          };
          continue;
        };
        GetDetailStat(params, Qry, DetailStat, DetailStatTotal, airline, "", full);
    }
};

struct TDetailStatCombo : public TOrderStatItem
{
    typedef std::pair<TDetailStatKey, TDetailStatRow> Tdata;
    Tdata data;
    TStatParams params;
    bool pr_pact;
    TDetailStatCombo(const Tdata &aData, const TStatParams &aParams)
        : data(aData), params(aParams), pr_pact(aParams.pr_pacts) {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void TDetailStatCombo::add_header(ostringstream &buf) const
{
    if(params.statType != statPactShort)
    {
        if (params.airp_column_first)
        {
            buf << "Код а/п" << delim;
            if (params.statType==statDetail)
                buf << "Код а/к" << delim;
        } else {
            buf << "Код а/к" << delim;
            if (params.statType==statDetail)
                buf << "Код а/п" << delim;
        }
    }
    if(params.statType != statPactShort)
        buf << "Кол-во рейсов" << delim;
    if(params.statType == statPactShort)
        buf << "№ договора" << delim;
    buf << "Кол-во пасс." << delim;
    if (params.statType != statPactShort)
    {
        buf << "П" << delim;
        buf << "Б" << delim;
        buf << "Э" << delim;
        /* TInetStat::toXML begin */
        buf << "Web" << delim;
        buf << "БГ" << delim;
        buf << "ПТ" << delim;
        buf << "Киоски" << delim;
        buf << "БГ" << delim;
        buf << "ПТ" << delim;
        buf << "Моб." << delim;
        buf << "БГ" << delim;
        buf << "ПТ" << delim;
        /* end */
    }
    if(pr_pact and params.statType != statPactShort)
        buf << "№ договора" << delim;
    buf << endl; /* остаётся пустой столбец */
}

void TDetailStatCombo::add_data(ostringstream &buf) const
{
    if(params.statType != statPactShort)
        buf << data.first.col1 << delim; // col1
    if (params.statType == statDetail)
        buf << data.first.col2 << delim; // col2
    if(params.statType != statPactShort)
        buf << (int)(pr_pact ? data.second.flts.size() : data.second.flt_amount) << delim; // Кол-во рейсов
    if(params.statType == statPactShort)
        buf << data.first.pact_descr << delim; // № договора
    buf << data.second.pax_amount << delim; // Кол-во пасс.
    if (params.statType != statPactShort)
    {
        buf << data.second.f << delim; // П
        buf << data.second.c << delim; // Б
        buf << data.second.y << delim; // Э
        buf << data.second.i_stat.web << delim; // Web
        buf << data.second.i_stat.web_bag << delim; // БГ
        buf << data.second.i_stat.web_bp << delim; // ПТ
        buf << data.second.i_stat.kiosk << delim; // Киоски
        buf << data.second.i_stat.kiosk_bag << delim; // БГ
        buf << data.second.i_stat.kiosk_bp << delim; // ПТ
        buf << data.second.i_stat.mobile << delim; // Моб.
        buf << data.second.i_stat.mobile_bag << delim; // БГ
        buf << data.second.i_stat.mobile_bp << delim; // ПТ
    }
    if(pr_pact and params.statType != statPactShort)
        buf << data.first.pact_descr << delim; // № договора
    buf << endl; /* остаётся пустой столбец */
}

template <class T>
void RunDetailStatFile(const TStatParams &params, T &writer, TPrintAirline &prn_airline)
{
    TDetailStat DetailStat;
    TDetailStatRow DetailStatTotal;
    if(params.pr_pacts)
        RunPactDetailStat(params, DetailStat, DetailStatTotal, prn_airline, true);
    else
        RunDetailStat(params, DetailStat, DetailStatTotal, prn_airline, true);
    for (TDetailStat::const_iterator i = DetailStat.begin(); i != DetailStat.end(); ++i)
        writer.insert(TDetailStatCombo(*i, params));
}

void RunFullStat(const TStatParams &params,
                 TFullStat &FullStat, TFullStatRow &FullStatTotal,
                 TPrintAirline &airline, bool full = false)
{
    TQuery Qry(&OraSession);
    Qry.CreateVariable("FirstDate", otDate, params.FirstDate);
    Qry.CreateVariable("LastDate", otDate, params.LastDate);
    if (params.statType==statFull)
    {
      Qry.CreateVariable("web", otString, EncodeClientType(ctWeb));
      Qry.CreateVariable("kiosk", otString, EncodeClientType(ctKiosk));
      Qry.CreateVariable("mobile", otString, EncodeClientType(ctMobile));
    };
    if (params.seance==seanceAirline) Qry.DeclareVariable("ak",otString);
    if (params.seance==seanceAirport) Qry.DeclareVariable("ap",otString);
    if(params.flt_no != NoExists)
        Qry.CreateVariable("flt_no", otString, params.flt_no);

    for(int pass = 0; pass <= 2; pass++) {
          Qry.SQLText = GetStatSQLText(params,pass).c_str();
        if (pass!=0)
          Qry.CreateVariable("arx_trip_date_range", otInteger, ARX_TRIP_DATE_RANGE());
        //ProgTrace(TRACE5, "RunFullStat: pass=%d SQL=\n%s", pass, Qry.SQLText.SQLText());

        if (params.seance==seanceAirline)
        {
          //цикл по компаниям
          if (params.airlines.elems_permit())
          {
            for(set<string>::const_iterator i=params.airlines.elems().begin();
                                            i!=params.airlines.elems().end(); i++)
            {
              Qry.SetVariable("ak",*i);
              GetFullStat(params, Qry, FullStat, FullStatTotal, airline, full);
            };
          };
          continue;
        };

        if (params.seance==seanceAirport)
        {
          //цикл по портам
          if (params.airps.elems_permit())
          {
            for(set<string>::const_iterator i=params.airps.elems().begin();
                                            i!=params.airps.elems().end(); i++)
            {
              Qry.SetVariable("ap",*i);
              GetFullStat(params, Qry, FullStat, FullStatTotal, airline, full);
            };
          };
          continue;
        };
        GetFullStat(params, Qry, FullStat, FullStatTotal, airline, full);
    }
};

struct TFullStatCombo : public TOrderStatItem
{
    typedef std::pair<TFullStatKey, TFullStatRow> Tdata;
    Tdata data;
    TStatParams params;
    TFullStatCombo(const Tdata &aData, const TStatParams &aParams)
        : data(aData), params(aParams) {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void TFullStatCombo::add_header(ostringstream &buf) const
{
    if (params.airp_column_first)
    {
        buf << "Код а/п" << delim;
        buf << "Код а/к" << delim;
    } else {
        buf << "Код а/к" << delim;
        buf << "Код а/п" << delim;
    }
    buf << "Номер рейса" << delim;
    buf << "Дата" << delim;
    buf << "Направление" << delim;
    buf << "Кол-во пасс." << delim;
    if (params.statType==statFull)
    {
        /* TInetStat::toXML begin */
        buf << "Web" << delim;
        buf << "БГ" << delim;
        buf << "ПТ" << delim;
        buf << "Киоски" << delim;
        buf << "БГ" << delim;
        buf << "ПТ" << delim;
        buf << "Моб." << delim;
        buf << "БГ" << delim;
        buf << "ПТ" << delim;
        /* end */
    }
    buf << "ВЗ" << delim;
    buf << "РБ" << delim;
    buf << "РМ" << delim;
    buf << "Р/кладь (вес)" << delim;
    buf << "БГ мест" << delim;
    buf << "БГ вес" << delim;
    buf << "Пл.м" << delim;
    buf << "Пл.вес" << endl;
}

void TFullStatCombo::add_data(ostringstream &buf) const
{
    string region;
    try { region = AirpTZRegion(data.first.airp); }
    catch(AstraLocale::UserException &E) { return; };
    buf << data.first.col1 << delim; // col1
    buf << data.first.col2 << delim; // col2
    buf << data.first.flt_no << delim; // Номер рейса
    buf << DateTimeToStr(UTCToClient(data.first.scd_out, region), "dd.mm.yy") << delim; // Дата
    buf << data.first.places.get() << delim; // Направление
    buf << data.second.pax_amount << delim; // Кол-во пасс.
    if (params.statType==statFull)
    {
        buf << data.second.i_stat.web << delim; // Web
        buf << data.second.i_stat.web_bag << delim; // БГ
        buf << data.second.i_stat.web_bp << delim; // ПТ
        buf << data.second.i_stat.kiosk << delim; // Киоски
        buf << data.second.i_stat.kiosk_bag << delim; // БГ
        buf << data.second.i_stat.kiosk_bp << delim; // ПТ
        buf << data.second.i_stat.mobile << delim; // Моб.
        buf << data.second.i_stat.mobile_bag << delim; // БГ
        buf << data.second.i_stat.mobile_bp << delim; // ПТ
    }
    buf << data.second.adult << delim; // ВЗ
    buf << data.second.child << delim; // РБ
    buf << data.second.baby << delim; // РМ
    buf << data.second.rk_weight << delim; // Р/кладь (вес)
    buf << data.second.bag_amount << delim; // БГ мест
    buf << data.second.bag_weight << delim; // БГ вес
    buf << data.second.paid.excess_pcs << delim; // Пл.м
    buf << data.second.paid.excess << endl; // Пл.вес
}

template <class T>
void RunFullStatFile(const TStatParams &params, T &writer, TPrintAirline &prn_airline)
{
    TFullStat FullStat;
    TFullStatRow FullStatTotal;
    RunFullStat(params, FullStat, FullStatTotal, prn_airline, true);
    for (TFullStat::const_iterator i = FullStat.begin(); i != FullStat.end(); ++i)
        writer.insert(TFullStatCombo(*i, params));
}

/**************************************************/

struct TSelfCkinStatRow {
    int pax_amount;
    int term_bp;
    int term_bag;
    int term_ckin_service;
    int adult;
    int child;
    int baby;
    int tckin;
    set<int> flts;
    TSelfCkinStatRow():
        pax_amount(0),
        term_bp(0),
        term_bag(0),
        term_ckin_service(0),
        adult(0),
        child(0),
        baby(0),
        tckin(0)
    {};
    bool operator == (const TSelfCkinStatRow &item) const
    {
        return pax_amount == item.pax_amount &&
               term_bp == item.term_bp &&
               term_bag == item.term_bag &&
               term_ckin_service == item.term_ckin_service &&
               adult == item.adult &&
               child == item.child &&
               baby == item.baby &&
               tckin == item.tckin &&
               flts.size() == item.flts.size();
    };
    void operator += (const TSelfCkinStatRow &item)
    {
        pax_amount += item.pax_amount;
        term_bp += item.term_bp;
        term_bag += item.term_bag;
        term_ckin_service += item.term_ckin_service;
        adult += item.adult;
        child += item.child;
        baby += item.baby;
        tckin += item.tckin;
        flts.insert(item.flts.begin(),item.flts.end());
    };
};

struct TSelfCkinStatKey {
    string client_type, descr, ak, ap;
    string desk, desk_airp;
    int flt_no;
    int point_id;
    TStatPlaces places;
    TDateTime scd_out;
    TSelfCkinStatKey(): flt_no(NoExists) {};
};

struct TKioskCmp {
    bool operator() (const TSelfCkinStatKey &lr, const TSelfCkinStatKey &rr) const
    {
        if(lr.client_type == rr.client_type)
            if(lr.desk == rr.desk)
                if(lr.desk_airp == rr.desk_airp)
                    if(lr.ak == rr.ak)
                        if(lr.ap == rr.ap)
                            if(lr.flt_no == rr.flt_no)
                                if(lr.scd_out == rr.scd_out)
                                    if(lr.point_id == rr.point_id)
                                        return lr.places.get() < rr.places.get();
                                    else
                                        return lr.point_id < rr.point_id;
                                else
                                    return lr.scd_out < rr.scd_out;
                            else
                                return lr.flt_no < rr.flt_no;
                        else
                            return lr.ap < rr.ap;
                    else
                        return lr.ak < rr.ak;
                else
                    return lr.desk_airp < rr.desk_airp;
            else
                return lr.desk < rr.desk;
        else
            return lr.client_type < rr.client_type;
    }
};

typedef map<TSelfCkinStatKey, TSelfCkinStatRow, TKioskCmp> TSelfCkinStat;

void RunSelfCkinStat(const TStatParams &params,
                  TSelfCkinStat &SelfCkinStat, TSelfCkinStatRow &SelfCkinStatTotal,
                  TPrintAirline &prn_airline, bool full = false)
{
    TQuery Qry(&OraSession);
    for(int pass = 0; pass <= 2; pass++) {
        string SQLText =
            "select "
            "    points.airline, "
            "    points.airp, "
            "    points.scd_out, "
            "    points.flt_no, "
            "    self_ckin_stat.point_id, "
            "    self_ckin_stat.client_type, "
            "    desk, "
            "    desk_airp, "
            "    descr, "
            "    adult, "
            "    child, "
            "    baby, "
            "    term_bp, "
            "    term_bag, "
            "    term_ckin_service, "
            "    tckin "
            "from ";
        if(pass != 0) {
            SQLText +=
                " arx_points points, "
                " arx_self_ckin_stat self_ckin_stat ";
            if(pass == 2)
                SQLText +=
                    ",(SELECT part_key, move_id FROM move_arx_ext \n"
                    "  WHERE part_key >= :LastDate+:arx_trip_date_range AND part_key <= :LastDate+date_range) arx_ext \n";
        } else {
            SQLText +=
                " points, "
                " self_ckin_stat ";
        }
        SQLText += "where ";
        if (pass==1)
            SQLText += " points.part_key >= :FirstDate AND points.part_key < :LastDate + :arx_trip_date_range AND \n";
        if (pass==2)
            SQLText += " points.part_key=arx_ext.part_key AND points.move_id=arx_ext.move_id AND \n";
        if (pass!=0)
            SQLText += " self_ckin_stat.part_key = points.part_key AND \n";
        SQLText +=
            "    self_ckin_stat.point_id = points.point_id and "
            "    points.pr_del >= 0 and "
            "    points.scd_out >= :FirstDate and "
            "    points.scd_out < :LastDate ";
        if(TReqInfo::Instance()->client_type==ctHTTP ||
           TReqInfo::Instance()->desk.compatible(SELF_CKIN_STAT_VERSION)) {
            if(not params.reg_type.empty()) {
                SQLText += " and self_ckin_stat.client_type = :reg_type ";
                Qry.CreateVariable("reg_type", otString, params.reg_type);
            }
        } else {
            SQLText += " and self_ckin_stat.client_type = :reg_type ";
            Qry.CreateVariable("reg_type", otString, EncodeClientType(ctKiosk));
        }
        if(params.flt_no != NoExists) {
            SQLText += " and points.flt_no = :flt_no ";
            Qry.CreateVariable("flt_no", otInteger, params.flt_no);
        }
        if (!params.airps.elems().empty()) {
            if (params.airps.elems_permit())
                SQLText += " AND points.airp IN " + GetSQLEnum(params.airps.elems()) + "\n";
            else
                SQLText += " AND points.airp NOT IN " + GetSQLEnum(params.airps.elems()) + "\n";
        };
        if (!params.airlines.elems().empty()) {
            if (params.airlines.elems_permit())
                SQLText += " AND points.airline IN " + GetSQLEnum(params.airlines.elems()) + "\n";
            else
                SQLText += " AND points.airline NOT IN " + GetSQLEnum(params.airlines.elems()) + "\n";
        };
        if(!params.desk.empty()) {
            SQLText += "and self_ckin_stat.desk = :desk ";
            Qry.CreateVariable("desk", otString, params.desk);
        }
        Qry.SQLText = SQLText;
        Qry.CreateVariable("FirstDate", otDate, params.FirstDate);
        Qry.CreateVariable("LastDate", otDate, params.LastDate);
        if (pass!=0)
            Qry.CreateVariable("arx_trip_date_range", otInteger, ARX_TRIP_DATE_RANGE());
        Qry.Execute();
        if(not Qry.Eof) {
            int col_airline = Qry.FieldIndex("airline");
            int col_airp = Qry.FieldIndex("airp");
            int col_scd_out = Qry.FieldIndex("scd_out");
            int col_flt_no = Qry.FieldIndex("flt_no");
            int col_point_id = Qry.FieldIndex("point_id");
            int col_client_type = Qry.FieldIndex("client_type");
            int col_desk = Qry.FieldIndex("desk");
            int col_desk_airp = Qry.FieldIndex("desk_airp");
            int col_descr = Qry.FieldIndex("descr");
            int col_adult = Qry.FieldIndex("adult");
            int col_child = Qry.FieldIndex("child");
            int col_term_bp = Qry.FieldIndex("term_bp");
            int col_term_bag = Qry.FieldIndex("term_bag");
            int col_term_ckin_service = Qry.FieldIndex("term_ckin_service");
            int col_baby = Qry.FieldIndex("baby");
            int col_tckin = Qry.FieldIndex("tckin");
            for(; not Qry.Eof; Qry.Next())
            {
              string airline = Qry.FieldAsString(col_airline);
              prn_airline.check(airline);

              TSelfCkinStatRow row;
              row.adult = Qry.FieldAsInteger(col_adult);
              row.child = Qry.FieldAsInteger(col_child);
              row.baby = Qry.FieldAsInteger(col_baby);
              row.tckin = Qry.FieldAsInteger(col_tckin);
              row.pax_amount = row.adult + row.child + row.baby;
              row.term_bp = Qry.FieldAsInteger(col_term_bp);
              row.term_bag = Qry.FieldAsInteger(col_term_bag);
              if (Qry.FieldIsNULL(col_term_ckin_service))
                row.term_ckin_service = row.pax_amount;
              else
                row.term_ckin_service = Qry.FieldAsInteger(col_term_ckin_service);
              int point_id=Qry.FieldAsInteger(col_point_id);
              row.flts.insert(point_id);
              if (!params.skip_rows)
              {
                string airp = Qry.FieldAsString(col_airp);
                TDateTime scd_out = Qry.FieldAsDateTime(col_scd_out);
                int flt_no = Qry.FieldAsInteger(col_flt_no);
                TSelfCkinStatKey key;
                key.client_type = Qry.FieldAsString(col_client_type);
                key.desk = Qry.FieldAsString(col_desk);
                key.desk_airp = ElemIdToCodeNative(etAirp, Qry.FieldAsString(col_desk_airp));
                key.descr = Qry.FieldAsString(col_descr);

                key.ak = ElemIdToCodeNative(etAirline, airline);
                if(
                        params.statType == statSelfCkinDetail or
                        params.statType == statSelfCkinFull
                  )
                    key.ap = airp;
                if(params.statType == statSelfCkinFull) {
                    key.flt_no = flt_no;
                    key.scd_out = scd_out;
                    key.point_id = point_id;
                    key.places.set(GetRouteAfterStr( NoExists, point_id, trtNotCurrent, trtNotCancelled), false);
                }

                AddStatRow(key, row, SelfCkinStat, full);
              }
              else
              {
                SelfCkinStatTotal+=row;
              };
            }
        }
    }
}

void createXMLSelfCkinStat(const TStatParams &params,
                        const TSelfCkinStat &SelfCkinStat, const TSelfCkinStatRow &SelfCkinStatTotal,
                        const TPrintAirline &airline, xmlNodePtr resNode)
{
    if(SelfCkinStat.empty() && SelfCkinStatTotal==TSelfCkinStatRow())
        throw AstraLocale::UserException("MSG.NOT_DATA");

    NewTextChild(resNode, "airline", airline.get(), "");
    xmlNodePtr grdNode = NewTextChild(resNode, "grd");
    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    TSelfCkinStatRow total;
    bool showTotal=true;
    int flts_total = 0;
    int rows = 0;
    for(TSelfCkinStat::const_iterator im = SelfCkinStat.begin(); im != SelfCkinStat.end(); ++im, rows++)
    {
        if(rows >= MAX_STAT_ROWS()) {
            throw MaxStatRowsException("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH", LParams() << LParam("num", MAX_STAT_ROWS()));
            /*AstraLocale::showErrorMessage("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH",
                    LParams() << LParam("num", MAX_STAT_ROWS()));
            if (WITHOUT_TOTAL_WHEN_PROBLEM) showTotal=false; //не будем показывать итоговую строку дабы не ввести в заблуждение
            break;*/
        }
        //region обязательно в начале цикла, иначе будет испорчен xml
        string region;
        if(params.statType == statSelfCkinFull)
        {
            try
            {
                region = AirpTZRegion(im->first.ap);
            }
            catch(AstraLocale::UserException &E)
            {
                AstraLocale::showErrorMessage("MSG.ERR_MSG.NOT_ALL_FLIGHTS_ARE_SHOWN", LParams() << LParam("msg", getLocaleText(E.getLexemaData())));
                if (WITHOUT_TOTAL_WHEN_PROBLEM) showTotal=false; //не будем показывать итоговую строку дабы не ввести в заблуждение
                continue;
            };
        };

        rowNode = NewTextChild(rowsNode, "row");
        // Тип рег.
        NewTextChild(rowNode, "col", im->first.client_type);
        // Пульт
        NewTextChild(rowNode, "col", im->first.desk);
        // А/П пульта
        NewTextChild(rowNode, "col", im->first.desk_airp);
        // примечание
        if(params.statType == statSelfCkinFull)
            NewTextChild(rowNode, "col", im->first.descr);
        // код а/к
        NewTextChild(rowNode, "col", im->first.ak);
        if(
                params.statType == statSelfCkinDetail or
                params.statType == statSelfCkinFull
          )
            // код а/п
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, im->first.ap));
        if(
                params.statType == statSelfCkinShort or
                params.statType == statSelfCkinDetail
          ) {
            // Кол-во рейсов
            NewTextChild(rowNode, "col", (int)im->second.flts.size());
            flts_total += im->second.flts.size();
        }
        if(params.statType == statSelfCkinFull) {
            // номер рейса
            ostringstream buf;
            buf << setw(3) << setfill('0') << im->first.flt_no;
            NewTextChild(rowNode, "col", buf.str().c_str());
            // Дата
            NewTextChild(rowNode, "col", DateTimeToStr(
                        UTCToClient(im->first.scd_out, region), "dd.mm.yy")
                    );
            // Направление
            NewTextChild(rowNode, "col", im->first.places.get());
        }
        // Кол-во пасс.
        NewTextChild(rowNode, "col", im->second.pax_amount);
        NewTextChild(rowNode, "col", im->second.term_bag);
        NewTextChild(rowNode, "col", im->second.term_bp);
        NewTextChild(rowNode, "col", im->second.pax_amount - im->second.term_ckin_service);

        if(params.statType == statSelfCkinFull) {
            // ВЗ
            NewTextChild(rowNode, "col", im->second.adult);
            // РБ
            NewTextChild(rowNode, "col", im->second.child);
            // РМ
            NewTextChild(rowNode, "col", im->second.baby);
        }
        if(
                params.statType == statSelfCkinDetail or
                params.statType == statSelfCkinFull
          )
            // Сквоз. рег.
            NewTextChild(rowNode, "col", im->second.tckin);
        if(
                params.statType == statSelfCkinShort or
                params.statType == statSelfCkinDetail
          )
            // Примечание
            NewTextChild(rowNode, "col", im->first.descr);

        total += im->second;
    };

    rowNode = NewTextChild(rowsNode, "row");

    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("Тип рег."));
    SetProp(colNode, "width", 75);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col", getLocaleText("Итого:"));
    colNode = NewTextChild(headerNode, "col", getLocaleText("Пульт"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col");
    colNode = NewTextChild(headerNode, "col", getLocaleText("АП пульта"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col");
    if(params.statType == statSelfCkinFull) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("Примечание"));
        SetProp(colNode, "width", 280);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col");
    }
    colNode = NewTextChild(headerNode, "col", getLocaleText("Код а/к"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col");
    if(
            params.statType == statSelfCkinDetail or
            params.statType == statSelfCkinFull
      ) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("Код а/п"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col");
    }
    if(
            params.statType == statSelfCkinShort or
            params.statType == statSelfCkinDetail
      ) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("Кол-во рейсов"));
        SetProp(colNode, "width", 85);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", flts_total);
    }
    if(params.statType == statSelfCkinFull) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("Номер рейса"));
        SetProp(colNode, "width", 75);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col");
        colNode = NewTextChild(headerNode, "col", getLocaleText("Дата"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortDate);
        NewTextChild(rowNode, "col");
        colNode = NewTextChild(headerNode, "col", getLocaleText("Направление"));
        SetProp(colNode, "width", 90);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col");
    }


    // Киоски
    colNode = NewTextChild(headerNode, "col", getLocaleText("Пас."));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.pax_amount);

    colNode = NewTextChild(headerNode, "col", getLocaleText("БГ"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.term_bag);

    colNode = NewTextChild(headerNode, "col", getLocaleText("ПТ"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.term_bp);

    colNode = NewTextChild(headerNode, "col", getLocaleText("Всё сами"));
    SetProp(colNode, "width", 45);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.pax_amount-total.term_ckin_service);

    if(params.statType == statSelfCkinFull) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("ВЗ"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.adult);
        colNode = NewTextChild(headerNode, "col", getLocaleText("РБ"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.child);
        colNode = NewTextChild(headerNode, "col", getLocaleText("РМ"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.baby);
    }
    if(
            params.statType == statSelfCkinDetail or
            params.statType == statSelfCkinFull
      ) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("Сквоз."));
        SetProp(colNode, "width", 45);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.tckin);
    }
    if(
            params.statType == statSelfCkinShort or
            params.statType == statSelfCkinDetail
      ) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("Примечание"));
        SetProp(colNode, "width", 280);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col");
    }

    if (!showTotal)
    {
        xmlUnlinkNode(rowNode);
        xmlFreeNode(rowNode);
    };

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText(
                ((TReqInfo::Instance()->client_type==ctHTTP ||
                  TReqInfo::Instance()->desk.compatible(SELF_CKIN_STAT_VERSION)) ?
                 "Саморегистрация" :
                 "Киоски саморегистрации"
                )
                ));
    string buf;
    switch(params.statType) {
        case statSelfCkinShort:
            buf = getLocaleText("Общая");
            break;
        case statSelfCkinDetail:
            buf = getLocaleText("Детализированная");
            break;
        case statSelfCkinFull:
            buf = getLocaleText("Подробная");
            break;
        default:
            throw Exception("createXMLSelfCkinStat: unexpected statType %d", params.statType);
            break;
    }
    NewTextChild(variablesNode, "stat_type_caption", buf);
}

struct TSelfCkinStatCombo : public TOrderStatItem
{
    std::pair<TSelfCkinStatKey, TSelfCkinStatRow> data;
    TStatParams params;
    TSelfCkinStatCombo(const std::pair<TSelfCkinStatKey, TSelfCkinStatRow> &aData,
        const TStatParams &aParams): data(aData), params(aParams) {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void TSelfCkinStatCombo::add_header(ostringstream &buf) const
{
    buf << "Тип рег." << delim;
    buf << "Пульт" << delim;
    buf << "АП пульта" << delim;
    if (params.statType == statSelfCkinFull)
        buf << "Примечание" << delim;
    buf << "Код а/к" << delim;
    if (params.statType == statSelfCkinDetail or params.statType == statSelfCkinFull)
        buf << "Код а/п" << delim;
    if (params.statType == statSelfCkinShort or params.statType == statSelfCkinDetail)
        buf << "Кол-во рейсов" << delim;
    if (params.statType == statSelfCkinFull)
    {
        buf << "Номер рейса" << delim;
        buf << "Дата" << delim;
        buf << "Направление" << delim;
    }
    buf << "Пас." << delim;
    buf << "БГ" << delim;
    buf << "ПТ" << delim;
    buf << "Всё сами" << delim;
    if (params.statType == statSelfCkinFull)
    {
        buf << "ВЗ" << delim;
        buf << "РБ" << delim;
        buf << "РМ" << delim;
    }
    if (params.statType == statSelfCkinDetail or params.statType == statSelfCkinFull)
        buf << "Сквоз." << delim;
    if (params.statType == statSelfCkinShort or params.statType == statSelfCkinDetail)
        buf << "Примечание" << delim;
    buf << endl;
}

void TSelfCkinStatCombo::add_data(ostringstream &buf) const
{
    string region;
    if (params.statType == statSelfCkinFull)
    {
        try { region = AirpTZRegion(data.first.ap); }
        catch(AstraLocale::UserException &E) { return; };
    }
    buf << data.first.client_type << delim; // Тип рег.
    buf << data.first.desk << delim; // Пульт
    buf << data.first.desk_airp << delim; // А/П пульта
    if (params.statType == statSelfCkinFull)
        buf << data.first.descr << delim; // примечание
    buf << data.first.ak << delim; // код а/к
    if (params.statType == statSelfCkinDetail or params.statType == statSelfCkinFull)
        buf << ElemIdToCodeNative(etAirp, data.first.ap) << delim; // код а/п
    if (params.statType == statSelfCkinShort or params.statType == statSelfCkinDetail)
        buf << (int)data.second.flts.size() << delim; // Кол-во рейсов
    if (params.statType == statSelfCkinFull)
    {
        ostringstream oss1;
        oss1 << setw(3) << setfill('0') << data.first.flt_no;
        buf << oss1.str() << delim; // номер рейса
        buf << DateTimeToStr(UTCToClient(data.first.scd_out, region), "dd.mm.yy")
         << delim; // Дата
        buf << data.first.places.get() << delim; // Направление
    }
    buf << data.second.pax_amount << delim; // Кол-во пасс.
    buf << data.second.term_bag << delim; // БГ
    buf << data.second.term_bp << delim; // ПТ
    buf << (data.second.pax_amount - data.second.term_ckin_service)
     << delim; // Всё сами
    if (params.statType == statSelfCkinFull)
    {
        buf << data.second.adult << delim; // ВЗ
        buf << data.second.child << delim; // РБ
        buf << data.second.baby << delim; // РМ
    }
    if (params.statType == statSelfCkinDetail or params.statType == statSelfCkinFull)
        buf << data.second.tckin << delim; // Сквоз. рег.
    if (params.statType == statSelfCkinShort or params.statType == statSelfCkinDetail)
        buf << data.first.descr << delim; // примечание
    buf << endl;
}

template <class T>
void RunSelfCkinStatFile(const TStatParams &params, T &writer, TPrintAirline &prn_airline)
{
    TSelfCkinStat SelfCkinStat;
    TSelfCkinStatRow SelfCkinStatTotal;
    RunSelfCkinStat(params, SelfCkinStat, SelfCkinStatTotal, prn_airline, true);
    for (TSelfCkinStat::const_iterator i = SelfCkinStat.begin(); i != SelfCkinStat.end(); ++i)
        writer.insert(TSelfCkinStatCombo(*i, params));
}

/****************** TlgOutStat *************************/
struct TTlgOutStatKey {
    string sender_sita_addr;
    string receiver_descr;
    string receiver_sita_addr;
    string receiver_country_view;
    TDateTime time_send;
    string airline_view;
    int flt_no;
    string suffix_view;
    string airp_dep_view;
    TDateTime scd_local_date;
    string tlg_type;
    string airline_mark_view;
    string extra;
    TTlgOutStatKey():time_send(NoExists),
                     flt_no(NoExists),
                     scd_local_date(NoExists) {};
};

struct TTlgOutStatRow {
    int tlg_count;
    double tlg_len;
    TTlgOutStatRow():tlg_count(0),
                     tlg_len(0.0) {};
    bool operator == (const TTlgOutStatRow &item) const
    {
        return tlg_count == item.tlg_count &&
               tlg_len == item.tlg_len;
               // FIXME проверка на равенство double tlg_len (c++ double equality comparison)
               // http://en.cppreference.com/w/cpp/types/numeric_limits/epsilon
    };
    void operator += (const TTlgOutStatRow &item)
    {
        tlg_count += item.tlg_count;
        tlg_len += item.tlg_len;
    };
};

struct TTlgOutStatCmp {
    bool operator() (const TTlgOutStatKey &key1, const TTlgOutStatKey &key2) const
    {
      if (key1.sender_sita_addr!=key2.sender_sita_addr)
        return key1.sender_sita_addr < key2.sender_sita_addr;
      if (key1.receiver_descr!=key2.receiver_descr)
        return key1.receiver_descr < key2.receiver_descr;
      if (key1.receiver_sita_addr!=key2.receiver_sita_addr)
        return key1.receiver_sita_addr < key2.receiver_sita_addr;
      if (key1.receiver_country_view!=key2.receiver_country_view)
        return key1.receiver_country_view < key2.receiver_country_view;
      if (key1.time_send!=key2.time_send)
        return key1.time_send < key2.time_send;
      if (key1.airline_view!=key2.airline_view)
        return key1.airline_view < key2.airline_view;
      if (key1.flt_no!=key2.flt_no)
        return key1.flt_no < key2.flt_no;
      if (key1.suffix_view!=key2.suffix_view)
        return key1.suffix_view < key2.suffix_view;
      if (key1.airp_dep_view!=key2.airp_dep_view)
        return key1.airp_dep_view < key2.airp_dep_view;
      if (key1.scd_local_date!=key2.scd_local_date)
        return key1.scd_local_date < key2.scd_local_date;
      if (key1.tlg_type!=key2.tlg_type)
        return key1.tlg_type < key2.tlg_type;
      if(key1.airline_mark_view != key2.airline_mark_view)
          return key1.airline_mark_view < key2.airline_mark_view;
      return key1.extra < key2.extra;
    }
};

typedef map<TTlgOutStatKey, TTlgOutStatRow, TTlgOutStatCmp> TTlgOutStat;

void RunTlgOutStat(const TStatParams &params,
                   TTlgOutStat &TlgOutStat, TTlgOutStatRow &TlgOutStatTotal,
                   TPrintAirline &prn_airline, bool full = false)
{
    TQuery Qry(&OraSession);
    for(int pass = 0; pass <= 1; pass++) {
        string SQLText =
            "SELECT \n"
            "  tlg_stat.sender_sita_addr, \n"
            "  tlg_stat.receiver_descr, \n"
            "  tlg_stat.receiver_sita_addr, \n"
            "  tlg_stat.receiver_country, \n"
            "  tlg_stat.time_send, \n"
            "  tlg_stat.airline, \n"
            "  tlg_stat.flt_no, \n"
            "  tlg_stat.suffix, \n"
            "  tlg_stat.airp_dep, \n"
            "  tlg_stat.scd_local_date, \n"
            "  tlg_stat.tlg_type, \n"
            "  tlg_stat.airline_mark, \n"
            "  tlg_stat.extra, \n"
            "  tlg_stat.tlg_len \n"
            "FROM \n";
        if(pass != 0) {
            SQLText +=
                "   arx_tlg_stat tlg_stat \n";
        } else {
            SQLText +=
                "   tlg_stat \n";
        }
        //SQLText += "WHERE rownum < 1000\n"; //SQLText += "WHERE \n";
        SQLText += "WHERE sender_canon_name=:own_canon_name AND \n";
        if (pass!=0)
          SQLText +=
            "    tlg_stat.part_key >= :FirstDate AND tlg_stat.part_key < :LastDate \n";
        else
          SQLText +=
            "    tlg_stat.time_send >= :FirstDate AND tlg_stat.time_send < :LastDate \n";
        if (!params.airps.elems().empty()) {
            if (params.airps.elems_permit())
                SQLText += " AND tlg_stat.airp_dep IN " + GetSQLEnum(params.airps.elems()) + "\n";
            else
                SQLText += " AND tlg_stat.airp_dep NOT IN " + GetSQLEnum(params.airps.elems()) + "\n";
        };
        if (!params.airlines.elems().empty()) {
            if (params.airlines.elems_permit())
                SQLText += " AND tlg_stat.airline IN " + GetSQLEnum(params.airlines.elems()) + "\n";
            else
                SQLText += " AND tlg_stat.airline NOT IN " + GetSQLEnum(params.airlines.elems()) + "\n";
        };
        if(!params.typeb_type.empty()) {
            SQLText += " AND tlg_stat.tlg_type = :tlg_type \n";
            Qry.CreateVariable("tlg_type", otString, params.typeb_type);
        }
        if(!params.sender_addr.empty()) {
            SQLText += " AND tlg_stat.sender_sita_addr = :sender_sita_addr \n";
            Qry.CreateVariable("sender_sita_addr", otString, params.sender_addr);
        }
        if(!params.receiver_descr.empty()) {
            SQLText += " AND tlg_stat.receiver_descr = :receiver_descr \n";
            Qry.CreateVariable("receiver_descr", otString, params.receiver_descr);
        }

        //ProgTrace(TRACE5, "RunTlgOutStat: pass=%d SQL=\n%s", pass, SQLText.c_str());
        Qry.SQLText = SQLText;
        Qry.CreateVariable("own_canon_name", otString, OWN_CANON_NAME());
        Qry.CreateVariable("FirstDate", otDate, params.FirstDate);
        Qry.CreateVariable("LastDate", otDate, params.LastDate);
        Qry.Execute();
        if(not Qry.Eof) {
            int col_sender_sita_addr = Qry.FieldIndex("sender_sita_addr");
            int col_receiver_descr = Qry.FieldIndex("receiver_descr");
            int col_receiver_sita_addr = Qry.FieldIndex("receiver_sita_addr");
            int col_receiver_country = Qry.FieldIndex("receiver_country");
            int col_time_send = Qry.FieldIndex("time_send");
            int col_airline = Qry.FieldIndex("airline");
            int col_flt_no = Qry.FieldIndex("flt_no");
            int col_suffix = Qry.FieldIndex("suffix");
            int col_airp_dep = Qry.FieldIndex("airp_dep");
            int col_scd_local_date = Qry.FieldIndex("scd_local_date");
            int col_tlg_type = Qry.FieldIndex("tlg_type");
            int col_airline_mark = Qry.FieldIndex("airline_mark");
            int col_extra = Qry.FieldIndex("extra");
            int col_tlg_len = Qry.FieldIndex("tlg_len");
            for(; not Qry.Eof; Qry.Next()) {
              string airline = Qry.FieldAsString(col_airline);
              prn_airline.check(airline);

              TTlgOutStatRow row;
              row.tlg_count = 1;
              row.tlg_len = Qry.FieldAsInteger(col_tlg_len);
              if (!params.skip_rows)
              {
                TTlgOutStatKey key;
                key.sender_sita_addr = Qry.FieldAsString(col_sender_sita_addr);
                key.receiver_descr = Qry.FieldAsString(col_receiver_descr);
                if (params.statType == statTlgOutShort)
                {
                  //для общей статистики выводим помимо кода гос-ва еще и полное название
                  key.receiver_country_view = ElemIdToNameLong(etCountry, Qry.FieldAsString(col_receiver_country));
                  if (!key.receiver_country_view.empty()) key.receiver_country_view += " / ";
                };
                key.receiver_country_view += ElemIdToCodeNative(etCountry, Qry.FieldAsString(col_receiver_country));
                key.extra = Qry.FieldAsString(col_extra);
                if (params.statType == statTlgOutDetail ||
                    params.statType == statTlgOutFull)
                {
                  key.airline_view = ElemIdToCodeNative(etAirline, airline);
                  key.airline_mark_view = ElemIdToCodeNative(etAirline, Qry.FieldAsString(col_airline_mark));
                  key.airp_dep_view = ElemIdToCodeNative(etAirp, Qry.FieldAsString(col_airp_dep));

                  if (params.statType == statTlgOutFull)
                  {
                    key.receiver_sita_addr = Qry.FieldAsString(col_receiver_sita_addr);
                    if (!Qry.FieldIsNULL(col_time_send))
                    {
                      key.time_send = Qry.FieldAsDateTime(col_time_send);
                      modf(key.time_send, &key.time_send);
                    };
                    if (!Qry.FieldIsNULL(col_flt_no))
                      key.flt_no = Qry.FieldAsInteger(col_flt_no);
                    key.suffix_view = ElemIdToCodeNative(etSuffix, Qry.FieldAsString(col_suffix));
                    if (!Qry.FieldIsNULL(col_scd_local_date))
                    {
                      key.scd_local_date = Qry.FieldAsDateTime(col_scd_local_date);
                      modf(key.scd_local_date, &key.scd_local_date);
                    };
                    key.tlg_type=Qry.FieldAsString(col_tlg_type);
                  };
                };
                AddStatRow(key, row, TlgOutStat, full);
              }
              else
              {
                TlgOutStatTotal+=row;
              };
            }
        }
    }
    return;
}

void createXMLTlgOutStat(const TStatParams &params,
                         const TTlgOutStat &TlgOutStat, const TTlgOutStatRow &TlgOutStatTotal,
                         const TPrintAirline &airline, xmlNodePtr resNode)
{
    if(TlgOutStat.empty() && TlgOutStatTotal==TTlgOutStatRow())
      throw AstraLocale::UserException("MSG.NOT_DATA");

    NewTextChild(resNode, "airline", airline.get(), "");
    xmlNodePtr grdNode = NewTextChild(resNode, "grd");
    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    TTlgOutStatRow total;
    ostringstream buf;
    bool showTotal=true;
    if (!params.skip_rows)
    {
      int rows = 0;
      for(TTlgOutStat::const_iterator im = TlgOutStat.begin(); im != TlgOutStat.end(); ++im, rows++)
      {
          if(rows >= MAX_STAT_ROWS()) {
              throw MaxStatRowsException("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH", LParams() << LParam("num", MAX_STAT_ROWS()));
              /*AstraLocale::showErrorMessage("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH",
                                            LParams() << LParam("num", MAX_STAT_ROWS()));
              if (WITHOUT_TOTAL_WHEN_PROBLEM) showTotal=false; //не будем показывать итоговую строку дабы не ввести в заблуждение
              break;*/
          }

          rowNode = NewTextChild(rowsNode, "row");
          NewTextChild(rowNode, "col", im->first.sender_sita_addr);
          NewTextChild(rowNode, "col", im->first.receiver_descr);
          if (params.statType == statTlgOutFull)
            NewTextChild(rowNode, "col", im->first.receiver_sita_addr);
          NewTextChild(rowNode, "col", im->first.receiver_country_view);
          if (params.statType == statTlgOutFull)
          {
            if (im->first.time_send!=NoExists)
              NewTextChild(rowNode, "col", DateTimeToStr(im->first.time_send, "dd.mm.yy"));
            else
              NewTextChild(rowNode, "col");
          };
          if (params.statType == statTlgOutDetail ||
              params.statType == statTlgOutFull)
          {
            NewTextChild(rowNode, "col", im->first.airline_view);
            NewTextChild(rowNode, "col", im->first.airline_mark_view);
            NewTextChild(rowNode, "col", im->first.airp_dep_view);
          };
          if (params.statType == statTlgOutFull)
          {
            NewTextChild(rowNode, "col", im->first.tlg_type);
            if (im->first.scd_local_date!=NoExists)
              NewTextChild(rowNode, "col", DateTimeToStr(im->first.scd_local_date, "dd.mm.yy"));
            else
              NewTextChild(rowNode, "col");
            if (im->first.flt_no!=NoExists)
            {
              buf.str("");
              buf << setw(3) << setfill('0') << im->first.flt_no
                  << im->first.suffix_view;
              NewTextChild(rowNode, "col", buf.str());
            }
            else
              NewTextChild(rowNode, "col");
          };
          NewTextChild(rowNode, "col", im->second.tlg_count);
          buf.str("");
          buf << fixed << setprecision(0) << im->second.tlg_len;
          NewTextChild(rowNode, "col", buf.str());
          NewTextChild(rowNode, "col", im->first.extra);

          total += im->second;
      };
    }
    else total=TlgOutStatTotal;

    rowNode = NewTextChild(rowsNode, "row");

    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("Адрес отпр."));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col", getLocaleText("Итого:"));
    colNode = NewTextChild(headerNode, "col", getLocaleText("Канал"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col");
    if (params.statType == statTlgOutFull)
    {
      colNode = NewTextChild(headerNode, "col", getLocaleText("Адрес получ."));
      SetProp(colNode, "width", 70);
      SetProp(colNode, "align", TAlignment::LeftJustify);
      SetProp(colNode, "sort", sortString);
      NewTextChild(rowNode, "col");
    };
    colNode = NewTextChild(headerNode, "col", getLocaleText("Гос-во"));
    if (params.statType == statTlgOutShort)
      SetProp(colNode, "width", 200);
    else
      SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col");
    if (params.statType == statTlgOutFull)
    {
      colNode = NewTextChild(headerNode, "col", getLocaleText("Дата отпр."));
      SetProp(colNode, "width", 60);
      SetProp(colNode, "align", TAlignment::LeftJustify);
      SetProp(colNode, "sort", sortDate);
      NewTextChild(rowNode, "col");
    };
    if (params.statType == statTlgOutDetail ||
        params.statType == statTlgOutFull)
    {
      colNode = NewTextChild(headerNode, "col", getLocaleText("А/к факт."));
      SetProp(colNode, "width", 50);
      SetProp(colNode, "align", TAlignment::LeftJustify);
      SetProp(colNode, "sort", sortString);
      NewTextChild(rowNode, "col");
      colNode = NewTextChild(headerNode, "col", getLocaleText("А/к комм."));
      SetProp(colNode, "width", 50);
      SetProp(colNode, "align", TAlignment::LeftJustify);
      SetProp(colNode, "sort", sortString);
      NewTextChild(rowNode, "col");
      colNode = NewTextChild(headerNode, "col", getLocaleText("Код а/п"));
      SetProp(colNode, "width", 50);
      SetProp(colNode, "align", TAlignment::LeftJustify);
      SetProp(colNode, "sort", sortString);
      NewTextChild(rowNode, "col");
    };
    if (params.statType == statTlgOutFull)
    {
      colNode = NewTextChild(headerNode, "col", getLocaleText("Тип тлг."));
      SetProp(colNode, "width", 50);
      SetProp(colNode, "align", TAlignment::LeftJustify);
      SetProp(colNode, "sort", sortString);
      NewTextChild(rowNode, "col");
      colNode = NewTextChild(headerNode, "col", getLocaleText("Дата вылета"));
      SetProp(colNode, "width", 70);
      SetProp(colNode, "align", TAlignment::LeftJustify);
      SetProp(colNode, "sort", sortDate);
      NewTextChild(rowNode, "col");
      colNode = NewTextChild(headerNode, "col", getLocaleText("Рейс"));
      SetProp(colNode, "width", 50);
      SetProp(colNode, "align", TAlignment::LeftJustify);
      SetProp(colNode, "sort", sortString);
      NewTextChild(rowNode, "col");
    };
    colNode = NewTextChild(headerNode, "col", getLocaleText("Кол-во"));
    SetProp(colNode, "width", params.statType == statTlgOutFull?40:70);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.tlg_count);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Объем (байт)"));
    SetProp(colNode, "width", params.statType == statTlgOutFull?70:100);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortFloat);
    buf.str("");
    buf << fixed << setprecision(0) << total.tlg_len;
    NewTextChild(rowNode, "col", buf.str());
    colNode = NewTextChild(headerNode, "col", getLocaleText("№ договора"));
    SetProp(colNode, "width", 150);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col");

    if (!showTotal)
    {
      xmlUnlinkNode(rowNode);
      xmlFreeNode(rowNode);
    };

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("Статистика отправленных телеграмм"));
    string stat_type_caption;
    switch(params.statType) {
        case statTlgOutShort:
            stat_type_caption = getLocaleText("Общая");
            break;
        case statTlgOutDetail:
            stat_type_caption = getLocaleText("Детализированная");
            break;
        case statTlgOutFull:
            stat_type_caption = getLocaleText("Подробная");
            break;
        default:
            throw Exception("createXMLTlgOutStat: unexpected statType %d", params.statType);
            break;
    }
    NewTextChild(variablesNode, "stat_type_caption", stat_type_caption);
}

struct TTlgOutStatCombo : public TOrderStatItem
{
    std::pair<TTlgOutStatKey, TTlgOutStatRow> data;
    TStatParams params;
    TTlgOutStatCombo(const std::pair<TTlgOutStatKey, TTlgOutStatRow> &aData,
        const TStatParams &aParams): data(aData), params(aParams) {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void TTlgOutStatCombo::add_header(ostringstream &buf) const
{
    buf << "Адрес отпр." << delim;
    buf << "Канал" << delim;
    if (params.statType == statTlgOutFull)
        buf << "Адрес получ." << delim;
    buf << "Гос-во" << delim;
    if (params.statType == statTlgOutFull)
        buf << "Дата отпр." << delim;
    if (params.statType == statTlgOutDetail || params.statType == statTlgOutFull)
    {
        buf << "А/к факт." << delim;
        buf << "А/к комм." << delim;
        buf << "Код а/п" << delim;
    }
    if (params.statType == statTlgOutFull)
    {
        buf << "Тип тлг." << delim;
        buf << "Дата вылета" << delim;
        buf << "Рейс" << delim;
    }
    buf << "Кол-во" << delim;
    buf << "Объем (байт)" << delim;
    buf << "№ договора" << endl;
}

void TTlgOutStatCombo::add_data(ostringstream &buf) const
{
    buf << data.first.sender_sita_addr << delim; // Адрес отпр.
    buf << data.first.receiver_descr << delim; // Канал
    if (params.statType == statTlgOutFull)
        buf << data.first.receiver_sita_addr << delim; // Адрес получ.
    buf << data.first.receiver_country_view << delim; // Гос-во
    if (params.statType == statTlgOutFull)
    {
        if (data.first.time_send != NoExists)
            buf << DateTimeToStr(data.first.time_send, "dd.mm.yy"); // Дата отпр.
        buf << delim;
    }
    if (params.statType == statTlgOutDetail || params.statType == statTlgOutFull)
    {
        buf << data.first.airline_view << delim; // А/к факт.
        buf << data.first.airline_mark_view << delim; // А/к комм.
        buf << data.first.airp_dep_view << delim; // Код а/п
    }
    if (params.statType == statTlgOutFull)
    {
        buf << data.first.tlg_type << delim; // Тип тлг.
        if (data.first.scd_local_date != NoExists)
            buf << DateTimeToStr(data.first.scd_local_date, "dd.mm.yy"); // Дата вылета
        buf << delim;
        if (data.first.flt_no != NoExists)
        {
            ostringstream oss1;
            oss1 << setw(3) << setfill('0')
             << data.first.flt_no << data.first.suffix_view;
            buf << oss1.str(); // Рейс
        }
        buf << delim;
    }
    buf << data.second.tlg_count << delim; // Кол-во
    ostringstream oss2;
    oss2 << fixed << setprecision(0) << data.second.tlg_len;
    buf << oss2.str() << delim; // Объем (байт)
    buf << data.first.extra << endl; // № договора
}

template <class T>
void RunTlgOutStatFile(const TStatParams &params, T &writer, TPrintAirline &prn_airline)
{
    TTlgOutStat TlgOutStat;
    TTlgOutStatRow TlgOutStatTotal;
    RunTlgOutStat(params, TlgOutStat, TlgOutStatTotal, prn_airline, true);
    for (TTlgOutStat::const_iterator i = TlgOutStat.begin(); i != TlgOutStat.end(); ++i)
        writer.insert(TTlgOutStatCombo(*i, params));
}

/****************** end of TlgOutStat ******************/

/****************** AgentStat *************************/
struct TAgentStatKey {
    int point_id;
    string airline_view;
    int flt_no;
    string suffix_view;
    string airp, airp_view;
    TDateTime scd_out, scd_out_local;
    string desk;
    int user_id;
    string user_descr;
    TAgentStatKey(): point_id(NoExists),
                     flt_no(NoExists),
                     scd_out(NoExists),
                     scd_out_local(NoExists),
                     user_id(NoExists) {};
};

struct TAgentStatRow {
    STAT::agent_stat_t dpax_amount;
    STAT::agent_stat_t dtckin_amount;
    STAT::agent_stat_t dbag_amount;
    STAT::agent_stat_t dbag_weight;
    STAT::agent_stat_t drk_amount;
    STAT::agent_stat_t drk_weight;
    int processed_pax;
    double time;
    TAgentStatRow():
        dpax_amount(0,0),
        dtckin_amount(0,0),
        dbag_amount(0,0),
        dbag_weight(0,0),
        drk_amount(0,0),
        drk_weight(0,0),
        processed_pax(0),
        time(0.0)
    {}
    bool operator == (const TAgentStatRow &item) const
    {
        return dpax_amount == item.dpax_amount &&
               dtckin_amount == item.dtckin_amount &&
               dbag_amount == item.dbag_amount &&
               dbag_weight == item.dbag_weight &&
               drk_amount == item.drk_amount &&
               drk_weight == item.drk_weight &&
               processed_pax == item.processed_pax &&
               time == item.time;
    };
    void operator += (const TAgentStatRow &item)
    {
        dpax_amount += item.dpax_amount;
        dtckin_amount += item.dtckin_amount;
        dbag_amount += item.dbag_amount;
        dbag_weight += item.dbag_weight;
        drk_amount += item.drk_amount;
        drk_weight += item.drk_weight;
        processed_pax += item.processed_pax;
        time += item.time;
    };
};

struct TAgentCmp {
    bool operator() (const TAgentStatKey &lr, const TAgentStatKey &rr) const
    {
        if(lr.airline_view == rr.airline_view)
            if(lr.flt_no == rr.flt_no)
                if(lr.suffix_view == rr.suffix_view)
                    if(lr.airp_view == rr.airp_view)
                        if(lr.scd_out_local == rr.scd_out_local)
                            if(lr.point_id == rr.point_id)
                                if(lr.desk == rr.desk)
                                    if(lr.user_descr == rr.user_descr)
                                        return lr.user_id < rr.user_id;
                                    else
                                        return lr.user_descr < rr.user_descr;
                                else
                                    return lr.desk < rr.desk;
                            else
                                return lr.point_id < rr.point_id;
                        else
                            return lr.scd_out_local < rr.scd_out_local;
                    else
                        return lr.airp_view < rr.airp_view;
                else
                    return lr.suffix_view < rr.suffix_view;
            else
                return lr.flt_no < rr.flt_no;
        else
            return lr.airline_view < rr.airline_view;
    }
};

typedef map<TAgentStatKey, TAgentStatRow, TAgentCmp> TAgentStat;

void RunAgentStat(const TStatParams &params,
                  TAgentStat &AgentStat, TAgentStatRow &AgentStatTotal,
                  TPrintAirline &prn_airline, bool override_max_rows = false)
{
    TQuery Qry(&OraSession);
    for(int pass = 0; pass <= 1; pass++) {
        string SQLText =
            "SELECT \n"
            "  points.point_id, \n"
            "  points.airline, \n"
            "  points.flt_no, \n"
            "  points.suffix, \n"
            "  points.airp, \n"
            "  points.scd_out, \n"
            "  users2.user_id, \n"
            "  users2.descr AS user_descr, \n"
            "  ags.desk, \n"
            "  pax_time, \n"
            "  pax_amount, \n"
            "  ags.dpax_amount.inc pax_am_inc, \n"
            "  ags.dpax_amount.dec pax_am_dec, \n"
            "  ags.dtckin_amount.inc tckin_am_inc, \n"
            "  ags.dtckin_amount.dec tckin_am_dec, \n"
            "  ags.dbag_amount.inc bag_am_inc, \n"
            "  ags.dbag_amount.dec bag_am_dec, \n"
            "  ags.dbag_weight.inc bag_we_inc, \n"
            "  ags.dbag_weight.dec bag_we_dec, \n"
            "  ags.drk_amount.inc rk_am_inc, \n"
            "  ags.drk_amount.dec rk_am_dec, \n"
            "  ags.drk_weight.inc rk_we_inc, \n"
            "  ags.drk_weight.dec rk_we_dec \n"
            "FROM \n"
            "   users2, \n";
        if(pass != 0) {
            SQLText +=
                "   arx_points points, \n"
                "   arx_agent_stat ags \n";
        } else {
            SQLText +=
                "   points, \n"
                "   agent_stat ags \n";
        }
        SQLText += "WHERE \n";
        if (pass!=0)
            SQLText += "    ags.point_part_key = points.part_key AND \n";
        SQLText +=
            "    ags.point_id = points.point_id AND \n"
            "    points.pr_del >= 0 AND \n"
            "    ags.user_id = users2.user_id AND \n";
        if (pass!=0)
          SQLText +=
            "    ags.part_key >= :FirstDate AND ags.part_key < :LastDate \n";
        else
          SQLText +=
            "    ags.ondate >= :FirstDate AND ags.ondate < :LastDate \n";
        if(params.flt_no != NoExists) {
            SQLText += " AND points.flt_no = :flt_no \n";
            Qry.CreateVariable("flt_no", otInteger, params.flt_no);
        }
        if (!params.airps.elems().empty()) {
            if (params.airps.elems_permit())
                SQLText += " AND points.airp IN " + GetSQLEnum(params.airps.elems()) + "\n";
            else
                SQLText += " AND points.airp NOT IN " + GetSQLEnum(params.airps.elems()) + "\n";
        };
        if (!params.airlines.elems().empty()) {
            if (params.airlines.elems_permit())
                SQLText += " AND points.airline IN " + GetSQLEnum(params.airlines.elems()) + "\n";
            else
                SQLText += " AND points.airline NOT IN " + GetSQLEnum(params.airlines.elems()) + "\n";
        };
        if(!params.desk.empty()) {
            SQLText += " AND ags.desk = :desk \n";
            Qry.CreateVariable("desk", otString, params.desk);
        }
        if(params.user_id != NoExists) {
            SQLText += " AND users2.user_id = :user_id \n";
            Qry.CreateVariable("user_id", otInteger, params.user_id);
        }
        if(!params.user_login.empty()) {
            SQLText += " AND users2.login = :user_login \n";
            Qry.CreateVariable("user_login", otString, params.user_login);
        }
        //ProgTrace(TRACE5, "RunAgentStat: pass=%d SQL=\n%s", pass, SQLText.c_str());
        Qry.SQLText = SQLText;
        Qry.CreateVariable("FirstDate", otDate, params.FirstDate);
        Qry.CreateVariable("LastDate", otDate, params.LastDate);
        Qry.Execute();
        if(not Qry.Eof) {
            int col_point_id = Qry.FieldIndex("point_id");
            int col_airline = Qry.FieldIndex("airline");
            int col_flt_no = Qry.FieldIndex("flt_no");
            int col_suffix = Qry.FieldIndex("suffix");
            int col_airp = Qry.FieldIndex("airp");
            int col_scd_out = Qry.FieldIndex("scd_out");
            int col_user_id = Qry.FieldIndex("user_id");
            int col_user_descr = Qry.FieldIndex("user_descr");
            int col_desk = Qry.FieldIndex("desk");
            int col_pax_time = Qry.FieldIndex("pax_time");
            int col_pax_amount = Qry.FieldIndex("pax_amount");
            int col_pax_am_inc = Qry.FieldIndex("pax_am_inc");
            int col_pax_am_dec = Qry.FieldIndex("pax_am_dec");
            int col_tckin_am_inc = Qry.FieldIndex("tckin_am_inc");
            int col_tckin_am_dec = Qry.FieldIndex("tckin_am_dec");
            int col_bag_am_inc = Qry.FieldIndex("bag_am_inc");
            int col_bag_am_dec = Qry.FieldIndex("bag_am_dec");
            int col_bag_we_inc = Qry.FieldIndex("bag_we_inc");
            int col_bag_we_dec = Qry.FieldIndex("bag_we_dec");
            int col_rk_am_inc = Qry.FieldIndex("rk_am_inc");
            int col_rk_am_dec = Qry.FieldIndex("rk_am_dec");
            int col_rk_we_inc = Qry.FieldIndex("rk_we_inc");
            int col_rk_we_dec = Qry.FieldIndex("rk_we_dec");
            for(; not Qry.Eof; Qry.Next())
            {
              string airline = Qry.FieldAsString(col_airline);
              prn_airline.check(airline);

              TAgentStatRow row;
              row.processed_pax = Qry.FieldAsInteger(col_pax_amount);
              row.time = Qry.FieldAsInteger(col_pax_time);
              row.dpax_amount.inc = Qry.FieldAsInteger(col_pax_am_inc);
              row.dpax_amount.dec = Qry.FieldAsInteger(col_pax_am_dec);
              row.dtckin_amount.inc = Qry.FieldAsInteger(col_tckin_am_inc);
              row.dtckin_amount.dec = Qry.FieldAsInteger(col_tckin_am_dec);
              row.dbag_amount.inc = Qry.FieldAsInteger(col_bag_am_inc);
              row.dbag_amount.dec = Qry.FieldAsInteger(col_bag_am_dec);
              row.dbag_weight.inc = Qry.FieldAsInteger(col_bag_we_inc);
              row.dbag_weight.dec = Qry.FieldAsInteger(col_bag_we_dec);
              row.drk_amount.inc = Qry.FieldAsInteger(col_rk_am_inc);
              row.drk_amount.dec = Qry.FieldAsInteger(col_rk_am_dec);
              row.drk_weight.inc = Qry.FieldAsInteger(col_rk_we_inc);
              row.drk_weight.dec = Qry.FieldAsInteger(col_rk_we_dec);

              if (params.statType == statAgentTotal)
              {
                row.dpax_amount.inc -= row.dpax_amount.dec; //сухой остаток (не суммируем)
                row.dpax_amount.dec = 0;
                row.dtckin_amount.inc -= row.dtckin_amount.dec;
                row.dtckin_amount.dec = 0;
                row.dbag_amount.inc -= row.dbag_amount.dec;
                row.dbag_amount.dec = 0;
                row.dbag_weight.inc -= row.dbag_weight.dec;
                row.dbag_weight.dec = 0;
                row.drk_amount.inc -= row.drk_amount.dec;
                row.drk_amount.dec = 0;
                row.drk_weight.inc -= row.drk_weight.dec;
                row.drk_weight.dec = 0;
              };

              if (!params.skip_rows)
              {
                string airp = Qry.FieldAsString(col_airp);
                TAgentStatKey key;
                if(
                        params.statType == statAgentFull or
                        params.statType == statAgentShort
                        ) {
                    key.point_id = Qry.FieldAsInteger(col_point_id);
                    key.airline_view = ElemIdToCodeNative(etAirline, airline);
                    key.flt_no = Qry.FieldAsInteger(col_flt_no);
                    key.suffix_view = ElemIdToCodeNative(etSuffix, Qry.FieldAsString(col_suffix));
                    key.airp = Qry.FieldAsString(col_airp);
                    key.airp_view = ElemIdToCodeNative(etAirp, key.airp);
                    key.scd_out = Qry.FieldAsDateTime(col_scd_out);
                    try
                    {
                        key.scd_out_local = UTCToClient(key.scd_out, AirpTZRegion(key.airp));
                    }
                    catch(AstraLocale::UserException &E) {};
                }

                if(params.statType == statAgentFull) {
                    key.desk = Qry.FieldAsString(col_desk);
                }
                if(
                        params.statType == statAgentFull or
                        params.statType == statAgentTotal
                  ) {
                    key.user_id = Qry.FieldAsInteger(col_user_id);
                    key.user_descr = Qry.FieldAsString(col_user_descr);
                }

                AddStatRow(key, row, AgentStat, override_max_rows);
              }
              else
              {
                AgentStatTotal+=row;
              };
            }
        }
    }
    return;
}

void createXMLAgentStat(const TStatParams &params,
                        const TAgentStat &AgentStat, const TAgentStatRow &AgentStatTotal,
                        const TPrintAirline &airline, xmlNodePtr resNode)
{
    if(AgentStat.empty() && AgentStatTotal==TAgentStatRow())
      throw AstraLocale::UserException("MSG.NOT_DATA");

    NewTextChild(resNode, "airline", airline.get(), "");
    xmlNodePtr grdNode = NewTextChild(resNode, "grd");
    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    TAgentStatRow total;
    bool showTotal=true;
    if (!params.skip_rows)
    {
      int rows = 0;
      for(TAgentStat::const_iterator im = AgentStat.begin(); im != AgentStat.end(); ++im, rows++)
      {
        if(rows >= MAX_STAT_ROWS()) {
            throw MaxStatRowsException("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH", LParams() << LParam("num", MAX_STAT_ROWS()));
            /*AstraLocale::showErrorMessage("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH",
                                          LParams() << LParam("num", MAX_STAT_ROWS()));
            if (WITHOUT_TOTAL_WHEN_PROBLEM) showTotal=false; //не будем показывать итоговую строку дабы не ввести в заблуждение
            break;*/
        }
        TDateTime scd_out_local=im->first.scd_out_local;
        if(
                params.statType == statAgentFull or
                params.statType == statAgentShort
          )
        {
          if (scd_out_local==NoExists)
          try
          {
              scd_out_local = UTCToClient(im->first.scd_out, AirpTZRegion(im->first.airp));
          }
          catch(AstraLocale::UserException &E)
          {
              AstraLocale::showErrorMessage("MSG.ERR_MSG.NOT_ALL_FLIGHTS_ARE_SHOWN", LParams() << LParam("msg", getLocaleText(E.getLexemaData())));
              if (WITHOUT_TOTAL_WHEN_PROBLEM) showTotal=false; //не будем показывать итоговую строку дабы не ввести в заблуждение
              continue;
          };
        };

        rowNode = NewTextChild(rowsNode, "row");
        ostringstream buf;
        if(
                params.statType == statAgentFull or
                params.statType == statAgentShort
          ) {
            // Код а/к
            buf << im->first.airline_view
                << setw(3) << setfill('0') << im->first.flt_no
                << im->first.suffix_view << " "
                << im->first.airp_view;
            NewTextChild(rowNode, "col", buf.str());
            // Дата
            NewTextChild(rowNode, "col", DateTimeToStr( scd_out_local, "dd.mm.yy"));
        }

        if(params.statType == statAgentFull) {
            // Пульт
            NewTextChild(rowNode, "col", im->first.desk);
        }
        if(
                params.statType == statAgentFull or
                params.statType == statAgentTotal
          ) {
            // Пользователь
            NewTextChild(rowNode, "col", im->first.user_descr);
        }
        if(params.statType == statAgentTotal) {
            // Кол-во пасс.
            NewTextChild(rowNode, "col", im->second.dpax_amount.inc);
            // Кол-во сквоз.
            NewTextChild(rowNode, "col", im->second.dtckin_amount.inc);
            // Багаж (мест/вес)
            NewTextChild(rowNode, "col", IntToString(im->second.dbag_amount.inc) +
                                         "/" +
                                         IntToString(im->second.dbag_weight.inc));
            // Р/кладь (вес)
            NewTextChild(rowNode, "col", im->second.drk_weight.inc);
        } else {
            // Кол-во пасс. (+)
            NewTextChild(rowNode, "col", im->second.dpax_amount.inc);
            // Кол-во пасс. (-)
            NewTextChild(rowNode, "col", -im->second.dpax_amount.dec);
            // Кол-во сквоз. (+)
            NewTextChild(rowNode, "col", im->second.dtckin_amount.inc);
            // Кол-во сквоз. (-)
            NewTextChild(rowNode, "col", -im->second.dtckin_amount.dec);
            // Багаж (мест/вес) (+)
            NewTextChild(rowNode, "col", IntToString(im->second.dbag_amount.inc) +
                                         "/" +
                                         IntToString(im->second.dbag_weight.inc));
            // Багаж (мест/вес) (-)
            NewTextChild(rowNode, "col", IntToString(-im->second.dbag_amount.dec) +
                                         "/" +
                                         IntToString(-im->second.dbag_weight.dec));
            // Р/кладь (вес) (+)
            NewTextChild(rowNode, "col", im->second.drk_weight.inc);
            // Р/кладь (вес) (-)
            NewTextChild(rowNode, "col", -im->second.drk_weight.dec);
            // Среднее время, затраченное на пассажира
            buf.str("");
            if (im->second.processed_pax!=0)
              buf << fixed << setprecision(2) << im->second.time / im->second.processed_pax;
            else
              buf << fixed << setprecision(2) << 0.0;
            NewTextChild(rowNode, "col", buf.str());
        }
        total += im->second;
      };
    }
    else total=AgentStatTotal;

    rowNode = NewTextChild(rowsNode, "row");

    xmlNodePtr colNode;
    if(
            params.statType == statAgentFull or
            params.statType == statAgentShort
      ) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("Рейс"));
        SetProp(colNode, "width", 70);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col", getLocaleText("Итого:"));
        colNode = NewTextChild(headerNode, "col", getLocaleText("Дата"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortDate);
        NewTextChild(rowNode, "col");
    }
    if(params.statType == statAgentFull) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("Стойка"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col");
    }
    if(
            params.statType == statAgentFull or
            params.statType == statAgentTotal
      ) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("Агент"));
        SetProp(colNode, "width", 100);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        if (params.statType == statAgentTotal)
          NewTextChild(rowNode, "col", getLocaleText("Итого:"));
        else
          NewTextChild(rowNode, "col");
    }
    if(params.statType == statAgentTotal) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("Пас."));
        SetProp(colNode, "width", 45);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.dpax_amount.inc);
        colNode = NewTextChild(headerNode, "col", getLocaleText("Сквоз."));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.dtckin_amount.inc);
        colNode = NewTextChild(headerNode, "col", getLocaleText("Баг."));
        SetProp(colNode, "width", 70);
        SetProp(colNode, "align", TAlignment::Center);
        SetProp(colNode, "sort", sortIntegerSlashInteger);
        NewTextChild(rowNode, "col", IntToString(total.dbag_amount.inc) +
                                     "/" +
                                     IntToString(total.dbag_weight.inc));
        colNode = NewTextChild(headerNode, "col", getLocaleText("Р/к"));
        SetProp(colNode, "width", 45);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.drk_weight.inc);
    } else {
        colNode = NewTextChild(headerNode, "col", getLocaleText("Пас.")+" (+)");
        SetProp(colNode, "width", 45);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.dpax_amount.inc);
        colNode = NewTextChild(headerNode, "col", getLocaleText("Пас.")+" (-)");
        SetProp(colNode, "width", 45);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", -total.dpax_amount.dec);
        colNode = NewTextChild(headerNode, "col", getLocaleText("Сквоз.")+"(+)");
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.dtckin_amount.inc);
        colNode = NewTextChild(headerNode, "col", getLocaleText("Сквоз.")+"(-)");
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", -total.dtckin_amount.dec);
        colNode = NewTextChild(headerNode, "col", getLocaleText("Баг.")+" (+)");
        SetProp(colNode, "width", 70);
        SetProp(colNode, "align", TAlignment::Center);
        SetProp(colNode, "sort", sortIntegerSlashInteger);
        NewTextChild(rowNode, "col", IntToString(total.dbag_amount.inc) +
                                     "/" +
                                     IntToString(total.dbag_weight.inc));
        colNode = NewTextChild(headerNode, "col", getLocaleText("Баг.")+" (-)");
        SetProp(colNode, "width", 70);
        SetProp(colNode, "align", TAlignment::Center);
        SetProp(colNode, "sort", sortIntegerSlashInteger);
        NewTextChild(rowNode, "col", IntToString(-total.dbag_amount.dec) +
                                     "/" +
                                     IntToString(-total.dbag_weight.dec));
        colNode = NewTextChild(headerNode, "col", getLocaleText("Р/к")+" (+)");
        SetProp(colNode, "width", 45);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.drk_weight.inc);
        colNode = NewTextChild(headerNode, "col", getLocaleText("Р/к")+" (-)");
        SetProp(colNode, "width", 45);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", -total.drk_weight.dec);
        colNode = NewTextChild(headerNode, "col", getLocaleText("Сек./пас."));
        SetProp(colNode, "width", 65);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortFloat);
        ostringstream buf;
        if (total.processed_pax!=0)
          buf << fixed << setprecision(2) << total.time / total.processed_pax;
        else
          buf << fixed << setprecision(2) << 0.0;
        NewTextChild(rowNode, "col", buf.str());
    };

    if (!showTotal)
    {
      xmlUnlinkNode(rowNode);
      xmlFreeNode(rowNode);
    };

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("Статистика по агентам"));
    string buf;
    switch(params.statType) {
        case statAgentShort:
            buf = getLocaleText("Общая");
            break;
        case statAgentFull:
            buf = getLocaleText("Подробная");
            break;
        case statAgentTotal:
            buf = getLocaleText("Итого");
            break;
        default:
            throw Exception("createXMLAgentStat: unexpected statType %d", params.statType);
            break;
    }
    NewTextChild(variablesNode, "stat_type_caption", buf);
}

struct TAgentStatCombo : public TOrderStatItem
{
    typedef std::pair<TAgentStatKey, TAgentStatRow> Tdata;
    Tdata data;
    TStatParams params;
    TAgentStatCombo(const Tdata &aData, const TStatParams &aParams)
        : data(aData), params(aParams) {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void TAgentStatCombo::add_header(ostringstream &buf) const
{
    if (params.statType == statAgentFull or params.statType == statAgentShort)
    {
        buf << "Рейс" << delim;
        buf << "Дата" << delim;
    }
    if (params.statType == statAgentFull)
        buf << "Стойка" << delim;
    if (params.statType == statAgentFull or params.statType == statAgentTotal)
        buf << "Агент" << delim;
    if (params.statType == statAgentTotal)
    {
        buf << "Пас." << delim;
        buf << "Сквоз." << delim;
        buf << "Баг." << delim;
        buf << "Р/к";
    }
    else
    {
        buf << "Пас. (+)" << delim;
        buf << "Пас. (-)" << delim;
        buf << "Сквоз.(+)" << delim;
        buf << "Сквоз.(-)" << delim;
        buf << "Баг. (+)" << delim;
        buf << "Баг. (-)" << delim;
        buf << "Р/к (+)" << delim;
        buf << "Р/к (-)" << delim;
        buf << "Сек./пас.";
    }
    buf << endl;
}

void TAgentStatCombo::add_data(ostringstream &buf) const
{
    TDateTime scd_out_local = data.first.scd_out_local;
    if (scd_out_local == NoExists)
    try { scd_out_local = UTCToClient(data.first.scd_out, AirpTZRegion(data.first.airp)); }
    catch(AstraLocale::UserException &E)
    {
        AstraLocale::showErrorMessage("MSG.ERR_MSG.NOT_ALL_FLIGHTS_ARE_SHOWN",
            LParams() << LParam("msg", getLocaleText(E.getLexemaData())));
        return;
    };
    ostringstream oss;
    if (params.statType == statAgentFull or params.statType == statAgentShort)
    {
        oss << data.first.airline_view
            << setw(3) << setfill('0') << data.first.flt_no
            << data.first.suffix_view << " "
            << data.first.airp_view;
        buf << oss.str() << delim; // Код а/к
        buf << DateTimeToStr( scd_out_local, "dd.mm.yy") << delim; // Дата
    }
    if (params.statType == statAgentFull)
        buf << data.first.desk << delim; // Пульт
    if (params.statType == statAgentFull or params.statType == statAgentTotal)
        buf << data.first.user_descr << delim; // Пользователь
    if (params.statType == statAgentTotal)
    {
        buf << data.second.dpax_amount.inc << delim; // Кол-во пасс.
        buf << data.second.dtckin_amount.inc << delim; // Кол-во сквоз.
        buf <<  IntToString(data.second.dbag_amount.inc) + "/" +
                IntToString(data.second.dbag_weight.inc) << delim; // Багаж (мест/вес)
        buf << data.second.drk_weight.inc << endl; // Р/кладь (вес)
    }
    else
    {
        buf << data.second.dpax_amount.inc << delim; // Кол-во пасс. (+)
        buf << -data.second.dpax_amount.dec << delim; // Кол-во пасс. (-)
        buf << data.second.dtckin_amount.inc << delim; // Кол-во сквоз. (+)
        buf << -data.second.dtckin_amount.dec << delim; // Кол-во сквоз. (-)
        buf <<  IntToString(data.second.dbag_amount.inc) + "/" +
                IntToString(data.second.dbag_weight.inc) << delim; // Багаж (мест/вес) (+)
        buf <<  IntToString(-data.second.dbag_amount.dec) + "/" +
                IntToString(-data.second.dbag_weight.dec) << delim; // Багаж (мест/вес) (-)
        buf << data.second.drk_weight.inc << delim; // Р/кладь (вес) (+)
        buf << -data.second.drk_weight.dec << delim; // Р/кладь (вес) (-)
        oss.str("");
        if (data.second.processed_pax!=0)
            oss << fixed << setprecision(2) << data.second.time / data.second.processed_pax;
        else
            oss << fixed << setprecision(2) << 0.0;
        buf << oss.str() << endl; // Среднее время, затраченное на пассажира
    }
}

template <class T>
void RunAgentStatFile(const TStatParams &params, T &writer, TPrintAirline &prn_airline)
{
    TAgentStat AgentStat;
    TAgentStatRow AgentStatTotal;
    RunAgentStat(params, AgentStat, AgentStatTotal, prn_airline, true);
    for(TAgentStat::const_iterator im = AgentStat.begin(); im != AgentStat.end(); ++im)
        writer.insert(TAgentStatCombo(*im, params));
}

/****************** end of AgentStat ******************/


// Отображение параметров Сирены-трэвел в параметры Астры.

/* !!! Если бы это был С++11
const map<string, string> m =
{
{"Short",       "Общая"},
{"Detail",      "Детализированная"},
{"Full",        "Подробная"},
{"Transfer",    "Трансфер"},
{"Pact",        "Договор"},
{"SelfCkin",    "Саморегистрация"},
{"Agent",       "По агентам"},
{"Tlg",         "Отпр. телеграммы"}
};
*/

#include <boost/assign/list_of.hpp>

const map<string, string> ST_to_Astra = boost::assign::map_list_of
("Short",       "Общая")
("Detail",      "Детализированная")
("Full",        "Подробная")
("Total",       "Итого")
("Transfer",    "Трансфер")
("Pact",        "Договор")
("SelfCkin",    "Саморегистрация")
("Agent",       "По агентам")
("Tlg",         "Отпр. телеграммы");

void convertStatParam(xmlNodePtr paramNode)
{
    string val = NodeAsString(paramNode);
    map<string, string>::const_iterator idx = ST_to_Astra.find(val);
    if(idx == ST_to_Astra.end())
        throw Exception("wrong param value '%s'", val.c_str());
    xmlNodeSetContent(paramNode, idx->second.c_str());
}

struct TParamItem {
    string code;
    int visible;
    string label;
    string caption;
    string ctype;
    string name; // control name property
    int width;
    int len;
    int isalnum;
    string ref;
    string ref_field;
    string tag;
    TParamItem():
        visible(NoExists),
        width(NoExists),
        len(NoExists),
        isalnum(NoExists)
    {}
    void fromDB(TQuery &Qry);
    void toXML(xmlNodePtr resNode);
};

void TParamItem::toXML(xmlNodePtr resNode)
{
    xmlNodePtr itemNode = NewTextChild(resNode, "item");
    NewTextChild(itemNode, "code", code);
    NewTextChild(itemNode, "visible", visible);
    NewTextChild(itemNode, "label", label);
    NewTextChild(itemNode, "caption", caption);
    NewTextChild(itemNode, "ctype", ctype);
    NewTextChild(itemNode, "name", name);
    NewTextChild(itemNode, "width", width);
    NewTextChild(itemNode, "len", len);
    NewTextChild(itemNode, "isalnum", isalnum);
    NewTextChild(itemNode, "ref", ref);
    NewTextChild(itemNode, "ref_field", ref_field);
    NewTextChild(itemNode, "tag", tag);

    if(code == "SegCategory") {
        xmlNodePtr dataNode = NewTextChild(itemNode, "data");
        xmlNodePtr CBoxItemNode = NewTextChild(dataNode, "item");
        NewTextChild(CBoxItemNode, "code");
        NewTextChild(CBoxItemNode, "caption");
        for(list<pair<TSegCategories::Enum, string> >::const_iterator
                i = TSegCategories::pairsCodes().begin();
                i != TSegCategories::pairsCodes().end(); i++) {
            if(i->first == TSegCategories::Unknown) continue;
            CBoxItemNode = NewTextChild(dataNode, "item");
            NewTextChild(CBoxItemNode, "code", i->second);
            NewTextChild(CBoxItemNode, "caption", getLocaleText(i->second));
        }
    }

    if(code == "Seance") {
        static const char *seances[] = {"", "АК", "АП"};
        xmlNodePtr dataNode = NewTextChild(itemNode, "data");
        for(size_t i = 0; i < sizeof(seances) / sizeof(seances[0]); i++) {
            xmlNodePtr CBoxItemNode = NewTextChild(dataNode, "item");
            NewTextChild(CBoxItemNode, "code", seances[i]);
            NewTextChild(CBoxItemNode, "caption", seances[i]);
        }
    }
}

void TParamItem::fromDB(TQuery &Qry)
{
    code = Qry.FieldAsString("code");
    visible = Qry.FieldAsInteger("visible");
    label = code + "L";
    caption = Qry.FieldAsString("caption");
    ctype = Qry.FieldAsString("ctype");
    name = code + ctype;
    width = Qry.FieldAsInteger("width");
    len = Qry.FieldAsInteger("len");
    isalnum = Qry.FieldAsInteger("isalnum");
    ref = Qry.FieldAsString("ref");
    ref_field = Qry.FieldAsString("ref_field");
    tag = Qry.FieldAsString("tag");
}

struct TLayout {
    map<int, TParamItem> params;
    void get_params();
    void toXML(xmlNodePtr resNode);
    void toXML(xmlNodePtr resNode, const string &tag, const string &qry);
};

void TLayout::get_params()
{
    if(params.empty()) {
        TCachedQuery Qry("select * from stat_params");
        Qry.get().Execute();
        for(; not Qry.get().Eof; Qry.get().Next()) {
            TParamItem item;
            item.fromDB(Qry.get());
            if(
                    item.ctype != "CkBox" or
                    TReqInfo::Instance()->desk.compatible(STAT_CKBOX_VERSION)
              )
                params.insert(make_pair(item.visible, item));
        }
    }
}

void TLayout::toXML(xmlNodePtr resNode, const string &tag, const string &qry)
{
    TCachedQuery Qry(qry);
    Qry.get().Execute();
    xmlNodePtr listNode = NewTextChild(resNode, tag.c_str());
    for(; not Qry.get().Eof; Qry.get().Next()) {
        xmlNodePtr itemNode = NewTextChild(listNode, "item");
        NewTextChild(itemNode, "type", Qry.get().FieldAsString("code"));
        NewTextChild(itemNode, "visible", Qry.get().FieldAsString("visible"));
        if(tag == "types") {
            int item_params = Qry.get().FieldAsInteger("params");
            xmlNodePtr paramsNode = NULL;
            for(map<int, TParamItem>::iterator i = params.begin(); i != params.end(); i++) {
                if((item_params & i->first) == i->first) {
                    if(not paramsNode) paramsNode = NewTextChild(itemNode, "params");
                    NewTextChild(paramsNode, "item", i->second.code);
                }
            }
        }
    }
}

void TLayout::toXML(xmlNodePtr resNode)
{
    get_params();
    xmlNodePtr paramsNode = NewTextChild(resNode, "params");
    for(map<int, TParamItem>::iterator i = params.begin(); i != params.end(); i++)
        i->second.toXML(paramsNode);
    toXML(resNode, "types", "select * from stat_types order by view_order");
    toXML(resNode, "levels", "select * from stat_levels order by view_order");
}

void StatInterface::Layout(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TLayout().toXML(resNode);
    LogTrace(TRACE5) << GetXMLDocText(resNode->doc); // !!!
}

void StatInterface::stat_srv(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr curNode = reqNode->children;
    curNode = NodeAsNodeFast( "content", curNode );
    if (not curNode) return;
    curNode = curNode->children;
    curNode = NodeAsNodeFast("run_stat", curNode);
    if(not curNode)
        throw Exception("wrong format");

    TAccess access;
    access.fromXML(NodeAsNode("access", curNode));
    TAccessElems<string> norm_airlines, norm_airps;
    //нормализуем компании
    norm_airlines.set_elems_permit(access.airlines().elems_permit());
    for(set<string>::iterator i=access.airlines().elems().begin();
                              i!=access.airlines().elems().end(); ++i)
      norm_airlines.add_elem(airl_fromXML(*i, cfErrorIfEmpty, __FUNCTION__, "airline"));
    //нормализуем порты
    norm_airps.set_elems_permit(access.airps().elems_permit());
    for(set<string>::iterator i=access.airps().elems().begin();
                              i!=access.airps().elems().end(); ++i)
      norm_airps.add_elem(airp_fromXML(*i, cfErrorIfEmpty, __FUNCTION__, "airp"));

    TReqInfo &reqInfo = *(TReqInfo::Instance());
    reqInfo.user.access.merge_airlines(norm_airlines);
    reqInfo.user.access.merge_airps(norm_airps);

    xmlNodePtr statModeNode = NodeAsNode("stat_mode", curNode);
    xmlNodePtr statTypeNode = NodeAsNode("stat_type", curNode);
    if(not statModeNode or not statTypeNode)
        throw Exception("wrong param format");
    convertStatParam(statModeNode);
    convertStatParam(statTypeNode);
    RunStat(ctxt, curNode, resNode);
    xmlNodePtr formDataNode = GetNode("form_data", resNode);
    if (formDataNode!=NULL)
    {
      xmlUnlinkNode(formDataNode);
      xmlFreeNode(formDataNode);
    };
    //ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
}

template <class T>
void RunRFISCStat(
        const TStatParams &params,
        T &RFISCStat,
        TPrintAirline &prn_airline
        )
{
    QParams QryParams;
    QryParams
        << QParam("FirstDate", otDate, params.FirstDate)
        << QParam("LastDate", otDate, params.LastDate);
    for(int pass = 0; pass <= 1; pass++) {
        string SQLText =
            "select "
            "   rfisc_stat.rfisc, "
            "   rfisc_stat.point_id, "
            "   rfisc_stat.point_num, "
            "   rfisc_stat.pr_trfer, "
            "   points.scd_out, "
            "   points.airline, "
            "   points.flt_no, "
            "   points.suffix, "
            "   points.airp, "
            "   rfisc_stat.airp_arv, "
            "   points.craft, "
            "   rfisc_stat.travel_time, "
            "   rfisc_stat.trfer_flt_no, "
            "   rfisc_stat.trfer_suffix, "
            "   rfisc_stat.trfer_airp_arv, "
            "   rfisc_stat.desk, "
            "   rfisc_stat.user_login, "
            "   rfisc_stat.user_descr, "
            "   rfisc_stat.time_create, "
            "   rfisc_stat.tag_no, "
            "   rfisc_stat.fqt_no, "
            "   rfisc_stat.excess, "
            "   rfisc_stat.paid "
            "from ";
        if(pass != 0) {
            SQLText +=
                "   arx_points points, \n"
                "   arx_rfisc_stat rfisc_stat \n";
        } else {
            SQLText +=
                "   points, \n"
                "   rfisc_stat \n";
        }
        SQLText +=
            "where ";
        SQLText +=
            "   rfisc_stat.point_id = points.point_id and "
            "   points.pr_del >= 0 and ";
        if (!params.airps.elems().empty()) {
            if (params.airps.elems_permit())
                SQLText += " points.airp IN " + GetSQLEnum(params.airps.elems()) + "and \n";
            else
                SQLText += " points.airp NOT IN " + GetSQLEnum(params.airps.elems()) + "and \n";
        };
        if (!params.airlines.elems().empty()) {
            if (params.airlines.elems_permit())
                SQLText += " points.airline IN " + GetSQLEnum(params.airlines.elems()) + "and \n";
            else
                SQLText += " points.airline NOT IN " + GetSQLEnum(params.airlines.elems()) + "and \n";
        };
        if (pass!=0)
          SQLText +=
            "    points.part_key >= :FirstDate AND points.part_key < :LastDate and \n"
            "    rfisc_stat.part_key >= :FirstDate AND rfisc_stat.part_key < :LastDate \n";
        else
          SQLText +=
            "    points.scd_out >= :FirstDate AND points.scd_out < :LastDate \n";

        TCachedQuery Qry(SQLText, QryParams);

        Qry.get().Execute();
        if(not Qry.get().Eof) {
            int col_rfisc = Qry.get().GetFieldIndex("rfisc");
            int col_point_id = Qry.get().GetFieldIndex("point_id");
            int col_point_num = Qry.get().GetFieldIndex("point_num");
            int col_pr_trfer = Qry.get().GetFieldIndex("pr_trfer");
            int col_scd_out = Qry.get().GetFieldIndex("scd_out");
            int col_airline = Qry.get().GetFieldIndex("airline");
            int col_flt_no = Qry.get().GetFieldIndex("flt_no");
            int col_suffix = Qry.get().GetFieldIndex("suffix");
            int col_airp = Qry.get().GetFieldIndex("airp");
            int col_airp_arv = Qry.get().GetFieldIndex("airp_arv");
            int col_craft = Qry.get().GetFieldIndex("craft");
            int col_travel_time = Qry.get().GetFieldIndex("travel_time");
            int col_trfer_flt_no = Qry.get().GetFieldIndex("trfer_flt_no");
            int col_trfer_suffix = Qry.get().GetFieldIndex("trfer_suffix");
            int col_trfer_airp_arv = Qry.get().GetFieldIndex("trfer_airp_arv");
            int col_desk = Qry.get().GetFieldIndex("desk");
            int col_user_login = Qry.get().GetFieldIndex("user_login");
            int col_user_descr = Qry.get().GetFieldIndex("user_descr");
            int col_time_create = Qry.get().GetFieldIndex("time_create");
            int col_tag_no = Qry.get().GetFieldIndex("tag_no");
            int col_fqt_no = Qry.get().GetFieldIndex("fqt_no");
            int col_excess = Qry.get().GetFieldIndex("excess");
            int col_paid = Qry.get().GetFieldIndex("paid");
            for(; not Qry.get().Eof; Qry.get().Next()) {
                prn_airline.check(Qry.get().FieldAsString(col_airline));
                TRFISCStatRow row;
                row.rfisc = Qry.get().FieldAsString(col_rfisc);
                row.point_id = Qry.get().FieldAsInteger(col_point_id);
                row.point_num = Qry.get().FieldAsInteger(col_point_num);
                row.pr_trfer = Qry.get().FieldAsInteger(col_pr_trfer);
                row.scd_out = Qry.get().FieldAsDateTime(col_scd_out);
                row.flt_no = Qry.get().FieldAsInteger(col_flt_no);
                row.suffix = Qry.get().FieldAsString(col_suffix);
                row.airp = Qry.get().FieldAsString(col_airp);
                row.airp_arv = Qry.get().FieldAsString(col_airp_arv);
                row.craft = Qry.get().FieldAsString(col_craft);
                if(not Qry.get().FieldIsNULL(col_travel_time))
                    row.travel_time = Qry.get().FieldAsDateTime(col_travel_time);
                row.trfer_flt_no = Qry.get().FieldAsInteger(col_trfer_flt_no);
                row.trfer_suffix = Qry.get().FieldAsString(col_trfer_suffix);
                row.trfer_airp_arv = Qry.get().FieldAsString(col_trfer_airp_arv);
                row.desk = Qry.get().FieldAsString(col_desk);
                row.user_login = Qry.get().FieldAsString(col_user_login);
                row.user_descr = Qry.get().FieldAsString(col_user_descr);
                if(not Qry.get().FieldIsNULL(col_time_create))
                    row.time_create = Qry.get().FieldAsDateTime(col_time_create);
                row.tag_no = Qry.get().FieldAsFloat(col_tag_no);
                row.fqt_no = Qry.get().FieldAsString(col_fqt_no);
                if(not Qry.get().FieldIsNULL(col_excess))
                    row.excess = Qry.get().FieldAsInteger(col_excess);
                if(not Qry.get().FieldIsNULL(col_paid))
                    row.paid = Qry.get().FieldAsInteger(col_paid);
                RFISCStat.insert(row);
            }
        }
    }
}

void createCSVFullStat(const TStatParams &params, const TFullStat &FullStat, const TPrintAirline &prn_airline, ostringstream &data)
{
    if(FullStat.empty()) return;

    const char delim = ';';
    data
        << "Код а/к" << delim
        << "Код а/п" << delim
        << "Номер рейса" << delim
        << "Дата" << delim
        << "Направление" << delim
        << "Кол-во пасс." << delim
        << "Web" << delim
        << "БГ" << delim
        << "ПТ" << delim
        << "Киоски" << delim
        << "БГ" << delim
        << "ПТ" << delim
        << "Моб." << delim
        << "БГ" << delim
        << "ПТ" << delim
        << "ВЗ" << delim
        << "РБ" << delim
        << "РМ" << delim
        << "Р/кладь (вес)" << delim
        << "БГ мест" << delim
        << "БГ вес" << delim
        << "Пл.м" << delim
        << "Пл.вес"
        << endl;
    bool showTotal = true;
    for(TFullStat::const_iterator i = FullStat.begin(); i != FullStat.end(); i++) {
        //region обязательно в начале цикла, иначе будет испорчен xml
        string region;
        try
        {
            region = AirpTZRegion(i->first.airp);
        }
        catch(AstraLocale::UserException &E)
        {
            AstraLocale::showErrorMessage("MSG.ERR_MSG.NOT_ALL_FLIGHTS_ARE_SHOWN", LParams() << LParam("msg", getLocaleText(E.getLexemaData())));
            if (WITHOUT_TOTAL_WHEN_PROBLEM) showTotal=false; //не будем показывать итоговую строку дабы не ввести в заблуждение
            continue;
        };

        // Код а/к
        data << i->first.col1 << delim;
        // Код а/п
        data << i->first.col2 << delim;
        // Номер рейса
        data << i->first.flt_no << delim;
        // Дата
        data << DateTimeToStr(
                UTCToClient(i->first.scd_out, region), "dd.mm.yy")
            << delim;
        // Направление
        data << i->first.places.get() << delim;
        // Кол-во пасс.
        data << i->second.pax_amount << delim;
        // Web
        data << i->second.i_stat.web << delim;
        // БГ
        data << i->second.i_stat.web_bag << delim;
        // ПТ
        data << i->second.i_stat.web_bp << delim;
        // Киоски
        data << i->second.i_stat.kiosk << delim;
        // БГ
        data << i->second.i_stat.kiosk_bag << delim;
        // ПТ
        data << i->second.i_stat.kiosk_bp << delim;
        // Моб.
        data << i->second.i_stat.mobile << delim;
        // БГ
        data << i->second.i_stat.mobile_bag << delim;
        // ПТ
        data << i->second.i_stat.mobile_bp << delim;
        // ВЗ
        data << i->second.adult << delim;
        // РБ
        data << i->second.child << delim;
        // РМ
        data << i->second.baby << delim;
        // Р/кладь (вес)
        data << i->second.rk_weight << delim;
        // БГ мест
        data << i->second.bag_amount << delim;
        // БГ вес
        data << i->second.bag_weight << delim;
        // Пл.м
        data << i->second.paid.excess_pcs << delim;
        // Пл.вес
        data << i->second.paid.excess << endl;
    }
}

void createXMLRFISCStat(const TStatParams &params, const TRFISCStat &RFISCStat, const TPrintAirline &prn_airline, xmlNodePtr resNode)
{
    if(RFISCStat.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");
    if (RFISCStat.size() > (size_t)MAX_STAT_ROWS()) throw MaxStatRowsException("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH", LParams() << LParam("num", MAX_STAT_ROWS()));
    NewTextChild(resNode, "airline", prn_airline.get(), "");
    xmlNodePtr grdNode = NewTextChild(resNode, "grd");

    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", "RFISC");
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Платн."));
    SetProp(colNode, "width", 100);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Опл."));
    SetProp(colNode, "width", 80);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Бирка"));
    SetProp(colNode, "width", 80);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("SPEQ"));
    SetProp(colNode, "width", 100);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("FQTV"));
    SetProp(colNode, "width", 150);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Дата вылета"));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Рейс"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("От"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("До"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Тип ВС"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Время в пути"));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Трфр.рейс"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("От"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("До"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("АП рег."));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Стойка"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", "LOGIN");
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Агент"));
    SetProp(colNode, "width", 100);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Дата оформ."));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    ostringstream buf;
    for(TRFISCStat::iterator i = RFISCStat.begin(); i != RFISCStat.end(); i++) {
        rowNode = NewTextChild(rowsNode, "row");
        // RFISC
        NewTextChild(rowNode, "col", i->rfisc);
        // Признак платности багажа
        if(i->excess == NoExists)
            NewTextChild(rowNode, "col");
        else
            NewTextChild(rowNode, "col", i->excess);
        // Признак оплаты
        if(i->paid == NoExists)
            NewTextChild(rowNode, "col");
        else
            NewTextChild(rowNode, "col", i->paid);
        // Бирка
        buf.str("");
        buf
            << fixed
            << setprecision(0)
            << setw(10)
            << setfill('0')
            << i->tag_no;
        NewTextChild(rowNode, "col", buf.str());
        // SPEQ - признак спец. багажа
        NewTextChild(rowNode, "col");
        // Статус или признак FQTV
        NewTextChild(rowNode, "col", i->fqt_no);

        // Информация о рейсе
        // Дата вылета
        NewTextChild(rowNode, "col", DateTimeToStr(i->scd_out, "dd.mm.yyyy"));
        // Рейс
        buf.str("");
        buf << setw(3) << setfill('0') << i->flt_no << ElemIdToCodeNative(etSuffix, i->suffix);
        NewTextChild(rowNode, "col", buf.str());
        // От
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->airp));
        // До
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->airp_arv));
        // Тип ВС
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etCraft, i->craft));
        // Время в пути
        if(i->travel_time == NoExists)
            NewTextChild(rowNode, "col");
        else
            NewTextChild(rowNode, "col", DateTimeToStr(i->travel_time, "hh:nn"));

        // Трансфер на
        ostringstream trfer_flt_no;
        string trfer_airp_dep;
        string trfer_airp_arv;
        if(i->trfer_flt_no) {
            trfer_flt_no << setw(3) << setfill('0') << i->trfer_flt_no << ElemIdToCodeNative(etSuffix, i->trfer_suffix);
            trfer_airp_dep = i->airp;
            trfer_airp_arv = i->trfer_airp_arv;
        }
        // Рейс
        NewTextChild(rowNode, "col", trfer_flt_no.str());
        // От (совпадает с От рейса)
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, trfer_airp_dep));
        // До
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, trfer_airp_arv));

        // Информация об агенте
        // АП рег. (совпадает с От рейса)
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->airp));
        // Стойка
        NewTextChild(rowNode, "col", i->desk);
        // LOGIN
        NewTextChild(rowNode, "col", i->user_login);
        // Агент
        NewTextChild(rowNode, "col", i->user_descr);
        // Дата оформления
        if(i->time_create == NoExists)
            NewTextChild(rowNode, "col");
        else
            NewTextChild(rowNode, "col", DateTimeToStr(i->time_create, "dd.mm.yyyy"));
    }

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("Багажные RFISC"));
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("Подробная"));

    NewTextChild(variablesNode, "CAP.STAT.AGENT_INFO", getLocaleText("CAP.STAT.AGENT_INFO"));
    NewTextChild(variablesNode, "CAP.STAT.BAG_INFO", getLocaleText("CAP.STAT.BAG_INFO"));
    NewTextChild(variablesNode, "CAP.STAT.PAX_INFO", getLocaleText("CAP.STAT.PAX_INFO"));
    NewTextChild(variablesNode, "CAP.STAT.FLT_INFO", getLocaleText("CAP.STAT.FLT_INFO"));
}

/******************* Service Stat ****************************/

namespace ServiceStat {
    struct TFltInfo {
        struct THelperFltInfo {
            string airp_last;
            TDateTime travel_time;
            THelperFltInfo(): travel_time(NoExists) {}
        };
        typedef map<int, THelperFltInfo> TItems;
        TItems items;
        TItems::iterator get(const int &point_id, const string &craft, const string &airp)
        {
            TItems::iterator result = items.find(point_id);
            if(result == items.end()) {
                THelperFltInfo hfi;
                TTripRoute route;
                route.GetRouteAfter(NoExists, point_id, trtNotCurrent, trtNotCancelled);
                hfi.airp_last = route.back().airp;

                hfi.travel_time = getTimeTravel(craft, airp, hfi.airp_last);
                pair<TItems::iterator, bool> res = items.insert(make_pair(point_id, hfi));
                result = res.first;
            }
            return result;
        }
    };
}

struct TRemInfo {
    string rfisc;
    double rate;
    string rate_cur;
    TRemInfo(const string &rem) {
        parse(rem);
    }
    void parse(const string &rem);
};

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

void TRemInfo::parse(const string &rem)
{
    rfisc.clear();
    rate_cur.clear();
    rate = NoExists;

    vector<string> tokens;
    boost::split(tokens, rem, boost::is_any_of("/"));
    if(tokens.size() == 2) { // PRSA/0B7
        rfisc = tokens[1];
    } else if(tokens.size() == 4) { // PRSA/0B7/1500/RUB
        rfisc = tokens[1];
        if ( StrToFloat( tokens[2].c_str(), rate ) == EOF )
            rate = NoExists;
        TElemFmt fmt;
        rate_cur = ElemToElemId(etCurrency, tokens[3], fmt);
    }
}

struct TAirpArvInfo {
    map<int, string> items;
    string get(TQuery &Qry)
    {
        int grp_id = Qry.FieldAsInteger("grp_id");
        map<int, string>::iterator im = items.find(grp_id);
        if(im == items.end()) {
            TCkinRoute tckin_route;
            tckin_route.GetRouteAfter(grp_id, crtWithCurrent, crtIgnoreDependent);
            string airp_arv;
            if(tckin_route.empty())
                airp_arv = Qry.FieldAsString("airp_arv");
            else
                airp_arv = tckin_route.back().airp_arv;
            pair<map<int, string>::iterator, bool> res = items.insert(make_pair(grp_id, airp_arv));
            im = res.first;
        }
        return im->second;
    }
};

void get_limited_capability_stat(int point_id)
{
    TCachedQuery delQry("delete from limited_capability_stat where point_id = :point_id", QParams() << QParam("point_id", otInteger, point_id));
    delQry.get().Execute();

    TRemGrp rem_grp;
    rem_grp.Load(retLIMITED_CAPAB_STAT, point_id);
    if(rem_grp.empty()) return;

    TCachedQuery Qry(
            "select pax.grp_id, pax_grp.airp_arv, rem_code "
            "from pax_grp, pax, pax_rem where "
            "   pax_grp.point_dep = :point_id and "
            "   pax.grp_id = pax_grp.grp_id and "
            "   pax_rem.pax_id = pax.pax_id ",
            QParams() << QParam("point_id", otInteger, point_id)
            );
    Qry.get().Execute();
    if(not Qry.get().Eof) {

        TAirpArvInfo airp_arv_info;

        map<string, map<string, int> > rems;
        for(; not Qry.get().Eof; Qry.get().Next()) {
            string airp_arv = airp_arv_info.get(Qry.get());
            string rem = Qry.get().FieldAsString("rem_code");
            if(rem_grp.exists(rem)) rems[airp_arv][rem]++;
        }

        if(not rems.empty()) {
            TCachedQuery insQry(
                    "insert into limited_capability_stat ( "
                    "   point_id, "
                    "   airp_arv, "
                    "   rem_code, "
                    "   pax_amount "
                    ") values ( "
                    "   :point_id, "
                    "   :airp_arv, "
                    "   :rem_code, "
                    "   :pax_amount "
                    ") ",
                    QParams()
                    << QParam("point_id", otInteger, point_id)
                    << QParam("airp_arv", otString)
                    << QParam("rem_code", otString)
                    << QParam("pax_amount", otInteger)
                    );
            for(map<string, map<string, int> >::iterator airp_arv = rems.begin(); airp_arv != rems.end(); airp_arv++) {
                for(map<string, int>::iterator rem = airp_arv->second.begin(); rem != airp_arv->second.end(); rem++) {
                    insQry.get().SetVariable("airp_arv", airp_arv->first);
                    insQry.get().SetVariable("rem_code", rem->first);
                    insQry.get().SetVariable("pax_amount", rem->second);
                    insQry.get().Execute();
                }
            }
        }
    }
}

string segListFromDB(int tckin_id)
{
    TCachedQuery Qry(
            "select grp_id from tckin_pax_grp where tckin_id = :tckin_id order by seg_no",
            QParams() << QParam("tckin_id", otInteger, tckin_id));
    Qry.get().Execute();
    ostringstream result;
    for(; not Qry.get().Eof; Qry.get().Next()) {
        int grp_id = Qry.get().FieldAsInteger("grp_id");

        if(not result.str().empty()) result << ";";
        TTripInfo info;
        if(info.getByGrpId(grp_id)) {
            result
                << info.airline << ","
                << info.airp << ","
                << info.flt_no << ","
                << info.suffix << ","
                << DateTimeToStr(info.scd_out) << ",";
            CheckIn::TPaxGrpItem grp;
            if(grp.fromDB(grp_id))
                result << grp.airp_arv;
        }
    }
    return result.str();
}

void get_trfer_pax_stat(int point_id)
{
    TCachedQuery delQry("delete from trfer_pax_stat where point_id = :point_id",
            QParams() << QParam("point_id", otInteger, point_id));
    delQry.get().Execute();

    TTripInfo info;
    info.getByPointId(point_id);

    TCachedQuery insQry(
            "insert into trfer_pax_stat( "
            "   point_id, "
            "   scd_out, "
            "   pax_id, "
            "   rk_weight, "
            "   bag_weight, "
            "   bag_amount, "
            "   segments "
            ") values( "
            "   :point_id, "
            "   :scd_out, "
            "   :pax_id, "
            "   :rk_weight, "
            "   :bag_weight, "
            "   :bag_amount, "
            "   :segments "
            ")",
            QParams()
            << QParam("point_id", otInteger, point_id)
            << QParam("scd_out", otDate, info.scd_out)
            << QParam("pax_id", otInteger)
            << QParam("rk_weight", otInteger)
            << QParam("bag_weight", otInteger)
            << QParam("bag_amount", otInteger)
            << QParam("segments", otString)
            );

    TCachedQuery selQry(
            "select "
            "    pax.pax_id, "
            "    tckin_pax_grp.tckin_id, "
            "    NVL(ckin.get_rkWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num),0) AS rk_weight, "
            "    NVL(ckin.get_bagAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num),0) AS bag_amount, "
            "    NVL(ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num),0) AS bag_weight "
            "from "
            "    pax_grp, "
            "    tckin_pax_grp, "
            "    pax "
            "where "
            "    pax_grp.point_dep = :point_id and "
            "    pax_grp.grp_id = tckin_pax_grp.grp_id and "
            "    tckin_pax_grp.seg_no = 1 and "
            "    pax_grp.grp_id = pax.grp_id ",
            QParams() << QParam("point_id", otInteger, point_id));
    selQry.get().Execute();
    if(not selQry.get().Eof) {
        int col_pax_id = selQry.get().FieldIndex("pax_id");
        int col_tckin_id = selQry.get().FieldIndex("tckin_id");
        int col_rk_weight = selQry.get().FieldIndex("rk_weight");
        int col_bag_amount = selQry.get().FieldIndex("bag_amount");
        int col_bag_weight = selQry.get().FieldIndex("bag_weight");
        for(; not selQry.get().Eof; selQry.get().Next()) {

            insQry.get().SetVariable("pax_id", selQry.get().FieldAsInteger(col_pax_id));
            insQry.get().SetVariable("rk_weight", selQry.get().FieldAsInteger(col_rk_weight));
            insQry.get().SetVariable("bag_weight", selQry.get().FieldAsInteger(col_bag_weight));
            insQry.get().SetVariable("bag_amount", selQry.get().FieldAsInteger(col_bag_amount));
            insQry.get().SetVariable("segments", segListFromDB(selQry.get().FieldAsInteger(col_tckin_id)));

            insQry.get().Execute();
        }
    }
}

void get_service_stat(int point_id)
{
    TCachedQuery delQry("delete from service_stat where point_id = :point_id", QParams() << QParam("point_id", otInteger, point_id));
    delQry.get().Execute();
    TCachedQuery Qry(
            "select "
            "    pax.pax_id, "
            "    points.craft, "
            "    points.airp, "
            "    pro.rem_code, "
            "    pro.rem, "
            "    pro.user_id, "
            "    pro.desk "
            "from "
            "    points, "
            "    pax_grp, "
            "    pax, "
            "    pax_rem_origin pro "
            "where "
            "    points.point_id = :point_id and "
            "    pax_grp.point_dep = points.point_id and "
            "    pax_grp.grp_id = pax.grp_id and"
            "    pax.pax_id = pro.pax_id ",
            QParams() << QParam("point_id", otInteger, point_id)
            );
    TCachedQuery insQry(
            "insert into service_stat( "
            "   point_id, "
            "   travel_time, "
            "   rem_code, "
            "   ticket_no, "
            "   airp_last, "
            "   user_id, "
            "   desk, "
            "   rfisc, "
            "   rate, "
            "   rate_cur "
            ") values ( "
            "   :point_id, "
            "   :travel_time, "
            "   :rem_code, "
            "   :ticket_no, "
            "   :airp_last, "
            "   :user_id, "
            "   :desk, "
            "   :rfisc, "
            "   :rate, "
            "   :rate_cur "
            ") ",
            QParams()
            << QParam("point_id", otInteger, point_id)
            << QParam("travel_time", otDate)
            << QParam("rem_code", otString)
            << QParam("ticket_no", otString)
            << QParam("airp_last", otString)
            << QParam("user_id", otInteger)
            << QParam("desk", otString)
            << QParam("rfisc", otString)
            << QParam("rate", otFloat)
            << QParam("rate_cur", otString)
            );

    Qry.get().Execute();


    if(not Qry.get().Eof) {
        int col_pax_id = Qry.get().FieldIndex("pax_id");
        int col_craft = Qry.get().FieldIndex("craft");
        int col_airp = Qry.get().FieldIndex("airp");
        int col_rem_code = Qry.get().FieldIndex("rem_code");
        int col_rem = Qry.get().FieldIndex("rem");
        int col_user_id = Qry.get().FieldIndex("user_id");
        int col_desk = Qry.get().FieldIndex("desk");
        ServiceStat::TFltInfo flt_info;
        for(; not Qry.get().Eof; Qry.get().Next()) {
            int pax_id = Qry.get().FieldAsInteger(col_pax_id);
            string craft = Qry.get().FieldAsString(col_craft);
            string airp = Qry.get().FieldAsString(col_airp);

            ServiceStat::TFltInfo::TItems::iterator flt_info_idx = flt_info.get(point_id, craft, airp);

            CheckIn::TPaxTknItem tkn;
            LoadPaxTkn(pax_id, tkn);
            ostringstream ticket_no;
            if(not tkn.empty()) {
                ticket_no << tkn.no;
                if (tkn.coupon!=ASTRA::NoExists)
                    ticket_no << "/" << tkn.coupon;
            }

            if(flt_info_idx->second.travel_time == NoExists)
                insQry.get().SetVariable("travel_time", FNull);
            else
                insQry.get().SetVariable("travel_time", flt_info_idx->second.travel_time);
            insQry.get().SetVariable("rem_code", Qry.get().FieldAsString(col_rem_code));
            insQry.get().SetVariable("ticket_no", ticket_no.str());
            insQry.get().SetVariable("airp_last", flt_info_idx->second.airp_last);
            insQry.get().SetVariable("user_id", Qry.get().FieldAsInteger(col_user_id));
            insQry.get().SetVariable("desk", Qry.get().FieldAsString(col_desk));

            TRemInfo remInfo(Qry.get().FieldAsString(col_rem));

            insQry.get().SetVariable("rfisc", remInfo.rfisc);
            if(remInfo.rate == NoExists)
                insQry.get().SetVariable("rate", FNull);
            else
                insQry.get().SetVariable("rate", remInfo.rate);
            insQry.get().SetVariable("rate_cur", remInfo.rate_cur);

            insQry.get().Execute();
        }
    }
}

struct TServiceStatRow:public TOrderStatItem {
    int point_id;
    string ticket_no;
    TDateTime scd_out;
    int flt_no;
    string suffix;
    string airp;
    string airp_last;
    string craft;
    TDateTime travel_time;
    string rem_code;
    string airline;
    string user;
    string desk;
    string rfisc;
    double rate;
    string rate_cur;

    string rate_str() const;

    TServiceStatRow():
        point_id(NoExists),
        scd_out(NoExists),
        flt_no(NoExists),
        travel_time(NoExists),
        rate(NoExists)
    {}
    bool operator < (const TServiceStatRow &val) const
    {
        if(point_id == val.point_id)
            return rem_code < val.rem_code;
        return point_id < val.point_id;
    }

    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void TServiceStatRow::add_header(ostringstream &buf) const
{
    buf
        << "АП рег." << delim
        << "Стойка" << delim
        << "Агент" << delim
        << "Билет" << delim
        << "Дата" << delim
        << "Рейс" << delim
        << "От" << delim
        << "До" << delim
        << "Тип ВС" << delim
        << "Время в пути" << delim
        << "Код услуги" << delim
        << "RFISC" << delim
        << "Тариф" << delim
        << "Валюта"
        << endl;
}

void TServiceStatRow::add_data(ostringstream &buf) const
{
    buf
        // АП рег
        << ElemIdToCodeNative(etAirp, airp) << delim
        // Стойка
        << desk << delim
        // Агент
        << user << delim
        // Билет
        << ticket_no << delim
        // Дата
        << DateTimeToStr(scd_out, "dd.mm.yyyy") << delim;
    // Рейс
    ostringstream flt_no_str;
    flt_no_str << setw(3) << setfill('0') << flt_no << ElemIdToCodeNative(etSuffix, suffix);
    buf
        << flt_no_str.str() << delim
        // От
        << ElemIdToCodeNative(etAirp, airp) << delim
        // До
        << ElemIdToCodeNative(etAirp, airp_last) << delim
        // Тип ВС
        << ElemIdToCodeNative(etCraft, craft) << delim;
    // Время в пути
    if(travel_time != NoExists)
        buf << DateTimeToStr(travel_time, "hh:nn");
    buf << delim
        // Код услуги
        << rem_code << delim
        // RFISC
        << rfisc << delim
        // Тариф
        << rate_str() << delim
        // Валюта
        << ElemIdToCodeNative(etCurrency, rate_cur)
        << endl;
}

string TServiceStatRow::rate_str() const
{
    ostringstream result;
    if(rate != NoExists) {
        result << fixed << setprecision(2) << setfill('0') << rate;
    }
    return result.str();
}

//------------------------------ UnaccBagStat -----------------------------------------------
struct TUnaccBagStatRow:public TOrderStatItem {
    string craft;
    string airline;
    int flt_no;
    string suffix;
    TDateTime scd_out;
    string airp;
    string airp_arv;

    string original_tag_no;
    string surname;
    string name;
    string prev_airline;
    int prev_flt_no;
    string prev_suffix;
    TDateTime prev_scd;

    int grp_id;
    string descr;
    string desk;
    TDateTime time_create;
    int bag_type;
    int num;
    int amount;
    int weight;
    double no;

    string trfer_airline;
    string trfer_airp_dep;
    int trfer_flt_no;
    string trfer_suffix;
    TDateTime trfer_scd;
    string trfer_airp_arv;

    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;

    TUnaccBagStatRow():
        flt_no(NoExists),
        scd_out(NoExists),
        prev_flt_no(NoExists),
        prev_scd(NoExists),
        grp_id(NoExists),
        time_create(NoExists),
        bag_type(NoExists),
        num(NoExists),
        amount(NoExists),
        weight(NoExists),
        no(NoExists),
        trfer_flt_no(NoExists)
    {}
};

void TUnaccBagStatRow::add_data(ostringstream &buf) const
{
        // АП рег. багажа
        buf << ElemIdToCodeNative(etAirp, airp) << delim;
        //Стойка
        buf << desk << delim;
        //Агент
        buf << descr << delim;
        //Дата оформления
        buf << (time_create == NoExists ? "" : DateTimeToStr(time_create, "dd.mm.yyyy")) << delim;
        //AK
        buf << ElemIdToCodeNative(etAirline, airline) << delim;
        //Рейс
        ostringstream tmp_s;
        tmp_s << setw(3) << setfill('0') << flt_no << ElemIdToCodeNative(etSuffix, suffix);
        buf << tmp_s.str() << delim;
        //От
        buf << ElemIdToCodeNative(etAirp, airp) << delim;
        //До
        buf << ElemIdToCodeNative(etAirp, airp_arv) << delim;
        //Тип ВС
        buf << ElemIdToCodeNative(etCraft, craft) << delim;
        //Время в пути
        TDateTime time_travel = getTimeTravel(craft, airp, airp_arv);
        if(time_travel != NoExists)
            buf << DateTimeToStr(time_travel, "hh:nn") << delim;
        else
            buf << delim;
        //Трфр
        buf << getLocaleText((trfer_flt_no == NoExists ? "НЕТ" : "ДА")) << delim;
        if(trfer_flt_no == NoExists) {
            //АК трфр
            buf << delim;
            //Рейс трфр
            buf << delim;
            //От трфр
            buf << delim;
            //До трфр
            buf << delim;
        } else {
            //АК трфр
            buf << ElemIdToCodeNative(etAirline, trfer_airline) << delim;
            //Рейс трфр
            tmp_s.str("");
            tmp_s << setw(3) << setfill('0') << trfer_flt_no << ElemIdToCodeNative(etSuffix, trfer_suffix);
            buf << tmp_s.str() << delim;
            //От трфр
            buf << ElemIdToCodeNative(etAirp, trfer_airp_dep) << delim;
            //До трфр
            buf << ElemIdToCodeNative(etAirp, trfer_airp_arv) << delim;
        }
        //Тип багажа
        buf <<  ElemIdToNameLong(etBagType, bag_type) << delim;
        //№ баг. бирки
        tmp_s.str("");
        tmp_s << fixed << setprecision(0) << setw(10) << setfill('0') << no;
        buf << tmp_s.str() << delim;
        //ФИО пассажира
        tmp_s.str("");
        if(not surname.empty())
            tmp_s << surname;
        if(not name.empty()) {
            if(not tmp_s.str().empty())
                tmp_s << " ";
            tmp_s << name;
        }
        buf << tmp_s.str() << delim;
        //Первичный № бирки
        buf << original_tag_no << delim;
        //Первичная АК
        if(prev_airline.empty())
            buf << delim;
        else
            buf << ElemIdToCodeNative(etAirline, prev_airline) << delim;
        //Первичный рейс
        tmp_s.str("");
        if(prev_flt_no != NoExists)
            tmp_s << setw(3) << setfill('0') << prev_flt_no << ElemIdToCodeNative(etSuffix, prev_suffix);
        buf << tmp_s.str() << delim;
        //Дата вылета рейса
        if(prev_scd != NoExists)
            buf << DateTimeToStr(prev_scd, "dd.mm.yyyy");
        buf << endl;
}

void TUnaccBagStatRow::add_header(ostringstream &buf) const
{
    buf
        << "АП рег. багажа" << delim
        << "Стойка" << delim
        << "Агент" << delim
        << "Дата оформления" << delim
        << "АК" << delim
        << "Рейс" << delim
        << "От" << delim
        << "До" << delim
        << "Тип ВС" << delim
        << "Время в пути" << delim
        << "Трфр" << delim
        << "АК трфр" << delim
        << "Рейс трфр" << delim
        << "От трфр" << delim
        << "До трфр" << delim
        << "Тип багажа" << delim
        << "Бирка" << delim
        << "ФИО пассажира" << delim
        << "Первичная бирка" << delim
        << "Первичная АК" << delim
        << "Первичный рейс" << delim
        << "Дата вылета" << endl;
}

struct TUnaccBagStat {
    list<TUnaccBagStatRow> rows;
    void insert(const TUnaccBagStatRow &row);
};

void TUnaccBagStat::insert(const TUnaccBagStatRow &row)
{
    rows.push_back(row);
}

void TStatParams::AccessClause(string &SQLText) const
{
    if (!airps.elems().empty()) {
        if (airps.elems_permit())
            SQLText += " points.airp IN " + GetSQLEnum(airps.elems()) + "and ";
        else
            SQLText += " points.airp NOT IN " + GetSQLEnum(airps.elems()) + "and ";
    };
    if (!airlines.elems().empty()) {
        if (airlines.elems_permit())
            SQLText += " points.airline IN " + GetSQLEnum(airlines.elems()) + "and ";
        else
            SQLText += " points.airline NOT IN " + GetSQLEnum(airlines.elems()) + "and ";
    };
}

template <class T>
void RunUnaccBagStat(
        const TStatParams &params,
        T &UnaccBagStat,
        TPrintAirline &prn_airline
        )
{
    for(int pass = 0; pass <= 2; pass++) {
        QParams QryParams;
        QryParams
            << QParam("FirstDate", otDate, params.FirstDate)
            << QParam("LastDate", otDate, params.LastDate);
        if (pass!=0)
            QryParams << QParam("arx_trip_date_range", otInteger, ARX_TRIP_DATE_RANGE());
        string SQLText =
            "select \n"
            "   points.craft, \n"
            "   points.airline, \n"
            "   points.flt_no, \n"
            "   points.suffix, \n"
            "   points.scd_out, \n"
            "   points.airp, \n"
            "   pax_grp.airp_arv, \n"
            "   unaccomp.original_tag_no, \n"
            "   unaccomp.surname, \n"
            "   unaccomp.name, \n"
            "   unaccomp.airline prev_airline, \n"
            "   unaccomp.flt_no prev_flt_no, \n"
            "   unaccomp.suffix prev_suffix, \n"
            "   unaccomp.scd prev_scd, \n"
            "   pax_grp.grp_id, \n"
            "   users2.descr, \n"
            "   pax_grp.desk, \n"
            "   pax_grp.time_create, \n"
            "   bag2.bag_type, \n"
            "   bag2.num, \n"
            "   bag2.amount, \n"
            "   bag2.weight, \n"
            "   bag_tags.no, \n";
        if(pass != 0)
            SQLText +=
                "   transfer.airline trfer_airline, \n"
                "   transfer.airp_dep trfer_airp_dep, \n"
                "   transfer.flt_no trfer_flt_no, \n"
                "   transfer.suffix trfer_suffix, \n"
                "   transfer.scd trfer_scd, \n"
                "   transfer.airp_arv trfer_airp_arv \n";
        else
            SQLText +=
                "   trfer_trips.airline trfer_airline, \n"
                "   trfer_trips.airp_dep trfer_airp_dep, \n"
                "   trfer_trips.flt_no trfer_flt_no, \n"
                "   trfer_trips.suffix trfer_suffix, \n"
                "   trfer_trips.scd trfer_scd, \n"
                "   transfer.airp_arv trfer_airp_arv \n";
        SQLText +=
            "from \n";
        if(pass != 0) {
            SQLText +=
                "   arx_points points, \n"
                "   arx_pax_grp pax_grp, \n"
                "   arx_bag2 bag2, \n"
                "   arx_bag_tags bag_tags, \n"
                "   users2, \n"
                "   arx_unaccomp_bag_info unaccomp, \n"
                "   arx_transfer transfer \n";
            if(pass == 2)
                SQLText += ",(SELECT part_key, move_id FROM move_arx_ext \n"
                    "  WHERE part_key >= :LastDate+:arx_trip_date_range AND part_key <= :LastDate+date_range) arx_ext \n";
        } else
            SQLText +=
                "   points, \n"
                "   pax_grp, \n"
                "   bag2, \n"
                "   bag_tags, \n"
                "   users2, \n"
                "   unaccomp_bag_info unaccomp, \n"
                "   transfer, \n"
                "   trfer_trips \n";
        SQLText +=
            "where \n";
        if(pass != 0)
            SQLText +=
                "   bag2.part_key = bag_tags.part_key and \n"
                "   pax_grp.part_key = points.part_key and \n"
                "   bag2.part_key = pax_grp.part_key and \n"
                "   unaccomp.part_key(+) = bag2.part_key and \n" // не забываем про плюсики...
                "   pax_grp.part_key = transfer.part_key(+) and \n";
        if(pass == 1)
            SQLText += " points.part_key >= :FirstDate AND points.part_key < :LastDate + :arx_trip_date_range AND \n";
        if(pass == 2)
            SQLText += " points.part_key=arx_ext.part_key AND points.move_id=arx_ext.move_id AND \n";
        SQLText +=
            "   points.scd_out >= :FirstDate AND points.scd_out < :LastDate and \n"
            "   pax_grp.user_id = users2.user_id(+) and \n"
            "   bag2.grp_id = bag_tags.grp_id and \n"
            "   bag2.num = bag_tags.bag_num and \n";
        params.AccessClause(SQLText);
        if(params.flt_no != NoExists) {
            SQLText += " points.flt_no = :flt_no and \n";
            QryParams << QParam("flt_no", otInteger, params.flt_no);
        }
        SQLText +=
            "   pax_grp.point_dep = points.point_id and \n"
            "   pax_grp.class is null and \n"
            "   bag2.grp_id = pax_grp.grp_id and \n"
            "   unaccomp.grp_id(+) = bag2.grp_id and \n"
            "   unaccomp.num(+) = bag2.num and \n"
            "   pax_grp.grp_id = transfer.grp_id(+) and \n"
            "   transfer.pr_final(+) <> 0 \n";
        if(pass == 0)
            SQLText +=
                "   and transfer.point_id_trfer = trfer_trips.point_id(+) \n";
        SQLText +=
            "order by points.point_id, time_create \n";

        TCachedQuery Qry(SQLText, QryParams);
        Qry.get().Execute();
        if(not Qry.get().Eof) {
            TAirpArvInfo airp_arv_info;
            int col_craft = Qry.get().FieldIndex("craft");
            int col_airline = Qry.get().FieldIndex("airline");
            int col_flt_no = Qry.get().FieldIndex("flt_no");
            int col_suffix = Qry.get().FieldIndex("suffix");
            int col_scd_out = Qry.get().FieldIndex("scd_out");
            int col_airp = Qry.get().FieldIndex("airp");
            int col_original_tag_no = Qry.get().FieldIndex("original_tag_no");
            int col_surname = Qry.get().FieldIndex("surname");
            int col_name = Qry.get().FieldIndex("name");
            int col_prev_airline = Qry.get().FieldIndex("prev_airline");
            int col_prev_flt_no = Qry.get().FieldIndex("prev_flt_no");
            int col_prev_suffix = Qry.get().FieldIndex("prev_suffix");
            int col_prev_scd = Qry.get().FieldIndex("prev_scd");
            int col_grp_id = Qry.get().FieldIndex("grp_id");
            int col_descr = Qry.get().FieldIndex("descr");
            int col_desk = Qry.get().FieldIndex("desk");
            int col_time_create = Qry.get().FieldIndex("time_create");
            int col_bag_type = Qry.get().FieldIndex("bag_type");
            int col_num = Qry.get().FieldIndex("num");
            int col_amount = Qry.get().FieldIndex("amount");
            int col_weight = Qry.get().FieldIndex("weight");
            int col_no = Qry.get().FieldIndex("no");

            int col_trfer_airline = Qry.get().FieldIndex("trfer_airline");
            int col_trfer_airp_dep = Qry.get().FieldIndex("trfer_airp_dep");
            int col_trfer_flt_no = Qry.get().FieldIndex("trfer_flt_no");
            int col_trfer_suffix = Qry.get().FieldIndex("trfer_suffix");
            int col_trfer_scd = Qry.get().FieldIndex("trfer_scd");
            int col_trfer_airp_arv = Qry.get().FieldIndex("trfer_airp_arv");

            for(; not Qry.get().Eof; Qry.get().Next()) {
                prn_airline.check(Qry.get().FieldAsString(col_airline));
                TUnaccBagStatRow row;
                row.craft = Qry.get().FieldAsString(col_craft);
                row.airline = Qry.get().FieldAsString(col_airline);
                row.flt_no = Qry.get().FieldAsInteger(col_flt_no);
                row.suffix = Qry.get().FieldAsString(col_suffix);
                row.scd_out = Qry.get().FieldAsDateTime(col_scd_out);
                row.airp = Qry.get().FieldAsString(col_airp);
                row.airp_arv = airp_arv_info.get(Qry.get());
                row.original_tag_no = Qry.get().FieldAsString(col_original_tag_no);
                row.surname = Qry.get().FieldAsString(col_surname);
                row.name = Qry.get().FieldAsString(col_name);
                row.prev_airline = Qry.get().FieldAsString(col_prev_airline);
                if(not Qry.get().FieldIsNULL(col_prev_flt_no))
                    row.prev_flt_no = Qry.get().FieldAsInteger(col_prev_flt_no);
                row.prev_suffix = Qry.get().FieldAsString(col_prev_suffix);
                if(not Qry.get().FieldIsNULL(col_prev_scd))
                    row.prev_scd = Qry.get().FieldAsDateTime(col_prev_scd);
                row.grp_id = Qry.get().FieldAsInteger(col_grp_id);
                row.descr = Qry.get().FieldAsString(col_descr);
                row.desk = Qry.get().FieldAsString(col_desk);
                if(not Qry.get().FieldIsNULL(col_time_create))
                    row.time_create = Qry.get().FieldAsDateTime(col_time_create);
                if(not Qry.get().FieldIsNULL(col_bag_type))
                    row.bag_type = Qry.get().FieldAsInteger(col_bag_type);
                row.num = Qry.get().FieldAsInteger(col_num);
                row.amount = Qry.get().FieldAsInteger(col_amount);
                row.weight = Qry.get().FieldAsInteger(col_weight);
                row.no = Qry.get().FieldAsFloat(col_no);

                row.trfer_airline = Qry.get().FieldAsString(col_trfer_airline);
                row.trfer_airp_dep = Qry.get().FieldAsString(col_trfer_airp_dep);
                if(not Qry.get().FieldIsNULL(col_trfer_flt_no))
                    row.trfer_flt_no = Qry.get().FieldAsInteger(col_trfer_flt_no);
                row.trfer_suffix = Qry.get().FieldAsString(col_trfer_suffix);
                if(not Qry.get().FieldIsNULL(col_trfer_scd))
                    row.trfer_scd = Qry.get().FieldAsDateTime(col_trfer_scd);
                row.trfer_airp_arv = Qry.get().FieldAsString(col_trfer_airp_arv);

                UnaccBagStat.insert(row);
            }
        }
    }
}

void createXMLUnaccBagStat(
        const TStatParams &params,
        const TUnaccBagStat &UnaccBagStat,
        const TPrintAirline &prn_airline,
        xmlNodePtr resNode)
{
    if(UnaccBagStat.rows.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");
    if (UnaccBagStat.rows.size() > (size_t)MAX_STAT_ROWS()) throw MaxStatRowsException("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH", LParams() << LParam("num", MAX_STAT_ROWS()));
    NewTextChild(resNode, "airline", prn_airline.get(), "");
    xmlNodePtr grdNode = NewTextChild(resNode, "grd");

    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("АП рег. багажа"));
    SetProp(colNode, "width", 90);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Стойка"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Агент"));
    SetProp(colNode, "width", 80);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Дата оформления"));
    SetProp(colNode, "width", 100);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDate);
    colNode = NewTextChild(headerNode, "col", getLocaleText("АК"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Рейс"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("От"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("До"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Тип ВС"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Время в пути"));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Трфр"));
    SetProp(colNode, "width", 35);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("АК трфр"));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Рейс трфр"));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("От трфр"));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortFloat);
    colNode = NewTextChild(headerNode, "col", getLocaleText("До трфр"));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Тип багажа"));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Бирка"));
    SetProp(colNode, "width", 75);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("ФИО пассажира"));
    SetProp(colNode, "width", 85);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Первичная бирка"));
    SetProp(colNode, "width", 110);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Первичная АК"));
    SetProp(colNode, "width", 80);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Первичный рейс"));
    SetProp(colNode, "width", 90);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Дата вылета"));
    SetProp(colNode, "width", 105);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    for(list<TUnaccBagStatRow>::const_iterator i = UnaccBagStat.rows.begin(); i != UnaccBagStat.rows.end(); i++) {
        rowNode = NewTextChild(rowsNode, "row");
        // АП рег. багажа
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->airp));
        //Стойка
        NewTextChild(rowNode, "col", i->desk);
        //Агент
        NewTextChild(rowNode, "col", i->descr);
        //Дата оформления
        NewTextChild(rowNode, "col", (i->time_create == NoExists ? "" : DateTimeToStr(i->time_create, "dd.mm.yyyy")));
        //AK
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirline, i->airline));
        //Рейс
        ostringstream buf;
        buf << setw(3) << setfill('0') << i->flt_no << ElemIdToCodeNative(etSuffix, i->suffix);
        NewTextChild(rowNode, "col", buf.str());
        //От
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->airp));
        //До
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->airp_arv));
        //Тип ВС
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etCraft, i->craft));
        //Время в пути
        TDateTime time_travel = getTimeTravel(i->craft, i->airp, i->airp_arv);
        if(time_travel != NoExists)
            NewTextChild(rowNode, "col", DateTimeToStr(time_travel, "hh:nn"));
        else
            NewTextChild(rowNode, "col");
        //Трфр
        NewTextChild(rowNode, "col", getLocaleText((i->trfer_flt_no == NoExists ? "НЕТ" : "ДА")));
        if(i->trfer_flt_no == NoExists) {
            //АК трфр
            NewTextChild(rowNode, "col");
            //Рейс трфр
            NewTextChild(rowNode, "col");
            //От трфр
            NewTextChild(rowNode, "col");
            //До трфр
            NewTextChild(rowNode, "col");
        } else {
            //АК трфр
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirline, i->trfer_airline));
            //Рейс трфр
            buf.str("");
            buf << setw(3) << setfill('0') << i->trfer_flt_no << ElemIdToCodeNative(etSuffix, i->trfer_suffix);
            NewTextChild(rowNode, "col", buf.str());
            //От трфр
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->trfer_airp_dep));
            //До трфр
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->trfer_airp_arv));
        }
        //Тип багажа
        if(i->bag_type == NoExists)
            NewTextChild(rowNode, "col");
        else
            NewTextChild(rowNode, "col",  ElemIdToNameLong(etBagType, i->bag_type));
        //№ баг. бирки
        buf.str("");
        buf << fixed << setprecision(0) << setw(10) << setfill('0') << i->no;
        NewTextChild(rowNode, "col", buf.str());
        //ФИО пассажира
        buf.str("");
        if(not i->surname.empty())
            buf << i->surname;
        if(not i->name.empty()) {
            if(not buf.str().empty())
                buf << " ";
            buf << i->name;
        }
        NewTextChild(rowNode, "col", buf.str());
        //Первичный № бирки
        NewTextChild(rowNode, "col", i->original_tag_no);
        //Первичная АК
        if(i->prev_airline.empty())
            NewTextChild(rowNode, "col");
        else
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirline, i->prev_airline));
        //Первичный рейс
        buf.str("");
        if(i->prev_flt_no != NoExists)
            buf << setw(3) << setfill('0') << i->prev_flt_no << ElemIdToCodeNative(etSuffix, i->prev_suffix);
        NewTextChild(rowNode, "col", buf.str());
        //Дата вылета рейса
        if(i->prev_scd == NoExists)
            NewTextChild(rowNode, "col");
        else
            NewTextChild(rowNode, "col", DateTimeToStr(i->prev_scd, "dd.mm.yyyy"));
    }
    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("Несопр. багаж"));
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("Подробная"));
}

//------------------------------ UnaccBagStat end--------------------------------------------

struct TFlight {
    int point_id;
    string airline;
    string airp;
    int flt_no;
    string suffix;
    TDateTime scd_out;
    TFlight():
        flt_no(NoExists),
        scd_out(NoExists)
    {}
    bool operator < (const TFlight &val) const
    {
        return point_id < val.point_id;
    }

};

//---------------AnnulBT-----------------//

struct TAnnulBTStatRow {
    vector<t_tag_nos_row> tags;
    string airline;
    string airp;
    int flt_no;
    string suffix;
    int id;
    string airp_dep;
    string airp_arv;
    string full_name;
    int pax_id;
    int point_id;
    int bag_type;
    string rfisc;
    TDateTime time_create;
    TDateTime time_annul;
    int amount;
    int weight;
    int user_id;
    string agent;
    string trfer_airline;
    int trfer_flt_no;
    string trfer_suffix;
    TDateTime trfer_scd;
    string trfer_airp_arv;
    void get_tags(TDateTime part_key, int id);
    TAnnulBTStatRow():
        flt_no(NoExists),
        id(NoExists),
        pax_id(NoExists),
        point_id(NoExists),
        bag_type(NoExists),
        time_create(NoExists),
        time_annul(NoExists),
        amount(NoExists),
        weight(NoExists),
        user_id(NoExists),
        trfer_flt_no(NoExists),
        trfer_scd(NoExists)
    {}

};

void TAnnulBTStatRow::get_tags(TDateTime part_key, int id)
{
    QParams QryParams;
    QryParams << QParam("id", otInteger, id);

    string SQLText = "select no from ";
    if(part_key != NoExists)
        SQLText += "arx_annul_tags annul_tags ";
    else
        SQLText += "annul_tags ";
    SQLText += "where ";
    if(part_key != NoExists) {
        SQLText += " part_key = :part_key and ";
        QryParams << QParam("part_key", otDate, part_key);
    }
    SQLText += " id = :id order by no";
    TCachedQuery Qry(SQLText, QryParams);
    Qry.get().Execute();
    for(; not Qry.get().Eof; Qry.get().Next()) {
        t_tag_nos_row tag;
        tag.pr_liab_limit = false;
        tag.no = Qry.get().FieldAsFloat("no");
        tags.push_back(tag);
    }
}

struct TAnnulBTStat {
    list<TAnnulBTStatRow> rows;
};

void RunAnnulBTStat(
        const TStatParams &params,
        TAnnulBTStat &AnnulBTStat,
        TPrintAirline &prn_airline,
        int point_id = NoExists
        )
{
    map<int, string> agents;
    TCachedQuery agentQry("select descr from users2 where user_id = :user_id",
            QParams() << QParam("user_id", otInteger));

    int pass_count = (point_id == NoExists ? 2 : 0);
    for(int pass = 0; pass <= pass_count; pass++) {
        QParams QryParams;
        if(point_id == NoExists)
            QryParams
                << QParam("FirstDate", otDate, params.FirstDate)
                << QParam("LastDate", otDate, params.LastDate);
        else
            QryParams << QParam("point_id", otInteger, point_id);
        if (pass!=0)
            QryParams << QParam("arx_trip_date_range", otInteger, ARX_TRIP_DATE_RANGE());
        string SQLText =
            "select ";
        if(pass != 0)
            SQLText += "    points.part_key, ";
        else
            SQLText += "    null part_key, ";
        SQLText +=
            "   points.airline, "
            "   points.airp, "
            "   points.flt_no, "
            "   points.suffix, "
            "   annul_bag.id, "
            "   pax_grp.airp_dep, "
            "   pax_grp.airp_arv, "
            "   pax.surname||' '||pax.name full_name, "
            "   annul_bag.pax_id, "
            "   annul_bag.bag_type, "
            "   annul_bag.rfisc, "
            "   annul_bag.time_create, "
            "   annul_bag.time_annul, "
            "   annul_bag.amount, "
            "   annul_bag.weight, "
            "   annul_bag.user_id, ";
        if(pass != 0)
            SQLText +=
                "   transfer.airline trfer_airline, \n"
                "   transfer.airp_dep trfer_airp_dep, \n"
                "   transfer.flt_no trfer_flt_no, \n"
                "   transfer.suffix trfer_suffix, \n"
                "   transfer.scd trfer_scd, \n"
                "   transfer.airp_arv trfer_airp_arv \n";
        else
            SQLText +=
                "   trfer_trips.airline trfer_airline, \n"
                "   trfer_trips.airp_dep trfer_airp_dep, \n"
                "   trfer_trips.flt_no trfer_flt_no, \n"
                "   trfer_trips.suffix trfer_suffix, \n"
                "   trfer_trips.scd trfer_scd, \n"
                "   transfer.airp_arv trfer_airp_arv \n";
        SQLText +=
            "from ";
        if(pass != 0) {
            SQLText +=
                "   arx_pax_grp pax_grp, \n"
                "   arx_pax pax, \n"
                "   arx_annul_bag annul_bag,"
                "   arx_points points, \n "
                "   arx_transfer transfer \n";
            if(pass == 2)
                SQLText += ",(SELECT part_key, move_id FROM move_arx_ext \n"
                    "  WHERE part_key >= :LastDate+:arx_trip_date_range AND part_key <= :LastDate+date_range) arx_ext \n";
        } else {
            SQLText +=
                "   annul_bag, "
                "   pax_grp, \n"
                "   pax, \n"
                "   transfer, \n"
                "   trfer_trips, \n"
                "   points ";
        }
        SQLText += "where ";
        if(pass != 0)
            SQLText +=
                "   points.part_key = annul_bag.part_key and "
                "   pax_grp.part_key = points.part_key and \n"
                "   annul_bag.part_key = pax.part_key(+) and \n"
                "   pax_grp.part_key = transfer.part_key(+) and \n";
        if(pass == 1)
            SQLText += " points.part_key >= :FirstDate AND points.part_key < :LastDate + :arx_trip_date_range AND \n";
        if(pass == 2)
            SQLText += " points.part_key=arx_ext.part_key AND points.move_id=arx_ext.move_id AND \n";
        if(point_id == NoExists) {
            params.AccessClause(SQLText);
            SQLText +=
                "   points.scd_out >= :FirstDate AND points.scd_out < :LastDate and \n";
        } else
            SQLText +=
                "   points.point_id = :point_id and ";
        SQLText +=
            "   points.point_id = pax_grp.point_dep and "
            "   pax_grp.grp_id = annul_bag.grp_id and "
            "   annul_bag.pax_id = pax.pax_id(+) and "
            "   pax_grp.grp_id = transfer.grp_id(+) and \n"
            "   transfer.pr_final(+) <> 0 \n";
        if(pass == 0)
            SQLText +=
                "   and transfer.point_id_trfer = trfer_trips.point_id(+) \n";
        if(point_id == NoExists and params.flt_no != NoExists) {
            SQLText += " and points.flt_no = :flt_no \n";
            QryParams << QParam("flt_no", otInteger, params.flt_no);
        }
        TCachedQuery Qry(SQLText, QryParams);
        LogTrace(TRACE5) << Qry.get().SQLText.SQLText();
        Qry.get().Execute();
        if(not Qry.get().Eof) {
            int col_part_key = Qry.get().FieldIndex("part_key");
            int col_airline = Qry.get().FieldIndex("airline");
            int col_airp = Qry.get().FieldIndex("airp");
            int col_flt_no = Qry.get().FieldIndex("flt_no");
            int col_suffix = Qry.get().FieldIndex("suffix");
            int col_id = Qry.get().FieldIndex("id");
            int col_airp_dep = Qry.get().FieldIndex("airp_dep");
            int col_airp_arv = Qry.get().FieldIndex("airp_arv");
            int col_full_name = Qry.get().FieldIndex("full_name");
            int col_pax_id = Qry.get().FieldIndex("pax_id");
            int col_bag_type = Qry.get().FieldIndex("bag_type");
            int col_rfisc = Qry.get().FieldIndex("rfisc");
            int col_time_create = Qry.get().FieldIndex("time_create");
            int col_time_annul = Qry.get().FieldIndex("time_annul");
            int col_amount = Qry.get().FieldIndex("amount");
            int col_weight = Qry.get().FieldIndex("weight");
            int col_user_id = Qry.get().FieldIndex("user_id");
            int col_trfer_airline = Qry.get().FieldIndex("trfer_airline");
            int col_trfer_flt_no = Qry.get().FieldIndex("trfer_flt_no");
            int col_trfer_suffix = Qry.get().FieldIndex("trfer_suffix");
            int col_trfer_scd = Qry.get().FieldIndex("trfer_scd");
            int col_trfer_airp_arv = Qry.get().FieldIndex("trfer_airp_arv");

            for(; not Qry.get().Eof; Qry.get().Next()) {
                prn_airline.check(Qry.get().FieldAsString(col_airline));

                TDateTime part_key = NoExists;
                if(not Qry.get().FieldIsNULL(col_part_key))
                    part_key = Qry.get().FieldAsDateTime(col_part_key);

                TAnnulBTStatRow row;
                row.airline = Qry.get().FieldAsString(col_airline);
                row.airp = Qry.get().FieldAsString(col_airp);
                row.flt_no = Qry.get().FieldAsInteger(col_flt_no);
                row.suffix = Qry.get().FieldAsString(col_suffix);
                row.id = Qry.get().FieldAsInteger(col_id);
                row.airp_dep = Qry.get().FieldAsString(col_airp_dep);
                row.airp_arv = Qry.get().FieldAsString(col_airp_arv);
                if(not Qry.get().FieldIsNULL(col_pax_id)) {
                    row.pax_id = Qry.get().FieldAsInteger(col_pax_id);
                    row.full_name = Qry.get().FieldAsString(col_full_name);
                }
                if(not Qry.get().FieldIsNULL(col_bag_type))
                    row.bag_type = Qry.get().FieldAsInteger(col_bag_type);
                row.rfisc = Qry.get().FieldAsString(col_rfisc);
                if(not Qry.get().FieldIsNULL(col_time_create))
                    row.time_create = Qry.get().FieldAsDateTime(col_time_create);
                if(not Qry.get().FieldIsNULL(col_time_annul))
                    row.time_annul = Qry.get().FieldAsDateTime(col_time_annul);
                if(not Qry.get().FieldIsNULL(col_amount))
                    row.amount = Qry.get().FieldAsInteger(col_amount);
                if(not Qry.get().FieldIsNULL(col_weight))
                    row.weight = Qry.get().FieldAsInteger(col_weight);
                if(not Qry.get().FieldIsNULL(col_user_id))
                    row.user_id = Qry.get().FieldAsInteger(col_user_id);
                if(row.user_id != NoExists) {
                    map<int, string>::iterator agent = agents.find(row.user_id);
                    if(agent == agents.end()) {
                        agentQry.get().SetVariable("user_id", row.user_id);
                        agentQry.get().Execute();
                        string buf;
                        if(not agentQry.get().Eof)
                            buf = agentQry.get().FieldAsString("descr");
                        pair<map<int, string>::iterator, bool> ret =
                            agents.insert(make_pair(row.user_id, buf));
                        agent = ret.first;
                    }
                    row.agent = agent->second;
                }
                row.trfer_airline = Qry.get().FieldAsString(col_trfer_airline);
                if(not Qry.get().FieldIsNULL(col_trfer_flt_no))
                    row.trfer_flt_no = Qry.get().FieldAsInteger(col_trfer_flt_no);
                row.trfer_suffix = Qry.get().FieldAsString(col_trfer_suffix);
                if(not Qry.get().FieldIsNULL(col_trfer_scd))
                    row.trfer_scd = Qry.get().FieldAsDateTime(col_trfer_scd);
                row.trfer_airp_arv = Qry.get().FieldAsString(col_trfer_airp_arv);

                LogTrace(TRACE5) << "trfer_airp_arv: " << row.trfer_airp_arv;

                row.get_tags(part_key, row.id);
                AnnulBTStat.rows.push_back(row);
            }
        }
    }
}

void createXMLAnnulBTStat(
        const TStatParams &params,
        const TAnnulBTStat &AnnulBTStat,
        const TPrintAirline &prn_airline,
        xmlNodePtr resNode)
{
    if(AnnulBTStat.rows.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");
    if (AnnulBTStat.rows.size() > (size_t)MAX_STAT_ROWS())
        throw MaxStatRowsException("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH", LParams() << LParam("num", MAX_STAT_ROWS()));
    NewTextChild(resNode, "airline", prn_airline.get(), "");
    xmlNodePtr grdNode = NewTextChild(resNode, "grd");

    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("АК"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("АП"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Рейс"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Агент"));
    SetProp(colNode, "width", 100);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Пассажир"));
    SetProp(colNode, "width", 100);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("№№ баг. бирок"));
    SetProp(colNode, "width", 90);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("От"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("До"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("БГ мест"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("БГ вес"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Тип багажа/RFISC"));
    SetProp(colNode, "width", 100);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Дата выпуска"));
    SetProp(colNode, "width", 80);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Время выпуска"));
    SetProp(colNode, "width", 80);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Дата удаления"));
    SetProp(colNode, "width", 85);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Время удаления"));
    SetProp(colNode, "width", 85);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Трфр"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Рейс"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Дата"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    for(list<TAnnulBTStatRow>::const_iterator i = AnnulBTStat.rows.begin(); i != AnnulBTStat.rows.end(); i++) {
        rowNode = NewTextChild(rowsNode, "row");
        // АК
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirline, i->airline));
        // АП
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->airp));
        //Рейс
        ostringstream buf;
        buf << setw(3) << setfill('0') << i->flt_no << ElemIdToCodeNative(etSuffix, i->suffix);
        NewTextChild(rowNode, "col", buf.str());
        // Агент
        NewTextChild(rowNode, "col", i->agent);
        // Пассажир
        NewTextChild(rowNode, "col", transliter(i->full_name, 1, TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU));
        // №№ баг. бирок
        NewTextChild(rowNode, "col", get_tag_range(i->tags, LANG_EN));
        // От
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->airp_dep));
        // До
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->airp_arv));
        // Мест
        if(i->amount != NoExists)
            NewTextChild(rowNode, "col", i->amount);
        else
            NewTextChild(rowNode, "col");
        // Вес
        if(i->weight != NoExists)
            NewTextChild(rowNode, "col", i->weight);
        else
            NewTextChild(rowNode, "col");
        // Тип багажа/RFISC
        buf.str("");
        if(not i->rfisc.empty())
            buf << i->rfisc;
        else if(i->bag_type != NoExists)
            buf << ElemIdToNameLong(etBagType, i->bag_type);
        NewTextChild(rowNode, "col", buf.str());
        if(i->time_create != NoExists) {
            // Дата выпуска
            NewTextChild(rowNode, "col", DateTimeToStr(i->time_create, "dd.mm.yyyy"));
            // Время выпуска
            NewTextChild(rowNode, "col", DateTimeToStr(i->time_create, "hh:nn"));
        } else {
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
        }
        if(i->time_create != NoExists) {
            // Дата удаления
            NewTextChild(rowNode, "col", DateTimeToStr(i->time_annul, "dd.mm.yyyy"));
            // Время удаления
            NewTextChild(rowNode, "col", DateTimeToStr(i->time_annul, "hh:nn"));
        } else {
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
        }
        // Трфр
        if(i->trfer_airline.empty()) {
            NewTextChild(rowNode, "col", getLocaleText("НЕТ"));
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
        } else {
            // Трфр
            NewTextChild(rowNode, "col", getLocaleText("ДА"));
            //Рейс
            buf.str("");
            buf
                << ElemIdToCodeNative(etAirline, i->trfer_airline)
                << setw(3) << setfill('0') << i->trfer_flt_no << ElemIdToCodeNative(etSuffix, i->trfer_suffix);
            NewTextChild(rowNode, "col", buf.str());
            // Дата
            NewTextChild(rowNode, "col", DateTimeToStr(i->trfer_scd, "dd.mm.yyyy"));
        }
    }
    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("Аннул. бирки"));
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("Подробная"));
}

struct TAnnulBTStatCombo : public TOrderStatItem
{
    TAnnulBTStatRow data;
    TAnnulBTStatCombo(const TAnnulBTStatRow &aData): data(aData) {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void TAnnulBTStatCombo::add_header(ostringstream &buf) const
{
    buf
        << "АК" << delim
        << "АП" << delim
        << "Рейс" << delim
        << "Агент" << delim
        << "Пассажир" << delim
        << "№№ баг. бирок" << delim
        << "От" << delim
        << "До" << delim
        << "БГ мест" << delim
        << "БГ вес" << delim
        << "Тип багажа/RFISC" << delim
        << "Дата выпуска" << delim
        << "Время выпуска" << delim
        << "Дата удаления" << delim
        << "Время удаления" << delim
        << "Трфр" << delim
        << "Рейс" << delim
        << "Дата" << endl;
}

void TAnnulBTStatCombo::add_data(ostringstream &buf) const
{
        // АК
    buf <<  ElemIdToCodeNative(etAirline, data.airline) << delim
        // АП
        <<  ElemIdToCodeNative(etAirp, data.airp) << delim;
        //Рейс
    ostringstream oss1;
    oss1 << setw(3) << setfill('0') << data.flt_no
            << ElemIdToCodeNative(etSuffix, data.suffix);
    buf <<  oss1.str() << delim
        // Агент
        <<  data.agent << delim
        // Пассажир
        <<  transliter(data.full_name, 1, TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU) << delim
        // №№ баг. бирок
        <<  get_tag_range(data.tags, LANG_EN) << delim
        // От
        << ElemIdToCodeNative(etAirp, data.airp_dep) << delim
        // До
        <<  ElemIdToCodeNative(etAirp, data.airp_arv) << delim;
    // Мест
    if (data.amount != NoExists) buf << data.amount;
    buf << delim;
    // Вес
    if (data.weight != NoExists) buf << data.weight;
    buf << delim;
    // Тип багажа/RFISC
    oss1.str("");
    if (not data.rfisc.empty())
        oss1 << data.rfisc;
    else if (data.bag_type != NoExists)
        oss1 << ElemIdToNameLong(etBagType, data.bag_type);
    buf <<  oss1.str() << delim;
    if (data.time_create != NoExists)
    {
        // Дата выпуска
        buf <<  DateTimeToStr(data.time_create, "dd.mm.yyyy") << delim
        // Время выпуска
            <<  DateTimeToStr(data.time_create, "hh:nn") << delim;
    } else buf << delim << delim;
    if (data.time_create != NoExists)
    {
        // Дата удаления
        buf <<  DateTimeToStr(data.time_annul, "dd.mm.yyyy") << delim
        // Время удаления
            <<  DateTimeToStr(data.time_annul, "hh:nn") << delim;
    } else buf << delim << delim;
    // Трфр
    if (data.trfer_airline.empty())
    {
        buf << getLocaleText("НЕТ") << delim << delim;
    } else {
        // Трфр
        buf << getLocaleText("ДА") << delim;
        //Рейс
        oss1.str("");
        oss1 << ElemIdToCodeNative(etAirline, data.trfer_airline)
                << setw(3) << setfill('0') << data.trfer_flt_no
                << ElemIdToCodeNative(etSuffix, data.trfer_suffix);
        buf <<  oss1.str() << delim;
        // Дата
        buf << DateTimeToStr(data.trfer_scd, "dd.mm.yyyy");
    }
    buf << endl;
}

template <class T>
void RunAnnulBTStatFile(const TStatParams &params, T &writer, TPrintAirline &prn_airline)
{
    TAnnulBTStat AnnulBTStat;
    RunAnnulBTStat(params, AnnulBTStat, prn_airline);
    for (list<TAnnulBTStatRow>::const_iterator i = AnnulBTStat.rows.begin(); i != AnnulBTStat.rows.end(); ++i)
        writer.insert(TAnnulBTStatCombo(*i));
}

//---------------------------------------//
struct TLimitedCapabStat {
    typedef map<string, int> TRems;
    typedef map<string, TRems> TAirpArv;
    typedef map<TFlight, TAirpArv> TRows;
    TRems total;
    TRows rows;
};

void RunLimitedCapabStat(
        const TStatParams &params,
        TLimitedCapabStat &LimitedCapabStat,
        TPrintAirline &prn_airline
        )
{
    for(int pass = 0; pass <= 1; pass++) {
        QParams QryParams;
        QryParams
            << QParam("FirstDate", otDate, params.FirstDate)
            << QParam("LastDate", otDate, params.LastDate);
        string SQLText =
            "select "
            "   points.point_id, "
            "   points.airline, "
            "   points.airp, "
            "   stat.airp_arv, "
            "   points.flt_no, "
            "   points.suffix, "
            "   points.scd_out, "
            "   stat.rem_code, "
            "   stat.pax_amount "
            "from ";
        if(pass != 0) {
            SQLText +=
                "   arx_limited_capability_stat stat, "
                "   arx_points points ";
        } else {
            SQLText +=
                "   limited_capability_stat stat, "
                "   points ";
        }
        SQLText +=
            "where "
            "   stat.point_id = points.point_id and ";
        params.AccessClause(SQLText);
        if(params.flt_no != NoExists) {
            SQLText += " points.flt_no = :flt_no and ";
            QryParams << QParam("flt_no", otInteger, params.flt_no);
        }
        if(pass != 0)
            SQLText +=
                " points.part_key >= :FirstDate and points.part_key < :FirstDate and "
                " stat.part_key >= :FirstDate and stat.part_key < :LastDate ";
        else
            SQLText +=
                "    points.scd_out >= :FirstDate AND points.scd_out < :LastDate ";
        TCachedQuery Qry(SQLText, QryParams);
        Qry.get().Execute();
        if(not Qry.get().Eof) {
            int col_point_id = Qry.get().FieldIndex("point_id");
            int col_airline = Qry.get().FieldIndex("airline");
            int col_airp = Qry.get().FieldIndex("airp");
            int col_airp_arv = Qry.get().FieldIndex("airp_arv");
            int col_flt_no = Qry.get().FieldIndex("flt_no");
            int col_suffix = Qry.get().FieldIndex("suffix");
            int col_scd_out = Qry.get().FieldIndex("scd_out");
            int col_rem_code = Qry.get().FieldIndex("rem_code");
            int col_pax_amount = Qry.get().FieldIndex("pax_amount");
            for(; not Qry.get().Eof; Qry.get().Next()) {
                prn_airline.check(Qry.get().FieldAsString(col_airline));
                TFlight row;
                row.point_id = Qry.get().FieldAsInteger(col_point_id);
                row.airline = Qry.get().FieldAsString(col_airline);
                row.airp = Qry.get().FieldAsString(col_airp);
                row.flt_no = Qry.get().FieldAsInteger(col_flt_no);
                row.suffix = Qry.get().FieldAsString(col_suffix);
                row.scd_out = Qry.get().FieldAsDateTime(col_scd_out);

                string airp_arv = Qry.get().FieldAsString(col_airp_arv);
                string rem_code = Qry.get().FieldAsString(col_rem_code);
                int pax_amount = Qry.get().FieldAsInteger(col_pax_amount);


                LimitedCapabStat.total[rem_code] += pax_amount;
                LimitedCapabStat.rows[row][airp_arv][rem_code] = pax_amount;
            }
        }
    }
}

void createXMLLimitedCapabStat(const TStatParams &params, const TLimitedCapabStat &LimitedCapabStat, const TPrintAirline &prn_airline, xmlNodePtr resNode)
{
    if(LimitedCapabStat.rows.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");
    if (LimitedCapabStat.rows.size() >= (size_t)MAX_STAT_ROWS())
        throw MaxStatRowsException("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH", LParams() << LParam("num", MAX_STAT_ROWS()));
    NewTextChild(resNode, "airline", prn_airline.get(), "");

    xmlNodePtr grdNode = NewTextChild(resNode, "grd");
    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("АК"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("АП"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Рейс"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Дата"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDate);
    colNode = NewTextChild(headerNode, "col", getLocaleText("До"));
    SetProp(colNode, "width", 90);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    for(TLimitedCapabStat::TRems::const_iterator rem_col = LimitedCapabStat.total.begin();
            rem_col != LimitedCapabStat.total.end(); rem_col++)
    {
        colNode = NewTextChild(headerNode, "col", rem_col->first);
        SetProp(colNode, "width", 40);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
    }

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    for(TLimitedCapabStat::TRows::const_iterator row = LimitedCapabStat.rows.begin();
            row != LimitedCapabStat.rows.end(); row++)
    {
        for(TLimitedCapabStat::TAirpArv::const_iterator airp = row->second.begin();
                airp != row->second.end(); airp++)
        {
            rowNode = NewTextChild(rowsNode, "row");
            // АК
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirline, row->first.airline));
            // АП
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, row->first.airp));
            // Рейс
            ostringstream buf;
            buf << setw(3) << setfill('0') << row->first.flt_no << ElemIdToCodeNative(etSuffix, row->first.suffix);
            NewTextChild(rowNode, "col", buf.str());
            // Дата
            NewTextChild(rowNode, "col", DateTimeToStr(row->first.scd_out, "dd.mm.yyyy"));
            // До
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, airp->first));

            const TLimitedCapabStat::TRems &rems = airp->second;

            for(TLimitedCapabStat::TRems::const_iterator rem_col = LimitedCapabStat.total.begin();
                    rem_col != LimitedCapabStat.total.end(); rem_col++)
            {
                TLimitedCapabStat::TRems::const_iterator rems_idx = rems.find(rem_col->first);
                int pax_count = 0;
                if(rems_idx != rems.end()) pax_count = rems_idx->second;
                NewTextChild(rowNode, "col", pax_count);
            }
        }
    }
    rowNode = NewTextChild(rowsNode, "row");
    NewTextChild(rowNode, "col", getLocaleText("Итого:"));
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col");
    for(TLimitedCapabStat::TRems::const_iterator rem_col = LimitedCapabStat.total.begin();
            rem_col != LimitedCapabStat.total.end(); rem_col++)
        NewTextChild(rowNode, "col", rem_col->second);

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "rem_col_num", (int)LimitedCapabStat.total.size());
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("Пассажиры с ограниченными возможностями"));
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("Подробная"));
}

struct TLimitedCapabStatCombo : public TOrderStatItem
{
    typedef std::pair<TFlight, TLimitedCapabStat::TAirpArv> Tdata;
    Tdata data;
    TLimitedCapabStat::TRems total;
    TLimitedCapabStatCombo(const Tdata &aData, TLimitedCapabStat::TRems &aTotal)
        : data(aData), total(aTotal) {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void TLimitedCapabStatCombo::add_header(ostringstream &buf) const
{
    buf << "АК" << delim;
    buf << "АП" << delim;
    buf << "Рейс" << delim;
    buf << "Дата" << delim;
    buf << "До" << delim;
    for (TLimitedCapabStat::TRems::const_iterator rem_col = total.begin();;)
    {
        if (rem_col != total.end()) buf << rem_col->first;
        else { buf << endl; break; }
        if (++rem_col != total.end()) buf << delim;
        else { buf << endl; break; }
    }
}

void TLimitedCapabStatCombo::add_data(ostringstream &buf) const
{
    for(TLimitedCapabStat::TAirpArv::const_iterator airp = data.second.begin();
                airp != data.second.end(); airp++)
    {
        // АК
        buf << ElemIdToCodeNative(etAirline, data.first.airline) << delim;
        // АП
        buf << ElemIdToCodeNative(etAirp, data.first.airp) << delim;
        // Рейс
        ostringstream oss1;
        oss1 << setw(3) << setfill('0') << data.first.flt_no << ElemIdToCodeNative(etSuffix, data.first.suffix);
        buf << oss1.str() << delim;
        // Дата
        buf << DateTimeToStr(data.first.scd_out, "dd.mm.yyyy") << delim;
        // До
        buf << ElemIdToCodeNative(etAirp, airp->first) << delim;

        const TLimitedCapabStat::TRems &rems = airp->second;
        for(TLimitedCapabStat::TRems::const_iterator rem_col = total.begin(); rem_col != total.end();)
        {
            TLimitedCapabStat::TRems::const_iterator rems_idx = rems.find(rem_col->first);
            int pax_count = 0;
            if (rems_idx != rems.end()) pax_count = rems_idx->second;
            buf << pax_count;
            if (++rem_col != total.end()) buf << delim;
        }
        buf << endl;
    }
}

template <class T>
void RunLimitedCapabStatFile(const TStatParams &params, T &writer, TPrintAirline &prn_airline)
{
    TLimitedCapabStat LimitedCapabStat;
    RunLimitedCapabStat(params, LimitedCapabStat, prn_airline);
    for(TLimitedCapabStat::TRows::const_iterator row = LimitedCapabStat.rows.begin();
            row != LimitedCapabStat.rows.end(); row++)
        writer.insert(TLimitedCapabStatCombo(*row, LimitedCapabStat.total));
}

/********************************************************/

typedef multiset<TServiceStatRow> TServiceStat;

template <class T>
void RunServiceStat(
        const TStatParams &params,
        T &ServiceStat,
        TPrintAirline &prn_airline
        )
{
    for(int pass = 0; pass <= 1; pass++) {
        QParams QryParams;
        QryParams
            << QParam("FirstDate", otDate, params.FirstDate)
            << QParam("LastDate", otDate, params.LastDate);
        string SQLText =
            "select "
            "   points.point_id, "
            "   cs.ticket_no, "
            "   points.scd_out, "
            "   points.flt_no, "
            "   points.suffix, "
            "   points.airp, "
            "   cs.airp_last, "
            "   points.craft, "
            "   cs.travel_time, "
            "   cs.rem_code, "
            "   points.airline, "
            "   users2.descr, "
            "   cs.desk, "
            "   cs.rfisc, "
            "   cs.rate, "
            "   cs.rate_cur "
            "from ";
        if(pass != 0) {
            SQLText +=
                "   arx_service_stat cs, "
                "   arx_points points, ";
        } else {
            SQLText +=
                "   service_stat cs, "
                "   points, ";
        }
        SQLText +=
            "   users2 "
            "where "
            "   cs.point_id = points.point_id and ";
        params.AccessClause(SQLText);
        if(params.flt_no != NoExists) {
            SQLText += " points.flt_no = :flt_no and ";
            QryParams << QParam("flt_no", otInteger, params.flt_no);
        }
        if(pass != 0)
            SQLText +=
                " points.part_key >= :FirstDate and points.part_key < :FirstDate and "
                " cs.part_key >= :FirstDate and cs.part_key < :LastDate and ";
        else
            SQLText +=
                "    points.scd_out >= :FirstDate AND points.scd_out < :LastDate and ";
        SQLText +=
            "    cs.user_id = users2.user_id ";
        TCachedQuery Qry(SQLText, QryParams);
        Qry.get().Execute();
        if(not Qry.get().Eof) {
            int col_point_id = Qry.get().GetFieldIndex("point_id");
            int col_ticket_no = Qry.get().GetFieldIndex("ticket_no");
            int col_scd_out = Qry.get().GetFieldIndex("scd_out");
            int col_flt_no = Qry.get().GetFieldIndex("flt_no");
            int col_suffix = Qry.get().GetFieldIndex("suffix");
            int col_airp = Qry.get().GetFieldIndex("airp");
            int col_airp_last = Qry.get().GetFieldIndex("airp_last");
            int col_craft = Qry.get().GetFieldIndex("craft");
            int col_travel_time = Qry.get().GetFieldIndex("travel_time");
            int col_rem_code = Qry.get().GetFieldIndex("rem_code");
            int col_airline = Qry.get().GetFieldIndex("airline");
            int col_descr = Qry.get().GetFieldIndex("descr");
            int col_desk = Qry.get().GetFieldIndex("desk");
            int col_rfisc = Qry.get().GetFieldIndex("rfisc");
            int col_rate = Qry.get().GetFieldIndex("rate");
            int col_rate_cur = Qry.get().GetFieldIndex("rate_cur");
            for(; not Qry.get().Eof; Qry.get().Next()) {
                prn_airline.check(Qry.get().FieldAsString(col_airline));
                TServiceStatRow row;
                row.point_id = Qry.get().FieldAsInteger(col_point_id);
                row.ticket_no = Qry.get().FieldAsString(col_ticket_no);
                row.scd_out = Qry.get().FieldAsDateTime(col_scd_out);
                row.flt_no = Qry.get().FieldAsInteger(col_flt_no);
                row.suffix = Qry.get().FieldAsString(col_suffix);
                row.airp = Qry.get().FieldAsString(col_airp);
                row.airp_last = Qry.get().FieldAsString(col_airp_last);
                row.craft = Qry.get().FieldAsString(col_craft);
                if(not Qry.get().FieldIsNULL(col_travel_time))
                    row.travel_time = Qry.get().FieldAsDateTime(col_travel_time);
                row.rem_code = Qry.get().FieldAsString(col_rem_code);
                row.user = Qry.get().FieldAsString(col_descr);
                row.desk = Qry.get().FieldAsString(col_desk);
                row.rfisc = Qry.get().FieldAsString(col_rfisc);
                if(not Qry.get().FieldIsNULL(col_rate))
                    row.rate = Qry.get().FieldAsFloat(col_rate);
                row.rate_cur = Qry.get().FieldAsString(col_rate_cur);
                ServiceStat.insert(row);
            }
        }
    }
}

void createXMLServiceStat(const TStatParams &params, const TServiceStat &ServiceStat, const TPrintAirline &prn_airline, xmlNodePtr resNode)
{
    if(ServiceStat.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");
    if (ServiceStat.size() > (size_t)MAX_STAT_ROWS()) throw MaxStatRowsException("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH", LParams() << LParam("num", MAX_STAT_ROWS()));
    NewTextChild(resNode, "airline", prn_airline.get(), "");
    xmlNodePtr grdNode = NewTextChild(resNode, "grd");

    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("АП рег."));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Стойка"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Агент"));
    SetProp(colNode, "width", 80);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Билет"));
    SetProp(colNode, "width", 100);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Дата"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Рейс"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("От"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("До"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Тип ВС"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Время в пути"));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Код услуги"));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("RFISC"));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Тариф"));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortFloat);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Валюта"));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    ostringstream buf;
    for(TServiceStat::iterator i = ServiceStat.begin(); i != ServiceStat.end(); i++) {
        rowNode = NewTextChild(rowsNode, "row");
        // АП рег
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->airp));
        // Стойка
        NewTextChild(rowNode, "col", i->desk);
        // Агент
        NewTextChild(rowNode, "col", i->user);
        // Билет
        NewTextChild(rowNode, "col", i->ticket_no);
        // Дата
        NewTextChild(rowNode, "col", DateTimeToStr(i->scd_out, "dd.mm.yyyy"));
        // Рейс
        ostringstream buf;
        buf << setw(3) << setfill('0') << i->flt_no << ElemIdToCodeNative(etSuffix, i->suffix);
        NewTextChild(rowNode, "col", buf.str());
        // От
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->airp));
        // До
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->airp_last));
        // Тип ВС
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etCraft, i->craft));
        // Время в пути
        if(i->travel_time == NoExists)
            NewTextChild(rowNode, "col");
        else
            NewTextChild(rowNode, "col", DateTimeToStr(i->travel_time, "hh:nn"));
        // Код услуги
        NewTextChild(rowNode, "col", i->rem_code);
        // RFISC
        NewTextChild(rowNode, "col", i->rfisc);
        // Тариф
        NewTextChild(rowNode, "col", i->rate_str());
        // Валюта
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etCurrency, i->rate_cur));
    }

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("Багажные RFISC"));
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("Подробная"));
}

/******************* End of Service Stat ****************************/

struct TPeriods {
    typedef list<pair<TDateTime, TDateTime> > TItems;
    TItems items;
    void get(TDateTime FirstDate, TDateTime LastDate);
    void dump();
    void dump(TItems::iterator i);
};

void TPeriods::dump(TItems::iterator i)
{
    if(i == items.end())
        LogTrace(TRACE5) << "at the end.";
    else
        LogTrace(TRACE5)
            << DateTimeToStr(i->first, ServerFormatDateTimeAsString)
            << "-"
            << DateTimeToStr(i->second, ServerFormatDateTimeAsString);
}

void TPeriods::dump()
{
    for(TItems::iterator i = items.begin(); i != items.end(); i++) dump(i);
}

void TPeriods::get(TDateTime FirstDate, TDateTime LastDate)
{
    items.clear();
    TCachedQuery Qry("select trunc(:FirstDate, 'month'), add_months(trunc(:FirstDate, 'month'), 1) from dual",
            QParams() << QParam("FirstDate", otDate));
    TDateTime tmp_begin = FirstDate;
    while(true) {
        Qry.get().SetVariable("FirstDate", tmp_begin);
        Qry.get().Execute();
        TDateTime begin = Qry.get().FieldAsDateTime(0);
        TDateTime end = Qry.get().FieldAsDateTime(1);
        items.push_back(make_pair(begin, end));
        if(LastDate < end) break;
        tmp_begin = end;
    }
    items.front().first = FirstDate;
    if(items.back().first == LastDate)
        items.pop_back();
    else
        items.back().second = LastDate;
}

struct TFileParams {
    std::map<std::string,std::string> items;
    void get(int file_id);
    TFileParams() {}
    TFileParams(const map<string, string> &vitems): items(vitems) {}
    string get_name();
};

string get_part_file_name(int file_id, TDateTime month)
{
    // get part file name
    ostringstream file_name;
    file_name << ORDERS_PATH() << file_id << '.' << DateTimeToStr(month, "yymm");
    return file_name.str();
}

void commit_progress(TQuery &Qry, int parts, int size)
{
    Qry.SetVariable("progress", round((double)parts / size * 100));
    Qry.Execute();
    OraSession.Commit();
}

int GetCrc32(const string& my_string) {
    boost::crc_32_type result;
    result.process_bytes(my_string.data(), my_string.length());
    return result.checksum();
}

struct TErrCommit {
    TCachedQuery Qry;
    TErrCommit();
    static TErrCommit *Instance()
    {
        static TErrCommit *instance_ = 0;
        if(not instance_)
            instance_ = new TErrCommit();
        return instance_;
    }
    void exec(int file_id, TOrderStatus st, const string &err);
};

void TErrCommit::exec(int file_id, TOrderStatus st, const string &err)
{
    // Запускал processStatOrders из nosir, долго не мог понять, почему не работает.
    // А просто была ошибка вставки ошибки в базу.
    // Поэтому ошибка продублирована LogTrace-ом.
    LogTrace(TRACE5) << "TErrCommit::exec: " << err;
    Qry.get().SetVariable("file_id", file_id);
    Qry.get().SetVariable("status", st);
    Qry.get().SetVariable("error", err);
    Qry.get().Execute();
    OraSession.Commit();
}

TErrCommit::TErrCommit():
    Qry(
        "update stat_orders set "
        "   status = :status, "
        "   error = :error "
        "where "
        "   file_id = :file_id ",
        QParams()
        << QParam("file_id", otInteger)
        << QParam("status", otInteger)
        << QParam("error", otString)
       )
{
}

struct TStatOrderDataItem
{
    int file_id;
    TDateTime month;
    string file_name;
    double file_size;
    double file_size_zip;
    string md5_sum;

    void complete() const;

    void clear()
    {
        file_id = NoExists;
        month = NoExists;
        file_name.clear();
        file_size = NoExists;
        file_size_zip = NoExists;
        md5_sum.clear();
    }

    TStatOrderDataItem()
    {
        clear();
    }

};

void TStatOrderDataItem::complete() const
{
    TCachedQuery Qry(
            "update stat_orders_data set download_times = download_times + 1 where "
            "   file_id = :file_id and "
            "   month = :month",
            QParams()
            << QParam("file_id", otInteger, file_id)
            << QParam("month", otDate, month)
            );
    Qry.get().Execute();
}

typedef list<TStatOrderDataItem> TStatOrderData;

struct TStatOrder {
    int file_id;
    string name;
    int user_id;
    TDateTime time_ordered;
    TDateTime time_created;
    TOrderSource source;
    double data_size;
    double data_size_zip;
    TOrderStatus status;
    string error;
    int progress;

    TStatOrderData so_data;

    TStatOrder(int afile_id, int auser_id, TOrderSource asource):
        file_id(afile_id),
        user_id(auser_id),
        source(asource)
    { };
    TStatOrder() { clear(); }
    void clear()
    {
        file_id = NoExists;
        user_id = NoExists;
        time_ordered = NoExists;
        time_created = NoExists;
        source = osUnknown;
        data_size = NoExists;
        data_size_zip = NoExists;
        status = stUnknown;
        error.clear();
        progress = NoExists;
    }

    void del() const;
    void toDB();
    void fromDB(TCachedQuery &Qry);
    void get_parts();
    void check_integrity(TDateTime month);
};

void TStatOrder::del() const
{
    for(TStatOrderData::const_iterator i = so_data.begin(); i != so_data.end(); i++)
        remove( get_part_file_name(i->file_id, i->month).c_str());
    TCachedQuery delQry(
            "begin "
            "   delete from stat_orders_data where file_id = :file_id; "
            "   delete from stat_orders where file_id = :file_id; "
            "end; ", QParams() << QParam("file_id", otInteger, file_id)
            );
    delQry.get().Execute();
}

void TStatOrder::toDB()
{
    TCachedQuery Qry(
            "insert into stat_orders( "
            "   file_id, "
            "   user_id, "
            "   time_ordered, "
            "   source, "
            "   status, "
            "   progress "
            ") values ( "
            "   :file_id, "
            "   :user_id, "
            "   (select time from files where id = :file_id), "
            "   :os, "
            "   :status, "
            "   0 "
            ") ",
            QParams()
            << QParam("file_id", otInteger, file_id)
            << QParam("user_id", otInteger, user_id)
            << QParam("os", otString, EncodeOrderSource(source))
            << QParam("status", otInteger, stRunning)
            );
    Qry.get().Execute();
}

void TStatOrder::check_integrity(TDateTime month)
{
    if(status == stRunning or status == stError)
        return;

    try {
        for(TStatOrderData::iterator data_item = so_data.begin(); data_item != so_data.end(); data_item++) {
            if(month != NoExists and data_item->month < month) continue;
            string file_name = get_part_file_name(data_item->file_id, data_item->month);
            ifstream is(file_name.c_str(), ifstream::binary);
            if(is.is_open()) {
                is.seekg (0, is.end);
                int length = is.tellg();
                is.seekg (0, is.beg);

                boost::shared_array<char> data (new char[length]);
                is.read (data.get(), length);

                TMD5Sum::Instance()->init();
                TMD5Sum::Instance()->update(data.get(), length);
                TMD5Sum::Instance()->Final();

                if(data_item->md5_sum != TMD5Sum::Instance()->str())
                    throw UserException((string)"The order is corrupted! part_file_name: " + file_name);
            } else
                throw UserException((string)"file open error: " + file_name);
        }

        status = stReady;
        TCachedQuery readyQry("update stat_orders set status = :status, error = :error where file_id = :file_id",
                QParams()
                << QParam("file_id", otInteger, file_id)
                << QParam("status", otInteger, status)
                << QParam("error", otString, string())
                );
        readyQry.get().Execute();
    } catch (Exception &E) {
        status = stCorrupted;
        TErrCommit::Instance()->exec(file_id, status, string(E.what()).substr(0, 250).c_str());
    } catch(...) {
        status = stCorrupted;
        TErrCommit::Instance()->exec(file_id, status, "unknown");
    }
}

typedef map<TDateTime, TStatOrder> TStatOrderMap;

struct TStatOrders {

    TStatOrderMap items;

    void get(int file_id);
    void get(int user_id, int file_id, const string &source = string());
    void get(const string &source = string());
    TStatOrderData::const_iterator get_part(int file_id, TDateTime month);
    void toXML(xmlNodePtr resNode);
    bool so_data_empty(int file_id);
    void check_integrity(); // check integrity for each item
    double size(); // summary size of all orders in the list
    bool is_running(); // true if at least one order has stRunning state
};

bool TStatOrders::is_running()
{
    bool result = false;
    for(TStatOrderMap::iterator i = items.begin(); i != items.end(); i++)
        if(i->second.status == stRunning) {
            result = true;
            break;
        }
    return result;
}

double TStatOrders::size()
{
    double result = 0;
    for(TStatOrderMap::iterator i = items.begin(); i != items.end(); i++)
        if(i->second.data_size != NoExists)
            result += i->second.data_size;
    return result;
}

TStatOrderData::const_iterator TStatOrders::get_part(int file_id, TDateTime month)
{
    get(file_id);
    if(items.size() != 1)
        throw Exception("TStatOrders::get_part: file not found");
    TStatOrderData &so_data = items.begin()->second.so_data;
    TStatOrderData::const_iterator result = so_data.begin();
    for(; result != so_data.end(); result++) {
        if(result->month == month)
            break;
    }
    if(result == so_data.end())
        throw Exception("TStatOrders::get_part: part not found");
    items.begin()->second.check_integrity(month);
    if(items.begin()->second.status != stReady)
        throw UserException("MSG.STAT_ORDERS.FILE_TRANSFER_ERROR",
                LParams() << LParam("status", EncodeOrderStatus(items.begin()->second.status)));
    return result;
}

bool TStatOrders::so_data_empty(int file_id)
{
    bool result = items.empty();
    if(not result) {
        TStatOrderMap::iterator order = items.begin();
        for(; order != items.end(); order++) {
            if(order->second.file_id == file_id) {
                result = order->second.so_data.empty();
                break;
            }
        }
        if(order == items.end()) result = true;
    }
    return result;
}

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/device/file.hpp>

void seg_fault_emul()
{
    // seg fault emul
    char **a = NULL;
    char *b = a[0];
    LogTrace(TRACE5) << *b;

}

class TMD5Filter : public boost::iostreams::multichar_output_filter {
    public:
        template<typename Sink>
            std::streamsize write(Sink& dest, const char* s, std::streamsize n)
            {
                TMD5Sum::Instance()->update(s, n);
                boost::iostreams::copy(boost::iostreams::basic_array_source<char>(s, n), dest);
                return n;
            }

        /* Other members */
};

// TODO перенести выше (или в header), чтобы сделать функцию RunTlgOutStatFile не шаблонной
struct TOrderStatWriter {
    const string enc;
    int file_id;
    TDateTime month;
    string file_name;
    double data_size = 0;
    double data_size_zip = 0;
    boost::iostreams::filtering_ostream out;
    size_t rowcount;

    void push_back(const TOrderStatItem &row)
    {
        insert(row);
    }

    void insert(const TOrderStatItem &row);
    void finish();
    size_t size() { return 0; }

    TOrderStatWriter(int afile_id, TDateTime amonth, const string &afile_name):
        enc("CP1251"),
        file_id(afile_id),
        month(amonth),
        file_name(afile_name),
        rowcount(0)
    {}
};

void TOrderStatWriter::finish()
{
    if(rowcount == 0) return;
    // forces all the underlying buffers to be flushed, thus TMD5Sum updates correctly
    //
    // from boost manual:
    //  void reset()
    //  Clears the underlying chain. If the chain is initially complete,
    //  causes each Filter and Device in the chain to be closed using the function close.
    out.reset();
    TMD5Sum::Instance()->Final();
    data_size_zip = TMD5Sum::Instance()->data_size;
    TCachedQuery updDataQry(
            "update stat_orders_data set "
            "   file_size = :file_size, "
            "   file_size_zip = :file_size_zip, "
            "   md5_sum = :md5_sum "
            "where "
            "   file_id = :file_id and "
            "   month = :month ",
            QParams()
            << QParam("file_id", otInteger, file_id)
            << QParam("month", otDate, month)
            << QParam("file_size", otFloat, data_size)
            << QParam("file_size_zip", otFloat, data_size_zip)
            << QParam("md5_sum", otString, TMD5Sum::Instance()->str())
            );
    updDataQry.get().Execute();
}

void TOrderStatWriter::insert(const TOrderStatItem &row)
{
    if(rowcount == 0) { // первый инсерт, открываем файл
        TMD5Sum::Instance()->init();
        out.push(boost::iostreams::zlib_compressor(), ORDERS_BLOCK_SIZE());
        out.push(TMD5Filter(), ORDERS_BLOCK_SIZE());
        out.push(boost::iostreams::file_sink(get_part_file_name(file_id, month)), ORDERS_BLOCK_SIZE());

        TCachedQuery insDataQry(
                "begin "
                "   insert into stat_orders_data(file_id, month, file_name, download_times) values ( "
                "   :file_id, :month, :file_name, 0); "
                "exception "
                "   when dup_val_on_index then "
                "       update stat_orders_data set "
                "           file_name = :file_name "
                "       where "
                "           file_id = :file_id and "
                "           month = :month; "
                "end; "
                ,
                QParams() << QParam("file_id", otInteger, file_id)
                << QParam("month", otDate, month)
                << QParam("file_name", otString, file_name)
                );
        insDataQry.get().Execute();
        OraSession.Commit(); // чтобы сборщик мусора все видел и не удалял.

        ostringstream buf;
        row.add_header(buf);
        out << ConvertCodepage(buf.str(), "CP866", enc);
        data_size += buf.str().size();
    }
    ostringstream buf;
    row.add_data(buf);
    rowcount++;
    out << ConvertCodepage(buf.str(), "CP866", enc);
    data_size += buf.str().size();
    out.flush();
}

/*------------------------------- PFS STAT ---------------------------------------*/
struct TPFSStatRow {
    int point_id;
    int pax_id;
    TDateTime scd_out;
    string flt;
    string status;
    string route;
    int seats;
    string subcls;
    string pnr;
    string surname;
    string name;
    string gender;
    TDateTime birth_date;
    TPFSStatRow():
        point_id(NoExists),
        pax_id(NoExists),
        scd_out(NoExists),
        seats(NoExists),
        birth_date(NoExists)
    {}

    bool operator < (const TPFSStatRow &val) const
    {
        if(point_id == val.point_id)
            return pax_id < val.pax_id;
        return point_id < val.point_id;
    }
};

typedef multiset<TPFSStatRow> TPFSStat;

void RunPFSStat(
        const TStatParams &params,
        TPFSStat &PFSStat,
        TPrintAirline &prn_airline
        )
{
    for(int pass = 0; pass <= 2; pass++) {
        QParams QryParams;
        QryParams
            << QParam("FirstDate", otDate, params.FirstDate)
            << QParam("LastDate", otDate, params.LastDate);
        if (pass!=0)
            QryParams << QParam("arx_trip_date_range", otInteger, ARX_TRIP_DATE_RANGE());
        string SQLText =
            "select "
            "   points.point_id, "
            "   points.scd_out, "
            "   points.airline, "
            "   points.flt_no, "
            "   points.suffix, "
            "   points.airp, "
            "   pfs_stat.pax_id, "
            "   pfs_stat.status, "
            "   pfs_stat.airp_arv, "
            "   pfs_stat.seats, "
            "   pfs_stat.subcls, "
            "   pfs_stat.pnr, "
            "   pfs_stat.surname, "
            "   pfs_stat.name, "
            "   pfs_stat.gender, "
            "   pfs_stat.birth_date "
            "from ";
        if(pass != 0) {
            SQLText +=
                "   arx_pfs_stat pfs_stat, "
                "   arx_points points ";
            if(pass == 2)
                SQLText += ",(SELECT part_key, move_id FROM move_arx_ext \n"
                    "  WHERE part_key >= :LastDate+:arx_trip_date_range AND part_key <= :LastDate+date_range) arx_ext \n";
        } else {
            SQLText +=
                "   pfs_stat, "
                "   points ";
        }
        SQLText +=
            "where "
            "   pfs_stat.point_id = points.point_id and "
            "   points.pr_del >= 0 and ";
        params.AccessClause(SQLText);
        if(params.flt_no != NoExists) {
            SQLText += " points.flt_no = :flt_no and ";
            QryParams << QParam("flt_no", otInteger, params.flt_no);
        }
        if(pass != 0)
            SQLText +=
                " points.part_key = pfs_stat.part_key and ";
        if(pass == 1)
            SQLText += " points.part_key >= :FirstDate AND points.part_key < :LastDate + :arx_trip_date_range AND \n";
        if(pass == 2)
            SQLText += " points.part_key=arx_ext.part_key AND points.move_id=arx_ext.move_id AND \n";
        SQLText +=
            "    points.scd_out >= :FirstDate AND points.scd_out < :LastDate ";
        TCachedQuery Qry(SQLText, QryParams);
        Qry.get().Execute();
        if(not Qry.get().Eof) {
            int col_point_id = Qry.get().FieldIndex("point_id");
            int col_scd_out = Qry.get().FieldIndex("scd_out");
            int col_airline = Qry.get().FieldIndex("airline");
            int col_flt_no = Qry.get().FieldIndex("flt_no");
            int col_suffix = Qry.get().FieldIndex("suffix");
            // int col_airp = Qry.get().FieldIndex("airp");
            int col_pax_id = Qry.get().FieldIndex("pax_id");
            int col_status = Qry.get().FieldIndex("status");
            // int col_airp_arv = Qry.get().FieldIndex("airp_arv");
            int col_seats = Qry.get().FieldIndex("seats");
            int col_subcls = Qry.get().FieldIndex("subcls");
            int col_pnr = Qry.get().FieldIndex("pnr");
            int col_surname = Qry.get().FieldIndex("surname");
            int col_name = Qry.get().FieldIndex("name");
            int col_gender = Qry.get().FieldIndex("gender");
            int col_birth_date = Qry.get().FieldIndex("birth_date");
            for(; not Qry.get().Eof; Qry.get().Next()) {
                prn_airline.check(Qry.get().FieldAsString(col_airline));
                TPFSStatRow row;
                row.point_id = Qry.get().FieldAsInteger(col_point_id);
                row.pax_id = Qry.get().FieldAsInteger(col_pax_id);
                row.status = Qry.get().FieldAsString(col_status);
                row.scd_out = Qry.get().FieldAsDateTime(col_scd_out);
                ostringstream buf;
                buf
                    << ElemIdToCodeNative(etAirline, Qry.get().FieldAsString(col_airline))
                    << setw(3) << setfill('0') << Qry.get().FieldAsInteger(col_flt_no)
                    << ElemIdToCodeNative(etSuffix, Qry.get().FieldAsString(col_suffix));
                row.flt = buf.str();
                /*
                   buf.str("");
                   buf
                   << ElemIdToCodeNative(etAirp, Qry.get().FieldAsString(col_airp)) << "-"
                   << ElemIdToCodeNative(etAirp, Qry.get().FieldAsString(col_airp_arv));
                   row.route = buf.str();
                   */


                /*
                   route.set(GetRouteAfterStr( col_part_key>=0?Qry.FieldAsDateTime(col_part_key):NoExists,
                   Qry.FieldAsInteger(col_point_id),
                   trtNotCurrent,
                   trtNotCancelled),
                   */

                row.route = GetRouteAfterStr(NoExists, row.point_id, trtWithCurrent, trtNotCancelled);


                row.seats = Qry.get().FieldAsInteger(col_seats);
                row.subcls = ElemIdToCodeNative(etSubcls, Qry.get().FieldAsString(col_subcls));
                row.pnr = Qry.get().FieldAsString(col_pnr);
                row.surname = transliter(Qry.get().FieldAsString(col_surname), 1, TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU);
                row.name = transliter(Qry.get().FieldAsString(col_name), 1, TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU);

                row.gender = Qry.get().FieldAsString(col_gender);
                switch(CheckIn::is_female(row.gender, row.name)) {
                    case NoExists:
                        row.gender.clear();
                        break;
                    case 0:
                        row.gender = getLocaleText("CAP.DOC.MALE");
                        break;
                    case 1:
                        row.gender = getLocaleText("CAP.DOC.FEMALE");
                        break;
                }

                if(Qry.get().FieldIsNULL(col_birth_date))
                    row.birth_date = NoExists;
                else
                    row.birth_date = Qry.get().FieldAsDateTime(col_birth_date);
                PFSStat.insert(row);
            }
        }
    }
}

void createXMLPFSStat(
        const TStatParams &params,
        const TPFSStat &PFSStat,
        const TPrintAirline &prn_airline,
        xmlNodePtr resNode)
{
    if(PFSStat.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");
    if(PFSStat.size() > (size_t)MAX_STAT_ROWS()) throw MaxStatRowsException("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH", LParams() << LParam("num", MAX_STAT_ROWS()));
    NewTextChild(resNode, "airline", prn_airline.get(), "");
    xmlNodePtr grdNode = NewTextChild(resNode, "grd");

    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("Дата"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Рейс"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Маршрут"));
    SetProp(colNode, "width", 100);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Статус"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Мест"));
    SetProp(colNode, "width", 65);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("RBD"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("PNR"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Фамилия"));
    SetProp(colNode, "width", 80);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Имя"));
    SetProp(colNode, "width", 80);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Пол"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Рождение"));
    SetProp(colNode, "width", 85);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    for(TPFSStat::iterator i = PFSStat.begin(); i != PFSStat.end(); i++) {
        rowNode = NewTextChild(rowsNode, "row");
        // Дата
        NewTextChild(rowNode, "col", DateTimeToStr(i->scd_out, "dd.mm.yyyy"));
        // Рейс
        NewTextChild(rowNode, "col", i->flt);
        // Маршрут
        NewTextChild(rowNode, "col", i->route);
        // Статус
        NewTextChild(rowNode, "col", i->status);
        // Кол-во мест
        NewTextChild(rowNode, "col", i->seats);
        // RBD
        NewTextChild(rowNode, "col", i->subcls);
        // PNR
        NewTextChild(rowNode, "col", i->pnr);
        // Фамилия
        NewTextChild(rowNode, "col", i->surname);
        // Имя
        NewTextChild(rowNode, "col", i->name);
        // Пол
        NewTextChild(rowNode, "col", i->gender);
        // Дата рождения
        if(i->birth_date == NoExists)
            NewTextChild(rowNode, "col");
        else
            NewTextChild(rowNode, "col", DateTimeToStr(i->birth_date, "dd.mm.yyyy"));
    }

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", "PFS");
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("Подробная"));
}

typedef map<string, int> TPFSStatusMap;
typedef map<string, TPFSStatusMap> TPFSRouteMap;
typedef map<string, TPFSRouteMap> TPFSFltMap;
typedef map<TDateTime, TPFSFltMap> TPFSShortStat;

void RunPFSShortStat(
        const TStatParams &params,
        TPFSShortStat &PFSShortStat,
        TPrintAirline &prn_airline
        )
{
    TPFSStat pfs_stat;
    RunPFSStat(params, pfs_stat, prn_airline);
    for(TPFSStat::iterator i = pfs_stat.begin(); i != pfs_stat.end(); i++) {
        PFSShortStat[i->scd_out][i->flt][i->route][i->status]++;
    }
}

void createXMLPFSShortStat(
        const TStatParams &params,
        TPFSShortStat &PFSShortStat,
        const TPrintAirline &prn_airline,
        xmlNodePtr resNode)
{
    if(PFSShortStat.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");
    if(PFSShortStat.size() > (size_t)MAX_STAT_ROWS()) throw MaxStatRowsException("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH", LParams() << LParam("num", MAX_STAT_ROWS()));
    NewTextChild(resNode, "airline", prn_airline.get(), "");
    xmlNodePtr grdNode = NewTextChild(resNode, "grd");

    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("Дата"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Рейс"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Маршрут"));
    SetProp(colNode, "width", 100);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("NOREC"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("NOSHO"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("GOSHO"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    for(TPFSShortStat::iterator
            stat = PFSShortStat.begin();
            stat != PFSShortStat.end(); stat++)
        for(TPFSFltMap::iterator
                flt = stat->second.begin();
                flt != stat->second.end(); flt++)
            for(TPFSRouteMap::iterator
                    route = flt->second.begin();
                    route != flt->second.end(); route++)
            {
                rowNode = NewTextChild(rowsNode, "row");
                // Дата
                NewTextChild(rowNode, "col", DateTimeToStr(stat->first, "dd.mm.yyyy"));
                // Рейс
                NewTextChild(rowNode, "col", flt->first);
                // Маршрут
                NewTextChild(rowNode, "col", route->first);
                // NOREC
                NewTextChild(rowNode, "col", route->second["NOREC"]);
                // NOSHO
                NewTextChild(rowNode, "col", route->second["NOSHO"]);
                // GOSHO
                NewTextChild(rowNode, "col", route->second["GOSHO"]);
            }


    /*
    for(TPFSShortStat::iterator i = PFSShortStat.begin(); i != PFSShortStat.end(); i++) {
        rowNode = NewTextChild(rowsNode, "row");
        // Дата
        NewTextChild(rowNode, "col", DateTimeToStr(i->scd_out, "dd.mm.yyyy"));
        // Рейс
        NewTextChild(rowNode, "col", i->flt);
        // Маршрут
        NewTextChild(rowNode, "col", i->route);
        // Статус
        NewTextChild(rowNode, "col", i->status);
        // Кол-во мест
        NewTextChild(rowNode, "col", i->seats);
        // RBD
        NewTextChild(rowNode, "col", i->subcls);
        // PNR
        NewTextChild(rowNode, "col", i->pnr);
        // Фамилия
        NewTextChild(rowNode, "col", i->surname);
        // Имя
        NewTextChild(rowNode, "col", i->name);
        // Пол
        NewTextChild(rowNode, "col", i->gender);
        // Дата рождения
        if(i->birth_date == NoExists)
            NewTextChild(rowNode, "col");
        else
            NewTextChild(rowNode, "col", DateTimeToStr(i->birth_date, "dd.mm.yyyy"));
    }
    */

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", "PFS");
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("Общая"));
}

struct TPFSFullStatCombo : public TOrderStatItem
{
    const TPFSStatRow &row;
    TPFSFullStatCombo(const TPFSStatRow &_row): row(_row) {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void TPFSFullStatCombo::add_data(ostringstream &buf) const
{
        // Дата
        buf << DateTimeToStr(row.scd_out, "dd.mm.yyyy") << delim;
        // Рейс
        buf << row.flt << delim;
        // Маршрут
        buf << row.route << delim;
        // Статус
        buf << row.status << delim;
        // Кол-во мест
        buf << row.seats << delim;
        // RBD
        buf << row.subcls << delim;
        // PNR
        buf << row.pnr << delim;
        // Фамилия
        buf << row.surname << delim;
        // Имя
        buf << row.name << delim;
        // Пол
        buf << row.gender << delim;
        // Дата рождения
        buf << (row.birth_date == NoExists ? "" : DateTimeToStr(row.birth_date, "dd.mm.yyyy"));
        buf << endl;
}

void TPFSFullStatCombo::add_header(ostringstream &buf) const
{
    buf
        << "Дата" << delim
        << "Рейс" << delim
        << "Маршрут" << delim
        << "Статус" << delim
        << "Мест" << delim
        << "RBD" << delim
        << "PNR" << delim
        << "Фамилия" << delim
        << "Имя" << delim
        << "Пол" << delim
        << "Рождение"
        << endl;
}

template <class T>
void RunPFSFullFile(const TStatParams &params, T &writer, TPrintAirline &prn_airline)
{
    TPFSStat PFSStat;
    RunPFSStat(params, PFSStat, prn_airline);
    for(TPFSStat::iterator i = PFSStat.begin(); i != PFSStat.end(); i++)
        writer.insert(TPFSFullStatCombo(*i));
}

/*--------------------------------------------------------------------------------*/

template <class T>
void RunTrferPaxStat(
        const TStatParams &params,
        T &TrferPaxStat,
        TPrintAirline &prn_airline
        );

/* GRISHA */
void create_plain_files(
        const TStatParams &params,
        double &data_size,
        double &data_size_zip,
        TQueueItem &item
        )
{
    TFileParams file_params(item.params);
    // get file name
    string file_name =
        file_params.get_name() + "." +
        DateTimeToStr(item.time, "yymmddhhnn") + "." +
        DateTimeToStr(params.FirstDate, "yymm") + ".csv";

    TPrintAirline airline;
    TOrderStatWriter order_writer(item.id, params.FirstDate, file_name);
    switch(params.statType) {
        case statTrferPax:
            RunTrferPaxStat(params, order_writer, airline);
            break;
        case statRFISC:
            RunRFISCStat(params, order_writer, airline);
            break;
        case statService:
            RunServiceStat(params, order_writer, airline);
            break;
        case statUnaccBag:
            RunUnaccBagStat(params, order_writer, airline);
            break;
        case statTlgOutShort:
        case statTlgOutDetail:
        case statTlgOutFull:
            RunTlgOutStatFile(params, order_writer, airline);
            break;
        case statSelfCkinShort:
        case statSelfCkinDetail:
        case statSelfCkinFull:
            RunSelfCkinStatFile(params, order_writer, airline);
            break;
        case statAnnulBT:
            RunAnnulBTStatFile(params, order_writer, airline);
            break;
        case statFull:
        case statTrferFull:
            RunFullStatFile(params, order_writer, airline);
            break;
        case statLimitedCapab:
            RunLimitedCapabStatFile(params, order_writer, airline);
            break;
        case statAgentShort:
        case statAgentFull:
        case statAgentTotal:
            RunAgentStatFile(params, order_writer, airline);
            break;
        case statShort:
        case statDetail:
        case statPactShort:
            RunDetailStatFile(params, order_writer, airline);
            break;
        case statPFSFull:
            RunPFSFullFile(params, order_writer, airline);
            break;
            /*
        case statPFSShort:
            RunPFSShortFile(params, order_writer, airline);
            break;
            */
        default:
            throw Exception("unsupported statType %d", params.statType);
    }
    order_writer.finish();
    data_size += order_writer.data_size;
    data_size_zip += order_writer.data_size_zip;
}

void processStatOrders(TQueueItem &item) {
    TPerfTimer tm;
    tm.Init();
    try {
        TCachedQuery finishQry(
                "update stat_orders set "
                "   data_size = :data_size, "
                "   data_size_zip = :data_size_zip, "
                "   time_created = :tc, "
                "   status = :status "
                "where file_id = :file_id",
                QParams()
                << QParam("data_size", otFloat)
                << QParam("data_size_zip", otFloat)
                << QParam("tc", otDate)
                << QParam("file_id", otInteger, item.id)
                << QParam("status", otInteger)
                );
        TCachedQuery progressQry("update stat_orders set progress = :progress where file_id = :file_id",
                QParams()
                << QParam("file_id", otInteger, item.id)
                << QParam("progress", otInteger)
                );

        TStatParams params;
        params.fromFileParams(item.params);

        TReqInfo::Instance()->Initialize(params.desk_city);
        TReqInfo::Instance()->desk.lang = params.desk_lang;

        // По client_type = ctHTTP будем определять, что статистика формируется из заказа
        // В частности в статистике Саморегистрация
        TReqInfo::Instance()->client_type = ctHTTP;

        TPeriods periods;
        periods.get(params.FirstDate, params.LastDate);

        int parts = 0;
        double data_size = 0;
        double data_size_zip = 0;
        TPeriods::TItems::iterator i = periods.items.begin();

        // Возможно, в базе уже есть данные отчета (so_data не пустой)
        // тогда цикл надо начинать с последнего собранного куска
        // Иначе говоря, перематываем на текущее состояние сборки.
        TStatOrders so;
        so.get(item.id);
        if(not so.so_data_empty(item.id)) {
            const TStatOrderData &so_data = so.items.begin()->second.so_data;
            for(
                    TStatOrderData::const_iterator i_data = so_data.begin();
                    (i_data != so_data.end() and not i_data->md5_sum.empty());
                    i_data++
               ) {
                data_size += i_data->file_size;
                data_size_zip += i_data->file_size_zip;
                // отматываем периоды
                // 1. в so_data могут быть пропуски в месяцах, если в них не было данных
                // поэтому простой i++ не получится
                // 2. Дата периода не обязана быть первым днем месяца, в то время как
                // месяц отчета всегда первый день месяца
                while(true) {
                    if(DateTimeToStr(i->first, "mm.yy") == DateTimeToStr(i_data->month, "mm.yy"))
                        break;
                    i++;
                    parts++;
                }
            }
            // После перемотки итератор периода стоит на последнем успешном месяце
            // Надо его передвинуть на новый, еще не собранный месяц
            // как и счетчик parts, чтобы прогресс отображался правильно
            i++;
            parts++;
        }

        periods.dump(i);

        for(; i != periods.items.end();
                i++,
                commit_progress(progressQry.get(), ++parts, periods.items.size())
           ) {
            params.FirstDate = i->first;
            params.LastDate = i->second;

            create_plain_files(
                    params,
                    data_size,
                    data_size_zip,
                    item
                    );
        }
        finishQry.get().SetVariable("data_size", data_size);
        finishQry.get().SetVariable("data_size_zip", data_size_zip);
        finishQry.get().SetVariable("tc", NowUTC());
        finishQry.get().SetVariable("status", stReady);
        finishQry.get().Execute();
    } catch(Exception &E) {
        TErrCommit::Instance()->exec(item.id, stError, string(E.what()).substr(0, 250).c_str());
    } catch(...) {
        TErrCommit::Instance()->exec(item.id, stError, "unknown");
    }
     ProgTrace(TRACE5, "Stat Orders processing time: %s", tm.PrintWithMessage().c_str());
}

void stat_orders_synchro(void)
{
    TStatOrders so;
    so.get(NoExists);
    TDateTime time_out = NowUTC() - ORDERS_TIMEOUT();
    set<string> files;
    for(TStatOrderMap::iterator i = so.items.begin(); i != so.items.end(); i++) {
        const TStatOrder &so_item = i->second;
        if(so_item.status != stRunning and so_item.time_created <= time_out)
            so_item.del();
        else {
            for(TStatOrderData::const_iterator so_data = so_item.so_data.begin();
                    so_data != so_item.so_data.end();
                    so_data++) {
                files.insert(
                        get_part_file_name(
                            so_item.file_id,
                            so_data->month
                            )
                        );
            }
        }
    }

    namespace fs = boost::filesystem;
    fs::path so_path(ORDERS_PATH());
    fs::directory_iterator end_iter;
    for ( fs::directory_iterator dir_itr( so_path ); dir_itr != end_iter; ++dir_itr ) {
        if(files.find(dir_itr->path().c_str()) == files.end()) {
            fs::remove_all(dir_itr->path());
        }
    }
}

int nosir_synchro(int argc,char **argv)
{
    cout << DateTimeToStr(NowUTC(), "dd.mm.yyyy 00:00:00") << endl;
    return 1;
}


void stat_orders_collect(void)
{
    TFileQueue file_queue;
    file_queue.get( TFilterQueue( OWN_POINT_ADDR(), FILE_COLLECT_TYPE ) );
    for ( TFileQueue::iterator item=file_queue.begin();
            item!=file_queue.end();
            item++, OraSession.Commit() ) {
        try {
            switch(DecodeOrderSource(item->params[PARAM_ORDER_SOURCE])) {
                case osSTAT :
                    processStatOrders(*item);
                    break;
                default:
                    break;
            }
            TFileQueue::deleteFile(item->id);
        }
        catch(Exception &E) {
            OraSession.Rollback();
            try
            {
                EOracleError *orae=dynamic_cast<EOracleError*>(&E);
                if (orae!=NULL&&
                        (orae->Code==4061||orae->Code==4068)) continue;
                ProgError(STDLOG,"Exception: %s (file id=%d), SQLText: %s",E.what(),item->id, orae->SQLText());
            }
            catch(...) {};
        }
        catch(...) {
            OraSession.Rollback();
            ProgError(STDLOG, "Something goes wrong");
        }
    }
}

void orderStat(const TStatParams &params, XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TStatOrders so;
    so.get(NoExists); // list contains all orders in the system
    if(so.size() >= ORDERS_MAX_TOTAL_SIZE()) {
        NewTextChild(resNode, "collect_msg", getLocaleText("MSG.STAT_ORDERS.ORDERS_MAX_TOTAL_SIZE_EXCEEDED"));
    } else {
        so.get(); // default behaviour, orders list for current user
        if(so.is_running()) {
            NewTextChild(resNode, "collect_msg", getLocaleText("MSG.STAT_ORDERS.IS_RUNNING"));
        } else
            if(so.size() >= ORDERS_MAX_SIZE()) {
                NewTextChild(resNode, "collect_msg", getLocaleText("MSG.STAT_ORDERS.MAX_ORDERS_SIZE_EXCEEDED",
                            LParams() << LParam("max", getFileSizeStr(ORDERS_MAX_SIZE()))
                            ));
            } else {
                map<string, string> file_params;
                params.toFileParams(file_params);
                int file_id = TFileQueue::putFile(
                        OWN_POINT_ADDR(),
                        OWN_POINT_ADDR(),
                        FILE_COLLECT_TYPE,
                        file_params,
                        ""
                        );
                TStatOrder(file_id, TReqInfo::Instance()->user.user_id, osSTAT).toDB();
                NewTextChild(resNode, "collect_msg", getLocaleText("MSG.COLLECT_STAT_INFO"));
            }
    }
}

/***************** TrferPaxStat *********************/


struct TTrferPaxStatItem:public TOrderStatItem {
    string airline;
    string airp;
    int flt_no1;
    string suffix1;

    TDateTime date1;

    string trfer_airp;
    string airline2;
    int flt_no2;
    string suffix2;

    TDateTime date2;

    string airp_arv;
    TSegCategories::Enum seg_category;
    string pax_name;
    string pax_doc;
    int pax_amount;
    int adult;
    int child;
    int baby;

    int rk_weight;
    int bag_amount;
    int bag_weight;

    TTrferPaxStatItem()
    {
        clear();
    }

    void clear()
    {
        airline.clear();
        airp.clear();
        flt_no1 = NoExists;
        suffix1.clear();

        date1 = 0;

        trfer_airp.clear();
        flt_no2 = NoExists;
        suffix2.clear();

        date2 = 0;

        airp_arv.clear();
        seg_category = TSegCategories::Unknown;
        pax_name.clear();
        pax_doc.clear();
        pax_amount = 0;
        adult = 0;
        child = 0;
        baby = 0;

        rk_weight = 0;
        bag_amount = 0;
        bag_weight = 0;
    }

    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void TTrferPaxStatItem::add_header(ostringstream &buf) const
{
    buf
        << getLocaleText("АК") << delim
        << getLocaleText("АП") << delim
        << getLocaleText("Сег.1") << delim
        << getLocaleText("Дата") << delim
        << getLocaleText("Трансфер") << delim
        << getLocaleText("Сег.2") << delim
        << getLocaleText("Дата") << delim
        << getLocaleText("А/п прилета") << delim
        << getLocaleText("Категория") << delim
        << getLocaleText("ФИО пассажира") << delim
        << getLocaleText("Документ") << delim
        << getLocaleText("Кол-во пасс.") << delim
        << getLocaleText("ВЗ") << delim
        << getLocaleText("РБ") << delim
        << getLocaleText("РМ") << delim
        << getLocaleText("Р/кладь (вес)") << delim
        << getLocaleText("БГ мест") << delim
        << getLocaleText("БГ вес") << endl;
}

void TTrferPaxStatItem::add_data(ostringstream &buf) const
{
    if(airline.empty()) {
        buf
            << getLocaleText("Итого:") << delim
            << delim
            << delim
            << delim
            << delim
            << delim
            << delim
            << delim
            << delim
            << delim
            << delim;
    } else {
        // АК
        buf << ElemIdToCodeNative(etAirline, airline) << delim;
        // АП
        buf << ElemIdToCodeNative(etAirp, airp) << delim;
        //Сег.1
        ostringstream tmp;
        tmp << setw(3) << setfill('0') << flt_no1 << ElemIdToCodeNative(etSuffix, suffix1);
        buf << tmp.str() << delim;
        //Дата
        buf << DateTimeToStr(date1, "dd.mm.yyyy") << delim;
        //Трансфер
        buf << ElemIdToCodeNative(etAirp, trfer_airp) << delim;
        //Сег.2
        tmp.str("");
        tmp
            << ElemIdToCodeNative(etAirline, airline2)
            << setw(3) << setfill('0') << flt_no2 << ElemIdToCodeNative(etSuffix, suffix2);
        buf << tmp.str() << delim;
        //Дата
        buf << DateTimeToStr(date2, "dd.mm.yyyy") << delim;
        //А/п прилета
        buf << ElemIdToCodeNative(etAirp, airp_arv) << delim;
        //Категория
        buf << getLocaleText(TSegCategory().encode(seg_category)) << delim;
        //ФИО
        buf << pax_name << delim;
        //Паспорт
        buf << pax_doc << delim;
    }
    //Кол-во пасс.
    buf << pax_amount << delim;
    //ВЗ
    buf << adult << delim;
    //РБ
    buf << child << delim;
    //РМ
    buf << baby << delim;
    //Р/кладь(вес)
    buf << rk_weight << delim;
    //БГ мест
    buf << bag_amount << delim;
    //БГ вес
    buf << bag_weight << endl;
}

typedef list<TTrferPaxStatItem> TTrferPaxStat;

void getSegList(const string &segments, list<pair<TTripInfo, string> > &seg_list)
{
    vector<string> tokens;
    boost::split(tokens, segments, boost::is_any_of(";"));
    for(vector<string>::iterator i = tokens.begin();
            i != tokens.end(); i++) {
        vector<string> flt_info;
        boost::split(flt_info, *i, boost::is_any_of(","));
        TTripInfo flt;
        flt.airline = flt_info[0];
        flt.airp = flt_info[1];
        flt.flt_no = ToInt(flt_info[2]);
        flt.suffix = flt_info[3];
        StrToDateTime(flt_info[4].c_str(), ServerFormatDateTimeAsString, flt.scd_out);

        string airp_arv;
        if(flt_info.size() > 5)
            airp_arv = flt_info[5];
        seg_list.push_back(make_pair(flt, airp_arv));
    }
}

template <class T>
void RunTrferPaxStat(
        const TStatParams &params,
        T &TrferPaxStat,
        TPrintAirline &prn_airline
        )
{
    QParams QryParams;
    QryParams
        << QParam("FirstDate", otDate, params.FirstDate)
        << QParam("LastDate", otDate, params.LastDate);
    TTrferPaxStatItem totals;
    for(int pass = 0; pass <= 2; pass++) {
        if (pass!=0)
            QryParams << QParam("arx_trip_date_range", otInteger, ARX_TRIP_DATE_RANGE());
        string SQLText =
            "select ";
        if(pass)
            SQLText += "points.part_key, ";
        else
            SQLText += "null part_key, ";
        SQLText +=
            "   trfer_pax_stat.*, "
            "   pax.surname||' '||pax.name full_name, "
            "   pax.pers_type "
            "from ";

        if (pass!=0)
        {
            SQLText +=
            "   arx_trfer_pax_stat trfer_pax_stat, "
            "   arx_pax pax, "
            "   arx_points points ";
            if (pass==2)
                SQLText += ",(SELECT part_key, move_id FROM move_arx_ext \n"
                    "  WHERE part_key >= :LastDate+:arx_trip_date_range AND part_key <= :LastDate+date_range) arx_ext \n";
        }
        else
            SQLText +=
            "   trfer_pax_stat, "
            "   pax, "
            "   points ";

        SQLText +=
            "where ";
        if(pass != 0)
            SQLText +=
                "   points.part_key = trfer_pax_stat.part_key and "
                "   pax.part_key = trfer_pax_stat.part_key and ";

        if (pass==1)
            SQLText += " points.part_key >= :FirstDate AND points.part_key < :LastDate + :arx_trip_date_range AND \n";
        if (pass==2)
            SQLText += " points.part_key=arx_ext.part_key AND points.move_id=arx_ext.move_id AND \n";
        SQLText +=
            "   trfer_pax_stat.point_id = points.point_id and ";
        params.AccessClause(SQLText);
        SQLText +=
            "   trfer_pax_stat.scd_out>=:FirstDate AND trfer_pax_stat.scd_out<:LastDate and "
            "   trfer_pax_stat.pax_id = pax.pax_id ";
        TCachedQuery Qry(SQLText, QryParams);
        Qry.get().Execute();
        if(not Qry.get().Eof) {
            int col_part_key = Qry.get().FieldIndex("part_key");
            int col_point_id = Qry.get().FieldIndex("point_id");
            int col_scd_out = Qry.get().FieldIndex("scd_out");
            int col_pax_id = Qry.get().FieldIndex("pax_id");
            int col_rk_weight = Qry.get().FieldIndex("rk_weight");
            int col_bag_weight = Qry.get().FieldIndex("bag_weight");
            int col_bag_amount = Qry.get().FieldIndex("bag_amount");
            int col_segments = Qry.get().FieldIndex("segments");
            int col_full_name = Qry.get().FieldIndex("full_name");
            int col_pers_type = Qry.get().FieldIndex("pers_type");
            for(; not Qry.get().Eof; Qry.get().Next()) {
                TDateTime part_key = NoExists;
                if(not Qry.get().FieldIsNULL(col_part_key))
                    part_key = Qry.get().FieldAsDateTime(col_part_key);
                int pax_id = Qry.get().FieldAsInteger(col_pax_id);
                int rk_weight = Qry.get().FieldAsInteger(col_rk_weight);
                int bag_weight = Qry.get().FieldAsInteger(col_bag_weight);
                int bag_amount = Qry.get().FieldAsInteger(col_bag_amount);
                string segments = Qry.get().FieldAsString(col_segments);
                string full_name = Qry.get().FieldAsString(col_full_name);
                string pers_type = Qry.get().FieldAsString(col_pers_type);

                list<pair<TTripInfo, string> > seg_list;
                getSegList(segments, seg_list);

                TTrferPaxStat tmp_stat;
                TTrferPaxStatItem item;
                for(list<pair<TTripInfo, string> >::iterator flt = seg_list.begin();
                        flt != seg_list.end(); flt++) {
                    if(item.airline.empty()) {
                        prn_airline.check(flt->first.airline);
                        item.airline = flt->first.airline;
                        item.airp = flt->first.airp;
                        item.flt_no1 = flt->first.flt_no;
                        item.suffix1 = flt->first.suffix;
                        item.date1 = flt->first.scd_out;
                    } else {
                        item.trfer_airp = flt->first.airp;
                        item.airline2 = flt->first.airline;
                        item.flt_no2 = flt->first.flt_no;
                        item.suffix2 = flt->first.suffix;
                        item.date2 = flt->first.scd_out;
                        item.airp_arv = flt->second;
                        item.pax_name = transliter(full_name, 1, TReqInfo::Instance()->desk.lang != AstraLocale::LANG_RU);
                        item.pax_doc = CheckIn::GetPaxDocStr(part_key, pax_id, false);

                        typedef map<bool, TSegCategories::Enum> TSeg2Map;
                        typedef map<bool, TSeg2Map> TCategoryMap;

                        static TCategoryMap category_map =
                        {
                            {false, {{false, TSegCategories::IntInt}}},
                            {false, {{true,  TSegCategories::IntFor}}},
                            {true,  {{false, TSegCategories::ForInt}}},
                            {true,  {{true,  TSegCategories::ForFor}}}
                        };

                        string country1 = get_airp_country(item.airp);
                        string country2 = get_airp_country(item.trfer_airp);
                        string country3 = get_airp_country(item.airp_arv);
                        bool is_inter1 = country1 != country2;
                        bool is_inter2 = country2 != country3;
                        item.seg_category = category_map[is_inter1][is_inter2];

                        TSegCategories::Enum seg_category = TSegCategories::Unknown;
                        if(params.seg_category != TSegCategories::Unknown) {
                            seg_category = item.seg_category;
                        }

                        if(
                                params.seg_category == seg_category and
                                (params.trfer_airp.empty() or params.trfer_airp == item.trfer_airp) and
                                (params.trfer_airline.empty() or params.trfer_airline == item.airline)
                          )
                            tmp_stat.push_back(item);

                        item.clear();
                        item.airline = flt->first.airline;
                        item.airp = flt->first.airp;
                        item.flt_no1 = flt->first.flt_no;
                        item.suffix1 = flt->first.suffix;
                        item.date1 = flt->first.scd_out;
                    }
                }

                if(tmp_stat.begin() != tmp_stat.end()) {
                    tmp_stat.begin()->pax_amount = 1;
                    tmp_stat.begin()->adult = pers_type == "ВЗ";
                    tmp_stat.begin()->child = pers_type == "РБ";
                    tmp_stat.begin()->baby = pers_type == "РМ";
                    tmp_stat.begin()->rk_weight = rk_weight;
                    tmp_stat.begin()->bag_amount = bag_amount;
                    tmp_stat.begin()->bag_weight = bag_weight;

                    totals.pax_amount++;
                    totals.adult += tmp_stat.begin()->adult;
                    totals.child += tmp_stat.begin()->child;
                    totals.baby += tmp_stat.begin()->baby;
                    totals.rk_weight += rk_weight;
                    totals.bag_amount += bag_amount;
                    totals.bag_weight += bag_weight;
                }

                for(TTrferPaxStat::iterator i = tmp_stat.begin();
                        i != tmp_stat.end(); i++) TrferPaxStat.push_back(*i);

                if(
                        not is_same<T, TOrderStatWriter>::value and
                        TrferPaxStat.size() > (size_t)MAX_STAT_ROWS()
                  ) throw MaxStatRowsException("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH", LParams() << LParam("num", MAX_STAT_ROWS()));
            }
        }
    }
    if(totals.pax_amount != 0)
        TrferPaxStat.push_back(totals);
}

void createXMLTrferPaxStat(
        const TStatParams &params,
        TTrferPaxStat &TrferPaxStat,
        const TPrintAirline &prn_airline,
        xmlNodePtr resNode)
{
    if(TrferPaxStat.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");
    NewTextChild(resNode, "airline", prn_airline.get(), "");

    xmlNodePtr grdNode = NewTextChild(resNode, "grd");

    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("АК"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("АП"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Сег.1"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Дата"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Трансфер"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Сег.2"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Дата"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("А/п прилета"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Категория"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("ФИО пассажира"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Документ"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Кол-во пасс."));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("ВЗ"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("РБ"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("РМ"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Р/кладь (вес)"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("БГ мест"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("БГ вес"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;

    for(TTrferPaxStat::iterator stat = TrferPaxStat.begin();
            stat != TrferPaxStat.end(); stat++) {
        rowNode = NewTextChild(rowsNode, "row");
        if(stat->airline.empty()) {
            NewTextChild(rowNode, "col", getLocaleText("Итого:"));
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
        } else {
            // АК
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirline, stat->airline));
            // АП
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, stat->airp));
            //Сег.1
            ostringstream buf;
            buf << setw(3) << setfill('0') << stat->flt_no1 << ElemIdToCodeNative(etSuffix, stat->suffix1);
            NewTextChild(rowNode, "col", buf.str());
            //Дата
            NewTextChild(rowNode, "col", DateTimeToStr(stat->date1, "dd.mm.yyyy"));
            //Трансфер
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, stat->trfer_airp));
            //Сег.2
            buf.str("");
            buf
                << ElemIdToCodeNative(etAirline, stat->airline2)
                << setw(3) << setfill('0') << stat->flt_no2 << ElemIdToCodeNative(etSuffix, stat->suffix2);
            NewTextChild(rowNode, "col", buf.str());
            //Дата
            NewTextChild(rowNode, "col", DateTimeToStr(stat->date2, "dd.mm.yyyy"));
            //А/п прилета
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, stat->airp_arv));
            //Категория
            NewTextChild(rowNode, "col", getLocaleText(TSegCategory().encode(stat->seg_category)));
            //ФИО
            NewTextChild(rowNode, "col", stat->pax_name);
            //Паспорт
            NewTextChild(rowNode, "col", stat->pax_doc);
        }
        //Кол-во пасс.
        NewTextChild(rowNode, "col", stat->pax_amount);
        //ВЗ
        NewTextChild(rowNode, "col", stat->adult);
        //РБ
        NewTextChild(rowNode, "col", stat->child);
        //РМ
        NewTextChild(rowNode, "col", stat->baby);
        //Р/кладь(вес)
        NewTextChild(rowNode, "col", stat->rk_weight);
        //БГ мест
        NewTextChild(rowNode, "col", stat->bag_amount);
        //БГ вес
        NewTextChild(rowNode, "col", stat->bag_weight);
    }

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("Трансфер"));
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("Подробная"));
}


/***************** end of TrferPaxStat *********************/


void StatInterface::RunStat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqInfo = TReqInfo::Instance();
    if (!reqInfo->user.access.rights().permitted(600))
        throw AstraLocale::UserException("MSG.INSUFFICIENT_RIGHTS.NOT_ACCESS");

    if (reqInfo->user.access.totally_not_permitted())
        throw AstraLocale::UserException("MSG.NOT_DATA");

    TStatParams params;
    params.get(reqNode);

/*
    if(
            (TReqInfo::Instance()->desk.compatible(STAT_ORDERS_VERSION) and (
                params.statType == statRFISC
                or params.statType == statService
                or params.statType == statTlgOutFull
                )) or
            params.order
      )
        return orderStat(params, ctxt, reqNode, resNode);
*/
/*
    if (
            params.statType==statFull ||
            params.statType==statTrferFull ||
            params.statType==statSelfCkinFull ||
            params.statType==statAgentFull ||
            params.statType==statAgentShort ||
            params.statType==statAgentTotal ||
            params.statType==statTlgOutShort ||
            params.statType==statTlgOutDetail ||
            params.statType==statTlgOutFull ||
            params.statType==statRFISC ||
            params.statType==statUnaccBag
       )
    {
        if(IncMonth(params.FirstDate, 1) < params.LastDate)
            throw AstraLocale::UserException("MSG.SEARCH_PERIOD_SHOULD_NOT_EXCEED_ONE_MONTH");
    } else {
        if(IncMonth(params.FirstDate, 12) < params.LastDate)
            throw AstraLocale::UserException("MSG.SEARCH_PERIOD_SHOULD_NOT_EXCEED_ONE_YEAR");
    }
    */

    if(IncMonth(params.FirstDate, 12) < params.LastDate)
        throw AstraLocale::UserException("MSG.SEARCH_PERIOD_SHOULD_NOT_EXCEED_ONE_YEAR");

    switch(params.statType)
    {
        case statFull:
            get_compatible_report_form("FullStat", reqNode, resNode);
            break;
        case statTrferFull:
            get_compatible_report_form("TrferFullStat", reqNode, resNode);
            break;
        case statShort:
            get_compatible_report_form("ShortStat", reqNode, resNode);
            break;
        case statDetail:
            get_compatible_report_form("DetailStat", reqNode, resNode);
            break;
        case statRFISC:
            get_compatible_report_form("RFISCStat", reqNode, resNode);
            break;
        case statService:
            get_compatible_report_form("ServiceStat", reqNode, resNode);
            break;
        case statSelfCkinFull:
        case statSelfCkinShort:
        case statSelfCkinDetail:
        case statAgentFull:
        case statAgentShort:
        case statAgentTotal:
        case statTlgOutFull:
        case statTlgOutShort:
        case statTlgOutDetail:
        case statPactShort:
        case statLimitedCapab:
        case statUnaccBag:
        case statAnnulBT:
        case statPFSFull:
        case statPFSShort:
        case statTrferPax:
            get_compatible_report_form("stat", reqNode, resNode);
            break;
        default:
            throw Exception("unexpected stat type %d", params.statType);
    };


    try
    {
        if (params.statType==statShort || params.statType==statDetail || params.statType == statPactShort)
        {
            params.pr_pacts =
                reqInfo->user.access.rights().permitted(605) and params.seance == seanceAll;

            if(not params.pr_pacts and params.statType == statPactShort)
                throw UserException("MSG.INSUFFICIENT_RIGHTS.NOT_ACCESS");

            TDetailStat DetailStat;
            TDetailStatRow DetailStatTotal;
            TPrintAirline airline;

            if (params.pr_pacts)
                RunPactDetailStat(params, DetailStat, DetailStatTotal, airline);
            else
                RunDetailStat(params, DetailStat, DetailStatTotal, airline);

            createXMLDetailStat(params, params.pr_pacts, DetailStat, DetailStatTotal, airline, resNode);
        };
        if (params.statType==statFull || params.statType==statTrferFull)
        {
            TFullStat FullStat;
            TFullStatRow FullStatTotal;
            TPrintAirline airline;

            RunFullStat(params, FullStat, FullStatTotal, airline);

            createXMLFullStat(params, FullStat, FullStatTotal, airline, resNode);
        };
        if(
                params.statType == statSelfCkinShort or
                params.statType == statSelfCkinDetail or
                params.statType == statSelfCkinFull
          ) {
            TSelfCkinStat SelfCkinStat;
            TSelfCkinStatRow SelfCkinStatTotal;
            TPrintAirline airline;
            RunSelfCkinStat(params, SelfCkinStat, SelfCkinStatTotal, airline);
            createXMLSelfCkinStat(params, SelfCkinStat, SelfCkinStatTotal, airline, resNode);
        }
        if(
                params.statType == statAgentShort or
                params.statType == statAgentFull or
                params.statType == statAgentTotal
          ) {
            TAgentStat AgentStat;
            TAgentStatRow AgentStatTotal;
            TPrintAirline airline;
            RunAgentStat(params, AgentStat, AgentStatTotal, airline);
            createXMLAgentStat(params, AgentStat, AgentStatTotal, airline, resNode);
        }
        if(
                params.statType == statTlgOutShort or
                params.statType == statTlgOutDetail or
                params.statType == statTlgOutFull
          ) {
            TTlgOutStat TlgOutStat;
            TTlgOutStatRow TlgOutStatTotal;
            TPrintAirline airline;
            RunTlgOutStat(params, TlgOutStat, TlgOutStatTotal, airline);
            createXMLTlgOutStat(params, TlgOutStat, TlgOutStatTotal, airline, resNode);
        }
        if(params.statType == statRFISC)
        {
            TPrintAirline airline;
            TRFISCStat RFISCStat;
            RunRFISCStat(params, RFISCStat, airline);
            createXMLRFISCStat(params,RFISCStat, airline, resNode);
        }
        if(params.statType == statService)
        {
            TPrintAirline airline;
            TServiceStat ServiceStat;
            RunServiceStat(params, ServiceStat, airline);
            createXMLServiceStat(params,ServiceStat, airline, resNode);
        }
        if(params.statType == statLimitedCapab)
        {
            TPrintAirline airline;
            TLimitedCapabStat LimitedCapabStat;
            RunLimitedCapabStat(params, LimitedCapabStat, airline);
            createXMLLimitedCapabStat(params, LimitedCapabStat, airline, resNode);
        }
        if(params.statType == statUnaccBag)
        {
            TPrintAirline airline;
            TUnaccBagStat UnaccBagStat;
            RunUnaccBagStat(params, UnaccBagStat, airline);
            createXMLUnaccBagStat(params, UnaccBagStat, airline, resNode);
        }
        if(params.statType == statAnnulBT)
        {
            TPrintAirline airline;
            TAnnulBTStat AnnulBTStat;
            RunAnnulBTStat(params, AnnulBTStat, airline);
            createXMLAnnulBTStat(params, AnnulBTStat, airline, resNode);
        }
        if(params.statType == statPFSFull)
        {
            TPrintAirline airline;
            TPFSStat PFSStat;
            RunPFSStat(params, PFSStat, airline);
            createXMLPFSStat(params, PFSStat, airline, resNode);
        }
        if(params.statType == statPFSShort)
        {
            TPrintAirline airline;
            TPFSShortStat PFSShortStat;
            RunPFSShortStat(params, PFSShortStat, airline);
            createXMLPFSShortStat(params, PFSShortStat, airline, resNode);
        }
        if(params.statType == statTrferPax)
        {
            TPrintAirline airline;
            TTrferPaxStat TrferPaxStat;
            RunTrferPaxStat(params, TrferPaxStat, airline);
            createXMLTrferPaxStat(params, TrferPaxStat, airline, resNode);
        }
    }
    /* GRISHA */
    catch (MaxStatRowsException &E)
    {
        if(TReqInfo::Instance()->desk.compatible(STAT_ORDERS_VERSION))
        {
            RemoveChildNodes(resNode);
            return orderStat(params, ctxt, reqNode, resNode);
        } else {
            AstraLocale::showErrorMessage(E.getLexemaData());
        }
    }
    catch (EOracleError &E)
    {
        if(E.Code == 376)
            throw AstraLocale::UserException("MSG.ONE_OF_DB_FILES_UNAVAILABLE.CALL_ADMIN");
        else
            throw;
    }
}

void TStatOrders::check_integrity()
{
    for(TStatOrderMap::iterator i = items.begin(); i != items.end(); i++)
        i->second.check_integrity(NoExists);
}

void TStatOrders::get(int file_id)
{
    return get(NoExists, file_id);
}

void TStatOrders::get(const string &source)
{
    return get(TReqInfo::Instance()->user.user_id, NoExists, source);
}

string TFileParams::get_name()
{
    string result;
    string type = items[PARAM_TYPE];
    string name = items[PARAM_NAME];
    result = getLocaleText((type.empty() ? "Unknown" : type), LANG_EN) + "-";
    result += getLocaleText((name.empty() ? "Unknown" : name), LANG_EN);
    boost::replace_all(result, " ", "_");
    return result;
}

void TFileParams::get(int file_id)
{
    items.clear();
    TCachedQuery Qry(
            "select name, value from file_params where id = :file_id", QParams() << QParam("file_id", otInteger, file_id)
            );
    Qry.get().Execute();
    for(; not Qry.get().Eof; Qry.get().Next())
        items[Qry.get().FieldAsString("name")] = Qry.get().FieldAsString("value");
}

void TStatOrder::get_parts()
{
    TCachedQuery Qry(
            "select * from stat_orders_data where "
            "   file_id = :file_id "
            "order by month "
            ,
            QParams() << QParam("file_id", otInteger, file_id)
            );
    Qry.get().Execute();
    for(; not Qry.get().Eof; Qry.get().Next()) {
        TStatOrderDataItem item;
        item.file_id = file_id;
        item.month = Qry.get().FieldAsDateTime("month");
        item.file_name = Qry.get().FieldAsString("file_name");
        item.file_size = Qry.get().FieldAsFloat("file_size");
        item.file_size_zip = Qry.get().FieldAsFloat("file_size_zip");
        item.md5_sum = Qry.get().FieldAsString("md5_sum");
        so_data.push_back(item);
    }
}

void TStatOrder::fromDB(TCachedQuery &Qry)
{
    clear();
    if(not Qry.get().Eof) {
        file_id = Qry.get().FieldAsInteger("file_id");
        TFileParams params;
        params.get(file_id);
        name = params.get_name();
        user_id = Qry.get().FieldAsInteger("user_id");
        time_ordered = Qry.get().FieldAsDateTime("time_ordered");
        if(not Qry.get().FieldIsNULL("time_created"))
            time_created = Qry.get().FieldAsDateTime("time_created");
        if(time_ordered == time_created)
            time_created = NoExists;
        if(time_created != NoExists) { // Если заказ еще не сформирован, size неизвестен (NoExists)
            data_size = Qry.get().FieldAsFloat("data_size");
            data_size_zip = Qry.get().FieldAsFloat("data_size_zip");
        }
        source = DecodeOrderSource(Qry.get().FieldAsString("source"));
        status = (TOrderStatus)Qry.get().FieldAsInteger("status");
        error = Qry.get().FieldAsString("error");
        progress = Qry.get().FieldAsInteger("progress");
        get_parts();
    }
}

void TStatOrders::get(int user_id, int file_id, const string &source)
{
    items.clear();
    QParams QryParams;
    string condition;
    if(user_id != NoExists) {
        condition = " user_id = :user_id ";
        QryParams << QParam("user_id", otInteger, user_id);
    }
    if(file_id != NoExists) {
        if(not condition.empty())
            condition += " and ";
        condition += " file_id = :file_id ";
        QryParams << QParam("file_id", otInteger, file_id);
    } else if(not source.empty()) {
        if(not condition.empty())
            condition += " and ";
        condition += "   source = :source ";
        QryParams << QParam("source", otString, source);
    }
    string SQLText = "select * from stat_orders ";
    if(not condition.empty())
        SQLText += " where " + condition;
    TCachedQuery Qry(SQLText, QryParams);

    Qry.get().Execute();
    TPerfTimer tm;
    tm.Init();
    for(; not Qry.get().Eof; Qry.get().Next()) {
        TStatOrder item;
        item.fromDB(Qry);
        items[item.time_ordered] = item;
    }
    ProgTrace(TRACE5, "Stat Orders from DB: %s", tm.PrintWithMessage().c_str());
}

enum TColType { ctCompressRate };

void TStatOrders::toXML(xmlNodePtr resNode)
{
    xmlNodePtr grdNode = NewTextChild(resNode, "grd");
    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("Отчет"));
    SetProp(colNode, "width", 150);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Время заказа"));
    SetProp(colNode, "width", 110);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDate);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Время выполнения"));
    SetProp(colNode, "width", 110);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDate);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Время удаления"));
    SetProp(colNode, "width", 110);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDate);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Статус"));
    SetProp(colNode, "width", 110);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Готовность"));
    SetProp(colNode, "width", 110);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDate);
    colNode = NewTextChild(headerNode, "col", getLocaleText("Размер"));
    SetProp(colNode, "width", 110);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortFloat);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;

    double total_size = 0;
    for(TStatOrderMap::iterator i = items.begin(); i != items.end(); i++) {
        const TStatOrder &curr_order = i->second;
        rowNode = NewTextChild(rowsNode, "row");

        SetProp(rowNode, "file_id", curr_order.file_id);
        SetProp(rowNode, "size", FloatToString(curr_order.data_size));
        SetProp(rowNode, "size_zip", FloatToString(curr_order.data_size_zip));
        SetProp(rowNode, "status", EncodeOrderStatus(curr_order.status));

        // Отчет
        NewTextChild(rowNode, "col", curr_order.name);

        // Время заказа
        NewTextChild(rowNode, "col", DateTimeToStr(curr_order.time_ordered));

        // Время выполнения и удаления
        if(curr_order.time_created == NoExists) {
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
        } else {
            NewTextChild(rowNode, "col", DateTimeToStr(curr_order.time_created));
            NewTextChild(rowNode, "col", DateTimeToStr(curr_order.time_created + ORDERS_TIMEOUT()));
        }

        // Статус
        NewTextChild(rowNode, "col", getLocaleText(EncodeOrderStatus(curr_order.status)));

        // Готовность
        NewTextChild(rowNode, "col", IntToString(curr_order.progress) + "%");

        // Размер
        if(curr_order.data_size == NoExists)
            NewTextChild(rowNode, "col");
        else
            NewTextChild(rowNode, "col", getFileSizeStr(curr_order.data_size));

        total_size += (curr_order.data_size == NoExists ? 0 : curr_order.data_size);
    }
    rowNode = NewTextChild(rowsNode, "row");
    NewTextChild(rowNode, "col", getLocaleText("Итого:"));
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col");
    NewTextChild(rowNode, "col", getFileSizeStr(total_size));
}

void StatInterface::FileList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int file_id = NodeAsInteger("file_id", reqNode);
    TStatOrders so;
    so.get(file_id);
    so.items.begin()->second.check_integrity(NoExists);
    if(so.items.begin()->second.status != stReady)
        throw UserException("MSG.STAT_ORDERS.FILE_TRANSFER_ERROR",
                LParams() << LParam("status", EncodeOrderStatus(so.items.begin()->second.status)));
    if(so.so_data_empty(file_id))
        throw UserException("MSG.STAT_ORDERS.ORDER_IS_EMPTY");
    xmlNodePtr filesNode = NewTextChild(resNode, "files");
    for(TStatOrderMap::iterator curr_order_idx = so.items.begin(); curr_order_idx != so.items.end(); curr_order_idx++) {
        const TStatOrder &so = curr_order_idx->second;
        for(TStatOrderData::const_iterator so_data_idx = so.so_data.begin(); so_data_idx != so.so_data.end(); so_data_idx++) {
            xmlNodePtr itemNode = NewTextChild(filesNode, "item");
            NewTextChild(itemNode, "month", DateTimeToStr(so_data_idx->month, ServerFormatDateTimeAsString));
            NewTextChild(itemNode, "file_name", so_data_idx->file_name);
            NewTextChild(itemNode, "file_size", so_data_idx->file_size);
            NewTextChild(itemNode, "file_size_zip", so_data_idx->file_size_zip);
        }
    }
}

void StatInterface::DownloadOrder(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int file_id = NodeAsInteger("file_id", reqNode);
    TDateTime month = NodeAsDateTime("month", reqNode, NoExists);
    int pos = NodeAsInteger("pos", reqNode, 0);
    TDateTime finished_month = NodeAsDateTime("finished_month", reqNode, NoExists);
    TStatOrders so;
    if(finished_month != NoExists)
        so.get_part(file_id, finished_month)->complete();

    if(month == NoExists) return;

    so.get_part(file_id, month);

    ifstream in(get_part_file_name(file_id, month).c_str(), ios::binary);
    if(in.is_open()) {
        boost::shared_array<char> data (new char[ORDERS_BLOCK_SIZE()]);
        in.seekg(pos);
        in.read(data.get(), ORDERS_BLOCK_SIZE());

        NewTextChild(resNode, "data", StrUtils::b64_encode(data.get(), in.gcount()));
    } else
        throw UserException("file open error");
}

void StatInterface::StatOrderDel(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    int file_id = NodeAsInteger("file_id", reqNode);
    TStatOrders so;
    so.get(file_id);

    if(so.items.size() != 1) return; // must be exactly one item

    TStatOrder &order = so.items.begin()->second;
    if(order.status == stRunning)
        throw UserException("MSG.STAT_ORDERS.CANT_DEL_RUNNING");
    order.del();
}

void StatInterface::StatOrders(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TPerfTimer tm;
    string source = NodeAsString("source", reqNode);
    int file_id = NodeAsInteger("file_id", reqNode, NoExists);
    TStatOrders so;
    so.get(TReqInfo::Instance()->user.user_id, file_id, source);
    so.check_integrity();
    if(file_id != NoExists) { // был запрошен ровно один отчет, для него проверка целостности
        so.items.begin()->second.check_integrity(NoExists);
    }
    so.toXML(resNode);
    LogTrace(TRACE5) << "StatOrders handler time: " << tm.PrintWithMessage();
}

void StatInterface::PaxSrcRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo &info = *(TReqInfo::Instance());
    if (!info.user.access.rights().permitted(620))
        throw AstraLocale::UserException("MSG.PAX_SRC.ACCESS_DENIED");
    if (info.user.access.totally_not_permitted())
        throw AstraLocale::UserException("MSG.PASSENGERS.NOT_FOUND");
    TDateTime FirstDate = NodeAsDateTime("FirstDate", reqNode);
    TDateTime LastDate = NodeAsDateTime("LastDate", reqNode);
    if(IncMonth(FirstDate, 1) < LastDate)
        throw AstraLocale::UserException("MSG.SEARCH_PERIOD_SHOULD_NOT_EXCEED_ONE_MONTH");
    TPerfTimer tm;
    TQuery Qry(&OraSession);
    Qry.CreateVariable("FirstDate", otDate, FirstDate);
    Qry.CreateVariable("LastDate", otDate, LastDate);
    Qry.CreateVariable("pr_lat", otInteger, info.desk.lang != AstraLocale::LANG_RU);
    xmlNodePtr paramNode = reqNode->children;
    string airline = NodeAsStringFast("airline", paramNode, "");
    if(!airline.empty())
        Qry.CreateVariable("airline", otString, airline);
    string city = NodeAsStringFast("dest", paramNode, "");
    if(!city.empty())
        Qry.CreateVariable("city", otString, city);
    string flt_no = NodeAsStringFast("flt_no", paramNode, "");
    if(!flt_no.empty())
        Qry.CreateVariable("flt_no", otString, flt_no);
    string surname = NodeAsStringFast("surname", paramNode, "");
    if(!surname.empty())
        Qry.CreateVariable("surname", otString, surname);
    string document = NodeAsStringFast("document", paramNode, "");
    if(!document.empty()) {
        if(document.size() < 6)
            throw AstraLocale::UserException("MSG.PAX_SRC.MIN_DOC_LENGTH");
        Qry.CreateVariable("document", otString, document);
    }
    string ticket_no = NodeAsStringFast("ticket_no", paramNode, "");
    if(!ticket_no.empty()) {
        if(ticket_no.size() < 6)
            throw AstraLocale::UserException("MSG.PAX_SRC.MIN_TKT_LENGTH");
        Qry.CreateVariable("ticket_no", otString, ticket_no);
    }
    string tag_no = NodeAsStringFast("tag_no", paramNode, "");
    if(!tag_no.empty()) {
        if(tag_no.size() < 3)
            throw AstraLocale::UserException("MSG.PAX_SRC.MIN_TAG_LENGTH");
        double Value;
        if ( StrToFloat( tag_no.c_str(), Value ) == EOF )
            throw Exception("Cannot convert tag no '%s' to an Float", tag_no.c_str());
        Qry.CreateVariable("tag_no", otFloat, Value);
    }
    int count = 0;
    for(int pass = 0; (pass <= 2) && (count < MAX_STAT_ROWS()); pass++) {
        ostringstream sql;
        sql << "SELECT \n";
        if (pass==0)
          sql << " NULL part_key, \n";
        else
          sql << " points.part_key, \n";
        sql << " pax_grp.point_dep point_id, \n"
               " points.airline, \n"
               " points.flt_no, \n"
               " points.suffix, \n"
               " points.airline_fmt, \n"
               " points.airp_fmt, \n"
               " points.suffix_fmt, \n"
               " points.airp, \n"
               " points.scd_out, \n"
               " NVL(points.act_out,NVL(points.est_out,points.scd_out)) AS real_out, \n"
               " pax.reg_no, \n"
               " pax_grp.airp_arv, \n"
               " pax.surname||' '||pax.name full_name, \n";
        if (pass==0)
          sql << " NVL(ckin.get_bagAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) bag_amount, \n"
                 " NVL(ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) bag_weight, \n"
                 " NVL(ckin.get_rkWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) rk_weight, \n"
                 " NVL(ckin.get_excess(pax.grp_id,pax.pax_id),0) excess, \n"
                 "   pax_grp.piece_concept, "
                 " ckin.get_birks2(pax.grp_id,pax.pax_id,pax.bag_pool_num,:pr_lat) tags, \n"
                 " salons.get_seat_no(pax.pax_id, pax.seats, pax.is_jmp, pax_grp.status, pax_grp.point_dep, 'seats', rownum) seat_no, \n";
        else
          sql << " NVL(arch.get_bagAmount2(pax.part_key,pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) bag_amount, \n"
                 " NVL(arch.get_bagWeight2(pax.part_key,pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) bag_weight, \n"
                 " NVL(arch.get_rkWeight2(pax.part_key,pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) rk_weight, \n"
                 " NVL(arch.get_excess(pax.part_key,pax.grp_id,pax.pax_id),0) excess, \n"
                 "   pax_grp.piece_concept, "
                 " arch.get_birks2(pax.part_key,pax.grp_id,pax.pax_id,pax.bag_pool_num,:pr_lat) tags, \n"
                 " LPAD(seat_no,3,'0')|| \n"
                 "      DECODE(SIGN(1-seats),-1,'+'||TO_CHAR(seats-1),'') seat_no, \n";
        sql << " pax_grp.grp_id, \n"
               " pax.pr_brd, \n"
               " pax.refuse, \n"
               " pax_grp.class_grp, \n"
               " pax_grp.hall, \n"
               " pax.ticket_no, \n"
               " pax.pax_id, \n"
               " pax_grp.status \n";
        if (pass==0)
          sql << "FROM pax_grp, pax, points \n";
        else
          sql << "FROM arx_pax_grp pax_grp, arx_pax pax, arx_points points \n";
        if (pass==2)
          sql << " , (SELECT part_key, move_id FROM move_arx_ext \n"
                 "    WHERE part_key >= :LastDate+:arx_trip_date_range AND part_key <= :LastDate+date_range) arx_ext \n";
        if(!document.empty())
        {
          if (pass==0)
            sql << " , pax_doc \n";
          else
            sql << " , arx_pax_doc pax_doc \n";
        };
        if(!tag_no.empty())
        {
          if (pass==0)
            sql << " , bag_tags \n";
          else
            sql << " , arx_bag_tags bag_tags \n";
        };
        sql << "WHERE \n";
        if (pass==1)
          sql << " points.part_key >= :FirstDate AND points.part_key < :LastDate + :arx_trip_date_range AND \n";
        if (pass==2)
          sql << " points.part_key = arx_ext.part_key AND points.move_id = arx_ext.move_id AND \n";
        sql << " points.scd_out >= :FirstDate AND points.scd_out < :LastDate AND \n";
        if (pass!=0)
          sql << " points.part_key = pax_grp.part_key AND \n";
        sql << " points.point_id = pax_grp.point_dep AND \n";
        if (pass!=0)
          sql << " pax_grp.part_key = pax.part_key AND \n";
        sql << " pax_grp.grp_id=pax.grp_id AND \n"
               " points.pr_del>=0 \n";
        if(!document.empty())
        {
          if (pass!=0)
            sql << " AND pax.part_key = pax_doc.part_key \n";
          sql << " AND pax.pax_id = pax_doc.pax_id \n"
                 " AND pax_doc.no like '%'||:document||'%' \n";
        };
        if(!tag_no.empty())
        {
          if (pass!=0)
            sql << " AND pax_grp.part_key = bag_tags.part_key \n";
          sql << " AND pax_grp.grp_id = bag_tags.grp_id \n"
                 " AND bag_tags.no like '%'||:tag_no \n";
        };
        if (!info.user.access.airps().elems().empty()) {
          if (info.user.access.airps().elems_permit())
            sql << " AND points.airp IN " << GetSQLEnum(info.user.access.airps().elems()) << "\n";
          else
            sql << " AND points.airp NOT IN " << GetSQLEnum(info.user.access.airps().elems()) << "\n";
        }
        if (!info.user.access.airlines().elems().empty()) {
          if (info.user.access.airlines().elems_permit())
            sql << " AND points.airline IN " << GetSQLEnum(info.user.access.airlines().elems()) << "\n";
          else
            sql << " AND points.airline NOT IN " << GetSQLEnum(info.user.access.airlines().elems()) << "\n";
        }
        if(!airline.empty())
          sql << " AND points.airline = :airline \n";
        if(!city.empty())
          sql << " AND pax_grp.airp_arv = :city \n";
        if(!flt_no.empty())
          sql << " AND points.flt_no = :flt_no \n";
        if(!surname.empty()) {
          if(FirstDate + 1 < LastDate && surname.size() < 4)
            sql << " AND pax.surname = :surname \n";
          else
            sql << " AND pax.surname like :surname||'%' \n";
        }
        if(!ticket_no.empty())
          sql << " AND pax.ticket_no like '%'||:ticket_no||'%' \n";
        if (pass!=0)
          Qry.CreateVariable("arx_trip_date_range", otInteger, ARX_TRIP_DATE_RANGE());

        //ProgTrace(TRACE5, "PaxSrcRun: pass=%d SQL=\n%s", pass, sql.str().c_str());
        Qry.SQLText = sql.str().c_str();
        try {
            tm.Init();
            Qry.Execute();
            ProgTrace(TRACE5, "EXEC QRY%d: %s", pass, tm.PrintWithMessage().c_str());
        } catch (EOracleError &E) {
            if(E.Code == 376)
                throw AstraLocale::UserException("MSG.ONE_OF_DB_FILES_UNAVAILABLE.CALL_ADMIN");
            else
                throw;
        }
        TExcessNodeList excessNodeList;
        PaxListToXML(Qry, resNode, excessNodeList, true, pass, count);

    }
    if(count == 0)
        throw AstraLocale::UserException("MSG.PASSENGERS.NOT_FOUND");

    xmlNodePtr paxListNode = GetNode("paxList", resNode);
    if(paxListNode!=NULL)
    {
      xmlNodePtr headerNode = NewTextChild(paxListNode, "header"); // для совместимости со старым терминалом
      NewTextChild(headerNode, "col", "Рейс");
    };

    STAT::set_variables(resNode);
    get_compatible_report_form("ArxPaxList", reqNode, resNode);
}

int STAT::agent_stat_delta(int argc,char **argv)
{
    try {
        TQuery Qry(&OraSession);
        Qry.SQLText =
            "begin "
            "   delete from agent_stat; "
            "end; ";
        Qry.Execute();
        Qry.SQLText =
            "select "
            "  point_id, "
            "  user_id, "
            "  desk, "
            "  ondate, "
            "  pax_time, "
            "  pax_amount, "
            "  asp.dpax_amount.inc pax_am_inc, "
            "  asp.dpax_amount.dec pax_am_dec, "
            "  asp.dtckin_amount.inc tckin_am_inc, "
            "  asp.dtckin_amount.dec tckin_am_dec, "
            "  asp.dbag_amount.inc bag_am_inc, "
            "  asp.dbag_amount.dec bag_am_dec, "
            "  asp.dbag_weight.inc bag_we_inc, "
            "  asp.dbag_weight.dec bag_we_dec, "
            "  asp.drk_amount.inc rk_am_inc, "
            "  asp.drk_amount.dec rk_am_dec, "
            "  asp.drk_weight.inc rk_we_inc, "
            "  asp.drk_weight.dec rk_we_dec "
            "from agent_stat_params asp";
        Qry.Execute();
        for(; not Qry.Eof; Qry.Next()) {
            agent_stat_delta(
                    Qry.FieldAsInteger("point_id"),
                    Qry.FieldAsInteger("user_id"),
                    Qry.FieldAsString("desk"),
                    Qry.FieldAsDateTime("ondate"),
                    Qry.FieldAsInteger("pax_time"),
                    Qry.FieldAsInteger("pax_amount"),
                    agent_stat_t(Qry.FieldAsInteger("pax_am_inc"), Qry.FieldAsInteger("pax_am_dec")),
                    agent_stat_t(Qry.FieldAsInteger("tckin_am_inc"), Qry.FieldAsInteger("tckin_am_dec")),
                    agent_stat_t(Qry.FieldAsInteger("bag_am_inc"), Qry.FieldAsInteger("bag_am_dec")),
                    agent_stat_t(Qry.FieldAsInteger("bag_we_inc"), Qry.FieldAsInteger("bag_we_dec")),
                    agent_stat_t(Qry.FieldAsInteger("rk_am_inc"), Qry.FieldAsInteger("rk_am_dec")),
                    agent_stat_t(Qry.FieldAsInteger("rk_we_inc"), Qry.FieldAsInteger("rk_we_dec"))
                    );
        }
        Qry.Clear();
    } catch(Exception &E) {
        cout << "Error: " << E.what() << endl;
        return 1;
    }
    return 0;
}

void STAT::agent_stat_delta(
        int point_id,
        int user_id,
        const std::string &desk,
        TDateTime ondate,
        int pax_time,
        int pax_amount,
        agent_stat_t dpax_amount,
        agent_stat_t dtckin_amount,
        agent_stat_t dbag_amount,
        agent_stat_t dbag_weight,
        agent_stat_t drk_amount,
        agent_stat_t drk_weight
        )
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "begin "
        "  update agent_stat ags set "
        "    pax_time = pax_time + :pax_time, "
        "    pax_amount = pax_amount + :pax_amount, "
        "    ags.dpax_amount.inc = ags.dpax_amount.inc + :pax_am_inc, "
        "    ags.dpax_amount.dec = ags.dpax_amount.dec + :pax_am_dec, "
        "    ags.dtckin_amount.inc = ags.dtckin_amount.inc + :tckin_am_inc, "
        "    ags.dtckin_amount.dec = ags.dtckin_amount.dec + :tckin_am_dec, "
        "    ags.dbag_amount.inc = ags.dbag_amount.inc + :bag_am_inc, "
        "    ags.dbag_amount.dec = ags.dbag_amount.dec + :bag_am_dec, "
        "    ags.dbag_weight.inc = ags.dbag_weight.inc + :bag_we_inc, "
        "    ags.dbag_weight.dec = ags.dbag_weight.dec + :bag_we_dec, "
        "    ags.drk_amount.inc = ags.drk_amount.inc + :rk_am_inc, "
        "    ags.drk_amount.dec = ags.drk_amount.dec + :rk_am_dec, "
        "    ags.drk_weight.inc = ags.drk_weight.inc + :rk_we_inc, "
        "    ags.drk_weight.dec = ags.drk_weight.dec + :rk_we_dec "
        "  where "
        "    point_id = :point_id and "
        "    user_id = :user_id and "
        "    desk = :desk and "
        "    ondate = TRUNC(:ondate); "
        "  if sql%notfound then "
        "    insert into agent_stat( "
        "        point_id, "
        "        user_id, "
        "        desk, "
        "        ondate, "
        "        pax_time, "
        "        pax_amount, "
        "        dpax_amount, "
        "        dtckin_amount, "
        "        dbag_amount, "
        "        dbag_weight, "
        "        drk_amount, "
        "        drk_weight "
        "    ) values ( "
        "        :point_id, "
        "        :user_id, "
        "        :desk, "
        "        TRUNC(:ondate), "
        "        :pax_time, "
        "        :pax_amount, "
        "        agent_stat_t(:pax_am_inc, :pax_am_dec), "
        "        agent_stat_t(:tckin_am_inc, :tckin_am_dec), "
        "        agent_stat_t(:bag_am_inc, :bag_am_dec), "
        "        agent_stat_t(:bag_we_inc, :bag_we_dec), "
        "        agent_stat_t(:rk_am_inc, :rk_am_dec), "
        "        agent_stat_t(:rk_we_inc, :rk_we_dec) "
        "    ); "
        "  end if; "
        "end; ";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("user_id", otInteger, user_id);
    Qry.CreateVariable("desk", otString, desk);
    Qry.CreateVariable("ondate", otDate, ondate);
    Qry.CreateVariable("pax_time", otInteger, pax_time);
    Qry.CreateVariable("pax_amount", otInteger, pax_amount);
    Qry.CreateVariable("pax_am_inc", otInteger, dpax_amount.inc);
    Qry.CreateVariable("pax_am_dec", otInteger, dpax_amount.dec);
    Qry.CreateVariable("tckin_am_inc", otInteger, dtckin_amount.inc);
    Qry.CreateVariable("tckin_am_dec", otInteger, dtckin_amount.dec);
    Qry.CreateVariable("bag_am_inc", otInteger, dbag_amount.inc);
    Qry.CreateVariable("bag_am_dec", otInteger, dbag_amount.dec);
    Qry.CreateVariable("bag_we_inc", otInteger, dbag_weight.inc);
    Qry.CreateVariable("bag_we_dec", otInteger, dbag_weight.dec);
    Qry.CreateVariable("rk_am_inc", otInteger, drk_amount.inc);
    Qry.CreateVariable("rk_am_dec", otInteger, drk_amount.dec);
    Qry.CreateVariable("rk_we_inc", otInteger, drk_weight.inc);
    Qry.CreateVariable("rk_we_dec", otInteger, drk_weight.dec);
    Qry.Execute();
}

struct TRFISCBag {

    struct TBagInfo {
        int excess, paid;
        TBagInfo():
            excess(NoExists),
            paid(NoExists)
        {}
    };

    typedef list<TBagInfo> TBagInfoList;
    typedef map<string, TBagInfoList> TRFISCGroup; // Группировка по кодам RFISC
    typedef map<int, TRFISCGroup> TGrpIdGroup; // Группировка по grp_id
    TGrpIdGroup items;

    TGrpIdGroup::iterator get(int grp_id)
    {
        TGrpIdGroup::iterator result = items.find(grp_id);
        if(result == items.end()) {
          TPaidRFISCListWithAuto paid;
          paid.fromDB(grp_id, true);
          TPaidRFISCStatusList statusList;
          for(TPaidRFISCListWithAuto::const_iterator i=paid.begin(); i!=paid.end(); ++i)
          {
            const TPaidRFISCItem &item=i->second;
            if (!item.list_item)
              throw Exception("TRFISCBag::get: item.list_item=boost::none! (%s)", item.traceStr().c_str());

            if (!item.list_item.get().carry_on()) continue;
            //только относящиеся к багажу или ручной клади
            if (item.list_item.get().carry_on().get()) continue;
            //только относящиеся к багажу
            if (item.trfer_num!=0) continue;
            //только относящиеся к багажу и только на начальном сегменте
            item.addStatusList(statusList);
          };
          for(TPaidRFISCStatusList::const_iterator i=statusList.begin();
                                                   i!=statusList.end(); ++i)
          {
            TBagInfo val;

            switch(i->status) {
                case TServiceStatus::Free:
                    val.excess = 0;
                    break;
                case TServiceStatus::Paid:
                case TServiceStatus::Need:
                    val.excess = 1;
                    break;
                default:
                    val.excess = NoExists;
                    break;
            }

            switch(i->status) {
                case TServiceStatus::Paid:
                    val.paid = 1;
                    break;
                case TServiceStatus::Unknown:
                case TServiceStatus::Need:
                    val.paid = 0;
                    break;
                default:
                    val.paid = NoExists;
                    break;
            }

            items[grp_id][i->RFISC].push_back(val);
          }
          result = items.find(grp_id);
        }
        return result;
    }
};


void get_rfisc_stat(int point_id)
{
    QParams QryParams;
    QryParams << QParam("point_id", otInteger, point_id);

    TCachedQuery delQry("delete from rfisc_stat where point_id = :point_id", QryParams);
    delQry.get().Execute();

    TCachedQuery bagQry(
            "select "
            "    points.point_id, "
            "    nvl2(transfer.grp_id, 1, 0) pr_trfer, "
            "    trfer_trips.airline trfer_airline, "
            "    trfer_trips.flt_no trfer_flt_no, "
            "    trfer_trips.suffix trfer_suffix, "
            "    transfer.airp_arv trfer_airp_arv, "
            "    trfer_trips.scd trfer_scd, "
            "    arv_point.point_num, "
            "    pax_grp.grp_id, "
            "    pax_grp.airp_arv, "
            "    pax.pax_id, "
            "    pax.subclass, "
            "    bag2.rfisc, "
            "    bag2.num bag_num, "
            "    bag2.desk, "
            "    bag2.time_create, "
            "    users2.login, "
            "    users2.descr, "
            "    points.airp, "
            "    points.craft, "
            "    arv_point.airp airp_last "
            "from "
            "    pax_grp, "
            "    points arv_point, "
            "    points, "
            "    bag2, "
            "    pax, "
            "    transfer, "
            "    trfer_trips, "
            "    users2 "
            "where "
            "    points.point_id = :point_id and "
            "    arv_point.pr_del>=0 AND "
            "    pax_grp.point_dep = points.point_id and "
            "    pax_grp.point_arv = arv_point.point_id and "
            "    pax_grp.grp_id = bag2.grp_id and "
            "    pax_grp.status NOT IN ('E') and "
            "    ckin.bag_pool_refused(bag2.grp_id,bag2.bag_pool_num,pax_grp.class,pax_grp.bag_refuse)=0 and "
            "    bag2.pr_cabin = 0 and "
            "    bag2.is_trfer = 0 and "
            "    bag2.rfisc is not null and "
            "    ckin.get_bag_pool_pax_id(bag2.grp_id, bag2.bag_pool_num) = pax.pax_id(+)  and "
            "    pax_grp.grp_id=transfer.grp_id(+) and "
            "    transfer.pr_final(+) <> 0 and "
            "    transfer.point_id_trfer = trfer_trips.point_id(+) and "
            "    bag2.user_id = users2.user_id(+) ",
        QryParams
            );

    TCachedQuery insQry(
            "insert into rfisc_stat( "
            "  point_id, "
            "  pr_trfer, "
            "  trfer_airline, "
            "  trfer_flt_no, "
            "  trfer_suffix, "
            "  trfer_airp_arv, "
            "  trfer_scd, "
            "  point_num, "
            "  airp_arv, "
            "  rfisc, "
            "  travel_time, "
            "  desk, "
            "  time_create, "
            "  user_login, "
            "  user_descr, "
            "  tag_no, "
            "  fqt_no, "
            "  excess, "
            "  paid "
            ") values ( "
            "  :point_id, "
            "  :pr_trfer, "
            "  :trfer_airline, "
            "  :trfer_flt_no, "
            "  :trfer_suffix, "
            "  :trfer_airp_arv, "
            "  :trfer_scd, "
            "  :point_num, "
            "  :airp_arv, "
            "  :rfisc, "
            "  :travel_time, "
            "  :desk, "
            "  :time_create, "
            "  :login, "
            "  :descr, "
            "  :bag_tag, "
            "  :fqt_no, "
            "  :excess, "
            "  :paid "
            ") ",
        QParams()
            << QParam("point_id", otInteger)
            << QParam("pr_trfer", otInteger)
            << QParam("trfer_airline", otString)
            << QParam("trfer_flt_no", otInteger)
            << QParam("trfer_suffix", otString)
            << QParam("trfer_airp_arv", otString)
            << QParam("trfer_scd", otDate)
            << QParam("point_num", otInteger)
            << QParam("airp_arv", otString)
            << QParam("rfisc", otString)
            << QParam("travel_time", otDate)
            << QParam("login", otString)
            << QParam("descr", otString)
            << QParam("desk", otString)
            << QParam("time_create", otDate)
            << QParam("bag_tag", otFloat)
            << QParam("fqt_no", otString)
            << QParam("excess", otInteger)
            << QParam("paid", otInteger)
            );

    TCachedQuery tagsQry("select no from bag_tags where grp_id = :grp_id and bag_num = :bag_num",
            QParams()
            << QParam("grp_id", otInteger)
            << QParam("bag_num", otInteger)
            );

    TCachedQuery fqtQry(
        "select "
        "   pax_fqt.rem_code, "
        "   pax_fqt.airline, "
        "   pax_fqt.no, "
        "   pax_fqt.extra, "
        "   crs_pnr.subclass "
        "from "
        "   pax_fqt, "
        "   crs_pax, "
        "   crs_pnr "
        "where "
        "   pax_fqt.pax_id = :pax_id and "
        "   pax_fqt.pax_id = crs_pax.pax_id(+) and "
        "   crs_pax.pr_del(+)=0 and "
        "   crs_pax.pnr_id = crs_pnr.pnr_id(+) and "
        "   pax_fqt.rem_code in('FQTV', 'FQTU', 'FQTR') ",
            QParams() << QParam("pax_id", otInteger));

    bagQry.get().Execute();
    if(not bagQry.get().Eof) {
        int col_point_id = bagQry.get().FieldIndex("point_id");
        int col_pr_trfer = bagQry.get().FieldIndex("pr_trfer");
        int col_trfer_airline = bagQry.get().FieldIndex("trfer_airline");
        int col_trfer_flt_no = bagQry.get().FieldIndex("trfer_flt_no");
        int col_trfer_suffix = bagQry.get().FieldIndex("trfer_suffix");
        int col_trfer_airp_arv = bagQry.get().FieldIndex("trfer_airp_arv");
        int col_trfer_scd = bagQry.get().FieldIndex("trfer_scd");
        int col_point_num = bagQry.get().FieldIndex("point_num");
        int col_grp_id = bagQry.get().FieldIndex("grp_id");
        int col_airp_arv = bagQry.get().FieldIndex("airp_arv");
        int col_pax_id = bagQry.get().FieldIndex("pax_id");
        int col_subclass = bagQry.get().FieldIndex("subclass");
        int col_rfisc = bagQry.get().FieldIndex("rfisc");
        int col_bag_num = bagQry.get().FieldIndex("bag_num");
        int col_desk = bagQry.get().FieldIndex("desk");
        int col_time_create = bagQry.get().FieldIndex("time_create");
        int col_login = bagQry.get().FieldIndex("login");
        int col_descr = bagQry.get().FieldIndex("descr");
        int col_airp = bagQry.get().FieldIndex("airp");
        int col_craft = bagQry.get().FieldIndex("craft");
        int col_airp_last = bagQry.get().FieldIndex("airp_last");
        map<int, TDateTime> travel_times;
        TRFISCBag rfisc_bag;
        for(; not bagQry.get().Eof; bagQry.get().Next()) {
            int grp_id = bagQry.get().FieldAsInteger(col_grp_id);
            TRFISCBag::TGrpIdGroup::iterator rfisc_grp = rfisc_bag.get(grp_id);
            if(rfisc_grp == rfisc_bag.items.end()) continue;

            int point_id =  bagQry.get().FieldAsInteger(col_point_id);
            insQry.get().SetVariable("point_id", point_id);
            insQry.get().SetVariable("pr_trfer", bagQry.get().FieldAsInteger(col_pr_trfer));
            insQry.get().SetVariable("trfer_airline", bagQry.get().FieldAsString(col_trfer_airline));
            insQry.get().SetVariable("trfer_suffix", bagQry.get().FieldAsString(col_trfer_suffix));
            insQry.get().SetVariable("trfer_airp_arv", bagQry.get().FieldAsString(col_trfer_airp_arv));
            insQry.get().SetVariable("point_num", bagQry.get().FieldAsInteger(col_point_num));
            insQry.get().SetVariable("airp_arv", bagQry.get().FieldAsString(col_airp_arv));
            insQry.get().SetVariable("rfisc", bagQry.get().FieldAsString(col_rfisc));
            insQry.get().SetVariable("desk", bagQry.get().FieldAsString(col_desk));

            if(bagQry.get().FieldIsNULL(col_trfer_flt_no))
                insQry.get().SetVariable("trfer_flt_no", FNull);
            else
                insQry.get().SetVariable("trfer_flt_no", bagQry.get().FieldAsInteger(col_trfer_flt_no));

            if(bagQry.get().FieldIsNULL(col_trfer_scd))
                insQry.get().SetVariable("trfer_scd", FNull);
            else
                insQry.get().SetVariable("trfer_scd", bagQry.get().FieldAsDateTime(col_trfer_scd));


            map<int, TDateTime>::iterator travel_times_idx = travel_times.find(point_id);
            if(travel_times_idx == travel_times.end()) {
                pair<map<int, TDateTime>::iterator, bool> ret =
                    travel_times.insert(
                            make_pair(point_id,
                                getTimeTravel(
                                    bagQry.get().FieldAsString(col_craft),
                                    bagQry.get().FieldAsString(col_airp),
                                    bagQry.get().FieldAsString(col_airp_last)
                                    )
                                )
                            );
                travel_times_idx = ret.first;
            }

            if(travel_times_idx->second == NoExists)
                insQry.get().SetVariable("travel_time", FNull);
            else
                insQry.get().SetVariable("travel_time", travel_times_idx->second);

            if(bagQry.get().FieldIsNULL(col_time_create))
                insQry.get().SetVariable("time_create", FNull);
            else
                insQry.get().SetVariable("time_create", bagQry.get().FieldAsDateTime(col_time_create));

            insQry.get().SetVariable("login", bagQry.get().FieldAsString(col_login));
            insQry.get().SetVariable("descr", bagQry.get().FieldAsString(col_descr));

            string fqt_no;
            if(not bagQry.get().FieldIsNULL(col_pax_id)) {
                string subcls = bagQry.get().FieldAsString(col_subclass);
                fqtQry.get().SetVariable("pax_id", bagQry.get().FieldAsInteger(col_pax_id));
                fqtQry.get().Execute();


                if(!fqtQry.get().Eof) {
                    int col_rem_code = fqtQry.get().FieldIndex("rem_code");
                    int col_airline = fqtQry.get().FieldIndex("airline");
                    int col_no = fqtQry.get().FieldIndex("no");
                    int col_extra = fqtQry.get().FieldIndex("extra");
                    int col_subclass = fqtQry.get().FieldIndex("subclass");
                    for(; !fqtQry.get().Eof; fqtQry.get().Next()) {
                        string item;
                        string rem_code = fqtQry.get().FieldAsString(col_rem_code);
                        string airline = fqtQry.get().FieldAsString(col_airline);
                        string no = fqtQry.get().FieldAsString(col_no);
                        string extra = fqtQry.get().FieldAsString(col_extra);
                        string subclass = fqtQry.get().FieldAsString(col_subclass);
                        item +=
                            rem_code + " " +
                            ElemIdToElem(etAirline, airline, efmtCodeNative, LANG_EN) + " " +
                            transliter(no, 1, true);
                        if(rem_code == "FQTV") {
                            if(not subclass.empty() and subclass != subcls)
                                item += "-" + ElemIdToElem(etSubcls, subclass, efmtCodeNative, LANG_EN);
                        } else {
                            if(not extra.empty())
                                item += "-" + transliter(extra, 1, true);
                        }
                        fqt_no = item;
                        break;
                    }
                }
            }

            tagsQry.get().SetVariable("grp_id", grp_id);
            tagsQry.get().SetVariable("bag_num", bagQry.get().FieldAsInteger(col_bag_num));
            tagsQry.get().Execute();
            for(; not tagsQry.get().Eof; tagsQry.get().Next()) {
                TRFISCBag::TBagInfoList &bag_info = rfisc_grp->second[bagQry.get().FieldAsString(col_rfisc)];
                TRFISCBag::TBagInfo paid_bag_item;
                if(not bag_info.empty()) {
                    paid_bag_item = bag_info.back();
                    bag_info.pop_back();
                }

                if(paid_bag_item.excess == NoExists)
                    insQry.get().SetVariable("excess", FNull);
                else
                    insQry.get().SetVariable("excess", paid_bag_item.excess);
                if(paid_bag_item.paid == NoExists)
                    insQry.get().SetVariable("paid", FNull);
                else
                    insQry.get().SetVariable("paid", paid_bag_item.paid);

                insQry.get().SetVariable("bag_tag", tagsQry.get().FieldAsFloat("no"));
                insQry.get().SetVariable("fqt_no", fqt_no);
                fqt_no.clear();

                insQry.get().Execute();
            }
        }
    }
}

void nosir_rfisc_stat_point(int point_id)
{
    TFlights flightsForLock;
    flightsForLock.Get( point_id, ftTranzit );
    flightsForLock.Lock(__FUNCTION__);

    TQuery Qry(&OraSession);
    Qry.SQLText = "SELECT count(*) from points where point_id=:point_id AND pr_del=0";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.Execute();
    if (Qry.Eof || Qry.FieldAsInteger(0) == 0)
    {
        OraSession.Rollback();
        return;
    }

    bool pr_stat = false;
    Qry.SQLText = "SELECT pr_stat FROM trip_sets WHERE point_id=:point_id";
    Qry.Execute();
    if(not Qry.Eof) pr_stat = Qry.FieldAsInteger(0) != 0;

    int count = 0;
    Qry.SQLText = "select count(*) from rfisc_stat where point_id=:point_id";
    Qry.Execute();
    if(not Qry.Eof) count = Qry.FieldAsInteger(0);

    if(pr_stat and count == 0)
        get_rfisc_stat(point_id);

    OraSession.Commit();
}

int nosir_self_ckin(int argc,char **argv)
{
    cout << "start time: " << DateTimeToStr(NowUTC(), ServerFormatDateTimeAsString) << endl;
    map<string, map<string, int> > result;
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "select "
        "    points.point_id, "
        "    points.airline, "
        "    scs.client_type "
        "from "
        "    self_ckin_stat scs, "
        "    points "
        "where "
        "    points.scd_out >= to_date('01.11.2015 00:00:00', 'DD.MM.YYYY HH24:MI:SS') and "
        "    points.scd_out < to_date('01.02.2016 00:00:00', 'DD.MM.YYYY HH24:MI:SS') and "
        "    points.point_id = scs.point_id ";
    Qry.Execute();
    if(not Qry.Eof) {
        int col_point_id = Qry.GetFieldIndex("point_id");
        int col_airline = Qry.GetFieldIndex("airline");
        int col_client_type = Qry.GetFieldIndex("client_type");
        int count = 0;
        map<int, string> points;
        for(; not Qry.Eof; Qry.Next(), count++) {
            if(count % 100 == 0) cout << count << endl;
            int point_id = Qry.FieldAsInteger(col_point_id);

            map<int, string>::iterator idx = points.find(point_id);
            if(idx == points.end()) {
                TTripRoute route;
                route.GetRouteAfter(NoExists, point_id, trtNotCurrent, trtNotCancelled);
                string airp_arv;
                if(not route.empty())
                    airp_arv = route.back().airp;
                pair<map<int, string>::iterator, bool> res = points.insert(make_pair(point_id, airp_arv));
                idx = res.first;
            }

            if(idx->second != "СОЧ") continue;
            result[Qry.FieldAsString(col_airline)][Qry.FieldAsString(col_client_type)]++;
        }
    }
    ofstream out("self_ckin.csv");
    for(map<string, map<string, int> >::iterator i = result.begin(); i != result.end(); i++) {
        for(map<string, int>::iterator j = i->second.begin(); j != i->second.end(); j++) {
            out
                << i->first << ","
                << j->first << ","
                << "СОЧ,"
                << j->second << endl;
        }
    }
    cout << "end time: " << DateTimeToStr(NowUTC(), ServerFormatDateTimeAsString) << endl;
    return 1;
}

int nosir_rfisc_all_xml(int argc,char **argv)
{
    TDateTime begin;
    TDateTime now = NowUTC();
    StrToDateTime("01.10.2015 00:00:00", "dd.mm.yyyy hh:nn:ss", begin);
    TQuery nextDateQry(&OraSession);
    nextDateQry.SQLText = "select add_months(:begin, 1) from dual";
    nextDateQry.DeclareVariable("begin", otDate);
    while(begin < now) {
        nextDateQry.SetVariable("begin", begin);
        nextDateQry.Execute();
        TDateTime end = nextDateQry.FieldAsDateTime(0);

        TStatParams params;
        TPrintAirline airline;
        TRFISCStat RFISCStat;
        RunRFISCStat(params, RFISCStat, airline);
        //createXMLRFISCStat(params,RFISCStat, airline, resNode);

        begin = end;
    }
    return 1;
}

int nosir_rfisc_all(int argc,char **argv)
{
    TDateTime begin;
    StrToDateTime("01.10.2015 00:00:00", "dd.mm.yyyy hh:nn:ss", begin);
    TQuery nextDateQry(&OraSession);
    nextDateQry.SQLText = "select add_months(:begin, 1) from dual";
    nextDateQry.DeclareVariable("begin", otDate);

    TQuery rfiscQry(&OraSession);
    rfiscQry.SQLText =
        "select "
        "   rfisc_stat.rfisc, "
        "   rfisc_stat.point_id, "
        "   rfisc_stat.point_num, "
        "   rfisc_stat.pr_trfer, "
        "   points.scd_out, "
        "   points.airline, "
        "   points.flt_no, "
        "   points.suffix, "
        "   points.airp, "
        "   rfisc_stat.airp_arv, "
        "   points.craft, "
        "   rfisc_stat.travel_time, "
        "   rfisc_stat.trfer_flt_no, "
        "   rfisc_stat.trfer_suffix, "
        "   rfisc_stat.trfer_airp_arv, "
        "   rfisc_stat.desk, "
        "   rfisc_stat.user_login, "
        "   rfisc_stat.user_descr, "
        "   rfisc_stat.time_create, "
        "   rfisc_stat.tag_no, "
        "   rfisc_stat.fqt_no, "
        "   rfisc_stat.excess, "
        "   rfisc_stat.paid "
        "from "
        "   points, "
        "   rfisc_stat "
        "where "
        "   rfisc_stat.point_id = points.point_id and "
        "   points.pr_del >= 0 and "
        "   points.scd_out >= :FirstDate AND points.scd_out < :LastDate "
        "order by "
        "   rfisc_stat.point_id, "
        "   rfisc_stat.point_num, "
        "   rfisc_stat.pr_trfer, "
        "   rfisc_stat.airp_arv ";
    rfiscQry.DeclareVariable("FirstDate", otDate);
    rfiscQry.DeclareVariable("LastDate", otDate);
    TDateTime now = NowUTC();
    const char delim = ';';
    ostringstream header;
    header
        << "RFISC" << delim
        << "Платн." << delim
        << "Опл." << delim
        << "Бирка" << delim
        << "SPEC" << delim
        << "FQTV" << delim
        << "Дата вылета" << delim
        << "Рейс" << delim
        << "От" << delim
        << "До" << delim
        << "Тип ВС" << delim
        << "Время в пути" << delim
        << "Трфр.рейс" << delim
        << "От" << delim
        << "До" << delim
        << "АП рег." << delim
        << "Стойка" << delim
        << "LOGIN" << delim
        << "Агент" << delim
        << "Дата оформ.";
    ofstream rfisc_all("rfisc_all.csv");
    rfisc_all << header.str() << endl;
    int count = 0;
    while(begin < now) {
        nextDateQry.SetVariable("begin", begin);
        nextDateQry.Execute();
        TDateTime end = nextDateQry.FieldAsDateTime(0);
        cout << "begin: " << DateTimeToStr(begin, ServerFormatDateTimeAsString) << endl;
        cout << "end: " << DateTimeToStr(end, ServerFormatDateTimeAsString) << endl;
        rfiscQry.SetVariable("FirstDate", begin);
        rfiscQry.SetVariable("LastDate", end);
        rfiscQry.Execute();
        if(not rfiscQry.Eof) {
            string fname = "rfisc." + DateTimeToStr(begin, "yyyymm") + ".csv";
            ofstream rfisc_month(fname);
            rfisc_month << header.str() << endl;
            ostringstream out;

            int col_rfisc = rfiscQry.GetFieldIndex("rfisc");
            int col_scd_out = rfiscQry.GetFieldIndex("scd_out");
            int col_flt_no = rfiscQry.GetFieldIndex("flt_no");
            int col_suffix = rfiscQry.GetFieldIndex("suffix");
            int col_airp = rfiscQry.GetFieldIndex("airp");
            int col_airp_arv = rfiscQry.GetFieldIndex("airp_arv");
            int col_craft = rfiscQry.GetFieldIndex("craft");
            int col_travel_time = rfiscQry.GetFieldIndex("travel_time");
            int col_pr_trfer = rfiscQry.GetFieldIndex("pr_trfer");
            int col_trfer_flt_no = rfiscQry.GetFieldIndex("trfer_flt_no");
            int col_trfer_suffix = rfiscQry.GetFieldIndex("trfer_suffix");
            int col_trfer_airp_arv = rfiscQry.GetFieldIndex("trfer_airp_arv");
            int col_desk = rfiscQry.GetFieldIndex("desk");
            int col_user_login = rfiscQry.GetFieldIndex("user_login");
            int col_user_descr = rfiscQry.GetFieldIndex("user_descr");
            int col_time_create = rfiscQry.GetFieldIndex("time_create");
            int col_tag_no = rfiscQry.GetFieldIndex("tag_no");
            int col_fqt_no = rfiscQry.GetFieldIndex("fqt_no");
            int col_excess = rfiscQry.GetFieldIndex("excess");
            int col_paid = rfiscQry.GetFieldIndex("paid");

            for(; not rfiscQry.Eof; rfiscQry.Next(), count++) {
                // RFISC
                out
                    << rfiscQry.FieldAsString(col_rfisc) << delim;
                // Платн.
                if(not rfiscQry.FieldIsNULL(col_excess))
                    out << rfiscQry.FieldAsInteger(col_excess);
                out << delim;
                // Опл.
                if(not rfiscQry.FieldIsNULL(col_paid))
                    out << rfiscQry.FieldAsInteger(col_paid);
                out
                    << delim
                    // Бирка
                    << rfiscQry.FieldAsString(col_tag_no) << delim
                    // SPEQ
                    << delim
                    // FQTV
                    << rfiscQry.FieldAsString(col_fqt_no) << delim
                    // Дата вылета
                    << DateTimeToStr(rfiscQry.FieldAsDateTime(col_scd_out), "dd.mm.yyyy") << delim
                    // Рейс
                    << rfiscQry.FieldAsInteger(col_flt_no) << rfiscQry.FieldAsString(col_suffix) << delim
                    // От
                    << rfiscQry.FieldAsString(col_airp) << delim
                    // До
                    << rfiscQry.FieldAsString(col_airp_arv) << delim
                    // Тип ВС
                    << rfiscQry.FieldAsString(col_craft) << delim;
                // Время в пути
                if(not rfiscQry.FieldIsNULL(col_travel_time))
                    out << DateTimeToStr(rfiscQry.FieldAsDateTime(col_travel_time), "hh:nn");
                out
                    << delim;
                if(rfiscQry.FieldAsInteger(col_pr_trfer) != 0) {
                    out
                        // Трфр.рейс
                        << rfiscQry.FieldAsInteger(col_trfer_flt_no) << rfiscQry.FieldAsString(col_trfer_suffix) << delim
                        // От
                        << rfiscQry.FieldAsString(col_airp) << delim
                        // До
                        << rfiscQry.FieldAsString(col_trfer_airp_arv) << delim;
                } else {
                    out << delim << delim << delim;
                }
                out
                    // АП рег
                    << rfiscQry.FieldAsString(col_airp) << delim
                    // Стойка
                    << rfiscQry.FieldAsString(col_desk) << delim
                    // Логин
                    << rfiscQry.FieldAsString(col_user_login) << delim
                    // Агент
                    << rfiscQry.FieldAsString(col_user_descr) << delim;
                // Дата оформ.
                if(not rfiscQry.FieldIsNULL(col_time_create))
                    out << DateTimeToStr(rfiscQry.FieldAsDateTime(col_scd_out), "dd.mm.yyyy");
                out << endl;
                if(count % 10000 == 0)
                    cout << count << endl;
            }
            rfisc_month << out.str();
            rfisc_all << out.str();
        }
        OraSession.Rollback();
        begin = end;
    }
    cout << count << endl;
    return 1;
}

void nosir_lim_capab_stat_point(int point_id)
{
    TFlights flightsForLock;
    flightsForLock.Get( point_id, ftTranzit );
    flightsForLock.Lock(__FUNCTION__);

    TQuery Qry(&OraSession);
    Qry.SQLText = "SELECT count(*) from points where point_id=:point_id AND pr_del=0";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.Execute();
    if (Qry.Eof || Qry.FieldAsInteger(0) == 0)
    {
        OraSession.Rollback();
        return;
    }

    bool pr_stat = false;
    Qry.SQLText = "SELECT pr_stat FROM trip_sets WHERE point_id=:point_id";
    Qry.Execute();
    if(not Qry.Eof) pr_stat = Qry.FieldAsInteger(0) != 0;

    int count = 0;
    Qry.SQLText = "select count(*) from limited_capability_stat where point_id=:point_id";
    Qry.Execute();
    if(not Qry.Eof) count = Qry.FieldAsInteger(0);

    if(pr_stat and count == 0)
        get_limited_capability_stat(point_id);

    OraSession.Commit();
}

int nosir_lim_capab_stat(int argc,char **argv)
{
    cout << "start time: " << DateTimeToStr(NowUTC(), ServerFormatDateTimeAsString) << endl;
    list<int> point_ids;
    TQuery Qry(&OraSession);
    Qry.SQLText = "select point_id from trip_sets";
    Qry.Execute();
    for(; not Qry.Eof; Qry.Next()) point_ids.push_back(Qry.FieldAsInteger(0));
    OraSession.Rollback();
    cout << point_ids.size() << " points to process." << endl;
    int count = 0;
    for(list<int>::iterator i = point_ids.begin(); i != point_ids.end(); i++, count++) {
        nosir_lim_capab_stat_point(*i);
        if(not (count % 1000))
            cout << count << endl;
    }
    cout << "end time: " << DateTimeToStr(NowUTC(), ServerFormatDateTimeAsString) << endl;
    return 0;
}

int nosir_rfisc_stat(int argc,char **argv)
{
    cout << "start time: " << DateTimeToStr(NowUTC(), ServerFormatDateTimeAsString) << endl;
    list<int> point_ids;
    TQuery Qry(&OraSession);
    Qry.SQLText = "select point_id from trip_sets";
    Qry.Execute();
    for(; not Qry.Eof; Qry.Next()) point_ids.push_back(Qry.FieldAsInteger(0));
    OraSession.Rollback();
    cout << point_ids.size() << " points to process." << endl;
    int count = 0;
    for(list<int>::iterator i = point_ids.begin(); i != point_ids.end(); i++, count++) {
        nosir_rfisc_stat_point(*i);
        cout << count << endl;
    }
    cout << "end time: " << DateTimeToStr(NowUTC(), ServerFormatDateTimeAsString) << endl;
    return 0;
}

void add_stat_time(map<string, long> &stat_times, const string &name, long time)
{
    map<string, long>::iterator i = stat_times.find(name);
    if(i == stat_times.end())
        stat_times[name] = time;
    else
        stat_times[name] += time;
}

void get_flight_stat(map<string, long> &stat_times, int point_id, bool final_collection)
{
   TFlights flightsForLock;
   flightsForLock.Get( point_id, ftTranzit );
   flightsForLock.Lock(__FUNCTION__);

   {
     QParams QryParams;
     QryParams << QParam("point_id", otInteger, point_id);
     QryParams << QParam("final_collection", otInteger, (int)final_collection);
     TCachedQuery Qry("UPDATE trip_sets SET pr_stat=:final_collection WHERE point_id=:point_id AND pr_stat=0", QryParams);
     Qry.get().Execute();
     if (Qry.get().RowsProcessed()<=0) return; //статистику не собираем
   };
   {
     QParams QryParams;
     QryParams << QParam("point_id", otInteger, point_id);
     TCachedQuery Qry("BEGIN "
                      "  statist.get_stat(:point_id); "
                      "  statist.get_trfer_stat(:point_id); "
                      "  statist.get_self_ckin_stat(:point_id); "
                      "END;",
                      QryParams);
     TPerfTimer tm;
     tm.Init();
     Qry.get().Execute();
     add_stat_time(stat_times, "statist", tm.Print());
     tm.Init();
     get_rfisc_stat(point_id);
     add_stat_time(stat_times, "rfisc_stat", tm.Print());
     tm.Init();
     get_service_stat(point_id);
     add_stat_time(stat_times, "service_stat", tm.Print());
     tm.Init();
     get_limited_capability_stat(point_id);
     add_stat_time(stat_times, "limited_capability_stat", tm.Print());
     tm.Init();
     get_kuf_stat(point_id);
     add_stat_time(stat_times, "kuf_stat", tm.Print());
     tm.Init();
     get_pfs_stat(point_id);
     add_stat_time(stat_times, "pfs_stat", tm.Print());
     tm.Init();
     get_trfer_pax_stat(point_id);
     add_stat_time(stat_times, "pfs_stat", tm.Print());
   };

   TReqInfo::Instance()->LocaleToLog("EVT.COLLECT_STATISTIC", evtFlt, point_id);
};

string getCountry(int point_id, TDateTime part_key, TDateTime &scd_in, TDateTime &scd_out, string &airp)
{
    QParams QryParams;
    QryParams << QParam("point_id", otInteger, point_id);
    if(part_key != NoExists) QryParams << QParam("part_key", otDate, part_key);
    string SQL = (string)
            "select cities.country, scd_out, scd_in, airp from "
            "   " + (part_key == NoExists ? "" : "arx_points") + " points, "
            "   airps, "
            "   cities "
            "where "
            "   points.point_id = :point_id and " +
            (part_key == NoExists ? "" : "   points.part_key = :part_key and ") +
            "   points.airp = airps.code and "
            "   airps.city = cities.code ";
    TCachedQuery Qry(SQL, QryParams);
    Qry.get().Execute();
    if(Qry.get().Eof)
        throw Exception("country code not found for point_id = %d", point_id);
    scd_in = Qry.get().FieldAsDateTime("scd_in");
    scd_out = Qry.get().FieldAsDateTime("scd_out");
    airp = Qry.get().FieldAsString("airp");
    return Qry.get().FieldAsString(0);
}

void collect(map<string, int> &result, TDateTime from, TDateTime to)
{
    TPerfTimer tm;
    tm.Init();
    for(int oper = 0; oper < 2; oper++) { // oper == 0 archive tables used, otherwise operating tables
        if(oper)
            cout << "process operating tables" << endl;
        else
            cout << "process archive tables" << endl;
        string pointsSQL = (string)
            "select point_id " +
            (oper ? "" : ", part_key ") +
            "from " +
            (oper ? "points" : "arx_points") +
            " where "
            "   ((scd_out >= :from_date and "
            "   scd_out < :to_date) or "
            "   (scd_in >= :from_date and "
            "   scd_in < :to_date)) and "
            "   airp = :airp";

        string grpSQL = (string)
            "select grp_id, point_dep, point_arv from " +
            (oper ? "pax_grp" : "arx_pax_grp") +
            " where "
            "   (point_dep = :point_id or point_arv = :point_id) and "
            "   status not in ('E') " +
            (oper ? "" : " and part_key = :part_key");

        string paxSQL = (string)
            "select pax_id from " + (oper ? "pax" : "arx_pax") + " where "
            "   grp_id = :grp_id and "
            "   refuse is null " +
            (oper ? "" : " and part_key = :part_key");

        QParams QryParams;
        QryParams
            << QParam("from_date", otDate, from)
            << QParam("to_date", otDate, to)
            << QParam("airp", otString, "ТЛЧ");
        TCachedQuery Qry(pointsSQL, QryParams);
        Qry.get().Execute();
        int flights = 0;
        int pax_count = 0;
        for(; not Qry.get().Eof; Qry.get().Next(), flights++) {
            int point_id = Qry.get().FieldAsInteger("point_id");
            TDateTime part_key = NoExists;
            QParams grpParams;
            grpParams << QParam("point_id", otInteger, point_id);
            if(not oper) {
                part_key = Qry.get().FieldAsDateTime("part_key");
                grpParams << QParam("part_key", otDate, part_key);
            }
            TCachedQuery grpQry(grpSQL, grpParams);
            grpQry.get().Execute();
            for(; not grpQry.get().Eof; grpQry.get().Next()) {
                int grp_id = grpQry.get().FieldAsInteger("grp_id");
                int point_dep = grpQry.get().FieldAsInteger("point_dep");
                int point_arv = grpQry.get().FieldAsInteger("point_arv");
                TDateTime dep_scd_in, dep_scd_out;
                TDateTime arv_scd_in, arv_scd_out;
                string dep_airp, arv_airp;
                if(getCountry(point_dep, part_key, dep_scd_in, dep_scd_out, dep_airp) != "РФ") continue;
                if(getCountry(point_arv, part_key, arv_scd_in, arv_scd_out, arv_airp) != "РФ") continue;
                QParams paxParams;
                paxParams << QParam("grp_id", otInteger, grp_id);
                if(not oper) paxParams << QParam("part_key", otDate, part_key);
                TCachedQuery paxQry(paxSQL, paxParams);
                paxQry.get().Execute();
                for(; not paxQry.get().Eof; paxQry.get().Next()) {
                    int pax_id = paxQry.get().FieldAsInteger("pax_id");
                    CheckIn::TPaxDocItem doc;
                    LoadPaxDoc(part_key, pax_id, doc);
                    if(doc.type != "P") continue;
                    string no_begin = doc.no.substr(0, 2);
                    if(
                            no_begin == "52" or
                            no_begin == "04" or
                            no_begin == "69" or
                            no_begin == "32" or
                            no_begin == "01"
                      ) {
                        cout
                            << setw(10) << pax_id
                            << setw(20) << doc.no
                            << setw(4) << dep_airp
                            << "(" << DateTimeToStr(dep_scd_out, ServerFormatDateTimeAsString) << ")"
                            << " -> "
                            << setw(4) << arv_airp
                            << "(" << DateTimeToStr(arv_scd_in, ServerFormatDateTimeAsString) << ")"
                            << endl;

                        result[no_begin]++;
                        pax_count++;
                        if(pax_count % 10 == 0) cout << pax_count << " pax processed" << endl;
                    }
                }
            }
        }
        if(pax_count % 10 != 0) cout << pax_count << " pax processed" << endl;
        cout << "flights: " << flights << endl;
    }
    cout << "interval time: " << tm.PrintWithMessage() << endl;
}

int STAT::ovb(int argc,char **argv)
{
    TPerfTimer tm;
    tm.Init();
    TDateTime from, to;
    StrToDateTime("01.01.2015 00:00:00","dd.mm.yyyy hh:nn:ss",from);
//    StrToDateTime("01.08.2015 00:00:00","dd.mm.yyyy hh:nn:ss",from);

    QParams QryParams;
    QryParams << QParam("from_date", otDate, from);
    TCachedQuery Qry("select last_day(:from_date) from dual", QryParams);
    map<string, int> result;
    for(int i = 0; i < 6; i++) {
//    for(int i = 0; i < 1; i++) {
        cout << "------ loop " << i << " ------" << endl;
        Qry.get().Execute();
        to = Qry.get().FieldAsDateTime(0) + 1;
        cout << "from: " << DateTimeToStr(from, ServerFormatDateTimeAsString) << endl;
        cout << "to: " << DateTimeToStr(to, ServerFormatDateTimeAsString) << endl;
        collect(result, from, to);
        from = to;
        Qry.get().SetVariable("from_date", from);
    }
    for(map<string, int>::iterator i = result.begin(); i != result.end(); i++) {
        cout << "'" << i->first << "' -> " << i->second << endl;
    }


    cout << "time: " << tm.PrintWithMessage() << endl;
    return 1; // 0 - изменения коммитятся, 1 - rollback
}

int nosir_months(int argc,char **argv)
{
    TDateTime FirstDate, LastDate;
    StrToDateTime("12.04.2014 00:00:00", ServerFormatDateTimeAsString, FirstDate);
    StrToDateTime("13.04.2018 00:00:00", ServerFormatDateTimeAsString, LastDate);
    TPeriods periods;
    periods.get(FirstDate, LastDate);
    periods.dump();
    return 1;
}

int nosir_stat_order(int argc,char **argv)
{
    TQuery Qry(&OraSession);
    Qry.SQLText = "select text from stat_orders_data where file_id = 14167838 order by page_no";
    Qry.Execute();
    string data;
    for(; not Qry.Eof; Qry.Next()) {
        data += Qry.FieldAsString(0);
    }
    ofstream out("decompressed14167838.csv");
    out << Zip::decompress(StrUtils::b64_decode(data));
    ofstream b64("b64_14167838");
    b64 << data;
    return 1;
}

int nosir_md5(int argc,char **argv)
{
    ifstream is("14213060.1508.0005", ifstream::binary);


    if (is) {
        // get length of file:
        is.seekg (0, is.end);
        int length = is.tellg();
        is.seekg (0, is.beg);

        char * buffer = new char [length];

        std::cout << "Reading " << length << " characters... ";
        // read data as a block:
        is.read (buffer,length);

        u_char digest[16];
        //MD5((unsigned char*) buffer, length, digest);

        MD5_CTX c;
        MD5_Init(&c);
        MD5_Update(&c, (unsigned char*) buffer, length);
        MD5_Final(digest, &c);

        /*
        MD5Context ctxt;
        MD5Init(&ctxt);
        MD5Update(&ctxt, (const u_char *)buffer, length);
        u_char digest[16];
        MD5Final(digest, &ctxt);
        */
        ostringstream md5sum;
        for(size_t i = 0; i < sizeof(digest) / sizeof(u_char); i++)
            md5sum << hex << setw(2) << setfill('0') << (int)digest[i];

        if (is)
            std::cout << "all characters read successfully.";
        else
            std::cout << "error: only " << is.gcount() << " could be read";
        is.close();

        // ...buffer contains the entire file...

        delete[] buffer;
    }
    return 1;
}

void departed_flt(TQuery &Qry, TEncodedFileStream &of)
{
    int point_id = Qry.FieldAsInteger("point_id");
    TDateTime part_key = NoExists;

    if(not Qry.FieldIsNULL("part_key"))
        part_key = Qry.FieldAsDateTime("part_key");

    TRegEvents events;
    events.fromDB(part_key, point_id);

    TQuery paxQry(&OraSession);
    paxQry.CreateVariable("point_id", otInteger, point_id);
    if(part_key != NoExists)
        paxQry.CreateVariable("part_key", otDate, part_key);

    string SQLText =
        "select \n"
        "   pax.name, \n"
        "   pax.surname, \n"
        "   pax.ticket_no, \n"
        "   pax.coupon_no, \n"
        "   pax.pax_id, \n"
        "   pax.grp_id, \n"
        "   pax.reg_no, \n"
        "   pax_grp.client_type, \n"
        "   pax_grp.airp_arv, \n";
    if(part_key == NoExists) {
        SQLText +=
            "   (SELECT 1 FROM confirm_print cnf  "
            "   WHERE " OP_TYPE_COND("op_type")" and cnf.pax_id=pax.pax_id AND voucher is null and"
            "   client_type='TERM' AND pr_print<>0 AND rownum<2) AS term_bp, "
            "   salons.get_seat_no(pax.pax_id,pax.seats,NULL,NULL,NULL,'list',NULL,0) AS seat_no, "
            "   NVL(ckin.get_bagAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) bag_amount, \n"
            "   NVL(ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) bag_weight \n";
        paxQry.CreateVariable("op_type", otString, DevOperTypes().encode(TDevOper::PrnBP));
    } else
    SQLText +=
          " NVL(arch.get_bagAmount2(pax.part_key,pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) bag_amount, \n"
          " NVL(arch.get_bagWeight2(pax.part_key,pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) bag_weight \n";
    SQLText +=
        "from \n";
    if(part_key == NoExists)
        SQLText +=
            "   pax, pax_grp \n";
    else
        SQLText +=
            "   arx_pax pax, arx_pax_grp pax_grp \n";
    SQLText +=
        "where \n"
        "   pax_grp.point_dep = :point_id and \n"
        "   pax_grp.grp_id = pax.grp_id \n";
    if(part_key != NoExists)
        SQLText +=
            " and pax.part_key = :part_key and \n"
            " pax_grp.part_key = :part_key \n";

    TQuery pnrQry(&OraSession);
    pnrQry.SQLText =
        "select crs_pnr.pnr_id from "
        "  crs_pnr, crs_pax "
        "where "
        "   crs_pax.pax_id = :pax_id and "
        "   crs_pax.pr_del = 0 and "
        "   crs_pax.pnr_id = crs_pnr.pnr_id ";
    pnrQry.DeclareVariable("pax_id", otInteger);

    string delim = ";";

    vector<TPnrAddrItem> pnrs;

    paxQry.SQLText = SQLText;
    paxQry.Execute();
    TAirpArvInfo airp_arv_info;
    for(; not paxQry.Eof; paxQry.Next()) {
        int grp_id = paxQry.FieldAsInteger("grp_id");
        int reg_no = paxQry.FieldAsInteger("reg_no");
        string name = paxQry.FieldAsString("name");
        string surname = paxQry.FieldAsString("surname");

        string airline = Qry.FieldAsString("airline");
        int flt_no = Qry.FieldAsInteger("flt_no");
        string suffix = Qry.FieldAsString("suffix");
        string airp = Qry.FieldAsString("airp");
        TDateTime scd_out = Qry.FieldAsDateTime("scd_out");
        ostringstream flt_str;
        flt_str
            << airline
            << setw(3) << setfill('0') << flt_no
            << (suffix.empty() ? "" : suffix);

        string ticket_no = paxQry.FieldAsString("ticket_no");
        string coupon_no = paxQry.FieldAsString("coupon_no");
        ostringstream ticket;
        if(not ticket_no.empty())
            ticket << ticket_no << (coupon_no.empty() ? "" : "/") << coupon_no;

        string pnr_addr;
        if(part_key == NoExists) {
            pnrQry.SetVariable("pax_id", paxQry.FieldAsInteger("pax_id"));
            pnrQry.Execute();
            if(not pnrQry.Eof) {
                GetPnrAddr(pnrQry.FieldAsInteger("pnr_id"), pnrs);
                if (!pnrs.empty())
                    pnr_addr.append(pnrs.begin()->addr).append("/").append(pnrs.begin()->airline);
            }
        }


        CheckIn::TPaxDocItem doc;
        LoadPaxDoc(part_key, paxQry.FieldAsInteger("pax_id"), doc);
        string birth_date = (doc.birth_date!=ASTRA::NoExists?DateTimeToStr(doc.birth_date, "dd.mmm.yy"):"");
        string gender = doc.gender;

        of
            // ФИО
            << name << (name.empty() ? "" : " ") << surname << delim
            // Дата рождения
            << birth_date << delim
            // Пол
            << gender << delim
            // Тип документа
            << doc.type << delim
            // Серия и номер документа
            << doc.no << delim
            // Номер билета
            << ticket.str() << delim
            // Номер бронирования
            << pnr_addr << delim
            // Рейс
            << flt_str.str() << delim
            // Дата вылета
            << DateTimeToStr(scd_out, "ddmmm") << delim
            // От
            << airp << delim
            // До
            << airp_arv_info.get(paxQry) << delim
            // Багаж мест
            << paxQry.FieldAsInteger("bag_amount") << delim
            // Багаж вес
            << paxQry.FieldAsInteger("bag_weight") << delim;
        // Время регистрации
        TRegEvents::const_iterator evt = events.find(make_pair(grp_id, reg_no));
        if(evt != events.end())
            of << DateTimeToStr(evt->second.first, "dd.mm.yyyy hh:nn:ss");
        of << delim;
        // Номер места
        if(part_key == NoExists)
            of << paxQry.FieldAsString("seat_no");
        of << delim
            // Способ регистрации
            << paxQry.FieldAsString("client_type") << delim;
        // Печать ПТ на стойке
        if(part_key == NoExists)
            of << (paxQry.FieldAsInteger("term_bp") == 0 ? "НЕТ" : "ДА");
        of << endl;
    }

    /*
       ofstream out(IntToString(part_key == NoExists ? 0 : 1) + "pax1.sql");
       out << SQLText;
       out.close();
       */
}

void departed_month(const pair<TDateTime, TDateTime> &interval, TEncodedFileStream &of)
{
    TQuery Qry(&OraSession);
    Qry.CreateVariable("first_date", otDate, interval.first);
    Qry.CreateVariable("last_date", otDate, interval.second);
    for(int pass = 0; pass <= 2; pass++) {
        if (pass!=0)
            Qry.CreateVariable("arx_trip_date_range", otInteger, ARX_TRIP_DATE_RANGE());
        ostringstream sql;
        sql << "SELECT ";
        if(pass)
            sql << "points.part_key, ";
        else
            sql << "null part_key, ";
        sql << "point_id, airline, flt_no, suffix, airp, scd_out  FROM ";
        if (pass!=0)
        {
            sql << " arx_points points \n";
            if (pass==2)
                sql << ",(SELECT part_key, move_id FROM move_arx_ext \n"
                    "  WHERE part_key >= :last_date+:arx_trip_date_range AND part_key <= :last_date+date_range) arx_ext \n";
        }
        else
            sql << " points \n";
        sql << "WHERE \n";
        if (pass==1)
            sql << " points.part_key >= :first_date AND points.part_key < :last_date + :arx_trip_date_range AND \n";
        if (pass==2)
            sql << " points.part_key=arx_ext.part_key AND points.move_id=arx_ext.move_id AND \n";
        sql <<
            "   scd_out>=:first_date AND scd_out<:last_date AND airline='ЮТ' AND "
            "      pr_reg<>0 AND pr_del>=0";

        /*
           ofstream out(IntToString(pass) + ".sql");
           out << sql.str();
           out.close();
           */

        Qry.SQLText = sql.str().c_str();
        Qry.Execute();
        for(; not Qry.Eof; Qry.Next())
            departed_flt(Qry, of);
    }
}

int nosir_departed_sql(int argc, char **argv)
{
    TQuery Qry(&OraSession);
    Qry.SQLText = "select :part_key part_key from dual";
    Qry.CreateVariable("part_key", otDate, FNull);
    Qry.Execute();
//    departed_flt(Qry);
    Qry.SetVariable("part_key", NowUTC());
    Qry.Execute();
//    departed_flt(Qry);
    return 1;
}

int nosir_departed(int argc, char **argv)
{
    if(argc != 3) {
        cout << "usage: " << argv[0] << " yyyymmdd yyyymmdd" << endl;
        return 1;
    }
    TPerfTimer tm;
    tm.Init();
    TDateTime FirstDate, LastDate;

    if(StrToDateTime(argv[1], "yyyymmdd", FirstDate) == EOF) {
        cout << "wrong first date: " << argv[1] << endl;
        return 1;
    }

    if(StrToDateTime(argv[2], "yyyymmdd", LastDate) == EOF) {
        cout << "wrong last date: " << argv[1] << endl;
        return 1;
    }

    TPeriods period;
    period.get(FirstDate, LastDate);
    string delim = ";";
    for(TPeriods::TItems::iterator i = period.items.begin(); i != period.items.end(); i++) {
        TEncodedFileStream of("cp1251", (string)"departed." + DateTimeToStr(i->first, "yymm") + ".csv");
        of
            << "ФИО" << delim
            << "Дата рождения" << delim
            << "Пол" << delim
            << "Тип документа" << delim
            << "Серия и номер документа" << delim
            << "Номер билета" << delim
            << "Номер бронирования" << delim
            << "Рейс" << delim
            << "Дата вылета" << delim
            << "От" << delim
            << "До" << delim
            << "Багаж мест" << delim
            << "Багаж вес" << delim
            << "Время регистрации (UTC)" << delim
            << "Номер места" << delim
            << "Способ регистрации" << delim
            << "Печать ПТ на стойке" << endl;
        departed_month(*i, of);
    }
    LogTrace(TRACE5) << "time: " << tm.PrintWithMessage();
    return 1;
}

int nosir_departed_pax(int argc, char **argv)
{
    TDateTime FirstDate, LastDate;
    StrToDateTime("01.04.2016 00:00:00", "dd.mm.yyyy hh:nn:ss", FirstDate);
    LastDate = NowUTC();
    TQuery Qry(&OraSession);
    Qry.SQLText =
    "SELECT point_id, airline, flt_no, suffix, airp, scd_out  FROM points "
    "WHERE scd_out>=:FirstDate AND scd_out<:LastDate AND airline='ЮТ' AND "
    "      pr_reg<>0 AND pr_del>=0";
    Qry.CreateVariable("FirstDate", otDate, FirstDate);
    Qry.CreateVariable("LastDate", otDate, LastDate);

    TQuery paxQry(&OraSession);
    paxQry.SQLText =
        "select "
        "   pax.name, "
        "   pax.surname, "
        "   pax.ticket_no, "
        "   pax.coupon_no, "
        "   crs_pnr.pnr_id, "
        "   pax.pax_id "
        "from "
        "   pax, pax_grp, crs_pnr, crs_pax where "
        "   pax.pax_id = crs_pax.pax_id(+) and "
        "   crs_pax.pr_del(+)=0 and "
        "   crs_pax.pnr_id = crs_pnr.pnr_id(+) and "
        "   pax_grp.point_dep = :point_id and "
        "   pax_grp.grp_id = pax.grp_id ";
    paxQry.DeclareVariable("point_id", otInteger);

    Qry.Execute();
    bool pr_header = true;
    string delim = ";";
    ofstream of;
    TTripRoute route;
    vector<TPnrAddrItem> pnrs;
    for(; not Qry.Eof; Qry.Next()) {
        int point_id = Qry.FieldAsInteger("point_id");
        route.GetRouteAfter(NoExists, point_id, trtNotCurrent, trtNotCancelled);
        string airp_arv;
        if(not route.empty())
            airp_arv = route.begin()->airp;

        paxQry.SetVariable("point_id", point_id);
        paxQry.Execute();
        for(; not paxQry.Eof; paxQry.Next()) {
            if(pr_header) {
                pr_header = false;
                of.open("pax_departed.csv");
                of
                    << "ФИО" << delim
                    << "Номер билета" << delim
                    << "Номер бронирования" << delim
                    << "Рейс" << delim
                    << "Дата вылета" << delim
                    << "Направление" << delim
                    << "Дата рождения" << delim
                    << "Пол" << endl;
            }
            string name = paxQry.FieldAsString("name");
            string surname = paxQry.FieldAsString("surname");

            string airline = Qry.FieldAsString("airline");
            int flt_no = Qry.FieldAsInteger("flt_no");
            string suffix = Qry.FieldAsString("suffix");
            string airp = Qry.FieldAsString("airp");
            TDateTime scd_out = Qry.FieldAsDateTime("scd_out");
            ostringstream flt_str;
            flt_str
                << airline
                << setw(3) << setfill('0') << flt_no
                << (suffix.empty() ? "" : suffix);

            string ticket_no = paxQry.FieldAsString("ticket_no");
            string coupon_no = paxQry.FieldAsString("coupon_no");
            ostringstream ticket;
            if(not ticket_no.empty())
                ticket << ticket_no << (coupon_no.empty() ? "" : "/") << coupon_no;

            GetPnrAddr(paxQry.FieldAsInteger("pnr_id"), pnrs);
            string pnr_addr;
            if (!pnrs.empty())
                pnr_addr.append(pnrs.begin()->addr).append("/").append(pnrs.begin()->airline);

            CheckIn::TPaxDocItem doc;
            LoadPaxDoc(paxQry.FieldAsInteger("pax_id"), doc);
            string birth_date = (doc.birth_date!=ASTRA::NoExists?DateTimeToStr(doc.birth_date, "dd.mmm.yy"):"");
            string gender = doc.gender;

            of
                // ФИО
                << name << (name.empty() ? "" : " ") << surname << delim
                // Дата рождения
                << birth_date << delim
                // Пол
                << gender << delim
                // Тип документа
                << delim
                // Серия документа
                << delim
                // Номер документа
                << delim
                // Номер билета
                << ticket.str() << delim
                // Номер бронирования
                << pnr_addr << delim
                // Рейс
                << flt_str.str() << delim
                // Дата вылета
                << DateTimeToStr(scd_out, "ddmmm") << delim
                // От
                << delim
                // До
                << route.begin()->airp << delim;
        }
    }
    return 1;
}


int nosir_seDCSAddReport(int argc, char **argv)
{
    if(argc != 2) {
        cout << "usage: " << argv[0] << " yyyymmdd" << endl;
        return 1;
    }

    TDateTime FirstDate;
    if(StrToDateTime(argv[1], "yyyymmdd", FirstDate) == EOF) {
        cout << "wrong date: " << argv[1] << endl;
        return 1;
    }

    struct TFltStat {
        int col_point_id;
        int col_airline;
        int col_airp;
        int col_flt_no;
        int col_suffix;
        int col_scd_out;

        TCachedQuery fltQry;

        const char *delim;


        typedef map<bool, int> TBagRow;
        typedef map<bool, TBagRow> TWebRow;
        typedef map<bool, TWebRow> TPersRow;
        typedef map<int, TPersRow> TFltData;

        // data[point_id][pr_adult][pr_web][pr_bag] = count;
        TFltData data;

        void data_dump()
        {
            for(TFltData::iterator flt = data.begin(); flt != data.end(); flt++) {
                for(TPersRow::iterator pers = flt->second.begin(); pers != flt->second.end(); pers++) {
                    for(TWebRow::iterator web = pers->second.begin(); web != pers->second.end(); web++) {
                        for(TBagRow::iterator bag = web->second.begin(); bag != web->second.end(); bag++) {
                            LogTrace(TRACE5)
                                << "data["
                                << flt->first
                                << "]["
                                << pers->first
                                << "]["
                                << web->first
                                << "]["
                                << bag->first
                                << "] = "
                                << bag->second;
                        }
                    }
                }
            }
        }

        TFltStat(TQuery &Qry, const char *adelim): fltQry(
                "select "
                "    pax_grp.client_type, "
                "    pax.pers_type, "
                "    nvl2(bag2.grp_id, 1, 0) pr_bag "
                "from "
                "    pax_grp, "
                "    pax, "
                "    bag2 "
                "where "
                "    pax_grp.point_dep = :point_id and "
                "    pax.grp_id = pax_grp.grp_id and "
                "    bag2.grp_id(+) = pax_grp.grp_id and "
                "    bag2.num(+) = 1 ",
                QParams() << QParam("point_id", otInteger)
                ),
        delim(adelim)
        {
            col_point_id = Qry.GetFieldIndex("point_id");
            col_airline = Qry.GetFieldIndex("airline");
            col_airp = Qry.GetFieldIndex("airp");
            col_flt_no = Qry.GetFieldIndex("flt_no");
            col_suffix = Qry.GetFieldIndex("suffix");
            col_scd_out = Qry.GetFieldIndex("scd_out");
        }

        void get(TQuery &Qry, ofstream &of) {
            data.clear();
            int point_id = Qry.FieldAsInteger(col_point_id);
            fltQry.get().SetVariable("point_id", point_id);
            fltQry.get().Execute();
            if(not fltQry.get().Eof) {
                int col_client_type = fltQry.get().GetFieldIndex("client_type");
                int col_pers_type = fltQry.get().GetFieldIndex("pers_type");
                int col_pr_bag = fltQry.get().GetFieldIndex("pr_bag");
                for(; not fltQry.get().Eof; fltQry.get().Next()) {
                    bool pr_adult = DecodePerson(fltQry.get().FieldAsString(col_pers_type)) == adult;
                    bool pr_web = DecodeClientType(fltQry.get().FieldAsString(col_client_type)) != ctTerm;
                    bool pr_bag = fltQry.get().FieldAsInteger(col_pr_bag) != 0;
                    data[point_id][pr_adult][pr_web][pr_bag]++;
                }
            }
            if(not data.empty()) {
                data_dump();
                of
                    //Код аэропорта (города)
                    << Qry.FieldAsString(col_airp) << delim
                    //Перевозчик
                    << Qry.FieldAsString(col_airline) << delim
                    //Номер рейса
                    << Qry.FieldAsString(col_flt_no) << delim
                    //Литера
                    << Qry.FieldAsString(col_suffix) << delim
                    //Дата рейса
                    << DateTimeToStr(Qry.FieldAsDateTime(col_scd_out), "dd.mm.yyyy") << delim
                    //Пассажиры ВЗР с регистрацией в а/п
                    <<
                    data[point_id][true][false][false] +
                    data[point_id][true][false][true]
                    << delim
                    //Пассажиры РБ с регистрацией в а/п
                    <<
                    data[point_id][false][false][false] +
                    data[point_id][false][false][true]
                    << delim
                    //Пассажиры ВЗР с регистрацией в а/п без багажа
                    <<
                    data[point_id][true][false][false]
                    << delim
                    //Пассажиры РБ с регистрацией в а/п без багажа
                    <<
                    data[point_id][false][false][false]
                    << delim
                    //Пассажиры ВЗР с саморегистрацией и багажом
                    <<
                    data[point_id][true][true][true]
                    << delim
                    //Пассажиры РБ с саморегистрацией и багажом
                    <<
                    data[point_id][false][true][true]
                    << delim
                    //Пассажиры ВЗР с саморегистрацией без багажа
                    <<
                    data[point_id][true][true][false]
                    << delim
                    //Пассажиры РБ с саморегистрацией без багажа
                    <<
                    data[point_id][false][true][false]
                    << endl;
            }
        }
    };

    TCachedQuery Qry(
            "select "
            "   point_id, "
            "   airline, "
            "   airp, "
            "   flt_no, "
            "   suffix, "
            "   scd_out "
            "from "
            "   points "
            "where "
            "   scd_out>=:first_date AND scd_out<:last_date AND airline='ЮТ' AND "
            "   pr_reg<>0 AND pr_del>=0 ",
            QParams()
            << QParam("first_date", otDate, FirstDate)
            << QParam("last_date", otDate, FirstDate + 1)
            );

    Qry.get().Execute();
    if(not Qry.get().Eof) {
        const char *delim = ",";
        ofstream of(((string)"seDCSAddReport." + argv[1] + ".csv").c_str());
        of
            << "Код аэропорта (города)" << delim
            << "Перевозчик" << delim
            << "Номер рейса" << delim
            << "Литера" << delim
            << "Дата рейса" << delim
            << "Пассажиры ВЗР с регистрацией в а/п" << delim
            << "Пассажиры РБ с регистрацией в а/п" << delim
            << "Пассажиры ВЗР с регистрацией в а/п без багажа" << delim
            << "Пассажиры РБ с регистрацией в а/п без багажа" << delim
            << "Пассажиры ВЗР с саморегистрацией и багажом" << delim
            << "Пассажиры РБ с саморегистрацией и багажом" << delim
            << "Пассажиры ВЗР с саморегистрацией без багажа" << delim
            << "Пассажиры РБ с саморегистрацией без багажа" << endl;
        TFltStat flt_stat(Qry.get(), delim);
        for(; not Qry.get().Eof; Qry.get().Next())
            flt_stat.get(Qry.get(), of);
    }

    return 1;
}

struct TPaxInfo {
    int reg_no;
    string surname;
    string name;
    TPaxInfo(): reg_no(NoExists) {}
};

void ANNUL_TAGS(TRptParams &rpt_params, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    get_compatible_report_form("annul_tags", reqNode, resNode);
    xmlNodePtr formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr dataSetsNode = NewTextChild(formDataNode, "datasets");
    xmlNodePtr dataSetNode = NewTextChild(dataSetsNode, "v_annul");
    // переменные отчёта
    xmlNodePtr variablesNode = NewTextChild(formDataNode, "variables");
    PaxListVars(rpt_params.point_id, rpt_params, variablesNode);
    // заголовок отчёта
    NewTextChild(variablesNode, "caption",
            getLocaleText("CAP.DOC.ANNUL_TAGS", LParams() << LParam("flight", get_flight(variablesNode)), rpt_params.GetLang()));
    populate_doc_cap(variablesNode, rpt_params.GetLang());

    NewTextChild(variablesNode, "doc_cap_annul_reg_no", getLocaleText("№"));
    NewTextChild(variablesNode, "doc_cap_annul_fio", getLocaleText("Ф.И.О."));
    NewTextChild(variablesNode, "doc_cap_annul_no", getLocaleText("№№ баг. бирок"));
    NewTextChild(variablesNode, "doc_cap_annul_weight", getLocaleText("БГ вес"));
    NewTextChild(variablesNode, "doc_cap_annul_bag_type", getLocaleText("Тип багажа/RFISC"));
    NewTextChild(variablesNode, "doc_cap_annul_trfer", getLocaleText("Трфр"));
    NewTextChild(variablesNode, "doc_cap_annul_trfer_dir", getLocaleText("До трфр"));

    TStatParams params;
    TPrintAirline airline;
    TAnnulBTStat AnnulBTStat;
    RunAnnulBTStat(params, AnnulBTStat, airline, rpt_params.point_id);

    TCachedQuery paxQry("select reg_no, name, surname from pax where pax_id = :pax_id",
            QParams() << QParam("pax_id", otInteger));

    map<int, TPaxInfo> pax_map;

    for(list<TAnnulBTStatRow>::const_iterator i = AnnulBTStat.rows.begin(); i != AnnulBTStat.rows.end(); i++) {
        xmlNodePtr rowNode = NewTextChild(dataSetNode, "row");

        map<int, TPaxInfo>::iterator iPax = pax_map.find(i->pax_id);

        if(iPax == pax_map.end()) {
            TPaxInfo pax;
            if(i->pax_id != NoExists) {
                paxQry.get().SetVariable("pax_id", i->pax_id);
                paxQry.get().Execute();
                if(not paxQry.get().Eof) {
                    pax.reg_no = paxQry.get().FieldAsInteger("reg_no");
                    pax.name = paxQry.get().FieldAsString("name");
                    pax.surname = paxQry.get().FieldAsString("surname");
                }
            }
            pair<map<int, TPaxInfo>::iterator, bool> ret =
                pax_map.insert(make_pair(i->pax_id, pax));
            iPax = ret.first;
        }

        //  Рег№
        if(iPax->second.reg_no == NoExists)
            NewTextChild(rowNode, "reg_no");
        else
            NewTextChild(rowNode, "reg_no", iPax->second.reg_no);
        //  пассажира ФИО
        ostringstream buf;
        if(iPax->second.reg_no != NoExists)
            buf
                << transliter(iPax->second.surname, 1, rpt_params.GetLang() != AstraLocale::LANG_RU) << " "
                << transliter(iPax->second.name, 1, rpt_params.GetLang() != AstraLocale::LANG_RU);
        NewTextChild(rowNode, "fio", buf.str());
        //  номер бирки
        NewTextChild(rowNode, "no", get_tag_range(i->tags, LANG_EN));
        //  значение по весу
        if (i->weight != NoExists)
            NewTextChild(rowNode, "weight", i->weight);
        else
            NewTextChild(rowNode, "weight");

        //  тип багажа
        buf.str("");
        if(not i->rfisc.empty())
            buf << i->rfisc;
        else if(i->bag_type != NoExists)
            buf << ElemIdToNameLong(etBagType, i->bag_type);
        NewTextChild(rowNode, "bag_type", buf.str());

        if(i->trfer_airline.empty()) {
            //  призн.трансфера
            NewTextChild(rowNode, "pr_trfer", getLocaleText("НЕТ"));
            //  направление трфр
            NewTextChild(rowNode, "trfer_airp_arv");
        } else {
            NewTextChild(rowNode, "pr_trfer", getLocaleText("ДА"));
            NewTextChild(rowNode, "trfer_airp_arv", rpt_params.ElemIdToReportElem(etAirp, i->trfer_airp_arv, efmtCodeNative));
        }
    }
    //LogTrace(TRACE5) << GetXMLDocText(resNode->doc);
}
