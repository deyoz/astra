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
#include "astra_misc.h"
#include "term_version.h"
#include "passenger.h"
#include "points.h"
#include "qrys.h"
#include "tlg/tlg.h"
#include "astra_elem_utils.h"
#include "baggage_pc.h"
#include "astra_elems.h"
#include "serverlib/xml_stuff.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

#define MAX_STAT_ROWS 2000
#define WITHOUT_TOTAL_WHEN_PROBLEM false

using namespace std;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace BASIC;

const string SYSTEM_USER = "���⥬�";

const int arx_trip_date_range=5;

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
                      sortSlashedInt}; // �� ��᫥����⥫쭮���, ���� N.../N.../N...

void GetSystemLogAgentSQL(TQuery &Qry)
{
    string SQLText =
        "select -1, null agent, -1 view_order from dual "
        "union "
        "select 0, '";
    if(TReqInfo::Instance()->desk.compatible(ARX_MODULE_LST_VERSION))
        SQLText += getLocaleText(SYSTEM_USER);
    else
        SQLText += SYSTEM_USER;
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
    if(TReqInfo::Instance()->desk.compatible(ARX_MODULE_LST_VERSION))
        SQLText += getLocaleText(SYSTEM_USER);
    else
        SQLText += SYSTEM_USER;
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
    if(TReqInfo::Instance()->desk.compatible(ARX_MODULE_LST_VERSION))
        SQLText += getLocaleText(SYSTEM_USER);
    else
        SQLText += SYSTEM_USER;
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
                    if(count >= MAX_STAT_ROWS) {
                        AstraLocale::showErrorMessage("MSG.TOO_MANY_FLIGHTS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_SEARCH",
                                LParams() << LParam("num", MAX_STAT_ROWS));
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
    //�訡�� ���
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
      //ࠧ��� ����஢ ᮢ������
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
        if(TReqInfo::Instance()->desk.compatible(ARX_MODULE_LST_VERSION)) {
                NewTextChild(fNode, "key", Qry.FieldAsInteger(0));
                NewTextChild(fNode, "value", getLocaleText(Qry.FieldAsString(1)));
        } else {
            NewTextChild(fNode, "key", 0);
            NewTextChild(fNode, "value", Qry.FieldAsString(1));
        }
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
    NewTextChild(variablesNode, "report_title", getLocaleText("��ୠ� ����� ३�"));
    TQuery Qry(&OraSession);
    int count = 0;

    xmlNodePtr paxLogNode = NewTextChild(resNode, "PaxLog");
    xmlNodePtr headerNode = NewTextChild(paxLogNode, "header");
    NewTextChild(headerNode, "col", "�����"); // ��� ᮢ���⨬��� � ��ன ���ᨥ� �ନ����

    Qry.Clear();
    string SQLQuery;
    string airline;
    if (part_key == NoExists) {
        {
            TQuery Qry(&OraSession);
            Qry.SQLText = "select airline from points where point_id = :point_id "; // pr_del>=0 - �� ���� �.�. ����� ��ᬠ�ਢ��� 㤠����� ३��
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
                "where part_key = :part_key and point_id = :point_id "; // pr_del >= 0 - �� ����, �.�. � ��娢� ��� 㤠������ ३ᮢ
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
    NewTextChild(variablesNode, "report_title", getLocaleText("��ୠ� ����権 ३�"));
    TQuery Qry(&OraSession);
    int count = 0;

    xmlNodePtr paxLogNode = NewTextChild(resNode, "PaxLog");
    xmlNodePtr headerNode = NewTextChild(paxLogNode, "header");
    NewTextChild(headerNode, "col", "�����"); // ��� ᮢ���⨬��� � ��ன ���ᨥ� �ନ����

    Qry.Clear();
    string qry1, qry2;
    int move_id = 0;
    string airline;
    if (part_key == NoExists) {
        {
            TQuery Qry(&OraSession);
            Qry.SQLText = "select move_id, airline from points where point_id = :point_id "; // pr_del>=0 - �� ���� �.�. ����� ��ᬠ�ਢ��� 㤠����� ३��
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
                "where part_key = :part_key and point_id = :point_id "; // pr_del >= 0 - �� ����, �.�. � ��娢� ��� 㤠������ ३ᮢ
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
    NewTextChild(variablesNode, "report_title", getLocaleText("����樨 �� ���ᠦ���"));
    TReqInfo *reqInfo = TReqInfo::Instance();
    TQuery Qry(&OraSession);
    int count = 0;

    xmlNodePtr paxLogNode = NewTextChild(resNode, "PaxLog");
    xmlNodePtr headerNode = NewTextChild(paxLogNode, "header");
    NewTextChild(headerNode, "col", "�����"); // ��� ᮢ���⨬��� � ��ன ���ᨥ� �ନ����
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
            if(count >= MAX_STAT_ROWS) {
                AstraLocale::showErrorMessage("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_SEARCH",
                        LParams() << LParam("num", MAX_STAT_ROWS));
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
    NewTextChild(variablesNode, "report_title", getLocaleText("����樨 � ��⥬�"));
    string module;

    TQuery Qry(&OraSession);
    if(TReqInfo::Instance()->desk.compatible(ARX_MODULE_LST_VERSION)) {
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
    } else {
        module = NodeAsString("module", reqNode);
        TQuery Qry(&OraSession);
        Qry.SQLText = "select exe from screen where name = :module";
        Qry.CreateVariable("module", otString, module);
        Qry.Execute();
        if(!Qry.Eof) module = Qry.FieldAsString("exe");
    }

    string agent, station;
    if(TReqInfo::Instance()->desk.compatible(ARX_MODULE_LST_VERSION)) {
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
    } else {
        agent = NodeAsString("agent", reqNode);
        station = NodeAsString("station", reqNode);
    }

    ProgTrace(TRACE5, "module: '%s'", module.c_str());
    ProgTrace(TRACE5, "agent: '%s'", agent.c_str());
    ProgTrace(TRACE5, "station: '%s'", station.c_str());

    int count = 0;

    xmlNodePtr paxLogNode = NewTextChild(resNode, "PaxLog");
    xmlNodePtr headerNode = NewTextChild(paxLogNode, "header");
    NewTextChild(headerNode, "col", "�����"); // ��� ᮢ���⨬��� � ��ன ���ᨥ� �ନ����

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
                "  arx_events.part_key >= :FirstDate - 10 and " // time � part_key �� ᮢ������ ���
                "  arx_events.part_key < :LastDate + 10 and "   // ࠧ��� ⨯�� ᮡ�⨩
                "  (arx_events.lang = :lang OR arx_events.lang = :lang_undef) and "
                "  arx_events.time >= :FirstDate and "         // ���⮬� ��� part_key ��६ ����訩 �������� time
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
                            TripItems[point_id]; // �����뢠�� ������ ��ப� ��� ������� point_id
                        else {
                            if(tripQry.FieldIsNULL("flt_no")) { // �᫨ ��� ���� �� ������ ३�, ��祣� �� �뢮���
                                TripItems[point_id]; // �����뢠�� ������ ��ப� ��� ������� point_id
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
                if(count >= MAX_STAT_ROWS) {
                    AstraLocale::showErrorMessage("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_SEARCH",
                            LParams() << LParam("num", MAX_STAT_ROWS));
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

void UnaccompListToXML(TQuery &Qry, xmlNodePtr resNode, PieceConcept::TNodeList &pcNodeList, bool isPaxSearch, int pass, int &count)
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
      NewTextChild(paxNode, "full_name", getLocaleText("����� ��� ᮯ஢�������"));
      NewTextChild(paxNode, "bag_amount", Qry.FieldAsInteger(col_bag_amount));
      NewTextChild(paxNode, "bag_weight", Qry.FieldAsInteger(col_bag_weight));
      NewTextChild(paxNode, "rk_weight", Qry.FieldAsInteger(col_rk_weight));

      xmlNodePtr excessNode =  NewTextChild(paxNode, "excess", Qry.FieldAsInteger(col_excess));;
      pcNodeList.set_concept(excessNode,  Qry.FieldAsInteger(col_piece_concept));

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
        if(count >= MAX_STAT_ROWS) {
            AstraLocale::showErrorMessage("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_SEARCH",
                    LParams() << LParam("num", MAX_STAT_ROWS));
            break;
        }
      };
  }
  ProgTrace(TRACE5, "XML%d: %s", pass, tm.PrintWithMessage().c_str());
};

void PaxListToXML(TQuery &Qry, xmlNodePtr resNode, PieceConcept::TNodeList& pcNodeList, bool isPaxSearch, int pass, int &count)
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
      pcNodeList.set_concept(excessNode,  Qry.FieldAsInteger(col_piece_concept));

      NewTextChild(paxNode, "grp_id", Qry.FieldAsInteger(col_grp_id));
      NewTextChild(paxNode, "airp_arv", ElemIdToCodeNative(etAirp, Qry.FieldAsString(col_airp_arv)));
      NewTextChild(paxNode, "tags", Qry.FieldAsString(col_tags));
      string status;
      if (DecodePaxStatus(Qry.FieldAsString(col_status))!=psCrew)
      {
        if(Qry.FieldIsNULL(col_refuse))
            status = getLocaleText(Qry.FieldAsInteger(col_pr_brd) == 0 ? "��ॣ." : "��ᠦ.");
      }
      else
      {
        status = getLocaleText("������");
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
        if(count >= MAX_STAT_ROWS) {
            AstraLocale::showErrorMessage("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_SEARCH",
                    LParams() << LParam("num", MAX_STAT_ROWS));
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
                "   salons.get_seat_no(pax.pax_id, pax.seats, pax_grp.status, pax_grp.point_dep, 'seats', rownum) seat_no, "
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
        PieceConcept::TNodeList pcNodeList;
        PaxListToXML(Qry, resNode, pcNodeList, false, 0, count);

        ProgTrace(TRACE5, "XML: %s", tm.PrintWithMessage().c_str());

        //��ᮯ஢������� �����
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

        UnaccompListToXML(Qry, resNode, pcNodeList, false, 0, count);


        xmlNodePtr paxListNode = GetNode("paxList", resNode);
        if(paxListNode!=NULL) { // ��� ᮢ���⨬��� � ��ன ���ᨥ� �ନ����
            xmlNodePtr headerNode = NewTextChild(paxListNode, "header");
            NewTextChild(headerNode, "col", "����");
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
    NewTextChild(variablesNode, "use_seances", false); //!!!��⮬ ����
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
    statRFISC
};

enum TSeanceType { seanceAirline, seanceAirport, seanceAll };

struct TStatParams {
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
    bool skip_rows;
    void get(xmlNodePtr resNode);
};

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

void TStatParams::get(xmlNodePtr reqNode)
{
    string name = NodeAsString("stat_mode", reqNode);
    string type = NodeAsString("stat_type", reqNode, "����");

    if(type == "����") {
        if(name == "���஡���") statType=statFull;
        else
            if(name == "����") statType=statShort;
            else
                if(name == "��⠫���஢�����") statType=statDetail;
                else
                    if(name == "�࠭���") statType=statTrferFull;
                    else throw Exception("Unknown stat mode " + name);
    } else if(type ==
            ((TReqInfo::Instance()->client_type==ctHTTP ||
              TReqInfo::Instance()->desk.compatible(SELF_CKIN_STAT_VERSION)) ?
             "����ॣ������" :
             "�� ���᪠�"
            )
            ) {
        if(name == "���஡���")
            statType=statSelfCkinFull;
        else if(name == "����")
            statType=statSelfCkinShort;
        else if(name == "��⠫���஢�����")
            statType=statSelfCkinDetail;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "�� ����⠬") {
        if(name == "���஡���")
            statType=statAgentFull;
        else if(name == "����")
            statType=statAgentShort;
        else if(name == "�⮣�")
            statType=statAgentTotal;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "���. ⥫��ࠬ��") {
        if(name == "���஡���")
            statType=statTlgOutFull;
        else if(name == "����")
            statType=statTlgOutShort;
        else if(name == "��⠫���஢�����")
            statType=statTlgOutDetail;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "�������") {
        if(name == "����")
            statType=statPactShort;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "������� RFISC") {
        if(name == "���஡���")
            statType=statRFISC;
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

    //ᥠ��� (��������)
    string seance_str = NodeAsStringFast("seance", curNode, "");
    seance = seanceAll;
    if (seance_str=="��") seance=seanceAirline;
    if (seance_str=="��") seance=seanceAirport;

    bool all_seances_permit = info.user.access.rights().permitted(615);

    if (info.desk.compatible(AIRL_AIRP_STAT_VERSION) or
        info.client_type==ctHTTP)
    {
        if (info.user.user_type != utSupport && !all_seances_permit)
        {
            if (info.user.user_type == utAirline)
                seance=seanceAirline;
            else
                seance=seanceAirport;
        };
    }
    else
    {
        if (!ak.empty()) seance=seanceAirline;
        if (!ap.empty()) seance=seanceAirport;
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
};

struct TInetStat {
    int web;
    int kiosk;
    int mobile;
    int web_bp, kiosk_bp;   // ���-�� ���ᠦ�஢ �� ��� � ���᪠, ����� �ᯥ�⠫� ��ᠤ��� �� �⮩��
    int web_bag, kiosk_bag; //        ___,,____                 , ����� ��ॣ. ����� �� �⮩��
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
        SetProp(colNode, "align", taRightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", web);

        colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", taRightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", web_bag);

        colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", taRightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", web_bp);

        // ���᪨
        colNode = NewTextChild(headerNode, "col", getLocaleText("���᪨"));
        SetProp(colNode, "width", 40);
        SetProp(colNode, "align", taRightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", kiosk);

        colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", taRightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", kiosk_bag);

        colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", taRightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", kiosk_bp);

        // ���.
        colNode = NewTextChild(headerNode, "col", getLocaleText("���."));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", taRightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", mobile);

        colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", taRightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", mobile_bag);

        colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", taRightJustify);
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

struct TRFISCStatRow {
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
};

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
                result << excess << getLocaleText("��");
            if(excess_pcs != 0) {
                if(not result.str().empty())
                    result << " + ";
                result << excess_pcs << getLocaleText("�");
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
    string airp; // � ���஢�� �� ������, �㦥� ��� AirpTZRegion
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


template <class keyClass, class rowClass, class cmpClass>
void AddStatRow(const keyClass &key, const rowClass &row, map<keyClass, rowClass, cmpClass> &stat)
{
  typename map< keyClass, rowClass, cmpClass >::iterator i = stat.find(key);
  if (i!=stat.end())
    i->second+=row;
  else
  {
    if (stat.size()<=MAX_STAT_ROWS)
      stat.insert(make_pair(key,row));
  };
};

void GetDetailStat(const TStatParams &params, TQuery &Qry,
                   TDetailStat &DetailStat, TDetailStatRow &DetailStatTotal,
                   TPrintAirline &airline, string pact_descr = "")
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

      AddStatRow(key, row, DetailStat);
    }
    else
    {
      DetailStatTotal+=row;
    };
  }
};

void GetFullStat(const TStatParams &params, TQuery &Qry,
                 TFullStat &FullStat, TFullStatRow &FullStatTotal,
                 TPrintAirline &airline)
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
          AddStatRow(key, row, FullStat);
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
    vector<string> airlines; // ���᮪ ��, ����� �᪫�砥��� �� �� �������.
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
          ) {// ���� �ࠩ �� ������� �����, ࠧ������ �� ������� �� 2 ��� � ������塞 �� � ᯨ᮪ �᪫�祭��� ��� �⮣� �� �������
            TPact new_pact = *iv;
            new_pact.first_date = airline_pact.first_date;
            iv->last_date = airline_pact.first_date;
            new_pact.airlines.push_back(airline_pact.airline);
            added_pacts.push_back(new_pact);
        } if(
                airline_pact.last_date >= iv->first_date and
                airline_pact.last_date < iv->last_date and
                airline_pact.first_date < iv->first_date
          ) { // �ࠢ� �ࠩ �� ������� �����
            TPact new_pact = *iv;
            new_pact.last_date = airline_pact.last_date;
            iv->first_date = airline_pact.last_date;
            new_pact.airlines.push_back(airline_pact.airline);
            added_pacts.push_back(new_pact);
        } if(
                airline_pact.first_date >= iv->first_date and
                airline_pact.last_date < iv->last_date
          ) { // �� ������� 楫���� ����� ��ਮ�� �� �������, ࠧ������ �� ������� �� 3 ���.
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
          ) { // �� ������� 楫���� ����� �� �������, ���� ������塞 � ᯨ᮪ �᪫�砥��� �� �⮣� ���. ⥪����.
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
          if(rows >= MAX_STAT_ROWS) {
              AstraLocale::showErrorMessage("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH",
                      LParams() << LParam("num", MAX_STAT_ROWS));
              if (WITHOUT_TOTAL_WHEN_PROBLEM) showTotal=false; //�� �㤥� �����뢠�� �⮣���� ��ப� ���� �� ����� � ����㦤����
              break;
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
            colNode = NewTextChild(headerNode, "col", getLocaleText("��� �/�"));
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", taLeftJustify);
            SetProp(colNode, "sort", sortString);
            NewTextChild(rowNode, "col", getLocaleText("�⮣�:"));
            if (params.statType==statDetail)
            {
                colNode = NewTextChild(headerNode, "col", getLocaleText("��� �/�"));
                SetProp(colNode, "width", 50);
                SetProp(colNode, "align", taLeftJustify);
                SetProp(colNode, "sort", sortString);
                NewTextChild(rowNode, "col");
            };
        } else {
            colNode = NewTextChild(headerNode, "col", getLocaleText("��� �/�"));
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", taLeftJustify);
            SetProp(colNode, "sort", sortString);
            NewTextChild(rowNode, "col", getLocaleText("�⮣�:"));
            if (params.statType==statDetail)
            {
                colNode = NewTextChild(headerNode, "col", getLocaleText("��� �/�"));
                SetProp(colNode, "width", 50);
                SetProp(colNode, "align", taLeftJustify);
                SetProp(colNode, "sort", sortString);
                NewTextChild(rowNode, "col");
            };
        }
    }

    if(params.statType != statPactShort) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("���-�� ३ᮢ"));
        SetProp(colNode, "width", 85);
        SetProp(colNode, "align", taRightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", (int)(pr_pact?total.flts.size():total.flt_amount));
    }

    if(params.statType == statPactShort)
    {
        colNode = NewTextChild(headerNode, "col", getLocaleText("� �������"));
        SetProp(colNode, "width", 230);
        SetProp(colNode, "align", taLeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col", getLocaleText("�⮣�:"));
    }

    colNode = NewTextChild(headerNode, "col", getLocaleText("���-�� ����."));
    SetProp(colNode, "width", 85);
    SetProp(colNode, "align", taRightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.pax_amount);

    if(params.statType != statPactShort) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("�"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", taRightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.f);

        colNode = NewTextChild(headerNode, "col", getLocaleText("�"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", taRightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.c);

        colNode = NewTextChild(headerNode, "col", getLocaleText("�"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", taRightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.y);

        total.i_stat.toXML(headerNode, rowNode);
    }

    if(pr_pact and params.statType != statPactShort)
    {
        colNode = NewTextChild(headerNode, "col", getLocaleText("� �������"));
        SetProp(colNode, "width", 230);
        SetProp(colNode, "align", taLeftJustify);
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
        NewTextChild(variablesNode, "stat_mode", getLocaleText("�������� �� �ᯮ�짮����� DCS �����"));
        string stat_type_caption;
        switch(params.statType) {
            case statPactShort:
                stat_type_caption = getLocaleText("����");
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
          if(rows >= MAX_STAT_ROWS) {
              AstraLocale::showErrorMessage("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH",
                                            LParams() << LParam("num", MAX_STAT_ROWS));
              if (WITHOUT_TOTAL_WHEN_PROBLEM) showTotal=false; //�� �㤥� �����뢠�� �⮣���� ��ப� ���� �� ����� � ����㦤����
              break;
          }
          //region ��易⥫쭮 � ��砫� 横��, ���� �㤥� �ᯮ�祭 xml
          string region;
          try
          {
              region = AirpTZRegion(im->first.airp);
          }
          catch(AstraLocale::UserException &E)
          {
              AstraLocale::showErrorMessage("MSG.ERR_MSG.NOT_ALL_FLIGHTS_ARE_SHOWN", LParams() << LParam("msg", getLocaleText(E.getLexemaData())));
              if (WITHOUT_TOTAL_WHEN_PROBLEM) showTotal=false; //�� �㤥� �����뢠�� �⮣���� ��ப� ���� �� ����� � ����㦤����
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
        colNode = NewTextChild(headerNode, "col", getLocaleText("��� �/�"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", taLeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col", getLocaleText("�⮣�:"));

        colNode = NewTextChild(headerNode, "col", getLocaleText("��� �/�"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", taLeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col");
    } else {
        colNode = NewTextChild(headerNode, "col", getLocaleText("��� �/�"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", taLeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col", getLocaleText("�⮣�:"));

        colNode = NewTextChild(headerNode, "col", getLocaleText("��� �/�"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", taLeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col");
    }
    colNode = NewTextChild(headerNode, "col", getLocaleText("����� ३�"));
    SetProp(colNode, "width", 75);
    SetProp(colNode, "align", taRightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col");

    colNode = NewTextChild(headerNode, "col", getLocaleText("���"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", taLeftJustify);
    SetProp(colNode, "sort", sortDate);
    NewTextChild(rowNode, "col");

    colNode = NewTextChild(headerNode, "col", getLocaleText("���ࠢ�����"));
    SetProp(colNode, "width", 90);
    SetProp(colNode, "align", taLeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col");

    colNode = NewTextChild(headerNode, "col", getLocaleText("���-�� ����."));
    SetProp(colNode, "width", 75);
    SetProp(colNode, "align", taRightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.pax_amount);

    if (params.statType==statFull)
        total.i_stat.toXML(headerNode, rowNode);

    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", taRightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.adult);

    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", taRightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.child);

    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", taRightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.baby);

    colNode = NewTextChild(headerNode, "col", getLocaleText("�/����� (���)"));
    SetProp(colNode, "width", 80);
    SetProp(colNode, "align", taRightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.rk_weight);

    colNode = NewTextChild(headerNode, "col", getLocaleText("�� ����"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", taRightJustify);
    SetProp(colNode, "sort", sortIntegerSlashInteger);
    NewTextChild(rowNode, "col", total.bag_amount);

    colNode = NewTextChild(headerNode, "col", getLocaleText("�� ���"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", taRightJustify);
    SetProp(colNode, "sort", sortIntegerSlashInteger);
    NewTextChild(rowNode, "col", total.bag_weight);

    colNode = NewTextChild(headerNode, "col", getLocaleText("��.�"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", taRightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col",  total.paid.excess_pcs);

    colNode = NewTextChild(headerNode, "col", getLocaleText("��.���"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", taRightJustify);
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
      NewTextChild(variablesNode, "caption", getLocaleText("���஡��� ᢮���"));
    else
      NewTextChild(variablesNode, "caption", getLocaleText("�࠭��ୠ� ᢮���"));
};

void RunPactDetailStat(const TStatParams &params,
                       TDetailStat &DetailStat, TDetailStatRow &DetailStatTotal,
                       TPrintAirline &prn_airline)
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

                    AddStatRow(key, row, DetailStat);
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
                   TPrintAirline &airline)
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
          //横� �� ��������
          if (params.airlines.elems_permit())
          {
            for(set<string>::const_iterator i=params.airlines.elems().begin();
                                            i!=params.airlines.elems().end(); i++)
            {
              Qry.SetVariable("ak",*i);
              GetDetailStat(params, Qry, DetailStat, DetailStatTotal, airline);
            };
          };
          continue;
        };

        if (params.seance==seanceAirport)
        {
          //横� �� ���⠬
          if (params.airps.elems_permit())
          {
            for(set<string>::const_iterator i=params.airps.elems().begin();
                                            i!=params.airps.elems().end(); i++)
            {
              Qry.SetVariable("ap",*i);
              GetDetailStat(params, Qry, DetailStat, DetailStatTotal, airline);
            };
          };
          continue;
        };
        GetDetailStat(params, Qry, DetailStat, DetailStatTotal, airline);
    }
};

void RunFullStat(const TStatParams &params,
                 TFullStat &FullStat, TFullStatRow &FullStatTotal,
                 TPrintAirline &airline)
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
          //横� �� ��������
          if (params.airlines.elems_permit())
          {
            for(set<string>::const_iterator i=params.airlines.elems().begin();
                                            i!=params.airlines.elems().end(); i++)
            {
              Qry.SetVariable("ak",*i);
              GetFullStat(params, Qry, FullStat, FullStatTotal, airline);
            };
          };
          continue;
        };

        if (params.seance==seanceAirport)
        {
          //横� �� ���⠬
          if (params.airps.elems_permit())
          {
            for(set<string>::const_iterator i=params.airps.elems().begin();
                                            i!=params.airps.elems().end(); i++)
            {
              Qry.SetVariable("ap",*i);
              GetFullStat(params, Qry, FullStat, FullStatTotal, airline);
            };
          };
          continue;
        };
        GetFullStat(params, Qry, FullStat, FullStatTotal, airline);
    }
};



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
                  TPrintAirline &prn_airline)
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

                AddStatRow(key, row, SelfCkinStat);
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
        if(rows >= MAX_STAT_ROWS) {
            AstraLocale::showErrorMessage("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH",
                    LParams() << LParam("num", MAX_STAT_ROWS));
            if (WITHOUT_TOTAL_WHEN_PROBLEM) showTotal=false; //�� �㤥� �����뢠�� �⮣���� ��ப� ���� �� ����� � ����㦤����
            break;
        }
        //region ��易⥫쭮 � ��砫� 横��, ���� �㤥� �ᯮ�祭 xml
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
                if (WITHOUT_TOTAL_WHEN_PROBLEM) showTotal=false; //�� �㤥� �����뢠�� �⮣���� ��ப� ���� �� ����� � ����㦤����
                continue;
            };
        };

        rowNode = NewTextChild(rowsNode, "row");
        // ��� ॣ.
        NewTextChild(rowNode, "col", im->first.client_type);
        // ����
        NewTextChild(rowNode, "col", im->first.desk);
        // �/� ����
        NewTextChild(rowNode, "col", im->first.desk_airp);
        // �ਬ�砭��
        if(params.statType == statSelfCkinFull)
            NewTextChild(rowNode, "col", im->first.descr);
        // ��� �/�
        NewTextChild(rowNode, "col", im->first.ak);
        if(
                params.statType == statSelfCkinDetail or
                params.statType == statSelfCkinFull
          )
            // ��� �/�
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, im->first.ap));
        if(
                params.statType == statSelfCkinShort or
                params.statType == statSelfCkinDetail
          ) {
            // ���-�� ३ᮢ
            NewTextChild(rowNode, "col", (int)im->second.flts.size());
            flts_total += im->second.flts.size();
        }
        if(params.statType == statSelfCkinFull) {
            // ����� ३�
            ostringstream buf;
            buf << setw(3) << setfill('0') << im->first.flt_no;
            NewTextChild(rowNode, "col", buf.str().c_str());
            // ���
            NewTextChild(rowNode, "col", DateTimeToStr(
                        UTCToClient(im->first.scd_out, region), "dd.mm.yy")
                    );
            // ���ࠢ�����
            NewTextChild(rowNode, "col", im->first.places.get());
        }
        // ���-�� ����.
        NewTextChild(rowNode, "col", im->second.pax_amount);
        NewTextChild(rowNode, "col", im->second.term_bag);
        NewTextChild(rowNode, "col", im->second.term_bp);
        NewTextChild(rowNode, "col", im->second.pax_amount - im->second.term_ckin_service);

        if(params.statType == statSelfCkinFull) {
            // ��
            NewTextChild(rowNode, "col", im->second.adult);
            // ��
            NewTextChild(rowNode, "col", im->second.child);
            // ��
            NewTextChild(rowNode, "col", im->second.baby);
        }
        if(
                params.statType == statSelfCkinDetail or
                params.statType == statSelfCkinFull
          )
            // �����. ॣ.
            NewTextChild(rowNode, "col", im->second.tckin);
        if(
                params.statType == statSelfCkinShort or
                params.statType == statSelfCkinDetail
          )
            // �ਬ�砭��
            NewTextChild(rowNode, "col", im->first.descr);

        total += im->second;
    };

    rowNode = NewTextChild(rowsNode, "row");

    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", getLocaleText("��� ॣ."));
    SetProp(colNode, "width", 75);
    SetProp(colNode, "align", taLeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col", getLocaleText("�⮣�:"));
    colNode = NewTextChild(headerNode, "col", getLocaleText("����"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", taLeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col");
    colNode = NewTextChild(headerNode, "col", getLocaleText("�� ����"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", taLeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col");
    if(params.statType == statSelfCkinFull) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("�ਬ�砭��"));
        SetProp(colNode, "width", 280);
        SetProp(colNode, "align", taLeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col");
    }
    colNode = NewTextChild(headerNode, "col", getLocaleText("��� �/�"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", taLeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col");
    if(
            params.statType == statSelfCkinDetail or
            params.statType == statSelfCkinFull
      ) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("��� �/�"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", taLeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col");
    }
    if(
            params.statType == statSelfCkinShort or
            params.statType == statSelfCkinDetail
      ) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("���-�� ३ᮢ"));
        SetProp(colNode, "width", 85);
        SetProp(colNode, "align", taRightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", flts_total);
    }
    if(params.statType == statSelfCkinFull) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("����� ३�"));
        SetProp(colNode, "width", 75);
        SetProp(colNode, "align", taRightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col");
        colNode = NewTextChild(headerNode, "col", getLocaleText("���"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", taLeftJustify);
        SetProp(colNode, "sort", sortDate);
        NewTextChild(rowNode, "col");
        colNode = NewTextChild(headerNode, "col", getLocaleText("���ࠢ�����"));
        SetProp(colNode, "width", 90);
        SetProp(colNode, "align", taLeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col");
    }


    // ���᪨
    colNode = NewTextChild(headerNode, "col", getLocaleText("���."));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", taRightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.pax_amount);

    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", taRightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.term_bag);

    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", taRightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.term_bp);

    colNode = NewTextChild(headerNode, "col", getLocaleText("��� ᠬ�"));
    SetProp(colNode, "width", 45);
    SetProp(colNode, "align", taRightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.pax_amount-total.term_ckin_service);

    if(params.statType == statSelfCkinFull) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", taRightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.adult);
        colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", taRightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.child);
        colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", taRightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.baby);
    }
    if(
            params.statType == statSelfCkinDetail or
            params.statType == statSelfCkinFull
      ) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("�����."));
        SetProp(colNode, "width", 45);
        SetProp(colNode, "align", taRightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.tckin);
    }
    if(
            params.statType == statSelfCkinShort or
            params.statType == statSelfCkinDetail
      ) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("�ਬ�砭��"));
        SetProp(colNode, "width", 280);
        SetProp(colNode, "align", taLeftJustify);
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
                 "����ॣ������" :
                 "���᪨ ᠬ�ॣ����樨"
                )
                ));
    string buf;
    switch(params.statType) {
        case statSelfCkinShort:
            buf = getLocaleText("����");
            break;
        case statSelfCkinDetail:
            buf = getLocaleText("��⠫���஢�����");
            break;
        case statSelfCkinFull:
            buf = getLocaleText("���஡���");
            break;
        default:
            throw Exception("createXMLSelfCkinStat: unexpected statType %d", params.statType);
            break;
    }
    NewTextChild(variablesNode, "stat_type_caption", buf);
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
                   TPrintAirline &prn_airline)
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
                  //��� ��饩 ����⨪� �뢮��� ������ ���� ���-�� �� � ������ ��������
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
                AddStatRow(key, row, TlgOutStat);
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
          if(rows >= MAX_STAT_ROWS) {
              AstraLocale::showErrorMessage("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH",
                                            LParams() << LParam("num", MAX_STAT_ROWS));
              if (WITHOUT_TOTAL_WHEN_PROBLEM) showTotal=false; //�� �㤥� �����뢠�� �⮣���� ��ப� ���� �� ����� � ����㦤����
              break;
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
    colNode = NewTextChild(headerNode, "col", getLocaleText("���� ���."));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", taLeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col", getLocaleText("�⮣�:"));
    colNode = NewTextChild(headerNode, "col", getLocaleText("�����"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", taLeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col");
    if (params.statType == statTlgOutFull)
    {
      colNode = NewTextChild(headerNode, "col", getLocaleText("���� �����."));
      SetProp(colNode, "width", 70);
      SetProp(colNode, "align", taLeftJustify);
      SetProp(colNode, "sort", sortString);
      NewTextChild(rowNode, "col");
    };
    colNode = NewTextChild(headerNode, "col", getLocaleText("���-��"));
    if (params.statType == statTlgOutShort)
      SetProp(colNode, "width", 200);
    else
      SetProp(colNode, "width", 40);
    SetProp(colNode, "align", taLeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col");
    if (params.statType == statTlgOutFull)
    {
      colNode = NewTextChild(headerNode, "col", getLocaleText("��� ���."));
      SetProp(colNode, "width", 60);
      SetProp(colNode, "align", taLeftJustify);
      SetProp(colNode, "sort", sortDate);
      NewTextChild(rowNode, "col");
    };
    if (params.statType == statTlgOutDetail ||
        params.statType == statTlgOutFull)
    {
      colNode = NewTextChild(headerNode, "col", getLocaleText("�/� 䠪�."));
      SetProp(colNode, "width", 50);
      SetProp(colNode, "align", taLeftJustify);
      SetProp(colNode, "sort", sortString);
      NewTextChild(rowNode, "col");
      colNode = NewTextChild(headerNode, "col", getLocaleText("�/� ����."));
      SetProp(colNode, "width", 50);
      SetProp(colNode, "align", taLeftJustify);
      SetProp(colNode, "sort", sortString);
      NewTextChild(rowNode, "col");
      colNode = NewTextChild(headerNode, "col", getLocaleText("��� �/�"));
      SetProp(colNode, "width", 50);
      SetProp(colNode, "align", taLeftJustify);
      SetProp(colNode, "sort", sortString);
      NewTextChild(rowNode, "col");
    };
    if (params.statType == statTlgOutFull)
    {
      colNode = NewTextChild(headerNode, "col", getLocaleText("��� ⫣."));
      SetProp(colNode, "width", 50);
      SetProp(colNode, "align", taLeftJustify);
      SetProp(colNode, "sort", sortString);
      NewTextChild(rowNode, "col");
      colNode = NewTextChild(headerNode, "col", getLocaleText("��� �뫥�"));
      SetProp(colNode, "width", 70);
      SetProp(colNode, "align", taLeftJustify);
      SetProp(colNode, "sort", sortDate);
      NewTextChild(rowNode, "col");
      colNode = NewTextChild(headerNode, "col", getLocaleText("����"));
      SetProp(colNode, "width", 50);
      SetProp(colNode, "align", taLeftJustify);
      SetProp(colNode, "sort", sortString);
      NewTextChild(rowNode, "col");
    };
    colNode = NewTextChild(headerNode, "col", getLocaleText("���-��"));
    SetProp(colNode, "width", params.statType == statTlgOutFull?40:70);
    SetProp(colNode, "align", taRightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.tlg_count);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��ꥬ (����)"));
    SetProp(colNode, "width", params.statType == statTlgOutFull?70:100);
    SetProp(colNode, "align", taRightJustify);
    SetProp(colNode, "sort", sortFloat);
    buf.str("");
    buf << fixed << setprecision(0) << total.tlg_len;
    NewTextChild(rowNode, "col", buf.str());
    colNode = NewTextChild(headerNode, "col", getLocaleText("� �������"));
    SetProp(colNode, "width", 150);
    SetProp(colNode, "align", taLeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col");

    if (!showTotal)
    {
      xmlUnlinkNode(rowNode);
      xmlFreeNode(rowNode);
    };

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("����⨪� ��ࠢ������ ⥫��ࠬ�"));
    string stat_type_caption;
    switch(params.statType) {
        case statTlgOutShort:
            stat_type_caption = getLocaleText("����");
            break;
        case statTlgOutDetail:
            stat_type_caption = getLocaleText("��⠫���஢�����");
            break;
        case statTlgOutFull:
            stat_type_caption = getLocaleText("���஡���");
            break;
        default:
            throw Exception("createXMLTlgOutStat: unexpected statType %d", params.statType);
            break;
    }
    NewTextChild(variablesNode, "stat_type_caption", stat_type_caption);
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
                  TPrintAirline &prn_airline)
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
                row.dpax_amount.inc -= row.dpax_amount.dec; //��宩 ���⮪ (�� �㬬��㥬)
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

                AddStatRow(key, row, AgentStat);
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
        if(rows >= MAX_STAT_ROWS) {
            AstraLocale::showErrorMessage("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH",
                                          LParams() << LParam("num", MAX_STAT_ROWS));
            if (WITHOUT_TOTAL_WHEN_PROBLEM) showTotal=false; //�� �㤥� �����뢠�� �⮣���� ��ப� ���� �� ����� � ����㦤����
            break;
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
              if (WITHOUT_TOTAL_WHEN_PROBLEM) showTotal=false; //�� �㤥� �����뢠�� �⮣���� ��ப� ���� �� ����� � ����㦤����
              continue;
          };
        };

        rowNode = NewTextChild(rowsNode, "row");
        ostringstream buf;
        if(
                params.statType == statAgentFull or
                params.statType == statAgentShort
          ) {
            // ��� �/�
            buf << im->first.airline_view
                << setw(3) << setfill('0') << im->first.flt_no
                << im->first.suffix_view << " "
                << im->first.airp_view;
            NewTextChild(rowNode, "col", buf.str());
            // ���
            NewTextChild(rowNode, "col", DateTimeToStr( scd_out_local, "dd.mm.yy"));
        }

        if(params.statType == statAgentFull) {
            // ����
            NewTextChild(rowNode, "col", im->first.desk);
        }
        if(
                params.statType == statAgentFull or
                params.statType == statAgentTotal
          ) {
            // ���짮��⥫�
            NewTextChild(rowNode, "col", im->first.user_descr);
        }
        if(params.statType == statAgentTotal) {
            // ���-�� ����.
            NewTextChild(rowNode, "col", im->second.dpax_amount.inc);
            // ���-�� ᪢��.
            NewTextChild(rowNode, "col", im->second.dtckin_amount.inc);
            // ����� (����/���)
            NewTextChild(rowNode, "col", IntToString(im->second.dbag_amount.inc) +
                                         "/" +
                                         IntToString(im->second.dbag_weight.inc));
            // �/����� (���)
            NewTextChild(rowNode, "col", im->second.drk_weight.inc);
        } else {
            // ���-�� ����. (+)
            NewTextChild(rowNode, "col", im->second.dpax_amount.inc);
            // ���-�� ����. (-)
            NewTextChild(rowNode, "col", -im->second.dpax_amount.dec);
            // ���-�� ᪢��. (+)
            NewTextChild(rowNode, "col", im->second.dtckin_amount.inc);
            // ���-�� ᪢��. (-)
            NewTextChild(rowNode, "col", -im->second.dtckin_amount.dec);
            // ����� (����/���) (+)
            NewTextChild(rowNode, "col", IntToString(im->second.dbag_amount.inc) +
                                         "/" +
                                         IntToString(im->second.dbag_weight.inc));
            // ����� (����/���) (-)
            NewTextChild(rowNode, "col", IntToString(-im->second.dbag_amount.dec) +
                                         "/" +
                                         IntToString(-im->second.dbag_weight.dec));
            // �/����� (���) (+)
            NewTextChild(rowNode, "col", im->second.drk_weight.inc);
            // �/����� (���) (-)
            NewTextChild(rowNode, "col", -im->second.drk_weight.dec);
            // �।��� �६�, ����祭��� �� ���ᠦ��
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
        colNode = NewTextChild(headerNode, "col", getLocaleText("����"));
        SetProp(colNode, "width", 70);
        SetProp(colNode, "align", taLeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col", getLocaleText("�⮣�:"));
        colNode = NewTextChild(headerNode, "col", getLocaleText("���"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", taLeftJustify);
        SetProp(colNode, "sort", sortDate);
        NewTextChild(rowNode, "col");
    }
    if(params.statType == statAgentFull) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("�⮩��"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", taLeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col");
    }
    if(
            params.statType == statAgentFull or
            params.statType == statAgentTotal
      ) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("�����"));
        SetProp(colNode, "width", 100);
        SetProp(colNode, "align", taLeftJustify);
        SetProp(colNode, "sort", sortString);
        if (params.statType == statAgentTotal)
          NewTextChild(rowNode, "col", getLocaleText("�⮣�:"));
        else
          NewTextChild(rowNode, "col");
    }
    if(params.statType == statAgentTotal) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("���."));
        SetProp(colNode, "width", 45);
        SetProp(colNode, "align", taRightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.dpax_amount.inc);
        colNode = NewTextChild(headerNode, "col", getLocaleText("�����."));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", taRightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.dtckin_amount.inc);
        colNode = NewTextChild(headerNode, "col", getLocaleText("���."));
        SetProp(colNode, "width", 70);
        SetProp(colNode, "align", taCenter);
        SetProp(colNode, "sort", sortIntegerSlashInteger);
        NewTextChild(rowNode, "col", IntToString(total.dbag_amount.inc) +
                                     "/" +
                                     IntToString(total.dbag_weight.inc));
        colNode = NewTextChild(headerNode, "col", getLocaleText("�/�"));
        SetProp(colNode, "width", 45);
        SetProp(colNode, "align", taRightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.drk_weight.inc);
    } else {
        colNode = NewTextChild(headerNode, "col", getLocaleText("���.")+" (+)");
        SetProp(colNode, "width", 45);
        SetProp(colNode, "align", taRightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.dpax_amount.inc);
        colNode = NewTextChild(headerNode, "col", getLocaleText("���.")+" (-)");
        SetProp(colNode, "width", 45);
        SetProp(colNode, "align", taRightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", -total.dpax_amount.dec);
        colNode = NewTextChild(headerNode, "col", getLocaleText("�����.")+"(+)");
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", taRightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.dtckin_amount.inc);
        colNode = NewTextChild(headerNode, "col", getLocaleText("�����.")+"(-)");
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", taRightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", -total.dtckin_amount.dec);
        colNode = NewTextChild(headerNode, "col", getLocaleText("���.")+" (+)");
        SetProp(colNode, "width", 70);
        SetProp(colNode, "align", taCenter);
        SetProp(colNode, "sort", sortIntegerSlashInteger);
        NewTextChild(rowNode, "col", IntToString(total.dbag_amount.inc) +
                                     "/" +
                                     IntToString(total.dbag_weight.inc));
        colNode = NewTextChild(headerNode, "col", getLocaleText("���.")+" (-)");
        SetProp(colNode, "width", 70);
        SetProp(colNode, "align", taCenter);
        SetProp(colNode, "sort", sortIntegerSlashInteger);
        NewTextChild(rowNode, "col", IntToString(-total.dbag_amount.dec) +
                                     "/" +
                                     IntToString(-total.dbag_weight.dec));
        colNode = NewTextChild(headerNode, "col", getLocaleText("�/�")+" (+)");
        SetProp(colNode, "width", 45);
        SetProp(colNode, "align", taRightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.drk_weight.inc);
        colNode = NewTextChild(headerNode, "col", getLocaleText("�/�")+" (-)");
        SetProp(colNode, "width", 45);
        SetProp(colNode, "align", taRightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", -total.drk_weight.dec);
        colNode = NewTextChild(headerNode, "col", getLocaleText("���./���."));
        SetProp(colNode, "width", 65);
        SetProp(colNode, "align", taRightJustify);
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
    NewTextChild(variablesNode, "stat_mode", getLocaleText("����⨪� �� ����⠬"));
    string buf;
    switch(params.statType) {
        case statAgentShort:
            buf = getLocaleText("����");
            break;
        case statAgentFull:
            buf = getLocaleText("���஡���");
            break;
        case statAgentTotal:
            buf = getLocaleText("�⮣�");
            break;
        default:
            throw Exception("createXMLAgentStat: unexpected statType %d", params.statType);
            break;
    }
    NewTextChild(variablesNode, "stat_type_caption", buf);
}

/****************** end of AgentStat ******************/


// �⮡ࠦ���� ��ࠬ��஢ ��७�-���� � ��ࠬ���� �����.

/* !!! �᫨ �� �� �� �++11
const map<string, string> m =
{
{"Short",       "����"},
{"Detail",      "��⠫���஢�����"},
{"Full",        "���஡���"},
{"Transfer",    "�࠭���"},
{"Pact",        "�������"},
{"SelfCkin",    "����ॣ������"},
{"Agent",       "�� ����⠬"},
{"Tlg",         "���. ⥫��ࠬ��"}
};
*/

#include <boost/assign/list_of.hpp>

const map<string, string> ST_to_Astra = boost::assign::map_list_of
("Short",       "����")
("Detail",      "��⠫���஢�����")
("Full",        "���஡���")
("Total",       "�⮣�")
("Transfer",    "�࠭���")
("Pact",        "�������")
("SelfCkin",    "����ॣ������")
("Agent",       "�� ����⠬")
("Tlg",         "���. ⥫��ࠬ��");

void convertStatParam(xmlNodePtr paramNode)
{
    string val = NodeAsString(paramNode);
    map<string, string>::const_iterator idx = ST_to_Astra.find(val);
    if(idx == ST_to_Astra.end())
        throw Exception("wrong param value '%s'", val.c_str());
    xmlNodeSetContent(paramNode, idx->second.c_str());
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
    //��ଠ���㥬 ��������
    norm_airlines.set_elems_permit(access.airlines().elems_permit());
    for(set<string>::iterator i=access.airlines().elems().begin();
                              i!=access.airlines().elems().end(); ++i)
      norm_airlines.add_elem(airl_fromXML(*i, cfErrorIfEmpty, __FUNCTION__, "airline"));
    //��ଠ���㥬 �����
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

void RunRFISCStat(
        const TStatParams &params,
        TRFISCStat &RFISCStat,
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

void createXMLRFISCStat(const TStatParams &params, const TRFISCStat &RFISCStat, const TPrintAirline &prn_airline, xmlNodePtr resNode)
{
    if(RFISCStat.empty()) throw AstraLocale::UserException("MSG.NOT_DATA");
    NewTextChild(resNode, "airline", prn_airline.get(), "");
    xmlNodePtr grdNode = NewTextChild(resNode, "grd");

    xmlNodePtr headerNode = NewTextChild(grdNode, "header");
    xmlNodePtr colNode;
    colNode = NewTextChild(headerNode, "col", "RFISC");
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", taLeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("����."));
    SetProp(colNode, "width", 100);
    SetProp(colNode, "align", taLeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("���."));
    SetProp(colNode, "width", 80);
    SetProp(colNode, "align", taLeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��ઠ"));
    SetProp(colNode, "width", 80);
    SetProp(colNode, "align", taLeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("SPEQ"));
    SetProp(colNode, "width", 100);
    SetProp(colNode, "align", taLeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("FQTV"));
    SetProp(colNode, "width", 150);
    SetProp(colNode, "align", taLeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��� �뫥�"));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", taLeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("����"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", taLeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", taLeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", taLeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��� ��"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", taLeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�६� � ���"));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", taLeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("����.३�"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", taLeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", taLeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", taLeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�� ॣ."));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", taLeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�⮩��"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", taLeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", "LOGIN");
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", taLeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�����"));
    SetProp(colNode, "width", 100);
    SetProp(colNode, "align", taLeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��� ���."));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", taLeftJustify);
    SetProp(colNode, "sort", sortString);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    ostringstream buf;
    for(TRFISCStat::iterator i = RFISCStat.begin(); i != RFISCStat.end(); i++) {
        rowNode = NewTextChild(rowsNode, "row");
        // RFISC
        NewTextChild(rowNode, "col", i->rfisc);
        // �ਧ��� ���⭮�� ������
        if(i->excess == NoExists)
            NewTextChild(rowNode, "col");
        else
            NewTextChild(rowNode, "col", i->excess);
        // �ਧ��� ������
        if(i->paid == NoExists)
            NewTextChild(rowNode, "col");
        else
            NewTextChild(rowNode, "col", i->paid);
        // ��ઠ
        buf.str("");
        buf
            << fixed
            << setprecision(0)
            << setw(10)
            << setfill('0')
            << i->tag_no;
        NewTextChild(rowNode, "col", buf.str());
        // SPEQ - �ਧ��� ᯥ�. ������
        NewTextChild(rowNode, "col");
        // ����� ��� �ਧ��� FQTV
        NewTextChild(rowNode, "col", i->fqt_no);

        // ���ଠ�� � ३�
        // ��� �뫥�
        NewTextChild(rowNode, "col", DateTimeToStr(i->scd_out, "dd.mm.yyyy"));
        // ����
        buf.str("");
        buf << setw(3) << setfill('0') << i->flt_no << ElemIdToCodeNative(etSuffix, i->suffix);
        NewTextChild(rowNode, "col", buf.str());
        // ��
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->airp));
        // ��
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->airp_arv));
        // ��� ��
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etCraft, i->craft));
        // �६� � ���
        if(i->travel_time == NoExists)
            NewTextChild(rowNode, "col");
        else
            NewTextChild(rowNode, "col", DateTimeToStr(i->travel_time, "hh:nn"));

        // �࠭��� ��
        ostringstream trfer_flt_no;
        string trfer_airp_dep;
        string trfer_airp_arv;
        if(i->trfer_flt_no) {
            trfer_flt_no << setw(3) << setfill('0') << i->trfer_flt_no << ElemIdToCodeNative(etSuffix, i->trfer_suffix);
            trfer_airp_dep = i->airp;
            trfer_airp_arv = i->trfer_airp_arv;
        }
        // ����
        NewTextChild(rowNode, "col", trfer_flt_no.str());
        // �� (ᮢ������ � �� ३�)
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, trfer_airp_dep));
        // ��
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, trfer_airp_arv));

        // ���ଠ�� �� �����
        // �� ॣ. (ᮢ������ � �� ३�)
        NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, i->airp));
        // �⮩��
        NewTextChild(rowNode, "col", i->desk);
        // LOGIN
        NewTextChild(rowNode, "col", i->user_login);
        // �����
        NewTextChild(rowNode, "col", i->user_descr);
        // ��� ��ଫ����
        if(i->time_create == NoExists)
            NewTextChild(rowNode, "col");
        else
            NewTextChild(rowNode, "col", DateTimeToStr(i->time_create, "dd.mm.yyyy"));
    }

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("������� RFISC"));
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("���஡���"));

    NewTextChild(variablesNode, "CAP.STAT.AGENT_INFO", getLocaleText("CAP.STAT.AGENT_INFO"));
    NewTextChild(variablesNode, "CAP.STAT.BAG_INFO", getLocaleText("CAP.STAT.BAG_INFO"));
    NewTextChild(variablesNode, "CAP.STAT.PAX_INFO", getLocaleText("CAP.STAT.PAX_INFO"));
    NewTextChild(variablesNode, "CAP.STAT.FLT_INFO", getLocaleText("CAP.STAT.FLT_INFO"));
}

void StatInterface::RunStat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqInfo = TReqInfo::Instance();
    if (!reqInfo->user.access.rights().permitted(600))
      throw AstraLocale::UserException("MSG.INSUFFICIENT_RIGHTS.NOT_ACCESS");

    if (reqInfo->user.access.totally_not_permitted())
      throw AstraLocale::UserException("MSG.NOT_DATA");

    TStatParams params;
    params.get(reqNode);

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
            params.statType==statRFISC
            )
    {
      if(IncMonth(params.FirstDate, 1) < params.LastDate)
        throw AstraLocale::UserException("MSG.SEARCH_PERIOD_SHOULD_NOT_EXCEED_ONE_MONTH");
    } else {
      if(IncMonth(params.FirstDate, 12) < params.LastDate)
        throw AstraLocale::UserException("MSG.SEARCH_PERIOD_SHOULD_NOT_EXCEED_ONE_YEAR");
    }

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
        get_compatible_report_form("stat", reqNode, resNode);
        break;
      default:
        throw Exception("unexpected stat type %d", params.statType);
    };


    try
    {
        if (params.statType==statShort || params.statType==statDetail || params.statType == statPactShort)
        {
          bool pr_pacts =
            reqInfo->user.access.rights().permitted(605) and params.seance == seanceAll;

          if(not pr_pacts and params.statType == statPactShort)
              throw UserException("MSG.INSUFFICIENT_RIGHTS.NOT_ACCESS");

          TDetailStat DetailStat;
          TDetailStatRow DetailStatTotal;
          TPrintAirline airline;

          if (pr_pacts)
            RunPactDetailStat(params, DetailStat, DetailStatTotal, airline);
          else
            RunDetailStat(params, DetailStat, DetailStatTotal, airline);

          createXMLDetailStat(params, pr_pacts, DetailStat, DetailStatTotal, airline, resNode);
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
    }
    catch (EOracleError &E)
    {
        if(E.Code == 376)
            throw AstraLocale::UserException("MSG.ONE_OF_DB_FILES_UNAVAILABLE.CALL_ADMIN");
        else
            throw;
    }
    ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str()); //!!!
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
    if(IncMonth(FirstDate, 3) < LastDate)
        throw AstraLocale::UserException("MSG.SEARCH_PERIOD_SHOULD_NOT_EXCEED_THREE_MONTHS");
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
        Qry.CreateVariable("tag_no", otInteger, ToInt(tag_no));
    }
    int count = 0;
    for(int pass = 0; (pass <= 2) && (count < MAX_STAT_ROWS); pass++) {
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
                 " salons.get_seat_no(pax.pax_id, pax.seats, pax_grp.status, pax_grp.point_dep, 'seats', rownum) seat_no, \n";
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
        PieceConcept::TNodeList pcNodeList;
        PaxListToXML(Qry, resNode, pcNodeList, true, pass, count);

    }
    if(count == 0)
        throw AstraLocale::UserException("MSG.PASSENGERS.NOT_FOUND");

    xmlNodePtr paxListNode = GetNode("paxList", resNode);
    if(paxListNode!=NULL)
    {
      xmlNodePtr headerNode = NewTextChild(paxListNode, "header"); // ��� ᮢ���⨬��� � ���� �ନ�����
      NewTextChild(headerNode, "col", "����");
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
            "    place_calc.time_out_in travel_time, "
            "    bag2.desk, "
            "    bag2.time_create, "
            "    users2.login, "
            "    users2.descr "
            "from "
            "    pax_grp, "
            "    points arv_point, "
            "    points, "
            "    bag2, "
            "    pax, "
            "    transfer, "
            "    trfer_trips, "
            "    place_calc, "
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
            "    bag2.rfisc is not null and "
            "    ckin.get_bag_pool_pax_id(bag2.grp_id, bag2.bag_pool_num) = pax.pax_id(+)  and "
            "    pax_grp.grp_id=transfer.grp_id(+) and "
            "    transfer.pr_final(+) <> 0 and "
            "    transfer.point_id_trfer = trfer_trips.point_id(+) and "
            "    place_calc.bc = points.craft(+) and "
            "    place_calc.cod_out = points.airp(+) and "
            "    place_calc.cod_in = arv_point.airp(+) and "
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
        int col_travel_time = bagQry.get().FieldIndex("travel_time");
        int col_desk = bagQry.get().FieldIndex("desk");
        int col_time_create = bagQry.get().FieldIndex("time_create");
        int col_login = bagQry.get().FieldIndex("login");
        int col_descr = bagQry.get().FieldIndex("descr");
        for(; not bagQry.get().Eof; bagQry.get().Next()) {
            insQry.get().SetVariable("point_id", bagQry.get().FieldAsInteger(col_point_id));
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

            if(bagQry.get().FieldIsNULL(col_travel_time))
                insQry.get().SetVariable("travel_time", FNull);
            else
                insQry.get().SetVariable("travel_time", bagQry.get().FieldAsDateTime(col_travel_time));

            if(bagQry.get().FieldIsNULL(col_time_create))
                insQry.get().SetVariable("time_create", FNull);
            else
                insQry.get().SetVariable("time_create", bagQry.get().FieldAsDateTime(col_time_create));

            insQry.get().SetVariable("login", bagQry.get().FieldAsString(col_login));
            insQry.get().SetVariable("descr", bagQry.get().FieldAsString(col_descr));

            int grp_id = bagQry.get().FieldAsInteger(col_grp_id);
            list<PieceConcept::TPaidBagItem> paid;

            PaidBagFromDB(grp_id, true, paid);

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

            set<PieceConcept::TBagStatus> bs_set;
            for(list<PieceConcept::TPaidBagItem>::iterator i = paid.begin(); i != paid.end(); i++) {
                if(i->trfer_num == 0 and not i->pr_cabin) {
                    bs_set.insert(i->status);
                }
            }

            tagsQry.get().SetVariable("grp_id", grp_id);
            tagsQry.get().SetVariable("bag_num", bagQry.get().FieldAsInteger(col_bag_num));
            tagsQry.get().Execute();
            set<PieceConcept::TBagStatus>::iterator bs_idx = bs_set.begin();
            for(; not tagsQry.get().Eof; tagsQry.get().Next()) {

                int excess = NoExists;
                int paid = NoExists;

                if(bs_idx != bs_set.end()) {
                    switch(*bs_idx) {
                        case PieceConcept::bsFree:
                            excess = 0;
                            break;
                        case PieceConcept::bsPaid:
                        case PieceConcept::bsNeed:
                            excess = 1;
                            break;
                        default:
                            excess = NoExists;
                            break;
                    }

                    paid = (*bs_idx == PieceConcept::bsPaid ? 1 : NoExists);
                    bs_idx++;
                }

                if(excess == NoExists)
                    insQry.get().SetVariable("excess", FNull);
                else
                    insQry.get().SetVariable("excess", excess);
                if(paid == NoExists)
                    insQry.get().SetVariable("paid", FNull);
                else
                    insQry.get().SetVariable("paid", paid);

                insQry.get().SetVariable("bag_tag", tagsQry.get().FieldAsFloat("no"));
                insQry.get().SetVariable("fqt_no", fqt_no);
                fqt_no.clear();

                insQry.get().Execute();
            }
        }
    }
}

void get_flight_stat(int point_id, bool final_collection)
{
   TFlights flightsForLock;
   flightsForLock.Get( point_id, ftTranzit );
   flightsForLock.Lock();

   {
     QParams QryParams;
     QryParams << QParam("point_id", otInteger, point_id);
     QryParams << QParam("final_collection", otInteger, (int)final_collection);
     TCachedQuery Qry("UPDATE trip_sets SET pr_stat=:final_collection WHERE point_id=:point_id AND pr_stat=0", QryParams);
     Qry.get().Execute();
     if (Qry.get().RowsProcessed()<=0) return; //����⨪� �� ᮡ�ࠥ�
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
     Qry.get().Execute();
     get_rfisc_stat(point_id);
   };

   TReqInfo::Instance()->LocaleToLog("EVT.COLLECT_STATISTIC", evtFlt, point_id);
};
