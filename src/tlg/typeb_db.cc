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

std::set<int> LoadPaxIdSet(int point_id, const std::string& system,
                           const std::string& sender)
{
  LogTrace(TRACE5) << __func__
                   << ": point_id=" << point_id
                   << ": system=" << system
                   << ": sender=" << sender;
  std::set<int> result;
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
      .bind(":point_id", point_id)
      .bind(":system", system)
      .bind(":sender", sender)
      .exec();

  while (!cur.fen()) {
    result.insert(pax_id);
  }
  LogTrace(TRACE5) << __func__
                   << ": count=" << result.size();
  return result;
}

bool DeleteCrsSeatsBlocking(int pax_id)
{
  LogTrace(TRACE5) << __func__
                   << ": pax_id=" << pax_id;
  auto cur = make_db_curs(
        "DELETE FROM crs_seats_blocking "
        "WHERE pax_id=:pax_id ",
        PgOra::getRWSession("CRS_SEATS_BLOCKING"));
  cur.stb()
      .bind(":pax_id", pax_id)
      .exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool DeleteCrsInf(int pax_id)
{
  LogTrace(TRACE5) << __func__
                   << ": pax_id=" << pax_id;
  auto cur = make_db_curs(
        "DELETE FROM crs_inf "
        "WHERE pax_id=:pax_id ",
        PgOra::getRWSession("CRS_INF"));
  cur.stb()
      .bind(":pax_id", pax_id)
      .exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool DeleteCrsInfDeleted(int pax_id)
{
  LogTrace(TRACE5) << __func__
                   << ": pax_id=" << pax_id;
  auto cur = make_db_curs(
        "DELETE FROM crs_inf_deleted "
        "WHERE pax_id=:pax_id ",
        PgOra::getRWSession("CRS_INF_DELETED"));
  cur.stb()
      .bind(":pax_id", pax_id)
      .exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool DeleteCrsPaxRem(int pax_id)
{
  LogTrace(TRACE5) << __func__
                   << ": pax_id=" << pax_id;
  auto cur = make_db_curs(
        "DELETE FROM crs_pax_rem "
        "WHERE pax_id=:pax_id ",
        PgOra::getRWSession("CRS_PAX_REM"));
  cur.stb()
      .bind(":pax_id", pax_id)
      .exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool DeleteCrsPaxDoco(int pax_id)
{
  LogTrace(TRACE5) << __func__
                   << ": pax_id=" << pax_id;
  auto cur = make_db_curs(
        "DELETE FROM crs_pax_doco "
        "WHERE pax_id=:pax_id ",
        PgOra::getRWSession("CRS_PAX_DOCO"));
  cur.stb()
      .bind(":pax_id", pax_id)
      .exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool DeleteCrsPaxDoca(int pax_id)
{
  LogTrace(TRACE5) << __func__
                   << ": pax_id=" << pax_id;
  auto cur = make_db_curs(
        "DELETE FROM crs_pax_doca "
        "WHERE pax_id=:pax_id ",
        PgOra::getRWSession("CRS_PAX_DOCA"));
  cur.stb()
      .bind(":pax_id", pax_id)
      .exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool DeleteCrsPaxTkn(int pax_id)
{
  LogTrace(TRACE5) << __func__
                   << ": pax_id=" << pax_id;
  auto cur = make_db_curs(
        "DELETE FROM crs_pax_tkn "
        "WHERE pax_id=:pax_id ",
        PgOra::getRWSession("CRS_PAX_TKN"));
  cur.stb()
      .bind(":pax_id", pax_id)
      .exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool DeleteCrsPaxFqt(int pax_id)
{
  LogTrace(TRACE5) << __func__
                   << ": pax_id=" << pax_id;
  auto cur = make_db_curs(
        "DELETE FROM crs_pax_fqt "
        "WHERE pax_id=:pax_id ",
        PgOra::getRWSession("CRS_PAX_FQT"));
  cur.stb()
      .bind(":pax_id", pax_id)
      .exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool DeleteCrsPaxChkd(int pax_id)
{
  LogTrace(TRACE5) << __func__
                   << ": pax_id=" << pax_id;
  auto cur = make_db_curs(
        "DELETE FROM crs_pax_chkd "
        "WHERE pax_id=:pax_id ",
        PgOra::getRWSession("CRS_PAX_CHKD"));
  cur.stb()
      .bind(":pax_id", pax_id)
      .exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool DeleteCrsPaxAsvc(int pax_id)
{
  LogTrace(TRACE5) << __func__
                   << ": pax_id=" << pax_id;
  auto cur = make_db_curs(
        "DELETE FROM crs_pax_asvc "
        "WHERE pax_id=:pax_id ",
        PgOra::getRWSession("CRS_PAX_ASVC"));
  cur.stb()
      .bind(":pax_id", pax_id)
      .exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool DeleteCrsPaxRefuse(int pax_id)
{
  LogTrace(TRACE5) << __func__
                   << ": pax_id=" << pax_id;
  auto cur = make_db_curs(
        "DELETE FROM crs_pax_refuse "
        "WHERE pax_id=:pax_id ",
        PgOra::getRWSession("CRS_PAX_REFUSE"));
  cur.stb()
      .bind(":pax_id", pax_id)
      .exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool DeleteCrsPaxAlarms(int pax_id)
{
  LogTrace(TRACE5) << __func__
                   << ": pax_id=" << pax_id;
  auto cur = make_db_curs(
        "DELETE FROM crs_pax_alarms "
        "WHERE pax_id=:pax_id ",
        PgOra::getRWSession("CRS_PAX_ALARMS"));
  cur.stb()
      .bind(":pax_id", pax_id)
      .exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool DeleteCrsPaxContext(int pax_id)
{
  LogTrace(TRACE5) << __func__
                   << ": pax_id=" << pax_id;
  auto cur = make_db_curs(
        "DELETE FROM crs_pax_context "
        "WHERE pax_id=:pax_id ",
        PgOra::getRWSession("CRS_PAX_CONTEXT"));
  cur.stb()
      .bind(":pax_id", pax_id)
      .exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool DeleteDcsBag(int pax_id)
{
  LogTrace(TRACE5) << __func__
                   << ": pax_id=" << pax_id;
  auto cur = make_db_curs(
        "DELETE FROM dcs_bag "
        "WHERE pax_id=:pax_id ",
        PgOra::getRWSession("DCS_BAG"));
  cur.stb()
      .bind(":pax_id", pax_id)
      .exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool DeleteDcsTags(int pax_id)
{
  LogTrace(TRACE5) << __func__
                   << ": pax_id=" << pax_id;
  auto cur = make_db_curs(
        "DELETE FROM dcs_tags "
        "WHERE pax_id=:pax_id ",
        PgOra::getRWSession("DCS_TAGS"));
  cur.stb()
      .bind(":pax_id", pax_id)
      .exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool DeleteTripCompLayers(int pax_id)
{
  LogTrace(TRACE5) << __func__
                   << ": pax_id=" << pax_id;
  auto cur = make_db_curs(
        "DELETE FROM trip_comp_layers "
        "WHERE pax_id=:pax_id ",
        PgOra::getRWSession("TRIP_COMP_LAYERS"));
  cur.stb()
      .bind(":pax_id", pax_id)
      .exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool DeleteTlgCompLayers(int pax_id)
{
  LogTrace(TRACE5) << __func__
                   << ": pax_id=" << pax_id;
  auto cur = make_db_curs(
        "DELETE FROM tlg_comp_layers "
        "WHERE pax_id=:pax_id ",
        PgOra::getRWSession("tlg_comp_layers"));
  cur.stb()
      .bind(":pax_id", pax_id)
      .exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool DeletePaxCalcData(int pax_id)
{
  LogTrace(TRACE5) << __func__
                   << ": pax_id=" << pax_id;
  auto cur = make_db_curs(
        "DELETE FROM pax_calc_data "
        "WHERE pax_id=:pax_id ",
        PgOra::getRWSession("PAX_CALC_DATA"));
  cur.stb()
      .bind(":pax_id", pax_id)
      .exec();

  LogTrace(TRACE5) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool DeleteTypeBData(int point_id, const std::string& system, const std::string& sender,
                     bool delete_trip_comp_layers)
{
  int result = 0;
  const std::set<int> paxIdSet = LoadPaxIdSet(point_id, system, sender);
  for (int pax_id: paxIdSet) {
      result += DeleteCrsSeatsBlocking(pax_id) ? 1 : 0;
      result += DeleteCrsInf(pax_id) ? 1 : 0;
      result += DeleteCrsInfDeleted(pax_id) ? 1 : 0;
      result += DeleteCrsPaxRem(pax_id) ? 1 : 0;
      result += DeleteCrsPaxDoco(pax_id) ? 1 : 0;
      result += DeleteCrsPaxDoca(pax_id) ? 1 : 0;
      result += DeleteCrsPaxTkn(pax_id) ? 1 : 0;
      result += DeleteCrsPaxFqt(pax_id) ? 1 : 0;
      result += DeleteCrsPaxChkd(pax_id) ? 1 : 0;
      result += DeleteCrsPaxAsvc(pax_id) ? 1 : 0;
      result += DeleteCrsPaxRefuse(pax_id) ? 1 : 0;
      result += DeleteCrsPaxAlarms(pax_id) ? 1 : 0;
      result += DeleteCrsPaxContext(pax_id) ? 1 : 0;
      result += DeleteDcsBag(pax_id) ? 1 : 0;
      result += DeleteDcsTags(pax_id) ? 1 : 0;
      result += DeletePaxCalcData(pax_id) ? 1 : 0;

      if (delete_trip_comp_layers) {
          result += DeleteTripCompLayers(pax_id) ? 1 : 0;
      }

      result += DeleteTlgCompLayers(pax_id) ? 1 : 0;
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
