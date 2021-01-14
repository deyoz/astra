#if HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef XP_TESTING

#include "astra_types.h"
#include "astra_elems.h"
#include "astra_main.h"
#include "astra_misc.h"
#include "flt_settings.h"
#include "astra_api.h"
#include "checkin.h"
#include "season.h"
#include "salons.h"
#include "date_time.h"
#include "apps_interaction.h"
#include "alarms.h"
#include "xml_unit.h"
#include "pg_session.h"
#include "tlg/tlg.h"
#include "tlg/remote_system_context.h"
#include "tlg/edi_tlg.h"
#include "tlg/apps_handler.h"
#include "hooked_session.h"
#include "iapi_interaction.h"
#include "prn_tag_store.h"
#include "cache.h"
#include "PgOraConfig.h"
#include "timer.h"

#include <queue>
#include <fstream>
#include <sstream>
#include <boost/regex.hpp>
#include <boost/foreach.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <serverlib/tscript.h>
#include <serverlib/exception.h>
#include <serverlib/func_placeholders.h>
#include <serverlib/posthooks.h>
#include <serverlib/cursctl.h>
#include <serverlib/dbcpp_cursctl.h>
#include <serverlib/dates_oci.h>
#include <serverlib/str_utils.h>
#include <serverlib/tcl_utils.h>
#include <serverlib/dates_io.h>
#include <serverlib/dump_table.h>
#include <serverlib/rip_oci.h>
#include <serverlib/pg_rip.h>
#include <jxtlib/jxtlib.h>
#include <jxtlib/utf2cp866.h>

#define NICKNAME "ROMAN"
#include <serverlib/slogger.h>

void tests_init() {}

void runEdiTimer_4testsOnly();
static std::vector<string> getFlightTasks(const std::string& table_name, const PointId_t &point_id);

using namespace xp_testing::tscript;

static std::string executeAstraRequest(const std::string &request,
    const std::string &ignore_err_code,
    bool ignore_err)
{
    ServerFramework::QueryRunner query_runner (ServerFramework::TextQueryRunner());

    const std::string answer =
        jxtlib::JXTLib::Instance()->GetCallbacks()->Main(std::string(),
                            request, GetTestContext()->vars["JXT_PULT"], "");

    XMLDoc doc(answer);
    if(!ignore_err && GetNode("/term/answer/command/error", doc.docPtr())) {
        const std::string error = NodeAsString( "/term/answer/command/error", doc.docPtr() );
        if(ignore_err_code.empty())
            throw EXCEPTIONS::Exception("failed with code: " + error);
        else if(ignore_err_code != error) {
            throw EXCEPTIONS::Exception("expected error: " + ignore_err_code +
                            " but failed with: " + error);
        }
    }
    return answer;
}

static std::string removeVersionRecursive(const std::string answer, bool &removed) {
    std::string::size_type ver_pos = answer.find("ver=\"");
    if(ver_pos != std::string::npos) {
        std::string::size_type end_ver_pos = answer.find("\"", ver_pos + 5);
        if(end_ver_pos != std::string::npos) {
            removed = true;
            return answer.substr(0, ver_pos) + answer.substr(end_ver_pos + 1);
        }

    }
    removed = false;
    return answer;
}

static std::string removeVersion(const std::string answer) {
    bool removed = true;
    std::string result = answer;
    while(removed) {
        result = removeVersionRecursive(result, removed);
    }

    return result;
}

static void executeRequest(
            const std::vector<tok::Param>& params,
            const std::string& req,
            std::string& reply, /* io */
            std::queue<std::string>& outq /* io */)
{
    const std::string capture = tok::Validate(tok::GetValue(params, "capture", "off"), "noformat on off");
    const std::string errStr = tok::GetValue(params, "err");
    reply = executeAstraRequest(req, errStr, errStr == "ignore");
    if (capture == "on") {
        reply = removeVersion(reply);
        reply = formatXmlString(reply);
        reply = UTF8toCP866(reply);
        reply = StrUtils::replaceSubstrCopy(reply, "\"", "'");
        reply = StrUtils::replaceSubstrCopy(reply, "encoding='UTF-8'", "encoding='CP866'");
        outq.push(reply);
    }
}

static std::string FP_astra_hello(const std::vector<std::string>& args)
{
    return "Hello world!";
}

static std::string LastRedisplay;

void setLastRedisplay(const std::string &redisplay)
{
    LastRedisplay = redisplay;
}
const std::string &getLastRedisplay()
{
    return LastRedisplay;
}

static std::string FP_lastRedisplay(const std::vector<std::string> &args)
{
    return getLastRedisplay();
}

static std::string FP_req(const std::vector<tok::Param>& params)
{
    std::queue<std::string>& outq = GetTestContext()->outq;
    tok::ValidateParams(params, 1, 1, "err ignore pages capture ws");
    const std::string text = tok::PositionalValues(params).at(0);

    if (text.empty()) {
        LogTrace(TRACE5) << __FUNCTION__ << ": skipping empty request";
        return std::string();
    }

    LogTrace(TRACE5) << __FUNCTION__ << ": top";
    CheckEmpty(outq);

    executeRequest(params, text, GetTestContext()->reply, outq);
    return GetTestContext()->reply;
}

static std::string FP_init_jxt_pult(const std::vector<std::string> &args)
{
    if(init_locale() < 0) {
        puts("Init locale failed");
        return "";
    }

    assert(args.size() == 1 && args[0].length() == 6);
    GetTestContext()->vars["JXT_PULT"] = args[0];
    return "";
}

static std::string FP_init(const std::vector<std::string> &args)
{
    if(init_locale() < 0) {
        puts("Init locale failed");
        return "";
    }

    return "";
}

static std::vector<std::string> edifactOurrefVector()
{
    std::vector<std::string> refs;
    std::string ref;
    OciCpp::CursCtl cur = make_curs("select OURREF from EDISESSION order by LAST_ACCESS desc, IDA desc");

    cur.
        def(ref).
        exec();
    while(!cur.fen()) {
        refs.push_back(ref);
    }
    return refs;
}

static std::string lastEdifactOurref(size_t pos)
{
    std::vector<std::string> res = edifactOurrefVector();
    return (res.empty() || (res.size() - 1) < pos) ? "" : res.at(pos);
}


/* $(last_edifact_ref) */
static std::string FP_last_edifact_ref(const std::vector<std::string>& p)
{
    if (p.size() > 0) {
        return lastEdifactOurref(std::stoi(p.at(0)));
    } else {
        return lastEdifactOurref(0);
    }
}

namespace xp_testing {
    namespace tscript {
        void ExecuteTlg(const std::vector<tok::Param>& params);
    }
}

static std::string FP_tlg_in(const std::vector<tok::Param>& params)
{
    LogTrace(TRACE5) << __FUNCTION__ << ": top";
    std::queue<std::string>& outq = GetTestContext()->outq;
    CheckEmpty(outq);

    ExecuteTlg(params); /* Подаём на вход */
    return std::string();
}

static std::string FP_init_eds(const std::vector<std::string> &p)
{
    using namespace Ticketing::RemoteSystemContext;

    assert(p.size() > 2);

    bool translit = false;
    if(p.size() > 3) {
        translit = (p.at(3) == "translit");
    }

    std::string h2hAddr = "",
             ourH2hAddr = "";
    if(p.size() > 4) {
        h2hAddr = p.at(4);
        ourH2hAddr = p.at(5);
    }


    EdsSystemContext::create4TestsOnly(p.at(0) /*airline*/,
                                       p.at(1) /*remote edi address - to*/,
                                       p.at(2) /*our edi address - from*/,
                                       translit,
                                       h2hAddr,
                                       ourH2hAddr);

    // for compatibility
    set_edi_addrs(std::make_pair(p.at(2) /*from*/, p.at(1) /*to*/));
    return "";
}

static std::string FP_init_dcs(const std::vector<std::string> &p)
{
    using namespace Ticketing::RemoteSystemContext;

    assert(p.size() > 2);

    std::string airAddr = "",
    ourAirAddr = "";
    if(p.size() > 4) {
        airAddr = p.at(3);
        ourAirAddr = p.at(4);
    }

    std::string h2hAddr = "",
    ourH2hAddr = "";
    if(p.size() > 6) {
        h2hAddr = p.at(5);
        ourH2hAddr = p.at(6);
    }
    DcsSystemContext::create4TestsOnly(p.at(0) /*airline*/,
                                       p.at(1) /*remote edi address - to*/,
                                       p.at(2) /*our edi address - from*/,
                                       airAddr, /*remote airimp address - to */
                                       ourAirAddr, /*our airimp address - from */
                                       h2hAddr,
                                       ourH2hAddr);

    return "";
}

std::string FP_make_spp(const std::vector<std::string> &p)
{
    boost::gregorian::date d = boost::posix_time::second_clock::local_time().date();

    std::string fmt = "%d%m%y";
    if(p.size() > 1) {
        fmt = p.at(1);
    }
    if(p.size() > 0) {
        d = HelpCpp::date_cast(p.at(0).c_str(), fmt.c_str());
    }

    CreateSPP(BASIC::date_time::BoostToDateTime(d));

    return "";
}

static std::string FP_run_daemon(const std::vector<std::string> &params) {
    assert(params.size() > 0);
    if(params.at(0) == "edi_timeout") {
        runEdiTimer_4testsOnly();
    }
    return "";
}

static std::vector<std::string> pointIdSppVector()
{
    std::vector<std::string> res;
    std::string point_id_spp;
    OciCpp::CursCtl cur = make_curs("select POINT_ID_SPP from TLG_BINDING order by POINT_ID_TLG desc");

    cur.
        def(point_id_spp).
        exec();
    while(!cur.fen()) {
        res.push_back(point_id_spp);
    }

    if(res.empty()) {
        throw EXCEPTIONS::Exception("Empty table: TLG_BINDING!");
    }

    return res;
}

static std::string lastPointIdSpp(size_t pos)
{
    std::vector<std::string> res = pointIdSppVector();
    return (res.empty() || (res.size() - 1) < pos) ? "" : res.at(pos);
}

static std::string FP_last_point_id_spp(const std::vector<std::string>& p)
{
    if (p.size() > 0) {
        return lastPointIdSpp(std::stoi(p.at(0)));
    } else {
        return lastPointIdSpp(0);
    }
}

static std::string getNextTripPointId(int pointId)
{
    TTripRoute route;
    route.GetRouteAfter(ASTRA::NoExists,
                        pointId,
                        trtNotCurrent, trtNotCancelled);
    return (route.empty() ? "" : std::to_string(route.front().point_id));
}

static int getMoveId(int pointId)
{
   OciCpp::CursCtl cur = make_curs(
"select MOVE_ID from POINTS where POINT_ID=:point_id");

   int move_id = 0;
   cur.bind(":point_id", pointId)
      .def(move_id).exfet();
   return move_id;
}

static std::string FP_get_move_id(const std::vector<std::string>& p)
{
    assert(p.size() > 0);
    int move_id = getMoveId(std::stoi(p.at(0)));
    return std::to_string(move_id);
}

static std::string FP_get_next_trip_point_id(const std::vector<std::string>& p)
{
    assert(p.size() > 0);
    return getNextTripPointId(std::stoi(p.at(0)));
}

static std::string FP_get_dep_point_id(const std::vector<std::string>& p)
{
    assert(p.size() == 4);
    PointId_t point_id = astra_api::findDepPointId(p.at(0)/*airport*/,
                                                   p.at(1)/*airline*/,
                                                   std::stoi(p.at(2)),/*flight num*/
                                                   Dates::rrmmdd(p.at(3).c_str()));/*dep date*/
    return std::to_string(point_id.get());
}

static std::string FP_autoSetCraft(const std::vector<std::string>& p)
{
    assert(p.size() == 1);
    int point_id = std::stoi(p.at(0));
    SALONS2::AutoSetCraft(point_id);
    return "";
}

static std::string FP_getPaxId(const std::vector<std::string>& p)
{
    using namespace astra_api::xml_entities;
    assert(p.size() == 3);
    PointId_t pointDep(std::stoi(p.at(0)));
    Surname_t paxSurname(p.at(1));
    Name_t paxName(p.at(2));

    LogTrace(TRACE5) << __FUNCTION__;
    SearchPaxXmlResult spRes =
            astra_api::AstraEngine::singletone().SearchCheckInPax(pointDep,
                                                                  paxSurname,
                                                                  paxName);
    LogTrace(TRACE5) << __FUNCTION__;
    std::list<XmlTrip> lTrip = spRes.filterTrips(paxSurname.get(), paxName.get());
    assert(!lTrip.empty());
    const XmlTrip& frontTrip = lTrip.front();

    std::list<XmlPnr> lPnr = frontTrip.filterPnrs(paxSurname.get(), paxName.get());
    assert(!lPnr.empty());
    const XmlPnr& frontPnr = lPnr.front();

    std::list<XmlPax> lPax = frontPnr.filterPaxes(paxSurname.get(), paxName.get());
    assert(!lPax.empty());
    const XmlPax& pax = lPax.front();
    LogTrace(TRACE5) << __FUNCTION__;
    return std::to_string(pax.pax_id);
}

static std::string FP_getSingleGrpId(const std::vector<std::string>& p)
{
    using namespace astra_api::xml_entities;
    assert(p.size() == 3);
    PointId_t pointDep(std::stoi(p.at(0)));
    std::string paxSurname = p.at(1);
    std::string paxName = p.at(2);

    PaxListXmlResult plRes = astra_api::AstraEngine::singletone().PaxList(pointDep);
    std::list<XmlPax> lPax = plRes.applyNameFilter(paxSurname, paxName);

    assert(!plRes.lPax.empty());
    const XmlPax& pax = lPax.front();
    return std::to_string(pax.grp_id);
}

static std::string FP_getSinglePaxId(const std::vector<std::string>& p)
{
    using namespace astra_api::xml_entities;
    assert(p.size() == 3);
    PointId_t pointDep(std::stoi(p.at(0)));
    std::string paxSurname = p.at(1);
    std::string paxName = p.at(2);

    PaxListXmlResult plRes = astra_api::AstraEngine::singletone().PaxList(pointDep);
    std::list<XmlPax> lPax = plRes.applyNameFilter(paxSurname, paxName);

    assert(!plRes.lPax.empty());
    const XmlPax& pax = lPax.front();
    return std::to_string(pax.pax_id);
}

static std::string FP_getSingleTid(const std::vector<std::string>& p)
{
    using namespace astra_api::xml_entities;
    assert(p.size() == 3);
    PointId_t pointDep(std::stoi(p.at(0)));
    std::string paxSurname = p.at(1);
    std::string paxName = p.at(2);

    PaxListXmlResult plRes = astra_api::AstraEngine::singletone().PaxList(pointDep);
    std::list<XmlPax> lPax = plRes.applyNameFilter(paxSurname, paxName);
    assert(!plRes.lPax.empty());
    const XmlPax& pax = lPax.front();
    assert(pax.reg_no != ASTRA::NoExists);
    LoadPaxXmlResult lpRes = astra_api::AstraEngine::singletone().LoadPax(pointDep, RegNo_t(pax.reg_no));
    assert(!lpRes.lSeg.empty());
    const XmlSegment& paxSeg = lpRes.lSeg.front();
    return std::to_string(paxSeg.seg_info.tid);
}

static std::string FP_getSinglePaxTid(const std::vector<std::string>& p)
{
    using namespace astra_api::xml_entities;
    assert(p.size() == 3);
    PointId_t pointDep(std::stoi(p.at(0)));
    std::string paxSurname = p.at(1);
    std::string paxName = p.at(2);

    PaxListXmlResult plRes = astra_api::AstraEngine::singletone().PaxList(pointDep);
    std::list<XmlPax> lPax = plRes.applyNameFilter(paxSurname, paxName);
    assert(!plRes.lPax.empty());
    const XmlPax& pax = lPax.front();
    assert(pax.reg_no != ASTRA::NoExists);
    LoadPaxXmlResult lpRes = astra_api::AstraEngine::singletone().LoadPax(pointDep, RegNo_t(pax.reg_no));
    lpRes.applyPaxFilter(PaxFilter(NameFilter(paxSurname, paxName), {}, {}));
    assert(!lpRes.lSeg.empty());
    const XmlSegment& paxSeg = lpRes.lSeg.front();
    return std::to_string(paxSeg.passengers.front().tid);
}

static std::string FP_getIatciTabId(const std::vector<std::string>& p)
{
    assert(p.size() == 2);
    GrpId_t grpId(std::stoi(p.at(0)));
    unsigned tabInd = std::stoi(p.at(1));
    int id;

    auto cur = make_db_curs(
"select ID from IATCI_TABS where GRP_ID=:grp_id and TAB_IND=:tab_ind",
                PgOra::getROSession("IATCI_TABS"));

    cur
            .def(id)
            .bind(":grp_id", grpId.get())
            .bind(":tab_ind",tabInd)
            .EXfet();

    if(cur.err() == DbCpp::ResultCode::NoDataFound) {
        throw EXCEPTIONS::Exception("Iatci tab not found by grp_id=" + std::to_string(grpId.get())
                                    + " and tab_ind=" + std::to_string(tabInd));
    }

    return std::to_string(id);
}

static std::string FP_getPointTid(const std::vector<std::string>& p)
{
    assert(p.size() == 1);
    int pointId = std::stoi(p.at(0));
    int tid = 0;
    OciCpp::CursCtl cur = make_curs(
"select TID from POINTS where POINT_ID=:point_id");
    cur.def(tid)
       .bind(":point_id", pointId)
       .EXfet();
    return std::to_string(tid);
}

static std::string FP_get_lat_code(const std::vector<std::string>& p)
{
    assert(p.size() == 2);
    std::string table = p.at(0),
                 code = p.at(1);
    if(table == "awk") {
        return BaseTables::Company(code)->lcode();
    } else if(table == "sfe") {
        return BaseTables::City(code)->lcode();
    } else if(table == "aer") {
        return BaseTables::Port(code)->lcode();
    } else {
        return "";
    }
}

static std::string FP_getElemId(const std::vector<std::string>& p)
{
    assert(p.size() == 2);
    std::string type = p.at(0),
                elem = p.at(1);
    TElemFmt fmt=efmtUnknown;
    return ElemToElemId(DecodeElemType(type.c_str()), elem, fmt);
}

static std::string FP_getRandomBpTypeCode(const std::vector<std::string>& p)
{
    OciCpp::CursCtl cur = make_curs(
"select CODE from BP_TYPES where CODE in ('TST') AND op_type='PRINT_BP' order by dbms_random.random");
    std::string code;
    cur.def(code).exfet();
    return code;
}

static std::string FP_settcl(const std::vector<std::string>& par)
{
    ASSERT(par.size() == 2);
    setTclVar(par.at(0), par.at(1));
    return std::string();
}

static std::string FP_lastGeneratedPaxId(const std::vector<std::string>& par)
{
    int lgpid = lastGeneratedPaxId();
    if(lgpid == ASTRA::NoExists) {
        return "";
    }

    return std::to_string(lgpid);
}

static std::string FP_substr(const std::vector<std::string>& par)
{
    ASSERT(par.size() == 3);
    return par.at(0).substr(std::stoi(par.at(1)), std::stoi(par.at(2)));
}

static std::string FP_setDeskVersion(const std::vector<std::string>& par)
{
    ASSERT(par.size() > 0);
    std::string sql = "update DESKS set VERSION=:version";
    if(par.size() > 1) {
        sql += " where CODE=:code";
    }

    auto cur = make_curs(sql);
    cur.bind(":version", par.at(0));

    if(par.size() > 1) {
        cur.bind(":code", par.at(1));
    }

    cur.exec();

    return "";
}

static std::string FP_setUserTime(const std::vector<std::string>& par)
{
    ASSERT(par.size() == 2);
    const std::string userTimeType = par.at(0);
    int time;
    if(userTimeType == "UTC") {
        time = ustTimeUTC;
    } else if(userTimeType == "LocalDesk") {
        time = ustTimeLocalDesk;
    } else if(userTimeType == "LocalAirp") {
        time = ustTimeLocalAirp;
    } else {
        throw EXCEPTIONS::Exception("Unknown user time type!");
    }

    for(bool insert : {false, true})
    {

      std::string sql = insert?
        "INSERT INTO user_sets(user_id, time) SELECT user_id, :time FROM users2 WHERE login=:login":
        "UPDATE user_sets SET time=:time WHERE user_id IN (SELECT user_id FROM users2 WHERE login=:login)";

      auto cur = make_curs(sql);
      cur.bind(":time", time)
         .bind(":login", par.at(1))
         .exec();

      if (cur.rowcount()>0) break;
    }

    return "";
}


static std::string FP_initApps(const std::vector<tok::Param>& par)
{
    ASSERT(par.size() > 2);
    std::string airline = PositionalValues(par).at(0);
    std::string country = PositionalValues(par).at(1);
    std::string format  = PositionalValues(par).at(2);

    bool inbound    = tok::GetValue(par, "inbound",  "true")    == "true";
    bool outbound   = tok::GetValue(par, "outbound", "true")    == "true";
    bool closeout   = tok::GetValue(par, "closeout", "true")    == "true";
    bool prdenial   = tok::GetValue(par, "denial",   "false")   == "true";
    bool precheckin = tok::GetValue(par, "pre_checkin", "true")  == "true";
    LogTrace(TRACE1) << "init_apps: "
                     << "airline: "    << airline    << "; "
                     << "country: "    << country    << "; "
                     << "format: "     << format     << "; "
                     << "inbound: "    << inbound    << "; "
                     << "outbound: "   << outbound   << "; "
                     << "closeout: "   << closeout   << "; "
                     << "prdenial: "   << prdenial   << "; "
                     << "precheckin: " << precheckin << "; ";


    make_curs(
"delete from APPS_SETS where AIRLINE=:airline and APPS_COUNTRY=:country")
            .bind(":airline", airline)
            .bind(":country", country)
            .exec();

    make_curs(
"insert into APPS_SETS(AIRLINE, APPS_COUNTRY, FORMAT, FLT_CLOSEOUT, INBOUND, OUTBOUND, PR_DENIAL, PRE_CHECKIN, ID) "
"values(:airline, :country, :format, :closeout, :inbound, :outbound, :prdenial, :precheckin, id__seq.nextval)")
            .bind(":airline", airline)
            .bind(":country", country)
            .bind(":format",  format)
            .bind(":closeout",closeout)
            .bind(":inbound", inbound)
            .bind(":outbound",outbound)
            .bind(":prdenial",prdenial)
            .bind(":precheckin", precheckin)
            .exec();
    return "";
}

static std::string FP_translit(const std::vector<std::string>& par)
{
    ASSERT(par.size() == 1);
    return StrUtils::translit(par.at(0));
}

void checkPresenceTask(const PointId_t& pointId, const std::string & taskName)
{
    auto tasks = getFlightTasks("TRIP_TASKS", pointId);
    std::string nameOfTask;
    if(taskName == "send_apps") {
        nameOfTask = "SEND_NEW_APPS_INFO";
    } else if(taskName == "send_all_apps") {
        nameOfTask = "SEND_ALL_APPS_INFO";
    } else if(taskName == "check_trip_alarms") {
        nameOfTask = "CHECK_ALARM";
    } else {
        LogTrace(TRACE3) << __FUNCTION__ << " Unknown task: " << taskName;
        ASSERT(false);
    }
    if(find(tasks.begin(), tasks.end(), nameOfTask) == tasks.end()) {
        LogTrace(TRACE3) << __FUNCTION__ << " Not find task: " << nameOfTask;
        ASSERT(false);
    }
}

static std::string FP_run_trip_task(const std::vector<std::string>& par)
{
    ASSERT(par.size() <= 3);
    const std::string taskName = par.at(0);
    PointId_t pointId(std::stoi(par.at(1)));
    std::string checkPresenceOfTheTask = "check";
    if(par.size() == 3)  {
        checkPresenceOfTheTask = par.at(2);
    }
    if(checkPresenceOfTheTask == "check") {
        checkPresenceTask(pointId, taskName);
    }

    try{
        if(taskName == "send_apps") {
            APPS::sendNewInfo(TTripTaskKey(pointId, "SEND_NEW_APPS_INFO", ""));
        } else if(taskName == "send_all_apps") {
            APPS::sendAllInfo(TTripTaskKey(pointId, "SEND_ALL_APPS_INFO", ""));
        } else if(taskName == "check_trip_alarms") {
            checkAlarm(TTripTaskKey(pointId.get(), "CHECK_ALARM" , "APPS_NOT_SCD_IN_TIME"));
        }
    }
    catch(EXCEPTIONS::Exception &E) {
        LogTrace(TRACE5) << " Continue testing after throw: " << E.what();
    }
    return "";
}

void updateAppsMsg(int msg_id, Dates::DateTime_t send_time, int send_attempts)
{
    //LogTrace(TRACE5) << __FUNCTION__ << " send_time: " << send_time << " send_attempts:" << send_attempts;
    auto cur = make_curs(
               "update APPS_MESSAGES "
               "set SEND_ATTEMPTS = :send_attempts, SEND_TIME = :send_time "
               "where MSG_ID = :msg_id ");
    cur
        .bind(":send_attempts", send_attempts)
        .bind(":send_time", send_time)
        .bind(":msg_id", msg_id)
        .exec();
}

static std::string FP_runUpdateMsg(const std::vector<std::string>& par)
{
    ASSERT(par.size() == 3);
    int msg_id = std::stoi(par.at(0));
    int send_time_after = std::stoi(par.at(1));
    int send_attempts = std::stoi(par.at(2));
    //auto msg = APPS::APPSMessage::readAppsMsg(msg_id);
    Dates::DateTime_t send_time = Dates::second_clock::universal_time() - seconds(send_time_after);

    LogTrace(TRACE5) << __FUNCTION__ << " time: " << HelpCpp::string_cast(send_time, "%H%M%S");
    updateAppsMsg(msg_id, send_time, send_attempts);
    return "";
}

static std::string FP_runResendTlg(const std::vector<std::string>& par)
{
    resend_tlg();
    return "";
}

static std::string FP_dump_db_table(const std::vector<tok::Param>& params)
{
    ASSERT(params.size() > 0);
    std::string tableName = params[0].value;

    std::string db = "pg";
    for (size_t i = 1; i < params.size(); ++i) {
        if(params[i].name == "db") {
            db = params[i].value;
        }
    }

    std::string dump;
    if(db == "pg") {
        DbCpp::DumpTable(*get_main_pg_rw_sess(STDLOG), tableName).exec(dump);
    } else if(db == "ora") {
        DbCpp::DumpTable(*get_main_ora_sess(STDLOG), tableName).exec(dump);
    }
    LogTrace(TRACE3) << dump;
    return "";
}

static std::vector<string> getPaxAlarms(const std::string& table_name, int pax_id)
{
    std::string alarm;
    auto cur = make_curs(
"select ALARM_TYPE from "+table_name+" where PAX_ID=:pax_id");
    cur.def(alarm)
       .bind(":pax_id",pax_id)
       .exec();
    std::vector<std::string> res;
    while(!cur.fen()){
        res.push_back(alarm);
    }
    return res;
}

static std::string formatPaxAlarms(const std::vector<std::string>& alarms)
{
    std::ostringstream oss;
    for(const auto & alarm : alarms){
        oss << alarm << '\n';
    }
    return oss.str();
}

static std::vector<string> getTripAlarms(const std::string& table_name, int point_id)
{
    std::string alarm;
    auto cur = make_curs(
               "select ALARM_TYPE from "+table_name+" where POINT_ID=:point_id");
    cur.def(alarm)
            .bind(":point_id", point_id)
            .exec();
    std::vector<std::string> res;
    while(!cur.fen()) {
        res.push_back(alarm);
    }
    return res;
}

static std::vector<string> getFlightTasks(const std::string& table_name, const PointId_t& point_id)
{
    std::string task;
    auto cur = make_curs(
               "select NAME from "+table_name+" where POINT_ID=:point_id order by ID");
    cur.def(task)
            .bind(":point_id", point_id.get())
            .exec();
    std::vector<std::string> res;
    while(!cur.fen()) {
        res.push_back(task);
    }
    return res;
}

static std::string formatTripAlarms(const std::vector<std::string>& alarms)
{
    std::ostringstream oss;
    for(const auto & alarm : alarms){
        oss << alarm << '\n';
    }
    return oss.str();
}

static std::string FP_checkPaxAlarms(const std::vector<std::string>& par)
{
    ASSERT(par.size() == 1);
    int pax_id = std::stoi(par.at(0));
    return formatPaxAlarms(getPaxAlarms("PAX_ALARMS", pax_id));
}

static std::string FP_checkCrsPaxAlarms(const std::vector<std::string>& par)
{
    ASSERT(par.size() == 1);
    int pax_id = std::stoi(par.at(0));
    return formatPaxAlarms(getPaxAlarms("CRS_PAX_ALARMS", pax_id));
}

static std::string FP_checkTripAlarms(const std::vector<std::string>& par)
{
    ASSERT(par.size() == 1);
    int point_id = std::stoi(par.at(0));
    return formatTripAlarms(getTripAlarms("TRIP_ALARMS", point_id));
}

static std::string FP_checkFlightTasks(const std::vector<std::string>& par)
{
    ASSERT(par.size() == 1);
    PointId_t point_id(std::stoi(par.at(0)));
    return formatTripAlarms(getFlightTasks("TRIP_TASKS", point_id));
}

static std::string FP_runUpdateCoupon(const std::vector<std::string>& par)
{
    ASSERT(par.size() == 3);
    int status = std::stoi(par.at(0));
    std::string ticknum = par.at(1);
    int num = std::stoi(par.at(2));
    auto cur = make_db_curs(
"UPDATE WC_COUPON set STATUS=:status where TICKNUM=:ticknum and NUM=:num",
                PgOra::getRWSession("WC_COUPON"));
    cur
            .bind(":status",  status)
            .bind(":ticknum", ticknum)
            .bind(":num",     num)
            .exec();
    return "";
}

static std::string FP_initIapiRequestId(const std::vector<std::string> &par)
{
  ASSERT(par.size() == 1);
  IAPI::initRequestIdGenerator(std::stoi(par.at(0)));
  return "";
}

static std::string FP_getBCBP(const std::vector<std::string> &par)
{
  ASSERT(par.size() >= 2);
  GrpId_t grpId(std::stoi(par.at(0)));
  PaxId_t paxId(std::stoi(par.at(1)));
  bool langRu=(par.size() >= 3) && (par.at(2) == AstraLocale::LANG_RU);

  TPrnTagStore pts(TDevOper::PrnBP, grpId.get(), paxId.get(), false, false, nullptr);

  if (langRu)
    return pts.get_tag(TAG::BCBP_M_2);
  else
    return pts.get_tag(TAG::BCBP_M_2, {}, "E");
}

static std::string FP_cache(const std::vector<std::string> &par)
{
  CacheTableTermRequest cacheRequest(par);

  return cacheRequest.getXml();
}

static std::string FP_getCacheIfaceVer(const std::vector<std::string> &par)
{
  ASSERT(par.size() == 1);
  boost::optional<int> ifaceVersion=CacheTableTermRequest::getInterfaceVersion(par[0]);

  if (ifaceVersion) return std::to_string(ifaceVersion.get());

  return "";
}

static std::string FP_getCacheSQLParam(const std::vector<std::string> &par)
{
  return CacheTableTermRequest::getSQLParamXml(par);
}

static std::string FP_runEtFltTask(const std::vector<std::string> &par)
{
  ETCheckStatusFlt();
  return "";
}

FP_REGISTER("<<", FP_tlg_in);
FP_REGISTER("!!", FP_req);
FP_REGISTER("astra_hello", FP_astra_hello);
FP_REGISTER("last_edifact_ref", FP_last_edifact_ref);
FP_REGISTER("last_point_id_spp", FP_last_point_id_spp);
FP_REGISTER("get_next_trip_point_id", FP_get_next_trip_point_id);
FP_REGISTER("get_dep_point_id", FP_get_dep_point_id);
FP_REGISTER("get_move_id", FP_get_move_id);
FP_REGISTER("init_jxt_pult", FP_init_jxt_pult);
FP_REGISTER("init", FP_init);
FP_REGISTER("lastRedisplay", FP_lastRedisplay);
FP_REGISTER("init_eds",   FP_init_eds);
FP_REGISTER("init_dcs",   FP_init_dcs);
FP_REGISTER("make_spp", FP_make_spp);
FP_REGISTER("run_daemon", FP_run_daemon);
FP_REGISTER("auto_set_craft", FP_autoSetCraft);
FP_REGISTER("get_pax_id", FP_getPaxId);
FP_REGISTER("get_single_grp_id", FP_getSingleGrpId);
FP_REGISTER("get_single_pax_id", FP_getSinglePaxId);
FP_REGISTER("get_single_tid", FP_getSingleTid);
FP_REGISTER("get_single_pax_tid",FP_getSinglePaxTid);
FP_REGISTER("get_iatci_tab_id", FP_getIatciTabId);
FP_REGISTER("get_point_tid", FP_getPointTid);
FP_REGISTER("get_lat_code", FP_get_lat_code);
FP_REGISTER("get_elem_id", FP_getElemId);
FP_REGISTER("get_random_bp_type", FP_getRandomBpTypeCode);
FP_REGISTER("settcl", FP_settcl);
FP_REGISTER("last_generated_pax_id", FP_lastGeneratedPaxId);
FP_REGISTER("substr", FP_substr);
FP_REGISTER("set_desk_version", FP_setDeskVersion);
FP_REGISTER("set_user_time_type", FP_setUserTime);
FP_REGISTER("init_apps", FP_initApps);
FP_REGISTER("translit", FP_translit);
FP_REGISTER("run_trip_task", FP_run_trip_task);
FP_REGISTER("check_pax_alarms", FP_checkPaxAlarms);
FP_REGISTER("check_crs_pax_alarms", FP_checkCrsPaxAlarms);
FP_REGISTER("check_trip_alarms", FP_checkTripAlarms);
FP_REGISTER("check_flight_tasks", FP_checkFlightTasks);
FP_REGISTER("update_msg", FP_runUpdateMsg);
FP_REGISTER("resend", FP_runResendTlg);
FP_REGISTER("update_pg_coupon", FP_runUpdateCoupon);
FP_REGISTER("init_iapi_request_id", FP_initIapiRequestId);
FP_REGISTER("get_bcbp", FP_getBCBP);
FP_REGISTER("cache", FP_cache);
FP_REGISTER("cache_iface_ver", FP_getCacheIfaceVer);
FP_REGISTER("cache_sql_param", FP_getCacheSQLParam);
FP_REGISTER("dump_db_table", FP_dump_db_table);
FP_REGISTER("run_et_flt_task", FP_runEtFltTask);

#endif /* XP_TESTING */
