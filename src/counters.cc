#include "counters.h"
#include "qrys.h"
#include "exceptions.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

using namespace std;

namespace CheckIn
{

const TCrsCountersKey& TCrsCountersKey::toDB(TQuery &Qry) const
{
  Qry.SetVariable("point_dep", point_dep);
  Qry.SetVariable("airp_arv", airp_arv);
  Qry.SetVariable("class", cl);
  return *this;
}

TCrsCountersKey& TCrsCountersKey::fromDB(TQuery &Qry)
{
  clear();
  point_dep=Qry.FieldAsInteger("point_dep");
  airp_arv=Qry.FieldAsString("airp_arv");
  cl=Qry.FieldAsString("class");
  return *this;
}

const TCountersKey& TCountersKey::toDB(TQuery &Qry) const
{
  Qry.SetVariable("point_dep", point_dep);
  Qry.SetVariable("point_arv", point_arv);
  Qry.SetVariable("class", cl);
  return *this;
}

TCountersKey& TCountersKey::fromDB(TQuery &Qry)
{
  clear();
  point_dep=Qry.FieldAsInteger("point_dep");
  point_arv=Qry.FieldAsInteger("point_arv");
  cl=Qry.FieldAsString("class");
  return *this;
}

const TCrsCountersData& TCrsCountersData::toDB(TQuery &Qry) const
{
  Qry.SetVariable("crs_tranzit", tranzit);
  Qry.SetVariable("crs_ok", ok);
  return *this;
}

TCrsCountersData& TCrsCountersData::fromDB(TQuery &Qry)
{
  clear();
  tranzit=Qry.FieldAsInteger("crs_tranzit");
  ok=Qry.FieldAsInteger("crs_ok");
  return *this;
}

const TRegCountersData& TRegCountersData::toDB(TQuery &Qry) const
{
  Qry.SetVariable("tranzit", tranzit);
  Qry.SetVariable("ok", ok);
  Qry.SetVariable("goshow", goshow);
  Qry.SetVariable("jmp_tranzit", jmp_tranzit);
  Qry.SetVariable("jmp_ok", jmp_ok);
  Qry.SetVariable("jmp_goshow", jmp_goshow);
  return *this;
}

boost::optional<int> TCrsCountersMap::getMaxCrsPriority() const
{
  LogTrace(TRACE5) << __FUNCTION__;

  boost::optional<int> max;

  TCachedQuery Qry("SELECT priority, crs FROM crs_set "
                   "WHERE airline=:airline AND "
                   "      (flt_no=:flt_no OR flt_no IS NULL) AND "
                   "      (airp_dep=:airp_dep OR airp_dep IS NULL) "
                   "ORDER BY crs,flt_no,airp_dep ",
                   QParams() << QParam("airline", otString, _flt.airline)
                             << QParam("flt_no", otInteger, _flt.flt_no)
                             << QParam("airp_dep", otString, _flt.airp));
  string prior_crs;
  Qry.get().Execute();
  for(; !Qry.get().Eof; Qry.get().Next())
  {
    string curr_crs=Qry.get().FieldAsString("crs");
    int priority=Qry.get().FieldAsInteger("priority");
    if (prior_crs==curr_crs) continue;
    if (!max || max.get()<priority) max=priority;
    prior_crs=curr_crs;
  }

  return max;
}

void TCrsCountersMap::loadCrsDataOnly()
{
  LogTrace(TRACE5) << __FUNCTION__;

  clear();

  boost::optional<int> priority=getMaxCrsPriority();

  TCachedQuery Qry("SELECT :point_id_spp AS point_dep, airp_arv, class, "
                   "       NVL(SUM(tranzit),0) AS crs_tranzit, NVL(SUM(resa),0) AS crs_ok "
                   "FROM "
                   "  (SELECT sender,airp_arv,class,SUM(tranzit) AS tranzit,SUM(resa) AS resa "
                   "   FROM crs_data,tlg_binding "
                   "   WHERE crs_data.point_id=tlg_binding.point_id_tlg AND "
                   "         point_id_spp=:point_id_spp AND crs_data.system='CRS' "
                   "   GROUP BY sender,airp_arv,class) "
                   "WHERE NVL(ckin.get_crs_priority(sender,:airline,:flt_no,:airp_dep),0)=NVL(:priority,0) "
                   "GROUP BY airp_arv,class "
                   "HAVING SUM(tranzit) IS NOT NULL OR SUM(resa) IS NOT NULL",
                   QParams() << QParam("point_id_spp", otInteger, _flt.point_id)
                             << QParam("airline", otString, _flt.airline)
                             << QParam("flt_no", otInteger, _flt.flt_no)
                             << QParam("airp_dep", otString, _flt.airp)
                             << QParam("priority", otInteger, FNull));
  if (priority) Qry.get().SetVariable("priority", priority.get());
  Qry.get().Execute();

  for(; !Qry.get().Eof; Qry.get().Next())
    insert(make_pair(TCrsCountersKey().fromDB(Qry.get()), TCrsCountersData().fromDB(Qry.get())));
}

void TCrsCountersMap::loadSummary()
{
  LogTrace(TRACE5) << __FUNCTION__;

  clear();
  TCachedQuery Qry("SELECT point_dep, airp_arv, class, crs_tranzit, crs_ok, 0 AS priority "
                   "FROM crs_counters "
                   "WHERE point_dep=:point_dep "
                   "UNION "
                   "SELECT point_id AS point_dep, airp_arv, class, tranzit AS crs_tranzit, resa AS crs_ok, 1 AS priority "
                   "FROM trip_data "
                   "WHERE point_id=:point_dep "
                   "ORDER BY priority DESC",
                   QParams() << QParam("point_dep", otInteger, _flt.point_id));
  Qry.get().Execute();

  for(; !Qry.get().Eof; Qry.get().Next())
    insert(make_pair(TCrsCountersKey().fromDB(Qry.get()), TCrsCountersData().fromDB(Qry.get())));
}

void TCrsCountersMap::loadCrsCountersOnly()
{
  LogTrace(TRACE5) << __FUNCTION__;

  clear();
  TCachedQuery Qry("SELECT * FROM crs_counters WHERE point_dep=:point_dep",
                   QParams() << QParam("point_dep", otInteger, _flt.point_id));
  Qry.get().Execute();

  for(; !Qry.get().Eof; Qry.get().Next())
    insert(make_pair(TCrsCountersKey().fromDB(Qry.get()), TCrsCountersData().fromDB(Qry.get())));
}

void TCrsCountersMap::deleteCrsCountersOnly(int point_id)
{
  LogTrace(TRACE5) << __FUNCTION__;

  TCachedQuery Qry("DELETE FROM crs_counters WHERE point_dep=:point_dep",
                   QParams() << QParam("point_dep", otInteger, point_id));
  Qry.get().Execute();
}

void TCrsCountersMap::saveCrsCountersOnly() const
{
  LogTrace(TRACE5) << __FUNCTION__;

  deleteCrsCountersOnly(_flt.point_id);

  if (!empty())
  {
    TCachedQuery InsQry("INSERT INTO crs_counters(point_dep, airp_arv, class, crs_tranzit, crs_ok) "
                        "VALUES(:point_dep, :airp_arv, :class, :crs_tranzit, :crs_ok)",
                        QParams() << QParam("point_dep", otInteger)
                                  << QParam("airp_arv", otString)
                                  << QParam("class", otString)
                                  << QParam("crs_tranzit", otInteger)
                                  << QParam("crs_ok", otInteger));
    for(const auto &i : *this)
    {
      i.first.toDB(InsQry.get());
      i.second.toDB(InsQry.get());
      InsQry.get().Execute();
    }
  }
}

void TCrsFieldsMap::convertFrom(const TCrsCountersMap &src, const TAdvTripInfo &flt, const TTripRoute &routeAfter)
{
  clear();
  for(const auto &i : src)
  {
    TCountersKey key;
    key.point_dep=i.first.point_dep;
    key.cl=i.first.cl;
    for(const TTripRouteItem &r : routeAfter)
      if (!r.pr_cancel && r.point_num>flt.point_num && r.airp==i.first.airp_arv)
      {
        key.point_arv=r.point_id;
        break;
      }
    if (key.point_arv==ASTRA::NoExists) continue; //???? ????????? ? ????????
    TCrsCountersData data(i.second);
    if (!insert(make_pair(key, data)).second)
      LogError(STDLOG) << __FUNCTION__ << ": duplicate key!";
  }
}

void TCrsFieldsMap::apply(const TAdvTripInfo &flt, const bool pr_tranz_reg) const
{
  LogTrace(TRACE5) << __FUNCTION__;

  TCachedQuery DelQry("UPDATE counters2 SET crs_tranzit=0, crs_ok=0 WHERE point_dep=:point_dep",
                      QParams() << QParam("point_dep", otInteger, flt.point_id));
  DelQry.get().Execute();

  if (!empty())
  {
    TCachedQuery UpdQry("UPDATE counters2 "
                        "SET crs_tranzit=DECODE(:pr_tranzit, 0, 0, :crs_tranzit), "
                        "    crs_ok=:crs_ok, "
                        "    tranzit=    DECODE(:pr_tranzit, 0, 0, "
                        "                       DECODE(:pr_tranz_reg, 0, :crs_tranzit, tranzit)) "
                        "WHERE point_dep=:point_dep AND point_arv=:point_arv AND class=:class",
                        QParams() << QParam("pr_tranzit", otInteger, flt.pr_tranzit)
                                  << QParam("pr_tranz_reg", otInteger, pr_tranz_reg)
                                  << QParam("point_dep", otInteger)
                                  << QParam("point_arv", otInteger)
                                  << QParam("class", otString)
                                  << QParam("crs_tranzit", otInteger)
                                  << QParam("crs_ok", otInteger));
    for(const auto &i : *this)
    {
      i.first.toDB(UpdQry.get());
      i.second.toDB(UpdQry.get());
      UpdQry.get().Execute();
    }
  }
}

void TRegDifferenceMap::trace(const CheckIn::TPaxGrpItem &grp,
                              const CheckIn::TSimplePaxList &prior_paxs,
                              const CheckIn::TSimplePaxList &curr_paxs)   //!!!vlad ????????
{
  LogTrace(TRACE5) << "grp.status: " << grp.status;
  for(int pass=0; pass<2; pass++)
  {
    for(const CheckIn::TSimplePaxItem& pax : (pass==0?prior_paxs:curr_paxs))
    {
      LogTrace(TRACE5) << " id: " << pax.id
                       << " refuse: " << pax.refuse
                       << " seats: " << pax.seats
                       << " is_jmp: " << boolalpha << pax.is_jmp;
    }
  }
}

void TRegDifferenceMap::getDifference(const CheckIn::TPaxGrpItem &grp,
                                      const CheckIn::TSimplePaxList &prior_paxs,
                                      const CheckIn::TSimplePaxList &curr_paxs)
{
  LogTrace(TRACE5) << __FUNCTION__;

  clear();

  TRegDifferenceMap localDifferenceMap;

  for(int pass=0; pass<2; pass++)
  {
    for(const CheckIn::TSimplePaxItem& pax : (pass==0?prior_paxs:curr_paxs))
    {
      TCountersKey key;
      key.point_dep=grp.point_dep;
      key.point_arv=grp.point_arv;
      key.cl=pax.cabin.cl.empty()?grp.cl:pax.cabin.cl;

      TRegCountersData& data=localDifferenceMap.emplace(key, TRegCountersData()).first->second;

      if (!pax.refuse.empty()) continue;
      if (pax.seats<=0) continue;
      int* d=nullptr;
      switch (grp.status)
      {
        case ASTRA::psCheckin:
        case ASTRA::psTCheckin:
          d=pax.is_jmp?(&data.jmp_ok):(&data.ok); break;
        case ASTRA::psTransit:
          d=pax.is_jmp?(&data.jmp_tranzit):(&data.tranzit); break;
        case ASTRA::psGoshow:
          d=pax.is_jmp?(&data.jmp_goshow):(&data.goshow); break;
        case ASTRA::psCrew: ;
      }
      if (d==nullptr) continue;

      if (pass==0)
        *d-=pax.seats;
      else
        *d+=pax.seats;
    }
  }

  for(const auto& i : localDifferenceMap)
    if (!i.second.isZero())
      insert(i);
}

void TRegDifferenceMap::apply(const TAdvTripInfo &flt, const bool pr_tranz_reg) const
{
  LogTrace(TRACE5) << __FUNCTION__;

  if (!empty())
  {
    TCachedQuery UpdQry("UPDATE counters2 "
                        "SET tranzit=    DECODE(:pr_tranzit, 0, 0, "
                        "                       DECODE(:pr_tranz_reg, 0, crs_tranzit, tranzit+:tranzit)), "
                        "    ok=ok+:ok, "
                        "    goshow=goshow+:goshow, "
                        "    jmp_tranzit=DECODE(:pr_tranzit, 0, 0, "
                        "                       DECODE(:pr_tranz_reg, 0, 0, jmp_tranzit+:jmp_tranzit)), "
                        "    jmp_ok=jmp_ok+:jmp_ok, "
                        "    jmp_goshow=jmp_goshow+:jmp_goshow "
                        "WHERE point_dep=:point_dep AND point_arv=:point_arv AND class=:class",
                        QParams() << QParam("pr_tranzit", otInteger, flt.pr_tranzit)
                                  << QParam("pr_tranz_reg", otInteger, pr_tranz_reg)
                                  << QParam("point_dep", otInteger)
                                  << QParam("point_arv", otInteger)
                                  << QParam("class", otString)
                                  << QParam("tranzit", otInteger)
                                  << QParam("ok", otInteger)
                                  << QParam("goshow", otInteger)
                                  << QParam("jmp_tranzit", otInteger)
                                  << QParam("jmp_ok", otInteger)
                                  << QParam("jmp_goshow", otInteger));
    for(const auto &i : *this)
    {
      i.first.toDB(UpdQry.get());
      i.second.toDB(UpdQry.get());
      UpdQry.get().Execute();
    }
  }
}

bool TCounters::setFlt(int point_id)
{
  if (point_id!=_flt.point_id)
  {
    clear();
    if (!_flt.getByPointId(point_id)) return false;
    if (_flt.airline.empty()) return false;
  }

  return _flt.point_id!=ASTRA::NoExists;
}

const TAdvTripInfo& TCounters::flt()
{
  if (_flt.point_id==ASTRA::NoExists)
    throw EXCEPTIONS::Exception("TCounters::flt(): flt.point_id==ASTRA::NoExists");

  return _flt;
}

const TTripSetList& TCounters::fltSettings()
{
  if (!_fltSettings)
  {
    _fltSettings=boost::in_place();
    _fltSettings.get().fromDB(flt().point_id);
  }
  return _fltSettings.get();
}

const TTripRoute& TCounters::fltRouteAfter()
{
  if (!_fltRouteAfter)
  {
    _fltRouteAfter=TTripRoute();
    _fltRouteAfter.get().GetRouteAfter(ASTRA::NoExists,
                                       flt().point_id, flt().point_num, flt().first_point, flt().pr_tranzit,
                                       trtNotCurrent, trtNotCancelled);
  }

  return _fltRouteAfter.get();
}

const bool& TCounters::pr_tranz_reg()
{
  if (!_pr_tranz_reg)
  {
    TCachedQuery Qry("SELECT ckin.get_pr_tranz_reg(point_id) AS pr_tranz_reg FROM points WHERE point_id=:point_id AND pr_del>=0",
                     QParams() << QParam("point_id", otInteger, flt().point_id));
    Qry.get().Execute();
    _pr_tranz_reg=!Qry.get().Eof && Qry.get().FieldAsInteger("pr_tranz_reg")!=0;
  }

  return _pr_tranz_reg.get();
}

const bool& TCounters::cfg_exists()
{
  if (!_cfg_exists)
  {
    TCachedQuery Qry("SELECT COUNT(*) AS cfg_exists FROM trip_classes WHERE point_id=:point_id AND cfg>0 AND rownum<2",
                     QParams() << QParam("point_id", otInteger, flt().point_id));
    Qry.get().Execute();
    _cfg_exists=!Qry.get().Eof && Qry.get().FieldAsInteger("cfg_exists")!=0;
  }

  return _cfg_exists.get();
}

void TCounters::deleteInitially(int point_id)
{
  LogTrace(TRACE5) << __FUNCTION__;

  TCachedQuery Qry("DELETE FROM counters2 WHERE point_dep=:point_dep",
                   QParams() << QParam("point_dep", otInteger, point_id));
  Qry.get().Execute();
}

void TCounters::lockInitially(int point_id)
{
  LogTrace(TRACE5) << __FUNCTION__;

  TCachedQuery Qry("SELECT point_dep FROM counters2 WHERE point_dep=:point_dep FOR UPDATE",
                   QParams() << QParam("point_dep", otInteger, point_id));
  Qry.get().Execute();
}

void TCounters::recountInitially()
{
  LogTrace(TRACE5) << __FUNCTION__;

  deleteInitially(flt().point_id);

  TCachedQuery Qry(
        "INSERT INTO counters2 "
        "  (point_dep, point_arv, class, crs_tranzit, crs_ok, "
        "   tranzit, ok, goshow, "
        "   jmp_tranzit, jmp_ok, jmp_goshow, "
        "   avail, free_ok, free_goshow, nooccupy, jmp_nooccupy) "
        "SELECT "
        "   :point_dep, main.point_arv, main.class, 0, 0, "
        "   DECODE(:pr_tranzit, 0, 0, "
        "          DECODE(:pr_tranz_reg, 0, 0, NVL(pax.tranzit,0))), "
        "   NVL(pax.ok,0), "
        "   NVL(pax.goshow,0), "
        "   DECODE(:pr_tranzit, 0, 0, "
        "          DECODE(:pr_tranz_reg, 0, 0, NVL(pax.jmp_tranzit,0))), "
        "   NVL(pax.jmp_ok,0), "
        "   NVL(pax.jmp_goshow,0), "
        "   0, 0, 0, 0, 0 "
        "FROM "
        "     (SELECT points.point_id AS point_arv, point_num, classes.code AS class "
        "      FROM points, classes, trip_classes "
        "      WHERE classes.code=trip_classes.class(+) AND trip_classes.point_id(+)=:point_dep AND "
        "            points.first_point=:first_point AND points.point_num>:point_num AND points.pr_del=0 AND "
        "            (trip_classes.point_id IS NOT NULL OR :cfg_exists=0 AND :pr_free_seating<>0)) main, "
        "     (SELECT pax_grp.point_arv, NVL(pax.cabin_class, pax_grp.class) AS class, "
        "             SUM(DECODE(pax.is_jmp, 0, DECODE(pax_grp.status,'T',pax.seats,0), 0)) AS tranzit, "
        "             SUM(DECODE(pax.is_jmp, 0, DECODE(pax_grp.status,'K',pax.seats,'C',pax.seats,0), 0)) AS ok, "
        "             SUM(DECODE(pax.is_jmp, 0, DECODE(pax_grp.status,'P',pax.seats,0), 0)) AS goshow, "
        "             SUM(DECODE(pax.is_jmp, 0, 0, DECODE(pax_grp.status,'T',pax.seats,0))) AS jmp_tranzit, "
        "             SUM(DECODE(pax.is_jmp, 0, 0, DECODE(pax_grp.status,'K',pax.seats,'C',pax.seats,0))) AS jmp_ok, "
        "             SUM(DECODE(pax.is_jmp, 0, 0, DECODE(pax_grp.status,'P',pax.seats,0))) AS jmp_goshow "
        "      FROM pax_grp, pax "
        "      WHERE pax_grp.grp_id=pax.grp_id AND "
        "            pax_grp.point_dep=:point_dep AND "
        "            pax_grp.status NOT IN ('E') AND "
        "            pax.refuse IS NULL "
        "      GROUP BY pax_grp.point_arv, NVL(pax.cabin_class, pax_grp.class)) pax "
        "WHERE main.point_arv=pax.point_arv(+) AND main.class=pax.class(+)",
        QParams() << QParam("point_dep", otInteger, flt().point_id)
                  << QParam("first_point", otInteger, !flt().pr_tranzit?flt().point_id:flt().first_point)
                  << QParam("point_num", otInteger, flt().point_num)
                  << QParam("pr_tranzit", otInteger, flt().pr_tranzit)
                  << QParam("pr_tranz_reg", otInteger, pr_tranz_reg())
                  << QParam("cfg_exists", otInteger, cfg_exists())
                  << QParam("pr_free_seating", otInteger, (int)fltSettings().value<bool>(tsFreeSeating, false)));
  Qry.get().Execute();
}

void TCounters::recountFinally()
{
  LogTrace(TRACE5) << __FUNCTION__;

  TCachedQuery Qry(
        "DECLARE "
        "  CURSOR cur IS "
        "    SELECT counters2.point_dep, "
        "           counters2.class, "
        "           MAX(NVL(cfg,0))-MAX(NVL(block,0))-SUM(crs_tranzit)-SUM(crs_ok) AS avail, "
        "           MAX(NVL(cfg,0))-MAX(NVL(block,0))-SUM(GREATEST(crs_tranzit, tranzit))-SUM(ok)-SUM(goshow) AS free_ok, "
        "           MAX(NVL(cfg,0))-MAX(NVL(block,0))-SUM(GREATEST(crs_tranzit, tranzit))-SUM(GREATEST(crs_ok, ok))-SUM(goshow) AS free_goshow, "
        "           MAX(NVL(cfg,0))-MAX(NVL(block,0))-SUM(tranzit)-SUM(ok)-SUM(goshow) AS nooccupy, "
        "           SUM(jmp_tranzit)+SUM(jmp_ok)+SUM(jmp_goshow) AS jmp "
        "    FROM counters2, trip_classes "
        "    WHERE counters2.point_dep=trip_classes.point_id(+) AND "
        "          counters2.class=trip_classes.class(+) AND "
        "          counters2.point_dep=:point_dep "
        "    GROUP BY counters2.point_dep, counters2.class; "
        "jmp  NUMBER(6); "
        "BEGIN "
        "  jmp:=0; "
        "  FOR curRow IN cur LOOP "
        "    UPDATE counters2 "
        "    SET avail=curRow.avail, "
        "        free_ok=curRow.free_ok, "
        "        free_goshow=curRow.free_goshow, "
        "        nooccupy=curRow.nooccupy "
        "    WHERE point_dep=curRow.point_dep AND class=curRow.class; "
        "    jmp:=jmp+curRow.jmp; "
        "  END LOOP; "
        "  UPDATE counters2 SET jmp_nooccupy=DECODE(:use_jmp, 0, 0, :jmp_cfg)-jmp WHERE point_dep=:point_dep; "
        "END;",
        QParams() << QParam("point_dep", otInteger, flt().point_id)
                  << QParam("use_jmp", otInteger, (int)fltSettings().value<bool>(tsUseJmp, false))
                  << QParam("jmp_cfg", otInteger, fltSettings().value<int>(tsJmpCfg, 0))
        );
  Qry.get().Execute();
}

void TCounters::recountCrsFields()
{
  LogTrace(TRACE5) << __FUNCTION__;

  TCrsCountersMap crsCounters(flt());
  crsCounters.loadSummary();

  TCrsFieldsMap crsFields;
  crsFields.convertFrom(crsCounters, flt(), fltRouteAfter());

  crsFields.apply(flt(), pr_tranz_reg());
}

const TCounters &TCounters::recount(int point_id, RecountType type, const std::string& whence)
{
  LogTrace(TRACE5) << __FUNCTION__ << ": point_id=" << point_id << ", whence=" << whence;

  if (!setFlt(point_id))
  {
    if (type==Total)
      TCounters::deleteInitially(point_id);
    if (type==CrsCounters)
      TCrsCountersMap::deleteCrsCountersOnly(point_id);
    return *this;
  }

  if (type==Total)
    recountInitially();

  if (type==CrsCounters)
  {
    TCrsCountersMap priorCrsCounters(flt()), currCrsCounters(flt());
    priorCrsCounters.loadCrsCountersOnly();
    currCrsCounters.loadCrsDataOnly();
    if (priorCrsCounters==currCrsCounters) return *this;  //???????? ????? ?? ??????????

    lockInitially(point_id);
    currCrsCounters.saveCrsCountersOnly();
  }

  recountCrsFields();
  recountFinally();

  return *this;
}

const TCounters &TCounters::recount(const CheckIn::TPaxGrpItem& grp,
                                    const CheckIn::TSimplePaxList& prior_paxs,
                                    const CheckIn::TSimplePaxList& curr_paxs,
                                    const std::string& whence)
{
  LogTrace(TRACE5) << __FUNCTION__ << ": point_id=" << grp.point_dep << ", whence=" << whence;

  TRegDifferenceMap regDifferenceMap;
  regDifferenceMap.getDifference(grp, prior_paxs, curr_paxs);
  if (regDifferenceMap.empty()) return *this;

  if (!setFlt(grp.point_dep))
  {
    TCounters::deleteInitially(grp.point_dep);
    return *this;
  }

  lockInitially(grp.point_dep);
  regDifferenceMap.apply(flt(), pr_tranz_reg());

  recountFinally();

  return *this;
}

int TCounters::totalRegisteredPassengers(int point_id)
{
  TCachedQuery Qry("SELECT SUM(tranzit)+SUM(ok)+SUM(goshow)+ "
                   "       SUM(jmp_tranzit)+SUM(jmp_ok)+SUM(jmp_goshow) AS reg FROM counters2 "
                   "WHERE point_dep=:point_id",
                   QParams() << QParam("point_id", otInteger, point_id));
  Qry.get().Execute();
  if (Qry.get().Eof) return 0;
  return Qry.get().FieldAsInteger("reg");
}

void AvailableByClasses::getSummaryResult(int& need, int& avail) const
{
  need=0;
  avail=0;
  for(const auto& i : *this)
  {
    const CheckIn::AvailableByClass& item=i.second;
    need+=item.need;
    if (item.avail!=ASTRA::NoExists && item.avail<item.need)
      avail+=item.avail;
    else
      avail+=item.need;
  }
}

void AvailableByClasses::dump() const
{
  for(const auto& i: *this)
  {
    const AvailableByClass& item=i.second;
    LogTrace(TRACE5) << "cl=" << (item.is_jmp?"JMP":item.cl)
                     << ", need=" << item.need
                     << ", avail=" << (item.avail==ASTRA::NoExists?"NoExists":IntToString(item.avail));
  }
}

void CheckCounters(int point_dep,
                   int point_arv,
                   const string &cl,
                   ASTRA::TPaxStatus grp_status,
                   const TCFG &cfg,
                   bool free_seating,
                   bool is_jmp,
                   int &free)
{
    free=ASTRA::NoExists;
    if (cfg.empty() && free_seating) return;
    if (grp_status==ASTRA::psCrew) return;
    //???????? ??????? ????????? ????
    TQuery Qry(&OraSession);
    Qry.Clear();
    if (!is_jmp)
    {
      Qry.SQLText=
        "SELECT free_ok, free_goshow, nooccupy, jmp_nooccupy FROM counters2 "
        "WHERE point_dep=:point_dep AND point_arv=:point_arv AND class=:class ";
      Qry.CreateVariable("class", otString, cl);
    }
    else
    {
      Qry.SQLText=
        "SELECT free_ok, free_goshow, nooccupy, jmp_nooccupy FROM counters2 "
        "WHERE point_dep=:point_dep AND point_arv=:point_arv AND rownum<2 ";
    }
    Qry.CreateVariable("point_dep", otInteger, point_dep);
    Qry.CreateVariable("point_arv", otInteger, point_arv);

    Qry.Execute();
    if (Qry.Eof)
    {
      ProgTrace(TRACE0,"counters2 empty! (point_dep=%d point_arv=%d cl=%s)",point_dep,point_arv,cl.c_str());
      CheckIn::TCounters().recount(point_dep, CheckIn::TCounters::Total, __FUNCTION__);
      Qry.Execute();
    }
    if (Qry.Eof)
    {
      free=0;
      return;
    };

    if (!is_jmp)
    {
      TTripStages tripStages( point_dep );
      TStage ckin_stage =  tripStages.getStage( stCheckIn );
      switch (grp_status)
      {
        case ASTRA::psTransit:
                   free=Qry.FieldAsInteger("nooccupy");
                   break;
        case ASTRA::psCheckin:
        case ASTRA::psTCheckin:
                   if (ckin_stage==sNoActive ||
                       ckin_stage==sPrepCheckIn ||
                       ckin_stage==sOpenCheckIn)
                     free=Qry.FieldAsInteger("free_ok");
                   else
                     free=Qry.FieldAsInteger("nooccupy");
                   break;
        default:   if (ckin_stage==sNoActive ||
                       ckin_stage==sPrepCheckIn ||
                       ckin_stage==sOpenCheckIn)
                     free=Qry.FieldAsInteger("free_goshow");
                   else
                     free=Qry.FieldAsInteger("nooccupy");
      }
    }
    else
      free=Qry.FieldAsInteger("jmp_nooccupy");

    if (free<0) free=0;
}

void CheckCounters(const CheckIn::TPaxGrpItem& grp,
                   bool free_seating,
                   AvailableByClasses& availableByClasses)
{
  for(auto& i : availableByClasses)
    CheckCounters(grp.point_dep,grp.point_arv,i.second.cl,grp.status,TCFG(grp.point_dep),free_seating,i.second.is_jmp,i.second.avail);
}

void CheckCounters(int point_dep,
                   int point_arv,
                   ASTRA::TPaxStatus grp_status,
                   const TCFG &cfg,
                   bool free_seating,
                   AvailableByClasses& availableByClasses)
{
  for(auto& i : availableByClasses)
    CheckCounters(point_dep,point_arv,i.second.cl,grp_status,cfg,free_seating,i.second.is_jmp,i.second.avail);
}

} //namespace CheckIn

#include <serverlib/EdiHelpManager.h>
#include <serverlib/query_runner.h>
#include <tclmon/internal_msgid.h>

namespace Timing
{

void Points::start(const std::string& _what, const boost::optional<int>& _seg_no)
{
  try
  {
    Intervals& intervals=emplace(Point(_what, _seg_no), Intervals()).first->second;
    intervals.emplace_back(boost::posix_time::microsec_clock::local_time(), boost::posix_time::ptime());
  }
  catch(...) {}
}

void Points::finish(const std::string& _what, const boost::optional<int>& _seg_no)
{
  try
  {
    Intervals& intervals=emplace(Point(_what, _seg_no), Intervals()).first->second;
    if (!intervals.empty() && intervals.back().second.is_not_a_date_time())
      intervals.back().second=boost::posix_time::microsec_clock::local_time();
    else
      intervals.emplace_back(boost::posix_time::ptime(), boost::posix_time::microsec_clock::local_time());
  }
  catch(...) {}
}

Points::~Points()
{
  try
  {
    TDateTime now=BASIC::date_time::NowUTC();
    for(const auto& p : *this)
    {
      boost::optional<long> msecs;
      msecs=0;
      msecs=boost::none;
      for(const auto& i : p.second)
        if (!i.first.is_not_a_date_time() && !i.second.is_not_a_date_time())
        {
          if (!msecs) msecs=0;
          msecs.get()+=(i.second - i.first).total_milliseconds();
        }
        else
        {
          msecs=boost::none;
          break;
        }
      LogTrace(TRACE5) << _traceTitle << ": " << ServerFramework::getQueryRunner().getEdiHelpManager().msgId().asString()
                       << "|" << p.first.what
                       << "|" << (p.first.seg_no?IntToString(p.first.seg_no.get()):"")
                       << "|" << (msecs?IntToString(msecs.get()):"")
                       << "|" << std::fixed << now;
    }
  }
  catch(...) {}
}

} //namespace Timing

