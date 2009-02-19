#include <fstream>
#include "stat.h"
#include "oralib.h"
#include "cache.h"
#include "xml_unit.h"
#include "exceptions.h"
#include "stl_utils.h"
#include "docs.h"
#include "base_tables.h"
#include "tripinfo.h"
#include "misc.h"
#include "timer.h"
#include "astra_utils.h"

#define NICKNAME "DENIS"
#include "serverlib/test.h"

#define MAX_STAT_ROWS 2000

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;

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

void GetPaxSrcDestSQL(TQuery &Qry);
void GetPaxSrcAwkSQL(TQuery &Qry);
void GetPaxListSQL(TQuery &Qry);
void GetFltLogSQL(TQuery &Qry);
void GetSystemLogAgentSQL(TQuery &Qry);
void GetSystemLogStationSQL(TQuery &Qry);
void GetSystemLogModuleSQL(TQuery &Qry);

enum TScreenState {None,Stat,Pax,Log,DepStat,BagTagStat,PaxList,FltLog,SystemLog,PaxSrc,TlgArch};
typedef void (*TGetSQL)( TQuery &Qry );

const int depends_len = 3;
struct TCBox {
    TGetSQL GetSQL;
    string cbox;
    string depends[depends_len];
};

const int cboxes_len = 5;
struct TCategory {
    TScreenState scr;
    TCBox cboxes[cboxes_len];
};

TCategory Category[] = {
    {},
    {},
    {},
    {},
    {
        DepStat,
        {
            {
                NULL,
                "Flt",
                {"Dest", "Awk", "Class"}
            },
            {
                NULL,
                "Awk",
                {"Flt",    "Dest", "Class"}
            },
            {
                NULL,
                "Dest",
                {"Awk",    "Flt",  "Class"}
            },
            {
                NULL,
                "Class",
                {"Awk",    "Flt",  "Dest"}
            },
            {
                NULL,
                "Rem",
            },
        }
    },
    {
        BagTagStat,
        {
            {
                NULL,
                "Awk",
            }
        }
    },
    {
        PaxList,
        {
            {
                GetPaxListSQL,
                "Flt",
            }
        }
    },
    {
        FltLog,
        {
            {
                GetFltLogSQL,
                "Flt",
            }
        }
    },
    {
        SystemLog,
        {
            {
                GetSystemLogAgentSQL,
                "Agent",
            },
            {
                GetSystemLogStationSQL,
                "Station",
            },
            {
                GetSystemLogModuleSQL,
                "Module",
            }
        }
    },
    {
        PaxSrc,
        {
            {
                NULL,
                "Flt",
            },
            {
                GetPaxSrcAwkSQL,
                "Awk",
            },
            {
                GetPaxSrcDestSQL,
                "Dest",
            }
        }
    },
    {}
};

const int CategorySize = sizeof(Category)/sizeof(Category[0]);

void GetFltLogSQL(TQuery &Qry)
{
    TReqInfo &info = *(TReqInfo::Instance());
    string res;
    if (info.user.access.airlines.empty() && info.user.access.airlines_permit ||
            info.user.access.airps.empty() && info.user.access.airps_permit)
        res = "select * from dual where 0 = 1";
    else {
        res =
            "SELECT "
            "    points.point_id, "
            "    points.airp, "
            "    points.airline, "
            "    points.flt_no, "
            "    nvl(points.suffix, ' ') suffix, "
            "    points.scd_out, "
            "    trunc(NVL(points.act_out,NVL(points.est_out,points.scd_out))) AS real_out, "
            "    move_id, "
            "    point_num "
            "FROM "
            "    points "
            "WHERE "
            "    points.pr_del >= 0 and "
            "    points.scd_out >= :FirstDate AND points.scd_out < :LastDate ";
        if (!info.user.access.airps.empty()) {
            if (info.user.access.airps_permit)
                res += " AND points.airp IN "+GetSQLEnum(info.user.access.airps);
            else
                res += " AND points.airp NOT IN "+GetSQLEnum(info.user.access.airps);
        }
        if (!info.user.access.airlines.empty()) {
            if (info.user.access.airlines_permit)
                res += " AND points.airline IN "+GetSQLEnum(info.user.access.airlines);
            else
                res += " AND points.airline NOT IN "+GetSQLEnum(info.user.access.airlines);
        }
        res +=
            "union "
            "SELECT "
            "    arx_points.point_id, "
            "    arx_points.airp, "
            "    arx_points.airline, "
            "    arx_points.flt_no, "
            "    nvl(arx_points.suffix, ' ') suffix, "
            "    arx_points.scd_out, "
            "    trunc(NVL(arx_points.act_out,NVL(arx_points.est_out,arx_points.scd_out))) AS real_out, "
            "    move_id, "
            "    point_num "
            "FROM "
            "    arx_points "
            "WHERE "
            "    arx_points.pr_del >= 0 and "
            "    arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate and "
            "    arx_points.part_key >= :FirstDate ";
        if (!info.user.access.airps.empty()) {
            if (info.user.access.airps_permit)
                res += " AND arx_points.airp IN "+GetSQLEnum(info.user.access.airps);
            else
                res += " AND arx_points.airp NOT IN "+GetSQLEnum(info.user.access.airps);
        }
        if (!info.user.access.airlines.empty()) {
            if (info.user.access.airlines_permit)
                res += " AND arx_points.airline IN "+GetSQLEnum(info.user.access.airlines);
            else
                res += " AND arx_points.airline NOT IN "+GetSQLEnum(info.user.access.airlines);
        }
        res +=
            "ORDER BY "
            "   real_out DESC, "
            "   flt_no, "
            "   airline, "
            "   suffix, "
            "   move_id, "
            "   point_num ";
    }
    Qry.SQLText = res;
}

void GetPaxSrcDestSQL(TQuery &Qry)
{

}

void GetPaxSrcAwkSQL(TQuery &Qry)
{
    TReqInfo &info = *(TReqInfo::Instance());
    string res =
        "select "
        "   airline "
        "from "
        "   points "
        "where "
        "    points.pr_del >= 0 and "
        "    points.pr_reg <> 0 and "
        "    points.scd_out >= :FirstDate AND points.scd_out < :LastDate ";
    if (!info.user.access.airps.empty()) {
        if (info.user.access.airps_permit)
            res += " AND points.airp IN "+GetSQLEnum(info.user.access.airps);
        else
            res += " AND points.airp NOT IN "+GetSQLEnum(info.user.access.airps);
    }
    if (!info.user.access.airlines.empty()) {
        if (info.user.access.airlines_permit)
            res += " AND points.airline IN "+GetSQLEnum(info.user.access.airlines);
        else
            res += " AND points.airline NOT IN "+GetSQLEnum(info.user.access.airlines);
    }
    res +=
        "union "
        "select "
        "   airline "
        "from "
        "   arx_points "
        "where "
        "    arx_points.pr_del >= 0 and "
        "    arx_points.pr_reg <> 0 and "
        "    arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate and "
        "    arx_points.part_key >= :FirstDate ";
    if (!info.user.access.airps.empty()) {
        if (info.user.access.airps_permit)
            res += " AND arx_points.airp IN "+GetSQLEnum(info.user.access.airps);
        else
            res += " AND arx_points.airp NOT IN "+GetSQLEnum(info.user.access.airps);
    }
    if (!info.user.access.airlines.empty()) {
        if (info.user.access.airlines_permit)
            res += " AND arx_points.airline IN "+GetSQLEnum(info.user.access.airlines);
        else
            res += " AND arx_points.airline NOT IN "+GetSQLEnum(info.user.access.airlines);
    }
    res +=
        "order by "
        "   airline ";
    Qry.SQLText = res;
}

void GetPaxListSQL(TQuery &Qry)
{
    TReqInfo &info = *(TReqInfo::Instance());
    tst();
    string res =
        "SELECT "
        "    points.point_id, "
        "    points.airp, "
        "    points.airline, "
        "    points.flt_no, "
        "    nvl(points.suffix, ' ') suffix, "
        "    points.scd_out, "
        "    NVL(points.act_out,NVL(points.est_out,points.scd_out)) AS real_out, "
        "    move_id, "
        "    point_num "
        "FROM "
        "    points "
        "WHERE "
        "    points.pr_del >= 0 and "
        "    points.pr_reg <> 0 and "
        "    points.scd_out >= :FirstDate AND points.scd_out < :LastDate ";
    if (!info.user.access.airps.empty()) {
        if (info.user.access.airps_permit)
            res += " AND points.airp IN "+GetSQLEnum(info.user.access.airps);
        else
            res += " AND points.airp NOT IN "+GetSQLEnum(info.user.access.airps);
    }
    if (!info.user.access.airlines.empty()) {
        if (info.user.access.airlines_permit)
            res += " AND points.airline IN "+GetSQLEnum(info.user.access.airlines);
        else
            res += " AND points.airline NOT IN "+GetSQLEnum(info.user.access.airlines);
    }
    res +=
        "union "
        "SELECT "
        "    arx_points.point_id, "
        "    arx_points.airp, "
        "    arx_points.airline, "
        "    arx_points.flt_no, "
        "    nvl(arx_points.suffix, ' ') suffix, "
        "    arx_points.scd_out, "
        "    NVL(arx_points.act_out,NVL(arx_points.est_out,arx_points.scd_out)) AS real_out, "
        "    move_id, "
        "    point_num "
        "FROM "
        "    arx_points "
        "WHERE "
        "    arx_points.pr_del >= 0 and "
        "    arx_points.pr_reg <> 0 and "
        "    arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate and "
        "    arx_points.part_key >= :FirstDate ";
    if (!info.user.access.airps.empty()) {
        if (info.user.access.airps_permit)
            res += " AND arx_points.airp IN "+GetSQLEnum(info.user.access.airps);
        else
            res += " AND arx_points.airp NOT IN "+GetSQLEnum(info.user.access.airps);
    }
    if (!info.user.access.airlines.empty()) {
        if (info.user.access.airlines_permit)
            res += " AND arx_points.airline IN "+GetSQLEnum(info.user.access.airlines);
        else
            res += " AND arx_points.airline NOT IN "+GetSQLEnum(info.user.access.airlines);
    }
    res +=
        "ORDER BY "
        "   flt_no, "
        "   airline, "
        "   suffix, "
        "   move_id, "
        "   point_num ";
    Qry.SQLText = res;
}

void GetSystemLogAgentSQL(TQuery &Qry)
{
    Qry.SQLText =
        "select null agent, -1 view_order from dual "
        "union "
        "select '���⥬�' agent, 0 view_order from dual "
        "union "
        "select descr agent, 1 view_order from users2 where "
        "  (adm.check_user_access(user_id,:SYS_user_id)<>0 or user_id=:SYS_user_id) "
        "order by "
        "   view_order, agent";
    Qry.CreateVariable("SYS_user_id", otInteger, TReqInfo::Instance()->user.user_id);
}

void GetSystemLogStationSQL(TQuery &Qry)
{
    Qry.SQLText =
        "select null station, -1 view_order from dual "
        "union "
        "select '���⥬�' station, 0 view_order from dual "
        "union "
        "select code, 1 from desks where "
        "   adm.check_desk_view_access(code, :SYS_user_id) <> 0 "
        "order by "
        "   view_order, station";
    Qry.CreateVariable("SYS_user_id", otInteger, TReqInfo::Instance()->user.user_id);
}

void GetSystemLogModuleSQL(TQuery &Qry)
{
    Qry.SQLText =
        "select null module, -1 view_order from dual "
        "union "
        "select '���⥬�' module, 0 view_order from dual "
        "union "
        "select name, view_order from screen where view_order is not null order by view_order";
}


typedef struct {
    TDateTime part_key, real_out_local_date;
    string airline, suffix, name;
    int point_id, flt_no, move_id, point_num;
} TPointsRow;

bool lessPointsRow(const TPointsRow& item1,const TPointsRow& item2)
{
    bool result;
    if(item1.real_out_local_date == item2.real_out_local_date) {
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
        result = item1.real_out_local_date > item2.real_out_local_date;
    return result;
};

void StatInterface::FltCBoxDropDown(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    bool pr_show_del = false;
    xmlNodePtr prDelNode = GetNode("pr_del", reqNode);
    if(prDelNode)
        pr_show_del = NodeAsInteger(prDelNode) == 1;
    TScreenState scr = TScreenState(NodeAsInteger("scr", reqNode));
    TReqInfo &reqInfo = *(TReqInfo::Instance());
    TQuery Qry(&OraSession);
    Qry.CreateVariable("FirstDate", otDate, ClientToUTC(NodeAsDateTime("FirstDate", reqNode), reqInfo.desk.tz_region));
    Qry.CreateVariable("LastDate", otDate, ClientToUTC(NodeAsDateTime("LastDate", reqNode), reqInfo.desk.tz_region));
    TTripInfo tripInfo;
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
                    "SELECT \n"
                    "    null part_key, \n"
                    "    points.point_id, \n"
                    "    points.airp, \n"
                    "    points.airline, \n"
                    "    points.flt_no, \n"
                    "    nvl(points.suffix, ' ') suffix, \n"
                    "    points.scd_out, \n"
                    "    trunc(NVL(points.act_out,NVL(points.est_out,points.scd_out))) AS real_out, \n"
                    "    points.pr_del, \n"
                    "    move_id, \n"
                    "    point_num \n"
                    "FROM \n"
                    "    points \n"
                    "WHERE \n"
                    "    points.scd_out >= :FirstDate AND points.scd_out < :LastDate \n";
                if(scr == PaxList)
                    SQLText += " and points.pr_del = 0 \n";
                if(scr == FltLog and !pr_show_del)
                    SQLText += " and points.pr_del >= 0 \n";
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
                    "SELECT \n"
                    "    arx_points.part_key, \n"
                    "    arx_points.point_id, \n"
                    "    arx_points.airp, \n"
                    "    arx_points.airline, \n"
                    "    arx_points.flt_no, \n"
                    "    nvl(arx_points.suffix, ' ') suffix, \n"
                    "    arx_points.scd_out, \n"
                    "    trunc(NVL(arx_points.act_out,NVL(arx_points.est_out,arx_points.scd_out))) AS real_out, \n"
                    "    arx_points.pr_del, \n"
                    "    move_id, \n"
                    "    point_num \n"
                    "FROM \n"
                    "    arx_points \n"
                    "WHERE \n"
                    "    arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate and \n"
                    "    arx_points.part_key >= :FirstDate \n";
                if(scr == PaxList)
                    SQLText += " and arx_points.pr_del = 0 \n";
                if(scr == FltLog and !pr_show_del)
                    SQLText += " and arx_points.pr_del >= 0 \n";
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
            ProgTrace(TRACE5, "SQLText: %s", SQLText.c_str());
            Qry.SQLText = SQLText;
            try {
                Qry.Execute();
            } catch (EOracleError E) {
                if(E.Code == 376)
                    throw UserException("� �������� ��������� ��� ���� �� 䠩��� �� �⪫�祭. ������� � ������������");
                else
                    throw;
            }
            if(!Qry.Eof) {
                int col_airline=Qry.FieldIndex("airline");
                int col_flt_no=Qry.FieldIndex("flt_no");
                int col_suffix=Qry.FieldIndex("suffix");
                int col_airp=Qry.FieldIndex("airp");
                int col_scd_out=Qry.FieldIndex("scd_out");
                int col_real_out=Qry.FieldIndex("real_out");
                int col_move_id=Qry.FieldIndex("move_id");
                int col_point_num=Qry.FieldIndex("point_num");
                int col_point_id=Qry.FieldIndex("point_id");
                int col_pr_del=Qry.FieldIndex("pr_del");
                int col_part_key=Qry.FieldIndex("part_key");
                for( ; !Qry.Eof; Qry.Next()) {
                    tripInfo.airline = Qry.FieldAsString(col_airline);
                    tripInfo.flt_no = Qry.FieldAsInteger(col_flt_no);
                    tripInfo.suffix = Qry.FieldAsString(col_suffix);
                    tripInfo.airp = Qry.FieldAsString(col_airp);
                    tripInfo.scd_out = Qry.FieldAsDateTime(col_scd_out);
                    tripInfo.real_out = Qry.FieldAsDateTime(col_real_out);
                    tripInfo.real_out_local_date=ASTRA::NoExists;
                    tripInfo.pr_del=Qry.FieldAsInteger(col_pr_del);
                    try
                    {
                        trip_name = GetTripName(tripInfo,false,true);
                    }
                    catch(UserException &E)
                    {
                        showErrorMessage((string)E.what()+". ������� ३�� �� �⮡ࠦ�����");
                        continue;
                    };
                    TPointsRow pointsRow;
                    if(Qry.FieldIsNULL(col_part_key))
                        pointsRow.part_key = NoExists;
                    else
                        pointsRow.part_key = Qry.FieldAsDateTime(col_part_key);
                    pointsRow.point_id = Qry.FieldAsInteger(col_point_id);
                    pointsRow.real_out_local_date = tripInfo.real_out_local_date;
                    pointsRow.airline = tripInfo.airline;
                    pointsRow.suffix = tripInfo.suffix;
                    pointsRow.name = trip_name;
                    pointsRow.flt_no = tripInfo.flt_no;
                    pointsRow.move_id = Qry.FieldAsInteger(col_move_id);
                    pointsRow.point_num = Qry.FieldAsInteger(col_point_num);
                    points.push_back(pointsRow);

                    count++;
                    if(count >= MAX_STAT_ROWS) {
                        showErrorMessage(
                                "��࠭� ᫨誮� ����� ३ᮢ. �������� " +
                                IntToString(MAX_STAT_ROWS) +
                                " �ந������� ३ᮢ."
                                " ��筨� ��ਮ� ���᪠."
                                );
                        break;
                    }
                }
            }
        }
    }
    ProgTrace(TRACE5, "FltCBoxDropDown QRY: %s", Qry.SQLText.SQLText());
    ProgTrace(TRACE5, "FltCBoxDropDown EXEC QRY: %s", tm.PrintWithMessage().c_str());
    if(count == 0)
        throw UserException("�� ������� �� ������ ३�.");
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

    TScreenState scr = TScreenState(NodeAsInteger("scr", reqNode));
    TCategory *Ctg = &Category[scr];
    int i = 0;
    for(; i < cboxes_len; i++)
        if(Ctg->cboxes[i].cbox + "CBox" == cbox) break;
    if(i == cboxes_len)throw Exception((string)"CommonCBoxDropDown: data not found for " + cbox);
    TCBox *cbox_data = &Ctg->cboxes[i];
    TQuery Qry(&OraSession);
    if(!cbox_data->GetSQL) throw Exception("GetSQL is NULL");
    cbox_data->GetSQL(Qry);
    ProgTrace(TRACE5, "%s", Qry.SQLText.SQLText());
    TReqInfo *reqInfo = TReqInfo::Instance();
    if(
            cbox == "FltCBox" ||
            cbox == "AwkCBox"
            ) {
        Qry.CreateVariable("FirstDate", otDate, ClientToUTC(NodeAsDateTime("FirstDate", reqNode), reqInfo->desk.tz_region));
        Qry.CreateVariable("LastDate", otDate, ClientToUTC(NodeAsDateTime("LastDate", reqNode), reqInfo->desk.tz_region));
    }
    /*
    Qry.CreateVariable("FirstDate", otDate, ClientToUTC(NodeAsDateTime("FirstDate", reqNode), reqInfo->desk.tz_region));
    Qry.CreateVariable("LastDate", otDate, ClientToUTC(NodeAsDateTime("LastDate", reqNode), reqInfo->desk.tz_region));
    if(scr == FltLog) {
        Qry.CreateVariable("evtFlt", otString, EncodeEventType(evtFlt));
        Qry.CreateVariable("evtGraph", otString, EncodeEventType(evtGraph));
        Qry.CreateVariable("evtTlg", otString, EncodeEventType(evtTlg));
        Qry.CreateVariable("evtPax", otString, EncodeEventType(evtPax));
        Qry.CreateVariable("evtPay", otString, EncodeEventType(evtPay));
    }
    */
    xmlNodePtr dependNode = GetNode("depends", reqNode);
    if(dependNode) {
        dependNode = dependNode->children;
        while(dependNode) {
            Qry.CreateVariable((char *)dependNode->name, otString, NodeAsString(dependNode));
            dependNode = dependNode->next;
        }
    }
    try {
        Qry.Execute();
    } catch (EOracleError E) {
        if(E.Code == 376)
            throw UserException("� �������� ��������� ��� ���� �� 䠩��� �� �⪫�祭. ������� � ������������");
        else
            throw;
    }
    xmlNodePtr cboxNode = NewTextChild(resNode, "cbox");
    if(cbox_data->cbox == "Flt") {
        //�᫨ �� �������� � ���⠬ �������稩 ��� - ���⮩ ᯨ᮪ ३ᮢ
        if (reqInfo->user.user_type==utAirport && reqInfo->user.access.airps.empty() ||
                reqInfo->user.user_type==utAirline && reqInfo->user.access.airlines.empty() ) return;
        while(!Qry.Eof) {
            TTripInfo info(Qry);
            string trip_name;
            try
            {
                trip_name = GetTripName(info,false,true);
            }
            catch(UserException &E)
            {
                showErrorMessage((string)E.what()+". ������� ३�� �� �⮡ࠦ�����");
                Qry.Next();
                continue;
            };
            xmlNodePtr fNode = NewTextChild(cboxNode, "f");
            NewTextChild(fNode, "key", Qry.FieldAsInteger("point_id"));
            NewTextChild( fNode, "value", trip_name);

            Qry.Next();
        }
    } else
        while(!Qry.Eof) {
            xmlNodePtr fNode = NewTextChild(cboxNode, "f");
            NewTextChild(fNode, "key", 0);
            NewTextChild(fNode, "value", Qry.FieldAsString(0));
            Qry.Next();
        }
}

void StatInterface::FltLogRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
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
        get_report_form("ArxPaxLog", resNode);
    else
        get_report_form("FltLog", resNode);
    STAT::set_variables(resNode);
    xmlNodePtr variablesNode = GetNode("form_data/variables", resNode);
    NewTextChild(variablesNode, "report_title", "��ୠ� ����権 ३�");
    TReqInfo *reqInfo = TReqInfo::Instance();
    TQuery Qry(&OraSession);
    int count = 0;

    xmlNodePtr paxLogNode = NewTextChild(resNode, "PaxLog");
    xmlNodePtr headerNode = NewTextChild(paxLogNode, "header");
    xmlNodePtr colNode;


    colNode = NewTextChild(headerNode, "col", "�����");
    SetProp(colNode, "width", 73);
    SetProp(colNode, "align", taLeftJustify);

    colNode = NewTextChild(headerNode, "col", "�⮩��");
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", taLeftJustify);

    colNode = NewTextChild(headerNode, "col", "�६�");
    SetProp(colNode, "width", 90);
    SetProp(colNode, "align", taLeftJustify);

    colNode = NewTextChild(headerNode, "col", "����");
    SetProp(colNode, "width", 90);
    SetProp(colNode, "align", taLeftJustify);

    colNode = NewTextChild(headerNode, "col", "��� �");
    SetProp(colNode, "width", 45);
    SetProp(colNode, "align", taRightJustify);

    colNode = NewTextChild(headerNode, "col", "������");
    SetProp(colNode, "width", 750);
    SetProp(colNode, "align", taLeftJustify);

    Qry.Clear();
    string qry1, qry2;
    int move_id = 0;
    if (part_key == NoExists) {
        {
            TQuery Qry(&OraSession);
            Qry.SQLText = "select move_id from points where point_id = :point_id";
            Qry.CreateVariable("point_id", otInteger, point_id);
            Qry.Execute();
            if(Qry.Eof) throw UserException("���� ��६�饭 � ��娢 ��� 㤠���. �롥�� ������ �� ᯨ᪠");
            move_id = Qry.FieldAsInteger("move_id");
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
            Qry.SQLText = "select move_id from arx_points where part_key = :part_key and point_id = :point_id";
            Qry.CreateVariable("part_key", otDate, part_key);
            Qry.CreateVariable("point_id", otInteger, point_id);
            Qry.Execute();
            if(Qry.Eof) throw UserException("���� �� ������");
            move_id = Qry.FieldAsInteger("move_id");
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
        } catch (EOracleError E) {
            if(E.Code == 376)
                throw UserException("� �������� ��������� ��� ���� �� 䠩��� �� �⪫�祭. ������� � ������������");
            else
                throw;
        }

        if(Qry.Eof && part_key == NoExists) {
            TQuery Qry(&OraSession);
            Qry.SQLText = "select point_id from points where point_id = :point_id";
            Qry.CreateVariable("point_id", otInteger, point_id);
            Qry.Execute();
            if(Qry.Eof)
                throw UserException("���� ��६�饭 � ��娢 ��� 㤠���. �롥�� ������ �� ᯨ᪠");
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
                        screen_map[screen] = Qry.FieldAsString(0);
                    }
                    screen = screen_map[screen];
                }
                NewTextChild(rowNode, "screen", screen, "");

                count++;
                if(count > MAX_STAT_ROWS) {
                    showErrorMessage(
                            "��࠭� ᫨誮� ����� ��ப. �������� " +
                            IntToString(MAX_STAT_ROWS) +
                            " �ந������� ��ப."
                            " ��筨� ���ਨ ���᪠."
                            );
                    break;
                }
            }
        }
        if(count > MAX_STAT_ROWS) break;
    }
    ProgTrace(TRACE5, "count: %d", count);
    if(!count)
        throw UserException("�� ������� �� ����� ����樨.");
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
        get_report_form("ArxPaxLog", resNode);
    else
        get_report_form("FltLog", resNode);
    STAT::set_variables(resNode);
    xmlNodePtr variablesNode = GetNode("form_data/variables", resNode);
    NewTextChild(variablesNode, "report_title", "����樨 �� ���ᠦ���");
    TReqInfo *reqInfo = TReqInfo::Instance();
    TQuery Qry(&OraSession);
    int count = 0;

    xmlNodePtr paxLogNode = NewTextChild(resNode, "PaxLog");
    xmlNodePtr headerNode = NewTextChild(paxLogNode, "header");
    xmlNodePtr colNode;


    colNode = NewTextChild(headerNode, "col", "�����");
    SetProp(colNode, "width", 73);
    SetProp(colNode, "align", taLeftJustify);

    colNode = NewTextChild(headerNode, "col", "�⮩��");
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", taLeftJustify);

    colNode = NewTextChild(headerNode, "col", "�६�");
    SetProp(colNode, "width", 90);
    SetProp(colNode, "align", taLeftJustify);

    colNode = NewTextChild(headerNode, "col", "����");
    SetProp(colNode, "width", 90);
    SetProp(colNode, "align", taLeftJustify);

    colNode = NewTextChild(headerNode, "col", "��� �");
    SetProp(colNode, "width", 45);
    SetProp(colNode, "align", taRightJustify);

    colNode = NewTextChild(headerNode, "col", "������");
    SetProp(colNode, "width", 750);
    SetProp(colNode, "align", taLeftJustify);

    Qry.Clear();
    if (part_key == NoExists) {
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
    } catch (EOracleError E) {
        if(E.Code == 376)
            throw UserException("� �������� ��������� ��� ���� �� 䠩��� �� �⪫�祭. ������� � ������������");
        else
            throw;
    }

    if(Qry.Eof && part_key == NoExists) {
        TQuery Qry(&OraSession);
        Qry.SQLText = "select point_id from points where point_id = :point_id and pr_del >= 0";
        Qry.CreateVariable("point_id", otInteger, point_id);
        Qry.Execute();
        if(Qry.Eof)
            throw UserException("���� ��६�饭 � ��娢 ��� 㤠���. �롥�� ������ �� ᯨ᪠");
    }

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
                showErrorMessage(
                        "��࠭� ᫨誮� ����� ��ப. �������� " +
                        IntToString(MAX_STAT_ROWS) +
                        " �ந������� ��ப."
                        " ��筨� ���ਨ ���᪠."
                        );
                break;
            }
        }
    }
    ProgTrace(TRACE5, "count: %d", count);
    if(!count)
        throw UserException("�� ������� �� ����� ����樨.");
}

typedef struct {
    string trip, scd_out;
} TTripItem;

void StatInterface::SystemLogRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr client_with_trip_col_in_SysLogNode = GetNode("client_with_trip_col_in_SysLog", reqNode);
    if(client_with_trip_col_in_SysLogNode == NULL)
        get_report_form("ArxPaxLog", resNode);
    else
        get_report_form("SystemLog", resNode);
    STAT::set_variables(resNode);
    xmlNodePtr variablesNode = GetNode("form_data/variables", resNode);
    NewTextChild(variablesNode, "report_title", "����樨 � ��⥬�");
    TReqInfo *reqInfo = TReqInfo::Instance();
    TQuery Qry(&OraSession);
    Qry.SQLText = "select exe from screen where name = :module";
    Qry.CreateVariable("module", otString, NodeAsString("module", reqNode));
    Qry.Execute();
    string module;
    if(!Qry.Eof) module = Qry.FieldAsString("exe");
    int count = 0;

    xmlNodePtr paxLogNode = NewTextChild(resNode, "PaxLog");
    xmlNodePtr headerNode = NewTextChild(paxLogNode, "header");
    xmlNodePtr colNode;


    colNode = NewTextChild(headerNode, "col", "�����");
    SetProp(colNode, "width", 73);
    SetProp(colNode, "align", taLeftJustify);

    colNode = NewTextChild(headerNode, "col", "�⮩��");
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", taLeftJustify);

    colNode = NewTextChild(headerNode, "col", "�६�");
    SetProp(colNode, "width", 90);
    SetProp(colNode, "align", taLeftJustify);

    colNode = NewTextChild(headerNode, "col", "����");
    SetProp(colNode, "width", 90);
    SetProp(colNode, "align", taLeftJustify);

    colNode = NewTextChild(headerNode, "col", "��� �");
    SetProp(colNode, "width", 45);
    SetProp(colNode, "align", taRightJustify);

    colNode = NewTextChild(headerNode, "col", "������");
    SetProp(colNode, "width", 750);
    SetProp(colNode, "align", taLeftJustify);

    map<int, string> TripItems;
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
                "  ev_user, station, ev_order "
                "FROM "
                "   events "
                "WHERE "
                "  events.time >= :FirstDate and "
                "  events.time < :LastDate and "
                "  (:agent is null or nvl(ev_user, '���⥬�') = :agent) and "
                "  (:module is null or nvl(screen, '���⥬�') = :module) and "
                "  (:station is null or nvl(station, '���⥬�') = :station) and "
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
                "  ev_user, station, ev_order "
                "FROM "
                "   arx_events "
                "WHERE "
                "  arx_events.part_key >= :FirstDate - 5 and " // time � part_key �� ᮢ������ ���
                "  arx_events.part_key < :LastDate + 5 and "   // ࠧ��� ⨯�� ᮡ�⨩
                "  arx_events.time >= :FirstDate and "         // ���⮬� ��� part_key ��६ ����訩 �������� time
                "  arx_events.time < :LastDate and "
                "  (:agent is null or nvl(ev_user, '���⥬�') = :agent) and "
                "  (:module is null or nvl(screen, '���⥬�') = :module) and "
                "  (:station is null or nvl(station, '���⥬�') = :station) and "
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
        Qry.CreateVariable("agent", otString, NodeAsString("agent", reqNode));
        Qry.CreateVariable("station", otString, NodeAsString("station", reqNode));
        Qry.CreateVariable("module", otString, module);

        TPerfTimer tm;
        tm.Init();
        try {
            Qry.Execute();
        } catch (EOracleError E) {
            if(E.Code == 376)
                throw UserException("� �������� ��������� ��� ���� �� 䠩��� �� �⪫�祭. ������� � ������������");
            else
                throw;
        }
        ProgTrace(TRACE5, "SystemLogRun%d EXEC QRY: %s", j, tm.PrintWithMessage().c_str());

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

            xmlNodePtr rowsNode = NewTextChild(paxLogNode, "rows");
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
                            "   NVL(act_out,NVL(est_out,scd_out)) AS real_out, "
                            "   pr_del "
                            "from ";
                        SQLText += (j == 0 ? "points" : "arx_points");
                        SQLText +=
                            " where "
                            "   point_id = :point_id ";
                        tripQry.SQLText = SQLText;
                        tripQry.CreateVariable("point_id", otInteger,  point_id);
                        tripQry.Execute();
                        TTripInfo trip_info(tripQry);
                        trip_info.pr_del = tripQry.FieldAsInteger("pr_del");
                        TripItems[point_id] = GetTripName(trip_info);
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
                    showErrorMessage(
                            "��࠭� ᫨誮� ����� ��ப. �������� " +
                            IntToString(MAX_STAT_ROWS) +
                            " �ந������� ��ப."
                            " ��筨� ���ਨ ���᪠."
                            );
                    break;
                }
            }
        }
        ProgTrace(TRACE5, "FORM XML2: %s", tm.PrintWithMessage().c_str());
        ProgTrace(TRACE5, "count %d: %d", j, count);
    }
    if(!count)
        throw UserException("�� ������� �� ����� ����樨.");
    ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
}

struct THallItem {
    int id;
    string name;
};

class THalls: public vector<THallItem> {
    public:
        void Init();
};

void THalls::Init()
{
    TQuery Qry(&OraSession);
    Qry.SQLText = "SELECT id,name FROM astra.halls2,astra.options WHERE halls2.airp=options.cod ORDER BY id";
    Qry.Execute();
    while(!Qry.Eof) {
        THallItem hi;
        hi.id = Qry.FieldAsInteger("id");
        hi.name = Qry.FieldAsString("name");
        this->push_back(hi);
        Qry.Next();
    }
}

void StatInterface::PaxListRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo &info = *(TReqInfo::Instance());
    if (info.user.access.airlines.empty() && info.user.access.airlines_permit ||
            info.user.access.airps.empty() && info.user.access.airps_permit)
        throw UserException("�� ������� �� ������ ���ᠦ��");
    xmlNodePtr paramNode = reqNode->children;

    int point_id = NodeAsIntegerFast("point_id", paramNode);
    TDateTime part_key;
    xmlNodePtr partKeyNode = GetNodeFast("part_key", paramNode);
    if(partKeyNode == NULL)
        part_key = NoExists;
    else
        part_key = NodeAsDateTime(partKeyNode);
    get_report_form("ArxPaxList", resNode);
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
                "   points.airp, "
                "   points.scd_out, "
                "   NVL(points.act_out,NVL(points.est_out,points.scd_out)) AS real_out, "
                "   pax.reg_no, "
                "   pax_grp.airp_arv, "
                "   pax.surname||' '||pax.name full_name, "
                "   NVL(ckin.get_bagAmount(pax.grp_id,pax.pax_id,rownum),0) bag_amount, "
                "   NVL(ckin.get_bagWeight(pax.grp_id,pax.pax_id,rownum),0) bag_weight, "
                "   NVL(ckin.get_rkWeight(pax.grp_id,pax.pax_id,rownum),0) rk_weight, "
                "   NVL(ckin.get_excess(pax.grp_id,pax.pax_id),0) excess, "
                "   pax_grp.grp_id, "
                "   ckin.get_birks(pax.grp_id,pax.pax_id,0) tags, "
                "   DECODE(pax.refuse,NULL,DECODE(pax.pr_brd,0,'��ॣ.','��ᠦ��'),'���ॣ.('||pax.refuse||')') AS status, "
                "   cls_grp.code class, "
                "   salons.get_seat_no(pax.pax_id, pax.seats, pax_grp.status, pax_grp.point_dep, 'seats', rownum) seat_no, "
                "   halls2.name hall, "
                "   pax.document, "
                "   pax.ticket_no "
                "FROM  pax_grp,pax, points, cls_grp, halls2 "
                "WHERE "
                "   points.point_id = :point_id and points.pr_del>=0 and "
                "   points.point_id = pax_grp.point_dep and "
                "   pax_grp.grp_id=pax.grp_id AND "
                "   pax_grp.class_grp = cls_grp.id and "
                "   pax_grp.hall = halls2.id(+) ";
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
                "   NVL(arx_points.act_out,NVL(arx_points.est_out,arx_points.scd_out)) AS real_out, "
                "   arx_pax.reg_no, "
                "   arx_pax_grp.airp_arv, "
                "   arx_pax.surname||' '||arx_pax.name full_name, "
                "   NVL(arch.get_bagAmount(arx_pax.part_key,arx_pax.grp_id,arx_pax.pax_id,rownum),0) bag_amount, "
                "   NVL(arch.get_bagWeight(arx_pax.part_key,arx_pax.grp_id,arx_pax.pax_id,rownum),0) bag_weight, "
                "   NVL(arch.get_rkWeight(arx_pax.part_key,arx_pax.grp_id,arx_pax.pax_id,rownum),0) rk_weight, "
                "   NVL(arch.get_excess(arx_pax.part_key,arx_pax.grp_id,arx_pax.pax_id),0) excess, "
                "   arx_pax_grp.grp_id, "
                "   arch.get_birks(arx_pax.part_key,arx_pax.grp_id,arx_pax.pax_id,0) tags, "
                "   DECODE(arx_pax.refuse,NULL,DECODE(arx_pax.pr_brd,0,'��ॣ.','��ᠦ��'), "
                "       '���ॣ.('||arx_pax.refuse||')') AS status, "
                "   cls_grp.code class, "
                "   LPAD(seat_no,3,'0')|| "
                "       DECODE(SIGN(1-seats),-1,'+'||TO_CHAR(seats-1),'') seat_no, "
                "   halls2.name hall, "
                "   arx_pax.document, "
                "   arx_pax.ticket_no "
                "FROM  arx_pax_grp,arx_pax, arx_points, cls_grp, halls2 "
                "WHERE "
                "   arx_points.point_id = :point_id and arx_points.pr_del>=0 and "
                "   arx_points.part_key = arx_pax_grp.part_key and "
                "   arx_points.point_id = arx_pax_grp.point_dep and "
                "   arx_pax_grp.part_key=arx_pax.part_key AND "
                "   arx_pax_grp.grp_id=arx_pax.grp_id AND "
                "   arx_pax_grp.class_grp = cls_grp.id and "
                "   arx_points.part_key = :part_key and "
                "   pr_brd IS NOT NULL  and "
                "   arx_pax_grp.hall = halls2.id(+) ";
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
            int col_status = Qry.FieldIndex("status");
            int col_class = Qry.FieldIndex("class");
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
                    trip = GetTripName(trip_info);
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
                NewTextChild(paxNode, "airp_arv", Qry.FieldAsString(col_airp_arv));
                NewTextChild(paxNode, "tags", Qry.FieldAsString(col_tags));
                NewTextChild(paxNode, "status", Qry.FieldAsString(col_status));
                NewTextChild(paxNode, "class", Qry.FieldAsString(col_class));
                NewTextChild(paxNode, "seat_no", Qry.FieldAsString(col_seat_no));
                NewTextChild(paxNode, "document", Qry.FieldAsString(col_document));
                NewTextChild(paxNode, "ticket_no", Qry.FieldAsString(col_ticket_no));
                NewTextChild(paxNode, "hall", Qry.FieldAsString(col_hall));

                Qry.Next();
            }
            ProgTrace(TRACE5, "XML: %s", tm.PrintWithMessage().c_str());
        }

        //��ᮯ஢������� �����
        Qry.Clear();
        if(part_key == NoExists)  {
            Qry.SQLText=
                "SELECT "
                "   null part_key, "
                "   pax_grp.point_dep point_id, "
                "   points.airline, "
                "   points.flt_no, "
                "   points.suffix, "
                "   points.airp, "
                "   points.scd_out, "
                "   NVL(points.act_out,NVL(points.est_out,points.scd_out)) AS real_out, "
                "   pax_grp.airp_arv, "
                "   report.get_last_trfer(pax_grp.grp_id) AS last_trfer, "
                "   ckin.get_bagAmount(pax_grp.grp_id,NULL) AS bag_amount, "
                "   ckin.get_bagWeight(pax_grp.grp_id,NULL) AS bag_weight, "
                "   ckin.get_rkWeight(pax_grp.grp_id,NULL) AS rk_weight, "
                "   ckin.get_excess(pax_grp.grp_id,NULL) AS excess, "
                "   ckin.get_birks(pax_grp.grp_id,NULL) AS tags, "
                "   pax_grp.grp_id, "
                "   halls2.name AS hall_id, "
                "   pax_grp.point_arv,pax_grp.user_id "
                "FROM pax_grp, points, halls2 "
                "WHERE "
                "   pax_grp.point_dep=:point_id AND "
                "   pax_grp.class IS NULL and "
                "   pax_grp.point_dep = points.point_id and points.pr_del>=0 and "
                "   pax_grp.hall = halls2.id(+) ";
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
                "  arx_points.airp, "
                "  arx_points.scd_out, "
                "  NVL(arx_points.act_out,NVL(arx_points.est_out,arx_points.scd_out)) AS real_out, "
                "  arx_pax_grp.airp_arv, "
                "  report.get_last_trfer(arx_pax_grp.grp_id) AS last_trfer, "
                "  ckin.get_bagAmount(arx_pax_grp.grp_id,NULL) AS bag_amount, "
                "  ckin.get_bagWeight(arx_pax_grp.grp_id,NULL) AS bag_weight, "
                "  ckin.get_rkWeight(arx_pax_grp.grp_id,NULL) AS rk_weight, "
                "  ckin.get_excess(arx_pax_grp.grp_id,NULL) AS excess, "
                "  ckin.get_birks(arx_pax_grp.grp_id,NULL) AS tags, "
                "  arx_pax_grp.grp_id, "
                "  halls2.name AS hall_id, "
                "  arx_pax_grp.point_arv,arx_pax_grp.user_id "
                "FROM arx_pax_grp, arx_points, halls2 "
                "WHERE point_dep=:point_id AND class IS NULL and "
                "   arx_pax_grp.part_key = arx_points.part_key and "
                "   arx_pax_grp.point_dep = arx_points.point_id and arx_points.pr_del>=0 and "
                "   arx_pax_grp.part_key = :part_key and "
                "   arx_pax_grp.hall = halls2.id(+) ";
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

        Qry.CreateVariable("point_id",otInteger,point_id);
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
                trip = GetTripName(trip_info);
                scd_out =
                    DateTimeToStr(
                            UTCToClient( Qry.FieldAsDateTime("scd_out"), info.desk.tz_region),
                            ServerFormatDateTimeAsString
                            );
            }
            NewTextChild(paxNode, "trip", trip);
            NewTextChild( paxNode, "scd_out", scd_out);
            NewTextChild(paxNode, "reg_no", 0);
            NewTextChild(paxNode, "full_name", "����� ��� ᮯ஢�������");
            NewTextChild(paxNode, "bag_amount", Qry.FieldAsInteger("bag_amount"));
            NewTextChild(paxNode, "bag_weight", Qry.FieldAsInteger("bag_weight"));
            NewTextChild(paxNode, "rk_weight", Qry.FieldAsInteger("rk_weight"));
            NewTextChild(paxNode, "excess", Qry.FieldAsInteger("excess"));
            NewTextChild(paxNode, "grp_id", Qry.FieldAsInteger("grp_id"));
            NewTextChild(paxNode, "airp_arv", Qry.FieldAsString("airp_arv"));
            NewTextChild(paxNode, "tags", Qry.FieldAsString("tags"));
            NewTextChild(paxNode, "status");
            NewTextChild(paxNode, "class");
            NewTextChild(paxNode, "seat_no");
            NewTextChild(paxNode, "document");
            NewTextChild(paxNode, "ticket_no");
            NewTextChild(paxNode, "hall", Qry.FieldAsString("hall_id"));
        };

        if(paxListNode) {
            xmlNodePtr headerNode = NewTextChild(paxListNode, "header");
            xmlNodePtr colNode;

            colNode = NewTextChild(headerNode, "col", "����");
            SetProp(colNode, "width", 53);
            SetProp(colNode, "align", taLeftJustify);

            colNode = NewTextChild(headerNode, "col", "���");
            SetProp(colNode, "width", 61);
            SetProp(colNode, "align", taLeftJustify);

            colNode = NewTextChild(headerNode, "col", "�");
            SetProp(colNode, "width", 25);
            SetProp(colNode, "align", taRightJustify);

            colNode = NewTextChild(headerNode, "col", "�������");
            SetProp(colNode, "width", 173);
            SetProp(colNode, "align", taLeftJustify);

            colNode = NewTextChild(headerNode, "col", "�/�");
            SetProp(colNode, "width", 32);
            SetProp(colNode, "align", taLeftJustify);

            colNode = NewTextChild(headerNode, "col", "����");
            SetProp(colNode, "width", 40);
            SetProp(colNode, "align", taRightJustify);

            colNode = NewTextChild(headerNode, "col", "���");
            SetProp(colNode, "width", 40);
            SetProp(colNode, "align", taRightJustify);

            colNode = NewTextChild(headerNode, "col", "�/�");
            SetProp(colNode, "width", 40);
            SetProp(colNode, "align", taRightJustify);

            colNode = NewTextChild(headerNode, "col", "����");
            SetProp(colNode, "width", 40);
            SetProp(colNode, "align", taRightJustify);

            colNode = NewTextChild(headerNode, "col", "��ન");
            SetProp(colNode, "width", 163);
            SetProp(colNode, "align", taLeftJustify);

            colNode = NewTextChild(headerNode, "col", "�����");
            SetProp(colNode, "width", 93);
            SetProp(colNode, "align", taLeftJustify);

            colNode = NewTextChild(headerNode, "col", "��.");
            SetProp(colNode, "width", 25);
            SetProp(colNode, "align", taLeftJustify);

            colNode = NewTextChild(headerNode, "col", "� �");
            SetProp(colNode, "width", 40);
            SetProp(colNode, "align", taLeftJustify);

            colNode = NewTextChild(headerNode, "col", "���");
            SetProp(colNode, "width", 78);
            SetProp(colNode, "align", taLeftJustify);

            colNode = NewTextChild(headerNode, "col", "���㬥��");
            SetProp(colNode, "width", 114);
            SetProp(colNode, "align", taLeftJustify);

            colNode = NewTextChild(headerNode, "col", "� �����");
            SetProp(colNode, "width", 101);
            SetProp(colNode, "align", taLeftJustify);
        } else
            throw UserException("�� ������� �� ������ ���ᠦ��");
        tm.Init();
        STAT::set_variables(resNode);
        ProgTrace(TRACE5, "set_variables: %s", tm.PrintWithMessage().c_str());
        tm.Init();
        ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
        ProgTrace(TRACE5, "GetXMLDocText: %s", tm.PrintWithMessage().c_str());

        return;
    }
}

struct TTagQryParts {
    string
        select,
        order_by,
        astra_select,
        arx_select,
        astra_from,
        arx_from,
        where,
        astra_group_by,
        arx_group_by,
        get_birks;
    TTagQryParts(int state);
    bool TagDetailCheck(int i);
    bool checked[5];
    private:
    bool cscd, cflt, ctarget, ctarget_trfer, ccls;
    string fgroup_by, fuselect;
};

bool TTagQryParts::TagDetailCheck(int i)
{
    if(i > 4 || i == 1)
        return true;
    else if(i == 2)
        return checked[1];
    else if(i == 3)
        return checked[2] || checked[3];
    else
        return checked[i];
}

TTagQryParts::TTagQryParts(int state)
{
    cscd = state & 16;
    cflt = state & 8;
    ctarget = state & 4;
    ctarget_trfer = state & 2;
    ccls = state & 1;

    checked[0] = cscd;
    checked[1] = cflt;
    checked[2] = ctarget;
    checked[3] = ctarget_trfer;
    checked[4] = ccls;

    get_birks = "trip_company, ";
    order_by = "trip_company, ";
    fuselect = "trips.company   trip_company, ";
    fgroup_by = "trips.company, ";

    if(cflt) {
        get_birks = get_birks +
            "trip_flt_no, " +
            "nvl(trip_suffix, ' '), ";
        order_by = order_by +
            "trip_flt_no, " +
            "trip_suffix, ";
        fuselect = fuselect +
            "trips.flt_no    trip_flt_no, " +
            "trips.suffix    trip_suffix, ";
        fgroup_by = fgroup_by +
            "trips.flt_no, " +
            "trips.suffix, ";
    } else
        get_birks = get_birks +
            "NULL, " +
            "NULL, ";


    if(cflt && ctarget_trfer) {
        get_birks = get_birks +
            "nvl(transfer_company, ' '), " +
            "nvl(transfer_flt_no, -1), " +
            "nvl(transfer_suffix, ' '), ";
        order_by = order_by +
            "transfer_company, " +
            "transfer_flt_no, " +
            "transfer_suffix, ";
        fuselect = fuselect +
            "lt.airline      transfer_company, " +
            "lt.flt_no       transfer_flt_no, " +
            "lt.suffix       transfer_suffix, ";
        fgroup_by = fgroup_by +
            "lt.airline, " +
            "lt.flt_no, " +
            "lt.suffix, ";
    } else
        get_birks = get_birks +
            "NULL, " +
            "NULL, " +
            "NULL, ";

    if(ctarget) {
        get_birks = get_birks + "target, ";
        order_by = order_by + "target, ";
        fuselect = fuselect + "pax_grp.target  target, ";
        fgroup_by = fgroup_by + "pax_grp.target, ";
    } else
        get_birks = get_birks + "NULL, ";

    if(ctarget_trfer) {
        get_birks = get_birks + "nvl(transfer_target, ' '), ";
        order_by = order_by + "transfer_target, ";
        fuselect = fuselect + "lt.airp_arv     transfer_target, ";
        fgroup_by = fgroup_by + "lt.airp_arv, ";
    } else
        get_birks = get_birks + "NULL, ";

    if(ccls) {
        get_birks = get_birks + "cls, ";
        order_by = order_by + "cls, ";
        fuselect = fuselect + "pax_grp.class   cls, ";
        fgroup_by = fgroup_by + "pax_grp.class, ";
    } else
        get_birks = get_birks + "NULL, ";

    if(cscd) {
        select = "scd, " + order_by;
        astra_select =
            "to_char(trips.scd, 'dd.mm.yy')   scd, " +
            fuselect;
        arx_select =
            "to_char(trips.part_key, 'dd.mm.yy')   scd, " +
            fuselect;
        astra_group_by =
            fgroup_by;
        arx_group_by =
            fgroup_by;
    } else {
        select = order_by;
        astra_select = fuselect;
        arx_select = fuselect;
        astra_group_by = fgroup_by;
        arx_group_by = fgroup_by;
    }

    if(ctarget_trfer) {
        astra_from =
            "( "
            "  SELECT grp_id,airline,flt_no,suffix,airp_arv "
            "  FROM trfer_trips,transfer "
            "  WHERE transfer.point_id_trfer=trfer_trips.point_id AND pr_final<>0 "
            ") lt, ";
        arx_from =
            "( "
            "  ���� �� ���� ��� �㤥�!!! "
            ") lt, ";
        where = "pax_grp.grp_id = lt.grp_id(+) and ";
    }
}

const int BagTagColsSize = 8;

void StatInterface::BagTagStatRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TTagQryParts qry_parts(NodeAsInteger("BagTagCBState", reqNode));
    string qry = (string)
        "select "
        "  tscd, " +
        qry_parts.select +
        "  color, "
        "  arx.test.get_birks(tscd, " +
        qry_parts.get_birks +
        "    nvl(color, ' ') "
        "  ) tags "
        "from ( "
        "select "
        "  trips.scd tscd, " +
        qry_parts.astra_select +
        "  bag_tags.color  color "
        "from "
        "  astra.trips, "
        "  astra.pax_grp, " +
        qry_parts.astra_from +
        "  astra.bag_tags "
        "where "
        "  trips.scd >= :FirstDate and "
        "  trips.scd < :LastDate and "
        "  (:awk is null or trips.company = :awk) and "
        "  trips.trip_id = pax_grp.point_id and " +
        qry_parts.where +
        "  pax_grp.grp_id = bag_tags.grp_id "
        "group by "
        "  trips.scd, " +
        qry_parts.astra_group_by +
        "  bag_tags.color "
        "union  "
        "select "
        "  trips.part_key tscd, " +
        qry_parts.arx_select +
        "  bag_tags.color  color "
        "from "
        "  arx.trips, "
        "  arx.pax_grp, " +
        qry_parts.arx_from +
        "  arx.bag_tags "
        "where "
        "  trips.part_key >= :FirstDate and "
        "  trips.part_key < :LastDate and "
        "  (:awk is null or trips.company = :awk) and "
        "  trips.trip_id = pax_grp.point_id and "
        "  trips.part_key = pax_grp.part_key and " +
        qry_parts.where +
        "  pax_grp.part_key = bag_tags.part_key and "
        "  pax_grp.grp_id = bag_tags.grp_id "
        "group by "
        "  trips.part_key, " +
        qry_parts.arx_group_by +
        "  bag_tags.color "
        ") "
        "order by "
        "  tscd, " +
        qry_parts.order_by +
        "color ";

    TQuery Qry(&OraSession);
    Qry.SQLText = qry;
    TParams1 SQLParams;
    SQLParams.getParams(GetNode("sqlparams", reqNode));
    SQLParams.setSQL(&Qry);
    Qry.Execute();

    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    xmlNodePtr BagTagGrdNode = NewTextChild(dataNode, "BagTagGrd");
    while(!Qry.Eof) {
        xmlNodePtr rowNode = NewTextChild(BagTagGrdNode, "row");
        for(int i = 0; i < BagTagColsSize - 3; i++) {
            bool checked = qry_parts.TagDetailCheck(i);
            if(checked && i == 0)
                NewTextChild(rowNode, "col", Qry.FieldAsString("scd"));
            if(checked && i == 1)
                NewTextChild(rowNode, "col", Qry.FieldAsString("trip_company"));
            if(checked && i == 2) {
                string trip_company = Qry.FieldAsString("trip_company");
                string trip_flt_no = IntToString(Qry.FieldAsInteger("trip_flt_no"));
                string trip_suffix = Qry.FieldAsString("trip_suffix");
                string transfer_company;
                string transfer_flt_no;
                string transfer_suffix;

                if(qry_parts.checked[3]) {
                    transfer_company = Qry.FieldAsString("transfer_company");
                    transfer_flt_no = IntToString(Qry.FieldAsInteger("transfer_flt_no"));
                    transfer_suffix = Qry.FieldAsString("transfer_suffix");
                }
                string col2 = trip_company + trip_flt_no + trip_suffix;
                if(transfer_company.size())
                    col2 += "-" + transfer_company + transfer_flt_no + transfer_suffix;
                NewTextChild(rowNode, "col", col2);
            }
            if(checked && i == 3) {
                string target, transfer_target, col3;
                if(qry_parts.checked[2])
                    target = Qry.FieldAsString("target");
                if(qry_parts.checked[3])
                    transfer_target = Qry.FieldAsString("transfer_target");
                if(target.size()) {
                    col3 = target;
                    if(transfer_target.size())
                        col3 += "-" + transfer_target;
                } else
                    col3 = transfer_target;
                NewTextChild(rowNode, "col", col3);
            }
            if(checked && i == 4)
                NewTextChild(rowNode, "col", Qry.FieldAsString("cls"));
        }
        NewTextChild(rowNode, "col", Qry.FieldAsString("color"));
        string tags = Qry.FieldAsString("tags");
        string::size_type pos = tags.find(":");
        NewTextChild(rowNode, "col", tags.substr(pos + 1));
        NewTextChild(rowNode, "col", tags.substr(0, pos));
        Qry.Next();
    }
}

void StatInterface::DepStatRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TQuery Qry(&OraSession);
    Qry.SQLText =
        "SELECT "
        "  0 AS priority, "
        "  halls2.name AS hall, "
        "  NVL(SUM(trips),0) AS trips, "
        "  NVL(SUM(pax),0) AS pax, "
        "  NVL(SUM(weight),0) AS weight, "
        "  NVL(SUM(unchecked),0) AS unchecked, "
        "  NVL(SUM(excess),0) AS excess "
        "FROM "
        " (SELECT     "
        "    0,stat.hall, "
        "    COUNT(DISTINCT trips.trip_id) AS trips, "
        "    SUM(f+c+y) AS pax, "
        "    SUM(weight) AS weight, "
        "    SUM(unchecked) AS unchecked, "
        "    SUM(stat.excess) AS excess "
        "  FROM astra.trips,astra.stat "
        "  WHERE trips.trip_id=stat.trip_id AND "
        "        trips.scd>= :FirstDate AND trips.scd< :LastDate AND "
        "        (:flt IS NULL OR trips.trip= :flt) AND "
        "        (:dest IS NULL OR stat.target IN (SELECT cod FROM astra.place WHERE place.trip_id=trips.trip_id AND place.city=:dest)) and "
        "        (:awk IS NULL OR trips.company = :awk) and "
        "        exists ( "
        "          select * from astra.pax_grp where "
        "            trips.trip_id = pax_grp.point_id(+) and "
        "            (:class is null or pax_grp.class = :class) "
        "        ) "
        "  GROUP BY stat.hall "
        "  UNION "
        "  SELECT "
        "    1,stat.hall, "
        "    COUNT(DISTINCT trips.trip_id) AS trips, "
        "    SUM(f+c+y) AS pax, "
        "    SUM(weight) AS weight, "
        "    SUM(unchecked) AS unchecked, "
        "    SUM(stat.excess) AS excess "
        "  FROM arx.trips,arx.stat "
        "  WHERE trips.part_key=stat.part_key AND trips.trip_id=stat.trip_id AND "
        "        stat.part_key>= :FirstDate AND stat.part_key< :LastDate AND "
        "        (:flt IS NULL OR trips.trip= :flt) AND "
        "        (:dest IS NULL OR stat.target IN (SELECT cod FROM arx.place WHERE place.part_key=trips.part_key AND place.trip_id=trips.trip_id AND place.city=:dest)) and "
        "        (:awk IS NULL OR trips.company = :awk) and "
        "        exists ( "
        "          select * from arx.pax_grp where "
        "            trips.trip_id = pax_grp.point_id(+) and "
        "            (:class is null or pax_grp.class = :class) "
        "        ) "
        "  GROUP BY stat.hall) stat,astra.halls2 "
        "WHERE stat.hall(+)=halls2.id "
        "GROUP BY halls2.name "
        "UNION "
        "SELECT "
        "  1 AS priority, "
        "  '�ᥣ�' AS hall, "
        "  NVL(SUM(trips),0) AS trips, "
        "  NVL(SUM(pax),0) AS pax, "
        "  NVL(SUM(weight),0) AS weight, "
        "  NVL(SUM(unchecked),0) AS unchecked, "
        "  NVL(SUM(excess),0) AS excess "
        "FROM "
        " (SELECT         "
        "    COUNT(DISTINCT trips.trip_id) AS trips, "
        "    SUM(f+c+y) AS pax, "
        "    SUM(weight) AS weight, "
        "    SUM(unchecked) AS unchecked, "
        "    SUM(stat.excess) AS excess "
        "  FROM astra.trips,astra.stat "
        "  WHERE trips.trip_id=stat.trip_id AND "
        "        trips.scd>= :FirstDate AND trips.scd< :LastDate AND "
        "        (:flt IS NULL OR trips.trip= :flt) AND "
        "        (:dest IS NULL OR stat.target IN (SELECT cod FROM astra.place WHERE place.trip_id=trips.trip_id AND place.city=:dest)) and "
        "        (:awk IS NULL OR trips.company = :awk) and "
        "        exists ( "
        "          select * from astra.pax_grp where "
        "            trips.trip_id = pax_grp.point_id(+) and "
        "            (:class is null or pax_grp.class = :class) "
        "        ) "
        "union "
        "  SELECT "
        "    COUNT(DISTINCT trips.trip_id) AS trips, "
        "    SUM(f+c+y) AS pax, "
        "    SUM(weight) AS weight, "
        "    SUM(unchecked) AS unchecked, "
        "    SUM(stat.excess) AS excess "
        "  FROM arx.trips,arx.stat "
        "  WHERE trips.part_key=stat.part_key AND trips.trip_id=stat.trip_id AND "
        "        stat.part_key>= :FirstDate AND stat.part_key< :LastDate AND "
        "        (:flt IS NULL OR trips.trip= :flt) AND "
        "        (:dest IS NULL OR stat.target IN (SELECT cod FROM arx.place WHERE place.part_key=trips.part_key AND place.trip_id=trips.trip_id AND place.city=:dest)) and "
        "        (:awk IS NULL OR trips.company = :awk) and "
        "        exists ( "
        "          select * from arx.pax_grp where "
        "            trips.trip_id = pax_grp.point_id(+) and "
        "            (:class is null or pax_grp.class = :class) "
        "        ) "
        ") stat "
        "ORDER BY priority,hall ";

    TParams1 SQLParams;

    xmlNodePtr paramsNode = GetNode("sqlparams", reqNode);
    TDateTime FirstDate = NodeAsDateTime("FirstDate", paramsNode);
    TDateTime LastDate = NodeAsDateTime("LastDate", paramsNode);
    SQLParams.getParams(paramsNode);

    SQLParams.setSQL(&Qry);
    try {
        Qry.Execute();
    } catch (EOracleError E) {
        if(E.Code == 376)
            throw UserException(376);
        else
            throw;
    }

    xmlNodePtr dataNode = NewTextChild(resNode, "data");
    xmlNodePtr DepStatNode = NewTextChild(dataNode, "DepStat");
    while(!Qry.Eof) {
        int pax = Qry.FieldAsInteger("pax");
        xmlNodePtr rowNode = NewTextChild(DepStatNode, "row");
        NewTextChild(rowNode, "hall", Qry.FieldAsString("hall"));
        NewTextChild(rowNode, "trips", Qry.FieldAsInteger("trips"));
        NewTextChild(rowNode, "pax", pax);
        NewTextChild(rowNode, "weight", Qry.FieldAsInteger("weight"));
        NewTextChild(rowNode, "unchecked", Qry.FieldAsInteger("unchecked"));
        NewTextChild(rowNode, "excess", Qry.FieldAsInteger("excess"));
        char avg[50];
        sprintf(avg, "%.2f", pax/(LastDate - FirstDate));
        NewTextChild(rowNode, "avg", avg);
        Qry.Next();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void STAT::set_variables(xmlNodePtr resNode)
{

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
        tz = "(" + reqInfo->desk.city + ")";

    NewTextChild(variablesNode, "print_date",
            DateTimeToStr(issued, "dd.mm.yyyy hh:nn:ss ") + tz);
    NewTextChild(variablesNode, "print_oper", reqInfo->user.login);
    NewTextChild(variablesNode, "print_term", reqInfo->desk.code);
    NewTextChild(variablesNode, "print_term", reqInfo->desk.code);
    NewTextChild(variablesNode, "test_server", get_test_server());
}

struct TFullStatRow {
    int pax_amount;
    int adult;
    int child;
    int baby;
    int rk_weight;
    int bag_amount;
    int bag_weight;
    int excess;
    TFullStatRow():
        pax_amount(NoExists),
        adult(NoExists),
        child(NoExists),
        baby(NoExists),
        rk_weight(NoExists),
        bag_amount(NoExists),
        bag_weight(NoExists),
        excess(NoExists)
    {}
};
struct TFullStatKey {
    string col1, col2;
    int flt_no;
    TDateTime scd_out;
    int point_id;
    string places;
    TFullStatKey():
        flt_no(NoExists),
        scd_out(NoExists),
        point_id(NoExists)
    {}
};
struct TFullCmp {
    bool operator() (const TFullStatKey &lr, const TFullStatKey &rr) const
    {
        if(lr.col1 == rr.col1)
            if(lr.col2 == rr.col2)
                if(lr.flt_no == rr.flt_no)
                    if(lr.scd_out == rr.scd_out)
                        if(lr.point_id == rr.point_id)
                            return lr.places < rr.places;
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

void RunTrferFullStat(xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo &info = *(TReqInfo::Instance());
    if (info.user.access.airlines.empty() && info.user.access.airlines_permit ||
            info.user.access.airps.empty() && info.user.access.airps_permit)
        throw UserException("��� ������");
    get_report_form("TrferFullStat", resNode);

    string ak = Trim(NodeAsString("ak", reqNode));
    string ap = Trim(NodeAsString("ap", reqNode));

    TQuery Qry(&OraSession);
    string mainSQLText =
        "select \n"
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
        "  excess \n"
        "from \n"
        "  points, \n"
        "  trfer_stat \n";
    if(!ap.empty())
        mainSQLText += ", " + AIRP_PERIODS;
    else if(!ak.empty())
        mainSQLText += ", " + AIRLINE_PERIODS;
    mainSQLText +=
        "where \n"
        "  points.point_id = trfer_stat.point_id and points.pr_del>=0 and \n";
    if(ap.empty() and ak.empty())
        mainSQLText +=
            "  points.scd_out >= :FirstDate AND points.scd_out < :LastDate \n";
    else
        mainSQLText +=
            "  points.scd_out >= periods.period_first_date AND points.scd_out < periods.period_last_date  \n";
    if (!info.user.access.airps.empty()) {
        if (info.user.access.airps_permit)
            mainSQLText += " AND points.airp IN "+GetSQLEnum(info.user.access.airps);
        else
            mainSQLText += " AND points.airp NOT IN "+GetSQLEnum(info.user.access.airps);
    }
    if (!info.user.access.airlines.empty()) {
        if (info.user.access.airlines_permit)
            mainSQLText += " AND points.airline IN "+GetSQLEnum(info.user.access.airlines);
        else
            mainSQLText += " AND points.airline NOT IN "+GetSQLEnum(info.user.access.airlines);
    }
    if(ap.size()) {
        mainSQLText +=
            " and points.airp = :ap and \n"
            " points.airline not in " + AIRLINE_LIST;
        Qry.CreateVariable("ap", otString, ap);
    } else if(ak.size()) {
        mainSQLText +=
            " and points.airline = :ak and \n"
            " points.airp in " + AIRP_LIST;
        Qry.CreateVariable("ak", otString, ak);
    }

    string arxSQLText =
        "select \n"
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
        "  excess \n"
        "from \n"
        "  arx_points, \n"
        "  arx_trfer_stat \n";
    if(!ap.empty())
        arxSQLText += ", " + AIRP_PERIODS;
    else if(!ak.empty())
        arxSQLText += ", " + AIRLINE_PERIODS;
    arxSQLText +=
        "where \n";
    if(ap.empty() and ak.empty())
        arxSQLText +=
            "  arx_points.part_key >= :FirstDate AND arx_points.part_key < :LastDate + 5 AND "
            "  arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate AND ";
    else
        arxSQLText +=
        "  arx_points.part_key >= periods.period_first_date AND arx_points.part_key < periods.period_last_date + 5 AND "
        "  arx_points.scd_out >= periods.period_first_date AND arx_points.scd_out < periods.period_last_date  AND ";
    arxSQLText +=
        "  arx_points.pr_del>=0 AND \n"
        "  arx_points.part_key = arx_trfer_stat.part_key AND \n"
        "  arx_points.point_id = arx_trfer_stat.point_id \n";
    if (!info.user.access.airps.empty()) {
        if (info.user.access.airps_permit)
            arxSQLText += " AND arx_points.airp IN "+GetSQLEnum(info.user.access.airps);
        else
            arxSQLText += " AND arx_points.airp NOT IN "+GetSQLEnum(info.user.access.airps);
    }
    if (!info.user.access.airlines.empty()) {
        if (info.user.access.airlines_permit)
            arxSQLText += " AND arx_points.airline IN "+GetSQLEnum(info.user.access.airlines);
        else
            arxSQLText += " AND arx_points.airline NOT IN "+GetSQLEnum(info.user.access.airlines);
    }
    if(ap.size()) {
        arxSQLText +=
            " and arx_points.airp = :ap and \n"
            " arx_points.airline not in " + AIRLINE_LIST;
        Qry.CreateVariable("ap", otString, ap);
    } else if(ak.size()) {
        arxSQLText +=
            " and arx_points.airline = :ak and \n"
            " arx_points.airp in " + AIRP_LIST;
        Qry.CreateVariable("ak", otString, ak);
    }

    TDateTime FirstDate = NodeAsDateTime("FirstDate", reqNode);
    TDateTime LastDate = NodeAsDateTime("LastDate", reqNode);
    if(IncMonth(FirstDate, 1) < LastDate)
        throw UserException("��ਮ� ���᪠ �� ������ �ॢ���� 1 �����");
    Qry.CreateVariable("FirstDate", otDate, FirstDate);
    Qry.CreateVariable("LastDate", otDate, LastDate);
    TFullStat FullStat;

    for(int i = 0; i < 2; i++) {
        Qry.SQLText = (i == 0 ? mainSQLText : arxSQLText);
        Qry.Execute();
        if(!Qry.Eof) {
            int col_point_id = Qry.FieldIndex("point_id");
            int col_airp = Qry.FieldIndex("airp");
            int col_airline = Qry.FieldIndex("airline");
            int col_pax_amount = Qry.FieldIndex("pax_amount");
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
                if(!ap.empty()) {
                    key.col1 = Qry.FieldAsString(col_airp);
                    key.col2 = Qry.FieldAsString(col_airline);
                } else {
                    key.col1 = Qry.FieldAsString(col_airline);
                    key.col2 = Qry.FieldAsString(col_airp);
                }
                key.flt_no = Qry.FieldAsInteger(col_flt_no);
                key.scd_out = Qry.FieldAsDateTime(col_scd_out);
                key.point_id = Qry.FieldAsInteger(col_point_id);
                key.places = Qry.FieldAsString(col_places);
                TFullStatRow &row = FullStat[key];
                if(row.pax_amount == NoExists) {
                    row.pax_amount = Qry.FieldAsInteger(col_pax_amount);
                    row.adult = Qry.FieldAsInteger(col_adult);
                    row.child = Qry.FieldAsInteger(col_child);
                    row.baby = Qry.FieldAsInteger(col_baby);
                    row.rk_weight = Qry.FieldAsInteger(col_rk_weight);
                    row.bag_amount = Qry.FieldAsInteger(col_bag_amount);
                    row.bag_weight = Qry.FieldAsInteger(col_bag_weight);
                    row.excess = Qry.FieldAsInteger(col_excess);
                } else {
                    row.pax_amount += Qry.FieldAsInteger(col_pax_amount);
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
    }

    if(!FullStat.empty()) {
        xmlNodePtr grdNode = NewTextChild(resNode, "grd");
        xmlNodePtr headerNode = NewTextChild(grdNode, "header");
        xmlNodePtr colNode;
        if(ap.size()) {
            colNode = NewTextChild(headerNode, "col", "��� �/�");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", taLeftJustify);

            colNode = NewTextChild(headerNode, "col", "��� �/�");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", taLeftJustify);
        } else {
            colNode = NewTextChild(headerNode, "col", "��� �/�");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", taLeftJustify);

            colNode = NewTextChild(headerNode, "col", "��� �/�");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", taLeftJustify);
        }
        colNode = NewTextChild(headerNode, "col", "����� ३�");
        SetProp(colNode, "width", 75);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", "���");
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", taLeftJustify);

        colNode = NewTextChild(headerNode, "col", "���ࠢ�����");
        SetProp(colNode, "width", 90);
        SetProp(colNode, "align", taLeftJustify);

        colNode = NewTextChild(headerNode, "col", "���-�� ����.");
        SetProp(colNode, "width", 75);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", "��");
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", "��");
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", "��");
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", "�/����� (���)");
        SetProp(colNode, "width", 80);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", "����� (����/���)");
        SetProp(colNode, "width", 100);
        SetProp(colNode, "align", taCenter);

        colNode = NewTextChild(headerNode, "col", "����. (���)");
        SetProp(colNode, "width", 70);
        SetProp(colNode, "align", taRightJustify);

        xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
        xmlNodePtr rowNode;
        int total_pax_amount = 0;
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
                string airp;
                if(!ap.empty()) {
                    airp = im->first.col1;
                } else {
                    airp = im->first.col2;
                }
                region = AirpTZRegion(airp);
            }
            catch(UserException &E)
            {
                showErrorMessage((string)E.what()+". ������� ३�� �� �⮡ࠦ�����");
                Qry.Next();
                continue;
            };

            rowNode = NewTextChild(rowsNode, "row");
            NewTextChild(rowNode, "col", im->first.col1);
            NewTextChild(rowNode, "col", im->first.col2);

            total_pax_amount += im->second.pax_amount;
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
            NewTextChild(rowNode, "col", im->first.places);
            NewTextChild(rowNode, "col", im->second.pax_amount);
            NewTextChild(rowNode, "col", im->second.adult);
            NewTextChild(rowNode, "col", im->second.child);
            NewTextChild(rowNode, "col", im->second.baby);
            NewTextChild(rowNode, "col", im->second.rk_weight);
            NewTextChild(rowNode, "col", IntToString(im->second.bag_amount) + "/" + IntToString(im->second.bag_weight));
            NewTextChild(rowNode, "col", im->second.excess);
            NewTextChild(rowNode, "col", im->first.point_id);
        }
        rowNode = NewTextChild(rowsNode, "row");
        NewTextChild(rowNode, "col", "�⮣�:");
        NewTextChild(rowNode, "col");
        NewTextChild(rowNode, "col");
        NewTextChild(rowNode, "col");
        NewTextChild(rowNode, "col");
        NewTextChild(rowNode, "col", total_pax_amount);
        NewTextChild(rowNode, "col", total_adult);
        NewTextChild(rowNode, "col", total_child);
        NewTextChild(rowNode, "col", total_baby);
        NewTextChild(rowNode, "col", total_rk_weight);
        NewTextChild(rowNode, "col", IntToString(total_bag_amount) + "/" + IntToString(total_bag_weight));
        NewTextChild(rowNode, "col", total_excess);
    } else
        throw UserException("��� ������");
    STAT::set_variables(resNode);
}

void RunFullStat(xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo &info = *(TReqInfo::Instance());
    if (info.user.access.airlines.empty() && info.user.access.airlines_permit ||
            info.user.access.airps.empty() && info.user.access.airps_permit)
        throw UserException("��� ������");
    get_report_form("FullStat", resNode);

    string ak = Trim(NodeAsString("ak", reqNode));
    string ap = Trim(NodeAsString("ap", reqNode));

    TQuery Qry(&OraSession);
    string mainSQLText =
        "select \n"
        "  points.airp, \n"
        "  points.airline, \n"
        "  points.flt_no, \n"
        "  points.scd_out, \n"
        "  stat.point_id, \n"
        "  ckin.get_airps(stat.point_id) places, \n"
        "  sum(adult + child + baby) pax_amount, \n"
        "  sum(adult) adult, \n"
        "  sum(child) child, \n"
        "  sum(baby) baby, \n"
        "  sum(unchecked) rk_weight, \n"
        "  sum(pcs) bag_amount, \n"
        "  sum(weight) bag_weight, \n"
        "  sum(excess) excess \n"
        "from \n"
        "  points, \n"
        "  stat \n";
    if(!ap.empty())
        mainSQLText += ", " + AIRP_PERIODS;
    else if(!ak.empty())
        mainSQLText += ", " + AIRLINE_PERIODS;
    mainSQLText +=
        "where \n"
        "  points.point_id = stat.point_id and points.pr_del>=0 and \n";
    if(ap.empty() and ak.empty())
        mainSQLText +=
            "  points.scd_out >= :FirstDate AND points.scd_out < :LastDate \n";
    else
        mainSQLText +=
            "  points.scd_out >= periods.period_first_date AND points.scd_out < periods.period_last_date  \n";
    if (!info.user.access.airps.empty()) {
        if (info.user.access.airps_permit)
            mainSQLText += " AND points.airp IN "+GetSQLEnum(info.user.access.airps);
        else
            mainSQLText += " AND points.airp NOT IN "+GetSQLEnum(info.user.access.airps);
    }
    if (!info.user.access.airlines.empty()) {
        if (info.user.access.airlines_permit)
            mainSQLText += " AND points.airline IN "+GetSQLEnum(info.user.access.airlines);
        else
            mainSQLText += " AND points.airline NOT IN "+GetSQLEnum(info.user.access.airlines);
    }
    if(ap.size()) {
        mainSQLText +=
            " and points.airp = :ap and \n"
            " points.airline not in " + AIRLINE_LIST;
        Qry.CreateVariable("ap", otString, ap);
    } else if(ak.size()) {
        mainSQLText +=
            " and points.airline = :ak and \n"
            " points.airp in " + AIRP_LIST;
        Qry.CreateVariable("ak", otString, ak);
    }
    mainSQLText +=
        "group by \n"
        "  points.airp, \n"
        "  points.airline, \n"
        "  points.flt_no, \n"
        "  points.scd_out, \n"
        "  stat.point_id \n";

    string arxSQLText =
        "select \n"
        "  arx_points.airp, \n"
        "  arx_points.airline, \n"
        "  arx_points.flt_no, \n"
        "  arx_points.scd_out, \n"
        "  arx_stat.point_id, \n"
        "  arch.get_airps(arx_stat.point_id, arx_stat.part_key) places, \n"
        "  sum(adult + child + baby) pax_amount, \n"
        "  sum(adult) adult, \n"
        "  sum(child) child, \n"
        "  sum(baby) baby, \n"
        "  sum(unchecked) rk_weight, \n"
        "  sum(pcs) bag_amount, \n"
        "  sum(weight) bag_weight, \n"
        "  sum(excess) excess \n"
        "from \n"
        "  arx_points, \n"
        "  arx_stat \n";
    if(!ap.empty())
        arxSQLText += ", " + AIRP_PERIODS;
    else if(!ak.empty())
        arxSQLText += ", " + AIRLINE_PERIODS;
    arxSQLText +=
        "where \n";
    if(ap.empty() and ak.empty())
        arxSQLText +=
            "  arx_points.part_key >= :FirstDate AND arx_points.part_key < :LastDate + 5 AND "
            "  arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate AND ";
    else
        arxSQLText +=
        "  arx_points.part_key >= periods.period_first_date AND arx_points.part_key < periods.period_last_date + 5 AND "
        "  arx_points.scd_out >= periods.period_first_date AND arx_points.scd_out < periods.period_last_date  AND ";
    arxSQLText +=
        "  arx_points.pr_del>=0 AND \n"
        "  arx_points.part_key = arx_stat.part_key AND \n"
        "  arx_points.point_id = arx_stat.point_id \n";
    if (!info.user.access.airps.empty()) {
        if (info.user.access.airps_permit)
            arxSQLText += " AND arx_points.airp IN "+GetSQLEnum(info.user.access.airps);
        else
            arxSQLText += " AND arx_points.airp NOT IN "+GetSQLEnum(info.user.access.airps);
    }
    if (!info.user.access.airlines.empty()) {
        if (info.user.access.airlines_permit)
            arxSQLText += " AND arx_points.airline IN "+GetSQLEnum(info.user.access.airlines);
        else
            arxSQLText += " AND arx_points.airline NOT IN "+GetSQLEnum(info.user.access.airlines);
    }
    if(ap.size()) {
        arxSQLText +=
            " and arx_points.airp = :ap and \n"
            " arx_points.airline not in " + AIRLINE_LIST;
        Qry.CreateVariable("ap", otString, ap);
    } else if(ak.size()) {
        arxSQLText +=
            " and arx_points.airline = :ak and \n"
            " arx_points.airp in " + AIRP_LIST;
        Qry.CreateVariable("ak", otString, ak);
    }
        arxSQLText +=
        "group by \n"
        "  arx_points.airp, \n"
        "  arx_points.airline, \n"
        "  arx_points.flt_no, \n"
        "  arx_points.scd_out, \n"
        "  arx_stat.point_id, \n"
        "  arx_stat.part_key \n";

    TDateTime FirstDate = NodeAsDateTime("FirstDate", reqNode);
    TDateTime LastDate = NodeAsDateTime("LastDate", reqNode);
    if(IncMonth(FirstDate, 1) < LastDate)
        throw UserException("��ਮ� ���᪠ �� ������ �ॢ���� 1 �����");
    Qry.CreateVariable("FirstDate", otDate, FirstDate);
    Qry.CreateVariable("LastDate", otDate, LastDate);
    TFullStat FullStat;

    for(int i = 0; i < 2; i++) {
        Qry.SQLText = (i == 0 ? mainSQLText : arxSQLText);
        Qry.Execute();
        if(!Qry.Eof) {
            int col_point_id = Qry.FieldIndex("point_id");
            int col_airp = Qry.FieldIndex("airp");
            int col_airline = Qry.FieldIndex("airline");
            int col_pax_amount = Qry.FieldIndex("pax_amount");
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
                if(!ap.empty()) {
                    key.col1 = Qry.FieldAsString(col_airp);
                    key.col2 = Qry.FieldAsString(col_airline);
                } else {
                    key.col1 = Qry.FieldAsString(col_airline);
                    key.col2 = Qry.FieldAsString(col_airp);
                }
                key.flt_no = Qry.FieldAsInteger(col_flt_no);
                key.scd_out = Qry.FieldAsDateTime(col_scd_out);
                key.point_id = Qry.FieldAsInteger(col_point_id);
                key.places = Qry.FieldAsString(col_places);
                TFullStatRow &row = FullStat[key];
                if(row.pax_amount == NoExists) {
                    row.pax_amount = Qry.FieldAsInteger(col_pax_amount);
                    row.adult = Qry.FieldAsInteger(col_adult);
                    row.child = Qry.FieldAsInteger(col_child);
                    row.baby = Qry.FieldAsInteger(col_baby);
                    row.rk_weight = Qry.FieldAsInteger(col_rk_weight);
                    row.bag_amount = Qry.FieldAsInteger(col_bag_amount);
                    row.bag_weight = Qry.FieldAsInteger(col_bag_weight);
                    row.excess = Qry.FieldAsInteger(col_excess);
                } else {
                    row.pax_amount += Qry.FieldAsInteger(col_pax_amount);
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
    }

    if(!FullStat.empty()) {
        xmlNodePtr grdNode = NewTextChild(resNode, "grd");
        xmlNodePtr headerNode = NewTextChild(grdNode, "header");
        xmlNodePtr colNode;
        if(ap.size()) {
            colNode = NewTextChild(headerNode, "col", "��� �/�");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", taLeftJustify);

            colNode = NewTextChild(headerNode, "col", "��� �/�");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", taLeftJustify);
        } else {
            colNode = NewTextChild(headerNode, "col", "��� �/�");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", taLeftJustify);

            colNode = NewTextChild(headerNode, "col", "��� �/�");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", taLeftJustify);
        }
        colNode = NewTextChild(headerNode, "col", "����� ३�");
        SetProp(colNode, "width", 75);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", "���");
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", taLeftJustify);

        colNode = NewTextChild(headerNode, "col", "���ࠢ�����");
        SetProp(colNode, "width", 90);
        SetProp(colNode, "align", taLeftJustify);

        colNode = NewTextChild(headerNode, "col", "���-�� ����.");
        SetProp(colNode, "width", 75);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", "��");
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", "��");
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", "��");
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", "�/����� (���)");
        SetProp(colNode, "width", 80);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", "����� (����/���)");
        SetProp(colNode, "width", 100);
        SetProp(colNode, "align", taCenter);

        colNode = NewTextChild(headerNode, "col", "����. (���)");
        SetProp(colNode, "width", 70);
        SetProp(colNode, "align", taRightJustify);

        xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
        xmlNodePtr rowNode;
        int total_pax_amount = 0;
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
                string airp;
                if(!ap.empty()) {
                    airp = im->first.col1;
                } else {
                    airp = im->first.col2;
                }
                region = AirpTZRegion(airp);
            }
            catch(UserException &E)
            {
                showErrorMessage((string)E.what()+". ������� ३�� �� �⮡ࠦ�����");
                Qry.Next();
                continue;
            };

            rowNode = NewTextChild(rowsNode, "row");
            NewTextChild(rowNode, "col", im->first.col1);
            NewTextChild(rowNode, "col", im->first.col2);

            total_pax_amount += im->second.pax_amount;
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
            NewTextChild(rowNode, "col", im->first.places);
            NewTextChild(rowNode, "col", im->second.pax_amount);
            NewTextChild(rowNode, "col", im->second.adult);
            NewTextChild(rowNode, "col", im->second.child);
            NewTextChild(rowNode, "col", im->second.baby);
            NewTextChild(rowNode, "col", im->second.rk_weight);
            NewTextChild(rowNode, "col", IntToString(im->second.bag_amount) + "/" + IntToString(im->second.bag_weight));
            NewTextChild(rowNode, "col", im->second.excess);
        }
        rowNode = NewTextChild(rowsNode, "row");
        NewTextChild(rowNode, "col", "�⮣�:");
        NewTextChild(rowNode, "col");
        NewTextChild(rowNode, "col");
        NewTextChild(rowNode, "col");
        NewTextChild(rowNode, "col");
        NewTextChild(rowNode, "col", total_pax_amount);
        NewTextChild(rowNode, "col", total_adult);
        NewTextChild(rowNode, "col", total_child);
        NewTextChild(rowNode, "col", total_baby);
        NewTextChild(rowNode, "col", total_rk_weight);
        NewTextChild(rowNode, "col", IntToString(total_bag_amount) + "/" + IntToString(total_bag_weight));
        NewTextChild(rowNode, "col", total_excess);
    } else
        throw UserException("��� ������");
    STAT::set_variables(resNode);
}

struct TShortStatRow {
    int flt_amount, pax_amount;
    TShortStatRow():
        flt_amount(NoExists),
        pax_amount(NoExists)
    {}
};
typedef map<string, TShortStatRow> TShortStat;

void RunShortStat(xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo &info = *(TReqInfo::Instance());
    if (info.user.access.airlines.empty() && info.user.access.airlines_permit ||
            info.user.access.airps.empty() && info.user.access.airps_permit)
        throw UserException("��� ������");
    get_report_form("ShortStat", resNode);

    string ak = Trim(NodeAsString("ak", reqNode));
    string ap = Trim(NodeAsString("ap", reqNode));

    TQuery Qry(&OraSession);
    string mainSQLText =
        "select  \n";
    if(ap.size())
        mainSQLText +=
        "    points.airp,  \n";
    else
        mainSQLText +=
        "    points.airline,  \n";
    mainSQLText +=
        "    count(distinct stat.point_id) flt_amount, \n"
        "    sum(adult + child + baby) pax_amount \n"
        "from  \n"
        "  points, \n"
        "  stat \n";
    if(!ap.empty())
        mainSQLText += ", " + AIRP_PERIODS;
    else if(!ak.empty())
        mainSQLText += ", " + AIRLINE_PERIODS;
    mainSQLText +=
        "where \n"
        "  points.point_id = stat.point_id and points.pr_del>=0 and \n";
    if(ap.empty() and ak.empty())
        mainSQLText +=
            "  points.scd_out >= :FirstDate AND points.scd_out < :LastDate \n";
    else
        mainSQLText +=
            "  points.scd_out >= periods.period_first_date AND points.scd_out < periods.period_last_date  \n";
    if (!info.user.access.airps.empty()) {
        if (info.user.access.airps_permit)
            mainSQLText += " AND points.airp IN "+GetSQLEnum(info.user.access.airps);
        else
            mainSQLText += " AND points.airp NOT IN "+GetSQLEnum(info.user.access.airps);
    }
    if (!info.user.access.airlines.empty()) {
        if (info.user.access.airlines_permit)
            mainSQLText += " AND points.airline IN "+GetSQLEnum(info.user.access.airlines);
        else
            mainSQLText += " AND points.airline NOT IN "+GetSQLEnum(info.user.access.airlines);
    }
    if(ap.size()) {
        mainSQLText +=
            " and points.airp = :ap and \n"
            " points.airline not in " + AIRLINE_LIST;
        Qry.CreateVariable("ap", otString, ap);
    } else if(ak.size()) {
        mainSQLText +=
            " and points.airline = :ak and \n"
            " points.airp in " + AIRP_LIST;
        Qry.CreateVariable("ak", otString, ak);
    }
        mainSQLText +=
        "group by  \n";
    if(ap.size())
        mainSQLText +=
        "    points.airp \n";
    else
        mainSQLText +=
        "    points.airline \n";

    string arxSQLText =
        "select  \n";
    if(ap.size())
        arxSQLText +=
        "    arx_points.airp,  \n";
    else
        arxSQLText +=
        "    arx_points.airline,  \n";
    arxSQLText +=
        "    count(distinct arx_stat.point_id) flt_amount, \n"
        "    sum(adult + child + baby) pax_amount \n"
        "from  \n"
        "  arx_points, \n"
        "  arx_stat \n";
    if(!ap.empty())
        arxSQLText += ", " + AIRP_PERIODS;
    else if(!ak.empty())
        arxSQLText += ", " + AIRLINE_PERIODS;
    arxSQLText +=
        "where \n";
    if(ap.empty() and ak.empty())
        arxSQLText +=
            "  arx_points.part_key >= :FirstDate AND arx_points.part_key < :LastDate + 5 AND \n"
            "  arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate AND \n";
    else
        arxSQLText +=
        "  arx_points.part_key >= periods.period_first_date AND arx_points.part_key < periods.period_last_date + 5 AND \n"
        "  arx_points.scd_out >= periods.period_first_date AND arx_points.scd_out < periods.period_last_date  AND \n";
    arxSQLText +=
        "  arx_points.pr_del>=0 AND \n"
        "  arx_points.part_key = arx_stat.part_key AND \n"
        "  arx_points.point_id = arx_stat.point_id \n";
    if (!info.user.access.airps.empty()) {
        if (info.user.access.airps_permit)
            arxSQLText += " AND arx_points.airp IN "+GetSQLEnum(info.user.access.airps);
        else
            arxSQLText += " AND arx_points.airp NOT IN "+GetSQLEnum(info.user.access.airps);
    }
    if (!info.user.access.airlines.empty()) {
        if (info.user.access.airlines_permit)
            arxSQLText += " AND arx_points.airline IN "+GetSQLEnum(info.user.access.airlines);
        else
            arxSQLText += " AND arx_points.airline NOT IN "+GetSQLEnum(info.user.access.airlines);
    }
    if(ap.size()) {
        arxSQLText +=
            " and arx_points.airp = :ap and \n"
            " arx_points.airline not in " + AIRLINE_LIST;
        Qry.CreateVariable("ap", otString, ap);
    } else if(ak.size()) {
        arxSQLText +=
            " and arx_points.airline = :ak and \n"
            " arx_points.airp in " + AIRP_LIST;
        Qry.CreateVariable("ak", otString, ak);
    }
        arxSQLText +=
        "group by  \n";
    if(ap.size())
        arxSQLText +=
        "    arx_points.airp \n";
    else
        arxSQLText +=
        "    arx_points.airline \n";
    Qry.CreateVariable("FirstDate", otDate, NodeAsDateTime("FirstDate", reqNode));
    Qry.CreateVariable("LastDate", otDate, NodeAsDateTime("LastDate", reqNode));

    TShortStat ShortStat;
    for(int i = 0; i < 2; i++) {
        Qry.SQLText = (i == 0 ? mainSQLText : arxSQLText);
        Qry.Execute();
        for(; !Qry.Eof; Qry.Next()) {
            string stat_key = Qry.FieldAsString(0);
            TShortStatRow &row = ShortStat[stat_key];
            if(row.flt_amount == NoExists) {
                row.flt_amount = Qry.FieldAsInteger("flt_amount");
                row.pax_amount = Qry.FieldAsInteger("pax_amount");
            } else {
                row.flt_amount += Qry.FieldAsInteger("flt_amount");
                row.pax_amount += Qry.FieldAsInteger("pax_amount");
            }
        }
    }
    if(!ShortStat.empty()) {
        xmlNodePtr grdNode = NewTextChild(resNode, "grd");
        xmlNodePtr headerNode = NewTextChild(grdNode, "header");
        xmlNodePtr colNode;
        if(ap.size()) {
            colNode = NewTextChild(headerNode, "col", "��� �/�");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", taLeftJustify);
        } else {
            colNode = NewTextChild(headerNode, "col", "��� �/�");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", taLeftJustify);
        }
        colNode = NewTextChild(headerNode, "col", "���-�� ३ᮢ");
        SetProp(colNode, "width", 85);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", "���-�� ����.");
        SetProp(colNode, "width", 85);
        SetProp(colNode, "align", taRightJustify);

        xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
        xmlNodePtr rowNode;
        int total_flt_amount = 0;
        int total_pax_amount = 0;

        for(TShortStat::iterator si = ShortStat.begin(); si != ShortStat.end(); si++) {
            rowNode = NewTextChild(rowsNode, "row");
            NewTextChild(rowNode, "col", si->first);
            total_flt_amount += si->second.flt_amount;
            total_pax_amount += si->second.pax_amount;
            NewTextChild(rowNode, "col", si->second.flt_amount);
            NewTextChild(rowNode, "col", si->second.pax_amount);
        }
        rowNode = NewTextChild(rowsNode, "row");
        NewTextChild(rowNode, "col", "�⮣�:");
        NewTextChild(rowNode, "col", total_flt_amount);
        NewTextChild(rowNode, "col", total_pax_amount);
    } else
        throw UserException("��� ������");
    STAT::set_variables(resNode);
}

struct TDetailStatKey {
    string col1, col2;
};
struct TDetailCmp {
    bool operator() (const TDetailStatKey &lr, const TDetailStatKey &rr) const
    {
        if(lr.col1 == rr.col1)
            return lr.col2 < rr.col2;
        else
            return lr.col1 < rr.col1;
    };
};
typedef map<TDetailStatKey, TShortStatRow, TDetailCmp> TDetailStat;

void RunDetailStat(xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo &info = *(TReqInfo::Instance());
    if (info.user.access.airlines.empty() && info.user.access.airlines_permit ||
            info.user.access.airps.empty() && info.user.access.airps_permit)
        throw UserException("��� ������");
    get_report_form("DetailStat", resNode);

    string ak = Trim(NodeAsString("ak", reqNode));
    string ap = Trim(NodeAsString("ap", reqNode));

    TQuery Qry(&OraSession);
    string mainSQLText =
        "select \n"
        "  points.airp, \n"
        "  points.airline, \n"
        "  count(distinct stat.point_id) flt_amount, \n"
        "  sum(adult + child + baby) pax_amount \n"
        "from \n"
        "  points, \n"
        "  stat \n";
    if(!ap.empty())
        mainSQLText += ", " + AIRP_PERIODS;
    else if(!ak.empty())
        mainSQLText += ", " + AIRLINE_PERIODS;
    mainSQLText +=
        "where \n"
        "  points.point_id = stat.point_id and points.pr_del>=0 and \n";
    if(ap.empty() and ak.empty())
        mainSQLText +=
            "  points.scd_out >= :FirstDate AND points.scd_out < :LastDate \n";
    else
        mainSQLText +=
            "  points.scd_out >= periods.period_first_date AND points.scd_out < periods.period_last_date  \n";
    if (!info.user.access.airps.empty()) {
        if (info.user.access.airps_permit)
            mainSQLText += " AND points.airp IN "+GetSQLEnum(info.user.access.airps);
        else
            mainSQLText += " AND points.airp NOT IN "+GetSQLEnum(info.user.access.airps);
    }
    if (!info.user.access.airlines.empty()) {
        if (info.user.access.airlines_permit)
            mainSQLText += " AND points.airline IN "+GetSQLEnum(info.user.access.airlines);
        else
            mainSQLText += " AND points.airline NOT IN "+GetSQLEnum(info.user.access.airlines);
    }
    if(ap.size()) {
        mainSQLText +=
            " and points.airp = :ap and \n"
            " points.airline not in " + AIRLINE_LIST;
        Qry.CreateVariable("ap", otString, ap);
    } else if(ak.size()) {
        mainSQLText +=
            " and points.airline = :ak and \n"
            " points.airp in " + AIRP_LIST;
        Qry.CreateVariable("ak", otString, ak);
    }
    mainSQLText +=
        "group by \n"
        "  points.airp, \n"
        "  points.airline \n";

    string arxSQLText =
        "select \n"
        "  arx_points.airp, \n"
        "  arx_points.airline, \n"
        "  count(distinct arx_stat.point_id) flt_amount, \n"
        "  sum(adult + child + baby) pax_amount \n"
        "from \n"
        "  arx_points, \n"
        "  arx_stat \n";
    if(!ap.empty())
        arxSQLText += ", " + AIRP_PERIODS;
    else if(!ak.empty())
        arxSQLText += ", " + AIRLINE_PERIODS;
    arxSQLText +=
        "where \n";
    if(ap.empty() and ak.empty())
        arxSQLText +=
            "  arx_points.part_key >= :FirstDate AND arx_points.part_key < :LastDate + 5 AND \n"
            "  arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate AND \n";
    else
        arxSQLText +=
            "  arx_points.part_key >= periods.period_first_date AND arx_points.part_key < periods.period_last_date  + 5 AND \n"
            "  arx_points.scd_out >= periods.period_first_date AND arx_points.scd_out < periods.period_last_date  AND \n";
    arxSQLText +=
        "  arx_points.pr_del>=0 AND \n"
        "  arx_points.part_key = arx_stat.part_key AND \n"
        "  arx_points.point_id = arx_stat.point_id \n";
    if (!info.user.access.airps.empty()) {
        if (info.user.access.airps_permit)
            arxSQLText += " AND arx_points.airp IN "+GetSQLEnum(info.user.access.airps);
        else
            arxSQLText += " AND arx_points.airp NOT IN "+GetSQLEnum(info.user.access.airps);
    }
    if (!info.user.access.airlines.empty()) {
        if (info.user.access.airlines_permit)
            arxSQLText += " AND arx_points.airline IN "+GetSQLEnum(info.user.access.airlines);
        else
            arxSQLText += " AND arx_points.airline NOT IN "+GetSQLEnum(info.user.access.airlines);
    }
    if(ap.size()) {
        arxSQLText +=
            " and arx_points.airp = :ap and \n"
            " arx_points.airline not in " + AIRLINE_LIST;
        Qry.CreateVariable("ap", otString, ap);
    } else if(ak.size()) {
        arxSQLText +=
            " and arx_points.airline = :ak and \n"
            " arx_points.airp in " + AIRP_LIST;
        Qry.CreateVariable("ak", otString, ak);
    }
    arxSQLText +=
        "group by \n"
        "  arx_points.airp, \n"
        "  arx_points.airline \n";

    Qry.CreateVariable("FirstDate", otDate, NodeAsDateTime("FirstDate", reqNode));
    Qry.CreateVariable("LastDate", otDate, NodeAsDateTime("LastDate", reqNode));
    TDetailStat DetailStat;

    for(int i = 0; i < 2; i++) {
        Qry.SQLText = (i == 0 ? mainSQLText : arxSQLText);
        Qry.Execute();
        for(; !Qry.Eof; Qry.Next()) {
            TDetailStatKey key;
            if(!ap.empty()) {
                key.col1 = Qry.FieldAsString("airp");
                key.col2 = Qry.FieldAsString("airline");
            } else {
                key.col1 = Qry.FieldAsString("airline");
                key.col2 = Qry.FieldAsString("airp");
            }
            TShortStatRow &row = DetailStat[key];
            if(row.flt_amount == NoExists) {
                row.flt_amount = Qry.FieldAsInteger("flt_amount");
                row.pax_amount = Qry.FieldAsInteger("pax_amount");
            } else {
                row.flt_amount += Qry.FieldAsInteger("flt_amount");
                row.pax_amount += Qry.FieldAsInteger("pax_amount");
            }
        }
    }

    if(!DetailStat.empty()) {
        xmlNodePtr grdNode = NewTextChild(resNode, "grd");
        xmlNodePtr headerNode = NewTextChild(grdNode, "header");
        xmlNodePtr colNode;
        if(ap.size()) {
            colNode = NewTextChild(headerNode, "col", "��� �/�");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", taLeftJustify);

            colNode = NewTextChild(headerNode, "col", "��� �/�");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", taLeftJustify);
        } else {
            colNode = NewTextChild(headerNode, "col", "��� �/�");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", taLeftJustify);

            colNode = NewTextChild(headerNode, "col", "��� �/�");
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", taLeftJustify);
        }
        colNode = NewTextChild(headerNode, "col", "���-�� ३ᮢ");
        SetProp(colNode, "width", 85);
        SetProp(colNode, "align", taRightJustify);

        colNode = NewTextChild(headerNode, "col", "���-�� ����.");
        SetProp(colNode, "width", 85);
        SetProp(colNode, "align", taRightJustify);

        xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
        xmlNodePtr rowNode;
        int total_flt_amount = 0;
        int total_pax_amount = 0;
        for(TDetailStat::iterator si = DetailStat.begin(); si != DetailStat.end(); si++) {
            rowNode = NewTextChild(rowsNode, "row");
            NewTextChild(rowNode, "col", si->first.col1);
            NewTextChild(rowNode, "col", si->first.col2);

            total_flt_amount += si->second.flt_amount;
            total_pax_amount += si->second.pax_amount;

            NewTextChild(rowNode, "col", si->second.flt_amount);
            NewTextChild(rowNode, "col", si->second.pax_amount);
            Qry.Next();
        }
        rowNode = NewTextChild(rowsNode, "row");
        NewTextChild(rowNode, "col", "�⮣�:");
        NewTextChild(rowNode, "col");
        NewTextChild(rowNode, "col", total_flt_amount);
        NewTextChild(rowNode, "col", total_pax_amount);
    } else
        throw UserException("��� ������");
    STAT::set_variables(resNode);
}

void StatInterface::RunStat(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    string name = NodeAsString("stat_mode", reqNode);

    try {
        if(name == "���஡���") RunFullStat(reqNode, resNode);
        else if(name == "����") RunShortStat(reqNode, resNode);
        else if(name == "��⠫���஢�����") RunDetailStat(reqNode, resNode);
        else if(name == "�࠭���") RunTrferFullStat(reqNode, resNode);
        else throw Exception("Unknown stat mode " + name);
    } catch (EOracleError E) {
        if(E.Code == 376)
            throw UserException("� �������� ��������� ��� ���� �� 䠩��� �� �⪫�祭. ������� � ������������");
        else
            throw;
    }

    ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
}

void StatInterface::PaxSrcRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo &info = *(TReqInfo::Instance());
    if (info.user.access.airlines.empty() && info.user.access.airlines_permit ||
            info.user.access.airps.empty() && info.user.access.airps_permit)
        throw UserException("�� ������� �� ������ ���ᠦ��");
    TDateTime FirstDate = NodeAsDateTime("FirstDate", reqNode);
    TDateTime LastDate = NodeAsDateTime("LastDate", reqNode);
    if(IncMonth(FirstDate, 3) < LastDate)
        throw UserException("��ਮ� ���᪠ �� ������ �ॢ���� 3 �����");
    FirstDate = ClientToUTC(FirstDate, info.desk.tz_region);
    LastDate = ClientToUTC(LastDate, info.desk.tz_region);
    TPerfTimer tm;
    TQuery Qry(&OraSession);
    Qry.CreateVariable("FirstDate", otDate, FirstDate);
    Qry.CreateVariable("LastDate", otDate, LastDate);
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
            throw UserException("����� ��ᯮ�� ������ ᮤ�ঠ�� �� ����� 6-� ᨬ�����");
        Qry.CreateVariable("document", otString, document);
    }
    string ticket_no = NodeAsStringFast("ticket_no", paramNode, "");
    if(!ticket_no.empty()) {
        if(ticket_no.size() < 6)
            throw UserException("����� ����� ������ ᮤ�ঠ�� �� ����� 6-� ᨬ�����");
        Qry.CreateVariable("ticket_no", otString, ticket_no);
    }
    string tag_no = NodeAsStringFast("tag_no", paramNode, "");
    if(!tag_no.empty()) {
        if(tag_no.size() < 3)
            throw UserException("����� ��ન ������ ᮤ�ঠ�� �� ����� 3-� ��᫥���� ���");
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
                "   points.airp, "
                "   points.scd_out, "
                "   NVL(points.act_out,NVL(points.est_out,points.scd_out)) AS real_out, "
                "   pax.reg_no, "
                "   pax_grp.airp_arv, "
                "   pax.surname||' '||pax.name full_name, "
                "   NVL(ckin.get_bagAmount(pax.grp_id,pax.pax_id,rownum),0) bag_amount, "
                "   NVL(ckin.get_bagWeight(pax.grp_id,pax.pax_id,rownum),0) bag_weight, "
                "   NVL(ckin.get_rkWeight(pax.grp_id,pax.pax_id,rownum),0) rk_weight, "
                "   NVL(ckin.get_excess(pax.grp_id,pax.pax_id),0) excess, "
                "   pax_grp.grp_id, "
                "   ckin.get_birks(pax.grp_id,pax.pax_id,0) tags, "
                "   DECODE(pax.refuse,NULL,DECODE(pax.pr_brd,0,'��ॣ.','��ᠦ��'),'���ॣ.('||pax.refuse||')') AS status, "
                "   cls_grp.code class, "
                "   salons.get_seat_no(pax.pax_id, pax.seats, pax_grp.status, pax_grp.point_dep, 'seats', rownum) seat_no, "
                "   pax_grp.hall hall, "
                "   pax.document, "
                "   pax.ticket_no "
                "FROM  pax_grp,pax, points, cls_grp ";
            if(!tag_no.empty())
                SQLText +=
                    " , bag_tags ";
            SQLText +=
                "WHERE "
                "   points.scd_out >= :FirstDate AND points.scd_out < :LastDate and "
                "   points.point_id = pax_grp.point_dep and points.pr_del>=0 and "
                "   pax_grp.grp_id=pax.grp_id AND "
                "   pax_grp.class_grp = cls_grp.id ";
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
                "   arch.get_birks(arx_pax.part_key,arx_pax.grp_id,arx_pax.pax_id,0) tags, "
                "   DECODE(arx_pax.refuse,NULL,DECODE(arx_pax.pr_brd,0,'��ॣ.','��ᠦ��'), "
                "       '���ॣ.('||arx_pax.refuse||')') AS status, "
                "   cls_grp.code class, "
                "   LPAD(seat_no,3,'0')|| "
                "       DECODE(SIGN(1-seats),-1,'+'||TO_CHAR(seats-1),'') seat_no, "
                "   arx_pax_grp.hall hall, "
                "   arx_pax.document, "
                "   arx_pax.ticket_no "
                "FROM  arx_pax_grp,arx_pax, arx_points, cls_grp ";
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
                "   arx_pax_grp.class_grp = cls_grp.id and "
                "   arx_points.part_key >= :FirstDate and "
                "   pr_brd IS NOT NULL ";
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
        } catch (EOracleError E) {
            if(E.Code == 376)
                throw UserException("� �������� ��������� ��� ���� �� 䠩��� �� �⪫�祭. ������� � ������������");
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
            int col_status = Qry.FieldIndex("status");
            int col_class = Qry.FieldIndex("class");
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
                    trip_item.trip = GetTripName(trip_info);
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
                NewTextChild(paxNode, "airp_arv", Qry.FieldAsString(col_airp_arv));
                NewTextChild(paxNode, "tags", Qry.FieldAsString(col_tags));
                NewTextChild(paxNode, "status", Qry.FieldAsString(col_status));
                NewTextChild(paxNode, "class", Qry.FieldAsString(col_class));
                NewTextChild(paxNode, "seat_no", Qry.FieldAsString(col_seat_no));
                NewTextChild(paxNode, "document", Qry.FieldAsString(col_document));
                NewTextChild(paxNode, "ticket_no", Qry.FieldAsString(col_ticket_no));
                NewTextChild(paxNode, "hall", Qry.FieldAsInteger(col_hall));

                count++;
                if(count >= MAX_STAT_ROWS) {
                    showErrorMessage(
                            "��࠭� ᫨誮� ����� ��ப. �������� " +
                            IntToString(MAX_STAT_ROWS) +
                            " �ந������� ��ப."
                            " ��筨� ��ࠬ���� ���᪠."
                            );
                    break;
                }
            }
            ProgTrace(TRACE5, "XML%d: %s", i, tm.PrintWithMessage().c_str());
            ProgTrace(TRACE5, "count: %d", count);
        }
    }
    if(count == 0)
        throw UserException("�� ������� �� ������ ���ᠦ��.");

    xmlNodePtr headerNode = NewTextChild(paxListNode, "header");
    xmlNodePtr colNode;

    colNode = NewTextChild(headerNode, "col", "����");
    SetProp(colNode, "width", 53);
    SetProp(colNode, "align", taLeftJustify);

    colNode = NewTextChild(headerNode, "col", "���");
    SetProp(colNode, "width", 61);
    SetProp(colNode, "align", taLeftJustify);

    colNode = NewTextChild(headerNode, "col", "�");
    SetProp(colNode, "width", 25);
    SetProp(colNode, "align", taRightJustify);

    colNode = NewTextChild(headerNode, "col", "�������");
    SetProp(colNode, "width", 173);
    SetProp(colNode, "align", taLeftJustify);

    colNode = NewTextChild(headerNode, "col", "�/�");
    SetProp(colNode, "width", 32);
    SetProp(colNode, "align", taLeftJustify);

    colNode = NewTextChild(headerNode, "col", "����");
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", taRightJustify);

    colNode = NewTextChild(headerNode, "col", "���");
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", taRightJustify);

    colNode = NewTextChild(headerNode, "col", "�/�");
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", taRightJustify);

    colNode = NewTextChild(headerNode, "col", "����");
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", taRightJustify);

    colNode = NewTextChild(headerNode, "col", "��ન");
    SetProp(colNode, "width", 163);
    SetProp(colNode, "align", taLeftJustify);

    colNode = NewTextChild(headerNode, "col", "�����");
    SetProp(colNode, "width", 93);
    SetProp(colNode, "align", taLeftJustify);

    colNode = NewTextChild(headerNode, "col", "��.");
    SetProp(colNode, "width", 25);
    SetProp(colNode, "align", taLeftJustify);

    colNode = NewTextChild(headerNode, "col", "� �");
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", taLeftJustify);

    colNode = NewTextChild(headerNode, "col", "���");
    SetProp(colNode, "width", 78);
    SetProp(colNode, "align", taLeftJustify);

    colNode = NewTextChild(headerNode, "col", "���㬥��");
    SetProp(colNode, "width", 114);
    SetProp(colNode, "align", taLeftJustify);

    colNode = NewTextChild(headerNode, "col", "� �����");
    SetProp(colNode, "width", 101);
    SetProp(colNode, "align", taLeftJustify);

    STAT::set_variables(resNode);
    get_report_form("ArxPaxList", resNode);
    ProgTrace(TRACE5, "%s", GetXMLDocText(resNode->doc).c_str());
}

void StatInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{

}
