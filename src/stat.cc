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
#include "timer.h"
#include "astra_utils.h"
#include "astra_misc.h"
#include "term_version.h"

#define NICKNAME "DENIS"
#include "serverlib/test.h"

#define MAX_STAT_ROWS 2000

using namespace std;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace BASIC;

const string SYSTEM_USER = "Система";

const string AIRP_PERIODS =
" (SELECT DECODE(SIGN(period_first_date-:FirstDate),1,period_first_date,:FirstDate) AS period_first_date, \n"
"        DECODE(SIGN(period_last_date- :LastDate),-1,period_last_date, :LastDate) AS period_last_date \n"
" FROM \n"
" (SELECT distinct change_point AS period_first_date, \n"
"         report.get_airp_period_last_date(:ap,change_point) AS period_last_date \n"
"  FROM pacts, \n"
"   (SELECT first_date AS change_point \n"
"    FROM pacts \n"
"    WHERE airp=:ap \n"
"    UNION \n"
"    SELECT last_date   \n"
"    FROM pacts \n"
"    WHERE airp=:ap AND last_date IS NOT NULL) a \n"
"  WHERE airp=:ap AND  \n"
"        change_point>=first_date AND  \n"
"        (last_date IS NULL OR change_point<last_date) AND \n"
"        airline IS NULL) periods \n"
"WHERE (period_last_date IS NULL OR period_last_date>:FirstDate) AND  \n"
"      period_first_date<:LastDate) periods \n";
const string AIRLINE_PERIODS =
" (SELECT DECODE(SIGN(period_first_date-:FirstDate),1,period_first_date,:FirstDate) AS period_first_date, \n"
"        DECODE(SIGN(period_last_date- :LastDate),-1,period_last_date, :LastDate) AS period_last_date \n"
" FROM \n"
"  (SELECT distinct change_point AS period_first_date, \n"
"          report.get_airline_period_last_date(:ak,change_point) AS period_last_date \n"
"   FROM pacts,  \n"
"    (SELECT first_date AS change_point \n"
"     FROM pacts \n"
"     WHERE airline=:ak \n"
"     UNION \n"
"     SELECT last_date   \n"
"     FROM pacts \n"
"     WHERE airline=:ak AND last_date IS NOT NULL) a \n"
"  WHERE airline=:ak AND  \n"
"        change_point>=first_date AND  \n"
"        (last_date IS NULL OR change_point<last_date)) periods \n"
" WHERE (period_last_date IS NULL OR period_last_date>:FirstDate) AND  \n"
"       period_first_date<:LastDate) periods \n";

const string AIRLINE_LIST =
" (SELECT airline  \n"
"  FROM pacts  \n"
"  WHERE airp=:ap AND \n"
"  first_date<=periods.period_first_date AND  \n"
"  (last_date IS NULL OR periods.period_first_date<last_date) AND \n"
"  airline IS NOT NULL) \n";

const string AIRP_LIST =
" (SELECT airp  \n"
"  FROM pacts  \n"
"  WHERE airline=:ak AND \n"
"  first_date<=periods.period_first_date AND  \n"
"  (last_date IS NULL OR periods.period_first_date<last_date) ) \n";

void GetSystemLogAgentSQL(TQuery &Qry);
void GetSystemLogStationSQL(TQuery &Qry);
void GetSystemLogModuleSQL(TQuery &Qry);

enum TScreenState {ssNone,ssLog,ssPaxList,ssFltLog,ssSystemLog,ssPaxSrc};
enum TDROPScreenState {dssNone,dssStat,dssPax,dssLog,dssDepStat,dssBagTagStat,dssPaxList,dssFltLog,dssSystemLog,dssPaxSrc,dssTlgArch};

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


typedef struct {
    TDateTime part_key, real_out_client;
    string airline, suffix, name;
    int point_id, flt_no, move_id, point_num;
} TPointsRow;

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

void StatInterface::FltCBoxDropDown(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    bool pr_show_del = false;
    xmlNodePtr prDelNode = GetNode("pr_del", reqNode);
    if(prDelNode)
        pr_show_del = NodeAsInteger(prDelNode) == 1;
    TScreenState scr = ssNone;
    if (TReqInfo::Instance()->desk.compatible(NEW_TERM_VERSION))
        scr = TScreenState(NodeAsInteger("scr", reqNode));
    else {
        TDROPScreenState drop_scr = TDROPScreenState(NodeAsInteger("scr", reqNode));
        switch(drop_scr) {
            case dssNone:
                scr = ssNone;
                break;
            case dssLog:
                scr = ssLog;
                break;
            case dssPaxList:
                scr = ssPaxList;
                break;
            case dssFltLog:
                scr = ssFltLog;
                break;
            case dssSystemLog:
                scr = ssSystemLog;
                break;
            case dssPaxSrc:
                scr = ssPaxSrc;
                break;
            default:
                throw Exception("StatInterface::FltCBoxDropDown: unexpected drop_scr: %d", drop_scr);
        }
    }
    ProgTrace(TRACE5, "scr: %d", scr);
    TReqInfo &reqInfo = *(TReqInfo::Instance());
    TQuery Qry(&OraSession);
    Qry.CreateVariable("FirstDate", otDate, ClientToUTC(NodeAsDateTime("FirstDate", reqNode), reqInfo.desk.tz_region));
    Qry.CreateVariable("LastDate", otDate, ClientToUTC(NodeAsDateTime("LastDate", reqNode), reqInfo.desk.tz_region));
    string trip_name;
    typedef vector<TPointsRow> TPoints;
    TPoints points;
    TPerfTimer tm;
    tm.Init();
    int count = 0;
    ProgTrace(TRACE5, "TRACC");
    if (!(reqInfo.user.access.airlines.empty() && reqInfo.user.access.airlines_permit ||
            reqInfo.user.access.airps.empty() && reqInfo.user.access.airps_permit))
    {
        for(int i = 0; i < 2; i++) {
            string SQLText;
            if(i == 0) {
                SQLText =
                    "SELECT "
                    "    null part_key, "
                    "    points.point_id, "
                    "    points.airp, "
                    "    points.airline, "
                    "    points.flt_no, "
                    "    points.airline_fmt, "
                    "    points.airp_fmt, "
                    "    points.suffix_fmt, "
                    "    nvl(points.suffix, ' ') suffix, "
                    "    points.scd_out, "
                    "    trunc(NVL(points.act_out,NVL(points.est_out,points.scd_out))) AS real_out, "
                    "    points.pr_del, "
                    "    move_id, "
                    "    point_num "
                    "FROM "
                    "    points "
                    "WHERE "
                    "    points.scd_out >= :FirstDate AND points.scd_out < :LastDate ";
                if(scr == ssPaxList)
                    SQLText += " and points.pr_del = 0 ";
                if(scr == ssFltLog and !pr_show_del)
                    SQLText += " and points.pr_del >= 0 ";
                if (!reqInfo.user.access.airlines.empty()) {
                    if (reqInfo.user.access.airlines_permit)
                        SQLText += " AND points.airline IN "+GetSQLEnum(reqInfo.user.access.airlines);
                    else
                        SQLText += " AND points.airline NOT IN "+GetSQLEnum(reqInfo.user.access.airlines);
                }
                if (!reqInfo.user.access.airps.empty()) {
                    if (reqInfo.user.access.airps_permit)
                        SQLText+="AND (points.airp IN "+GetSQLEnum(reqInfo.user.access.airps)+" OR "+
                            "          ckin.next_airp(DECODE(points.pr_tranzit,0,points.point_id,points.first_point),points.point_num) IN "+
                            GetSQLEnum(reqInfo.user.access.airps)+")";
                    else
                        SQLText+="AND (points.airp NOT IN "+GetSQLEnum(reqInfo.user.access.airps)+" OR "+
                            "          ckin.next_airp(DECODE(points.pr_tranzit,0,points.point_id,points.first_point),points.point_num) NOT IN "+
                            GetSQLEnum(reqInfo.user.access.airps)+")";
                }
            } else {
                SQLText =
                    "SELECT "
                    "    arx_points.part_key, "
                    "    arx_points.point_id, "
                    "    arx_points.airp, "
                    "    arx_points.airline, "
                    "    arx_points.flt_no, "
                    "    arx_points.airline_fmt, "
                    "    arx_points.airp_fmt, "
                    "    arx_points.suffix_fmt, "
                    "    nvl(arx_points.suffix, ' ') suffix, "
                    "    arx_points.scd_out, "
                    "    trunc(NVL(arx_points.act_out,NVL(arx_points.est_out,arx_points.scd_out))) AS real_out, "
                    "    arx_points.pr_del, "
                    "    move_id, "
                    "    point_num "
                    "FROM "
                    "    arx_points "
                    "WHERE "
                    "    arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate and "
                    "    arx_points.part_key >= :FirstDate and arx_points.part_key < :LastDate + :arx_trip_date_range ";
                Qry.CreateVariable("arx_trip_date_range", otInteger, arx_trip_date_range);
                if(scr == ssPaxList)
                    SQLText += " and arx_points.pr_del = 0 ";
                if(scr == ssFltLog and !pr_show_del)
                    SQLText += " and arx_points.pr_del >= 0 ";
                if (!reqInfo.user.access.airlines.empty()) {
                    if (reqInfo.user.access.airlines_permit)
                        SQLText += " AND arx_points.airline IN "+GetSQLEnum(reqInfo.user.access.airlines);
                    else
                        SQLText += " AND arx_points.airline NOT IN "+GetSQLEnum(reqInfo.user.access.airlines);
                }
                if (!reqInfo.user.access.airps.empty()) {
                    if (reqInfo.user.access.airps_permit)
                        SQLText+="AND (arx_points.airp IN "+GetSQLEnum(reqInfo.user.access.airps)+" OR "+
                            "     arch.next_airp(DECODE(arx_points.pr_tranzit,0,arx_points.point_id,arx_points.first_point),arx_points.point_num, arx_points.part_key) IN "+
                            GetSQLEnum(reqInfo.user.access.airps)+")";
                    else
                        SQLText+="AND (arx_points.airp NOT IN "+GetSQLEnum(reqInfo.user.access.airps)+" OR "+
                            "          arch.next_airp(DECODE(arx_points.pr_tranzit,0,arx_points.point_id,arx_points.first_point),arx_points.point_num,arx_points.part_key) NOT IN "+
                            GetSQLEnum(reqInfo.user.access.airps)+")";
                }
            }
            Qry.SQLText = SQLText;
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
    }
    ProgTrace(TRACE5, "FltCBoxDropDown QRY: %s", Qry.SQLText.SQLText());
    ProgTrace(TRACE5, "FltCBoxDropDown EXEC QRY: %s", tm.PrintWithMessage().c_str());
    if(count == 0)
        throw AstraLocale::UserException("MSG.FLIGHTS_NOT_FOUND");
    tm.Init();
    sort(points.begin(), points.end(), lessPointsRow);
    ProgTrace(TRACE5, "FltCBoxDropDown SORT: %s", tm.PrintWithMessage().c_str());
    tm.Init();
    xmlNodePtr cboxNode = NewTextChild(resNode, "cbox");
    for(TPoints::iterator iv = points.begin(); iv != points.end(); iv++) {
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

void StatInterface::FltLogRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqInfo = TReqInfo::Instance();
    if(find( reqInfo->user.access.rights.begin(),
                reqInfo->user.access.rights.end(), 650 ) == reqInfo->user.access.rights.end())
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
        get_report_form("ArxPaxLog", reqNode, resNode);
    else
        get_report_form("FltLog", reqNode, resNode);
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
            Qry.SQLText = "select move_id, airline from points where point_id = :point_id";
            Qry.CreateVariable("point_id", otInteger, point_id);
            Qry.Execute();
            if(Qry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.MOVED_TO_ARX_OR_DEL.SELECT_AGAIN");
            move_id = Qry.FieldAsInteger("move_id");
            airline = Qry.FieldAsString("airline");
        }
        ProgTrace(TRACE5, "FltLogRun: work base qry");
        qry1 =
            "SELECT msg, time,  "
            "       id1 AS point_id,  "
            "       events.screen,  "
            "       DECODE(type,:evtPax,id2,:evtPay,id2,NULL) AS reg_no,  "
            "       DECODE(type,:evtPax,id3,:evtPay,id3,NULL) AS grp_id,  "
            "       ev_user, station, ev_order  "
            "FROM events  "
            "WHERE "
            "   events.type IN (:evtFlt,:evtGraph,:evtPax,:evtPay,:evtTlg) AND  "
            "   events.id1=:point_id  ";
        qry2 =
            "SELECT msg, time,  "
            "       id2 AS point_id,  "
            "       events.screen,  "
            "       NULL AS reg_no,  "
            "       NULL AS grp_id,  "
            "       ev_user, station, ev_order  "
            "FROM events  "
            "WHERE "
            "events.type IN (:evtDisp) AND "
            "events.id1=:move_id  ";
    } else {
        {
            TQuery Qry(&OraSession);
            Qry.SQLText =
                "select move_id, airline from arx_points "
                "where part_key = :part_key and point_id = :point_id and pr_del>=0";
            Qry.CreateVariable("part_key", otDate, part_key);
            Qry.CreateVariable("point_id", otInteger, point_id);
            Qry.Execute();
            if(Qry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND");
            move_id = Qry.FieldAsInteger("move_id");
            airline = Qry.FieldAsString("airline");
        }
        ProgTrace(TRACE5, "FltLogRun: arx base qry");
        qry1 =
            "SELECT msg, time,  "
            "       id1 AS point_id,  "
            "       arx_events.screen,  "
            "       DECODE(type,:evtPax,id2,:evtPay,id2,NULL) AS reg_no,  "
            "       DECODE(type,:evtPax,id3,:evtPay,id3,NULL) AS grp_id,  "
            "       ev_user, station, ev_order  "
            "FROM arx_events  "
            "WHERE "
            "   arx_events.part_key = :part_key and "
            "   arx_events.type IN (:evtFlt,:evtGraph,:evtPax,:evtPay,:evtTlg) AND  "
            "   arx_events.id1=:point_id  ";
        qry2 =
            "SELECT msg, time,  "
            "       id2 AS point_id,  "
            "       arx_events.screen,  "
            "       NULL AS reg_no,  "
            "       NULL AS grp_id,  "
            "       ev_user, station, ev_order  "
            "FROM arx_events  "
            "WHERE "
            "      arx_events.part_key = :part_key and "
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
        } else {
            Qry.SQLText = qry2;
            Qry.CreateVariable("move_id", otInteger, move_id);
            Qry.CreateVariable("evtDisp",otString,EncodeEventType(ASTRA::evtDisp));
        }
        if(part_key != NoExists)
            Qry.CreateVariable("part_key", otDate, part_key);
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
                if(count > MAX_STAT_ROWS) {
                    AstraLocale::showErrorMessage("MSG.TOO_MANY_FLIGHTS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_SEARCH",
                            LParams() << LParam("num", MAX_STAT_ROWS));
                    break;
                }
            }
        }
        if(count > MAX_STAT_ROWS) break;
    }
    ProgTrace(TRACE5, "count: %d", count);
    if(!count)
        throw AstraLocale::UserException("MSG.OPERATIONS_NOT_FOUND");
    ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
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
        get_report_form("ArxPaxLog", reqNode, resNode);
    else
        get_report_form("FltLog", reqNode, resNode);
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
        AirlineQry.SQLText = "select airline from points where point_id = :point_id";
        ProgTrace(TRACE5, "LogRun: work base qry");
        Qry.SQLText =
            "SELECT msg, time, id1 AS point_id, null as screen, id2 AS reg_no, id3 AS grp_id, "
            "       ev_user, station, ev_order "
            "FROM events "
            "WHERE type IN (:evtPax,:evtPay) AND "
            "      screen <> 'ASTRASERV.EXE' and "
            "      id1=:point_id AND "
            "      (id2 IS NULL OR id2=:reg_no) AND "
            "      (id3 IS NULL OR id3=:grp_id) ";
    } else {
        AirlineQry.SQLText = "select airline from arx_points where point_id = :point_id and part_key = :part_key and pr_del >= 0";
        AirlineQry.CreateVariable("part_key", otDate, part_key);
        ProgTrace(TRACE5, "LogRun: arx base qry");
        Qry.SQLText =
            "SELECT msg, time, id1 AS point_id, null as screen, id2 AS reg_no, id3 AS grp_id, "
            "       ev_user, station, ev_order "
            "FROM arx_events "
            "WHERE part_key = :part_key AND "
            "      type IN (:evtPax,:evtPay) AND "
            "      screen <> 'ASTRASERV.EXE' and "
            "      id1=:point_id AND "
            "      (id2 IS NULL OR id2=:reg_no) AND "
            "      (id3 IS NULL OR id3=:grp_id) ";
        Qry.CreateVariable("part_key", otDate, part_key);
    }

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
            if(!Qry.FieldIsNULL(col_grp_id))
                NewTextChild(rowNode, "grp_id", Qry.FieldAsInteger(col_grp_id));
            if(!Qry.FieldIsNULL(col_reg_no))
                NewTextChild(rowNode, "reg_no", Qry.FieldAsInteger(col_reg_no));
            NewTextChild(rowNode, "ev_user", ev_user, "");
            NewTextChild(rowNode, "station", station, "");
            NewTextChild(rowNode, "screen", Qry.FieldAsString(col_screen), "");

            count++;
            if(count > MAX_STAT_ROWS) {
                AstraLocale::showErrorMessage("MSG.TOO_MANY_FLIGHTS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_SEARCH",
                        LParams() << LParam("num", MAX_STAT_ROWS));
                break;
            }
        }
    }
    ProgTrace(TRACE5, "count: %d", count);
    if(!count)
        throw AstraLocale::UserException("MSG.OPERATIONS_NOT_FOUND");
}

typedef struct {
    string trip, scd_out;
} TTripItem;

void StatInterface::SystemLogRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqInfo = TReqInfo::Instance();
    if(find( reqInfo->user.access.rights.begin(),
                reqInfo->user.access.rights.end(), 655 ) == reqInfo->user.access.rights.end())
        throw AstraLocale::UserException("MSG.SYS_LOG.VIEW_DENIED");
    xmlNodePtr client_with_trip_col_in_SysLogNode = GetNode("client_with_trip_col_in_SysLog", reqNode);
    if(client_with_trip_col_in_SysLogNode == NULL)
        get_report_form("ArxPaxLog", reqNode, resNode);
    else
        get_report_form("SystemLog", reqNode, resNode);
    STAT::set_variables(resNode);
    xmlNodePtr variablesNode = GetNode("form_data/variables", resNode);
    NewTextChild(variablesNode, "report_title", getLocaleText("Операции в системе"));
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
    NewTextChild(headerNode, "col", "Агент"); // для совместимости со старой версией терминала

    map<int, string> TripItems;
    xmlNodePtr rowsNode = NULL;
    for(int j = 0; j < 2; j++) {
        Qry.Clear();
        if (j==0) {
            Qry.SQLText =
                "SELECT msg, time, "
                "       DECODE(type, :evtFlt, id1, :evtPax, id1, :evtPay, id1, :evtGraph, id1, :evtTlg, id1, "
                "                    :evtDisp, id2, NULL) AS point_id, "
                "       screen, "
                "       DECODE(type,:evtPax,id2,:evtPay,id2,NULL) AS reg_no, "
                "       DECODE(type,:evtPax,id3,:evtPay,id3,NULL) AS grp_id, "
                "  ev_user, station, ev_order, null part_key "
                "FROM "
                "   events "
                "WHERE "
                "  events.time >= :FirstDate and "
                "  events.time < :LastDate and "
                "  (:agent is null or nvl(ev_user, :system_user) = :agent) and "
                "  (:module is null or nvl(screen, :system_user) = :module) and "
                "  (:station is null or nvl(station, :system_user) = :station) and "
                "  events.type IN ( "
                "    :evtFlt, "
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
            Qry.SQLText =
                "SELECT msg, time, "
                "       DECODE(type, :evtFlt, id1, :evtPax, id1, :evtPay, id1, :evtGraph, id1, :evtTlg, id1, "
                "                    :evtDisp, id2, NULL) AS point_id, "
                "       screen, "
                "       DECODE(type,:evtPax,id2,:evtPay,id2,NULL) AS reg_no, "
                "       DECODE(type,:evtPax,id3,:evtPay,id3,NULL) AS grp_id, "
                "  ev_user, station, ev_order, part_key "
                "FROM "
                "   arx_events "
                "WHERE "
                "  arx_events.part_key >= :FirstDate - 10 and " // time и part_key не совпадают для
                "  arx_events.part_key < :LastDate + 10 and "   // разных типов событий
                "  arx_events.time >= :FirstDate and "         // поэтому для part_key берем больший диапазон time
                "  arx_events.time < :LastDate and "
                "  (:agent is null or nvl(ev_user, :system_user) = :agent) and "
                "  (:module is null or nvl(screen, :system_user) = :module) and "
                "  (:station is null or nvl(station, :system_user) = :station) and "
                "  arx_events.type IN ( "
                "    :evtFlt, "
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
        }

        Qry.CreateVariable("evtFlt", otString, NodeAsString("evtFlt", reqNode));
        Qry.CreateVariable("evtPax", otString, NodeAsString("evtPax", reqNode));
        Qry.CreateVariable("system_user", otString, SYSTEM_USER);
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

        Qry.CreateVariable("FirstDate", otDate, ClientToUTC(NodeAsDateTime("FirstDate", reqNode), reqInfo->desk.tz_region));
        Qry.CreateVariable("LastDate", otDate, ClientToUTC(NodeAsDateTime("LastDate", reqNode), reqInfo->desk.tz_region));
        Qry.CreateVariable("agent", otString, agent);
        Qry.CreateVariable("station", otString, station);
        Qry.CreateVariable("module", otString, module);

        ProgTrace(TRACE5, "SQLText %d: %s", j, Qry.SQLText.SQLText());

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
                            TTripInfo trip_info(tripQry);
                            TripItems[point_id] = GetTripName(trip_info, ecCkin);
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
                if(count > MAX_STAT_ROWS) {
                    AstraLocale::showErrorMessage("MSG.TOO_MANY_FLIGHTS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_SEARCH",
                            LParams() << LParam("num", MAX_STAT_ROWS));
                    break;
                }
            }
        }
        ProgTrace(TRACE5, "FORM XML2: %s", tm.PrintWithMessage().c_str());
        ProgTrace(TRACE5, "count %d: %d", j, count);
    }
    if(!count)
        throw AstraLocale::UserException("MSG.OPERATIONS_NOT_FOUND");
    ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
}

struct THallItem {
    int id;
    string name;
};

void StatInterface::PaxListRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo &info = *(TReqInfo::Instance());
    if(find( info.user.access.rights.begin(),
                info.user.access.rights.end(), 630 ) == info.user.access.rights.end())
        throw AstraLocale::UserException("MSG.PAX_LIST.VIEW_DENIED");
    if (info.user.access.airlines.empty() && info.user.access.airlines_permit ||
            info.user.access.airps.empty() && info.user.access.airps_permit)
        throw AstraLocale::UserException("MSG.PASSENGERS.NOT_FOUND");
    xmlNodePtr paramNode = reqNode->children;

    int point_id = NodeAsIntegerFast("point_id", paramNode);
    TDateTime part_key;
    xmlNodePtr partKeyNode = GetNodeFast("part_key", paramNode);
    if(partKeyNode == NULL)
        part_key = NoExists;
    else
        part_key = NodeAsDateTime(partKeyNode);
    get_report_form("ArxPaxList", reqNode, resNode);
    {
        TQuery Qry(&OraSession);
        string SQLText;
        if(part_key == NoExists)  {
            ProgTrace(TRACE5, "PaxListRun: current base qry");
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
                "   pax_grp.grp_id, "
                "   ckin.get_birks2(pax.grp_id,pax.pax_id,pax.bag_pool_num,:pr_lat) tags, "
                "   pax.refuse, "
                "   pax.pr_brd, "
                "   pax_grp.class_grp, "
                "   salons.get_seat_no(pax.pax_id, pax.seats, pax_grp.status, pax_grp.point_dep, 'seats', rownum) seat_no, "
                "   pax_grp.hall, "
                "   pax.document, "
                "   pax.ticket_no "
                "FROM  pax_grp,pax, points "
                "WHERE "
                "   points.point_id = :point_id and points.pr_del>=0 and "
                "   points.point_id = pax_grp.point_dep and "
                "   pax_grp.grp_id=pax.grp_id ";
            if (!info.user.access.airps.empty()) {
                if (info.user.access.airps_permit)
                    SQLText += " AND points.airp IN "+GetSQLEnum(info.user.access.airps);
                else
                    SQLText += " AND points.airp NOT IN "+GetSQLEnum(info.user.access.airps);
            }
            if (!info.user.access.airlines.empty()) {
                if (info.user.access.airlines_permit)
                    SQLText += " AND points.airline IN "+GetSQLEnum(info.user.access.airlines);
                else
                    SQLText += " AND points.airline NOT IN "+GetSQLEnum(info.user.access.airlines);
            }
        } else {
            ProgTrace(TRACE5, "PaxListRun: arx base qry");
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
                "   NVL(arch.get_bagAmount(arx_pax.part_key,arx_pax.grp_id,arx_pax.pax_id,rownum),0) bag_amount, "
                "   NVL(arch.get_bagWeight(arx_pax.part_key,arx_pax.grp_id,arx_pax.pax_id,rownum),0) bag_weight, "
                "   NVL(arch.get_rkWeight(arx_pax.part_key,arx_pax.grp_id,arx_pax.pax_id,rownum),0) rk_weight, "
                "   NVL(arch.get_excess(arx_pax.part_key,arx_pax.grp_id,arx_pax.pax_id),0) excess, "
                "   arx_pax_grp.grp_id, "
                "   arch.get_birks(arx_pax.part_key,arx_pax.grp_id,arx_pax.pax_id,:pr_lat) tags, "
                "   arx_pax.refuse, "
                "   arx_pax.pr_brd, "
                "   arx_pax_grp.class_grp, "
                "   LPAD(seat_no,3,'0')|| "
                "       DECODE(SIGN(1-seats),-1,'+'||TO_CHAR(seats-1),'') seat_no, "
                "   arx_pax_grp.hall, "
                "   arx_pax.document, "
                "   arx_pax.ticket_no "
                "FROM  arx_pax_grp,arx_pax, arx_points "
                "WHERE "
                "   arx_points.point_id = :point_id and arx_points.pr_del>=0 and "
                "   arx_points.part_key = arx_pax_grp.part_key and "
                "   arx_points.point_id = arx_pax_grp.point_dep and "
                "   arx_pax_grp.part_key=arx_pax.part_key AND "
                "   arx_pax_grp.grp_id=arx_pax.grp_id AND "
                "   arx_points.part_key = :part_key and "
                "   pr_brd IS NOT NULL  ";
            if (!info.user.access.airps.empty()) {
                if (info.user.access.airps_permit)
                    SQLText += " AND arx_points.airp IN "+GetSQLEnum(info.user.access.airps);
                else
                    SQLText += " AND arx_points.airp NOT IN "+GetSQLEnum(info.user.access.airps);
            }
            if (!info.user.access.airlines.empty()) {
                if (info.user.access.airlines_permit)
                    SQLText += " AND arx_points.airline IN "+GetSQLEnum(info.user.access.airlines);
                else
                    SQLText += " AND arx_points.airline NOT IN "+GetSQLEnum(info.user.access.airlines);
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
        xmlNodePtr paxListNode = NULL;
        xmlNodePtr rowsNode = NULL;
        if(!Qry.Eof) {
            tm.Init();
            paxListNode = NewTextChild(resNode, "paxList");
            rowsNode = NewTextChild(paxListNode, "rows");
            ProgTrace(TRACE5, "Header: %s", tm.PrintWithMessage().c_str());
            tm.Init();

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
            int col_grp_id = Qry.FieldIndex("grp_id");
            int col_airp_arv = Qry.FieldIndex("airp_arv");
            int col_tags = Qry.FieldIndex("tags");
            int col_refuse = Qry.FieldIndex("refuse");
            int col_pr_brd = Qry.FieldIndex("pr_brd");
            int col_class_grp = Qry.FieldIndex("class_grp");
            int col_seat_no = Qry.FieldIndex("seat_no");
            int col_document = Qry.FieldIndex("document");
            int col_ticket_no = Qry.FieldIndex("ticket_no");
            int col_hall = Qry.FieldIndex("hall");
            int col_part_key=Qry.FieldIndex("part_key");

            string trip, scd_out;
            while(!Qry.Eof) {
                xmlNodePtr paxNode = NewTextChild(rowsNode, "pax");

                if(!Qry.FieldIsNULL(col_part_key))
                    NewTextChild(paxNode, "part_key",
                            DateTimeToStr(Qry.FieldAsDateTime(col_part_key), ServerFormatDateTimeAsString));
                NewTextChild(paxNode, "point_id", Qry.FieldAsInteger(col_point_id));
                NewTextChild(paxNode, "airline", Qry.FieldAsString(col_airline));
                NewTextChild(paxNode, "flt_no", Qry.FieldAsInteger(col_flt_no));
                NewTextChild(paxNode, "suffix", Qry.FieldAsString(col_suffix));
                if(trip.empty()) {
                    TTripInfo trip_info(Qry);
                    trip = GetTripName(trip_info, ecCkin);
                    scd_out =
                        DateTimeToStr(
                                UTCToClient( Qry.FieldAsDateTime(col_scd_out), info.desk.tz_region),
                                ServerFormatDateTimeAsString
                                );
                }
                NewTextChild(paxNode, "trip", trip);
                NewTextChild( paxNode, "scd_out", scd_out);
                NewTextChild(paxNode, "reg_no", Qry.FieldAsInteger(col_reg_no));
                NewTextChild(paxNode, "full_name", Qry.FieldAsString(col_full_name));
                NewTextChild(paxNode, "bag_amount", Qry.FieldAsInteger(col_bag_amount));
                NewTextChild(paxNode, "bag_weight", Qry.FieldAsInteger(col_bag_weight));
                NewTextChild(paxNode, "rk_weight", Qry.FieldAsInteger(col_rk_weight));
                NewTextChild(paxNode, "excess", Qry.FieldAsInteger(col_excess));
                NewTextChild(paxNode, "grp_id", Qry.FieldAsInteger(col_grp_id));
                NewTextChild(paxNode, "airp_arv", ElemIdToCodeNative(etAirp, Qry.FieldAsString(col_airp_arv)));
                NewTextChild(paxNode, "tags", Qry.FieldAsString(col_tags));
                string status;
                if(Qry.FieldIsNULL(col_refuse))
                    status = getLocaleText(Qry.FieldAsInteger(col_pr_brd) == 0 ? "Зарег." : "Посаж.");
                else
                    status = getLocaleText("MSG.CANCEL_REG.REFUSAL",
                            LParams() << LParam("refusal", ElemIdToCodeNative(etRefusalType, Qry.FieldAsString(col_refuse))));
                NewTextChild(paxNode, "status", status);
                NewTextChild(paxNode, "class", ElemIdToCodeNative(etClsGrp, Qry.FieldAsInteger(col_class_grp)));
                NewTextChild(paxNode, "seat_no", Qry.FieldAsString(col_seat_no));
                NewTextChild(paxNode, "document", Qry.FieldAsString(col_document));
                NewTextChild(paxNode, "ticket_no", Qry.FieldAsString(col_ticket_no));
                if(Qry.FieldIsNULL(col_hall))
                    NewTextChild(paxNode, "hall");
                else
                    NewTextChild(paxNode, "hall", ElemIdToNameLong(etHall, Qry.FieldAsInteger(col_hall)));

                Qry.Next();
            }
            ProgTrace(TRACE5, "XML: %s", tm.PrintWithMessage().c_str());
        }

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
                "   ckin.get_bagAmount2(pax_grp.grp_id,NULL,null) AS bag_amount, "
                "   ckin.get_bagWeight2(pax_grp.grp_id,NULL,null) AS bag_weight, "
                "   ckin.get_rkWeight2(pax_grp.grp_id,NULL,null) AS rk_weight, "
                "   ckin.get_excess(pax_grp.grp_id,NULL) AS excess, "
                "   ckin.get_birks2(pax_grp.grp_id,NULL,null, :pr_lat) AS tags, "
                "   pax_grp.grp_id, "
                "   pax_grp.hall, "
                "   pax_grp.point_arv,pax_grp.user_id "
                "FROM pax_grp, points "
                "WHERE "
                "   pax_grp.point_dep=:point_id AND "
                "   pax_grp.class IS NULL and "
                "   pax_grp.point_dep = points.point_id and points.pr_del>=0 ";
            if (!info.user.access.airps.empty()) {
                if (info.user.access.airps_permit)
                    SQLText += " AND points.airp IN "+GetSQLEnum(info.user.access.airps);
                else
                    SQLText += " AND points.airp NOT IN "+GetSQLEnum(info.user.access.airps);
            }
            if (!info.user.access.airlines.empty()) {
                if (info.user.access.airlines_permit)
                    SQLText += " AND points.airline IN "+GetSQLEnum(info.user.access.airlines);
                else
                    SQLText += " AND points.airline NOT IN "+GetSQLEnum(info.user.access.airlines);
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
                "  arch.get_bagAmount(arx_pax_grp.part_key,arx_pax_grp.grp_id,NULL) AS bag_amount, "
                "  arch.get_bagWeight(arx_pax_grp.part_key,arx_pax_grp.grp_id,NULL) AS bag_weight, "
                "  arch.get_rkWeight(arx_pax_grp.part_key,arx_pax_grp.grp_id,NULL) AS rk_weight, "
                "  arch.get_excess(arx_pax_grp.part_key,arx_pax_grp.grp_id,NULL) AS excess, "
                "  arch.get_birks(arx_pax_grp.part_key,arx_pax_grp.grp_id,NULL, :pr_lat) AS tags, "
                "  arx_pax_grp.grp_id, "
                "  arx_pax_grp.hall, "
                "  arx_pax_grp.point_arv,arx_pax_grp.user_id "
                "FROM arx_pax_grp, arx_points "
                "WHERE point_dep=:point_id AND class IS NULL and "
                "   arx_pax_grp.part_key = arx_points.part_key and "
                "   arx_pax_grp.point_dep = arx_points.point_id and arx_points.pr_del>=0 and "
                "   arx_pax_grp.part_key = :part_key ";
            if (!info.user.access.airps.empty()) {
                if (info.user.access.airps_permit)
                    SQLText += " AND arx_points.airp IN "+GetSQLEnum(info.user.access.airps);
                else
                    SQLText += " AND arx_points.airp NOT IN "+GetSQLEnum(info.user.access.airps);
            }
            if (!info.user.access.airlines.empty()) {
                if (info.user.access.airlines_permit)
                    SQLText += " AND arx_points.airline IN "+GetSQLEnum(info.user.access.airlines);
                else
                    SQLText += " AND arx_points.airline NOT IN "+GetSQLEnum(info.user.access.airlines);
            }
            Qry.CreateVariable("part_key", otDate, part_key);
        }

        Qry.Execute();

        string trip, scd_out;
        for(;!Qry.Eof;Qry.Next())
        {
            if(!paxListNode) {
                paxListNode = NewTextChild(resNode, "paxList");
                rowsNode = NewTextChild(paxListNode, "rows");
            }
            xmlNodePtr paxNode=NewTextChild(rowsNode,"pax");
            if(!Qry.FieldIsNULL("part_key"))
                NewTextChild(paxNode, "part_key",
                        DateTimeToStr(Qry.FieldAsDateTime("part_key"), ServerFormatDateTimeAsString));
            NewTextChild(paxNode, "point_id", point_id);
            NewTextChild(paxNode, "airline");
            NewTextChild(paxNode, "flt_no", 0);
            NewTextChild(paxNode, "suffix");
            if(trip.empty()) {
                TTripInfo trip_info(Qry);
                trip = GetTripName(trip_info, ecCkin);
                scd_out =
                    DateTimeToStr(
                            UTCToClient( Qry.FieldAsDateTime("scd_out"), info.desk.tz_region),
                            ServerFormatDateTimeAsString
                            );
            }
            NewTextChild(paxNode, "trip", trip);
            NewTextChild( paxNode, "scd_out", scd_out);
            NewTextChild(paxNode, "reg_no", 0);
            NewTextChild(paxNode, "full_name", getLocaleText("Багаж без сопровождения"));
            NewTextChild(paxNode, "bag_amount", Qry.FieldAsInteger("bag_amount"));
            NewTextChild(paxNode, "bag_weight", Qry.FieldAsInteger("bag_weight"));
            NewTextChild(paxNode, "rk_weight", Qry.FieldAsInteger("rk_weight"));
            NewTextChild(paxNode, "excess", Qry.FieldAsInteger("excess"));
            NewTextChild(paxNode, "grp_id", Qry.FieldAsInteger("grp_id"));
            NewTextChild(paxNode, "airp_arv", ElemIdToCodeNative(etAirp, Qry.FieldAsString("airp_arv")));
            NewTextChild(paxNode, "tags", Qry.FieldAsString("tags"));
            NewTextChild(paxNode, "status");
            NewTextChild(paxNode, "class");
            NewTextChild(paxNode, "seat_no");
            NewTextChild(paxNode, "document");
            NewTextChild(paxNode, "ticket_no");
            if(Qry.FieldIsNULL("hall"))
                NewTextChild(paxNode, "hall");
            else
                NewTextChild(paxNode, "hall", ElemIdToNameLong(etHall, Qry.FieldAsInteger("hall")));
        };
        if(paxListNode) { // для совместимости со старой версией терминала
            xmlNodePtr headerNode = NewTextChild(paxListNode, "header");
            NewTextChild(headerNode, "col", "Рейс");
        } else
            throw AstraLocale::UserException("MSG.PASSENGERS.NOT_FOUND");
        tm.Init();
        STAT::set_variables(resNode);
        ProgTrace(TRACE5, "set_variables: %s", tm.PrintWithMessage().c_str());
        tm.Init();
        ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
        ProgTrace(TRACE5, "GetXMLDocText: %s", tm.PrintWithMessage().c_str());

        return;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

xmlNodePtr STAT::set_variables(xmlNodePtr resNode, string lang)
{
    if(lang.empty())
        lang = TReqInfo::Instance()->desk.lang;

    xmlNodePtr formDataNode = GetNode("form_data", resNode);
    if(!formDataNode)
        formDataNode = NewTextChild(resNode, "form_data");
    xmlNodePtr variablesNode = GetNode("variables", formDataNode);
    if(!variablesNode)
        variablesNode = NewTextChild(formDataNode, "variables");

    TReqInfo *reqInfo = TReqInfo::Instance();
    TDateTime issued = UTCToLocal(NowUTC(),reqInfo->desk.tz_region);
    string tz;
    if(reqInfo->user.sets.time == ustTimeUTC)
        tz = "(GMT)";
    else if(
            reqInfo->user.sets.time == ustTimeLocalDesk ||
            reqInfo->user.sets.time == ustTimeLocalAirp
           )
        tz = "(" + ElemIdToCodeNative(etCity, reqInfo->desk.city) + ")";

    NewTextChild(variablesNode, "print_date",
            DateTimeToStr(issued, "dd.mm.yyyy hh:nn:ss ") + tz);
    NewTextChild(variablesNode, "print_oper", reqInfo->user.login);
    NewTextChild(variablesNode, "print_term", reqInfo->desk.code);
    NewTextChild(variablesNode, "use_seances", USE_SEANCES());
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
    return variablesNode;
}

enum TStatType { statTrferFull, statFull, statShort, statDetail };

enum TSeanceType { seanceAirline, seanceAirport, seanceAll };

struct TStatParams {
    vector<string> airlines,airps;
    bool airlines_permit,airps_permit;
    bool airp_column_first;
    TSeanceType seance;
    void get(TQuery &Qry, xmlNodePtr resNode);
};

string GetStatSQLText( TStatType statType, const TStatParams &params, bool pr_arx)
{
    if (!pr_arx)
    {
        string mainSQLText =
            "select \n";
        if (USE_SEANCES())
          mainSQLText +=
            "  decode(trip_sets.pr_airp_seance, null, '', 1, 'АП', 'АК') seance, \n";
        else
          mainSQLText +=
            "  NULL seance, \n";
        if (statType==statTrferFull)
        {
            mainSQLText +=
                "  points.airp, \n"
                "  points.airline, \n"
                "  points.flt_no, \n"
                "  points.scd_out, \n"
                "  trfer_stat.point_id, \n"
                "  trfer_stat.trfer_route places, \n"
                "  adult + child + baby pax_amount, \n"
                "  adult, \n"
                "  child, \n"
                "  baby, \n"
                "  unchecked rk_weight, \n"
                "  pcs bag_amount, \n"
                "  weight bag_weight, \n"
                "  excess \n";
        };
        if (statType==statFull)
        {
            mainSQLText +=
                "  points.airp, \n"
                "  points.airline, \n"
                "  points.flt_no, \n"
                "  points.scd_out, \n"
                "  stat.point_id, \n"
                "  ckin.get_airps(stat.point_id,:vlang) places, \n"
                "  sum(adult + child + baby) pax_amount, \n"
                "  sum(decode(client_type, :web, adult + child + baby, 0)) web, \n"
                "  sum(decode(client_type, :kiosk, adult + child + baby, 0)) kiosk, \n"
                "  sum(adult) adult, \n"
                "  sum(child) child, \n"
                "  sum(baby) baby, \n"
                "  sum(unchecked) rk_weight, \n"
                "  sum(pcs) bag_amount, \n"
                "  sum(weight) bag_weight, \n"
                "  sum(excess) excess \n";
        };
        if (statType==statShort)
        {
            if(params.airp_column_first)
                mainSQLText +=
                    "    points.airp,  \n";
            else
                mainSQLText +=
                    "    points.airline,  \n";
            mainSQLText +=
                "    count(distinct stat.point_id) flt_amount, \n"
                "    sum(decode(client_type, :web, adult + child + baby, 0)) web, \n"
                "    sum(decode(client_type, :kiosk, adult + child + baby, 0)) kiosk, \n"
                "    sum(adult + child + baby) pax_amount \n";
        };
        if (statType==statDetail)
        {
            mainSQLText +=
                "  points.airp, \n"
                "  points.airline, \n"
                "  count(distinct stat.point_id) flt_amount, \n"
                "  sum(decode(client_type, :web, adult + child + baby, 0)) web, \n"
                "  sum(decode(client_type, :kiosk, adult + child + baby, 0)) kiosk, \n"
                "  sum(adult + child + baby) pax_amount \n";
        };
        mainSQLText +=
            "from \n"
            "  points, \n";
        if (statType==statTrferFull)
        {
            mainSQLText +=
                "  trfer_stat \n";
        };
        if (statType==statFull || statType==statShort || statType==statDetail)
        {
            mainSQLText +=
                "  stat \n";
        };
        if (USE_SEANCES())
          mainSQLText += ", trip_sets \n";
        else
        {
          if(params.seance==seanceAirport)
              mainSQLText += ", " + AIRP_PERIODS;
          if(params.seance==seanceAirline)
              mainSQLText += ", " + AIRLINE_PERIODS;
        };
        mainSQLText +=
            "where \n";
        if (USE_SEANCES())
        {
          if (params.seance!=seanceAll)
            mainSQLText += "  trip_sets.pr_airp_seance = :pr_airp_seance and \n";
        };

        if (statType==statTrferFull)
        {
            mainSQLText +=
                "  points.point_id = trfer_stat.point_id and points.pr_del>=0 and \n";
        };
        if (statType==statFull || statType==statShort || statType==statDetail)
        {
            mainSQLText +=
                "  points.point_id = stat.point_id and points.pr_del>=0 and \n";
        };
        if (USE_SEANCES())
        {
          mainSQLText +=
              "  points.scd_out >= :FirstDate AND points.scd_out < :LastDate and \n"
              "  points.point_id = trip_sets.point_id \n";
        }
        else
        {
          if (params.seance==seanceAirport ||
              params.seance==seanceAirline)
              mainSQLText +=
                  "  points.scd_out >= periods.period_first_date AND points.scd_out < periods.period_last_date  \n";
          else
              mainSQLText +=
                  "  points.scd_out >= :FirstDate AND points.scd_out < :LastDate \n";
        };
        if (!USE_SEANCES() && params.seance==seanceAirport)
        {
          mainSQLText +=
              " AND points.airp = :ap \n";
        }
        else
        {
          if (!params.airps.empty()) {
              if (params.airps_permit)
                  mainSQLText += " AND points.airp IN "+GetSQLEnum(params.airps)+"\n";
              else
                  mainSQLText += " AND points.airp NOT IN "+GetSQLEnum(params.airps)+"\n";
          };
        };

        if (!USE_SEANCES() && params.seance==seanceAirline)
        {
          mainSQLText +=
              " AND points.airline = :ak \n";
        }
        else
        {
          if (!params.airlines.empty()) {
              if (params.airlines_permit)
                  mainSQLText += " AND points.airline IN "+GetSQLEnum(params.airlines)+"\n";
              else
                  mainSQLText += " AND points.airline NOT IN "+GetSQLEnum(params.airlines)+"\n";
          }
        };

        if (!USE_SEANCES())
        {
          if(params.seance==seanceAirport)
            mainSQLText +=
              " and points.airline not in " + AIRLINE_LIST + "\n";
          if(params.seance==seanceAirline)
            mainSQLText +=
              " and points.airp in " + AIRP_LIST + "\n";
        };

        if (statType==statFull)
        {
            mainSQLText +=
                "group by \n";
            if (USE_SEANCES())
              mainSQLText +=
                "  trip_sets.pr_airp_seance, \n";
            mainSQLText +=
                "  points.airp, \n"
                "  points.airline, \n"
                "  points.flt_no, \n"
                "  points.scd_out, \n"
                "  stat.point_id \n";
        };
        if (statType==statShort)
        {
            mainSQLText +=
                "group by \n";
            if (USE_SEANCES())
              mainSQLText +=
                "  trip_sets.pr_airp_seance, \n";
            if(params.airp_column_first)
                mainSQLText +=
                    "    points.airp \n";
            else
                mainSQLText +=
                    "    points.airline \n";
        };
        if (statType==statDetail)
        {
            mainSQLText +=
                "group by \n";
            if (USE_SEANCES())
              mainSQLText +=
                "  trip_sets.pr_airp_seance, \n";
            mainSQLText +=
                "  points.airp, \n"
                "  points.airline \n";
        };
        return mainSQLText;
    }
    else
    {
        string arxSQLText =
            "select \n";
        if (USE_SEANCES())
          arxSQLText +=
            "  decode(arx_trip_sets.pr_airp_seance, null, '', 1, 'АП', 'АК') seance, \n";
        else
          arxSQLText +=
            "  NULL seance, \n";
        if (statType==statTrferFull)
        {
            arxSQLText +=
                "  arx_points.airp, \n"
                "  arx_points.airline, \n"
                "  arx_points.flt_no, \n"
                "  arx_points.scd_out, \n"
                "  arx_trfer_stat.point_id, \n"
                "  arx_trfer_stat.trfer_route places, \n"
                "  adult + child + baby pax_amount, \n"
                "  adult, \n"
                "  child, \n"
                "  baby, \n"
                "  unchecked rk_weight, \n"
                "  pcs bag_amount, \n"
                "  weight bag_weight, \n"
                "  excess \n";
        };
        if (statType==statFull)
        {
            arxSQLText +=
                "  arx_points.airp, \n"
                "  arx_points.airline, \n"
                "  arx_points.flt_no, \n"
                "  arx_points.scd_out, \n"
                "  arx_stat.point_id, \n"
                "  arch.get_airps(arx_stat.point_id, arx_stat.part_key,:vlang) places, \n"
                "  sum(adult + child + baby) pax_amount, \n"
                "  sum(decode(client_type, :web, adult + child + baby, 0)) web, \n"
                "  sum(decode(client_type, :kiosk, adult + child + baby, 0)) kiosk, \n"
                "  sum(adult) adult, \n"
                "  sum(child) child, \n"
                "  sum(baby) baby, \n"
                "  sum(unchecked) rk_weight, \n"
                "  sum(pcs) bag_amount, \n"
                "  sum(weight) bag_weight, \n"
                "  sum(excess) excess \n";
        };
        if (statType==statShort)
        {
            if(params.airp_column_first)
                arxSQLText +=
                    "    arx_points.airp,  \n";
            else
                arxSQLText +=
                    "    arx_points.airline,  \n";
            arxSQLText +=
                "    count(distinct arx_stat.point_id) flt_amount, \n"
                "    sum(adult + child + baby) pax_amount, \n"
                "    sum(decode(client_type, :web, adult + child + baby, 0)) web, \n"
                "    sum(decode(client_type, :kiosk, adult + child + baby, 0)) kiosk \n";
        };
        if (statType==statDetail)
        {
            arxSQLText +=
                "  arx_points.airp, \n"
                "  arx_points.airline, \n"
                "  count(distinct arx_stat.point_id) flt_amount, \n"
                "  sum(adult + child + baby) pax_amount, \n"
                "  sum(decode(client_type, :web, adult + child + baby, 0)) web, \n"
                "  sum(decode(client_type, :kiosk, adult + child + baby, 0)) kiosk \n";
        };
        arxSQLText +=
            "from \n"
            "  arx_points, \n";
        if (statType==statTrferFull)
        {
            arxSQLText +=
                "  arx_trfer_stat \n";
        };
        if (statType==statFull || statType==statShort || statType==statDetail)
        {
            arxSQLText +=
                "  arx_stat \n";
        };
        if (USE_SEANCES())
          arxSQLText += ", arx_trip_sets \n";
        else
        {
          if(params.seance==seanceAirport)
              arxSQLText += ", " + AIRP_PERIODS;
          if(params.seance==seanceAirline)
              arxSQLText += ", " + AIRLINE_PERIODS;
        };
        arxSQLText +=
            "where \n";
        if (USE_SEANCES())
        {
          if (params.seance!=seanceAll)
            arxSQLText += "  arx_trip_sets.pr_airp_seance = :pr_airp_seance and \n";
        };

        if (USE_SEANCES())
        {
          arxSQLText +=
            "  arx_points.part_key >= :FirstDate AND arx_points.part_key < :LastDate + :arx_trip_date_range AND \n"
            "  arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate AND \n";
          arxSQLText +=
            "  arx_points.part_key = arx_trip_sets.part_key AND \n"
            "  arx_points.point_id = arx_trip_sets.point_id AND \n";
        }
        else
        {
          if (params.seance==seanceAirport ||
              params.seance==seanceAirline)
            arxSQLText +=
              "  arx_points.part_key >= periods.period_first_date AND arx_points.part_key < periods.period_last_date + :arx_trip_date_range AND "
              "  arx_points.scd_out >= periods.period_first_date AND arx_points.scd_out < periods.period_last_date  AND ";

          else
            arxSQLText +=
              "  arx_points.part_key >= :FirstDate AND arx_points.part_key < :LastDate + :arx_trip_date_range AND "
              "  arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate AND ";

        };
        arxSQLText +=
            "  arx_points.pr_del>=0 AND \n";
        if (statType==statTrferFull)
        {
            arxSQLText +=
                "  arx_points.part_key = arx_trfer_stat.part_key AND \n"
                "  arx_points.point_id = arx_trfer_stat.point_id \n";
        };
        if (statType==statFull || statType==statShort || statType==statDetail)
        {
            arxSQLText +=
                "  arx_points.part_key = arx_stat.part_key AND \n"
                "  arx_points.point_id = arx_stat.point_id \n";
        };
        if (!USE_SEANCES() && params.seance==seanceAirport)
        {
          arxSQLText +=
                " and arx_points.airp = :ap \n";
        }
        else
        {
          if (!params.airps.empty()) {
              if (params.airps_permit)
                  arxSQLText += " AND arx_points.airp IN "+GetSQLEnum(params.airps);
              else
                  arxSQLText += " AND arx_points.airp NOT IN "+GetSQLEnum(params.airps);
          }
        };

        if (!USE_SEANCES() && params.seance==seanceAirline)
        {
          arxSQLText +=
                " and arx_points.airline = :ak \n";
        }
        else
        {
          if (!params.airlines.empty()) {
              if (params.airlines_permit)
                  arxSQLText += " AND arx_points.airline IN "+GetSQLEnum(params.airlines);
              else
                  arxSQLText += " AND arx_points.airline NOT IN "+GetSQLEnum(params.airlines);
          }
        };

        if (!USE_SEANCES())
        {
          if(params.seance==seanceAirport)
            arxSQLText +=
              " and arx_points.airline not in " + AIRLINE_LIST + "\n";
          if(params.seance==seanceAirline)
            arxSQLText +=
              " and arx_points.airp in " + AIRP_LIST + "\n";
        };
        if (statType==statFull)
        {
            arxSQLText +=
                "group by \n";
            if (USE_SEANCES())
              arxSQLText +=
                "  arx_trip_sets.pr_airp_seance, \n";
            arxSQLText +=
                "  arx_points.airp, \n"
                "  arx_points.airline, \n"
                "  arx_points.flt_no, \n"
                "  arx_points.scd_out, \n"
                "  arx_stat.point_id, \n"
                "  arx_stat.part_key \n";
        };
        if (statType==statShort)
        {
            arxSQLText +=
                "group by  \n";
            if (USE_SEANCES())
              arxSQLText +=
                "  arx_trip_sets.pr_airp_seance, \n";
            if(params.airp_column_first)
                arxSQLText +=
                    "    arx_points.airp \n";
            else
                arxSQLText +=
                    "    arx_points.airline \n";
        };
        if (statType==statDetail)
        {
            arxSQLText +=
                "group by \n";
            if (USE_SEANCES())
              arxSQLText +=
                "  arx_trip_sets.pr_airp_seance, \n";
            arxSQLText +=
                "  arx_points.airp, \n"
                "  arx_points.airline \n";
        };
        return arxSQLText;
    };
}

struct TPrintAirline {
    private:
        string val;
        bool multi_airlines;
    public:
        TPrintAirline(): multi_airlines(false) {};
        void check(string val);
        string get();
};

string TPrintAirline::get()
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

void TStatParams::get(TQuery &Qry, xmlNodePtr reqNode)
{
    TReqInfo &info = *(TReqInfo::Instance());

    xmlNodePtr curNode = reqNode->children;

    string ak = NodeAsStringFast("ak", curNode);
    string ap = NodeAsStringFast("ap", curNode);

    //составим вектор доступных компаний
    if (ak.empty())
    {
      //не указан фильтр по компании
      airlines.assign(info.user.access.airlines.begin(),info.user.access.airlines.end());
      airlines_permit=info.user.access.airlines_permit;
    }
    else
    {
      //проверим среди запрещенных/разрешенных
      bool found=find( info.user.access.airlines.begin(),
                       info.user.access.airlines.end(), ak ) != info.user.access.airlines.end();
      if ( info.user.access.airlines_permit &&  found ||
          !info.user.access.airlines_permit && !found) airlines.push_back(ak);
      airlines_permit=true;
    };

    //составим вектор доступных портов
    if (ap.empty())
    {
      //не указан фильтр по компании
      airps.assign(info.user.access.airps.begin(),info.user.access.airps.end());
      airps_permit=info.user.access.airps_permit;
    }
    else
    {
      //проверим среди запрещенных/разрешенных
      bool found=find( info.user.access.airps.begin(),
                       info.user.access.airps.end(), ap ) != info.user.access.airps.end();
      if ( info.user.access.airps_permit &&  found ||
          !info.user.access.airps_permit && !found) airps.push_back(ap);
      airps_permit=true;
    };

    if (airlines.empty() && airlines_permit ||
        airps.empty() && airps_permit)
      throw AstraLocale::UserException("MSG.NO_ACCESS");

    airp_column_first = (info.user.user_type == utAirport);

    //сеансы (договоры)
    string seance_str = NodeAsStringFast("seance", curNode, "");
    seance = seanceAll;
    if (seance_str=="АК") seance=seanceAirline;
    if (seance_str=="АП") seance=seanceAirport;

    bool all_seances_permit = find( info.user.access.rights.begin(),
                                    info.user.access.rights.end(), 615 ) != info.user.access.rights.end();

    if (USE_SEANCES() || info.desk.compatible(AIRL_AIRP_STAT_VERSION))
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

    if (!USE_SEANCES() && seance==seanceAirline)
    {
      if (!airlines_permit) throw UserException("MSG.NEED_SET_CODE_AIRLINE");
    };

    if (!USE_SEANCES() && seance==seanceAirport)
    {
      if (!airps_permit) throw UserException("MSG.NEED_SET_CODE_AIRP");
    };
};

struct TDetailStatRow {
    int flt_amount, pax_amount, web, kiosk;
    TDetailStatRow():
        flt_amount(NoExists),
        pax_amount(NoExists),
        web(NoExists),
        kiosk(NoExists)
    {}
};

struct TDetailStatKey {
    string seance, col1, col2;
};
struct TDetailCmp {
    bool operator() (const TDetailStatKey &lr, const TDetailStatKey &rr) const
    {
        if(lr.seance == rr.seance)
            if(lr.col1 == rr.col1)
                return lr.col2 < rr.col2;
            else
                return lr.col1 < rr.col1;
        else
            return lr.seance < rr.seance;
    };
};
typedef map<TDetailStatKey, TDetailStatRow, TDetailCmp> TDetailStat;

void GetDetailStat(TStatType statType, const TStatParams &params, TQuery &Qry,
                   TDetailStat &DetailStat, TPrintAirline &airline)
{
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next()) {
      TDetailStatKey key;
      key.seance = Qry.FieldAsString("seance");
      if(params.airp_column_first) {
          key.col1 = ElemIdToCodeNative(etAirp, Qry.FieldAsString("airp"));
          if (statType==statDetail)
          {
            key.col2 = ElemIdToCodeNative(etAirline, Qry.FieldAsString("airline"));
            airline.check(key.col2);
          };
      } else {
          key.col1 = ElemIdToCodeNative(etAirline, Qry.FieldAsString("airline"));
          if (statType==statDetail)
          {
            key.col2 = ElemIdToCodeNative(etAirp, Qry.FieldAsString("airp"));
          };
          airline.check(key.col1);
      }
      TDetailStatRow &row = DetailStat[key];
      if(row.flt_amount == NoExists) {
          row.flt_amount = Qry.FieldAsInteger("flt_amount");
          row.pax_amount = Qry.FieldAsInteger("pax_amount");
          row.web = Qry.FieldAsInteger("web");
          row.kiosk = Qry.FieldAsInteger("kiosk");
      } else {
          row.flt_amount += Qry.FieldAsInteger("flt_amount");
          row.pax_amount += Qry.FieldAsInteger("pax_amount");
          row.web += Qry.FieldAsInteger("web");
          row.kiosk += Qry.FieldAsInteger("kiosk");
      }
  }
};

void RunDetailStat(TStatType statType, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo &info = *(TReqInfo::Instance());
    if (info.user.access.airlines.empty() && info.user.access.airlines_permit ||
            info.user.access.airps.empty() && info.user.access.airps_permit)
        throw AstraLocale::UserException("MSG.NOT_DATA");

    if (statType==statShort)
      get_report_form("ShortStat", reqNode, resNode);
    else
      get_report_form("DetailStat", reqNode, resNode);

    TQuery Qry(&OraSession);
    TStatParams params;
    params.get(Qry, reqNode);

    Qry.CreateVariable("FirstDate", otDate, NodeAsDateTime("FirstDate", reqNode));
    Qry.CreateVariable("LastDate", otDate, NodeAsDateTime("LastDate", reqNode));
    Qry.CreateVariable("web", otString, EncodeClientType(ctWeb));
    Qry.CreateVariable("kiosk", otString, EncodeClientType(ctKiosk));
    if (!USE_SEANCES() && params.seance==seanceAirline) Qry.DeclareVariable("ak",otString);
    if (!USE_SEANCES() && params.seance==seanceAirport) Qry.DeclareVariable("ap",otString);
    if (USE_SEANCES() && params.seance!=seanceAll)
        Qry.CreateVariable("pr_airp_seance", otInteger, (int)(params.seance==seanceAirport));

    TDetailStat DetailStat;
    TPrintAirline airline;

    for(int pass = 0; pass < 2; pass++) {
        Qry.SQLText = GetStatSQLText(statType,params,pass!=0).c_str();
        if(pass != 0)
            Qry.CreateVariable("arx_trip_date_range", otInteger, arx_trip_date_range);
        ProgTrace(TRACE5, "RunDetailStat: SQL=\n%s", Qry.SQLText.SQLText());

        if (!USE_SEANCES() && params.seance==seanceAirline)
        {
          //цикл по компаниям
          if (params.airlines_permit)
          {
            for(vector<string>::iterator i=params.airlines.begin();
                                         i!=params.airlines.end(); i++)
            {
              Qry.SetVariable("ak",*i);
              GetDetailStat(statType, params, Qry, DetailStat, airline);
            };
          };
          continue;
        };

        if (!USE_SEANCES() && params.seance==seanceAirport)
        {
          //цикл по портам
          if (params.airps_permit)
          {
            for(vector<string>::iterator i=params.airps.begin();
                                         i!=params.airps.end(); i++)
            {
              Qry.SetVariable("ap",*i);
              GetDetailStat(statType, params, Qry, DetailStat, airline);
            };
          };
          continue;
        };
        GetDetailStat(statType, params, Qry, DetailStat, airline);
    }

    if(!DetailStat.empty()) {
        NewTextChild(resNode, "airline", airline.get(), "");
        xmlNodePtr grdNode = NewTextChild(resNode, "grd");
        xmlNodePtr headerNode = NewTextChild(grdNode, "header");
        xmlNodePtr colNode;
        if(params.airp_column_first) {
            colNode = NewTextChild(headerNode, "col", getLocaleText("Код а/п"));
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", taLeftJustify);
            if (statType==statDetail)
            {
              colNode = NewTextChild(headerNode, "col", getLocaleText("Код а/к"));
              SetProp(colNode, "width", 50);
              SetProp(colNode, "align", taLeftJustify);
            };
        } else {
            colNode = NewTextChild(headerNode, "col", getLocaleText("Код а/к"));
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", taLeftJustify);
            if (statType==statDetail)
            {
              colNode = NewTextChild(headerNode, "col", getLocaleText("Код а/п"));
              SetProp(colNode, "width", 50);
              SetProp(colNode, "align", taLeftJustify);
            };
        }
        if (USE_SEANCES())
        {
          colNode = NewTextChild(headerNode, "col", getLocaleText("Сеанс"));
          SetProp(colNode, "width", 40);
          SetProp(colNode, "align", taLeftJustify);
        };

        colNode = NewTextChild(headerNode, "col", getLocaleText("Кол-во рейсов"));
        SetProp(colNode, "width", 85);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", getLocaleText("Кол-во пасс."));
        SetProp(colNode, "width", 85);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", getLocaleText("Web"));
        SetProp(colNode, "width", 85);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", getLocaleText("Киоски"));
        SetProp(colNode, "width", 85);
        SetProp(colNode, "align", taRightJustify);

        xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
        xmlNodePtr rowNode;
        int total_flt_amount = 0;
        int total_pax_amount = 0;
        int total_web = 0;
        int total_kiosk = 0;
        for(TDetailStat::iterator si = DetailStat.begin(); si != DetailStat.end(); si++) {
            rowNode = NewTextChild(rowsNode, "row");
            NewTextChild(rowNode, "col", si->first.col1);
            if (statType==statDetail)
              NewTextChild(rowNode, "col", si->first.col2);

            total_flt_amount += si->second.flt_amount;
            total_pax_amount += si->second.pax_amount;
            total_web += si->second.web;
            total_kiosk += si->second.kiosk;

            if (USE_SEANCES())
              NewTextChild(rowNode, "col", getLocaleText(si->first.seance));
            NewTextChild(rowNode, "col", si->second.flt_amount);
            NewTextChild(rowNode, "col", si->second.pax_amount);
            NewTextChild(rowNode, "col", si->second.web);
            NewTextChild(rowNode, "col", si->second.kiosk);
            Qry.Next();
        }
        rowNode = NewTextChild(rowsNode, "row");
        NewTextChild(rowNode, "col", getLocaleText("Итого:"));
        if (statType==statDetail)
          NewTextChild(rowNode, "col");
        if (USE_SEANCES())
        {
          NewTextChild(rowNode, "col");
        };
        NewTextChild(rowNode, "col", total_flt_amount);
        NewTextChild(rowNode, "col", total_pax_amount);
        NewTextChild(rowNode, "col", total_web);
        NewTextChild(rowNode, "col", total_kiosk);
    } else
        throw AstraLocale::UserException("MSG.NOT_DATA");
    STAT::set_variables(resNode);
}

struct TFullStatRow {
    int pax_amount;
    int web;
    int kiosk;
    int adult;
    int child;
    int baby;
    int rk_weight;
    int bag_amount;
    int bag_weight;
    int excess;
    TFullStatRow():
        pax_amount(NoExists),
        web(NoExists),
        kiosk(NoExists),
        adult(NoExists),
        child(NoExists),
        baby(NoExists),
        rk_weight(NoExists),
        bag_amount(NoExists),
        bag_weight(NoExists),
        excess(NoExists)
    {}
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
    string seance, col1, col2;
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
};
struct TFullCmp {
    bool operator() (const TFullStatKey &lr, const TFullStatKey &rr) const
    {
        if(lr.seance == rr.seance)
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
        else
            return lr.seance < rr.seance;
    };
};
typedef map<TFullStatKey, TFullStatRow, TFullCmp> TFullStat;

void GetFullStat(TStatType statType, const TStatParams &params, TQuery &Qry,
                 TFullStat &FullStat, TPrintAirline &airline)
{
  Qry.Execute();
  if(!Qry.Eof) {
      int col_seance = Qry.FieldIndex("seance");
      int col_point_id = Qry.FieldIndex("point_id");
      int col_airp = Qry.FieldIndex("airp");
      int col_airline = Qry.FieldIndex("airline");
      int col_pax_amount = Qry.FieldIndex("pax_amount");
      int col_web = -1;
      int col_kiosk = -1;
      if (statType==statFull)
      {
        col_web = Qry.FieldIndex("web");
        col_kiosk = Qry.FieldIndex("kiosk");
      };
      int col_adult = Qry.FieldIndex("adult");
      int col_child = Qry.FieldIndex("child");
      int col_baby = Qry.FieldIndex("baby");
      int col_rk_weight = Qry.FieldIndex("rk_weight");
      int col_bag_amount = Qry.FieldIndex("bag_amount");
      int col_bag_weight = Qry.FieldIndex("bag_weight");
      int col_excess = Qry.FieldIndex("excess");
      int col_flt_no = Qry.FieldIndex("flt_no");
      int col_scd_out = Qry.FieldIndex("scd_out");
      int col_places = Qry.FieldIndex("places");
      for(; !Qry.Eof; Qry.Next()) {
          TFullStatKey key;
          key.seance = Qry.FieldAsString(col_seance);
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
          key.places.set(Qry.FieldAsString(col_places), statType==statTrferFull);
          TFullStatRow &row = FullStat[key];
          if(row.pax_amount == NoExists) {
              row.pax_amount = Qry.FieldAsInteger(col_pax_amount);
              if (statType==statFull)
              {
                row.web = Qry.FieldAsInteger(col_web);
                row.kiosk = Qry.FieldAsInteger(col_kiosk);
              };
              row.adult = Qry.FieldAsInteger(col_adult);
              row.child = Qry.FieldAsInteger(col_child);
              row.baby = Qry.FieldAsInteger(col_baby);
              row.rk_weight = Qry.FieldAsInteger(col_rk_weight);
              row.bag_amount = Qry.FieldAsInteger(col_bag_amount);
              row.bag_weight = Qry.FieldAsInteger(col_bag_weight);
              row.excess = Qry.FieldAsInteger(col_excess);
          } else {
              row.pax_amount += Qry.FieldAsInteger(col_pax_amount);
              if (statType==statFull)
              {
                row.web += Qry.FieldAsInteger(col_web);
                row.kiosk += Qry.FieldAsInteger(col_kiosk);
              };
              row.adult += Qry.FieldAsInteger(col_adult);
              row.child += Qry.FieldAsInteger(col_child);
              row.baby += Qry.FieldAsInteger(col_baby);
              row.rk_weight += Qry.FieldAsInteger(col_rk_weight);
              row.bag_amount += Qry.FieldAsInteger(col_bag_amount);
              row.bag_weight += Qry.FieldAsInteger(col_bag_weight);
              row.excess += Qry.FieldAsInteger(col_excess);
          }
      }
  }
};

void RunFullStat(TStatType statType, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo &info = *(TReqInfo::Instance());
    if (info.user.access.airlines.empty() && info.user.access.airlines_permit ||
            info.user.access.airps.empty() && info.user.access.airps_permit)
        throw AstraLocale::UserException("MSG.NOT_DATA");
    if (statType==statFull)
      get_report_form("FullStat", reqNode, resNode);
    else
      get_report_form("TrferFullStat", reqNode, resNode);

    TQuery Qry(&OraSession);
    TStatParams params;
    params.get(Qry, reqNode);

    TDateTime FirstDate = NodeAsDateTime("FirstDate", reqNode);
    TDateTime LastDate = NodeAsDateTime("LastDate", reqNode);
    if(IncMonth(FirstDate, 1) < LastDate)
        throw AstraLocale::UserException("MSG.SEARCH_PERIOD_SHOULD_NOT_EXCEED_ONE_MONTH");
    Qry.CreateVariable("FirstDate", otDate, FirstDate);
    Qry.CreateVariable("LastDate", otDate, LastDate);
    if (statType==statFull)
    {
      Qry.CreateVariable("web", otString, EncodeClientType(ctWeb));
      Qry.CreateVariable("kiosk", otString, EncodeClientType(ctKiosk));
      Qry.CreateVariable("vlang", otString, TReqInfo::Instance()->desk.lang );
    };
    if (!USE_SEANCES() && params.seance==seanceAirline) Qry.DeclareVariable("ak",otString);
    if (!USE_SEANCES() && params.seance==seanceAirport) Qry.DeclareVariable("ap",otString);
    if (USE_SEANCES() && params.seance!=seanceAll)
        Qry.CreateVariable("pr_airp_seance", otInteger, (int)(params.seance==seanceAirport));

    TFullStat FullStat;
    TPrintAirline airline;

    for(int pass = 0; pass < 2; pass++) {
        Qry.SQLText = GetStatSQLText(statType,params,pass!=0).c_str();
        if(pass != 0)
            Qry.CreateVariable("arx_trip_date_range", otInteger, arx_trip_date_range);
        ProgTrace(TRACE5, "RunFullStat: SQL=\n%s", Qry.SQLText.SQLText());

        if (!USE_SEANCES() && params.seance==seanceAirline)
        {
          //цикл по компаниям
          if (params.airlines_permit)
          {
            for(vector<string>::iterator i=params.airlines.begin();
                                         i!=params.airlines.end(); i++)
            {
              Qry.SetVariable("ak",*i);
              GetFullStat(statType, params, Qry, FullStat, airline);
            };
          };
          continue;
        };

        if (!USE_SEANCES() && params.seance==seanceAirport)
        {
          //цикл по портам
          if (params.airps_permit)
          {
            for(vector<string>::iterator i=params.airps.begin();
                                         i!=params.airps.end(); i++)
            {
              Qry.SetVariable("ap",*i);
              GetFullStat(statType, params, Qry, FullStat, airline);
            };
          };
          continue;
        };
        GetFullStat(statType, params, Qry, FullStat, airline);
    }

    if(!FullStat.empty()) {
        NewTextChild(resNode, "airline", airline.get(), "");
        xmlNodePtr grdNode = NewTextChild(resNode, "grd");
        xmlNodePtr headerNode = NewTextChild(grdNode, "header");
        xmlNodePtr colNode;
        if(params.airp_column_first) {
            colNode = NewTextChild(headerNode, "col", getLocaleText("Код а/п"));
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", taLeftJustify);

            colNode = NewTextChild(headerNode, "col", getLocaleText("Код а/к"));
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", taLeftJustify);
        } else {
            colNode = NewTextChild(headerNode, "col", getLocaleText("Код а/к"));
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", taLeftJustify);

            colNode = NewTextChild(headerNode, "col", getLocaleText("Код а/п"));
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", taLeftJustify);
        }
        colNode = NewTextChild(headerNode, "col", getLocaleText("Номер рейса"));
        SetProp(colNode, "width", 75);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", getLocaleText("Дата"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", taLeftJustify);

        colNode = NewTextChild(headerNode, "col", getLocaleText("Направление"));
        SetProp(colNode, "width", 90);
        SetProp(colNode, "align", taLeftJustify);

        if (USE_SEANCES())
        {
          colNode = NewTextChild(headerNode, "col", getLocaleText("Сеанс"));
          SetProp(colNode, "width", 40);
          SetProp(colNode, "align", taLeftJustify);
        };

        colNode = NewTextChild(headerNode, "col", getLocaleText("Кол-во пасс."));
        SetProp(colNode, "width", 75);
        SetProp(colNode, "align", taRightJustify);

        if (statType==statFull)
        {
          colNode = NewTextChild(headerNode, "col", getLocaleText("Web"));
          SetProp(colNode, "width", 35);
          SetProp(colNode, "align", taRightJustify);

          colNode = NewTextChild(headerNode, "col", getLocaleText("Киоски"));
          SetProp(colNode, "width", 40);
          SetProp(colNode, "align", taRightJustify);
        };

        colNode = NewTextChild(headerNode, "col", getLocaleText("ВЗ"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", getLocaleText("РБ"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", getLocaleText("РМ"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", getLocaleText("Р/кладь (вес)"));
        SetProp(colNode, "width", 80);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", getLocaleText("Багаж (мест/вес)"));
        SetProp(colNode, "width", 100);
        SetProp(colNode, "align", taCenter);

        colNode = NewTextChild(headerNode, "col", getLocaleText("Платн. (вес)"));
        SetProp(colNode, "width", 70);
        SetProp(colNode, "align", taRightJustify);

        xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
        xmlNodePtr rowNode;
        int total_pax_amount = 0;
        int total_web = 0;
        int total_kiosk = 0;
        int total_adult = 0;
        int total_child = 0;
        int total_baby = 0;
        int total_rk_weight = 0;
        int total_bag_amount = 0;
        int total_bag_weight = 0;
        int total_excess = 0;
        for(TFullStat::iterator im = FullStat.begin(); im != FullStat.end(); im++) {
            string region;
            try
            {
                region = AirpTZRegion(im->first.airp);
            }
            catch(AstraLocale::UserException &E)
            {
                AstraLocale::showErrorMessage("MSG.ERR_MSG.NOT_ALL_FLIGHTS_ARE_SHOWN", LParams() << LParam("msg", getLocaleText(E.getLexemaData())));
                continue;
            };

            rowNode = NewTextChild(rowsNode, "row");
            NewTextChild(rowNode, "col", im->first.col1);
            NewTextChild(rowNode, "col", im->first.col2);

            total_pax_amount += im->second.pax_amount;
            if (statType==statFull)
            {
              total_web += im->second.web;
              total_kiosk += im->second.kiosk;
            };
            total_adult += im->second.adult;
            total_child += im->second.child;
            total_baby += im->second.baby;
            total_rk_weight += im->second.rk_weight;
            total_bag_amount += im->second.bag_amount;
            total_bag_weight += im->second.bag_weight;
            total_excess += im->second.excess;

            NewTextChild(rowNode, "col", im->first.flt_no);
            NewTextChild(rowNode, "col", DateTimeToStr(
                        UTCToClient(im->first.scd_out, region), "dd.mm.yy")
                    );
            NewTextChild(rowNode, "col", im->first.places.get());
            if (USE_SEANCES())
              NewTextChild(rowNode, "col", getLocaleText(im->first.seance));
            NewTextChild(rowNode, "col", im->second.pax_amount);
            if (statType==statFull)
            {
              NewTextChild(rowNode, "col", im->second.web);
              NewTextChild(rowNode, "col", im->second.kiosk);
            };
            NewTextChild(rowNode, "col", im->second.adult);
            NewTextChild(rowNode, "col", im->second.child);
            NewTextChild(rowNode, "col", im->second.baby);
            NewTextChild(rowNode, "col", im->second.rk_weight);
            NewTextChild(rowNode, "col", IntToString(im->second.bag_amount) + "/" + IntToString(im->second.bag_weight));
            NewTextChild(rowNode, "col", im->second.excess);
            if (statType==statTrferFull)
              NewTextChild(rowNode, "col", im->first.point_id);
        }
        rowNode = NewTextChild(rowsNode, "row");
        NewTextChild(rowNode, "col", getLocaleText("Итого:"));
        NewTextChild(rowNode, "col");
        NewTextChild(rowNode, "col");
        NewTextChild(rowNode, "col");
        NewTextChild(rowNode, "col");
        if (USE_SEANCES())
        {
          NewTextChild(rowNode, "col");
        };
        NewTextChild(rowNode, "col", total_pax_amount);
        if (statType==statFull)
        {
          NewTextChild(rowNode, "col", total_web);
          NewTextChild(rowNode, "col", total_kiosk);
        };
        NewTextChild(rowNode, "col", total_adult);
        NewTextChild(rowNode, "col", total_child);
        NewTextChild(rowNode, "col", total_baby);
        NewTextChild(rowNode, "col", total_rk_weight);
        NewTextChild(rowNode, "col", IntToString(total_bag_amount) + "/" + IntToString(total_bag_weight));
        NewTextChild(rowNode, "col", total_excess);
    } else
        throw AstraLocale::UserException("MSG.NOT_DATA");
    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    if (statType==statFull)
      NewTextChild(variablesNode, "caption", getLocaleText("Подробная сводка"));
    else
      NewTextChild(variablesNode, "caption", getLocaleText("Трансферная сводка"));
}

void StatInterface::RunStat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	TReqInfo *reqInfo = TReqInfo::Instance();
    if(find( reqInfo->user.access.rights.begin(),
                reqInfo->user.access.rights.end(), 600 ) == reqInfo->user.access.rights.end())
        throw AstraLocale::UserException("MSG.INSUFFICIENT_RIGHTS.NOT_ACCESS");

    string name = NodeAsString("stat_mode", reqNode);

    try {
        if(name == "Подробная") RunFullStat(statFull, reqNode, resNode);
        else if(name == "Общая") RunDetailStat(statShort, reqNode, resNode);
        else if(name == "Детализированная") RunDetailStat(statDetail, reqNode, resNode);
        else if(name == "Трансфер") RunFullStat(statTrferFull, reqNode, resNode);
        else throw Exception("Unknown stat mode " + name);
    } catch (EOracleError &E) {
        if(E.Code == 376)
            throw AstraLocale::UserException("MSG.ONE_OF_DB_FILES_UNAVAILABLE.CALL_ADMIN");
        else
            throw;
    }
}

void StatInterface::PaxSrcRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo &info = *(TReqInfo::Instance());
    if(find( info.user.access.rights.begin(),
                info.user.access.rights.end(), 620 ) == info.user.access.rights.end())
        throw AstraLocale::UserException("MSG.PAX_SRC.ACCESS_DENIED");
    if (info.user.access.airlines.empty() && info.user.access.airlines_permit ||
            info.user.access.airps.empty() && info.user.access.airps_permit)
        throw AstraLocale::UserException("MSG.PASSENGERS.NOT_FOUND");
    TDateTime FirstDate = NodeAsDateTime("FirstDate", reqNode);
    TDateTime LastDate = NodeAsDateTime("LastDate", reqNode);
    if(IncMonth(FirstDate, 3) < LastDate)
        throw AstraLocale::UserException("MSG.SEARCH_PERIOD_SHOULD_NOT_EXCEED_THREE_MONTHS");
    FirstDate = ClientToUTC(FirstDate, info.desk.tz_region);
    LastDate = ClientToUTC(LastDate, info.desk.tz_region);
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
    xmlNodePtr paxListNode = NULL;
    xmlNodePtr rowsNode = NULL;
    for(int i = 0; (i < 2) && (count < MAX_STAT_ROWS); i++) {
        string SQLText;
        if(i == 0) {
            ProgTrace(TRACE5, "PaxSrcRun: current base qry");
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
                "   pax_grp.grp_id, "
                "   ckin.get_birks2(pax.grp_id,pax.pax_id,pax.bag_pool_num,:pr_lat) tags, "
                "   pax.pr_brd, "
                "   pax.refuse, "
                "   pax_grp.class_grp, "
                "   salons.get_seat_no(pax.pax_id, pax.seats, pax_grp.status, pax_grp.point_dep, 'seats', rownum) seat_no, "
                "   pax_grp.hall, "
                "   pax.document, "
                "   pax.ticket_no "
                "FROM  pax_grp,pax, points ";
            if(!tag_no.empty())
                SQLText +=
                    " , bag_tags ";
            SQLText +=
                "WHERE "
                "   points.scd_out >= :FirstDate AND points.scd_out < :LastDate and "
                "   points.point_id = pax_grp.point_dep and points.pr_del>=0 and "
                "   pax_grp.grp_id=pax.grp_id ";
            if(!tag_no.empty())
                SQLText +=
                    " and pax_grp.grp_id = bag_tags.grp_id and "
                    " bag_tags.no like '%'||:tag_no ";
            if (!info.user.access.airps.empty()) {
                if (info.user.access.airps_permit)
                    SQLText += " AND points.airp IN "+GetSQLEnum(info.user.access.airps);
                else
                    SQLText += " AND points.airp NOT IN "+GetSQLEnum(info.user.access.airps);
            }
            if (!info.user.access.airlines.empty()) {
                if (info.user.access.airlines_permit)
                    SQLText += " AND points.airline IN "+GetSQLEnum(info.user.access.airlines);
                else
                    SQLText += " AND points.airline NOT IN "+GetSQLEnum(info.user.access.airlines);
            }
            if(!airline.empty())
                SQLText += " and points.airline = :airline ";
            if(!city.empty())
                SQLText += " and pax_grp.airp_arv = :city ";
            if(!flt_no.empty())
                SQLText += " and points.flt_no = :flt_no ";
            if(!surname.empty()) {
                if(FirstDate + 1 < LastDate && surname.size() < 4)
                    SQLText += " and pax.surname = :surname ";
                else
                    SQLText += " and pax.surname like :surname||'%' ";
            }
            if(!document.empty())
                SQLText += " and pax.document like '%'||:document||'%' ";
            if(!ticket_no.empty())
                SQLText += " and pax.ticket_no like '%'||:ticket_no||'%' ";
        } else {
            ProgTrace(TRACE5, "PaxSrcRun: arx base qry");
            SQLText =
                "SELECT "
                "   arx_points.part_key, "
                "   arx_pax_grp.point_dep point_id, "
                "   arx_points.airline, "
                "   arx_points.flt_no, "
                "   arx_points.suffix, "
                "   arx_points.airline_fmt, "
                "   arx_points.airp_fmt, "
                "   arx_points.suffix_fmt, "
                "   arx_points.airp, "
                "   arx_points.scd_out, "
                "   NVL(arx_points.act_out,NVL(arx_points.est_out,arx_points.scd_out)) AS real_out, "
                "   arx_pax.reg_no, "
                "   arx_pax_grp.airp_arv, "
                "   arx_pax.surname||' '||arx_pax.name full_name, "
                "   NVL(arch.get_bagAmount(arx_pax.part_key,arx_pax.grp_id,arx_pax.pax_id,rownum),0) bag_amount, "
                "   NVL(arch.get_bagWeight(arx_pax.part_key,arx_pax.grp_id,arx_pax.pax_id,rownum),0) bag_weight, "
                "   NVL(arch.get_rkWeight(arx_pax.part_key,arx_pax.grp_id,arx_pax.pax_id,rownum),0) rk_weight, "
                "   NVL(arch.get_excess(arx_pax.part_key,arx_pax.grp_id,arx_pax.pax_id),0) excess, "
                "   arx_pax_grp.grp_id, "
                "   arch.get_birks(arx_pax.part_key,arx_pax.grp_id,arx_pax.pax_id,:pr_lat) tags, "
                "   arx_pax.refuse, "
                "   arx_pax.pr_brd, "
                "   arx_pax_grp.class_grp, "
                "   LPAD(seat_no,3,'0')|| "
                "       DECODE(SIGN(1-seats),-1,'+'||TO_CHAR(seats-1),'') seat_no, "
                "   arx_pax_grp.hall, "
                "   arx_pax.document, "
                "   arx_pax.ticket_no "
                "FROM  arx_pax_grp,arx_pax, arx_points ";
            if(!tag_no.empty())
                SQLText +=
                    " , arx_bag_tags ";
            SQLText +=
                "WHERE "
                "   arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate and "
                "   arx_points.part_key = arx_pax_grp.part_key and "
                "   arx_points.point_id = arx_pax_grp.point_dep and arx_points.pr_del>=0 and "
                "   arx_pax_grp.part_key = arx_pax.part_key and "
                "   arx_pax_grp.grp_id=arx_pax.grp_id AND "
                "   arx_points.part_key >= :FirstDate and arx_points.part_key < :LastDate + :arx_trip_date_range and "
                "   pr_brd IS NOT NULL ";
            Qry.CreateVariable("arx_trip_date_range", otInteger, arx_trip_date_range);
            if(!tag_no.empty())
                SQLText +=
                    " and arx_pax_grp.part_key = arx_bag_tags.part_key and "
                    " arx_pax_grp.grp_id = arx_bag_tags.grp_id and "
                    " arx_bag_tags.no like '%'||:tag_no ";
            if (!info.user.access.airps.empty()) {
                if (info.user.access.airps_permit)
                    SQLText += " AND arx_points.airp IN "+GetSQLEnum(info.user.access.airps);
                else
                    SQLText += " AND arx_points.airp NOT IN "+GetSQLEnum(info.user.access.airps);
            }
            if (!info.user.access.airlines.empty()) {
                if (info.user.access.airlines_permit)
                    SQLText += " AND arx_points.airline IN "+GetSQLEnum(info.user.access.airlines);
                else
                    SQLText += " AND arx_points.airline NOT IN "+GetSQLEnum(info.user.access.airlines);
            }
            if(!airline.empty())
                SQLText += " and arx_points.airline = :airline ";
            if(!city.empty())
                SQLText += " and arx_pax_grp.airp_arv = :city ";
            if(!flt_no.empty())
                SQLText += " and arx_points.flt_no = :flt_no ";
            if(!surname.empty()) {
                if(FirstDate + 1 < LastDate && surname.size() < 4)
                    SQLText += " and arx_pax.surname = :surname ";
                else
                    SQLText += " and arx_pax.surname like :surname||'%' ";
            }
            if(!document.empty())
                SQLText += " and arx_pax.document = :document ";
            if(!ticket_no.empty())
                SQLText += " and arx_pax.ticket_no = :ticket_no ";
        }
        ProgTrace(TRACE5, "Qry.SQLText [%d] : %s", i, SQLText.c_str());
        Qry.SQLText = SQLText;
        try {
            tm.Init();
            Qry.Execute();
            ProgTrace(TRACE5, "EXEC QRY%d: %s", i, tm.PrintWithMessage().c_str());
        } catch (EOracleError &E) {
            if(E.Code == 376)
                throw AstraLocale::UserException("MSG.ONE_OF_DB_FILES_UNAVAILABLE.CALL_ADMIN");
            else
                throw;
        }
        if(!Qry.Eof) {
            if(!paxListNode)
                paxListNode = NewTextChild(resNode, "paxList");
            if(!rowsNode)
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
            int col_grp_id = Qry.FieldIndex("grp_id");
            int col_airp_arv = Qry.FieldIndex("airp_arv");
            int col_tags = Qry.FieldIndex("tags");
            int col_pr_brd = Qry.FieldIndex("pr_brd");
            int col_refuse = Qry.FieldIndex("refuse");
            int col_class_grp = Qry.FieldIndex("class_grp");
            int col_seat_no = Qry.FieldIndex("seat_no");
            int col_document = Qry.FieldIndex("document");
            int col_ticket_no = Qry.FieldIndex("ticket_no");
            int col_hall = Qry.FieldIndex("hall");
            int col_part_key=Qry.FieldIndex("part_key");

            map<int, TTripItem> TripItems;

            tm.Init();
            for( ; !Qry.Eof; Qry.Next()) {
                xmlNodePtr paxNode = NewTextChild(rowsNode, "pax");

                int point_id = Qry.FieldAsInteger(col_point_id);

                if(!Qry.FieldIsNULL(col_part_key))
                    NewTextChild(paxNode, "part_key",
                            DateTimeToStr(Qry.FieldAsDateTime(col_part_key), ServerFormatDateTimeAsString));
                NewTextChild(paxNode, "point_id", point_id);
                NewTextChild(paxNode, "airline", Qry.FieldAsString(col_airline));
                NewTextChild(paxNode, "flt_no", Qry.FieldAsInteger(col_flt_no));
                NewTextChild(paxNode, "suffix", Qry.FieldAsString(col_suffix));
                if(TripItems.find(point_id) == TripItems.end()) {
                    TTripInfo trip_info(Qry);
                    TTripItem trip_item;
                    trip_item.trip = GetTripName(trip_info, ecCkin);
                    trip_item.scd_out =
                        DateTimeToStr(
                                UTCToClient( Qry.FieldAsDateTime(col_scd_out), info.desk.tz_region),
                                ServerFormatDateTimeAsString
                                );
                    TripItems[point_id] = trip_item;
                }
                NewTextChild(paxNode, "trip", TripItems[point_id].trip);
                NewTextChild( paxNode, "scd_out", TripItems[point_id].scd_out);
                NewTextChild(paxNode, "reg_no", Qry.FieldAsInteger(col_reg_no));
                NewTextChild(paxNode, "full_name", Qry.FieldAsString(col_full_name));
                NewTextChild(paxNode, "bag_amount", Qry.FieldAsInteger(col_bag_amount));
                NewTextChild(paxNode, "bag_weight", Qry.FieldAsInteger(col_bag_weight));
                NewTextChild(paxNode, "rk_weight", Qry.FieldAsInteger(col_rk_weight));
                NewTextChild(paxNode, "excess", Qry.FieldAsInteger(col_excess));
                NewTextChild(paxNode, "grp_id", Qry.FieldAsInteger(col_grp_id));
                NewTextChild(paxNode, "airp_arv", ElemIdToCodeNative(etAirp, Qry.FieldAsString(col_airp_arv)));
                NewTextChild(paxNode, "tags", Qry.FieldAsString(col_tags));
                string status;
                if(Qry.FieldIsNULL(col_refuse))
                    status = getLocaleText(Qry.FieldAsInteger(col_pr_brd) == 0 ? "Зарег." : "Посаж.");
                else
                    status = getLocaleText("MSG.CANCEL_REG.REFUSAL",
                            LParams() << LParam("refusal", ElemIdToCodeNative(etRefusalType, Qry.FieldAsString(col_refuse))));
                NewTextChild(paxNode, "status", status);
                NewTextChild(paxNode, "class", ElemIdToCodeNative(etClsGrp, Qry.FieldAsInteger(col_class_grp)));
                NewTextChild(paxNode, "seat_no", Qry.FieldAsString(col_seat_no));
                NewTextChild(paxNode, "document", Qry.FieldAsString(col_document));
                NewTextChild(paxNode, "ticket_no", Qry.FieldAsString(col_ticket_no));
                if(Qry.FieldIsNULL(col_hall))
                    NewTextChild(paxNode, "hall");
                else
                    NewTextChild(paxNode, "hall", ElemIdToNameLong(etHall, Qry.FieldAsInteger(col_hall)));

                count++;
                if(count >= MAX_STAT_ROWS) {
                    AstraLocale::showErrorMessage("MSG.TOO_MANY_FLIGHTS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_SEARCH",
                            LParams() << LParam("num", MAX_STAT_ROWS));
                    break;
                }
            }
            ProgTrace(TRACE5, "XML%d: %s", i, tm.PrintWithMessage().c_str());
            ProgTrace(TRACE5, "count: %d", count);
        }
    }
    if(count == 0)
        throw AstraLocale::UserException("MSG.PASSENGERS.NOT_FOUND");

    xmlNodePtr headerNode = NewTextChild(paxListNode, "header"); // для совместимости со старым терминалом
    NewTextChild(headerNode, "col", "Рейс");

    STAT::set_variables(resNode);
    get_report_form("ArxPaxList", reqNode, resNode);
    ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
}

void StatInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{

}
