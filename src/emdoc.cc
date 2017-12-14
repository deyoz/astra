#include "emdoc.h"
#include "edi_utils.h"
#include "astra_utils.h"
#include "qrys.h"
#include "date_time.h"
#include "alarms.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

using namespace std;
using namespace BASIC::date_time;
using namespace ASTRA;
using namespace Ticketing;
using namespace AstraEdifact;

namespace PaxASVCList
{

string GetSQL(const TListType ltype)
{
  ostringstream sql;

  if (ltype==unboundByPointId ||
      ltype==unboundByPaxId ||
      ltype==allWithTknByPointId)
  {
    sql << "SELECT a.rfic, \n"
           "       a.rfisc, \n"
           "       a.service_quantity, \n"
           "       a.ssr_code, \n"
           "       a.service_name, \n"
           "       a.emd_type, \n"
           "       a.emd_no, \n"
           "       a.emd_coupon, \n"
           "       a.transfer_num, \n"
           "       a.et_no, \n"
           "       a.et_coupon, \n"
           "       a.emd_no_base \n";
    if (ltype==allWithTknByPointId)
      sql << ",      a.grp_id, \n"
             "       a.pax_id, \n"
             "       a.surname, \n"
             "       a.name, \n"
             "       a.pers_type, \n"
             "       a.reg_no, \n"
             "       a.ticket_no, \n"
             "       a.coupon_no, \n"
             "       a.ticket_rem, \n"
             "       a.ticket_confirm, \n"
             "       a.pr_brd, \n"
             "       a.refuse, \n"
             "       b.grp_id AS paid_bag_emd_grp_id \n";
    sql << "FROM \n"
           " ( \n";
    for(int pass=0; pass<2; pass++)
    {
      sql << "  SELECT pax_grp.grp_id, \n"
             "         e.rfic, \n"
             "         e.rfisc, \n"
             "         e.service_quantity, \n"
             "         e.ssr_code, \n"
             "         e.service_name, \n"
             "         e.emd_type, \n"
             "         e.emd_no, \n"
             "         e.emd_coupon, \n";
      if (ltype==allWithTknByPointId)
        sql << "         pax.pax_id, \n"
               "         pax.surname, \n"
               "         pax.name, \n"
               "         pax.pers_type, \n"
               "         pax.reg_no, \n"
               "         pax.ticket_no, \n"
               "         pax.coupon_no, \n"
               "         pax.ticket_rem, \n"
               "         pax.ticket_confirm, \n"
               "         pax.pr_brd, \n"
               "         pax.refuse, \n";
      if (pass==0)
        sql << "         0 AS transfer_num, \n"
               "         NULL AS et_no, \n"
               "         NULL AS et_coupon, \n"
               "         NULL AS emd_no_base \n"
               "  FROM pax_grp, pax, pax_asvc e \n";
      else
        sql << "         e.transfer_num, \n"
               "         e.et_no, \n"
               "         e.et_coupon, \n"
               "         e.emd_no_base \n"
               "  FROM pax_grp, pax, pax_emd e \n";
      sql << "  WHERE pax_grp.grp_id=pax.grp_id AND \n"
             "        pax.pax_id=e.pax_id AND \n";
      if (ltype==unboundByPointId ||
          ltype==allWithTknByPointId)
        sql << "        pax_grp.point_dep=:id AND \n";
      if (ltype==unboundByPaxId)
        sql << "        pax.pax_id=:id AND \n";
      sql << "        pax_grp.status NOT IN ('E') AND \n"
             "        pax.refuse IS NULL \n";
      if (pass==0)
        sql << "        AND (e.emd_type='S' OR e.emd_type='A' AND NOT EXISTS(SELECT 0 FROM pax_emd WHERE pax_id=pax.pax_id)) \n"
               "  UNION \n";
    };
    sql << " ) a, \n"
           " (SELECT pax_grp.grp_id, paid_bag_emd.emd_no, paid_bag_emd.emd_coupon \n";
    if (ltype==unboundByPointId ||
        ltype==allWithTknByPointId)
      sql <<
           "  FROM pax_grp, paid_bag_emd \n"
           "  WHERE pax_grp.grp_id=paid_bag_emd.grp_id AND \n"
           "        pax_grp.point_dep=:id AND \n";
    if (ltype==unboundByPaxId)
      sql <<
           "  FROM pax_grp, paid_bag_emd, pax \n"
           "  WHERE pax_grp.grp_id=paid_bag_emd.grp_id AND \n"
           "        pax.grp_id=pax_grp.grp_id AND \n"
           "        pax.pax_id=:id AND \n";

    sql << "        pax_grp.excess_wt IS NULL AND pax_grp.excess_pc IS NULL AND \n"
           "        pax_grp.status NOT IN ('E') \n"
           "  UNION \n"
           "  SELECT pax_grp.grp_id, service_payment.doc_no, service_payment.doc_coupon \n";
    if (ltype==unboundByPointId ||
        ltype==allWithTknByPointId)
      sql <<
           "  FROM pax_grp, service_payment \n"
           "  WHERE pax_grp.grp_id=service_payment.grp_id AND \n"
           "        pax_grp.point_dep=:id AND \n";
    if (ltype==unboundByPaxId)
      sql <<
           "  FROM pax_grp, service_payment, pax \n"
           "  WHERE pax_grp.grp_id=service_payment.grp_id AND \n"
           "        pax.grp_id=pax_grp.grp_id AND \n"
           "        pax.pax_id=:id AND \n";

    sql << "        service_payment.doc_type IN ('EMDA', 'EMDS') AND \n"
           "        NOT(pax_grp.excess_wt IS NULL AND pax_grp.excess_pc IS NULL) AND \n"
           "        pax_grp.status NOT IN ('E') \n"
//           "  UNION \n"
//           "  SELECT pax_grp.grp_id, pax_services_auto.emd_no, pax_services_auto.emd_coupon \n"
//           "  FROM pax_grp, pax_services_auto, pax \n"
//           "  WHERE pax.pax_id=pax_services_auto.pax_id AND \n"
//           "        pax.grp_id=pax_grp.grp_id AND \n";
//    if (ltype==unboundByPointId ||
//        ltype==allWithTknByPointId)
//      sql <<
//           "        pax_grp.point_dep=:id AND \n";
//    if (ltype==unboundByPaxId)
//      sql <<
//           "        pax.pax_id=:id AND \n";
//    sql << "        pax_services_auto.emd_type IN ('A', 'S') AND \n"
//           "        NOT(pax_grp.excess_wt IS NULL AND pax_grp.excess_pc IS NULL) AND \n"
//           "        pax_grp.status NOT IN ('E') \n"
           " ) b \n"
           "WHERE a.grp_id=b.grp_id(+) AND \n"
           "      a.emd_no=b.emd_no(+) AND \n"
           "      a.emd_coupon=b.emd_coupon(+) \n";
    if (ltype==unboundByPointId ||
        ltype==unboundByPaxId)
      sql << "AND b.grp_id IS NULL \n";
  };

  if (ltype==oneWithTknByGrpId ||
      ltype==oneWithTknByPaxId)
  {
    for(int pass=0; pass<2; pass++)
    {
      sql << "SELECT pax.grp_id, \n"
             "       pax.pax_id, \n"
             "       pax.surname, \n"
             "       pax.name, \n"
             "       pax.pers_type, \n"
             "       pax.reg_no, \n"
             "       pax.ticket_no, \n"
             "       pax.coupon_no, \n"
             "       pax.ticket_rem, \n"
             "       pax.ticket_confirm, \n"
             "       pax.pr_brd, \n"
             "       pax.refuse, \n"
             "       e.rfic, \n"
             "       e.rfisc, \n"
             "       e.service_quantity, \n"
             "       e.ssr_code, \n"
             "       e.service_name, \n"
             "       e.emd_type, \n"
             "       e.emd_no, \n"
             "       e.emd_coupon, \n";

      if (pass==0)
      {
        sql << "       0 AS transfer_num, \n"
               "       NULL AS et_no, \n"
               "       NULL AS et_coupon, \n"
               "       NULL AS emd_no_base \n"
               "FROM pax, pax_asvc e \n";
      }
      else
      {
        sql << "       e.transfer_num, \n"
               "       e.et_no, \n"
               "       e.et_coupon, \n"
               "       e.emd_no_base \n"
               "FROM pax, pax_emd e \n";
      };
      sql << "WHERE pax.pax_id=e.pax_id AND \n";
      if (ltype==oneWithTknByGrpId)
        sql << "      pax.grp_id=:grp_id AND \n";
      else
        sql << "      pax.pax_id=:pax_id AND \n";
      sql << "      e.emd_no=:emd_no AND e.emd_coupon=:emd_coupon \n";
      if (pass==0)
        sql << "      AND (e.emd_type='S' OR e.emd_type='A' AND NOT EXISTS(SELECT 0 FROM pax_emd WHERE pax_id=pax.pax_id)) \n"
               "UNION \n";
      else
        sql << "ORDER BY reg_no \n";
    }
  }

  if (ltype==allByPaxId)
  {
    for(int pass=0; pass<2; pass++)
    {
      sql << "SELECT e.rfic, \n"
             "       e.rfisc, \n"
             "       e.service_quantity, \n"
             "       e.ssr_code, \n"
             "       e.service_name, \n"
             "       e.emd_type, \n"
             "       e.emd_no, \n"
             "       e.emd_coupon, \n";

      if (pass==0)
      {
        sql << "       0 AS transfer_num, \n"
               "       NULL AS et_no, \n"
               "       NULL AS et_coupon, \n"
               "       NULL AS emd_no_base \n"
               "FROM pax_asvc e \n";
      }
      else
      {
        sql << "       e.transfer_num, \n"
               "       e.et_no, \n"
               "       e.et_coupon, \n"
               "       e.emd_no_base \n"
               "FROM pax_emd e \n";
      };
      sql << "WHERE pax_id=:id \n";
      if (pass==0)
        sql << "      AND (e.emd_type='S' OR e.emd_type='A' AND NOT EXISTS(SELECT 0 FROM pax_emd WHERE pax_id=e.pax_id)) \n"
               "UNION \n";
    }
  }
  //ProgTrace(TRACE5, "%s: SQL=\n%s", __FUNCTION__, sql.str().c_str());
  return sql.str();
}

void printSQLs()
{
  ProgTrace(TRACE5, "%s: SQL(unboundByPointId)=\n%s", __FUNCTION__, GetSQL(unboundByPointId).c_str());
  ProgTrace(TRACE5, "%s: SQL(unboundByPaxId)=\n%s", __FUNCTION__, GetSQL(unboundByPaxId).c_str());
  ProgTrace(TRACE5, "%s: SQL(allByPaxId)=\n%s", __FUNCTION__, GetSQL(allByPaxId).c_str());
  ProgTrace(TRACE5, "%s: SQL(allWithTknByPointId)=\n%s", __FUNCTION__, GetSQL(allWithTknByPointId).c_str());
  ProgTrace(TRACE5, "%s: SQL(oneWithTknByGrpId)=\n%s", __FUNCTION__, GetSQL(oneWithTknByGrpId).c_str());
  ProgTrace(TRACE5, "%s: SQL(oneWithTknByPaxId)=\n%s", __FUNCTION__, GetSQL(oneWithTknByPaxId).c_str());
}

void GetUnboundBagEMD(int id, multiset<CheckIn::TPaxASVCItem> &asvc, bool is_pax_id, bool only_one)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText = PaxASVCList::GetSQL(is_pax_id?PaxASVCList::unboundByPaxId:
                                              PaxASVCList::unboundByPointId);
  Qry.CreateVariable( "id", otInteger, id );
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    CheckIn::TPaxASVCItem item;
    item.fromDB(Qry);
    std::set<ASTRA::TRcptServiceType> service_types;
    item.rcpt_service_types(service_types);
    if (service_types.find(ASTRA::rstExcess)==service_types.end() &&
        service_types.find(ASTRA::rstPaid)==service_types.end()) continue; //когда это уберется, тогда переименовать процедуру в GetUnboundEMD
    asvc.insert(item);
    if (only_one) break;
  };
};

void GetUnboundBagEMD(int point_id, multiset<CheckIn::TPaxASVCItem> &asvc)
{
  GetUnboundBagEMD(point_id, asvc, false, false);
};

bool ExistsUnboundBagEMD(int point_id)
{
  multiset<CheckIn::TPaxASVCItem> asvc;
  GetUnboundBagEMD(point_id, asvc, false, true);
  return !asvc.empty();
};

bool ExistsPaxUnboundBagEMD(int pax_id)
{
  multiset<CheckIn::TPaxASVCItem> asvc;
  GetUnboundBagEMD(pax_id, asvc, true, true);
  return !asvc.empty();
};

}; //namespace PaxASVCList

const TEMDCtxtItem& TEMDCtxtItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;

  TPaxCtxt::toXML(node);

  NewTextChild(node, "doc_no", emd_no);
  emd_coupon!=NoExists?NewTextChild(node, "coupon_no", emd_coupon):
                       NewTextChild(node, "coupon_no");
  status!=CouponStatus::Unavailable?NewTextChild(node, "coupon_status", status->dispCode()):
                                    NewTextChild(node, "coupon_status", FNull);
  NewTextChild(node, "associated", (int)(action==CpnStatAction::associate));
  NewTextChild(node, "associated_no", pax.tkn.no);
  pax.tkn.coupon !=NoExists?NewTextChild(node, "associated_coupon", pax.tkn.coupon):
                            NewTextChild(node, "associated_coupon");
  return *this;
};

TEMDCtxtItem& TEMDCtxtItem::fromXML(xmlNodePtr node, xmlNodePtr originNode)
{
  clear();
  if (node==NULL) return *this;

  TPaxCtxt::fromXML(node);
  if (originNode!=NULL) TOriginCtxt::fromXML(originNode);

  xmlNodePtr node2=node->children;
  emd_no=NodeAsStringFast("doc_no", node2);
  emd_coupon=NodeAsIntegerFast("coupon_no", node2, NoExists);
  action=NodeAsIntegerFast("associated", node2)!=0?CpnStatAction::associate:
                                                   CpnStatAction::disassociate;
  status=NodeIsNULLFast("coupon_status", node2)?CouponStatus(CouponStatus::Unavailable):
                                                CouponStatus(CouponStatus::fromDispCode(NodeAsStringFast("coupon_status", node2)));
  pax.tkn.no=NodeAsStringFast("associated_no", node2);
  pax.tkn.coupon=NodeAsIntegerFast("associated_coupon", node2, NoExists);
  return *this;
};

std::string TEMDCtxtItem::no_str() const
{
  ostringstream s;
  s << emd_no;
  if (emd_coupon!=ASTRA::NoExists)
    s << "/" << emd_coupon;
  else
    s << "/?";
  return s.str();
}

void GetBagEMDDisassocList(const int point_id,
                           const bool in_final_status,
                           std::list<TEMDCtxtItem> &emds)
{
  emds.clear();

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText = PaxASVCList::GetSQL(PaxASVCList::allWithTknByPointId);
  Qry.CreateVariable( "id", otInteger, point_id );
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    CheckIn::TPaxASVCItem asvc;
    asvc.fromDB(Qry);
    std::set<ASTRA::TRcptServiceType> service_types;
    asvc.rcpt_service_types(service_types);
    if (service_types.find(ASTRA::rstExcess)==service_types.end() &&
        service_types.find(ASTRA::rstPaid)==service_types.end()) continue; //когда это уберется, тогда переименовать процедуру в GetEMDDisassocList

    TEMDCtxtItem item;
    item.emd_no=asvc.emd_no;
    item.emd_coupon=asvc.emd_coupon;
    item.paxFromDB(Qry);
    item.point_id=point_id;

    item.status=calcPaxCouponStatus(item.pax.refuse,
                                    item.pax.pr_brd,
                                    in_final_status);
    if (item.status==CouponStatus::Flown) continue; //если финальный статус, то никаких ассоциаций

    if (item.status==CouponStatus::Boarded &&
        Qry.FieldIsNULL("paid_bag_emd_grp_id"))
      item.action=CpnStatAction::disassociate;
    else
      item.action=CpnStatAction::associate;
    emds.push_back(item);
  };
};

void GetEMDStatusList(const int grp_id,
                      const bool in_final_status,
                      const CheckIn::TServicePaymentList &prior_payment,
                      std::list<TEMDCtxtItem> &added_emds,
                      std::list<TEMDCtxtItem> &deleted_emds)
{
  added_emds.clear();
  deleted_emds.clear();

  if (in_final_status) return;

  CheckIn::TServicePaymentList curr_payment;
  curr_payment.fromDB(grp_id);
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText= GetSQL(PaxASVCList::oneWithTknByGrpId);
  Qry.CreateVariable("grp_id", otInteger, grp_id);
  Qry.DeclareVariable("emd_no", otString);
  Qry.DeclareVariable("emd_coupon", otInteger);
  for(int pass=0; pass<2; pass++)
  {
    //1 проход - добавленные EMD
    //2 проход - удаленные EMD
    const CheckIn::TServicePaymentList &payment1 = pass==0?curr_payment:prior_payment;
    const CheckIn::TServicePaymentList &payment2 = pass==0?prior_payment:curr_payment;
    std::list<TEMDCtxtItem> &emds = pass==0?added_emds:deleted_emds;
    for(CheckIn::TServicePaymentList::const_iterator p1=payment1.begin(); p1!=payment1.end(); ++p1)
    {
      CheckIn::TServicePaymentList::const_iterator p2=payment2.begin();
      for(; p2!=payment2.end(); ++p2)
      {
        if (p1->sameDoc(*p2)) break;
      };
      if (p2!=payment2.end()) continue;
      if (!p1->isEMD()) continue; //работаем только с EMD
      if (p1->trfer_num!=0) continue; //работаем только со статусом 'нашего' сегмента

      TEMDCtxtItem item;
      item.emd_no=p1->doc_no;
      item.emd_coupon=p1->doc_coupon;

      Qry.SetVariable("emd_no", p1->doc_no);
      Qry.SetVariable("emd_coupon", p1->doc_coupon);
      Qry.Execute();
      if (!Qry.Eof)
      {
        //вообще когда одинаковые EMD принадлежат разным пассажиром - это плохо и наверное надо что-то с этим делать
        //пока выбираем пассажира с более ранним регистрационным номером
        item.paxFromDB(Qry);
      };

      emds.push_back(item);
    };
  };
};

const TEMDocItem& TEMDocItem::toDB(const TEdiAction ediAction) const
{
  if (empty())
    throw EXCEPTIONS::Exception("TEMDocItem::toDB: empty emd");

  QParams QryParams;
  QryParams << QParam("doc_no", otString, emd_no)
            << QParam("coupon_no", otInteger, emd_coupon)
            << (point_id!=ASTRA::NoExists?QParam("point_id", otInteger, point_id):
                                          QParam("point_id", otInteger, FNull));
  switch(ediAction)
  {
    case ChangeOfStatus:
      QryParams << (status!=CouponStatus::Unavailable?QParam("coupon_status", otString, status->dispCode()):
                                                      QParam("coupon_status", otString, FNull))
                << QParam("change_status_error", otString, change_status_error.substr(0,100));
      break;
    case SystemUpdate:
      QryParams << QParam("associated", otInteger, (int)(action==CpnStatAction::associate))
                << QParam("associated_no", otString, et_no)
                << QParam("associated_coupon", otInteger, et_coupon)
                << QParam("system_update_error", otString, system_update_error.substr(0,100));
      break;
  };

  if (ediAction==ChangeOfStatus)
  {
    const char* sql=
        "BEGIN "
        "  UPDATE emdocs "
        "  SET change_status_error=:change_status_error, coupon_status=:coupon_status, "
        "      point_id=:point_id "
        "  WHERE doc_no=:doc_no AND coupon_no=:coupon_no; "
        "  IF SQL%NOTFOUND THEN "
        "    INSERT INTO emdocs(doc_no, coupon_no, coupon_status, change_status_error, associated, point_id) "
        "    VALUES(:doc_no, :coupon_no, :coupon_status, :change_status_error, 1, :point_id); "
        "  END IF; "
        "END;";

    TCachedQuery Qry(sql, QryParams);
    Qry.get().Execute();
  };
  if (ediAction==SystemUpdate)
  {
    const char* sql=
        "BEGIN "
        "  UPDATE emdocs "
        "  SET system_update_error=:system_update_error, "
        "      associated=:associated, associated_no=:associated_no, associated_coupon=:associated_coupon, "
        "      point_id=:point_id "
        "  WHERE doc_no=:doc_no AND coupon_no=:coupon_no; "
        "  IF SQL%NOTFOUND THEN "
        "    INSERT INTO emdocs(doc_no, coupon_no, system_update_error, associated, associated_no, associated_coupon, point_id) "
        "    VALUES(:doc_no, :coupon_no, :system_update_error, :associated, :associated_no, :associated_coupon, :point_id); "
        "  END IF; "
        "END;";

    TCachedQuery Qry(sql, QryParams);
    Qry.get().Execute();
  };
  return *this;
};

TEMDocItem& TEMDocItem::fromDB(const string &_emd_no,
                               const int _emd_coupon,
                               const bool lock)
{
  clear();

  if (_emd_no.empty() || _emd_coupon==ASTRA::NoExists)
    throw EXCEPTIONS::Exception("TEMDocItem::fromDB: empty emd");

  QParams QryParams;
  QryParams << QParam("doc_no", otString, _emd_no);
  QryParams << QParam("coupon_no", otInteger, _emd_coupon);

  const string sql="SELECT coupon_status, associated, associated_no, associated_coupon, "
                   "       change_status_error, system_update_error, point_id "
                   "FROM emdocs "
                   "WHERE doc_no=:doc_no AND coupon_no=:coupon_no";

  TCachedQuery Qry(sql+(lock?" FOR UPDATE":""), QryParams);

  Qry.get().Execute();
  if (!Qry.get().Eof)
  {
    emd_no=_emd_no;
    emd_coupon=_emd_coupon;

    et_no=Qry.get().FieldAsString("associated_no");
    et_coupon=Qry.get().FieldIsNULL("associated_coupon")?ASTRA::NoExists:
                                                         Qry.get().FieldAsInteger("associated_coupon");

    status=Qry.get().FieldIsNULL("coupon_status")?CouponStatus(CouponStatus::Unavailable):
                                                  CouponStatus(CouponStatus::fromDispCode(Qry.get().FieldAsString("coupon_status")));
    action=Qry.get().FieldAsInteger("associated")!=0?CpnStatAction::associate:
                                                     CpnStatAction::disassociate;

    change_status_error=Qry.get().FieldAsString("change_status_error");
    system_update_error=Qry.get().FieldAsString("system_update_error");

    point_id=Qry.get().FieldIsNULL("point_id")?ASTRA::NoExists:
                                               Qry.get().FieldAsInteger("point_id");
  };

  return *this;
};

bool ActualEMDEvent(const TEMDCtxtItem &EMDCtxt,
                    const xmlNodePtr eventCtxtNode,
                    TLogLocale &event)
{
  event.clear();

  if (EMDCtxt.paxUnknown() || eventCtxtNode==NULL) return false;

  TLogLocale ctxt_event;
  ctxt_event.fromXML(eventCtxtNode);

  if (ctxt_event.ev_time==NoExists || ctxt_event.ev_order==NoExists) return false;


  QParams QryParams;
  QryParams << QParam("pax_id", otInteger, EMDCtxt.pax.id)
            << QParam("emd_no", otString, EMDCtxt.emd_no)
            << QParam("emd_coupon", otInteger, EMDCtxt.emd_coupon);

  TCachedQuery Qry(PaxASVCList::GetSQL(PaxASVCList::oneWithTknByPaxId), QryParams);
  Qry.get().Execute();
  if (Qry.get().Eof) return false;

  TEMDCtxtItem EMDCtxtActual(EMDCtxt);
  EMDCtxtActual.paxFromDB(Qry.get());

  if (EMDCtxtActual.pax.grp_id==EMDCtxt.pax.grp_id &&
      EMDCtxtActual.pax.reg_no==EMDCtxt.pax.reg_no) return false;

  QParams DelQryParams;
  DelQryParams << QParam("ev_time", otDate, ctxt_event.ev_time)
               << QParam("ev_order", otInteger, ctxt_event.ev_order);

  TCachedQuery DelQry("DELETE FROM events_bilingual WHERE time=:ev_time AND ev_order=:ev_order", DelQryParams);
  DelQry.get().Execute();

  event.fromXML(eventCtxtNode);
  event.ev_type=ASTRA::evtPay;
  event.id1=EMDCtxtActual.point_id;
  event.id2=EMDCtxtActual.pax.reg_no;
  event.id3=EMDCtxtActual.pax.grp_id;
  return true;
};

#include "astra_emd.h"
#include "tlg/emd_edifact.h"

const TPaxEMDItem& TPaxEMDItem::toDB(TQuery &Qry) const
{
  CheckIn::TPaxASVCItem::toDB(Qry);
  Qry.SetVariable("transfer_num", trfer_num);
  Qry.SetVariable("et_no", et_no);
  if (et_coupon!=ASTRA::NoExists)
    Qry.SetVariable("et_coupon", et_coupon);
  else
    Qry.SetVariable("et_coupon", FNull);
  Qry.SetVariable("emd_no_base", emd_no_base);
  return *this;
}

TPaxEMDItem& TPaxEMDItem::fromDB(TQuery &Qry)
{
  clear();
  CheckIn::TPaxASVCItem::fromDB(Qry);
  trfer_num=Qry.FieldAsInteger("transfer_num");
  et_no=Qry.FieldAsString("et_no");
  if (!Qry.FieldIsNULL("et_coupon"))
    et_coupon=Qry.FieldAsInteger("et_coupon");
  emd_no_base=Qry.FieldAsString("emd_no_base");
  return *this;
}

std::string TPaxEMDItem::traceStr() const
{
  std::ostringstream s;
  s << RFIC << " | "
    << RFISC << " | "
    << service_name << " | "
    << emd_type << " | "
    << emd_no;
  if (emd_coupon!=NoExists)
    s << "/" << emd_coupon;
  s << " | " << emd_no_base;
  s << " | " << et_no;
  if (et_coupon!=NoExists)
    s << "/" << et_coupon;
  return s.str();
}

bool TPaxEMDItem::valid() const
{
  return (!RFIC.empty() &&
          !RFISC.empty() &&
          !service_name.empty() &&
          !emd_type.empty() &&
          !emd_no.empty() &&
          emd_coupon!=NoExists &&
          !et_no.empty() &&
          et_coupon!=NoExists &&
          !emd_no_base.empty());
}

void GetPaxEMD(int pax_id, multiset<TPaxEMDItem> &emds)
{
  emds.clear();
  TCachedQuery Qry(PaxASVCList::GetSQL(PaxASVCList::allByPaxId),
                   QParams() << QParam("id", otInteger, pax_id));
  Qry.get().Execute();
  for(; !Qry.get().Eof; Qry.get().Next())
   emds.insert(TPaxEMDItem().fromDB(Qry.get()));
}

void PaxEMDToDB(int pax_id, const list<TPaxEMDItem> &emds)
{
  TCachedQuery Qry(
    "INSERT INTO pax_emd(pax_id, transfer_num, rfic, rfisc, service_quantity, ssr_code, service_name, emd_type, "
    "  emd_no, emd_coupon, et_no, et_coupon, emd_no_base) "
    "VALUES(:pax_id, :transfer_num, :rfic, :rfisc, :service_quantity, :ssr_code, :service_name, :emd_type, "
    "  :emd_no, :emd_coupon, :et_no, :et_coupon, :emd_no_base)",
    QParams() << QParam("pax_id", otInteger, pax_id)
              << QParam("transfer_num", otInteger)
              << QParam("rfic", otString)
              << QParam("rfisc", otString)
              << QParam("service_quantity", otInteger)
              << QParam("ssr_code", otString)
              << QParam("service_name", otString)
              << QParam("emd_type", otString)
              << QParam("emd_no", otString)
              << QParam("emd_coupon", otInteger)
              << QParam("et_no", otString)
              << QParam("et_coupon", otInteger)
              << QParam("emd_no_base", otString));
  for(list<TPaxEMDItem>::const_iterator e=emds.begin(); e!=emds.end(); ++e)
  {
    e->toDB(Qry.get());
    Qry.get().Execute();
  };

  TPaxAlarmHook::set(Alarm::UnboundEMD, pax_id);
};

void SyncPaxEMD(const CheckIn::TTransferItem &trfer,
                const TPaxEMDItem &emd,
                const CouponStatus &emd_status,
                const set<string> &connected_et_no)
{
  if (trfer.operFlt.airp.empty() ||
      trfer.operFlt.scd_out==NoExists ||
      trfer.airp_arv.empty()) return;
  if (!emd.valid()) return;

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="DELETE FROM pax_emd WHERE emd_no=:emd_no AND emd_coupon=:emd_coupon";
  Qry.CreateVariable("emd_no", otString, emd.emd_no);
  Qry.CreateVariable("emd_coupon", otInteger, emd.emd_coupon);
  Qry.Execute();

  if (emd_status!=CouponStatus::OriginalIssue &&
      emd_status!=CouponStatus::Checked &&
      emd_status!=CouponStatus::Boarded &&
      emd_status!=CouponStatus::Flown) return;

  list<TPaxEMDItem> emds(1, emd);

  set< pair<int/*grp_id*/, int/*pax_id*/> > ids;

  Qry.Clear();
  Qry.SQLText="SELECT grp_id, pax_id FROM pax WHERE ticket_no=:et_no";
  Qry.DeclareVariable("et_no", otString);
  for(set<string>::const_iterator no=connected_et_no.begin(); no!=connected_et_no.end(); ++no)
  {
    Qry.SetVariable("et_no", *no);
    Qry.Execute();
    for(; !Qry.Eof; Qry.Next())
    {
      int grp_id=Qry.FieldAsInteger("grp_id");
      int pax_id=Qry.FieldAsInteger("pax_id");
      ids.insert(make_pair(grp_id, pax_id));
      map<int, CheckIn::TCkinPaxTknItem> tkns;
      CheckIn::GetTCkinTickets(pax_id, tkns);
      for(map<int, CheckIn::TCkinPaxTknItem>::const_iterator i=tkns.begin(); i!=tkns.end(); ++i)
        ids.insert(make_pair(i->second.grp_id, i->second.pax_id));
    };
  };

  for(set< pair<int/*grp_id*/, int/*pax_id*/> >::const_iterator i=ids.begin(); i!=ids.end(); ++i)
  {
    try
    {
      TTrferRoute route;
      route.GetRoute(i->first, trtWithFirstSeg);
      int trfer_num=0;
      for(TTrferRoute::iterator t=route.begin(); t!=route.end(); ++t, trfer_num++)
      {
        modf(t->operFlt.scd_out, &(t->operFlt.scd_out));
        if (t->operFlt.airp==trfer.operFlt.airp &&
            t->operFlt.scd_out==trfer.operFlt.scd_out &&
            t->airp_arv==trfer.airp_arv)
        {
          emds.front().trfer_num=trfer_num;
          PaxEMDToDB(i->second, emds);
          break;
        };
      }
    }
    catch(std::exception &e)
    {
      ProgError(STDLOG, "%s: %s", __FUNCTION__, e.what());
    };

  };
}

/*
const std::string amadeus_emd=
    "UNB+IATA:1+ETICK+ASTRA+171213:1125+ASTRA00EZ50001+++T'"
    "UNH+1+TKCRES:06:1:IA+ASTRA00EZ5'"
    "MSG+:791+3'"
    "TIF+SIDOROV:A+SIDOR MR'"
    "TAI+7906+1005RE/SU:B'"
    "RCI+1A:S4XL4U:1+UT:6K6126:1'"
    "MON+B:4000:RUB+T:4000:RUB'"
    "FOP+CA:3:4000'"
    "PTK+++131217'"
    "ORG+1A:MUC+00040655:021648+MOW++T+RU+A1005RESU'"
    "EQN+1:TD'"
    "IFT+4:39+MOSCOW+VIP CORPORATE TRAVEL'"
    "IFT+4:5+9652600172'"
    "IFT+4:15:1+SGC UT MOW UT GOJ4000RUB4000END'"
    "IFT+4:47+BAGGAGE'"
    "IFT+4:733:1'"
    "PTS+++++C'"
    "TKT+2982903207956:J:1'"
    "CPN+1:I::E'"
    "TVL++SGC+VKO+UT'"
    "PTS++++++0GP'"
    "IFT+4:47+PIECE OF BAG UPTO23KG 203LCM'"
    "CPN+2:I::E'"
    "TVL++VKO+GOJ+UT'"
    "PTS++++++0GP'"
    "IFT+4:47+PIECE OF BAG UPTO23KG 203LCM'"
    "TKT+2982903207956:J::4::2981085213369'"
    "CPN+1:::::::1::702'"
    "PTS'"
    "CPN+2:::::::2::702'"
    "PTS'"
    "UNT+31+1'"
    "UNZ+1+ASTRA00EZ50001'";
*/

boost::optional<Itin> getEtDispCouponItin(const Ticketing::TicketNum_t& ticknum,
                                          const Ticketing::CouponNum_t& cpnnum);

void handleEmdDispResponse(const edifact::RemoteResults& remRes)
{
  std::list<Emd> emdList = EmdEdifactReader::readList(remRes.tlgSource());
  for(list<Emd>::const_iterator e=emdList.begin(); e!=emdList.end(); ++e)
  {
    const Emd &emd=*e;
    set<string> connected_et_no;
    set<string> connected_emd_no;
    for(int pass=0; pass<2; pass++)
    {
      //1 проход: набираем connected_et_no
      //2 проход: привязываем через SyncPaxEMD
      for(list<EmdTicket>::const_iterator t=emd.lTicket().begin(); t!=emd.lTicket().end(); ++t)
      {
        const EmdTicket &emdTick=*t;
        for(list<EmdCoupon>::const_iterator c=emdTick.lCpn().begin(); c!=emdTick.lCpn().end(); ++c)
        {
          const EmdCoupon &emdCpn=*c;
          if(!emdCpn.haveItin()) continue; //не имеем данных о сегменте

          if (pass==0)
          {
            if(!emdCpn.tickNum().empty())
              connected_emd_no.insert(emdCpn.tickNum().get());
            if(!emdCpn.associatedTickNum().empty())
              connected_et_no.insert(emdCpn.associatedTickNum().get());
          }
          else
          {
            CheckIn::TTransferItem trferItem;
            trferItem.operFlt.airline=ElemToElemId(etAirline, emdCpn.itin().airCode(), trferItem.operFlt.airline_fmt);
            trferItem.operFlt.flt_no=emdCpn.itin().flightnum();
            trferItem.operFlt.airp=ElemToElemId(etAirp, emdCpn.itin().depPointCode(), trferItem.operFlt.airp_fmt);
            if (!emdCpn.itin().date1().is_special())
              trferItem.operFlt.scd_out=BoostToDateTime(emdCpn.itin().date1());
            trferItem.airp_arv=ElemToElemId(etAirp, emdCpn.itin().arrPointCode(), trferItem.airp_arv_fmt);

            TPaxEMDItem emdItem;
            emdItem.RFIC=emd.rfic()->code();
            emdItem.RFISC=emdCpn.rfisc()?emdCpn.rfisc().get().rfisc():"";
            emdItem.service_name=emdCpn.rfisc()?emdCpn.rfisc().get().description():"";
            if (emdItem.service_name.empty()) emdItem.service_name=emdItem.RFISC;
            emdItem.emd_type=(emd.type()==DocType::EmdA ? "A" : "S");
            if (emdItem.RFIC=="C" &&
                emdCpn.itin().luggage().haveLuggage() &&
                emdCpn.itin().luggage()->chargeQualifier()==Ticketing::Baggage::NumPieces &&
                emdCpn.itin().luggage()->quantity()>0)
              emdItem.service_quantity=emdCpn.quantity() * emdCpn.itin().luggage()->quantity();
            else
              emdItem.service_quantity=emdCpn.quantity();

            if(!emdCpn.tickNum().empty())
            {
              emdItem.emd_no=emdCpn.tickNum().get();
              if (emdCpn.num())
                emdItem.emd_coupon=emdCpn.num().get();
            };
            if(!emdCpn.associatedTickNum().empty())
            {
              emdItem.et_no=emdCpn.associatedTickNum().get();
              if (emdCpn.associatedNum())
              {
                emdItem.et_coupon=emdCpn.associatedNum().get();

                if (emdCpn.itin().date1().is_special())
                {
                  boost::optional<Itin> etDispItin=getEtDispCouponItin(emdCpn.associatedTickNum(), emdCpn.associatedNum());
                  if (!etDispItin) LogTrace(TRACE5) << __FUNCTION__
                                                    << ": etDispItin==boost::none"
                                                    << ", emdCpn.associatedTickNum()=" << emdCpn.associatedTickNum()
                                                    << ", emdCpn.associatedNum()=" << emdCpn.associatedNum();

                  if (etDispItin && !etDispItin.get().date1().is_special())
                    trferItem.operFlt.scd_out=BoostToDateTime(etDispItin.get().date1());
                };
              }
            };
            emdItem.emd_no_base=connected_emd_no.empty()?emdItem.emd_no:*(connected_emd_no.begin());

            ProgTrace(TRACE5, "%s: %s %s->%s: %s %s",
                              __FUNCTION__,
                              trferItem.operFlt.scd_out==NoExists?"??.??.??":
                                                                  DateTimeToStr(trferItem.operFlt.scd_out, "dd.mm.yy").c_str(),
                              trferItem.operFlt.airp.c_str(),
                              trferItem.airp_arv.c_str(),
                              emdItem.traceStr().c_str(),
                              emdCpn.couponInfo().status()->dispCode());

            SyncPaxEMD(trferItem, emdItem, emdCpn.couponInfo().status(), connected_et_no);
          }
        }
      }
    }
  }
}


