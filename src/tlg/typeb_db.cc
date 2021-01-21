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

std::set<PaxId_t> loadPaxIdSet(PointIdTlg_t point_id, const std::string& system,
                               const std::string& sender)
{
  LogTrace(TRACE6) << __func__
                   << ": point_id=" << point_id
                   << ": system=" << system
                   << ": sender=" << sender;
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
      .bind(":sender", sender)
      .exec();

  while (!cur.fen()) {
    result.emplace(pax_id);
  }
  LogTrace(TRACE6) << __func__
                   << ": count=" << result.size();
  return result;
}

namespace {

bool deleteByPaxId(const std::string& table_name, PaxId_t pax_id)
{
  LogTrace(TRACE6) << __func__
                   << ": pax_id=" << pax_id;
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

} //namespace

bool deleteCrsSeatsBlocking(PaxId_t pax_id)
{
  return deleteByPaxId("CRS_SEATS_BLOCKING", pax_id);
}

bool deleteCrsInf(PaxId_t pax_id)
{
  return deleteByPaxId("CRS_INF", pax_id);
}

bool deleteCrsInfDeleted(PaxId_t pax_id)
{
  return deleteByPaxId("CRS_INF_DELETED", pax_id);
}

bool deleteCrsPaxRem(PaxId_t pax_id)
{
  return deleteByPaxId("CRS_PAX_REM", pax_id);
}

bool deleteCrsPaxDoco(PaxId_t pax_id)
{
  return deleteByPaxId("CRS_PAX_DOCO", pax_id);
}

bool deleteCrsPaxDoca(PaxId_t pax_id)
{
  return deleteByPaxId("CRS_PAX_DOCA", pax_id);
}

bool deleteCrsPaxTkn(PaxId_t pax_id)
{
  return deleteByPaxId("CRS_PAX_TKN", pax_id);
}

bool deleteCrsPaxFqt(PaxId_t pax_id)
{
  return deleteByPaxId("CRS_PAX_FQT", pax_id);
}

bool deleteCrsPaxChkd(PaxId_t pax_id)
{
  return deleteByPaxId("CRS_PAX_CHKD", pax_id);
}

bool deleteCrsPaxAsvc(PaxId_t pax_id)
{
  return deleteByPaxId("CRS_PAX_ASVC", pax_id);
}

bool deleteCrsPaxRefuse(PaxId_t pax_id)
{
  return deleteByPaxId("CRS_PAX_REFUSE", pax_id);
}

bool deleteCrsPaxAlarms(PaxId_t pax_id)
{
  return deleteByPaxId("CRS_PAX_ALARMS", pax_id);
}

bool deleteCrsPaxContext(PaxId_t pax_id)
{
  return deleteByPaxId("CRS_PAX_CONTEXT", pax_id);
}

bool deleteDcsBag(PaxId_t pax_id)
{
  return deleteByPaxId("DCS_BAG", pax_id);
}

bool deleteDcsTags(PaxId_t pax_id)
{
  return deleteByPaxId("DCS_TAGS", pax_id);
}

bool deleteTripCompLayers(PaxId_t pax_id)
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

bool deleteTlgCompLayers(PaxId_t pax_id)
{
  LogTrace(TRACE6) << __func__
                   << ": pax_id=" << pax_id;
  auto cur = make_db_curs(
        "DELETE FROM tlg_comp_layers "
        "WHERE crs_pax_id=:pax_id ",
        PgOra::getRWSession("tlg_comp_layers"));
  cur.stb()
      .bind(":pax_id", pax_id.get())
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool deletePaxCalcData(PaxId_t pax_id)
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

bool deleteTypeBData(PointIdTlg_t point_id, const std::string& system, const std::string& sender,
                     bool delete_trip_comp_layers)
{
  int result = 0;
  const std::set<PaxId_t> paxIdSet = loadPaxIdSet(point_id, system, sender);
  for (PaxId_t pax_id: paxIdSet) {
      result += deleteCrsSeatsBlocking(pax_id) ? 1 : 0;
      result += deleteCrsInf(pax_id) ? 1 : 0;
      result += deleteCrsInfDeleted(pax_id) ? 1 : 0;
      result += deleteCrsPaxRem(pax_id) ? 1 : 0;
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
  return result > 0;
}

typedef std::pair<std::string,int> TicketCoupon;

//std::string getTKNO(int pax_id, const std::string& et_term = "/",
//                    bool only_TKNE)
//{
//    LogTrace(TRACE5) << __func__
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
//    LogTrace(TRACE5) << __func__
//                     << ": count=" << result.size();
//    return result;
//}

} //namespace TypeB
