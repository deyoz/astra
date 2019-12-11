#include "rfisc_sirena.h"
#include "baggage_calc.h"
#include "payment_base.h"
#include <regex>

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace std;
using namespace AstraLocale;

namespace SirenaExchange
{

const std::string TAvailability::id="svc_availability";
const std::string TPaymentStatus::id="svc_payment_status";
const std::string TGroupInfo::id="group_svc_info";
const std::string TPseudoGroupInfo::id="pseudogroup_svc_info";
const std::string TPassengers::id="passenger_with_svc";

const TSegItem& TSegItem::toSirenaXML(xmlNodePtr node, const OutputLang &lang) const
{
  if (node==NULL) return *this;

  SetProp(node, "id", id);
  if (markFlt)
  {
    SetProp(node, "company", airlineToPrefferedCode(markFlt.get().airline, lang));
    SetProp(node, "flight", flight(markFlt.get(), lang));
  };
  SetProp(node, "operating_company", airlineToPrefferedCode(operFlt.airline, lang));
  SetProp(node, "operating_flight", flight(operFlt, lang));
  SetProp(node, "departure", airpToPrefferedCode(operFlt.airp, lang));
  SetProp(node, "arrival", airpToPrefferedCode(airp_arv, lang));
  if (operFlt.scd_out!=ASTRA::NoExists)
  {
    if (scd_out_contain_time)
      SetProp(node, "departure_time", DateTimeToStr(operFlt.scd_out, "yyyy-mm-ddThh:nn:ss")); //локальное время
    else
      SetProp(node, "departure_date", DateTimeToStr(operFlt.scd_out, "yyyy-mm-dd")); //локальная дата

  }
  if (scd_in!=ASTRA::NoExists)
  {
    if (scd_in_contain_time)
      SetProp(node, "arrival_time", DateTimeToStr(scd_in, "yyyy-mm-ddThh:nn:ss")); //локальное время
    else
      SetProp(node, "arrival_date", DateTimeToStr(scd_in, "yyyy-mm-dd")); //локальная дата

  }
  SetProp(node, "equipment", craftToPrefferedCode(operFlt.craft, lang), "");

  return *this;
}

void TPaxSegItem::set(const CheckIn::TPaxTknItem& _tkn, TPaxSection* paxSection)
{
  tkn=_tkn;
  display_id=ASTRA::NoExists;
  if (paxSection!=nullptr && tkn.validET())
    display_id=paxSection->displays.add(TETickItem().fromDB(tkn.no, tkn.coupon, TETickItem::DisplayTlg, false).ediPnr);
}

const TPaxSegItem& TPaxSegItem::toSirenaXML(xmlNodePtr node, const OutputLang &lang) const
{
  if (node==NULL) return *this;

  TSegItem::toSirenaXML(node, lang);
  SetProp(node, "subclass", ElemIdToPrefferedElem(etSubcls, subcl, efmtCodeNative, lang.get()));
  if (!tkn.no.empty())
  {
    xmlNodePtr tknNode=NewTextChild(node, "ticket");
    SetProp(tknNode, "number", tkn.no);
    SetProp(tknNode, "coupon_num", tkn.coupon, ASTRA::NoExists);
    SetProp(tknNode, "display_id", display_id, ASTRA::NoExists);
  }
  pnrAddrs.toSirenaXML(node, lang);
  for(std::set<CheckIn::TPaxFQTItem>::const_iterator i=fqts.begin(); i!=fqts.end(); ++i)
    SetProp(NewTextChild(node, "ffp", i->no), "company", airlineToPrefferedCode(i->airline, lang));

  return *this;
}

std::string TSegItem::flight(const TTripInfo &flt, const OutputLang &lang)
{
  ostringstream s;
  s << setw(3) << setfill('0') << flt.flt_no
    << ElemIdToPrefferedElem(etSuffix, flt.suffix, efmtCodeNative, lang.get());
  return s.str();
}

const TPaxItem& TPaxItem::toSirenaXML(xmlNodePtr node, const OutputLang &lang) const
{
  if (node==NULL) return *this;

  SetProp(node, "id", id);
  SetProp(node, "surname", surname);
  SetProp(node, "name", name, "");
  SetProp(node, "category", category(), "");
  if (doc.birth_date!=ASTRA::NoExists)
    SetProp(node, "birthdate", DateTimeToStr(doc.birth_date, "yyyy-mm-dd"));
  SetProp(node, "sex", sex(), "");
  if (!doc.type_rcpt.empty() || !doc.no.empty() || !doc.issue_country.empty())
  {
    xmlNodePtr docNode=NewTextChild(node, "document");
    SetProp(docNode, "type", doc.type_rcpt, "");
    SetProp(docNode, "number", doc.no, "");
    if (doc.expiry_date!=ASTRA::NoExists)
      SetProp(docNode, "expiration_date", DateTimeToStr(doc.expiry_date, "yyyy-mm-dd"));
    SetProp(docNode, "country", PaxDocCountryIdToPrefferedElem(doc.issue_country, efmtCodeISOInter, lang.get()), "");
  }

  for(TPaxSegMap::const_iterator i=segs.begin(); i!=segs.end(); ++i)
    i->second.toSirenaXML(NewTextChild(node, "segment"), lang);

  return *this;
}

std::string TPaxItem::sex() const
{
  int f=CheckIn::is_female(doc.gender, name);
  if (f==ASTRA::NoExists) return "";
  return (f==0?"male":"female");
}

const TPaxItem2& TPaxItem2::toSirenaXML(xmlNodePtr node, const OutputLang &lang) const
{
  if (node==NULL) return *this;

  NewTextChild( node, "surname", surname );
  NewTextChild( node, "name", name );
  NewTextChild( node, "category", category());
  NewTextChild( node, "group_id", grp_id );
  NewTextChild( node, "reg_no", reg_no );
  pnrAddrs.toSirenaXML(node, lang);
  return *this;
}

const TSvcItem& TSvcItem::toSirenaXML(xmlNodePtr node, const OutputLang &lang) const
{
  if (node==NULL) return *this;

  TPaxSegRFISCKey::toSirenaXML(node, lang);
  SetProp(node, "paid", status==TServiceStatus::Paid?"true":"false", "false");
  SetProp(node, "ssr_code", ssr_code, "");
  SetProp(node, "ssr_text", ssr_text, "");
  return *this;
}

TSvcItem& TSvcItem::fromSirenaXML(xmlNodePtr node)
{
  clear();
  TPaxSegRFISCKey::fromSirenaXML(node);

  try
  {
    if (node==NULL) throw Exception("node not defined");

    string xml_status=NodeAsString("@payment_status", node, "");
    if (xml_status.empty()) throw Exception("Empty @payment_status");
    try
    {
      status=ServiceStatuses().decode(xml_status);
    }
    catch(EConvertError)
    {
      throw Exception("Wrong @payment_status='%s'", xml_status.c_str());
    };
  }
  catch(Exception &e)
  {
    throw Exception("TSvcItem::fromSirenaXML: %s", e.what());
  };
  return *this;
}

const TDisplayItem& TDisplayItem::toSirenaXML(xmlNodePtr displayParentNode) const
{
  if (displayParentNode==nullptr) return *this;

  xmlNodePtr displayNode=NewTextChild(displayParentNode, "display",
                                      regex_replace(ediText(), regex(regexInvalidXMLChars), "#"));
  SetProp(displayNode, "id", id, ASTRA::NoExists);
  return *this;
}

void TPaxSection::toXML(xmlNodePtr node) const
{
  if (paxs.empty() && throw_if_empty) throw Exception("TPaxSection::toXML: paxs.empty()");

  for(const TPaxItem& pax : paxs)
    pax.toSirenaXML(NewTextChild(node, "passenger"), OutputLang(LANG_EN, {OutputLang::OnlyTrueIataCodes}));

  for(const TDisplayItem& display : displays)
    display.toSirenaXML(node);
}

void TPaxSection::updateSeg(const Sirena::TPaxSegKey &key)
{
  for(list<TPaxItem>::iterator p=paxs.begin(); p!=paxs.end(); ++p)
  {
    if (p->id!=key.pax_id) continue;
    if (p->segs.empty() || p->segs.begin()->first==key.trfer_num) continue;

    int trfer_num=key.trfer_num;
    TPaxSegMap new_segs;
    for(TPaxSegMap::const_iterator s=p->segs.begin(); s!=p->segs.end(); ++s, trfer_num++)
      new_segs.insert(make_pair(trfer_num,s->second));
    p->segs=new_segs;
    for(TPaxSegMap::iterator s=p->segs.begin(); s!=p->segs.end(); ++s)
      s->second.id=s->first;
  };
}

void TSvcSection::toXML(xmlNodePtr node) const
{
  if (svcs.empty() && throw_if_empty) throw Exception("TSvcSection::toXML: svcs.empty()");

  for(TSvcList::const_iterator i=svcs.begin(); i!=svcs.end(); ++i)
    i->toSirenaXML(NewTextChild(node, "svc"), OutputLang(LANG_EN, {OutputLang::OnlyTrueIataCodes}));
}

void TSvcSection::updateSeg(const Sirena::TPaxSegKey &key)
{
  for(TSvcList::iterator i=svcs.begin(); i!=svcs.end(); ++i)
    if (i->pax_id==key.pax_id) i->trfer_num=key.trfer_num;
}

void TAvailabilityReq::toXML(xmlNodePtr node) const
{
  if (node==NULL) return;

  SetProp(node, "show_brand_info", "true");
  SetProp(node, "show_all_svc", "true");
  SetProp(node, "show_free_carry_on_norm", "true");

  TPaxSection::toXML(node);
}

void TAvailabilityResItem::remove_unnecessary()
{
  for(TRFISCList::iterator i=rfisc_list.begin(); i!=rfisc_list.end();)
  {
    const TRFISCListItem &item=i->second;
    if (item.isBaggageOrCarryOn())
    {
      bool carryOn=item.isCarryOn();
      //багаж или р/кладь
      if ((!carryOn && baggage_norm.airline!=item.airline) ||
          (carryOn && carry_on_norm.airline!=item.airline))
      {
        i=Erase<TRFISCList>(rfisc_list, i);
        continue;
      };
    };
    ++i;
  };
}

void TAvailabilityRes::fromXML(xmlNodePtr node)
{
  clear();

  try
  {
    if (node==NULL) throw Exception("node not defined");
    for(node=node->children; node!=NULL; node=node->next)
    {
      if (string((const char*)node->name)!="svc_list") continue;
      Sirena::TPaxSegKey key;
      key.fromSirenaXML(node);
      if (find(key)!=end())
        throw Exception("Duplicate passenger-id=%d segment-id=%d", key.pax_id, key.trfer_num);
      TAvailabilityResItem &item=insert(make_pair(key, TAvailabilityResItem())).first->second;
      item.rfisc_list.fromSirenaXML(node);
      item.baggage_norm.fromSirenaXMLAdv(node, false);
      item.carry_on_norm.fromSirenaXMLAdv(node, true);
      item.brand.fromSirenaXMLAdv(node);
      item.remove_unnecessary();
    };
    if (empty()) throw Exception("empty()");
  }
  catch(Exception &e)
  {
    throw Exception("TAvailabilityRes::fromXML: %s", e.what());
  };
}

bool TAvailabilityRes::identical_concept(int seg_id, bool carry_on, boost::optional<TBagConcept::Enum> &concept) const
{
  concept=boost::none;
  for(TAvailabilityResMap::const_iterator i=begin(); i!=end(); ++i)
  {
    if (i->first.trfer_num!=seg_id) continue;
    const Sirena::TSimplePaxNormItem &norm=carry_on?i->second.carry_on_norm:i->second.baggage_norm;
    if (!concept || concept.get()==TBagConcept::No)
      concept=norm.concept;
    else
    {
      if (concept.get()!=TBagConcept::No && norm.concept!=TBagConcept::No && concept.get()!=norm.concept)
      {
        concept=boost::none;
        return false;
      };
    };
  };
  return true;
}

void TAvailabilityRes::rfiscsToDB(const TCkinGrpIds &tckin_grp_ids, TBagConcept::Enum bag_concept, bool old_version) const
{
  TPaxServiceLists serviceLists;
  for(TAvailabilityRes::const_iterator i=begin(); i!=end(); ++i)
  {
    if (i->second.rfisc_list.empty()) continue;

    for(int pass=0; pass<3; pass++)
    {
      TServiceCategory::Enum cat;
      switch(pass)
      {
        case 1: cat=TServiceCategory::BaggageInHold; break;
        case 2: cat=TServiceCategory::BaggageInCabinOrCarryOn; break;
        default: cat=TServiceCategory::Other; break;
      };

      if (old_version && cat==TServiceCategory::BaggageInCabinOrCarryOn) continue;//!!! потом удалить

      if (cat==TServiceCategory::BaggageInHold ||
          cat==TServiceCategory::BaggageInCabinOrCarryOn)
      {
        if (bag_concept==TBagConcept::Weight ||
            bag_concept==TBagConcept::Unknown) continue;

        if (bag_concept==TBagConcept::No)
        {
          ProgError(STDLOG, "%s: strange situation bag_concept==TBagConcept::No!", __FUNCTION__);
          continue;
        }

        //проверяем норму
//        const Sirena::TSimplePaxNormItem& norm=(cat==TServiceCategory::Baggage)?i->second.baggage_norm:
//                                                                                i->second.carry_on_norm;
//        if (norm.concept==TBagConcept::Weight ||
//            norm.concept==TBagConcept::Unknown) continue;
//        if (norm.concept==TBagConcept::No)
//        {
//           boost::optional<TBagConcept::Enum> seg_concept;
//           identical_concept(i->first.trfer_num, cat==TServiceCategory::CarryOn, seg_concept);
//           if ((!seg_concept || seg_concept.get()==TBagConcept::No) &&
//               !i->second.rfisc_list.exists(TServiceType::BaggageCharge)) continue; //по сути - weight
//           if (seg_concept && seg_concept.get()==TBagConcept::Weight) continue;
//        };
      }

      TPaxServiceListsItem serviceItem;
      serviceItem.pax_id=i->first.pax_id;
      serviceItem.trfer_num=i->first.trfer_num;
      serviceItem.list_id=i->second.rfisc_list.toDBAdv();
      serviceItem.category=cat;
      serviceLists.insert(serviceItem);
      if (old_version && cat==TServiceCategory::BaggageInHold)
      {
        //!!! потом удалить
        //специально сделано чтобы при разных концептах багажа и ручной клади применялся багажный
        serviceItem.category=TServiceCategory::BaggageInCabinOrCarryOn;
        serviceLists.insert(serviceItem);
      }
    };
  }
  for(TCkinGrpIds::const_iterator i=tckin_grp_ids.begin(); i!=tckin_grp_ids.end(); ++i)
  {
    if (i==tckin_grp_ids.begin())
      serviceLists.toDB(false);
    else
      CopyPaxServiceLists(*tckin_grp_ids.begin(), *i, false, true);
  }
}

void TAvailabilityRes::setAdditionalListId(const TCkinGrpIds &tckin_grp_ids) const
{
  if (tckin_grp_ids.size()<=1) return;

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText = "BEGIN ckin.set_additional_list_id(:grp_id); END;";
  Qry.DeclareVariable("grp_id", otInteger);
  for(int grp_id : tckin_grp_ids)
  {
    Qry.SetVariable("grp_id", grp_id);
    Qry.Execute();
  }
}

void TAvailabilityRes::normsToDB(const TCkinGrpIds &tckin_grp_ids) const
{
  list<Sirena::TPaxNormItem> normsList;
  for(TAvailabilityResMap::const_iterator i=begin(); i!=end(); ++i)
  {
    for(bool carry_on=false;; carry_on=!carry_on)
    {
      if ((carry_on?i->second.carry_on_norm:i->second.baggage_norm).empty()) continue;
      Sirena::TPaxNormItem item;
      static_cast<Sirena::TSimplePaxNormItem&>(item)=carry_on?i->second.carry_on_norm:i->second.baggage_norm;
      item.pax_id=i->first.pax_id;
      item.trfer_num=i->first.trfer_num;
      normsList.push_back(item);

      if (carry_on) break;
    };
  }
  Sirena::PaxNormsToDB(tckin_grp_ids, normsList);
}

void TAvailabilityRes::brandsToDB(const TCkinGrpIds &tckin_grp_ids) const
{
  list<Sirena::TPaxBrandItem> brandsList;
  for(TAvailabilityResMap::const_iterator i=begin(); i!=end(); ++i)
  {
    if (i->second.brand.empty()) continue;
    Sirena::TPaxBrandItem item;
    static_cast<Sirena::TSimplePaxBrandItem&>(item)=i->second.brand;
    item.pax_id=i->first.pax_id;
    item.trfer_num=i->first.trfer_num;
    brandsList.push_back(item);
  }
  Sirena::PaxBrandsToDB(tckin_grp_ids, brandsList);
}

void TSvcList::get(const std::list<TSvcItem>& svcsAuto, TPaidRFISCList &paid) const
{
  paid.clear();

  std::list<TSvcItem> tmpAuto(svcsAuto);
  for(const TSvcItem& svc : *this)
  {
    std::list<TSvcItem>::iterator i=tmpAuto.begin();
    for(; i!=tmpAuto.end(); ++i)
      if (svc==*i && svc.status==i->status) break;
    if (i!=tmpAuto.end())
    {
      tmpAuto.erase(i);
      continue;
    }
    paid.inc(svc, svc.status);
  }
}

void TSvcList::addBaggageOrCarryOn(int pax_id, const TRFISCKey& key)
{
  _additionalBagList.emplace_back(TPaxSegRFISCKey(Sirena::TPaxSegKey(pax_id, 0), key), 1);
}

void TSvcList::addChecked(const TCheckedReqPassengers &req_grps, int grp_id, int tckin_seg_count, int trfer_seg_count)
{
  //вручную введенные на стойке
  TGrpServiceList svcs;
  svcs.fromDB(grp_id, !req_grps.include_refused);
  svcs.addBagInfo(grp_id, tckin_seg_count, trfer_seg_count, req_grps.include_refused);
  svcs.addBagList(_additionalBagList, tckin_seg_count, trfer_seg_count);

  TPaidRFISCList paid;
  paid.fromDB(grp_id, true);
  TPaidRFISCStatusList statusList;
  statusList.set(paid);

  CheckIn::TServicePaymentList payment;
  payment.fromDB(grp_id);
  for(const TGrpServiceItem& svc : svcs)
  {
    for(int j=svc.service_quantity; j>0; j--)
    {
      if (statusList.deleteIfFound(TPaidRFISCStatus(svc, TServiceStatus::Free)))
        emplace_back(svc, TServiceStatus::Need);
      else
        emplace_back(svc, payment.dec(svc)?TServiceStatus::Paid:TServiceStatus::Need);
    }
  }

  //автоматически зарегистрированные
  TGrpServiceAutoList svcsAuto;
  svcsAuto.fromDB(grp_id, true, !req_grps.include_refused);
  for(const TGrpServiceAutoItem& svcAuto : svcsAuto)
    for(TGrpServiceItem& svc : svcs)
      if (svc.similar(svcAuto))
      {
        for(int j=svcAuto.service_quantity; j>0; j--)
        {
          emplace_back(svc, svcAuto.serviceStatus());
          _autoChecked.emplace_back(svc, svcAuto.serviceStatus());
        }
        break;
      }

  //отфильтруем ненужные
  for(TSvcList::iterator i=begin(); i!=end();)
  {
    if (req_grps.only_first_segment && i->trfer_num!=0)
    {
      i=erase(i);
      continue;
    }
    if (!req_grps.pax_included(grp_id, i->pax_id))
    {
      i=erase(i);
      continue;
    }
    ++i;
  }
}

void TSvcList::addASVCs(int pax_id, const std::vector<CheckIn::TPaxASVCItem> &asvc)
{
  for(vector<CheckIn::TPaxASVCItem>::const_iterator i=asvc.begin(); i!=asvc.end(); ++i)
  {
    for(int j=i->service_quantity; j>0; j--)
    {
      TRFISCKey RFISCKey;
      RFISCKey.RFISC=i->RFISC;
      emplace_back(TPaxSegRFISCKey(Sirena::TPaxSegKey(pax_id, 0), RFISCKey),
                   i->withEMD()?TServiceStatus::Paid:TServiceStatus::Free);
    };
  }
}

void TSvcList::addUnbound(const TCheckedReqPassengers &req_grps, int grp_id, int pax_id)
{
  if (!req_grps.include_unbound_svcs ||
      !req_grps.pax_included(grp_id, pax_id)) return;

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText = PaxASVCList::GetSQL(PaxASVCList::unboundByPaxId);
  Qry.CreateVariable( "id", otInteger, pax_id );
  Qry.Execute();
  vector<CheckIn::TPaxASVCItem> asvc;
  for(;!Qry.Eof;Qry.Next())
    asvc.push_back(CheckIn::TPaxASVCItem().fromDB(Qry));
  addASVCs(pax_id, asvc);
}

void TSvcList::addFromCrs(const TNotCheckedReqPassengers &req_pnrs, int pnr_id, int pax_id)
{
  if (!req_pnrs.include_unbound_svcs ||
      !req_pnrs.pax_included(pnr_id, pax_id)) return;

  vector<CheckIn::TPaxASVCItem> asvc;
  CheckIn::LoadCrsPaxASVC(pax_id, asvc);
  addASVCs(pax_id, asvc);
}

bool deepTracing()
{
  const int developersDeskGrpId=1;
  return TReqInfo::Instance()->desk.grp_id==developersDeskGrpId;
}

void TPaymentStatusReq::toXML(xmlNodePtr node) const
{
  if (node==NULL) return;

  SetProp(node, "show_free_carry_on_norm", "true");
  SetProp(node, "set_pupil", deepTracing()?"true":"false", "false");

  TPaxSection::toXML(node);
  TSvcSection::toXML(node);
}

void TPaymentStatusRes::fromXML(xmlNodePtr node)
{
  clear();

  try
  {
    if (node==NULL) throw Exception("node not defined");
    for(node=node->children; node!=NULL; node=node->next)
    {
      string nodeName=(const char*)node->name;
      if (nodeName!="svc" &&
          nodeName!="free_baggage_norm" &&
          nodeName!="free_carry_on_norm") continue;
      Sirena::TPaxSegKey key;
      key.fromSirenaXML(node);
      if (nodeName=="svc")
      {
        TSvcItem &item=
          *(svcs.insert(svcs.end(), TSvcItem()));
        item.fromSirenaXML(node);
      };
      if (nodeName=="free_baggage_norm" ||
          nodeName=="free_carry_on_norm")
      {
        pair<Sirena::TPaxNormList::iterator, bool> res=
          norms.insert(make_pair(Sirena::TPaxNormListKey().fromSirenaXML(node), Sirena::TSimplePaxNormItem().fromSirenaXML(node)));
        if (!res.second)
          throw Exception("<%s> tag duplicated (passenger-id=%d segment-id=%d)",
                          nodeName.c_str(), res.first->first.pax_id, res.first->first.trfer_num);
      };
    };
    if (svcs.empty()) throw Exception("svcs.empty()");
    //if (norms.empty()) throw Exception("norms.empty()"); а надо ли?
  }
  catch(Exception &e)
  {
    throw Exception("TPaymentStatusRes::fromXML: %s", e.what());
  };
}

void TPaymentStatusRes::normsToDB(const TCkinGrpIds &tckin_grp_ids) const
{
  list<Sirena::TPaxNormItem> normsList;
  for(Sirena::TPaxNormList::const_iterator i=norms.begin(); i!=norms.end(); ++i)
  {
    Sirena::TPaxNormItem item;
    static_cast<Sirena::TSimplePaxNormItem&>(item)=i->second;
    item.pax_id=i->first.pax_id;
    item.trfer_num=i->first.trfer_num;
    normsList.push_back(item);
  }
  Sirena::PaxNormsToDB(tckin_grp_ids, normsList);
}

void TPaymentStatusRes::check_unknown_status(int seg_id, std::set<TRFISCListKey> &rfiscs) const
{
  rfiscs.clear();
  for(TSvcList::const_iterator i=svcs.begin(); i!=svcs.end(); ++i)
  {
    if (i->trfer_num!=seg_id) continue;
    if (i->status==TServiceStatus::Unknown)
      rfiscs.insert(*i);
  }
}

void TPassengersReq::fromXML(xmlNodePtr node)
{
  clear();

  try
  {
    string str;
    if (node==NULL) throw Exception("node not defined");

    str = NodeAsString("company", node);
    if (str.empty()) throw Exception("Empty <company>");
    airline = ElemToElemId( etAirline, str, airline_fmt );
    if (airline_fmt==efmtUnknown) throw Exception("Unknown <company> '%s'", str.c_str());

    string flight=NodeAsString("flight", node);
    str=flight;
    if (str.empty()) throw Exception("Empty <flight>");
    if (IsLetter(*str.rbegin()))
    {
      suffix=string(1,*str.rbegin());
      str.erase(str.size()-1);
      if (str.empty()) throw Exception("Empty flight number <flight> '%s'", flight.c_str());
    };

    if ( StrToInt( str.c_str(), flt_no ) == EOF ||
         flt_no > 99999 || flt_no <= 0 ) throw Exception("Wrong flight number <flight> '%s'", flight.c_str());

    str=suffix;
    if (!str.empty())
    {
      suffix = ElemToElemId( etSuffix, str, suffix_fmt );
      if (suffix_fmt==efmtUnknown) throw Exception("Unknown flight suffix <flight> '%s'", flight.c_str());
    };

    str=NodeAsString("departure_date", node);
    if (str.empty()) throw Exception("Empty <departure_date>");
    if ( StrToDateTime(str.c_str(), "yyyy-mm-dd", scd_out) == EOF )
      throw Exception("Wrong <departure_date> '%s'", str.c_str());

    str=NodeAsString("departure", node);
    if (str.empty()) throw Exception("Empty <departure>");
    airp = ElemToElemId( etAirp, str, airp_fmt );
    if (airp_fmt==efmtUnknown) throw Exception("Unknown <departure> '%s'", str.c_str());
  }
  catch(Exception &e)
  {
    throw Exception("TPassengersReq::fromXML: %s", e.what());
  };
}

void TPassengersRes::toXML(xmlNodePtr node) const
{
  if (node==NULL) return;

  for(list<TPaxItem2>::const_iterator p=begin(); p!=end(); ++p)
    p->toSirenaXML(NewTextChild(node, "passenger"), OutputLang(LANG_EN, {OutputLang::OnlyTrueIataCodes}));
}

void TGroupInfoReq::fromXML(xmlNodePtr node)
{
  clear();

  try
  {
    if (node==NULL) throw Exception("node not defined");
    pnr_addr = NodeAsString( "regnum", node );
    if (pnr_addr.size()>20) throw Exception("Wrong <regnum> '%s'", pnr_addr.c_str());
    grp_id = NodeAsInteger( "group_id", node );
  }
  catch(Exception &e)
  {
    throw Exception("TGroupInfoReq::fromXML: %s", e.what());
  };
}

void TGroupInfoReq::toDB(TQuery &Qry)
{
  Qry.SetVariable("grp_id", grp_id);
  Qry.SetVariable("pnr_addr", pnr_addr);
}

void TGroupInfoRes::toXML(xmlNodePtr node) const
{
  if (node==NULL) return;

  TPaxSection::toXML(node);
  TSvcSection::toXML(node);
}

void TPseudoGroupInfoReq::fromXML(xmlNodePtr node)
{
  clear();

  try
  {
    if (node==NULL) throw Exception("node not defined");
    for(node=node->children; node!=NULL; node=node->next)
    {
      string nodeName=(const char*)node->name;
      if (nodeName!="entity") continue;
      Sirena::TPaxSegKey key;
      key.fromSirenaXML(node);
      entities.insert(key);
    };
    if (entities.empty()) throw Exception("entities.empty()");
  }
  catch(Exception &e)
  {
    throw Exception("TPseudoGroupInfoReq::fromXML: %s", e.what());
  };
}

void TPseudoGroupInfoRes::toXML(xmlNodePtr node) const
{
  if (node==NULL) return;

  TPaxSection::toXML(node);
  TSvcSection::toXML(node);
}

} //namespace SirenaExchange

void SvcSirenaInterface::procRequestsFromSirena(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace( TRACE5, "%s: %s", __FUNCTION__, XMLTreeToText(resNode->doc).c_str());
  reqNode=NodeAsNode("content", reqNode);

  reqNode=NodeAsNode("query", reqNode)->children;
  std::string exchangeId = (const char*)reqNode->name;
  ProgTrace( TRACE5, "%s: exchangeId=<%s>", __FUNCTION__, exchangeId.c_str() );

  resNode=NewTextChild(resNode, "content");
  resNode=NewTextChild(resNode, "answer");
  resNode=NewTextChild(resNode, exchangeId.c_str());

  try
  {
    if (exchangeId==SirenaExchange::TPassengers::id)
    {
      SirenaExchange::TPassengersReq req;
      SirenaExchange::TPassengersRes res;
      req.fromXML(reqNode);
      procPassengers( req, res );
      res.toXML(resNode);
    }
    else if (exchangeId==SirenaExchange::TGroupInfo::id)
    {
      SirenaExchange::TGroupInfoReq req;
      SirenaExchange::TGroupInfoRes res;
      req.fromXML(reqNode);
      procGroupInfo( req, res );
      res.toXML(resNode);
    }
    else if (exchangeId==SirenaExchange::TPseudoGroupInfo::id)
    {
      SirenaExchange::TPseudoGroupInfoReq req;
      SirenaExchange::TPseudoGroupInfoRes res;
      req.fromXML(reqNode);
      procPseudoGroupInfo( req, res );
      res.toXML(resNode);
    }
    else throw Exception("%s: Unknown request <%s>", __FUNCTION__, exchangeId.c_str());
  }
  catch(std::exception &e)
  {
    if (resNode->children!=NULL)
    {
      xmlUnlinkNode(resNode->children);
      xmlFreeNode(resNode->children);
    };
    SirenaExchange::TErrorRes res(exchangeId);
    res.error_code="0";
    res.error_message=e.what();
    res.errorToXML(resNode);
  }
}

void SvcSirenaInterface::procPassengers( const SirenaExchange::TPassengersReq &req, SirenaExchange::TPassengersRes &res )
{
  res.clear();

  TSearchFltInfo fltInfo;
  fltInfo.airline = req.airline;
  fltInfo.airp_dep = req.airp;
  fltInfo.flt_no = req.flt_no;
  fltInfo.suffix = req.suffix;
  fltInfo.scd_out = req.scd_out;
  fltInfo.scd_out_in_utc = false;
  //фактические
  list<TAdvTripInfo> flts;
  SearchFlt( fltInfo, flts);
  set<int> operating_point_ids;
  for ( list<TAdvTripInfo>::const_iterator iflt=flts.begin(); iflt!=flts.end(); iflt++ )
    operating_point_ids.insert(iflt->point_id);
  //коммерческие
  set<int> marketing_point_ids;
  SearchMktFlt( fltInfo, marketing_point_ids );
  TQuery Qry(&OraSession);
  set<int> paxs;
  for(int pass=0; pass<2; pass++)
  {
    ostringstream sql;
    sql << "SELECT pax.* "
           "FROM pax, pax_grp "
           "WHERE pax_grp.grp_id=pax.grp_id AND ";
    if (pass==0)
      sql << "      pax_grp.point_dep=:point_id AND ";
    else
      sql << "      pax_grp.point_id_mark=:point_id AND ";
    sql << "      pax.refuse IS NULL AND pax_grp.status NOT IN ('E') AND "
           "      EXISTS(SELECT pax_id FROM paid_rfisc WHERE pax_id=pax.pax_id AND need>0 AND rownum<2) ";
    const set<int> &point_ids=pass==0?operating_point_ids:marketing_point_ids;
    Qry.Clear();
    Qry.SQLText=sql.str().c_str();
    Qry.DeclareVariable("point_id", otInteger);
    for(set<int>::const_iterator i=point_ids.begin(); i!=point_ids.end(); ++i)
    {
      Qry.SetVariable("point_id", *i);
      Qry.Execute();
      for ( ; !Qry.Eof; Qry.Next() ) {
        if (!paxs.insert( Qry.FieldAsInteger( "pax_id" ) ).second) continue;
        CheckIn::TSimplePaxItem pax;
        pax.fromDB(Qry);
        TETickItem etick;
        if (pax.tkn.validET())
          etick.fromDB(pax.tkn.no, pax.tkn.coupon, TETickItem::Display, false);
        SirenaExchange::TPaxItem2 resPax;
        resPax.set(Qry.FieldAsInteger("grp_id"), pax, etick);
        resPax.pnrAddrs.getByPaxIdFast(pax.id);
        res.push_back( resPax );
      }
    }
  };
}

void SvcSirenaInterface::procGroupInfo( const SirenaExchange::TGroupInfoReq &req,
                                        SirenaExchange::TGroupInfoRes &res )
{
  res.clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT grp_id FROM pax_grp WHERE grp_id=:grp_id";
  Qry.CreateVariable("grp_id", otInteger, req.grp_id);
  Qry.Execute();
  if (Qry.Eof) throw Exception("%s: Unknown <group_id> '%d'", __FUNCTION__, req.grp_id);

  Qry.Clear();
  Qry.SQLText =
    "BEGIN "
    "  UPDATE pnr_addrs_pc SET grp_id=:grp_id WHERE addr=:pnr_addr; "
    "  IF SQL%NOTFOUND THEN "
    "    INSERT INTO pnr_addrs_pc(addr, grp_id) VALUES(:pnr_addr, :grp_id); "
    "  END IF; "
    "END;";
  Qry.CreateVariable("pnr_addr", otString, req.pnr_addr);
  Qry.CreateVariable("grp_id", otInteger, req.grp_id);
  Qry.Execute();

  CheckIn::TPaxGrpCategory::Enum grp_cat;
  TCkinGrpIds tckin_grp_ids;
  SirenaExchange::fillPaxsBags(req.grp_id, res, grp_cat, tckin_grp_ids);
}

void SvcSirenaInterface::procPseudoGroupInfo( const SirenaExchange::TPseudoGroupInfoReq &req,
                                              SirenaExchange::TPseudoGroupInfoRes &res )
{
  SirenaExchange::fillPaxsSvcs(req.entities, res);
}

int verifyHTTP(int argc,char **argv)
{
//  try
//  {
//    SirenaExchange::TPassengersReq req;
//    SirenaExchange::TPassengersRes res;
//    string reqText
//    (
//      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
//      "<query>"
//      "  <passenger_with_svc>"
//      "    <company>UT</company>"
//      "    <flight>454</flight>"
//      "    <departure_date>2015-10-20</departure_date>"
//      "    <departure>РЩН</departure>"
//      "  </passenger_with_svc>"
//      "</query>");
//    reqText = ConvertCodepage( reqText, "CP866", "UTF-8" );
//    SirenaExchange::SendTestRequest(reqText);
//    req.parse(reqText);
//    PieceConceptInterface::procPassengers(req, res);
//    string resText;
//    res.build(resText);
//    printf("%s\n", resText.c_str());
//  }
//  catch(std::exception &e)
//  {
//    printf("%s\n", e.what());
//  }

//  try
//  {
//    SirenaExchange::TGroupInfoReq req;
//    SirenaExchange::TGroupInfoRes res;
//    string reqText
//    (
//      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
//      "<query>"
//      "  <group_svc_info>"
//      "    <regnum>SDA12F</regnum>"
//      "    <group_id>34877</group_id>"
//      "  </group_svc_info>"
//      "</query>");
//    reqText = ConvertCodepage( reqText, "CP866", "UTF-8" );
//    SirenaExchange::SendTestRequest(reqText);
//    req.parse(reqText);
//    PieceConceptInterface::procGroupInfo(req, res);
//    string resText;
//    res.build(resText);
//    printf("%s\n", resText.c_str());
//  }
//  catch(std::exception &e)
//  {
//    printf("%s\n", e.what());
//  }

  try
  {
    SirenaExchange::TPseudoGroupInfoReq req;
    SirenaExchange::TPseudoGroupInfoRes res;
    string reqText
    (
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
      "<query>"
      "  <pseudogroup_svc_info>"
      "    <entity passenger-id=\"33085384\" segment-id=\"1\"/> "
      "    <entity passenger-id=\"33085385\" segment-id=\"2\"/> "
      "    <entity passenger-id=\"33085386\" segment-id=\"3\"/> "
      "    <entity passenger-id=\"33085387\" segment-id=\"4\"/> "
      "    <entity passenger-id=\"33085388\" segment-id=\"5\"/> "
      "    <entity passenger-id=\"33085389\" segment-id=\"6\"/> "
      "    <entity passenger-id=\"33085390\" segment-id=\"7\"/> "
      "    <entity passenger-id=\"33085391\" segment-id=\"8\"/> "
      "    <entity passenger-id=\"33085392\" segment-id=\"9\"/> "
      "    <entity passenger-id=\"33085393\" segment-id=\"10\"/> "
      "    <entity passenger-id=\"33085871\" segment-id=\"11\"/> "
      "    <entity passenger-id=\"33176905\" segment-id=\"12\"/> "
      "    <entity passenger-id=\"33176906\" segment-id=\"13\"/> "
      "    <entity passenger-id=\"33176956\" segment-id=\"14\"/> "
      "    <entity passenger-id=\"33167631\" segment-id=\"15\"/> "
      "    <entity passenger-id=\"33167642\" segment-id=\"16\"/> "
      "  </pseudogroup_svc_info>"
      "</query>");
    reqText = ConvertCodepage( reqText, "CP866", "UTF-8" );
//    SirenaExchange::SendTestRequest(reqText);
    req.parse(reqText);
    SvcSirenaInterface::procPseudoGroupInfo(req, res);
    string resText;
    res.build(resText);
    printf("%s\n", resText.c_str());
  }
  catch(std::exception &e)
  {
    printf("%s\n", e.what());
  }

  return 0;
}

namespace SirenaExchange
{

void TAvailabilityReq::bagTypesToDB(const TCkinGrpIds &tckin_grp_ids, bool copy_all_segs) const
{
  if (tckin_grp_ids.empty()) return;

  TTripInfo operFlt;
  if (!operFlt.getByGrpId(tckin_grp_ids.front())) return;

  string airline=WeightConcept::GetCurrSegBagAirline(tckin_grp_ids.front()); //bagTypesToDB - checked!
  if (airline.empty()) return;

  TPaxServiceLists serviceLists;
  for(list<TPaxItem>::const_iterator p=paxs.begin(); p!=paxs.end(); ++p)
  {
    TPaxServiceLists paxServiceLists;
    paxServiceLists.fromDB(p->id, false);

    TPaxServiceListsItem serviceItem;
    serviceItem.pax_id=p->id;
    for(TPaxSegMap::const_iterator s=p->segs.begin(); s!=p->segs.end(); ++s)
    {
      serviceItem.trfer_num=s->second.id;
      for(int pass=0; pass<2; pass++)
      {
        serviceItem.category=(pass==0?TServiceCategory::BaggageInHold:
                                      TServiceCategory::BaggageInCabinOrCarryOn);
        if (paxServiceLists.find(serviceItem)==paxServiceLists.end())
        {
          if (serviceItem.list_id==ASTRA::NoExists)
          {
            TBagTypeList list;
            list.create(airline, operFlt.airp, false);
            if (list.empty()) continue;
            serviceItem.list_id=list.toDBAdv();
          };
          serviceLists.insert(serviceItem);
        };
      };
    }
  }
  for(TCkinGrpIds::const_iterator i=tckin_grp_ids.begin(); i!=tckin_grp_ids.end(); ++i)
  {
    if (i==tckin_grp_ids.begin())
      serviceLists.toDB(false);
    else if (copy_all_segs)
      CopyPaxServiceLists(*tckin_grp_ids.begin(), *i, false, false);
  }
}

bool needSync(xmlNodePtr answerResNode)
{
    return (answerResNode == NULL ||
            (answerResNode != NULL && !findNodeR(answerResNode, "answer")));
}

xmlNodePtr findAnswerNode(xmlNodePtr answerResNode)
{
    return findNodeR(answerResNode, "answer");
}

} //namespace SirenaExchange

void unaccBagTypesToDB(int grp_id, bool ignore_unaccomp_sets) //!!! потом удалить ignore_unaccomp_sets
{
  TTripInfo operFlt;
  if (!operFlt.getByGrpId(grp_id)) return;

  string airline=WeightConcept::GetCurrSegBagAirline(grp_id); //unaccBagTypesToDB - checked!
  if (airline.empty()) return;

  TTrferRoute trfer;
  trfer.GetRoute(grp_id, trtNotFirstSeg);

  TPaxServiceLists serviceLists;
  TPaxServiceListsItem serviceItem;
  serviceItem.pax_id=grp_id;
  for(int pass=0; pass<2; pass++)
  {
    serviceItem.trfer_num=0;
    serviceItem.category=(pass==0?TServiceCategory::BaggageInHold:
                                  TServiceCategory::BaggageInCabinOrCarryOn);
    if (serviceItem.list_id==ASTRA::NoExists)
    {
      TBagTypeList list;
      if (ignore_unaccomp_sets)
        list.create(airline);
      else
        list.create(airline, operFlt.airp, true);
      if (list.empty()) continue;
      serviceItem.list_id=list.toDBAdv();
    };
    serviceLists.insert(serviceItem);
    for(TTrferRoute::const_iterator t=trfer.begin(); t!=trfer.end(); ++t)
    {
      serviceItem.trfer_num++;
      serviceLists.insert(serviceItem);
    };
  };
  serviceLists.toDB(true);
}

void CopyPaxServiceLists(int grp_id_src, int grp_id_dest, bool is_grp_id, bool rfisc_used)
{
  if (is_grp_id)
    throw Exception("%s: not supported! (grp_id_src=%d, grp_id_dest=%d, is_grp_id=%d rfisc_used=%d)",
                    grp_id_src, grp_id_dest, (int)is_grp_id, (int)rfisc_used);

  int list_id=ASTRA::NoExists;
  if (!rfisc_used)
  {
    TTripInfo operFlt;
    if (!operFlt.getByGrpId(grp_id_dest)) return;

    string airline=WeightConcept::GetCurrSegBagAirline(grp_id_dest); //CopyPaxServiceLists - checked!
    if (airline.empty()) return;

    TBagTypeList list;
    list.create(airline, operFlt.airp, false);
    if (list.empty()) return;
    list_id=list.toDBAdv();
  };

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=is_grp_id?"DELETE FROM "
                        "  (SELECT * FROM grp_service_lists, service_lists "
                        "   WHERE grp_service_lists.list_id=service_lists.id AND "
                        "         grp_service_lists.grp_id=:grp_id AND service_lists.rfisc_used=:rfisc_used)":
                        "DELETE FROM "
                        "  (SELECT * FROM pax_service_lists, service_lists, pax "
                        "   WHERE pax_service_lists.list_id=service_lists.id AND "
                        "         pax_service_lists.pax_id=pax.pax_id AND "
                        "         pax.grp_id=:grp_id AND service_lists.rfisc_used=:rfisc_used)";
  Qry.CreateVariable("grp_id", otInteger, grp_id_dest);
  Qry.CreateVariable("rfisc_used", otInteger, (int)rfisc_used);
  Qry.Execute();

  ostringstream sql;
  sql << "INSERT INTO pax_service_lists(pax_id, transfer_num, category, list_id) "
         "SELECT dest.pax_id, "
         "       pax_service_lists.transfer_num+src.seg_no-dest.seg_no, "
         "       pax_service_lists.category, ";
  if (rfisc_used)
    sql << "       pax_service_lists.list_id ";
  else
    sql << "       :list_id ";

  sql << "FROM pax_service_lists, service_lists, "
         "     (SELECT pax.pax_id, "
         "             tckin_pax_grp.tckin_id, "
         "             tckin_pax_grp.seg_no, "
         "             tckin_pax_grp.first_reg_no-pax.reg_no AS distance "
         "      FROM pax, tckin_pax_grp "
         "      WHERE pax.grp_id=tckin_pax_grp.grp_id AND pax.grp_id=:grp_id_src) src, "
         "     (SELECT pax.pax_id, "
         "             tckin_pax_grp.tckin_id, "
         "             tckin_pax_grp.seg_no, "
         "             tckin_pax_grp.first_reg_no-pax.reg_no AS distance "
         "      FROM pax, tckin_pax_grp "
         "      WHERE pax.grp_id=tckin_pax_grp.grp_id AND pax.grp_id=:grp_id_dest) dest "
         "WHERE pax_service_lists.list_id=service_lists.id AND "
         "      service_lists.rfisc_used=:rfisc_used AND "
         "      src.tckin_id=dest.tckin_id AND "
         "      src.distance=dest.distance AND "
         "      pax_service_lists.pax_id=src.pax_id AND "
         "      pax_service_lists.transfer_num+src.seg_no-dest.seg_no>=0 ";
  Qry.Clear();
  Qry.SQLText=sql.str();
  if (!rfisc_used)
    Qry.CreateVariable("list_id", otInteger, list_id);
  Qry.CreateVariable("grp_id_src", otInteger, grp_id_src);
  Qry.CreateVariable("grp_id_dest", otInteger, grp_id_dest);
  Qry.CreateVariable("rfisc_used", otInteger, (int)rfisc_used);
  Qry.Execute();
}

void ServicePaymentInterface::LoadServiceLists(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="SELECT rfisc_used FROM service_lists WHERE id=:id";
  Qry.DeclareVariable("id", otInteger);
  xmlNodePtr svcNode=NewTextChild(resNode, "service_lists");
  for(xmlNodePtr node=NodeAsNode("service_lists", reqNode)->children; node!=NULL; node=node->next)
  {
    if (string((const char*)node->name)!="list_id") continue;
    ServiceListId list_id;
    list_id.fromXML(node);

    Qry.SetVariable("id", list_id.primary());
    Qry.Execute();
    if (Qry.Eof) continue;
    if (Qry.FieldAsInteger("rfisc_used")!=0)
    {
      TRFISCListWithProps list;
      list.fromDB(list_id, true);
      list.setPriority();
      list.toXML(list_id.forTerminal(), NewTextChild(svcNode, "service_list"));
    }
    else
    {
      TBagTypeList list;
      list.fromDB(list_id.primary(), true);
      list.toXML(list_id.forTerminal(), NewTextChild(svcNode, "service_list"));
    };
  }
}

#include "edi_utils.h"
#include "astra_context.h"

bool SvcSirenaInterface::equal(const SvcSirenaResponseHandler& handler1,
                               const SvcSirenaResponseHandler& handler2)
{
  return (func_equal::getAddress(handler1)==func_equal::getAddress(handler2));
}

bool SvcSirenaInterface::addResponseHandler(const SvcSirenaResponseHandler& res)
{
  if (!res) return false;
  for(const SvcSirenaResponseHandler& handler : resHandlers)
    if (equal(handler, res)) return false;

  resHandlers.push_back(res);
  return true;
}

void SvcSirenaInterface::handleResponse(xmlNodePtr reqNode, xmlNodePtr externalSysResNode, xmlNodePtr resNode) const
{
  for(const SvcSirenaResponseHandler& handler : resHandlers)
    if (handler) handler(reqNode, externalSysResNode, resNode);
}

void SvcSirenaInterface::DoRequest(xmlNodePtr reqNode,
                                   xmlNodePtr externalSysResNode,
                                   const SirenaExchange::TExchange& req)
{
    using namespace AstraEdifact;

    LogTrace(TRACE3) << __FUNCTION__ << ": " << req.exchangeId();

    int reqCtxtId = AstraContext::SetContext("TERM_REQUEST", XMLTreeToText(reqNode->doc));
    if (externalSysResNode!=nullptr)
      addToEdiResponseCtxt(reqCtxtId, externalSysResNode->children, "");

    SirenaExchange::SirenaClient sirClient;
    std::string reqText;
    req.build(reqText);
    sirClient.sendRequest(reqText, createKickInfo(reqCtxtId, SvcSirenaInterface::name()));

//    SvcSirenaInterface* iface=dynamic_cast<SvcSirenaInterface*>(JxtInterfaceMng::Instance()->GetInterface(SvcSirenaInterface::name()));
//    if (iface!=nullptr && iface->addResponseHandler(res))
//      LogTrace(TRACE5) << "added response handler for <" << req.exchangeId() << ">";
}

void SvcSirenaInterface::KickHandler(XMLRequestCtxt *ctxt,
                                     xmlNodePtr reqNode,
                                     xmlNodePtr resNode)
{
    using namespace AstraEdifact;

    const std::string DefaultAnswer = "<answer/>";
    std::string pult = TReqInfo::Instance()->desk.code;
    LogTrace(TRACE3) << __FUNCTION__ << " for pult [" << pult << "]";

    boost::optional<httpsrv::HttpResp> resp = SirenaExchange::SirenaClient::receive(pult);
    if(resp) {
        //LogTrace(TRACE3) << "req:\n" << resp->req.text;
        if(resp->commErr) {
             LogError(STDLOG) << "Http communication error! "
                              << "(" << resp->commErr->code << "/" << resp->commErr->errMsg << ")";
        }
    } else {
        LogError(STDLOG) << "Enter to KickHandler but HttpResponse is empty!";
    }

    if(GetNode("@req_ctxt_id",reqNode) != NULL)
    {
        int req_ctxt_id = NodeAsInteger("@req_ctxt_id", reqNode);

        XMLDoc termReqCtxt;
        getTermRequestCtxt(req_ctxt_id, true, "SvcSirenaInterface::KickHandler", termReqCtxt);
        xmlNodePtr termReqNode = NodeAsNode("/term/query", termReqCtxt.docPtr())->children;
        if(termReqNode == NULL)
          throw EXCEPTIONS::Exception("SvcSirenaInterface::KickHandler: context TERM_REQUEST termReqNode=NULL");;

        transformKickRequest(termReqNode, reqNode);

        std::string answerStr = DefaultAnswer;
        if(resp) {
            const auto fnd = resp->text.find("<answer>");
            if(fnd != std::string::npos) {
                answerStr = resp->text.substr(fnd);
            }
        }

        XMLDoc answerResDoc;
        try
        {
          answerResDoc = ASTRA::createXmlDoc2(answerStr);
          LogTrace(TRACE5) << "HTTP Response for [" << pult << "], text:\n" << XMLTreeToText(answerResDoc.docPtr());
        }
        catch(std::exception &e)
        {
          LogError(STDLOG) << "ASTRA::createXmlDoc2(answerStr) error";
          answerResDoc = ASTRA::createXmlDoc2(DefaultAnswer);
        }
        xmlNodePtr answerResNode = NodeAsNode("/answer", answerResDoc.docPtr());
        addToEdiResponseCtxt(req_ctxt_id, answerResNode, "");

        XMLDoc answerResCtxt;
        getEdiResponseCtxt(req_ctxt_id, true, "SvcSirenaInterface::KickHandler", answerResCtxt);
        answerResNode = NodeAsNode("/context", answerResCtxt.docPtr());
        if(answerResNode == NULL)
          throw EXCEPTIONS::Exception("SvcSirenaInterface::KickHandler: context EDI_RESPONSE answerResNode=NULL");;
        //LogTrace(TRACE3) << "answer res (old ediRes):\n" << XMLTreeToText(answerResCtxt.docPtr());

        handleResponse(termReqNode, answerResNode, resNode);
    }
}

