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
#include "crafts/ComponCreator.h"
#include "date_time.h"
#include "apps_interaction.h"
#include "alarms.h"
#include "xml_unit.h"
#include "pg_session.h"
#include "stat/stat_main.h"
#include "tlg/tlg.h"
#include "tlg/remote_system_context.h"
#include "tlg/edi_tlg.h"
#include "tlg/apps_handler.h"
#include "arx_daily_pg.h"
#include "dbostructures.h"
#include "iapi_interaction.h"
#include "prn_tag_store.h"
#include "PgOraConfig.h"
#include "cr_lf.h"
#include "cache.h"
#include "timer.h"
#include "stat/stat_departed.h"
#include "basel_aero.h"
#include "passenger.h"
#include "trip_tasks.h"
#include "hist.h"

#include <queue>
#include <fstream>
#include <sstream>
#include <regex>
#include <boost/foreach.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/algorithm/string.hpp>

#include <serverlib/tscript.h>
#include <serverlib/exception.h>
#include <serverlib/func_placeholders.h>
#include <serverlib/posthooks.h>
#include <serverlib/dbcpp_cursctl.h>
#include <serverlib/dates_oci.h>
#include <serverlib/str_utils.h>
#include <serverlib/tcl_utils.h>
#include <serverlib/dates_io.h>
#include <serverlib/dump_table.h>
#include <serverlib/pg_rip.h>
#include <jxtlib/jxtlib.h>
#include <jxtlib/utf2cp866.h>

#define NICKNAME "ROMAN"
#include <serverlib/slogger.h>

void tests_init() {}

namespace xp_testing {
    void runEdiTimer_4testsOnly();
    void runLibraLogEventsHandler_4testsOnly();
    void runLibraLogEventsCleaner_4testsOnly();
}//namespace xp_testing

static std::vector<string> getFlightTasks(const std::string& table_name, const PointId_t &point_id);

using namespace xp_testing;
using namespace xp_testing::tscript;

static std::string LF2CRLF(const string &par)
{
    return regex_replace(par, regex(LF), CR + LF) + CR + LF;
}

static std::string CRLF2LF(const string &par)
{
    return regex_replace(par, regex(CR + LF), LF) + LF;
}

static std::string executeHttpRequest(const std::string& heading, const std::string &body)
{
    static const std::string ContentLength="Content-Length";

    ServerFramework::HTTP::request_parser parser;
    ServerFramework::HTTP::request rq;
    const auto result=parser.parse(rq, heading.begin(), heading.end());

    if (!boost::get<0>(result))
      throw EXCEPTIONS::Exception("executeHttpRequest: wrong heading");

    const auto cl = std::find(rq.headers.begin(), rq.headers.end(), ContentLength);
    if (cl != rq.headers.end())
      cl->value = std::to_string(body.size());
    else
      rq.headers.push_back({ContentLength, std::to_string(body.size())});

    string answer;
    ServerFramework::http_main_for_test(rq.to_string()+body, answer);
    return answer;
}

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
    const std::string http_heading = tok::GetValue(params, "http_heading", "");

    if(!http_heading.empty()) {
        reply = executeHttpRequest(LF2CRLF(http_heading), req);
        if (capture == "on") {
            size_t pos=reply.find("\r\n\r\n");
            if (pos!=string::npos)
            {
              pos+=4;
              if (reply.compare(pos, 15, "<!doctype html>")==0)
              {
                //�� html
                reply = CRLF2LF(reply);
                reply = UTF8toCP866(reply);
              }
              if (reply.compare(pos, 6, "<?xml ")==0)
              {
                //�� xml
                reply = CRLF2LF(reply);
                reply = UTF8toCP866(reply);
                reply = StrUtils::replaceSubstrCopy(reply, "\"", "'");
                reply = StrUtils::replaceSubstrCopy(reply, "encoding='UTF-8'", "encoding='CP866'");
              }
            }
            outq.push(reply);
        }
    } else {
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
    tok::ValidateParams(params, 1, 1, "err ignore pages capture ws http_heading");
    const std::string text = tok::PositionalValues(params).at(0);

    if (text.empty()) {
      if (tok::GetValue(params, "http_heading", "").empty())
      {
        LogTrace(TRACE5) << __FUNCTION__ << ": skipping empty request";
        return std::string();
      }
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
    auto cur = get_pg_curs("select ourcarf from edisession order by last_access desc, ida desc");
    cur
            .def(ref)
            .exec();
    size_t pos = 0;
    while(!cur.fen()) {
        LogTrace(TRACE3) << "ref[" << pos++ << "]=" << ref;
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

    ExecuteTlg(params); /* ����� �� �室 */
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
    } else if(params.at(0) == "libra_log_events_handler") {
        runLibraLogEventsHandler_4testsOnly();
    } else if(params.at(0) == "libra_log_events_cleaner") {
        runLibraLogEventsCleaner_4testsOnly();
    }
    return "";
}

namespace {

struct PointsPair
{
    PointId_t    pointIdSpp;
    PointIdTlg_t pointIdTlg;
};

}//namespace

static std::vector<PointsPair> pointsPairVector()
{
    std::vector<PointsPair> res;
    PointId_t::base_type point_id_spp;
    PointIdTlg_t::base_type point_id_tlg;
    auto cur = make_db_curs(
        "select POINT_ID_SPP, POINT_ID_TLG from TLG_BINDING order by POINT_ID_TLG desc",
        PgOra::getROSession("TLG_BINDING"));

    cur
            .def(point_id_spp)
            .def(point_id_tlg)
            .exec();
    while(!cur.fen()) {
        res.push_back({PointId_t(point_id_spp),
                       PointIdTlg_t(point_id_tlg)});
    }

    if(res.empty()) {
        throw EXCEPTIONS::Exception("Empty table: TLG_BINDING!");
    }

    return res;
}

static std::string lastPointIdSpp(size_t pos)
{
    auto res = pointsPairVector();
    if((res.empty() || (res.size() - 1) < pos)) {
        return "";
    }
    return std::to_string(res.at(pos).pointIdSpp.get());
}

static std::string FP_last_point_id_spp(const std::vector<std::string>& p)
{
    if (p.size() > 0) {
        return lastPointIdSpp(std::stoi(p.at(0)));
    } else {
        return lastPointIdSpp(0);
    }
}

static std::string lastPointIdTlg(size_t pos)
{
    auto res = pointsPairVector();
    if((res.empty() || (res.size() - 1) < pos)) {
        return "";
    }
    return std::to_string(res.at(pos).pointIdTlg.get());
}

static std::string FP_last_point_id_tlg(const std::vector<std::string>& p)
{
    if (p.size() > 0) {
        return lastPointIdTlg(std::stoi(p.at(0)));
    } else {
        return lastPointIdTlg(0);
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
   auto cur = make_db_curs(
"select MOVE_ID from POINTS where POINT_ID=:point_id", PgOra::getROSession("POINTS"));

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
    ComponCreator::AutoSetCraft( point_id );
    return "";
}

static std::string FP_getPaxId(const std::vector<std::string>& p)
{
  using namespace astra_api::xml_entities;
  assert(p.size() == 2 || p.size() == 3);
  PointId_t pointDep(std::stoi(p.at(0)));

  if (p.size() == 3)
  {
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
  else
  {
    RegNo_t regNo(std::stoi(p.at(1)));

    auto cur = make_db_curs("SELECT pax.pax_id "
                            "FROM pax_grp, pax "
                            "WHERE pax_grp.grp_id=pax.grp_id AND "
                            "      pax_grp.point_dep=:point_dep AND pax.reg_no=:reg_no "
                            "ORDER BY pax.pax_id",
                            PgOra::getROSession({"PAX_GRP", "PAX"})); //����� ���� �� � ॣ. ����஬ ���᫮��
    int pax_id;
    cur
        .stb()
        .def(pax_id)
        .bind(":point_dep", pointDep.get())
        .bind(":reg_no", regNo.get())
        .exfet(); //����� ���� �� � ॣ. ����஬ ���᫮�� (�㡫�஢���� regNo)

    if(cur.err() == DbCpp::ResultCode::NoDataFound) return "";

    return std::to_string(pax_id);
  }
}

static std::string FP_getUnaccompId(const std::vector<std::string>& p)
{
  assert(p.size() == 2);
  PointId_t pointDep(std::stoi(p.at(0)));
  int rowNumRequired=std::stoi(p.at(1));

  ASSERT(rowNumRequired>=1);

  auto cur = make_db_curs("SELECT grp_id "
                          "FROM pax_grp "
                          "WHERE point_dep=:point_dep AND class IS NULL AND status NOT IN ('E') "
                          "ORDER BY grp_id",
                          PgOra::getROSession("PAX_GRP"));

  int grp_id;
  cur
      .stb()
      .def(grp_id)
      .bind(":point_dep", pointDep.get())
      .exec();

  int rowNum=0;
  while (!cur.fen()) {
    if ((++rowNum)==rowNumRequired) return std::to_string(grp_id);
  }

  return "";
}

static std::string FP_getSingleGrpId(const std::vector<std::string>& p)
{
  assert(p.size() == 1 || p.size() == 3);

  if (p.size() == 3)
  {
    using namespace astra_api::xml_entities;
    PointId_t pointDep(std::stoi(p.at(0)));
    std::string paxSurname = p.at(1);
    std::string paxName = p.at(2);

    PaxListXmlResult plRes = astra_api::AstraEngine::singletone().PaxList(pointDep);
    std::list<XmlPax> lPax = plRes.applyNameFilter(paxSurname, paxName);

    assert(!plRes.lPax.empty());
    const XmlPax& pax = lPax.front();
    return std::to_string(pax.grp_id);
  }
  else
  {
    CheckIn::TSimplePaxItem pax;
    LogTrace(TRACE5) << __func__ << " " << p.at(0);
    PaxId_t paxId(std::stoi(p.at(0)));
    assert(pax.getByPaxId( paxId.get() ));
    return std::to_string(pax.grp_id);
  }
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

static std::string FP_getSingleGrpTid(const std::vector<std::string>& p)
{
  assert(p.size() == 1 || p.size() == 3);

  if (p.size() == 3)
  {
    using namespace astra_api::xml_entities;
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
  else
  {
    CheckIn::TSimplePaxGrpItem grp;
    PaxId_t paxId(std::stoi(p.at(0)));
    assert(grp.getByPaxId( paxId.get() ));
    return std::to_string(grp.tid);
  }
}

static std::string FP_getSinglePaxTid(const std::vector<std::string>& p)
{
  assert(p.size() == 1 || p.size() == 3);

  if (p.size() == 3)
  {
    using namespace astra_api::xml_entities;
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
  else
  {
    CheckIn::TSimplePaxItem pax;
    PaxId_t paxId(std::stoi(p.at(0)));
    assert(pax.getByPaxId( paxId.get() ));
    return std::to_string(pax.tid);
  }
}

static std::string FP_getUnaccompTid(const std::vector<std::string>& p)
{
  assert(p.size() == 1);

  CheckIn::TSimplePaxGrpItem grp;
  GrpId_t grpId(std::stoi(p.at(0)));
  assert(grp.getByGrpId( grpId.get() ));
  return std::to_string(grp.tid);
}

static std::string FP_getCrsPaxUniqRef(const std::vector<std::string>& p)
{
    assert(p.size() == 3);
    PointIdTlg_t pointDep(std::stoi(p.at(0)));
    Surname_t surname(p.at(1));
    Name_t name(p.at(2));

    LogTrace(TRACE3) << __FUNCTION__ << " by "
                     << pointDep << "/"
                     << surname << "/" << name;

    std::string uniqRef;
    auto cur = make_db_curs(
          "select UNIQUE_REFERENCE from CRS_PAX, CRS_PNR "
          "where CRS_PAX.PNR_ID=CRS_PNR.PNR_ID "
          "and CRS_PAX.SURNAME=:surname and CRS_PAX.NAME=:name "
          "and CRS_PNR.POINT_ID=:point_dep",
          PgOra::getROSession({"CRS_PAX", "CRS_PNR"}));
    cur
        .stb()
        .defNull(uniqRef, "")
        .bind(":point_dep", pointDep.get())
        .bind(":surname", surname.get())
        .bind(":name", name.get())
        .EXfet();

    return uniqRef;
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
        .stb()
        .def(id)
        .bind(":grp_id", grpId.get())
        .bind(":tab_ind", tabInd)
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
    auto cur = make_db_curs(
        "select TID from POINTS where POINT_ID=:point_id", PgOra::getROSession("POINTS"));
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
    auto cur = make_db_curs(
        "select CODE from BP_TYPES where CODE in ('TST') AND op_type='PRINT_BP' order by dbms_random.random",
        PgOra::getROSession("BP_TYPES"));
    std::string code;
    cur.def(code).exfet();
    return code;
}

static std::string FP_exec_stage(const std::vector<std::string>& par)
{
    ASSERT(par.size() == 2);
    exec_stage(std::stoi(par.at(0)), DecodeStage(par.at(1).c_str()));
    return std::string();
}

static std::string FP_settcl(const std::vector<std::string>& par)
{
    ASSERT(par.size() == 2);
    setTclVar(par.at(0), par.at(1));
    return std::string();
}

static std::string FP_gettcl(const std::vector<std::string>& par)
{
    ASSERT(par.size() == 1);
    return readStringFromTcl(par.at(0));
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

    auto cur = make_db_curs(sql, PgOra::getRWSession("DESKS"));
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

      auto cur = make_db_curs(sql, PgOra::getRWSession({"USER_SETS", "USERS2"}));
      cur.bind(":time", time)
         .bind(":login", par.at(1))
         .exec();

      if (cur.rowcount()>0) break;
    }

    return "";
}

static std::string FP_getUserId(const std::vector<std::string>& par)
{
    ASSERT(par.size() == 1);

    auto cur = make_db_curs("SELECT user_id FROM users2 WHERE login=:login",
                            PgOra::getROSession("USERS2"));

    int user_id;
    cur.def(user_id)
       .bind(":login", par.at(0))
       .EXfet();

    if(cur.err() == DbCpp::ResultCode::NoDataFound) return "";

    return std::to_string(user_id);
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

    make_db_curs(
        "delete from APPS_SETS where AIRLINE=:airline and APPS_COUNTRY=:country",
        PgOra::getRWSession("APPS_SETS"))
        .bind(":airline", airline)
        .bind(":country", country)
        .exec();

    make_db_curs(
"insert into APPS_SETS(AIRLINE, APPS_COUNTRY, FORMAT, FLT_CLOSEOUT, INBOUND, OUTBOUND, PR_DENIAL, PRE_CHECKIN, ID) "
"values(:airline, :country, :format, :closeout, :inbound, :outbound, :prdenial, :precheckin, id__seq.nextval)",
PgOra::getRWSession("APPS_SETS"))
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
    } else if(taskName == "check_alarm_apps_scdtime" || taskName == "check_alarm_apps_problem") {
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

static std::string FP_kick_flt_tasks_daemon(const std::vector<std::string>& par)
{
  string handler_id=all_other_handler_id;
  if (par.size()>0) handler_id=par.at(0);

  check_trip_tasks(handler_id);

  return "";
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
        } else if(taskName == "check_alarm_apps_scdtime") {
            checkAlarm(TTripTaskKey(pointId, "CHECK_ALARM" , "APPS_NOT_SCD_IN_TIME"));
        } else if(taskName == "check_alarm_apps_problem") {
            checkAlarm(TTripTaskKey(pointId, "CHECK_ALARM" , "APPS_PROBLEM"));
        }
    }
    catch(EXCEPTIONS::Exception &E) {
        LogTrace(TRACE5) << " Continue testing after throw: " << E.what();
    }
    return "";
}

static std::string FP_runUpdateMsg(const std::vector<std::string>& par)
{
    ASSERT(par.size() == 3);
    int msg_id = std::stoi(par.at(0));
    int send_time_after = std::stoi(par.at(1));
    int send_attempts = std::stoi(par.at(2));
    Dates::DateTime_t send_time = Dates::second_clock::universal_time() - seconds(send_time_after);

    LogTrace(TRACE5) << __FUNCTION__ << " time: " << HelpCpp::string_cast(send_time, "%H%M%S");
    APPS::updateAppsMsg(msg_id, send_time, send_attempts);
    return "";
}

static std::string FP_runResendTlg(const std::vector<std::string>& par)
{
    resend_tlg();
    return "";
}

static std::string FP_dumpAppsData(const std::vector<std::string>& par)
{
    APPS::dumpAppsPaxData();
    return "";
}

static std::string FP_dumpAppsMessages(const std::vector<std::string>& par)
{
    APPS::dumpAppsMsg();
    return "";
}

static std::string FP_dump_table_astra(const std::vector<tok::Param> &params)
{
    ASSERT(params.size() > 0);

    std::string tableName = params[0].value;
    std::string session, fields, where, order;
    bool display = false;
    for (size_t i = 1; i < params.size(); ++i) {
        if (params[i].name == "session") {
            session = params[i].value;
        } else if (params[i].name == "fields") {
            fields = params[i].value;
        } else if (params[i].name == "where") {
            where = params[i].value;
        } else if (params[i].name == "order") {
            order = params[i].value;
        } else if (params[i].name == "display") {
            if (params[i].value == "on") {
                display = true;
            }
        }
    }
    auto dt = std::make_shared<ServerFramework::DbDumpTable>(PgOra::getRWSession(tableName), tableName);

    if (!fields.empty()) {
        dt->addFld(fields);
    }
    dt->where(where);
    dt->order(order);
    std::string answer;
    if (display) {
        dt->exec(answer);
    } else {
        dt->exec(TRACE5);
    }

    return answer;
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


static std::string FP_check_dump(const std::vector<tok::Param>& params)
{
    ASSERT(params.size() > 0);
    std::string tableName = params[0].value;
    std::string fields, where, order;
    bool display = false;

    for (size_t i = 1; i < params.size(); ++i) {
        if (params[i].name == "fields") {
            fields = params[i].value;
        } else if (params[i].name == "where") {
            where = params[i].value;
        } else if (params[i].name == "order") {
            order = params[i].value;
        } else if (params[i].name == "display") {
            if (params[i].value == "on") {
                display = true;
            }
        }
    }
    std::string resultQuery;
    if(!where.empty()) {
        resultQuery += " where " + where;
    }
    if(!order.empty()) {
        resultQuery += " order by " + order;
    }
    return dbo::Session().dump(tableName, {}, resultQuery);
}


static std::vector<string> getPaxAlarms(const std::string& table_name, int pax_id)
{
    std::string alarm;
    auto cur = make_db_curs(
          "select ALARM_TYPE from "+table_name+" where PAX_ID=:pax_id",
          PgOra::getROSession(table_name));
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
    auto cur = make_db_curs(
               "select ALARM_TYPE from "+table_name+" where POINT_ID=:point_id",
                PgOra::getROSession(table_name));
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
    auto cur = make_db_curs(
               "select NAME from "+table_name+" where POINT_ID=:point_id order by ID",
               PgOra::getROSession(StrUtils::ToUpper(table_name)));
    cur
        .stb()
        .def(task)
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

static std::string FP_descTest(const std::vector<std::string>& par)
{
    ASSERT(par.size() == 1);
    std::cout << "Test #" << par.at(0) << std::endl;
    return "";
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

static std::string FP_runArchStep(const std::vector<std::string>& par)
{
    ASSERT(par.size() <= 2);
    int step = 1;
    //boost::gregorian::date d = boost::posix_time::second_clock::local_time().date();
    //std::string fmt = "%d%m%y";
    Dates::DateTime_t date = HelpCpp::time_cast(par.at(0).c_str(), "%d%m%y");
    if(par.size() == 2) {
        step = std::stoi(par.at(1));
    }

    //TDateTime utcdate = BASIC::date_time::BoostToDateTime(d);
    //LogTrace(TRACE5) << __FUNCTION__ << " time: " << DateTimeToStr( utcdate, "DD.MM.YYYY" ); //HelpCpp::string_cast(send_time, "%H%M%S");

    PG_ARX::test_arx_daily(date, step);

    return "";
}

static std::string FP_runArch(const std::vector<std::string>& par)
{
    ASSERT(par.size() == 1);
    Dates::DateTime_t date = HelpCpp::time_cast(par.at(0).c_str(), "%d%m%y");
    bool resultPg = PG_ARX::arx_daily(date);
    return "";
}

static std::string FP_collectFlightStat(const std::vector<std::string>& par)
{
    ASSERT(par.size() == 1);
    PointId_t point_id(std::stoi(par.at(0)));
    get_flight_stat(point_id.get(), true);
    return "";
}

static std::string FP_pg_sql(const std::vector<std::string> &args)
{
    assert(args.size() > 0);
    std::string sqlStr;
    for(const std::string& str: args) {
        sqlStr += str;
    }
    LogTrace(TRACE3) << "exec SQL: " << sqlStr;
    get_pg_curs(sqlStr).exec();
    return "";
}

static std::string FP_db_sql(const std::vector<std::string> &args)
{
    assert(args.size() > 1);

    std::string tableName = args.at(0);

    std::string sqlStr;
    for (size_t i = 1; i < args.size(); ++i)
      sqlStr += args.at(i);

    make_db_curs(sqlStr, PgOra::getRWSession(tableName)).exec();
    return "";
}

static std::string FP_db_seq_nextval(const std::vector<std::string> &args)
{
    assert(args.size() == 1);

    const std::string seqName = args.at(0);
    return std::to_string(PgOra::getSeqNextVal_int(seqName));
}

static std::string FP_db_seq_currval(const std::vector<std::string> &args)
{
    assert(args.size() == 1);

    const std::string seqName = args.at(0);
    return std::to_string(PgOra::getSeqCurrVal_int(seqName));
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

static std::string FP_getCacheDataVer(const std::vector<std::string> &par)
{
  ASSERT(par.size() == 1);
  boost::optional<int> dataVersion=CacheTableTermRequest::getDataVersion(par[0]);

  if (dataVersion) return std::to_string(dataVersion.get());

  return "";
}

static std::string FP_getCacheSQLParam(const std::vector<std::string> &par)
{
  return CacheTableTermRequest::getSQLParamXml(par);
}

static std::string FP_lastHistoryRowId(const std::vector<std::string> &par)
{
  ASSERT(par.size() >= 1);

  int idx=0;
  if (par.size() > 1) idx=std::stoi(par.at(1));

  ASSERT(idx<=0);

  idx=-idx;

  vector<RowId_t> rowIds=HistoryTable(par.at(0)).getLastEventRowIds();

  if (idx<rowIds.size()) return std::to_string(rowIds.at(idx).get());

  return "";
}

static std::string FP_runEtFltTask(const std::vector<std::string> &par)
{
  ETCheckStatusFlt();
  return "";
}

static std::string FP_cleanOldRecords(const std::vector<std::string> &par)
{
  cleanOldRecords();
  return "";
}

enum class TlgIdent { TlgId, TlgNum, TypeBInId };

static std::string getTlgIdent(TlgIdent tlgIdent, const std::vector<std::string> &par)
{
  int rowNumRequired=0;
  if (par.size() > 0) rowNumRequired=std::stoi(par.at(0));

  ASSERT(rowNumRequired<=0);

  std::string sql;

  DbCpp::CursCtl cur = [](TlgIdent tlgIdent) {
    switch (tlgIdent)
    {
    case TlgIdent::TlgId:
      return make_db_curs(
       "SELECT id FROM tlgs ORDER BY id DESC"
        , PgOra::getRWSession("TLGS")
      );
    case TlgIdent::TlgNum:
      return make_db_curs(
       "SELECT tlg_num FROM tlgs ORDER BY tlg_num DESC"
        , PgOra::getRWSession("TLGS")
      );
    default: //TlgIdent::TypeBInId:
      return make_db_curs(
       "SELECT id FROM typeb_in ORDER BY id DESC"
        , PgOra::getRWSession("TYPEB_IN")
      );
    }
  }(tlgIdent);

  int tlgNum;
  cur.def(tlgNum)
     .exec();

  int rowNum=0;
  while (!cur.fen()) {
    if ((rowNum--)==rowNumRequired) return std::to_string(tlgNum);
  }

  if (rowNumRequired==0) throw EXCEPTIONS::Exception("Empty table TLGS!");

  return "";
}

static std::string FP_lastTlgId(const std::vector<std::string> &par)
{
  return getTlgIdent(TlgIdent::TlgId, par);
}

static std::string FP_lastTlgNum(const std::vector<std::string> &par)
{
  return getTlgIdent(TlgIdent::TlgNum, par);
}

static std::string FP_lastTypeBInId(const std::vector<std::string> &par)
{
  return getTlgIdent(TlgIdent::TypeBInId, par);
}

static std::string FP_departedFlt(const std::vector<std::string> &par)
{
  ASSERT(par.size() >= 2);
  char * argv[3] = {"departed", const_cast<char *>(par.at(0).c_str()), const_cast<char *>(par.at(1).c_str())};
  nosir_departed(3, argv);
  return "";
}

static std::string FP_baselAeroStat(const std::vector<std::string> &par)
{
    //ASSERT(par.size() >= 1);
    if(par.size() <= 2) {
        char * argv[2] = {"basel_stat", const_cast<char *>(par.at(0).c_str())};
        basel_stat(2, argv);
    } else {
        char * argv[4] = {"basel_stat", const_cast<char *>(par.at(0).c_str()),
                                        const_cast<char *>(par.at(1).c_str()),
                                        const_cast<char *>(par.at(2).c_str())};
        arx_basel_stat(4, argv);
    }
    return "";
}

static std::string FP_pnrAddrByPaxId(const std::vector<std::string> &p)
{
  assert(p.size() == 2);

  PaxId_t paxId(std::stoi(p.at(0)));
  std::string airline(p.at(1));
  std::string result=TPnrAddrs().getByPaxId(paxId.get(), airline);
  return "|"+result+" "+airline+"|";
}

static std::string FP_firstPnrAddrByPaxId(const std::vector<std::string> &p)
{
  assert(p.size() == 2);
  assert(p.at(1)=="AddrOnly" || p.at(1)=="AddrAndAirline");

  PaxId_t paxId(std::stoi(p.at(0)));

  return "|"+TPnrAddrs().firstAddrByPaxId(paxId.get(),
                                          p.at(1)=="AddrOnly"?TPnrAddrInfo::AddrOnly:
                                                              TPnrAddrInfo::AddrAndAirline) +"|";
}

static std::string FP_getCompId(const std::vector<std::string>& p)
{
  assert(p.size() == 1);
  auto cur = make_db_curs(
     "SELECT comp_id FROM trip_sets WHERE point_id = :point_id",
      PgOra::getROSession("TRIP_SETS"));
   int comp_id = 0;
   cur
      .stb()
      .bind(":point_id", std::stoi(p.at(0)))
          .def(comp_id)
          .EXfet();
  if(cur.err() == DbCpp::ResultCode::NoDataFound)
    throw EXCEPTIONS::Exception("TRIP_SETS EMPTY!");
  return std::to_string(comp_id);
}

FP_REGISTER("<<", FP_tlg_in);
FP_REGISTER("!!", FP_req);
FP_REGISTER("astra_hello", FP_astra_hello);
FP_REGISTER("last_edifact_ref", FP_last_edifact_ref);
FP_REGISTER("last_point_id_spp", FP_last_point_id_spp);
FP_REGISTER("last_point_id_tlg", FP_last_point_id_tlg);
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
FP_REGISTER("get_unaccomp_id", FP_getUnaccompId);
FP_REGISTER("get_single_grp_id", FP_getSingleGrpId);
FP_REGISTER("get_single_pax_id", FP_getSinglePaxId);
FP_REGISTER("get_single_tid", FP_getSingleGrpTid); //deprecated! use get_single_grp_tid instead
FP_REGISTER("get_single_grp_tid", FP_getSingleGrpTid);
FP_REGISTER("get_single_pax_tid", FP_getSinglePaxTid);
FP_REGISTER("get_unaccomp_tid", FP_getUnaccompTid);
FP_REGISTER("get_crs_pax_unique_ref", FP_getCrsPaxUniqRef);
FP_REGISTER("get_iatci_tab_id", FP_getIatciTabId);
FP_REGISTER("get_point_tid", FP_getPointTid);
FP_REGISTER("get_lat_code", FP_get_lat_code);
FP_REGISTER("get_elem_id", FP_getElemId);
FP_REGISTER("get_random_bp_type", FP_getRandomBpTypeCode);
FP_REGISTER("settcl", FP_settcl);
FP_REGISTER("exec_stage", FP_exec_stage);
FP_REGISTER("gettcl", FP_gettcl);
FP_REGISTER("last_generated_pax_id", FP_lastGeneratedPaxId);
FP_REGISTER("substr", FP_substr);
FP_REGISTER("set_desk_version", FP_setDeskVersion);
FP_REGISTER("set_user_time_type", FP_setUserTime);
FP_REGISTER("get_user_id", FP_getUserId);
FP_REGISTER("init_apps", FP_initApps);
FP_REGISTER("translit", FP_translit);
FP_REGISTER("kick_flt_tasks_daemon", FP_kick_flt_tasks_daemon);
FP_REGISTER("run_trip_task", FP_run_trip_task);
FP_REGISTER("check_pax_alarms", FP_checkPaxAlarms);
FP_REGISTER("check_crs_pax_alarms", FP_checkCrsPaxAlarms);
FP_REGISTER("check_trip_alarms", FP_checkTripAlarms);
FP_REGISTER("desc_test", FP_descTest);
FP_REGISTER("check_flight_tasks", FP_checkFlightTasks);
FP_REGISTER("update_msg", FP_runUpdateMsg);
FP_REGISTER("resend", FP_runResendTlg);
FP_REGISTER("dump_apps_pax_data", FP_dumpAppsData);
FP_REGISTER("dump_apps_messages", FP_dumpAppsMessages);
FP_REGISTER("update_pg_coupon", FP_runUpdateCoupon);
FP_REGISTER("run_arch_step", FP_runArchStep);
FP_REGISTER("run_arch", FP_runArch);
FP_REGISTER("db_dump_table", FP_dump_table_astra);
// FP_REGISTER("dump_db_table", FP_dump_db_table);
FP_REGISTER("collect_flight_stat", FP_collectFlightStat);
FP_REGISTER("pg_sql", FP_pg_sql);
FP_REGISTER("db_sql", FP_db_sql);
FP_REGISTER("db_seq_nextval", FP_db_seq_nextval);
FP_REGISTER("db_seq_currval", FP_db_seq_currval);
FP_REGISTER("init_iapi_request_id", FP_initIapiRequestId);
FP_REGISTER("get_bcbp", FP_getBCBP);
FP_REGISTER("cache", FP_cache);
FP_REGISTER("cache_iface_ver", FP_getCacheIfaceVer);
FP_REGISTER("cache_data_ver", FP_getCacheDataVer);
FP_REGISTER("cache_sql_param", FP_getCacheSQLParam);
FP_REGISTER("last_history_row_id", FP_lastHistoryRowId);
FP_REGISTER("run_et_flt_task", FP_runEtFltTask);
FP_REGISTER("clean_old_records", FP_cleanOldRecords);
FP_REGISTER("last_tlg_id", FP_lastTlgId);
FP_REGISTER("last_tlg_num", FP_lastTlgNum);
FP_REGISTER("last_typeb_in_id", FP_lastTypeBInId);
FP_REGISTER("nosir_departed_flt", FP_departedFlt);
FP_REGISTER("nosir_basel_stat", FP_baselAeroStat);
FP_REGISTER("check_dump", FP_check_dump);
FP_REGISTER("pnr_addr_by_pax_id", FP_pnrAddrByPaxId);
FP_REGISTER("first_pnr_addr_by_pax_id", FP_firstPnrAddrByPaxId);
FP_REGISTER("get_comp_id", FP_getCompId);

#include "xp_testing.h"

START_TEST(check_getTlgIdent)
{
    const int first_tlg_id = saveTlg("RECVR", "SENDR", "OUTA", "test1");
    const int second_tlg_id = saveTlg("RECVR", "SENDR", "OUTB", "test2");
    const int third_tlg_id = saveTlg("RECVR", "SENDR", "OAPP", "test3");

    fail_unless(getTlgIdent(TlgIdent::TlgId, {"-2"}) == std::to_string(first_tlg_id));
    fail_unless(getTlgIdent(TlgIdent::TlgId, {"-1"}) == std::to_string(second_tlg_id));
    fail_unless(getTlgIdent(TlgIdent::TlgId, {"0"}) == std::to_string(third_tlg_id));
}
END_TEST;

#define SUITENAME "tlg_queue"
TCASEREGISTER(testInitDB, testShutDBConnection)
{
    ADD_TEST(check_getTlgIdent);
}
TCASEFINISH;
#undef SUITENAME


#endif /* XP_TESTING */
