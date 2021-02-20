#include <stdlib.h>
#include <map>
#include "prepreg.h"
#include "exceptions.h"
#include "xml_unit.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "oralib.h"
#include "stages.h"
#include "oralib.h"
#include "stl_utils.h"
#include "tripinfo.h"
#include "docs/docs_common.h"
#include "stat/stat_utils.h"
#include "crafts/ComponCreator.h"
#include "points.h"
#include "term_version.h"
#include "trip_tasks.h"
#include "flt_settings.h"
#include "counters.h"

#define NICKNAME "DJEK"
#include <serverlib/slogger.h>
#include <serverlib/cursctl.h>

using namespace std;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace ASTRA;

void PrepRegInterface::readTripCounters( int point_id, xmlNodePtr dataNode )
{
  ProgTrace(TRACE5, "PrepRegInterface::readTripCounters" );
  TQuery Qry( &OraSession );

  bool cfg_exists=!TCFG(point_id).empty();
  bool free_seating=false;
  if (!cfg_exists)
    free_seating=SALONS2::isFreeSeating(point_id);

  Qry.SQLText =
     "SELECT -100 AS num, "
     "        'Всего' AS firstcol, "
     "        SUM(cfg) AS cfg, "
     "        SUM(crs_ok) AS resa, "
     "        SUM(crs_tranzit) AS tranzit, "
     "        SUM(block) AS block, "
     "        SUM(avail) AS avail, "
     "        SUM(prot) AS prot "
     "FROM "
     " (SELECT MAX(cfg) AS cfg, "
     "         SUM(crs_ok) AS crs_ok, "
     "         SUM(crs_tranzit) AS crs_tranzit, "
     "         MAX(block) AS block, "
     "         DECODE(:cfg_exists, 0, TO_NUMBER(NULL), GREATEST(MAX(avail),0)) AS avail, "
     "         MAX(prot) AS prot "
     "  FROM counters2 c,trip_classes,classes "
     "  WHERE c.point_dep=trip_classes.point_id(+) AND "
     "        c.class=trip_classes.class(+) AND "
     "        (trip_classes.point_id IS NOT NULL OR :cfg_exists=0 AND :free_seating<>0) AND "
     "        c.class=classes.code AND "
     "        c.point_dep=:point_id  "
     "  GROUP BY classes.priority,c.class) "
     "UNION "
     "SELECT classes.priority-10, "
     "       c.class, "
     "       MAX(cfg), "
     "       SUM(crs_ok), "
     "       SUM(crs_tranzit), "
     "       MAX(block), "
     "       DECODE(:cfg_exists, 0, TO_NUMBER(NULL), GREATEST(MAX(avail),0)), "
     "       MAX(prot) "
     "FROM counters2 c,trip_classes,classes "
     "WHERE c.point_dep=trip_classes.point_id(+) AND "
     "      c.class=trip_classes.class(+) AND "
     "      (trip_classes.point_id IS NOT NULL OR :cfg_exists=0 AND :free_seating<>0) AND "
     "      c.class=classes.code AND "
     "      c.point_dep=:point_id  "
     "GROUP BY classes.priority,c.class "
     "UNION "
     "SELECT points.point_num+100, "
     "       points.airp, "
     "       SUM(cfg), "
     "       SUM(crs_ok), "
     "       SUM(crs_tranzit), "
     "       SUM(block), "
     "       DECODE(:cfg_exists, 0, TO_NUMBER(NULL), GREATEST(SUM(avail),0)), "
     "       SUM(prot)  "
     "FROM counters2 c,trip_classes,points "
     "WHERE c.point_dep=trip_classes.point_id(+) AND "
     "      c.class=trip_classes.class(+) AND "
     "      (trip_classes.point_id IS NOT NULL OR :cfg_exists=0 AND :free_seating<>0) AND "
     "      c.point_arv=points.point_id AND "
     "      c.point_dep=:point_id AND points.pr_del>=0 "
     "GROUP BY points.point_num,points.airp "
     "UNION "
     "SELECT 1, "
     "       'JMP', "
     "       jmp_cfg, "
     "       TO_NUMBER(NULL), "
     "       TO_NUMBER(NULL), "
     "       TO_NUMBER(NULL), "
     "       jmp_nooccupy, "
     "       TO_NUMBER(NULL) "
     "FROM counters2 c, trip_sets "
     "WHERE c.point_dep=trip_sets.point_id AND "
     "      c.point_dep=:point_id AND trip_sets.use_jmp<>0 AND "
     "      rownum<2 "
     "ORDER BY 1";
  Qry.CreateVariable( "point_id", otInteger, point_id );
  Qry.CreateVariable( "cfg_exists", otInteger, (int)cfg_exists );
  Qry.CreateVariable( "free_seating", otInteger, (int)free_seating );
  Qry.Execute();

  xmlNodePtr node = NewTextChild( dataNode, "tripcounters" );
  while ( !Qry.Eof )
  {
    int num=Qry.FieldAsInteger( "num" );
    string firstcol=Qry.FieldAsString( "firstcol" );
    xmlNodePtr itemNode = NewTextChild( node, "item" );
    if ( num>=-10 && num < 0 ) // классы
      NewTextChild( itemNode, "firstcol", ElemIdToCodeNative(etClass, firstcol) );
    else // аэропорты
      if (num >= 100)
        NewTextChild( itemNode, "firstcol", ElemIdToCodeNative(etAirp, firstcol) );
      else
        NewTextChild( itemNode, "firstcol", getLocaleText(firstcol) );

    if (!Qry.FieldIsNULL( "cfg" ))
      NewTextChild( itemNode, "cfg", Qry.FieldAsInteger( "cfg" ) );
    else
      NewTextChild( itemNode, "cfg" );
    if (!Qry.FieldIsNULL( "resa" ))
      NewTextChild( itemNode, "resa", Qry.FieldAsInteger( "resa" ) );
    else
      NewTextChild( itemNode, "resa" );
    if (!Qry.FieldIsNULL( "tranzit" ))
      NewTextChild( itemNode, "tranzit", Qry.FieldAsInteger( "tranzit" ) );
    else
      NewTextChild( itemNode, "tranzit" );
    if (!Qry.FieldIsNULL( "block" ))
      NewTextChild( itemNode, "block", Qry.FieldAsInteger( "block" ) );
    else
      NewTextChild( itemNode, "block" );
    if (!Qry.FieldIsNULL( "avail" ))
      NewTextChild( itemNode, "avail", Qry.FieldAsInteger( "avail" ) );
    else
      NewTextChild( itemNode, "avail" );
    if (!Qry.FieldIsNULL( "prot" ))
      NewTextChild( itemNode, "prot", Qry.FieldAsInteger( "prot" ) );
    else
      NewTextChild( itemNode, "prot" );
    Qry.Next();
  }
}

namespace {

struct SenderPriority
{
  CrsSender_t sender;
  CrsPriority_t priority;
};

struct SenderInfo
{
  std::optional<CrsSender_t> sender;
  std::string name;
  bool is_charge = false;
  bool is_list = false;
  bool is_crs_main = false;
};

int countTripData(const PointId_t& point_id)
{
  LogTrace(TRACE6) << __func__
                   << ": point_id=" << point_id;
  int result = 0;
  auto cur = make_db_curs(
        "SELECT count(*) FROM trip_data "
        "WHERE point_id=:point_id ",
        PgOra::getROSession("TRIP_DATA"));
  cur.stb()
      .def(result)
      .bind(":point_id", point_id.get())
      .EXfet();

  return result;
}

std::vector<SenderPriority> getSenderPriorityList(const std::string& airline,
                                                  int flt_no,
                                                  const std::string& airp_dep)
{
  std::vector<SenderPriority> result;
  std::string sender;
  int priority = ASTRA::NoExists;
  auto cur = make_db_curs(
        "SELECT crs, priority FROM crs_set "
        "WHERE airline=:airline "
        "AND (flt_no=:flt_no OR flt_no IS NULL) "
        "AND (airp_dep=:airp_dep OR airp_dep IS NULL) "
        "ORDER BY flt_no,airp_dep ",
        PgOra::getROSession("CRS_SET"));
  cur.stb()
      .def(sender)
      .def(priority)
      .bind(":airline", airline)
      .bind(":flt_no", flt_no)
      .bind(":airp_dep", airp_dep)
      .exec();
  while (!cur.fen()) {
    if (!CrsSender_t::validate(sender)){
      continue;
    }
    const SenderPriority senderPriority = {
      CrsSender_t(sender),
      CrsPriority_t(priority)
    };
    result.push_back(senderPriority);
  }
  return result;
}

std::set<CrsSender_t> getCrsSetSenders(const std::vector<SenderPriority>& items)
{
  std::set<CrsSender_t> result;
  for (const SenderPriority& item: items) {
    result.insert(item.sender);
  }
  return result;
}

std::map<CrsSender_t, CrsPriority_t> getCrsPriority(const std::vector<SenderPriority>& items)
{
  std::map<CrsSender_t, CrsPriority_t> result;
  for (const SenderPriority& item: items) {
    if (result.find(item.sender) != result.end()) {
      continue;
    }
    result.insert(std::make_pair(item.sender, item.priority));
  }
  return result;
}

std::optional<SenderPriority> getMaxCrsPriority(const std::map<CrsSender_t, CrsPriority_t>& items)
{
  std::optional<SenderPriority> max;
  for (auto item: items) {
    const CrsSender_t& sender = item.first;
    const CrsPriority_t& priority = item.second;
    if (!max) {
      max = { sender, priority };
    } else {
      if (priority > max->priority) {
        max = { sender, priority };
      }
    }
  }
  return max;
}

std::set<CrsSender_t> getCrsDataSenders(const std::set<PointIdTlg_t>& point_id_tlg_set,
                                        const std::string& system = "CRS")
{
  std::set<CrsSender_t> result;
  for (const PointIdTlg_t& point_id_tlg: point_id_tlg_set) {
    std::string sender;
    auto cur = make_db_curs(
          "SELECT DISTINCT sender FROM crs_data "
          "WHERE point_id=:point_id_tlg "
          "AND system=:system ",
          PgOra::getROSession("CRS_DATA"));
    cur.stb()
        .def(sender)
        .bind(":point_id_tlg", point_id_tlg.get())
        .bind(":system", system)
        .exec();
    while (!cur.fen()) {
      if (!CrsSender_t::validate(sender)) {
        continue;
      }
      result.emplace(sender);
    }
  }
  return result;
}

std::set<CrsSender_t> getCrsPnrSenders(const std::set<PointIdTlg_t>& point_id_tlg_set,
                                        const std::string& system = "CRS")
{
  std::set<CrsSender_t> result;
  for (const PointIdTlg_t& point_id_tlg: point_id_tlg_set) {
    std::string sender;
    auto cur = make_db_curs(
          "SELECT DISTINCT sender FROM crs_pnr "
          "WHERE point_id=:point_id_tlg "
          "AND system=:system ",
          PgOra::getROSession("CRS_PNR"));
    cur.stb()
        .def(sender)
        .bind(":point_id_tlg", point_id_tlg.get())
        .bind(":system", system)
        .exec();
    while (!cur.fen()) {
      if (!CrsSender_t::validate(sender)) {
        continue;
      }
      result.emplace(sender);
    }
  }
  return result;
}

std::vector<SenderInfo> getSenderInfoList(const std::string& airline,
                                          int flt_no,
                                          const std::string& airp_dep,
                                          const PointId_t& point_id,
                                          const std::set<PointIdTlg_t>& point_id_tlg_set,
                                          std::optional<CrsPriority_t>& max_priority)
{
  const std::vector<SenderPriority> sender_priority_list = getSenderPriorityList(airline,
                                                                                 flt_no,
                                                                                 airp_dep);
  const std::set<CrsSender_t> crs_set_senders = getCrsSetSenders(sender_priority_list);
  const std::set<CrsSender_t> crs_data_senders = getCrsDataSenders(point_id_tlg_set, "CRS");
  const std::set<CrsSender_t> crs_pnr_senders  = getCrsPnrSenders(point_id_tlg_set, "CRS");

  const std::map<CrsSender_t, CrsPriority_t> sender_priority_map = getCrsPriority(sender_priority_list);
  const std::optional<SenderPriority> sender_max_priority = getMaxCrsPriority(sender_priority_map);
  if (sender_max_priority) {
    max_priority = sender_max_priority->priority;
  }

  const bool is_charge = countTripData(point_id) > 0;
  const SenderInfo common_data = {
    std::nullopt,
    getLocaleText(std::string("Общие данные")),
    is_charge,
    false, /*is_list*/
    false  /*is_crs_main*/
  };
  std::vector<SenderInfo> result = { common_data };
  for (const CrsSender_t& sender: crs_set_senders) {
    const bool is_charge = crs_data_senders.find(sender) != crs_data_senders.end();
    const bool is_list = crs_pnr_senders.find(sender) != crs_pnr_senders.end();
    bool is_crs_main = false;
    if (max_priority && max_priority->get() != 0) {
      auto sender_priority_pos = sender_priority_map.find(sender);
      if (sender_priority_pos != sender_priority_map.end()) {
        const CrsPriority_t& priority = sender_priority_pos->second;
        is_crs_main = (priority == *max_priority);
      }
    }
    SenderInfo info = {
      sender,
      ElemIdToNameLong(etTypeBSender,sender.get()),
      is_charge,
      is_list,
      is_crs_main
    };
    result.push_back(info);
  }
  return result;
}

TAdvTripInfo loadFltInfo(const PointId_t& point_id)
{
  DB::TQuery Qry(PgOra::getROSession("POINTS"), STDLOG);
  Qry.SQLText =
    "SELECT airline, flt_no, suffix, airp, scd_out, "
    "       point_id, point_num, first_point, pr_tranzit "
    "FROM points WHERE point_id=:point_id AND pr_del=0 AND pr_reg<>0";
  Qry.CreateVariable( "point_id", otInteger, point_id.get() );
  Qry.Execute();
  if (Qry.Eof) throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");
  return TAdvTripInfo(Qry);
}

std::map<AirportCode_t, int> loadAirportPointNumMap(int point_num, int first_point)
{
  std::map<AirportCode_t, int> result;
  std::string airp;
  int min_point_num = ASTRA::NoExists;
  auto cur = make_db_curs(
        "SELECT airp, MIN(point_num) AS point_num "
        "FROM points "
        "WHERE first_point=:first_point "
        "AND point_num>:point_num "
        "AND pr_del=0 "
        "GROUP BY airp ",
        PgOra::getROSession("POINTS"));
  cur.stb()
      .def(airp)
      .def(min_point_num)
      .bind(":first_point", first_point)
      .bind(":point_num", point_num)
      .exec();
  while (!cur.fen()) {
    result.insert(std::make_pair(AirportCode_t(airp), min_point_num));
  }
  return result;
}

struct CrsDataNumKey: public CrsDataKey
{
  int point_num;

  bool operator < (const CrsDataNumKey& key) const;
};

bool CrsDataNumKey::operator < (const CrsDataNumKey &key) const
{
  if (sender != key.sender) {
    return sender < key.sender;
  }
  if (point_num != key.point_num) {
    if (key.point_num == ASTRA::NoExists) {
      return true;
    }
    if (point_num == ASTRA::NoExists) {
      return false;
    }
    return point_num < key.point_num;
  }
  if (airp_arv != key.airp_arv) {
    return airp_arv < key.airp_arv;
  }
  return cls < key.cls;
}

struct CrsDataSegKey
{
  AirportCode_t airp_arv;
  Class_t cls;
  CrsPriority_t priority;

  bool operator < (const CrsDataSegKey& key) const;
};

bool CrsDataSegKey::operator < (const CrsDataSegKey &key) const
{
  if (airp_arv != key.airp_arv) {
    return airp_arv < key.airp_arv;
  }
  if (cls != key.cls) {
    return cls < key.cls;
  }
  return priority > key.priority;
}

std::map<CrsDataNumKey, CrsDataValue> getCrsDataGroupBySender(const TAdvTripInfo& fltInfo,
                                                              const std::set<PointIdTlg_t>& point_id_tlg_set)
{
  const int first_point = fltInfo.pr_tranzit ? fltInfo.first_point : fltInfo.point_id;
  const std::map<AirportCode_t, int> min_point_num_map
      = loadAirportPointNumMap(fltInfo.point_num, first_point);
  const std::vector<CrsData> crs_data_list = loadCrsData(point_id_tlg_set, "CRS",
                                                         std::nullopt /*by*/,
                                                         fltInfo.airp /*except*/,
                                                         true /*skipNullSums*/);
  std::map<CrsDataNumKey, CrsDataValue> result;
  for (const CrsData& crs_data: crs_data_list) {
    const CrsDataKey& key = crs_data.first;
    const CrsDataValue& value = crs_data.second;
    int point_num = ASTRA::NoExists;
    for (const auto& item: min_point_num_map) {
      const AirportCode_t& airport = item.first;
      if (key.airp_arv == airport) {
        point_num = item.second;
        break;
      }
    }
    CrsDataNumKey groupKey = {
      key.sender,
      key.airp_arv,
      key.cls,
      point_num
    };
    auto it = result.find(groupKey);
    if (it == result.end()) {
      result.insert(std::make_pair(groupKey, value));
    } else {
      sum_nullable(it->second.resa, value.resa);
      sum_nullable(it->second.tranzit, value.tranzit);
    }
  }
  return result;
}

std::map<CrsDataSegKey, CrsDataValue> loadTripData(const PointId_t& point_id)
{
  std::map<CrsDataSegKey, CrsDataValue> result;
  DB::TQuery Qry(PgOra::getROSession("TRIP_DATA"));
  Qry.Clear();
  Qry.SQLText =
      "SELECT airp_arv,class,resa,tranzit "
      "FROM trip_data "
      "WHERE point_id=:point_id "
      "ORDER BY airp_arv,class ";
  Qry.CreateVariable( "point_id", otInteger, point_id.get() );
  Qry.Execute();
  while (!Qry.Eof) {
    CrsDataSegKey key = {
      AirportCode_t(Qry.FieldAsString("airp_arv")),
      Class_t(Qry.FieldAsString("class")),
      CrsPriority_t(1)
    };
    CrsDataValue value = {
      Qry.FieldAsInteger("tranzit"),
      Qry.FieldAsInteger("resa"),
      ASTRA::NoExists
    };
    result.insert(std::make_pair(key, value));
    Qry.Next();
  }
  return result;
}

std::map<CrsDataSegKey, CrsDataValue> getCrsDataTotal(const TAdvTripInfo& fltInfo,
                                                      const PointId_t& point_id,
                                                      const std::set<PointIdTlg_t>& point_id_tlg_set,
                                                      const std::optional<CrsPriority_t>& max_priority)
{
  std::map<CrsDataSegKey, CrsDataValue> result = loadTripData(point_id);
  const std::vector<CrsData> crs_data_list = loadCrsData(point_id_tlg_set, "CRS",
                                                         std::nullopt /*by*/,
                                                         std::nullopt /*except*/,
                                                         false /*skipNullSums*/);
  for (const CrsData& crs_data: crs_data_list) {
    const CrsDataKey& key = crs_data.first;
    const CrsDataValue& value = crs_data.second;
    if (max_priority && max_priority->get() != 0) {
      std::optional<CrsPriority_t> priority
          = CheckIn::getCrsPriority(key.sender,
                                    AirlineCode_t(fltInfo.airline),
                                    fltInfo.flt_no,
                                    AirportCode_t(fltInfo.airp));
      if (!priority) {
        priority = CrsPriority_t(0);
      }
      if (priority != max_priority) {
        continue;
      }
    }

    CrsDataSegKey groupKey = {
      key.airp_arv,
      key.cls,
      CrsPriority_t(0)
    };
    auto it = result.find(groupKey);
    if (it == result.end()) {
      result.insert(std::make_pair(groupKey, value));
    } else {
      sum_nullable(it->second.resa, value.resa);
      sum_nullable(it->second.tranzit, value.tranzit);
    }
  }
  return result;
}

} // namespace

void makeNodeAirps(xmlNodePtr tripdataNode, const TAdvTripInfo& fltInfo)
{
  xmlNodePtr node = NewTextChild( tripdataNode, "airps" );
  TTripRoute route;
  route.GetRouteAfter( NoExists,
                       fltInfo.point_id,
                       fltInfo.point_num,
                       fltInfo.first_point,
                       fltInfo.pr_tranzit,
                       trtNotCurrent,
                       trtNotCancelled );
  for(TTripRoute::const_iterator r=route.begin(); r!=route.end(); ++r) {
    NewTextChild( node, "airp", r->airp );
  }
}

void makeNodeClasses(xmlNodePtr tripdataNode, const PointId_t& point_id)
{
  xmlNodePtr node;
  node = NewTextChild( tripdataNode, "classes" );
  TCFG cfg(point_id.get());
  for(TCFG::const_iterator c=cfg.begin(); c!=cfg.end(); ++c) {
    NewTextChild( node, "class", c->cls );
  }
}

void makeNodeCrs(xmlNodePtr tripdataNode, const TAdvTripInfo& fltInfo,
                 const PointId_t& point_id, const std::set<PointIdTlg_t>& point_id_tlg_set,
                 std::optional<CrsPriority_t>& max_priority)
{
  const std::vector<SenderInfo> sender_info_list = getSenderInfoList(fltInfo.airline,
                                                                     fltInfo.flt_no,
                                                                     fltInfo.airp,
                                                                     point_id,
                                                                     point_id_tlg_set,
                                                                     max_priority);

  xmlNodePtr node;
  node = NewTextChild( tripdataNode, "crs" );
  for (const SenderInfo& sender_info: sender_info_list) {
    xmlNodePtr itemNode = NewTextChild( node, "itemcrs" );
    NewTextChild( itemNode, "code", (sender_info.sender ? sender_info.sender->get() : "") );
    NewTextChild( itemNode, "name", sender_info.name );
    NewTextChild( itemNode, "pr_charge", sender_info.is_charge );
    NewTextChild( itemNode, "pr_list", sender_info.is_list );
    NewTextChild( itemNode, "pr_crs_main", sender_info.is_crs_main );
  }
}

void makeNodeCrsData(xmlNodePtr tripdataNode, const TAdvTripInfo& fltInfo,
                     const PointId_t& point_id,
                     const std::set<PointIdTlg_t>& point_id_tlg_set,
                     const std::optional<CrsPriority_t>& max_priority)
{
  const std::map<CrsDataNumKey, CrsDataValue> crs_data_sender
      = getCrsDataGroupBySender(fltInfo, point_id_tlg_set);

  xmlNodePtr node;
  node = NewTextChild( tripdataNode, "crsdata" );
  for (auto &crs_data: crs_data_sender) {
    const CrsDataNumKey& key = crs_data.first;
    const CrsDataValue& value = crs_data.second;
    xmlNodePtr itemNode = NewTextChild( node, "itemcrs" );
    NewTextChild( itemNode, "crs", key.sender.get() );
    NewTextChild( itemNode, "target", key.airp_arv.get() );
    NewTextChild( itemNode, "class", key.cls.get() );
    NewTextChild( itemNode, "resa", value.resa == ASTRA::NoExists ? -1 : value.resa );
    NewTextChild( itemNode, "tranzit", value.tranzit == ASTRA::NoExists ? -1 : value.tranzit );
  }

  std::map<CrsDataSegKey, CrsDataValue> crs_data_total = getCrsDataTotal(fltInfo,
                                                                         point_id,
                                                                         point_id_tlg_set,
                                                                         max_priority);
  for (auto &crs_data: crs_data_total) {
    const CrsDataSegKey& key = crs_data.first;
    const CrsDataValue& value = crs_data.second;
    xmlNodePtr itemNode = NewTextChild( node, "itemcrs" );
    NewTextChild( itemNode, "crs" );
    NewTextChild( itemNode, "target", key.airp_arv.get() );
    NewTextChild( itemNode, "class", key.cls.get() );
    NewTextChild( itemNode, "resa", value.resa == ASTRA::NoExists ? 0 : value.resa );
    NewTextChild( itemNode, "tranzit", value.tranzit == ASTRA::NoExists ? 0 : value.tranzit );
  }
}

void PrepRegInterface::readTripData( int point_id, xmlNodePtr dataNode )
{
  ProgTrace(TRACE5, "PrepRegInterface::readTripData" );
  xmlNodePtr tripdataNode = NewTextChild( dataNode, "tripdata" );
  const TAdvTripInfo fltInfo = loadFltInfo(PointId_t(point_id));

  makeNodeAirps(tripdataNode, fltInfo);
  makeNodeClasses(tripdataNode, PointId_t(point_id));

  const std::set<PointIdTlg_t> point_id_tlg_set = getPointIdTlgByPointIdsSpp(PointId_t(point_id));
  std::optional<CrsPriority_t> max_priority;
  makeNodeCrs(tripdataNode, fltInfo, PointId_t(point_id), point_id_tlg_set, max_priority);
  makeNodeCrsData(tripdataNode, fltInfo, PointId_t(point_id), point_id_tlg_set, max_priority);
}

void CheckJMPCount(int point_id, int jmp_cfg)
{
  TQuery Qry( &OraSession );
  Qry.Clear();
  Qry.SQLText="SELECT SUM(jmp_tranzit)+SUM(jmp_ok)+SUM(jmp_goshow) AS jmp_show "
              "FROM counters2 "
              "WHERE point_dep=:point_id";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  if (Qry.Eof) return;
  int jmp_show=Qry.FieldAsInteger("jmp_show");
  if (jmp_show>jmp_cfg)
  {
    if (jmp_cfg>0)
      throw UserException("MSG.NEED_TO_CANCEL_CKIN_FOR_PAX_ON_JUMP_SEATS");
    else
      throw UserException("MSG.NEED_TO_CANCEL_CKIN_FOR_ALL_PAX_ON_JUMP_SEATS");
  };
}

bool updateTripData(const PointId_t& point_id, const std::string& airp_arv,
                    const std::string& cls, int resa, int tranzit)
{
  LogTrace(TRACE6) << __func__
                   << ": point_id=" << point_id
                   << ", airp_arv=" << airp_arv
                   << ", cls=" << cls
                   << ", resa=" << resa
                   << ", tranzit=" << tranzit;
  auto cur = make_db_curs(
        "UPDATE trip_data "
        "SET resa= :resa, tranzit= :tranzit "
        "WHERE point_id=:point_id "
        "AND airp_arv=:airp_arv "
        "AND class=:class ",
        PgOra::getRWSession("TRIP_DATA"));
  cur.stb()
      .bind(":point_id", point_id.get())
      .bind(":airp_arv", airp_arv)
      .bind(":class", cls)
      .bind(":resa",resa)
      .bind(":tranzit",tranzit)
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

bool insertTripData(const PointId_t& point_id, const std::string& airp_arv,
                    const std::string& cls, int resa, int tranzit)
{
  LogTrace(TRACE6) << __func__
                   << ": point_id=" << point_id
                   << ", airp_arv=" << airp_arv
                   << ", cls=" << cls
                   << ", resa=" << resa
                   << ", tranzit=" << tranzit;
  auto cur = make_db_curs(
        "INSERT INTO trip_data( "
        "point_id,airp_arv,class,resa,tranzit,avail "
        ") VALUES ( "
        ":point_id,:airp_arv,:class,:resa,:tranzit,NULL "
        ") ",
        PgOra::getRWSession("TRIP_DATA"));
  cur.stb()
      .bind(":point_id", point_id.get())
      .bind(":airp_arv", airp_arv)
      .bind(":class", cls)
      .bind(":resa",resa)
      .bind(":tranzit",tranzit)
      .exec();

  LogTrace(TRACE6) << __func__
                   << ": rowcount=" << cur.rowcount();
  return cur.rowcount() > 0;
}

void saveTripData(const PointId_t& point_id, const std::string& airp_arv,
                  const std::string& cls, int resa, int tranzit)
{
  const bool updated = updateTripData(point_id, airp_arv, cls, resa, tranzit);
  if (!updated) {
    insertTripData(point_id, airp_arv, cls, resa, tranzit);
  }
}

void PrepRegInterface::CrsDataApplyUpdates(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id = NodeAsInteger( "point_id", reqNode );
  bool question = NodeAsInteger( "question", reqNode, 0 );
  ProgTrace(TRACE5, "TripInfoInterface::CrsDataApplyUpdates, point_id=%d", point_id );
  //TReqInfo::Instance()->user.check_access( amWrite );

  //лочим рейс - весь маршрут, т.к. pr_tranzit может поменяться
  TFlights flights;
  flights.Get( point_id, ftAll );
  flights.Lock(__FUNCTION__);

  xmlNodePtr node = GetNode( "crsdata", reqNode );
  if ( node != NULL )
  {
    string airp_arv, cl;
    int resa, tranzit;
    node = node->children;
    while ( node !=NULL ) {
      xmlNodePtr snode = node->children;
      airp_arv = NodeAsStringFast( "target", snode );
      cl = NodeAsStringFast( "class", snode );
      resa = NodeAsIntegerFast( "resa", snode );
      if (resa<0) resa=0;
      tranzit = NodeAsIntegerFast( "tranzit", snode );
      if (tranzit<0) tranzit=0;
      saveTripData(PointId_t(point_id), airp_arv, cl, resa, tranzit);
      TReqInfo::Instance()->LocaleToLog("EVT.SALE_CHANGED", LEvntPrms() << PrmElem<std::string>("airp", etAirp, airp_arv)
                                        << PrmElem<std::string>("cl", etClass, cl) << PrmSmpl<int>("resa", resa)
                                        << PrmSmpl<int>("tranzit", tranzit), evtFlt, point_id );
      node = node->next;
    };
    ComponCreator::AutoSetCraft( point_id );
  };
  bool pr_check_trip_tasks = false;

  node = GetNode( "trip_sets", reqNode );
  if ( node != NULL )
  {
    TAdvTripInfo flt;
    if (!flt.getByPointId(point_id, FlightProps(FlightProps::NotCancelled,
                                                FlightProps::WithCheckIn)))
      throw AstraLocale::UserException("MSG.FLIGHT.NOT_FOUND.REFRESH_DATA");
    bool transitable=flt.transitable();
    bool new_pr_tranzit=NodeAsInteger("pr_tranzit",node)!=0;

    TTripSetList oldSetList, newSetList;
    oldSetList.fromDB(point_id);
    if (oldSetList.empty()) throw Exception("Flight not found in trip_sets (point_id=%d)",point_id);

    newSetList.fromXML(node, new_pr_tranzit, oldSetList);

    vector<int> check_waitlist_alarms, check_diffcomp_alarms;
    bool pr_isTranzitSalons = (flt.pr_tranzit!=new_pr_tranzit ||
                               oldSetList.value<bool>(tsTransitReg)      != newSetList.value<bool>(tsTransitReg) ||
                               oldSetList.value<bool>(tsTransitBlocking) != newSetList.value<bool>(tsTransitBlocking) ||
                               oldSetList.value<bool>(tsFreeSeating)     != newSetList.value<bool>(tsFreeSeating));

    if (flt.pr_tranzit!=new_pr_tranzit ||
        oldSetList.value<bool>(tsTransitReg)!=newSetList.value<bool>(tsTransitReg) ||
        oldSetList.value<bool>(tsTransitBlocking)!=newSetList.value<bool>(tsTransitBlocking))
    {
      if (transitable)
      {
        if (oldSetList.value<bool>(tsTransitReg) && !newSetList.value<bool>(tsTransitReg)) // отмена перерегистрации транзита
        {
          TQuery DelQry( &OraSession );
          DelQry.SQLText =
              "SELECT grp_id  FROM pax_grp,points "
              " WHERE points.point_id=:point_id AND "
              "       point_dep=:point_id AND pax_grp.status NOT IN ('E') AND "
              "       bag_refuse=0 AND status=:status AND rownum<2 ";
          DelQry.CreateVariable( "point_id", otInteger, point_id );
          DelQry.CreateVariable( "status", otString, EncodePaxStatus( psTransit ) );
          DelQry.Execute();
          if ( !DelQry.Eof ) {
            ProgTrace( TRACE5, "question=%d", question );
            if ( question ) {
              xmlNodePtr dataNode = NewTextChild( resNode, "data" );
              NewTextChild( dataNode, "question", getLocaleText("QST.TRANZIT_RECHECKIN_CAUTION.CANCEL") );
              return;
            }
            map<int,TAdvTripInfo> segs; // набор рейсов
            TDeletePaxFilter filter;
            filter.status=EncodePaxStatus( psTransit );
            DeletePassengers( point_id, filter, segs );
            DeletePassengersAnswer( segs, resNode );
          }
        }

        updateTransitIfNeeded(flt, new_pr_tranzit);
        if (flt.pr_tranzit!=new_pr_tranzit)
          pr_check_trip_tasks = true;

        check_diffcomp_alarms.push_back( point_id );
        if ( !pr_isTranzitSalons ) {
          check_waitlist_alarms.push_back( point_id );
        }
      }
      if ( pr_isTranzitSalons ) {
        check_waitlist_alarms.push_back( point_id );
      }
    }

    if (oldSetList.value<bool>(tsUseJmp)!=newSetList.value<bool>(tsUseJmp) ||
        oldSetList.value<int>(tsJmpCfg)!=newSetList.value<int>(tsJmpCfg))
    {
      //проверим что кол-во зарегистрированных JMP меньше или равно кол-ву мест JMP
      CheckJMPCount(point_id, newSetList.value<bool>(tsUseJmp)?newSetList.value<int>(tsJmpCfg):0);
    }

    TTripSetList differSetList;
    set_difference(newSetList.begin(), newSetList.end(),
                   oldSetList.begin(), oldSetList.end(),
                   inserter(differSetList, differSetList.end()), TTripSetListItemLess);
    differSetList.toDB(point_id);

    if (oldSetList.value<bool>(tsFreeSeating)!=newSetList.value<bool>(tsFreeSeating))
    {
      if ( newSetList.value<bool>(tsFreeSeating) ) {
        SALONS2::DeleteSalons( point_id );
      }
      check_diffcomp_alarms.push_back( point_id );
      check_waitlist_alarms.push_back( point_id );
    }

    for ( vector<int>::iterator ipoint_id=check_diffcomp_alarms.begin();
          ipoint_id!=check_diffcomp_alarms.end(); ipoint_id++ ) {
      ComponCreator::check_diffcomp_alarm( *ipoint_id );
    }
    if ( !check_waitlist_alarms.empty() ) {
      if ( pr_isTranzitSalons ) {
        SALONS2::check_waitlist_alarm_on_tranzit_routes( check_waitlist_alarms, __FUNCTION__ );
      }
      else {
        for ( vector<int>::iterator ipoint_id=check_waitlist_alarms.begin();
              ipoint_id!=check_waitlist_alarms.end(); ipoint_id++ ) {
          check_waitlist_alarm( *ipoint_id );
        }
      }
    }
    if (oldSetList.value<bool>(tsAPISControl)!=newSetList.value<bool>(tsAPISControl))
    {
      check_apis_alarms(point_id);
    }
    if (oldSetList.value<bool>(tsAPISManualInput)!=newSetList.value<bool>(tsAPISManualInput))
    {
      set<Alarm::Enum> checked_alarms;
      checked_alarms.insert(Alarm::APISManualInput);
      check_apis_alarms(point_id, checked_alarms);
    }
  };

  if ( pr_check_trip_tasks ) {
    Qry.Clear();
    Qry.SQLText =
      "SELECT move_id FROM points WHERE point_id=:point_id";
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.Execute();
   try {
    check_trip_tasks( Qry.FieldAsInteger( "move_id" ) );
    }
    catch(std::exception &E) {
      ProgError(STDLOG,"CrsDataApplyUpdates.check_trip_tasks (move_id=%d): %s",Qry.FieldAsInteger( "move_id" ),E.what());
    };
  }
  on_change_trip( CALL_POINT, point_id, ChangeTrip::CrsDataApplyUpdates );

  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  if ( GetNode( "tripcounters", reqNode ) ) {
    readTripCounters( point_id, dataNode );
  }
}

void PrepRegInterface::ViewCRSList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  int point_id = NodeAsInteger( "point_id", reqNode );
  //TReqInfo::Instance()->user.check_access( amRead );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  viewCRSList( point_id, {}, dataNode );
  get_compatible_report_form("PNLPaxList", reqNode, resNode);
  xmlNodePtr formDataNode = STAT::set_variables(resNode);
  TRptParams rpt_params(TReqInfo::Instance()->desk.lang);
  PaxListVars(point_id, rpt_params, formDataNode);
  string real_out = NodeAsString("real_out", formDataNode);
  string scd_out = NodeAsString("scd_out", formDataNode);
  string date = real_out + (real_out == scd_out ? "" : "(" + scd_out + ")");
  NewTextChild(formDataNode, "caption", getLocaleText("CAP.DOC.PNL_PAX_LIST",
              LParams() << LParam("trip", NodeAsString("trip", formDataNode))
                  << LParam("date", date)
              ));
}

static bool updateCrsCabinClass(const CheckIn::TSimplePaxItem& pax,
                                const PointId_t& pointId,
                                xmlNodePtr reqNode)
{
  xmlNodePtr valueNode=GetNode("class/@edit_value", reqNode);
  if (valueNode==nullptr) return false;

  if (!TReqInfo::Instance()->user.access.rights().permitted(196))
    throw UserException("MSG.NO_ACCESS");

  std::string cabinClass;
  std::string value=NodeAsString(valueNode);
  TrimString(value);
  if (!value.empty())
  {
    TElemFmt fmt;
    cabinClass=ElemToElemId(etClass, value, fmt);
    if (cabinClass.empty())
      throw UserException("MSG.TABLE.INVALID_FIELD_VALUE",
                          LParams() << LParam("fieldname", getLocaleText("CAP.PNL_EDIT_FORM.CABIN_CLASS")));
  }

  auto upd=make_curs("UPDATE crs_pax "
                     "SET cabin_class=:cabin_class "
                     "WHERE pax_id=:pax_id AND pr_del=0");

  upd.bind(":cabin_class", cabinClass)
     .bind(":pax_id", pax.id)
     .exec();

  if (upd.rowcount()>0)
  {
    if (!cabinClass.empty())
      TReqInfo::Instance()->LocaleToLog("EVT.PRELIMINARY_ASSIGNED_CABIN_CLASS",
                                        LEvntPrms() << PrmSmpl<string>("pax_name", pax.full_name())
                                                    << PrmElem<string>("type", etPersType, EncodePerson(pax.pers_type))
                                                    << PrmElem<string>("cls", etClass, cabinClass),
                                        evtPax,
                                        pointId.get());
    else
      TReqInfo::Instance()->LocaleToLog("EVT.CANCEL_PRELIMINARY_ASSIGNED_CABIN_CLASS",
                                        LEvntPrms() << PrmSmpl<string>("pax_name", pax.full_name())
                                                    << PrmElem<string>("type", etPersType, EncodePerson(pax.pers_type)),
                                        evtPax,
                                        pointId.get());
  }

  return true;
}

static bool updateCrsBagNorm(const CheckIn::TSimplePaxItem& pax,
                             const PointId_t& pointId,
                             xmlNodePtr reqNode)
{
  xmlNodePtr valueNode=GetNode("bag_norm/@edit_value", reqNode);
  if (valueNode==nullptr) return false;

  if (!TReqInfo::Instance()->user.access.rights().permitted(195))
    throw UserException("MSG.NO_ACCESS");

  boost::optional<TBagKilos> bagNorm;
  std::string value=NodeAsString(valueNode);
  TrimString(value);
  if (!value.empty())
  {
    int quantity;
    if (StrToInt(value.c_str(), quantity)==EOF)
      throw UserException("MSG.TABLE.INVALID_FIELD_VALUE",
                          LParams() << LParam("fieldname", getLocaleText("CAP.PNL_EDIT_FORM.BAG_NORM")));
    bagNorm=boost::in_place(quantity);
  }

  if (bagNorm)
  {
    CheckIn::TPaxTknItem tkn;
    CheckIn::LoadCrsPaxTkn(pax.id, tkn);
    if (tkn.validET())
    {
      TETickItem etick;
      etick.fromDB(tkn.no, tkn.coupon, TETickItem::Display, false);
      if (etick.bagNorm && etick.bagNorm.get().getUnit()==Ticketing::Baggage::NumPieces)
        throw UserException("MSG.PAX_HAS_BAG_NORM_FOR_PIECE_CONCEPT.CHANGE_DENIED");
    }
  }

  auto upd=make_curs("UPDATE crs_pax "
                     "SET bag_norm=:bag_norm, bag_norm_unit=:bag_norm_unit "
                     "WHERE pax_id=:pax_id AND pr_del=0");

  if (bagNorm)
  {
    upd.bind(":bag_norm", bagNorm.get().getQuantity())
       .bind(":bag_norm_unit", TBagUnit(bagNorm.get().getUnit()).get_db_form())
       .bind(":pax_id", pax.id)
       .exec();

    if (upd.rowcount()>0)
    {
      PrmEnum norm("norm", "");
      norm.prms << PrmSmpl<int>("", bagNorm.get().getQuantity())
                << PrmLexema("", TBagUnit(bagNorm.get().getUnit()).get_events_form());
      TReqInfo::Instance()->LocaleToLog("EVT.PRELIMINARY_ASSIGNED_BAG_NORM",
                                        LEvntPrms() << PrmSmpl<string>("pax_name", pax.full_name())
                                                    << PrmElem<string>("type", etPersType, EncodePerson(pax.pers_type))
                                                    << norm,
                                        evtPax,
                                        pointId.get());
    }
  }
  else
  {
    upd.bind(":bag_norm", "")
       .bind(":bag_norm_unit", "")
       .bind(":pax_id", pax.id)
       .exec();

    if (upd.rowcount()>0)
    {
      TReqInfo::Instance()->LocaleToLog("EVT.CANCEL_PRELIMINARY_ASSIGNED_BAG_NORM",
                                        LEvntPrms() << PrmSmpl<string>("pax_name", pax.full_name())
                                                    << PrmElem<string>("type", etPersType, EncodePerson(pax.pers_type)),
                                        evtPax,
                                        pointId.get());
    }
  }

  return true;
}

void PrepRegInterface::UpdateCRSList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  PointId_t pointId(NodeAsInteger("point_id", reqNode));
  PaxId_t paxId(NodeAsInteger("pax_id", reqNode));

  CheckIn::Search search;
  CheckIn::TSimplePaxList paxs;
  search(paxs, PaxIdFilter(paxId));
  if (paxs.empty()) throw UserException("MSG.PASSENGER.NOT_FOUND.REFRESH_DATA");

  const CheckIn::TSimplePaxItem& pax=paxs.front();
  if (pax.origin()==paxCheckIn) throw UserException("MSG.PAX_ALREADY_CHECKED_IN.CHANGE_DENIED");

  bool changesApplied=false;
  if (updateCrsBagNorm(pax, pointId, reqNode)) changesApplied=true;
  if (updateCrsCabinClass(pax, pointId, reqNode)) changesApplied=true;

  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  if (changesApplied)
  {
    viewCRSList( pointId.get(), paxId, dataNode );
    AstraLocale::showMessage( "MSG.CHANGED_DATA_COMMIT" );
  }
  else
  {
    NewTextChild( dataNode, "tlg_trips" );
  }
}

