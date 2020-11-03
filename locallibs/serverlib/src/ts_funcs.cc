#ifdef XP_TESTING
#include <queue>
#include <fstream>
#include <algorithm>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

#include "tscript.h"
#include "func_placeholders.h"
#include "exception.h"
#include "profiler.h"
#include "cursctl.h"
#include "helpcpp.h"
#include "string_cast.h"
#include "helpcppheavy.h"
#include "EdiHelpManager.h"
#include "dump_table.h"
#include "dates_io.h"
#include "freq.h"
#include "lngv.h"
#include "localtime.h"
#include "crc32.h"
#include "str_utils.h"
#include "sirena_queue.h"
#include "httpsrv.h"
#include "query_runner.h"
#include "memcached_api.h"
#include "pg_cursctl.h"

#define NICKNAME "DMITRYVM"
#include "slogger.h"

#define THROW_MSG(msg)\
do {\
    std::ostringstream msgStream;\
    msgStream << msg;\
    throw comtech::Exception(STDLOG, __FUNCTION__, msgStream.str());\
} while (0)

using namespace xp_testing::tscript;

/* returns (offset, suffix) pair */
static std::pair<int, std::string> ParseDateTimeOffset(const std::string& text)
{
    static const boost::regex re("([+-]?\\d+)([a-z]+)?");
    boost::smatch m;
    if (!boost::regex_match(text, m, re))
        throw comtech::Exception(STDLOG, __FUNCTION__, "invalid date/time offset format: " + text);

    const int offset = boost::lexical_cast<int>(m[1]);
    const std::string suffix = m[2].matched ? m[2] : std::string();

    return std::make_pair(offset, suffix);
}

static bool IsIsoTime(const std::string& text)
{
    static const boost::regex re("[0-9]{8}T[0-9]{6}");
    return boost::regex_match(text, re);
}

/* isotime OR +/-[number]{y|mon} */
static boost::gregorian::date DateOffset(const boost::gregorian::date& now, const std::string& offsetText)
{
    if (IsIsoTime(offsetText))
        return Dates::time_from_iso_string(offsetText).date();

    const std::pair<int, std::string> offset = ParseDateTimeOffset(offsetText);
    if (offset.second.empty())
        return now + boost::gregorian::days(offset.first);
    else if (offset.second == "y")
        return now + boost::gregorian::years(offset.first);
    else if (offset.second == "mon")
        return now + boost::gregorian::months(offset.first);

    throw comtech::Exception(STDLOG, __FUNCTION__, "invalid date offset suffix: " + offset.second);
}

/* +/-[number]{y|mon|h|m|s} */
static boost::posix_time::ptime DateTimeOffset(const boost::posix_time::ptime& now, const std::vector<std::string>& offsets)
{
    boost::posix_time::ptime result = now;
    for (const std::string& offsetText:  offsets) {
        const std::pair<int, std::string> offset = ParseDateTimeOffset(offsetText);
        if (offset.second.empty())
            result += boost::gregorian::days(offset.first);
        else if (offset.second == "y")
            result += boost::gregorian::years(offset.first);
        else if (offset.second == "mon")
            result += boost::gregorian::months(offset.first);
        else if (offset.second == "h")
            result += boost::posix_time::hours(offset.first);
        else if (offset.second == "m")
            result += boost::posix_time::minutes(offset.first);
        else if (offset.second == "s")
            result += boost::posix_time::seconds(offset.first);
        else
            throw comtech::Exception(STDLOG, __FUNCTION__, "invalid date/time offset suffix: " + offset.second);
    }
    return result;
}


/*******************************************************************************
 * Функции для формирования текста, в т.ч. непечатных символов
 ******************************************************************************/

static std::string FP_sharp(const std::vector<std::string>& p)
{
    ASSERT(p.size() < 2);
    size_t n = p.empty() ? 1 : std::stoul(p[0]);
    return std::string(n, '#');
}
static std::string FP_lf(const std::vector<std::string>& p) { return "\n"; }
static std::string FP_cr(const std::vector<std::string>& p) { return "\r"; }
static std::string FP_tab(const std::vector<std::string>& p) { return "\t"; }

/* Повторяет первый агрумент заданное число раз (второй параметр, по умолчанию 1) */
static std::string FP_echo(const std::vector<std::string>& p)
{
    ASSERT(p.size() == 1 || p.size() == 2);

    const std::string what = p.at(0);
    const size_t times = p.size() == 1 ? 1 : boost::lexical_cast<size_t>(p.at(1));

    if (what.size() == 1) {
        return std::string(times, what.at(0));
    }

    std::string result;
    for (size_t i = 0; i < times; ++i)
        result += what;
    return result;
}

static std::string FP_chr(const std::vector<std::string>& p)
{
    ASSERT(p.size() == 1);
    const unsigned int code = boost::lexical_cast<unsigned int>(p.at(0));
    return std::string(1, (char)code);
}

/***************************************************************************
 * Функции
 **************************************************************************/

/* tscript_cmp.cc */
namespace xp_testing { namespace tscript {
std::string FP_cmp(const std::vector<tok::Param>& params);
} } // xp_testing::tscript

static std::string FP_output(const std::vector<tok::Param>& params)
{
    std::queue<std::string>& outq = GetTestContext()->outq;
    CheckEmpty(outq);

    tok::ValidateParams(params, 1, 1, "");
    outq.push(params.at(0).value);
    return std::string();
}

static std::string FP_set(const std::vector<std::string>& p)
{
    if (p.size() != 2)
        THROW_MSG("invalid number of parameters");
    if (!GetTestContext())
        THROW_MSG("no test context");
    GetTestContext()->vars[p.at(0)] = p.at(1);
    return std::string();
}

static std::string FP_get(const std::vector<std::string>& p)
{
    if (p.size() != 1)
        THROW_MSG("invalid number of parameters");
    if (!GetTestContext())
        THROW_MSG("no test context");

    std::map<std::string, std::string>::const_iterator it = GetTestContext()->vars.find(p.at(0));
    if (it == GetTestContext()->vars.end())
        THROW_MSG("undefined variable: " << p.at(0));

    return it->second;
}

static std::string FP_capture(const std::vector<std::string>& p)
{
    if (p.size() != 1)
        THROW_MSG("invalid number of parameters");
    if (!GetTestContext())
        THROW_MSG("no test context");

    const size_t idx = boost::lexical_cast<size_t>(p.at(0));
    std::map<size_t, std::string>::const_iterator it = GetTestContext()->captures.find(idx);
    if (it == GetTestContext()->captures.end())
        THROW_MSG("no such capture: " << p.at(0));

    return it->second;
}

static std::string FP_plus(const std::vector<std::string>& p)
{
    int res = 0;
    for(const std::string& s:  p) {
        res += boost::lexical_cast<int>(s);
    }
    return HelpCpp::string_cast(res);
}

static std::string FP_minus(const std::vector<std::string>& p)
{
    bool first = true;
    int res = 0;
    for(const std::string& s:  p) {
        if (first) {
            res = boost::lexical_cast<int>(s);
            first = false;
        } else {
            res -=  boost::lexical_cast<int>(s);
        }
    }
    return HelpCpp::string_cast(res);
}

static std::string FP_if(const std::vector<std::string>& args)
{
    ASSERT(args.size() == 2 || args.size() == 3);
    return (args[0] == "true")
        ? args[1]
        : args.size() == 3 ? args[2] : "";
}

static std::string FP_eq(const std::vector<std::string>& args)
{
    ASSERT(args.size() > 1);
    for (size_t i = 1; i < args.size(); ++i) {
        if (args[0] != args[i]) {
            return "false";
        }
    }
    return "true";
}

static std::string FP_cat(const std::vector<std::string>& p)
{
    ASSERT(p.size() == 1);
    std::string ret;
    readFile(p.at(0), ret);
    return ret;
}

static std::string FP_write_file(const std::vector<std::string>& p)
{
    ASSERT(p.size() == 2);
    writeFile(p.at(0), p.at(1));
    return "";
}

// $(sql update pnr set timelimit = NULL)
static std::string FP_sql(const std::vector<tok::Param>& params)
{
    ASSERT(params.size() > 0);
    std::string session, pgKey;
    bool output = false;
    for (size_t i = 0; i < params.size(); ++i) {
        if (params[i].name == "session") {
            ASSERT(session.empty() == true);
            session = params[i].value;
        }
        if (params[i].name == "pg") {
            pgKey = params[i].value;
        }
        if (params[i].name == "output" && params[i].value == "on")
            output = true;
    }
    std::ostringstream os;
    for (size_t i = 0; i < params.size(); ++i) {
        if (params[i].name.empty()) {
            int rowcount = 0;
            LogTrace(TRACE3) << "exec SQL: " << params[i].value;
            if (!pgKey.empty() && !(pgKey = readStringFromTcl(pgKey, "")).empty()) {
                auto cur = make_pg_curs(PgCpp::getManagedSession(pgKey), params[i].value);
                cur.exec();
                rowcount = cur.rowcount();
            } else {
                if (session.empty()) {
                    OciCpp::CursCtl cur = make_curs(params[i].value);
                    cur.exec();
                    rowcount = cur.rowcount();
                } else {
                    OciCpp::CursCtl cur = OciCpp::getSession(session).createCursor(STDLOG, params[i].value);
                    cur.exec();
                    rowcount = cur.rowcount();
                }
            }
            os << "\n" << rowcount << " rows processed";
        }
    }
    if(output) {
        if(os.str().empty())
            os << "\n";
        return os.str().substr(1);
    } else {
        return "";
    }
}
// dump_table pass fields="line,tline,p_id" where="regnum='REGNUM'" order="line.tline"
static std::string FP_dump_table(const std::vector<tok::Param>& params)
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
    OciCpp::DumpTable dt(OciCpp::getSession(session), tableName);
    if (!fields.empty()) {
        dt.addFld(fields);
    }
    dt.where(where);
    dt.order(order);
    std::string answer;
    if (display) {
        dt.exec(answer);
    } else {
        dt.exec(TRACE5);
    }

    return answer;
}

static std::string FP_savepoint(const std::vector<std::string>& p)
{
    ASSERT(p.size() == 1 || p.size() == 2);
    if (p.size() == 1 || (p.size() == 2 && p.at(1) == "start")) {
        make_curs("savepoint " + p.at(0)).exec();
    } else if (p.size() == 2 && p.at(1) == "rollback") {
        make_curs("rollback to savepoint " + p.at(0)).exec();
    }
    return std::string();
}

static std::string FP_clean_edi_help(const std::vector<std::string>& p)
{
    make_curs("UPDATE EDI_HELP SET DATE1 = DATE1 - 2/1440").exec();
    ServerFramework::EdiHelpManager::cleanOldRecords();
    return std::string();
}

static std::string FP_clear_edi_help(const std::vector<std::string> &args)
{
    make_curs("delete from edi_help").exec();
    return std::string();
}

static std::string FP_edi_help(const std::vector<std::string> &args)
{
    ASSERT(args.size() > 0);
    std::string address;
    std::string date1;
    std::string intmsgid;
    std::string pult;
    std::string text;
    std::string timeout;

    OciCpp::CursCtl cur = make_curs("select address, to_char(date1,'ddmmyy hh24miss'), "
                                    "INTMSGID, pult, text, timeout from edi_help");
    cur.
            def(address).
            def(date1).
            def(intmsgid).
            def(pult).
            def(text).
            def(timeout).
            exec();

    std::stringstream res;
    while(cur.fen() == 0) {

        for(const std::string &param:  args) {
            res << " ";
            if(param == "address")
                res << address;
            else if(param == "date")
                res << date1;
            else if(param == "intmsgid")
                res << intmsgid;
            else if(param == "pult")
                res << pult;
            else if(param == "text")
                res << text;
            else if(param == "timeout")
                res << timeout;
            else
                ASSERT(0)
        }
    }

    if(res.str().empty()) {
        res << " no data found";
    }

    return res.str().substr(1);
}

static std::string FP_start_prof(const std::vector<std::string>& p)
{
    LogTrace(TRACE1) << __FUNCTION__ << ": profiling start";
    start_profiling();
    return std::string();
}

static std::string FP_stop_prof(const std::vector<std::string>& p)
{
    LogTrace(TRACE1) << __FUNCTION__ << ": profiling stop";
    stop_profiling();
    return std::string();
}

/* $(ddhhmi) */
static std::string FP_ddhhmi(const std::vector<std::string>& p)
{
    ASSERT(p.size() == 0);
    return HelpCpp::string_cast(boost::posix_time::second_clock::universal_time(), "%d%H%M");
}

static std::string FP_hhmi(const std::vector<std::string>& p)
{
    ASSERT(p.size() < 2);
    if (p.empty()) {
        return Dates::hh24mi(Dates::currentDateTime().time_of_day(), false);
    }

    if (IsIsoTime(p.at(0))) {
        return Dates::hh24mi(Dates::time_from_iso_string(p.at(0)).time_of_day(), false);
    }

    const std::pair<int, std::string> offset = ParseDateTimeOffset(p.at(0));

    boost::posix_time::ptime tm = Dates::currentDateTime();
    if (offset.second == "h") {
        tm += boost::posix_time::hours(offset.first);
    } else if (offset.second == "m") {
        tm += boost::posix_time::minutes(offset.first);
    } else if (offset.second == "s") {
        tm += boost::posix_time::seconds(offset.first);
    } else {
        throw comtech::Exception(STDLOG, __FUNCTION__, "invalid time offset suffix: " + offset.second);
    }
    return Dates::hh24mi(tm.time_of_day(), false);
}

/* $(yymmdd +10) */
static std::string FP_yymmdd(const std::vector<std::string>& p)
{
    ASSERT(p.size() <= 2);
    std::string result = p.empty()
      ? Dates::rrmmdd(Dates::currentDate())
      : Dates::rrmmdd(DateOffset(Dates::currentDate(), p.at(0)));

    if (p.size() == 2) {
      result.insert(2, p.at(1));
      result.insert(5, p.at(1));
    }

    return result;
}

// смещние от прошлого понедельника до сегодня
static std::string FP_dayshift(const std::vector<std::string>& p)
{
    ASSERT(p.size() == 0);
    const int d = boost::gregorian::day_clock::local_day().day_of_week().as_number();
    return HelpCpp::string_cast(d == 0 ? 6 : d - 1);
}

// частота по дате или интервалу дат. $(freq +5 +7)
static std::string FP_freq( std::vector< std::string > const &p )
{
    ASSERT( p.size() == 1 || p.size() == 2 );
    boost::gregorian::date const d1( DateOffset( Dates::currentDate(), p.at( 0 ) ) );
    return Freq( d1, p.size() == 2 ? DateOffset( Dates::currentDate(), p.at( 1 ) ) : d1 ).str();
}

/* $(ddmonyy +10) */
static std::string FP_ddmonyy(const std::vector<std::string>& p)
{
    ASSERT(p.size() >= 1 && p.size() <= 3);
    std::string result = Dates::ddmonrr(
            DateOffset(Dates::currentDate(), p.at(0)),
            p.size() == 1 ? ENGLISH : languageFromStr(p.at(1)));
    if (p.size() == 3) {
        const std::string splitter = p.at(2);
        const std::string dd = result.substr(0, 2);
        const std::string mon = result.substr(2, 3);
        const std::string yy = result.substr(5, 2);
        result = dd + splitter + mon + splitter + yy;
    }
    return result;
}

/* $(ddmon +10 ru .) */
static std::string FP_ddmon(const std::vector<std::string>& p)
{
    ASSERT(p.size() >= 1 && p.size() <= 3);
    std::string result = Dates::ddmon(
                             DateOffset(Dates::currentDate(), p.at(0)),
                             p.size() == 1 ? ENGLISH : languageFromStr(p.at(1)));
    if (p.size() == 3) {
        const std::string splitter = p.at(2);
        const std::string dd = result.substr(0, 2);
        const std::string mon = result.substr(2, 3);
        result = dd + splitter + mon;
    }
    return result;
}

/* $(ddmmyy +10 .) */
static std::string FP_ddmmyy(const std::vector<std::string>& p)
{
    ASSERT(p.size() <= 2);
    std::string result = p.empty()
                         ? Dates::ddmmrr(Dates::currentDate())
                         : Dates::ddmmrr(DateOffset(Dates::currentDate(), p.at(0)));
    if (p.size() == 2) {
        const std::string splitter = p.at(1);
        const std::string dd = result.substr(0, 2);
        const std::string mm = result.substr(2, 2);
        const std::string yy = result.substr(4, 2);
        result = dd + splitter + mm + splitter + yy;
    }
    return result;
}

/* $(ddmmyyyy +10 .) */
static std::string FP_ddmmyyyy(const std::vector<std::string>& p)
{
    ASSERT(p.size() <= 2);
    std::string result = p.empty()
                         ? Dates::ddmmyyyy(Dates::currentDate())
                         : Dates::ddmmyyyy(DateOffset(Dates::currentDate(), p.at(0)));
    if (p.size() == 2) {
        const std::string splitter = p.at(1);
        const std::string dd = result.substr(0, 2);
        const std::string mm = result.substr(2, 2);
        const std::string yyyy = result.substr(4, 4);
        result = dd + splitter + mm + splitter + yyyy;
    }
    return result;
}

/* $(ddmm +10) */
static std::string FP_ddmm(const std::vector<std::string>& p)
{
    return FP_ddmmyy(p).substr(0, 4);
}

/* $(mmyy +10 .) */
static std::string FP_mmyy(const std::vector<std::string>& p)
{
    ASSERT(p.size() <= 2);
    std::string result = p.empty()
                         ? Dates::mmyy(Dates::currentDate())
                         : Dates::mmyy(DateOffset(Dates::currentDate(), p.at(0)));
    if (p.size() == 2) {
        const std::string splitter = p.at(1);
        const std::string mm = result.substr(0, 2);
        const std::string yy = result.substr(2, 2);
        result = mm + splitter + yy;
    }
    return result;
}

static std::string FP_dd(const std::vector<std::string>& p)
{
    return FP_ddmmyy(p).substr(0, 2);
}

/* $(yyyymmdd +10) */
static std::string FP_yyyymmdd(const std::vector<std::string>& p)
{
    ASSERT(p.size() <= 2);
    std::string result = p.empty()
      ? Dates::yyyymmdd(Dates::currentDate())
      : Dates::yyyymmdd(DateOffset(Dates::currentDate(), p.at(0)));

    if (p.size() == 2) {
      result.insert(4, p.at(1));
      result.insert(7, p.at(1));
    }

    return result;
}

/* 20110830T072100 */
static std::string FP_isotime(const std::vector<std::string>& p)
{
    return Dates::to_iso_string(DateTimeOffset(Dates::currentDateTime(), p));
}
/* $(+40 0700 Europe/Moscow) */
static std::string FP_gmt(const std::vector<std::string>& p)
{
    ASSERT(p.size() == 3);
    std::string tm = Dates::yyyymmdd(DateOffset(Dates::currentDate(), p.at(0))) + p.at(1) + "00";
    std::string gmt = City2GMT(tm, p.at(2));
    return gmt.substr(8, 4); //time only, no secs and date
}
static std::string FP_full_gmt_datetime(const std::vector<std::string>& p)
{
    boost::posix_time::ptime t=DateTimeOffset(boost::posix_time::second_clock::universal_time(), p);
    return HelpCpp::string_cast(t,"%Y%m%d%H%M"); //time only, no secs and date
}

static std::string FP_read_file(const std::vector<std::string>& args)
{
    ASSERT(args.size() == 1);
    const std::string fileName = args.at(0);
    std::ifstream file(fileName.c_str());
    if (!file)
        throw std::runtime_error("can not open file " + fileName);
    std::string data;
    std::copy(
            std::istreambuf_iterator<char>(file.rdbuf()),
            std::istreambuf_iterator<char>(),
            std::back_inserter(data));
    return data;
}

static std::string FP_crc32(const std::vector<std::string>& args)
{
    ASSERT(args.size() == 1);

    const std::string& data = args.at(0);
    const unsigned long crc32 = Crc32(args.at(0));

    LogTrace(TRACE5) << __FUNCTION__ << ": data size = " << data.size() << ", crc32 = " << crc32;
    std::ostringstream out;
    out << crc32;
    return out.str();
}

static std::string FP_lpad(const std::vector<std::string>& p)
{
    ASSERT(p.size() > 1 && p.size() < 4);
    char padder = ' ';
    std::string::size_type len = boost::lexical_cast<std::string::size_type>(p[1]);
    if (p.size() > 2)
    {
        ASSERT(p[2].size() == 1)
        padder = p[2][0];
    }
    return StrUtils::LPad(p[0], len, padder);
}

static std::string FP_rpad(const std::vector<std::string>& p)
{
    ASSERT(p.size() > 1 && p.size() < 4);
    char padder = ' ';
    std::string::size_type len = boost::lexical_cast<std::string::size_type>(p[1]);
    if (p.size() > 2)
    {
        ASSERT(p[2].size() == 1)
        padder = p[2][0];
    }
    return StrUtils::RPad(p[0], len, padder);
}

static std::string FP_replace(const std::vector<std::string> &p)
{
    ASSERT(p.size() == 3);
    return StrUtils::replaceSubstrCopy(p[0], p[1], p[2]);   
}

static std::string FP_length(const std::vector<std::string>& p)
{
    ASSERT(p.size() == 1);
    return HelpCpp::string_cast(p[0].length());
}

static std::string FP_lock_date_time(const std::vector<std::string>& p)
{
    ASSERT(p.size() >= 1 && p.size() <= 2);
    if (p.size() == 1)
    {
        xp_testing::fixDateTime(Dates::time_from_iso_string(p.front()));
    }
    else if (p.size() == 2)
    {
        xp_testing::fixDateTime(Dates::time_from_iso_string(p[0]), p[1]);
    }
    return std::string();
}

// $(date_format %Y-%m-%d +1y -2mon +3 -4h +5m -6s)
static std::string FP_date_format(const std::vector<std::string>& p)
{
    ASSERT(p.size() > 0);
    std::vector<std::string> offsetList;
    std::copy(p.begin() + 1, p.end(), std::back_inserter(offsetList));
    return HelpCpp::string_cast(DateTimeOffset(Dates::currentDateTime(), offsetList), p[0].c_str());
}

static std::string FP_uppercase(const std::vector<std::string>& p)
{
    ASSERT(p.size() == 1);
    return StrUtils::ToUpper(p[0]);
}

static std::string FP_set_pult(const std::vector<std::string>& p)
{
    ASSERT(p.size() == 1 && (httpsrv::Pult::maxLength() == p[0].size()));

    static ServerFramework::QueryRunner qr(ServerFramework::InternetQueryRunner());
    qr.setPult(p[0]);
    ServerFramework::setQueryRunner(qr);

    return std::string();
}

static std::string FP_set_perespros(const std::vector<std::string>& p)
{
    ASSERT(p.size() == 0);
    ServerFramework::setPerespros(true);
    return std::string();
}

static std::string FP_linecount(const std::vector<std::string>& p)
{
    if (p.empty())
        return "0";

    size_t count = 1;
    for (const std::string& s : p)
        count += std::count(s.begin(), s.end(), '\n');
    return HelpCpp::string_cast(count);
}

static std::string FP_check_memcached_available(const std::vector<std::string>& p)
{
    ASSERT(p.size() == 1);
    if(!memcache::isInstanceAvailable(p.at(0)))
    {
        LogTrace(TRACE0) << "Memcached instance '" << p.at(0) << "' is not available";
        throw xp_testing::tscript::SkipTest();
    }
    return "";
}

static std::string FP_flush_memcached(const std::vector<std::string>& p)
{
    memcache::callbacks()->flushAll();
    return "";
}

FP_REGISTER("linecount", FP_linecount)
FP_REGISTER("sharp", FP_sharp)
FP_REGISTER("lf", FP_lf)
FP_REGISTER("cr", FP_cr)
FP_REGISTER("tab", FP_tab)
FP_REGISTER("echo", FP_echo)
FP_REGISTER("chr", FP_chr)
FP_REGISTER(">>", xp_testing::tscript::FP_cmp)
FP_REGISTER("??", FP_output)
FP_REGISTER("set", FP_set)
FP_REGISTER("get", FP_get)
FP_REGISTER("capture", FP_capture)
FP_REGISTER("+", FP_plus)
FP_REGISTER("-", FP_minus)
FP_REGISTER("if", FP_if);
FP_REGISTER("eq", FP_eq);
FP_REGISTER("cat", FP_cat)
FP_REGISTER("sql",  FP_sql)
FP_REGISTER("dump_table", FP_dump_table)
FP_REGISTER("savepoint", FP_savepoint)
FP_REGISTER("clean_edi_help", FP_clean_edi_help)
FP_REGISTER("clear_edi_help", FP_clear_edi_help)
FP_REGISTER("edi_help",  FP_edi_help)
FP_REGISTER("start_prof", FP_start_prof)
FP_REGISTER("stop_prof", FP_stop_prof)
FP_REGISTER("ddhhmi", FP_ddhhmi)
FP_REGISTER("hhmi", FP_hhmi)
FP_REGISTER("yymmdd", FP_yymmdd)
FP_REGISTER("dayshift", FP_dayshift)
FP_REGISTER("freq", FP_freq)
FP_REGISTER("ddmonyy", FP_ddmonyy)
FP_REGISTER("ddmon", FP_ddmon)
FP_REGISTER("ddmmyy", FP_ddmmyy)
FP_REGISTER("ddmmyyyy", FP_ddmmyyyy)
FP_REGISTER("ddmm", FP_ddmm)
FP_REGISTER("mmyy", FP_mmyy)
FP_REGISTER("dd", FP_dd)
FP_REGISTER("yyyymmdd", FP_yyyymmdd)
FP_REGISTER("isotime", FP_isotime)
FP_REGISTER("gmt", FP_gmt)
FP_REGISTER("read_file", FP_read_file)
FP_REGISTER("write_file", FP_write_file)
FP_REGISTER("crc32", FP_crc32)
FP_REGISTER("full_gmt_datetime", FP_full_gmt_datetime)
FP_REGISTER("lpad", FP_lpad)
FP_REGISTER("rpad", FP_rpad)
FP_REGISTER("length", FP_length)
FP_REGISTER("replace", FP_replace)
FP_REGISTER("lock_date_time", FP_lock_date_time)
FP_REGISTER("date_format", FP_date_format)
FP_REGISTER("uppercase", FP_uppercase)
FP_REGISTER("set_pult", FP_set_pult)
FP_REGISTER("set_perespros", FP_set_perespros)
FP_REGISTER("chk_memcached_available", FP_check_memcached_available)
FP_REGISTER("flush_memcached", FP_flush_memcached)

void initTsFuncs()
{
}
#endif /* XP_TESTING */
