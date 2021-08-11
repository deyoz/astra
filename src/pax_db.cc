#include "pax_db.h"

#include <set>
#include "astra_consts.h"
#include <serverlib/dbcpp_cursctl.h>
#include "PgOraConfig.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

static bool deleteByPaxId(const std::string& table_name, PaxId_t pax_id, STDLOG_SIGNATURE)
{
  LogTrace(TRACE6) << __func__
                   << ": table_name=" << table_name
                   << ", pax_id=" << pax_id
                   << ", called from " << file << ":" << line;
  auto cur = DbCpp::make_curs_(STDLOG_VARIABLE,
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
  return deleteByPaxId("ANNUL_BAG", pax_id, STDLOG);
}

bool deletePaxEvents(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_EVENTS", pax_id, STDLOG);
}

bool deleteStatAd(PaxId_t pax_id)
{
  return deleteByPaxId("STAT_AD", pax_id, STDLOG);
}

bool deleteStatServices(PaxId_t pax_id)
{
  return deleteByPaxId("STAT_SERVICES", pax_id, STDLOG);
}

bool deleteConfirmPrint(PaxId_t pax_id)
{
  return deleteByPaxId("CONFIRM_PRINT", pax_id, STDLOG);
}

bool deletePaxDOC(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_DOC", pax_id, STDLOG);
}

bool deletePaxDOCO(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_DOCO", pax_id, STDLOG);
}

bool deletePaxDOCA(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_DOCA", pax_id, STDLOG);
}

bool deletePaxFQT(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_FQT", pax_id, STDLOG);
}

bool deletePaxASVC(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_ASVC", pax_id, STDLOG);
}

bool deletePaxEmd(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_EMD", pax_id, STDLOG);
}

bool deletePaxNorms(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_NORMS", pax_id, STDLOG);
}

bool deletePaxBrands(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_BRANDS", pax_id, STDLOG);
}

bool deletePaxRem(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_REM", pax_id, STDLOG);
}

bool deletePaxRemOrigin(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_REM_ORIGIN", pax_id, STDLOG);
}

bool deletePaxSeats(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_SEATS", pax_id, STDLOG);
}

bool deleteRozysk(PaxId_t pax_id)
{
  return deleteByPaxId("ROZYSK", pax_id, STDLOG);
}

bool deleteTransferSubcls(PaxId_t pax_id)
{
  return deleteByPaxId("TRANSFER_SUBCLS", pax_id, STDLOG);
}

bool deleteTripCompLayers(PaxId_t pax_id)
{
  return deleteByPaxId("TRIP_COMP_LAYERS", pax_id, STDLOG);
}

bool deletePaxAlarms(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_ALARMS", pax_id, STDLOG);
}

bool deletePaxCustomAlarms(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_CUSTOM_ALARMS", pax_id, STDLOG);
}

bool deletePaxServiceLists(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_SERVICE_LISTS", pax_id, STDLOG);
}

bool deletePaxServices(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_SERVICES", pax_id, STDLOG);
}

bool deletePaxServicesAuto(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_SERVICES_AUTO", pax_id, STDLOG);
}

bool deletePaidRfisc(PaxId_t pax_id)
{
  return deleteByPaxId("PAID_RFISC", pax_id, STDLOG);
}

bool deletePaxNormsText(PaxId_t pax_id)
{
  return deleteByPaxId("PAX_NORMS_TEXT", pax_id, STDLOG);
}

bool deleteTrferPaxStat(PaxId_t pax_id)
{
  return deleteByPaxId("TRFER_PAX_STAT", pax_id, STDLOG);
}

bool deleteBiStat(PaxId_t pax_id)
{
  return deleteByPaxId("BI_STAT", pax_id, STDLOG);
}

bool deleteSBDOTagsGenerated(PaxId_t pax_id)
{
  return deleteByPaxId("SBDO_TAGS_GENERATED", pax_id, STDLOG);
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
  return deleteByPaxId("PAX_CONFIRMATIONS", pax_id, STDLOG);
}

bool deletePax(PaxId_t pax_id)
{
  return deleteByPaxId("PAX", pax_id, STDLOG);
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
