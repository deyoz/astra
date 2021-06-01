#include "kassa.h"

#define STDLOG NICKNAME,__FILE__,__LINE__
#define NICKNAME "VLAD"

#include "serverlib/slogger.h"
#include "date_time.h"
#include "astra_consts.h"
#include "exceptions.h"
#include "qrys.h"
#include "db_tquery.h"
#include <serverlib/dbcpp_cursctl.h>
#include "PgOraConfig.h"

namespace Kassa {

namespace {

void checkPeriod(bool pr_new,
                 TDateTime first_date,
                 TDateTime last_date,
                 TDateTime now,
                 TDateTime& first,
                 TDateTime& last,
                 bool& pr_opd)
{
    if (first_date == ASTRA::NoExists && last_date == ASTRA::NoExists) {
        throw AstraLocale::UserException("MSG.TABLE.NOT_SET_RANGE");
    }
    std::modf(first_date, &first);
    std::modf(last_date, &last);

    if (first_date != ASTRA::NoExists
        && last_date != ASTRA::NoExists
        && first > last)
    {
        throw AstraLocale::UserException("MSG.TABLE.INVALID_RANGE");
    }

    TDateTime today;
    std::modf(now,&today);

    if (first != ASTRA::NoExists) {
        if (first < today) {
            if (pr_new) {
                throw AstraLocale::UserException("MSG.TABLE.FIRST_DATE_BEFORE_TODAY");
            } else {
                first = today;
            }
        }
        if (first == today) {
            first = now;
        }
        pr_opd = false;
    } else {
        pr_opd = true;
    }

    if (last != ASTRA::NoExists) {
        if (last < today) {
            throw AstraLocale::UserException("MSG.TABLE.LAST_DATE_BEFORE_TODAY");
        }
        last = last + 1;
    }
    if (pr_opd) {
        first = last;
        last  = ASTRA::NoExists;
    }
}

void checkParams(int id, TDateTime first_date, TDateTime last_date,
                 TDateTime& first, TDateTime& last, bool& pr_opd)
{
    checkPeriod(id == ASTRA::NoExists, first_date, last_date, BASIC::date_time::NowUTC(), first, last, pr_opd);
}

void updateBagTableRange(const std::string& table_name,
                         int id, int tid, TDateTime first_date, TDateTime last_date,
                         const std::string& setting_user, const std::string& station)
{
    const std::string sql =
            "UPDATE " + table_name + " SET "
            "first_date=:first_date,"
            "last_date=:last_date,"
            "tid=:tid "
            "WHERE id=:id";
    QParams params;
    params << QParam("first_date", otDate, first_date)
           << QParam("last_date", otDate, last_date, QParam::NullOnEmpty)
           << QParam("tid", otInteger, tid)
           << QParam("id", otInteger, id);
    DB::TCachedQuery Qry(PgOra::getRWSession(table_name), sql, params, STDLOG);
    Qry.get().Execute();
    if (Qry.get().RowsProcessed() > 0) {
        ASTRA::syncHistory(table_name, id, setting_user, station);
    }
}

void updateBagTableAsDeleted(const std::string& table_name, int id, int tid,
                             const std::string& setting_user, const std::string& station)
{
    const std::string sql =
            "UPDATE " + table_name + " SET "
            "pr_del=1,"
            "tid=:tid "
            "WHERE id=:id";
    QParams params;
    params << QParam("tid", otInteger, tid)
           << QParam("id", otInteger, id);
    DB::TCachedQuery Qry(PgOra::getRWSession(table_name), sql, params, STDLOG);
    Qry.get().Execute();
    if (Qry.get().RowsProcessed() > 0) {
        ASTRA::syncHistory(table_name, id, setting_user, station);
    }
}

void updateBagTableLastDate(const std::string& table_name, int id, int tid, TDateTime last_date,
                            const std::string& setting_user, const std::string& station)
{
    const std::string sql =
            "UPDATE " + table_name + " SET "
            "last_date=:last_date,"
            "tid=:tid "
            "WHERE id=:id";
    QParams params;
    params << QParam("last_date", otDate, last_date, QParam::NullOnEmpty)
           << QParam("tid", otInteger, tid)
           << QParam("id", otInteger, id);
    DB::TCachedQuery Qry(PgOra::getRWSession(table_name), sql, params, STDLOG);
    Qry.get().Execute();
    if (Qry.get().RowsProcessed() > 0) {
        ASTRA::syncHistory(table_name, id, setting_user, station);
    }
}

void updateBagTableTid(const std::string& table_name, int id, int tid,
                       const std::string& setting_user, const std::string& station)
{
    const std::string sql =
            "UPDATE " + table_name + " SET "
            "tid=:tid "
            "WHERE id=:id";
    QParams params;
    params << QParam("tid", otInteger, tid)
           << QParam("id", otInteger, id);
    DB::TCachedQuery Qry(PgOra::getRWSession(table_name), sql, params, STDLOG);
    Qry.get().Execute();
    if (Qry.get().RowsProcessed() > 0) {
        ASTRA::syncHistory(table_name, id, setting_user, station);
    }
}

struct ItemIdDateRange
{
    int id = ASTRA::NoExists;
    TDateTime first_date;
    TDateTime last_date;
};

class BaseRangedItem
{
public:
    virtual ~BaseRangedItem() {}
protected:
    virtual std::string tableName() const = 0;
    virtual void checkItem() const {}
    void updateRange(int id, int tid, TDateTime first_date, TDateTime last_date,
                     const std::string& setting_user,
                     const std::string& station) const
    {
        updateBagTableRange(tableName(), id, tid, first_date, last_date, setting_user, station);
    }

    void updateAsDeleted(int id, int tid,
                         const std::string& setting_user,
                         const std::string& station) const
    {
        updateBagTableAsDeleted(tableName(), id, tid, setting_user, station);
    }

    void updateLastDate(int id, int tid, TDateTime last_date,
                        const std::string& setting_user,
                        const std::string& station) const
    {
        updateBagTableLastDate(tableName(), id, tid,  last_date, setting_user, station);
    }

    void updateTid(int id, int tid,
                   const std::string& setting_user,
                   const std::string& station) const
    {
        updateBagTableTid(tableName(), id, tid, setting_user, station);
    }

    virtual int save(int tid, const std::string& setting_user, const std::string& station) const = 0;
    virtual int copy(int id, int tid, TDateTime first_date, TDateTime last_date,
                     const std::string& setting_user, const std::string& station) const = 0;
    virtual std::vector<ItemIdDateRange> loadRangedIdList(TDateTime first, TDateTime last) const = 0;

    template<typename TItem>
    static void modifyItemById(TItem& item, int id, TDateTime last_date, const std::string& setting_user,
                               const std::string& station, int tid = ASTRA::NoExists)
    {
        item.last_date = last_date;
        item.add(id, tid, item.first_date, item.last_date, setting_user, station);
    }

    template<typename TItem>
    static void deleteItemById(const TItem& item, int id, const std::string& setting_user, const std::string& station,
                               int tid = ASTRA::NoExists)
    {
        const TDateTime now = BASIC::date_time::NowUTC();
        const int tidh = tid == ASTRA::NoExists ? PgOra::getSeqNextVal_int("TID__SEQ")
                                                : tid;
        if (item.last_date == ASTRA::NoExists || item.last_date > now) {
            if (item.first_date < now) {
                item.updateLastDate(id, tidh, now, setting_user, station);
            } else {
                item.updateAsDeleted(id, tidh, setting_user, station);
            }
        } else {
            /* специально чтобы в кэше появилась неизмененная строка */
            item.updateTid(id, tidh, setting_user, station);
        }
    }

    template<typename TItem>
    static void copyBasicItems(const std::vector<TItem>& items, const std::string& airline,
                               const std::string& setting_user, const std::string& station)
    {
        const TDateTime now = BASIC::date_time::NowUTC();
        const int tidh = PgOra::getSeqNextVal_int("TID__SEQ");
        for (const TItem& item: items) {
            if (item.last_date == ASTRA::NoExists || item.last_date > now) {
                if (item.first_date < now) {
                    item.updateLastDate(item.id, tidh, now, setting_user, station);
                } else {
                    item.updateAsDeleted(item.id, tidh, setting_user, station);
                }
            }
        }
        TItem::copy(airline, tidh, now, setting_user, station);
    }

public:
    int add(int id, int tid, TDateTime first_date, TDateTime last_date,
            const std::string& setting_user, const std::string& station) const;
};

int BaseRangedItem::add(int id, int tid, TDateTime first_date, TDateTime last_date,
                        const std::string& setting_user, const std::string& station) const
{
    TDateTime first = ASTRA::NoExists;
    TDateTime last = ASTRA::NoExists;
    bool pr_opd = false;
    checkParams(id, first_date, last_date, first, last, pr_opd);
    if (!pr_opd) {
        checkItem();
    }
    const std::vector<ItemIdDateRange> item_ids = loadRangedIdList(first, last);

    int idh = ASTRA::NoExists;
    int tidh = tid != ASTRA::NoExists ? tid
                                      : PgOra::getSeqNextVal_int("TID__SEQ");
    /* пробуем разбить на отрезки */
    for (const ItemIdDateRange& item_id: item_ids) {
        idh = item_id.id;
        if (id == ASTRA::NoExists || (id != ASTRA::NoExists && id != item_id.id)) {
            if (item_id.first_date < first) {
                /* отрезок [first_date,first) */
                if (idh != ASTRA::NoExists) {
                    updateRange(item_id.id, tid, item_id.first_date, first, setting_user, station);
                    idh = ASTRA::NoExists;
                } else {
                    copy(item_id.id, tidh, item_id.first_date, first, setting_user, station);
                    idh = ASTRA::NoExists;
                }
            }
            if (last != ASTRA::NoExists && (item_id.last_date == ASTRA::NoExists
                                            || item_id.last_date > last))
            {
                /* отрезок [last,last_date)  */
                if (idh != ASTRA::NoExists) {
                    updateRange(item_id.id, tid, last, item_id.last_date, setting_user, station);
                    idh = ASTRA::NoExists;
                } else {
                    copy(item_id.id, tidh, last, item_id.last_date, setting_user, station);
                    idh = ASTRA::NoExists;
                }
            }

            if (idh != ASTRA::NoExists) {
                updateAsDeleted(item_id.id, tidh, setting_user, station);
            }
        }
    }
    if (id == ASTRA::NoExists) {
        if (!pr_opd) {
            /*новый отрезок [first,last) */
            idh = save(tidh, setting_user, station);
        }
    } else {
        /* при редактировании апдейтим строку */
        updateLastDate(id, tidh, last, setting_user, station);
    }

    return idh;
}

} // namespace

// BAG_NORMS

namespace BagNorm {
namespace {

struct BagNormData
{
    int id = ASTRA::NoExists;
    std::string airline;
    std::optional<bool> pr_trfer;
    std::string city_dep;
    std::string city_arv;
    std::string pax_cat;
    std::string subclass;
    std::string cls;
    int flt_no = ASTRA::NoExists;
    std::string craft;
    std::string trip_type;
    TDateTime first_date;
    TDateTime last_date;
    int bag_type = ASTRA::NoExists;
    int amount = ASTRA::NoExists;
    int weight = ASTRA::NoExists;
    int per_unit = ASTRA::NoExists;
    std::string norm_type;
    std::string extra;
};

class BagNormItem: public BagNormData, public BaseRangedItem
{
public:
    BagNormItem(const BagNormData& data): BagNormData(data) {}
    virtual ~BagNormItem() {}

protected:
    std::string tableName() const { return "bag_norms"; }
    static BagNormItem fromDB(DB::TQuery& Qry);

    int save(int tid, const std::string& setting_user, const std::string& station) const;
    int copy(int id, int tid, TDateTime first_date, TDateTime last_date,
             const std::string& setting_user, const std::string& station) const;
    std::vector<ItemIdDateRange> loadRangedIdList(TDateTime first, TDateTime last) const;
    static std::optional<BagNormItem> load(int id);
    static std::vector<BagNormItem> load(const std::string& airline);

public:
    static void copy(const std::string& airline, int tid, TDateTime now,
                     const std::string& setting_user, const std::string& station);
    static void modifyById(int id, TDateTime last_date, const std::string& setting_user,
                           const std::string& station, int tid = ASTRA::NoExists)
    {
        std::optional<BagNormItem> item = load(id);
        if (item) {
            modifyItemById(*item, id, last_date, setting_user, station, tid);
        }
    }

    static void deleteById(int id, const std::string& setting_user, const std::string& station,
                           int tid = ASTRA::NoExists)
    {
        std::optional<BagNormItem> item = load(id);
        if (item) {
            deleteItemById(*item, id, setting_user, station, tid);
        }
    }

    static void copyBasic(const std::string& airline, const std::string& setting_user,
                          const std::string& station)
    {
        const std::vector<BagNormItem> items = BagNormItem::load(airline);
        copyBasicItems(items, airline, setting_user, station);
    }
};

int BagNormItem::save(int tid, const std::string& setting_user, const std::string& station) const
{
    const int new_id = PgOra::getSeqNextVal_int("ID__SEQ");
    const std::string sql =
            "INSERT INTO bag_norms("
            "id,airline,pr_trfer,city_dep,city_arv,pax_cat,subclass,class,flt_no,craft,trip_type, "
            "first_date,last_date,bag_type,amount,weight,per_unit,norm_type,extra,pr_del,direct_action,tid"
            ") VALUES ("
            ":new_id,:airline,:pr_trfer,:city_dep,:city_arv,:pax_cat,:subclass,:class,:flt_no,:craft,:trip_type, "
            ":first_date,:last_date,:bag_type,:amount,:weight,:per_unit,:norm_type,:extra,0,0,:tid "
            ")";
    QParams params;
    params << QParam("new_id", otInteger, new_id)
           << QParam("airline", otString, airline);
    if (pr_trfer) {
        params << QParam("pr_trfer", otInteger, int(*pr_trfer));
    } else {
        params << QParam("pr_trfer", otInteger, FNull);
    }
    params << QParam("city_dep", otString, city_dep)
           << QParam("city_arv", otString, city_arv)
           << QParam("pax_cat", otString, pax_cat)
           << QParam("subclass", otString, subclass)
           << QParam("class", otString, cls)
           << QParam("flt_no", otInteger, flt_no, QParam::NullOnEmpty)
           << QParam("craft", otString, craft)
           << QParam("trip_type", otString, trip_type)
           << QParam("first_date", otDate, first_date)
           << QParam("last_date", otDate, last_date, QParam::NullOnEmpty)
           << QParam("bag_type", otInteger, bag_type)
           << QParam("amount", otInteger, amount, QParam::NullOnEmpty)
           << QParam("weight", otInteger, weight, QParam::NullOnEmpty)
           << QParam("per_unit", otInteger, per_unit, QParam::NullOnEmpty)
           << QParam("norm_type", otString, norm_type)
           << QParam("extra", otString, extra)
           << QParam("tid", otInteger, tid);
    DB::TCachedQuery Qry(PgOra::getRWSession("BAG_NORMS"), sql, params, STDLOG);
    Qry.get().Execute();
    if (Qry.get().RowsProcessed() > 0) {
        ASTRA::syncHistory("bag_norms", new_id, setting_user, station);
    }
    return new_id;
}

int BagNormItem::copy(int id, int tid, TDateTime first_date, TDateTime last_date,
                      const std::string& setting_user, const std::string& station) const
{
    const int new_id = PgOra::getSeqNextVal_int("ID__SEQ");
    const std::string sql =
            "INSERT INTO bag_norms("
            "id,airline,pr_trfer,city_dep,city_arv,pax_cat,subclass,class,flt_no,craft,trip_type, "
            "first_date,last_date,bag_type,amount,weight,per_unit,norm_type,extra,pr_del,direct_action,tid"
            ") "
            "SELECT "
            ":new_id,airline,pr_trfer,city_dep,city_arv,pax_cat,subclass,class,flt_no,craft,trip_type, "
            ":first_date,:last_date,bag_type,amount,weight,per_unit,norm_type,extra,0,0,:tid "
            "FROM bag_norms "
            "WHERE id=:id";
    QParams params;
    params << QParam("new_id", otInteger, new_id)
           << QParam("tid", otInteger, tid)
           << QParam("first_date", otDate, first_date)
           << QParam("last_date", otDate, last_date, QParam::NullOnEmpty)
           << QParam("id", otInteger, id);
    DB::TCachedQuery Qry(PgOra::getRWSession("BAG_NORMS"), sql, params, STDLOG);
    Qry.get().Execute();
    if (Qry.get().RowsProcessed() > 0) {
        ASTRA::syncHistory("bag_norms", new_id, setting_user, station);
    }
    return new_id;
}

void BagNormItem::copy(const std::string& airline, int tid, TDateTime now,
                       const std::string& setting_user, const std::string& station)
{
    int new_id = PgOra::getSeqNextVal_int("ID__SEQ");
    const std::string sql =
            "INSERT INTO bag_norms("
            "id,airline,pr_trfer,city_dep,city_arv,pax_cat,subclass,class,flt_no,craft,trip_type, "
            "first_date,last_date,bag_type,amount,weight,per_unit,norm_type,extra,pr_del,direct_action,tid"
            ") "
            "SELECT "
            ":new_id,:airline,pr_trfer,city_dep,city_arv,pax_cat,subclass,class,flt_no,craft,trip_type, "
            "CASE WHEN SIGN(:now - first_date) = 1 THEN :now ELSE first_date END,"
            "last_date,bag_type,amount,weight,per_unit,norm_type,extra,pr_del,direct_action,:tid "
            "FROM bag_norms "
            "WHERE airline IS NULL AND pr_del=0 AND direct_action=0 AND (last_date IS NULL OR last_date > :now)";
    QParams params;
    params << QParam("new_id", otInteger, new_id)
           << QParam("airline", otString, airline)
           << QParam("now", otDate, now)
           << QParam("tid", otInteger, tid);
    DB::TCachedQuery Qry(PgOra::getRWSession("BAG_NORMS"), sql, params, STDLOG);
    Qry.get().Execute();
    if (Qry.get().RowsProcessed() > 0) {
        ASTRA::syncHistory("bag_norms", new_id, setting_user, station);
    }
}

std::vector<ItemIdDateRange> BagNormItem::loadRangedIdList(TDateTime first, TDateTime last) const
{
    std::vector<ItemIdDateRange> result;
    /*попробовать оптимизировать запрос*/
    const std::string sql =
            "SELECT id,first_date,last_date "
            "FROM bag_norms "
            "WHERE (airline IS NULL AND :airline IS NULL OR airline=:airline) AND "
            "      (pr_trfer IS NULL AND :pr_trfer IS NULL OR pr_trfer=:pr_trfer) AND "
            "      (city_dep IS NULL AND :city_dep IS NULL OR city_dep=:city_dep) AND "
            "      (city_arv IS NULL AND :city_arv IS NULL OR city_arv=:city_arv) AND "
            "      (pax_cat IS NULL AND :pax_cat IS NULL OR pax_cat=:pax_cat) AND "
            "      (subclass IS NULL AND :subclass IS NULL OR subclass=:subclass) AND "
            "      (class IS NULL AND :class IS NULL OR class=:class) AND "
            "      (flt_no IS NULL AND :flt_no IS NULL OR flt_no=:flt_no) AND "
            "      (craft IS NULL AND :craft IS NULL OR craft=:craft) AND "
            "      (trip_type IS NULL AND :trip_type IS NULL OR trip_type=:trip_type) AND "
            "      (bag_type IS NULL AND :bag_type IS NULL OR bag_type=:bag_type) AND "
            "      (last_date IS NULL OR last_date>:first) AND "
            "      (:last IS NULL OR :last>first_date) AND "
            "      pr_del=0 AND direct_action=0 "
            "FOR UPDATE ";
    QParams params;
    params << QParam("airline", otString, airline);
    if (pr_trfer) {
        params << QParam("pr_trfer", otInteger, int(*pr_trfer));
    } else {
        params << QParam("pr_trfer", otInteger, FNull);
    }
    params << QParam("city_dep", otString, city_dep)
           << QParam("city_arv", otString, city_arv)
           << QParam("pax_cat", otString, pax_cat)
           << QParam("subclass", otString, subclass)
           << QParam("class", otString, cls)
           << QParam("flt_no", otInteger, flt_no, QParam::NullOnEmpty)
           << QParam("craft", otString, craft)
           << QParam("trip_type", otString, trip_type)
           << QParam("bag_type", otInteger, bag_type, QParam::NullOnEmpty)
           << QParam("first", otDate, first)
           << QParam("last", otDate, last, QParam::NullOnEmpty);
    DB::TCachedQuery Qry(PgOra::getRWSession("BAG_NORMS"), sql, params, STDLOG);
    Qry.get().Execute();

    for (;!Qry.get().Eof;Qry.get().Next()) {
        result.push_back(
                    ItemIdDateRange {
                        Qry.get().FieldAsInteger("id"),
                        Qry.get().FieldAsDateTime("first_date"),
                        Qry.get().FieldIsNULL("last_date") ? ASTRA::NoExists
                                                           : Qry.get().FieldAsDateTime("last_date")
                    });
    }
    return result;
}

BagNormItem BagNormItem::fromDB(DB::TQuery& Qry)
{
    return BagNormItem(
      BagNormData {
        Qry.FieldAsInteger("id"),
        Qry.FieldAsString("airline"),
        Qry.FieldIsNULL("pr_trfer") ? std::nullopt : std::make_optional(bool(Qry.FieldAsInteger("pr_trfer"))),
        Qry.FieldAsString("city_dep"),
        Qry.FieldAsString("city_arv"),
        Qry.FieldAsString("pax_cat"),
        Qry.FieldAsString("subclass"),
        Qry.FieldAsString("class"),
        Qry.FieldIsNULL("flt_no") ? ASTRA::NoExists : Qry.FieldAsInteger("flt_no"),
        Qry.FieldAsString("craft"),
        Qry.FieldAsString("trip_type"),
        Qry.FieldAsDateTime("first_date"),
        Qry.FieldIsNULL("last_date") ? ASTRA::NoExists : Qry.FieldAsDateTime("last_date"),
        Qry.FieldIsNULL("bag_type") ? ASTRA::NoExists : Qry.FieldAsInteger("bag_type"),
        Qry.FieldIsNULL("amount") ? ASTRA::NoExists : Qry.FieldAsInteger("amount"),
        Qry.FieldIsNULL("weight") ? ASTRA::NoExists : Qry.FieldAsInteger("weight"),
        Qry.FieldIsNULL("per_unit") ? ASTRA::NoExists : Qry.FieldAsInteger("per_unit"),
        Qry.FieldAsString("norm_type"),
        Qry.FieldAsString("extra")
    });
}

std::optional<BagNormItem> BagNormItem::load(int id)
{
    DB::TQuery Qry(PgOra::getRWSession("BAG_NORMS"), STDLOG);
    Qry.SQLText =
            "SELECT id,airline,pr_trfer,city_dep,city_arv,pax_cat,subclass,class,flt_no,craft,trip_type,"
            "       first_date,last_date,bag_type,amount,weight,per_unit,norm_type,extra "
            "FROM bag_norms "
            "WHERE id=:id "
            "AND pr_del=0 "
            "AND direct_action=0 "
            "FOR UPDATE";
    Qry.CreateVariable("id", otInteger, id);
    Qry.Execute();
    if (Qry.Eof) {
        return {};
    }
    return fromDB(Qry);
}

std::vector<BagNormItem> BagNormItem::load(const std::string& airline)
{
    std::vector<BagNormItem> result;
    DB::TQuery Qry(PgOra::getRWSession("BAG_NORMS"), STDLOG);
    Qry.SQLText =
            "SELECT id,airline,pr_trfer,city_dep,city_arv,pax_cat,subclass,class,flt_no,craft,trip_type,"
            "       first_date,last_date,bag_type,amount,weight,per_unit,norm_type,extra "
            "FROM bag_norms "
            "WHERE airline=:airline "
            "AND pr_del=0 "
            "AND direct_action=0 "
            "FOR UPDATE";
    Qry.CreateVariable("airline", otString, airline);
    Qry.Execute();
    for (;!Qry.Eof;Qry.Next()) {
        result.push_back(fromDB(Qry));
    }
    return result;
}

} // namespace

int add(int id, const std::string& airline, std::optional<bool> pr_trfer, const std::string& city_dep,
        const std::string& city_arv, const std::string& pax_cat, const std::string& subclass,
        const std::string& cls, int flt_no, const std::string& craft,
        const std::string& trip_type, TDateTime first_date, TDateTime last_date,
        int bag_type, int amount, int weight, int per_unit, const std::string& norm_type,
        const std::string& extra, int tid, const std::string& setting_user,
        const std::string& station)
{
    const BagNormData data = {
        id, airline, pr_trfer, city_dep, city_arv, pax_cat, subclass, cls, flt_no, craft, trip_type,
        first_date, last_date, bag_type, amount, weight, per_unit, norm_type, extra
    };
    return BagNormItem(data).add(id, tid, first_date, last_date, setting_user, station);
}

void modifyById(int id, TDateTime last_date, const std::string& setting_user,
                const std::string& station, int tid)
{
    BagNormItem::modifyById(id, last_date, setting_user, station, tid);
}

void deleteById(int id, const std::string& setting_user, const std::string& station,
                int tid)
{
    BagNormItem::deleteById(id, setting_user, station, tid);
}

void copyBasic(const std::string& airline,
               const std::string& setting_user,
               const std::string& station)
{
    BagNormItem::copyBasic(airline, setting_user, station);
}

} // namespace BagNorm

// BAG_RATES

namespace BagRate {
namespace {

struct BagRateData
{
    int id = ASTRA::NoExists;
    std::string airline;
    std::optional<bool> pr_trfer;
    std::string city_dep;
    std::string city_arv;
    std::string pax_cat;
    std::string subclass;
    std::string cls;
    int flt_no = ASTRA::NoExists;
    std::string craft;
    std::string trip_type;
    TDateTime first_date;
    TDateTime last_date;
    int bag_type = ASTRA::NoExists;
    int rate = ASTRA::NoExists;
    std::string rate_cur;
    int min_weight = ASTRA::NoExists;
    std::string extra;
};

class BagRateItem: public BagRateData, public BaseRangedItem
{
public:
    BagRateItem(const BagRateData& data): BagRateData(data) {}
    virtual ~BagRateItem() {}

protected:
    std::string tableName() const { return "bag_rates"; }
    virtual void checkItem() const;

    static BagRateItem fromDB(DB::TQuery& Qry);

    int save(int tid, const std::string& setting_user, const std::string& station) const;
    int copy(int id, int tid, TDateTime first_date, TDateTime last_date,
             const std::string& setting_user, const std::string& station) const;
    std::vector<ItemIdDateRange> loadRangedIdList(TDateTime first, TDateTime last) const;
    static std::optional<BagRateItem> load(int id);
    static std::vector<BagRateItem> load(const std::string& airline);

public:
    static void copy(const std::string& airline, int tid, TDateTime now,
                     const std::string& setting_user, const std::string& station);
    static void modifyById(int id, TDateTime last_date, const std::string& setting_user,
                           const std::string& station, int tid = ASTRA::NoExists)
    {
        std::optional<BagRateItem> item = load(id);
        if (item) {
            modifyItemById(*item, id, last_date, setting_user, station, tid);
        }
    }

    static void deleteById(int id, const std::string& setting_user, const std::string& station,
                           int tid = ASTRA::NoExists)
    {
        std::optional<BagRateItem> item = load(id);
        if (item) {
            deleteItemById(*item, id, setting_user, station, tid);
        }
    }

    static void copyBasic(const std::string& airline, const std::string& setting_user,
                          const std::string& station)
    {
        const std::vector<BagRateItem> items = BagRateItem::load(airline);
        copyBasicItems(items, airline, setting_user, station);
    }
};

void BagRateItem::checkItem() const
{
    if (rate == ASTRA::NoExists) {
        throw AstraLocale::UserException("MSG.PAYMENT.NOT_SET_BAG_RATE");
    }
    if (rate_cur.empty()) {
        throw AstraLocale::UserException("MSG.PAYMENT.NOT_SET_BAG_RATE_CUR");
    }
}

BagRateItem BagRateItem::fromDB(DB::TQuery& Qry)
{
    return BagRateItem(
      BagRateData {
        Qry.FieldAsInteger("id"),
        Qry.FieldAsString("airline"),
        Qry.FieldIsNULL("pr_trfer") ? std::nullopt : std::make_optional(bool(Qry.FieldAsInteger("pr_trfer"))),
        Qry.FieldAsString("city_dep"),
        Qry.FieldAsString("city_arv"),
        Qry.FieldAsString("pax_cat"),
        Qry.FieldAsString("subclass"),
        Qry.FieldAsString("class"),
        Qry.FieldIsNULL("flt_no") ? ASTRA::NoExists : Qry.FieldAsInteger("flt_no"),
        Qry.FieldAsString("craft"),
        Qry.FieldAsString("trip_type"),
        Qry.FieldAsDateTime("first_date"),
        Qry.FieldIsNULL("last_date") ? ASTRA::NoExists : Qry.FieldAsDateTime("last_date"),
        Qry.FieldIsNULL("bag_type") ? ASTRA::NoExists : Qry.FieldAsInteger("bag_type"),
        Qry.FieldAsInteger("rate"),
        Qry.FieldAsString("rate_cur"),
        Qry.FieldIsNULL("min_weight") ? ASTRA::NoExists : Qry.FieldAsInteger("min_weight"),
        Qry.FieldAsString("extra")
    });
}

int BagRateItem::save(int tid, const std::string& setting_user, const std::string& station) const
{
    const int new_id = PgOra::getSeqNextVal_int("ID__SEQ");
    const std::string sql =
            "INSERT INTO bag_rates("
            "id,airline,pr_trfer,city_dep,city_arv,bag_type,pax_cat,subclass,class,flt_no,craft,trip_type, "
            "first_date,last_date,amount,weight,per_unit,norm_type,extra,pr_del,tid"
            ") VALUES ("
            ":new_id,:airline,:pr_trfer,:city_dep,:city_arv,:bag_type,:pax_cat,:subclass,:class,:flt_no,:craft,:trip_type, "
            ":first_date,:last_date,:rate,:rate_cur,:min_weight,:extra,0,:tid "
            ")";
    QParams params;
    params << QParam("new_id", otInteger, new_id)
           << QParam("airline", otString, airline);
    if (pr_trfer) {
        params << QParam("pr_trfer", otInteger, int(*pr_trfer));
    } else {
        params << QParam("pr_trfer", otInteger, FNull);
    }
    params << QParam("city_dep", otString, city_dep)
           << QParam("city_arv", otString, city_arv)
           << QParam("bag_type", otInteger, bag_type, QParam::NullOnEmpty)
           << QParam("pax_cat", otString, pax_cat)
           << QParam("subclass", otString, subclass)
           << QParam("class", otString, cls)
           << QParam("flt_no", otInteger, flt_no, QParam::NullOnEmpty)
           << QParam("craft", otString, craft)
           << QParam("trip_type", otString, trip_type)
           << QParam("first_date", otDate, first_date)
           << QParam("last_date", otDate, last_date, QParam::NullOnEmpty)
           << QParam("rate", otInteger, rate)
           << QParam("rate_cur", otString, rate_cur)
           << QParam("min_weight", otInteger, min_weight, QParam::NullOnEmpty)
           << QParam("extra", otString, extra)
           << QParam("tid", otInteger, tid);
    DB::TCachedQuery Qry(PgOra::getRWSession("BAG_RATES"), sql, params, STDLOG);
    Qry.get().Execute();
    if (Qry.get().RowsProcessed() > 0) {
        ASTRA::syncHistory("bag_rates", new_id, setting_user, station);
    }
    return new_id;
}

int BagRateItem::copy(int id, int tid, TDateTime first_date, TDateTime last_date,
                      const std::string& setting_user, const std::string& station) const
{
    const int new_id = PgOra::getSeqNextVal_int("ID__SEQ");
    const std::string sql =
            "INSERT INTO bag_rates("
            "id,airline,pr_trfer,city_dep,city_arv,bag_type,pax_cat,subclass,class,flt_no,craft,trip_type, "
            "first_date,last_date,rate,rate_cur,min_weight,extra,pr_del,tid "
            ") "
            "SELECT "
            ":new_id,airline,pr_trfer,city_dep,city_arv,bag_type,pax_cat,subclass,class,flt_no,craft,trip_type, "
            ":first_date,:last_date,rate,rate_cur,min_weight,extra,0,:tid "
            "FROM bag_rates "
            "WHERE id=:id";
    QParams params;
    params << QParam("new_id", otInteger, new_id)
           << QParam("tid", otInteger, tid)
           << QParam("first_date", otDate, first_date)
           << QParam("last_date", otDate, last_date, QParam::NullOnEmpty)
           << QParam("id", otInteger, id);
    DB::TCachedQuery Qry(PgOra::getRWSession("BAG_RATES"), sql, params, STDLOG);
    Qry.get().Execute();
    if (Qry.get().RowsProcessed() > 0) {
        ASTRA::syncHistory("bag_rates", new_id, setting_user, station);
    }
    return new_id;
}

void BagRateItem::copy(const std::string& airline, int tid, TDateTime now,
                       const std::string& setting_user, const std::string& station)
{
    int new_id = PgOra::getSeqNextVal_int("ID__SEQ");
    const std::string sql =
            "INSERT INTO bag_rates("
            "id,airline,pr_trfer,city_dep,city_arv,pax_cat,subclass,class,flt_no,craft,trip_type, "
            "first_date,last_date,bag_type,rate,rate_cur,min_weight,extra,pr_del,tid"
            ") "
            "SELECT "
            ":new_id,:airline,pr_trfer,city_dep,city_arv,pax_cat,subclass,class,flt_no,craft,trip_type, "
            "CASE WHEN SIGN(:now - first_date) = 1 THEN :now ELSE first_date END,"
            "last_date,bag_type,rate,rate_cur,min_weight,extra,pr_del,:tid "
            "FROM bag_rates "
            "WHERE airline IS NULL AND pr_del=0 AND (last_date IS NULL OR last_date > :now)";
    QParams params;
    params << QParam("new_id", otInteger, new_id)
           << QParam("airline", otString, airline)
           << QParam("now", otDate, now)
           << QParam("tid", otInteger, tid);
    DB::TCachedQuery Qry(PgOra::getRWSession("BAG_RATES"), sql, params, STDLOG);
    Qry.get().Execute();
    if (Qry.get().RowsProcessed() > 0) {
        ASTRA::syncHistory("bag_rates", new_id, setting_user, station);
    }
}

std::vector<ItemIdDateRange> BagRateItem::loadRangedIdList(TDateTime first, TDateTime last) const
{
    std::vector<ItemIdDateRange> result;
    const std::string sql =
            "SELECT id,first_date,last_date "
            "FROM bag_rates "
            "WHERE (airline IS NULL AND :airline IS NULL OR airline=:airline) AND "
            "      (pr_trfer IS NULL AND :pr_trfer IS NULL OR pr_trfer=:pr_trfer) AND "
            "      (city_dep IS NULL AND :city_dep IS NULL OR city_dep=:city_dep) AND "
            "      (city_arv IS NULL AND :city_arv IS NULL OR city_arv=:city_arv) AND "
            "      (pax_cat IS NULL AND :pax_cat IS NULL OR pax_cat=:pax_cat) AND "
            "      (subclass IS NULL AND :subclass IS NULL OR subclass=:subclass) AND "
            "      (class IS NULL AND :class IS NULL OR class=:class) AND "
            "      (flt_no IS NULL AND :flt_no IS NULL OR flt_no=:flt_no) AND "
            "      (craft IS NULL AND :craft IS NULL OR craft=:craft) AND "
            "      (trip_type IS NULL AND :trip_type IS NULL OR trip_type=:trip_type) AND "
            "      (bag_type IS NULL AND :bag_type IS NULL OR bag_type=:bag_type) AND "
            "      (min_weight IS NULL AND :min_weight IS NULL OR min_weight=:min_weight) AND "
            "      (last_date IS NULL OR last_date>:first) AND "
            "      (:last IS NULL OR :last>first_date) AND "
            "      pr_del=0 "
            "FOR UPDATE ";
    QParams params;
    params << QParam("airline", otString, airline);
    if (pr_trfer) {
        params << QParam("pr_trfer", otInteger, int(*pr_trfer));
    } else {
        params << QParam("pr_trfer", otInteger, FNull);
    }
    params << QParam("city_dep", otString, city_dep)
           << QParam("city_arv", otString, city_arv)
           << QParam("pax_cat", otString, pax_cat)
           << QParam("subclass", otString, subclass)
           << QParam("class", otString, cls)
           << QParam("flt_no", otInteger, flt_no, QParam::NullOnEmpty)
           << QParam("craft", otString, craft)
           << QParam("trip_type", otString, trip_type)
           << QParam("bag_type", otInteger, bag_type, QParam::NullOnEmpty)
           << QParam("min_weight", otInteger, min_weight, QParam::NullOnEmpty)
           << QParam("first", otDate, first)
           << QParam("last", otDate, last, QParam::NullOnEmpty);
    DB::TCachedQuery Qry(PgOra::getRWSession("BAG_RATES"), sql, params, STDLOG);
    Qry.get().Execute();

    for (;!Qry.get().Eof;Qry.get().Next()) {
        result.push_back(
                    ItemIdDateRange {
                        Qry.get().FieldAsInteger("id"),
                        Qry.get().FieldAsDateTime("first_date"),
                        Qry.get().FieldIsNULL("last_date") ? ASTRA::NoExists
                                                           : Qry.get().FieldAsDateTime("last_date")
                    });
    }
    return result;
}

std::optional<BagRateItem> BagRateItem::load(int id)
{
    DB::TQuery Qry(PgOra::getRWSession("BAG_RATES"), STDLOG);
    Qry.SQLText =
            "SELECT id,airline,pr_trfer,city_dep,city_arv,pax_cat,subclass,class,flt_no,craft,trip_type,"
            "       first_date,bag_type,rate,rate_cur,min_weight,extra "
            "FROM bag_rates "
            "WHERE id=:id "
            "AND pr_del=0 "
            "FOR UPDATE";
    Qry.CreateVariable("id", otInteger, id);
    Qry.Execute();
    if (Qry.Eof) {
        return {};
    }
    return fromDB(Qry);
}

std::vector<BagRateItem> BagRateItem::load(const std::string& airline)
{
    std::vector<BagRateItem> result;
    DB::TQuery Qry(PgOra::getRWSession("BAG_RATES"), STDLOG);
    Qry.SQLText =
            "SELECT id,airline,pr_trfer,city_dep,city_arv,pax_cat,subclass,class,flt_no,craft,trip_type,"
            "       first_date,bag_type,rate,rate_cur,min_weight,extra "
            "FROM bag_rates "
            "WHERE airline=:airline "
            "AND pr_del=0 "
            "FOR UPDATE";
    Qry.CreateVariable("airline", otString, airline);
    Qry.Execute();
    for (;!Qry.Eof;Qry.Next()) {
        result.push_back(fromDB(Qry));
    }
    return result;
}

} // namespace

int add(int id, const std::string& airline, std::optional<bool> pr_trfer, const std::string& city_dep,
        const std::string& city_arv, const std::string& pax_cat, const std::string& subclass,
        const std::string& cls, int flt_no, const std::string& craft,
        const std::string& trip_type, TDateTime first_date, TDateTime last_date,
        int bag_type, int rate, const std::string& rate_cur, int min_weight,
        const std::string& extra, int tid, const std::string& setting_user,
        const std::string& station)
{
    const BagRateData data = {
        id, airline, pr_trfer, city_dep, city_arv, pax_cat, subclass, cls, flt_no, craft, trip_type,
        first_date, last_date, bag_type, rate, rate_cur, min_weight, extra
    };
    return BagRateItem(data).add(id, tid, first_date, last_date, setting_user, station);
}

void modifyById(int id, TDateTime last_date, const std::string& setting_user,
                const std::string& station, int tid)
{
    BagRateItem::modifyById(id, last_date, setting_user, station, tid);
}

void deleteById(int id, const std::string& setting_user, const std::string& station, int tid)
{
    BagRateItem::deleteById(id, setting_user, station, tid);
}

void copyBasic(const std::string& airline, const std::string& setting_user,
               const std::string& station)
{
    BagRateItem::copyBasic(airline, setting_user, station);
}

} // namespace BagRate

// VALUE_BAG_TAX

namespace ValueBagTax {
namespace {

struct ValueBagTaxData
{
    int id = ASTRA::NoExists;
    std::string airline;
    std::optional<bool> pr_trfer;
    std::string city_dep;
    std::string city_arv;
    TDateTime first_date;
    TDateTime last_date;
    int tax = ASTRA::NoExists;
    int min_value = ASTRA::NoExists;
    std::string min_value_cur;
    std::string extra;
};

class ValueBagTaxItem: public ValueBagTaxData, public BaseRangedItem
{
public:
    ValueBagTaxItem(const ValueBagTaxData& data): ValueBagTaxData(data) {}
    virtual ~ValueBagTaxItem() {}

protected:
    std::string tableName() const { return "value_bag_taxes"; }
    virtual void checkItem() const;

    static ValueBagTaxItem fromDB(DB::TQuery& Qry);

    int save(int tid, const std::string& setting_user, const std::string& station) const;
    int copy(int id, int tid, TDateTime first_date, TDateTime last_date,
             const std::string& setting_user, const std::string& station) const;
    std::vector<ItemIdDateRange> loadRangedIdList(TDateTime first, TDateTime last) const;
    static std::optional<ValueBagTaxItem> load(int id);
    static std::vector<ValueBagTaxItem> load(const std::string& airline);

public:
    static void copy(const std::string& airline, int tid, TDateTime now,
                     const std::string& setting_user, const std::string& station);
    static void modifyById(int id, TDateTime last_date, const std::string& setting_user,
                           const std::string& station, int tid = ASTRA::NoExists)
    {
        std::optional<ValueBagTaxItem> item = load(id);
        if (item) {
            modifyItemById(*item, id, last_date, setting_user, station, tid);
        }
    }

    static void deleteById(int id, const std::string& setting_user, const std::string& station,
                           int tid = ASTRA::NoExists)
    {
        std::optional<ValueBagTaxItem> item = load(id);
        if (item) {
            deleteItemById(*item, id, setting_user, station, tid);
        }
    }

    static void copyBasic(const std::string& airline, const std::string& setting_user,
                          const std::string& station)
    {
        const std::vector<ValueBagTaxItem> items = ValueBagTaxItem::load(airline);
        copyBasicItems(items, airline, setting_user, station);
    }
};

void ValueBagTaxItem::checkItem() const
{
    if (tax == ASTRA::NoExists) {
        throw AstraLocale::UserException("MSG.PAYMENT.NOT_SET_VALUE_BAG_TAX");
    }
}

ValueBagTaxItem ValueBagTaxItem::fromDB(DB::TQuery& Qry)
{
    return ValueBagTaxItem(
      ValueBagTaxData {
        Qry.FieldAsInteger("id"),
        Qry.FieldAsString("airline"),
        Qry.FieldIsNULL("pr_trfer") ? std::nullopt : std::make_optional(bool(Qry.FieldAsInteger("pr_trfer"))),
        Qry.FieldAsString("city_dep"),
        Qry.FieldAsString("city_arv"),
        Qry.FieldAsDateTime("first_date"),
        Qry.FieldIsNULL("last_date") ? ASTRA::NoExists : Qry.FieldAsDateTime("last_date"),
        Qry.FieldAsInteger("tax"),
        Qry.FieldIsNULL("min_value") ? ASTRA::NoExists : Qry.FieldAsInteger("min_value"),
        Qry.FieldAsString("min_value_cur"),
        Qry.FieldAsString("extra")
    });
}

int ValueBagTaxItem::save(int tid, const std::string& setting_user, const std::string& station) const
{
    const int new_id = PgOra::getSeqNextVal_int("ID__SEQ");
    const std::string sql =
            "INSERT INTO value_bag_taxes("
            "id,airline,pr_trfer,city_dep,city_arv, "
            "first_date,last_date,tax,min_value,min_value_cur,extra,pr_del,tid"
            ") VALUES ("
            ":new_id,:airline,:pr_trfer,:city_dep,:city_arv, "
            ":first_date,:last_date,:tax,:min_value,:min_value_cur,:extra,0,:tid "
            ")";
    QParams params;
    params << QParam("new_id", otInteger, new_id)
           << QParam("airline", otString, airline);
    if (pr_trfer) {
        params << QParam("pr_trfer", otInteger, int(*pr_trfer));
    } else {
        params << QParam("pr_trfer", otInteger, FNull);
    }
    params << QParam("city_dep", otString, city_dep)
           << QParam("city_arv", otString, city_arv)
           << QParam("first_date", otDate, first_date)
           << QParam("last_date", otDate, last_date, QParam::NullOnEmpty)
           << QParam("tax", otInteger, tax)
           << QParam("min_value", otInteger, min_value, QParam::NullOnEmpty)
           << QParam("min_value_cur", otString, min_value_cur)
           << QParam("extra", otString, extra)
           << QParam("tid", otInteger, tid);
    DB::TCachedQuery Qry(PgOra::getRWSession("VALUE_BAG_TAXES"), sql, params, STDLOG);
    Qry.get().Execute();
    if (Qry.get().RowsProcessed() > 0) {
        ASTRA::syncHistory("value_bag_taxes", new_id, setting_user, station);
    }
    return new_id;
}

int ValueBagTaxItem::copy(int id, int tid, TDateTime first_date, TDateTime last_date,
                          const std::string& setting_user, const std::string& station) const
{
    const int new_id = PgOra::getSeqNextVal_int("ID__SEQ");
    const std::string sql =
            "INSERT INTO value_bag_taxes("
            "id,airline,pr_trfer,city_dep,city_arv, "
            "first_date,last_date,tax,min_value,min_value_cur,extra,pr_del,tid "
            ") "
            "SELECT "
            ":new_id,airline,pr_trfer,city_dep,city_arv, "
            ":first_date,:last_date,tax,min_value,min_value_cur,extra,0,:tid "
            "FROM value_bag_taxes "
            "WHERE id=:id";
    QParams params;
    params << QParam("new_id", otInteger, new_id)
           << QParam("tid", otInteger, tid)
           << QParam("first_date", otDate, first_date)
           << QParam("last_date", otDate, last_date, QParam::NullOnEmpty)
           << QParam("id", otInteger, id);
    DB::TCachedQuery Qry(PgOra::getRWSession("VALUE_BAG_TAXES"), sql, params, STDLOG);
    Qry.get().Execute();
    if (Qry.get().RowsProcessed() > 0) {
        ASTRA::syncHistory("value_bag_taxes", new_id, setting_user, station);
    }
    return new_id;
}

void ValueBagTaxItem::copy(const std::string& airline, int tid, TDateTime now,
                           const std::string& setting_user, const std::string& station)
{
    int new_id = PgOra::getSeqNextVal_int("ID__SEQ");
    const std::string sql =
            "INSERT INTO value_bag_taxes("
            "id,airline,pr_trfer,city_dep,city_arv, "
            "first_date,last_date,tax,min_value,min_value_cur,extra,pr_del,tid"
            ") "
            "SELECT "
            ":new_id,:airline,pr_trfer,city_dep,city_arv, "
            "CASE WHEN SIGN(:now - first_date) = 1 THEN :now ELSE first_date END,"
            "last_date,tax,min_value,min_value_cur,extra,pr_del,:tid "
            "FROM value_bag_taxes "
            "WHERE airline IS NULL AND pr_del=0 AND (last_date IS NULL OR last_date > :now)";
    QParams params;
    params << QParam("new_id", otInteger, new_id)
           << QParam("airline", otString, airline)
           << QParam("now", otDate, now)
           << QParam("tid", otInteger, tid);
    DB::TCachedQuery Qry(PgOra::getRWSession("VALUE_BAG_TAXES"), sql, params, STDLOG);
    Qry.get().Execute();
    if (Qry.get().RowsProcessed() > 0) {
        ASTRA::syncHistory("value_bag_taxes", new_id, setting_user, station);
    }
}

std::vector<ItemIdDateRange> ValueBagTaxItem::loadRangedIdList(TDateTime first, TDateTime last) const
{
    std::vector<ItemIdDateRange> result;
    const std::string sql =
            "SELECT id,first_date,last_date "
            "FROM value_bag_taxes "
            "WHERE (airline IS NULL AND :airline IS NULL OR airline=:airline) AND "
            "      (pr_trfer IS NULL AND :pr_trfer IS NULL OR pr_trfer=:pr_trfer) AND "
            "      (city_dep IS NULL AND :city_dep IS NULL OR city_dep=:city_dep) AND "
            "      (city_arv IS NULL AND :city_arv IS NULL OR city_arv=:city_arv) AND "
            "      (min_value IS NULL AND min_value_cur IS NULL AND "
            "       :min_value IS NULL AND :min_value_cur IS NULL OR "
            "       min_value=:min_value AND min_value_cur=:min_value_cur) AND "
            "      (last_date IS NULL OR last_date>:first) AND "
            "      (:last IS NULL OR :last>first_date) AND "
            "      pr_del=0 "
            "FOR UPDATE ";
    QParams params;
    params << QParam("airline", otString, airline);
    if (pr_trfer) {
        params << QParam("pr_trfer", otInteger, int(*pr_trfer));
    } else {
        params << QParam("pr_trfer", otInteger, FNull);
    }
    params << QParam("city_dep", otString, city_dep)
           << QParam("city_arv", otString, city_arv)
           << QParam("min_value", otInteger, min_value, QParam::NullOnEmpty)
           << QParam("min_value_cur", otString, min_value_cur)
           << QParam("first", otDate, first)
           << QParam("last", otInteger, min_value, QParam::NullOnEmpty);
    DB::TCachedQuery Qry(PgOra::getRWSession("VALUE_BAG_TAXES"), sql, params, STDLOG);
    Qry.get().Execute();

    for (;!Qry.get().Eof;Qry.get().Next()) {
        result.push_back(
                    ItemIdDateRange {
                        Qry.get().FieldAsInteger("id"),
                        Qry.get().FieldAsDateTime("first_date"),
                        Qry.get().FieldIsNULL("last_date") ? ASTRA::NoExists
                                                           : Qry.get().FieldAsDateTime("last_date")
                    });
    }
    return result;
}

std::optional<ValueBagTaxItem> ValueBagTaxItem::load(int id)
{
    DB::TQuery Qry(PgOra::getRWSession("VALUE_BAG_TAXES"), STDLOG);
    Qry.SQLText =
            "SELECT id,airline,pr_trfer,city_dep,city_arv, "
            "       first_date,tax,min_value,min_value_cur,extra "
            "FROM value_bag_taxes "
            "WHERE id=:id AND pr_del=0 "
            "FOR UPDATE ";
    Qry.CreateVariable("id", otInteger, id);
    Qry.Execute();
    if (Qry.Eof) {
        return {};
    }
    return fromDB(Qry);
}

std::vector<ValueBagTaxItem> ValueBagTaxItem::load(const std::string& airline)
{
    std::vector<ValueBagTaxItem> result;
    DB::TQuery Qry(PgOra::getRWSession("VALUE_BAG_TAXES"), STDLOG);
    Qry.SQLText =
            "SELECT id,airline,pr_trfer,city_dep,city_arv, "
            "       first_date,tax,min_value,min_value_cur,extra "
            "FROM value_bag_taxes "
            "WHERE airline=:airline AND pr_del=0 "
            "FOR UPDATE ";
    Qry.CreateVariable("airline", otString, airline);
    Qry.Execute();
    for (;!Qry.Eof;Qry.Next()) {
        result.push_back(fromDB(Qry));
    }
    return result;
}

} // namespace

int add(int id, const std::string& airline, std::optional<bool> pr_trfer, const std::string& city_dep,
        const std::string& city_arv, TDateTime first_date, TDateTime last_date,
        int tax, int min_value, const std::string& min_value_cur, const std::string& extra,
        int tid, const std::string& setting_user, const std::string& station)
{
    const ValueBagTaxData data = {
        id, airline, pr_trfer, city_dep, city_arv,
        first_date, last_date, tax, min_value, min_value_cur, extra
    };
    return ValueBagTaxItem(data).add(id, tid, first_date, last_date, setting_user, station);

}

void modifyById(int id, TDateTime last_date, const std::string& setting_user,
                const std::string& station, int tid)
{
    ValueBagTaxItem::modifyById(id, last_date, setting_user, station, tid);
}

void deleteById(int id, const std::string& setting_user, const std::string& station, int tid)
{
    ValueBagTaxItem::deleteById(id, setting_user, station, tid);
}

void copyBasic(const std::string& airline, const std::string& setting_user,
               const std::string& station)
{
    ValueBagTaxItem::copyBasic(airline, setting_user, station);
}

} // namespace ValueBagTax

// EXCHANGE_RATES

namespace ExchangeRate {
namespace {

struct ExchangeRateData
{
    int id = ASTRA::NoExists;
    std::string airline;
    int rate1 = ASTRA::NoExists;
    std::string cur1;
    int rate2 = ASTRA::NoExists;
    std::string cur2;
    TDateTime first_date;
    TDateTime last_date;
    std::string extra;
};

class ExchangeRateItem: public ExchangeRateData, public BaseRangedItem
{
public:
    ExchangeRateItem(const ExchangeRateData& data): ExchangeRateData(data) {}
    virtual ~ExchangeRateItem() {}

protected:
    std::string tableName() const { return "exchange_rates"; }
    virtual void checkItem() const;

    static ExchangeRateItem fromDB(DB::TQuery& Qry);

    int save(int tid, const std::string& setting_user, const std::string& station) const;
    int copy(int id, int tid, TDateTime first_date, TDateTime last_date,
             const std::string& setting_user, const std::string& station) const;
    std::vector<ItemIdDateRange> loadRangedIdList(TDateTime first, TDateTime last) const;
    static std::optional<ExchangeRateItem> load(int id);
    static std::vector<ExchangeRateItem> load(const std::string& airline);

public:
    static void copy(const std::string& airline, int tid, TDateTime now,
                     const std::string& setting_user, const std::string& station);
    static void modifyById(int id, TDateTime last_date, const std::string& setting_user,
                           const std::string& station, int tid = ASTRA::NoExists)
    {
        std::optional<ExchangeRateItem> item = load(id);
        if (item) {
            modifyItemById(*item, id, last_date, setting_user, station, tid);
        }
    }

    static void deleteById(int id, const std::string& setting_user, const std::string& station,
                           int tid = ASTRA::NoExists)
    {
        std::optional<ExchangeRateItem> item = load(id);
        if (item) {
            deleteItemById(*item, id, setting_user, station, tid);
        }
    }

    static void copyBasic(const std::string& airline, const std::string& setting_user,
                          const std::string& station)
    {
        const std::vector<ExchangeRateItem> items = ExchangeRateItem::load(airline);
        copyBasicItems(items, airline, setting_user, station);
    }
};

void ExchangeRateItem::checkItem() const
{
    if (rate1 == ASTRA::NoExists) {
        throw AstraLocale::UserException("MSG.PAYMENT.NOT_SET_FIRST_CUR_VALUE");
    }
    if (cur1.empty()) {
        throw AstraLocale::UserException("MSG.PAYMENT.NOT_SET_FIRST_CUR_CODE");
    }
    if (rate2 == ASTRA::NoExists) {
        throw AstraLocale::UserException("MSG.PAYMENT.NOT_SET_SECOND_CUR_VALUE");
    }
    if (cur2.empty()) {
        throw AstraLocale::UserException("MSG.PAYMENT.NOT_SET_SECOND_CUR_CODE");
    }
}

ExchangeRateItem ExchangeRateItem::fromDB(DB::TQuery& Qry)
{
    return ExchangeRateItem(
      ExchangeRateData {
        Qry.FieldAsInteger("id"),
        Qry.FieldAsString("airline"),
        Qry.FieldAsInteger("rate1"),
        Qry.FieldAsString("cur1"),
        Qry.FieldAsInteger("rate2"),
        Qry.FieldAsString("cur2"),
        Qry.FieldAsDateTime("first_date"),
        Qry.FieldIsNULL("last_date") ? ASTRA::NoExists : Qry.FieldAsDateTime("last_date"),
        Qry.FieldAsString("extra")
    });
}

int ExchangeRateItem::save(int tid, const std::string& setting_user, const std::string& station) const
{
    const int new_id = PgOra::getSeqNextVal_int("ID__SEQ");
    const std::string sql =
            "INSERT INTO exchange_rates("
            "id,airline,rate1,cur1,rate2,cur2, "
            "first_date,last_date,extra,pr_del,tid"
            ") VALUES ("
            ":new_id,:airline,:rate1,:cur1,:rate2,:cur2, "
            ":first_date,:last_date,:extra,0,:tid "
            ")";
    QParams params;
    params << QParam("new_id", otInteger, new_id)
           << QParam("airline", otString, airline)
           << QParam("rate1", otInteger, rate1)
           << QParam("cur1", otString, cur1)
           << QParam("rate2", otInteger, rate2)
           << QParam("cur2", otString, cur2)
           << QParam("first_date", otDate, first_date)
           << QParam("last_date", otDate, last_date, QParam::NullOnEmpty)
           << QParam("extra", otString, extra)
           << QParam("tid", otInteger, tid);
    DB::TCachedQuery Qry(PgOra::getRWSession("EXCHANGE_RATES"), sql, params, STDLOG);
    Qry.get().Execute();
    if (Qry.get().RowsProcessed() > 0) {
        ASTRA::syncHistory("exchange_rates", new_id, setting_user, station);
    }
    return new_id;
}

int ExchangeRateItem::copy(int id, int tid, TDateTime first_date, TDateTime last_date,
                           const std::string& setting_user, const std::string& station) const
{
    const int new_id = PgOra::getSeqNextVal_int("ID__SEQ");
    const std::string sql =
            "INSERT INTO exchange_rates("
            "id,airline,rate1,cur1,rate2,cur2, "
            "first_date,last_date,extra,pr_del,tid "
            ") "
            "SELECT "
            ":new_id,airline,rate1,cur1,rate2,cur2, "
            ":first_date,:last_date,extra,0,:tid "
            "FROM exchange_rates "
            "WHERE id=:id";
    QParams params;
    params << QParam("new_id", otInteger, new_id)
           << QParam("tid", otInteger, tid)
           << QParam("first_date", otDate, first_date)
           << QParam("last_date", otDate, last_date, QParam::NullOnEmpty)
           << QParam("id", otInteger, id);
    DB::TCachedQuery Qry(PgOra::getRWSession("EXCHANGE_RATES"), sql, params, STDLOG);
    Qry.get().Execute();
    if (Qry.get().RowsProcessed() > 0) {
        ASTRA::syncHistory("exchange_rates", new_id, setting_user, station);
    }
    return new_id;
}

void ExchangeRateItem::copy(const std::string& airline, int tid, TDateTime now,
                            const std::string& setting_user, const std::string& station)
{
    int new_id = PgOra::getSeqNextVal_int("ID__SEQ");
    const std::string sql =
            "INSERT INTO exchange_rates("
            "id,airline,rate1,cur1,rate2,cur2, "
            "first_date,last_date,extra,pr_del,tid"
            ") "
            "SELECT "
            ":new_id,:airline,rate1,cur1,rate2,cur2, "
            "CASE WHEN SIGN(:now - first_date) = 1 THEN :now ELSE first_date END,"
            "last_date,extra,pr_del,:tid "
            "FROM exchange_rates "
            "WHERE airline IS NULL AND pr_del=0 AND (last_date IS NULL OR last_date > :now)";
    QParams params;
    params << QParam("new_id", otInteger, new_id)
           << QParam("airline", otString, airline)
           << QParam("now", otDate, now)
           << QParam("tid", otInteger, tid);
    DB::TCachedQuery Qry(PgOra::getRWSession("EXCHANGE_RATES"), sql, params, STDLOG);
    Qry.get().Execute();
    if (Qry.get().RowsProcessed() > 0) {
        ASTRA::syncHistory("exchange_rates", new_id, setting_user, station);
    }
}

std::vector<ItemIdDateRange> ExchangeRateItem::loadRangedIdList(TDateTime first, TDateTime last) const
{
    std::vector<ItemIdDateRange> result;
    const std::string sql =
            "SELECT id,first_date,last_date "
            "FROM exchange_rates "
            "WHERE (airline IS NULL AND :airline IS NULL OR airline=:airline) AND "
            "      (cur1=:cur1) AND "
            "      (cur2=:cur2) AND "
            "      (last_date IS NULL OR last_date>:first) AND "
            "      (:last IS NULL OR :last>first_date) AND "
            "      pr_del=0 "
            "FOR UPDATE ";
    QParams params;
    params << QParam("airline", otString, airline)
           << QParam("cur1", otString, cur1)
           << QParam("cur2", otString, cur2)
           << QParam("first", otDate, first)
           << QParam("last", otDate, last, QParam::NullOnEmpty);
    DB::TCachedQuery Qry(PgOra::getRWSession("EXCHANGE_RATES"), sql, params, STDLOG);
    Qry.get().Execute();

    for (;!Qry.get().Eof;Qry.get().Next()) {
        result.push_back(
                    ItemIdDateRange {
                        Qry.get().FieldAsInteger("id"),
                        Qry.get().FieldAsDateTime("first_date"),
                        Qry.get().FieldIsNULL("last_date") ? ASTRA::NoExists
                                                           : Qry.get().FieldAsDateTime("last_date")
                    });
    }
    return result;
}

std::optional<ExchangeRateItem> ExchangeRateItem::load(int id)
{
    DB::TQuery Qry(PgOra::getRWSession("EXCHANGE_RATES"), STDLOG);
    Qry.SQLText =
            "SELECT id,airline,rate1,cur1,rate2,cur2, "
            "       first_date,last_date,extra "
            "FROM exchange_rates "
            "WHERE id=:id AND pr_del=0 "
            "FOR UPDATE ";
    Qry.CreateVariable("id", otInteger, id);
    Qry.Execute();
    if (Qry.Eof) {
        return {};
    }
    return fromDB(Qry);
}

std::vector<ExchangeRateItem> ExchangeRateItem::load(const std::string& airline)
{
    std::vector<ExchangeRateItem> result;
    DB::TQuery Qry(PgOra::getRWSession("EXCHANGE_RATES"), STDLOG);
    Qry.SQLText =
            "SELECT id,airline,rate1,cur1,rate2,cur2, "
            "       first_date,last_date,extra "
            "FROM exchange_rates "
            "WHERE airline=:airline AND pr_del=0 "
            "FOR UPDATE ";
    Qry.CreateVariable("airline", otString, airline);
    Qry.Execute();
    for (;!Qry.Eof;Qry.Next()) {
        result.push_back(fromDB(Qry));
    }
    return result;
}

} // namespace

int add(int id, const std::string& airline, int rate1, const std::string& cur1,
        int rate2, const std::string& cur2, TDateTime first_date, TDateTime last_date,
        const std::string& extra, int tid, const std::string& setting_user,
        const std::string& station)
{
    const ExchangeRateData data = {
        id, airline, rate1,cur1,rate2,cur2,
        first_date, last_date, extra
    };
    return ExchangeRateItem(data).add(id, tid, first_date, last_date, setting_user, station);
}

void modifyById(int id, TDateTime last_date, const std::string& setting_user,
                const std::string& station, int tid)
{
    ExchangeRateItem::modifyById(id, last_date, setting_user, station, tid);
}

void deleteById(int id, const std::string& setting_user, const std::string& station, int tid)
{
    ExchangeRateItem::deleteById(id, setting_user, station, tid);
}

void copyBasic(const std::string& airline, const std::string& setting_user,
               const std::string& station)
{
    ExchangeRateItem::copyBasic(airline, setting_user, station);
}

} // namespace ExchangeRate

} // namespace Kassa
