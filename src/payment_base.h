#ifndef _PAYMENT_BASE_H_
#define _PAYMENT_BASE_H_

#include "exceptions.h"
#include "baggage_wt.h"
#include "rfisc.h"

namespace CheckIn
{

class TPaymentDoc
{
  public:
    std::string doc_type;
    std::string doc_aircode;
    std::string doc_no;
    int doc_coupon;
    int service_quantity;
  TPaymentDoc()
  {
    clear();
  }
  TPaymentDoc(const CheckIn::TPaxASVCItem& item)
  {
    clear();
    doc_type="EMD"+item.emd_type;
    doc_no=item.emd_no;
    doc_coupon=item.emd_coupon;
    service_quantity=item.service_quantity;
  }
  void clear()
  {
    doc_type.clear();
    doc_aircode.clear();
    doc_no.clear();
    doc_coupon=ASTRA::NoExists;
    service_quantity=ASTRA::NoExists;
  }
  bool operator == (const TPaymentDoc &doc) const
  {
    return doc_type==doc.doc_type &&
           doc_aircode==doc.doc_aircode &&
           doc_no==doc.doc_no &&
           doc_coupon==doc.doc_coupon &&
           service_quantity==doc.service_quantity;
  }
  bool operator < (const TPaymentDoc &doc) const
  {
    if (doc_type_priority() != doc.doc_type_priority())
      return doc_type_priority() < doc.doc_type_priority();
    if (doc_no != doc.doc_no)
      return doc_no < doc.doc_no;
    if (doc_coupon != doc.doc_coupon)
      return doc_coupon < doc.doc_coupon;
    return doc_aircode < doc.doc_aircode;
  }
  const TPaymentDoc& toXML(xmlNodePtr node) const;
  const TPaymentDoc& toXMLcompatible(xmlNodePtr node) const;
  TPaymentDoc& fromXML(xmlNodePtr node);
  TPaymentDoc& fromXMLcompatible(xmlNodePtr node);
  const TPaymentDoc& toDB(TQuery &Qry) const;
  TPaymentDoc& fromDB(TQuery &Qry);
  std::string no_str() const;
  std::string emd_no_str() const;
  bool isEMD() const;
  bool sameDoc(const TPaymentDoc& doc) const
  {
    return (isEMD() && doc.isEMD() &&
            doc_no==doc.doc_no && doc_coupon==doc.doc_coupon) ||
           (!isEMD() && !doc.isEMD() &&
            doc_no==doc.doc_no && doc_aircode==doc.doc_aircode);
  }
  bool service_quantity_valid() const { return service_quantity!=ASTRA::NoExists && service_quantity>0; }
  int doc_type_priority() const
  {
    if (doc_type=="EMDA")      return 0;
    else if (doc_type=="EMDS") return 1;
    else if (doc_type=="MCO")  return 2;
    else                       return 3;
  }
  std::string traceStr() const;
  bool notCompatibleWithPriorTermVersions() const;
};

class TServicePaymentItem : public TPaymentDoc
{
  public:
    int pax_id;
    int trfer_num;
    boost::optional<TRFISCKey> pc;
    boost::optional<TBagTypeKey> wt;
    int doc_weight;
  TServicePaymentItem()
  {
    clear();
  }
  TServicePaymentItem(const TGrpServiceAutoItem& item) : TPaymentDoc(item)
  {
    pax_id=item.pax_id;
    trfer_num=item.trfer_num;
    pc=TRFISCKey();
    pc.get().list_item=TRFISCListItem(item);
    (TRFISCListKey&)(pc.get())=pc.get().list_item.get();
  }
  void clear()
  {
    pax_id=ASTRA::NoExists;
    trfer_num=ASTRA::NoExists;
    pc=boost::none;
    wt=boost::none;
    doc_weight=ASTRA::NoExists;
  }
  bool operator == (const TServicePaymentItem &item) const
  {
    return TPaymentDoc::operator ==(item) &&
           //pax_id==item.pax_id &&
           trfer_num==item.trfer_num &&
           pc==item.pc &&
           wt==item.wt &&
           doc_weight==item.doc_weight;
  }
  bool operator < (const TServicePaymentItem &item) const
  {
    if (!(pc==item.pc))
      return (pc && !item.pc) ||
             (pc && item.pc && pc.get() < item.pc.get());
    if (!(wt==item.wt))
      return (wt && !item.wt) ||
             (wt && item.wt && wt.get() < item.wt.get());
    return TPaymentDoc::operator <(item);
  }
  const TServicePaymentItem& toXML(xmlNodePtr node) const;
  const TServicePaymentItem& toXMLcompatible(xmlNodePtr node) const;
  TServicePaymentItem& fromXML(xmlNodePtr node, bool is_unaccomp);
  TServicePaymentItem& fromXMLcompatible(xmlNodePtr node, bool baggage_pc);
  const TServicePaymentItem& toDB(TQuery &Qry) const;
  TServicePaymentItem& fromDB(TQuery &Qry);
  bool similar(const TServicePaymentItem &item) const
  {
    return sameDoc(item) &&
           trfer_num==item.trfer_num &&
           pc==item.pc &&
           wt==item.wt;
  }

  std::string traceStr() const;

  std::string key_str(const std::string& lang="") const;
  std::string key_str_compatible() const;
  bool is_auto_service() const
  {
    return pc && pc.get().service_type==TServiceType::Unknown;
  }
};

template <class T>
void splitServicePaymentItems(const T &src,
                              T &compatible,
                              T &not_compatible,
                              T &other_svc)
{
  compatible.clear();
  not_compatible.clear();
  other_svc.clear();

  for(typename T::const_iterator i=src.begin(); i!=src.end(); ++i)
  {
    if (i->pc)
    {
      if (!i->pc.get().list_item)  throw EXCEPTIONS::Exception("%s: !i->pc.get().list_item", __FUNCTION__);
      if (!i->pc.get().list_item.get().carry_on())
      {
        other_svc.push_back(*i);
        continue; //выводим только багаж
      };
    }
    if (i->notCompatibleWithPriorTermVersions())
    {
      not_compatible.push_back(*i);
      continue; //не выводим EMDS МСО service_quantity>1
    };
    compatible.push_back(*i);
  };
}

class TServicePaymentList : public std::list<TServicePaymentItem>
{
  public:
    void getCompatibleWithPriorTermVersions(TServicePaymentList &compatible,
                                            TServicePaymentList &not_compatible,
                                            TServicePaymentList &other_svc) const
    {
      splitServicePaymentItems<TServicePaymentList>(*this, compatible, not_compatible, other_svc);
    }
    void fromDB(int grp_id);
    void toDB(int grp_id) const;
    void getAllListKeys(int grp_id, bool is_unaccomp);
    void getAllListItems(int grp_id, bool is_unaccomp);
    bool dec(const TPaxSegRFISCKey &key);
    bool equal(const TServicePaymentList &list) const;
    void dump(const std::string &where) const;
    static void clearDB(int grp_id);
    static void copyDB(int grp_id_src, int grp_id_dest);
};

class TServicePaymentListWithAuto : public std::list<TServicePaymentItem>
{
  public:
    void getCompatibleWithPriorTermVersions(TServicePaymentListWithAuto &compatible,
                                            TServicePaymentListWithAuto &not_compatible,
                                            TServicePaymentListWithAuto &other_svc) const
    {
      splitServicePaymentItems<TServicePaymentListWithAuto>(*this, compatible, not_compatible, other_svc);
    }
    int getDocWeight(const TBagTypeListKey &key) const;
    void fromDB(int grp_id);
    void toXML(xmlNodePtr node) const;
};

void ServicePaymentFromXML(xmlNodePtr node,
                           int grp_id,
                           bool is_unaccomp,
                           bool baggage_pc,
                           boost::optional<TServicePaymentList> &payment);

void TryCleanServicePayment(const WeightConcept::TPaidBagList &curr_paid,
                            const CheckIn::TServicePaymentList &prior_payment,
                            boost::optional< CheckIn::TServicePaymentList > &curr_payment);

bool TryCleanServicePayment(const TPaidRFISCList &curr_paid,
                            CheckIn::TServicePaymentList &curr_payment);

class TPaidBagEMDPropsItem
{
  public:
    std::string emd_no;
    int emd_coupon;
    bool manual_bind;
  TPaidBagEMDPropsItem(const std::string &_emd_no,
                       const int &_emd_coupon,
                       bool _manual_bind=false)
  {
    emd_no=_emd_no;
    emd_coupon=_emd_coupon;
    manual_bind=_manual_bind;
  }
  TPaidBagEMDPropsItem(const TPaymentDoc &doc, bool _manual_bind=false)
  {
    emd_no=doc.doc_no;
    emd_coupon=doc.doc_coupon;
    manual_bind=_manual_bind;
  }
  bool operator < (const TPaidBagEMDPropsItem &item) const
  {
    if(emd_no != item.emd_no)
      return emd_no < item.emd_no;
    return emd_coupon < item.emd_coupon;
  }
};

class TPaidBagEMDProps : public std::set<TPaidBagEMDPropsItem>
{
  public:
    TPaidBagEMDPropsItem get(const std::string &_emd_no,
                             const int &_emd_coupon) const
    {
      TPaidBagEMDPropsItem item(_emd_no, _emd_coupon);
      TPaidBagEMDProps::const_iterator i=find(item);
      return i!=end()?*i:item;
    }
};

void PaidBagEMDPropsFromDB(int grp_id, TPaidBagEMDProps &props);
void PaidBagEMDPropsToDB(int grp_id, const TPaidBagEMDProps &props);

} //namespace CheckIn

#endif
