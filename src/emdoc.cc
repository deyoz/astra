#include "emdoc.h"
#include "edi_utils.h"
#include "astra_utils.h"
#include "qrys.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace std;
using namespace BASIC;
using namespace ASTRA;
using namespace Ticketing;
using namespace AstraEdifact;

namespace PaxASVCList
{

enum TListType {unboundByPointId, unboundByPaxId, allWithTknByPointId, oneWithTknByGrpId, oneWithTknByPaxId};

string GetSQL(const TListType ltype)
{
  ostringstream sql;

  if (ltype==unboundByPointId ||
      ltype==unboundByPaxId ||
      ltype==allWithTknByPointId)
  {
    sql << "SELECT a.rfic, \n"
           "       a.rfisc, \n"
           "       a.ssr_code, \n"
           "       a.service_name, \n"
           "       a.emd_type, \n"
           "       a.emd_no, \n"
           "       a.emd_coupon \n";
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
           " (SELECT pax_grp.grp_id, \n"
           "         pax_asvc.rfic, \n"
           "         pax_asvc.rfisc, \n"
           "         pax_asvc.ssr_code, \n"
           "         pax_asvc.service_name, \n"
           "         pax_asvc.emd_type, \n"
           "         pax_asvc.emd_no, \n"
           "         pax_asvc.emd_coupon \n";
    if (ltype==allWithTknByPointId)
      sql << ",        pax.pax_id, \n"
             "         pax.surname, \n"
             "         pax.name, \n"
             "         pax.pers_type, \n"
             "         pax.reg_no, \n"
             "         pax.ticket_no, \n"
             "         pax.coupon_no, \n"
             "         pax.ticket_rem, \n"
             "         pax.ticket_confirm, \n"
             "         pax.pr_brd, \n"
             "         pax.refuse \n";

    sql << "  FROM pax_grp, pax, pax_asvc \n"
           "  WHERE pax_grp.grp_id=pax.grp_id AND \n"
           "        pax.pax_id=pax_asvc.pax_id AND \n";
    if (ltype==unboundByPointId ||
        ltype==allWithTknByPointId)
      sql << "        pax_grp.point_dep=:id AND \n";
    if (ltype==unboundByPaxId)
      sql << "        pax.pax_id=:id AND \n";
    sql << "        pax_grp.status NOT IN ('E') AND \n"
           "        pax.refuse IS NULL) a, \n"
           " (SELECT pax_grp.grp_id, paid_bag_emd.emd_no, paid_bag_emd.emd_coupon \n";
    if (ltype==unboundByPointId ||
        ltype==allWithTknByPointId)
      sql << "  FROM pax_grp, paid_bag_emd \n"
             "  WHERE pax_grp.grp_id=paid_bag_emd.grp_id AND \n"
             "        pax_grp.point_dep=:id AND \n";
    if (ltype==unboundByPaxId)
      sql << "  FROM pax_grp, paid_bag_emd, pax \n"
             "  WHERE pax_grp.grp_id=paid_bag_emd.grp_id AND \n"
             "        pax.grp_id=pax_grp.grp_id AND \n"
             "        pax.pax_id=:id AND \n";
    sql << "        pax_grp.status NOT IN ('E')) b \n"
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
           "       pax.refuse \n"
           "FROM pax, pax_asvc \n"
           "WHERE pax.pax_id=pax_asvc.pax_id AND \n";
    if (ltype==oneWithTknByGrpId)
      sql << "      pax.grp_id=:grp_id AND \n";
    else
      sql << "      pax.pax_id=:pax_id AND \n";
    sql << "      pax_asvc.emd_no=:emd_no AND pax_asvc.emd_coupon=:emd_coupon \n"
           "ORDER BY pax.reg_no \n";
  }
  //ProgTrace(TRACE5, "%s: SQL=\n %s", __FUNCTION__, sql.str().c_str());
  return sql.str();
}

void GetUnboundEMD(int id, multiset<CheckIn::TPaxASVCItem> &asvc, bool is_pax_id, bool only_one)
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
        service_types.find(ASTRA::rstPaid)==service_types.end()) continue;
    asvc.insert(item);
    if (only_one) break;
  };
};

void GetUnboundEMD(int point_id, multiset<CheckIn::TPaxASVCItem> &asvc)
{
  GetUnboundEMD(point_id, asvc, false, false);
};

bool ExistsUnboundEMD(int point_id)
{
  multiset<CheckIn::TPaxASVCItem> asvc;
  GetUnboundEMD(point_id, asvc, false, true);
  return !asvc.empty();
};

bool ExistsPaxUnboundEMD(int pax_id)
{
  multiset<CheckIn::TPaxASVCItem> asvc;
  GetUnboundEMD(pax_id, asvc, true, true);
  return !asvc.empty();
};

void GetBoundPaidBagEMD(int grp_id, CheckIn::PaidBagEMDList &emd)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
    Qry.SQLText =
    "SELECT paid_bag_emd.bag_type, "
    "       paid_bag_emd.emd_no, "
    "       paid_bag_emd.emd_coupon, "
    "       paid_bag_emd.weight, "
    "       'C' AS rfic, "
    "       NULL AS rfisc, "
    "       NULL AS ssr_code, "
    "       NULL AS service_name, "
    "       'A' AS emd_type "
    "FROM paid_bag_emd "
    "WHERE paid_bag_emd.grp_id=:grp_id";
 /*
    "SELECT paid_bag_emd.bag_type, "
    "       paid_bag_emd.emd_no, "
    "       paid_bag_emd.emd_coupon, "
    "       paid_bag_emd.weight, "
    "       pax_asvc.rfic, "
    "       pax_asvc.rfisc, "
    "       pax_asvc.ssr_code, "
    "       pax_asvc.service_name, "
    "       pax_asvc.emd_type "
    "FROM paid_bag_emd, pax, pax_asvc "
    "WHERE paid_bag_emd.grp_id=pax.grp_id AND "
    "      pax.pax_id=pax_asvc.pax_id AND "
    "      paid_bag_emd.emd_no=pax_asvc.emd_no AND "
    "      paid_bag_emd.emd_coupon=pax_asvc.emd_coupon AND "
    "      paid_bag_emd.grp_id=:grp_id AND "
    "      pax.refuse IS NULL";*/
  Qry.CreateVariable("grp_id", otInteger, grp_id);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    CheckIn::TPaxASVCItem asvcItem;
    CheckIn::TPaidBagEMDItem emdItem;
    asvcItem.fromDB(Qry);
    emdItem.fromDB(Qry);
    std::set<ASTRA::TRcptServiceType> service_types;
    asvcItem.rcpt_service_types(service_types);
    if (service_types.find(ASTRA::rstExcess)==service_types.end() &&
        service_types.find(ASTRA::rstPaid)==service_types.end()) continue;
    emd.push_back(make_pair(asvcItem, emdItem));
  };
};

}; //namespace PaxASVCList


void TEdiOriginCtxt::toXML(xmlNodePtr node)
{
  if (node==NULL) return;
  TReqInfo *reqInfo = TReqInfo::Instance();
  SetProp(node,"desk",reqInfo->desk.code);
  SetProp(node,"user",reqInfo->user.descr);
  SetProp(node,"screen",reqInfo->screen.name);
}

TEdiOriginCtxt& TEdiOriginCtxt::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  screen=NodeAsString("@screen", node);
  user_descr=NodeAsString("@user", node);
  desk_code=NodeAsString("@desk", node);
  return *this;
}

const TEdiPaxCtxt& TEdiPaxCtxt::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;

  NewTextChild(node, "flight", flight);
  point_id       !=NoExists?NewTextChild(node, "point_id", point_id):
                            NewTextChild(node, "point_id");
  grp_id         !=NoExists?NewTextChild(node, "grp_id", grp_id):
                            NewTextChild(node, "grp_id");
  pax.id         !=NoExists?NewTextChild(node, "pax_id", pax.id):
                            NewTextChild(node, "pax_id");
  pax.reg_no     !=NoExists?NewTextChild(node, "reg_no", pax.reg_no):
                            NewTextChild(node, "reg_no");
  NewTextChild(node, "surname", pax.surname);
  NewTextChild(node, "name", pax.name);
  NewTextChild(node, "pers_type", EncodePerson(pax.pers_type));
  return *this;
}

TEdiPaxCtxt& TEdiPaxCtxt::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;

  flight=NodeAsStringFast("flight", node2);
  point_id=NodeAsIntegerFast("point_id", node2, NoExists);
  grp_id=NodeAsIntegerFast("grp_id", node2, NoExists);
  pax.id=NodeAsIntegerFast("pax_id", node2, NoExists);
  pax.reg_no=NodeAsIntegerFast("reg_no", node2, NoExists);
  pax.surname=NodeAsStringFast("surname", node2);
  pax.name=NodeAsStringFast("name", node2);
  pax.pers_type=DecodePerson(NodeAsStringFast("pers_type", node2));
  return *this;
}

TEdiPaxCtxt& TEdiPaxCtxt::paxFromDB(TQuery &Qry)
{
  pax.clear();
  pax.id=Qry.FieldAsInteger("pax_id");
  pax.surname=Qry.FieldAsString("surname");
  pax.name=Qry.FieldAsString("name");
  pax.pers_type=DecodePerson(Qry.FieldAsString("pers_type"));
  pax.refuse=Qry.FieldAsString("refuse");
  pax.reg_no=Qry.FieldAsInteger("reg_no");
  pax.pr_brd=!Qry.FieldIsNULL("pr_brd") && Qry.FieldAsInteger("pr_brd")!=0;
  pax.tkn.fromDB(Qry);
  return *this;
};

const TEMDCtxtItem& TEMDCtxtItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;

  TEdiPaxCtxt::toXML(node);

  NewTextChild(node, "doc_no", asvc.emd_no);
  asvc.emd_coupon!=NoExists?NewTextChild(node, "coupon_no", asvc.emd_coupon):
                            NewTextChild(node, "coupon_no");
  status!=Ticketing::CouponStatus::Unavailable?NewTextChild(node, "coupon_status", status->dispCode()):
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

  TEdiPaxCtxt::fromXML(node);
  if (originNode!=NULL) TEdiOriginCtxt::fromXML(originNode);

  xmlNodePtr node2=node->children;
  asvc.emd_no=NodeAsStringFast("doc_no", node2);
  asvc.emd_coupon=NodeAsIntegerFast("coupon_no", node2, NoExists);
  action=NodeAsIntegerFast("associated", node2)!=0?CpnStatAction::associate:
                                                   CpnStatAction::disassociate;
  status=NodeIsNULLFast("coupon_status", node2)?Ticketing::CouponStatus(Ticketing::CouponStatus::Unavailable):
                                                Ticketing::CouponStatus(CouponStatus::fromDispCode(NodeAsStringFast("coupon_status", node2)));
  pax.tkn.no=NodeAsStringFast("associated_no", node2);
  pax.tkn.coupon=NodeAsIntegerFast("associated_coupon", node2, NoExists);
  return *this;
};

void GetEMDDisassocList(const int point_id,
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
    TEMDCtxtItem item;
    item.asvc.fromDB(Qry);
    std::set<ASTRA::TRcptServiceType> service_types;
    item.asvc.rcpt_service_types(service_types);
    if (service_types.find(ASTRA::rstExcess)==service_types.end() &&
        service_types.find(ASTRA::rstPaid)==service_types.end()) continue;

    item.paxFromDB(Qry);
    item.point_id=point_id;
    item.grp_id=Qry.FieldAsInteger("grp_id");

    item.status=calcPaxCouponStatus(item.pax.refuse,
                                    item.pax.pr_brd,
                                    in_final_status);
    if (item.status==CouponStatus::Flown) continue; //если финальный статус, то никаких ассоциаций

    if (item.status==CouponStatus::Boarded &&
        Qry.FieldIsNULL("paid_bag_emd_grp_id"))
      item.action=Ticketing::CpnStatAction::disassociate;
    else
      item.action=Ticketing::CpnStatAction::associate;
    emds.push_back(item);
  };
};

void GetBoundEMDStatusList(const int grp_id,
                           const bool in_final_status,
                           std::list<TEMDCtxtItem> &emds)
{
  emds.clear();

  if (in_final_status) return;

  CheckIn::PaidBagEMDList boundEMDs;
  PaxASVCList::GetBoundPaidBagEMD(grp_id, boundEMDs);
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText= GetSQL(PaxASVCList::oneWithTknByGrpId);
  Qry.CreateVariable("grp_id", otInteger, grp_id);
  Qry.DeclareVariable("emd_no", otString);
  Qry.DeclareVariable("emd_coupon", otInteger);
  for(CheckIn::PaidBagEMDList::const_iterator e=boundEMDs.begin(); e!=boundEMDs.end(); ++e)
  {
    TEMDCtxtItem item;
    item.asvc=e->first;
    item.grp_id=grp_id;

    Qry.SetVariable("emd_no", e->second.emd_no);
    Qry.SetVariable("emd_coupon", e->second.emd_coupon);
    Qry.Execute();
    if (!Qry.Eof)
    {
      //вообще когда одинаковые EMD принадлежат разным пассажиром - это плохо и наверное надо что-то с этим делать
      //пока выбираем пассажира с более ранним регистрационным номером
      item.paxFromDB(Qry);
      item.status=calcPaxCouponStatus(item.pax.refuse,
                                      item.pax.pr_brd,
                                      in_final_status);
      emds.push_back(item);
    }
    else
      emds.push_back(item);
  };
};

const TEMDocItem& TEMDocItem::toDB(const TEdiAction ediAction) const
{
  if (emd_no.empty() || emd_coupon==ASTRA::NoExists)
    throw EXCEPTIONS::Exception("TEMDocItem::toDB: empty emd");

  QParams QryParams;
  QryParams << QParam("doc_no", otString, emd_no)
            << QParam("coupon_no", otInteger, emd_coupon)
            << (point_id!=ASTRA::NoExists?QParam("point_id", otInteger, point_id):
                                          QParam("point_id", otInteger, FNull));
  switch(ediAction)
  {
    case ChangeOfStatus:
      QryParams << (status!=Ticketing::CouponStatus::Unavailable?QParam("coupon_status", otString, status->dispCode()):
                                                                 QParam("coupon_status", otString, FNull))
                << QParam("change_status_error", otString, change_status_error.substr(0,100));
      break;
    case SystemUpdate:
      QryParams << QParam("associated", otInteger, (int)(action==Ticketing::CpnStatAction::associate))
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

TEMDocItem& TEMDocItem::fromDB(const string &v_emd_no,
                               const int v_emd_coupon,
                               const bool lock)
{
  clear();

  if (v_emd_no.empty() || v_emd_coupon==ASTRA::NoExists)
    throw EXCEPTIONS::Exception("TEMDocItem::fromDB: empty emd");

  QParams QryParams;
  QryParams << QParam("doc_no", otString, v_emd_no);
  QryParams << QParam("coupon_no", otInteger, v_emd_coupon);

  const string sql="SELECT coupon_status, associated, associated_no, associated_coupon, "
                   "       change_status_error, system_update_error, point_id "
                   "FROM emdocs "
                   "WHERE doc_no=:doc_no AND coupon_no=:coupon_no";

  TCachedQuery Qry(sql+(lock?" FOR UPDATE":""), QryParams);

  Qry.get().Execute();
  if (!Qry.get().Eof)
  {
    emd_no=v_emd_no;
    emd_coupon=v_emd_coupon;

    et_no=Qry.get().FieldAsString("associated_no");
    et_coupon=Qry.get().FieldIsNULL("associated_coupon")?ASTRA::NoExists:
                                                         Qry.get().FieldAsInteger("associated_coupon");

    status=Qry.get().FieldIsNULL("coupon_status")?Ticketing::CouponStatus(Ticketing::CouponStatus::Unavailable):
                                                  Ticketing::CouponStatus(CouponStatus::fromDispCode(Qry.get().FieldAsString("coupon_status")));
    action=Qry.get().FieldAsInteger("associated")!=0?Ticketing::CpnStatAction::associate:
                                                     Ticketing::CpnStatAction::disassociate;

    change_status_error=Qry.get().FieldAsString("change_status_error");
    system_update_error=Qry.get().FieldAsString("system_update_error");

    point_id=Qry.get().FieldIsNULL("point_id")?ASTRA::NoExists:
                                               Qry.get().FieldAsInteger("point_id");
  };

  return *this;
};

void ProcEdiEvent(const TLogLocale &event,
                  const TEdiCtxtItem &ctxt,
                  const xmlNodePtr eventCtxtNode,
                  const bool repeated)
{
  TLogLocale eventWithPax;
  eventWithPax.ev_type=event.ev_type;
  eventWithPax.id1=ctxt.point_id;
  eventWithPax.id3=ctxt.grp_id;
  if (!ctxt.paxUnknown())
  {
    eventWithPax.id2=ctxt.pax.reg_no;

    eventWithPax.lexema_id = "EVT.PASSENGER_DATA";
    eventWithPax.prms << PrmSmpl<string>("pax_name", ctxt.pax.full_name())
                      << PrmElem<string>("pers_type", etPersType, EncodePerson(ctxt.pax.pers_type))
                      << PrmLexema("param", event.lexema_id, event.prms);
  }
  else
  {
    eventWithPax.lexema_id=event.lexema_id;
    eventWithPax.prms=event.prms;
  };

  if (!repeated) eventWithPax.toDB(ctxt.screen, ctxt.user_descr, ctxt.desk_code);

  eventWithPax.toXML(eventCtxtNode); //важно, что после toDB, потому что инициализируются ev_time и ev_order
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
            << QParam("emd_no", otString, EMDCtxt.asvc.emd_no)
            << QParam("emd_coupon", otInteger, EMDCtxt.asvc.emd_coupon);

  TCachedQuery Qry(PaxASVCList::GetSQL(PaxASVCList::oneWithTknByPaxId), QryParams);
  Qry.get().Execute();
  if (Qry.get().Eof) return false;

  TEMDCtxtItem EMDCtxtActual(EMDCtxt);
  EMDCtxtActual.paxFromDB(Qry.get());
  EMDCtxtActual.grp_id=Qry.get().FieldAsInteger("grp_id");

  if (EMDCtxtActual.grp_id==EMDCtxt.grp_id &&
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
  event.id3=EMDCtxtActual.grp_id;
  return true;
};


