#include <fstream>
#include "stat_main.h"
#include "oralib.h"
#include "cache.h"
#include "xml_unit.h"
#include "exceptions.h"
#include "stl_utils.h"
#include "misc.h"
#include "docs/docs_common.h"
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
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/crc.hpp>
#include "md5_sum.h"
#include "payment_base.h"
#include "report_common.h"
#include "stat_utils.h"
#include "stat_bi.h"
#include "stat_vo.h"
#include "stat_ha.h"
#include "stat_ad.h"
#include "stat_unacc.h"
#include "stat_reprint.h"
#include "stat_annul_bt.h"
#include "stat_rem.h"
#include "stat_services.h"
#include "stat_rfisc.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

#define WITHOUT_TOTAL_WHEN_PROBLEM false

using namespace std;
using namespace EXCEPTIONS;
using namespace STAT;
using namespace AstraLocale;
using namespace BASIC::date_time;
using namespace ASTRA::date_time;

const string SYSTEM_USER = "���⥬�";

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
    "��⮢",
    "�믮������",
    "���०���",
    "�訡��",
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

            sql << "    " << TTripInfo::selectedFields() << ", \n"
                   "    point_num \n";
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
    NewTextChild(variablesNode, "report_title", getLocaleText("����樨 � ��⥬�"));
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
    NewTextChild(headerNode, "col", "�����"); // ��� ᮢ���⨬��� � ��ன ���ᨥ� �ନ����

    map<int, string> TripItems;
    xmlNodePtr rowsNode = NULL;
    TDeskAccess desk_access;
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

                if(not station.empty() and not desk_access.get(station))
                    continue;

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
                            "select " + TTripInfo::selectedFields() +
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

void UnaccompListToXML(TQuery &Qry, xmlNodePtr resNode, TComplexBagExcessNodeList &excessNodeList, bool isPaxSearch, int pass, int &count)
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
  int col_excess_wt = Qry.FieldIndex("excess_wt");
  int col_excess_pc = Qry.FieldIndex("excess_pc");
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

      excessNodeList.add(paxNode, "excess", TBagPieces(Qry.FieldAsInteger(col_excess_pc)),
                                            TBagKilos(Qry.FieldAsInteger(col_excess_wt)));

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

void PaxListToXML(TQuery &Qry, xmlNodePtr resNode, TComplexBagExcessNodeList& excessNodeList, bool isPaxSearch, int pass, int &count)
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
  int col_excess_wt = Qry.FieldIndex("excess_wt");
  int col_excess_pc = Qry.FieldIndex("excess_pc");
  int col_grp_id = Qry.FieldIndex("grp_id");
  int col_airp_arv = Qry.FieldIndex("airp_arv");
  int col_tags = Qry.FieldIndex("tags");
  int col_pr_brd = Qry.FieldIndex("pr_brd");
  int col_refuse = Qry.FieldIndex("refuse");
  int col_class_grp = Qry.FieldIndex("class_grp");
  int col_cabin_class_grp = Qry.FieldIndex("cabin_class_grp");
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

      excessNodeList.add(paxNode, "excess", TBagPieces(Qry.FieldAsInteger(col_excess_pc)),
                                            TBagKilos(Qry.FieldAsInteger(col_excess_wt)));

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
      NewTextChild(paxNode, "class", clsGrpIdsToCodeNative(Qry.FieldAsInteger(col_class_grp),
                                                           Qry.FieldAsInteger(col_cabin_class_grp)));
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
                "   null part_key, " +
                TTripInfo::selectedFields("points") + ", "
                "   pax.reg_no, "
                "   pax_grp.airp_arv, "
                "   pax.surname||' '||pax.name full_name, "
                "   NVL(ckin.get_bagAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) bag_amount, "
                "   NVL(ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) bag_weight, "
                "   NVL(ckin.get_rkWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) rk_weight, "
                "   ckin.get_excess_wt(pax.grp_id, pax.pax_id, pax_grp.excess_wt, pax_grp.bag_refuse) AS excess_wt, "
                "   ckin.get_excess_pc(pax.grp_id, pax.pax_id) AS excess_pc, "
                "   pax_grp.grp_id, "
                "   ckin.get_birks2(pax.grp_id,pax.pax_id,pax.bag_pool_num,:pr_lat) tags, "
                "   pax.refuse, "
                "   pax.pr_brd, "
                "   pax_grp.class_grp, "
                "   NVL(pax.cabin_class_grp, pax_grp.class_grp) AS cabin_class_grp, "
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
                "   arx_points.part_key, " +
                TTripInfo::selectedFields("arx_points") + ", "
                "   arx_pax.reg_no, "
                "   arx_pax_grp.airp_arv, "
                "   arx_pax.surname||' '||arx_pax.name full_name, "
                "   NVL(arch.get_bagAmount2(arx_pax.part_key,arx_pax.grp_id,arx_pax.pax_id,arx_pax.bag_pool_num,rownum),0) bag_amount, "
                "   NVL(arch.get_bagWeight2(arx_pax.part_key,arx_pax.grp_id,arx_pax.pax_id,arx_pax.bag_pool_num,rownum),0) bag_weight, "
                "   NVL(arch.get_rkWeight2(arx_pax.part_key,arx_pax.grp_id,arx_pax.pax_id,arx_pax.bag_pool_num,rownum),0) rk_weight, "
                "   arch.get_excess_wt(arx_pax.part_key, arx_pax.grp_id, arx_pax.pax_id, arx_pax_grp.excess_wt, arx_pax_grp.excess, arx_pax_grp.bag_refuse) AS excess_wt, "
                "   arx_pax.excess_pc, "
                "   arx_pax_grp.grp_id, "
                "   arch.get_birks2(arx_pax.part_key,arx_pax.grp_id,arx_pax.pax_id,arx_pax.bag_pool_num,:pr_lat) tags, "
                "   arx_pax.refuse, "
                "   arx_pax.pr_brd, "
                "   arx_pax_grp.class_grp, "
                "   NVL(arx_pax.cabin_class_grp, arx_pax_grp.class_grp) AS cabin_class_grp, "
                "   LPAD(seat_no,3,'0')|| "
                "       DECODE(SIGN(1-seats),-1,'+'||TO_CHAR(seats-1),'') seat_no, "
                "   arx_pax_grp.hall, "
                "   arx_pax.ticket_no, "
                "   arx_pax.pax_id, "
                "   arx_pax_grp.status "
                "FROM arx_pax_grp, arx_pax, arx_points "
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
        TComplexBagExcessNodeList excessNodeList(OutputLang(), {});
        PaxListToXML(Qry, resNode, excessNodeList, false, 0, count);

        ProgTrace(TRACE5, "XML: %s", tm.PrintWithMessage().c_str());

        //��ᮯ஢������� �����
        if(part_key == NoExists)  {
            Qry.SQLText=
                "SELECT "
                "   null part_key, " +
                TTripInfo::selectedFields("points") + ", "
                "   pax_grp.airp_arv, "
                "   ckin.get_bagAmount2(pax_grp.grp_id,NULL,NULL) AS bag_amount, "
                "   ckin.get_bagWeight2(pax_grp.grp_id,NULL,NULL) AS bag_weight, "
                "   ckin.get_rkWeight2(pax_grp.grp_id,NULL,NULL) AS rk_weight, "
                "   ckin.get_excess_wt(pax_grp.grp_id, NULL, pax_grp.excess_wt, pax_grp.bag_refuse) AS excess_wt, "
                "   ckin.get_excess_pc(pax_grp.grp_id, NULL) AS excess_pc, "
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
                "  arx_pax_grp.part_key, " +
                TTripInfo::selectedFields("arx_points") + ", "
                "  arx_pax_grp.airp_arv, "
                "  arch.get_bagAmount2(arx_pax_grp.part_key,arx_pax_grp.grp_id,NULL,NULL) AS bag_amount, "
                "  arch.get_bagWeight2(arx_pax_grp.part_key,arx_pax_grp.grp_id,NULL,NULL) AS bag_weight, "
                "  arch.get_rkWeight2(arx_pax_grp.part_key,arx_pax_grp.grp_id,NULL,NULL) AS rk_weight, "
                "  arch.get_excess_wt(arx_pax_grp.part_key, arx_pax_grp.grp_id, NULL, arx_pax_grp.excess_wt, arx_pax_grp.excess, arx_pax_grp.bag_refuse) AS excess_wt, "
                "  arx_pax_grp.excess_pc, "
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
           " excess_wt, \n"
           " NVL(excess_pc,0) AS excess_pc \n";
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
           " SUM(excess_wt) AS excess_wt, \n"
           " SUM(NVL(excess_pc,0)) AS excess_pc \n";
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

/* GRISHA */
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
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", web);

        colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", web_bag);

        colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", web_bp);

        // ���᪨
        colNode = NewTextChild(headerNode, "col", getLocaleText("���᪨"));
        SetProp(colNode, "width", 40);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", kiosk);

        colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", kiosk_bag);

        colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", kiosk_bp);

        // ���.
        colNode = NewTextChild(headerNode, "col", getLocaleText("���."));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", mobile);

        colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", mobile_bag);

        colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
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

struct TFullStatRow {
    int pax_amount;
    TInetStat i_stat;
    int adult;
    int child;
    int baby;
    int rk_weight;
    int bag_amount;
    int bag_weight;
    TBagKilos excess_wt;
    TBagPieces excess_pc;
    TFullStatRow():
        pax_amount(0),
        adult(0),
        child(0),
        baby(0),
        rk_weight(0),
        bag_amount(0),
        bag_weight(0),
        excess_wt(0),
        excess_pc(0)
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
            excess_wt == item.excess_wt &&
            excess_pc == item.excess_pc;
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
        excess_wt += item.excess_wt;
        excess_pc += item.excess_pc;
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

// TODO ��२�������� ����� � ���� bool full � ��-� ⨯� override_MAX_STAT_ROWS()
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
      int col_excess_wt = Qry.FieldIndex("excess_wt");
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
        row.excess_wt = Qry.FieldAsInteger(col_excess_wt);
        row.excess_pc = Qry.FieldAsInteger(col_excess_pc);
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
          if(rows >= MAX_STAT_ROWS()) {
              throw MaxStatRowsException("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH", LParams() << LParam("num", MAX_STAT_ROWS()));
              /*AstraLocale::showErrorMessage("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH",
                      LParams() << LParam("num", MAX_STAT_ROWS()));
              if (WITHOUT_TOTAL_WHEN_PROBLEM) showTotal=false; //�� �㤥� �����뢠�� �⮣���� ��ப� ���� �� ����� � ����㦤����
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
            colNode = NewTextChild(headerNode, "col", getLocaleText("��� �/�"));
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", TAlignment::LeftJustify);
            SetProp(colNode, "sort", sortString);
            NewTextChild(rowNode, "col", getLocaleText("�⮣�:"));
            if (params.statType==statDetail)
            {
                colNode = NewTextChild(headerNode, "col", getLocaleText("��� �/�"));
                SetProp(colNode, "width", 50);
                SetProp(colNode, "align", TAlignment::LeftJustify);
                SetProp(colNode, "sort", sortString);
                NewTextChild(rowNode, "col");
            };
        } else {
            colNode = NewTextChild(headerNode, "col", getLocaleText("��� �/�"));
            SetProp(colNode, "width", 50);
            SetProp(colNode, "align", TAlignment::LeftJustify);
            SetProp(colNode, "sort", sortString);
            NewTextChild(rowNode, "col", getLocaleText("�⮣�:"));
            if (params.statType==statDetail)
            {
                colNode = NewTextChild(headerNode, "col", getLocaleText("��� �/�"));
                SetProp(colNode, "width", 50);
                SetProp(colNode, "align", TAlignment::LeftJustify);
                SetProp(colNode, "sort", sortString);
                NewTextChild(rowNode, "col");
            };
        }
    }

    if(params.statType != statPactShort) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("���-�� ३ᮢ"));
        SetProp(colNode, "width", 85);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", (int)(pr_pact?total.flts.size():total.flt_amount));
    }

    if(params.statType == statPactShort)
    {
        colNode = NewTextChild(headerNode, "col", getLocaleText("� �������"));
        SetProp(colNode, "width", 230);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col", getLocaleText("�⮣�:"));
    }

    colNode = NewTextChild(headerNode, "col", getLocaleText("���-�� ����."));
    SetProp(colNode, "width", 85);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.pax_amount);

    if(params.statType != statPactShort) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("�"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.f);

        colNode = NewTextChild(headerNode, "col", getLocaleText("�"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.c);

        colNode = NewTextChild(headerNode, "col", getLocaleText("�"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.y);

        total.i_stat.toXML(headerNode, rowNode);
    }

    if(pr_pact and params.statType != statPactShort)
    {
        colNode = NewTextChild(headerNode, "col", getLocaleText("� �������"));
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
          if(rows >= MAX_STAT_ROWS()) {
              throw MaxStatRowsException("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH", LParams() << LParam("num", MAX_STAT_ROWS()));
              /*AstraLocale::showErrorMessage("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH",
                                            LParams() << LParam("num", MAX_STAT_ROWS()));
              if (WITHOUT_TOTAL_WHEN_PROBLEM) showTotal=false; //�� �㤥� �����뢠�� �⮣���� ��ப� ���� �� ����� � ����㦤����
              break;*/
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

          NewTextChild(rowNode, "col", im->second.excess_pc.getQuantity());
          NewTextChild(rowNode, "col", im->second.excess_wt.getQuantity());
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
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col", getLocaleText("�⮣�:"));

        colNode = NewTextChild(headerNode, "col", getLocaleText("��� �/�"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col");
    } else {
        colNode = NewTextChild(headerNode, "col", getLocaleText("��� �/�"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col", getLocaleText("�⮣�:"));

        colNode = NewTextChild(headerNode, "col", getLocaleText("��� �/�"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col");
    }
    colNode = NewTextChild(headerNode, "col", getLocaleText("����� ३�"));
    SetProp(colNode, "width", 75);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col");

    colNode = NewTextChild(headerNode, "col", getLocaleText("���"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDate);
    NewTextChild(rowNode, "col");

    colNode = NewTextChild(headerNode, "col", getLocaleText("���ࠢ�����"));
    SetProp(colNode, "width", 90);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col");

    colNode = NewTextChild(headerNode, "col", getLocaleText("���-�� ����."));
    SetProp(colNode, "width", 75);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.pax_amount);

    if (params.statType==statFull)
        total.i_stat.toXML(headerNode, rowNode);

    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.adult);

    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.child);

    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.baby);

    colNode = NewTextChild(headerNode, "col", getLocaleText("�/����� (���)"));
    SetProp(colNode, "width", 80);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.rk_weight);

    colNode = NewTextChild(headerNode, "col", getLocaleText("�� ����"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortIntegerSlashInteger);
    NewTextChild(rowNode, "col", total.bag_amount);

    colNode = NewTextChild(headerNode, "col", getLocaleText("�� ���"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortIntegerSlashInteger);
    NewTextChild(rowNode, "col", total.bag_weight);

    colNode = NewTextChild(headerNode, "col", getLocaleText("��.�"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col",  total.excess_pc.getQuantity());

    colNode = NewTextChild(headerNode, "col", getLocaleText("��.���"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col",  total.excess_wt.getQuantity());

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
          //横� �� ��������
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
          //横� �� ���⠬
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
            buf << "��� �/�" << delim;
            if (params.statType==statDetail)
                buf << "��� �/�" << delim;
        } else {
            buf << "��� �/�" << delim;
            if (params.statType==statDetail)
                buf << "��� �/�" << delim;
        }
    }
    if(params.statType != statPactShort)
        buf << "���-�� ३ᮢ" << delim;
    if(params.statType == statPactShort)
        buf << "� �������" << delim;
    buf << "���-�� ����." << delim;
    if (params.statType != statPactShort)
    {
        buf << "�" << delim;
        buf << "�" << delim;
        buf << "�" << delim;
        /* TInetStat::toXML begin */
        buf << "Web" << delim;
        buf << "��" << delim;
        buf << "��" << delim;
        buf << "���᪨" << delim;
        buf << "��" << delim;
        buf << "��" << delim;
        buf << "���." << delim;
        buf << "��" << delim;
        buf << "��" << delim;
        /* end */
    }
    if(pr_pact and params.statType != statPactShort)
        buf << "� �������" << delim;
    buf << endl; /* ������� ���⮩ �⮫��� */
}

void TDetailStatCombo::add_data(ostringstream &buf) const
{
    if(params.statType != statPactShort)
        buf << data.first.col1 << delim; // col1
    if (params.statType == statDetail)
        buf << data.first.col2 << delim; // col2
    if(params.statType != statPactShort)
        buf << (int)(pr_pact ? data.second.flts.size() : data.second.flt_amount) << delim; // ���-�� ३ᮢ
    if(params.statType == statPactShort)
        buf << data.first.pact_descr << delim; // � �������
    buf << data.second.pax_amount << delim; // ���-�� ����.
    if (params.statType != statPactShort)
    {
        buf << data.second.f << delim; // �
        buf << data.second.c << delim; // �
        buf << data.second.y << delim; // �
        buf << data.second.i_stat.web << delim; // Web
        buf << data.second.i_stat.web_bag << delim; // ��
        buf << data.second.i_stat.web_bp << delim; // ��
        buf << data.second.i_stat.kiosk << delim; // ���᪨
        buf << data.second.i_stat.kiosk_bag << delim; // ��
        buf << data.second.i_stat.kiosk_bp << delim; // ��
        buf << data.second.i_stat.mobile << delim; // ���.
        buf << data.second.i_stat.mobile_bag << delim; // ��
        buf << data.second.i_stat.mobile_bp << delim; // ��
    }
    if(pr_pact and params.statType != statPactShort)
        buf << data.first.pact_descr << delim; // � �������
    buf << endl; /* ������� ���⮩ �⮫��� */
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
          //横� �� ��������
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
          //横� �� ���⠬
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
        buf << "��� �/�" << delim;
        buf << "��� �/�" << delim;
    } else {
        buf << "��� �/�" << delim;
        buf << "��� �/�" << delim;
    }
    buf << "����� ३�" << delim;
    buf << "���" << delim;
    buf << "���ࠢ�����" << delim;
    buf << "���-�� ����." << delim;
    if (params.statType==statFull)
    {
        /* TInetStat::toXML begin */
        buf << "Web" << delim;
        buf << "��" << delim;
        buf << "��" << delim;
        buf << "���᪨" << delim;
        buf << "��" << delim;
        buf << "��" << delim;
        buf << "���." << delim;
        buf << "��" << delim;
        buf << "��" << delim;
        /* end */
    }
    buf << "��" << delim;
    buf << "��" << delim;
    buf << "��" << delim;
    buf << "�/����� (���)" << delim;
    buf << "�� ����" << delim;
    buf << "�� ���" << delim;
    buf << "��.�" << delim;
    buf << "��.���" << endl;
}

void TFullStatCombo::add_data(ostringstream &buf) const
{
    string region;
    try { region = AirpTZRegion(data.first.airp); }
    catch(AstraLocale::UserException &E) { return; };
    buf << data.first.col1 << delim; // col1
    buf << data.first.col2 << delim; // col2
    buf << data.first.flt_no << delim; // ����� ३�
    buf << DateTimeToStr(UTCToClient(data.first.scd_out, region), "dd.mm.yy") << delim; // ���
    buf << data.first.places.get() << delim; // ���ࠢ�����
    buf << data.second.pax_amount << delim; // ���-�� ����.
    if (params.statType==statFull)
    {
        buf << data.second.i_stat.web << delim; // Web
        buf << data.second.i_stat.web_bag << delim; // ��
        buf << data.second.i_stat.web_bp << delim; // ��
        buf << data.second.i_stat.kiosk << delim; // ���᪨
        buf << data.second.i_stat.kiosk_bag << delim; // ��
        buf << data.second.i_stat.kiosk_bp << delim; // ��
        buf << data.second.i_stat.mobile << delim; // ���.
        buf << data.second.i_stat.mobile_bag << delim; // ��
        buf << data.second.i_stat.mobile_bp << delim; // ��
    }
    buf << data.second.adult << delim; // ��
    buf << data.second.child << delim; // ��
    buf << data.second.baby << delim; // ��
    buf << data.second.rk_weight << delim; // �/����� (���)
    buf << data.second.bag_amount << delim; // �� ����
    buf << data.second.bag_weight << delim; // �� ���
    buf << data.second.excess_pc.getQuantity() << delim; // ��.�
    buf << data.second.excess_wt.getQuantity() << endl; // ��.���
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

        if(not params.reg_type.empty()) {
            SQLText += " and self_ckin_stat.client_type = :reg_type ";
            Qry.CreateVariable("reg_type", otString, params.reg_type);
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
            if (WITHOUT_TOTAL_WHEN_PROBLEM) showTotal=false; //�� �㤥� �����뢠�� �⮣���� ��ப� ���� �� ����� � ����㦤����
            break;*/
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
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col", getLocaleText("�⮣�:"));
    colNode = NewTextChild(headerNode, "col", getLocaleText("����"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col");
    colNode = NewTextChild(headerNode, "col", getLocaleText("�� ����"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col");
    if(params.statType == statSelfCkinFull) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("�ਬ�砭��"));
        SetProp(colNode, "width", 280);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col");
    }
    colNode = NewTextChild(headerNode, "col", getLocaleText("��� �/�"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col");
    if(
            params.statType == statSelfCkinDetail or
            params.statType == statSelfCkinFull
      ) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("��� �/�"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col");
    }
    if(
            params.statType == statSelfCkinShort or
            params.statType == statSelfCkinDetail
      ) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("���-�� ३ᮢ"));
        SetProp(colNode, "width", 85);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", flts_total);
    }
    if(params.statType == statSelfCkinFull) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("����� ३�"));
        SetProp(colNode, "width", 75);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col");
        colNode = NewTextChild(headerNode, "col", getLocaleText("���"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortDate);
        NewTextChild(rowNode, "col");
        colNode = NewTextChild(headerNode, "col", getLocaleText("���ࠢ�����"));
        SetProp(colNode, "width", 90);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col");
    }


    // ���᪨
    colNode = NewTextChild(headerNode, "col", getLocaleText("���."));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.pax_amount);

    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.term_bag);

    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.term_bp);

    colNode = NewTextChild(headerNode, "col", getLocaleText("��� ᠬ�"));
    SetProp(colNode, "width", 45);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.pax_amount-total.term_ckin_service);

    if(params.statType == statSelfCkinFull) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.adult);
        colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.child);
        colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
        SetProp(colNode, "width", 30);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.baby);
    }
    if(
            params.statType == statSelfCkinDetail or
            params.statType == statSelfCkinFull
      ) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("�����."));
        SetProp(colNode, "width", 45);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.tckin);
    }
    if(
            params.statType == statSelfCkinShort or
            params.statType == statSelfCkinDetail
      ) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("�ਬ�砭��"));
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
    NewTextChild(variablesNode, "stat_mode", getLocaleText("����ॣ������"));
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
    buf << getLocaleText("��� ॣ.") << delim;
    buf << getLocaleText("����") << delim;
    buf << getLocaleText("�� ����") << delim;
    if (params.statType == statSelfCkinFull)
        buf << getLocaleText("�ਬ�砭��") << delim;
    buf << getLocaleText("��� �/�") << delim;
    if (params.statType == statSelfCkinDetail or params.statType == statSelfCkinFull)
        buf << getLocaleText("��� �/�") << delim;
    if (params.statType == statSelfCkinShort or params.statType == statSelfCkinDetail)
        buf << getLocaleText("���-�� ३ᮢ") << delim;
    if (params.statType == statSelfCkinFull)
    {
        buf << getLocaleText("����� ३�") << delim;
        buf << getLocaleText("���") << delim;
        buf << getLocaleText("���ࠢ�����") << delim;
    }
    buf << getLocaleText("���.") << delim;
    buf << getLocaleText("��") << delim;
    buf << getLocaleText("��") << delim;
    buf << getLocaleText("��� ᠬ�") << delim;
    if (params.statType == statSelfCkinFull)
    {
        buf << getLocaleText("��") << delim;
        buf << getLocaleText("��") << delim;
        buf << getLocaleText("��") << delim;
    }
    if (params.statType == statSelfCkinDetail or params.statType == statSelfCkinFull)
        buf << getLocaleText("�����.") << delim;
    if (params.statType == statSelfCkinShort or params.statType == statSelfCkinDetail)
        buf << getLocaleText("�ਬ�砭��") << delim;
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
    buf << data.first.client_type << delim; // ��� ॣ.
    buf << data.first.desk << delim; // ����
    buf << data.first.desk_airp << delim; // �/� ����
    if (params.statType == statSelfCkinFull)
        buf << data.first.descr << delim; // �ਬ�砭��
    buf << data.first.ak << delim; // ��� �/�
    if (params.statType == statSelfCkinDetail or params.statType == statSelfCkinFull)
        buf << ElemIdToCodeNative(etAirp, data.first.ap) << delim; // ��� �/�
    if (params.statType == statSelfCkinShort or params.statType == statSelfCkinDetail)
        buf << (int)data.second.flts.size() << delim; // ���-�� ३ᮢ
    if (params.statType == statSelfCkinFull)
    {
        ostringstream oss1;
        oss1 << setw(3) << setfill('0') << data.first.flt_no;
        buf << oss1.str() << delim; // ����� ३�
        buf << DateTimeToStr(UTCToClient(data.first.scd_out, region), "dd.mm.yy")
         << delim; // ���
        buf << data.first.places.get() << delim; // ���ࠢ�����
    }
    buf << data.second.pax_amount << delim; // ���-�� ����.
    buf << data.second.term_bag << delim; // ��
    buf << data.second.term_bp << delim; // ��
    buf << (data.second.pax_amount - data.second.term_ckin_service)
     << delim; // ��� ᠬ�
    if (params.statType == statSelfCkinFull)
    {
        buf << data.second.adult << delim; // ��
        buf << data.second.child << delim; // ��
        buf << data.second.baby << delim; // ��
    }
    if (params.statType == statSelfCkinDetail or params.statType == statSelfCkinFull)
        buf << data.second.tckin << delim; // �����. ॣ.
    if (params.statType == statSelfCkinShort or params.statType == statSelfCkinDetail)
        buf << data.first.descr << delim; // �ਬ�砭��
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
               // FIXME �஢�ઠ �� ࠢ���⢮ double tlg_len (c++ double equality comparison)
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
              if (WITHOUT_TOTAL_WHEN_PROBLEM) showTotal=false; //�� �㤥� �����뢠�� �⮣���� ��ப� ���� �� ����� � ����㦤����
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
    colNode = NewTextChild(headerNode, "col", getLocaleText("���� ���."));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col", getLocaleText("�⮣�:"));
    colNode = NewTextChild(headerNode, "col", getLocaleText("�����"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col");
    if (params.statType == statTlgOutFull)
    {
      colNode = NewTextChild(headerNode, "col", getLocaleText("���� �����."));
      SetProp(colNode, "width", 70);
      SetProp(colNode, "align", TAlignment::LeftJustify);
      SetProp(colNode, "sort", sortString);
      NewTextChild(rowNode, "col");
    };
    colNode = NewTextChild(headerNode, "col", getLocaleText("���-��"));
    if (params.statType == statTlgOutShort)
      SetProp(colNode, "width", 200);
    else
      SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    NewTextChild(rowNode, "col");
    if (params.statType == statTlgOutFull)
    {
      colNode = NewTextChild(headerNode, "col", getLocaleText("��� ���."));
      SetProp(colNode, "width", 60);
      SetProp(colNode, "align", TAlignment::LeftJustify);
      SetProp(colNode, "sort", sortDate);
      NewTextChild(rowNode, "col");
    };
    if (params.statType == statTlgOutDetail ||
        params.statType == statTlgOutFull)
    {
      colNode = NewTextChild(headerNode, "col", getLocaleText("�/� 䠪�."));
      SetProp(colNode, "width", 50);
      SetProp(colNode, "align", TAlignment::LeftJustify);
      SetProp(colNode, "sort", sortString);
      NewTextChild(rowNode, "col");
      colNode = NewTextChild(headerNode, "col", getLocaleText("�/� ����."));
      SetProp(colNode, "width", 50);
      SetProp(colNode, "align", TAlignment::LeftJustify);
      SetProp(colNode, "sort", sortString);
      NewTextChild(rowNode, "col");
      colNode = NewTextChild(headerNode, "col", getLocaleText("��� �/�"));
      SetProp(colNode, "width", 50);
      SetProp(colNode, "align", TAlignment::LeftJustify);
      SetProp(colNode, "sort", sortString);
      NewTextChild(rowNode, "col");
    };
    if (params.statType == statTlgOutFull)
    {
      colNode = NewTextChild(headerNode, "col", getLocaleText("��� ⫣."));
      SetProp(colNode, "width", 50);
      SetProp(colNode, "align", TAlignment::LeftJustify);
      SetProp(colNode, "sort", sortString);
      NewTextChild(rowNode, "col");
      colNode = NewTextChild(headerNode, "col", getLocaleText("��� �뫥�"));
      SetProp(colNode, "width", 70);
      SetProp(colNode, "align", TAlignment::LeftJustify);
      SetProp(colNode, "sort", sortDate);
      NewTextChild(rowNode, "col");
      colNode = NewTextChild(headerNode, "col", getLocaleText("����"));
      SetProp(colNode, "width", 50);
      SetProp(colNode, "align", TAlignment::LeftJustify);
      SetProp(colNode, "sort", sortString);
      NewTextChild(rowNode, "col");
    };
    colNode = NewTextChild(headerNode, "col", getLocaleText("���-��"));
    SetProp(colNode, "width", params.statType == statTlgOutFull?40:70);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortInteger);
    NewTextChild(rowNode, "col", total.tlg_count);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��ꥬ (����)"));
    SetProp(colNode, "width", params.statType == statTlgOutFull?70:100);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortFloat);
    buf.str("");
    buf << fixed << setprecision(0) << total.tlg_len;
    NewTextChild(rowNode, "col", buf.str());
    colNode = NewTextChild(headerNode, "col", getLocaleText("� �������"));
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
    buf << "���� ���." << delim;
    buf << "�����" << delim;
    if (params.statType == statTlgOutFull)
        buf << "���� �����." << delim;
    buf << "���-��" << delim;
    if (params.statType == statTlgOutFull)
        buf << "��� ���." << delim;
    if (params.statType == statTlgOutDetail || params.statType == statTlgOutFull)
    {
        buf << "�/� 䠪�." << delim;
        buf << "�/� ����." << delim;
        buf << "��� �/�" << delim;
    }
    if (params.statType == statTlgOutFull)
    {
        buf << "��� ⫣." << delim;
        buf << "��� �뫥�" << delim;
        buf << "����" << delim;
    }
    buf << "���-��" << delim;
    buf << "��ꥬ (����)" << delim;
    buf << "� �������" << endl;
}

void TTlgOutStatCombo::add_data(ostringstream &buf) const
{
    buf << data.first.sender_sita_addr << delim; // ���� ���.
    buf << data.first.receiver_descr << delim; // �����
    if (params.statType == statTlgOutFull)
        buf << data.first.receiver_sita_addr << delim; // ���� �����.
    buf << data.first.receiver_country_view << delim; // ���-��
    if (params.statType == statTlgOutFull)
    {
        if (data.first.time_send != NoExists)
            buf << DateTimeToStr(data.first.time_send, "dd.mm.yy"); // ��� ���.
        buf << delim;
    }
    if (params.statType == statTlgOutDetail || params.statType == statTlgOutFull)
    {
        buf << data.first.airline_view << delim; // �/� 䠪�.
        buf << data.first.airline_mark_view << delim; // �/� ����.
        buf << data.first.airp_dep_view << delim; // ��� �/�
    }
    if (params.statType == statTlgOutFull)
    {
        buf << data.first.tlg_type << delim; // ��� ⫣.
        if (data.first.scd_local_date != NoExists)
            buf << DateTimeToStr(data.first.scd_local_date, "dd.mm.yy"); // ��� �뫥�
        buf << delim;
        if (data.first.flt_no != NoExists)
        {
            ostringstream oss1;
            oss1 << setw(3) << setfill('0')
             << data.first.flt_no << data.first.suffix_view;
            buf << oss1.str(); // ����
        }
        buf << delim;
    }
    buf << data.second.tlg_count << delim; // ���-��
    ostringstream oss2;
    oss2 << fixed << setprecision(0) << data.second.tlg_len;
    buf << oss2.str() << delim; // ��ꥬ (����)
    buf << data.first.extra << endl; // � �������
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
            if (WITHOUT_TOTAL_WHEN_PROBLEM) showTotal=false; //�� �㤥� �����뢠�� �⮣���� ��ப� ���� �� ����� � ����㦤����
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
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col", getLocaleText("�⮣�:"));
        colNode = NewTextChild(headerNode, "col", getLocaleText("���"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortDate);
        NewTextChild(rowNode, "col");
    }
    if(params.statType == statAgentFull) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("�⮩��"));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        NewTextChild(rowNode, "col");
    }
    if(
            params.statType == statAgentFull or
            params.statType == statAgentTotal
      ) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("�����"));
        SetProp(colNode, "width", 100);
        SetProp(colNode, "align", TAlignment::LeftJustify);
        SetProp(colNode, "sort", sortString);
        if (params.statType == statAgentTotal)
          NewTextChild(rowNode, "col", getLocaleText("�⮣�:"));
        else
          NewTextChild(rowNode, "col");
    }
    if(params.statType == statAgentTotal) {
        colNode = NewTextChild(headerNode, "col", getLocaleText("���."));
        SetProp(colNode, "width", 45);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.dpax_amount.inc);
        colNode = NewTextChild(headerNode, "col", getLocaleText("�����."));
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.dtckin_amount.inc);
        colNode = NewTextChild(headerNode, "col", getLocaleText("���."));
        SetProp(colNode, "width", 70);
        SetProp(colNode, "align", TAlignment::Center);
        SetProp(colNode, "sort", sortIntegerSlashInteger);
        NewTextChild(rowNode, "col", IntToString(total.dbag_amount.inc) +
                                     "/" +
                                     IntToString(total.dbag_weight.inc));
        colNode = NewTextChild(headerNode, "col", getLocaleText("�/�"));
        SetProp(colNode, "width", 45);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.drk_weight.inc);
    } else {
        colNode = NewTextChild(headerNode, "col", getLocaleText("���.")+" (+)");
        SetProp(colNode, "width", 45);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.dpax_amount.inc);
        colNode = NewTextChild(headerNode, "col", getLocaleText("���.")+" (-)");
        SetProp(colNode, "width", 45);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", -total.dpax_amount.dec);
        colNode = NewTextChild(headerNode, "col", getLocaleText("�����.")+"(+)");
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.dtckin_amount.inc);
        colNode = NewTextChild(headerNode, "col", getLocaleText("�����.")+"(-)");
        SetProp(colNode, "width", 50);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", -total.dtckin_amount.dec);
        colNode = NewTextChild(headerNode, "col", getLocaleText("���.")+" (+)");
        SetProp(colNode, "width", 70);
        SetProp(colNode, "align", TAlignment::Center);
        SetProp(colNode, "sort", sortIntegerSlashInteger);
        NewTextChild(rowNode, "col", IntToString(total.dbag_amount.inc) +
                                     "/" +
                                     IntToString(total.dbag_weight.inc));
        colNode = NewTextChild(headerNode, "col", getLocaleText("���.")+" (-)");
        SetProp(colNode, "width", 70);
        SetProp(colNode, "align", TAlignment::Center);
        SetProp(colNode, "sort", sortIntegerSlashInteger);
        NewTextChild(rowNode, "col", IntToString(-total.dbag_amount.dec) +
                                     "/" +
                                     IntToString(-total.dbag_weight.dec));
        colNode = NewTextChild(headerNode, "col", getLocaleText("�/�")+" (+)");
        SetProp(colNode, "width", 45);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", total.drk_weight.inc);
        colNode = NewTextChild(headerNode, "col", getLocaleText("�/�")+" (-)");
        SetProp(colNode, "width", 45);
        SetProp(colNode, "align", TAlignment::RightJustify);
        SetProp(colNode, "sort", sortInteger);
        NewTextChild(rowNode, "col", -total.drk_weight.dec);
        colNode = NewTextChild(headerNode, "col", getLocaleText("���./���."));
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
        buf << "����" << delim;
        buf << "���" << delim;
    }
    if (params.statType == statAgentFull)
        buf << "�⮩��" << delim;
    if (params.statType == statAgentFull or params.statType == statAgentTotal)
        buf << "�����" << delim;
    if (params.statType == statAgentTotal)
    {
        buf << "���." << delim;
        buf << "�����." << delim;
        buf << "���." << delim;
        buf << "�/�";
    }
    else
    {
        buf << "���. (+)" << delim;
        buf << "���. (-)" << delim;
        buf << "�����.(+)" << delim;
        buf << "�����.(-)" << delim;
        buf << "���. (+)" << delim;
        buf << "���. (-)" << delim;
        buf << "�/� (+)" << delim;
        buf << "�/� (-)" << delim;
        buf << "���./���.";
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
        buf << oss.str() << delim; // ��� �/�
        buf << DateTimeToStr( scd_out_local, "dd.mm.yy") << delim; // ���
    }
    if (params.statType == statAgentFull)
        buf << data.first.desk << delim; // ����
    if (params.statType == statAgentFull or params.statType == statAgentTotal)
        buf << data.first.user_descr << delim; // ���짮��⥫�
    if (params.statType == statAgentTotal)
    {
        buf << data.second.dpax_amount.inc << delim; // ���-�� ����.
        buf << data.second.dtckin_amount.inc << delim; // ���-�� ᪢��.
        buf <<  IntToString(data.second.dbag_amount.inc) + "/" +
                IntToString(data.second.dbag_weight.inc) << delim; // ����� (����/���)
        buf << data.second.drk_weight.inc << endl; // �/����� (���)
    }
    else
    {
        buf << data.second.dpax_amount.inc << delim; // ���-�� ����. (+)
        buf << -data.second.dpax_amount.dec << delim; // ���-�� ����. (-)
        buf << data.second.dtckin_amount.inc << delim; // ���-�� ᪢��. (+)
        buf << -data.second.dtckin_amount.dec << delim; // ���-�� ᪢��. (-)
        buf <<  IntToString(data.second.dbag_amount.inc) + "/" +
                IntToString(data.second.dbag_weight.inc) << delim; // ����� (����/���) (+)
        buf <<  IntToString(-data.second.dbag_amount.dec) + "/" +
                IntToString(-data.second.dbag_weight.dec) << delim; // ����� (����/���) (-)
        buf << data.second.drk_weight.inc << delim; // �/����� (���) (+)
        buf << -data.second.drk_weight.dec << delim; // �/����� (���) (-)
        oss.str("");
        if (data.second.processed_pax!=0)
            oss << fixed << setprecision(2) << data.second.time / data.second.processed_pax;
        else
            oss << fixed << setprecision(2) << 0.0;
        buf << oss.str() << endl; // �।��� �६�, ����祭��� �� ���ᠦ��
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
    NodeSetContent(paramNode, idx->second.c_str());
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
    string edit_fmt;
    string filter;
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
    NewTextChild(itemNode, "edit_fmt", edit_fmt);
    NewTextChild(itemNode, "filter", filter);

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
        static const char *seances[] = {"", "��", "��"};
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
    edit_fmt = Qry.FieldAsString("edit_fmt");
    filter = Qry.FieldAsString("filter");
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
                    (item.ctype != "CkBox" or
                     TReqInfo::Instance()->desk.compatible(STAT_CKBOX_VERSION)) and

                    (TReqInfo::Instance()->desk.compatible(BI_STAT_VERSION) or
                     (Qry.get().FieldIsNULL("edit_fmt") and
                      Qry.get().FieldIsNULL("filter")))
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
}

void createCSVFullStat(const TStatParams &params, const TFullStat &FullStat, const TPrintAirline &prn_airline, ostringstream &data)
{
    if(FullStat.empty()) return;

    const char delim = ';';
    data
        << "��� �/�" << delim
        << "��� �/�" << delim
        << "����� ३�" << delim
        << "���" << delim
        << "���ࠢ�����" << delim
        << "���-�� ����." << delim
        << "Web" << delim
        << "��" << delim
        << "��" << delim
        << "���᪨" << delim
        << "��" << delim
        << "��" << delim
        << "���." << delim
        << "��" << delim
        << "��" << delim
        << "��" << delim
        << "��" << delim
        << "��" << delim
        << "�/����� (���)" << delim
        << "�� ����" << delim
        << "�� ���" << delim
        << "��.�" << delim
        << "��.���"
        << endl;
    bool showTotal = true;
    for(TFullStat::const_iterator i = FullStat.begin(); i != FullStat.end(); i++) {
        //region ��易⥫쭮 � ��砫� 横��, ���� �㤥� �ᯮ�祭 xml
        string region;
        try
        {
            region = AirpTZRegion(i->first.airp);
        }
        catch(AstraLocale::UserException &E)
        {
            AstraLocale::showErrorMessage("MSG.ERR_MSG.NOT_ALL_FLIGHTS_ARE_SHOWN", LParams() << LParam("msg", getLocaleText(E.getLexemaData())));
            if (WITHOUT_TOTAL_WHEN_PROBLEM) showTotal=false; //�� �㤥� �����뢠�� �⮣���� ��ப� ���� �� ����� � ����㦤����
            continue;
        };

        // ��� �/�
        data << i->first.col1 << delim;
        // ��� �/�
        data << i->first.col2 << delim;
        // ����� ३�
        data << i->first.flt_no << delim;
        // ���
        data << DateTimeToStr(
                UTCToClient(i->first.scd_out, region), "dd.mm.yy")
            << delim;
        // ���ࠢ�����
        data << i->first.places.get() << delim;
        // ���-�� ����.
        data << i->second.pax_amount << delim;
        // Web
        data << i->second.i_stat.web << delim;
        // ��
        data << i->second.i_stat.web_bag << delim;
        // ��
        data << i->second.i_stat.web_bp << delim;
        // ���᪨
        data << i->second.i_stat.kiosk << delim;
        // ��
        data << i->second.i_stat.kiosk_bag << delim;
        // ��
        data << i->second.i_stat.kiosk_bp << delim;
        // ���.
        data << i->second.i_stat.mobile << delim;
        // ��
        data << i->second.i_stat.mobile_bag << delim;
        // ��
        data << i->second.i_stat.mobile_bp << delim;
        // ��
        data << i->second.adult << delim;
        // ��
        data << i->second.child << delim;
        // ��
        data << i->second.baby << delim;
        // �/����� (���)
        data << i->second.rk_weight << delim;
        // �� ����
        data << i->second.bag_amount << delim;
        // �� ���
        data << i->second.bag_weight << delim;
        // ��.�
        data << i->second.excess_pc.getQuantity() << delim;
        // ��.���
        data << i->second.excess_wt.getQuantity() << endl;
    }
}

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
            CheckIn::TSimplePaxGrpItem grp;
            if(grp.getByGrpId(grp_id))
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
    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("����"));
    SetProp(colNode, "width", 40);
    SetProp(colNode, "align", TAlignment::RightJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("���"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDate);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
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
            // ��
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirline, row->first.airline));
            // ��
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, row->first.airp));
            // ����
            ostringstream buf;
            buf << setw(3) << setfill('0') << row->first.flt_no << ElemIdToCodeNative(etSuffix, row->first.suffix);
            NewTextChild(rowNode, "col", buf.str());
            // ���
            NewTextChild(rowNode, "col", DateTimeToStr(row->first.scd_out, "dd.mm.yyyy"));
            // ��
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
    NewTextChild(rowNode, "col", getLocaleText("�⮣�:"));
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
    NewTextChild(variablesNode, "stat_mode", getLocaleText("���ᠦ��� � ��࠭�祭�묨 ����������ﬨ"));
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("���஡���"));
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
    buf << "��" << delim;
    buf << "��" << delim;
    buf << "����" << delim;
    buf << "���" << delim;
    buf << "��" << delim;
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
        // ��
        buf << ElemIdToCodeNative(etAirline, data.first.airline) << delim;
        // ��
        buf << ElemIdToCodeNative(etAirp, data.first.airp) << delim;
        // ����
        ostringstream oss1;
        oss1 << setw(3) << setfill('0') << data.first.flt_no << ElemIdToCodeNative(etSuffix, data.first.suffix);
        buf << oss1.str() << delim;
        // ���
        buf << DateTimeToStr(data.first.scd_out, "dd.mm.yyyy") << delim;
        // ��
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
    // ����᪠� processStatOrders �� nosir, ����� �� ��� ������, ��祬� �� ࠡ�⠥�.
    // � ���� �뫠 �訡�� ��⠢�� �訡�� � ����.
    // ���⮬� �訡�� �த㡫�஢��� LogTrace-��.
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

void seg_fault_emul()
{
    // seg fault emul
    char **a = NULL;
    char *b = a[0];
    LogTrace(TRACE5) << *b;

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

struct TPFSAbstractStat {
    virtual ~TPFSAbstractStat() {};
    virtual void add(const TPFSStatRow &row) = 0;
    virtual size_t RowCount() = 0;
    virtual void dump() = 0;
};

struct TPFSStat: public  multiset<TPFSStatRow>, TPFSAbstractStat {
    void dump() {};
    void add(const TPFSStatRow &row)
    {
        this->insert(row);
    }
    size_t RowCount()
    {
        return this->size();
    }
};

typedef map<string, int> TPFSStatusMap;
typedef map<string, TPFSStatusMap> TPFSRouteMap;
typedef map<string, TPFSRouteMap> TPFSFltMap;
typedef map<TDateTime, TPFSFltMap> TPFSScdOutMap;

struct TPFSShortStat: public TPFSScdOutMap, TPFSAbstractStat {
    int FRowCount;
    void dump();
    void add(const TPFSStatRow &row);
    size_t RowCount() { return FRowCount; }
    TPFSShortStat(): FRowCount(0) {}
};

void TPFSShortStat::dump()
{
    for(TPFSScdOutMap::iterator scd_out = begin();
            scd_out != end(); scd_out++) {
        for(TPFSFltMap::iterator flt = scd_out->second.begin();
                flt != scd_out->second.end(); flt++) {
            for(TPFSRouteMap::iterator route = flt->second.begin();
                    route != flt->second.end(); route++) {
                for(TPFSStatusMap::iterator status = route->second.begin();
                        status != route->second.end(); status++) {
                    LogTrace(TRACE5)
                        << "stat[" << DateTimeToStr(scd_out->first, "dd.mm.yyyy") << "]"
                        << "[" << flt->first << "]"
                        << "[" << route->first << "]"
                        << "[" << route->first << "]"
                        << "[" << status->first << "]"
                        << " = " << status->second;
                }
            }
        }
    }
}

void TPFSShortStat::add(const TPFSStatRow &row)
{
    TPFSStatusMap &status = (*this)[row.scd_out][row.flt][row.route];
    if(status.empty()) FRowCount++;
    status[row.status]++;
}

void RunPFSStat(
        const TStatParams &params,
        TPFSAbstractStat &PFSStat,
        TPrintAirline &prn_airline,
        bool full = false
        )
{
    TDateTime first_date = params.FirstDate;
    TDateTime last_date = params.LastDate;
    if(params.LT) {
        first_date -= 1;
        last_date += 1;
    };
    for(int pass = 0; pass <= 2; pass++) {
        QParams QryParams;
        QryParams
            << QParam("FirstDate", otDate, first_date)
            << QParam("LastDate", otDate, last_date);
        if (pass!=0)
            QryParams << QParam("arx_trip_date_range", otInteger, ARX_TRIP_DATE_RANGE());
        string SQLText =
            "select ";
        if(pass != 0)
            SQLText += " points.part_key, ";
        else
            SQLText += " null part_key, ";
        SQLText +=
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
            int col_part_key = Qry.get().FieldIndex("part_key");
            int col_point_id = Qry.get().FieldIndex("point_id");
            int col_scd_out = Qry.get().FieldIndex("scd_out");
            int col_airline = Qry.get().FieldIndex("airline");
            int col_flt_no = Qry.get().FieldIndex("flt_no");
            int col_suffix = Qry.get().FieldIndex("suffix");
            int col_airp = Qry.get().FieldIndex("airp");
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
                TDateTime local_scd_out = Qry.get().FieldAsDateTime(col_scd_out);
                if(params.LT) {
                    local_scd_out = UTCToLocal(local_scd_out,
                            AirpTZRegion(Qry.get().FieldAsString(col_airp)));
                    if(not(local_scd_out >= params.FirstDate and local_scd_out < params.LastDate))
                        continue;
                }
                prn_airline.check(Qry.get().FieldAsString(col_airline));
                TPFSStatRow row;
                row.point_id = Qry.get().FieldAsInteger(col_point_id);
                row.pax_id = Qry.get().FieldAsInteger(col_pax_id);
                row.status = Qry.get().FieldAsString(col_status);
                row.scd_out = local_scd_out;
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

                TDateTime part_key = Qry.get().FieldIsNULL(col_part_key) ? NoExists : Qry.get().FieldAsDateTime(col_part_key);
                row.route = GetRouteAfterStr(part_key, row.point_id, trtWithCurrent, trtNotCancelled);


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
                PFSStat.add(row);

                if ((not full) and (PFSStat.RowCount() > (size_t)MAX_STAT_ROWS())) {
                    throw MaxStatRowsException("MSG.TOO_MANY_ROWS_SELECTED.RANDOM_SHOWN_NUM.ADJUST_STAT_SEARCH", LParams() << LParam("num", MAX_STAT_ROWS()));
                }
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
    colNode = NewTextChild(headerNode, "col", getLocaleText("���"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("����"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�������"));
    SetProp(colNode, "width", 100);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�����"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("����"));
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
    colNode = NewTextChild(headerNode, "col", getLocaleText("�������"));
    SetProp(colNode, "width", 80);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("���"));
    SetProp(colNode, "width", 80);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("���"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��������"));
    SetProp(colNode, "width", 85);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;
    for(TPFSStat::iterator i = PFSStat.begin(); i != PFSStat.end(); i++) {
        rowNode = NewTextChild(rowsNode, "row");
        // ���
        NewTextChild(rowNode, "col", DateTimeToStr(i->scd_out, "dd.mm.yyyy"));
        // ����
        NewTextChild(rowNode, "col", i->flt);
        // �������
        NewTextChild(rowNode, "col", i->route);
        // �����
        NewTextChild(rowNode, "col", i->status);
        // ���-�� ����
        NewTextChild(rowNode, "col", i->seats);
        // RBD
        NewTextChild(rowNode, "col", i->subcls);
        // PNR
        NewTextChild(rowNode, "col", i->pnr);
        // �������
        NewTextChild(rowNode, "col", i->surname);
        // ���
        NewTextChild(rowNode, "col", i->name);
        // ���
        NewTextChild(rowNode, "col", i->gender);
        // ��� ஦�����
        if(i->birth_date == NoExists)
            NewTextChild(rowNode, "col");
        else
            NewTextChild(rowNode, "col", DateTimeToStr(i->birth_date, "dd.mm.yyyy"));
    }

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", "PFS");
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("���஡���"));
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
    colNode = NewTextChild(headerNode, "col", getLocaleText("���"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("����"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�������"));
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
    colNode = NewTextChild(headerNode, "col", getLocaleText("OFFLK"));
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
                // ���
                NewTextChild(rowNode, "col", DateTimeToStr(stat->first, "dd.mm.yyyy"));
                // ����
                NewTextChild(rowNode, "col", flt->first);
                // �������
                NewTextChild(rowNode, "col", route->first);
                // NOREC
                NewTextChild(rowNode, "col", route->second["NOREC"]);
                // NOSHO
                NewTextChild(rowNode, "col", route->second["NOSHO"]);
                // GOSHO
                NewTextChild(rowNode, "col", route->second["GOSHO"]);
                // OFFLK
                NewTextChild(rowNode, "col", route->second["OFFLK"]);
            }


    /*
    for(TPFSShortStat::iterator i = PFSShortStat.begin(); i != PFSShortStat.end(); i++) {
        rowNode = NewTextChild(rowsNode, "row");
        // ���
        NewTextChild(rowNode, "col", DateTimeToStr(i->scd_out, "dd.mm.yyyy"));
        // ����
        NewTextChild(rowNode, "col", i->flt);
        // �������
        NewTextChild(rowNode, "col", i->route);
        // �����
        NewTextChild(rowNode, "col", i->status);
        // ���-�� ����
        NewTextChild(rowNode, "col", i->seats);
        // RBD
        NewTextChild(rowNode, "col", i->subcls);
        // PNR
        NewTextChild(rowNode, "col", i->pnr);
        // �������
        NewTextChild(rowNode, "col", i->surname);
        // ���
        NewTextChild(rowNode, "col", i->name);
        // ���
        NewTextChild(rowNode, "col", i->gender);
        // ��� ஦�����
        if(i->birth_date == NoExists)
            NewTextChild(rowNode, "col");
        else
            NewTextChild(rowNode, "col", DateTimeToStr(i->birth_date, "dd.mm.yyyy"));
    }
    */

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", "PFS");
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("����"));
}

struct TPFSShortStatCombo : public TOrderStatItem
{
    TDateTime scd_out;
    string flt;
    string route;
    int norec;
    int nosho;
    int gosho;
    int offlk;

    TPFSShortStatCombo(
            TDateTime _scd_out,
            const string &_flt,
            const string &_route,
            int _norec,
            int _nosho,
            int _gosho,
            int _offlk):
        scd_out(_scd_out),
        flt(_flt),
        route(_route),
        norec(_norec),
        nosho(_nosho),
        gosho(_gosho),
        offlk(_offlk)
    {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

struct TPFSFullStatCombo : public TOrderStatItem
{
    const TPFSStatRow &row;
    TPFSFullStatCombo(const TPFSStatRow &_row): row(_row) {}
    void add_header(ostringstream &buf) const;
    void add_data(ostringstream &buf) const;
};

void TPFSShortStatCombo::add_data(ostringstream &buf) const
{
    buf << DateTimeToStr(scd_out, "dd.mm.yyyy") << delim;
    buf << flt << delim;
    buf << route << delim;
    buf << norec << delim;
    buf << nosho << delim;
    buf << gosho << delim;
    buf << offlk << endl;
}

void TPFSShortStatCombo::add_header(ostringstream &buf) const
{
    buf
        << getLocaleText("���") << delim
        << getLocaleText("����") << delim
        << getLocaleText("�������") << delim
        << "NOREC" << delim
        << "NOSHO" << delim
        << "GOSHO" << delim
        << "OFFLK" << endl;
}

void TPFSFullStatCombo::add_data(ostringstream &buf) const
{
        // ���
        buf << DateTimeToStr(row.scd_out, "dd.mm.yyyy") << delim;
        // ����
        buf << row.flt << delim;
        // �������
        buf << row.route << delim;
        // �����
        buf << row.status << delim;
        // ���-�� ����
        buf << row.seats << delim;
        // RBD
        buf << row.subcls << delim;
        // PNR
        buf << row.pnr << delim;
        // �������
        buf << row.surname << delim;
        // ���
        buf << row.name << delim;
        // ���
        buf << row.gender << delim;
        // ��� ஦�����
        buf << (row.birth_date == NoExists ? "" : DateTimeToStr(row.birth_date, "dd.mm.yyyy"));
        buf << endl;
}

void TPFSFullStatCombo::add_header(ostringstream &buf) const
{
    buf
        << getLocaleText("���") << delim
        << getLocaleText("����") << delim
        << getLocaleText("�������") << delim
        << getLocaleText("�����") << delim
        << getLocaleText("����") << delim
        << "RBD" << delim
        << "PNR" << delim
        << getLocaleText("�������") << delim
        << getLocaleText("���") << delim
        << getLocaleText("���") << delim
        << getLocaleText("��������")
        << endl;
}

template <class T>
void RunPFSShortFile(const TStatParams &params, T &writer, TPrintAirline &prn_airline)
{
    TPFSShortStat PFSShortStat;
    RunPFSStat(params, PFSShortStat, prn_airline, true);
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
                writer.insert(TPFSShortStatCombo(
                            stat->first,
                            flt->first,
                            route->first,
                            route->second["NOREC"],
                            route->second["NOSHO"],
                            route->second["GOSHO"],
                            route->second["OFFLK"]
                            ));
            }
}

template <class T>
void RunPFSFullFile(const TStatParams &params, T &writer, TPrintAirline &prn_airline)
{
    TPFSStat PFSStat;
    RunPFSStat(params, PFSStat, prn_airline, true);
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
        case statRem:
            RunRemStat(params, order_writer, airline);
            break;
        case statUnaccBag:
            RunUNACCFullFile(params, order_writer);
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
        case statPFSShort:
            RunPFSShortFile(params, order_writer, airline);
            break;
        case statBIShort:
            RunBIShortFile(params, order_writer);
            break;
        case statBIDetail:
            RunBIDetailFile(params, order_writer);
            break;
        case statBIFull:
            RunBIFullFile(params, order_writer);
            break;
        case statVOShort:
            RunVOShortFile(params, order_writer);
            break;
        case statVOFull:
            RunVOFullFile(params, order_writer);
            break;
        case statADFull:
            RunADFullFile(params, order_writer);
            break;
        case statHAShort:
            RunHAShortFile(params, order_writer);
            break;
        case statHAFull:
            RunHAFullFile(params, order_writer);
            break;
        case statReprintShort:
            RunReprintShortFile(params, order_writer);
            break;
        case statReprintFull:
            RunReprintFullFile(params, order_writer);
            break;
        case statServicesShort:
            RunServicesShortFile(params, order_writer);
            break;
        case statServicesDetail:
            RunServicesDetailFile(params, order_writer);
            break;
        case statServicesFull:
            RunServicesFullFile(params, order_writer);
            break;
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

        // �� client_type = ctHTTP �㤥� ��।�����, �� ����⨪� �ନ����� �� ������
        // � ��⭮�� � ����⨪� ����ॣ������
        TReqInfo::Instance()->client_type = ctHTTP;

        TPeriods periods;
        periods.get(params.FirstDate, params.LastDate);

        int parts = 0;
        double data_size = 0;
        double data_size_zip = 0;
        TPeriods::TItems::iterator i = periods.items.begin();

        // ��������, � ���� 㦥 ���� ����� ���� (so_data �� ���⮩)
        // ⮣�� 横� ���� ��稭��� � ��᫥����� ᮡ࠭���� ��᪠
        // ���� ������, ��६��뢠�� �� ⥪�饥 ���ﭨ� ᡮન.
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
                // �⬠�뢠�� ��ਮ��
                // 1. � so_data ����� ���� �ய�᪨ � ������, �᫨ � ��� �� �뫮 ������
                // ���⮬� ���⮩ i++ �� ��������
                // 2. ��� ��ਮ�� �� ��易�� ���� ���� ���� �����, � � �६� ���
                // ����� ���� �ᥣ�� ���� ���� �����
                while(true) {
                    if(DateTimeToStr(i->first, "mm.yy") == DateTimeToStr(i_data->month, "mm.yy"))
                        break;
                    i++;
                    parts++;
                }
            }
            // ��᫥ ��६�⪨ ����� ��ਮ�� �⮨� �� ��᫥���� �ᯥ譮� �����
            // ���� ��� ��।������ �� ����, �� �� ᮡ࠭�� �����
            // ��� � ���稪 parts, �⮡� �ண��� �⮡ࠦ���� �ࠢ��쭮
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
                if (isIgnoredEOracleError(E)) continue;
                ProgError(STDLOG,"Exception: %s (file id=%d)",E.what(),item->id);
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
    string tags;
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
        << getLocaleText("��") << delim
        << getLocaleText("���") << delim
        << getLocaleText("���.1") << delim
        << getLocaleText("���") << delim
        << getLocaleText("���") << delim
        << getLocaleText("���.2") << delim
        << getLocaleText("���") << delim
        << getLocaleText("���") << delim
        << getLocaleText("��⥣���") << delim
        << getLocaleText("��� ���ᠦ��") << delim
        << getLocaleText("���㬥��") << delim
        << getLocaleText("CAP.DOC.PAX") << delim
        << getLocaleText("��") << delim
        << getLocaleText("��") << delim
        << getLocaleText("��") << delim
        << getLocaleText("�/�") << delim
        << getLocaleText("�� ����") << delim
        << getLocaleText("�� ���") << delim
        << getLocaleText("��ન") << endl;
}

void TTrferPaxStatItem::add_data(ostringstream &buf) const
{
    if(airline.empty()) {
        buf
            << getLocaleText("�⮣�:") << delim
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
        // ��
        buf << ElemIdToCodeNative(etAirline, airline) << delim;
        // ��
        buf << ElemIdToCodeNative(etAirp, airp) << delim;
        //���.1
        ostringstream tmp;
        tmp << setw(3) << setfill('0') << flt_no1 << ElemIdToCodeNative(etSuffix, suffix1);
        buf << tmp.str() << delim;
        //���
        buf << DateTimeToStr(date1, "dd.mm.yyyy") << delim;
        //�࠭���
        buf << ElemIdToCodeNative(etAirp, trfer_airp) << delim;
        //���.2
        tmp.str("");
        tmp
            << ElemIdToCodeNative(etAirline, airline2)
            << setw(3) << setfill('0') << flt_no2 << ElemIdToCodeNative(etSuffix, suffix2);
        buf << tmp.str() << delim;
        //���
        buf << DateTimeToStr(date2, "dd.mm.yyyy") << delim;
        //�/� �ਫ��
        buf << ElemIdToCodeNative(etAirp, airp_arv) << delim;
        //��⥣���
        buf << getLocaleText(TSegCategory().encode(seg_category)) << delim;
        //���
        buf << pax_name << delim;
        //��ᯮ��
        buf << pax_doc << delim;
    }
    //���-�� ����.
    buf << pax_amount << delim;
    //��
    buf << adult << delim;
    //��
    buf << child << delim;
    //��
    buf << baby << delim;
    //�/�����(���)
    buf << rk_weight << delim;
    //�� ����
    buf << bag_amount << delim;
    //�� ���
    buf << bag_weight << delim;
    //��ન
    buf << tags << endl;
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
        << QParam("LastDate", otDate, params.LastDate)
        << QParam("pr_lat", otInteger, TReqInfo::Instance()->desk.lang!=AstraLocale::LANG_RU);
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
            "   pax.pers_type, ";
        if (pass!=0)
            SQLText +=
                " arch.get_birks2(pax.part_key,pax.grp_id,pax.pax_id,pax.bag_pool_num,:pr_lat) tags ";
        else
            SQLText +=
                " ckin.get_birks2(pax.grp_id,pax.pax_id,pax.bag_pool_num,:pr_lat) tags ";
        SQLText +=
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
            int col_pax_id = Qry.get().FieldIndex("pax_id");
            int col_rk_weight = Qry.get().FieldIndex("rk_weight");
            int col_bag_weight = Qry.get().FieldIndex("bag_weight");
            int col_bag_amount = Qry.get().FieldIndex("bag_amount");
            int col_segments = Qry.get().FieldIndex("segments");
            int col_full_name = Qry.get().FieldIndex("full_name");
            int col_pers_type = Qry.get().FieldIndex("pers_type");
            int col_tags = Qry.get().FieldIndex("tags");
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
                string tags = Qry.get().FieldAsString(col_tags);

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

                        static const TCategoryMap category_map =
                        {
                            {
                                false,
                                {
                                    {false, TSegCategories::IntInt},
                                    {true, TSegCategories::IntFor}
                                }
                            },
                            {
                                true,
                                {
                                    {false, TSegCategories::ForInt},
                                    {true, TSegCategories::ForFor}
                                }
                            }
                        };

                        string country1 = get_airp_country(item.airp);
                        string country2 = get_airp_country(item.trfer_airp);
                        string country3 = get_airp_country(item.airp_arv);

                        bool is_inter1 = country1 != country2;
                        bool is_inter2 = country2 != country3;
                        item.seg_category = category_map.at(is_inter1).at(is_inter2);

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
                    tmp_stat.begin()->adult = pers_type == "��";
                    tmp_stat.begin()->child = pers_type == "��";
                    tmp_stat.begin()->baby = pers_type == "��";
                    tmp_stat.begin()->rk_weight = rk_weight;
                    tmp_stat.begin()->bag_amount = bag_amount;
                    tmp_stat.begin()->bag_weight = bag_weight;
                    tmp_stat.begin()->tags = tags;

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
    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("���"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("���.1"));
    SetProp(colNode, "width", 50);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("���"));
    SetProp(colNode, "width", 55);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("���"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("���.2"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("���"));
    SetProp(colNode, "width", 55);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("���"));
    SetProp(colNode, "width", 30);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��⥣���"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��� ���ᠦ��"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("���㬥��"));
    SetProp(colNode, "width", 70);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("CAP.DOC.PAX"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�/�"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�� ����"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�� ���"));
    SetProp(colNode, "width", 60);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��ન"));
    SetProp(colNode, "width", 90);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);

    xmlNodePtr rowsNode = NewTextChild(grdNode, "rows");
    xmlNodePtr rowNode;

    for(TTrferPaxStat::iterator stat = TrferPaxStat.begin();
            stat != TrferPaxStat.end(); stat++) {
        rowNode = NewTextChild(rowsNode, "row");
        if(stat->airline.empty()) {
            NewTextChild(rowNode, "col", getLocaleText("�⮣�:"));
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
            // ��
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirline, stat->airline));
            // ��
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, stat->airp));
            //���.1
            ostringstream buf;
            buf << setw(3) << setfill('0') << stat->flt_no1 << ElemIdToCodeNative(etSuffix, stat->suffix1);
            NewTextChild(rowNode, "col", buf.str());
            //���
            NewTextChild(rowNode, "col", DateTimeToStr(stat->date1, "dd.mm.yy"));
            //�࠭���
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, stat->trfer_airp));
            //���.2
            buf.str("");
            buf
                << ElemIdToCodeNative(etAirline, stat->airline2)
                << setw(3) << setfill('0') << stat->flt_no2 << ElemIdToCodeNative(etSuffix, stat->suffix2);
            NewTextChild(rowNode, "col", buf.str());
            //���
            NewTextChild(rowNode, "col", DateTimeToStr(stat->date2, "dd.mm.yy"));
            //�/� �ਫ��
            NewTextChild(rowNode, "col", ElemIdToCodeNative(etAirp, stat->airp_arv));
            //��⥣���
            NewTextChild(rowNode, "col", getLocaleText(TSegCategory().encode(stat->seg_category)));
            //���
            NewTextChild(rowNode, "col", stat->pax_name);
            //��ᯮ��
            NewTextChild(rowNode, "col", stat->pax_doc);
        }
        //���-�� ����.
        NewTextChild(rowNode, "col", stat->pax_amount);
        //��
        NewTextChild(rowNode, "col", stat->adult);
        //��
        NewTextChild(rowNode, "col", stat->child);
        //��
        NewTextChild(rowNode, "col", stat->baby);
        //�/�����(���)
        NewTextChild(rowNode, "col", stat->rk_weight);
        //�� ����
        NewTextChild(rowNode, "col", stat->bag_amount);
        //�� ���
        NewTextChild(rowNode, "col", stat->bag_weight);
        //��ન
        NewTextChild(rowNode, "col", stat->tags);
    }

    xmlNodePtr variablesNode = STAT::set_variables(resNode);
    NewTextChild(variablesNode, "stat_type", params.statType);
    NewTextChild(variablesNode, "stat_mode", getLocaleText("�࠭���"));
    NewTextChild(variablesNode, "stat_type_caption", getLocaleText("���஡���"));
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
                or params.statType == statRem
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
        case statRem:
            get_compatible_report_form("RemStat", reqNode, resNode);
            break;
        case statHAFull:
        case statHAShort:
        case statVOFull:
        case statVOShort:
        case statADFull:
        case statReprintShort:
        case statReprintFull:
        case statBIFull:
        case statBIShort:
        case statBIDetail:
        case statServicesFull:
        case statServicesShort:
        case statServicesDetail:
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
        if(params.statType == statRem)
        {
            TPrintAirline airline;
            TRemStat RemStat;
            RunRemStat(params, RemStat, airline);
            createXMLRemStat(params,RemStat, airline, resNode);
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
            TUNACCFullStat UNACCFullStat;
            RunUNACCStat(params, UNACCFullStat);
            createXMLUNACCFullStat(params, UNACCFullStat, resNode);
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
            RunPFSStat(params, PFSShortStat, airline);
            createXMLPFSShortStat(params, PFSShortStat, airline, resNode);
        }
        if(params.statType == statTrferPax)
        {
            TPrintAirline airline;
            TTrferPaxStat TrferPaxStat;
            RunTrferPaxStat(params, TrferPaxStat, airline);
            createXMLTrferPaxStat(params, TrferPaxStat, airline, resNode);
        }
        if(params.statType == statBIShort)
        {
            TBIShortStat BIShortStat;
            RunBIStat(params, BIShortStat);
            createXMLBIShortStat(params, BIShortStat, resNode);
        }
        if(params.statType == statBIDetail)
        {
            TBIDetailStat BIDetailStat;
            RunBIStat(params, BIDetailStat);
            createXMLBIDetailStat(params, BIDetailStat, resNode);
        }
        if(params.statType == statBIFull)
        {
            TBIFullStat BIFullStat;
            RunBIStat(params, BIFullStat);
            createXMLBIFullStat(params, BIFullStat, resNode);
        }
        if(params.statType == statServicesShort)
        {
            TServicesShortStat ServicesShortStat;
            RunServicesStat(params, ServicesShortStat);
            createXMLServicesShortStat(params, ServicesShortStat, resNode);
        }
        if(params.statType == statServicesFull)
        {
            TServicesFullStat ServicesFullStat;
            RunServicesStat(params, ServicesFullStat);
            createXMLServicesFullStat(params, ServicesFullStat, resNode);
        }
        if(params.statType == statServicesDetail)
        {
            TServicesDetailStat ServicesDetailStat;
            RunServicesStat(params, ServicesDetailStat);
            createXMLServicesDetailStat(params, ServicesDetailStat, resNode);
        }
        if(params.statType == statReprintShort)
        {
            TReprintShortStat ReprintShortStat;
            RunReprintStat(params, ReprintShortStat);
            createXMLReprintShortStat(params, ReprintShortStat, resNode);
        }
        if(params.statType == statReprintFull)
        {
            TReprintFullStat ReprintFullStat;
            RunReprintStat(params, ReprintFullStat);
            createXMLReprintFullStat(params, ReprintFullStat, resNode);
        }
        if(params.statType == statHAShort)
        {
            THAShortStat HAShortStat;
            RunHAStat(params, HAShortStat);
            createXMLHAShortStat(params, HAShortStat, resNode);
        }
        if(params.statType == statHAFull)
        {
            THAFullStat HAFullStat;
            RunHAStat(params, HAFullStat);
            createXMLHAFullStat(params, HAFullStat, resNode);
        }
        if(params.statType == statVOShort)
        {
            TVOShortStat VOShortStat;
            RunVOStat(params, VOShortStat);
            createXMLVOShortStat(params, VOShortStat, resNode);
        }
        if(params.statType == statVOFull)
        {
            TVOFullStat VOFullStat;
            RunVOStat(params, VOFullStat);
            createXMLVOFullStat(params, VOFullStat, resNode);
        }
        if(params.statType == statADFull)
        {
            TADFullStat ADFullStat;
            RunADStat(params, ADFullStat);
            createXMLADFullStat(params, ADFullStat, resNode);
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
        if(time_created != NoExists) { // �᫨ ����� �� �� ��ନ஢��, size �������⥭ (NoExists)
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
    colNode = NewTextChild(headerNode, "col", getLocaleText("����"));
    SetProp(colNode, "width", 150);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�६� ������"));
    SetProp(colNode, "width", 110);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDate);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�६� �믮������"));
    SetProp(colNode, "width", 110);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDate);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�६� 㤠�����"));
    SetProp(colNode, "width", 110);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDate);
    colNode = NewTextChild(headerNode, "col", getLocaleText("�����"));
    SetProp(colNode, "width", 110);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortString);
    colNode = NewTextChild(headerNode, "col", getLocaleText("��⮢�����"));
    SetProp(colNode, "width", 110);
    SetProp(colNode, "align", TAlignment::LeftJustify);
    SetProp(colNode, "sort", sortDate);
    colNode = NewTextChild(headerNode, "col", getLocaleText("������"));
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

        // ����
        NewTextChild(rowNode, "col", curr_order.name);

        // �६� ������
        NewTextChild(rowNode, "col", DateTimeToStr(curr_order.time_ordered));

        // �६� �믮������ � 㤠�����
        if(curr_order.time_created == NoExists) {
            NewTextChild(rowNode, "col");
            NewTextChild(rowNode, "col");
        } else {
            NewTextChild(rowNode, "col", DateTimeToStr(curr_order.time_created));
            NewTextChild(rowNode, "col", DateTimeToStr(curr_order.time_created + ORDERS_TIMEOUT()));
        }

        // �����
        NewTextChild(rowNode, "col", getLocaleText(EncodeOrderStatus(curr_order.status)));

        // ��⮢�����
        NewTextChild(rowNode, "col", IntToString(curr_order.progress) + "%");

        // ������
        if(curr_order.data_size == NoExists)
            NewTextChild(rowNode, "col");
        else
            NewTextChild(rowNode, "col", getFileSizeStr(curr_order.data_size));

        total_size += (curr_order.data_size == NoExists ? 0 : curr_order.data_size);
    }
    rowNode = NewTextChild(rowsNode, "row");
    NewTextChild(rowNode, "col", getLocaleText("�⮣�:"));
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
    if(file_id != NoExists) { // �� ����襭 ஢�� ���� ����, ��� ���� �஢�ઠ 楫��⭮��
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
        sql << TTripInfo::selectedFields("points") << ", \n"
               " pax.reg_no, \n"
               " pax_grp.airp_arv, \n"
               " pax.surname||' '||pax.name full_name, \n";
        if (pass==0)
          sql << " NVL(ckin.get_bagAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) bag_amount, \n"
                 " NVL(ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) bag_weight, \n"
                 " NVL(ckin.get_rkWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) rk_weight, \n"
                 " ckin.get_excess_wt(pax.grp_id, pax.pax_id, pax_grp.excess_wt, pax_grp.bag_refuse) AS excess_wt, "
                 " ckin.get_excess_pc(pax.grp_id, pax.pax_id) AS excess_pc, "
                 " ckin.get_birks2(pax.grp_id,pax.pax_id,pax.bag_pool_num,:pr_lat) tags, \n"
                 " salons.get_seat_no(pax.pax_id, pax.seats, pax.is_jmp, pax_grp.status, pax_grp.point_dep, 'seats', rownum) seat_no, \n";
        else
          sql << " NVL(arch.get_bagAmount2(pax.part_key,pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) bag_amount, \n"
                 " NVL(arch.get_bagWeight2(pax.part_key,pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) bag_weight, \n"
                 " NVL(arch.get_rkWeight2(pax.part_key,pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) rk_weight, \n"
                 " arch.get_excess_wt(pax.part_key, pax.grp_id, pax.pax_id, pax_grp.excess_wt, pax_grp.excess, pax_grp.bag_refuse) AS excess_wt, "
                 " pax.excess_pc, "
                 " arch.get_birks2(pax.part_key,pax.grp_id,pax.pax_id,pax.bag_pool_num,:pr_lat) tags, \n"
                 " LPAD(seat_no,3,'0')|| \n"
                 "      DECODE(SIGN(1-seats),-1,'+'||TO_CHAR(seats-1),'') seat_no, \n";
        sql << " pax_grp.grp_id, \n"
               " pax.pr_brd, \n"
               " pax.refuse, \n"
               " pax_grp.class_grp, \n"
               " NVL(pax.cabin_class_grp, pax_grp.class_grp) AS cabin_class_grp, \n"
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
        TComplexBagExcessNodeList excessNodeList(OutputLang(), {});
        PaxListToXML(Qry, resNode, excessNodeList, true, pass, count);

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

            if(idx->second != "���") continue;
            result[Qry.FieldAsString(col_airline)][Qry.FieldAsString(col_client_type)]++;
        }
    }
    ofstream out("self_ckin.csv");
    for(map<string, map<string, int> >::iterator i = result.begin(); i != result.end(); i++) {
        for(map<string, int>::iterator j = i->second.begin(); j != i->second.end(); j++) {
            out
                << i->first << ","
                << j->first << ","
                << "���,"
                << j->second << endl;
        }
    }
    cout << "end time: " << DateTimeToStr(NowUTC(), ServerFormatDateTimeAsString) << endl;
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

void add_stat_time(map<string, long> &stat_times, const string &name, long time)
{
    map<string, long>::iterator i = stat_times.find(name);
    if(i == stat_times.end())
        stat_times[name] = time;
    else
        stat_times[name] += time;
}

void get_full_stat(TDateTime utcdate)
{
    //ᮡ�६ ����⨪� �� ���祭�� ���� ���� �� �뫥�,
    //�᫨ �� ���⠢��� �ਧ��� �����⥫쭮�� ᡮ� ����⨪� pr_stat

    TQuery PointsQry(&OraSession);
  PointsQry.Clear();
  PointsQry.SQLText =
    "SELECT points.point_id FROM points,trip_sets "
    "WHERE points.point_id=trip_sets.point_id AND "
    "      points.pr_del=0 AND points.pr_reg<>0 AND trip_sets.pr_stat=0 AND "
    "      time_out<:stat_date AND time_out>TO_DATE('01.01.0001','DD.MM.YYYY')";
  PointsQry.CreateVariable("stat_date",otDate,utcdate-2); //2 ���
  PointsQry.Execute();
  map<string, long> stat_times;
  for(;!PointsQry.Eof;PointsQry.Next())
  {
    get_flight_stat(stat_times, PointsQry.FieldAsInteger("point_id"), true);
    OraSession.Commit();
  };
  for(map<string, long>::iterator i = stat_times.begin(); i != stat_times.end(); i++)
      LogTrace(TRACE5) << i->first << ": " << i->second;
};


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
     TPerfTimer tm;
     tm.Init();
     Qry.get().Execute();
     add_stat_time(stat_times, "statist", tm.Print());
     tm.Init();
     get_rfisc_stat(point_id);
     add_stat_time(stat_times, "rfisc_stat", tm.Print());
     tm.Init();
     get_rem_stat(point_id);
     add_stat_time(stat_times, "rem_stat", tm.Print());
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
     add_stat_time(stat_times, "trfer_pax_stat", tm.Print());
     tm.Init();
     get_stat_vo(point_id);
     add_stat_time(stat_times, "stat_vo", tm.Print());
     tm.Init();
     get_stat_ha(point_id);
     add_stat_time(stat_times, "stat_ha", tm.Print());
     tm.Init();
     get_stat_ad(point_id);
     add_stat_time(stat_times, "stat_ad", tm.Print());
     tm.Init();
     get_stat_reprint(point_id);
     add_stat_time(stat_times, "stat_reprint", tm.Print());
     tm.Init();
     get_stat_services(point_id);
     add_stat_time(stat_times, "stat_services", tm.Print());
   };

   TReqInfo::Instance()->LocaleToLog("EVT.COLLECT_STATISTIC", evtFlt, point_id);
};

void collectStatTask(const TTripTaskKey &task)
{
  LogTrace(TRACE5) << __FUNCTION__ << ": " << task;

  time_t time_start=time(NULL);
  try
  {
    map<string, long> stat_times;
    get_flight_stat(stat_times, task.point_id, false);
  }
  catch(const EOracleError &E) {
    ProgError(STDLOG,"EOracleError %d: %s",E.Code,E.what());
    ProgError(STDLOG,"SQL: %s",E.SQLText());
  }
  catch(std::exception &E)
  {
    ProgError(STDLOG,"%s (point_id=%d): %s", __FUNCTION__, task.point_id, E.what());
  };
  time_t time_end=time(NULL);
  if (time_end-time_start>1)
    ProgTrace(TRACE5,"Attention! %s execute time: %ld secs, point_id=%d",
                     __FUNCTION__, time_end-time_start, task.point_id);
}

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
            << QParam("airp", otString, "���");
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
                if(getCountry(point_dep, part_key, dep_scd_in, dep_scd_out, dep_airp) != "��") continue;
                if(getCountry(point_arv, part_key, arv_scd_in, arv_scd_out, arv_airp) != "��") continue;
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
    return 1; // 0 - ��������� ����������, 1 - rollback
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

    string delim = ";";

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
        if(part_key == NoExists)
          pnr_addr=TPnrAddrs().firstAddrByPaxId(paxQry.FieldAsInteger("pax_id"), TPnrAddrInfo::AddrAndAirline);

        CheckIn::TPaxDocItem doc;
        LoadPaxDoc(part_key, paxQry.FieldAsInteger("pax_id"), doc);
        string birth_date = (doc.birth_date!=ASTRA::NoExists?DateTimeToStr(doc.birth_date, "dd.mmm.yy"):"");
        string gender = doc.gender;

        of
            // ���
            << name << (name.empty() ? "" : " ") << surname << delim
            // ��� ஦�����
            << birth_date << delim
            // ���
            << gender << delim
            // ��� ���㬥��
            << doc.type << delim
            // ���� � ����� ���㬥��
            << doc.no << delim
            // ����� �����
            << ticket.str() << delim
            // ����� �஭�஢����
            << pnr_addr << delim
            // ����
            << flt_str.str() << delim
            // ��� �뫥�
            << DateTimeToStr(scd_out, "ddmmm") << delim
            // ��
            << airp << delim
            // ��
            << airp_arv_info.get(paxQry) << delim
            // ����� ����
            << paxQry.FieldAsInteger("bag_amount") << delim
            // ����� ���
            << paxQry.FieldAsInteger("bag_weight") << delim;
        // �६� ॣ����樨
        TRegEvents::const_iterator evt = events.find(make_pair(grp_id, reg_no));
        if(evt != events.end())
            of << DateTimeToStr(evt->second.first, "dd.mm.yyyy hh:nn:ss");
        of << delim;
        // ����� ����
        if(part_key == NoExists)
            of << paxQry.FieldAsString("seat_no");
        of << delim
            // ���ᮡ ॣ����樨
            << paxQry.FieldAsString("client_type") << delim;
        // ����� �� �� �⮩��
        if(part_key == NoExists)
            of << (paxQry.FieldAsInteger("term_bp") == 0 ? "���" : "��");
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
            "   scd_out>=:first_date AND scd_out<:last_date AND airline='��' AND "
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
            << "���" << delim
            << "��� ஦�����" << delim
            << "���" << delim
            << "��� ���㬥��" << delim
            << "���� � ����� ���㬥��" << delim
            << "����� �����" << delim
            << "����� �஭�஢����" << delim
            << "����" << delim
            << "��� �뫥�" << delim
            << "��" << delim
            << "��" << delim
            << "����� ����" << delim
            << "����� ���" << delim
            << "�६� ॣ����樨 (UTC)" << delim
            << "����� ����" << delim
            << "���ᮡ ॣ����樨" << delim
            << "����� �� �� �⮩��" << endl;
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
    "WHERE scd_out>=:FirstDate AND scd_out<:LastDate AND airline='��' AND "
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
                    << "���" << delim
                    << "����� �����" << delim
                    << "����� �஭�஢����" << delim
                    << "����" << delim
                    << "��� �뫥�" << delim
                    << "���ࠢ�����" << delim
                    << "��� ஦�����" << delim
                    << "���" << endl;
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

            string pnr_addr=TPnrAddrs().firstAddrByPnrId(paxQry.FieldAsInteger("pnr_id"), TPnrAddrInfo::AddrAndAirline);

            CheckIn::TPaxDocItem doc;
            LoadPaxDoc(paxQry.FieldAsInteger("pax_id"), doc);
            string birth_date = (doc.birth_date!=ASTRA::NoExists?DateTimeToStr(doc.birth_date, "dd.mmm.yy"):"");
            string gender = doc.gender;

            of
                // ���
                << name << (name.empty() ? "" : " ") << surname << delim
                // ��� ஦�����
                << birth_date << delim
                // ���
                << gender << delim
                // ��� ���㬥��
                << delim
                // ���� ���㬥��
                << delim
                // ����� ���㬥��
                << delim
                // ����� �����
                << ticket.str() << delim
                // ����� �஭�஢����
                << pnr_addr << delim
                // ����
                << flt_str.str() << delim
                // ��� �뫥�
                << DateTimeToStr(scd_out, "ddmmm") << delim
                // ��
                << delim
                // ��
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
                    //��� ��ய��� (��த�)
                    << Qry.FieldAsString(col_airp) << delim
                    //��ॢ��稪
                    << Qry.FieldAsString(col_airline) << delim
                    //����� ३�
                    << Qry.FieldAsString(col_flt_no) << delim
                    //����
                    << Qry.FieldAsString(col_suffix) << delim
                    //��� ३�
                    << DateTimeToStr(Qry.FieldAsDateTime(col_scd_out), "dd.mm.yyyy") << delim
                    //���ᠦ��� ��� � ॣ����樥� � �/�
                    <<
                    data[point_id][true][false][false] +
                    data[point_id][true][false][true]
                    << delim
                    //���ᠦ��� �� � ॣ����樥� � �/�
                    <<
                    data[point_id][false][false][false] +
                    data[point_id][false][false][true]
                    << delim
                    //���ᠦ��� ��� � ॣ����樥� � �/� ��� ������
                    <<
                    data[point_id][true][false][false]
                    << delim
                    //���ᠦ��� �� � ॣ����樥� � �/� ��� ������
                    <<
                    data[point_id][false][false][false]
                    << delim
                    //���ᠦ��� ��� � ᠬ�ॣ����樥� � �������
                    <<
                    data[point_id][true][true][true]
                    << delim
                    //���ᠦ��� �� � ᠬ�ॣ����樥� � �������
                    <<
                    data[point_id][false][true][true]
                    << delim
                    //���ᠦ��� ��� � ᠬ�ॣ����樥� ��� ������
                    <<
                    data[point_id][true][true][false]
                    << delim
                    //���ᠦ��� �� � ᠬ�ॣ����樥� ��� ������
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
            "   scd_out>=:first_date AND scd_out<:last_date AND airline='��' AND "
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
            << "��� ��ய��� (��த�)" << delim
            << "��ॢ��稪" << delim
            << "����� ३�" << delim
            << "����" << delim
            << "��� ३�" << delim
            << "���ᠦ��� ��� � ॣ����樥� � �/�" << delim
            << "���ᠦ��� �� � ॣ����樥� � �/�" << delim
            << "���ᠦ��� ��� � ॣ����樥� � �/� ��� ������" << delim
            << "���ᠦ��� �� � ॣ����樥� � �/� ��� ������" << delim
            << "���ᠦ��� ��� � ᠬ�ॣ����樥� � �������" << delim
            << "���ᠦ��� �� � ᠬ�ॣ����樥� � �������" << delim
            << "���ᠦ��� ��� � ᠬ�ॣ����樥� ��� ������" << delim
            << "���ᠦ��� �� � ᠬ�ॣ����樥� ��� ������" << endl;
        TFltStat flt_stat(Qry.get(), delim);
        for(; not Qry.get().Eof; Qry.get().Next())
            flt_stat.get(Qry.get(), of);
    }

    return 1;
}

