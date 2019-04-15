#ifndef _PAYMENT_BASE_H_
#define _PAYMENT_BASE_H_

#include "exceptions.h"
#include "baggage_wt.h"
#include "rfisc.h"
#include <etick/tick_data.h>
#include <forward_list>

class TComplexBagExcess
{
  friend class TComplexBagExcessNodeList;
  private:
    TBagQuantity pc, wt;
  public:
    TComplexBagExcess(const TBagQuantity& excess1,
                      const TBagQuantity& excess2);
    std::string view(const AstraLocale::OutputLang &lang,
                     const bool &unitRequired,
                     const std::string& separator="/") const;
    std::string deprecatedView(const AstraLocale::OutputLang &lang) const;
    bool zero() const { return pc.zero() && wt.zero(); }
    int getDeprecatedInt() const;

    bool operator == (const TComplexBagExcess &item) const
    {
      return pc==item.pc &&
             wt==item.wt;
    }

    TComplexBagExcess& operator += (const TComplexBagExcess &item)
    {
      pc+=item.pc;
      wt+=item.wt;
      return *this;
    }
};

class TComplexBagExcessNodeItem
{
  public:
    xmlNodePtr node;
    TComplexBagExcess complexExcess;

    TComplexBagExcessNodeItem(xmlNodePtr _node,
                              const TComplexBagExcess& _complexExcess) :
      node(_node), complexExcess(_complexExcess) {}
};


class TComplexBagExcessNodeList
{
  public:
    enum Props {ContainsOnlyNonZeroExcess, UnitRequiredAnyway, DeprecatedIntegerOutput};

  private:
    void apply() const;
    std::forward_list<TComplexBagExcessNodeItem> excessList;
    bool pcExists, wtExists;
    bool _containsOnlyNonZeroExcess;
    bool _unitRequiredAnyway;
    bool _deprecatedIntegerOutput;
    AstraLocale::OutputLang _lang;

    std::string _separator;
  public:
    void add(xmlNodePtr parent,
             const char *name,
             const TBagQuantity& excess1,
             const TBagQuantity& excess2);

    TComplexBagExcessNodeList(const AstraLocale::OutputLang &lang,
                              const std::set<Props> &props,
                              const std::string& separator="/") :
      pcExists(false),
      wtExists(false),
      _containsOnlyNonZeroExcess(props.find(ContainsOnlyNonZeroExcess)!=props.end()),
      _unitRequiredAnyway(props.find(UnitRequiredAnyway)!=props.end()),
      _deprecatedIntegerOutput(props.find(DeprecatedIntegerOutput)!=props.end()),
      _lang(lang),
      _separator(separator) {}
    ~TComplexBagExcessNodeList() { apply(); }

    bool unitRequired() const { return (pcExists && wtExists) || _unitRequiredAnyway; }
};

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
    clear();
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
        if (!i->pc.get().list_item.get().isBaggageOrCarryOn())
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
  private:
    static std::string copySelectSQL();
    static std::string copySelectSQL2();
    static void copyDBOneByOne(int grp_id_src, int grp_id_dest);
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
    bool sameDocExists(const CheckIn::TPaxASVCItem& asvc) const;
    void dump(const std::string &where) const;
    static void clearDB(int grp_id);
    static void copyDB(int grp_id_src, int grp_id_dest);
};

class TServicePaymentListWithAuto : public std::list<TServicePaymentItem>, public TRFISCListItemsCache
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
    bool isRFISCGrpExists(int pax_id, const std::string &grp, const std::string &subgrp) const;
    void getUniqRFISCSs(int pax_id, std::set<std::string> &rfisc_set) const;
    void dump(const std::string &file, int line) const;
};

class TPaidRFISCAndServicePaymentListWithAuto : public std::list< std::pair<TPaidRFISCStatus, boost::optional<TServicePaymentItem> > >, public TRFISCListItemsCache
{
    public:
        void fromDB(int grp_id);
};

class TServiceReport {
    private:
        std::map<int, boost::optional<TPaidRFISCAndServicePaymentListWithAuto>> services_map;
    public:
        const TPaidRFISCAndServicePaymentListWithAuto &get(int grp_id);
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

class TGrpEMDProps;

class TGrpEMDPropsItem
{
  friend class TGrpEMDProps;

  private:
    boost::optional<bool> manual_bind;
    boost::optional<bool> not_auto_checkin;
  public:
    std::string emd_no;
    int emd_coupon;
    bool get_manual_bind() const { return manual_bind?manual_bind.get():false; }
    bool get_not_auto_checkin() const { return not_auto_checkin?not_auto_checkin.get():false; }
    TGrpEMDPropsItem(const std::string &_emd_no,
                     const int &_emd_coupon,
                     bool _manual_bind,
                     bool _not_auto_checkin)
    {
      emd_no=_emd_no;
      emd_coupon=_emd_coupon;
      manual_bind=_manual_bind;
      not_auto_checkin=_not_auto_checkin;
    }
    TGrpEMDPropsItem(const TServicePaymentItem &doc)
    {
      emd_no=doc.doc_no;
      emd_coupon=doc.doc_coupon;
      manual_bind=true;
      not_auto_checkin=boost::none;
    }
    TGrpEMDPropsItem(const TGrpServiceAutoItem &svc)
    {
      emd_no=svc.emd_no;
      emd_coupon=svc.emd_coupon;
      manual_bind=true;
      not_auto_checkin=true;
    }
    bool operator < (const TGrpEMDPropsItem &item) const
    {
      if(emd_no != item.emd_no)
        return emd_no < item.emd_no;
      return emd_coupon < item.emd_coupon;
    }
};

class TGrpEMDProps : public std::set<TGrpEMDPropsItem>
{
  public:
    TGrpEMDPropsItem get(const std::string &_emd_no,
                         const int &_emd_coupon) const
    {
      TGrpEMDPropsItem item(_emd_no, _emd_coupon, boost::none, boost::none);
      TGrpEMDProps::const_iterator i=find(item);
      return i!=end()?*i:item;
    }
    static TGrpEMDProps& fromDB(int grp_id, TGrpEMDProps &props);
    static void toDB(int grp_id, const TGrpEMDProps &props);
};

} //namespace CheckIn

#endif
