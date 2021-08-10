#include "typeb_addrs.h"
#include "PgOraConfig.h"
#include <serverlib/dbcpp_cursctl.h>

#define NICKNAME "EFREMOV"
#include <serverlib/slogger.h>

namespace TypeB {
namespace {

const std::string sqlHead =
    "SELECT :typebAddrsId AS typeb_addrs_id, src.tlg_type, src.category, dest.id, ";

const std::string sqlTail =
    "FROM (SELECT * FROM typeb_addr_options WHERE typeb_addrs_id=:typebAddrsId) dest "
    "FULL OUTER JOIN (SELECT * FROM typeb_options WHERE tlg_type=:basicType) src "
    "ON (dest.tlg_type=src.tlg_type AND dest.category=src.category)";

void lockTypebAddrs(const RowId_t &typebAddrsId)
{
    make_db_curs("UPDATE typeb_addrs SET id=id WHERE id=:typebAddrsId",
                 PgOra::getRWSession("TYPEB_ADDRS"))
        .bind(":typebAddrsId", typebAddrsId.get())
        .exec();
}

void syncTypebOptions(DbCpp::CursCtl &cur)
{
    static const int idNull = 0;
    static const std::string categoryNull = "";
    static const std::string valueNull = "";

    int typebAddrsId = 0;
    std::string tlgType;
    std::string category;
    int id = 0;
    std::string value;

    cur.def(typebAddrsId)
       .def(tlgType)
       .defNull(category, categoryNull)
       .defNull(id, idNull)
       .defNull(value, valueNull)
       .exec();

    while (cur.fen() == DbCpp::ResultCode::Ok) {
        if (id != idNull) {
            if (category == categoryNull) {
                make_db_curs("DELETE FROM typeb_addr_options "
                             "WHERE id=:id",
                             PgOra::getRWSession("TYPEB_ADDR_OPTIONS"))
                    .bind(":id", id)
                    .exec();
            } else {
                make_db_curs("UPDATE typeb_addr_options "
                             "SET value=:value "
                             "WHERE id=:id AND COALESCE(value, ' ') <> COALESCE(NULLIF(:value, ''), ' ')",
                             PgOra::getRWSession("TYPEB_ADDR_OPTIONS"))
                    .bind(":id", id)
                    .bind(":value", value)
                    .exec();
            }
        } else {
            id = PgOra::getSeqNextVal_int("ID__SEQ");

            make_db_curs("INSERT INTO typeb_addr_options(typeb_addrs_id, tlg_type, category, value, id) "
                         "VALUES(:typebAddrsId, :tlgType, :category, :value, :id)",
                         PgOra::getRWSession("TYPEB_ADDR_OPTIONS"))
                .bind(":typebAddrsId", typebAddrsId)
                .bind(":tlgType", tlgType)
                .bind(":category", category)
                .bind(":value", value)
                .bind(":id", id)
                .exec();
        }

        HistoryTable("typeb_addr_options").synchronize(RowId_t(id));
    }
}

} // namespace

void deleteCreatePoints(const RowId_t &typebAddrsId)
{
    auto cur = make_db_curs("SELECT id FROM typeb_create_points WHERE typeb_addrs_id=:typebAddrsId FOR UPDATE",
                            PgOra::getRWSession("TYPEB_CREATE_POINTS"));

    int id = 0;
    cur.stb()
       .def(id)
       .bind(":typebAddrsId", typebAddrsId.get())
       .exec();

    while (cur.fen() == DbCpp::ResultCode::Ok) {
        auto del = make_db_curs("DELETE FROM typeb_create_points WHERE id=:id",
                                PgOra::getRWSession("TYPEB_CREATE_POINTS"));
        del.bind(":id", id)
           .exec();

        HistoryTable("typeb_create_points").synchronize(RowId_t(id));
    }
}

void deleteTypebOptions(const RowId_t &typebAddrsId)
{
    auto cur = make_db_curs("SELECT id FROM typeb_addr_options WHERE typeb_addrs_id=:typebAddrsId FOR UPDATE",
                            PgOra::getRWSession("TYPEB_ADDR_OPTIONS"));

    int id = 0;
    cur.bind(":typebAddrsId", typebAddrsId.get())
       .def(id)
       .exec();

    while (cur.fen() == DbCpp::ResultCode::Ok) {
        auto del = make_db_curs("DELETE FROM typeb_addr_options WHERE id=:id",
                                PgOra::getRWSession("TYPEB_ADDR_OPTIONS"));
        del.bind(":id", id)
           .exec();

        HistoryTable("typeb_addr_options").synchronize(RowId_t(id));
    }
}

void syncTypebOptions(const RowId_t &typebAddrsId, const std::string &basicType)
{
    lockTypebAddrs(typebAddrsId);

    auto sql = sqlHead +
               "(CASE "
                   "WHEN dest.id IS NULL THEN default_value "
                   "ELSE dest.value "
               "END) AS value "
               + sqlTail;

    auto cur = make_db_curs(sql, PgOra::getROSession({"TYPEB_ADDR_OPTIONS", "TYPEB_OPTIONS"}));
    cur.bind(":typebAddrsId", typebAddrsId.get())
       .bind(":basicType", basicType);

    syncTypebOptions(cur);
}

void syncCOMOptions(const RowId_t &typebAddrsId, const std::string &basicType,
                    const std::string &version)
{
    lockTypebAddrs(typebAddrsId);

    auto sql = sqlHead +
               "(CASE "
                   "WHEN src.category='VERSION' THEN :version "
                   "ELSE default_value "
               "END) AS value "
               + sqlTail;

    auto cur = make_db_curs(sql, PgOra::getROSession({"TYPEB_ADDR_OPTIONS", "TYPEB_OPTIONS"}));
    cur.bind(":typebAddrsId", typebAddrsId.get())
       .bind(":basicType", basicType)
       .bind(":version", version);

    syncTypebOptions(cur);
}

void syncETLOptions(const RowId_t &typebAddrsId, const std::string &basicType,
                    const std::string &rbd)
{
    lockTypebAddrs(typebAddrsId);

    auto sql = sqlHead +
               "(CASE "
                   "WHEN src.category='RBD' THEN :rbd "
                   "ELSE default_value "
               "END) AS value "
               + sqlTail;

    auto cur = make_db_curs(sql, PgOra::getROSession({"TYPEB_ADDR_OPTIONS", "TYPEB_OPTIONS"}));
    cur.bind(":typebAddrsId", typebAddrsId.get())
       .bind(":basicType", basicType)
       .bind(":rbd", rbd);

    syncTypebOptions(cur);
}

void syncMVTOptions(const RowId_t &typebAddrsId, const std::string &basicType,
                    const std::string &version, const std::string noend)
{
    lockTypebAddrs(typebAddrsId);

    auto sql = sqlHead +
               "(CASE "
                   "WHEN src.category='VERSION' THEN :version "
                   "WHEN src.category='NOEND'   THEN :noend "
                   "ELSE default_value "
               "END) AS value "
               + sqlTail;

    auto cur = make_db_curs(sql, PgOra::getROSession({"TYPEB_ADDR_OPTIONS", "TYPEB_OPTIONS"}));
    cur.bind(":typebAddrsId", typebAddrsId.get())
       .bind(":basicType", basicType)
       .bind(":version", version)
       .bind(":noend", noend);

    syncTypebOptions(cur);
}

void syncLDMOptions(const RowId_t &typebAddrsId, const std::string &basicType,
                    const std::string &version, const std::string cfg,
                    const std::string cabinBaggage, const std::string gender,
                    const std::string exb, const std::string noend)
{
    lockTypebAddrs(typebAddrsId);

    auto sql = sqlHead +
               "(CASE "
                   "WHEN src.category='VERSION'       THEN :version "
                   "WHEN src.category='CFG'           THEN :cfg "
                   "WHEN src.category='CABIN_BAGGAGE' THEN :cabinBaggage "
                   "WHEN src.category='GENDER'        THEN :gender "
                   "WHEN src.category='EXB'           THEN :exb "
                   "WHEN src.category='NOEND'         THEN :noend "
                   "ELSE default_value"
               "END) AS value "
               + sqlTail;

    auto cur = make_db_curs(sql, PgOra::getROSession({"TYPEB_ADDR_OPTIONS", "TYPEB_OPTIONS"}));
    cur.bind(":typebAddrsId", typebAddrsId.get())
       .bind(":basicType", basicType)
       .bind(":version", version)
       .bind(":cfg", cfg)
       .bind(":cabinBaggage", cabinBaggage)
       .bind(":gender", gender)
       .bind(":exb", exb)
       .bind(":noend", noend);

    syncTypebOptions(cur);
}

void syncLCIOptions(const RowId_t &typebAddrsId, const std::string &basicType,
                    const std::string &version, const std::string equipment,
                    const std::string weightAvail, const std::string seating,
                    const std::string weightMode, const std::string seatRestrict,
                    const std::string pasTotals, const std::string bagTotals,
                    const std::string pasDistrib, const std::string seatPlan)
{
    lockTypebAddrs(typebAddrsId);

    auto sql = sqlHead +
               "(CASE "
                   "WHEN src.category='VERSION'       THEN :version "
                   "WHEN src.category='EQUIPMENT'     THEN :equipment "
                   "WHEN src.category='WEIGHT_AVAIL'  THEN :weightAvail "
                   "WHEN src.category='SEATING'       THEN :seating "
                   "WHEN src.category='WEIGHT_MODE'   THEN :weightMode "
                   "WHEN src.category='SEAT_RESTRICT' THEN :seatRestrict "
                   "WHEN src.category='PAS_TOTALS'    THEN :pasTotals "
                   "WHEN src.category='BAG_TOTALS'    THEN :bagTotals "
                   "WHEN src.category='PAS_DISTRIB'   THEN :pasDistrib "
                   "WHEN src.category='SEAT_PLAN'     THEN :seatPlan "
                   "ELSE default_value"
               "END) AS value "
               + sqlTail;

    auto cur = make_db_curs(sql, PgOra::getROSession({"TYPEB_ADDR_OPTIONS", "TYPEB_OPTIONS"}));
    cur.bind(":typebAddrsId", typebAddrsId.get())
       .bind(":basicType", basicType)
       .bind(":version", version)
       .bind(":equipment", equipment)
       .bind(":weightAvail", weightAvail)
       .bind(":seating", seating)
       .bind(":weightMode", weightMode)
       .bind(":seatRestrict", seatRestrict)
       .bind(":pasTotals", pasTotals)
       .bind(":bagTotals", bagTotals)
       .bind(":pasDistrib", pasDistrib)
       .bind(":seatPlan", seatPlan);

    syncTypebOptions(cur);
}

void syncPRLOptions(const RowId_t &typebAddrsId, const std::string &basicType,
                    const std::string &version, const std::string createPoint,
                    const std::string paxState, const std::string rbd,
                    const std::string xbag)
{
    lockTypebAddrs(typebAddrsId);

    auto sql = sqlHead +
               "(CASE "
                   "WHEN src.category='VERSION'      THEN :version "
                   "WHEN src.category='CREATE_POINT' THEN :createPoint "
                   "WHEN src.category='PAX_STATE'    THEN :paxState "
                   "WHEN src.category='RBD'          THEN :rbd "
                   "WHEN src.category='XBAG'         THEN :xbag "
                   "ELSE default_value"
               "END) AS value "
               + sqlTail;

    auto cur = make_db_curs(sql, PgOra::getROSession({"TYPEB_ADDR_OPTIONS", "TYPEB_OPTIONS"}));
    cur.bind(":typebAddrsId", typebAddrsId.get())
       .bind(":basicType", basicType)
       .bind(":version", version)
       .bind(":createPoint", createPoint)
       .bind(":paxState", paxState)
       .bind(":rbd", rbd)
       .bind(":xbag", xbag);

    syncTypebOptions(cur);
}

void syncBSMOptions(const RowId_t &typebAddrsId, const std::string &basicType,
                    const std::string classOfTravel, const std::string tagPrinterId,
                    const std::string pasNameRp1745, const std::string actualDepDate,
                    const std::string brd, const std::string trferIn,
                    const std::string longFltNo)
{
    lockTypebAddrs(typebAddrsId);

    auto sql = sqlHead +
               "(CASE "
                   "WHEN src.category='CLASS_OF_TRAVEL' THEN :classOfTravel "
                   "WHEN src.category='TAG_PRINTER_ID'  THEN :tagPrinterId "
                   "WHEN src.category='PAS_NAME_RP1745' THEN :pasNameRp1745 "
                   "WHEN src.category='ACTUAL_DEP_DATE' THEN :actualDepDate "
                   "WHEN src.category='BRD'             THEN :brd "
                   "WHEN src.category='TRFER_IN'        THEN :trferIn "
                   "WHEN src.category='LONG_FLT_NO'     THEN :longFltNo "
                   "ELSE default_value"
               "END) AS value "
               + sqlTail;

    auto cur = make_db_curs(sql, PgOra::getROSession({"TYPEB_ADDR_OPTIONS", "TYPEB_OPTIONS"}));
    cur.bind(":typebAddrsId", typebAddrsId.get())
       .bind(":basicType", basicType)
       .bind(":classOfTravel", classOfTravel)
       .bind(":tagPrinterId", tagPrinterId)
       .bind(":pasNameRp1745", pasNameRp1745)
       .bind(":actualDepDate", actualDepDate)
       .bind(":brd", brd)
       .bind(":trferIn", trferIn)
       .bind(":longFltNo", longFltNo);

    syncTypebOptions(cur);
}

void syncPNLOptions(const RowId_t &typebAddrsId, const std::string &basicType,
                    const std::string &forwarding)
{
    lockTypebAddrs(typebAddrsId);

    auto sql = sqlHead +
               "(CASE "
                   "WHEN src.category='FORWARDING' THEN :forwarding "
                   "ELSE default_value "
               "END) AS value "
               + sqlTail;

    auto cur = make_db_curs(sql, PgOra::getROSession({"TYPEB_ADDR_OPTIONS", "TYPEB_OPTIONS"}));
    cur.bind(":typebAddrsId", typebAddrsId.get())
       .bind(":basicType", basicType)
       .bind(":forwarding", forwarding);

    syncTypebOptions(cur);
}

} // namespace TypeB
