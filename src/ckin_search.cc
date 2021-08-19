#include "ckin_search.h"

#include "astra_utils.h"
#include "date_time.h"
#include "tlg/typeb_db.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"
#include "serverlib/slogger.h"

using namespace std;
using namespace BASIC::date_time;
using namespace ASTRA;
using namespace CheckIn;

int PaxInfoForSearch::calcStagePriority(const TAdvTripInfo& flt)
{
  TStage ckinStage;
  if (flt.act_out_exists())
    ckinStage=sTakeoff;
  else
    ckinStage=TTripStages( flt.point_id ).getStage(stCheckIn);

  switch(ckinStage)
  {
    case sNoActive:      return 10;
    case sPrepCheckIn:   return 3;
    case sOpenCheckIn:   return 1;
    case sCloseCheckIn:  return 2;
    case sCloseBoarding: return 10;
    case sTakeoff:       return 10;
    default:             return 10;
  }
}

PaxInfoForSearch::PaxInfoForSearch(const TSimplePaxItem& _pax,
                                   const boost::optional<TAdvTripInfo>& _flt,
                                   const TDateTime& timePoint)
  : pax(_pax), flt(_flt)
{
  if (flt)
  {
    stagePriority=calcStagePriority(flt.get());
    if (flt.get().act_est_scd_out()!=ASTRA::NoExists)
      timeOutAbsDistance=abs(timePoint-flt.get().act_est_scd_out());
  }
}

bool PaxInfoForSearch::operator < (const PaxInfoForSearch &info) const
{
  if ((stagePriority==boost::none)!=(info.stagePriority==boost::none))
    return stagePriority!=boost::none;

  if (stagePriority && info.stagePriority &&
      stagePriority.get() != info.stagePriority.get())
    return stagePriority.get() < info.stagePriority.get();

  if ((timeOutAbsDistance==boost::none)!=(info.timeOutAbsDistance==boost::none))
    return timeOutAbsDistance!=boost::none;

  if (timeOutAbsDistance && info.timeOutAbsDistance &&
      timeOutAbsDistance.get() != info.timeOutAbsDistance.get())
    return timeOutAbsDistance.get() < info.timeOutAbsDistance.get();

  return pax.id<info.pax.id;
}

PaxInfoForSearchList::PaxInfoForSearchList(const TSimplePaxList& paxs)
{
  TDateTime nowUTC=NowUTC();
  for(const TSimplePaxItem& pax : paxs)
  {
    TAdvTripInfoList flts;
    if (pax.grp_id!=ASTRA::NoExists)
    {
      TAdvTripInfo flt;
      if (flt.getByGrpId(pax.grp_id)) flts.push_back(flt);
    }
    else getTripsByCRSPaxId(pax.id, flts);

    if (flts.empty())
      emplace(pax, boost::none, nowUTC);
    else
      for(const TAdvTripInfo& flt : flts)
        emplace(pax, flt, nowUTC);
  }
}

void PaxInfoForSearchList::trace() const
{
  for(const PaxInfoForSearch& info : *this)
    LogTrace(TRACE5) << "pax: id=" << info.pax.id
                     << " grp_id=" << (info.pax.grp_id!=ASTRA::NoExists?IntToString(info.pax.grp_id):"NoExists")
                     << "; stagePriority=" << (info.stagePriority?IntToString(info.stagePriority.get()):"boost::none")
                     << "; timeOutAbsDistance=" << (info.timeOutAbsDistance?FloatToString(info.timeOutAbsDistance.get()):"boost::none");
}

void ClassesList::add(const std::string& cl)
{
  if (cls.find(cl)==string::npos) cls.append(cl);
}

std::string ClassesList::view() const
{
  std::string result;
  for(const char& c : cls)
    result.append(ElemIdToCodeNative(etClass, std::string(1,c)));
  return result;
}

std::string ClassesList::getStrictlyOneClass() const
{
  return cls.size()==1?cls:"";
}

std::string getSearchPaxSubquery(const TPaxStatus& pax_status,
                                 const bool& return_pnr_ids,
                                 const bool& exclude_checked,
                                 const bool& exclude_deleted,
                                 const bool& select_pad_with_ok,
                                 const std::string& sql_filter,
                                 bool forOracle)
{
  ostringstream sql;
  string status_param;
  switch (pax_status)
  {
    case psTransit: status_param=":ps_transit"; break;
     case psGoshow: status_param=":ps_goshow";  break;
           default: status_param=":ps_ok";      break;
  }

  //2 прохода:
  for(int pass=1;pass<=2;pass++)
  {
    if (pass==2 && pax_status!=psGoshow) continue;
    if (pass==2)
      sql << "   UNION ";

    if (return_pnr_ids)
      sql << "   SELECT DISTINCT crs_pnr.pnr_id, " << status_param << " AS status ";
    else
      sql << "   SELECT DISTINCT crs_pax.pax_id, " << status_param << " AS status ";

    sql <<   "   FROM crs_pnr ";

    if (!return_pnr_ids || exclude_checked || exclude_deleted) {
      sql << "JOIN (crs_pax ";
      if (exclude_checked) {
        sql << "LEFT OUTER JOIN pax ON crs_pax.pax_id = pax.pax_id AND pax.pax_id IS NULL ";
      }
      sql << ") ON crs_pnr.pnr_id = crs_pax.pnr_id ";
    }

    sql <<   "   WHERE crs_pnr.point_id=:point_id_tlg AND "
             "         crs_pnr.system='CRS' AND "
             "         crs_pnr.airp_arv=:airp_arv_tlg AND "
             "         crs_pnr.class=:class_tlg ";

    if (pass==1)
      sql << "         AND :status= " << status_param << " ";

    if (pass==1 && pax_status==psCheckin && !select_pad_with_ok)
      sql << "         AND (crs_pnr.status IS NULL OR crs_pnr.status NOT IN ('DG2','RG2','ID2','WL')) ";

    if (pass==2 && pax_status==psGoshow)
      sql << "         AND :status= :ps_ok "
             "         AND crs_pnr.status IN ('DG2','RG2','ID2','WL') ";

    if (exclude_deleted)
      sql << "         AND crs_pax.pr_del=0 ";

    if (!sql_filter.empty())
      sql << "         AND ("+sql_filter+") ";

  }

  //2 прохода:
  for(int pass=1;pass<=2;pass++)
  {
    if ((pass==1 && pax_status!=psCheckin && pax_status!=psGoshow) ||
        (pass==2 && pax_status==psCheckin)) continue;
    if (pass==1)
      sql << "   UNION ";
    else
      sql << (forOracle ? "   MINUS " : "   EXCEPT ");

    if (return_pnr_ids)
      sql << "   SELECT DISTINCT crs_pnr.pnr_id, " << status_param << " AS status ";
    else
      sql << "   SELECT DISTINCT crs_pax.pax_id, " << status_param << " AS status ";

    sql <<   "   FROM crs_pnr "
          << "     JOIN tlg_binding ON crs_pnr.point_id=tlg_binding.point_id_tlg ";

    if (!return_pnr_ids || exclude_checked || exclude_deleted) {
      sql << "JOIN (crs_pax ";
      if (exclude_checked) {
        sql << "LEFT OUTER JOIN pax ON crs_pax.pax_id = pax.pax_id AND pax.pax_id IS NULL ";
      }
      sql << ") ON crs_pnr.pnr_id = crs_pax.pnr_id ";
    }

    sql   << "   WHERE crs_pnr.system='CRS' AND "
             "         tlg_binding.point_id_spp= :point_id ";

    if ((pass==1 && pax_status==psCheckin && !select_pad_with_ok) ||
        (pass==2 && pax_status==psGoshow))
      sql << "         AND (crs_pnr.status IS NULL OR crs_pnr.status NOT IN ('DG2','RG2','ID2','WL')) ";

    if (pass==1 && pax_status==psGoshow)
      sql << "         AND crs_pnr.status IN ('DG2','RG2','ID2','WL') ";

    if (exclude_deleted)
      sql << "         AND crs_pax.pr_del=0 ";

    if (!sql_filter.empty())
      sql << "         AND ("+sql_filter+") ";
  }

  return sql.str();
}

void bindSearchPaxQuery(DB::TQuery& Qry, const PointId_t& point_id, const TPaxStatus& pax_status)
{
  switch (pax_status)
  {
    case psTransit: Qry.CreateVariable( "ps_transit", otString, EncodePaxStatus(ASTRA::psTransit) );
                    break;
     case psGoshow: Qry.CreateVariable( "ps_goshow", otString, EncodePaxStatus(ASTRA::psGoshow) );
                    //break не надо!
                    [[fallthrough]];
           default: Qry.CreateVariable( "ps_ok", otString, EncodePaxStatus(ASTRA::psCheckin) );
  }
  Qry.CreateVariable("point_id", otInteger, point_id.get());
  Qry.CreateVariable("point_id_tlg", otInteger, FNull);
  Qry.CreateVariable("airp_arv_tlg", otString, FNull);
  Qry.CreateVariable("class_tlg", otString, FNull);
  Qry.CreateVariable("status", otString, FNull);
}

static const std::string& getSearchPaxQuerySelectPart()
{
  static const std::string result=
         "SELECT crs_pax.pax_id,crs_pnr.point_id,crs_pnr.airp_arv, "+
         CheckIn::TSimplePaxItem::origClassFromCrsSQL()+" AS class, "+
         CheckIn::TSimplePaxItem::origSubclassFromCrsSQL()+" AS subclass, "+
         CheckIn::TSimplePaxItem::cabinClassFromCrsSQL()+" AS cabin_class, "
         "       crs_pnr.status AS pnr_status, crs_pnr.priority AS pnr_priority, "
         "       crs_pax.surname,crs_pax.name,crs_pax.pers_type, "
         "       crs_pax.seat_xname,crs_pax.seat_yname, "
         "       crs_pax.seat_type,crs_pax.seats, "
         "       crs_pnr.pnr_id ";
  return result;
}

static const std::string& getSearchPaxQueryOrderByPart()
{
  static const std::string result=
         "ORDER BY crs_pnr.point_id,crs_pax.pnr_id,class,subclass,crs_pax.surname,crs_pax.pax_id ";
  return result;
}

void getTCkinSearchPaxQuery(DB::TQuery& Qry)
{
    std::ostringstream sql;

    sql << getSearchPaxQuerySelectPart()
        << "FROM tlg_binding "
           "JOIN (crs_pnr JOIN (crs_pax LEFT OUTER JOIN pax ON crs_pax.pax_id = pax.pax_id) "
           "              ON crs_pnr.pnr_id = crs_pax.pnr_id) "
           "ON tlg_binding.point_id_tlg=crs_pnr.point_id "
           "WHERE tlg_binding.point_id_spp=:point_id AND "
           "      crs_pnr.system='CRS' AND "
           "      crs_pnr.airp_arv=:airp_arv AND "
           "      crs_pnr.subclass=:subclass AND "
           "      (crs_pnr.status IS NULL OR crs_pnr.status NOT IN ('DG2','RG2','ID2','WL')) AND "
           "      crs_pax.pers_type=:pers_type AND "
           "      (CASE WHEN crs_pax.seats = 0 THEN 0 ELSE 1 END = :seats) AND "
           "      EXISTS ( "
           "          SELECT 1 FROM crs_pax_translit "
           "          WHERE crs_pax_translit.point_id = crs_pnr.point_id "
           "          AND crs_pax_translit.surname = :surname "
           "          AND crs_pax_translit.name = :name "
           "      ) AND "
           "      crs_pax.pr_del=0 AND "
           "      pax.pax_id IS NULL "
        << getSearchPaxQueryOrderByPart();

    Qry.SQLText=sql.str().c_str();
    Qry.DeclareVariable("point_id",otInteger);
    Qry.DeclareVariable("airp_arv",otString);
    Qry.DeclareVariable("subclass",otString);
    Qry.DeclareVariable("pers_type",otString);
    Qry.DeclareVariable("seats",otInteger);
    Qry.DeclareVariable("surname",otString);
    Qry.DeclareVariable("name",otString);
}

std::vector<SearchPaxResult> runSearchPax(const PnrId_t& pnr_id)
{
  DB::TQuery Qry(PgOra::getROSession({"CRS_PNR", "CRS_PAX", "PAX"}), STDLOG);
  std::ostringstream sql;

  sql << getSearchPaxQuerySelectPart()
      << "FROM crs_pnr "
         "  JOIN ( "
         "    crs_pax "
         "      LEFT OUTER JOIN pax "
         "        ON crs_pax.pax_id = pax.pax_id "
         "  ) "
         "    ON crs_pnr.pnr_id = crs_pax.pnr_id "
         "WHERE crs_pnr.pnr_id = :pnr_id "
         "AND crs_pax.pr_del = 0 "
         "AND pax.pax_id IS NULL "
      << getSearchPaxQueryOrderByPart();

  Qry.SQLText = sql.str().c_str();
  Qry.CreateVariable("pnr_id", otInteger, pnr_id.get());
  Qry.Execute();
  return fetchSearchPaxResults(Qry);
}

std::string makeSearchPaxQuery(const TPaxStatus& pax_status,
                               const bool& return_pnr_ids,
                               const std::string& sql_filter,
                               bool forOracle)
{
  //обычный поиск
  ostringstream sql;

  sql << getSearchPaxQuerySelectPart()
      << "FROM crs_pnr "
         "  JOIN (crs_pax LEFT OUTER JOIN pax ON crs_pax.pax_id = pax.pax_id) "
         "    ON crs_pnr.pnr_id = crs_pax.pnr_id "
         "  JOIN ( ";


  sql << getSearchPaxSubquery(pax_status,
                              return_pnr_ids,
                              true,
                              true,
                              true,
                              sql_filter,
                              forOracle);

  sql << "  ) ids ON ";
  if (return_pnr_ids) {
    sql << "ids.pnr_id = crs_pnr.pnr_id ";
  } else {
    sql << "ids.pax_id = crs_pax.pax_id ";
  }

  sql << "WHERE crs_pax.pr_del=0 AND "
         "      pax.pax_id IS NULL "
      << getSearchPaxQueryOrderByPart();

//  ProgTrace(TRACE5,"CheckInInterface::SearchPax: status=%s",EncodePaxStatus(pax_status));
//  ProgTrace(TRACE5,"CheckInInterface::SearchPax: sql=\n%s",sql.c_str());

  return sql.str();
}

std::vector<CrsDisplaceData> CrsDisplaceData::load(const PointId_t& point_id)
{
  LogTrace(TRACE6) << __func__
                   << ": point_id=" << point_id;
  std::vector<CrsDisplaceData> result;

  DB::TQuery Qry(PgOra::getROSession("CRS_DISPLACE2"), STDLOG);
  Qry.SQLText =
      "SELECT point_id_tlg, airp_arv_tlg, class_tlg, status "
      "FROM crs_displace2 "
      "WHERE point_id_spp=:point_id ";
  Qry.CreateVariable("point_id", otInteger, point_id.get());
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next()) {
    const std::set<PointId_t> b1_point_id_set = getPointIdsSppByPointIdTlg(
          PointIdTlg_t(Qry.FieldAsInteger("point_id_tlg")));
    for (const PointId_t& b1_point_id: b1_point_id_set) {
      if (b1_point_id == point_id) {
        continue;
      }
      const std::set<PointIdTlg_t> b2_point_id_tlg_set = getPointIdTlgByPointIdsSpp(b1_point_id);
      for (const PointIdTlg_t& b2_point_id_tlg: b2_point_id_tlg_set) {
        const CrsDisplaceData data = {
          b2_point_id_tlg,
          Qry.FieldAsString("airp_arv_tlg"),
          Qry.FieldAsString("class_tlg"),
          Qry.FieldAsString("status")
        };
        result.push_back(data);
      }
    }
  }
  LogTrace(TRACE6) << __func__
                   << ": result.size=" << result.size();
  return result;
}

std::vector<SearchPaxResult> fetchSearchPaxResults(DB::TQuery& Qry)
{
  std::vector<SearchPaxResult> result;
  int rownum = 0;
  for(;!Qry.Eof;Qry.Next()) {
    rownum++;
    DB::TQuery QrySeat(PgOra::getROSession("ORACLE"), STDLOG);
    QrySeat.SQLText = "SELECT salons.get_crs_seat_no(:pax_id,:seat_xname,:seat_yname,:seats,:point_id,'one',:num) AS seat_no FROM dual ";
    QrySeat.CreateVariable("pax_id", otInteger, Qry.FieldAsInteger("pax_id"));
    QrySeat.CreateVariable("seat_xname", otString, Qry.FieldAsString("seat_xname"));
    QrySeat.CreateVariable("seat_yname", otString, Qry.FieldAsString("seat_yname"));
    QrySeat.CreateVariable("seats", otInteger, Qry.FieldAsInteger("seats"));
    QrySeat.CreateVariable("point_id", otInteger, Qry.FieldAsInteger("point_id"));
    QrySeat.CreateVariable("num", otInteger, rownum);
    QrySeat.Execute();
    SearchPaxResult data = {
      PaxId_t(Qry.FieldAsInteger("pax_id")),
      PointIdTlg_t(Qry.FieldAsInteger("point_id")),
      Qry.FieldAsString("airp_arv"),
      Qry.FieldAsString("class"),
      Qry.FieldAsString("subclass"),
      Qry.FieldAsString("cabin_class"),
      Qry.FieldAsString("pnr_status"),
      Qry.FieldAsString("pnr_priority"),
      Qry.FieldAsString("surname"),
      Qry.FieldAsString("name"),
      Qry.FieldAsString("pers_type"),
      QrySeat.FieldAsString("seat_no"),
      Qry.FieldAsString("seat_type"),
      Qry.FieldAsInteger("seats"),
      PnrId_t(Qry.FieldAsInteger("pnr_id")),
      TypeB::getTKNO(Qry.FieldAsInteger("pax_id"), "/", false /*only_TKNE*/),
      TypeB::getTKNO(Qry.FieldAsInteger("pax_id"), "/", true /*only_TKNE*/)
    };
    result.push_back(data);
  }
  return result;
}

std::vector<SearchPaxResult> runSearchPax(const PointId_t& point_dep,
                                          const TPaxStatus& pax_status,
                                          const bool& return_pnr_ids,
                                          const std::string& sql_filter)
{
  LogTrace(TRACE6) << __func__;
  std::vector<SearchPaxResult> result;
  auto& sess = PgOra::getROSession({"CRS_PNR", "CRS_PAX", "TLG_BINDING", "PAX"});
  DB::TQuery Qry(sess, STDLOG);
  Qry.SQLText = makeSearchPaxQuery(pax_status, return_pnr_ids, sql_filter, sess.isOracle());
  bindSearchPaxQuery(Qry, point_dep, pax_status);
  const std::vector<CrsDisplaceData> items = CrsDisplaceData::load(point_dep);
  if (items.empty()) {
    Qry.Execute();
    return fetchSearchPaxResults(Qry);
  }
  for (const CrsDisplaceData& item: items) {
    Qry.SetVariable("point_id_tlg", item.point_id_tlg.get());
    Qry.SetVariable("airp_arv_tlg", item.airp_arv_tlg);
    Qry.SetVariable("class_tlg", item.class_tlg);
    Qry.SetVariable("status", item.status);
    Qry.Execute();
    const std::vector<SearchPaxResult> data = fetchSearchPaxResults(Qry);
    result.insert(result.end(), data.begin(), data.end());
  }
  return result;
}

namespace CheckIn
{

std::string Search::getSQLText() const
{
  if (conditions.empty()) return "";

  std::ostringstream sql;
  switch(origin)
  {
    case paxCheckIn:
      sql << "SELECT pax.* "
             "FROM pax ";
      for(const std::string& t : tables) {
        sql << ", " << t;
      }
      sql << " ";
      for(std::list<std::string>::const_iterator c=conditions.begin(); c!=conditions.end(); ++c)
        sql << (c==conditions.begin() ? "WHERE ":"  AND ") << *c << " ";
      break;
    case paxPnl:
      sql << "SELECT crs_pax.*, "
          << CheckIn::TSimplePaxItem::origSubclassFromCrsSQL() << " AS subclass, "
          << CheckIn::TSimplePaxItem::cabinSubclassFromCrsSQL() << " AS cabin_subclass, "
          << CheckIn::TSimplePaxItem::cabinClassFromCrsSQL() << " AS cabin_class, "
             "       NULL AS cabin_class_grp "
             "FROM crs_pnr, crs_pax";
      for(const std::string& t : tables) {
        sql << ", " << t;
      }
      sql << " ";
      sql << "WHERE crs_pax.pnr_id=crs_pnr.pnr_id "
             "  AND crs_pnr.system='CRS' "
             "  AND crs_pax.pr_del=0 ";
      for(std::list<std::string>::const_iterator c=conditions.begin(); c!=conditions.end(); ++c) {
        sql << "  AND " << *c << " ";
      }
      break;
    case paxTest:
      sql << "SELECT test_pax.id AS pax_id, surname, name, subclass, tkn_no "
             "FROM test_pax ";
      for(const std::string& t : tables) {
        sql << ", " << t;
      }
      sql << " ";
      for(std::list<std::string>::const_iterator c=conditions.begin(); c!=conditions.end(); ++c)
        sql << (c==conditions.begin() ? "WHERE ":"  AND ") << *c << " ";
      break;
  }

  return sql.str();
}

bool Search::executePaxQuery(const std::string& sql, TSimplePaxList& paxs) const
{
  std::set<std::string> involvedTablesSet({"PAX","CRS_PAX","CRS_PNR","TEST_PAX"});
  involvedTablesSet.insert(tables.begin(), tables.end());
  std::list<std::string> involvedTables;
  involvedTables.insert(involvedTables.begin(), involvedTablesSet.begin(), involvedTablesSet.end());
  DB::TCachedQuery Qry(PgOra::getROSession(involvedTables), sql, params, STDLOG);

  if (timeIsUp()) return false;
  Qry.get().Execute();
  for(; !Qry.get().Eof; Qry.get().Next())
  {
    if (timeIsUp()) return false;

    CheckIn::TSimplePaxItem pax;
    if (origin==paxCheckIn) {
      pax.fromDB(Qry.get());
    } else {
      pax.fromDBCrs(Qry.get(), false);
    }
    if (foundPaxIds.insert(pax.id).second) {
      paxs.push_back(pax);
    }
  }

  return true;
}

bool Search::addPassengers(CheckIn::TSimplePaxList& paxs)
{
  if (conditions.empty()) return true;

  string sql = getSQLText();

  LogTrace(TRACE6) << __FUNCTION__ << ": " << endl << sql;

  if (useSearchPaxIds) {
    for (PaxId_t pax_id: searchPaxIds) {
      params.remove_if([](const QParam& param){ return param.name=="pax_id"; });
      params << QParam("pax_id", otInteger, pax_id.get());
      LogTrace(TRACE6) << __FUNCTION__ << ": " << endl << params;
      if (!executePaxQuery(sql, paxs)) {
        return false;
      }
    }
    return true;
  }
  LogTrace(TRACE6) << __FUNCTION__ << ": " << endl << params;
  return executePaxQuery(sql, paxs);
}

bool Search::timeIsUp() const
{
  if (!timeout) return false;
  if (startTime.is_not_a_date_time()) return false;

  return (boost::posix_time::microsec_clock::local_time() - startTime).total_milliseconds() > timeout.get();
}

} //namespace CheckIn

bool PaxIdFilter::validForSearch() const
{
  return true;
}

void PaxIdFilter::addSQLConditionsForSearch(const PaxOrigin& origin, std::list<std::string>& conditions) const
{
  switch(origin)
  {
    case paxCheckIn:
      conditions.push_back("pax.pax_id = :pax_id");
      break;
    case paxPnl:
      conditions.push_back("crs_pax.pax_id = :pax_id");
      break;
    case paxTest:
      conditions.push_back("test_pax.id = :pax_id");
      break;
  }
}

void PaxIdFilter::addSQLParamsForSearch(const PaxOrigin& origin, QParams& params) const
{
  params << QParam("pax_id", otInteger, paxId.get());
}

bool SurnameFilter::validForSearch() const
{
  return !surname.empty();
}

void SurnameFilter::addSQLTablesForSearch(const PaxOrigin& origin, std::set<string>& tables,
                                          std::set<std::string>& session_tables) const
{
  switch(origin)
  {
    case paxCheckIn:
      session_tables.insert("pax_translit");
      break;
    case paxPnl:
      session_tables.insert("crs_pax_translit");
      break;
    case paxTest:
      session_tables.insert("test_pax_translit");
      break;
  }
}

void SurnameFilter::addSQLConditionsForSearch(const PaxOrigin& origin, std::list<std::string>& conditions) const
{
  ostringstream sql;
  switch(origin)
  {
    case paxCheckIn:
      sql << "EXISTS ( "
             "    SELECT 1 FROM pax_translit "
             "    WHERE pax_translit.pax_id = pax.pax_id "
          << (checkSurnameEqualBeginning ? "    AND (pax_translit.surname LIKE :surname_v1 || '%' "
                                           "         OR pax_translit.surname LIKE :surname_v2 || '%' "
                                           "         OR pax_translit.surname LIKE :surname_v3 || '%') "
                                         : "    AND pax_translit.surname IN (:surname_v1, :surname_v2, :surname_v3) ")
          << ") ";
      break;
    case paxPnl:
      sql << "EXISTS ( "
             "    SELECT 1 FROM crs_pax_translit "
             "    WHERE crs_pax_translit.pax_id = crs_pax.pax_id "
          << (checkSurnameEqualBeginning ? "    AND (crs_pax_translit.surname LIKE :surname_v1 || '%' "
                                           "         OR crs_pax_translit.surname LIKE :surname_v2 || '%' "
                                           "         OR crs_pax_translit.surname LIKE :surname_v3 || '%') "
                                         : "    AND crs_pax_translit.surname IN (:surname_v1, :surname_v2, :surname_v3) ")
          << ") ";
      break;
    case paxTest:
      sql << "EXISTS ( "
             "    SELECT 1 FROM test_pax_translit "
             "    WHERE test_pax_translit.pax_id = test_pax.id "
          << (checkSurnameEqualBeginning ? "    AND (test_pax_translit.surname LIKE :surname_v1 || '%' "
                                           "         OR test_pax_translit.surname LIKE :surname_v2 || '%' "
                                           "         OR test_pax_translit.surname LIKE :surname_v3 || '%') "
                                         : "    AND test_pax_translit.surname IN (:surname_v1, :surname_v2, :surname_v3) ")
          << ") ";
      break;
  }
  conditions.push_back(sql.str());
}

void SurnameFilter::addSQLParamsForSearch(const PaxOrigin& origin, QParams& params) const
{
  params << QParam("surname_v1", otString, transliter(surname, TranslitFormat::V1, true));
  params << QParam("surname_v2", otString, transliter(surname, TranslitFormat::V2, true));
  params << QParam("surname_v3", otString, transliter(surname, TranslitFormat::V3, true));
}

bool SurnameFilter::suitable(const CheckIn::TSimplePaxItem& pax) const
{
  return surname.empty() ||
         checkSurnameEqualBeginning?transliter_equal_begin(pax.surname, surname):
                                    transliter_equal      (pax.surname, surname);
}

std::string FullnameFilter::firstName(const std::string& name)
{
  string result(name);
  result.erase(find(result.begin(), result.end(), ' '), result.end());

  return result;
}

std::string FullnameFilter::transformName(const std::string& name) const
{
  return checkFirstNameOnly?firstName(name):name;
}

bool FullnameFilter::finalPassengerCheck(const CheckIn::TSimplePaxItem& pax) const
{
  if (!SurnameFilter::finalPassengerCheck(pax)) return false;

  //здесь проверяем имя
  return checkNameEqualBeginning?transliter_equal_begin(transformName(pax.name), transformName(name)):
                                 transliter_equal      (transformName(pax.name), transformName(name));
}

bool FullnameFilter::suitable(const CheckIn::TSimplePaxItem& pax) const
{
  return SurnameFilter::suitable(pax) &&
         (name.empty() ||
          checkNameEqualBeginning?transliter_equal_begin(transformName(pax.name), transformName(name)):
                                  transliter_equal      (transformName(pax.name), transformName(name)));
}

bool BarcodePaxFilter::finalPassengerCheck(const CheckIn::TSimplePaxItem& pax) const
{
  if (!FullnameFilter::finalPassengerCheck(pax)) return false;

  if (reg_no==ASTRA::NoExists) return true;
  return pax.reg_no==reg_no;
}

bool BarcodePaxFilter::suitable(const CheckIn::TSimplePaxItem& pax) const
{
  return FullnameFilter::suitable(pax) &&
         (reg_no==ASTRA::NoExists || pax.reg_no==reg_no);
}

bool TCkinPaxFilter::validForSearch() const
{
  return FullnameFilter::validForSearch() &&
         !subclass.empty();
}

void TCkinPaxFilter::addSQLTablesForSearch(const PaxOrigin& origin, std::set<std::string>& tables,
                                           std::set<std::string>& session_tables) const
{
  FullnameFilter::addSQLTablesForSearch(origin, tables, session_tables);

  switch(origin)
  {
    case paxCheckIn:
      tables.insert("pax_grp");
      break;
    case paxPnl:
      break;
    case paxTest:
      break;
  }
}

void TCkinPaxFilter::addSQLConditionsForSearch(const PaxOrigin& origin, std::list<std::string>& conditions) const
{
  FullnameFilter::addSQLConditionsForSearch(origin, conditions);

  switch(origin)
  {
    case paxCheckIn:
      conditions.push_back("pax_grp.grp_id=pax.grp_id");
      conditions.push_back("COALESCE(pax.cabin_subclass, pax.subclass)=:subclass");
      conditions.push_back("pax_grp.status IN (:checkin_status, :tcheckin_status)");
      break;
    case paxPnl:
      conditions.push_back("crs_pnr.subclass=:subclass");
      conditions.push_back("(crs_pnr.status IS NULL OR crs_pnr.status NOT IN ('DG2','RG2','ID2','WL'))");
      break;
    case paxTest:
      break;
  }
}

void TCkinPaxFilter::addSQLParamsForSearch(const PaxOrigin& origin, QParams& params) const
{
  FullnameFilter::addSQLParamsForSearch(origin, params);

  params << QParam("subclass", otString, subclass);

  switch(origin)
  {
    case paxCheckIn:
      params << QParam("checkin_status", otString, EncodePaxStatus(ASTRA::psCheckin))
             << QParam("tcheckin_status", otString, EncodePaxStatus(ASTRA::psTCheckin));
      break;
    case paxPnl:
      break;
    case paxTest:
      break;
  }
}

bool TCkinPaxFilter::finalPassengerCheck(const CheckIn::TSimplePaxItem& pax) const
{
  if (!FullnameFilter::finalPassengerCheck(pax)) return false;

  return (pax.pers_type!=NoPerson && pers_type!=NoPerson && pax.pers_type==pers_type) &&
         (pax.seats!=NoExists && seats!=NoExists && (pax.seats>0)==(seats>0));
}

bool TCkinPaxFilter::suitable(const CheckIn::TSimplePaxItem& pax) const
{
  return FullnameFilter::suitable(pax) &&
         (pers_type==NoPerson || pax.pers_type==pers_type) &&
         (seats==NoExists || (pax.seats!=NoExists && (pax.seats>0)==(seats>0)));

  return true;
}

FlightFilter createFlightFilter(const TTransferItem &item) {
    FlightFilter fltFilter(item.operFlt);
    fltFilter.setLocalDate(item.operFlt.scd_out);
    fltFilter.airp_arv = item.airp_arv;
    return fltFilter;
}

void FlightFilter::setLocalDate(TDateTime localDate)
{
    if (localDate==NoExists) return;

    modf(localDate, &localDate);
    scd_out=ASTRA::NoExists;
    min_scd_out=localDate;
    max_scd_out=localDate+1.0;
    scdOutIsLocal=true;
}

bool FlightFilter::validForSearch() const
{
  if (airp.empty()) return false;
  if (scd_out!=ASTRA::NoExists)
    return true;
  else
    return min_scd_out!=ASTRA::NoExists &&
           max_scd_out!=ASTRA::NoExists &&
           min_scd_out<max_scd_out &&
           max_scd_out-min_scd_out<=2.0;
}

void FlightFilter::addSQLTablesForSearch(const PaxOrigin& origin, std::set<std::string>& tables,
                                         std::set<std::string>& session_tables) const
{
  tables.insert("points");
  switch(origin)
  {
    case paxCheckIn:
      tables.insert("pax_grp");
      break;
    case paxPnl:
      tables.insert("tlg_binding");
      break;
    case paxTest:
      break;
  }
}

void FlightFilter::addSQLConditionsForSearch(const PaxOrigin& origin, std::list<std::string>& conditions) const
{
  if (scd_out!=ASTRA::NoExists)
    conditions.push_back("points.scd_out=:scd_out");
  else
  {
    conditions.push_back("points.scd_out>=:min_scd_out");
    conditions.push_back("points.scd_out<:max_scd_out");
  }
  conditions.push_back("points.airp=:airp_dep");
  if (!airline.empty())
    conditions.push_back("points.airline=:airline");
  if (flt_no!=ASTRA::NoExists)
  {
    conditions.push_back("points.flt_no=:flt_no");
    if (!suffix.empty())
      conditions.push_back("points.suffix=:suffix");
    else
      conditions.push_back("points.suffix IS NULL");
  }

  switch(origin)
  {
    case paxCheckIn:
      conditions.push_back("points.point_id=pax_grp.point_dep");
      conditions.push_back("pax_grp.grp_id=pax.grp_id");
      if (!airp_arv.empty())
        conditions.push_back("pax_grp.airp_arv=:airp_arv");
      break;
    case paxPnl:
      conditions.push_back("points.point_id=tlg_binding.point_id_spp");
      conditions.push_back("tlg_binding.point_id_tlg=crs_pnr.point_id");
      if (!airp_arv.empty())
        conditions.push_back("crs_pnr.airp_arv=:airp_arv");
      break;
    case paxTest:
      break;
  }
}

void FlightFilter::addSQLParamsForSearch(const PaxOrigin& origin, QParams& params) const
{
  params << QParam("airp_dep", otString, airp);

  if (scd_out!=ASTRA::NoExists)
  {
    if (scdOutIsLocal)
      params << QParam("scd_out", otDate, LocalToUTC(scd_out, AirpTZRegion(airp)));
    else
      params << QParam("scd_out", otDate, scd_out);
  }
  else
  {
    if (scdOutIsLocal)
    {
      params << QParam("min_scd_out", otDate, LocalToUTC(min_scd_out, AirpTZRegion(airp), BackwardWhenProblem))
             << QParam("max_scd_out", otDate, LocalToUTC(max_scd_out, AirpTZRegion(airp), ForwardWhenProblem));
    }
    else
      params << QParam("min_scd_out", otDate, min_scd_out)
             << QParam("max_scd_out", otDate, max_scd_out);
  }

  if (!airline.empty())
    params << QParam("airline", otString, airline);
  if (flt_no!=ASTRA::NoExists)
  {
    params << QParam("flt_no", otInteger, flt_no);
    if (!suffix.empty())
      params << QParam("suffix", otString, suffix);
  }
  if (!airp_arv.empty())
    params << QParam("airp_arv", otString, airp_arv);
}

bool FlightFilter::suitable(const TAdvTripRouteItem& departure,
                            const TAdvTripRouteItem& arrival) const
{
  TDateTime departure_scd_out=scdOutIsLocal?departure.scd_out_local():departure.scd_out;

  if (!airp.empty() && airp!=departure.airp) return false;

  if (scd_out!=ASTRA::NoExists &&
      (departure_scd_out==ASTRA::NoExists || scd_out!=departure_scd_out)) return false;

  if (min_scd_out!=ASTRA::NoExists &&
      (departure_scd_out==ASTRA::NoExists || min_scd_out>departure_scd_out)) return false;

  if (max_scd_out!=ASTRA::NoExists &&
      (departure_scd_out==ASTRA::NoExists || max_scd_out<=departure_scd_out)) return false;

  if (!airline.empty() && airline!=departure.airline_out) return false;
  if (flt_no!=ASTRA::NoExists && (flt_no!=departure.flt_num_out || suffix!=departure.suffix_out)) return false;
  if (!airp_arv.empty() && airp_arv!=arrival.airp) return false;

  return true;
}

void BarcodeSegmentFilter::set(const BCBPUniqueSections& unique,
                               const BCBPRepeatedSections& repeated)
{
  clear();

  try
  {
    //фамилия/имя пассажира
    pair<string, string> name_pair=unique.passengerName();

    pax.surname=name_pair.first;
    pax.name=name_pair.second;

    if (pax.surname.size()+pax.name.size()+1 >= 20)
    {
      pax.checkSurnameEqualBeginning=true;
      if (!pax.name.empty())
        pax.checkNameEqualBeginning=true;
    };

    //рег. номер
    pax.reg_no=repeated.checkinSeqNumber().first;

    //номер PNR
    pnr.addr=repeated.operatingCarrierPNRCode();

    TElemFmt fmt;
    //аэропорт вылета, а надо бы еще и город анализировать!!!
    seg.airp = ElemToElemId( etAirp, repeated.fromCityAirpCode() , fmt );
    if (fmt==efmtUnknown)
      throw EXCEPTIONS::EConvertError("unknown item 26 <From City Airport Code> %s", repeated.fromCityAirpCode().c_str());

    //аэропорт прилета, а надо бы еще и город анализировать!!!
    seg.airp_arv = ElemToElemId( etAirp, repeated.toCityAirpCode(), fmt );
    if (fmt==efmtUnknown)
      throw EXCEPTIONS::EConvertError("unknown item 38 <To City Airport Code> %s", repeated.toCityAirpCode().c_str());

    //авиакомпания
    seg.airline = ElemToElemId( etAirline, repeated.operatingCarrierDesignator(), fmt );
    if (fmt==efmtUnknown)
      throw EXCEPTIONS::EConvertError("unknown item 42 <Operating carrier Designator> %s", repeated.operatingCarrierDesignator().c_str());

    //номер рейса + суффикс
    pair<int, string> flt_no_pair=repeated.flightNumber();
    if (!flt_no_pair.second.empty())
    {
      seg.suffix = ElemToElemId( etSuffix, flt_no_pair.second, fmt );
      if (fmt==efmtUnknown)
        throw EXCEPTIONS::EConvertError("unknown item 43 <Flight Number> suffix %s", flt_no_pair.second.c_str());
    };

    seg.flt_no=flt_no_pair.first;

    //дата рейса (julian format)
    int julian_date=repeated.dateOfFlight();
    try
    {
      JulianDate scd_out_local(julian_date, NowUTC(), JulianDate::TDirection::everywhere);
      scd_out_local.trace(__FUNCTION__);
      seg.min_scd_out=scd_out_local.getDateTime();
      seg.max_scd_out=scd_out_local.getDateTime()+1.0;
      seg.scdOutIsLocal=true;
    }
    catch(const EXCEPTIONS::EConvertError&)
    {
      throw EXCEPTIONS::EConvertError("unknown item 46 <Date of Flight (Julian Date)> %d", julian_date);
    };
  }
  catch(const EXCEPTIONS::EConvertError&)
  {
    clear();
    throw;
  }
}

void BarcodeFilter::set(const std::string &barcode)
{
  clear();
  BCBPSections sections;
  BCBPSections::get(barcode, 0, barcode.size(), sections, false);
  for(const BCBPRepeatedSections& repeated : sections.repeated)
    emplace(end())->set(sections.unique, repeated);
}

void BarcodeFilter::getPassengers(CheckIn::Search& search, CheckIn::TSimplePaxList& paxs, bool checkOnlyFullname) const
{
  paxs.clear();
  for(const BarcodeSegmentFilter& filter : *this)
  {
    CheckIn::TSimplePaxList paxsOnSegment;
    if (checkOnlyFullname)
      search(paxsOnSegment, filter.seg, static_cast<const FullnameFilter&>(filter.pax), filter.pnr);
    else
      search(paxsOnSegment, filter.seg, filter.pax, filter.pnr);
    paxs.insert(paxs.end(), paxsOnSegment.begin(), paxsOnSegment.end());

    if (search.timeoutIsReached()) return;
  }
}

bool BarcodeFilter::suitable(const TAdvTripRouteItem& departure,
                             const TAdvTripRouteItem& arrival,
                             const CheckIn::TSimplePaxItem& pax,
                             const TPnrAddrs& pnrs,
                             bool checkOnlyFullname) const
{
  for(const BarcodeSegmentFilter& filter : *this)
  {
    if (!filter.seg.suitable(departure, arrival)) continue;
    if (checkOnlyFullname) {
      if (!static_cast<const FullnameFilter&>(filter.pax).suitable(pax)) continue;
    } else {
      if (!filter.pax.suitable(pax)) continue;
    }
    if (!filter.pnr.suitable(pnrs)) continue;

    return true;
  }

  return false;
}
