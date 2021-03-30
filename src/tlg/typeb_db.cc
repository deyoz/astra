#include <set>
#include "typeb_db.h"

#define STDLOG NICKNAME,__FILE__,__LINE__
#define NICKNAME "VLAD"
#include "serverlib/slogger.h"
#include "astra_consts.h"
#include "db_tquery.h"
#include <serverlib/dbcpp_cursctl.h>
#include "PgOraConfig.h"

namespace TypeB
{

std::set<PaxId_t> loadPaxIdSet(const PointIdTlg_t& point_id, const std::string& system,
                               const std::optional<CrsSender_t>& sender)
{
  LogTrace(TRACE6) << __func__
                   << ": point_id=" << point_id
                   << ": system=" << system
                   << ": sender=" << (sender ? sender->get() : "<undefined>");
  std::set<PaxId_t> result;
  int pax_id = ASTRA::NoExists;
  auto cur = make_db_curs(
        "SELECT pax_id "
        "FROM crs_pnr, crs_pax "
        "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
        "      crs_pnr.point_id=:point_id AND "
        "      (:system IS NULL OR system=:system) AND "
        "      (:sender IS NULL OR sender=:sender) ",
        PgOra::getROSession("CRS_PAX"));

  cur.stb()
      .def(pax_id)
      .bind(":point_id", point_id.get())
      .bind(":system", system)
      .bind(":sender", sender ? sender->get() : "")
      .exec();

  while (!cur.fen()) {
    result.emplace(pax_id);
  }
  LogTrace(TRACE6) << __func__
                   << ": count=" << result.size();
  return result;
}

std::set<PnrId_t> loadPnrIdSet(const PointIdTlg_t& point_id, const std::string& system,
                               const std::optional<CrsSender_t>& sender)
{
  LogTrace(TRACE6) << __func__
                   << ": point_id=" << point_id
                   << ": system=" << system
                   << ": sender=" << (sender ? sender->get() : "<undefined>");
  std::set<PnrId_t> result;
  int pnr_id = ASTRA::NoExists;
  auto cur = make_db_curs(
        "SELECT pnr_id "
        "FROM crs_pnr "
        "WHERE point_id=:point_id AND "
        "      (:system IS NULL OR system=:system) AND "
        "      (:sender IS NULL OR sender=:sender) ",
        PgOra::getROSession("CRS_PNR"));

  cur.stb()
      .def(pnr_id)
      .bind(":point_id", point_id.get())
      .bind(":system", system)
      .bind(":sender", sender ? sender->get() : "")
      .exec();

  while (!cur.fen()) {
    result.emplace(pnr_id);
  }
  LogTrace(TRACE6) << __func__
                   << ": count=" << result.size();
  return result;
}

namespace {

bool deleteByPaxId(const std::string& table_name, const PaxId_t& pax_id)
{
  LogTrace(TRACE6) << __func__
                   << ": table_name=" << table_name
                   << ", pax_id=" << pax_id;
  auto cur = make_db_curs(
        "DELETE FROM " + table_name + " "
        "WHERE pax_id=:pax_id ",
        PgOra::getRWSession(table_name));
  cur.stb()
      .bind(":pax_id", pax_id.get())
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool deleteByPnrId(const std::string& table_name, const PnrId_t& pnr_id)
{
  LogTrace(TRACE6) << __func__
                   << ": table_name=" << table_name
                   << ", pnr_id=" << pnr_id;
  auto cur = make_db_curs(
        "DELETE FROM " + table_name + " "
        "WHERE pnr_id=:pnr_id ",
        PgOra::getRWSession(table_name));
  cur.stb()
      .bind(":pnr_id", pnr_id.get())
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool deleteByPointId(const std::string& table_name, const PointIdTlg_t& point_id,
                     const std::string& system, const std::optional<CrsSender_t>& sender)
{
  LogTrace(TRACE6) << __func__
                   << ": table_name=" << table_name
                   << ", point_id=" << point_id
                   << ", system=" << system
                   << ", sender=" << (sender ? sender->get() : "<undefined>");
  auto cur = make_db_curs(
        "DELETE FROM " + table_name + " "
        "WHERE point_id=:point_id AND "
        "      (:system IS NULL OR system=:system) AND "
        "      (:sender IS NULL OR sender=:sender) ",
        PgOra::getRWSession(table_name));
  cur.stb()
      .bind(":point_id", point_id.get())
      .bind(":system", system)
      .bind(":sender", sender ? sender->get() : "")
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

} //namespace

bool deleteCrsSeatsBlocking(const PaxId_t& pax_id)
{
  return deleteByPaxId("CRS_SEATS_BLOCKING", pax_id);
}

bool deleteCrsInf(const PaxId_t& pax_id)
{
  return deleteByPaxId("CRS_INF", pax_id);
}

bool deleteCrsInfDeleted(const PaxId_t& pax_id)
{
  return deleteByPaxId("CRS_INF_DELETED", pax_id);
}

bool deleteCrsPaxRem(const PaxId_t& pax_id)
{
  return deleteByPaxId("CRS_PAX_REM", pax_id);
}

bool deleteCrsPaxDoc(const PaxId_t& pax_id)
{
  return deleteByPaxId("CRS_PAX_DOC", pax_id);
}

bool deleteCrsPaxDoco(const PaxId_t& pax_id)
{
  return deleteByPaxId("CRS_PAX_DOCO", pax_id);
}

bool deleteCrsPaxDoca(const PaxId_t& pax_id)
{
  return deleteByPaxId("CRS_PAX_DOCA", pax_id);
}

bool deleteCrsPaxTkn(const PaxId_t& pax_id)
{
  return deleteByPaxId("CRS_PAX_TKN", pax_id);
}

bool deleteCrsPaxFqt(const PaxId_t& pax_id)
{
  return deleteByPaxId("CRS_PAX_FQT", pax_id);
}

bool deleteCrsPaxChkd(const PaxId_t& pax_id)
{
  return deleteByPaxId("CRS_PAX_CHKD", pax_id);
}

bool deleteCrsPaxAsvc(const PaxId_t& pax_id)
{
  return deleteByPaxId("CRS_PAX_ASVC", pax_id);
}

bool deleteCrsPaxRefuse(const PaxId_t& pax_id)
{
  return deleteByPaxId("CRS_PAX_REFUSE", pax_id);
}

bool deleteCrsPaxAlarms(const PaxId_t& pax_id)
{
  return deleteByPaxId("CRS_PAX_ALARMS", pax_id);
}

bool deleteCrsPaxContext(const PaxId_t& pax_id)
{
  return deleteByPaxId("CRS_PAX_CONTEXT", pax_id);
}

bool deleteCrsPaxContext(const PaxId_t& pax_id, const std::string& key)
{
  LogTrace(TRACE6) << __func__
                   << ": pax_id=" << pax_id;
  auto cur = make_db_curs(
        "DELETE FROM CRS_PAX_CONTEXT "
        "WHERE pax_id=:pax_id ",
        PgOra::getRWSession("CRS_PAX_CONTEXT"));
  cur.stb()
      .bind(":pax_id", pax_id.get())
      .bind(":key", key)
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool deleteDcsBag(const PaxId_t& pax_id)
{
  return deleteByPaxId("DCS_BAG", pax_id);
}

bool deleteDcsTags(const PaxId_t& pax_id)
{
  return deleteByPaxId("DCS_TAGS", pax_id);
}

bool deleteTripCompLayers(const PaxId_t& pax_id)
{
  LogTrace(TRACE6) << __func__
                   << ": pax_id=" << pax_id;
  auto cur = make_db_curs(
        "DELETE FROM trip_comp_layers "
        "WHERE crs_pax_id=:pax_id ",
        PgOra::getRWSession("TRIP_COMP_LAYERS"));
  cur.stb()
      .bind(":pax_id", pax_id.get())
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool deleteTlgCompLayers(const PaxId_t& pax_id)
{
  LogTrace(TRACE6) << __func__
                   << ": pax_id=" << pax_id;
  auto cur = make_db_curs(
        "DELETE FROM tlg_comp_layers "
        "WHERE crs_pax_id=:pax_id ",
        PgOra::getRWSession("TLG_COMP_LAYERS"));
  cur.stb()
      .bind(":pax_id", pax_id.get())
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool deleteTlgCompLayers(const PointIdTlg_t& point_id)
{
  LogTrace(TRACE6) << __func__
                   << ": point_id=" << point_id;
  auto cur = make_db_curs(
        "DELETE FROM tlg_comp_layers "
        "WHERE point_id=:point_id ",
        PgOra::getRWSession("TLG_COMP_LAYERS"));
  cur.stb()
      .bind(":point_id", point_id.get())
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool deleteTlgSource(const PointIdTlg_t& point_id)
{
  LogTrace(TRACE6) << __func__
                   << ": point_id=" << point_id;
  auto cur = make_db_curs(
        "DELETE FROM tlg_source "
        "WHERE point_id_tlg = :point_id ",
        PgOra::getRWSession("TLG_SOURCE"));
  cur.stb()
      .bind(":point_id", point_id.get())
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool deleteTlgTrips(const PointIdTlg_t& point_id)
{
  LogTrace(TRACE6) << __func__
                   << ": point_id=" << point_id;
  auto cur = make_db_curs(
        "DELETE FROM tlg_trips "
        "WHERE point_id = :point_id ",
        PgOra::getRWSession("TLG_TRIPS"));
  cur.stb()
      .bind(":point_id", point_id.get())
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool deletePaxCalcData(const PaxId_t& pax_id)
{
  LogTrace(TRACE6) << __func__
                   << ": pax_id=" << pax_id;
  auto cur = make_db_curs(
        "DELETE FROM pax_calc_data "
        "WHERE pax_calc_data_id=:pax_id ",
        PgOra::getRWSession("PAX_CALC_DATA"));
  cur.stb()
      .bind(":pax_id", pax_id.get())
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool deleteCrsDataStat(const PointIdTlg_t& point_id)
{
  LogTrace(TRACE6) << __func__
                   << ": point_id=" << point_id;
  auto cur = make_db_curs(
        "DELETE FROM crs_data_stat "
        "WHERE point_id=:point_id ",
        PgOra::getRWSession("CRS_DATA_STAT"));
  cur.stb()
      .bind(":point_id", point_id.get())
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool deleteTypeBDataStat(const PointIdTlg_t& point_id)
{
  LogTrace(TRACE6) << __func__
                   << ": point_id=" << point_id;
  auto cur = make_db_curs(
        "DELETE FROM typeb_data_stat "
        "WHERE point_id=:point_id ",
        PgOra::getRWSession("TYPEB_DATA_STAT"));
  cur.stb()
      .bind(":point_id", point_id.get())
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool nullCrsDisplace2_point_id_tlg(const PointIdTlg_t& point_id)
{
  LogTrace(TRACE6) << __func__
                   << ": point_id=" << point_id;
  auto cur = make_db_curs(
        "UPDATE crs_displace2 "
        "SET point_id_tlg=NULL "
        "WHERE point_id_tlg = :point_id ",
        PgOra::getRWSession("CRS_DISPLACE2"));
  cur.stb()
      .bind(":point_id", point_id.get())
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool deletePnrAddrs(const PnrId_t& pnr_id)
{
  return deleteByPnrId("PNR_ADDRS", pnr_id);
}

bool deleteCrsTransfer(const PnrId_t& pnr_id)
{
  return deleteByPnrId("CRS_TRANSFER", pnr_id);
}

bool deleteCrsPax(const PnrId_t& pnr_id)
{
  return deleteByPnrId("CRS_PAX", pnr_id);
}

bool deletePnrMarketFlt(const PnrId_t& pnr_id)
{
  return deleteByPnrId("PNR_MARKET_FLT", pnr_id);
}

bool deleteCrsPnr(const PointIdTlg_t& point_id,
                  const std::string& system,
                  const std::optional<CrsSender_t>& sender)
{
    return deleteByPointId("CRS_PNR", point_id, system, sender);
}

bool deleteCrsData(const PointIdTlg_t& point_id,
                   const std::string& system,
                   const std::optional<CrsSender_t>& sender)
{
    return deleteByPointId("CRS_DATA", point_id, system, sender);
}

bool deleteCrsRbd(const PointIdTlg_t& point_id,
                  const std::string& system,
                  const std::optional<CrsSender_t>& sender)
{
    return deleteByPointId("CRS_RBD", point_id, system, sender);
}

bool deleteTypeBData(const PointIdTlg_t& point_id, const std::string& system,
                     const std::optional<CrsSender_t>& sender,
                     bool delete_trip_comp_layers)
{
  int result = 0;
  const std::set<PaxId_t> pax_id_set = loadPaxIdSet(point_id, system, sender);
  for (const PaxId_t& pax_id: pax_id_set) {
      result += deleteCrsSeatsBlocking(pax_id) ? 1 : 0;
      result += deleteCrsInf(pax_id) ? 1 : 0;
      result += deleteCrsInfDeleted(pax_id) ? 1 : 0;
      result += deleteCrsPaxRem(pax_id) ? 1 : 0;
      result += deleteCrsPaxDoc(pax_id) ? 1 : 0;
      result += deleteCrsPaxDoco(pax_id) ? 1 : 0;
      result += deleteCrsPaxDoca(pax_id) ? 1 : 0;
      result += deleteCrsPaxTkn(pax_id) ? 1 : 0;
      result += deleteCrsPaxFqt(pax_id) ? 1 : 0;
      result += deleteCrsPaxChkd(pax_id) ? 1 : 0;
      result += deleteCrsPaxAsvc(pax_id) ? 1 : 0;
      result += deleteCrsPaxRefuse(pax_id) ? 1 : 0;
      result += deleteCrsPaxAlarms(pax_id) ? 1 : 0;
      result += deleteCrsPaxContext(pax_id) ? 1 : 0;
      result += deleteDcsBag(pax_id) ? 1 : 0;
      result += deleteDcsTags(pax_id) ? 1 : 0;
      result += deletePaxCalcData(pax_id) ? 1 : 0;

      if (delete_trip_comp_layers) {
          result += deleteTripCompLayers(pax_id) ? 1 : 0;
      }

      result += deleteTlgCompLayers(pax_id) ? 1 : 0;
  }

  const std::set<PnrId_t> pnr_id_set = loadPnrIdSet(point_id, system, sender);
  for (const PnrId_t& pnr_id: pnr_id_set) {
      result += deletePnrAddrs(pnr_id) ? 1 : 0;
      result += deleteCrsTransfer(pnr_id) ? 1 : 0;
      result += deleteCrsPax(pnr_id) ? 1 : 0;
      result += deletePnrMarketFlt(pnr_id) ? 1 : 0;
  }

  result += deleteCrsPnr(point_id, system, sender) ? 1 : 0;
  result += deleteCrsData(point_id, system, sender) ? 1 : 0;
  result += deleteCrsRbd(point_id, system, sender) ? 1 : 0;

  return result > 0;
}

typedef std::pair<std::string,int> TicketCoupon;

//std::string getTKNO(int pax_id, const std::string& et_term = "/",
//                    bool only_TKNE)
//{
//    LogTrace(TRACE6) << __func__
//                     << ": pax_id=" << pax_id
//                     << ": et_term=" << et_term
//                     << ": only_TKNE=" << only_TKNE;
//    std::set<TicketCoupon> result;
//    std::string ticket_no;
//    int coupon_no = ASTRA::NoExists;
//    auto cur = make_db_curs(
//                "SELECT "
//                "ticket_no, "
//                "CASE WHEN rem_code='TKNE' THEN coupon_no ELSE NULL END) AS coupon_no "
//                "FROM crs_pax_tkn "
//                "WHERE pax_id=:pax_id "
//                "AND (COALESCE(:only_tkne,0)=0 OR rem_code='TKNE') "
//                "ORDER BY "
//                "CASE WHEN rem_code='TKNE' THEN 0 "
//                "     WHEN rem_code='TKNA' THEN 1 "
//                "     WHEN rem_code='TKNO' THEN 2 "
//                "     ELSE 3 END, "
//                "ticket_no,coupon_no ",
//          PgOra::getROSession("CRS_PAX_TKN"));

//    cur.stb()
//        .def(ticket_no)
//        .defNull(coupon_no, ASTRA::NoExists)
//        .bind(":pax_id", pax_id)
//        .bind(":only_tkne", only_TKNE ? 1 : 0)
//        .exec();

//    while (!cur.fen()) {
//      result.insert(std::make_pair(ticket_no, coupon_no));
//    }
//    LogTrace(TRACE6) << __func__
//                     << ": count=" << result.size();
//    return result;
//}

std::string getPSPT(int pax_id, bool with_issue_country, const std::string& language)
{
    LogTrace(TRACE6) << __func__
                     << ": pax_id=" << pax_id
                     << ": with_issue_country=" << with_issue_country
                     << ": language=" << language;
    std::string issue_country;
    std::string no;
    auto cur = make_db_curs(
                "SELECT issue_country,no "
                "FROM crs_pax_doc "
                "WHERE pax_id=:pax_id "
                "ORDER BY "
                "CASE WHEN type='P' THEN 0 "
                "     WHEN type IS NOT NULL THEN 1 "
                "     ELSE 2 END, "
                "no NULLS LAST",
          PgOra::getROSession("CRS_PAX_DOC"));

    cur.stb()
        .defNull(issue_country, "")
        .defNull(no, "")
        .bind(":pax_id", pax_id)
        .exfet();

    LogTrace(TRACE6) << __func__
                     << ": count=" << cur.rowcount();
    if (cur.err() == DbCpp::ResultCode::NoDataFound) {
      return std::string();
    }
    std::string result = no;
    if (!result.empty()
        && with_issue_country
        && !issue_country.empty())
    {
        result += " " + issue_country;
    }
    return result;
}

} //namespace TypeB
