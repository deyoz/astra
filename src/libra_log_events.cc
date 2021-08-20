#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "libra_log_events.h"
#include "libra.h"
#include "astra_main.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "astra_types.h"
#include "date_time.h"
#include "PgOraConfig.h"

#include <serverlib/exception.h>
#include <serverlib/dates_oci.h>
#include <serverlib/dates_io.h>
#include <serverlib/new_daemon.h>
#include <serverlib/daemon_event.h>
#include <serverlib/monitor_ctl.h>
#include <serverlib/dbcpp_cursctl.h>
#include <serverlib/testmode.h>
#include <serverlib/tclmon.h>

#include <optional>

#define NICKNAME "LIBRA"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>


namespace LIBRA
{

std::ostream& operator<<(std::ostream& os, const LogEvent& e)
{
    os << std::endl
       << "Airline[" << e.Airline << "]; "
       << "Bort[" << e.BortNum << "]; "
       << "Category[" << e.Category << "]; "
       << "PointId[" << e.PointId << "]; "
       << "LogType[" << e.LogType << "]; "
       << "LogTime[" << e.LogTime << "]; "
       << "EvUser[" << e.EvUser << "]; "
       << "EvStation[" << e.EvStation << "]; "
       << "RusMsg[" << e.RusMsg << "]; "
       << "LatMsg[" << e.LatMsg << "]";
    return os;
}

static LogEvent row2LogEvent(const LIBRA::RowData& row)
{
    LogEvent ev = {};
    ev.LogType  = row.at("log_type").fieldAsString();
    ev.Airline  = row.at("airline").fieldAsString();
    ev.BortNum  = row.at("bort_num").fieldAsString();
    ev.Category = row.at("category").fieldAsString();
    ev.PointId  = row.at("point_id").fieldAsInteger();
    ev.RusMsg   = row.at("rus_msg").fieldAsString();
    ev.LatMsg   = row.at("lat_msg").fieldAsString();
    ev.LogTime  = BASIC::date_time::DateTimeToBoost(row.at("log_time").fieldAsDateTime());
    ev.EvUser   = row.at("ev_user").fieldAsString();
    ev.EvStation= row.at("ev_station").fieldAsString();
    return ev;
}

static std::list<LogEvent> requestLogEventsViaHttp(const Dates::DateTime_t& from,
                                                   const Dates::DateTime_t& to)
{
    LogTrace(TRACE3) << __func__ << " called by " << from << " and " << to;

    const std::string params = LIBRA::makeHttpQueryString({{"from", HelpCpp::string_cast(from, "%d%m%Y_%H%M%S")},
                                                           {"to",   HelpCpp::string_cast(to,   "%d%m%Y_%H%M%S")}});
    const auto rows = LIBRA::getHttpRequestDataRows("/libra/get_log_events", params);

    std::list<LogEvent> logEvents;
    for (const LIBRA::RowData& row: rows) {
        logEvents.emplace_back(row2LogEvent(row));
        LogTrace(TRACE3) << logEvents.back();
    }
    return logEvents;
}

static std::list<LogEvent> requestLogEventsFromDb(const Dates::DateTime_t& from,
                                                  const Dates::DateTime_t& to)
{
    LogTrace(TRACE3) << __func__ << " called by " << from << " and " << to;
    auto cur = make_db_curs(
"select AIRLINE, BORT_NUM, CATEGORY, POINT_ID, EV_STATION, EV_USER, "
"LOG_TIME, LOG_TYPE, RUS_MSG, LAT_MSG from LIBRA_LOG_EVENTS "
"where LOG_TIME > :time_from and LOG_TIME <= :time_to "
"order by LOG_TIME, EV_ORDER",
                PgOra::getROSession("LIBRA_LOG_EVENTS"));

    LogEvent e = {};
    cur
            .bind(":time_from", from)
            .bind(":time_to",   to)
            .defNull(e.Airline, "")
            .defNull(e.BortNum, "")
            .defNull(e.Category,"")
            .defNull(e.PointId, ASTRA::NoExists)
            .def(e.EvStation)
            .def(e.EvUser)
            .def(e.LogTime)
            .def(e.LogType)
            .def(e.RusMsg)
            .def(e.LatMsg)
            .exec();
    std::list<LogEvent> logEvents;
    while(!cur.fen()) {
        logEvents.emplace_back(e);
        LogTrace(TRACE3) << logEvents.back();
    }

    return logEvents;
}

static size_t removeLogEventsViaHttp(const Dates::DateTime_t& to)
{
    LogTrace(TRACE3) << __func__ << " called by " << to;
    const std::string params = LIBRA::makeHttpQueryString({{"to", HelpCpp::string_cast(to, "%d%m%Y_%H%M%S")}});
    const auto row = LIBRA::getHttpRequestDataRow("/libra/remove_log_events", params, HttpMethod::Post);
    return static_cast<size_t>(row.at("rowcount").fieldAsInteger());
}

static size_t removeLogEventsFromDb(const Dates::DateTime_t& to)
{
    LogTrace(TRACE3) << __func__ << " called by " << to;
    auto cur = make_db_curs(
"delete from LIBRA_LOG_EVENTS where LOG_TIME <= :time_to",
                PgOra::getRWSession("LIBRA_LOG_EVENTS"));
    cur
            .bind(":time_to", to)
            .exec();

    return static_cast<size_t>(cur.rowcount());
}

static std::string screenByLogEvent(const LogEvent& e)
{
    return e.isAhm() ? "LIBRA_AHM.EXE" : "LIBRA_BAL.EXE";
}

static int genAhmId(int airlineId,
                    const std::optional<AhmCategory_t>& category,
                    const std::optional<BortNum_t>& bort)
{
    int id = PgOra::getSeqNextVal_int("AHM_ID__SEQ");

    std::string categVal = category ? category->get() : "";
    std::string bortVal  = bort ? bort->get() : "";
    short null = -1, nnull = 0;
    make_db_curs(
"insert into AHM_DICT(ID, AIRLINE, CATEGORY, BORT_NUM) "
"values (:id, :airl, :cat, :bort)",
                PgOra::getRWSession("AHM_DICT"))
            .stb()
            .bind(":id",   id)
            .bind(":airl", airlineId)
            .bind(":cat",  categVal, category ? &nnull : &null)
            .bind(":bort", bortVal,  bort ? &nnull : &null)
            .exec();
    return id;
}

static int getOrGenAhmId(int airlineId,
                         const AhmCategory_t& category,
                         const BortNum_t& bort)
{
    int id = 0;
    auto sel = make_db_curs(
"select ID from AHM_DICT where AIRLINE=:airl and CATEGORY=:cat and BORT_NUM=:bort",
                 PgOra::getRWSession("AHM_DICT"));
    sel
            .def(id)
            .bind(":airl", airlineId)
            .bind(":cat",  category.get())
            .bind(":bort", bort.get())
            .EXfet();
    if(sel.err() != DbCpp::ResultCode::NoDataFound) {
        return id;
    }

    return genAhmId(airlineId, category, bort);
}

static int getOrGenAhmId(int airlineId,
                         const AhmCategory_t& category)
{
    int id = 0;
    auto sel = make_db_curs(
"select ID from AHM_DICT where AIRLINE=:airl and CATEGORY=:cat and BORT_NUM is null",
                 PgOra::getRWSession("AHM_DICT"));
    sel
            .def(id)
            .bind(":airl", airlineId)
            .bind(":cat",  category.get())
            .EXfet();
    if(sel.err() != DbCpp::ResultCode::NoDataFound) {
        return id;
    }

    return genAhmId(airlineId, category, std::nullopt);
}

static int getOrGenAhmId(int airlineId,
                         const BortNum_t& bort)
{
    int id = 0;
    auto sel = make_db_curs(
"select ID from AHM_DICT where AIRLINE=:airl and BORT_NUM=:bort and CATEGORY is null",
                 PgOra::getRWSession("AHM_DICT"));
    sel
            .def(id)
            .bind(":airl", airlineId)
            .bind(":bort", bort.get())
            .EXfet();
    if(sel.err() != DbCpp::ResultCode::NoDataFound) {
        return id;
    }

    return genAhmId(airlineId, std::nullopt, bort);
}

static int getOrGenAhmId(int airlineId)
{
    int id = 0;
    auto sel = make_db_curs(
"select ID from AHM_DICT where AIRLINE=:airl and BORT_NUM is null and CATEGORY is null",
                 PgOra::getRWSession("AHM_DICT"));
    sel
            .def(id)
            .bind(":airl", airlineId)
            .EXfet();
    if(sel.err() != DbCpp::ResultCode::NoDataFound) {
        return id;
    }

    return genAhmId(airlineId, std::nullopt, std::nullopt);
}

static int getAirlineId(const AirlineCode_t& airlineCode)
{
    // TODO DROPME
    if(airlineCode.get() == "226") return 226;
    if(airlineCode.get() == "534") return 534;

    const TAirlinesRow& row = dynamic_cast<const TAirlinesRow&>(base_tables.get("airlines").get_row("code/code_lat", airlineCode.get()));
    return row.id;
}

static int ahmIdByLogEvent(const LogEvent& e)
{
    ASSERT(e.isAhm() && !e.Airline.empty());

    int airlineId = getAirlineId(AirlineCode_t(e.Airline));
    if(!e.Category.empty() && !e.BortNum.empty()) {
        return getOrGenAhmId(airlineId,
                             AhmCategory_t(e.Category),
                             BortNum_t(e.BortNum));
    }

    if(!e.Category.empty() && e.BortNum.empty()) {
        return getOrGenAhmId(airlineId,
                             AhmCategory_t(e.Category));
    }

    if(e.Category.empty() && !e.BortNum.empty()) {
        return getOrGenAhmId(airlineId,
                             BortNum_t(e.BortNum));
    }

    return getOrGenAhmId(airlineId);
}

static void handleLogEvent(const LogEvent& e)
{
    LogTrace(TRACE3) << __func__ << e;
    const std::string screen  = screenByLogEvent(e);
    const std::string user    = e.EvUser;
    const std::string station = e.EvStation;

    TLogMsg msg;
    msg.ev_type = e.isAhm() ? ASTRA::evtAhm : ASTRA::evtFlt;
    msg.ev_time = BASIC::date_time::BoostToDateTime(e.LogTime);
    if(e.isAhm()) {
        msg.id1 = ahmIdByLogEvent(e);
    } else {
        msg.id1 = e.PointId;
    }

    // RU
    msg.msg = e.RusMsg;
    msg.toDB(screen, user, station, AstraLocale::LANG_RU);

    // EN
    msg.msg = e.LatMsg;
    msg.toDB(screen, user, station, AstraLocale::LANG_EN);
}

std::list<LogEvent> requestLogEvents(const Dates::DateTime_t& from,
                                     const Dates::DateTime_t& to)
{
    if(LIBRA::needSendHttpRequest()) {
        return requestLogEventsViaHttp(from, to);
    } else {
        return requestLogEventsFromDb(from, to);
    }
}

size_t removeLogEvents(const Dates::DateTime_t& to)
{
    if(LIBRA::needSendHttpRequest()) {
        return removeLogEventsViaHttp(to);
    } else {
        return removeLogEventsFromDb(to);
    }
}

}//namespace LIBRA

/////////////////////////////////////////////////////////////////////////////////////////

namespace {

using namespace LIBRA;


struct LogEventTimes
{
    Dates::DateTime_t From;
    Dates::DateTime_t To;
};

std::optional<LogEventTimes> readTimes()
{
    auto cur = make_db_curs(
"select TIME_FROM, TIME_TO from LIBRA_PROC_LOG_EVENTS for update",
                PgOra::getRWSession("LIBRA_PROC_LOG_EVENTS"));

    LogEventTimes times = {};
    cur
            .def(times.From)
            .def(times.To)
            .EXfet();
    if(cur.err() == DbCpp::ResultCode::NoDataFound) {
        return {};
    }

    return times;
}

void writeTimes(const Dates::DateTime_t& from,
                const Dates::DateTime_t& to)
{
    LogTrace(TRACE3) << __func__ << " called by " << from << " and " << to;
    make_db_curs(
"delete from LIBRA_PROC_LOG_EVENTS",
                PgOra::getRWSession("LIBRA_PROC_LOG_EVENTS"))
            .exec();

    make_db_curs(
"insert into LIBRA_PROC_LOG_EVENTS(TIME_FROM, TIME_TO) "
"values (:time_from, :time_to)",
                PgOra::getRWSession("LIBRA_PROC_LOG_EVENTS"))
            .bind(":time_from", from)
            .bind(":time_to",   to)
            .exec();
}

//---------------------------------------------------------------------------------------

void run_libra_log_events_handler()
{
    auto nowUtc = boost::posix_time::second_clock::universal_time();

    LogTrace(TRACE1) << __func__ << " runs at " << nowUtc << "(UTC)";

    try {
        auto lastTimes = readTimes();
        Dates::DateTime_t from = lastTimes ? lastTimes->To
                                           : nowUtc - boost::gregorian::days(1);
        Dates::DateTime_t to = nowUtc - boost::posix_time::seconds(1);
#ifdef XP_TESTING
        if(inTestMode()) {
            to = nowUtc;
        }
#endif//XP_TESTING

        auto logEvents = requestLogEvents(from, to);
        for(auto& ev: logEvents) {
            handleLogEvent(ev);
        }
        writeTimes(from, to);

        monitor_idle_zapr_type(logEvents.size(), QUEPOT_NULL);
    } catch(std::exception& e) {
        LogError(STDLOG) << e.what() << ": rollback!";
        ASTRA::rollback();
    }
}

void run_libra_log_events_cleaner()
{
    auto nowUtc = boost::posix_time::second_clock::universal_time();
    size_t removed = removeLogEvents(nowUtc - boost::gregorian::days(1));
    LogTrace(TRACE1) << __func__ << " " << removed << " log records were removed at" << nowUtc << "(UTC)";
    monitor_idle_zapr_type(removed, QUEPOT_NULL);
}

}//namespace

/////////////////////////////////////////////////////////////////////////////////////////

using namespace ServerFramework;


class LibraLogEventsDaemon: public NewDaemon
{
    virtual void init();
public:
    static const char *Name;
    LibraLogEventsDaemon();
};

const char* LibraLogEventsDaemon::Name = "Libra log events daemon";

LibraLogEventsDaemon::LibraLogEventsDaemon()
    : NewDaemon("LIBRA_LOG_EVENTS")
{}

void LibraLogEventsDaemon::init()
{
    init_locale_throwIfFailed();
}

//---------------------------------------------------------------------------------------

class LibraLogEventsHandler: public DaemonTask
{
public:
    virtual int run(const boost::posix_time::ptime&);
    virtual void monitorRequest() {}
    LibraLogEventsHandler();
};

//

LibraLogEventsHandler::LibraLogEventsHandler()
    : DaemonTask(DaemonTaskTraits::OracleAndHooks())
{}

int LibraLogEventsHandler::run(const boost::posix_time::ptime&)
{
    run_libra_log_events_handler();
    return 0;
}

//---------------------------------------------------------------------------------------

class LibraLogEventsCleaner: public DaemonTask
{
public:
    virtual int run(const boost::posix_time::ptime&);
    virtual void monitorRequest() {}
    LibraLogEventsCleaner();
};

//

LibraLogEventsCleaner::LibraLogEventsCleaner()
    : DaemonTask(DaemonTaskTraits::OracleAndHooks())
{}

int LibraLogEventsCleaner::run(const boost::posix_time::ptime&)
{
    run_libra_log_events_cleaner();
    return 0;
}

//---------------------------------------------------------------------------------------

int main_libra_log_events_daemon_tcl(int supervisorSocket, int argc, char *argv[])
{
    LibraLogEventsDaemon daemon;

    // обработка очереди
    DaemonEventPtr handleTimer(new TimerDaemonEvent(10));
    handleTimer->addTask(DaemonTaskPtr(new LibraLogEventsHandler));
    daemon.addEvent(handleTimer);
    // очистка очереди
    DaemonEventPtr cleanTimer(new TimerDaemonEvent(600));
    cleanTimer->addTask(DaemonTaskPtr(new LibraLogEventsCleaner));
    daemon.addEvent(cleanTimer);

    daemon.run();
    return 0;
}

//---------------------------------------------------------------------------------------

#ifdef XP_TESTING
namespace xp_testing {

    void runLibraLogEventsHandler_4testsOnly()
    {
        run_libra_log_events_handler();
    }

    void runLibraLogEventsCleaner_4testsOnly()
    {
        run_libra_log_events_cleaner();
    }

}//namespace xp_testing
#endif//XP_TESTING
