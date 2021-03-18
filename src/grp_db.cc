#include "grp_db.h"

#include <set>
#include "astra_consts.h"
#include "pax_db.h"
#include <serverlib/dbcpp_cursctl.h>
#include "PgOraConfig.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

namespace {

struct TransferData
{
  int num;
  int point_id;
};

std::vector<TransferData> loadTransfers(GrpId_t grp_id)
{
  LogTrace(TRACE6) << __func__
                   << ": grp_id=" << grp_id;
  std::vector<TransferData> result;
  TransferData item = { ASTRA::NoExists, ASTRA::NoExists };
  auto cur = make_db_curs(
    "SELECT transfer_num, point_id_trfer "
    "FROM transfer "
    "WHERE grp_id=:grp_id "
    "FOR UPDATE ",
    PgOra::getRWSession("TRANSFER"));

  cur.stb()
    .def(item.num)
    .def(item.point_id)
    .bind(":grp_id", grp_id.get())
    .exec();

  while (!cur.fen()) {
    result.push_back(item);
  }
  LogTrace(TRACE6) << __func__
                   << ": count=" << result.size();
  return result;
}

typedef TransferData TCkinSegment;

std::vector<TCkinSegment> loadTCkinSegments(GrpId_t grp_id)
{
  LogTrace(TRACE6) << __func__
                   << ": grp_id=" << grp_id;
  std::vector<TCkinSegment> result;
  TCkinSegment item = { ASTRA::NoExists, ASTRA::NoExists };
  auto cur = make_db_curs(
    "SELECT seg_no, point_id_trfer "
    "FROM tckin_segments "
    "WHERE grp_id=:grp_id "
    "FOR UPDATE ",
    PgOra::getRWSession("TCKIN_SEGMENTS"));

  cur.stb()
    .def(item.num)
    .def(item.point_id)
    .bind(":grp_id", grp_id.get())
    .exec();

  while (!cur.fen()) {
    result.push_back(item);
  }
  LogTrace(TRACE6) << __func__
                   << ": count=" << result.size();
  return result;
}

bool deleteByGrpId(const std::string& table_name, GrpId_t grp_id)
{
  LogTrace(TRACE6) << __func__
                   << ": table_name=" << table_name
                   << ", grp_id=" << grp_id;
  auto cur = make_db_curs(
        "DELETE FROM " + table_name + " "
        "WHERE grp_id=:grp_id ",
        PgOra::getRWSession(table_name));
  cur.stb()
      .bind(":grp_id", grp_id.get())
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": table_name=" << table_name
                   << ", rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

std::set<int> getAnnulBagIdSet(GrpId_t grp_id)
{
  LogTrace(TRACE5) << __func__
                   << ": grp_id=" << grp_id;
  std::set<int> result;
  int id = ASTRA::NoExists;
  auto cur = make_db_curs(
        "SELECT id "
        "FROM annul_bag "
        "WHERE grp_id=:grp_id ",
        PgOra::getROSession("ANNUL_BAG"));

  cur.stb()
      .def(id)
      .bind(":grp_id", grp_id.get())
      .exec();

  while (!cur.fen()) {
    result.insert(id);
  }
  LogTrace(TRACE5) << __func__
                   << ": count=" << result.size();
  return result;
}

bool deleteTransfer(GrpId_t grp_id, int num)
{
  LogTrace(TRACE6) << __func__
                   << ": grp_id=" << grp_id
                   << ", num=" << num;
  auto cur = make_db_curs(
        "DELETE FROM transfer "
        "WHERE grp_id=:grp_id "
        "AND transfer_num=:num ",
        PgOra::getRWSession("TRANSFER"));
  cur.stb()
      .bind(":grp_id", grp_id.get())
      .bind(":num", num)
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool deleteTCkinSegment(GrpId_t grp_id, int num)
{
  LogTrace(TRACE6) << __func__
                   << ": grp_id=" << grp_id
                   << ", num=" << num;
  auto cur = make_db_curs(
        "DELETE FROM tckin_segments "
        "WHERE grp_id=:grp_id "
        "AND seg_no=:num ",
        PgOra::getRWSession("TCKIN_SEGMENTS"));
  cur.stb()
      .bind(":grp_id", grp_id.get())
      .bind(":num", num)
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool existsTransfer(PointId_t point_id)
{
  LogTrace(TRACE6) << __func__
                   << ": point_id=" << point_id;
  auto cur = make_db_curs(
        "SELECT 1 FROM TRANSFER "
        "WHERE POINT_ID_TRFER = :point_id "
        "FETCH FIRST 1 ROWS ONLY ",
        PgOra::getRWSession("TRFER_TRIPS"));
  cur.stb()
      .bind(":point_id", point_id.get())
      .EXfet();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool existsTCkinSegments(PointId_t point_id)
{
  LogTrace(TRACE6) << __func__
                   << ": point_id=" << point_id;
  auto cur = make_db_curs(
        "SELECT 1 FROM TCKIN_SEGMENTS "
        "WHERE POINT_ID_TRFER = :point_id "
        "FETCH FIRST 1 ROWS ONLY ",
        PgOra::getRWSession("TRFER_TRIPS"));
  cur.stb()
      .bind(":point_id", point_id.get())
      .EXfet();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool deleteTransferTrips(PointId_t point_id)
{
  if (existsTransfer(point_id)) {
    return false;
  }
  if (existsTCkinSegments(point_id)) {
    return false;
  }
  LogTrace(TRACE6) << __func__
                   << ": point_id=" << point_id;
  auto cur = make_db_curs(
        "DELETE FROM trfer_trips "
        "WHERE point_id=:point_id ",
        PgOra::getRWSession("TRFER_TRIPS"));
  cur.stb()
      .bind(":point_id", point_id.get())
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

} //namespace

bool deleteTransfers(GrpId_t grp_id)
{
  LogTrace(TRACE6) << __func__ << ": grp_id=" << grp_id;
  int result = 0;
  const std::vector<TransferData> transfers = loadTransfers(grp_id);
  for (const TransferData& transfer: transfers) {
    result += deleteTransfer(grp_id, transfer.num) ? 1 : 0;
    result += deleteTransferTrips(PointId_t(transfer.point_id)) ? 1 : 0;
  }
  result += deleteTCkinSegments(grp_id) ? 1 : 0;
  return result > 0;
}

bool deleteTCkinSegments(GrpId_t grp_id)
{
  int result = 0;
  const std::vector<TCkinSegment> segments = loadTCkinSegments(grp_id);
  for (const TCkinSegment& segment: segments) {
    result += deleteTCkinSegment(grp_id, segment.num) ? 1 : 0;
    result += deleteTransferTrips(PointId_t(segment.point_id)) ? 1 : 0;
  }
  return result > 0;
}

bool deleteAnnulTags(GrpId_t grp_id)
{
  int result = 0;
  const std::set<int> ids = getAnnulBagIdSet(grp_id);
  for (int id: ids) {
      result += deleteAnnulTagsById(id) ? 1 : 0;
  }
  return result > 0;
}

bool deleteAnnulBag(GrpId_t grp_id)
{
  return deleteByGrpId("ANNUL_BAG", grp_id);
}

bool deleteBagPrepay(GrpId_t grp_id)
{
  return deleteByGrpId("BAG_PREPAY", grp_id);
}

bool deleteBagTags(GrpId_t grp_id)
{
  return deleteByGrpId("BAG_TAGS", grp_id);
}

bool deleteBagTags(GrpId_t grp_id, int bag_num)
{
    LogTrace(TRACE6) << __func__
                     << ": grp_id=" << grp_id
                     << ", bag_num=" << bag_num;
    auto cur = make_db_curs(
          "DELETE FROM bag_tags "
          "WHERE grp_id=:grp_id "
          "AND bag_num=:bag_num ",
          PgOra::getRWSession("BAG_TAGS"));
    cur.stb()
        .bind(":grp_id", grp_id.get())
        .bind(":bag_num", bag_num)
        .exec();

    LogTrace(TRACE6) << __func__
                     << ": rowcount=" << cur.rowcount();
    return cur.rowcount() > 0;
}

bool deleteBagTagsGenerated(GrpId_t grp_id)
{
  return deleteByGrpId("BAG_TAGS_GENERATED", grp_id);
}

bool deleteUnaccompBagInfo(GrpId_t grp_id)
{
  return deleteByGrpId("UNACCOMP_BAG_INFO", grp_id);
}

bool deleteUnaccompBagInfo(GrpId_t grp_id, int num)
{
  LogTrace(TRACE6) << __func__
                   << ": grp_id=" << grp_id
                   << ", num=" << num;
  auto cur = make_db_curs(
        "DELETE FROM unaccomp_bag_info "
        "WHERE grp_id=:grp_id "
        "AND num=:num ",
        PgOra::getRWSession("UNACCOMP_BAG_INFO"));
  cur.stb()
      .bind(":grp_id", grp_id.get())
      .bind(":num", num)
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool deleteBag2(GrpId_t grp_id)
{
  return deleteByGrpId("BAG2", grp_id);
}

bool deleteBag2(GrpId_t grp_id, int num)
{
  LogTrace(TRACE6) << __func__
                   << ": grp_id=" << grp_id
                   << ", num=" << num;
  auto cur = make_db_curs(
        "DELETE FROM BAG2 "
        "WHERE grp_id=:grp_id "
        "AND num=:num ",
        PgOra::getRWSession("BAG2"));
  cur.stb()
      .bind(":grp_id", grp_id.get())
      .bind(":num", num)
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool deleteGrpNorms(GrpId_t grp_id)
{
  return deleteByGrpId("GRP_NORMS", grp_id);
}

bool deletePaidBag(GrpId_t grp_id)
{
  return deleteByGrpId("PAID_BAG", grp_id);
}

bool deletePaidBagEmdProps(GrpId_t grp_id)
{
  return deleteByGrpId("PAID_BAG_EMD_PROPS", grp_id);
}

bool deleteServicePayment(GrpId_t grp_id)
{
  return deleteByGrpId("SERVICE_PAYMENT", grp_id);
}

bool deleteTckinPaxGrp(GrpId_t grp_id)
{
  return deleteByGrpId("TCKIN_PAX_GRP", grp_id);
}

bool deleteValueBag(GrpId_t grp_id)
{
  return deleteByGrpId("VALUE_BAG", grp_id);
}

bool deleteValueBag(GrpId_t grp_id, int num)
{
  LogTrace(TRACE6) << __func__
                   << ": grp_id=" << grp_id
                   << ", num=" << num;
  auto cur = make_db_curs(
        "DELETE FROM value_bag "
        "WHERE grp_id=:grp_id "
        "AND num=:num ",
        PgOra::getRWSession("VALUE_BAG"));
  cur.stb()
      .bind(":grp_id", grp_id.get())
      .bind(":num", num)
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool deletePnrAddrsPC(GrpId_t grp_id)
{
  return deleteByGrpId("PNR_ADDRS_PC", grp_id);
}

bool deleteGrpServiceLists(GrpId_t grp_id)
{
  return deleteByGrpId("GRP_SERVICE_LISTS", grp_id);
}

bool deleteSvcPrices(GrpId_t grp_id)
{
  return deleteByGrpId("SVC_PRICES", grp_id);
}

bool deletePaxGrp(GrpId_t grp_id)
{
  return deleteByGrpId("PAX_GRP", grp_id);
}

bool clearBagReceiptsGrpId(GrpId_t grp_id)
{
  LogTrace(TRACE6) << __func__
                   << ": grp_id=" << grp_id;
  auto cur = make_db_curs(
        "UPDATE bag_receipts "
        "SET grp_id=NULL "
        "WHERE grp_id=:grp_id ",
        PgOra::getRWSession("BAG_RECEIPTS"));
  cur.stb()
      .bind(":grp_id", grp_id.get())
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool deleteGroupData(GrpId_t grp_id)
{
  int result = 0;
  result += deleteAnnulTags(grp_id) ? 1 : 0;
  result += deleteAnnulBag(grp_id) ? 1 : 0;
  result += deleteBagPrepay(grp_id) ? 1 : 0;
  result += clearBagReceiptsGrpId(grp_id) ? 1 : 0;
  result += deleteBagTags(grp_id) ? 1 : 0;
  result += deleteBagTagsGenerated(grp_id) ? 1 : 0;
  result += deleteUnaccompBagInfo(grp_id) ? 1 : 0;
  result += deleteBag2(grp_id) ? 1 : 0;
  result += deleteGrpNorms(grp_id) ? 1 : 0;
  result += deletePaidBag(grp_id) ? 1 : 0;
  result += deletePaidBagEmdProps(grp_id) ? 1 : 0;
  result += deleteServicePayment(grp_id) ? 1 : 0;
  result += deleteTckinPaxGrp(grp_id) ? 1 : 0;
  result += deleteTransfers(grp_id) ? 1 : 0;
  result += deleteValueBag(grp_id) ? 1 : 0;
  result += deletePnrAddrsPC(grp_id) ? 1 : 0;
  result += deleteGrpServiceLists(grp_id) ? 1 : 0;
  result += deleteSvcPrices(grp_id) ? 1 : 0;
  result += deletePaxGrp(grp_id) ? 1 : 0;
  return result > 0;
}

