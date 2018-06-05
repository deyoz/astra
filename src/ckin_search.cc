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

void executeSearchPaxQuery(const int& point_dep,
                           const TPaxStatus& pax_status,
                           const bool& return_pnr_ids,
                           const std::string& sql_filter,
                           TQuery& Qry)
{
  //обычный поиск
  ostringstream sql;

  sql <<
    "SELECT crs_pax.pax_id,crs_pnr.point_id,crs_pnr.airp_arv,crs_pnr.subclass, \n"
    "       crs_pnr.class, crs_pnr.status AS pnr_status, crs_pnr.priority AS pnr_priority, \n"
    "       crs_pax.surname,crs_pax.name,crs_pax.pers_type, \n"
    "       salons.get_crs_seat_no(crs_pax.pax_id,crs_pax.seat_xname,crs_pax.seat_yname,crs_pax.seats,crs_pnr.point_id,'one',rownum) AS seat_no, \n"
    "       crs_pax.seat_type,crs_pax.seats, \n"
    "       crs_pnr.pnr_id, \n"
    "       report.get_TKNO(crs_pax.pax_id,'/',0) AS ticket, \n"
    "       report.get_TKNO(crs_pax.pax_id,'/',1) AS eticket \n"
    "FROM crs_pnr,crs_pax,pax,( \n";


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
         "ORDER BY crs_pnr.point_id,crs_pax.pnr_id,crs_pax.surname,crs_pax.pax_id \n";

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
           default: Qry.CreateVariable( "ps_ok", otString, EncodePaxStatus(ASTRA::psCheckin) );
  }
  Qry.Execute();
}
