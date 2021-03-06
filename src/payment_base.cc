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

TComplexBagExcess::TComplexBagExcess(const TBagQuantity& excess1,
                                     const TBagQuantity& excess2) :
  pc(0, TBagUnit()),
  wt(0, TBagUnit())
{
  if (excess1.getUnit()==Ticketing::Baggage::NumPieces)
    pc=excess1;
  if (excess2.getUnit()==Ticketing::Baggage::NumPieces)
    pc=excess2;
  if (excess1.getUnit()==Ticketing::Baggage::WeightKilo ||
      excess1.getUnit()==Ticketing::Baggage::WeightPounds)
    wt=excess1;
  if (excess2.getUnit()==Ticketing::Baggage::WeightKilo ||
      excess2.getUnit()==Ticketing::Baggage::WeightPounds)
    wt=excess2;

  if (pc.empty() || wt.empty())
    throw Exception("%s: wrong parameters (excess1=%s, excess2=%s)",
                    __FUNCTION__,
                    excess1.view(AstraLocale::OutputLang(LANG_EN)).c_str(),
                    excess2.view(AstraLocale::OutputLang(LANG_EN)).c_str());
}

std::string TComplexBagExcess::view(const AstraLocale::OutputLang &lang,
                                    const bool &unitRequired,
                                    const std::string& separator) const
{
  ostringstream s;
  if (!wt.zero() && !pc.zero())
    s << wt.view(lang, true) << separator << pc.view(lang, true);
  else
  {
    if (!wt.zero())
      s << wt.view(lang, unitRequired);
    else if (!pc.zero())
      s << pc.view(lang, unitRequired);
    else
      s << "0";
  };
  return s.str();
}

std::string TComplexBagExcess::deprecatedView(const AstraLocale::OutputLang &lang) const
{
  return !wt.zero()?wt.view(lang, false):
                    pc.view(lang, false);
}

int TComplexBagExcess::getDeprecatedInt() const
{
  return (!wt.zero()?wt:pc).getQuantity();
}

void TComplexBagExcessNodeList::add(xmlNodePtr parent,
                                    const char *name,
                                    const TBagQuantity& excess1,
                                    const TBagQuantity& excess2)
{
  if (parent==nullptr) return;
  TComplexBagExcess excess(excess1, excess2);
  if (_containsOnlyNonZeroExcess && excess.pc.zero() && excess.wt.zero()) return;
  if (!excess.pc.zero()) pcExists=true;
  if (!excess.wt.zero()) wtExists=true;
  excessList.emplace_front(NewTextChild(parent, name, "?"), excess);
}

void TComplexBagExcessNodeList::apply() const
{
  try
  {
    bool unitRequiredTmp=unitRequired();
    for(const TComplexBagExcessNodeItem& i : excessList)
      NodeSetContent(i.node, _deprecatedIntegerOutput?
                               i.complexExcess.deprecatedView(_lang):
                               i.complexExcess.view(_lang, unitRequiredTmp, _separator));
  }
  catch(...) {}
}

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

  trfer_num=NodeAsIntegerFast("transfer_num", node2);

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
  if (!pc) TRFISCKey().toDB(Qry);   //????????????? FNull
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
  for(const auto& i : *this)
  {
      // ??????? '|| i.pax_id == ASTRA::NoExists' ????????? ??-?? MCO
      // ???????, ?? ?? ?????????? ???????
      // ???????? ?????? ?? ?????? ????????? (c) ????
    if (i.pc and (i.pax_id == pax_id || i.pax_id == ASTRA::NoExists) and i.trfer_num == 0)
    {
      TPaxSegRFISCKey key(Sirena::TPaxSegKey(i.pax_id, i.trfer_num), i.pc.get());

      if (TRFISCListItemsCache::isRFISCGrpExists(key, grp, subgrp)) return true;
    };
  };
  return false;
}

void TServicePaymentListWithAuto::fromDB(int grp_id)
{
  clear();
  clearCache();

  TServicePaymentList list1;
  list1.fromDB(grp_id);
  for(const TServicePaymentItem& i : list1)
    push_back(i);

  TGrpServiceAutoList list2;
  list2.fromDB(grp_id, true);
  for(const TGrpServiceAutoItem& i : list2)
    if (i.withEMD())
      push_back(TServicePaymentItem(i));
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
  for(const TServicePaymentItem& i : *this)
  {
    i.toDB(Qry.get());
    try
    {
      Qry.get().Execute();
    }
    catch(const EOracleError& E)
    {
      if (E.Code==1)
      {
        if (i.isEMD())
          throw UserException("MSG.DUPLICATED_EMD_NUMBER", LParams()<<LParam("emd_no",i.no_str()));
        else
          throw UserException("MSG.DUPLICATED_MCO_NUMBER", LParams()<<LParam("mco_no",i.no_str()));
      }
      else
        throw;
    };
  }

  TGrpAlarmHook::set(Alarm::UnboundEMD, grp_id);
}

std::string TServicePaymentList::copySelectSQL()
{
  return
      "SELECT dest.grp_id, "
      "       dest.pax_id, "
      "       service_payment.transfer_num+src.seg_no-dest.seg_no AS transfer_num, "
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
      "       service_payment.doc_weight " +
      TCkinRoute::copySubselectSQL("service_payment", {}, true) +
      "      AND service_payment.pax_id IS NOT NULL AND service_payment.doc_weight IS NULL ";
}

std::string TServicePaymentList::copySelectSQL2()
{
  return
      "SELECT dest.grp_id, "
      "       NULL AS pax_id, "
      "       service_payment.transfer_num+src.seg_no-dest.seg_no AS transfer_num, "
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
      "       service_payment.doc_weight " +
      TCkinRoute::copySubselectSQL("service_payment", {}, false) +
      "      AND service_payment.pax_id IS NULL AND service_payment.doc_weight IS NULL ";
}

void TServicePaymentList::copyDBOneByOne(int grp_id_src, int grp_id_dest)
{
  TServicePaymentList list;

  TCachedQuery Qry(copySelectSQL()+" UNION ALL "+copySelectSQL2(),
                   QParams() << QParam("grp_id_src", otInteger, grp_id_src)
                             << QParam("grp_id_dest", otInteger, grp_id_dest));
  Qry.get().Execute();
  for(; !Qry.get().Eof; Qry.get().Next())
    list.push_back(TServicePaymentItem().fromDB(Qry.get()));

  list.toDB(grp_id_dest);
}

void TServicePaymentList::copyDB(int grp_id_src, int grp_id_dest)
{
  clearDB(grp_id_dest);
  TCachedQuery Qry(
    "INSERT INTO service_payment(grp_id, pax_id, transfer_num, list_id, bag_type, rfisc, service_type, airline, service_quantity, "
    "  doc_type, doc_aircode, doc_no, doc_coupon, doc_weight) " +
    //??????????? ? pax_id:
    copySelectSQL() +
    " UNION ALL " +
    //????????????? ? pax_id:
    //???????????? ?????? ????????? ?????? ??? wt ?? ??????????? ?????????. ???? ??????? is_trfer ????????, ?????????? ??????
    copySelectSQL2(),
    QParams() << QParam("grp_id_src", otInteger, grp_id_src)
              << QParam("grp_id_dest", otInteger, grp_id_dest));

  try
  {
    Qry.get().Execute();
  }
  catch(const EOracleError& E)
  {
    if (E.Code==1)
      copyDBOneByOne(grp_id_src, grp_id_dest);
    else
      throw;
  };

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

bool TServicePaymentList::sameDocExists(const CheckIn::TPaxASVCItem& asvc) const
{
  for(const TServicePaymentItem item : *this)
    if (item.sameDoc(asvc)) return true;
  return false;
}

void TServicePaymentList::dump(const std::string &where) const
{
  for(TServicePaymentList::const_iterator i=begin(); i!=end(); ++i)
    ProgTrace(TRACE5, "%s: %s", where.c_str(), i->traceStr().c_str());
}

void TPaidRFISCAndServicePaymentListWithAuto::fromDB(int grp_id)
{
  clear();

  {
    //??????? ??????? ??? ?????? ? ???????????????? ?????????
    TPaidRFISCListWithAuto paid;
    paid.fromDB(grp_id, true);
    TPaidRFISCStatusList paidStatuses;
    paidStatuses.set(paid);
    for(const TPaidRFISCStatus& status : paidStatuses) emplace_back(status, boost::none);
  }

  //????? ?????? ?????? ?????? ? ?????????? ??????
  CheckIn::TServicePaymentListWithAuto payment;
  payment.fromDB(grp_id);
  payment.sort();

  if (payment.empty()) return;

  for(bool unboundPaymentPass : {false, true})
    for(auto& i : *this)
    {
      if (i.first.status==TServiceStatus::Free || i.second) continue; //?????? ?????????? ??? ??? ????????? ??????
      const TPaidRFISCStatus& rfisc=i.first;
      for(CheckIn::TServicePaymentListWithAuto::iterator iPayment=payment.begin(); iPayment!=payment.end(); ++iPayment)
      {
        if (
            iPayment->service_quantity > 0 &&
            iPayment->pc &&
            ((!unboundPaymentPass && iPayment->pax_id!=ASTRA::NoExists && iPayment->pax_id==rfisc.pax_id) ||
             (unboundPaymentPass && iPayment->pax_id==ASTRA::NoExists)) &&
            iPayment->trfer_num==rfisc.trfer_num &&
            iPayment->pc->key()==rfisc.key())
        {
          // ???? ???????? ?? ????????? ?????
          // ?????? ????????? ??????????? ?????? ????? ????????????? ???????? ??????
          // ? service_quantity ?? 1 ??????, ??? ? ??????????
          i.second=*iPayment;
          if((--iPayment->service_quantity) == 0)
              payment.erase(iPayment);
          break;
        }
      }
    }
}

const TPaidRFISCAndServicePaymentListWithAuto &TServiceReport::get(int grp_id)
{
    auto &service_info = services_map[grp_id];
    if(not service_info) {
        service_info = boost::in_place();
        service_info->fromDB(grp_id);
    }
    return service_info.get();
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
        if (item.pc)
        {
          if (item.pc.get().RFISC.empty())
            throw UserException("MSG.EMD_ATTACHED_TO_UNKNOWN_RFISC", LParams()<<LParam("emd_no",item.no_str()));
        };
      }
      else
      {
        if (string((const char*)itemNode->name)!="item") continue;
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
    //???? ????????? EMD ? ?????????
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

  map< TPaxSegRFISCKey, pair<int/*????? ?????????? ???-??*/,
                             int/*??????????? ???-?? ??? ?????? ?????????*/> > tmp_payment;
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

TGrpEMDProps& TGrpEMDProps::fromDB(int grp_id, TGrpEMDProps &props)
{
  props.clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT * FROM paid_bag_emd_props WHERE grp_id=:grp_id";
  Qry.CreateVariable("grp_id", otInteger, grp_id);
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next())
    props.insert(TGrpEMDPropsItem(Qry.FieldAsString("emd_no"),
                                  Qry.FieldAsInteger("emd_coupon"),
                                  Qry.FieldAsInteger("manual_bind")!=0,
                                  Qry.FieldAsInteger("not_auto_checkin")!=0));
  return props;
}

void TGrpEMDProps::toDB(int grp_id, const TGrpEMDProps &props)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  UPDATE paid_bag_emd_props "
    "  SET grp_id=:grp_id, "
    "      manual_bind=NVL(:manual_bind, manual_bind), "
    "      not_auto_checkin=NVL(:not_auto_checkin, not_auto_checkin) "
    "  WHERE emd_no=:emd_no AND emd_coupon=:emd_coupon; "
    "  IF SQL%NOTFOUND THEN "
    "    INSERT INTO paid_bag_emd_props(grp_id, emd_no, emd_coupon, manual_bind, not_auto_checkin) "
    "    VALUES(:grp_id, :emd_no, :emd_coupon, NVL(:manual_bind, 0), NVL(:not_auto_checkin,0)); "
    "  END IF; "
    "END;";
  Qry.CreateVariable("grp_id", otInteger, grp_id);
  Qry.DeclareVariable("emd_no", otString);
  Qry.DeclareVariable("emd_coupon", otInteger);
  Qry.DeclareVariable("manual_bind" ,otInteger);
  Qry.DeclareVariable("not_auto_checkin" ,otInteger);
  for(TGrpEMDProps::const_iterator i=props.begin(); i!=props.end(); ++i)
  {
    Qry.SetVariable("emd_no", i->emd_no);
    Qry.SetVariable("emd_coupon", i->emd_coupon);
    i->manual_bind?Qry.SetVariable("manual_bind", (int)i->manual_bind.get()):
                   Qry.SetVariable("manual_bind", FNull);
    i->not_auto_checkin?Qry.SetVariable("not_auto_checkin", (int)i->not_auto_checkin.get()):
                        Qry.SetVariable("not_auto_checkin", FNull);
    Qry.Execute();
  };
}

} //namespace CheckIn
