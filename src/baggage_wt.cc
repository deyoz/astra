#include "baggage_wt.h"
#include "qrys.h"
#include "term_version.h"
#include <boost/crc.hpp>

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

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

const TBagTypeListItem& TBagTypeListItem::toDB(TQuery &Qry) const
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

const TBagTypeListKey& TBagTypeListKey::toDB(TQuery &Qry) const
{
  Qry.SetVariable("bag_type", bag_type);
  Qry.SetVariable("airline", airline);
  return *this;
}

const TBagTypeKey& TBagTypeKey::toDB(TQuery &Qry) const
{
  TBagTypeListKey::toDB(Qry);
  if (!(*this==TBagTypeKey()) && list_id==ASTRA::NoExists) //!!!vlad
  {
    ProgError(STDLOG, "TBagTypeKey::toDB: list_id==ASTRA::NoExists! (%s)", traceStr().c_str());
    ProgError(STDLOG, "SQL=%s", Qry.SQLText.SQLText());
  };
  list_id!=ASTRA::NoExists?Qry.SetVariable("list_id", list_id):
                           Qry.SetVariable("list_id", FNull);
  return *this;
}

const TBagTypeKey& TBagTypeKey::toDBcompatible(TQuery &Qry, const std::string &where) const
{
  BagTypeToDB(Qry, bag_type, where);
  Qry.SetVariable("airline", airline);
  if (!(*this==TBagTypeKey()) && list_id==ASTRA::NoExists) //!!!vlad
  {
    ProgError(STDLOG, "TBagTypeKey::toDB: list_id==ASTRA::NoExists! (%s)", traceStr().c_str());
    ProgError(STDLOG, "SQL=%s", Qry.SQLText.SQLText());
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

void TBagTypeKey::getListKey(GetItemWay way, int id, int transfer_num, int bag_pool_num,
                             boost::optional<TServiceCategory::Enum> category,
                             const std::string &where)
{
  if (!airline.empty()) return;
  ostringstream sql;
  QParams QryParams;
  if (way==Unaccomp)
  {
    sql << "SELECT DISTINCT bag_type_list_items.airline \n"
           "FROM grp_service_lists, bag_type_list_items \n"
           "WHERE grp_service_lists.list_id=bag_type_list_items.list_id AND \n"
           "      grp_service_lists.grp_id=:id AND \n"
           "      grp_service_lists.transfer_num=:transfer_num AND \n";
  };
  if (way==ByGrpId)
  {
    sql << "SELECT DISTINCT bag_type_list_items.airline \n"
           "FROM pax, pax_service_lists, bag_type_list_items \n"
           "WHERE pax.pax_id=pax_service_lists.pax_id AND \n"
           "      pax_service_lists.list_id=bag_type_list_items.list_id AND \n"
           "      pax.grp_id=:id AND \n"
           "      pax_service_lists.transfer_num=:transfer_num AND \n";
  };
  if (way==ByPaxId)
  {
    sql << "SELECT DISTINCT bag_type_list_items.airline \n"
           "FROM pax_service_lists, bag_type_list_items \n"
           "WHERE pax_service_lists.list_id=bag_type_list_items.list_id AND \n"
           "      pax_service_lists.pax_id=:id AND \n"
           "      pax_service_lists.transfer_num=:transfer_num AND \n";
  };
  if (way==ByBagPool)
  {
    sql << "SELECT DISTINCT bag_type_list_items.airline \n"
           "FROM pax_service_lists, bag_type_list_items \n"
           "WHERE pax_service_lists.list_id=bag_type_list_items.list_id AND \n"
           "      pax_service_lists.pax_id=ckin.get_bag_pool_pax_id(:id, :bag_pool_num) AND \n"
           "      pax_service_lists.transfer_num=:transfer_num AND \n";
    QryParams << QParam("bag_pool_num", otInteger, bag_pool_num);
  };

  sql << "      bag_type_list_items.bag_type=:bag_type \n";

  if (category)
  {
    if (way==Unaccomp)
      sql << "      AND grp_service_lists.category=:category \n";
    else
      sql << "      AND pax_service_lists.category=:category \n";
    QryParams << QParam("category", otInteger, (int)category.get());
  }

  QryParams << QParam("id", otInteger, id)
            << QParam("transfer_num", otInteger, transfer_num)
            << QParam("bag_type", otString, bag_type);

  TCachedQuery Qry(sql.str(), QryParams);
  if (bag_type==WeightConcept::REGULAR_BAG_TYPE)
    Qry.get().SetVariable("bag_type", WeightConcept::REGULAR_BAG_TYPE_IN_DB);
  Qry.get().Execute();
  string error;
  if (!Qry.get().Eof && !Qry.get().FieldIsNULL("airline"))
  {
    airline=Qry.get().FieldAsString("airline");
    Qry.get().Next();
    if (!Qry.get().Eof) error="airline duplicate";
  }
  else error="airline not found";

  if (!error.empty())
  {
    ProgTrace(TRACE5, "\n%s", Qry.get().SQLText.SQLText());
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

void TBagTypeKey::getListItem(GetItemWay way, int id, int transfer_num, int bag_pool_num,
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

  ostringstream sql;
  QParams QryParams;
  if (way==Unaccomp)
  {
    sql << "SELECT bag_type_list_items.* \n"
           "FROM grp_service_lists, bag_type_list_items \n"
           "WHERE grp_service_lists.list_id=bag_type_list_items.list_id AND \n"
           "      grp_service_lists.grp_id=:id AND \n"
           "      grp_service_lists.transfer_num=:transfer_num AND \n";
  };
  if (way==ByGrpId)
  {
    sql << "SELECT bag_type_list_items.* \n"
           "FROM pax, pax_service_lists, bag_type_list_items \n"
           "WHERE pax.pax_id=pax_service_lists.pax_id AND \n"
           "      pax_service_lists.list_id=bag_type_list_items.list_id AND \n"
           "      pax.grp_id=:id AND \n"
           "      pax_service_lists.transfer_num=:transfer_num AND \n";
  };
  if (way==ByPaxId)
  {
    sql << "SELECT bag_type_list_items.* \n"
           "FROM pax_service_lists, bag_type_list_items \n"
           "WHERE pax_service_lists.list_id=bag_type_list_items.list_id AND \n"
           "      pax_service_lists.pax_id=:id AND \n"
           "      pax_service_lists.transfer_num=:transfer_num AND \n";
  };
  if (way==ByBagPool)
  {
    sql << "SELECT bag_type_list_items.* \n"
           "FROM pax_service_lists, bag_type_list_items \n"
           "WHERE pax_service_lists.list_id=bag_type_list_items.list_id AND \n"
           "      pax_service_lists.pax_id=ckin.get_bag_pool_pax_id(:id, :bag_pool_num) AND \n"
           "      pax_service_lists.transfer_num=:transfer_num AND \n";
    QryParams << QParam("bag_pool_num", otInteger, bag_pool_num);
  };

  sql << "      bag_type_list_items.bag_type=:bag_type AND \n"
         "      bag_type_list_items.airline=:airline \n";

  if (category)
  {
    if (way==Unaccomp)
      sql << "      AND grp_service_lists.category=:category \n";
    else
      sql << "      AND pax_service_lists.category=:category \n";
    QryParams << QParam("category", otInteger, (int)category.get());
  }
  sql << "      AND rownum<2 \n";

  QryParams << QParam("id", otInteger, id)
            << QParam("transfer_num", otInteger, transfer_num)
            << QParam("bag_type", otString)
            << QParam("airline", otString);

  TCachedQuery Qry(sql.str(), QryParams);
  TBagTypeListKey::toDB(Qry.get());
  if (bag_type==WeightConcept::REGULAR_BAG_TYPE)
    Qry.get().SetVariable("bag_type", WeightConcept::REGULAR_BAG_TYPE_IN_DB);
  Qry.get().Execute();
  if (!Qry.get().Eof && !Qry.get().FieldIsNULL("list_id"))
  {
    list_id=Qry.get().FieldAsInteger("list_id");
    list_item=TBagTypeListItem();
    list_item.get().fromDB(Qry.get());
  }

  if (list_id==ASTRA::NoExists)
  {
    ProgTrace(TRACE5, "\n%s", Qry.get().SQLText.SQLText());
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

void TBagTypeKey::getListItem()
{
  list_item=boost::none;
  if (list_id==ASTRA::NoExists) return;

  TCachedQuery Qry("SELECT * "
                   "FROM bag_type_list_items "
                   "WHERE bag_type_list_items.list_id=:list_id AND "
                   "      bag_type_list_items.bag_type=:bag_type AND "
                   "      bag_type_list_items.airline=:airline",
                   QParams() << QParam("list_id", otInteger, list_id)
                             << QParam("bag_type", otString)
                             << QParam("airline", otString));
  TBagTypeListKey::toDB(Qry.get());
  if (bag_type==WeightConcept::REGULAR_BAG_TYPE)
    Qry.get().SetVariable("bag_type", WeightConcept::REGULAR_BAG_TYPE_IN_DB);

  Qry.get().Execute();
  if (Qry.get().Eof)
    throw Exception("%s: item not found (%s)",  __FUNCTION__,  traceStr().c_str());
  list_item=TBagTypeListItem();
  list_item.get().fromDB(Qry.get());
}

TBagTypeListItem& TBagTypeListItem::fromDB(TQuery &Qry)
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

TBagTypeListKey& TBagTypeListKey::fromDB(TQuery &Qry)
{
  clear();
  bag_type=Qry.FieldAsString("bag_type");
  airline=Qry.FieldAsString("airline");
  return *this;
}

TBagTypeKey& TBagTypeKey::fromDB(TQuery &Qry)
{
  clear();
  TBagTypeListKey::fromDB(Qry);
  if (!Qry.FieldIsNULL("list_id"))
    list_id=Qry.FieldAsInteger("list_id");
  return *this;
}

TBagTypeKey& TBagTypeKey::fromDBcompatible(TQuery &Qry)
{
  clear();
  bag_type=BagTypeFromDB(Qry);
  airline=Qry.FieldAsString("airline");
  if (!Qry.FieldIsNULL("list_id"))
    list_id=Qry.FieldAsInteger("list_id");
  return *this;
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
  TQuery Qry(&OraSession);
  Qry.Clear();
  if (only_visible)
    Qry.SQLText = "SELECT * FROM bag_type_list_items WHERE list_id=:list_id AND visible<>0";
  else
    Qry.SQLText = "SELECT * FROM bag_type_list_items WHERE list_id=:list_id";
  Qry.CreateVariable( "list_id", otInteger, list_id );
  Qry.Execute();
  for ( ;!Qry.Eof; Qry.Next())
  {
    TBagTypeListItem item;
    item.fromDB(Qry);
    if (insert(make_pair(item, item)).second) update_stat(item);
  }
}

void TBagTypeList::toDB(int list_id) const
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "INSERT INTO bag_type_list_items(list_id, bag_type, airline, category, name, name_lat, descr, descr_lat, visible) "
    "VALUES(:list_id, :bag_type, :airline, :category, :name, :name_lat, :descr, :descr_lat, :visible)";
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

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText = "SELECT id FROM service_lists WHERE crc=:crc AND rfisc_used=:rfisc_used";
  Qry.CreateVariable( "rfisc_used", otInteger, (int)false );
  Qry.CreateVariable( "crc", otInteger, crc_tmp );
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next())
  {
    int list_id=Qry.FieldAsInteger("id");
    TBagTypeList list;
    list.fromDB(list_id);
    if (*this==list) return list_id;
  };

  Qry.Clear();
  Qry.SQLText =
      "BEGIN "
      "  SELECT service_lists__seq.nextval INTO :id FROM dual; "
      "  INSERT INTO service_lists(id, crc, rfisc_used, time_create) VALUES(:id, :crc, :rfisc_used, SYSTEM.UTCSYSDATE); "
      "END;";
  Qry.CreateVariable("rfisc_used", otInteger, (int)false);
  Qry.CreateVariable("id", otInteger, FNull);
  Qry.CreateVariable("crc", otInteger, crc_tmp);
  Qry.Execute();
  int list_id=Qry.GetVariableAsInteger("id");
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
      "       bag_unaccomp.pr_additional "
      "FROM bag_types, bag_unaccomp "
      "WHERE bag_types.code=bag_unaccomp.code AND bag_unaccomp.pr_unaccomp<>0 AND "
      "      (bag_unaccomp.airline IS NULL OR bag_unaccomp.airline=:airline) AND "
      "      (bag_unaccomp.airp IS NULL OR bag_unaccomp.airp=:airp) AND "
      "       bag_types.code<>99 "
      "ORDER BY DECODE(bag_unaccomp.airline,NULL,0,2) + "
      "         DECODE(bag_unaccomp.airp,NULL,0,1) DESC";
    Qry.CreateVariable("airline", otString, airline);
    Qry.CreateVariable("airp", otString, airp);
  };
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    TBagTypeListItem item;
    item.bag_type=Qry.FieldAsString("code_str");
    item.airline=Qry.FieldAsString("airline");
    item.category=Qry.FieldAsInteger("pr_additional")!=0?
                    TServiceCategory::BothWithOrigInfo:
                    TServiceCategory::Both;
    item.name=Qry.FieldAsString("name");
    item.name_lat=Qry.FieldAsString("name_lat");
    item.descr=Qry.FieldAsString("descr");
    item.descr_lat=Qry.FieldAsString("descr_lat");
    item.visible=true;

    if (item.name_lat.empty()) item.name_lat=item.name;
    if (item.descr_lat.empty()) item.descr_lat=item.descr;
    if (item.descr.size()>150) item.descr.erase(150);
    if (item.descr_lat.size()>150)item.descr_lat.erase(150);

    insert(make_pair(item, item));
  };

  TBagTypeListItem item;
  item.airline=airline;
  item.bag_type=WeightConcept::REGULAR_BAG_TYPE;
  item.category=TServiceCategory::Both;
  item.name=getLocaleText(WeightConcept::REGULAR_BAG_NAME, LANG_RU);
  item.name_lat=getLocaleText(WeightConcept::REGULAR_BAG_NAME, LANG_EN);
  item.visible=!is_unaccomp;
  insert(make_pair(item, item));
}

void TBagTypeList::createForInboundTrferFromBTM(const std::string &airline)
{
  if (airline.empty())
    throw Exception("TBagTypeList::create: airline.empty()");
  clear();
  TBagTypeListItem item;
  item.airline=airline;
  item.bag_type=WeightConcept::REGULAR_BAG_TYPE;
  item.category=TServiceCategory::Both;
  item.name=getLocaleText(WeightConcept::REGULAR_BAG_NAME, LANG_RU);
  item.name_lat=getLocaleText(WeightConcept::REGULAR_BAG_NAME, LANG_EN);
  item.visible=false;
  insert(make_pair(item, item));
}

void TBagTypeList::update_stat(const TBagTypeListItem &item)
{
  airline_stat.add(item.airline);
  category_stat.add(item.category);
  priority_stat.add(item.priority);
}

namespace WeightConcept
{

const TBagTypeListKey& OldTrferBagType()
{
  static TBagTypeListKey result(OLD_TRFER_BAG_TYPE, "");
  return result;
}

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

TNormItem& TNormItem::fromDB(TQuery &Qry)
{
  clear();
  norm_type=DecodeBagNormType(Qry.FieldAsString("norm_type"));
  if (!Qry.FieldIsNULL("amount"))
    amount=Qry.FieldAsInteger("amount");
  if (!Qry.FieldIsNULL("weight"))
    weight=Qry.FieldAsInteger("weight");
  if (!Qry.FieldIsNULL("per_unit"))
    per_unit=(int)(Qry.FieldAsInteger("per_unit")!=0);
  return *this;
};

std::string TNormItem::str(const std::string& lang) const
{
  if (empty()) return "";
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
  if (empty()) return;

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

const TPaxNormItem& TPaxNormItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  NewTextChild(node,"bag_type",bag_type222);
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
};

TPaxNormItem& TPaxNormItem::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  bag_type222=BagTypeFromXML(NodeAsStringFast("bag_type",node2));
  if (!NodeIsNULLFast("norm_id",node2))
  {
    norm_id=NodeAsIntegerFast("norm_id",node2);
    norm_trfer=NodeAsIntegerFast("norm_trfer",node2,0)!=0;
  };
  return *this;
};

const TPaxNormItem& TPaxNormItem::toDB(TQuery &Qry) const
{
  BagTypeToDB(Qry, bag_type222, "TPaxNormItem::toDB");
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
  if (handmade!=ASTRA::NoExists)
    Qry.SetVariable("handmade",handmade);
  else
    Qry.SetVariable("handmade",FNull);
  return *this;
};

TPaxNormItem& TPaxNormItem::fromDB(TQuery &Qry)
{
  clear();
  bag_type222=BagTypeFromDB(Qry);
  if (!Qry.FieldIsNULL("norm_id"))
  {
    norm_id=Qry.FieldAsInteger("norm_id");
    norm_trfer=Qry.FieldAsInteger("norm_trfer")!=0;
  };
  handmade=(int)(Qry.FieldAsInteger("handmade")!=0);
  return *this;
};

bool PaxNormsFromDB(TDateTime part_key, int pax_id, list< pair<TPaxNormItem, TNormItem> > &norms)
{
  norms.clear();
  const char* sql=
      "SELECT pax_norms.bag_type, pax_norms.norm_id, pax_norms.norm_trfer, pax_norms.handmade, "
      "       bag_norms.norm_type, bag_norms.amount, bag_norms.weight, bag_norms.per_unit "
      "FROM pax_norms,bag_norms "
      "WHERE pax_norms.norm_id=bag_norms.id(+) AND pax_norms.pax_id=:pax_id ";
  const char* sql_arx=
      "SELECT arx_pax_norms.bag_type, arx_pax_norms.norm_id, arx_pax_norms.norm_trfer, arx_pax_norms.handmade, "
      "       bag_norms.norm_type, bag_norms.amount, bag_norms.weight, bag_norms.per_unit "
      "FROM arx_pax_norms,bag_norms "
      "WHERE arx_pax_norms.norm_id=bag_norms.id(+) AND "
      "      arx_pax_norms.part_key=:part_key AND "
      "      arx_pax_norms.pax_id=:pax_id "
      "UNION "
      "SELECT arx_pax_norms.bag_type, arx_pax_norms.norm_id, arx_pax_norms.norm_trfer, arx_pax_norms.handmade, "
      "       arx_bag_norms.norm_type, arx_bag_norms.amount, arx_bag_norms.weight, arx_bag_norms.per_unit "
      "FROM arx_pax_norms,arx_bag_norms "
      "WHERE arx_pax_norms.norm_id=arx_bag_norms.id(+) AND "
      "      arx_pax_norms.part_key=:part_key AND "
      "      arx_pax_norms.pax_id=:pax_id ";
  const char *result_sql = NULL;
  QParams QryParams;
  if (part_key!=ASTRA::NoExists)
  {
    result_sql = sql_arx;
    QryParams << QParam("part_key", otDate, part_key);
  }
  else
    result_sql = sql;
  QryParams << QParam("pax_id", otInteger, pax_id);
  TCachedQuery NormQry(result_sql, QryParams);
  NormQry.get().Execute();
  for(;!NormQry.get().Eof;NormQry.get().Next())
  {
    norms.push_back( make_pair(TPaxNormItem().fromDB(NormQry.get()),
                               TNormItem().fromDB(NormQry.get())) );
  };

  return !norms.empty();
};

bool GrpNormsFromDB(TDateTime part_key, int grp_id, list< pair<TPaxNormItem, TNormItem> > &norms)
{
  norms.clear();
  const char* sql=
      "SELECT grp_norms.bag_type, grp_norms.norm_id, grp_norms.norm_trfer, grp_norms.handmade, "
      "       bag_norms.norm_type, bag_norms.amount, bag_norms.weight, bag_norms.per_unit "
      "FROM grp_norms,bag_norms "
      "WHERE grp_norms.norm_id=bag_norms.id(+) AND grp_norms.grp_id=:grp_id ";
  const char* sql_arx=
      "SELECT arx_grp_norms.bag_type, arx_grp_norms.norm_id, arx_grp_norms.norm_trfer, arx_grp_norms.handmade, "
      "       bag_norms.norm_type, bag_norms.amount, bag_norms.weight, bag_norms.per_unit "
      "FROM arx_grp_norms,bag_norms "
      "WHERE arx_grp_norms.norm_id=bag_norms.id(+) AND "
      "      arx_grp_norms.part_key=:part_key AND "
      "      arx_grp_norms.grp_id=:grp_id "
      "UNION "
      "SELECT arx_grp_norms.bag_type, arx_grp_norms.norm_id, arx_grp_norms.norm_trfer, arx_grp_norms.handmade, "
      "       arx_bag_norms.norm_type, arx_bag_norms.amount, arx_bag_norms.weight, arx_bag_norms.per_unit "
      "FROM arx_grp_norms,arx_bag_norms "
      "WHERE arx_grp_norms.norm_id=arx_bag_norms.id(+) AND "
      "      arx_grp_norms.part_key=:part_key AND "
      "      arx_grp_norms.grp_id=:grp_id ";
  const char *result_sql = NULL;
  QParams QryParams;
  if (part_key!=ASTRA::NoExists)
  {
    result_sql = sql_arx;
    QryParams << QParam("part_key", otDate, part_key);
  }
  else
    result_sql = sql;
  QryParams << QParam("grp_id", otInteger, grp_id);
  TCachedQuery NormQry(result_sql, QryParams);
  NormQry.get().Execute();
  for(;!NormQry.get().Eof;NormQry.get().Next())
  {
    norms.push_back( make_pair(TPaxNormItem().fromDB(NormQry.get()),
                               TNormItem().fromDB(NormQry.get())) );
  };

  return !norms.empty();
};

void NormsToXML(const list< pair<TPaxNormItem, TNormItem> > &norms, bool groupBagTrferExists, xmlNodePtr node)
{
  if (node==NULL) return;

  xmlNodePtr normsNode=NewTextChild(node,"norms");
  for(list< pair<TPaxNormItem, TNormItem> >::const_iterator i=norms.begin();i!=norms.end();++i)
  {
    xmlNodePtr normNode=NewTextChild(normsNode,"norm");
    i->first.toXML(normNode);
    i->second.toXML(normNode);
  };

  if (!TReqInfo::Instance()->desk.compatible(PIECE_CONCEPT_VERSION) && groupBagTrferExists)
  {
    //добавим неопределенную норму по 99 багажу для старых терминалов
    xmlNodePtr normNode=NewTextChild(normsNode,"norm");
    TPaxNormItem paxNormItem;
    paxNormItem.bag_type222=OLD_TRFER_BAG_TYPE;
    paxNormItem.norm_id=1000000000;
    paxNormItem.toXML(normNode);
    TNormItem normItem;
    normItem.norm_type=ASTRA::bntFree;
    normItem.toXML(normNode);
  };
};

void PaxNormsToDB(int pax_id, const boost::optional< list<TPaxNormItem> > &norms)
{
  if (!norms) return;
  TQuery NormQry(&OraSession);
  NormQry.Clear();
  NormQry.SQLText="DELETE FROM pax_norms WHERE pax_id=:pax_id";
  NormQry.CreateVariable("pax_id",otInteger,pax_id);
  NormQry.Execute();
  NormQry.SQLText=
    "INSERT INTO pax_norms(pax_id,bag_type,norm_id,norm_trfer,handmade) "
    "VALUES(:pax_id,:bag_type,:norm_id,:norm_trfer,:handmade)";
  NormQry.DeclareVariable("bag_type",otInteger);
  NormQry.DeclareVariable("norm_id",otInteger);
  NormQry.DeclareVariable("norm_trfer",otInteger);
  NormQry.DeclareVariable("handmade",otInteger);
  for(list<TPaxNormItem>::const_iterator n=norms.get().begin(); n!=norms.get().end(); ++n)
  {
    n->toDB(NormQry);
    NormQry.Execute();
  };
};

void GrpNormsToDB(int grp_id, const boost::optional< list<TPaxNormItem> > &norms)
{
  if (!norms) return;
  TQuery NormQry(&OraSession);
  NormQry.Clear();
  NormQry.SQLText="DELETE FROM grp_norms WHERE grp_id=:grp_id";
  NormQry.CreateVariable("grp_id",otInteger,grp_id);
  NormQry.Execute();
  NormQry.SQLText=
    "INSERT INTO grp_norms(grp_id,bag_type,norm_id,norm_trfer,handmade) "
    "VALUES(:grp_id,:bag_type,:norm_id,:norm_trfer,:handmade)";
  NormQry.DeclareVariable("bag_type",otInteger);
  NormQry.DeclareVariable("norm_id",otInteger);
  NormQry.DeclareVariable("norm_trfer",otInteger);
  NormQry.DeclareVariable("handmade",otInteger);
  for(list<TPaxNormItem>::const_iterator n=norms.get().begin(); n!=norms.get().end(); ++n)
  {
    n->toDB(NormQry);
    NormQry.Execute();
  };
};

void ConvertNormsList(const std::list< std::pair<TPaxNormItem, TNormItem> > &norms,
                      std::map<std::string, TNormItem> &result)
{
  result.clear();
  std::list< std::pair<WeightConcept::TPaxNormItem, WeightConcept::TNormItem> >::const_iterator i=norms.begin();
  for(; i!=norms.end(); ++i)
  {
    if (i->second.empty()) continue;
    result.insert(make_pair(i->first.bag_type222, i->second));
  };
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

const TPaidBagItem& TPaidBagItem::toDB(TQuery &Qry) const
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
  if (handmade!=ASTRA::NoExists)
    Qry.SetVariable("handmade",handmade);
  else
    Qry.SetVariable("handmade",FNull);
  return *this;
};

TPaidBagItem& TPaidBagItem::fromDB(TQuery &Qry)
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
  handmade=(int)(Qry.FieldAsInteger("handmade")!=0);
  return *this;
};

int TPaidBagItem::priority() const
{
  if (list_item)
  {
    const TServiceCategory::Enum &cat=list_item.get().category;
    if (cat==TServiceCategory::Baggage ||
        cat==TServiceCategory::Both ||
        cat==TServiceCategory::BaggageWithOrigInfo ||
        cat==TServiceCategory::BothWithOrigInfo)
      return 2;
    if (cat==TServiceCategory::CarryOn ||
        cat==TServiceCategory::CarryOnWithOrigInfo)
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
    {
      paid.get().push_back(TPaidBagItem().fromXML(node));
      if (!TReqInfo::Instance()->desk.compatible(PIECE_CONCEPT_VERSION))
      {
        if (paid.get().back().key()==OldTrferBagType()) paid.get().pop_back();
      };
    };
    if (!TReqInfo::Instance()->desk.compatible(PAX_SERVICE_VERSION))
      paid.get().getAllListKeys(grp_id, is_unaccomp);
  };
};

void PaidBagToDB(int grp_id, bool is_unaccomp,
                 TPaidBagList &paid)
{
  paid.getAllListItems(grp_id, is_unaccomp);

  TQuery BagQry(&OraSession);
  BagQry.Clear();
  BagQry.SQLText="DELETE FROM paid_bag WHERE grp_id=:grp_id";
  BagQry.CreateVariable("grp_id",otInteger,grp_id);
  BagQry.Execute();
  BagQry.SQLText=
    "INSERT INTO paid_bag(grp_id, list_id, bag_type, bag_type_str, airline, weight, rate_id, rate_trfer, handmade) "
    "VALUES(:grp_id, :list_id, :bag_type, :bag_type_str, :airline, :weight, :rate_id, :rate_trfer, :handmade)";
  BagQry.DeclareVariable("list_id",otInteger);
  BagQry.DeclareVariable("bag_type",otInteger);
  BagQry.DeclareVariable("bag_type_str",otString);
  BagQry.DeclareVariable("airline",otString);
  BagQry.DeclareVariable("weight",otInteger);
  BagQry.DeclareVariable("rate_id",otInteger);
  BagQry.DeclareVariable("rate_trfer",otInteger);
  BagQry.DeclareVariable("handmade",otInteger);
  int excess=0;
  for(TPaidBagList::iterator i=paid.begin(); i!=paid.end(); ++i)
  {
    excess+=i->weight;
    i->toDB(BagQry);
    BagQry.Execute();
  };

  BagQry.Clear();
  BagQry.SQLText=
    "UPDATE pax_grp "
    "SET excess_wt=:excess, excess=DECODE(NVL(piece_concept,0), 0, :excess, excess) "
    "WHERE grp_id=:grp_id";
  BagQry.CreateVariable("grp_id", otInteger, grp_id);
  BagQry.CreateVariable("excess", otInteger, excess);
  BagQry.Execute();
};

void PaidBagFromDB(TDateTime part_key, int grp_id, TPaidBagList &paid)
{
  paid.clear();
  const char* sql=
      "SELECT paid_bag.*, bag_rates.rate, bag_rates.rate_cur "
      "FROM paid_bag,bag_rates "
      "WHERE paid_bag.rate_id=bag_rates.id(+) AND paid_bag.grp_id=:grp_id";
  const char* sql_arx=
      "SELECT arx_paid_bag.bag_type,arx_paid_bag.weight,arx_paid_bag.handmade, "
      "       arx_paid_bag.rate_id,bag_rates.rate,bag_rates.rate_cur,arx_paid_bag.rate_trfer "
      "FROM arx_paid_bag,bag_rates "
      "WHERE arx_paid_bag.rate_id=bag_rates.id(+) AND "
      "      arx_paid_bag.part_key=:part_key AND "
      "      arx_paid_bag.grp_id=:grp_id "
      "UNION "
      "SELECT arx_paid_bag.bag_type,arx_paid_bag.weight,arx_paid_bag.handmade, "
      "       arx_paid_bag.rate_id,arx_bag_rates.rate,arx_bag_rates.rate_cur,arx_paid_bag.rate_trfer "
      "FROM arx_paid_bag,arx_bag_rates "
      "WHERE arx_paid_bag.rate_id=arx_bag_rates.id(+) AND "
      "      arx_paid_bag.part_key=:part_key AND "
      "      arx_paid_bag.grp_id=:grp_id ";
  const char *result_sql = NULL;
  QParams QryParams;
  if (part_key!=ASTRA::NoExists)
  {
    result_sql = sql_arx;
    QryParams << QParam("part_key", otDate, part_key);
  }
  else
    result_sql = sql;
  QryParams << QParam("grp_id", otInteger, grp_id);
  TCachedQuery BagQry(result_sql, QryParams);
  BagQry.get().Execute();
  for(;!BagQry.get().Eof;BagQry.get().Next())
    paid.push_back(TPaidBagItem().fromDB(BagQry.get()));
}

void PaidBagToXML(const TPaidBagList &paid, bool groupBagTrferExists, xmlNodePtr paidbagNode)
{
  if (paidbagNode==NULL) return;

  xmlNodePtr node=NewTextChild(paidbagNode,"paid_bags");
  for(TPaidBagList::const_iterator i=paid.begin(); i!=paid.end(); ++i)
  {
    xmlNodePtr paidBagNode=NewTextChild(node,"paid_bag");
    i->toXML(paidBagNode);
  };

  if (!TReqInfo::Instance()->desk.compatible(PIECE_CONCEPT_VERSION) && groupBagTrferExists)
  {
    //добавим нулевой платный 99 багаж для старых терминалов
    xmlNodePtr paidBagNode=NewTextChild(node,"paid_bag");
    TPaidBagItem item;
    item.key(OldTrferBagType());
    item.weight=0;
    item.handmade=false;
    item.toXML(paidBagNode);
  };
}

std::string GetCurrSegBagAirline(int grp_id)
{
  TCachedQuery Qry("SELECT DECODE(pax_grp.pr_mark_norms, 0, points.airline, mark_trips.airline) AS airline "
                   "FROM pax_grp, mark_trips, points "
                   "WHERE pax_grp.point_dep=points.point_id AND "
                   "      pax_grp.point_id_mark=mark_trips.point_id AND "
                   "      pax_grp.grp_id=:id",
                   QParams() << QParam("id", otInteger, grp_id));
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


