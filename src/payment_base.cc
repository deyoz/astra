#include "payment_base.h"
#include "term_version.h"
#include "qrys.h"
#include "alarms.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/slogger.h"

using namespace std;
using namespace AstraLocale;
using namespace EXCEPTIONS;
using namespace BASIC::date_time;

namespace CheckIn
{

const TPaymentDoc& TPaymentDoc::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  NewTextChild(node, "doc_type", doc_type);
  NewTextChild(node, "doc_aircode", doc_aircode, "");
  NewTextChild(node, "doc_no", doc_no);
  NewTextChild(node, "doc_coupon", doc_coupon, ASTRA::NoExists);
  NewTextChild(node, "service_quantity", service_quantity);
  return *this;
}

const TPaymentDoc& TPaymentDoc::toXMLcompatible(xmlNodePtr node) const
{
  if (notCompatibleWithPriorTermVersions())
    throw Exception("TPaymentDoc::toXMLcompatible: not compatible");

  if (node==NULL) return *this;
  NewTextChild(node, "emd_no", doc_no);
  NewTextChild(node, "emd_coupon", doc_coupon);
  return *this;
}

TPaymentDoc& TPaymentDoc::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  doc_type=NodeAsStringFast("doc_type", node2);
  doc_aircode=NodeAsStringFast("doc_aircode", node2, "");
  doc_no=NodeAsStringFast("doc_no", node2);
  doc_coupon=NodeAsIntegerFast("doc_coupon", node2, ASTRA::NoExists);
  service_quantity=NodeAsIntegerFast("service_quantity", node2);
  return *this;
}

TPaymentDoc& TPaymentDoc::fromXMLcompatible(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  doc_type="EMDA";
  doc_no=NodeAsStringFast("emd_no", node2);
  doc_coupon=NodeAsIntegerFast("emd_coupon", node2);
  service_quantity=1;
  return *this;
}

const TPaymentDoc& TPaymentDoc::toDB(TQuery &Qry) const
{
  Qry.SetVariable("doc_type", doc_type);
  Qry.SetVariable("doc_aircode", doc_aircode);
  Qry.SetVariable("doc_no", doc_no);
  doc_coupon!=ASTRA::NoExists?Qry.SetVariable("doc_coupon", doc_coupon):
                              Qry.SetVariable("doc_coupon", FNull);
  service_quantity!=ASTRA::NoExists?Qry.SetVariable("service_quantity", service_quantity):
                                    Qry.SetVariable("service_quantity", FNull);
  return *this;
}

TPaymentDoc& TPaymentDoc::fromDB(TQuery &Qry)
{
  clear();
  doc_type=Qry.FieldAsString("doc_type");
  doc_aircode=Qry.FieldAsString("doc_aircode");
  doc_no=Qry.FieldAsString("doc_no");
  if (!Qry.FieldIsNULL("doc_coupon"))
    doc_coupon=Qry.FieldAsInteger("doc_coupon");
  service_quantity=Qry.FieldAsInteger("service_quantity");
  return *this;
}

std::string TPaymentDoc::no_str() const
{
  ostringstream s;
  if (!doc_aircode.empty())
    s << doc_aircode << " ";
  s << doc_no;
  if (doc_coupon!=ASTRA::NoExists)
    s << "/" << doc_coupon;
  return s.str();
}

std::string TPaymentDoc::emd_no_str() const
{
  ostringstream s;
  if (isEMD())
  {
    s << doc_no;
    if (doc_coupon!=ASTRA::NoExists)
      s << "/" << doc_coupon;
  };
  return s.str();
}

bool TPaymentDoc::isEMD() const
{
  return doc_type=="EMDA" || doc_type=="EMDS";
}

std::string TPaymentDoc::traceStr() const
{
  ostringstream s;
  s << "doc=" << doc_type << " " << no_str() << ", "
    << "service_quantity=" << (service_quantity!=ASTRA::NoExists?IntToString(service_quantity):"NoExists");
  return s.str();
}

bool TPaymentDoc::notCompatibleWithPriorTermVersions() const
{
  return doc_type!="EMDA" ||
         !doc_aircode.empty() ||
         doc_coupon==ASTRA::NoExists ||
         service_quantity!=1;
}

std::string TServicePaymentItem::traceStr() const
{
  ostringstream s;
  s << TPaymentDoc::traceStr() << ", "
    << "pax_id=" << (pax_id!=ASTRA::NoExists?IntToString(pax_id):"NoExists") << ", "
    << "trfer_num=" << (trfer_num!=ASTRA::NoExists?IntToString(trfer_num):"NoExists") << ", "
    << "doc_weight=" << (doc_weight!=ASTRA::NoExists?IntToString(doc_weight):"NoExists");
  if (pc) s << ", " << pc.get().traceStr();
  if (wt) s << ", " << wt.get().traceStr();
  return s.str();
}

std::string TServicePaymentItem::key_str(const std::string& lang) const
{
  if (pc)
    return pc.get().str(lang);
  else if (wt)
    return wt.get().str(lang);
  else
    return "";
}

std::string TServicePaymentItem::key_str_compatible() const
{
  if (pc)
    return pc.get().RFISC;
  else if (wt)
    return wt.get().bag_type;
  else
    return "";
}

const TServicePaymentItem& TServicePaymentItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  TPaymentDoc::toXML(node);
  NewTextChild(node, "pax_id", pax_id, ASTRA::NoExists);
  NewTextChild(node, "transfer_num", trfer_num);
  if (pc) pc.get().toXML(node);
  else if (wt) wt.get().toXML(node);
  NewTextChild(node, "doc_weight", doc_weight, ASTRA::NoExists);
  NewTextChild(node, "read_only", (int)is_auto_service(), (int)false);
  return *this;
}

const TServicePaymentItem& TServicePaymentItem::toXMLcompatible(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  TPaymentDoc::toXMLcompatible(node);
  NewTextChild(node, "bag_type", key_str_compatible());
  NewTextChild(node, "transfer_num", trfer_num);
  if (doc_weight!=ASTRA::NoExists)
    NewTextChild(node, "weight", doc_weight);
  else
    NewTextChild(node, "weight", 0);
  return *this;
}

TServicePaymentItem& TServicePaymentItem::fromXML(xmlNodePtr node, bool is_unaccomp)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  TPaymentDoc::fromXML(node);
  pax_id=NodeAsIntegerFast("pax_id", node2, ASTRA::NoExists);
  trfer_num=NodeAsIntegerFast("transfer_num", node2);
  if (GetNodeFast("rfisc",node2)!=NULL)
  {
    pc=TRFISCKey();
    pc.get().fromXML(node);
  }
  else
  {
    wt=TBagTypeKey();
    wt.get().fromXML(node);
  };
  doc_weight=NodeAsIntegerFast("doc_weight", node2, ASTRA::NoExists);
  return *this;
};

TServicePaymentItem& TServicePaymentItem::fromXMLcompatible(xmlNodePtr node, bool baggage_pc)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  TPaymentDoc::fromXMLcompatible(node);

  if (TReqInfo::Instance()->client_type==ASTRA::ctTerm && TReqInfo::Instance()->desk.compatible(PIECE_CONCEPT_VERSION))
    trfer_num=NodeAsIntegerFast("transfer_num", node2);
  else
    trfer_num=0;
  if (baggage_pc)
  {
    pc=TRFISCKey();
    pc.get().fromXMLcompatible(node);
  }
  else
  {
    wt=TBagTypeKey();
    wt.get().fromXMLcompatible(node);
  };

  if (!baggage_pc)
    doc_weight=NodeAsIntegerFast("weight",node2);
  return *this;
}

const TServicePaymentItem& TServicePaymentItem::toDB(TQuery &Qry) const
{
  TPaymentDoc::toDB(Qry);
  pax_id!=ASTRA::NoExists?Qry.SetVariable("pax_id", pax_id):
                          Qry.SetVariable("pax_id", FNull);
  trfer_num!=ASTRA::NoExists?Qry.SetVariable("transfer_num", trfer_num):
                             Qry.SetVariable("transfer_num", FNull);
  if (!pc) TRFISCKey().toDB(Qry);   //устанавливаем FNull
  if (!wt) TBagTypeKey().toDB(Qry);
  if (pc) pc.get().toDB(Qry);
  else if (wt) wt.get().toDB(Qry);
  doc_weight!=ASTRA::NoExists?Qry.SetVariable("doc_weight", doc_weight):
                              Qry.SetVariable("doc_weight", FNull);

  return *this;
}

TServicePaymentItem& TServicePaymentItem::fromDB(TQuery &Qry)
{
  clear();
  TPaymentDoc::fromDB(Qry);
  if (!Qry.FieldIsNULL("pax_id"))
    pax_id=Qry.FieldAsInteger("pax_id");
  trfer_num=Qry.FieldAsInteger("transfer_num");
  if (!Qry.FieldIsNULL("rfisc"))
  {
    pc=TRFISCKey();
    pc.get().fromDB(Qry);
    pc.get().getListItem();
  }
  else
  {
    wt=TBagTypeKey();
    wt.get().fromDB(Qry);
    wt.get().getListItem();
  }
  if (!Qry.FieldIsNULL("doc_weight"))
    doc_weight=Qry.FieldAsInteger("doc_weight");
  return *this;
}

void TServicePaymentList::fromDB(int grp_id)
{
  clear();
  TCachedQuery Qry("SELECT * FROM service_payment WHERE grp_id=:grp_id",
                   QParams() << QParam("grp_id", otInteger, grp_id));
  Qry.get().Execute();
  for(; !Qry.get().Eof; Qry.get().Next())
    push_back(TServicePaymentItem().fromDB(Qry.get()));

  if (empty()) //!!!потом удалить
  {
    string airline_wt=WeightConcept::GetCurrSegBagAirline(grp_id); //TServicePaymentList::fromDB - checked!

    TCachedQuery Qry("SELECT paid_bag_emd.grp_id, "
                     "       paid_bag_emd.pax_id, "
                     "       paid_bag_emd.transfer_num, "
                     "       -paid_bag_emd.grp_id AS list_id, "
                     "       DECODE(paid_bag_emd.bag_type, NULL, NULL, LPAD(paid_bag_emd.bag_type,2,'0')) AS bag_type, "
                     "       paid_bag_emd.rfisc, "
                     "       :airline_wt AS airline, "
                     "       NULL AS service_type, "
                     "       1 AS service_quantity, "
                     "       'EMDA' AS doc_type, "
                     "       emd_no AS doc_no, "
                     "       emd_coupon AS doc_coupon, "
                     "       NULL AS doc_aircode, "
                     "       weight AS doc_weight "
                     "FROM paid_bag_emd "
                     "WHERE paid_bag_emd.grp_id=:grp_id AND paid_bag_emd.rfisc IS NULL "
                     "UNION "
                     "SELECT paid_bag_emd.grp_id, "
                     "       paid_bag_emd.pax_id, "
                     "       paid_bag_emd.transfer_num, "
                     "       -paid_bag_emd.grp_id AS list_id, "
                     "       DECODE(paid_bag_emd.bag_type, NULL, NULL, LPAD(paid_bag_emd.bag_type,2,'0')) AS bag_type, "
                     "       paid_bag_emd.rfisc, "
                     "       rfisc_list_items.airline, "
                     "       rfisc_list_items.service_type, "
                     "       1 AS service_quantity, "
                     "       'EMDA' AS doc_type, "
                     "       emd_no AS doc_no, "
                     "       emd_coupon AS doc_coupon, "
                     "       NULL AS doc_aircode, "
                     "       NULL AS doc_weight "
                     "FROM paid_bag_emd, "
                     "     (SELECT bag_types_lists.airline, grp_rfisc_lists.service_type, grp_rfisc_lists.rfisc "
                     "      FROM pax_grp, bag_types_lists, grp_rfisc_lists "
                     "      WHERE pax_grp.bag_types_id=bag_types_lists.id AND "
                     "            bag_types_lists.id=grp_rfisc_lists.list_id AND "
                     "            pax_grp.grp_id=:grp_id) rfisc_list_items "
                     "WHERE paid_bag_emd.grp_id=:grp_id AND paid_bag_emd.rfisc IS NOT NULL AND "
                     "      paid_bag_emd.rfisc=rfisc_list_items.rfisc(+) ",
                     QParams() << QParam("grp_id", otInteger, grp_id)
                               << QParam("airline_wt", otString, airline_wt));
    Qry.get().Execute();
    for(; !Qry.get().Eof; Qry.get().Next())
    {
      if (!Qry.get().FieldIsNULL("rfisc") &&
          (Qry.get().FieldIsNULL("airline") || Qry.get().FieldIsNULL("service_type")))
        throw Exception("TServicePaymentList::fromDB: wrong data (grp_id=%d, rfisc=%s)",
                        grp_id, Qry.get().FieldAsString("rfisc"));
      if (Qry.get().FieldIsNULL("rfisc") && Qry.get().FieldIsNULL("airline"))
        throw Exception("TServicePaymentList::fromDB: wrong data (grp_id=%d, bag_type=%s)",
                        grp_id, Qry.get().FieldAsString("bag_type"));
      push_back(TServicePaymentItem().fromDB(Qry.get()));
    };
  };
}

void TServicePaymentListWithAuto::dump(const string &file, int line) const
{
    LogTrace(TRACE5) << "-----TServicePaymentListWithAuto::dump: " << file << ": " << line << "-----";
    for(TServicePaymentListWithAuto::const_iterator i = begin(); i != end(); i++)
        LogTrace(TRACE5) << i->traceStr();
    LogTrace(TRACE5) << "-------------------------------------";
}

void TServicePaymentListWithAuto::getUniqRFISCSs(int pax_id, set<string> &rfisc_set) const
{
    for(TServicePaymentListWithAuto::const_iterator i = begin(); i != end(); i++)
        if(
                i->trfer_num == 0 and
                i->pax_id == pax_id and
                i->pc)
            rfisc_set.insert(i->pc->RFISC);
}

bool TServicePaymentListWithAuto::isRFISCGrpExists(int pax_id, const string &grp, const string &subgrp) const
{
    for(TServicePaymentListWithAuto::const_iterator item = begin(); item != end(); item++) {
        if(
                item->pax_id == pax_id and item->trfer_num == 0 and
                item->pc and
                item->pc->list_item and
                item->pc->list_item->grp == grp and
                (subgrp.empty() or item->pc->list_item->subgrp == subgrp))
            return true;
    }
    return false;
}

void TServicePaymentListWithAuto::fromDB(int grp_id)
{
  TServicePaymentList list1;
  list1.fromDB(grp_id);
  for(TServicePaymentList::const_iterator i=list1.begin(); i!=list1.end(); ++i)
    push_back(*i);

  TGrpServiceAutoList list2;
  list2.fromDB(grp_id, true);
  for(TGrpServiceAutoList::const_iterator i=list2.begin(); i!=list2.end(); ++i)
  {
    TServicePaymentItem item(*i);
    if (item.pc)
      item.pc.get().getListItemAuto(i->pax_id, i->trfer_num, i->RFIC);
    push_back(item);
  };
}

void TServicePaymentList::clearDB(int grp_id)
{
  TCachedQuery Qry("DELETE FROM service_payment WHERE grp_id=:grp_id",
                   QParams() << QParam("grp_id", otInteger, grp_id));
  Qry.get().Execute();
}

void TServicePaymentList::toDB(int grp_id) const
{
  clearDB(grp_id);
  TCachedQuery Qry("INSERT INTO service_payment(grp_id, pax_id, transfer_num, list_id, bag_type, rfisc, service_type, airline, service_quantity, "
                   "  doc_type, doc_aircode, doc_no, doc_coupon, doc_weight) "
                   "VALUES(:grp_id, :pax_id, :transfer_num, :list_id, :bag_type, :rfisc, :service_type, :airline, :service_quantity, "
                   "  :doc_type, :doc_aircode, :doc_no, :doc_coupon, :doc_weight) ",

                   QParams() << QParam("grp_id", otInteger, grp_id)
                             << QParam("pax_id", otInteger)
                             << QParam("transfer_num", otInteger)
                             << QParam("list_id", otInteger)
                             << QParam("bag_type", otString)
                             << QParam("rfisc", otString)
                             << QParam("service_type", otString)
                             << QParam("airline", otString)
                             << QParam("service_quantity", otInteger)
                             << QParam("doc_type", otString)
                             << QParam("doc_aircode", otString)
                             << QParam("doc_no", otString)
                             << QParam("doc_coupon", otInteger)
                             << QParam("doc_weight", otInteger));
  for(TServicePaymentList::const_iterator i=begin(); i!=end(); ++i)
  {
    i->toDB(Qry.get());
    try
    {
      Qry.get().Execute();
    }
    catch(EOracleError E)
    {
      if (E.Code==1)
      {
        if (i->isEMD())
          throw UserException("MSG.DUPLICATED_EMD_NUMBER", LParams()<<LParam("emd_no",i->no_str()));
        else
          throw UserException("MSG.DUPLICATED_MCO_NUMBER", LParams()<<LParam("mco_no",i->no_str()));
      }
      else
        throw;
    };
  }

  TGrpAlarmHook::set(Alarm::UnboundEMD, grp_id);
}

void TServicePaymentList::copyDB(int grp_id_src, int grp_id_dest)
{
  clearDB(grp_id_dest);
  //привязанные к pax_id:
  TCachedQuery Qry(
    "INSERT INTO service_payment(grp_id, pax_id, transfer_num, list_id, bag_type, rfisc, service_type, airline, service_quantity, "
    "  doc_type, doc_aircode, doc_no, doc_coupon, doc_weight) "
    "SELECT dest.grp_id, "
    "       dest.pax_id, "
    "       service_payment.transfer_num+src.seg_no-dest.seg_no, "
    "       service_payment.list_id, "
    "       service_payment.bag_type, "
    "       service_payment.rfisc, "
    "       service_payment.service_type, "
    "       service_payment.airline, "
    "       service_payment.service_quantity, "
    "       service_payment.doc_type, "
    "       service_payment.doc_aircode, "
    "       service_payment.doc_no, "
    "       service_payment.doc_coupon, "
    "       service_payment.doc_weight "
    "FROM service_payment, "
    "     (SELECT pax.pax_id, "
    "             tckin_pax_grp.grp_id, "
    "             tckin_pax_grp.tckin_id, "
    "             tckin_pax_grp.seg_no, "
    "             tckin_pax_grp.first_reg_no-pax.reg_no AS distance "
    "      FROM pax, tckin_pax_grp "
    "      WHERE pax.grp_id=tckin_pax_grp.grp_id AND pax.grp_id=:grp_id_src) src, "
    "     (SELECT pax.pax_id, "
    "             tckin_pax_grp.grp_id, "
    "             tckin_pax_grp.tckin_id, "
    "             tckin_pax_grp.seg_no, "
    "             tckin_pax_grp.first_reg_no-pax.reg_no AS distance "
    "      FROM pax, tckin_pax_grp "
    "      WHERE pax.grp_id=tckin_pax_grp.grp_id AND pax.grp_id=:grp_id_dest) dest "
    "WHERE src.tckin_id=dest.tckin_id AND "
    "      src.distance=dest.distance AND "
    "      service_payment.pax_id=src.pax_id AND "
    "      service_payment.transfer_num+src.seg_no-dest.seg_no>=0 AND "
    "      service_payment.pax_id IS NOT NULL AND service_payment.doc_weight IS NULL ",
    QParams() << QParam("grp_id_src", otInteger, grp_id_src)
              << QParam("grp_id_dest", otInteger, grp_id_dest));
  Qry.get().Execute();

  //непривязанные к pax_id:
  //неправильный расчет платности багажа при wt на последующих сегментах. Надо вводить is_trfer возможно, аналогично багажу
  TCachedQuery Qry2(
    "INSERT INTO service_payment(grp_id, pax_id, transfer_num, list_id, bag_type, rfisc, service_type, airline, service_quantity, "
    "  doc_type, doc_aircode, doc_no, doc_coupon, doc_weight) "
    "SELECT dest.grp_id, "
    "       NULL, "
    "       service_payment.transfer_num+src.seg_no-dest.seg_no, "
    "       service_payment.list_id, "
    "       service_payment.bag_type, "
    "       service_payment.rfisc, "
    "       service_payment.service_type, "
    "       service_payment.airline, "
    "       service_payment.service_quantity, "
    "       service_payment.doc_type, "
    "       service_payment.doc_aircode, "
    "       service_payment.doc_no, "
    "       service_payment.doc_coupon, "
    "       service_payment.doc_weight "
    "FROM service_payment, "
    "     (SELECT tckin_pax_grp.grp_id, "
    "             tckin_pax_grp.tckin_id, "
    "             tckin_pax_grp.seg_no "
    "      FROM tckin_pax_grp "
    "      WHERE tckin_pax_grp.grp_id=:grp_id_src) src, "
    "     (SELECT tckin_pax_grp.grp_id, "
    "             tckin_pax_grp.tckin_id, "
    "             tckin_pax_grp.seg_no "
    "      FROM tckin_pax_grp "
    "      WHERE tckin_pax_grp.grp_id=:grp_id_dest) dest "
    "WHERE src.tckin_id=dest.tckin_id AND "
    "      service_payment.grp_id=src.grp_id AND "
    "      service_payment.transfer_num+src.seg_no-dest.seg_no>=0 AND "
    "      service_payment.pax_id IS NULL AND service_payment.doc_weight IS NULL ",
    QParams() << QParam("grp_id_src", otInteger, grp_id_src)
              << QParam("grp_id_dest", otInteger, grp_id_dest));
  Qry2.get().Execute();

  TGrpAlarmHook::set(Alarm::UnboundEMD, grp_id_dest);
}

void TServicePaymentListWithAuto::toXML(xmlNodePtr node) const
{
  if (node==NULL) return;
  if (TReqInfo::Instance()->desk.compatible(PAX_SERVICE_VERSION))
  {
    xmlNodePtr paymentNode=NewTextChild(node, "service_payment");
    for(TServicePaymentListWithAuto::const_iterator i=begin(); i!=end(); ++i)
      i->toXML(NewTextChild(paymentNode, "item"));
  }
  else
  {
    TServicePaymentListWithAuto compatible, not_compatible, other_svc;
    getCompatibleWithPriorTermVersions(compatible, not_compatible, other_svc);

    xmlNodePtr paymentNode=NewTextChild(node, "paid_bag_emd");
    for(TServicePaymentListWithAuto::const_iterator i=compatible.begin(); i!=compatible.end(); ++i)
    {
      if (i->notCompatibleWithPriorTermVersions()) continue;
      i->toXMLcompatible(NewTextChild(paymentNode, "emd"));
    };
  };
}

void TServicePaymentList::getAllListKeys(int grp_id, bool is_unaccomp)
{
  for(TServicePaymentList::iterator i=begin(); i!=end(); ++i)
  {
    TServicePaymentItem &item=*i;
    try
    {
      if (item.pc)
      {
        if (is_unaccomp)
          item.pc.get().getListKeyUnaccomp(grp_id, item.trfer_num, boost::none, "TServicePaymentList");
        else
        {
          if (item.pax_id!=ASTRA::NoExists)
            item.pc.get().getListKeyByPaxId(item.pax_id, item.trfer_num, boost::none, "TServicePaymentList");
          else
            item.pc.get().getListKeyByGrpId(grp_id, item.trfer_num, boost::none, "TServicePaymentList");
        }
      }
      else if (item.wt)
      {
        if (is_unaccomp)
          item.wt.get().getListKeyUnaccomp(grp_id, item.trfer_num, boost::none, "TServicePaymentList");
        else
        {
          if (item.pax_id!=ASTRA::NoExists)
            item.wt.get().getListKeyByPaxId(item.pax_id, item.trfer_num, boost::none, "TServicePaymentList");
          else
            item.wt.get().getListKeyByGrpId(grp_id, item.trfer_num, boost::none, "TServicePaymentList");
        }
      };
    }
    catch(EConvertError &e)
    {
      if (item.trfer_num>get_max_tckin_num(grp_id))
      {
        ProgTrace(TRACE5, e.what());
        throw UserException("MSG.PAYMENT_DISABLED_ON_SEG_DUE_BAG_CONCEPT",
                            LParams() << LParam("flight", IntToString(item.trfer_num+1))
                                      << LParam("svc_key_view", item.key_str_compatible()));
      };
      throw;
    };
  }
}

void TServicePaymentList::getAllListItems(int grp_id, bool is_unaccomp)
{
  for(TServicePaymentList::iterator i=begin(); i!=end(); ++i)
  {
    TServicePaymentItem &item=*i;
    try
    {
      if (item.pc)
      {
        if (is_unaccomp)
          item.pc.get().getListItemUnaccomp(grp_id, item.trfer_num, boost::none, "TServicePaymentList");
        else
        {
          if (item.pax_id!=ASTRA::NoExists)
            item.pc.get().getListItemByPaxId(item.pax_id, item.trfer_num, boost::none, "TServicePaymentList");
          else
            item.pc.get().getListItemByGrpId(grp_id, item.trfer_num, boost::none, "TServicePaymentList");
        }
      }
      else if (item.wt)
      {
        if (is_unaccomp)
          item.wt.get().getListItemUnaccomp(grp_id, item.trfer_num, boost::none, "TServicePaymentList");
        else
        {
          if (item.pax_id!=ASTRA::NoExists)
            item.wt.get().getListItemByPaxId(item.pax_id, item.trfer_num, boost::none, "TServicePaymentList");
          else
            item.wt.get().getListItemByGrpId(grp_id, item.trfer_num, boost::none, "TServicePaymentList");
        }
      };
    }
    catch(EConvertError &e)
    {
      if (item.trfer_num>get_max_tckin_num(grp_id))
      {
        ProgTrace(TRACE5, e.what());
        throw UserException("MSG.PAYMENT_DISABLED_ON_SEG_DUE_BAG_CONCEPT",
                            LParams() << LParam("flight", IntToString(item.trfer_num+1))
                                      << LParam("svc_key_view", item.key_str()));
      };
      throw;
    };
  }
}

bool TServicePaymentList::dec(const TPaxSegRFISCKey &key)
{
  for(int pass=0; pass<2; pass++)
  {
    int pax_id=(pass==0?key.pax_id:ASTRA::NoExists);
    for(TServicePaymentList::iterator i=begin(); i!=end(); ++i)
    {
      TServicePaymentItem &item=*i;
      if (item.pc &&
          item.pc.get()==key &&
          item.trfer_num==key.trfer_num &&
          item.pax_id==pax_id)
      {
        if (item.service_quantity>1)
          item.service_quantity--;
        else
          erase(i);
        return true;
      };
    };
  };
  return false;
}

int TServicePaymentListWithAuto::getDocWeight(const TBagTypeListKey &key) const
{
  int result=0;
  for(TServicePaymentListWithAuto::const_iterator i=begin(); i!=end(); ++i)
    if (i->wt && key==i->wt.get() && i->trfer_num==0)
    {
      if (i->doc_weight!=ASTRA::NoExists && i->doc_weight>0)
        result+=i->doc_weight;
    };
  return result;
}

bool TServicePaymentList::equal(const TServicePaymentList &list) const
{
  TServicePaymentList tmp=list;
  for(TServicePaymentList::const_iterator i=begin(); i!=end(); ++i)
  {
    TServicePaymentList::iterator j=tmp.begin();
    for(; j!=tmp.end(); ++j)
      if (*i==*j) break;
    if (j!=tmp.end())
      tmp.erase(j);
    else
      return false;
  };
  return tmp.empty();
}

void TServicePaymentList::dump(const std::string &where) const
{
  for(TServicePaymentList::const_iterator i=begin(); i!=end(); ++i)
    ProgTrace(TRACE5, "%s: %s", where.c_str(), i->traceStr().c_str());
}

void ServicePaymentFromXML(xmlNodePtr node,
                           int grp_id,
                           bool is_unaccomp,
                           bool baggage_pc,
                           boost::optional<TServicePaymentList> &payment)
{
  payment=boost::none;
  if (node==NULL) return;
  xmlNodePtr paymentNode;
  if (!TReqInfo::Instance()->desk.compatible(PAX_SERVICE_VERSION))
    paymentNode=GetNode("paid_bag_emd", node);
  else
    paymentNode=GetNode("service_payment", node);
  if (paymentNode!=NULL)
  {
    payment=TServicePaymentList();
    for(xmlNodePtr itemNode=paymentNode->children; itemNode!=NULL; itemNode=itemNode->next)
    {
      TServicePaymentItem item;
      if (!TReqInfo::Instance()->desk.compatible(PAX_SERVICE_VERSION))
      {
        item.fromXMLcompatible(itemNode, baggage_pc);
        if (item.wt)
        {
          if (/*item.trfer_num!=0 || !!vlad (это старое)*/
              item.wt.get().bag_type==WeightConcept::OLD_TRFER_BAG_TYPE) continue;  //!!vlad потом докрутить (это старое)
        };
        if (item.pc)
        {
          if (item.pc.get().RFISC.empty())
            throw UserException("MSG.EMD_ATTACHED_TO_UNKNOWN_RFISC", LParams()<<LParam("emd_no",item.no_str()));
        };
      }
      else
      {
        if (string((char*)itemNode->name)!="item") continue;
        item.fromXML(itemNode, is_unaccomp);
      };
      if (item.is_auto_service()) continue;
      payment.get().push_back(item);
    }
    if (!TReqInfo::Instance()->desk.compatible(PAX_SERVICE_VERSION))
      payment.get().getAllListKeys(grp_id, is_unaccomp);
  };
}

void TryCleanServicePayment(const WeightConcept::TPaidBagList &curr_paid,
                            const CheckIn::TServicePaymentList &prior_payment,
                            boost::optional< CheckIn::TServicePaymentList > &curr_payment)
{
  set<TBagTypeListKey> bag_types;
  for(WeightConcept::TPaidBagList::const_iterator p=curr_paid.begin(); p!=curr_paid.end(); ++p)
    if (p->weight>0) bag_types.insert(*p);

  if (curr_payment)
  {
    //были изменения EMD с терминала
    for(CheckIn::TServicePaymentList::iterator i=curr_payment.get().begin(); i!=curr_payment.get().end();)
      if (i->wt && bag_types.find(i->wt.get())==bag_types.end())
        i=curr_payment.get().erase(i);
      else
        ++i;
  }
  else
  {
    CheckIn::TServicePaymentList payment;
    bool modified=false;
    for(CheckIn::TServicePaymentList::const_iterator i=prior_payment.begin(); i!=prior_payment.end(); ++i)
      if (i->wt && bag_types.find(i->wt.get())==bag_types.end())
        modified=true;
      else
        payment.push_back(*i);
    if (modified) curr_payment=payment;
  };
}

bool TryCleanServicePayment(const TPaidRFISCList &curr_paid,
                            CheckIn::TServicePaymentList &curr_payment)
{
  bool modified=false;
  set<TPaxSegRFISCKey> rfiscs;
  for(TPaidRFISCList::const_iterator p=curr_paid.begin(); p!=curr_paid.end(); ++p)
    if (p->second.paid_positive())
    {
      rfiscs.insert(p->first);
      rfiscs.insert(TPaxSegRFISCKey(Sirena::TPaxSegKey(ASTRA::NoExists, p->first.trfer_num), p->first));
    };

  map< TPaxSegRFISCKey, pair<int/*общее оплаченное кол-во*/,
                             int/*минимальное кол-во для одного документа*/> > tmp_payment;
  for(CheckIn::TServicePaymentList::iterator i=curr_payment.begin(); i!=curr_payment.end();)
  {
    if (i->pc)
    {
      TPaxSegRFISCKey key(Sirena::TPaxSegKey(i->pax_id, i->trfer_num), i->pc.get());

      if (rfiscs.find(key)==rfiscs.end())
      {
        i=curr_payment.erase(i);
        modified=true;
        continue;
      };

      if (i->pax_id!=ASTRA::NoExists && i->service_quantity_valid())
      {
        map< TPaxSegRFISCKey, pair<int, int> >::iterator j=
          tmp_payment.insert(make_pair(key, make_pair(0, i->service_quantity))).first;
        if (j==tmp_payment.end())
          throw Exception("%s: j==tmp_payment.end()!", __FUNCTION__);
        j->second.first+=i->service_quantity;
        if (j->second.second>i->service_quantity) j->second.second=i->service_quantity;
      };
    };
    ++i;
  };

  for(map< TPaxSegRFISCKey, pair<int, int> >::const_iterator i=tmp_payment.begin(); i!=tmp_payment.end(); ++i)
  {
    TPaidRFISCList::const_iterator p=curr_paid.find(i->first);
    if (p==curr_paid.end() ||
        (p->second.paid!=ASTRA::NoExists && i->second.first-i->second.second>=p->second.paid))
      throw UserException("MSG.EXCESS_EMD_ATTACHED_FOR_BAG_TYPE",
                          LParams() << LParam("bag_type", static_cast<TRFISCListKey>(i->first).str()));
  }

  return modified;
}

void PaidBagEMDPropsFromDB(int grp_id, TPaidBagEMDProps &props)
{
  props.clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT * FROM paid_bag_emd_props WHERE grp_id=:grp_id";
  Qry.CreateVariable("grp_id", otInteger, grp_id);
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next())
    props.insert(TPaidBagEMDPropsItem(Qry.FieldAsString("emd_no"),
                                      Qry.FieldAsInteger("emd_coupon"),
                                      Qry.FieldAsInteger("manual_bind")!=0));
}

void PaidBagEMDPropsToDB(int grp_id, const TPaidBagEMDProps &props)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  UPDATE paid_bag_emd_props "
    "  SET grp_id=:grp_id, manual_bind=:manual_bind "
    "  WHERE emd_no=:emd_no AND emd_coupon=:emd_coupon; "
    "  IF SQL%NOTFOUND THEN "
    "    INSERT INTO paid_bag_emd_props(grp_id, emd_no, emd_coupon, manual_bind) "
    "    VALUES(:grp_id, :emd_no, :emd_coupon, :manual_bind); "
    "  END IF; "
    "END;";
  Qry.CreateVariable("grp_id", otInteger, grp_id);
  Qry.DeclareVariable("emd_no", otString);
  Qry.DeclareVariable("emd_coupon", otInteger);
  Qry.DeclareVariable("manual_bind" ,otInteger);
  for(TPaidBagEMDProps::const_iterator i=props.begin(); i!=props.end(); ++i)
  {
    Qry.SetVariable("emd_no", i->emd_no);
    Qry.SetVariable("emd_coupon", i->emd_coupon);
    Qry.SetVariable("manual_bind" ,(int)i->manual_bind);
    Qry.Execute();
  };
}

} //namespace CheckIn
