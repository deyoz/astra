#include "pax_db.h"

#include <set>
#include "astra_consts.h"
#include <serverlib/dbcpp_cursctl.h>
#include "PgOraConfig.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

static bool deleteByPaxId(const std::string& table_name, PaxId_t pax_id)
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
                   << ": table_name=" << table_name
                   << ", rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

std::set<int> getAnnulBagIdSet(PaxId_t pax_id)
{
  LogTrace(TRACE5) << __func__
                   << ": pax_id=" << pax_id;
  std::set<int> result;
  int id = ASTRA::NoExists;
  auto cur = make_db_curs(
        "SELECT id "
        "FROM annul_bag "
        "WHERE pax_id=:pax_id ",
        PgOra::getROSession("ANNUL_BAG"));

  cur.stb()
      .def(id)
      .bind(":pax_id", pax_id.get())
      .exec();

  while (!cur.fen()) {
    result.insert(id);
  }
  LogTrace(TRACE5) << __func__
                   << ": count=" << result.size();
  return result;
}

bool deleteAnnulTagsById(int id)
{
  LogTrace(TRACE6) << __func__
                   << ": id=" << id;
  auto cur = make_db_curs(
        "DELETE FROM annul_tags "
        "WHERE id=:id ",
        PgOra::getRWSession("ANNUL_TAGS"));
  cur.stb()
      .bind(":id", id)
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool deleteAnnulTags(PaxId_t pax_id)
{
  int result = 0;
  const std::set<int> ids = getAnnulBagIdSet(pax_id);
  for (int id: ids) {
      result += deleteAnnulTagsById(id) ? 1 : 0;
  }
  return result > 0;
}

bool deleteAnnulBag(PaxId_t pax_id)
{
  return deleteByPaxId("ANNUL_BAG", pax_id);
}

bool deletePaxEvents(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_EVENTS", pax_id);
}

bool deleteStatAd(PaxId_t pax_id)
{
  return deleteByPaxId("STAT_AD", pax_id);
}

bool deleteStatServices(PaxId_t pax_id)
{
  return deleteByPaxId("STAT_SERVICES", pax_id);
}

bool deleteConfirmPrint(PaxId_t pax_id)
{
  return deleteByPaxId("CONFIRM_PRINT", pax_id);
}

bool deletePaxDOC(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_DOC", pax_id);
}

bool deletePaxDOCO(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_DOCO", pax_id);
}

bool deletePaxDOCA(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_DOCA", pax_id);
}

bool deletePaxFQT(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_FQT", pax_id);
}

bool deletePaxASVC(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_ASVC", pax_id);
}

bool deletePaxEmd(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_EMD", pax_id);
}

bool deletePaxNorms(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_NORMS", pax_id);
}

bool deletePaxBrands(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_BRANDS", pax_id);
}

bool deletePaxRem(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_REM", pax_id);
}

bool deletePaxRemOrigin(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_REM_ORIGIN", pax_id);
}

bool deletePaxSeats(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_SEATS", pax_id);
}

bool deleteRozysk(PaxId_t pax_id)
{
  return deleteByPaxId("ROZYSK", pax_id);
}

bool deleteTransferSubcls(PaxId_t pax_id)
{
  return deleteByPaxId("TRANSFER_SUBCLS", pax_id);
}

bool deleteTripCompLayers(PaxId_t pax_id)
{
  return deleteByPaxId("TRIP_COMP_LAYERS", pax_id);
}

bool deletePaxAlarms(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_ALARMS", pax_id);
}

bool deletePaxCustomAlarms(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_CUSTOM_ALARMS", pax_id);
}

bool deletePaxServiceLists(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_SERVICE_LISTS", pax_id);
}

bool deletePaxServices(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_SERVICES", pax_id);
}

bool deletePaxServicesAuto(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_SERVICES_AUTO", pax_id);
}

bool deletePaidRfisc(PaxId_t pax_id)
{
  return deleteByPaxId("PAID_RFISC", pax_id);
}

bool deletePaxNormsText(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_NORMS_TEXT", pax_id);
}

bool deleteTrferPaxStat(PaxId_t pax_id)
{
  return deleteByPaxId("TRFER_PAX_STAT", pax_id);
}

bool deleteBiStat(PaxId_t pax_id)
{
  return deleteByPaxId("BI_STAT", pax_id);
}

bool deleteSBDOTagsGenerated(PaxId_t pax_id)
{
  return deleteByPaxId("SBDO_TAGS_GENERATED", pax_id);
}

bool deletePaxCalcData(PaxId_t pax_id)
{
  LogTrace(TRACE6) << __func__
                   << ": pax_id=" << pax_id;
  auto cur = make_db_curs(
        "DELETE FROM pax_calc_data "
        "WHERE pax_calc_data_id=:pax_id "
        "AND crs_deleted IS NULL ",
        PgOra::getRWSession("PAX_CALC_DATA"));
  cur.stb()
      .bind(":pax_id", pax_id.get())
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool deletePaxConfirmations(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_CONFIRMATIONS", pax_id);
}

bool deletePax(PaxId_t pax_id)
{
  return deleteByPaxId("PAX", pax_id);
}

bool clearServicePaymentPaxId(PaxId_t pax_id)
{
  LogTrace(TRACE6) << __func__
                   << ": pax_id=" << pax_id;
  auto cur = make_db_curs(
        "UPDATE service_payment "
        "SET pax_id=NULL "
        "WHERE pax_id=:pax_id ",
        PgOra::getRWSession("SERVICE_PAYMENT"));
  cur.stb()
      .bind(":pax_id", pax_id.get())
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool deletePaxData(PaxId_t pax_id)
{
  int result = 0;
  result += deleteAnnulTags(pax_id) ? 1 : 0;
  result += deleteAnnulBag(pax_id) ? 1 : 0;
  result += deletePaxEvents(pax_id) ? 1 : 0;
  result += deleteStatAd(pax_id) ? 1 : 0;
  result += deleteStatServices(pax_id) ? 1 : 0;
  result += deleteConfirmPrint(pax_id) ? 1 : 0;
  result += deletePaxDOC(pax_id) ? 1 : 0;
  result += deletePaxDOCO(pax_id) ? 1 : 0;
  result += deletePaxDOCA(pax_id) ? 1 : 0;
  result += deletePaxFQT(pax_id) ? 1 : 0;
  result += deletePaxASVC(pax_id) ? 1 : 0;
  result += deletePaxEmd(pax_id) ? 1 : 0;
  result += deletePaxNorms(pax_id) ? 1 : 0;
  result += deletePaxBrands(pax_id) ? 1 : 0;
  result += deletePaxRem(pax_id) ? 1 : 0;
  result += deletePaxRemOrigin(pax_id) ? 1 : 0;
  result += deletePaxSeats(pax_id) ? 1 : 0;
  result += deleteRozysk(pax_id) ? 1 : 0;
  result += deleteTransferSubcls(pax_id) ? 1 : 0;
  result += deleteTripCompLayers(pax_id) ? 1 : 0;
  result += clearServicePaymentPaxId(pax_id) ? 1 : 0;
  result += deletePaxAlarms(pax_id) ? 1 : 0;
  result += deletePaxCustomAlarms(pax_id) ? 1 : 0;
  result += deletePaxServiceLists(pax_id) ? 1 : 0;
  result += deletePaxServices(pax_id) ? 1 : 0;
  result += deletePaxServicesAuto(pax_id) ? 1 : 0;
  result += deletePaidRfisc(pax_id) ? 1 : 0;
  result += deletePaxNormsText(pax_id) ? 1 : 0;
  result += deleteTrferPaxStat(pax_id) ? 1 : 0;
  result += deleteBiStat(pax_id) ? 1 : 0;
  result += deleteSBDOTagsGenerated(pax_id) ? 1 : 0;
  result += deletePaxCalcData(pax_id) ? 1 : 0;
  result += deletePaxConfirmations(pax_id) ? 1 : 0;
  result += deletePax(pax_id) ? 1 : 0;
  return result > 0;
}
