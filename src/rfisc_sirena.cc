#include "rfisc_sirena.h"
#include "baggage_calc.h"
#include "payment_base.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

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

const TSegItem& TSegItem::toSirenaXML(xmlNodePtr node, const std::string &lang) const
{
  if (node==NULL) return *this;

  SetProp(node, "id", id);
  if (markFlt)
  {
    SetProp(node, "company", airlineToXML(markFlt.get().airline, lang));
    SetProp(node, "flight", flight(markFlt.get(), lang));
  };
  SetProp(node, "operating_company", airlineToXML(operFlt.airline, lang));
  SetProp(node, "operating_flight", flight(operFlt, lang));
  SetProp(node, "departure", airpToXML(operFlt.airp, lang));
  SetProp(node, "arrival", airpToXML(airp_arv, lang));
  if (operFlt.scd_out!=ASTRA::NoExists)
  {
    if (scd_out_contain_time)
      SetProp(node, "departure_time", DateTimeToStr(operFlt.scd_out, "yyyy-mm-ddThh:nn:ss")); //�����쭮� �६�
    else
      SetProp(node, "departure_date", DateTimeToStr(operFlt.scd_out, "yyyy-mm-dd")); //�����쭠� ���

  }
  if (scd_in!=ASTRA::NoExists)
  {
    if (scd_in_contain_time)
      SetProp(node, "arrival_time", DateTimeToStr(scd_in, "yyyy-mm-ddThh:nn:ss")); //�����쭮� �६�
    else
      SetProp(node, "arrival_date", DateTimeToStr(scd_in, "yyyy-mm-dd")); //�����쭠� ���

  }
  SetProp(node, "equipment", craftToXML(operFlt.craft, lang), "");

  return *this;
}

const TPaxSegItem& TPaxSegItem::toSirenaXML(xmlNodePtr node, const std::string &lang) const
{
  if (node==NULL) return *this;

  TSegItem::toSirenaXML(node, lang);
  SetProp(node, "subclass", ElemIdToPrefferedElem(etSubcls, subcl, efmtCodeNative, lang));
  if (!tkn.no.empty())
  {
    xmlNodePtr tknNode=NewTextChild(node, "ticket");
    SetProp(tknNode, "number", tkn.no);
    SetProp(tknNode, "coupon_num", tkn.coupon, ASTRA::NoExists);
  }
  for(list<CheckIn::TPnrAddrItem>::const_iterator i=pnrs.begin(); i!=pnrs.end(); ++i)
    SetProp(NewTextChild(node, "recloc", i->addr), "crs", airlineToXML(i->airline, lang));
  for(std::set<CheckIn::TPaxFQTItem>::const_iterator i=fqts.begin(); i!=fqts.end(); ++i)
    SetProp(NewTextChild(node, "ffp", i->no), "company", airlineToXML(i->airline, lang));

  return *this;
}

std::string TSegItem::flight(const TTripInfo &flt, const std::string &lang)
{
  ostringstream s;
  s << setw(3) << setfill('0') << flt.flt_no
    << ElemIdToPrefferedElem(etSuffix, flt.suffix, efmtCodeNative, lang);
  return s.str();
}

const TPaxItem& TPaxItem::toSirenaXML(xmlNodePtr node, const std::string &lang) const
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
    SetProp(docNode, "country", ElemIdToCodeNative(etPaxDocCountry, doc.issue_country), "");
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

const TPaxItem2& TPaxItem2::toSirenaXML(xmlNodePtr node, const std::string &lang) const
{
  if (node==NULL) return *this;

  NewTextChild( node, "surname", surname );
  NewTextChild( node, "name", name );
  NewTextChild( node, "category", category());
  NewTextChild( node, "group_id", grp_id );
  NewTextChild( node, "reg_no", reg_no );
  return *this;
}

const TSvcItem& TSvcItem::toSirenaXML(xmlNodePtr node, const std::string &lang) const
{
  if (node==NULL) return *this;

  TPaxSegRFISCKey::toSirenaXML(node, lang);
  SetProp(node, "paid", status==TServiceStatus::Paid?"true":"false", "false");
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

void TAvailabilityReq::toXML(xmlNodePtr node) const
{
  if (node==NULL) return;

  if (paxs.empty()) throw Exception("TAvailabilityReq::toXML: paxs.empty()");

  SetProp(node, "show_brand_info", "true");
  SetProp(node, "show_all_svc", "true");
  SetProp(node, "show_free_carry_on_norm", "true");

  for(list<TPaxItem>::const_iterator p=paxs.begin(); p!=paxs.end(); ++p)
    p->toSirenaXML(NewTextChild(node, "passenger"), LANG_EN);
}

void TAvailabilityResItem::remove_unnecessary()
{
  for(TRFISCList::iterator i=rfisc_list.begin(); i!=rfisc_list.end();)
  {
    const TRFISCListItem &item=i->second;
    if (item.carry_on())
    {
      //����� ��� �/�����
      if ((!item.carry_on().get() && baggage_norm.airline!=item.airline) ||
          (item.carry_on().get() && carry_on_norm.airline!=item.airline))
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
      if (string((char*)node->name)!="svc_list") continue;
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

bool TAvailabilityRes::identical_rfisc_list(int seg_id, boost::optional<TRFISCList> &rfisc_list) const //!!!��⮬ ����
{
  rfisc_list=boost::none;
  for(TAvailabilityResMap::const_iterator i=begin(); i!=end(); ++i)
  {
    if (i->first.trfer_num!=seg_id) continue;
    if (!rfisc_list)
      rfisc_list=i->second.rfisc_list;
    else
    {
      if (!rfisc_list.get().equal(i->second.rfisc_list,true))  //old_version - �ࠢ������ ⮫쪮 ������� RFISC
      {
        rfisc_list=boost::none;
        return false;
      }
    };
  };
  return true;
}

bool TAvailabilityRes::exists_rfisc(int seg_id, TServiceType::Enum service_type) const
{
  for(TAvailabilityResMap::const_iterator i=begin(); i!=end(); ++i)
  {
    if (i->first.trfer_num!=seg_id) continue;
    if (i->second.rfisc_list.exists(service_type)) return true;
  };
  return false;
};

void TAvailabilityRes::rfiscsToDB(const TCkinGrpIds &tckin_grp_ids, bool old_version) const
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
        case 1: cat=TServiceCategory::Baggage; break;
        case 2: cat=TServiceCategory::CarryOn; break;
        default: cat=TServiceCategory::Other; break;
      };

      if (old_version && cat==TServiceCategory::CarryOn) continue;//!!! ��⮬ 㤠����

      if (cat==TServiceCategory::Baggage ||
          cat==TServiceCategory::CarryOn)
      {
        //�஢��塞 ����
        const Sirena::TSimplePaxNormItem& norm=(cat==TServiceCategory::Baggage)?i->second.baggage_norm:
                                                                                i->second.carry_on_norm;
        if (norm.concept==TBagConcept::Weight ||
            norm.concept==TBagConcept::Unknown) continue;
        if (norm.concept==TBagConcept::No)
        {
           boost::optional<TBagConcept::Enum> seg_concept;
           identical_concept(i->first.trfer_num, cat==TServiceCategory::CarryOn, seg_concept);
           if ((!seg_concept || seg_concept.get()==TBagConcept::No) &&
               !i->second.rfisc_list.exists(TServiceType::BaggageCharge)) continue; //�� ��� - weight
           if (seg_concept && seg_concept.get()==TBagConcept::Weight) continue;
        };
      }

      TPaxServiceListsItem serviceItem;
      serviceItem.pax_id=i->first.pax_id;
      serviceItem.trfer_num=i->first.trfer_num;
      serviceItem.list_id=i->second.rfisc_list.toDBAdv(false);
      serviceItem.category=cat;
      serviceLists.insert(serviceItem);
      if (old_version && cat==TServiceCategory::Baggage)
      {
        //!!! ��⮬ 㤠����
        //ᯥ樠�쭮 ᤥ���� �⮡� �� ࠧ��� ���楯�� ������ � ��筮� ����� �ਬ����� �������
        serviceItem.category=TServiceCategory::CarryOn;
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

void TAvailabilityRes::normsToDB(const TCkinGrpIds &tckin_grp_ids) const
{
  list<Sirena::TPaxNormItem> normsList;
  list<Sirena::TPaxNormItem> oldNormsList;
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

      if (oldNormsList.empty())
        oldNormsList.push_back(item);
      else
      {
        Sirena::TPaxNormItem &last=oldNormsList.back();
        if (last.pax_id==item.pax_id &&
            last.trfer_num==item.trfer_num)
          last.add(item);
        else
          oldNormsList.push_back(item);
      }

      if (carry_on) break;
    };
  }
  Sirena::PaxNormsToDB(tckin_grp_ids, normsList, false);
  Sirena::PaxNormsToDB(tckin_grp_ids, oldNormsList, true);
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

void TSvcList::get(TPaidRFISCList &paid) const
{
  paid.clear();
  for(TSvcList::const_iterator i=begin(); i!=end(); ++i)
    paid.inc(*i, i->status);
}

void TSvcList::set(int grp_id, int tckin_seg_count, int trfer_seg_count)
{
  clear();
  TGrpServiceList list;
  list.prepareForSirena(grp_id, tckin_seg_count, trfer_seg_count);
  CheckIn::TServicePaymentList payment;
  payment.fromDB(grp_id);
  for(TGrpServiceList::const_iterator i=list.begin(); i!=list.end(); ++i)
  {
    for(int j=i->service_quantity; j>0; j--)
      push_back(TSvcItem(*i, payment.dec(*i)?TServiceStatus::Paid:TServiceStatus::Need));
  };
}

void TPaymentStatusReq::toXML(xmlNodePtr node) const
{
  if (node==NULL) return;

  if (paxs.empty()) throw Exception("TPaymentStatusReq::toXML: paxs.empty()");
  if (svcs.empty()) throw Exception("TPaymentStatusReq::toXML: svcs.empty()");


  SetProp(node, "show_free_carry_on_norm", "true");

  for(list<TPaxItem>::const_iterator p=paxs.begin(); p!=paxs.end(); ++p)
    p->toSirenaXML(NewTextChild(node, "passenger"), LANG_EN);
  for(TSvcList::const_iterator i=svcs.begin(); i!=svcs.end(); ++i)
    i->toSirenaXML(NewTextChild(node, "svc"), LANG_EN);
}

void TPaymentStatusRes::fromXML(xmlNodePtr node)
{
  clear();

  try
  {
    if (node==NULL) throw Exception("node not defined");
    for(node=node->children; node!=NULL; node=node->next)
    {
      string nodeName=(char*)node->name;
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
    //if (norms.empty()) throw Exception("norms.empty()"); � ���� ��?
  }
  catch(Exception &e)
  {
    throw Exception("TPaymentStatusRes::fromXML: %s", e.what());
  };
}

void TPaymentStatusRes::normsToDB(const TCkinGrpIds &tckin_grp_ids) const
{
  list<Sirena::TPaxNormItem> normsList;
  list<Sirena::TPaxNormItem> oldNormsList;
  for(Sirena::TPaxNormList::const_iterator i=norms.begin(); i!=norms.end(); ++i)
  {
    Sirena::TPaxNormItem item;
    static_cast<Sirena::TSimplePaxNormItem&>(item)=i->second;
    item.pax_id=i->first.pax_id;
    item.trfer_num=i->first.trfer_num;
    normsList.push_back(item);

    if (oldNormsList.empty())
      oldNormsList.push_back(item);
    else
    {
      Sirena::TPaxNormItem &last=oldNormsList.back();
      if (last.pax_id==item.pax_id &&
          last.trfer_num==item.trfer_num)
        last.add(item);
      else
        oldNormsList.push_back(item);
    }
  }
  Sirena::PaxNormsToDB(tckin_grp_ids, normsList, false);
  Sirena::PaxNormsToDB(tckin_grp_ids, oldNormsList, true);
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
    p->toSirenaXML(NewTextChild(node, "passenger"), LANG_EN);
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

  if (paxs.empty()) throw Exception("TGroupInfoRes::toXML: paxs.empty()");

  for(list<TPaxItem>::const_iterator p=paxs.begin(); p!=paxs.end(); ++p)
    p->toSirenaXML(NewTextChild(node, "passenger"), LANG_EN);

  if (svcs.empty()) throw Exception("TGroupInfoRes::toXML: svcs.empty()");

  for(TSvcList::const_iterator i=svcs.begin(); i!=svcs.end(); ++i)
    i->toSirenaXML(NewTextChild(node, "svc"), LANG_EN);
}

void TPseudoGroupInfoReq::fromXML(xmlNodePtr node)
{
  clear();

  try
  {
    if (node==NULL) throw Exception("node not defined");
    for(node=node->children; node!=NULL; node=node->next)
    {
      string nodeName=(char*)node->name;
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

  for(list<TPaxItem>::const_iterator p=paxs.begin(); p!=paxs.end(); ++p)
    p->toSirenaXML(NewTextChild(node, "passenger"), LANG_EN);

  for(TSvcList::const_iterator i=svcs.begin(); i!=svcs.end(); ++i)
    i->toSirenaXML(NewTextChild(node, "svc"), LANG_EN);
}

} //namespace SirenaExchange

void PieceConceptInterface::procPieceConcept(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  ProgTrace( TRACE5, "%s: %s", __FUNCTION__, XMLTreeToText(resNode->doc).c_str());
  reqNode=NodeAsNode("content", reqNode);

  reqNode=NodeAsNode("query", reqNode)->children;
  std::string exchangeId = (char *)reqNode->name;
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

void PieceConceptInterface::procPassengers( const SirenaExchange::TPassengersReq &req, SirenaExchange::TPassengersRes &res )
{
  res.clear();

  TSearchFltInfo fltInfo;
  fltInfo.airline = req.airline;
  fltInfo.airp_dep = req.airp;
  fltInfo.flt_no = req.flt_no;
  fltInfo.suffix = req.suffix;
  fltInfo.scd_out = req.scd_out;
  fltInfo.scd_out_in_utc = false;
  //䠪��᪨�
  list<TAdvTripInfo> flts;
  SearchFlt( fltInfo, flts);
  set<int> operating_point_ids;
  for ( list<TAdvTripInfo>::const_iterator iflt=flts.begin(); iflt!=flts.end(); iflt++ )
    operating_point_ids.insert(iflt->point_id);
  //�������᪨�
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
        res.push_back( resPax );
      }
    }
  };
}

void PieceConceptInterface::procGroupInfo( const SirenaExchange::TGroupInfoReq &req,
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

void PieceConceptInterface::procPseudoGroupInfo( const SirenaExchange::TPseudoGroupInfoReq &req,
                                                 SirenaExchange::TPseudoGroupInfoRes &res )
{
  SirenaExchange::fillPaxsSvcs(req.entities, res);
}

int verifyHTTP(int argc,char **argv)
{
  try
  {
    SirenaExchange::TPassengersReq req;
    SirenaExchange::TPassengersRes res;
    string reqText
    (
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
      "<query>"
      "  <passenger_with_svc>"
      "    <company>UT</company>"
      "    <flight>454</flight>"
      "    <departure_date>2015-10-20</departure_date>"
      "    <departure>���</departure>"
      "  </passenger_with_svc>"
      "</query>");
    reqText = ConvertCodepage( reqText, "CP866", "UTF-8" );
    SirenaExchange::SendTestRequest(reqText);
    req.parse(reqText);
    PieceConceptInterface::procPassengers(req, res);
    string resText;
    res.build(resText);
    printf("%s\n", resText.c_str());
  }
  catch(std::exception &e)
  {
    printf("%s\n", e.what());
  }

  try
  {
    SirenaExchange::TGroupInfoReq req;
    SirenaExchange::TGroupInfoRes res;
    string reqText
    (
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
      "<query>"
      "  <group_svc_info>"
      "    <regnum>SDA12F</regnum>"
      "    <group_id>34877</group_id>"
      "  </group_svc_info>"
      "</query>");
    reqText = ConvertCodepage( reqText, "CP866", "UTF-8" );
    SirenaExchange::SendTestRequest(reqText);
    req.parse(reqText);
    PieceConceptInterface::procGroupInfo(req, res);
    string resText;
    res.build(resText);
    printf("%s\n", resText.c_str());
  }
  catch(std::exception &e)
  {
    printf("%s\n", e.what());
  }

  try
  {
    SirenaExchange::TPseudoGroupInfoReq req;
    SirenaExchange::TPseudoGroupInfoRes res;
    string reqText
    (
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
      "<query>"
      "  <pseudogroup_svc_info>"
      "    <entity passenger-id=\"33085384\" segment-id=\"0\"/> "
      "    <entity passenger-id=\"33085385\" segment-id=\"0\"/> "
      "    <entity passenger-id=\"33085386\" segment-id=\"0\"/> "
      "    <entity passenger-id=\"33085387\" segment-id=\"0\"/> "
      "    <entity passenger-id=\"33085388\" segment-id=\"0\"/> "
      "    <entity passenger-id=\"33085389\" segment-id=\"0\"/> "
      "    <entity passenger-id=\"33085390\" segment-id=\"0\"/> "
      "    <entity passenger-id=\"33085391\" segment-id=\"0\"/> "
      "    <entity passenger-id=\"33085392\" segment-id=\"0\"/> "
      "    <entity passenger-id=\"33085393\" segment-id=\"0\"/> "
      "    <entity passenger-id=\"33085871\" segment-id=\"0\"/> "
      "  </pseudogroup_svc_info>"
      "</query>");
    reqText = ConvertCodepage( reqText, "CP866", "UTF-8" );
    SirenaExchange::SendTestRequest(reqText);
    req.parse(reqText);
    PieceConceptInterface::procPseudoGroupInfo(req, res);
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
    paxServiceLists.fromDB(p->id, false, false);

    TPaxServiceListsItem serviceItem;
    serviceItem.pax_id=p->id;
    for(TPaxSegMap::const_iterator s=p->segs.begin(); s!=p->segs.end(); ++s)
    {
      serviceItem.trfer_num=s->second.id;
      for(int pass=0; pass<2; pass++)
      {
        serviceItem.category=(pass==0?TServiceCategory::Baggage:TServiceCategory::CarryOn);
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

} //namespace SirenaExchange

void unaccBagTypesToDB(int grp_id, bool ignore_unaccomp_sets) //!!! ��⮬ 㤠���� ignore_unaccomp_sets
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
    serviceItem.category=(pass==0?TServiceCategory::Baggage:TServiceCategory::CarryOn);
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

void UpgradeDBForServices(int grp_id)
{
  using namespace SirenaExchange;

  ProgTrace(TRACE5, "UpgradeDBForServices started (grp_id=%d)", grp_id);

  string progress="fillPaxsBags";
  try
  {
    CheckIn::TPaxGrpCategory::Enum grp_cat;
    TCkinGrpIds tckin_grp_ids;
    SirenaExchange::TAvailabilityReq req;
    SirenaExchange::fillPaxsBags(grp_id, req, grp_cat, tckin_grp_ids, true);

    progress="serviceLists.toDB";

    if (grp_cat!=CheckIn::TPaxGrpCategory::UnnacompBag)
    {
      bool pc=false, wt=false;
      int bag_types_id=ASTRA::NoExists;
      GetBagConceptsCompatible(grp_id, pc, wt, bag_types_id);
      if (pc && bag_types_id!=ASTRA::NoExists)
      {
        TRFISCList list;
        list.fromDB(bag_types_id, true, false);
        int list_id=list.toDBAdv(false);

        TTrferRoute trfer;
        trfer.GetRoute(grp_id, trtNotFirstSeg);
        int max_pc_seg_id=0;
        for(TTrferRoute::const_iterator t=trfer.begin(); t!=trfer.end(); ++t, max_pc_seg_id++)
          if (!t->piece_concept || !t->piece_concept.get()) break;

        TPaxServiceLists serviceLists;
        for(std::list<TPaxItem>::const_iterator p=req.paxs.begin(); p!=req.paxs.end(); ++p)
        {
          TPaxServiceListsItem serviceItem;
          serviceItem.pax_id=p->id;
          for(TPaxSegMap::const_iterator s=p->segs.begin(); s!=p->segs.end(); ++s)
          {
            if (s->second.id>max_pc_seg_id) continue;
            serviceItem.trfer_num=s->second.id;
            for(int pass=0; pass<2; pass++)
            {
              serviceItem.category=(pass==0?TServiceCategory::Baggage:TServiceCategory::CarryOn);
              serviceItem.list_id=list_id;
              serviceLists.insert(serviceItem);
            };
          }
        }
        serviceLists.toDB(false);
      }
      else if (wt)
      {
        req.bagTypesToDB(tckin_grp_ids, false);  //������塞 ��ᮢ묨 ⨯��� ������
      };
    }
    else unaccBagTypesToDB(grp_id, true);

    progress="TGroupBagItem";

    CheckIn::TGroupBagItem group_bag;
    group_bag.bagFromDB(grp_id);
    for(CheckIn::TBagMap::iterator i=group_bag.bags.begin(); i!=group_bag.bags.end(); ++i)
    {
      CheckIn::TBagItem &item=i->second;
      if (item.pc)
      {
        if (!item.is_trfer && item.pc.get().list_id!=-grp_id)
          throw Exception("%s: list_id!=-grp_id! (grp_id=%d, %s)", __FUNCTION__,
                          grp_id, item.pc.get().traceStr().c_str());
        item.pc.get().list_id=ASTRA::NoExists;
        item.pc.get().list_item=boost::none;
      }
      if (item.wt)
      {
        if (item.wt.get().list_id!=-grp_id)
          throw Exception("%s: list_id!=-grp_id! (grp_id=%d, %s)", __FUNCTION__,
                          grp_id, item.wt.get().traceStr().c_str());
        item.wt.get().list_id=ASTRA::NoExists;
        item.wt.get().list_item=boost::none;
      }
    }
    group_bag.getAllListItems(grp_id, grp_cat==CheckIn::TPaxGrpCategory::UnnacompBag, 0);
    group_bag.bagToDB(grp_id);

    progress="TPaidRFISCList";

    TPaidRFISCList paid_rfisc_old, paid_rfisc_new;
    paid_rfisc_old.fromDB(grp_id, true);
    for(TPaidRFISCList::const_iterator i=paid_rfisc_old.begin(); i!=paid_rfisc_old.end(); ++i)
    {
      TPaidRFISCItem item=i->second;
      if (item.list_id!=-grp_id)
        throw Exception("%s: list_id!=-grp_id! (grp_id=%d, %s)", __FUNCTION__,
                        grp_id, item.traceStr().c_str());
      item.list_id=ASTRA::NoExists;
      item.list_item=boost::none;
      paid_rfisc_new.insert(make_pair(item, item));
    }
    paid_rfisc_new.getAllListItems();
    paid_rfisc_new.toDB(grp_id);

    progress="PaidBagToDB";

    WeightConcept::TPaidBagList paid_bag;
    WeightConcept::PaidBagFromDB(ASTRA::NoExists, grp_id, paid_bag);
    for(WeightConcept::TPaidBagList::iterator i=paid_bag.begin(); i!=paid_bag.end(); ++i)
    {
      WeightConcept::TPaidBagItem &item=*i;
      if (item.list_id!=-grp_id)
        throw Exception("%s: list_id!=-grp_id! (grp_id=%d, %s)", __FUNCTION__,
                        grp_id, item.traceStr().c_str());
      item.list_id=ASTRA::NoExists;
      item.list_item=boost::none;
    };
    WeightConcept::PaidBagToDB(grp_id, grp_cat==CheckIn::TPaxGrpCategory::UnnacompBag, paid_bag);

    progress="TServicePaymentList";

    CheckIn::TServicePaymentList payment;
    payment.fromDB(grp_id);
    for(CheckIn::TServicePaymentList::iterator i=payment.begin(); i!=payment.end(); ++i)
    {
      CheckIn::TServicePaymentItem &item=*i;
      if (item.pc)
      {
        if (item.pc.get().list_id!=-grp_id)
          throw Exception("%s: list_id!=-grp_id! (grp_id=%d, %s)", __FUNCTION__,
                          grp_id, item.pc.get().traceStr().c_str());
        item.pc.get().list_id=ASTRA::NoExists;
        item.pc.get().list_item=boost::none;
      }
      if (item.wt)
      {
        if (item.wt.get().list_id!=-grp_id)
          throw Exception("%s: list_id!=-grp_id! (grp_id=%d, %s)", __FUNCTION__,
                          grp_id, item.wt.get().traceStr().c_str());
        item.wt.get().list_id=ASTRA::NoExists;
        item.wt.get().list_item=boost::none;
      }
    }
    payment.getAllListItems(grp_id, grp_cat==CheckIn::TPaxGrpCategory::UnnacompBag);
    payment.toDB(grp_id);
  }
  catch(Exception &e)
  {
    throw Exception("%s: %s", progress.c_str(), e.what());
  }

  ProgTrace(TRACE5, "UpgradeDBForServices finished (grp_id=%d)", grp_id);
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
    if (string((char*)node->name)!="list_id") continue;
    int list_id=NodeAsInteger(node);
    if (list_id>=0)
    {
      Qry.SetVariable("id", list_id);
      Qry.Execute();
      if (Qry.Eof) continue;
      if (Qry.FieldAsInteger("rfisc_used")!=0)
      {
        TRFISCListWithProps list;
        list.fromDB(list_id, false, true);
        list.getBagProps();
        list.setPriority();
        list.toXML(list_id, NewTextChild(svcNode, "service_list"));
      }
      else
      {
        TBagTypeList list;
        list.fromDB(list_id, true);
        list.toXML(list_id, NewTextChild(svcNode, "service_list"));
      };
    }
    else
    {
      //!!!��⮬ 㤠����
      int grp_id=-list_id;
      bool pc=false, wt=false;
      int bag_types_id=ASTRA::NoExists;
      GetBagConceptsCompatible(grp_id, pc, wt, bag_types_id);
      if (pc && bag_types_id!=ASTRA::NoExists)
      {
        TRFISCListWithProps list;
        list.fromDB(bag_types_id, true, true);
        list.getBagProps();
        list.setPriority();
        list.toXML(list_id, NewTextChild(svcNode, "service_list"));
      }
      else if (wt)
      {
        TBagTypeList list;
        list.createWithCurrSegBagAirline(grp_id); //ServicePaymentInterface::LoadServiceLists - checked!
        list.toXML(list_id, NewTextChild(svcNode, "service_list"));
      }
    }
  }
}
