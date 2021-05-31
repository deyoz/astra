#include "rfisc.h"
#include <boost/crc.hpp>
#include "qrys.h"
#include "term_version.h"
#include "base_callbacks.h"
#include "passenger.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace std;
using namespace AstraLocale;

const TServiceTypes& ServiceTypes()
{
  static TServiceTypes serviceTypes;
  return serviceTypes;
}

const TServiceTypesView& ServiceTypesView()
{
  static TServiceTypesView serviceTypesView;
  return serviceTypesView;
}

const TServiceStatuses& ServiceStatuses()
{
  static TServiceStatuses serviceStatuses;
  return serviceStatuses;
}

std::ostream & operator <<(std::ostream &os, TServiceType::Enum const &value)
{
  os << ServiceTypesView().encode(value);
  return os;
}


bool TRFISCListItem::isCarryOn() const
{
  if (service_type==TServiceType::BaggageCharge ||
      service_type==TServiceType::BaggagePrepaid)
  {
    if (grp=="BG"/*(�����)*/&& subgrp=="CY"/*��筠� �����*/) return true;
  }
  return false;
}

bool TRFISCListItem::isBaggageOrCarryOn() const
{
  return service_type==TServiceType::BaggageCharge ||
         service_type==TServiceType::BaggagePrepaid;
}

TServiceCategory::Enum TRFISCListItem::calc_category() const
{
  if (service_type==TServiceType::BaggageCharge ||
      service_type==TServiceType::BaggagePrepaid)
  {
    if ((grp=="BG"/*(�����)*/&& subgrp=="CY"/*��筠� �����*/) ||
        (grp=="PT"/*(������)*/&& subgrp=="PC"/*(� ������)*/))
      return TServiceCategory::BaggageInCabinOrCarryOn;
    else
      return TServiceCategory::BaggageInHold;
  }
  return TServiceCategory::Other;
}

bool TRFISCListItem::isBaggageInCabinOrCarryOn() const
{
  return calc_category()==TServiceCategory::BaggageInCabinOrCarryOn;
}

bool TRFISCListItem::isBaggageInHold() const
{
  return calc_category()==TServiceCategory::BaggageInHold;
}

const std::string& TRFISCListItem::name_view(const std::string &lang) const
{
  return (lang.empty()?TReqInfo::Instance()->desk.lang:lang)==AstraLocale::LANG_RU?name:name_lat;
}

const std::string TRFISCListItem::descr_view(const std::string &lang) const
{
  return "";
}

const TRFISCListItem& TRFISCListItem::toXML(xmlNodePtr node, const boost::optional<TRFISCListItem> &def) const
{
  if (node==NULL) return *this;
  if (!(def && RFISC==def.get().RFISC))
    NewTextChild(node, "rfisc", RFISC);
  if (!(def && service_type==def.get().service_type))
    NewTextChild(node, "service_type", ServiceTypes().encode(service_type));
  if (!(def && airline==def.get().airline))
    NewTextChild(node, "airline", airline);
  if (!(def && category==def.get().category))
    NewTextChild(node, "category", (int)category);
  if (!(def && name_view()==def.get().name_view()))
    NewTextChild(node, "name_view", lowerc(name_view()));
  if (!(def && descr_view()==def.get().descr_view()))
    NewTextChild(node, "descr_view", descr_view());
  if (!(def && priority==def.get().priority))
    priority?NewTextChild(node, "priority", priority.get()):
             NewTextChild(node, "priority");
  return *this;
}

const TRFISCListKey& TRFISCListKey::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  NewTextChild(node, "rfisc", RFISC);
  NewTextChild(node, "service_type", ServiceTypes().encode(service_type));
  NewTextChild(node, "airline", airline);
  return *this;
}

const TRFISCKey& TRFISCKey::toSirenaXML(xmlNodePtr node, const OutputLang &lang) const
{
  if (node==NULL) return *this;
  list_item?list_item.get().toSirenaXML(node, lang):TRFISCListKey::toSirenaXML(node, lang);
  return *this;
}

const TRFISCKey& TRFISCKey::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  TRFISCListKey::toXML(node);
  NewTextChild(node, "name_view", list_item?lowerc(list_item.get().name_view()):"");
  return *this;
}

std::string TRFISCListKey::str(const std::string& lang) const
{
  std::ostringstream s;
  s << RFISC << "/"
    << ElemIdToPrefferedElem(etAirline, airline, efmtCodeNative,
                             lang.empty()?TReqInfo::Instance()->desk.lang:lang) << "/"
    << ServiceTypes().encode(service_type);
  return s.str();
}

const TRFISCListKey& TRFISCListKey::toSirenaXML(xmlNodePtr node, const OutputLang &lang) const
{
  if (node==NULL) return *this;

  if (!airline.empty())
    SetProp(node, "company", airlineToPrefferedCode(airline, lang));
  if (service_type!=TServiceType::Unknown)
    SetProp(node, "service_type", ServiceTypes().encode(service_type));
  SetProp(node, "rfisc", RFISC);
  return *this;
}

const TRFISCListItem& TRFISCListItem::toSirenaXML(xmlNodePtr node, const OutputLang &lang) const
{
  if (node==NULL) return *this;

  TRFISCListKey::toSirenaXML(node, lang);
  SetProp(node, "rfic", RFIC, "");
  SetProp(node, "emd_type", emd_type, "");
  SetProp(NewTextChild(node, "name", name_lat), "language", "en");
  SetProp(NewTextChild(node, "name", name    ), "language", "ru");
  return *this;
}

TRFISCListItem& TRFISCListItem::fromSirenaXML(xmlNodePtr node)
{
  clear();
  TRFISCListKey::fromSirenaXML(node);

  try
  {
    if (node==NULL) throw Exception("node not defined");

    RFIC=NodeAsString("@rfic", node, "");
    if (RFIC.size()>1) throw Exception("Wrong @rfic='%s'", RFIC.c_str());

    emd_type=NodeAsString("@emd_type", node, "");
    if (emd_type.size()>6) throw Exception("Wrong @emd_type='%s'", emd_type.c_str());

    if (GetNode("@group", node)!=NULL)
    {
      grp=NodeAsString("@group", node);
      if (grp.empty()) throw Exception("Empty @group");
    };

    if (GetNode("@subgroup", node)!=NULL)
    {
      subgrp=NodeAsString("@subgroup", node);
      if (subgrp.empty()) throw Exception("Empty @subgroup");
    };

    xmlNodePtr nameNode=node->children;
    for(; nameNode!=NULL; nameNode=nameNode->next)
    {
      if (string((const char*)nameNode->name)!="name") continue;
      string lang=NodeAsString("@language", nameNode, "");
      if (lang.empty()) throw Exception("Empty @language");
      if (lang=="ru" || lang=="en")
      {
        string &n=(lang=="ru"?name:name_lat);
        if (!n.empty()) throw Exception("Duplicate <name language='%s'>", lang.c_str());
        n=NodeAsString(nameNode);
        if (n.empty()) throw Exception("Empty <name language='%s'>", lang.c_str());
        if (n.size()>100) throw Exception("Wrong <name language='%s'> '%s'", lang.c_str(), n.c_str());
      }
      else
        throw Exception("Wrong @language='%s'", lang.c_str());
    };

    if (name.empty()) throw Exception("Not found <name language='%s'>", "ru");
    if (name_lat.empty()) throw Exception("Not found <name language='%s'>", "en");

    xmlNodePtr descrNode=node->children;
    for(int i=0; descrNode!=NULL; descrNode=descrNode->next)
    {
      if (string((const char*)descrNode->name)!="description") continue;
      if (i>=2) throw Exception("Excess <description>");
      string &d=i==0?descr1:descr2;
      d=NodeAsString(descrNode);
      if (d.size()>2) throw Exception("Wrong <description> '%s'", d.c_str());
      i++;
    };

    category=calc_category();
    visible=(emd_type!="OTHR");
  }
  catch(Exception &e)
  {
    throw Exception("TRFISCListItem::fromSirenaXML: %s", e.what());
  };
  return *this;
};

TRFISCListKey& TRFISCListKey::fromSirenaXML(xmlNodePtr node)
{
  clear();

  try
  {
    if (node==NULL) throw Exception("node not defined");

    string xml_airline = NodeAsString("@company", node);
    TElemFmt fmt;
    if (xml_airline.empty()) throw Exception("Empty @company");
    airline = ElemToElemId( etAirline, xml_airline, fmt );
    if (fmt==efmtUnknown) throw Exception("Unknown @company='%s'", xml_airline.c_str());

    string xml_service_type=NodeAsString("@service_type", node, "");
    if (xml_service_type.empty()) throw Exception("Empty @service_type");
    service_type=ServiceTypes().decode(xml_service_type);
    if (service_type==TServiceType::Unknown)
      throw Exception("Wrong @service_type='%s'", xml_service_type.c_str());

    RFISC=NodeAsString("@rfisc", node, "");
    if (RFISC.empty()) throw Exception("Empty @rfisc");
    if (RFISC.size()>15) throw Exception("Wrong @rfisc='%s'", RFISC.c_str());
  }
  catch(Exception &e)
  {
    throw Exception("TRFISCListKey::fromSirenaXML: %s", e.what());
  };
  return *this;
};

TRFISCListKey& TRFISCListKey::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  RFISC=NodeAsStringFast("rfisc",node2);
  service_type=ServiceTypes().decode(NodeAsStringFast("service_type",node2));
  airline=NodeAsStringFast("airline",node2);
  return *this;
}

TRFISCListKey& TRFISCListKey::fromXMLcompatible(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  RFISC=NodeAsStringFast("bag_type",node2);
  service_type=TServiceType::BaggageCharge;
  return *this;
}

const TRFISCListItem& TRFISCListItem::toDB(DB::TQuery &Qry) const
{
  Qry.SetVariable("rfic", RFIC);
  Qry.SetVariable("rfisc", RFISC);
  Qry.SetVariable("service_type", ServiceTypes().encode(service_type));
  Qry.SetVariable("emd_type", emd_type);
  Qry.SetVariable("name", name);
  Qry.SetVariable("name_lat", name_lat);
  Qry.SetVariable("airline", airline);
  Qry.SetVariable("grp", grp);
  Qry.SetVariable("subgrp", subgrp);
  Qry.SetVariable("descr1", descr1);
  Qry.SetVariable("descr2", descr2);
  Qry.SetVariable("category", (int)category);
  Qry.SetVariable("visible", (int)visible);
  return *this;
};

const TRFISCListKey& TRFISCListKey::toDB(TQuery &Qry) const
{
  Qry.SetVariable("rfisc", RFISC);
  service_type!=TServiceType::Unknown?
    Qry.SetVariable("service_type", ServiceTypes().encode(service_type)):
    Qry.SetVariable("service_type", FNull);
  Qry.SetVariable("airline", airline);
  return *this;
}

const TRFISCListKey& TRFISCListKey::toDB(DB::TQuery &Qry) const
{
  Qry.SetVariable("rfisc", RFISC);
  service_type!=TServiceType::Unknown?
    Qry.SetVariable("service_type", ServiceTypes().encode(service_type)):
    Qry.SetVariable("service_type", FNull);
  Qry.SetVariable("airline", airline);
  return *this;
}

const TRFISCKey& TRFISCKey::toDB(TQuery &Qry) const
{
  TRFISCListKey::toDB(Qry);
  if (!(*this==TRFISCKey()) && list_id==ASTRA::NoExists) //!!!vlad
  {
    ProgError(STDLOG, "TRFISCKey::toDB: list_id==%d! (%s)", list_id, traceStr().c_str());
    ProgError(STDLOG, "SQL=%s", Qry.SQLText.SQLText());
  }
  list_id!=ASTRA::NoExists?Qry.SetVariable("list_id", list_id):
                           Qry.SetVariable("list_id", FNull);
  return *this;
}

const TRFISCKey& TRFISCKey::toDB(DB::TQuery &Qry) const
{
  TRFISCListKey::toDB(Qry);
  if (!(*this==TRFISCKey()) && list_id==ASTRA::NoExists) //!!!vlad
  {
    ProgError(STDLOG, "TRFISCKey::toDB: list_id==%d! (%s)", list_id, traceStr().c_str());
    ProgError(STDLOG, "SQL=%s", Qry.SQLText.c_str());
  }
  list_id!=ASTRA::NoExists?Qry.SetVariable("list_id", list_id):
                           Qry.SetVariable("list_id", FNull);
  return *this;
}

void TRFISCKey::getListItemUnaccomp(int grp_id, int transfer_num,
                                    boost::optional<TServiceCategory::Enum> category,
                                    const std::string &where)
{
  getListItem(ServiceGetItemWay::Unaccomp, grp_id, transfer_num, ASTRA::NoExists, category, where);
}

void TRFISCKey::getListItemByGrpId(int grp_id, int transfer_num,
                                   boost::optional<TServiceCategory::Enum> category,
                                   const std::string &where)
{
  getListItem(ServiceGetItemWay::ByGrpId, grp_id, transfer_num, ASTRA::NoExists, category, where);
}

void TRFISCKey::getListItemByPaxId(int pax_id, int transfer_num,
                                   boost::optional<TServiceCategory::Enum> category,
                                   const std::string &where)
{
  getListItem(ServiceGetItemWay::ByPaxId, pax_id, transfer_num, ASTRA::NoExists, category, where);
}

void TRFISCKey::getListItemByBagPool(int grp_id, int transfer_num, int bag_pool_num,
                                     boost::optional<TServiceCategory::Enum> category,
                                     const std::string &where)
{
  getListItem(ServiceGetItemWay::ByBagPool, grp_id, transfer_num, bag_pool_num, category, where);
}

void TRFISCKey::getListKeyUnaccomp(int grp_id, int transfer_num,
                                   boost::optional<TServiceCategory::Enum> category,
                                   const std::string &where)
{
  getListKey(ServiceGetItemWay::Unaccomp, grp_id, transfer_num, ASTRA::NoExists, category, where);
}

void TRFISCKey::getListKeyByGrpId(int grp_id, int transfer_num,
                                  boost::optional<TServiceCategory::Enum> category,
                                  const std::string &where)
{
  getListKey(ServiceGetItemWay::ByGrpId, grp_id, transfer_num, ASTRA::NoExists, category, where);
}

void TRFISCKey::getListKeyByPaxId(int pax_id, int transfer_num,
                                  boost::optional<TServiceCategory::Enum> category,
                                  const std::string &where)
{
  getListKey(ServiceGetItemWay::ByPaxId, pax_id, transfer_num, ASTRA::NoExists, category, where);
}

void TRFISCKey::getListKeyByBagPool(int grp_id, int transfer_num, int bag_pool_num,
                                    boost::optional<TServiceCategory::Enum> category,
                                    const std::string &where)
{
  getListKey(ServiceGetItemWay::ByBagPool, grp_id, transfer_num, bag_pool_num, category, where);
}

std::string TRFISCKey::traceStr() const
{
  ostringstream s;
  s << "list_id=" << (list_id!=ASTRA::NoExists?IntToString(list_id):"NoExists")
    << ", RFISC=" << RFISC
    << ", service_type=" << ServiceTypesView().encode(service_type)
    << ", airline=" << airline;
  return s.str();
}

bool TRFISCKey::isBaggageOrCarryOn(const std::string &where) const
{
  if (!list_item)
    throw Exception("%s: item.list_item=boost::none! (%s)", where.c_str(), traceStr().c_str());

  return list_item.get().isBaggageOrCarryOn();
}

std::string getAirlineByRfiscListItem(int list_id, const std::string& rfisc,
                                      const std::string& service_type, std::string& error)
{
  DB::TQuery Qry(PgOra::getROSession("RFISC_LIST_ITEMS"), STDLOG);
  Qry.SQLText =
      "SELECT DISTINCT airline "
      "FROM rfisc_list_items "
      "WHERE list_id=:list_id "
      "AND rfisc=:rfisc "
      "AND service_type=:service_type ";
  Qry.CreateVariable("list_id", otInteger, list_id);
  Qry.CreateVariable("rfisc", otString, rfisc);
  Qry.CreateVariable("service_type", otString, service_type);
  Qry.Execute();
  if (!Qry.Eof)
  {
    const std::string result = Qry.FieldAsString("airline");
    Qry.Next();
    if (!Qry.Eof) {
      error="airline duplicate";
    }
    return result;
  } else {
    error="airline not found";
  }
  return std::string();
}

void TRFISCKey::getListKey(ServiceGetItemWay way, int id, int transfer_num, int bag_pool_num,
                           boost::optional<TServiceCategory::Enum> category,
                           const std::string &where)
{
  if (!airline.empty()) return;

  string error;
  const auto list_id_set = getServiceListIdSet(way, id, transfer_num, bag_pool_num, category);
  for (int list_id: list_id_set) {
    const std::string current_airline = getAirlineByRfiscListItem(list_id, RFISC,
                                                                  ServiceTypes().encode(service_type),
                                                                  error);
    if (!airline.empty() && airline != current_airline) {
      error="airline duplicate";
      break;
    } else {
      airline = current_airline;
    }
  }
  if (airline.empty()) {
    error="airline not found";
  }

  if (!error.empty())
  {
    throw EConvertError("%s: %s: %s (way=%d, id=%d, transfer_num=%d, bag_pool_num=%s, category=%s, RFISC=%s)",
                        where.c_str(),
                        __FUNCTION__,
                        error.c_str(),
                        way,
                        id,
                        transfer_num,
                        bag_pool_num==ASTRA::NoExists?"NoExists":IntToString(bag_pool_num).c_str(),
                        category?ServiceCategories().encode(category.get()).c_str():"",
                        RFISC.c_str());
  };
}

void TRFISCKey::getListItemLastSimilar()
{
  if (list_item) return;
  if (list_id!=ASTRA::NoExists)
  {
    getListItem();
    return;
  };

  DB::TCachedQuery Qry(
        PgOra::getROSession("RFISC_LIST_ITEMS"),
        "SELECT * FROM rfisc_list_items "
        "WHERE rfisc_list_items.rfisc=:rfisc "
        "AND rfisc_list_items.service_type=:service_type "
        "AND rfisc_list_items.airline=:airline "
        "FETCH FIRST 1 ROWS ONLY "
        "ORDER BY list_id DESC",
        QParams() << QParam("rfisc", otString)
        << QParam("service_type", otString)
        << QParam("airline", otString),
        STDLOG);

  TRFISCListKey::toDB(Qry.get());
  Qry.get().Execute();
  if (!Qry.get().Eof && !Qry.get().FieldIsNULL("list_id"))
  {
    list_id=Qry.get().FieldAsInteger("list_id");
    list_item=TRFISCListItem();
    list_item.get().fromDB(Qry.get());
  }
}

void TRFISCKey::getListItem(ServiceGetItemWay way, int id, int transfer_num, int bag_pool_num,
                            boost::optional<TServiceCategory::Enum> category,
                            const std::string &where)
{
  if (list_item) return;
  if (list_id!=ASTRA::NoExists)
  {
    getListItem();
    return;
  };

  if (id==ASTRA::NoExists) {
    throw Exception("%s: %s: id==ASTRA::NoExists (way=%d, transfer_num=%d, bag_pool_num=%s, category=%s, %s)",
                    where.c_str(),
                    __FUNCTION__,
                    way,
                    transfer_num,
                    bag_pool_num==ASTRA::NoExists?"NoExists":IntToString(bag_pool_num).c_str(),
                    category?ServiceCategories().encode(category.get()).c_str():"",
                    traceStr().c_str());
  }

  const auto list_id_set = getServiceListIdSet(way, id, transfer_num, bag_pool_num, category);
  for (int list_id_: list_id_set) {
    const auto result = getListItem(list_id_,
                                    RFISC,
                                    (service_type!=TServiceType::Unknown ? ServiceTypes().encode(service_type)
                                                                         : std::string()),
                                    airline);
    if (result) {
      list_id = list_id_;
      list_item = *result;
      break;
    }
  }

  if (list_id==ASTRA::NoExists)
  {
    throw EConvertError("%s: %s: list_id not found (way=%d, id=%d, transfer_num=%d, bag_pool_num=%s, category=%s, %s)",
                        where.c_str(),
                        __FUNCTION__,
                        way,
                        id,
                        transfer_num,
                        bag_pool_num==ASTRA::NoExists?"NoExists":IntToString(bag_pool_num).c_str(),
                        category?ServiceCategories().encode(category.get()).c_str():"",
                        traceStr().c_str());
  };
}

void TRFISCKey::getListItemIfNone()
{
  if (!list_item) getListItem();
}

void TRFISCKey::getListItemsAuto(int pax_id, int transfer_num, const std::string& rfic,
                                 TRFISCListItems& items) const
{
  items.clear();
  const auto list_id_set = getServiceListIdSet(
        ServiceGetItemWay::ByPaxId, pax_id, transfer_num, ASTRA::NoExists, boost::none);
  for (int list_id_: list_id_set) {
    DB::TCachedQuery Qry(
          PgOra::getROSession("RFISC_LIST_ITEMS"),
          "SELECT * FROM rfisc_list_items "
          "WHERE list_id=:list_id "
          "AND rfisc=:rfisc "
          "AND rfic=:rfic",
          QParams() << QParam("list_id", otInteger, list_id_)
                    << QParam("rfisc", otString, RFISC)
                    << QParam("rfic", otString, rfic),
          STDLOG);
    Qry.get().Execute();
    for(;!Qry.get().Eof; Qry.get().Next())
    {
      TRFISCListItem item;
      item.fromDB(Qry.get());
      items.push_back(item);
    }
  }
}

boost::optional<TRFISCListItem> TRFISCKey::getListItem(int list_id,
                                                       const std::string& rfisc,
                                                       const std::string& service_type,
                                                       const std::string& airline)
{
  if (list_id==ASTRA::NoExists) return boost::none;

  DB::TCachedQuery Qry(
        PgOra::getROSession("RFISC_LIST_ITEMS"),
        "SELECT * FROM rfisc_list_items "
        "WHERE list_id=:list_id "
        "AND rfisc=:rfisc "
        "AND service_type=:service_type "
        "AND airline=:airline",
        QParams() << QParam("list_id", otInteger, list_id)
                  << QParam("rfisc", otString, rfisc)
                  << QParam("service_type", otString, service_type)
                  << QParam("airline", otString, airline),
        STDLOG);
  Qry.get().Execute();
  if (Qry.get().Eof) {
    return boost::none;
  }
  return TRFISCListItem().fromDB(Qry.get());
}


void TRFISCKey::getListItem()
{
  if (list_id==ASTRA::NoExists) return;
  list_item=getListItem(list_id,
                        RFISC,
                        (service_type!=TServiceType::Unknown ? ServiceTypes().encode(service_type)
                                                             : std::string()),
                        airline);

  if (!list_item) {
    throw Exception("%s: item not found (%s)",  __FUNCTION__,  traceStr().c_str());
  }
}

TRFISCListItem& TRFISCListItem::fromDB(DB::TQuery &Qry)
{
  clear();
  TRFISCListKey::fromDB(Qry);
  RFIC=Qry.FieldAsString("rfic");
  emd_type=Qry.FieldAsString("emd_type");
  grp=Qry.FieldAsString("grp");
  subgrp=Qry.FieldAsString("subgrp");
  name=Qry.FieldAsString("name");
  name_lat=Qry.FieldAsString("name_lat");
  descr1=Qry.FieldAsString("descr1");
  descr2=Qry.FieldAsString("descr2");
  category=TServiceCategory::Enum(Qry.FieldAsInteger("category"));
  visible=Qry.FieldAsInteger("visible")!=0;
  if (Qry.GetFieldIndex("priority")>=0 && !Qry.FieldIsNULL("priority"))
    priority=Qry.FieldAsInteger("priority");
  return *this;
}

std::string TRFISCListItem::traceStr() const
{
  ostringstream s;
  s << TRFISCListKey::str(AstraLocale::LANG_EN)
    << ", RFIC=" << RFIC
    << ", grp=" << grp
    << ", subgrp=" << subgrp;
  return s.str();
}

std::optional<TRFISCListItem> TRFISCListItem::load(int list_id, const string& rfisc,
                                                   TServiceType::Enum service_type,
                                                   const string& airline)
{
  DB::TQuery Qry(PgOra::getROSession("RFISC_LIST_ITEMS"), STDLOG);
  Qry.SQLText = "SELECT * FROM rfisc_list_items "
                "WHERE list_id = :list_id "
                "AND rfisc = :rfisc "
                "AND service_type = :service_type "
                "AND airline = :airline ";
  Qry.CreateVariable("list_id", otInteger, list_id);
  Qry.CreateVariable("rfisc", otString, rfisc);
  Qry.CreateVariable("service_type", otString, ServiceTypes().encode(service_type));
  Qry.CreateVariable("airline", otString, airline);
  Qry.Execute();
  if (!Qry.Eof) {
    TRFISCListItem item;
    item.fromDB(Qry);
    return item;
  }
  return {};
}

TRFISCListKey& TRFISCListKey::fromDB(TQuery &Qry)
{
  clear();
  RFISC=Qry.FieldAsString("rfisc");
  service_type=ServiceTypes().decode(Qry.FieldAsString("service_type"));
  airline=Qry.FieldAsString("airline");
  return *this;
}

TRFISCListKey& TRFISCListKey::fromDB(DB::TQuery &Qry)
{
  clear();
  RFISC=Qry.FieldAsString("rfisc");
  service_type=ServiceTypes().decode(Qry.FieldAsString("service_type"));
  airline=Qry.FieldAsString("airline");
  return *this;
}

TRFISCKey& TRFISCKey::fromDB(TQuery &Qry)
{
  clear();
  TRFISCListKey::fromDB(Qry);
  if (!Qry.FieldIsNULL("list_id"))
    list_id=Qry.FieldAsInteger("list_id");
  return *this;
}

TRFISCKey& TRFISCKey::fromDB(DB::TQuery &Qry)
{
  clear();
  TRFISCListKey::fromDB(Qry);
  if (!Qry.FieldIsNULL("list_id"))
    list_id=Qry.FieldAsInteger("list_id");
  return *this;
}

void TRFISCList::fromSirenaXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) throw Exception("TRFISCListItem::fromSirenaXML: node not defined");
  for(node=node->children; node!=NULL; node=node->next)
  {
    if (string((const char*)node->name)!="svc") continue;
    TRFISCListItem item;
    item.fromSirenaXML(node);
    if (insert(make_pair(TRFISCListKey(item), item)).second)
      update_stat(item);
    else
      ProgTrace(TRACE5, "TRFISCListItem::fromSirenaXML: Duplicate @airline='%s' @service_type='%s' @rfisc='%s'",
                        item.airline.c_str(),
                        ServiceTypes().encode(item.service_type).c_str(),
                        item.RFISC.c_str());
  }
}

void TRFISCList::toXML(int list_id, xmlNodePtr node) const
{
  if (node==NULL) throw Exception("TRFISCListItem::toXML: node not defined");
  SetProp(node, "list_id", list_id);
  SetProp(node, "rfisc_used", (int)true);
  if (empty()) return;
  boost::optional<TRFISCListItem> def=TRFISCListItem();
  def.get().service_type=service_type_stat.frequent(TServiceType::FlightRelated);
  def.get().airline=airline_stat.frequent("");
  def.get().category=category_stat.frequent(TServiceCategory::Other);
  def.get().priority=priority_stat.frequent(boost::none);
  def.get().toXML(NewTextChild(node, "defaults"));
  for(TRFISCList::const_iterator i=begin(); i!=end(); ++i)
    i->second.toXML(NewTextChild(node, "item"), def);
}

void TRFISCList::fromDB(const ServiceListId& list_id, bool only_visible)
{
  clear();
  DB::TQuery Qry(PgOra::getROSession("RFISC_LIST_ITEMS"), STDLOG);
  if (only_visible)
    Qry.SQLText =
      "SELECT * FROM rfisc_list_items WHERE list_id=:list_id AND visible<>0";
  else
    Qry.SQLText =
      "SELECT * FROM rfisc_list_items WHERE list_id=:list_id";
  Qry.DeclareVariable( "list_id", otInteger);
  for(int id : {list_id.primary(), list_id.additional})
  {
    if (id==ASTRA::NoExists) continue;
    Qry.SetVariable( "list_id", id );
    Qry.Execute();
    for ( ;!Qry.Eof; Qry.Next())
    {
      TRFISCListItem item;
      item.fromDB(Qry);
      if (id!=list_id.primary() && !item.isBaggageOrCarryOn()) continue;
      if (insert(make_pair(TRFISCListKey(item), item)).second) update_stat(item);
    }
  }
}

void TRFISCList::toDB(int list_id) const
{
  DB::TQuery Qry(PgOra::getRWSession("RFISC_LIST_ITEMS"), STDLOG);
  Qry.SQLText =
      "INSERT INTO rfisc_list_items("
      "list_id, rfisc, service_type, airline, rfic, emd_type, grp, subgrp, "
      "name, name_lat, descr1, descr2, category, visible"
      ") VALUES("
      ":list_id, :rfisc, :service_type, :airline, :rfic, :emd_type, :grp, :subgrp, "
      ":name, :name_lat, :descr1, :descr2, :category, :visible)";
  Qry.CreateVariable( "list_id", otInteger, list_id );
  Qry.DeclareVariable( "rfisc", otString );
  Qry.DeclareVariable( "service_type", otString );
  Qry.DeclareVariable( "airline", otString );
  Qry.DeclareVariable( "grp", otString );
  Qry.DeclareVariable( "subgrp", otString );
  Qry.DeclareVariable( "descr1", otString );
  Qry.DeclareVariable( "descr2", otString );
  Qry.DeclareVariable( "category", otInteger );
  Qry.DeclareVariable( "visible", otInteger );
  Qry.DeclareVariable( "rfic", otString );
  Qry.DeclareVariable( "emd_type", otString );
  Qry.DeclareVariable( "name", otString );
  Qry.DeclareVariable( "name_lat", otString );

  for(TRFISCList::const_iterator i=begin(); i!=end(); ++i)
  {
    i->second.toDB(Qry);
    Qry.Execute();
  };
}

int TRFISCList::crc() const
{
  if (empty()) throw Exception("TRFISCList::crc: empty list");
  boost::crc_basic<32> crc32( 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true, true );
  crc32.reset();
  ostringstream buf;
  for(TRFISCList::const_iterator i=begin(); i!=end(); ++i)
    buf << i->second.RFISC << ";"
        << i->second.service_type << ";"
        << i->second.airline << ";"
        << i->second.category << ";"
        << (i->second.visible?"+":"-") << ";"
        << i->second.name.substr(0, 10) << ";"
        << i->second.name_lat.substr(0, 10) << ";";
  //ProgTrace(TRACE5, "TRFISCList::crc: %s", buf.str().c_str());
  crc32.process_bytes( buf.str().c_str(), buf.str().size() );
  return crc32.checksum();
}

int TRFISCList::toDBAdv() const
{
  int crc_tmp=crc();

  const std::set<int> list_id_set = getServiceListIdSet(crc_tmp, true /*rfisc_used*/);
  for(int list_id: list_id_set) {
    TRFISCList list;
    list.fromDB(list_id);
    if (equal(list)) return list_id;
  }

  const int list_id = saveServiceLists(crc_tmp, true /*rfisc_used*/);
  toDB(list_id);
  return list_id;
}

std::string TRFISCList::localized_name(const TRFISCListKey& key, const std::string& lang) const
{
  if (key.empty()) return "";
  TRFISCList::const_iterator i=find(key);
  if (i==end()) return "";
  return lang==AstraLocale::LANG_RU?i->second.name:i->second.name_lat;
}

bool TRFISCList::exists(const TServiceType::Enum service_type) const
{
  for(TRFISCList::const_iterator i=begin(); i!=end(); ++i)
    if (i->second.service_type==service_type) return true;

  return false;
}

bool TRFISCList::exists(const bool carryOn) const
{
  for(TRFISCList::const_iterator i=begin(); i!=end(); ++i)
  {
    if (!i->second.isBaggageOrCarryOn()) continue;
    if (i->second.isCarryOn()==carryOn) return true;
  }

  return false;
}

bool TRFISCList::equal(const TRFISCList &list) const
{
  return *this==list;
}

void TRFISCList::update_stat(const TRFISCListItem &item)
{
  service_type_stat.add(item.service_type);
  airline_stat.add(item.airline);
  category_stat.add(item.category);
  priority_stat.add(item.priority);
}

void TRFISCList::recalc_stat()
{
  service_type_stat.clear();
  airline_stat.clear();
  category_stat.clear();
  priority_stat.clear();
  for(TRFISCList::const_iterator i=begin(); i!=end(); ++i) update_stat(i->second);
}

void TRFISCList::dump() const
{
  for(const auto& i : *this)
    LogTrace(TRACE5) << "TRFISCList::dump: " << i.second.traceStr();
}

TRFISCBagProps& TRFISCBagProps::fromDB(TQuery &Qry)
{
  clear();
  airline=Qry.FieldAsString("airline");
  RFISC=Qry.FieldAsString("code");
  if (!Qry.FieldIsNULL("priority"))
    priority=Qry.FieldAsInteger("priority");
  if (!Qry.FieldIsNULL("min_weight"))
    min_weight=Qry.FieldAsInteger("min_weight");
  if (!Qry.FieldIsNULL("max_weight"))
    max_weight=Qry.FieldAsInteger("max_weight");
  rem_code_lci=Qry.FieldAsString("rem_code_lci");
  rem_code_ldm=Qry.FieldAsString("rem_code_ldm");
  return *this;
}

void TRFISCBagPropsList::fromDB(const set<string> &airlines)
{
  clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT * FROM rfisc_bag_props WHERE airline=:airline";
  for(set<string>::const_iterator a=airlines.begin(); a!=airlines.end(); ++a)
  {
    Qry.CreateVariable( "airline", otString, *a );
    Qry.Execute();
    for ( ;!Qry.Eof; Qry.Next())
    {
      TRFISCBagProps props;
      props.fromDB(Qry);
      insert(make_pair(TRFISCListKey(props.RFISC, TServiceType::BaggageCharge, props.airline), props));
    }
  }
}

void TRFISCListWithProps::bagPropsFromDB()
{
  if (!bagPropsList)
    bagPropsList=boost::in_place();
  set<string> airlines;
  for(TRFISCListWithProps::const_iterator i=begin(); i!=end(); ++i)
    airlines.insert(i->first.airline);
  bagPropsList.get().fromDB(airlines);
}

const TRFISCBagPropsList& TRFISCListWithProps::getBagPropsList()
{
  if (!bagPropsList) bagPropsFromDB();
  if (!bagPropsList)
    throw EXCEPTIONS::Exception("TRFISCListWithProps::getBagPropsList: !bagPropsList");
  return bagPropsList.get();
}

void TRFISCListWithProps::setPriority()
{
  if (!bagPropsList)
    throw EXCEPTIONS::Exception("TRFISCListWithProps::setPriority: !bagPropsList");
  for(TRFISCBagPropsList::const_iterator p=bagPropsList.get().begin(); p!=bagPropsList.get().end(); ++p)
  {
    if (p->second.priority==ASTRA::NoExists) continue;
    TRFISCListWithProps::iterator i=find(p->first);
    if (i!=end()) i->second.priority=p->second.priority;
  }
  recalc_stat();
}

void TRFISCListWithProps::fromDB(const ServiceListId& list_id, bool only_visible)
{
  clear();
  TRFISCList::fromDB(list_id, only_visible);
  bagPropsFromDB();
}

boost::optional<TRFISCBagProps> TRFISCListWithProps::getBagProps(const TRFISCListKey& key) const
{
  if (!bagPropsList) return boost::none;
  const auto& i=bagPropsList.get().find(key);
  if (i==bagPropsList.get().end()) return boost::none;
  return i->second;
}

const TRFISCBagPropsList& TRFISCListWithPropsCache::getBagPropsList(int list_id)
{
  TRFISCListWithPropsCache::iterator i=find(list_id);
  if (i==end())
  {
    i=insert(make_pair(list_id, TRFISCListWithProps())).first;
    if (i==end())
      throw EXCEPTIONS::Exception("TRFISCListWithPropsCache::getBagProps: i==end() (list_id=%d)", list_id);
    i->second.fromDB(list_id);
  };
  return i->second.getBagPropsList();
}

const ServiceListId& ServiceListId::toDB(TQuery &Qry) const
{
  list_id==ASTRA::NoExists?Qry.SetVariable("list_id", FNull):
                           Qry.SetVariable("list_id", list_id);
  return *this;
}

const ServiceListId& ServiceListId::toDB(DB::TQuery &Qry) const
{
  list_id==ASTRA::NoExists?Qry.SetVariable("list_id", FNull):
                           Qry.SetVariable("list_id", list_id);
  return *this;
}

ServiceListId& ServiceListId::fromDB(TQuery &Qry)
{
  clear();
  list_id=Qry.FieldIsNULL("list_id")?ASTRA::NoExists:
                                     Qry.FieldAsInteger("list_id");
  if (Qry.GetFieldIndex("term_list_id")>=0)
  {
    additional=Qry.FieldIsNULL("additional_list_id")?ASTRA::NoExists:
                                                     Qry.FieldAsInteger("additional_list_id");
    term_list_id=Qry.FieldIsNULL("term_list_id")?ASTRA::NoExists:
                                                 Qry.FieldAsInteger("term_list_id");
  }
  return *this;
}

ServiceListId& ServiceListId::fromDB(DB::TQuery &Qry)
{
  clear();
  list_id=Qry.FieldIsNULL("list_id")?ASTRA::NoExists:
                                     Qry.FieldAsInteger("list_id");
  if (Qry.GetFieldIndex("term_list_id")>=0)
  {
    additional=Qry.FieldIsNULL("additional_list_id")?ASTRA::NoExists:
                                                     Qry.FieldAsInteger("additional_list_id");
    term_list_id=Qry.FieldIsNULL("term_list_id")?ASTRA::NoExists:
                                                 Qry.FieldAsInteger("term_list_id");
  }
  return *this;
}

const ServiceListId& ServiceListId::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  SetProp(node, "list_id", forTerminal());
  return *this;
}

ServiceListId& ServiceListId::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  list_id=NodeAsInteger(node);
  if (list_id<0)
  {
    DB::TCachedQuery Qry(
          PgOra::getROSession("SERVICE_LISTS_GROUP"),
          "SELECT * FROM service_lists_group "
          "WHERE term_list_id=:term_list_id",
          QParams() << QParam("term_list_id", otInteger, -list_id),
          STDLOG);
    Qry.get().Execute();
    if (!Qry.get().Eof) fromDB(Qry.get());
  }

  return *this;
}

const TPaxServiceListsKey& TPaxServiceListsKey::toDB(DB::TQuery &Qry) const
{
  TPaxSegKey::toDB(Qry);
  Qry.SetVariable("category", (int)category);
  return *this;
}

TPaxServiceListsKey& TPaxServiceListsKey::fromDB(DB::TQuery &Qry)
{
  clear();
  TPaxSegKey::fromDB(Qry);
  category=TServiceCategory::Enum(Qry.FieldAsInteger("category"));
  return *this;
}

const TPaxServiceListsItem& TPaxServiceListsItem::toDB(DB::TQuery &Qry) const
{
  TPaxServiceListsKey::toDB(Qry);
  ServiceListId::toDB(Qry);
  return *this;
}

TPaxServiceListsItem& TPaxServiceListsItem::fromDB(DB::TQuery &Qry)
{
  clear();
  TPaxServiceListsKey::fromDB(Qry);
  ServiceListId::fromDB(Qry);
  return *this;
}

const TPaxServiceListsItem& TPaxServiceListsItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  SetProp(node, "seg_no", trfer_num);
  SetProp(node, "category", (int)category);
  ServiceListId::toXML(node);
  return *this;
}

std::string TPaxServiceListsItem::traceStr() const
{
  ostringstream s;
  s << "pax_id=" << pax_id << ", "
    << "trfer_num=" << trfer_num << ", "
    << "category=" << ServiceCategories().encode(category) << ", "
    << "list_id=" << list_id;
  return s.str();
}

void TPaxServiceLists::fromDB(int id, bool is_unaccomp)
{
  clear();
  DB::TCachedQuery Qry(
        is_unaccomp ? PgOra::getROSession("GRP_SERVICE_LISTS")
                    : PgOra::getROSession({"PAX_SERVICE_LISTS","SERVICE_LISTS_GROUP"}),
        is_unaccomp
        ? "SELECT grp_service_lists.*, grp_id AS pax_id "
          "FROM grp_service_lists "
          "WHERE grp_id=:id"
        : (TReqInfo::Instance()->desk.compatible(PAX_SERVICE_VERSION)
           ?
             "SELECT pax_service_lists.*, service_lists_group.term_list_id "
             "FROM pax_service_lists "
             "LEFT OUTER JOIN service_lists_group ON ( "
             "      pax_service_lists.list_id = service_lists_group.list_id "
             "      AND pax_service_lists.additional_list_id = service_lists_group.additional_list_id "
             ") "
             "WHERE pax_service_lists.pax_id = :id "
           :
             "SELECT * FROM pax_service_lists "
             "WHERE pax_id=:id"),
        QParams() << QParam("id", otInteger, id),
        STDLOG);
  Qry.get().Execute();
  for(; !Qry.get().Eof; Qry.get().Next())
    insert(TPaxServiceListsItem().fromDB(Qry.get()));
}

void TPaxServiceLists::toDB(const GrpId_t& grp_id, bool is_unaccomp) const
{
  QParams params;
  if (!is_unaccomp) {
    params << QParam("grp_id", otInteger, grp_id.get());
  }
  params << QParam("pax_id", otInteger)
         << QParam("transfer_num", otInteger)
         << QParam("category", otInteger)
         << QParam("list_id", otInteger);
  DB::TCachedQuery Qry(
        PgOra::getRWSession(is_unaccomp ? "GRP_SERVICE_LISTS" : "PAX_SERVICE_LISTS"),
        is_unaccomp
        ?
          "INSERT INTO grp_service_lists(grp_id, transfer_num, category, list_id) "
          "VALUES(:pax_id, :transfer_num, :category, :list_id)"
        :
          "INSERT INTO pax_service_lists(grp_id, pax_id, transfer_num, category, list_id) "
          "VALUES(:grp_id, :pax_id, :transfer_num, :category, :list_id)",
        params,
        STDLOG);
  for(TPaxServiceLists::const_iterator i=begin(); i!=end(); ++i)
  {
    i->toDB(Qry.get());
    Qry.get().Execute();
  }
}

void TPaxServiceLists::toXML(int id, bool is_unaccomp, int tckin_seg_count, const TCkinRoute& ckinRouteAfter, xmlNodePtr node)
{
  if (node==NULL) return;
  fromDB(id, is_unaccomp);
  for(TPaxServiceLists::const_iterator i=begin(); i!=end(); ++i)
  {
    if (i->trfer_num>=tckin_seg_count) continue; //�� ��।��� �������⥫�� ᥣ����� �࠭���
    i->toXML(NewTextChild(node, "service_list"));
  };

  if (!TReqInfo::Instance()->desk.compatible(NON_STRICT_BAG_CHECK_VERSION) && !ckinRouteAfter.empty())
  {
    if (algo::all_of(*this, [](const TPaxServiceListsItem& item) { return item.trfer_num==0; } ) &&
        algo::all_of(ckinRouteAfter, [](const TCkinRouteItem& item) { return item.transit_num!=0; } ))
    {
      for(const TPaxServiceListsItem& i : *this)
      {
        if (i.category!=TServiceCategory::BaggageInHold) continue;

        TPaxServiceListsItem item(i);
        for(item.trfer_num=1; item.trfer_num<=(int)ckinRouteAfter.size(); item.trfer_num++)
          item.toXML(NewTextChild(node, "service_list"));
      }
    }
  }
}

const TPaxSegRFISCKey& TPaxSegRFISCKey::toSirenaXML(xmlNodePtr node, const OutputLang &lang) const
{
  if (node==NULL) return *this;
  TPaxSegKey::toSirenaXML(node);
  TRFISCKey::toSirenaXML(node, lang);
  return *this;
}

TPaxSegRFISCKey& TPaxSegRFISCKey::fromSirenaXML(xmlNodePtr node)
{
  clear();
  TPaxSegKey::fromSirenaXML(node);
  TRFISCKey::fromSirenaXML(node);
  return *this;
}

const TPaxSegRFISCKey& TPaxSegRFISCKey::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  TPaxSegKey::toXML(node);
  TRFISCKey::toXML(node);
  return *this;
}

TGrpServiceItem::TGrpServiceItem(const TGrpServiceAutoItem& item)
{
  list_item=TRFISCListItem(item);
  (TPaxSegKey&)(*this)=item;
  (TRFISCListKey&)(*this)=list_item.get();
  service_quantity=item.service_quantity;
}

const TGrpServiceItem& TGrpServiceItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  TPaxSegRFISCKey::toXML(node);
  NewTextChild(node, "service_quantity", service_quantity);
  return *this;
}

TPaxSegRFISCKey& TPaxSegRFISCKey::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  TPaxSegKey::fromXML(node);
  TRFISCKey::fromXML(node);
  return *this;
}

TGrpServiceItem& TGrpServiceItem::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  TPaxSegRFISCKey::fromXML(node);
  service_quantity=NodeAsIntegerFast("service_quantity",node2);
  return *this;
}

const TPaxSegRFISCKey& TPaxSegRFISCKey::toDB(TQuery &Qry) const
{
  TPaxSegKey::toDB(Qry);
  TRFISCKey::toDB(Qry);
  return *this;
}

const TPaxSegRFISCKey& TPaxSegRFISCKey::toDB(DB::TQuery &Qry) const
{
  TPaxSegKey::toDB(Qry);
  TRFISCKey::toDB(Qry);
  return *this;
}

const TGrpServiceItem& TGrpServiceItem::toDB(TQuery &Qry) const
{
  TPaxSegRFISCKey::toDB(Qry);
  Qry.SetVariable("service_quantity",service_quantity);
  return *this;
}

const TGrpServiceItem& TGrpServiceItem::toDB(DB::TQuery &Qry) const
{
  TPaxSegRFISCKey::toDB(Qry);
  Qry.SetVariable("service_quantity",service_quantity);
  return *this;
}

TPaxSegRFISCKey& TPaxSegRFISCKey::fromDB(TQuery &Qry)
{
  clear();
  TPaxSegKey::fromDB(Qry);
  TRFISCKey::fromDB(Qry);
  return *this;
}

TPaxSegRFISCKey& TPaxSegRFISCKey::fromDB(DB::TQuery &Qry)
{
  clear();
  TPaxSegKey::fromDB(Qry);
  TRFISCKey::fromDB(Qry);
  return *this;
}

TGrpServiceItem& TGrpServiceItem::fromDB(TQuery &Qry)
{
  clear();
  TPaxSegRFISCKey::fromDB(Qry);
  getListItem();
  service_quantity=Qry.FieldAsInteger("service_quantity");
  return *this;
}

TGrpServiceItem& TGrpServiceItem::fromDB(DB::TQuery &Qry)
{
  clear();
  TPaxSegRFISCKey::fromDB(Qry);
  getListItem();
  service_quantity=Qry.FieldAsInteger("service_quantity");
  return *this;
}

std::string TPaxSegRFISCKey::traceStr() const
{
  ostringstream s;
  s << "pax_id=" << pax_id << ", "
    << "trfer_num=" << trfer_num << ", "
    << TRFISCKey::traceStr();
  return s.str();
}

std::string TPaidRFISCStatus::traceStr() const
{
  ostringstream s;
  s << TPaxSegRFISCKey::traceStr() << ", "
    << "status=" << ServiceStatuses().encode(status);
  return s.str();
}

void TPaidRFISCStatusList::add(const TPaidRFISCItem& item)
{
  if (!item.service_quantity_valid()) return;

  TPaidRFISCItem tmpItem=item;
  while(tmpItem.service_quantity>0)
  {
    if (tmpItem.need==ASTRA::NoExists ||
        tmpItem.paid==ASTRA::NoExists)
      emplace_back(tmpItem, TServiceStatus::Unknown);
    else if (tmpItem.need>0)
      emplace_back(tmpItem, TServiceStatus::Need);
    else if (tmpItem.paid>0)
      emplace_back(tmpItem, TServiceStatus::Paid);
    else
      emplace_back(tmpItem, TServiceStatus::Free);
    if (tmpItem.need!=ASTRA::NoExists) tmpItem.need--;
    if (tmpItem.paid!=ASTRA::NoExists) tmpItem.paid--;
    tmpItem.service_quantity--;
  };
}

bool TPaidRFISCStatusList::deleteIfFound(const TPaidRFISCStatus& item)
{
  TPaidRFISCStatusList::iterator i=std::find(begin(), end(), item);
  if (i!=end())
  {
    erase(i);
    return true;
  }
  return false;
}

TGrpServiceAutoItem& TGrpServiceAutoItem::fromDB(DB::TQuery &Qry)
{
  clear();
  TPaxSegKey::fromDB(Qry);
  TPaxASVCItem::fromDB(Qry);
  return *this;
}

const TGrpServiceAutoItem& TGrpServiceAutoItem::toDB(DB::TQuery &Qry) const
{
  TPaxSegKey::toDB(Qry);
  TPaxASVCItem::toDB(Qry);
  return *this;
}

bool TGrpServiceAutoItem::permittedForAutoCheckin(const TTripInfo& flt) const
{
  TCachedQuery Qry("SELECT auto_checkin, "
                   "    DECODE(flt_no,NULL,0,2)+ "
                   "    DECODE(airp_dep,NULL,0,4)+ "
                   "    DECODE(rfisc,NULL,0,1) AS priority "
                   "FROM rfisc_sets "
                   "WHERE airline=:airline AND "
                   "      rfic=:rfic AND "
                   "      (flt_no IS NULL OR flt_no=:flt_no) AND "
                   "      (airp_dep IS NULL OR airp_dep=:airp_dep) AND "
                   "      (rfisc IS NULL OR rfisc=:rfisc) "
                   "ORDER BY priority DESC",
                   QParams() << QParam("airline", otString, flt.airline)
                             << QParam("flt_no", otInteger, flt.flt_no)
                             << QParam("airp_dep", otString, flt.airp)
                             << QParam("rfic", otString, RFIC)
                             << QParam("rfisc", otString, RFISC));
  Qry.get().Execute();
  if (Qry.get().Eof) return false;
  return Qry.get().FieldAsInteger("auto_checkin")!=0;
}

bool TGrpServiceAutoItem::equalWithoutEMD(const TGrpServiceAutoItem& item) const
{
  return !withEMD() && !item.withEMD() &&
         TPaxSegKey::operator ==(item) &&
         RFIC==item.RFIC &&
         RFISC==item.RFISC &&
         service_quantity==item.service_quantity &&
         ssr_code==item.ssr_code &&
         service_name==item.service_name;
}

std::map<PaxId_t,CheckIn::TSimplePaxItem> getPaxItemMap(int id, bool is_grp_id)
{
  std::map<PaxId_t,CheckIn::TSimplePaxItem> result;
  std::list<CheckIn::TSimplePaxItem> paxes;
  if (is_grp_id) {
    paxes = CheckIn::TSimplePaxItem::getByGrpId(GrpId_t(id));
    for (const CheckIn::TSimplePaxItem& pax: paxes) {
      result.emplace(PaxId_t(pax.id), pax);
    }
  } else {
    CheckIn::TSimplePaxItem pax;
    if (pax.getByPaxId(id)) {
      result.emplace(PaxId_t(pax.id), pax);
    }
  }
  return result;
}

void TGrpServiceList::fromDB(const GrpId_t& grp_id, bool without_refused)
{
  clear();
  std::map<PaxId_t,CheckIn::TSimplePaxItem> pax_map;
  if (without_refused) {
    pax_map = getPaxItemMap(grp_id.get(), true /*is_grp_id*/);
  }
  DB::TCachedQuery Qry(
        PgOra::getROSession("PAX_SERVICES"),
        "SELECT * FROM pax_services "
        "WHERE grp_id=:grp_id",
        QParams() << QParam("grp_id", otInteger, grp_id.get()),
        STDLOG);
  Qry.get().Execute();
  for(; !Qry.get().Eof; Qry.get().Next()) {
    TGrpServiceItem item;
    item.fromDB(Qry.get());
    if (without_refused) {
      const auto pax_pos = pax_map.find(PaxId_t(item.pax_id));
      if (pax_pos != pax_map.end()) {
        const CheckIn::TSimplePaxItem& pax = pax_pos->second;
        if (!pax.refuse.empty()) {
          continue;
        }
      }
    }
    push_back(item);
  }
}

TGrpServiceAutoList& TGrpServiceAutoList::fromDB(const GrpId_t& grp_id, bool without_refused)
{
  return fromDB(grp_id.get(), true /*is_grp_id*/, without_refused);
}

TGrpServiceAutoList& TGrpServiceAutoList::fromDB(const PaxId_t& pax_id, bool without_refused)
{
  return fromDB(pax_id.get(), false /*is_grp_id*/, without_refused);
}

TGrpServiceAutoList& TGrpServiceAutoList::fromDB(int id, bool is_grp_id, bool without_refused)
{
  LogTrace(TRACE6) << "TGrpServiceAutoList::" << __func__
                   << ": id=" << id
                   << ", is_grp_id=" << is_grp_id
                   << ", without_refused=" << without_refused;
  clear();
  std::map<PaxId_t,CheckIn::TSimplePaxItem> pax_map;
  if (without_refused) {
    pax_map = getPaxItemMap(id, is_grp_id);
  }
  DB::TCachedQuery Qry(
        PgOra::getROSession("PAX_SERVICES_AUTO"),
        "SELECT * FROM pax_services_auto "
        + std::string(is_grp_id ? "WHERE grp_id=:id"
                                : "WHERE pax_id=:id"),
        QParams() << QParam("id", otInteger, id),
        STDLOG);
    Qry.get().Execute();
  for(; !Qry.get().Eof; Qry.get().Next())
  {
    TGrpServiceAutoItem item;
    item.fromDB(Qry.get());
    if (without_refused) {
      const auto pax_pos = pax_map.find(PaxId_t(item.pax_id));
      if (pax_pos != pax_map.end()) {
        const CheckIn::TSimplePaxItem& pax = pax_pos->second;
        if (!pax.refuse.empty()) {
          continue;
        }
      }
    }
    if (!item.isSuitableForAutoCheckin()) continue;
    push_back(item);
  }
  return *this;
}

void TGrpServiceListWithAuto::fromDB(const GrpId_t& grp_id, bool without_refused)
{
  LogTrace(TRACE6) << "TGrpServiceListWithAuto::" << __func__
                   << ": grp_id=" << grp_id
                   << ", without_refused=" << without_refused;
  clear();

  TGrpServiceList list1;
  list1.fromDB(grp_id, without_refused);
  for(TGrpServiceList::const_iterator i=list1.begin(); i!=list1.end(); ++i)
    push_back(*i);

  TGrpServiceAutoList list2;
  list2.fromDB(grp_id, without_refused);
  for(TGrpServiceAutoList::const_iterator i=list2.begin(); i!=list2.end(); ++i)
    addItem(*i);
}

void TGrpServiceListWithAuto::addItem(const TGrpServiceAutoItem &svcAuto)
{
  TGrpServiceItem item(svcAuto);

  TGrpServiceList::iterator i=begin();
  for(; i!=end(); ++i)
  {
    const TPaxSegRFISCKey& key=*i;
    if (key==item) break;
  };
  if (i!=end())
  {
    TGrpServiceItem& dest=*i;

    if (item.service_quantity_valid())
    {
      if (dest.service_quantity_valid())
        dest.service_quantity+=item.service_quantity;
      else
        dest.service_quantity=item.service_quantity;
    };
  }
  else push_back(item);
}

void TRFISCListItemsCache::getRFISCListItems(const TPaxSegRFISCKey& key,
                                             TRFISCListItems& items) const
{
  items.clear();
  if (!key.list_item) return;

  if (key.is_auto_service())
  {
    auto i=secret_map.find(key);
    if (i!=secret_map.end())
      items=i->second;
    else
    {
      key.getListItemsAuto(key.pax_id, key.trfer_num, key.list_item.get().RFIC, items);
      secret_map.insert(make_pair(key, items));
    };
  }
  else
    items.push_back(key.list_item.get());
}

bool TRFISCListItemsCache::isRFISCGrpExists(const TPaxSegRFISCKey& key,
                                            const string &grp, const string &subgrp) const
{
  TRFISCListItems items;
  getRFISCListItems(key, items);
  for(const auto& j : items)
    if (j.grp == grp and
        (subgrp.empty() or j.subgrp == subgrp)) return true;
  return false;
}

std::string TRFISCListItemsCache::getRFISCNameIfUnambiguous(const TPaxSegRFISCKey& key,
                                                            const std::string& lang) const
{
  std::string result;

  TRFISCListItems items;
  getRFISCListItems(key, items);
  for(const TRFISCListItem& item : items)
    if (result.empty())
      result=item.name_view(lang);
    else
      if (result!=item.name_view(lang)) return "";

  return result;
}

std::string TRFISCListItemsCache::getRFISCName(const TPaxSegRFISCKey& item, const std::string& lang) const
{
  std::string result;

  if (item.is_auto_service() || !item.list_item)
    result = getRFISCNameIfUnambiguous(item, lang);

  if (result.empty() && item.list_item)
    return item.list_item->name_view(lang);

  return result;
}

void TRFISCListItemsCache::dumpCache() const
{
  ProgTrace(TRACE5, "secret_map: start dump ");
  for(const auto& i : secret_map)
  {
    ProgTrace(TRACE5, "secret_map: %s", i.first.traceStr().c_str());
    for(const auto& j : i.second)
      ProgTrace(TRACE5, "secret_map:     %s", j.traceStr().c_str());
  }
  ProgTrace(TRACE5, "secret_map: finish dump ");
}

bool TPaidRFISCListWithAuto::isRFISCGrpExists(int pax_id, const std::string &grp, const std::string &subgrp) const
{
  for(const auto& i : *this)
  {
    if (i.second.pax_id == pax_id and i.second.trfer_num == 0)
    {
      if (TRFISCListItemsCache::isRFISCGrpExists(i.second, grp, subgrp)) return true;
    };
  };
  return false;
}

bool TPaidRFISCListWithAuto::isRFISCGrpNeedForPayment(int pax_id, const std::string &grp, const std::string &subgrp) const
{
  for(const auto& i : *this)
  {
    if (i.second.pax_id == pax_id and i.second.trfer_num == 0)
    {
      if (TRFISCListItemsCache::isRFISCGrpExists(i.second, grp, subgrp) &&
          i.second.need_positive()) return true;
    };
  };
  return false;
}

void TPaidRFISCListWithAuto::getUniqRFISCSs(int pax_id, std::set<std::string> &rfisc_set) const
{
  for(const auto& i : *this)
  {
    const TPaxSegRFISCKey& key=i.first;
    if (key.pax_id == pax_id && key.trfer_num == 0)
      rfisc_set.insert(key.RFISC);
  }
}

void TPaidRFISCListWithAuto::addItem(const TGrpServiceAutoItem &svcAuto, bool squeeze)
{
  TPaidRFISCItem item(svcAuto);

  TPaidRFISCList::iterator i=squeeze?begin():end();
  for(; i!=end(); ++i)
    if (i->second.similar(svcAuto)) break;

  if (i==end()) i=find(item);
  if (i!=end())
  {
    TPaidRFISCItem& dest=i->second;

    if (item.service_quantity_valid())
    {
      if (dest.service_quantity_valid())
        dest.service_quantity+=item.service_quantity;
      else
        dest.service_quantity=item.service_quantity;
    };
    if (item.paid_positive())
      dest.paid=(dest.paid==ASTRA::NoExists?0:dest.paid) + item.paid;
    if (item.need_positive())
      dest.need=(dest.need==ASTRA::NoExists?0:dest.need) + item.need;
  }
  else insert( make_pair(item, item) );
}

std::set<PaxId_t> loadPaxIdSet(GrpId_t grp_id);

void TPaidRFISCList::fromDB(int id, bool is_grp_id)
{
  clear();
  if (is_grp_id) {
    const std::set<PaxId_t> pax_id_set = loadPaxIdSet(GrpId_t(id));
    for (const PaxId_t& pax_id: pax_id_set) {
      fromDB(pax_id, false /*need_clear*/);
    }
  } else {
    fromDB(PaxId_t(id), false /*need_clear*/);
  }
}

void TPaidRFISCList::fromDB(const GrpId_t& grp_id)
{
  fromDB(grp_id.get(), true /*is_grp_id*/);
}

void TPaidRFISCList::fromDB(const PaxId_t& pax_id, bool need_clear)
{
  if (need_clear) {
    clear();
  }
  DB::TCachedQuery Qry(
        PgOra::getROSession("PAID_RFISC"),
        "SELECT * FROM paid_rfisc "
        "WHERE pax_id=:pax_id",
        QParams() << QParam("pax_id", otInteger, pax_id.get()),
        STDLOG);
  Qry.get().Execute();
  for(; !Qry.get().Eof; Qry.get().Next())
  {
    TPaidRFISCItem item;
    item.fromDB(Qry.get());
    insert( make_pair(item, item) );
  }
}

TPaidRFISCListWithAuto& TPaidRFISCListWithAuto::set(const TPaidRFISCList& list1,
                                                    const TGrpServiceAutoList& list2,
                                                    bool squeeze)
{
  clear();
  clearCache();

  for(const auto& i : list1) insert(i);
  for(const auto& i : list2) addItem(i, squeeze);

  return *this;
}

void TPaidRFISCListWithAuto::fromDB(int id, bool is_grp_id, bool squeeze)
{
  TPaidRFISCList list1;
  list1.fromDB(id, is_grp_id);

  TGrpServiceAutoList list2;
  if (is_grp_id) {
    list2.fromDB(GrpId_t(id));
  } else {
    list2.fromDB(PaxId_t(id));
  }

  set(list1, list2, squeeze);
}

void TGrpServiceList::clearDB(const GrpId_t& grpId)
{
  DB::TCachedQuery Qry(
        PgOra::getRWSession("PAX_SERVICES"),
        "DELETE FROM pax_services "
        "WHERE grp_id=:grp_id",
        QParams() << QParam("grp_id", otInteger, grpId.get()),
        STDLOG);
  Qry.get().Execute();
}

void TGrpServiceAutoList::clearDB(const GrpId_t& grpId)
{
  DB::TCachedQuery Qry(
        PgOra::getRWSession("PAX_SERVICES_AUTO"),
        "DELETE FROM pax_services_auto "
        "WHERE grp_id=:grp_id",
        QParams() << QParam("grp_id", otInteger, grpId.get()),
        STDLOG);
  Qry.get().Execute();
}

void TGrpServiceList::toDB(const GrpId_t& grp_id) const
{
  clearDB(grp_id);
  DB::TCachedQuery Qry(
        PgOra::getRWSession("PAX_SERVICES"),
        "INSERT INTO pax_services ("
        "pax_id, grp_id, transfer_num, list_id, rfisc, service_type, airline, service_quantity"
        ") VALUES ("
        ":pax_id, :grp_id, :transfer_num, :list_id, :rfisc, :service_type, :airline, :service_quantity"
        ")",
        QParams() << QParam("grp_id", otInteger, grp_id.get())
        << QParam("pax_id", otInteger)
        << QParam("transfer_num", otInteger)
        << QParam("list_id", otInteger)
        << QParam("rfisc", otString)
        << QParam("service_type", otString)
        << QParam("airline", otString)
        << QParam("service_quantity", otInteger),
        STDLOG);
  for(TGrpServiceList::const_iterator i=begin(); i!=end(); ++i)
  {
    i->toDB(Qry.get());
    Qry.get().Execute();
  }
}

void TGrpServiceAutoList::toDB(const GrpId_t& grp_id, bool clear) const
{
  LogTrace(TRACE6) << "TGrpServiceAutoList::" << __func__
                   << ": grp_id=" << grp_id;
  if (clear) {
    clearDB(grp_id);
  }
  DB::TCachedQuery Qry(
        PgOra::getRWSession("PAX_SERVICES_AUTO"),
        "INSERT INTO pax_services_auto ("
        "pax_id, grp_id, transfer_num, rfic, rfisc, service_quantity, "
        "ssr_code, service_name, emd_type, emd_no, emd_coupon"
        ") VALUES ("
        ":pax_id, :grp_id, :transfer_num, :rfic, :rfisc, :service_quantity, "
        ":ssr_code, :service_name, :emd_type, :emd_no, :emd_coupon"
        ")",
        QParams() << QParam("pax_id", otInteger)
        << QParam("grp_id", otInteger, grp_id.get())
        << QParam("transfer_num", otInteger)
        << QParam("rfic", otString)
        << QParam("rfisc", otString)
        << QParam("service_quantity", otInteger)
        << QParam("ssr_code", otString)
        << QParam("service_name", otString)
        << QParam("emd_type", otString)
        << QParam("emd_no", otString)
        << QParam("emd_coupon", otInteger),
        STDLOG);

  Statistic<CheckIn::TPaxASVCItem> svcStat;
  for(const TGrpServiceAutoItem& item : *this)
    if (item.withEMD())
      svcStat.add(item);

  for(const TGrpServiceAutoItem& item : *this)
  {
    if (item.withEMD())
    {
      const auto iSvc=svcStat.find(item);
      if (iSvc!=svcStat.end() && iSvc->second>1) continue; //�������騥�� ����� EMD � ࠬ�� ��㯯� �� �����뢠��!
    }
    item.toDB(Qry.get());
    Qry.get().Execute();
  }
  try {
      callbacks<RFISCCallbacks>()->afterRFISCChange(TRACE5, grp_id.get());
  } catch(...) {
      CallbacksExceptionFilter(STDLOG);
  }
}

void TGrpServiceList::copyDB(const GrpId_t& grp_id_src, const GrpId_t& grp_id_dest)
{
  LogTrace(TRACE6) << "TGrpServiceList::" << __func__
                   << ": grp_id_src=" << grp_id_src
                   << ", grp_id_dest=" << grp_id_dest;

  TGrpServiceList grpServiceListDest;
  TGrpServiceList grpServiceListSrc;
  grpServiceListSrc.fromDB(grp_id_src);
  for (const TGrpServiceItem& grpService: grpServiceListSrc) {
    const std::vector<PaxGrpRoute> routes = PaxGrpRoute::load(PaxId_t(grpService.pax_id),
                                                              grpService.trfer_num,
                                                              grp_id_src,
                                                              grp_id_dest);
    for (const PaxGrpRoute& route: routes) {
      const int pax_id = route.dest.pax_id.get();
      const int transfer_num = grpService.trfer_num + route.src.seg_no - route.dest.seg_no;
      const TPaxSegRFISCKey& old_rfisc_item = grpService;
      TPaxSegRFISCKey new_rfisc_item = old_rfisc_item;
      Sirena::TPaxSegKey &new_paxseg_key = new_rfisc_item;
      new_paxseg_key = Sirena::TPaxSegKey(pax_id, transfer_num);

      const TGrpServiceItem new_grpService(
            new_rfisc_item, grpService.service_quantity);
      grpServiceListDest.push_back(new_grpService);
    }
  }
  grpServiceListDest.toDB(grp_id_dest);
}

void TGrpServiceAutoList::copyDB(const GrpId_t& grp_id_src, const GrpId_t& grp_id_dest,
                                 bool clear)
{
  LogTrace(TRACE6) << "TGrpServiceAutoList::" << __func__
                   << ": grp_id_src=" << grp_id_src
                   << ", grp_id_dest=" << grp_id_dest;

  TGrpServiceAutoList grpServiceAutoListDest;
  TGrpServiceAutoList grpServiceAutoListSrc;
  grpServiceAutoListSrc.fromDB(grp_id_src);
  for (const TGrpServiceAutoItem& grpServiceAuto: grpServiceAutoListSrc) {
    const std::vector<PaxGrpRoute> routes = PaxGrpRoute::load(PaxId_t(grpServiceAuto.pax_id),
                                                              grpServiceAuto.trfer_num,
                                                              grp_id_src,
                                                              grp_id_dest);
    for (const PaxGrpRoute& route: routes) {
      const int pax_id = route.dest.pax_id.get();
      const int transfer_num = grpServiceAuto.trfer_num + route.src.seg_no - route.dest.seg_no;
      const Sirena::TPaxSegKey new_paxseg_key(pax_id, transfer_num);
      const CheckIn::TPaxASVCItem& old_svc_item = grpServiceAuto;

      const TGrpServiceAutoItem new_grpServiceAuto(new_paxseg_key, old_svc_item);
      grpServiceAutoListDest.push_back(new_grpServiceAuto);
    }
  }
  grpServiceAutoListDest.toDB(grp_id_dest, clear);
}

bool TGrpServiceAutoList::sameDocExists(const CheckIn::TPaxASVCItem& asvc) const
{
  for(const TGrpServiceAutoItem item : *this)
    if (item.withEMD() && item.sameDoc(asvc)) return true;
  return false;
}

bool TGrpServiceAutoList::removeEqualWithoutEMD(const TGrpServiceAutoItem& item)
{
  for(TGrpServiceAutoList::iterator i=begin(); i!=end(); ++i)
    if (item.equalWithoutEMD(*i))
    {
      erase(i);
      return true;
    }
  return false;
}

void TGrpServiceAutoList::replaceWithoutEMDFrom(const TGrpServiceAutoList& list)
{
  this->remove_if(not1(mem_fun_ref(&TGrpServiceAutoItem::withEMD)));
  copy_if(list.begin(), list.end(), inserter(*this, this->end()), not1(mem_fun_ref(&TGrpServiceAutoItem::withEMD)));
}

void TGrpServiceListWithAuto::split(int grp_id, TGrpServiceList& list1, TGrpServiceAutoList& list2) const
{
  list1.clear();
  list2.clear();
  TGrpServiceAutoList prior;
  prior.fromDB(GrpId_t(grp_id));
  for(TGrpServiceList::const_iterator i=begin(); i!=end(); ++i)
  {
    if (i->is_auto_service())
    {
      int service_quantity=0;
      for(TGrpServiceAutoList::iterator j=prior.begin(); j!=prior.end();)
      {
        TGrpServiceItem item(*j);
        if (*i==item)
        {
          list2.push_back(*j);
          if (j->service_quantity_valid()) service_quantity+=j->service_quantity;
          j=prior.erase(j);
        }
        else ++j;
      };
      if (!i->service_quantity_valid() ||
          i->service_quantity!=service_quantity)
        throw Exception("TGrpServiceList::split: wrong service_quantity (%s)", i->traceStr().c_str());
    }
    else list1.push_back(*i);
  };
}

void TGrpServiceList::mergeWith(const TGrpServiceAutoList& list)
{
  for(const TGrpServiceAutoItem& autoItem : list)
    for(TGrpServiceItem& item : *this)
      if (item.similar(autoItem))
      {
        if (autoItem.service_quantity_valid())
        {
          if (item.service_quantity_valid())
            item.service_quantity+=autoItem.service_quantity;
          else
            item.service_quantity=autoItem.service_quantity;
          break;
        }
      }
}

void TGrpServiceList::moveFrom(TGrpServiceAutoList& list)
{
  for(TGrpServiceAutoList::iterator i=list.begin(); i!=list.end();)
  {
    TGrpServiceList::iterator j=begin();
    for(; j!=end(); ++j)
    {
      if (j->similar(*i))
      {
        if (i->service_quantity_valid())
        {
          if (j->service_quantity_valid())
            j->service_quantity+=i->service_quantity;
          else
            j->service_quantity=i->service_quantity;
          break;
        };
      }
    };
    if (j!=end())
      i=list.erase(i);
    else
      ++i;
  }
}

void TPaidRFISCList::clearDB(const GrpId_t& grpId)
{
  const std::set<PaxId_t> pax_id_set = loadPaxIdSet(grpId);
  for (const PaxId_t& pax_id: pax_id_set) {
    DB::TCachedQuery Qry(
          PgOra::getRWSession("PAID_RFISC"),
          "DELETE FROM paid_rfisc "
          "WHERE pax_id=:pax_id",
          QParams() << QParam("pax_id", otInteger, pax_id.get()),
          STDLOG);
    Qry.get().Execute();
  }
}

void TPaidRFISCList::toDB(const GrpId_t& grp_id) const
{
  clearDB(grp_id);
  DB::TCachedQuery Qry(
        PgOra::getRWSession("PAID_RFISC"),
        "INSERT INTO paid_rfisc("
        "pax_id, transfer_num, list_id, rfisc, service_type, airline, service_quantity, paid, need"
        ") VALUES ("
        ":pax_id, :transfer_num, :list_id, :rfisc, :service_type, :airline, :service_quantity, :paid, :need"
        ")",
        QParams()
        << QParam("pax_id", otInteger)
        << QParam("transfer_num", otInteger)
        << QParam("list_id", otInteger)
        << QParam("rfisc", otString)
        << QParam("service_type", otString)
        << QParam("airline", otString)
        << QParam("service_quantity", otInteger)
        << QParam("paid", otInteger)
        << QParam("need", otInteger),
        STDLOG);
  for(TPaidRFISCList::const_iterator i=begin(); i!=end(); ++i)
  {
    i->second.toDB(Qry.get());
    Qry.get().Execute();
  }

  updateExcess(grp_id);
  try {
      callbacks<RFISCCallbacks>()->afterRFISCChange(TRACE5, grp_id.get());
  } catch(...) {
      CallbacksExceptionFilter(STDLOG);
  }
}

void TPaidRFISCList::updateExcess(const GrpId_t& grp_id)
{
  const int excess_pc = countPaidExcessPC(grp_id);
  DB::TCachedQuery Qry(
        PgOra::getRWSession("PAX_GRP"),
        "UPDATE pax_grp "
        "SET excess_pc=:excess_pc "
        "WHERE grp_id=:grp_id ",
        QParams()
        << QParam("grp_id", otInteger, grp_id.get())
        << QParam("excess_pc", otInteger, excess_pc),
        STDLOG);
  Qry.get().Execute();
}

void TPaidRFISCList::copyDB(const GrpId_t& grpIdSrc, const GrpId_t& grpIdDest)
{
  LogTrace(TRACE6) << "TPaidRFISCList::" << __func__
                   << ": grpIdSrc=" << grpIdSrc
                   << ", grpIdDest=" << grpIdDest;
  TPaidRFISCList PaidRFISCListDest;
  TPaidRFISCList PaidRFISCListSrc;
  PaidRFISCListSrc.fromDB(grpIdSrc);
  for (const auto& PaidRFISC: PaidRFISCListSrc) {
    const TPaxSegRFISCKey& key = PaidRFISC.first;
    const TPaidRFISCItem& item = PaidRFISC.second;
    const std::vector<PaxGrpRoute> routes = PaxGrpRoute::load(PaxId_t(key.pax_id),
                                                              key.trfer_num,
                                                              grpIdSrc,
                                                              grpIdDest);
    for (const PaxGrpRoute& route: routes) {
      const int pax_id = route.dest.pax_id.get();
      const int transfer_num = key.trfer_num + route.src.seg_no - route.dest.seg_no;
      const Sirena::TPaxSegKey new_paxseg_key(pax_id, transfer_num);
      TPaidRFISCItem new_item = item;
      Sirena::TPaxSegKey& old_paxseg_key = new_item;
      old_paxseg_key = new_paxseg_key;
      const TPaxSegRFISCKey& new_key = new_item;
      PaidRFISCListDest.emplace(new_key, new_item);
    }
  }
  PaidRFISCListDest.toDB(grpIdDest);
}

int countPaidExcessPC(const TPaidRFISCList& PaidRFISCList, bool include_all_svc)
{
  int excess_pc = 0;
  for (const auto& PaidRFISC: PaidRFISCList) {
    const TPaxSegRFISCKey& key = PaidRFISC.first;
    const TPaidRFISCItem& item = PaidRFISC.second;
    if (item.paid <= 0) {
      continue;
    }
    if (item.trfer_num != 0) {
      continue;
    }
    const std::optional<TRFISCListItem> rfisc_item = TRFISCListItem::load(
          item.list_id, key.RFISC, key.service_type, key.airline);
    if (!rfisc_item) {
      continue;
    }
    if (!include_all_svc) {
      if (rfisc_item->category != TServiceCategory::BaggageInHold
          || rfisc_item->category != TServiceCategory::BaggageInCabinOrCarryOn)
      {
        continue;
      }
    }
    excess_pc += item.paid;
  }
  return excess_pc;
}

int countPaidExcessPC(const GrpId_t& grp_id, bool include_all_svc)
{
  TPaidRFISCList PaidRFISCList;
  PaidRFISCList.fromDB(grp_id);
  return countPaidExcessPC(PaidRFISCList);
}

int countPaidExcessPC(const PaxId_t& pax_id, bool include_all_svc)
{
  TPaidRFISCList PaidRFISCList;
  PaidRFISCList.fromDB(pax_id);
  return countPaidExcessPC(PaidRFISCList);
}

void TGrpServiceList::addBagInfo(int grp_id,
                                 int tckin_seg_count,
                                 int trfer_seg_count,
                                 bool include_refused)
{
  TGrpServiceList bagList;

  TCachedQuery Qry("SELECT ckin.get_bag_pool_pax_id(bag2.grp_id, bag2.bag_pool_num, :include_refused) AS pax_id, "
                   "       0 AS transfer_num, "
                   "       bag2.list_id, "
                   "       bag2.rfisc, "
                   "       bag2.service_type, "
                   "       bag2.airline, "
                   "       bag2.amount AS service_quantity "
                   "FROM bag2 "
                   "WHERE bag2.grp_id=:grp_id AND bag2.rfisc IS NOT NULL ",
                   QParams() << QParam("grp_id", otInteger, grp_id)
                             << QParam("include_refused", otInteger, (int)include_refused));
  Qry.get().Execute();
  for(; !Qry.get().Eof; Qry.get().Next())
  {
    if (Qry.get().FieldIsNULL("pax_id")) continue;
    bagList.push_back(TGrpServiceItem().fromDB(Qry.get()));
  }

  addBagList(bagList, tckin_seg_count, trfer_seg_count);
}

void TGrpServiceList::addBagList(const TGrpServiceList& bagList,
                                 int tckin_seg_count,
                                 int trfer_seg_count)
{
  for(TGrpServiceItem item : bagList)
  {
    if (item.trfer_num!=0)
      throw Exception("%s: item.trfer_num!=0", __FUNCTION__);
    if (!item.list_item)
      throw Exception("%s: !item.list_item", __FUNCTION__);
    if (!item.list_item.get().isBaggageOrCarryOn())
      throw Exception("%s: !item.list_item.get().isBaggageOrCarryOn()", __FUNCTION__);
    bool carryOn=item.list_item.get().isCarryOn();

    for(int trfer_num=0; trfer_num<trfer_seg_count; trfer_num++)
    {
      if (trfer_num>=tckin_seg_count && carryOn) continue;
      item.trfer_num=trfer_num;

      if (trfer_num>0)
      {
        addTrueBagInfo(item);
        continue;
      }

      push_back(item);
    };
  }
}

void TGrpServiceList::addTrueBagInfo(const TGrpServiceItem& item)
{
  TRFISCKey RFISCKey;
  RFISCKey.key(item);
  for(int pass=0; pass<2; pass++)
  try
  {
    if (pass!=0)
    {
      RFISCKey.airline.clear();
      RFISCKey.getListKeyByPaxId(item.pax_id, item.trfer_num, item.list_item.get().category, __FUNCTION__);
    }
    RFISCKey.getListItemByPaxId(item.pax_id, item.trfer_num, item.list_item.get().category, __FUNCTION__);
    break;
  }
  catch(const EConvertError&) {}

  if (item.list_item &&
      item.list_item.get().isBaggageOrCarryOn() &&
      RFISCKey.list_item &&
      RFISCKey.list_item.get().isBaggageOrCarryOn() &&
      RFISCKey.list_item.get().isCarryOn()==item.list_item.get().isCarryOn())
    emplace_back(TPaxSegRFISCKey(item, RFISCKey), item.service_quantity);
  else
    push_back(item);
}

void TGrpServiceList::getAllListItems()
{
  for(TGrpServiceList::iterator i=begin(); i!=end(); ++i)
    i->getListItemByPaxId(i->pax_id, i->trfer_num, TServiceCategory::Other, "TGrpServiceList");
}

bool TGrpServiceListWithAuto::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return false;
  xmlNodePtr servicesNode=GetNode("services", node);
  if (servicesNode==NULL) return false;
  for(xmlNodePtr itemNode=servicesNode->children; itemNode!=NULL; itemNode=itemNode->next)
  {
    if (string((const char*)itemNode->name)!="item") continue;
    push_back(TGrpServiceItem().fromXML(itemNode));
  }
  return true;
}

void TGrpServiceListWithAuto::toXML(xmlNodePtr node) const
{
  if (node==NULL) return;
  xmlNodePtr servicesNode=NewTextChild(node, "services");
  for(TGrpServiceList::const_iterator i=begin(); i!=end(); ++i)
    i->toXML(NewTextChild(servicesNode, "item"));
}

const TPaidRFISCItem& TPaidRFISCItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  TGrpServiceItem::toXML(node);
  NewTextChild(node, "paid", paid);
  return *this;
}

const TPaidRFISCItem& TPaidRFISCItem::toDB(DB::TQuery &Qry) const
{
  TGrpServiceItem::toDB(Qry);
  Qry.SetVariable("paid",paid);
  Qry.SetVariable("need",need);
  return *this;
}

TPaidRFISCItem& TPaidRFISCItem::fromDB(DB::TQuery &Qry)
{
  clear();
  TGrpServiceItem::fromDB(Qry);
  paid=Qry.FieldAsInteger("paid");
  need=Qry.FieldAsInteger("need");
  return *this;
}

void TPaidRFISCList::inc(const TPaxSegRFISCKey& key, const TServiceStatus::Enum status)
{
  TPaidRFISCList::iterator i=find(key);
  if (i==end())
    i=insert( make_pair(key, TGrpServiceItem(key, 0)) ).first;
  if (i==end()) throw Exception("TPaidRFISCList::inc: i==end()!");
  i->second.service_quantity+=1;
  i->second.paid=(i->second.paid==ASTRA::NoExists?0:i->second.paid)+
    (status==TServiceStatus::Free?0:1);
  i->second.need=(i->second.need==ASTRA::NoExists?0:i->second.need)+
    (status!=TServiceStatus::Need?0:1);
}

boost::optional<TRFISCKey> TPaidRFISCList::getKeyIfSingleRFISC(int pax_id, int trfer_num, const std::string &rfisc) const
{
  boost::optional<TRFISCKey> result=boost::none;
  for(TPaidRFISCList::const_iterator i=begin(); i!=end(); ++i)
  {
    const TPaidRFISCItem &item=i->second;
    if (item.pax_id==pax_id && item.trfer_num==trfer_num && item.RFISC==rfisc)
    {
      if (result && !(result.get()==item)) return boost::none;
      result=item;
    };
  };
  return result;
}

void TPaidRFISCList::getAllListItems()
{
  for(TPaidRFISCList::iterator i=begin(); i!=end(); ++i)
    i->second.getListItemByPaxId(i->second.pax_id, i->second.trfer_num, boost::none, "TPaidRFISCList");
}

bool TPaidRFISCList::becamePaid(int grp_id) const
{
  LogTrace(TRACE5) << __func__;

  TPaidRFISCList prior;
  prior.fromDB(grp_id, true);

  for(const auto& a : *this)
    if (a.second.paid_positive())
    {
      TPaidRFISCList::const_iterator b=prior.find(a.first);
      if (b==prior.end()) return true;
      if (b->second.paid==ASTRA::NoExists || a.second.paid>b->second.paid) return true;
      if (a.second.need_positive() &&
          (b->second.need==ASTRA::NoExists || a.second.need>b->second.need)) return true;
    }
  return false;
}

const TPaidRFISCViewKey& TPaidRFISCViewKey::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  TRFISCKey::toXML(node);
  NewTextChild(node, "transfer_num", trfer_num);
  return *this;
}

std::string TPaidRFISCViewItem::name_view(const std::string& lang) const
{
  ostringstream s;
  s << RFISC << ": ";
  if (list_item) s << lowerc(list_item.get().name_view());
  return s.str();
}

const TPaidRFISCViewItem& TPaidRFISCViewItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  TPaidRFISCViewKey::toXML(node);
  NewTextChild(node, "service_quantity", service_quantity);
  NewTextChild(node, "paid", paid);
  NewTextChild(node, "priority", priority(), ASTRA::NoExists);
  return *this;
}

const TPaidRFISCViewItem& TPaidRFISCViewItem::GridInfoToXML(xmlNodePtr node,
                                                            const TTrferRoute &trfer) const
{
  if (node==NULL) return *this;

  xmlNodePtr rowNode=NewTextChild(node, "row");
  xmlNodePtr colNode;
  ostringstream s;
  s << RFISC << ": ";
  if (list_item) s << lowerc(list_item.get().name_view());
  colNode=NewTextChild(rowNode, "col", s.str());
  colNode=NewTextChild(rowNode, "col", service_quantity);
  s.str("");
  s << (trfer_num+1) << ": ";
  if (trfer_num<(int)trfer.size())
    s << trfer[trfer_num].operFlt.flight_view(ecNone, false, true);
  colNode=NewTextChild(rowNode, "col", s.str());
  colNode=NewTextChild(rowNode, "col", paid);
  if (paid!=0)
  {
    SetProp(colNode, "font_style", fsBold);
    if (paid>paid_rcpt)
    {
      SetProp(colNode, "font_color", "clInactiveAlarm");
      SetProp(colNode, "font_color_selected", "clInactiveAlarm");
    }
    else
    {
      SetProp(colNode, "font_color", "clInactiveBright");
      SetProp(colNode, "font_color_selected", "clInactiveBright");
    };
  }
  colNode=NewTextChild(rowNode, "col", paid_rcpt);
  if (paid!=0 || paid_rcpt!=0)
    SetProp(colNode, "font_style", fsBold);

  return *this;
}

int TPaidRFISCViewItem::priority() const
{
  if (list_item)
  {
    const TServiceCategory::Enum &cat=list_item.get().category;
    if (cat==TServiceCategory::BaggageInHold ||
        cat==TServiceCategory::BaggageAndCarryOn ||
        cat==TServiceCategory::BaggageInHoldWithOrigInfo ||
        cat==TServiceCategory::BaggageAndCarryOnWithOrigInfo)
      return 0;
    if (cat==TServiceCategory::BaggageInCabinOrCarryOn ||
        cat==TServiceCategory::BaggageInCabinOrCarryOnWithOrigInfo)
      return 1;
  };
  return ASTRA::NoExists;
}

void CalcPaidRFISCView(const TPaidRFISCListWithAuto &paid,
                       TPaidRFISCViewMap &paid_view)
{
  paid_view.clear();
  for(TPaidRFISCListWithAuto::const_iterator p=paid.begin(); p!=paid.end(); ++p)
  {
    TPaidRFISCViewItem item(p->second);
    TPaidRFISCViewMap::iterator i=paid_view.find(item);
    if (i!=paid_view.end())
      i->second.add(p->second);
    else
      paid_view.insert(make_pair(item, item));
  };
}

class TSumPaidRFISCViewItem : public set<TPaidRFISCViewItem>
{
  public:
    const TSumPaidRFISCViewItem& toXML(xmlNodePtr node) const;
    const TSumPaidRFISCViewItem& toXMLcompatible(xmlNodePtr node) const;
};

class TSumPaidRFISCViewMap : public map< TRFISCKey, TSumPaidRFISCViewItem >
{
  public:
    void add(const TPaidRFISCViewItem &item)
    {
      TSumPaidRFISCViewMap::iterator i=insert(make_pair(item, TSumPaidRFISCViewItem())).first;
      if (i==end()) throw Exception("%s: i==end()!", __FUNCTION__);
      i->second.insert(item);
    }
};

const TSumPaidRFISCViewItem& TSumPaidRFISCViewItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;

  if (empty()) return *this;

  ostringstream total_view, paid_view;
  int trfer_num=0;
  for(TSumPaidRFISCViewItem::const_iterator i=begin(); i!=end();)
  {
    if (!total_view.str().empty()) total_view << "/";
    if (!paid_view.str().empty()) paid_view << "/";
    if (trfer_num>=i->trfer_num)
    {
      total_view << i->service_quantity;
      paid_view << i->paid;
      if (trfer_num==i->trfer_num) trfer_num++;
      ++i;
    }
    else
    {
      total_view << "-";
      paid_view << "-";
      trfer_num++;
    };
  };

  for(TSumPaidRFISCViewItem::const_iterator i=begin(); i!=end(); ++i)
  {
    xmlNodePtr itemNode=NewTextChild(node, "item");
    i->toXML(itemNode);
    NewTextChild(itemNode, "total_view", total_view.str());
    NewTextChild(itemNode, "paid_view", paid_view.str());
  };

  return *this;
}

const TSumPaidRFISCViewItem& TSumPaidRFISCViewItem::toXMLcompatible(xmlNodePtr node) const
{
  if (node==NULL) return *this;

  if (empty()) return *this;
  int max_service_quantity=begin()->service_quantity;
  int max_paid=begin()->paid;
  bool different_paid=false;
  ostringstream paid_view;
  int trfer_num=0;
  for(TSumPaidRFISCViewItem::const_iterator i=begin(); i!=end();)
  {
    if (trfer_num>=i->trfer_num)
    {
      if (max_service_quantity<i->service_quantity) max_service_quantity=i->service_quantity;
      if (max_paid!=i->paid)
      {
        different_paid=true;
        if (max_paid<i->paid) max_paid=i->paid;
      };
      if (!paid_view.str().empty()) paid_view << "/";
      paid_view << i->paid;
      if (trfer_num==i->trfer_num) trfer_num++;
      ++i;
    }
    else
    {
      different_paid=true;
      if (!paid_view.str().empty()) paid_view << "/";
      paid_view << "?";
      trfer_num++;
    };
  }

  if (!different_paid)
  {
    paid_view.str("");
    paid_view << max_paid;
  }

  xmlNodePtr rowNode=NewTextChild(node,"paid_bag");
  NewTextChild(rowNode, "bag_type", begin()->RFISC);
  NewTextChild(rowNode, "weight", max_paid);
  NewTextChild(rowNode, "weight_calc", max_paid);
  NewTextChild(rowNode, "rate_id");

  NewTextChild(rowNode, "bag_type_view", begin()->RFISC);
  NewTextChild(rowNode, "bag_number_view", max_service_quantity);
  NewTextChild(rowNode, "weight_view", paid_view.str());
  NewTextChild(rowNode, "weight_calc_view", paid_view.str());

  return *this;
}

void PaidRFISCViewToXML(const TPaidRFISCViewMap &paid_view, xmlNodePtr node)
{
  if (node==NULL) return;

  TSumPaidRFISCViewMap paid_view_sum;
  for(TPaidRFISCViewMap::const_iterator i=paid_view.begin(); i!=paid_view.end(); ++i)
    paid_view_sum.add(i->second);

  if (TReqInfo::Instance()->desk.compatible(PAX_SERVICE_VERSION))
  {
    xmlNodePtr servicesNode=NewTextChild(node, "paid_rfiscs");
    for(TSumPaidRFISCViewMap::const_iterator i=paid_view_sum.begin(); i!=paid_view_sum.end(); ++i)
      i->second.toXML(servicesNode);
  }
  else
  {
    xmlNodePtr servicesNode=NewTextChild(node, "paid_bags");
    for(TSumPaidRFISCViewMap::const_iterator i=paid_view_sum.begin(); i!=paid_view_sum.end(); ++i)
    {
      if (!i->first.list_item || !i->first.list_item.get().isBaggageOrCarryOn()) continue; //�뢮��� ⮫쪮 �����
      i->second.toXMLcompatible(servicesNode);
    };
  }
}

bool RFISCPaymentCompleted(int grp_id, int pax_id, bool only_tckin_segs)
{
  int max_trfer_num=ASTRA::NoExists;
  if (only_tckin_segs)
    max_trfer_num=get_max_tckin_num(grp_id);

  TPaidRFISCListWithAuto paid;
  paid.fromDB(pax_id, false);
  for(TPaidRFISCListWithAuto::const_iterator i=paid.begin(); i!=paid.end(); ++i)
  {
    if (max_trfer_num!=ASTRA::NoExists && i->second.trfer_num>max_trfer_num) continue;
    if (i->second.need_positive()) return false;
  };
  return true;
}

void GetBagConcepts(int grp_id, bool &pc, bool &wt, bool &rfisc_used)
{
  pc=false;
  wt=false;
  rfisc_used=false;
  DB::TQuery Qry(PgOra::getROSession({"PAX_SERVICE_LISTS", "GRP_SERVICE_LISTS", "SERVICE_LISTS"}), STDLOG);
  Qry.SQLText=
    "SELECT DISTINCT service_lists.rfisc_used, pax_service_lists.category "
    "FROM pax_service_lists, service_lists "
    "WHERE pax_service_lists.list_id=service_lists.id AND "
    "      pax_service_lists.grp_id=:grp_id AND transfer_num=0 "
    "UNION "
    "SELECT DISTINCT service_lists.rfisc_used, grp_service_lists.category "
    "FROM grp_service_lists, service_lists "
    "WHERE grp_service_lists.list_id=service_lists.id AND "
    "      grp_service_lists.grp_id=:grp_id AND transfer_num=0";
  Qry.CreateVariable("grp_id", otInteger, grp_id);
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    bool _rfisc_used=Qry.FieldAsInteger("rfisc_used");
    int category=Qry.FieldAsInteger("category");
    if (_rfisc_used) rfisc_used=true;
    if (category==(int)TServiceCategory::BaggageInHold ||
        category==(int)TServiceCategory::BaggageInCabinOrCarryOn)
    {
      _rfisc_used?pc=true:wt=true;
    };
  }
}

bool need_for_payment(const GrpId_t& grp_id,
                      const std::string& cls,
                      int bag_refuse,
                      bool piece_concept,
                      int excess_wt,
                      const std::optional<PaxId_t>& pax_id)
{
  if (bag_refuse != 0) return false;

  if (piece_concept == 0) {
    if (excess_wt > 0) return true;
  } else {
    if (pax_id) {
      DB::TQuery Qry(PgOra::getROSession("PAID_RFISC"), STDLOG);
      Qry.SQLText = "SELECT 1 "
                    "FROM paid_rfisc "
                    "WHERE pax_id=:pax_id AND paid>0 "
                    "FETCH FIRST 1 ROWS ONLY ";
      Qry.CreateVariable("pax_id", otInteger, pax_id->get());
      Qry.Execute();
      if (!Qry.Eof) return true;
    }
  }

  DB::TQuery Qry(PgOra::getROSession("VALUE_BAG-BAG2"), STDLOG);
  Qry.SQLText = "SELECT 1 "
                "FROM value_bag "
                "LEFT OUTER JOIN bag2 ON ( "
                "     value_bag.grp_id = bag2.grp_id "
                "     AND value_bag.num = bag2.value_bag_num "
                ") "
                "WHERE "
                "(bag2.grp_id IS NULL "
                " OR ckin.bag_pool_refused(bag2.grp_id, bag2.bag_pool_num, :class, :bag_refuse) = 0) "
                "AND value_bag.grp_id = :grp_id "
                "AND value_bag.value > 0 "
                "FETCH FIRST 1 ROWS ONLY ";
  Qry.CreateVariable("grp_id", otInteger, grp_id.get());
  Qry.CreateVariable("class", otString, cls);
  Qry.CreateVariable("bag_refuse", otInteger, bag_refuse);
  Qry.Execute();
  if (!Qry.Eof) return true;

  return false;
}

