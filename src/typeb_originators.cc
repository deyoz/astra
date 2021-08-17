#include "typeb_originators.h"
#include "astra_locale_adv.h"
#include "astra_utils.h"
#include "cache_callbacks.h"
#include "db_tquery.h"
#include "PgOraConfig.h"
#include "stl_utils.h"

#define NICKNAME "EFREMOV"
#include <serverlib/slogger.h>

using AstraLocale::LParam;
using AstraLocale::LParams;
using AstraLocale::UserException;

namespace TypeB {
namespace {
bool DateIsNull(TDateTime datetime) { return static_cast<int>(datetime) == ASTRA::NoExists; }
} // namespace

void checkSitaAddr(const std::string &str,
                   const std::string &cacheTable,
                   const std::string &cacheField,
                   const std::string &lang)
{
    if (str.size() != 7 || IsLatinUpperLettersOrDigits(str) == false)
        throw UserException("MSG.TABLE.INVALID_FIELD_VALUE", LParams() << LParam("fieldname", getCacheInfo(cacheTable).fieldTitle.at(cacheField)));
}

void modifyOriginator(const RowId_t &id,
                      const TDateTime lastDate,
                      const std::string &lang,
                      const RowId_t &tid)
{
    DB::TQuery QryLock(PgOra::getRWSession("TYPEB_ORIGINATORS"), STDLOG);
    QryLock.SQLText =
        "SELECT id, airline, airp_dep, tlg_type, first_date, addr, double_sign, descr "
        "FROM typeb_originators "
        "WHERE id=:id AND pr_del=0 "
        "FOR UPDATE";

    QryLock.CreateVariable("id", otInteger, id.get());
    QryLock.Execute();

    if (QryLock.RowsProcessed() != 1)
        return;

    auto sid = RowId_t(QryLock.FieldAsInteger("id"));
    addOriginator(sid,
                  QryLock.FieldAsString("airline"),
                  QryLock.FieldAsString("airp_dep"),
                  QryLock.FieldAsString("tlg_type"),
                  QryLock.FieldAsDateTime("first_date"),
                  lastDate,
                  QryLock.FieldAsString("addr"),
                  QryLock.FieldAsString("double_sign"),
                  QryLock.FieldAsString("descr"),
                  tid,
                  lang);
}

void deleteOriginator(const RowId_t &id,
                      const RowId_t &tid)
{
    auto now = BASIC::date_time::NowUTC();

    DB::TQuery QryLock(PgOra::getRWSession("TYPEB_ORIGINATORS"), STDLOG);
    QryLock.SQLText =
        "SELECT first_date, last_date "
        "FROM typeb_originators "
        "WHERE id=:id AND pr_del=0 "
        "FOR UPDATE";

    QryLock.CreateVariable("id", otInteger, id.get());
    QryLock.Execute();

    if (QryLock.RowsProcessed() != 1)
        return;

    auto firstDate = QryLock.FieldAsDateTime("first_date");
    auto lastDate  = QryLock.FieldAsDateTime("last_date");

    int tidh = tid.get();
    if (tidh == ASTRA::NoExists)
        tidh = PgOra::getSeqNextVal_int("TID__SEQ");

    if (DateIsNull(lastDate) || lastDate > now) {
        DB::TQuery QryUpd(PgOra::getRWSession("TYPEB_ORIGINATORS"), STDLOG);

        if (firstDate < now) {
            QryUpd.SQLText =
                "UPDATE typeb_originators "
                "SET last_date=:now, tid=:tidh "
                "WHERE id=:id";
            QryUpd.CreateVariable("now", otDate, now);
        } else {
            QryUpd.SQLText =
                "UPDATE typeb_originators "
                "SET pr_del=1, tid=:tidh "
                "WHERE id=:id";
        }

        QryUpd.CreateVariable("tidh", otInteger, tidh);
        QryUpd.CreateVariable("id", otInteger, id.get());
        QryUpd.Execute();

        HistoryTable("typeb_originators").synchronize(id);
    } else {
        /* специально чтобы в кэше появилась неизмененная строка */
        DB::TQuery QryUpd(PgOra::getRWSession("TYPEB_ORIGINATORS"), STDLOG);
        QryUpd.SQLText =
            "UPDATE typeb_originators "
            "SET tid=:tidh "
            "WHERE id=:id";

        QryUpd.CreateVariable("tidh", otInteger, tidh);
        QryUpd.CreateVariable("id", otInteger, id.get());
        QryUpd.Execute();
    }
}

void addOriginator(RowId_t &id,
                   const std::string &airline,
                   const std::string &airpDep,
                   const std::string &tlgType,
                   const BASIC::date_time::TDateTime firstDate,
                   const BASIC::date_time::TDateTime lastDate,
                   const std::string &addr,
                   const std::string &doubleSign,
                   const std::string &descr,
                   const RowId_t &tid,
                   const std::string &lang)
{
    TDateTime first = ASTRA::NoExists;
    TDateTime last = ASTRA::NoExists;
    bool prOpd = false;

    checkPeriod(id.get() == ASTRA::NoExists, firstDate, lastDate, BASIC::date_time::NowUTC(), first, last, prOpd);

    if (prOpd == false) {
        if (addr.empty())
            throw UserException("MSG.TABLE.NOT_SET_FIELD_VALUE",
                                LParams() << LParam("fieldname", getCacheInfo("TYPEB_ORIGINATORS").fieldTitle.at("ADDR")));
        else
            checkSitaAddr(addr, "TYPEB_ORIGINATORS", "ADDR", lang);

        if (doubleSign.empty() == false && (doubleSign.size() != 2 || IsUpperLettersOrDigits(doubleSign) == false))
            throw UserException("MSG.TABLE.INVALID_FIELD_VALUE",
                                LParams() << LParam("fieldname", getCacheInfo("TYPEB_ORIGINATORS").fieldTitle.at("DOUBLE_SIGN")));

        if (descr.empty())
            throw UserException("MSG.TABLE.NOT_SET_FIELD_VALUE",
                                LParams() << LParam("fieldname", getCacheInfo("TYPEB_ORIGINATORS").fieldTitle.at("DESCR")));
    }

    int tidh = tid.get();
    if (tidh == ASTRA::NoExists)
        tidh = PgOra::getSeqNextVal_int("TID__SEQ");

    DB::TQuery QryLock(PgOra::getRWSession("TYPEB_ORIGINATORS"), STDLOG);
    QryLock.SQLText =
        "SELECT id, first_date, last_date "
        "FROM typeb_originators "
        "WHERE (airline  IS NULL AND NULLIF(:airline, '')  IS NULL OR airline=:airline) AND "
              "(airp_dep IS NULL AND NULLIF(:airp_dep, '') IS NULL OR airp_dep=:airp_dep) AND "
              "(tlg_type IS NULL AND NULLIF(:tlg_type, '') IS NULL OR tlg_type=:tlg_type) AND "
              "(last_date IS NULL OR last_date>:first) AND ";

    if (DateIsNull(last))
        QryLock.SQLText += "(1=1 OR :last>first_date) AND pr_del=0 FOR UPDATE";
    else
        QryLock.SQLText += "(0=1 OR :last>first_date) AND pr_del=0 FOR UPDATE";

    QryLock.CreateVariable(":airline", otString, airline);
    QryLock.CreateVariable(":airp_dep", otString, airpDep);
    QryLock.CreateVariable(":tlg_type", otString, tlgType);
    QryLock.CreateVariable(":first", otDate, first);
    QryLock.CreateVariable(":last", otDate, last);
    QryLock.Execute();

    /* пробуем разбить на отрезки */
    for (; QryLock.Eof; QryLock.Next()) {
        auto rowId = QryLock.FieldAsInteger("id");
        auto rowFirstDate = QryLock.FieldAsInteger("first_date");
        auto rowLastDate = QryLock.FieldAsInteger("last_date");
        auto idh = rowId;

        if (id.get() == ASTRA::NoExists || (id.get() != ASTRA::NoExists && id.get() != rowId)) {

            if (rowFirstDate < first) {
                /* отрезок [first_date,first) */
                if (idh != ASTRA::NoExists) {
                    DB::TQuery QryUpd(PgOra::getRWSession("TYPEB_ORIGINATORS"), STDLOG);
                    QryUpd.SQLText =
                        "UPDATE typeb_originators "
                        "SET first_date=:rowFirstDate, last_date=:first, tid=:tidh "
                        "WHERE id=:rowId";
                    QryUpd.CreateVariable(":rowFirstDate", otDate, rowFirstDate);
                    QryUpd.CreateVariable(":first", otDate, first);
                    QryUpd.CreateVariable(":tidh", otInteger, tidh);
                    QryUpd.CreateVariable(":rowId", otInteger, rowId);
                    QryUpd.Execute();

                    HistoryTable("typeb_originators").synchronize(RowId_t(rowId));
                    idh = ASTRA::NoExists;
                } else {
                    idh = PgOra::getSeqNextVal_int("ID__SEQ");
                    DB::TQuery QryIns(PgOra::getRWSession("TYPEB_ORIGINATORS"), STDLOG);
                    QryIns.SQLText =
                        "INSERT INTO typeb_originators ("
                            "id, "
                            "airline, "
                            "airp_dep, "
                            "tlg_type, "
                            "first_date, "
                            "last_date, "
                            "addr, "
                            "double_sign, "
                            "descr, "
                            "pr_del, "
                            "tid) "
                        "SELECT "
                            ":idh, "
                            "airline, "
                            "airp_dep, "
                            "tlg_type, "
                            ":rowFirstDate, "
                            ":first, "
                            "addr, "
                            "double_sign, "
                            "descr, "
                            "0, "
                            ":tidh "
                        "FROM typeb_originators "
                        "WHERE id=:rowId";

                    QryIns.CreateVariable("idh", otInteger, idh);
                    QryIns.CreateVariable("rowFirstDate", otDate, rowFirstDate);
                    QryIns.CreateVariable("first", otDate, first);
                    QryIns.CreateVariable("tidh", otInteger, tidh);
                    QryIns.CreateVariable("rowId", otInteger, rowId);
                    QryIns.Execute();

                    HistoryTable("typeb_originators").synchronize(RowId_t(idh));
                    idh = ASTRA::NoExists;
                }
            }

            if (DateIsNull(last) == false && (DateIsNull(rowLastDate) || rowLastDate > last)) {
                /* отрезок [last,last_date)  */
                if (idh != ASTRA::NoExists) {
                    DB::TQuery QryUpd(PgOra::getRWSession("TYPEB_ORIGINATORS"), STDLOG);
                    QryUpd.SQLText =
                        "UPDATE typeb_originators "
                        "SET first_date=:last, last_date=:rowLastDate, tid=:tidh "
                        "WHERE id=:rowId";
                    QryUpd.CreateVariable(":last", otDate, last);
                    QryUpd.CreateVariable(":rowLastDate", otDate, rowLastDate);
                    QryUpd.CreateVariable(":tidh", otInteger, tidh);
                    QryUpd.CreateVariable(":rowId", otInteger, rowId);
                    QryUpd.Execute();

                    HistoryTable("typeb_originators").synchronize(RowId_t(rowId));
                    idh = ASTRA::NoExists;
                } else {
                    idh = PgOra::getSeqNextVal_int("ID__SEQ");
                    DB::TQuery QryIns(PgOra::getRWSession("TYPEB_ORIGINATORS"), STDLOG);
                    QryIns.SQLText =
                        "INSERT INTO typeb_originators ("
                            "id, "
                            "airline, "
                            "airp_dep, "
                            "tlg_type, "
                            "first_date, "
                            "last_date, "
                            "addr, "
                            "double_sign, "
                            "descr, "
                            "pr_del, "
                            "tid) "
                        "SELECT "
                            ":idh, "
                            "airline, "
                            "airp_dep, "
                            "tlg_type, "
                            ":last, "
                            ":rowLastDate, "
                            "addr, "
                            "double_sign, "
                            "descr, "
                            "0, "
                            ":tidh "
                        "FROM typeb_originators "
                        "WHERE id=:rowId";

                    QryIns.CreateVariable("idh", otInteger, idh);
                    QryIns.CreateVariable("last", otDate, last);
                    QryIns.CreateVariable("rowLastDate", otDate, rowLastDate);
                    QryIns.CreateVariable("tidh", otInteger, tidh);
                    QryIns.CreateVariable("rowId", otInteger, rowId);
                    QryIns.Execute();

                    HistoryTable("typeb_originators").synchronize(RowId_t(idh));
                    idh = ASTRA::NoExists;
                }
            }

            if (idh != ASTRA::NoExists) {
                DB::TQuery QryUpd(PgOra::getRWSession("TYPEB_ORIGINATORS"), STDLOG);
                QryUpd.SQLText =
                    "UPDATE typeb_originators "
                    "SET pr_del=1, tid=:tidh "
                    "WHERE id=:rowId";
                QryUpd.CreateVariable(":tidh", otInteger, tidh);
                QryUpd.CreateVariable(":rowId", otInteger, rowId);
                QryUpd.Execute();

                HistoryTable("typeb_originators").synchronize(RowId_t(rowId));
            }
        }
    }

    if (id.get() == ASTRA::NoExists) {
        if (prOpd == false) {
            /*новый отрезок [first,last) */
            id = RowId_t(PgOra::getSeqNextVal_int("ID__SEQ"));
            DB::TQuery QryIns(PgOra::getRWSession("TYPEB_ORIGINATORS"), STDLOG);
            QryIns.SQLText =
                "INSERT INTO typeb_originators ("
                    "id, "
                    "airline, "
                    "airp_dep, "
                    "tlg_type, "
                    "first_date, "
                    "last_date, "
                    "addr, "
                    "double_sign, "
                    "descr, "
                    "pr_del, "
                    "tid) "
                "VALUES ("
                    ":id, "
                    ":airline, "
                    ":airp_dep, "
                    ":tlg_type, "
                    ":first, "
                    ":last, "
                    ":addr, "
                    ":double_sign, "
                    ":descr, "
                    "0, "
                    ":tidh) "
                "RETURNING id AS vid";

            QryIns.CreateVariable("id", otInteger, id.get());
            QryIns.CreateVariable("airline", otString, airline);
            QryIns.CreateVariable("airp_dep", otString, airpDep);
            QryIns.CreateVariable("tlg_type", otString, tlgType);
            QryIns.CreateVariable("first", otDate, first);
            QryIns.CreateVariable("last", otDate, last);
            QryIns.CreateVariable("addr", otString, addr);
            QryIns.CreateVariable("double_sign", otString, doubleSign);
            QryIns.CreateVariable("descr", otString, descr);
            QryIns.CreateVariable("tidh", otInteger, tidh);
            QryIns.Execute();

            id = RowId_t(QryIns.FieldAsInteger("vid"));
            HistoryTable("typeb_originators").synchronize(id);
        }
    } else {
        /* при редактировании апдейтим строку */
        DB::TQuery QryUpd(PgOra::getRWSession("TYPEB_ORIGINATORS"), STDLOG);
        QryUpd.SQLText =
            "UPDATE typeb_originators "
            "SET last_date=:last, tid=:tidh "
            "WHERE id=:id";
        QryUpd.CreateVariable(":last", otDate, last);
        QryUpd.CreateVariable(":tidh", otInteger, tidh);
        QryUpd.CreateVariable(":id", otInteger, id.get());
        QryUpd.Execute();

        HistoryTable("typeb_originators").synchronize(id);
    }
}

} // namespace TypeB
