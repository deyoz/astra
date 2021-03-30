#include "stat_main.h"
#include "xml_unit.h"
#include "qrys.h"
#include "astra_utils.h"
#include "docs/docs_common.h"
#include "stat_utils.h"
#include "astra_date_time.h"
#include "db_tquery.h"
#include "arx_daily_pg.h"
#include "exch_checkin_result.h"
#include "PgOraConfig.h"
#include "jms/jms.hpp"

#define NICKNAME "DENIS"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/slogger.h"



using namespace std;
using namespace AstraLocale;
using namespace EXCEPTIONS;
using namespace ASTRA;
using namespace ASTRA::date_time;

const string SYSTEM_USER = "���⥬�";

typedef struct {
    string trip, scd_out;
} TTripItem;

std::string get_seat_no(int seats, std::string seat_no)
{
    std::string res = StrUtils::lpad(seat_no,3,'0');
    if(seats >= 2) {
        res += std::to_string(seats-1);
    }
    return res;
}

void PaxListToXML(DB::TQuery &Qry, xmlNodePtr resNode, TComplexBagExcessNodeList& excessNodeList,
                  bool isPaxSearch, int pass, int &count, bool isArch)
{
  LogTrace5 << __func__ << " isArch: " << isArch << " isPaxSearch: " << isPaxSearch;
  if(Qry.Eof) {
      LogTrace5 << __func__ << " NO Data found";
      return;
  }

  xmlNodePtr paxListNode = GetNode("paxList", resNode);
  if (paxListNode==NULL)
    paxListNode = NewTextChild(resNode, "paxList");

  xmlNodePtr rowsNode = GetNode("rows", paxListNode);
  if (rowsNode==NULL)
    rowsNode = NewTextChild(paxListNode, "rows");

  int col_pax_id=Qry.FieldIndex("pax_id");
  int col_grp_id = Qry.FieldIndex("grp_id");
  int col_point_id = Qry.FieldIndex("point_id");
  int col_airline = Qry.FieldIndex("airline");
  int col_flt_no = Qry.FieldIndex("flt_no");
  int col_suffix = Qry.FieldIndex("suffix");
  int col_scd_out = Qry.FieldIndex("scd_out");
  int col_reg_no = Qry.FieldIndex("reg_no");
  int col_full_name = Qry.FieldIndex("full_name");
  int col_part_key=Qry.FieldIndex("part_key");
  int col_airp_arv = Qry.FieldIndex("airp_arv");
  int col_pr_brd = Qry.FieldIndex("pr_brd");
  int col_refuse = Qry.FieldIndex("refuse");
  int col_class_grp = Qry.FieldIndex("class_grp");
  int col_cabin_class_grp = Qry.FieldIndex("cabin_class_grp");
  int col_ticket_no = Qry.FieldIndex("ticket_no");
  int col_hall = Qry.FieldIndex("hall");
  int col_status=Qry.FieldIndex("status");



  map<int, TTripItem> TripItems;

  TPerfTimer tm;
  tm.Init();
  for( ; !Qry.Eof; Qry.Next()) {

      xmlNodePtr paxNode = NewTextChild(rowsNode, "pax");

      int point_id = Qry.FieldAsInteger(col_point_id);
      int grp_id = Qry.FieldAsInteger(col_grp_id);
      int pax_id =  Qry.FieldAsInteger(col_pax_id);
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

      if(isArch) {
          int col_bag_pool_num=Qry.FieldIndex("bag_pool_num");
          int col_seat_no = Qry.FieldIndex("seat_no");
          int col_seats = Qry.FieldIndex("seats");
          int bag_refuse = Qry.FieldAsInteger("bag_refuse");
          int seats = Qry.FieldAsInteger(col_seats);
          int excess_pc = Qry.FieldAsInteger("excess_pc");
          int excess_wt =  Qry.FieldAsInteger("excess_wt");
          int excess = Qry.FieldAsInteger("excess");
          int bag_pool_num = Qry.FieldAsInteger(col_bag_pool_num);
          int pr_lat =  TReqInfo::Instance()->desk.lang!=AstraLocale::LANG_RU;
          std::string seat_no = Qry.FieldAsString(col_seat_no);
          Dates::DateTime_t b_part_key = DateTimeToBoost(part_key);
          NewTextChild(paxNode, "bag_amount", PG_ARX::get_bagAmount2(b_part_key,
                                                grp_id, pax_id, bag_pool_num).value_or(0));
          NewTextChild(paxNode, "bag_weight", PG_ARX::get_bagWeight2(b_part_key,
                                                grp_id, pax_id, bag_pool_num).value_or(0));
          NewTextChild(paxNode, "rk_weight",  PG_ARX::get_rkWeight2(b_part_key,
                                                grp_id, pax_id, bag_pool_num).value_or(0));

          excessNodeList.add(paxNode, "excess", TBagPieces(excess_pc),
            TBagKilos(PG_ARX::get_excess_wt(b_part_key, grp_id, pax_id, excess_wt,
                                            excess, bag_refuse).value_or(0)));
          NewTextChild(paxNode, "tags", PG_ARX::get_birks2(b_part_key, grp_id, pax_id,
                                                           bag_pool_num, pr_lat).value_or(""));
          NewTextChild(paxNode, "seat_no",  get_seat_no(seats, seat_no));
      } else {
          int col_bag_amount = Qry.FieldIndex("bag_amount");
          int col_bag_weight = Qry.FieldIndex("bag_weight");
          int col_rk_weight = Qry.FieldIndex("rk_weight");
          int col_excess_wt = Qry.FieldIndex("excess_wt");
          int col_excess_pc = Qry.FieldIndex("excess_pc");
          int col_seat_no = Qry.FieldIndex("seat_no");
          int col_tags = Qry.FieldIndex("tags");

          NewTextChild(paxNode, "bag_amount", Qry.FieldAsInteger(col_bag_amount));
          NewTextChild(paxNode, "bag_weight", Qry.FieldAsInteger(col_bag_weight));
          NewTextChild(paxNode, "rk_weight", Qry.FieldAsInteger(col_rk_weight));

          excessNodeList.add(paxNode, "excess", TBagPieces(Qry.FieldAsInteger(col_excess_pc)),
                                                TBagKilos(Qry.FieldAsInteger(col_excess_wt)));
          NewTextChild(paxNode, "tags", Qry.FieldAsString(col_tags));
          NewTextChild(paxNode, "seat_no", Qry.FieldAsString(col_seat_no));

//          NewTextChild(paxNode, "bag_amount", Qry.FieldAsInteger("bag_amount"));
//          NewTextChild(paxNode, "bag_weight", Qry.FieldAsInteger("bag_weight"));
//          NewTextChild(paxNode, "rk_weight", Qry.FieldAsInteger("rk_weight"));

//          excessNodeList.add(paxNode, "excess", TBagPieces(Qry.FieldAsInteger("excess_pc")),
//                                                TBagKilos(Qry.FieldAsInteger("excess_wt")));
//          NewTextChild(paxNode, "tags", Qry.FieldAsString("tags"));
//          NewTextChild(paxNode, "seat_no", Qry.FieldAsString("seat_no"));
      }

      NewTextChild(paxNode, "grp_id", grp_id);
      NewTextChild(paxNode, "airp_arv", ElemIdToCodeNative(etAirp, Qry.FieldAsString(col_airp_arv)));

      string status;
      if (DecodePaxStatus(Qry.FieldAsString(col_status).c_str())!=psCrew)
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

void ArxFltTaskLogRun(TDateTime part_key, XMLRequestCtxt *ctxt,
                      xmlNodePtr reqNode, xmlNodePtr resNode)
{
    LogTrace5 << __func__ << " part_key: " << DateTimeToBoost(part_key);
    TReqInfo *reqInfo = TReqInfo::Instance();
    xmlNodePtr paramNode = reqNode->children;
    int point_id = NodeAsIntegerFast("point_id", paramNode);

    get_compatible_report_form("FltTaskLog", reqNode, resNode);
    STAT::set_variables(resNode);
    xmlNodePtr variablesNode = GetNode("form_data/variables", resNode);
    NewTextChild(variablesNode, "report_title", getLocaleText("��ୠ� ����� ३�"));

    int count = 0;

    xmlNodePtr paxLogNode = NewTextChild(resNode, "PaxLog");
    xmlNodePtr headerNode = NewTextChild(paxLogNode, "header");
    NewTextChild(headerNode, "col", "�����"); // ��� ᮢ���⨬��� � ��ன ���ᨥ� �ନ����

    string SQLQuery;
    string airline;

    if(ARX_EVENTS_DISABLED()) {
            throw UserException("MSG.ERR_MSG.ARX_EVENTS_DISABLED");
    }
    {
        DB::TQuery Qry(PgOra::getROSession("ARX_POINTS"));
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
    "       ev_user, station, ev_order, COALESCE(part_num, 1) AS part_num  "
    "FROM arx_events  "
    "WHERE "
    "   arx_events.part_key = :part_key and "
    "   (arx_events.lang = :lang OR arx_events.lang = :lang_undef) AND "
    "   arx_events.type = :evtFltTask AND  "
    "   arx_events.id1=:point_id  ";

    NewTextChild(resNode, "airline", airline);

    TPerfTimer tm;
    tm.Init();
    xmlNodePtr rowsNode = NULL;
    DB::TQuery Qry(PgOra::getROSession("ARX_EVENTS"));
    Qry.SQLText = SQLQuery;
    Qry.CreateVariable("lang", otString, TReqInfo::Instance()->desk.lang);
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("evtFltTask",otString,EncodeEventType(ASTRA::evtFltTask));
    Qry.CreateVariable("part_key", otDate, part_key);
    Qry.CreateVariable("lang_undef", otString, "ZZ");

    try {
        Qry.Execute();
    } catch (EOracleError &E) {
        if(E.Code == 376)
            throw AstraLocale::UserException("MSG.ONE_OF_DB_FILES_UNAVAILABLE.CALL_ADMIN");
        else
            throw;
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
                    DB::TQuery Qry(*get_main_ora_sess(STDLOG));
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

void StatInterface::FltTaskLogRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqInfo = TReqInfo::Instance();
    if (!reqInfo->user.access.rights().permitted(651))
        throw AstraLocale::UserException("MSG.FLT_TASK_LOG.VIEW_DENIED");
    xmlNodePtr paramNode = reqNode->children;
    int point_id = NodeAsIntegerFast("point_id", paramNode);
    TDateTime part_key = NoExists;
    xmlNodePtr partKeyNode = GetNodeFast("part_key", paramNode);
    if(partKeyNode != NULL)
        part_key = NodeAsDateTime(partKeyNode);
    if(part_key != NoExists) {
        return ArxFltTaskLogRun(part_key, ctxt, reqNode, resNode);
    }
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
    NewTextChild(resNode, "airline", airline);

    TPerfTimer tm;
    tm.Init();
    xmlNodePtr rowsNode = NULL;
    Qry.Clear();
    Qry.SQLText = SQLQuery;
    Qry.CreateVariable("lang", otString, TReqInfo::Instance()->desk.lang);
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.CreateVariable("evtFltTask",otString,EncodeEventType(ASTRA::evtFltTask));
    try {
        Qry.Execute();
    } catch (EOracleError &E) {
        if(E.Code == 376)
            throw AstraLocale::UserException("MSG.ONE_OF_DB_FILES_UNAVAILABLE.CALL_ADMIN");
        else
            throw;
    }

    if(Qry.Eof) {
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

void ArxFltLogRun(TDateTime part_key, XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    LogTrace5 << __func__ << " part_key: " << DateTimeToBoost(part_key);
    TReqInfo *reqInfo = TReqInfo::Instance();
    xmlNodePtr paramNode = reqNode->children;
    int point_id = NodeAsIntegerFast("point_id", paramNode);
    xmlNodePtr client_with_trip_col_in_SysLogNode = GetNodeFast("client_with_trip_col_in_SysLog", paramNode);
    if(client_with_trip_col_in_SysLogNode == NULL)
        get_compatible_report_form("ArxPaxLog", reqNode, resNode);
    else
        get_compatible_report_form("FltLog", reqNode, resNode);
    STAT::set_variables(resNode);
    xmlNodePtr variablesNode = GetNode("form_data/variables", resNode);
    NewTextChild(variablesNode, "report_title", getLocaleText("��ୠ� ����権 ३�"));
    DB::TQuery Qry(PgOra::getROSession("ARX_EVENTS"));
    int count = 0;

    xmlNodePtr paxLogNode = NewTextChild(resNode, "PaxLog");
    xmlNodePtr headerNode = NewTextChild(paxLogNode, "header");
    NewTextChild(headerNode, "col", "�����"); // ��� ᮢ���⨬��� � ��ன ���ᨥ� �ନ����

    string qry1, qry2;
    int move_id = 0;
    string airline;
    {
        if(ARX_EVENTS_DISABLED()) {
            throw UserException("MSG.ERR_MSG.ARX_EVENTS_DISABLED");
        }
        DB::TQuery Qry(PgOra::getROSession("ARX_POINTS"));
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
        "  id1 AS point_id,  "
        "  arx_events.screen,  "
        "  (CASE WHEN type=:evtPax THEN id2 WHEN type=:evtPay THEN id2 ELSE NULL END) AS reg_no,  "
        "  (CASE WHEN type=:evtPax THEN id3 WHEN type=:evtPay THEN id3 ELSE NULL END) AS grp_id,  "
        "  ev_user, station, ev_order, COALESCE(part_num, 1) AS part_num  "
        "FROM arx_events  "
        "WHERE "
        "   arx_events.part_key = :part_key and "
        "   (arx_events.lang = :lang OR arx_events.lang = :lang_undef) and "
        "   arx_events.type IN (:evtFlt,:evtGraph,:evtPax,:evtPay,:evtTlg,:evtPrn) AND  "
        "   arx_events.id1=:point_id  "
        " ORDER BY ev_order";
    qry2 =
        "SELECT msg, time,  "
        "       id2 AS point_id,  "
        "       arx_events.screen,  "
        "       NULL AS reg_no,  "
        "       NULL AS grp_id,  "
        "       ev_user, station, ev_order, COALESCE(part_num, 1) AS part_num  "
        "FROM arx_events  "
        "WHERE "
        "      arx_events.part_key = :part_key and "
        "      (arx_events.lang = :lang OR arx_events.lang = :lang_undef) and "
        "      arx_events.type IN (:evtDisp) AND "
        "      arx_events.id1=:move_id  "
        " ORDER BY ev_order";
    NewTextChild(resNode, "airline", airline);

    TPerfTimer tm;
    tm.Init();
    xmlNodePtr rowsNode = NULL;
    for(int i = 0; i < 2; i++) {
        Qry.ClearParams();
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
        Qry.CreateVariable("part_key", otDate, part_key);
        Qry.CreateVariable("lang_undef", otString, "ZZ");
        try {
            Qry.Execute();
        } catch (EOracleError &E) {
            if(E.Code == 376)
                throw AstraLocale::UserException("MSG.ONE_OF_DB_FILES_UNAVAILABLE.CALL_ADMIN");
            else
                throw;
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
                        DB::TQuery Qry(PgOra::getROSession("SCREEN"));
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

void StatInterface::FltLogRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqInfo = TReqInfo::Instance();
    if (!reqInfo->user.access.rights().permitted(650))
        throw AstraLocale::UserException("MSG.FLT_LOG.VIEW_DENIED");
    xmlNodePtr paramNode = reqNode->children;
    int point_id = NodeAsIntegerFast("point_id", paramNode);
    TDateTime part_key = NoExists;
    xmlNodePtr partKeyNode = GetNodeFast("part_key", paramNode);
    if(partKeyNode != NULL) {
        part_key = NodeAsDateTime(partKeyNode);
    }
    if(part_key != NoExists) {
        return ArxFltLogRun(part_key, ctxt, reqNode, resNode);
    }
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
        try {
            Qry.Execute();
        } catch (EOracleError &E) {
            if(E.Code == 376)
                throw AstraLocale::UserException("MSG.ONE_OF_DB_FILES_UNAVAILABLE.CALL_ADMIN");
            else
                throw;
        }

        if(Qry.Eof) {
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

void ArxLogRun(TDateTime part_key, XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    LogTrace5 << __func__ << " part_key: " << DateTimeToBoost(part_key);
    xmlNodePtr paramNode = reqNode->children;
    int point_id = NodeAsIntegerFast("point_id", paramNode);
    int reg_no = NodeAsIntegerFast("reg_no", paramNode);
    int grp_id = NodeAsIntegerFast("grp_id", paramNode);

    xmlNodePtr client_with_trip_col_in_SysLogNode = GetNode("client_with_trip_col_in_SysLog", reqNode);
    if(client_with_trip_col_in_SysLogNode == NULL)
        get_compatible_report_form("ArxPaxLog", reqNode, resNode);
    else
        get_compatible_report_form("FltLog", reqNode, resNode);
    STAT::set_variables(resNode);
    xmlNodePtr variablesNode = GetNode("form_data/variables", resNode);
    NewTextChild(variablesNode, "report_title", getLocaleText("����樨 �� ���ᠦ���"));
    TReqInfo *reqInfo = TReqInfo::Instance();
    int count = 0;

    xmlNodePtr paxLogNode = NewTextChild(resNode, "PaxLog");
    xmlNodePtr headerNode = NewTextChild(paxLogNode, "header");
    NewTextChild(headerNode, "col", "�����"); // ��� ᮢ���⨬��� � ��ன ���ᨥ� �ନ����
    DB::TQuery AirlineQry(PgOra::getROSession("ARX_POINTS"));
    AirlineQry.CreateVariable("point_id", otInteger, point_id);

    if(ARX_EVENTS_DISABLED())
        throw UserException("MSG.ERR_MSG.ARX_EVENTS_DISABLED");
    AirlineQry.SQLText = "select airline from arx_points where point_id = :point_id and part_key = :part_key and pr_del >= 0";
    AirlineQry.CreateVariable("part_key", otDate, part_key);

    DB::TQuery Qry(PgOra::getROSession("ARX_EVENTS"));
    Qry.SQLText =
        "SELECT msg, time, id1 AS point_id, null as screen, id2 AS reg_no, id3 AS grp_id, "
        "       ev_user, station, ev_order, COALESCE(part_num, 1) AS part_num "
        "FROM arx_events "
        "WHERE part_key = :part_key AND "
        "      (lang = :lang OR lang = :lang_undef) AND "
        "      type IN (:evtPax,:evtPay) AND "
        "      screen <> 'ASTRASERV.EXE' and "
        "      id1=:point_id AND "
        "      (id2 IS NULL OR id2=:reg_no) AND "
        "      (id3 IS NULL OR id3=:grp_id) "
        "ORDER BY ev_order";
    Qry.CreateVariable("part_key", otDate, part_key);
    Qry.CreateVariable("lang_undef", otString, "ZZ");

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

void StatInterface::LogRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    xmlNodePtr paramNode = reqNode->children;
    int point_id = NodeAsIntegerFast("point_id", paramNode);
    int reg_no = NodeAsIntegerFast("reg_no", paramNode);
    int grp_id = NodeAsIntegerFast("grp_id", paramNode);
    TDateTime part_key = NoExists;
    xmlNodePtr partKeyNode = GetNodeFast("part_key", paramNode);
    if(partKeyNode != NULL) {
        part_key = NodeAsDateTime(partKeyNode);
    }
    if(part_key != NoExists) {
        return ArxLogRun(part_key, ctxt, reqNode, resNode);
    }

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
        "      (id3 IS NULL OR id3=:grp_id) "
        "ORDER BY ev_order";

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

    if(Qry.Eof) {
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

void ArxSystemLogRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode, int & count,
                     std::string agent, std::string station, std::string module)
{
    LogTrace5 << __func__ << " agent: " << agent << " station: " << station
              << " module: " << module;
    TReqInfo *reqInfo = TReqInfo::Instance();

//    string module;

//    DB::TQuery Qry(PgOra::getROSession("screen"));
//    xmlNodePtr moduleNode = GetNode("module", reqNode);
//    if(not moduleNode)
//        ;
//    else if(NodeIsNULL(moduleNode))
//        module = SYSTEM_USER;
//    else {
//        Qry.SQLText = "select exe from screen where id = :module";
//        Qry.CreateVariable("module", otInteger, NodeAsInteger(moduleNode));
//        Qry.Execute();
//        if(!Qry.Eof) module = Qry.FieldAsString("exe");
//    }

//    string agent, station;
//    xmlNodePtr agentNode = GetNode("agent", reqNode);
//    if(not agentNode)
//        ;
//    else if(NodeIsNULL(agentNode))
//        agent = SYSTEM_USER;
//    else
//        agent = NodeAsString(agentNode);

//    xmlNodePtr stationNode = GetNode("station", reqNode);
//    if(not stationNode)
//        ;
//    else if(NodeIsNULL(stationNode))
//        station = SYSTEM_USER;
//    else
//        station = NodeAsString(stationNode);

//    ProgTrace(TRACE5, "module: '%s'", module.c_str());
//    ProgTrace(TRACE5, "agent: '%s'", agent.c_str());
//    ProgTrace(TRACE5, "station: '%s'", station.c_str());

//    int count = 0;

    xmlNodePtr paxLogNode = NewTextChild(resNode, "PaxLog");
    xmlNodePtr headerNode = NewTextChild(paxLogNode, "header");
    NewTextChild(headerNode, "col", "�����"); // ��� ᮢ���⨬��� � ��ன ���ᨥ� �ନ����

    DB::TQuery Qry(PgOra::getROSession("ARX_EVENTS"));
    map<int, string> TripItems;
    xmlNodePtr rowsNode = NULL;
    TDeskAccess desk_access;
    if(ARX_EVENTS_DISABLED())
        throw UserException("MSG.ERR_MSG.ARX_EVENTS_DISABLED");
    Qry.SQLText =
        "SELECT msg, time, "
        "       CASE WHEN type IN (:evtFlt, :evtFltTask, :evtPax, :evtPay,:evtGraph, :evtTlg) THEN id1 "
        "                    WHEN type = :evtDisp THEN id2 ELSE NULL END AS point_id, "
        "       screen, "
        "       CASE WHEN type IN(:evtPax,:evtPay) THEN id2 ELSE NULL END AS reg_no, "
        "       CASE WHEN type IN(:evtPax,:evtPay) THEN id3 ELSE NULL END AS grp_id, "
        "  ev_user, station, ev_order, COALESCE(part_num, 1) AS part_num, part_key "
        "FROM "
        "   arx_events "
        "WHERE "
        "  arx_events.part_key >= :FirstDateMINUS10 and " // time � part_key �� ᮢ������ ���
        "  arx_events.part_key < :LastDatePLUS10 and "   // ࠧ��� ⨯�� ᮡ�⨩
        "  (arx_events.lang = :lang OR arx_events.lang = :lang_undef) and "
        "  arx_events.time >= :FirstDate and "         // ���⮬� ��� part_key ��६ ����訩 �������� time
        "  arx_events.time < :LastDate and "
        "  (:agent is null or COALESCE(ev_user, :system_user) = :agent) and "
        "  (:module is null or COALESCE(screen, :system_user) = :module) and "
        "  (:station is null or COALESCE(station, :system_user) = :station) and "
        "  arx_events.type IN ( "
        "    :evtFlt, "
        "    :evtFltTask, "
        "    :evtPax, "
        "    :evtPay, "
        "    :evtGraph, "
        "    :evtTimatic, "
        "    :evtTlg, "
        "    :evtComp, "
        "    :evtAccess, "
        "    :evtSystem, "
        "    :evtCodif, "
        "    :evtSeason, "
        "    :evtDisp, "
        "    :evtPeriod, "
        "    :evtAhm ) "
        "ORDER BY ev_order";
    Qry.CreateVariable("lang_undef", otString, "ZZ");
    Qry.CreateVariable("lang", otString, TReqInfo::Instance()->desk.lang);
    Qry.CreateVariable("evtFlt", otString, NodeAsString("evtFlt", reqNode));
    Qry.CreateVariable("evtPax", otString, NodeAsString("evtPax", reqNode));
    Qry.CreateVariable("system_user", otString, SYSTEM_USER);

    Qry.CreateVariable("evtFltTask", otString, NodeAsString("evtFltTask", reqNode, {}));
    Qry.CreateVariable("evtPay", otString, NodeAsString("evtPay", reqNode, {}));
    Qry.CreateVariable("evtDisp", otString, NodeAsString("evtDisp", reqNode, {}));
    Qry.CreateVariable("evtSeason", otString, NodeAsString("evtSeason", reqNode, {}));
    Qry.CreateVariable("evtAhm", otString, NodeAsString("evtAhm", reqNode, {}));
    Qry.CreateVariable("evtTimatic", otString, NodeAsString("evtTimatic", reqNode, {}));

    Qry.CreateVariable("evtGraph", otString, NodeAsString("evtGraph", reqNode));
    Qry.CreateVariable("evtTlg", otString, NodeAsString("evtTlg", reqNode));
    Qry.CreateVariable("evtComp", otString, NodeAsString("evtComp", reqNode));
    Qry.CreateVariable("evtAccess", otString, NodeAsString("evtAccess", reqNode));
    Qry.CreateVariable("evtSystem", otString, NodeAsString("evtSystem", reqNode));
    Qry.CreateVariable("evtCodif", otString, NodeAsString("evtCodif", reqNode));
    Qry.CreateVariable("evtPeriod", otString, NodeAsString("evtPeriod", reqNode));

    Qry.CreateVariable("FirstDate", otDate, NodeAsDateTime("FirstDate", reqNode));
    Qry.CreateVariable("LastDate", otDate, NodeAsDateTime("LastDate", reqNode));
    Qry.CreateVariable("FirstDateMINUS10", otDate, NodeAsDateTime("FirstDate", reqNode)-10);
    Qry.CreateVariable("LastDatePLUS10", otDate, NodeAsDateTime("LastDate", reqNode)+10);
    Qry.CreateVariable("agent", otString, agent);
    Qry.CreateVariable("station", otString, station);
    Qry.CreateVariable("module", otString, module);

    TPerfTimer tm;
    tm.Init();
    tst();
    try {
        Qry.Execute();
    } catch (EOracleError &E) {
        tst();
        if(E.Code == 376)
            throw AstraLocale::UserException("MSG.ONE_OF_DB_FILES_UNAVAILABLE.CALL_ADMIN");
        else
            throw;
    }
    LogTrace5 << __func__ << "     " << tm.PrintWithMessage().c_str();
        tst();
    typedef map<string, bool> TAccessMap;
    TAccessMap user_access;
        tst();
    if(!Qry.Eof) {
            tst();
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
                tst();
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
                    DB::TQuery tripQry(PgOra::getROSession("ARX_POINTS"));
                    string SQLText =
                        "select " + TTripInfo::selectedFields() +
                        " from arx_points "
                        " where point_id = :point_id and part_key = :part_key ";
                    tripQry.CreateVariable("part_key", otDate,  Qry.FieldAsDateTime(col_part_key));
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
}


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
    Qry.Clear();
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
        "    :evtTimatic, "
        "    :evtTlg, "
        "    :evtComp, "
        "    :evtAccess, "
        "    :evtSystem, "
        "    :evtCodif, "
        "    :evtSeason, "
        "    :evtDisp, "
        "    :evtPeriod, "
        "    :evtAhm) "
        " ORDER BY ev_order";

    Qry.CreateVariable("lang", otString, TReqInfo::Instance()->desk.lang);
    Qry.CreateVariable("evtFlt", otString, NodeAsString("evtFlt", reqNode));
    Qry.CreateVariable("evtPax", otString, NodeAsString("evtPax", reqNode));
    Qry.CreateVariable("system_user", otString, SYSTEM_USER);

    Qry.CreateVariable("evtFltTask", otString, NodeAsString("evtFltTask", reqNode, {}));
    Qry.CreateVariable("evtPay", otString, NodeAsString("evtPay", reqNode, {}));
    Qry.CreateVariable("evtDisp", otString, NodeAsString("evtDisp", reqNode, {}));
    Qry.CreateVariable("evtSeason", otString, NodeAsString("evtSeason", reqNode, {}));
    Qry.CreateVariable("evtAhm", otString, NodeAsString("evtAhm", reqNode, {}));
    Qry.CreateVariable("evtTimatic", otString, NodeAsString("evtTimatic", reqNode, {}));

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
    LogTrace5 << __func__ << "     " << tm.PrintWithMessage().c_str();

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
                        "from points "
                        " where "
                        "   point_id = :point_id ";

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

    ArxSystemLogRun(ctxt, reqNode, resNode, count, agent, station, module);

    if(!count)
        throw AstraLocale::UserException("MSG.OPERATIONS_NOT_FOUND");
}

void UnaccompListToXML(DB::TQuery &Qry, xmlNodePtr resNode, TComplexBagExcessNodeList &excessNodeList,
                       bool isPaxSearch, int pass, int &count, bool isArch)
{
  LogTrace5 << __func__ << " isArch: " << isArch << " isPaxSearch: " << isPaxSearch;
  if(Qry.Eof) {
      LogTrace5 << __func__ << " No Data found";
      return;
  }

  xmlNodePtr paxListNode = GetNode("paxList", resNode);
  if (paxListNode==NULL)
    paxListNode = NewTextChild(resNode, "paxList");

  xmlNodePtr rowsNode = GetNode("rows", paxListNode);
  if (rowsNode==NULL)
    rowsNode = NewTextChild(paxListNode, "rows");

  int col_point_id = Qry.FieldIndex("point_id");
  int col_scd_out = Qry.FieldIndex("scd_out");
  int col_excess_pc = Qry.FieldIndex("excess_pc");
  int col_grp_id = Qry.FieldIndex("grp_id");
  int col_airp_arv = Qry.FieldIndex("airp_arv");
  int col_hall = Qry.FieldIndex("hall");
  int col_part_key=Qry.FieldIndex("part_key");

  map<int, TTripItem> TripItems;
  TPerfTimer tm;
  tm.Init();
  for(;!Qry.Eof;Qry.Next())
  {
      xmlNodePtr paxNode=NewTextChild(rowsNode,"pax");

      int point_id = Qry.FieldAsInteger(col_point_id);
      int grp_id = Qry.FieldAsInteger(col_grp_id);

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

      if(isArch) {
          int excess_wt =  Qry.FieldAsInteger("excess_wt");
          int excess = Qry.FieldAsInteger("excess");
          int bag_refuse = Qry.FieldAsInteger("bag_refuse");
          int pr_lat = TReqInfo::Instance()->desk.lang!=AstraLocale::LANG_RU;
          Dates::DateTime_t b_part_key = DateTimeToBoost(part_key);
          NewTextChild(paxNode, "bag_amount", PG_ARX::get_bagAmount2(b_part_key,
                                                grp_id, std::nullopt, 0).value_or(0));
          NewTextChild(paxNode, "bag_weight", PG_ARX::get_bagWeight2(b_part_key,
                                                grp_id, std::nullopt, 0).value_or(0));
          NewTextChild(paxNode, "rk_weight",  PG_ARX::get_rkWeight2(b_part_key,
                                                grp_id, std::nullopt, 0).value_or(0));
          excessNodeList.add(paxNode, "excess", TBagPieces(Qry.FieldAsInteger(col_excess_pc)),
                  TBagKilos(PG_ARX::get_excess_wt(b_part_key,
                            grp_id, std::nullopt, excess_wt, excess, bag_refuse).value_or(0)));
          NewTextChild(paxNode, "tags", PG_ARX::get_birks2(b_part_key,
                                            grp_id, std::nullopt, 0, pr_lat).value_or(""));
      } else {
          NewTextChild(paxNode, "bag_amount", Qry.FieldAsInteger("bag_amount"));
          NewTextChild(paxNode, "bag_weight", Qry.FieldAsInteger("bag_weight"));
          NewTextChild(paxNode, "rk_weight", Qry.FieldAsInteger("rk_weight"));
          excessNodeList.add(paxNode, "excess", TBagPieces(Qry.FieldAsInteger("excess_pc")),
                                                TBagKilos(Qry.FieldAsInteger("excess_wt")));
          NewTextChild(paxNode, "tags", Qry.FieldAsString("tags"));
      }
      NewTextChild(paxNode, "grp_id", grp_id);
      NewTextChild(paxNode, "airp_arv", ElemIdToCodeNative(etAirp, Qry.FieldAsString(col_airp_arv)));

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

void ArxPaxListRun(Dates::DateTime_t part_key, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    LogTrace5 << __func__ << " part_key: " << part_key;
    xmlNodePtr paramNode = reqNode->children;
    int point_id = NodeAsIntegerFast("point_id", paramNode);
    get_compatible_report_form("ArxPaxList", reqNode, resNode);

    DB::TQuery Qry(PgOra::getROSession("ARX_PAX_GRP"));
    string SQLText;
    SQLText =
        "SELECT "
        "   arx_points.part_key, " +
        TTripInfo::selectedFields("arx_points") + ", "
        "   arx_pax.reg_no, "
        "   arx_pax.surname||' '||arx_pax.name full_name, "
        "   arx_pax.excess_pc, "
        "   arx_pax.refuse, "
        "   arx_pax.pr_brd, "
        "   arx_pax.ticket_no, "
        "   arx_pax.pax_id, "
        "   arx_pax.bag_pool_num, "
        "   arx_pax.seats, "
        "   arx_pax.seat_no, "
        "   COALESCE(arx_pax.cabin_class_grp, arx_pax_grp.class_grp) AS cabin_class_grp, "
        "   arx_pax_grp.grp_id, "
        "   arx_pax_grp.airp_arv, "
        "   arx_pax_grp.status, "
        "   arx_pax_grp.class_grp, "
        "   arx_pax_grp.hall, "
        "   arx_pax_grp.excess_wt, "
        "   arx_pax_grp.excess, "
        "   arx_pax_grp.bag_refuse "
        "FROM arx_pax_grp, arx_pax, arx_points "
        "WHERE "
        "   arx_points.point_id = :point_id and arx_points.pr_del>=0 and "
        "   arx_points.part_key = arx_pax_grp.part_key AND "
        "   arx_points.point_id = arx_pax_grp.point_dep and "
        "   arx_pax_grp.part_key = arx_pax.part_key AND "
        "   arx_pax_grp.grp_id = arx_pax.grp_id AND "
        "   arx_points.part_key = :part_key and "
        "   arx_pax.pr_brd IS NOT NULL  ";
    TReqInfo &info = *(TReqInfo::Instance());
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
    Qry.CreateVariable("part_key", otDate, BoostToDateTime(part_key));

    Qry.SQLText = SQLText;
    Qry.CreateVariable("point_id", otInteger, point_id);
    //Qry.CreateVariable("pr_lat", otInteger, TReqInfo::Instance()->desk.lang!=AstraLocale::LANG_RU);

    TPerfTimer tm;
    tm.Init();
    Qry.Execute();
    ProgTrace(TRACE5, "Qry.Execute: %s", tm.PrintWithMessage().c_str());

    int count=0;
    TComplexBagExcessNodeList excessNodeList(OutputLang(), {});
    PaxListToXML(Qry, resNode, excessNodeList, false, 0, count, true);

    ProgTrace(TRACE5, "XML: %s", tm.PrintWithMessage().c_str());

    //��ᮯ஢������� �����
    Qry.SQLText=
        "SELECT "
        "  arx_pax_grp.part_key, " +
        TTripInfo::selectedFields("arx_points") + ", "
        "  arx_pax_grp.airp_arv, "
        "  arx_pax_grp.excess_pc, "
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
    Qry.CreateVariable("part_key", otDate, BoostToDateTime(part_key));
    Qry.Execute();
    UnaccompListToXML(Qry, resNode, excessNodeList, false, 0, count, true);

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

class MqRabbitSearchParamsSender {
private:
  static const std::string MQRABBIT_SEARCH_PAX_TYPE;
public:
  class StatMQRSender: public MQRABBIT_TRANSPORT::MQRSender {
   private:
      std::map<std::string,std::string> searchParams;
   public:
    virtual void send( const std::string& senderType,
                       const MQRABBIT_TRANSPORT::MQRabbitRequest &request,
                       std::map<std::string,std::string>& params ) {
      //�ନ஢���� ������ ��� ��ࠢ�� � ࠡ���
      LogTrace(TRACE5)<< "airlines= " << GetSQLEnum( request.airlines ) << " flt_nos=" << getSQLEnum( request.flts ) << " airps=" << GetSQLEnum( request.airps );
      stringstream sb;
      for ( const auto& s : params ) {
        sb << s.first << "=" << s.second << "\n";
      }
      LogTrace(TRACE5)<< "params " << sb.str();
      std::map<std::string,std::string>::const_iterator iparam = params.find( MQRABBIT_TRANSPORT::PARAM_NAME_ADDR );
      if ( iparam == params.end() ||
           iparam->second.empty() ) {
        ProgError( STDLOG, "putMQRabbitPaxs: invalid connect string in file_params_sets" );
        return;
      }
      MQRABBIT_TRANSPORT::MQRabbitParams p( iparam->second );
      if ( p.addr.empty() ||
           p.queue.empty() ) {
        LogTrace(TRACE5) << "addr is empty";
        return;
      }
      XMLDoc docSearchData = NULL;
      std::map<std::string,std::string>::const_iterator ip;
      try {
        docSearchData = XMLDoc( "SearchPaxParams" );
        xmlNodePtr node = docSearchData.docPtr()->children;
        SetProp( node, "src", "Astra" );
        ip = searchParams.find( "request" );
        if ( ip != searchParams.end() ) {
          SetProp( node, "request", ip->second );
        }
        ip = searchParams.find( "lang" );
        if ( ip != searchParams.end() ) {
          SetProp( node, "lang", ip->second );
        }
        SetProp( node, "time", "UTC" );
        NewTextChild( node, "requestType", senderType );
        xmlNodePtr n = NewTextChild( node, "user" );
        NewTextChild( n, "login", TReqInfo::Instance()->user.login );
        NewTextChild( n, "descr", TReqInfo::Instance()->user.descr );
        NewTextChild( node, "desk", TReqInfo::Instance()->desk.code );
        node = NewTextChild( node, "params" );
        for ( const auto &p : searchParams ) {
          xmlNodePtr itemNode;
          if ( p.first == "point_id" ||
               p.first == "part_key" ||
               p.first == "request" ||
               p.first == "lang" )
            continue;
          itemNode = NewTextChild( node, "item" );
          NewTextChild( itemNode, "name", p.first );
          NewTextChild( itemNode, "value", p.second );
        }
        //put to queue
        if ( docSearchData.docPtr() != nullptr ) {
          try {
            jms::text_message in1;
            jms::connection cl( p.addr, false );
            jms::text_queue queue = cl.create_text_queue( p.queue );//"astra_exch/CRM_DATA/astra.tst.crm");
            in1.text = ConvertCodepage( docSearchData.text(), "CP866", "UTF-8" );
            LogTrace(TRACE5) << docSearchData.text();
            queue.enqueue(in1);
            cl.commit();
          }
          catch( std::exception &E ) {
            throw EXCEPTIONS::Exception( string(E.what()) + " mqrabbit params '" +  p.addr + ";" + p.queue + "'" );
          }
        }
        else
          throw EXCEPTIONS::Exception( "Can't create SearchData xml dcoument" );
      }
      catch( std::exception &E ) {
        LogError(STDLOG) << __func__ << " Exception: "  << E.what();
      }
      catch(...) {
        LogError(STDLOG) << __func__ << " unknown error";
      }
    }
    StatMQRSender( const std::map<std::string,std::string>& params ):MQRSender(),searchParams(params) {
    }
  };
public:
  enum TSearchParamsSenderType { tsPaxListRun, tsPaxSrcRun };
  static void send( const TSearchParamsSenderType &senderType, std::map<std::string,std::string>& params ) {
    TTripInfo tripInfo;
    std::map<std::string,std::string>::const_iterator ip;
    switch ( senderType ) {
      case tsPaxListRun:
        params.insert( make_pair( "request", "PaxListRun(���� ᯨ᪠ ���ᠦ�஢)" ) );
        int point_id;
        TDateTime part_key;
        if ( (ip=params.find( "part_key" )) != params.end() ) {
          StrToDateTime( ip->second.c_str(), part_key );
        }
        else
          part_key = NoExists;
        StrToInt( params.find( "point_id" )->second.c_str(), point_id );
        tripInfo.getByPointId(part_key,point_id);
        params.insert( make_pair("airline", tripInfo.airline) );
        params.insert( make_pair("flt_no", IntToString(tripInfo.flt_no) + tripInfo.suffix) );
        params.insert( make_pair("scd_out", DateTimeToStr( tripInfo.scd_out ) ) );
        break;
      case tsPaxSrcRun:
        params.insert( make_pair( "request", "PaxSrcRun(���� ������ ���ᠦ��)" ) );
        if ( (ip = params.find( "airline" )) != params.end() ) {
          tripInfo.airline = ip->second;
        }
        if ( (ip=params.find( "flt_no" )) != params.end() ) {
          StrToInt( ip->second.c_str(), tripInfo.flt_no );
        }
        if ( (ip=params.find( "dest" )) != params.end() ) {
          tripInfo.airp = ip->second; //����� ���ࠢ����� - ��ய��� �ਫ��, � �� �뫥� (᪮॥ �ᥣ� �� �㤥� �ᯮ�짮���� �������)
        }
        break;
      default:
        return;
    }
    LogTrace(TRACE5) << "airline=" << tripInfo.airline << " flt_no=" << tripInfo.flt_no << " airp_arv=" << tripInfo.airp;
    if ( not is_sync_FileParamSets( tripInfo, MQRABBIT_SEARCH_PAX_TYPE ) ) {
      return;
    }
    StatMQRSender sender( params );
    sender.execute( MQRABBIT_SEARCH_PAX_TYPE );
  }
};
const std::string MqRabbitSearchParamsSender::MQRABBIT_SEARCH_PAX_TYPE = "MQSEARCH";

void StatInterface::PaxListRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    std::map<std::string,std::string> params;
    TReqInfo &info = *(TReqInfo::Instance());
    if (!info.user.access.rights().permitted(630))
        throw AstraLocale::UserException("MSG.PAX_LIST.VIEW_DENIED");
    if (info.user.access.totally_not_permitted())
        throw AstraLocale::UserException("MSG.PASSENGERS.NOT_FOUND");
    xmlNodePtr paramNode = reqNode->children;

    int point_id = NodeAsIntegerFast("point_id", paramNode);
    params.insert( make_pair("point_id", IntToString(point_id)) );
    TDateTime part_key = NoExists;
    xmlNodePtr partKeyNode = GetNodeFast("part_key", paramNode);
    if(partKeyNode != NULL) {
        part_key = NodeAsDateTime(partKeyNode);
        params.insert( make_pair( "part_key", DateTimeToStr( part_key ) ) );
    }
    params.insert( make_pair("lang", TReqInfo::Instance()->desk.lang) );
    MqRabbitSearchParamsSender::send( MqRabbitSearchParamsSender::tsPaxListRun, params );
    if(part_key != NoExists) {
        return ArxPaxListRun(DateTimeToBoost(part_key), reqNode, resNode);
    }
    get_compatible_report_form("ArxPaxList", reqNode, resNode);
    {
        DB::TQuery Qry(*get_main_ora_sess(STDLOG));
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
        PaxListToXML(Qry, resNode, excessNodeList, false, 0, count, false);

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
        }
        Qry.Execute();
        UnaccompListToXML(Qry, resNode, excessNodeList, false, 0, count, false);

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

void fillSqlSrcRunQuery(ostringstream & sql, const TReqInfo & info, const std::string& airline,
                         const std::string& city, const std::string& surname, int flt_no,
                         const std::string& ticket_no, TDateTime FirstDate, TDateTime LastDate)
{
    if (!info.user.access.airps().elems().empty()) {
      if (info.user.access.airps().elems_permit())
        sql << " AND airp IN " << GetSQLEnum(info.user.access.airps().elems()) << "\n";
      else
        sql << " AND airp NOT IN " << GetSQLEnum(info.user.access.airps().elems()) << "\n";
    }
    if (!info.user.access.airlines().elems().empty()) {
      if (info.user.access.airlines().elems_permit())
        sql << " AND airline IN " << GetSQLEnum(info.user.access.airlines().elems()) << "\n";
      else
        sql << " AND airline NOT IN " << GetSQLEnum(info.user.access.airlines().elems()) << "\n";
    }
    if(!airline.empty())
      sql << " AND airline = :airline \n";
    if(!city.empty())
      sql << " AND airp_arv = :city \n";
    if(flt_no != NoExists)
      sql << " AND flt_no = :flt_no \n";
    if(!surname.empty()) {
      if(FirstDate + 1 < LastDate && surname.size() < 4)
        sql << " AND surname = :surname \n";
      else
        sql << " AND surname like :surname||'%' \n";
    }
    if(!ticket_no.empty())
      sql << " AND ticket_no like '%'||:ticket_no||'%' \n";
}

void ArxPaxSrcRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode, int & count)
{
    LogTrace5 << __func__;
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
    DB::TQuery Qry(PgOra::getROSession("ARX_PAX_GRP"));
    Qry.CreateVariable("FirstDate", otDate, FirstDate);
    Qry.CreateVariable("LastDate", otDate, LastDate);
    //Qry.CreateVariable("pr_lat", otInteger, info.desk.lang != AstraLocale::LANG_RU);
    xmlNodePtr paramNode = reqNode->children;
    string airline = NodeAsStringFast("airline", paramNode, "");
    if(!airline.empty())
        Qry.CreateVariable("airline", otString, airline);
    string city = NodeAsStringFast("dest", paramNode, "");
    if(!city.empty())
        Qry.CreateVariable("city", otString, city);
    int flt_no = NodeAsInteger("flt_no", paramNode, NoExists);
    if(flt_no != NoExists)
        Qry.CreateVariable("flt_no", otInteger, flt_no);
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
    for(int pass = 1; (pass <= 2) && (count < MAX_STAT_ROWS()); pass++) {
        ostringstream sql;
        sql << "SELECT arx_points.part_key, " <<
               TTripInfo::selectedFields("arx_points") << ", \n"
            << " arx_pax.reg_no, \n"
               " arx_pax.surname||' '||arx_pax.name full_name, \n"
               " arx_pax.excess_pc, \n"
               " arx_pax.pr_brd, \n"
               " arx_pax.refuse, \n"
               " arx_pax.ticket_no, \n"
               " arx_pax.pax_id, \n"
               " arx_pax.part_key, \n"
               " arx_pax.bag_pool_num, \n"
               " arx_pax.seat_no, \n"
               " arx_pax.seats, \n"
               " COALESCE(arx_pax.cabin_class_grp, arx_pax_grp.class_grp) AS cabin_class_grp, \n"
               " arx_pax_grp.airp_arv, \n"
               " arx_pax_grp.grp_id, \n"
               " arx_pax_grp.status, \n"
               " arx_pax_grp.class_grp, \n"
               " arx_pax_grp.hall, \n"
               " arx_pax_grp.excess_wt, \n"
               " arx_pax_grp.excess, \n"
               " arx_pax_grp.bag_refuse \n"
               "FROM arx_pax_grp , arx_pax , arx_points \n";
        if (pass==2) {
            sql << getMoveArxQuery();
        }
        if(!document.empty())
        {
            sql << " , arx_pax_doc \n";
        };
        if(!tag_no.empty())
        {
            sql << " , arx_bag_tags \n";
        };
        sql << "WHERE \n";
        if (pass==1) {
          sql << " arx_points.part_key >= :FirstDate AND arx_points.part_key <:arx_trip_date_range AND \n";
        }
        if (pass==2) {
          sql << " arx_points.part_key = arx_ext.part_key AND arx_points.move_id = arx_ext.move_id AND \n";
        }
        sql << " arx_points.scd_out >= :FirstDate AND arx_points.scd_out < :LastDate AND \n"
               " arx_points.part_key = arx_pax_grp.part_key AND \n"
               " arx_points.point_id = arx_pax_grp.point_dep AND \n"
               " arx_pax_grp.part_key= arx_pax.part_key AND \n"
               " arx_pax_grp.grp_id  = arx_pax.grp_id AND \n"
               " arx_points.pr_del>=0 \n";
        if(!document.empty())
        {
          sql << " AND arx_pax.part_key = arx_pax_doc.part_key \n"
                 " AND arx_pax.pax_id   = arx_pax_doc.pax_id \n"
                 " AND arx_pax_doc.no like '%'||:document||'%' \n";
        };
        if(!tag_no.empty())
        {
          sql << " AND arx_pax_grp.part_key = arx_bag_tags.part_key \n"
                 " AND arx_pax_grp.grp_id = arx_bag_tags.grp_id \n"
                 " AND arx_bag_tags.no like '%'||:tag_no \n";
        };

        fillSqlSrcRunQuery(sql, info, airline, city, surname, flt_no, ticket_no,
                           FirstDate, LastDate);
        Qry.CreateVariable("arx_trip_date_range", otDate, LastDate + ARX_TRIP_DATE_RANGE());

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
        PaxListToXML(Qry, resNode, excessNodeList, true, pass, count, true);

    }
}

void StatInterface::PaxSrcRun(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    std::map<std::string,std::string> params;
    TReqInfo &info = *(TReqInfo::Instance());
    if (!info.user.access.rights().permitted(620))
        throw AstraLocale::UserException("MSG.PAX_SRC.ACCESS_DENIED");
    if (info.user.access.totally_not_permitted())
        throw AstraLocale::UserException("MSG.PASSENGERS.NOT_FOUND");
    TDateTime FirstDate = NodeAsDateTime("FirstDate", reqNode);
    params.insert( make_pair("FirstDate", DateTimeToStr(FirstDate)) );
    TDateTime LastDate = NodeAsDateTime("LastDate", reqNode);
    params.insert( make_pair("LastDate", DateTimeToStr(LastDate)) );
    if(IncMonth(FirstDate, 1) < LastDate)
        throw AstraLocale::UserException("MSG.SEARCH_PERIOD_SHOULD_NOT_EXCEED_ONE_MONTH");
    TPerfTimer tm;
    DB::TQuery Qry(*get_main_ora_sess(STDLOG));
    Qry.CreateVariable("FirstDate", otDate, FirstDate);
    Qry.CreateVariable("LastDate", otDate, LastDate);
    Qry.CreateVariable("pr_lat", otInteger, TReqInfo::Instance()->desk.lang!=AstraLocale::LANG_RU);
    params.insert( make_pair("lang", TReqInfo::Instance()->desk.lang) );
    xmlNodePtr paramNode = reqNode->children;
    string airline = NodeAsStringFast("airline", paramNode, "");
    if(!airline.empty()) {
        Qry.CreateVariable("airline", otString, airline);
        params.insert( make_pair("airline", airline) );
    }
    string city = NodeAsStringFast("dest", paramNode, "");
    if(!city.empty()) {
        Qry.CreateVariable("city", otString, city);
        params.insert( make_pair("airp_arv", city) );
    }
    int flt_no = NodeAsInteger("flt_no", paramNode, NoExists);
    if(flt_no != NoExists) {
        Qry.CreateVariable("flt_no", otInteger, flt_no);
        params.insert( make_pair("flt_no", IntToString(flt_no)));
    }
    string surname = NodeAsStringFast("surname", paramNode, "");
    if(!surname.empty()) {
        Qry.CreateVariable("surname", otString, surname);
        params.insert( make_pair("surname", surname) );
    }
    string document = NodeAsStringFast("document", paramNode, "");
    if(!document.empty()) {
        if(document.size() < 6)
            throw AstraLocale::UserException("MSG.PAX_SRC.MIN_DOC_LENGTH");
        Qry.CreateVariable("document", otString, document);
        params.insert( make_pair("document", document) );
    }
    string ticket_no = NodeAsStringFast("ticket_no", paramNode, "");
    if(!ticket_no.empty()) {
        if(ticket_no.size() < 6)
            throw AstraLocale::UserException("MSG.PAX_SRC.MIN_TKT_LENGTH");
        Qry.CreateVariable("ticket_no", otString, ticket_no);
        params.insert( make_pair("ticket_no", ticket_no) );
    }
    string tag_no = NodeAsStringFast("tag_no", paramNode, "");
    if(!tag_no.empty()) {
        if(tag_no.size() < 3)
            throw AstraLocale::UserException("MSG.PAX_SRC.MIN_TAG_LENGTH");
        double Value;
        if ( StrToFloat( tag_no.c_str(), Value ) == EOF )
            throw Exception("Cannot convert tag no '%s' to an Float", tag_no.c_str());
        Qry.CreateVariable("tag_no", otFloat, Value);
        params.insert( make_pair("tag_no", FloatToString(Value)) );
    }
    MqRabbitSearchParamsSender::send( MqRabbitSearchParamsSender::tsPaxSrcRun, params );
    int count = 0;
    for(int pass = 0; pass < 1; pass++) {
        ostringstream sql;
        sql << "SELECT NULL part_key, \n";
        sql << TTripInfo::selectedFields("points") << ", \n"
               " pax.reg_no, \n"
               " pax_grp.airp_arv, \n"
               " pax.surname||' '||pax.name full_name, \n"
               " NVL(ckin.get_bagAmount2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) bag_amount, \n"
               " NVL(ckin.get_bagWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) bag_weight, \n"
               " NVL(ckin.get_rkWeight2(pax.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) rk_weight, \n"
               " ckin.get_excess_wt(pax.grp_id, pax.pax_id, pax_grp.excess_wt, pax_grp.bag_refuse) AS excess_wt, "
               " ckin.get_excess_pc(pax.grp_id, pax.pax_id) AS excess_pc, "
               " ckin.get_birks2(pax.grp_id,pax.pax_id,pax.bag_pool_num,:pr_lat) tags, \n"
               " salons.get_seat_no(pax.pax_id, pax.seats, pax.is_jmp, pax_grp.status, pax_grp.point_dep, 'seats', rownum) seat_no, \n"
               " pax_grp.grp_id, \n"
               " pax.pr_brd, \n"
               " pax.refuse, \n"
               " pax_grp.class_grp, \n"
               " NVL(pax.cabin_class_grp, pax_grp.class_grp) AS cabin_class_grp, \n"
               " pax_grp.hall, \n"
               " pax.ticket_no, \n"
               " pax.pax_id, \n"
               " pax_grp.status \n"
               "FROM pax_grp, pax, points \n";
        if(!document.empty())
        {
            sql << " , pax_doc \n";
        };
        if(!tag_no.empty())
        {
            sql << " , bag_tags \n";
        };
        sql << "WHERE \n"
               " points.scd_out >= :FirstDate AND points.scd_out < :LastDate AND \n"
               " points.point_id = pax_grp.point_dep AND \n"
               " pax_grp.grp_id=pax.grp_id AND \n"
               " points.pr_del>=0 \n";
        if(!document.empty())
        {
          sql << " AND pax.pax_id = pax_doc.pax_id \n"
                 " AND pax_doc.no like '%'||:document||'%' \n";
        };
        if(!tag_no.empty())
        {
          sql << " AND pax_grp.grp_id = bag_tags.grp_id \n"
                 " AND bag_tags.no like '%'||:tag_no \n";
        };

        fillSqlSrcRunQuery(sql, info, airline, city, surname, flt_no, ticket_no,
                           FirstDate, LastDate);
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
        PaxListToXML(Qry, resNode, excessNodeList, true, pass, count, false);

    }
    ArxPaxSrcRun(ctxt, reqNode, resNode,count);
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

