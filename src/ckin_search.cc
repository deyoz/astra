#include "ckin_search.h"

#include "astra_utils.h"
#include "date_time.h"

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
                                 const std::string& sql_filter)
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
      sql << "   UNION \n";

    if (return_pnr_ids)
      sql << "   SELECT DISTINCT crs_pnr.pnr_id, " << status_param << " AS status \n";
    else
      sql << "   SELECT DISTINCT crs_pax.pax_id, " << status_param << " AS status \n";

    sql <<   "   FROM crs_pnr";

    if (!return_pnr_ids || exclude_checked || exclude_deleted)
      sql << ", crs_pax";

    if (exclude_checked)
      sql << ", pax";

    sql <<   ", \n";

    sql <<   "    (SELECT b2.point_id_tlg, \n"
             "            airp_arv_tlg,class_tlg,status \n"
             "     FROM crs_displace2,tlg_binding b1,tlg_binding b2 \n"
             "     WHERE crs_displace2.point_id_tlg=b1.point_id_tlg AND \n"
             "           b1.point_id_spp=b2.point_id_spp AND \n"
             "           crs_displace2.point_id_spp=:point_id AND \n"
             "           b1.point_id_spp<>:point_id) crs_displace \n"
             "   WHERE crs_pnr.point_id=crs_displace.point_id_tlg AND \n"
             "         crs_pnr.system='CRS' AND \n"
             "         crs_pnr.airp_arv=crs_displace.airp_arv_tlg AND \n"
             "         crs_pnr.class=crs_displace.class_tlg \n";

    if (!return_pnr_ids || exclude_checked || exclude_deleted)
      sql << "         AND crs_pnr.pnr_id=crs_pax.pnr_id \n";

    if (pass==1)
      sql << "         AND crs_displace.status= " << status_param << " \n";

    if (pass==1 && pax_status==psCheckin && !select_pad_with_ok)
      sql << "         AND (crs_pnr.status IS NULL OR crs_pnr.status NOT IN ('DG2','RG2','ID2','WL')) \n";

    if (pass==2 && pax_status==psGoshow)
      sql << "         AND crs_displace.status= :ps_ok \n"
             "         AND crs_pnr.status IN ('DG2','RG2','ID2','WL') \n";

    if (exclude_checked)
      sql << "         AND crs_pax.pax_id=pax.pax_id(+) AND pax.pax_id IS NULL \n";

    if (exclude_deleted)
      sql << "         AND crs_pax.pr_del=0 \n";

    if (!sql_filter.empty())
      sql << "         AND ("+sql_filter+") \n";

  }

  //2 прохода:
  for(int pass=1;pass<=2;pass++)
  {
    if ((pass==1 && pax_status!=psCheckin && pax_status!=psGoshow) ||
        (pass==2 && pax_status==psCheckin)) continue;
    if (pass==1)
      sql << "   UNION \n";
    else
      sql << "   MINUS \n";

    if (return_pnr_ids)
      sql << "   SELECT DISTINCT crs_pnr.pnr_id, " << status_param << " AS status \n";
    else
      sql << "   SELECT DISTINCT crs_pax.pax_id, " << status_param << " AS status \n";

    sql <<   "   FROM crs_pnr, tlg_binding";

    if (!return_pnr_ids || exclude_checked || exclude_deleted)
      sql << ", crs_pax";

    if (exclude_checked)
      sql << ", pax \n";
    else
      sql << " \n";

    sql   << "   WHERE crs_pnr.point_id=tlg_binding.point_id_tlg AND \n"
             "         crs_pnr.system='CRS' AND \n"
             "         tlg_binding.point_id_spp= :point_id \n";

    if (!return_pnr_ids || exclude_checked || exclude_deleted)
      sql << "         AND crs_pnr.pnr_id=crs_pax.pnr_id \n";

    if ((pass==1 && pax_status==psCheckin && !select_pad_with_ok) ||
        (pass==2 && pax_status==psGoshow))
      sql << "         AND (crs_pnr.status IS NULL OR crs_pnr.status NOT IN ('DG2','RG2','ID2','WL')) \n";

    if (pass==1 && pax_status==psGoshow)
      sql << "         AND crs_pnr.status IN ('DG2','RG2','ID2','WL') \n";

    if (exclude_checked)
      sql << "         AND crs_pax.pax_id=pax.pax_id(+) AND pax.pax_id IS NULL \n";

    if (exclude_deleted)
      sql << "         AND crs_pax.pr_del=0 \n";

    if (!sql_filter.empty())
      sql << "         AND ("+sql_filter+") \n";
  }
  return sql.str();
}

static const std::string& getSearchPaxQuerySelectPart()
{
  static const std::string result=
         "SELECT crs_pax.pax_id,crs_pnr.point_id,crs_pnr.airp_arv, \n"+
         CheckIn::TSimplePaxItem::origClassFromCrsSQL()+" AS class, \n"+
         CheckIn::TSimplePaxItem::origSubclassFromCrsSQL()+" AS subclass, \n"+
         CheckIn::TSimplePaxItem::cabinClassFromCrsSQL()+" AS cabin_class, \n"
         "       crs_pnr.status AS pnr_status, crs_pnr.priority AS pnr_priority, \n"
         "       crs_pax.surname,crs_pax.name,crs_pax.pers_type, \n"
         "       salons.get_crs_seat_no(crs_pax.pax_id,crs_pax.seat_xname,crs_pax.seat_yname,crs_pax.seats,crs_pnr.point_id,'one',rownum) AS seat_no, \n"
         "       crs_pax.seat_type,crs_pax.seats, \n"
         "       crs_pnr.pnr_id, \n"
         "       report.get_TKNO(crs_pax.pax_id,'/',0) AS ticket, \n"
         "       report.get_TKNO(crs_pax.pax_id,'/',1) AS eticket \n";
  return result;
}

static const std::string& getSearchPaxQueryOrderByPart()
{
  static const std::string result=
         "ORDER BY crs_pnr.point_id,crs_pax.pnr_id,class,subclass,crs_pax.surname,crs_pax.pax_id \n";
  return result;
}

void getTCkinSearchPaxQuery(TQuery& Qry)
{
    Qry.Clear();

    ostringstream sql;

    sql << getSearchPaxQuerySelectPart()
        << "FROM tlg_binding,crs_pnr,crs_pax,pax "
           "WHERE tlg_binding.point_id_tlg=crs_pnr.point_id AND "
           "      crs_pnr.pnr_id=crs_pax.pnr_id AND "
           "      crs_pax.pax_id=pax.pax_id(+) AND "
           "      tlg_binding.point_id_spp=:point_id AND "
           "      crs_pnr.system='CRS' AND "
           "      crs_pnr.airp_arv=:airp_arv AND "
           "      crs_pnr.subclass=:subclass AND "
           "      (crs_pnr.status IS NULL OR crs_pnr.status NOT IN ('DG2','RG2','ID2','WL')) AND "
           "      crs_pax.pers_type=:pers_type AND "
           "      DECODE(crs_pax.seats,0,0,1)=:seats AND "
           "      system.transliter_equal(crs_pax.surname,:surname)<>0 AND "
           "      system.transliter_equal(crs_pax.name,:name)<>0 AND "
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
    Qry.DeclareVariable("name",otString);}

void executeSearchPaxQuery(const int& pnr_id,
                           TQuery& Qry)
{
  ostringstream sql;

  sql << getSearchPaxQuerySelectPart()
      << "FROM crs_pnr,crs_pax,pax "
         "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND "
         "      crs_pax.pax_id=pax.pax_id(+) AND "
         "      crs_pnr.pnr_id=:pnr_id AND "
         "      crs_pax.pr_del=0 AND "
         "      pax.pax_id IS NULL "
      << getSearchPaxQueryOrderByPart();

  Qry.Clear();
  Qry.SQLText = sql.str().c_str();
  Qry.CreateVariable("pnr_id", otInteger, pnr_id);
  Qry.Execute();
}

void executeSearchPaxQuery(const int& point_dep,
                           const TPaxStatus& pax_status,
                           const bool& return_pnr_ids,
                           const std::string& sql_filter,
                           TQuery& Qry)
{
  //обычный поиск
  ostringstream sql;

  sql << getSearchPaxQuerySelectPart()
      << "FROM crs_pnr,crs_pax,pax,( \n";


  sql << getSearchPaxSubquery(pax_status,
                              return_pnr_ids,
                              true,
                              true,
                              true,
                              sql_filter);

  sql << "  ) ids  \n"
         "WHERE crs_pnr.pnr_id=crs_pax.pnr_id AND \n"
         "      crs_pax.pax_id=pax.pax_id(+) AND \n";
  if (return_pnr_ids)
    sql <<
         "      ids.pnr_id=crs_pnr.pnr_id AND \n";
  else
    sql <<
         "      ids.pax_id=crs_pax.pax_id AND \n";

  sql << "      crs_pax.pr_del=0 AND \n"
         "      pax.pax_id IS NULL \n"
      << getSearchPaxQueryOrderByPart();

//  ProgTrace(TRACE5,"CheckInInterface::SearchPax: status=%s",EncodePaxStatus(pax_status));
//  ProgTrace(TRACE5,"CheckInInterface::SearchPax: sql=\n%s",sql.c_str());

  Qry.Clear();
  Qry.SQLText = sql.str().c_str();
  Qry.CreateVariable("point_id", otInteger, point_dep);
  switch (pax_status)
  {
    case psTransit: Qry.CreateVariable( "ps_transit", otString, EncodePaxStatus(ASTRA::psTransit) );
                    break;
     case psGoshow: Qry.CreateVariable( "ps_goshow", otString, EncodePaxStatus(ASTRA::psGoshow) );
                    //break не надо!
                    [[fallthrough]];
           default: Qry.CreateVariable( "ps_ok", otString, EncodePaxStatus(ASTRA::psCheckin) );
  }
  Qry.Execute();
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
      sql << "SELECT pax.* \n"
             "FROM pax";
      for(const std::string& t : tables)
        sql << ", " << t;
      sql << " \n";
      for(std::list<std::string>::const_iterator c=conditions.begin(); c!=conditions.end(); ++c)
        sql << (c==conditions.begin()?"WHERE ":"  AND ") << *c << " \n";
      break;
    case paxPnl:
      sql << "SELECT crs_pax.*, \n"
          << CheckIn::TSimplePaxItem::origSubclassFromCrsSQL() << " AS subclass, \n"
          << CheckIn::TSimplePaxItem::cabinSubclassFromCrsSQL() << " AS cabin_subclass, \n"
          << CheckIn::TSimplePaxItem::cabinClassFromCrsSQL() << " AS cabin_class, \n"
             "       NULL AS cabin_class_grp \n"
             "FROM crs_pnr, crs_pax";
      for(const std::string& t : tables)
        sql << ", " << t;
      sql << " \n";
      sql << "WHERE crs_pax.pnr_id=crs_pnr.pnr_id \n"
             "  AND crs_pnr.system='CRS' \n"
             "  AND crs_pax.pr_del=0 \n";
      for(std::list<std::string>::const_iterator c=conditions.begin(); c!=conditions.end(); ++c)
        sql << "  AND " << *c << " \n";
      break;
    case paxTest:
      sql << "SELECT test_pax.id AS pax_id, surname, name, subclass, tkn_no \n"
             "FROM test_pax";
      for(const std::string& t : tables)
        sql << ", " << t;
      sql << " \n";
      for(std::list<std::string>::const_iterator c=conditions.begin(); c!=conditions.end(); ++c)
        sql << (c==conditions.begin()?"WHERE ":"  AND ") << *c << " \n";
      break;
  }

  return sql.str();
}

bool Search::addPassengers(CheckIn::TSimplePaxList& paxs) const
{
  if (conditions.empty()) return true;

  string sql = getSQLText();

//  LogTrace(TRACE5) << __FUNCTION__ << ": " << endl << sql;
//  LogTrace(TRACE5) << __FUNCTION__ << ": " << endl << params;

  TCachedQuery Qry(sql, params);

  if (timeIsUp()) return false;
  Qry.get().Execute();
  for(; !Qry.get().Eof; Qry.get().Next())
  {
    if (timeIsUp()) return false;

    CheckIn::TSimplePaxItem pax;
    if (origin==paxCheckIn)
      pax.fromDB(Qry.get());
    else
      pax.fromDBCrs(Qry.get(), false);
    if (foundPaxIds.insert(pax.id).second)
      paxs.push_back(pax);
  }

  return true;
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

void SurnameFilter::addSQLConditionsForSearch(const PaxOrigin& origin, std::list<std::string>& conditions) const
{
  std::string field_name;

  switch(origin)
  {
    case paxCheckIn:
      field_name="pax.surname";
      break;
    case paxPnl:
      field_name="crs_pax.surname";
      break;
    case paxTest:
      field_name="test_pax.surname";
      break;
  }
  ostringstream sql;
  sql << (checkSurnameEqualBeginning?"system.transliter_equal_begin(":
                                     "system.transliter_equal(")
      << field_name << ", :surname)<>0";

  conditions.push_back(sql.str());
}

void SurnameFilter::addSQLParamsForSearch(const PaxOrigin& origin, QParams& params) const
{
  params << QParam("surname", otString, surname);
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

void TCkinPaxFilter::addSQLTablesForSearch(const PaxOrigin& origin, std::set<std::string>& tables) const
{
  FullnameFilter::addSQLTablesForSearch(origin, tables);

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
      conditions.push_back("NVL(pax.cabin_subclass, pax.subclass)=:subclass");
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

void FlightFilter::addSQLTablesForSearch(const PaxOrigin& origin, std::set<std::string>& tables) const
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

  if (!airline.empty() && airline!=departure.airline) return false;
  if (flt_no!=ASTRA::NoExists && (flt_no!=departure.flt_num || suffix!=departure.suffix)) return false;
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

