#include "codeshare_sets.h"

#define STDLOG NICKNAME,__FILE__,__LINE__
#define NICKNAME "VLAD"

#include "serverlib/slogger.h"
#include "date_time.h"
#include "astra_consts.h"
#include "exceptions.h"
#include "qrys.h"
#include "hist.h"
#include "db_tquery.h"
#include <serverlib/dbcpp_cursctl.h>
#include "PgOraConfig.h"


namespace CodeshareSet {

void writeHistory(int id)
{
    HistoryTable("CODESHARE_SETS").synchronize(RowId_t(id));
}

void checkParams(int id, TDateTime first_date, TDateTime last_date,
                 TDateTime& first, TDateTime& last, bool& pr_opd)
{
    checkPeriod(id == ASTRA::NoExists, first_date, last_date, BASIC::date_time::NowUTC(), first, last, pr_opd);
}

void updateRange(int id, int tid, TDateTime first_date, TDateTime last_date)
{
    const std::string sql =
            "UPDATE codeshare_sets SET "
            "first_date=:first_date,"
            "last_date=:last_date,"
            "tid=:tid "
            "WHERE id=:id";
    QParams params;
    params << QParam("first_date", otDate, first_date)
           << QParam("last_date", otDate, last_date, QParam::NullOnEmpty)
           << QParam("tid", otInteger, tid)
           << QParam("id", otInteger, id);
    DB::TCachedQuery Qry(PgOra::getRWSession("CODESHARE_SETS"), sql, params, STDLOG);
    Qry.get().Execute();
    if (Qry.get().RowsProcessed() > 0) {
        writeHistory(id);
    }
}

void updateDaysRange(int id, int tid, TDateTime first_date, TDateTime last_date, const std::string& days)
{
    const std::string sql =
        "UPDATE codeshare_sets SET "
        "first_date=:first_date,"
        "last_date=:last_date,"
        "days=:days,"
        "tid=:tid "
        "WHERE id=:id";
    QParams params;
    params << QParam("first_date", otDate, first_date)
           << QParam("last_date", otDate, last_date, QParam::NullOnEmpty)
           << QParam("days", otString, days)
           << QParam("tid", otInteger, tid)
           << QParam("id", otInteger, id);
    DB::TCachedQuery Qry(PgOra::getRWSession("CODESHARE_SETS"), sql, params, STDLOG);
    Qry.get().Execute();
    if (Qry.get().RowsProcessed() > 0) {
        writeHistory(id);
    }
}

void updateAsDeleted(int id, int tid)
{
    const std::string sql =
            "UPDATE codeshare_sets SET "
            "pr_del=1,"
            "tid=:tid "
            "WHERE id=:id";
    QParams params;
    params << QParam("tid", otInteger, tid)
           << QParam("id", otInteger, id);
    DB::TCachedQuery Qry(PgOra::getRWSession("CODESHARE_SETS"), sql, params, STDLOG);
    Qry.get().Execute();
    if (Qry.get().RowsProcessed() > 0) {
        writeHistory(id);
    }
}

void updateLastDate(int id, int tid, TDateTime last_date)
{
    const std::string sql =
            "UPDATE codeshare_sets SET "
            "last_date=:last_date,"
            "tid=:tid "
            "WHERE id=:id";
    QParams params;
    params << QParam("last_date", otDate, last_date, QParam::NullOnEmpty)
           << QParam("tid", otInteger, tid)
           << QParam("id", otInteger, id);
    DB::TCachedQuery Qry(PgOra::getRWSession("CODESHARE_SETS"), sql, params, STDLOG);
    Qry.get().Execute();
    if (Qry.get().RowsProcessed() > 0) {
        writeHistory(id);
    }
}

void updateTid(int id, int tid)
{
    const std::string sql =
            "UPDATE codeshare_sets SET "
            "tid=:tid "
            "WHERE id=:id";
    QParams params;
    params << QParam("tid", otInteger, tid)
           << QParam("id", otInteger, id);
    DB::TCachedQuery Qry(PgOra::getRWSession("CODESHARE_SETS"), sql, params, STDLOG);
    Qry.get().Execute();
    if (Qry.get().RowsProcessed() > 0) {
        writeHistory(id);
    }
}

int copy(int id, int tid, TDateTime first_date, TDateTime last_date)
{
    const int new_id = PgOra::getSeqNextVal_int("ID__SEQ");
    const std::string sql =
            "INSERT INTO codeshare_sets("
            "id,airline_oper,flt_no_oper,suffix_oper,airp_dep,airline_mark,flt_no_mark,suffix_mark, "
            "pr_mark_norms,pr_mark_bp,pr_mark_rpt,days,first_date,last_date,pr_del,tid"
            ") "
            "SELECT "
            ":new_id,airline_oper,flt_no_oper,suffix_oper,airp_dep,airline_mark,flt_no_mark,suffix_mark,"
            "pr_mark_norms,pr_mark_bp,pr_mark_rpt,days,:first_date,:last_date,0,:tid "
            "FROM codeshare_sets "
            "WHERE id=:id";
    QParams params;
    params << QParam("new_id", otInteger, new_id)
        << QParam("tid", otInteger, tid)
        << QParam("first_date", otDate, first_date)
        << QParam("last_date", otDate, last_date, QParam::NullOnEmpty)
        << QParam("id", otInteger, id);
    DB::TCachedQuery Qry(PgOra::getRWSession("CODESHARE_SETS"), sql, params, STDLOG);
    Qry.get().Execute();
    if (Qry.get().RowsProcessed() > 0) {
        writeHistory(new_id);
    }
    return new_id;
}

bool normalize_days(std::string& days)
{
    if (days.empty()) {
        return true;
    }
    std::string aux_days(".......");
    for (size_t i = 0; i != days.size(); ++i) {
        if (days[i] >= '1' && days[i] <= '7') {
            size_t position = days[i] - '1';
            aux_days[position] = days[i];
        } else if (days[i] != '.') {
            return false;
        }
    }
    if (aux_days == "......") {
        return false;
    }
    if (aux_days == "1234567") {
        days.clear();
    } else {
        days = aux_days;
    }
    return true;
}

void subtract_days(std::string days_from, std::string days_what, std::string& days_res)
{
    days_res.clear();
    if (normalize_days(days_from) && normalize_days(days_what)) {
        days_res = days_from;
        if (days_res.empty()) {
            days_res = "1234567";
        }
        for (size_t i = 0; i != days_res.size(); ++i) {
            char c = days_res[i];
            if (days_what.empty() || days_what.find_first_of(c) != std::string::npos) {
                days_res[i] = '.';
            }
        }
        if (days_res == ".......") {
            days_res.clear();
        }
    }
}

struct ItemIdDateDaysRange
{
    int id = ASTRA::NoExists;
    TDateTime first_date;
    TDateTime last_date;
    std::string days;
};

struct CodeshareSetsData
{
    int id = ASTRA::NoExists;
    TDateTime first_date;
    TDateTime last_date;
    std::string days;
    std::string airline_oper;
    int flt_no_oper = ASTRA::NoExists;
    std::string suffix_oper;
    std::string airp_dep;
    std::string airline_mark;
    int flt_no_mark = ASTRA::NoExists;
    std::string suffix_mark;
    bool pr_mark_norms;
    bool pr_mark_bp;
    bool pr_mark_rpt;
};

CodeshareSetsData FromDB(DB::TQuery& Qry) {
    return CodeshareSetsData{
        .id = Qry.FieldAsInteger("id"),
        .first_date = Qry.FieldAsDateTime("first_date"),
        .last_date = Qry.FieldIsNULL("last_date") ? ASTRA::NoExists : Qry.FieldAsDateTime("last_date"),
        .days = Qry.FieldAsString("days"),
        .airline_oper = Qry.FieldAsString("airline_oper"),
        .flt_no_oper = Qry.FieldAsInteger("flt_no_oper"),
        .suffix_oper = Qry.FieldAsString("suffix_oper"),
        .airp_dep = Qry.FieldAsString("airp_dep"),
        .airline_mark = Qry.FieldAsString("airline_mark"),
        .flt_no_mark = Qry.FieldAsInteger("flt_no_mark"),
        .suffix_mark = Qry.FieldAsString("suffix_mark"),
        .pr_mark_norms = Qry.FieldAsInteger("pr_mark_norms") ? true : false,
        .pr_mark_bp = Qry.FieldAsInteger("pr_mark_bp") ? true : false,
        .pr_mark_rpt = Qry.FieldAsInteger("pr_mark_rpt") ? true : false,
    };
}

std::vector<ItemIdDateDaysRange> loadRangedIdList(CodeshareSetsData& data, TDateTime first, TDateTime last)
{
    std::vector<ItemIdDateDaysRange> result;
    /*попробовать оптимизировать запрос*/
    const std::string sql =
            "SELECT id,days,first_date,last_date "
            "FROM codeshare_sets "
            "WHERE airline_oper=:airline_oper AND "
            "flt_no_oper=:flt_no_oper AND "
            "(suffix_oper IS NULL AND :suffix_oper IS NULL OR suffix_oper=:suffix_oper) AND "
            "airp_dep=:airp_dep AND "
            "airline_mark=:airline_mark AND "
            "flt_no_mark=:flt_no_mark AND "
            "(suffix_mark IS NULL AND :suffix_mark IS NULL OR suffix_mark=:suffix_mark) AND "
            "(last_date IS NULL OR last_date>:first) AND "
            "(:last IS NULL OR :last>first_date) AND "
            "pr_del=0 "
            "FOR UPDATE ";
    QParams params;
    params
            << QParam("airline_oper", otString, data.airline_oper)
            << QParam("flt_no_oper", otInteger, data.flt_no_oper)
            << QParam("suffix_oper", otString, data.suffix_oper)
            << QParam("airp_dep", otString, data.airp_dep)
            << QParam("airline_mark", otString, data.airline_mark)
            << QParam("flt_no_mark", otInteger, data.flt_no_mark)
            << QParam("suffix_mark", otString, data.suffix_mark)
            << QParam("first", otDate, first)
            << QParam("last", otDate, last, QParam::NullOnEmpty);
    DB::TCachedQuery Qry(PgOra::getRWSession("CODESHARE_SETS"), sql, params, STDLOG);
    Qry.get().Execute();

    for (;!Qry.get().Eof;Qry.get().Next()) {
        result.push_back(
                    ItemIdDateDaysRange {
                        Qry.get().FieldAsInteger("id"),
                        Qry.get().FieldAsDateTime("first_date"),
                        Qry.get().FieldIsNULL("last_date") ? ASTRA::NoExists
                                                            : Qry.get().FieldAsDateTime("last_date"),
                        Qry.get().FieldAsString("days"),
                    });
    }
    return result;
}

int save(CodeshareSetsData& data, int tid, TDateTime first, TDateTime last, const std::string& days)
{
    const int new_id = PgOra::getSeqNextVal_int("ID__SEQ");
    const std::string sql =
            "INSERT INTO codeshare_sets("
            "id,airline_oper,flt_no_oper,suffix_oper,airp_dep,airline_mark,flt_no_mark,suffix_mark, "
            "pr_mark_norms,pr_mark_bp,pr_mark_rpt,days,first_date,last_date,pr_del,tid"
            ") VALUES ("
            ":new_id,:airline_oper,:flt_no_oper,:suffix_oper,:airp_dep,:airline_mark,:flt_no_mark,:suffix_mark, "
            ":pr_mark_norms,:pr_mark_bp,:pr_mark_rpt,:days,:first_date,:last_date,0,:tid "
            ")";

    QParams params;
    params << QParam("new_id", otInteger, new_id)
           << QParam("airline_oper", otString, data.airline_oper)
           << QParam("flt_no_oper", otInteger, data.flt_no_oper)
           << QParam("suffix_oper", otString, data.suffix_oper)
           << QParam("airp_dep", otString, data.airp_dep)
           << QParam("airline_mark", otString, data.airline_mark)
           << QParam("flt_no_mark", otInteger, data.flt_no_mark)
           << QParam("suffix_mark", otString, data.suffix_mark)
           << QParam("pr_mark_norms", otInteger, data.pr_mark_norms)
           << QParam("pr_mark_bp", otInteger, data.pr_mark_bp)
           << QParam("pr_mark_rpt", otInteger, data.pr_mark_rpt)
           << QParam("days", otString, days)
           << QParam("first_date", otDate, first)
           << QParam("last_date", otDate, last, QParam::NullOnEmpty)
           << QParam("tid", otInteger, tid);
    DB::TCachedQuery Qry(PgOra::getRWSession("CODESHARE_SETS"), sql, params, STDLOG);
    Qry.get().Execute();
    if (Qry.get().RowsProcessed() > 0) {
        writeHistory(new_id);
    }
    return new_id;
}

std::optional<CodeshareSetsData> load(int id) {
    DB::TQuery Qry(PgOra::getRWSession("CODESHARE_SETS"), STDLOG);
    Qry.SQLText =
    /*почему то в оригинале не берется суффикс*/
    "SELECT "
    "id,airline_oper,flt_no_oper,suffix_oper,airp_dep,airline_mark,flt_no_mark,suffix_mark,"
    "pr_mark_norms,pr_mark_bp,pr_mark_rpt,days,first_date,last_date "
    "FROM codeshare_sets "
    "WHERE id=:id AND pr_del=0 "
    "FOR UPDATE";
    Qry.CreateVariable("id", otInteger, id);
    Qry.Execute();
    if (Qry.Eof) {
        return std::nullopt;
    }
    return FromDB(Qry);
}

int copyRenewDays(int id, int tid, TDateTime first_date, TDateTime last_date, const std::string& days)
{
    const int new_id = PgOra::getSeqNextVal_int("ID__SEQ");
    const std::string sql =
            "INSERT INTO codeshare_sets("
            "id,airline_oper,flt_no_oper,suffix_oper,airp_dep,airline_mark,flt_no_mark,suffix_mark, "
            "pr_mark_norms,pr_mark_bp,pr_mark_rpt,days,first_date,last_date,pr_del,tid"
            ") "
            "SELECT "
            ":new_id,airline_oper,flt_no_oper,suffix_oper,airp_dep,airline_mark,flt_no_mark,suffix_mark,"
            "pr_mark_norms,pr_mark_bp,pr_mark_rpt,:days,:first_date,:last_date,0,:tid"
            "FROM codeshare_sets "
            "WHERE id=:id";
    QParams params;
    params << QParam("new_id", otInteger, new_id)
        << QParam("tid", otInteger, tid)
        << QParam("days", otString, days)
        << QParam("first_date", otDate, first_date)
        << QParam("last_date", otDate, last_date, QParam::NullOnEmpty)
        << QParam("id", otInteger, id);
    DB::TCachedQuery Qry(PgOra::getRWSession("CODESHARE_SETS"), sql, params, STDLOG);
    Qry.get().Execute();
    if (Qry.get().RowsProcessed() > 0) {
        writeHistory(new_id);
    }
    return new_id;
}

int add(CodeshareSetsData data, int id, int tid, TDateTime first_date, TDateTime last_date, const std::string& daysh)
{
    TDateTime first = ASTRA::NoExists;
    TDateTime last = ASTRA::NoExists;
    bool pr_opd = false;

    checkParams(ASTRA::NoExists, first_date, last_date, first, last, pr_opd);

    const std::vector<ItemIdDateDaysRange> item_ids = loadRangedIdList(data, first, last);

    int idh = ASTRA::NoExists;
    int tidh = tid != ASTRA::NoExists ? tid
                                      : PgOra::getSeqNextVal_int("TID__SEQ");

    TDateTime first2 = ASTRA::NoExists;
    TDateTime last2 = ASTRA::NoExists;
    /* пробуем разбить на отрезки */
    for (const ItemIdDateDaysRange& item_id: item_ids) {
        /* проверим на пересекаемость дни недели */
        std::string days_rest;
        subtract_days(item_id.days, daysh, days_rest);
        idh = item_id.id;
        if ((item_id.days.empty() || days_rest.empty() || item_id.days != days_rest)
            && (id == ASTRA::NoExists || (id != ASTRA::NoExists && id != item_id.id))) {
            if (item_id.first_date < first) {
                /* отрезок [first_date,first) */
                if (idh != ASTRA::NoExists) {
                    updateRange(item_id.id, tidh, item_id.first_date, first);
                    idh = ASTRA::NoExists;
                } else {
                    copy(item_id.id, tidh, item_id.first_date, first);
                    idh = ASTRA::NoExists;
                }
                first2 = first;
            } else {
                first2 = item_id.first_date;
            }
            if (last != ASTRA::NoExists && (item_id.last_date == ASTRA::NoExists
                                            || item_id.last_date > last))
            {
                /* отрезок [last,last_date)  */
                if (idh != ASTRA::NoExists) {
                    updateRange(item_id.id, tidh, last, item_id.last_date);
                    idh = ASTRA::NoExists;
                } else {
                    copy(item_id.id, tidh, last, item_id.last_date);
                    idh = ASTRA::NoExists;
                }
                last2 = last;
            } else {
                last2 = item_id.last_date;
            }

            if (idh != ASTRA::NoExists) {
                if (!days_rest.empty()) {
                    /*что-то из дней осталось*/
                    updateDaysRange(item_id.id, tidh, first2, last2, days_rest);
                } else {
                    updateAsDeleted(item_id.id, tidh);
                }
            } else {
                if (!days_rest.empty()) {
                    /*что-то из дней осталось*/
                    /* вставим новую строку */
                    copyRenewDays(item_id.id, tidh, first2, last2, days_rest);
                    idh = ASTRA::NoExists;
                }
            }
        }
    }
    if (id == ASTRA::NoExists) {
        if (!pr_opd) {
            /*новый отрезок [first,last) */
            idh = save(data, tidh, first, last, daysh);
        }
    } else {
        /* при редактировании апдейтим строку */
        updateLastDate(id, tidh, last);
    }

    return idh;
}

int add(int id, const std::string& airline_oper, int flt_no_oper, const std::string& suffix_oper,
    const std::string& airp_dep, const std::string& airline_mark, int flt_no_mark, const std::string& suffix_mark,
    bool pr_mark_norms, bool pr_mark_bp, bool pr_mark_rpt,
    const std::string& days, TDateTime first_date, TDateTime last_date, bool pr_denial, int tid)
{
    std::string daysh = days;

    if (airline_oper == airline_mark && flt_no_oper == flt_no_mark && suffix_oper == suffix_mark) {
        throw AstraLocale::UserException("MSG.OPER_AND_MARK_FLIGHTS_MUST_DIFFER");
    }
    if (!normalize_days(daysh)) {
        throw AstraLocale::UserException("MSG.INVALID_FLIGHT_DAYS", AstraLocale::LParams() << AstraLocale::LParam("days", days));
    }

    CodeshareSetsData data {
        .id = id,
        .first_date = first_date,
        .last_date = last_date,
        .days = daysh,
        .airline_oper = airline_oper,
        .flt_no_oper = flt_no_oper,
        .suffix_oper = suffix_oper,
        .airp_dep = airp_dep,
        .airline_mark = airline_mark,
        .flt_no_mark = flt_no_mark,
        .suffix_mark = suffix_mark,
        .pr_mark_norms = pr_mark_norms,
        .pr_mark_bp = pr_mark_bp,
        .pr_mark_rpt = pr_mark_rpt,
    };

    return add(data, id, tid, first_date, last_date, daysh);
}

void modifyById(int id, TDateTime last_date, int tid)
{
    std::optional<CodeshareSetsData> item = load(id);
    if (item) {
        item->last_date = last_date;
        add(*item, id, tid, item->first_date, last_date, item->days);
    }
}

void deleteById(int id, int tid)
{
    std::optional<CodeshareSetsData> item = load(id);
    if (item) {
        const TDateTime now = BASIC::date_time::NowUTC();
        const int tidh = tid == ASTRA::NoExists ? PgOra::getSeqNextVal_int("TID__SEQ")
                                                : tid;
        if (item->last_date == ASTRA::NoExists || item->last_date > now) {
            if (item->first_date < now) {
                updateLastDate(id, tidh, now);
            } else {
                updateAsDeleted(id, tidh);
            }
        } else {
            /* специально чтобы в кэше появилась неизмененная строка */
            updateTid(id, tidh);
        }
    }
}

}

#ifdef XP_TESTING

#include "xp_testing.h"
#include <vector>
#include <string>
#include <tuple>

START_TEST(normalizeDays)
{
    std::vector<std::tuple<std::string, std::string, bool>> tests = {
        {    "123", "123....",  true},
        {      "1", "1......",  true},
        {      "3", "..3....",  true},
        { "234234", ".234...",  true},
        {     "57", "....5.7",  true},
        {    "007",     "007", false},
        {    " 1 ",     " 1 ", false},
        { " 2 4 5",  " 2 4 5", false},
        {"4567123",        "",  true},
    };
    for (const auto& [input, output, result]: tests) {
        std::string test = input;
        bool returned = CodeshareSet::normalize_days(test);
        fail_unless(output == test && result == returned);
    }
}
END_TEST

START_TEST(subtractDays)
{
    std::vector<std::tuple<std::string, std::string, std::string>> tests = {
        {"1234567", ".234...", "1...567"},
        {"1234567",     "246", "1.3.5.7"},
        {    "567",     "567",        ""},
        { "......",     "123",        ""},
        {       "",     "123", "...4567"},
        {    "147",     "147",        ""},
    };
    for (const auto& [minuend, subtrahend, difference]: tests) {
        std::string result;
        CodeshareSet::subtract_days(minuend, subtrahend, result);
        fail_unless(result == difference);
    }
}
END_TEST

#define SUITENAME "CodeshareSet"
TCASEREGISTER(testInitDB, testShutDBConnection)
{
    ADD_TEST(normalizeDays);
    ADD_TEST(subtractDays);
}
TCASEFINISH;
#undef SUITENAME // "CodeshareSet"

#endif // XP_TESTING