#include "baggage_wt.h"
#include "qrys.h"
#include "term_version.h"
#include <boost/crc.hpp>

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

using namespace std;
using namespace AstraLocale;
using namespace EXCEPTIONS;
using namespace BASIC::date_time;

const std::string& TBagTypeListItem::name_view(const std::string& lang) const
{
  return (lang.empty()?TReqInfo::Instance()->desk.lang:lang)==AstraLocale::LANG_RU?name:name_lat;
}

const std::string& TBagTypeListItem::descr_view(const std::string& lang) const
{
  return (lang.empty()?TReqInfo::Instance()->desk.lang:lang)==AstraLocale::LANG_RU?descr:descr_lat;
}

const TBagTypeListItem& TBagTypeListItem::toXML(xmlNodePtr node, const boost::optional<TBagTypeListItem> &def) const
{
  if (node==NULL) return *this;
  if (!(def && bag_type==def.get().bag_type))
    NewTextChild(node, "bag_type", bag_type);
  if (!(def && airline==def.get().airline))
    NewTextChild(node, "airline", airline);
  if (!(def && category==def.get().category))
    NewTextChild(node, "category", (int)category);
  if (!(def && name_view()==def.get().name_view()))
    NewTextChild(node, "name_view", name_view());
  if (!(def && descr_view()==def.get().descr_view()))
    NewTextChild(node, "descr_view", descr_view());
  if (!(def && priority==def.get().priority))
    priority?NewTextChild(node, "priority", priority.get()):
             NewTextChild(node, "priority");
  return *this;
}

std::string TBagTypeListKey::str(const std::string& lang) const
{
  std::ostringstream s;
  if (!bag_type.empty())
    s << bag_type << "/";
  s << ElemIdToPrefferedElem(etAirline, airline, efmtCodeNative,
                             lang.empty()?TReqInfo::Instance()->desk.lang:lang);
  return s.str();
}

const TBagTypeListKey& TBagTypeListKey::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  NewTextChild(node, "bag_type", bag_type);
  NewTextChild(node, "airline", airline);
  return *this;
}

const TBagTypeKey& TBagTypeKey::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  TBagTypeListKey::toXML(node);
  NewTextChild(node, "name_view", list_item?list_item.get().name_view():"");
  return *this;
}

TBagTypeListKey& TBagTypeListKey::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  bag_type=NodeAsStringFast("bag_type",node2);
  airline=NodeAsStringFast("airline",node2);
  return *this;
}

TBagTypeListKey& TBagTypeListKey::fromXMLcompatible(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  bag_type=BagTypeFromXML(NodeAsStringFast("bag_type",node2));
  return *this;
}

const TBagTypeListItem& TBagTypeListItem::toDB(DB::TQuery &Qry) const
{
  Qry.SetVariable("bag_type", bag_type==WeightConcept::REGULAR_BAG_TYPE?WeightConcept::REGULAR_BAG_TYPE_IN_DB:bag_type);
  Qry.SetVariable("airline", airline);
  Qry.SetVariable("category", (int)category);
  Qry.SetVariable("name", name);
  Qry.SetVariable("name_lat", name_lat);
  Qry.SetVariable("descr", descr);
  Qry.SetVariable("descr_lat", descr_lat);
  Qry.SetVariable("visible", (int)visible);
  return *this;
}

const TBagTypeListKey& TBagTypeListKey::toDB(DB::TQuery &Qry) const
{
  Qry.SetVariable("bag_type", bag_type);
  Qry.SetVariable("airline", airline);
  return *this;
}

const TBagTypeKey& TBagTypeKey::toDB(DB::TQuery &Qry) const
{
  TBagTypeListKey::toDB(Qry);
  if (!(*this==TBagTypeKey()) && list_id==ASTRA::NoExists) //!!!vlad
  {
    ProgError(STDLOG, "TBagTypeKey::toDB: list_id==ASTRA::NoExists! (%s)\nSQL=%s",
              traceStr().c_str(), Qry.SQLText.c_str());
  };
  list_id!=ASTRA::NoExists?Qry.SetVariable("list_id", list_id):
                           Qry.SetVariable("list_id", FNull);
  return *this;
}

const TBagTypeKey& TBagTypeKey::toDBcompatible(DB::TQuery &Qry, const std::string &where) const
{
  BagTypeToDB(Qry, bag_type, where);
  Qry.SetVariable("airline", airline);
  if (!(*this==TBagTypeKey()) && list_id==ASTRA::NoExists) //!!!vlad
  {
    ProgError(STDLOG, "TBagTypeKey::toDB: list_id==ASTRA::NoExists! (%s)", traceStr().c_str());
    ProgError(STDLOG, "SQL=%s", Qry.SQLText.c_str());
  };
  list_id!=ASTRA::NoExists?Qry.SetVariable("list_id", list_id):
                           Qry.SetVariable("list_id", FNull);
  return *this;
}

std::string TBagTypeKey::traceStr() const
{
  ostringstream s;
  s << "list_id=" << (list_id!=ASTRA::NoExists?IntToString(list_id):"NoExists")
    << ", bag_type=" << bag_type
    << ", airline=" << airline;
  return s.str();
}

std::string getAirlineByBagTypeList(int list_id, const std::string& bag_type, std::string& error)
{
  DB::TQuery Qry(PgOra::getROSession("BAG_TYPE_LIST_ITEMS"), STDLOG);
  Qry.SQLText =
      "SELECT DISTINCT airline "
      "FROM bag_type_list_items "
      "WHERE list_id=:list_id "
      "AND bag_type=:bag_type ";
  Qry.CreateVariable("list_id", otInteger, list_id);
  Qry.CreateVariable("bag_type", otString, bag_type);;
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

void TBagTypeKey::getListKey(ServiceGetItemWay way, int id, int transfer_num, int bag_pool_num,
                             boost::optional<TServiceCategory::Enum> category,
                             const std::string &where)
{
  if (!airline.empty()) return;

  std::string error;
  const auto list_id_set = getServiceListIdSet(way, id, transfer_num, bag_pool_num, category);
  for (int list_id: list_id_set) {
    const std::string current_airline = getAirlineByBagTypeList(list_id,
                                                                bag_type==WeightConcept::REGULAR_BAG_TYPE
                                                                ? WeightConcept::REGULAR_BAG_TYPE_IN_DB
                                                                : bag_type,
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
    throw EConvertError("%s: %s: %s (way=%d, id=%d, transfer_num=%d, bag_pool_num=%s, category=%s, bag_type=%s)",
                        where.c_str(),
                        __FUNCTION__,
                        error.c_str(),
                        way,
                        id,
                        transfer_num,
                        bag_pool_num==ASTRA::NoExists?"NoExists":IntToString(bag_pool_num).c_str(),
                        category?ServiceCategories().encode(category.get()).c_str():"",
                        bag_type.c_str());
  };
}

void TBagTypeKey::getListItem(ServiceGetItemWay way, int id, int transfer_num, int bag_pool_num,
                              boost::optional<TServiceCategory::Enum> category,
                              const std::string &where)
{
  if (list_item) return;
  if (list_id!=ASTRA::NoExists)
  {
    getListItem();
    return;
  };

  if (id==ASTRA::NoExists)
    throw Exception("%s: %s: id==ASTRA::NoExists (way=%d, transfer_num=%d, bag_pool_num=%s, category=%s, %s)",
                    where.c_str(),
                    __FUNCTION__,
                    way,
                    transfer_num,
                    bag_pool_num==ASTRA::NoExists?"NoExists":IntToString(bag_pool_num).c_str(),
                    category?ServiceCategories().encode(category.get()).c_str():"",
                    traceStr().c_str());

  const auto list_id_set = getServiceListIdSet(way, id, transfer_num, bag_pool_num, category);
  for (int list_id_: list_id_set) {
    const auto result = getListItem(list_id_,
                                    bag_type,
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

void TBagTypeKey::getListItemIfNone()
{
  if (!list_item) getListItem();
}

boost::optional<TBagTypeListItem> TBagTypeKey::getListItem(int list_id,
                                                           const std::string& bag_type,
                                                           const std::string& airline)
{
  if (list_id==ASTRA::NoExists) return boost::none;

  DB::TCachedQuery Qry(
        PgOra::getROSession("BAG_TYPE_LIST_ITEMS"),
        "SELECT * "
        "FROM bag_type_list_items "
        "WHERE list_id=:list_id "
        "AND bag_type=:bag_type "
        "AND airline=:airline",
        QParams() << QParam("list_id", otInteger, list_id)
        << QParam("bag_type", otString, bag_type)
        << QParam("airline", otString, airline),
        STDLOG);
  if (bag_type==WeightConcept::REGULAR_BAG_TYPE)
    Qry.get().SetVariable("bag_type", WeightConcept::REGULAR_BAG_TYPE_IN_DB);

  Qry.get().Execute();
  if (Qry.get().Eof)
    return boost::none;
  return TBagTypeListItem().fromDB(Qry.get());
}

void TBagTypeKey::getListItem()
{
  if (list_id==ASTRA::NoExists) return;
  list_item=getListItem(list_id, bag_type, airline);

  if (!list_item) {
    throw Exception("%s: item not found (%s)",  __FUNCTION__,  traceStr().c_str());
  }
}

void TBagTypeKey::getListItemUnaccomp(int grp_id, int transfer_num,
                                      boost::optional<TServiceCategory::Enum> category,
                                      const std::string &where)
{
  getListItem(ServiceGetItemWay::Unaccomp, grp_id, transfer_num, ASTRA::NoExists, category, where);
}

void TBagTypeKey::getListItemByGrpId(int grp_id, int transfer_num,
                                     boost::optional<TServiceCategory::Enum> category,
                                     const std::string &where)
{
  getListItem(ServiceGetItemWay::ByGrpId, grp_id, transfer_num, ASTRA::NoExists, category, where);
}

void TBagTypeKey::getListItemByPaxId(int pax_id, int transfer_num,
                                     boost::optional<TServiceCategory::Enum> category,
                                     const std::string &where)
{
  getListItem(ServiceGetItemWay::ByPaxId, pax_id, transfer_num, ASTRA::NoExists, category, where);
}

void TBagTypeKey::getListItemByBagPool(int grp_id, int transfer_num, int bag_pool_num,
                                       boost::optional<TServiceCategory::Enum> category,
                                       const std::string &where)
{
  getListItem(ServiceGetItemWay::ByBagPool, grp_id, transfer_num, bag_pool_num, category, where);
}

void TBagTypeKey::getListKeyUnaccomp(int grp_id, int transfer_num,
                                     boost::optional<TServiceCategory::Enum> category,
                                     const std::string &where)
{
  getListKey(ServiceGetItemWay::Unaccomp, grp_id, transfer_num, ASTRA::NoExists, category, where);
}

void TBagTypeKey::getListKeyByGrpId(int grp_id, int transfer_num,
                                    boost::optional<TServiceCategory::Enum> category,
                                    const std::string &where)
{
  getListKey(ServiceGetItemWay::ByGrpId, grp_id, transfer_num, ASTRA::NoExists, category, where);
}

void TBagTypeKey::getListKeyByPaxId(int pax_id, int transfer_num,
                                    boost::optional<TServiceCategory::Enum> category,
                                    const std::string &where)
{
  getListKey(ServiceGetItemWay::ByPaxId, pax_id, transfer_num, ASTRA::NoExists, category, where);
}

void TBagTypeKey::getListKeyByBagPool(int grp_id, int transfer_num, int bag_pool_num,
                                      boost::optional<TServiceCategory::Enum> category,
                                      const std::string &where)
{
  getListKey(ServiceGetItemWay::ByBagPool, grp_id, transfer_num, bag_pool_num, category, where);
}

TBagTypeListItem& TBagTypeListItem::fromDB(DB::TQuery &Qry)
{
  clear();
  bag_type=Qry.FieldAsString("bag_type");
  if (bag_type==WeightConcept::REGULAR_BAG_TYPE_IN_DB) bag_type=WeightConcept::REGULAR_BAG_TYPE;
  airline=Qry.FieldAsString("airline");
  category=TServiceCategory::Enum(Qry.FieldAsInteger("category"));
  name=Qry.FieldAsString("name");
  name_lat=Qry.FieldAsString("name_lat");
  descr=Qry.FieldAsString("descr");
  descr_lat=Qry.FieldAsString("descr_lat");
  visible=Qry.FieldAsInteger("visible")!=0;
  return *this;
}

TBagTypeListKey& TBagTypeListKey::fromDB(DB::TQuery &Qry)
{
  clear();
  bag_type=Qry.FieldAsString("bag_type");
  airline=Qry.FieldAsString("airline");
  return *this;
}

TBagTypeKey& TBagTypeKey::fromDB(DB::TQuery &Qry)
{
  clear();
  TBagTypeListKey::fromDB(Qry);
  if (!Qry.FieldIsNULL("list_id"))
    list_id=Qry.FieldAsInteger("list_id");
  return *this;
}

TBagTypeKey& TBagTypeKey::fromDBcompatible(DB::TQuery &Qry)
{
  clear();
  bag_type=BagTypeFromDB(Qry);
  airline=Qry.FieldAsString("airline");
  if (!Qry.FieldIsNULL("list_id"))
    list_id=Qry.FieldAsInteger("list_id");
  return *this;
}

bool TBagTypeListItem::isBaggageOrCarryOn() const
{
  return category!=TServiceCategory::Other;
}

bool TBagTypeListItem::isBaggageInCabinOrCarryOn() const
{
  return category==TServiceCategory::BaggageInCabinOrCarryOn ||
         category==TServiceCategory::BaggageAndCarryOn ||
         category==TServiceCategory::BaggageInCabinOrCarryOnWithOrigInfo ||
         category==TServiceCategory::BaggageAndCarryOnWithOrigInfo;
}

bool TBagTypeListItem::isBaggageInHold() const
{
  return category==TServiceCategory::BaggageInHold ||
         category==TServiceCategory::BaggageAndCarryOn ||
         category==TServiceCategory::BaggageInHoldWithOrigInfo ||
         category==TServiceCategory::BaggageAndCarryOnWithOrigInfo;
}

void TBagTypeList::toXML(int list_id, xmlNodePtr node) const
{
  if (node==NULL) throw Exception("TBagTypeList::toXML: node not defined");
  SetProp(node, "list_id", list_id);
  SetProp(node, "rfisc_used", (int)false);
  if (empty()) return;
  boost::optional<TBagTypeListItem> def=TBagTypeListItem();
  def.get().airline=airline_stat.frequent("");
  def.get().category=category_stat.frequent(TServiceCategory::Other);
  def.get().priority=priority_stat.frequent(boost::none);
  def.get().toXML(NewTextChild(node, "defaults"));
  for(TBagTypeList::const_iterator i=begin(); i!=end(); ++i)
    i->second.toXML(NewTextChild(node, "item"), def);
}

void TBagTypeList::fromDB(int list_id, bool only_visible)
{
  clear();
  DB::TQuery Qry(PgOra::getROSession("BAG_TYPE_LIST_ITEMS"), STDLOG);
  if (only_visible) {
    Qry.SQLText = "SELECT * FROM bag_type_list_items "
                  "WHERE list_id=:list_id AND visible<>0";
  } else {
    Qry.SQLText = "SELECT * FROM bag_type_list_items "
                  "WHERE list_id=:list_id";
  }
  Qry.CreateVariable( "list_id", otInteger, list_id );
  Qry.Execute();
  for ( ;!Qry.Eof; Qry.Next())
  {
    TBagTypeListItem item;
    item.fromDB(Qry);
    if (emplace(item, item).second) update_stat(item);
  }
}

void TBagTypeList::toDB(int list_id) const
{
  DB::TQuery Qry(PgOra::getRWSession("BAG_TYPE_LIST_ITEMS"), STDLOG);
  Qry.SQLText =
    "INSERT INTO bag_type_list_items("
    "list_id, bag_type, airline, category, name, name_lat, descr, descr_lat, visible "
    ") VALUES("
    ":list_id, :bag_type, :airline, :category, :name, :name_lat, :descr, :descr_lat, :visible"
    ")";
  Qry.CreateVariable( "list_id", otInteger, list_id );
  Qry.DeclareVariable( "bag_type", otString );
  Qry.DeclareVariable( "airline", otString );
  Qry.DeclareVariable( "category", otInteger );
  Qry.DeclareVariable( "name", otString );
  Qry.DeclareVariable( "name_lat", otString );
  Qry.DeclareVariable( "descr", otString );
  Qry.DeclareVariable( "descr_lat", otString );
  Qry.DeclareVariable( "visible", otInteger );
  for(TBagTypeList::const_iterator i=begin(); i!=end(); ++i)
  {
    i->second.toDB(Qry);
    Qry.Execute();
  }
}

int TBagTypeList::crc() const
{
  if (empty()) throw Exception("TBagTypeList::crc: empty list");
  boost::crc_basic<32> crc32( 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true, true );
  crc32.reset();
  ostringstream buf;
  for(TBagTypeList::const_iterator i=begin(); i!=end(); ++i)
    buf << i->second.bag_type << ";"
        << i->second.airline << ";"
        << i->second.category << ";"
        << (i->second.visible?"+":"-") << ";"
        << i->second.name.substr(0, 10) << ";"
        << i->second.name_lat.substr(0, 10) << ";"
        << i->second.descr.substr(0, 10) << ";"
        << i->second.descr_lat.substr(0, 10) << ";";
  crc32.process_bytes( buf.str().c_str(), buf.str().size() );
  return crc32.checksum();
}

int TBagTypeList::toDBAdv() const
{
  int crc_tmp=crc();

  const std::set<int> list_id_set = getServiceListIdSet(crc_tmp, false /*rfisc_used*/);
  for(int list_id: list_id_set) {
    TBagTypeList list;
    list.fromDB(list_id);
    if (*this==list) return list_id;
  }

  const int list_id = saveServiceLists(crc_tmp, false /*rfisc_used*/);
  toDB(list_id);
  return list_id;
}

void TBagTypeList::createWithCurrSegBagAirline(int grp_id)
{
  clear();
  create(WeightConcept::GetCurrSegBagAirline(grp_id));
}

void TBagTypeList::create(const std::string &airline)
{
  clear();
  create(airline, "", false);
}

void TBagTypeList::create(const std::string &airline,
                          const std::string &airp,
                          const bool is_unaccomp)
{
  if (airline.empty())
    throw Exception("TBagTypeList::create: airline.empty()");
  clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  if (!is_unaccomp)
  {
    Qry.SQLText=
      "SELECT bag_types.*, "
      "       LPAD(bag_types.code, 2, '0') AS code_str, "
      "       :airline AS airline, "
      "       0 AS pr_additional "
      "FROM bag_types "
      "WHERE bag_types.code<>99 ";
    Qry.CreateVariable("airline", otString, airline);
  }
  else
  {
    Qry.SQLText=
      "SELECT bag_types.*, "
      "       LPAD(bag_types.code, 2, '0') AS code_str, "
      "       :airline AS airline, "
      "       bag_unaccomp.pr_unaccomp, "
      "       bag_unaccomp.pr_additional "
      "FROM bag_types, bag_unaccomp "
      "WHERE bag_types.code=bag_unaccomp.code AND "
      "      (bag_unaccomp.airline IS NULL OR bag_unaccomp.airline=:airline) AND "
      "      (bag_unaccomp.airp IS NULL OR bag_unaccomp.airp=:airp) AND "
      "       bag_types.code<>99 "
      "ORDER BY DECODE(bag_unaccomp.airline,NULL,0,2) + "
      "         DECODE(bag_unaccomp.airp,NULL,0,1) DESC";
    Qry.CreateVariable("airline", otString, airline);
    Qry.CreateVariable("airp", otString, airp);
  };
  Qry.Execute();
  set<TBagTypeListKey> processed;
  for(;!Qry.Eof;Qry.Next())
  {
    TBagTypeListItem item;
    item.bag_type=Qry.FieldAsString("code_str");
    item.airline=Qry.FieldAsString("airline");
    item.category=Qry.FieldAsInteger("pr_additional")!=0?
                    TServiceCategory::BaggageAndCarryOnWithOrigInfo:
                    TServiceCategory::BaggageAndCarryOn;
    item.name=Qry.FieldAsString("name");
    item.name_lat=Qry.FieldAsString("name_lat");
    item.descr=Qry.FieldAsString("descr");
    item.descr_lat=Qry.FieldAsString("descr_lat");
    item.visible=true;

    if (item.name_lat.empty()) item.name_lat=item.name;
    if (item.descr_lat.empty()) item.descr_lat=item.descr;
    if (item.descr.size()>150) item.descr.erase(150);
    if (item.descr_lat.size()>150)item.descr_lat.erase(150);

    if (is_unaccomp)
    {
      if (!processed.insert(item).second) continue;
      if (Qry.FieldAsInteger("pr_unaccomp")==0) continue;
    }

    emplace(item, item);
  };

  TBagTypeListItem item;
  item.airline=airline;
  item.bag_type=WeightConcept::REGULAR_BAG_TYPE;
  item.category=TServiceCategory::BaggageAndCarryOn;
  item.name=getLocaleText(WeightConcept::REGULAR_BAG_NAME, LANG_RU);
  item.name_lat=getLocaleText(WeightConcept::REGULAR_BAG_NAME, LANG_EN);
  item.visible=!is_unaccomp;
  emplace(item, item);
}

void TBagTypeList::createForInboundTrferFromBTM(const std::string &airline)
{
  if (airline.empty())
    throw Exception("TBagTypeList::create: airline.empty()");
  clear();
  TBagTypeListItem item;
  item.airline=airline;
  item.bag_type=WeightConcept::REGULAR_BAG_TYPE;
  item.category=TServiceCategory::BaggageAndCarryOn;
  item.name=getLocaleText(WeightConcept::REGULAR_BAG_NAME, LANG_RU);
  item.name_lat=getLocaleText(WeightConcept::REGULAR_BAG_NAME, LANG_EN);
  item.visible=false;
  emplace(item, item);
}

void TBagTypeList::update_stat(const TBagTypeListItem &item)
{
  airline_stat.add(item.airline);
  category_stat.add(item.category);
  priority_stat.add(item.priority);
}

namespace WeightConcept
{

TBagTypeListKey RegularBagType(const std::string& airline)
{
  return TBagTypeListKey(REGULAR_BAG_TYPE, airline);
}

const TNormItem& TNormItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  NewTextChild(node,"norm_type", EncodeBagNormType(norm_type));
  if (amount!=ASTRA::NoExists)
    NewTextChild(node,"amount", amount);
  else
    NewTextChild(node,"amount");
  if (weight!=ASTRA::NoExists)
    NewTextChild(node,"weight", weight);
  else
    NewTextChild(node,"weight");
  if (per_unit!=ASTRA::NoExists)
    NewTextChild(node,"per_unit",(int)(per_unit!=0));
  else
    NewTextChild(node,"per_unit");
  return *this;
};

bool TNormItem::getByNormId(int normId, bool& isDirectActionNorm)
{
  isDirectActionNorm=false;
  DB::TCachedQuery Qry(
        PgOra::getROSession("BAG_NORMS"),
        "SELECT norm_type, amount, weight, per_unit, direct_action "
        "FROM bag_norms "
        "WHERE id=:id",
        QParams() << QParam("id", otInteger, normId),
        STDLOG);
  Qry.get().Execute();
  if (Qry.get().Eof) return false;
  fromDB(Qry.get());
  isDirectActionNorm=Qry.get().FieldAsInteger("direct_action")!=0;
  return true;
}

bool TNormItem::getByNormId(int normId)
{
  bool isDirectActionNorm;
  return getByNormId(normId, isDirectActionNorm);
}

std::string TNormItem::str(const std::string& lang) const
{
  ostringstream result;
  result << ElemIdToPrefferedElem(etBagNormType, EncodeBagNormType(norm_type), efmtCodeNative, lang);
  if (weight!=ASTRA::NoExists)
  {
    if (amount!=ASTRA::NoExists)
      result << " " << amount << getLocaleText("м", lang) << weight;
    else
      result << " " << weight;
    if (per_unit!=ASTRA::NoExists && per_unit!=0)
      result << getLocaleText("кг/м", lang);
    else
      result << getLocaleText("кг", lang);
  }
  else
  {
    if (amount!=ASTRA::NoExists)
      result << " " << amount << getLocaleText("м", lang);
  };
  return result.str();
}

void TNormItem::GetNorms(PrmEnum& prmenum) const
{
  if (isUnknown())
  {
     prmenum.prms << PrmBool("", false);
     return;
  }

  prmenum.prms << PrmElem<string>("", etBagNormType, EncodeBagNormType(norm_type), efmtCodeNative);

  if (weight!=ASTRA::NoExists)
  {
    if (amount!=ASTRA::NoExists)
      prmenum.prms << PrmSmpl<string>("", " ") << PrmSmpl<int>("", amount) << PrmLexema("", "EVT.P") << PrmSmpl<int>("", weight);
    else
      prmenum.prms << PrmSmpl<string>("", " ") << PrmSmpl<int>("", weight);
    if (per_unit!=ASTRA::NoExists && per_unit!=0)
      prmenum.prms << PrmLexema("", "EVT.KG_P");
    else
      prmenum.prms << PrmLexema("", "EVT.KG");
  }
  else
  {
    if (amount!=ASTRA::NoExists)
      prmenum.prms << PrmSmpl<string>("", " ") << PrmSmpl<int>("", amount) << PrmLexema("", "EVT.P");
  };
}

boost::optional<TNormItem> TNormItem::create(const boost::optional<TBagQuantity>& bagNorm)
{
  if (!bagNorm) return boost::none;

  if (bagNorm.get().getUnit()!=Ticketing::Baggage::WeightKilo &&
      bagNorm.get().getUnit()!=Ticketing::Baggage::Nil) return boost::none;

  TNormItem result;

  if (bagNorm.get().getQuantity()>0)
  {
    //норма известна и она не нулевая
    result.norm_type=ASTRA::bntFreeExcess;
    result.weight=bagNorm.get().getQuantity();
    result.per_unit=false;
  }

  return result;
}

const TPaxNormItem& TPaxNormItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  TBagTypeKey::toXML(node);
  if (norm_id!=ASTRA::NoExists)
  {
    NewTextChild(node,"norm_id",norm_id);
    NewTextChild(node,"norm_trfer",(int)norm_trfer);
  }
  else
  {
    NewTextChild(node,"norm_id");
    NewTextChild(node,"norm_trfer");
  };
  return *this;
}

TPaxNormItem& TPaxNormItem::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  TBagTypeKey::fromXMLcompatible(node);
  xmlNodePtr node2=node->children;
  if (!NodeIsNULLFast("norm_id",node2))
  {
    norm_id=NodeAsIntegerFast("norm_id",node2);
    norm_trfer=NodeAsIntegerFast("norm_trfer",node2,0)!=0;
  };
  return *this;
}

const TPaxNormItem& TPaxNormItem::toDB(DB::TQuery &Qry) const
{
  TBagTypeKey::toDBcompatible(Qry, "TPaxNormItem::toDB");
  if (norm_id!=ASTRA::NoExists)
  {
    Qry.SetVariable("norm_id",norm_id);
    Qry.SetVariable("norm_trfer",(int)norm_trfer);
  }
  else
  {
    Qry.SetVariable("norm_id",FNull);
    Qry.SetVariable("norm_trfer",FNull);
  };
  if (!handmade)
    throw Exception("TPaxNormItem::toDB: !handmade");
  Qry.SetVariable("handmade",(int)handmade.get());
  return *this;
}

TPaxNormItem& TPaxNormItem::fromDB(DB::TQuery &Qry)
{
  clear();
  TBagTypeKey::fromDBcompatible(Qry);
  if (!Qry.FieldIsNULL("norm_id"))
  {
    norm_id=Qry.FieldAsInteger("norm_id");
    norm_trfer=Qry.FieldAsInteger("norm_trfer")!=0;
  };
  handmade=Qry.FieldAsInteger("handmade")!=0;
  return *this;
}

std::ostream& operator<<(std::ostream& os, const TPaxNormItem& item)
{
  os << item.key().str(LANG_EN) << ": ";
  if (item.norm_id!=ASTRA::NoExists)
    os << item.norm_id << " (" << (item.norm_trfer?"trfer":"not trfer") << ")";
  else
    os << "-";
  os << "/" << (item.handmade?(item.handmade.get()?"hand":"auto"):"?");

  return os;
}

TPaxNormComplex fromDBO(const dbo::ARX_PAX_NORMS & pax, const dbo::ARX_BAG_NORMS & bag)
{
    TPaxNormComplex paxNorm;
    paxNorm.handmade = pax.handmade != 0;
    if(dbo::isNotNull(pax.norm_id)) {
        paxNorm.norm_id = pax.norm_id;
        paxNorm.norm_trfer = pax.norm_trfer != 0;
    }

    //Эти поля составляют bagTypeKey который используется для сравнения при добавлении в set
    {
        if(dbo::isNotNull(pax.bag_type_str))
            paxNorm.bag_type = pax.bag_type_str;
        else if(dbo::isNotNull(pax.bag_type)) {
            ostringstream s;
            s << setw(2) << setfill('0') << pax.bag_type;
            paxNorm.bag_type = s.str();
        }
        paxNorm.airline = pax.airline;
        if(dbo::isNotNull(pax.list_id)) paxNorm.list_id = pax.list_id;
    }

    paxNorm.norm_type = DecodeBagNormType(bag.norm_type.c_str());
    if(dbo::isNotNull(bag.amount)) paxNorm.amount = bag.amount;
    if(dbo::isNotNull(bag.weight))  paxNorm.weight = bag.weight;
    if(dbo::isNotNull(bag.per_unit)) paxNorm.per_unit = (int)(bag.per_unit != 0);
    return paxNorm;
}

bool PaxNormsFromDBArx(TDateTime part_key, int pax_id, TPaxNormComplexContainer &norms)
{
  LogTrace5 << __FUNCTION__ << " part_key: " << part_key << " pax_id: " << pax_id;
  norms.clear();
  dbo::Session session;
  std::vector<dbo::ARX_PAX_NORMS> arx_pax_norms = session.query<dbo::ARX_PAX_NORMS>()
          .where(" PART_KEY=:part_key and PAX_ID=:pax_id ")
          .setBind({{":part_key", DateTimeToBoost(part_key)}, {":pax_id", pax_id}});

  for(const auto &apn :  arx_pax_norms) {
      std::optional<dbo::BAG_NORMS> bag_norm = session.query<dbo::BAG_NORMS>()
              .where(" ID = :apn_norm_id")
              .setBind({{":apn_norm_id", apn.norm_id}});
      if(bag_norm) {
          norms.insert(fromDBO(apn, dbo::ARX_BAG_NORMS(*bag_norm)));
      }
      std::optional<dbo::ARX_BAG_NORMS> arx_bag_norm = session.query<dbo::ARX_BAG_NORMS>()
              .where(" ID = :apn_norm_id and PART_KEY = :part_key")
              .setBind({{":apn_norm_id", apn.norm_id}, {":part_key", apn.part_key}});

      if(arx_bag_norm) {
          norms.insert(fromDBO(apn, *arx_bag_norm));
      }
      //^^^потом удалить, когда станет pax_norms.airline NOT NULL
  }
  return !norms.empty();
}

bool PaxNormsFromDB(TDateTime part_key, int pax_id, TPaxNormComplexContainer &norms)
{
  norms.clear();
  if (part_key!=ASTRA::NoExists)
  {
      return PaxNormsFromDBArx(part_key, pax_id, norms);
  }
  DB::TCachedQuery Qry(
        PgOra::getROSession({"PAX_NORMS", "BAG_NORMS"}),
        "SELECT "
        "  pax_norms.*, "
        "  bag_norms.norm_type, "
        "  bag_norms.amount, "
        "  bag_norms.weight, "
        "  bag_norms.per_unit "
        "FROM pax_norms "
        "  LEFT OUTER JOIN bag_norms "
        "    ON pax_norms.norm_id = bag_norms.id "
        "WHERE pax_norms.pax_id = :pax_id ",
        QParams() << QParam("pax_id", otInteger, pax_id),
        STDLOG);
  Qry.get().Execute();

  //!!!потом удалить, когда станет pax_norms.airline NOT NULL
  if (part_key==ASTRA::NoExists)
  {
    for(; !Qry.get().Eof; Qry.get().Next())
    {
      TPaxNormComplex n;
      n.fromDB(Qry.get());
      n.getListKeyByPaxId(pax_id, 0, boost::none, __func__);
      norms.insert(n);
    }
  }
  return !norms.empty();
}

TPaxNormComplex fromDBO(const dbo::ARX_GRP_NORMS & pax, const dbo::ARX_BAG_NORMS & bag)
{
    TPaxNormComplex paxNorm;
    paxNorm.handmade = pax.handmade != 0;
    if(dbo::isNotNull(pax.norm_id)) {
        paxNorm.norm_id = pax.norm_id;
        paxNorm.norm_trfer = pax.norm_trfer != 0;
    }

    //Эти поля составляют bagTypeKey который используется для сравнения при добавлении в set
    if(dbo::isNotNull(pax.bag_type_str)) {
        paxNorm.bag_type = pax.bag_type_str;
    }
    else if(dbo::isNotNull(pax.bag_type)) {
        ostringstream s;
        s << setw(2) << setfill('0') << pax.bag_type;
        paxNorm.bag_type = s.str();
    }
    paxNorm.airline = pax.airline;
    if(dbo::isNotNull(pax.list_id)) paxNorm.list_id = pax.list_id;

    paxNorm.norm_type = DecodeBagNormType(bag.norm_type.c_str());
    if(dbo::isNotNull(bag.amount)) paxNorm.amount = bag.amount;
    if(dbo::isNotNull(bag.weight)) paxNorm.weight = bag.weight;
    if(dbo::isNotNull(bag.per_unit)) paxNorm.per_unit = (int)(bag.per_unit != 0);
    return paxNorm;
}

bool GrpNormsFromDBArx(TDateTime part_key, int grp_id, TPaxNormComplexContainer &norms)
{
  LogTrace5 << __FUNCTION__ << " part_key : " << part_key << " grp_id: " << grp_id;
  norms.clear();
  dbo::Session session;
  std::vector<dbo::ARX_GRP_NORMS> arx_grp_norms = session.query<dbo::ARX_GRP_NORMS>()
          .where(" PART_KEY = :part_key AND GRP_ID=:pax_id ")
          .setBind({{":part_key", DateTimeToBoost(part_key)}, {":grp_id", grp_id}});

  for(const auto &agn :  arx_grp_norms) {
      std::optional<dbo::BAG_NORMS> bag_norm = session.query<dbo::BAG_NORMS>()
              .where(" ID = :agn_norm_id ")
              .setBind({{":agn_norm_id", agn.norm_id}});
      if(bag_norm) {
          norms.insert(fromDBO(agn, dbo::ARX_BAG_NORMS(*bag_norm)));
      }
      std::optional<dbo::ARX_BAG_NORMS> arx_bag_norm = session.query<dbo::ARX_BAG_NORMS>()
              .where(" ID = :apn_norm_id ")
              .setBind({{":apn_norm_id", agn.norm_id}});

      if(arx_bag_norm) {
          norms.insert(fromDBO(agn, *arx_bag_norm));
      }
      //^^^потом удалить, когда станет pax_norms.airline NOT NULL
  }
  return !norms.empty();
}

bool GrpNormsFromDB(TDateTime part_key, int grp_id, TPaxNormComplexContainer &norms)
{
    norms.clear();
    if (part_key!=ASTRA::NoExists)
    {
        return GrpNormsFromDBArx(part_key, grp_id, norms);
    }
    DB::TCachedQuery Qry(
          PgOra::getROSession({"GRP_NORMS","BAG_NORMS"}),
          "SELECT "
          "  grp_norms.*, "
          "  bag_norms.norm_type, "
          "  bag_norms.amount, "
          "  bag_norms.weight, "
          "  bag_norms.per_unit "
          "FROM grp_norms "
          "  LEFT OUTER JOIN bag_norms "
          "    ON grp_norms.norm_id = bag_norms.id "
          "WHERE grp_norms.grp_id = :grp_id ",
          QParams() << QParam("grp_id", otInteger, grp_id),
          STDLOG);
    Qry.get().Execute();
    //!!!потом удалить, когда станет grp_norms.airline NOT NULL
    if (part_key==ASTRA::NoExists)
    {
        for(; !Qry.get().Eof; Qry.get().Next())
        {
          TPaxNormComplex n;
          n.fromDB(Qry.get());
          n.getListKeyUnaccomp(grp_id, 0, boost::none, __func__);
          norms.insert(n);
        }
    }
    return !norms.empty();
}

void NormsToXML(const TPaxNormComplexContainer &norms, xmlNodePtr node)
{
  if (node==NULL) return;

  xmlNodePtr normsNode=NewTextChild(node,"norms");
  for(const TPaxNormComplex& n : norms)
    n.toXML(NewTextChild(normsNode,"norm"));
}

void PaxNormsToDB(int pax_id, const boost::optional< TPaxNormItemContainer > &norms)
{
  if (!norms) return;

  LogTrace(TRACE5) << __func__ << ": pax_id=" << pax_id;

  DB::TQuery QryDel(PgOra::getRWSession("PAX_NORMS"), STDLOG);
  QryDel.SQLText="DELETE FROM pax_norms WHERE pax_id=:pax_id";
  QryDel.CreateVariable("pax_id",otInteger,pax_id);
  QryDel.Execute();

  DB::TQuery QryIns(PgOra::getRWSession("PAX_NORMS"), STDLOG);
  QryIns.SQLText=
    "INSERT INTO pax_norms(pax_id, list_id, bag_type, bag_type_str, airline, norm_id, norm_trfer, handmade) "
    "VALUES(:pax_id, :list_id, :bag_type, :bag_type_str, :airline, :norm_id, :norm_trfer, :handmade)";
  QryIns.CreateVariable("pax_id",otInteger,pax_id);
  QryIns.DeclareVariable("list_id",otInteger);
  QryIns.DeclareVariable("bag_type",otInteger);
  QryIns.DeclareVariable("bag_type_str",otString);
  QryIns.DeclareVariable("airline",otString);
  QryIns.DeclareVariable("norm_id",otInteger);
  QryIns.DeclareVariable("norm_trfer",otInteger);
  QryIns.DeclareVariable("handmade",otInteger);
  for(TPaxNormItem n : norms.get())
  {
    n.getListItemByPaxId(pax_id, 0, boost::none, __func__);
    n.toDB(QryIns);
    QryIns.Execute();
  }
}

void GrpNormsToDB(int grp_id, const boost::optional< TPaxNormItemContainer > &norms)
{
  if (!norms) return;

  LogTrace(TRACE5) << __func__ << ": grp_id=" << grp_id;

  DB::TQuery QryDel(PgOra::getRWSession("GRP_NORMS"), STDLOG);
  QryDel.SQLText="DELETE FROM grp_norms WHERE grp_id=:grp_id";
  QryDel.CreateVariable("grp_id",otInteger,grp_id);
  QryDel.Execute();
  DB::TQuery QryIns(PgOra::getRWSession("GRP_NORMS"), STDLOG);
  QryIns.SQLText=
    "INSERT INTO grp_norms(grp_id, list_id, bag_type, bag_type_str, airline, norm_id, norm_trfer, handmade) "
    "VALUES(:grp_id, :list_id, :bag_type, :bag_type_str, :airline, :norm_id, :norm_trfer, :handmade)";
  QryIns.CreateVariable("grp_id",otInteger,grp_id);
  QryIns.DeclareVariable("list_id",otInteger);
  QryIns.DeclareVariable("bag_type",otInteger);
  QryIns.DeclareVariable("bag_type_str",otString);
  QryIns.DeclareVariable("airline",otString);
  QryIns.DeclareVariable("norm_id",otInteger);
  QryIns.DeclareVariable("norm_trfer",otInteger);
  QryIns.DeclareVariable("handmade",otInteger);
  for(TPaxNormItem n : norms.get())
  {
    n.getListItemUnaccomp(grp_id, 0, boost::none, __func__);
    n.toDB(QryIns);
    QryIns.Execute();
  }
}

const TPaidBagItem& TPaidBagItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  TBagTypeKey::toXML(node);
  NewTextChild(node,"weight",weight);
  if (rate_id!=ASTRA::NoExists)
  {
    NewTextChild(node,"rate_id",rate_id);
    NewTextChild(node,"rate",rate);
    NewTextChild(node,"rate_cur",rate_cur);
    NewTextChild(node,"rate_trfer",(int)rate_trfer);
  }
  else
  {
    NewTextChild(node,"rate_id");
    NewTextChild(node,"rate");
    NewTextChild(node,"rate_cur");
    NewTextChild(node,"rate_trfer");
  };
  NewTextChild(node, "priority", priority(), ASTRA::NoExists);
  return *this;
};

TPaidBagItem& TPaidBagItem::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  if (TReqInfo::Instance()->desk.compatible(PAX_SERVICE_VERSION))
    TBagTypeKey::fromXML(node);
  else
    TBagTypeKey::fromXMLcompatible(node);
  weight=NodeAsIntegerFast("weight",node2);
  if (!NodeIsNULLFast("rate_id",node2))
  {
    rate_id=NodeAsIntegerFast("rate_id",node2);
    rate_trfer=NodeAsIntegerFast("rate_trfer",node2,0)!=0;
  };
  return *this;
};

const TPaidBagItem& TPaidBagItem::toDB(DB::TQuery &Qry) const
{
  TBagTypeKey::toDBcompatible(Qry, "TPaidBagItem::toDB");
  Qry.SetVariable("weight",weight);
  if (rate_id!=ASTRA::NoExists)
  {
    Qry.SetVariable("rate_id",rate_id);
    Qry.SetVariable("rate_trfer",(int)rate_trfer);
  }
  else
  {
    Qry.SetVariable("rate_id",FNull);
    Qry.SetVariable("rate_trfer",FNull);
  };
  if (handmade)
    Qry.SetVariable("handmade",(int)handmade.get());
  else
    Qry.SetVariable("handmade",FNull);
  return *this;
};

TPaidBagItem& TPaidBagItem::fromDB(DB::TQuery &Qry)
{
  clear();
  TBagTypeKey::fromDBcompatible(Qry);
  TBagTypeKey::getListItem();
  weight=Qry.FieldAsInteger("weight");
  if (!Qry.FieldIsNULL("rate_id"))
  {
    rate_id=Qry.FieldAsInteger("rate_id");
    rate=Qry.FieldAsFloat("rate");
    rate_cur=Qry.FieldAsString("rate_cur");
    rate_trfer=Qry.FieldAsInteger("rate_trfer")!=0;
  };
  handmade=Qry.FieldAsInteger("handmade")!=0;
  return *this;
};

int TPaidBagItem::priority() const
{
  if (list_item)
  {
    const TServiceCategory::Enum &cat=list_item.get().category;
    if (cat==TServiceCategory::BaggageInHold ||
        cat==TServiceCategory::BaggageAndCarryOn ||
        cat==TServiceCategory::BaggageInHoldWithOrigInfo ||
        cat==TServiceCategory::BaggageAndCarryOnWithOrigInfo)
      return 2;
    if (cat==TServiceCategory::BaggageInCabinOrCarryOn ||
        cat==TServiceCategory::BaggageInCabinOrCarryOnWithOrigInfo)
      return 3;
  };
  return ASTRA::NoExists;
}

std::string TPaidBagItem::bag_type_view(const std::string& lang) const
{
  ostringstream result;
  result << bag_type;
  if (list_item)
  {
    if (!bag_type.empty()) result << ": ";
    result << list_item.get().name_view(lang);
  };
  return result.str();
}

std::ostream& operator<<(std::ostream& os, const TPaidBagItem& item)
{
  os << item.key().str(LANG_EN) << ": " << item.weight;
  if (item.rate_id!=ASTRA::NoExists)
    os << "/" << item.rate_id;
  else
    os << "/-";
  os << "/" << (item.handmade?(item.handmade.get()?"hand":"auto"):"?");

  return os;
}

void TPaidBagList::getAllListKeys(int grp_id, bool is_unaccomp)
{
  for(TPaidBagList::iterator i=begin(); i!=end(); ++i)
    if (is_unaccomp)
      i->getListKeyUnaccomp(grp_id, 0, boost::none, "TPaidBagList");
    else
      i->getListKeyByGrpId(grp_id, 0, boost::none, "TPaidBagList");
}

void TPaidBagList::getAllListItems(int grp_id, bool is_unaccomp)
{
  for(TPaidBagList::iterator i=begin(); i!=end(); ++i)
    if (is_unaccomp)
      i->getListItemUnaccomp(grp_id, 0, boost::none, "TPaidBagList");
    else
      i->getListItemByGrpId(grp_id, 0, boost::none, "TPaidBagList");
}

bool TPaidBagList::becamePaid(const TPaidBagList& paidBefore) const
{
  LogTrace(TRACE5) << __func__;

  std::map<TBagTypeKey, TPaidBagItem> prior, curr;
  for_each(this->cbegin(), this->cend(),
           [&curr](const TPaidBagItem& item){ curr.emplace(item, item); });
  for_each(paidBefore.cbegin(), paidBefore.cend(),
           [&prior](const TPaidBagItem& item){ prior.emplace(item, item); });

  for(const auto& a : curr)
    if (a.second.paid_positive())
    {
      auto b=prior.find(a.first);
      if (b==prior.end()) return true;
      if (a.second.weight>b->second.weight) return true;
    }

  return false;
}

void PaidBagFromXML(xmlNodePtr paidbagNode,
                    int grp_id, bool is_unaccomp, bool trfer_confirm,
                    boost::optional<TPaidBagList> &paid)
{
  paid=boost::none;
  if (paidbagNode==NULL) return;
  xmlNodePtr paidBagNode=GetNode("paid_bags",paidbagNode);
  if (paidBagNode!=NULL)
  {
    paid=TPaidBagList();
    for(xmlNodePtr node=paidBagNode->children;node!=NULL;node=node->next)
      paid.get().push_back(TPaidBagItem().fromXML(node));

    if (!TReqInfo::Instance()->desk.compatible(PAX_SERVICE_VERSION))
      paid.get().getAllListKeys(grp_id, is_unaccomp);
  };
};

void PaidBagToDB(int grp_id, bool is_unaccomp,
                 TPaidBagList &paid)
{
  paid.getAllListItems(grp_id, is_unaccomp);

  DB::TQuery QryDel(PgOra::getRWSession("PAID_BAG"), STDLOG);
  QryDel.SQLText="DELETE FROM paid_bag WHERE grp_id=:grp_id";
  QryDel.CreateVariable("grp_id",otInteger,grp_id);
  QryDel.Execute();

  DB::TQuery QryIns(PgOra::getRWSession("PAID_BAG"), STDLOG);
  QryIns.SQLText=
    "INSERT INTO paid_bag("
    "grp_id, list_id, bag_type, bag_type_str, airline, weight, rate_id, rate_trfer, handmade"
    ") VALUES("
    ":grp_id, :list_id, :bag_type, :bag_type_str, :airline, :weight, :rate_id, :rate_trfer, :handmade"
    ")";
  QryIns.CreateVariable("grp_id",otInteger,grp_id);
  QryIns.DeclareVariable("list_id",otInteger);
  QryIns.DeclareVariable("bag_type",otInteger);
  QryIns.DeclareVariable("bag_type_str",otString);
  QryIns.DeclareVariable("airline",otString);
  QryIns.DeclareVariable("weight",otInteger);
  QryIns.DeclareVariable("rate_id",otInteger);
  QryIns.DeclareVariable("rate_trfer",otInteger);
  QryIns.DeclareVariable("handmade",otInteger);
  int excess_wt=0;
  for(TPaidBagList::iterator i=paid.begin(); i!=paid.end(); ++i)
  {
    excess_wt+=i->weight;
    i->toDB(QryIns);
    QryIns.Execute();
  }

  DB::TQuery QryGrp(PgOra::getRWSession("PAX_GRP"), STDLOG);
  QryGrp.SQLText =
    "UPDATE pax_grp "
    "SET excess_wt=:excess_wt "
    "WHERE grp_id=:grp_id";
  QryGrp.CreateVariable("grp_id", otInteger, grp_id);
  QryGrp.CreateVariable("excess_wt", otInteger, excess_wt);
  QryGrp.Execute();
}

TPaidBagItem& TPaidBagItem::fromDBO(const dbo::ARX_PAID_BAG & apb, const dbo::ARX_BAG_RATES & bag_rate)
{
    TPaidBagItem paidBag;
    //тоже самое что TBagTypeKey::fromDBcompatible(Qry);
    if(dbo::isNotNull(apb.bag_type_str))
        paidBag.bag_type = apb.bag_type_str;
    else if(dbo::isNotNull(apb.bag_type)) {
        ostringstream s;
        s << setw(2) << setfill('0') << apb.bag_type;
        paidBag.bag_type = s.str();
    }
    paidBag.airline = apb.airline;
    if(dbo::isNotNull(apb.list_id))
        paidBag.list_id = apb.list_id;
    TBagTypeKey::getListItem();

    paidBag.weight = apb.weight;
    if (dbo::isNotNull(apb.rate_id))
    {
      paidBag.rate_id = apb.rate_id;
      paidBag.rate_trfer = apb.rate_trfer != 0;
      paidBag.rate = bag_rate.rate;
      paidBag.rate_cur = bag_rate.rate_cur;
    };
    paidBag.handmade = apb.handmade != 0;
    return *this;
}

void PaidBagFromDBArx(TDateTime part_key, int grp_id, TPaidBagList &paid)
{
  LogTrace5 << __FUNCTION__ << " part_key: " << part_key << " grp_id: " << grp_id;
  paid.clear();
  dbo::Session session;
  std::vector<dbo::ARX_PAID_BAG> arx_paid_bags = session.query<dbo::ARX_PAID_BAG>()
          .where(" PART_KEY = :part_key AND GRP_ID=:pax_id ")
          .setBind({{":part_key", DateTimeToBoost(part_key)}, {":grp_id", grp_id}});

  //BAG_RATES, ARX_BAG_RATES для полей : rate, rate_cur
  for(const auto &apb :  arx_paid_bags) {
      std::optional<dbo::BAG_RATES> bag_rate = session.query<dbo::BAG_RATES>()
              .where(" ID = :apb_rate_id ").setBind({{":apb_rate_id", apb.rate_id}});
      std::optional<dbo::ARX_BAG_RATES> arx_bag_rate = session.query<dbo::ARX_BAG_RATES>()
              .where(" ID = :apb_rate_id and PART_KEY = :part_key")
              .setBind({{":apb_rate_id", apb.rate_id}, {":part_key", apb.part_key}});

      if(bag_rate && arx_bag_rate) {
          //если строки прочитанные из bag_rates и arx_bag_rates равны,то не нужно добавлять
          // 2 строки , потому что union отсекает одинаковые строки
          if(bag_rate->rate - arx_bag_rate->rate < 0.001 &&
                  bag_rate->rate_cur == arx_bag_rate->rate_cur)
          {
            paid.push_back(TPaidBagItem().fromDBO(apb, *arx_bag_rate));
            continue;
          }
      }
      if(bag_rate) {
          paid.push_back(TPaidBagItem().fromDBO(apb, *bag_rate));
      }
      if(arx_bag_rate) {
          paid.push_back(TPaidBagItem().fromDBO(apb, *arx_bag_rate));
      }
  }
}

void PaidBagFromDB(TDateTime part_key, int grp_id, TPaidBagList &paid)
{
    if (part_key!=ASTRA::NoExists)
    {
        return PaidBagFromDBArx(part_key, grp_id, paid);
    }
    paid.clear();
    DB::TCachedQuery Qry(
          PgOra::getROSession({"PAID_BAG", "BAG_RATES"}),
          "SELECT "
          "  paid_bag.*, "
          "  bag_rates.rate, "
          "  bag_rates.rate_cur "
          "FROM paid_bag "
          "  LEFT OUTER JOIN bag_rates "
          "    ON paid_bag.rate_id = bag_rates.id "
          "WHERE paid_bag.grp_id = :grp_id",
          QParams() << QParam("grp_id", otInteger, grp_id),
          STDLOG);
    Qry.get().Execute();
    for(;!Qry.get().Eof;Qry.get().Next())
    paid.push_back(TPaidBagItem().fromDB(Qry.get()));
}

std::string GetCurrSegBagAirline(int grp_id)
{
  DB::TCachedQuery Qry(
        PgOra::getROSession({"PAX_GRP", "MARK_TRIPS", "POINTS"}),
        "SELECT "
        "CASE WHEN pax_grp.pr_mark_norms = 0 THEN points.airline ELSE mark_trips.airline END AS airline "
        "FROM pax_grp, mark_trips, points "
        "WHERE pax_grp.point_dep=points.point_id AND "
        "      pax_grp.point_id_mark=mark_trips.point_id AND "
        "      pax_grp.grp_id=:id",
        QParams() << QParam("id", otInteger, grp_id),
        STDLOG);
  Qry.get().Execute();
  if (!Qry.get().Eof)
    return Qry.get().FieldAsString("airline");

  return "";
}

} //namespace WeightConcept

std::string TPaidBagViewItem::name_view(const std::string& lang) const
{
  ostringstream s;
  if (!bag_type.empty()) s << bag_type << ": ";
  if (list_item) s << lowerc(list_item.get().name_view());
  return s.str();
}

std::string TPaidBagViewItem::paid_view() const
{
  ostringstream s;
  if (weight!=ASTRA::NoExists)
    s << weight;
  else
    s << "?";
  return s.str();
}

std::string TPaidBagViewItem::paid_calc_view() const
{
  ostringstream s;
  if (weight_calc!=ASTRA::NoExists)
    s << weight_calc;
  else
    s << "?";
  return s.str();
}

std::string TPaidBagViewItem::paid_rcpt_view() const
{
  ostringstream s;
  if (weight_rcpt!=ASTRA::NoExists)
    s << weight_rcpt;
  else
    s << "?";
  return s.str();
}

const TPaidBagViewItem& TPaidBagViewItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  TPaidBagItem::toXML(node);
  if (weight_calc!=ASTRA::NoExists)
    NewTextChild(node, "weight_calc", weight_calc);
  else
    NewTextChild(node, "weight_calc");
  if (TReqInfo::Instance()->desk.compatible(PAX_SERVICE_VERSION))
  {
    NewTextChild(node, "total_view", total_view);
    NewTextChild(node, "paid_view", paid_view());
  }
  else
  {
    NewTextChild(node, "bag_type_view", bag_type_view());
    NewTextChild(node, "bag_number_view", total_view);
    NewTextChild(node, "norms_view", norms_view);
    NewTextChild(node, "norms_trfer_view", norms_trfer_view);
    NewTextChild(node, "weight_view", paid_view());
    NewTextChild(node, "weight_calc_view", paid_calc_view());
    NewTextChild(node, "emd_weight_view", paid_rcpt_view());
  };
  return *this;
}

const TPaidBagViewItem& TPaidBagViewItem::GridInfoToXML(xmlNodePtr node, bool wide_format) const
{
  if (node==NULL) return *this;

  xmlNodePtr rowNode=NewTextChild(node, "row");
  xmlNodePtr colNode;
  colNode=NewTextChild(rowNode, "col", name_view());
  colNode=NewTextChild(rowNode, "col", total_view);
  colNode=NewTextChild(rowNode, "col", norms_view);
  if (wide_format)
    //только весовая система расчета
    colNode=NewTextChild(rowNode, "col", norms_trfer_view);
  colNode=NewTextChild(rowNode, "col", paid_view());
  if (weight!=0)
  {
    SetProp(colNode, "font_style", fsBold);
    if (weight>weight_rcpt)
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
  colNode=NewTextChild(rowNode, "col", paid_rcpt_view());
  if (weight!=0 || weight_rcpt!=0)
    SetProp(colNode, "font_style", fsBold);
  return *this;
}


