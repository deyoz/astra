#include "rfisc.h"
#include <boost/crc.hpp>
#include "qrys.h"
#include "term_version.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

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

const TEmdTypes& EmdTypes()
{
  static TEmdTypes emdTypes;
  return emdTypes;
}

std::ostream & operator <<(std::ostream &os, TServiceType::Enum const &value)
{
  os << ServiceTypesView().encode(value);
  return os;
}

boost::optional<bool> TRFISCListItem::carry_on() const
{
  if (service_type==TServiceType::BaggageCharge ||
      service_type==TServiceType::BaggagePrepaid)
  {
    return (grp=="BG"/*(Багаж)*/&& subgrp=="CY"/*Ручная кладь*/) ||
           (grp=="PT"/*(Животные)*/&& subgrp=="PC"/*(В кабине)*/);
  }
  return boost::none;
}

TServiceCategory::Enum TRFISCListItem::calc_category() const
{
  boost::optional<bool> carry_on_tmp=carry_on();
  return carry_on_tmp?(carry_on_tmp.get()?TServiceCategory::CarryOn:TServiceCategory::Baggage):TServiceCategory::Other;
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

const TRFISCKey& TRFISCKey::toSirenaXML(xmlNodePtr node, const std::string &lang) const
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

const TRFISCListKey& TRFISCListKey::toSirenaXML(xmlNodePtr node, const std::string &lang) const
{
  if (node==NULL) return *this;

  SetProp(node, "company", airlineToXML(airline, lang));
  SetProp(node, "service_type", ServiceTypes().encode(service_type));
  SetProp(node, "rfisc", RFISC);
  return *this;
}

const TRFISCListItem& TRFISCListItem::toSirenaXML(xmlNodePtr node, const std::string &lang) const
{
  if (node==NULL) return *this;

  TRFISCListKey::toSirenaXML(node, lang);
  SetProp(node, "rfic", RFIC, "");
  SetProp(node, "emd_type", emd_type, "");

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
      if (string((char*)nameNode->name)!="name") continue;
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
      if (string((char*)descrNode->name)!="description") continue;
      if (i>=2) throw Exception("Excess <description>");
      string &d=i==0?descr1:descr2;
      d=NodeAsString(descrNode);
      if (d.size()>2) throw Exception("Wrong <description> '%s'", d.c_str());
      i++;
    };

    category=calc_category();
    visible=true;
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

const TRFISCListItem& TRFISCListItem::toDB(TQuery &Qry, bool old_version) const
{
  Qry.SetVariable("rfic", RFIC);
  Qry.SetVariable("rfisc", RFISC);
  Qry.SetVariable("service_type", ServiceTypes().encode(service_type));
  if (old_version)
    Qry.SetVariable("emd_type", EmdTypes().decode(emd_type));
  else
    Qry.SetVariable("emd_type", emd_type);
  Qry.SetVariable("name", name);
  Qry.SetVariable("name_lat", name_lat);
  if (old_version)
  {
    carry_on()?Qry.SetVariable("pr_cabin", (int)carry_on().get()):
               Qry.SetVariable("pr_cabin", FNull);
  }
  else
  {
    Qry.SetVariable("airline", airline);
    Qry.SetVariable("grp", grp);
    Qry.SetVariable("subgrp", subgrp);
    Qry.SetVariable("descr1", descr1);
    Qry.SetVariable("descr2", descr2);
    Qry.SetVariable("category", (int)category);
    Qry.SetVariable("visible", (int)visible);
  }
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

  return list_item.get().carry_on();
}

void TRFISCKey::getListKey(GetItemWay way, int id, int transfer_num, int bag_pool_num,
                           boost::optional<TServiceCategory::Enum> category,
                           const std::string &where)
{
  if (!airline.empty()) return;
  ostringstream sql;
  QParams QryParams;
  if (way==Unaccomp)
  {
    sql << "SELECT DISTINCT rfisc_list_items.airline \n"
           "FROM grp_service_lists, rfisc_list_items \n"
           "WHERE grp_service_lists.list_id=rfisc_list_items.list_id AND \n"
           "      grp_service_lists.grp_id=:id AND \n"
           "      grp_service_lists.transfer_num=:transfer_num AND \n";
  };
  if (way==ByGrpId)
  {
    sql << "SELECT DISTINCT rfisc_list_items.airline \n"
           "FROM pax, pax_service_lists, rfisc_list_items \n"
           "WHERE pax.pax_id=pax_service_lists.pax_id AND \n"
           "      pax_service_lists.list_id=rfisc_list_items.list_id AND \n"
           "      pax.grp_id=:id AND \n"
           "      pax_service_lists.transfer_num=:transfer_num AND \n";
  };
  if (way==ByPaxId)
  {
    sql << "SELECT DISTINCT rfisc_list_items.airline \n"
           "FROM pax_service_lists, rfisc_list_items \n"
           "WHERE pax_service_lists.list_id=rfisc_list_items.list_id AND \n"
           "      pax_service_lists.pax_id=:id AND \n"
           "      pax_service_lists.transfer_num=:transfer_num AND \n";
  };
  if (way==ByBagPool)
  {
    sql << "SELECT DISTINCT rfisc_list_items.airline \n"
           "FROM pax_service_lists, rfisc_list_items \n"
           "WHERE pax_service_lists.list_id=rfisc_list_items.list_id AND \n"
           "      pax_service_lists.pax_id=ckin.get_bag_pool_pax_id(:id, :bag_pool_num) AND \n"
           "      pax_service_lists.transfer_num=:transfer_num AND \n";
    QryParams << QParam("bag_pool_num", otInteger, bag_pool_num);
  };

  sql << "      rfisc_list_items.rfisc=:rfisc AND \n"
         "      rfisc_list_items.service_type=:service_type \n";

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
            << QParam("rfisc", otString, RFISC)
            << QParam("service_type", otString, ServiceTypes().encode(service_type));

  TCachedQuery Qry(sql.str(), QryParams);
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

bool TRFISCKey::getPseudoListIdForInboundTrfer(int grp_id)
{
  list_id=ASTRA::NoExists;
  list_item=boost::none;
  TCachedQuery Qry("SELECT points.airline AS oper_airline, mark_trips.airline AS mark_airline "
                   "FROM pax_grp, mark_trips, points "
                   "WHERE pax_grp.point_dep=points.point_id AND "
                   "      pax_grp.point_id_mark=mark_trips.point_id AND "
                   "      pax_grp.grp_id=:id",
                   QParams() << QParam("id", otInteger, grp_id));
  Qry.get().Execute();
  if (Qry.get().Eof) return false;
  TRFISCListKey tmp_key=*this;

  for(int pass=0; pass<3; pass++)
  {
    tmp_key.airline=pass<2?(Qry.get().FieldAsString(pass==0?"oper_airline":"mark_airline")):"";
    tmp_key.service_type=TServiceType::BaggageCharge;

    ostringstream sql;
    sql << "SELECT pax_grp.grp_id, bag_types_lists.airline "
           "FROM grp_rfisc_lists, bag_types_lists, pax_grp "
           "WHERE grp_rfisc_lists.list_id=pax_grp.bag_types_id AND "
           "      bag_types_lists.id=pax_grp.bag_types_id AND "
           "      grp_rfisc_lists.rfisc=:rfisc AND "
           "      grp_rfisc_lists.service_type=:service_type AND "
        << (pass<2?
           "      bag_types_lists.airline=:airline AND ":
           "      :airline IS NULL AND ")
        << "      rownum<2";
    TCachedQuery Qry2(sql.str(),
                      QParams() << QParam("rfisc", otString)
                                << QParam("service_type", otString)
                                << QParam("airline", otString));
    tmp_key.toDB(Qry2.get());
    Qry2.get().Execute();
    if (!Qry2.get().Eof && !Qry2.get().FieldIsNULL("grp_id"))
    {
      airline=Qry2.get().FieldAsString("airline");
      service_type=tmp_key.service_type;
      list_id=-Qry2.get().FieldAsInteger("grp_id");
      return true;
    };
  };
  ProgError(STDLOG, "%s: %s", __FUNCTION__, tmp_key.str(LANG_EN).c_str());
  return false;
}


void TRFISCKey::getListItemInboundTrferTmp(const std::string &where) //!!! потом удалить
{
  if (list_item) return;
  if (list_id!=ASTRA::NoExists)
  {
    getListItem();
    return;
  };

  if (airline.empty())
    throw Exception("%s: %s: airline.empty() (%s)",
                    where.c_str(),
                    __FUNCTION__,
                    traceStr().c_str());

  TCachedQuery Qry("SELECT MAX(list_id) AS list_id "
                   "FROM grp_rfisc_lists, bag_types_lists "
                   "WHERE grp_rfisc_lists.list_id=bag_types_lists.id AND "
                   "      grp_rfisc_lists.rfisc=:rfisc AND "
                   "      grp_rfisc_lists.service_type=:service_type AND "
                   "      bag_types_lists.airline=:airline",
                   QParams() << QParam("rfisc", otString)
                             << QParam("service_type", otString)
                             << QParam("airline", otString));
  TRFISCListKey::toDB(Qry.get());
  Qry.get().Execute();
  if (!Qry.get().Eof && !Qry.get().FieldIsNULL("list_id"))
  {
    TRFISCList list;
    list.fromDB(Qry.get().FieldAsInteger("list_id"), true);
    list_id=list.toDBAdv(false);
    getListItem();
  };

  if (list_id==ASTRA::NoExists)
  {
    ProgTrace(TRACE5, "\n%s", Qry.get().SQLText.SQLText());
    throw EConvertError("%s: %s: list_id not found (%s)",
                        where.c_str(),
                        __FUNCTION__,
                        traceStr().c_str());
  };
}

void TRFISCKey::getListItem(GetItemWay way, int id, int transfer_num, int bag_pool_num,
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
    sql << "SELECT rfisc_list_items.* \n"
           "FROM grp_service_lists, rfisc_list_items \n"
           "WHERE grp_service_lists.list_id=rfisc_list_items.list_id AND \n"
           "      grp_service_lists.grp_id=:id AND \n"
           "      grp_service_lists.transfer_num=:transfer_num AND \n";
  };
  if (way==ByGrpId)
  {
    sql << "SELECT rfisc_list_items.* \n"
           "FROM pax, pax_service_lists, rfisc_list_items \n"
           "WHERE pax.pax_id=pax_service_lists.pax_id AND \n"
           "      pax_service_lists.list_id=rfisc_list_items.list_id AND \n"
           "      pax.grp_id=:id AND \n"
           "      pax_service_lists.transfer_num=:transfer_num AND \n";
  };
  if (way==ByPaxId)
  {
    sql << "SELECT rfisc_list_items.* \n"
           "FROM pax_service_lists, rfisc_list_items \n"
           "WHERE pax_service_lists.list_id=rfisc_list_items.list_id AND \n"
           "      pax_service_lists.pax_id=:id AND \n"
           "      pax_service_lists.transfer_num=:transfer_num AND \n";
  };
  if (way==ByBagPool)
  {
    sql << "SELECT rfisc_list_items.* \n"
           "FROM pax_service_lists, rfisc_list_items \n"
           "WHERE pax_service_lists.list_id=rfisc_list_items.list_id AND \n"
           "      pax_service_lists.pax_id=ckin.get_bag_pool_pax_id(:id, :bag_pool_num) AND \n"
           "      pax_service_lists.transfer_num=:transfer_num AND \n";
    QryParams << QParam("bag_pool_num", otInteger, bag_pool_num);
  };

  sql << "      rfisc_list_items.rfisc=:rfisc AND \n"
         "      rfisc_list_items.service_type=:service_type AND \n"
         "      rfisc_list_items.airline=:airline \n";

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
            << QParam("rfisc", otString)
            << QParam("service_type", otString)
            << QParam("airline", otString);

  TCachedQuery Qry(sql.str(), QryParams);
  TRFISCListKey::toDB(Qry.get());
  Qry.get().Execute();
  if (!Qry.get().Eof && !Qry.get().FieldIsNULL("list_id"))
  {
    list_id=Qry.get().FieldAsInteger("list_id");
    list_item=TRFISCListItem();
    list_item.get().fromDB(Qry.get(), false);
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

void TRFISCKey::getListItemIfNone()
{
  if (!list_item) getListItem();
}

void TRFISCKey::getListItem()
{
  list_item=boost::none;
  if (list_id==ASTRA::NoExists) return;

  string sql="SELECT * "
             "FROM rfisc_list_items "
             "WHERE rfisc_list_items.list_id=:list_id AND "
             "      rfisc_list_items.rfisc=:rfisc AND "
             "      rfisc_list_items.service_type=:service_type AND "
             "      rfisc_list_items.airline=:airline";

  if (list_id<0) //!!! потом удалить
  {
    sql="SELECT :list_id AS list_id, "
        "       grp_rfisc_lists.rfisc, "
        "       grp_rfisc_lists.service_type, "
        "       bag_types_lists.airline, "
        "       grp_rfisc_lists.rfic, "
        "       grp_rfisc_lists.emd_type, "
        "       DECODE(grp_rfisc_lists.pr_cabin, NULL, NULL, 0, NULL, 'BG') AS grp, "
        "       DECODE(grp_rfisc_lists.pr_cabin, NULL, NULL, 0, NULL, 'CY') AS subgrp, "
        "       grp_rfisc_lists.name, "
        "       grp_rfisc_lists.name_lat, "
        "       NULL AS descr1, NULL AS descr2, "
        "       DECODE(grp_rfisc_lists.pr_cabin, NULL, 0, 0, 1, 2) AS category, "
        "       1 AS visible "
        "FROM grp_rfisc_lists, bag_types_lists, pax_grp "
        "WHERE grp_rfisc_lists.list_id=pax_grp.bag_types_id AND "
        "      bag_types_lists.id=pax_grp.bag_types_id AND "
        "      grp_rfisc_lists.rfisc=:rfisc AND "
        "      grp_rfisc_lists.service_type=:service_type AND "
        "      bag_types_lists.airline=:airline AND "
        "      pax_grp.grp_id=-(:list_id) ";
  }

  TCachedQuery Qry(sql,
                   QParams() << QParam("list_id", otInteger, list_id)
                             << QParam("rfisc", otString)
                             << QParam("service_type", otString)
                             << QParam("airline", otString));
  TRFISCListKey::toDB(Qry.get());
  Qry.get().Execute();
  if (Qry.get().Eof)
    throw Exception("%s: item not found (%s)",  __FUNCTION__,  traceStr().c_str());
  list_item=TRFISCListItem();
  list_item.get().fromDB(Qry.get(), list_id<0);
}

TRFISCListItem& TRFISCListItem::fromDB(TQuery &Qry, bool old_version)
{
  clear();
  TRFISCListKey::fromDB(Qry);
  RFIC=Qry.FieldAsString("rfic");
  if (old_version)
    emd_type=EmdTypes().encode(Qry.FieldAsString("emd_type"));
  else
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
};

TRFISCListKey& TRFISCListKey::fromDB(TQuery &Qry)
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

void TRFISCList::fromSirenaXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) throw Exception("TRFISCListItem::fromSirenaXML: node not defined");
  for(node=node->children; node!=NULL; node=node->next)
  {
    if (string((char*)node->name)!="svc") continue;
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

void TRFISCList::fromDB(int list_id, bool old_version, bool only_visible)
{
  clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  if (old_version)
    Qry.SQLText =
      "SELECT grp_rfisc_lists.*, "
      "       bag_types_lists.airline, "
      "       DECODE(grp_rfisc_lists.pr_cabin, NULL, NULL, 0, NULL, 'BG') AS grp, "
      "       DECODE(grp_rfisc_lists.pr_cabin, NULL, NULL, 0, NULL, 'CY') AS subgrp, "
      "       NULL AS descr1, NULL AS descr2, "
      "       DECODE(grp_rfisc_lists.pr_cabin, NULL, 0, 0, 1, 2) AS category, "
      "       1 AS visible "
      "FROM grp_rfisc_lists, bag_types_lists "
      "WHERE grp_rfisc_lists.list_id=:list_id AND bag_types_lists.id=:list_id";
  else
    if (only_visible)
      Qry.SQLText =
        "SELECT * FROM rfisc_list_items WHERE list_id=:list_id AND visible<>0";
    else
      Qry.SQLText =
        "SELECT * FROM rfisc_list_items WHERE list_id=:list_id";
  Qry.CreateVariable( "list_id", otInteger, list_id );
  Qry.Execute();
  for ( ;!Qry.Eof; Qry.Next())
  {
    TRFISCListItem item;
    item.fromDB(Qry, old_version);
    if (insert(make_pair(TRFISCListKey(item), item)).second) update_stat(item);
  }
}

void TRFISCList::toDB(int list_id, bool old_version, const std::string &baggage_airline) const
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  if (old_version)
    Qry.SQLText =
        "INSERT INTO grp_rfisc_lists(list_id, rfisc, service_type, rfic, emd_type, pr_cabin, name, name_lat) "
        "VALUES(:list_id, :rfisc, :service_type, :rfic, :emd_type, :pr_cabin, :name, :name_lat)";
  else
    Qry.SQLText =
        "INSERT INTO rfisc_list_items(list_id, rfisc, service_type, airline, rfic, emd_type, grp, subgrp, name, name_lat, descr1, descr2, category, visible) "
        "VALUES(:list_id, :rfisc, :service_type, :airline, :rfic, :emd_type, :grp, :subgrp, :name, :name_lat, :descr1, :descr2, :category, :visible)";
  Qry.CreateVariable( "list_id", otInteger, list_id );
  Qry.DeclareVariable( "rfisc", otString );
  Qry.DeclareVariable( "service_type", otString );
  if (old_version)
    Qry.DeclareVariable( "pr_cabin", otInteger );
  else
  {
    Qry.DeclareVariable( "airline", otString );
    Qry.DeclareVariable( "grp", otString );
    Qry.DeclareVariable( "subgrp", otString );
    Qry.DeclareVariable( "descr1", otString );
    Qry.DeclareVariable( "descr2", otString );
    Qry.DeclareVariable( "category", otInteger );
    Qry.DeclareVariable( "visible", otInteger );
  };
  Qry.DeclareVariable( "rfic", otString );
  Qry.DeclareVariable( "emd_type", otString );
  Qry.DeclareVariable( "name", otString );
  Qry.DeclareVariable( "name_lat", otString );
  if (old_version)
  {
    for(TRFISCList::const_iterator i=begin(); i!=end(); ++i)
    {
      if (!included_in_old_version(i->second, baggage_airline)) continue;
      i->second.toDB(Qry, old_version);
      try
      {
        Qry.Execute();
      }
      catch(EOracleError E)
      {
        if (E.Code==1)
          ProgError(STDLOG, "Warning: TRFISCList::toDB: duplicate list_id=%d rfisc=%s", list_id, i->second.RFISC.c_str());
        else
          throw;
      };
    };
  }
  else
  {
    for(TRFISCList::const_iterator i=begin(); i!=end(); ++i)
    {
      i->second.toDB(Qry, old_version);
      Qry.Execute();
    };
  };
}

bool TRFISCList::included_in_old_version(const TRFISCListItem& item, const std::string &baggage_airline) const
{
  if (item.service_type!=TServiceType::BaggageCharge) return false;
  if (!baggage_airline.empty() && item.airline!=baggage_airline)
  {
    //услуга не принадлежит багажу
    TRFISCListKey key=item;
    key.airline=baggage_airline;
    if (find(key)!=end()) return false;
  };
  return true;
}

int TRFISCList::crc(bool old_version, const std::string& baggage_airline) const
{
  if (empty()) throw Exception("TRFISCList::crc: empty list");
  boost::crc_basic<32> crc32( 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true, true );
  crc32.reset();
  ostringstream buf;
  if (old_version)
  {
    for(TRFISCList::const_iterator i=begin(); i!=end(); ++i)
    {
      if (!included_in_old_version(i->second, baggage_airline)) continue;
      buf << i->second.RFISC << ";"
          << i->second.service_type << ";"
          << (baggage_airline.empty()?i->second.airline:baggage_airline) << ";"
          << i->second.category << ";"
          << (i->second.visible?"+":"-") << ";"
          << i->second.name.substr(0, 10) << ";"
          << i->second.name_lat.substr(0, 10) << ";";
    };
    if (buf.str().empty())
      throw Exception("TRFISCList::crc: empty baggage dominant RFISC list");
  }
  else
  {
    for(TRFISCList::const_iterator i=begin(); i!=end(); ++i)
      buf << i->second.RFISC << ";"
          << i->second.service_type << ";"
          << i->second.airline << ";"
          << i->second.category << ";"
          << (i->second.visible?"+":"-") << ";"
          << i->second.name.substr(0, 10) << ";"
          << i->second.name_lat.substr(0, 10) << ";";
  };
  //ProgTrace(TRACE5, "TRFISCList::crc: %s", buf.str().c_str());
  crc32.process_bytes( buf.str().c_str(), buf.str().size() );
  return crc32.checksum();
}

boost::optional<TRFISCListKey> TRFISCList::getBagRFISCListKey(const std::string &RFISC, const bool &carry_on) const
{
  for(TRFISCList::const_iterator i=begin(); i!=end(); ++i)
  {
    const TRFISCListItem &item=i->second;
    if (item.service_type==TServiceType::BaggageCharge && item.carry_on() && item.carry_on().get()==carry_on &&
        (RFISC.empty() || item.RFISC==RFISC))
      return i->first;
  }
  return boost::none;
}

int TRFISCList::toDBAdv(bool old_version) const
{
  string baggage_airline;
  if (old_version)
  {
    boost::optional<TRFISCListKey> key=getBagRFISCListKey("", false);
    if (!key) throw Exception("TRFISCList::toDBAdv: empty baggage dominant RFISC list");
    baggage_airline=key.get().airline;
  }

  int crc_tmp=crc(old_version, baggage_airline);

  TQuery Qry(&OraSession);
  Qry.Clear();
  if (old_version)
    Qry.SQLText = "SELECT id FROM bag_types_lists WHERE crc=:crc AND rownum<=10";
  else
  {
    Qry.SQLText = "SELECT id FROM service_lists WHERE crc=:crc AND rfisc_used=:rfisc_used";
    Qry.CreateVariable( "rfisc_used", otInteger, (int)true );
  };
  Qry.CreateVariable( "crc", otInteger, crc_tmp );
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next())
  {
    int list_id=Qry.FieldAsInteger("id");
    TRFISCList list;
    list.fromDB(list_id, old_version);
    if (equal(list, old_version, baggage_airline)) return list_id;
  };

  Qry.Clear();
  if (old_version)
  {
    Qry.SQLText =
        "BEGIN "
        "  SELECT bag_types_lists__seq.nextval INTO :id FROM dual; "
        "  INSERT INTO bag_types_lists(id, airline, crc) VALUES(:id, :airline, :crc); "
        "END;";
    Qry.CreateVariable("airline", otString, baggage_airline);
  }
  else
  {
    Qry.SQLText =
        "BEGIN "
        "  SELECT service_lists__seq.nextval INTO :id FROM dual; "
        "  INSERT INTO service_lists(id, crc, rfisc_used) VALUES(:id, :crc, :rfisc_used); "
        "END;";
    Qry.CreateVariable("rfisc_used", otInteger, (int)true);
  };
  Qry.CreateVariable("id", otInteger, FNull);
  Qry.CreateVariable("crc", otInteger, crc_tmp);
  Qry.Execute();
  int list_id=Qry.GetVariableAsInteger("id");
  toDB(list_id, old_version, baggage_airline);
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

bool TRFISCList::equal(const TRFISCList &list, bool old_version, const std::string& baggage_airline) const
{
  if (old_version)
  {
    TRFISCList::const_iterator i1=begin();
    TRFISCList::const_iterator i2=list.begin();
    for(;i1!=end() || i2!=list.end();)
    {
      if (i1!=end() && !included_in_old_version(i1->second, baggage_airline))
      {
        ++i1;
        continue;
      };
      if (i2!=list.end() && !list.included_in_old_version(i2->second, baggage_airline))
      {
        ++i2;
        continue;
      };
      if (i1!=end() &&
          i2!=list.end() &&
          i1->second.RFIC==i2->second.RFIC &&
          i1->second.RFISC==i2->second.RFISC &&
          i1->second.service_type==i2->second.service_type &&
          (baggage_airline.empty()?i1->second.airline:baggage_airline)==i2->second.airline &&
          i1->second.emd_type==i2->second.emd_type &&
          i1->second.carry_on()==i2->second.carry_on() &&
          i1->second.name==i2->second.name &&
          i1->second.name_lat==i2->second.name_lat)
      {
        ++i1;
        ++i2;
        continue;
      }
      break;
    }
    return i1==end() && i2==list.end();
  }
  else
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
  rem_code=Qry.FieldAsString("rem_code");
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

string TRFISCBagPropsList::get_rem_code(const std::string &airline, const std::string &rfisc) const
{
  if (rfisc.empty()) return "";
  TRFISCBagPropsList::const_iterator i=find(TRFISCListKey(rfisc, TServiceType::BaggageCharge, airline));
  if (i==end()) return "";
  return i->second.rem_code;
}

string TRFISCListWithProps::get_rem_code(const std::string &rfisc, const bool pr_cabin)
{
  if (rfisc.empty()) return "";
  boost::optional<TRFISCListKey> key=getBagRFISCListKey(rfisc, pr_cabin);
  if (!key) return "";
  return getBagProps().get_rem_code(key.get().airline, rfisc);
}

const TRFISCBagPropsList& TRFISCListWithProps::getBagProps()
{
  if (!bagProps)
  {
    set<string> airlines;
    for(TRFISCListWithProps::const_iterator i=begin(); i!=end(); ++i)
      airlines.insert(i->first.airline);
    bagProps=TRFISCBagPropsList();
    bagProps.get().fromDB(airlines);
  }
  return bagProps.get();
}

void TRFISCListWithProps::setPriority()
{
  if (!bagProps) return;
  for(TRFISCBagPropsList::const_iterator p=bagProps.get().begin(); p!=bagProps.get().end(); ++p)
  {
    if (p->second.priority==ASTRA::NoExists) continue;
    TRFISCListWithProps::iterator i=find(p->first);
    if (i!=end()) i->second.priority=p->second.priority;
  }
  recalc_stat();
}

const TRFISCBagPropsList& TRFISCListWithPropsCache::getBagProps(int list_id)
{
  TRFISCListWithPropsCache::iterator i=find(list_id);
  if (i==end())
  {
    i=insert(make_pair(list_id, TRFISCListWithProps())).first;
    if (i==end())
      throw EXCEPTIONS::Exception("TRFISCListWithPropsCache::getBagProps: i==end() (list_id=%d)", list_id);
    i->second.fromDB(list_id, false);
  };
  return i->second.getBagProps();
}

const TPaxServiceListsKey& TPaxServiceListsKey::toDB(TQuery &Qry) const
{
  TPaxSegKey::toDB(Qry);
  Qry.SetVariable("category", (int)category);
  return *this;
}

TPaxServiceListsKey& TPaxServiceListsKey::fromDB(TQuery &Qry)
{
  clear();
  TPaxSegKey::fromDB(Qry);
  category=TServiceCategory::Enum(Qry.FieldAsInteger("category"));
  return *this;
}

const TPaxServiceListsItem& TPaxServiceListsItem::toDB(TQuery &Qry) const
{
  TPaxServiceListsKey::toDB(Qry);
  list_id==ASTRA::NoExists?Qry.SetVariable("list_id", FNull):
                           Qry.SetVariable("list_id", list_id);
  return *this;
}

TPaxServiceListsItem& TPaxServiceListsItem::fromDB(TQuery &Qry)
{
  clear();
  TPaxServiceListsKey::fromDB(Qry);
  list_id=Qry.FieldIsNULL("list_id")?ASTRA::NoExists:
                                     Qry.FieldAsInteger("list_id");
  return *this;
}

const TPaxServiceListsItem& TPaxServiceListsItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  SetProp(node, "seg_no", trfer_num);
  SetProp(node, "category", (int)category);
  SetProp(node, "list_id", list_id);
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

void TPaxServiceLists::fromDB(int id, bool is_unaccomp, bool emul_nonexistent_lists)
{
  clear();
  TCachedQuery Qry(is_unaccomp?"SELECT grp_service_lists.*, grp_id AS pax_id FROM grp_service_lists WHERE grp_id=:id":
                               "SELECT * FROM pax_service_lists WHERE pax_id=:id",
                   QParams() << QParam("id", otInteger, id));
  Qry.get().Execute();
  for(; !Qry.get().Eof; Qry.get().Next())
    insert(TPaxServiceListsItem().fromDB(Qry.get()));

  if (empty() && emul_nonexistent_lists) //!!!потом удалить
  {
    int grp_id;
    if (!is_unaccomp)
    {
      TCachedQuery Qry("SELECT grp_id FROM pax WHERE pax_id=:id",
                       QParams() << QParam("id", otInteger, id));
      Qry.get().Execute();
      if (Qry.get().Eof) return;
      grp_id=Qry.get().FieldAsInteger("grp_id");
    }
    else
      grp_id=id;
    bool pc=false, wt=false;
    int bag_types_id=ASTRA::NoExists;
    GetBagConceptsCompatible(grp_id, pc, wt, bag_types_id);
    if (pc || wt)
    {
      int max_trfer_num=get_max_tckin_num(grp_id);
      for(int pass=0; pass<2; pass++)
      {
        TPaxServiceListsItem item;
        item.pax_id=id;
        item.category=pass==0?TServiceCategory::Baggage:TServiceCategory::CarryOn;
        item.list_id=-grp_id;
        for(item.trfer_num=0; item.trfer_num<=max_trfer_num; item.trfer_num++)
          insert(item);
      };
    }
  }
}

void TPaxServiceLists::toDB(bool is_unaccomp) const
{
  TCachedQuery Qry(is_unaccomp?"INSERT INTO grp_service_lists(grp_id, transfer_num, category, list_id) "
                               "VALUES(:pax_id, :transfer_num, :category, :list_id)":
                               "INSERT INTO pax_service_lists(pax_id, transfer_num, category, list_id) "
                               "VALUES(:pax_id, :transfer_num, :category, :list_id)",
                   QParams() << QParam("pax_id", otInteger)
                             << QParam("transfer_num", otInteger)
                             << QParam("category", otInteger)
                             << QParam("list_id", otInteger));
  for(TPaxServiceLists::const_iterator i=begin(); i!=end(); ++i)
  {
    i->toDB(Qry.get());
    Qry.get().Execute();
  }
}

void TPaxServiceLists::toXML(int id, bool is_unaccomp, int tckin_seg_count, xmlNodePtr node)
{
  if (node==NULL) return;
  fromDB(id, is_unaccomp, true);
  for(TPaxServiceLists::const_iterator i=begin(); i!=end(); ++i)
  {
    if (i->trfer_num>=tckin_seg_count) continue; //не передаем дополнительные сегменты трансфера
    i->toXML(NewTextChild(node, "service_list"));
  };
}

const TPaxSegRFISCKey& TPaxSegRFISCKey::toSirenaXML(xmlNodePtr node, const std::string &lang) const
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

const TGrpServiceItem& TGrpServiceItem::toDB(TQuery &Qry) const
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

TGrpServiceItem& TGrpServiceItem::fromDB(TQuery &Qry)
{
  clear();
  TPaxSegRFISCKey::fromDB(Qry);
  getListItem();
  service_quantity=Qry.FieldAsInteger("service_quantity");
  return *this;
}

TGrpServiceAutoItem& TGrpServiceAutoItem::fromDB(TQuery &Qry)
{
  clear();
  TPaxSegKey::fromDB(Qry);
  TPaxASVCItem::fromDB(Qry);
  return *this;
}

const TGrpServiceAutoItem& TGrpServiceAutoItem::toDB(TQuery &Qry) const
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


void TGrpServiceList::fromDB(int grp_id, bool without_refused)
{
  clear();
  TCachedQuery Qry(without_refused?
                     "SELECT pax_services.* FROM pax, pax_services "
                     "WHERE pax_services.pax_id=pax.pax_id AND pax.grp_id=:grp_id AND pax.refuse IS NULL":
                     "SELECT pax_services.* FROM pax, pax_services "
                     "WHERE pax_services.pax_id=pax.pax_id AND pax.grp_id=:grp_id",
                   QParams() << QParam("grp_id", otInteger, grp_id));
  Qry.get().Execute();
  for(; !Qry.get().Eof; Qry.get().Next())
    push_back(TGrpServiceItem().fromDB(Qry.get()));
}

void TGrpServiceAutoList::fromDB(int id, bool is_grp_id, bool without_refused)
{
  clear();
  TCachedQuery Qry(is_grp_id?
                   (without_refused?
                      "SELECT pax_services_auto.* FROM pax, pax_services_auto "
                      "WHERE pax_services_auto.pax_id=pax.pax_id AND pax.grp_id=:id AND pax.refuse IS NULL":
                      "SELECT pax_services_auto.* FROM pax, pax_services_auto "
                      "WHERE pax_services_auto.pax_id=pax.pax_id AND pax.grp_id=:id"):
                   (without_refused?
                      "SELECT pax_services_auto.* FROM pax, pax_services_auto "
                      "WHERE pax_services_auto.pax_id=pax.pax_id AND pax.pax_id=:id AND pax.refuse IS NULL":
                      "SELECT * FROM pax_services_auto WHERE pax_id=:id"),
                   QParams() << QParam("id", otInteger, id));
  Qry.get().Execute();
  for(; !Qry.get().Eof; Qry.get().Next())
  {
    TGrpServiceAutoItem item;
    item.fromDB(Qry.get());
    if (!item.isSuitableForAutoCheckin()) continue;
    push_back(item);
  }
}

void TGrpServiceListWithAuto::fromDB(int grp_id, bool without_refused)
{
  TGrpServiceList list1;
  list1.fromDB(grp_id, without_refused);
  for(TGrpServiceList::const_iterator i=list1.begin(); i!=list1.end(); ++i)
    push_back(*i);

  TGrpServiceAutoList list2;
  list2.fromDB(grp_id, true, without_refused);
  for(TGrpServiceAutoList::const_iterator i=list2.begin(); i!=list2.end(); ++i)
    addItem(TGrpServiceItem(*i));
}

void TGrpServiceListWithAuto::addItem(const TGrpServiceItem& item)
{
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

void TPaidRFISCListWithAuto::addItem(const TPaidRFISCItem& item)
{
  TPaidRFISCList::iterator i=find(item);
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

void TPaidRFISCList::fromDB(int id, bool is_grp_id)
{
  clear();
  TCachedQuery Qry(is_grp_id?"SELECT paid_rfisc.* FROM pax, paid_rfisc WHERE paid_rfisc.pax_id=pax.pax_id AND pax.grp_id=:id":
                             "SELECT * FROM paid_rfisc WHERE paid_rfisc.pax_id=:id",
                   QParams() << QParam("id", otInteger, id));
  Qry.get().Execute();
  for(; !Qry.get().Eof; Qry.get().Next())
  {
    TPaidRFISCItem item;
    item.fromDB(Qry.get());
    insert( make_pair(item, item) );
  }

  if (empty()) //!!!потом удалить
  {
    ostringstream sql;
    sql << "SELECT paid_bag_pc.pax_id, "
           "       paid_bag_pc.transfer_num, "
           "       paid_bag_pc.rfisc, "
           "       rfisc_list_items.airline, "
           "       rfisc_list_items.service_type, "
           "       DECODE(rfisc_list_items.airline, NULL, TO_NUMBER(NULL), -pax.grp_id) AS list_id, "
           "       SUM(1) AS service_quantity, "
           "       SUM(DECODE(paid_bag_pc.status,'free',0,1)) AS paid, "
           "       SUM(DECODE(paid_bag_pc.status,'free',0,'paid',0,1)) AS need "
           "FROM paid_bag_pc, pax, "
           "     (SELECT bag_types_lists.airline, grp_rfisc_lists.service_type, grp_rfisc_lists.rfisc "
        << (is_grp_id?
           "      FROM pax_grp, bag_types_lists, grp_rfisc_lists "
           "      WHERE pax_grp.grp_id=:id AND ":
           "      FROM pax, pax_grp, bag_types_lists, grp_rfisc_lists "
           "      WHERE pax.grp_id=pax_grp.grp_id AND "
           "            pax.pax_id=:id AND ")
        << "            pax_grp.bag_types_id=bag_types_lists.id AND "
           "            bag_types_lists.id=grp_rfisc_lists.list_id) rfisc_list_items "
           "WHERE paid_bag_pc.pax_id=pax.pax_id AND "
           "      paid_bag_pc.rfisc=rfisc_list_items.rfisc(+) AND "
        << (is_grp_id?
           "      pax.grp_id=:id ":
           "      pax.pax_id=:id ")
        << "GROUP BY pax.grp_id, "
           "         paid_bag_pc.pax_id, "
           "         paid_bag_pc.transfer_num, "
           "         paid_bag_pc.rfisc, "
           "         rfisc_list_items.airline, "
           "         rfisc_list_items.service_type ";
    TCachedQuery Qry(sql.str(),
                     QParams() << QParam("id", otInteger, id));
    Qry.get().Execute();
    for(; !Qry.get().Eof; Qry.get().Next())
    {
      if (!Qry.get().FieldIsNULL("rfisc") &&
          (Qry.get().FieldIsNULL("airline") || Qry.get().FieldIsNULL("service_type")))
        throw Exception("TPaidRFISCList::fromDB: wrong data (id=%d, is_grp_id=%d, rfisc=%s)",
                        id, (int)is_grp_id, Qry.get().FieldAsString("rfisc"));
      TPaidRFISCItem item;
      item.fromDB(Qry.get());
      insert( make_pair(item, item) );
    };
  };
}

void TPaidRFISCListWithAuto::fromDB(int id, bool is_grp_id)
{
  TPaidRFISCList list1;
  list1.fromDB(id, is_grp_id);
  for(TPaidRFISCList::const_iterator i=list1.begin(); i!=list1.end(); ++i)
    insert(*i);

  TGrpServiceAutoList list2;
  list2.fromDB(id, is_grp_id);
  for(TGrpServiceAutoList::const_iterator i=list2.begin(); i!=list2.end(); ++i)
    addItem(TPaidRFISCItem(*i));
}

void TGrpServiceList::clearDB(int grp_id)
{
  TCachedQuery Qry("DELETE FROM (SELECT * FROM pax, pax_services WHERE pax_services.pax_id=pax.pax_id AND pax.grp_id=:grp_id)",
                   QParams() << QParam("grp_id", otInteger, grp_id));
  Qry.get().Execute();
}

void TGrpServiceAutoList::clearDB(int grp_id)
{
  TCachedQuery Qry("DELETE FROM (SELECT * FROM pax, pax_services_auto WHERE pax_services_auto.pax_id=pax.pax_id AND pax.grp_id=:grp_id)",
                   QParams() << QParam("grp_id", otInteger, grp_id));
  Qry.get().Execute();
}

void TGrpServiceList::toDB(int grp_id) const
{
  clearDB(grp_id);
  TCachedQuery Qry("INSERT INTO pax_services(pax_id, transfer_num, list_id, rfisc, service_type, airline, service_quantity) "
                   "VALUES(:pax_id, :transfer_num, :list_id, :rfisc, :service_type, :airline, :service_quantity)",
                   QParams() << QParam("pax_id", otInteger)
                             << QParam("transfer_num", otInteger)
                             << QParam("list_id", otInteger)
                             << QParam("rfisc", otString)
                             << QParam("service_type", otString)
                             << QParam("airline", otString)
                             << QParam("service_quantity", otInteger));
  for(TGrpServiceList::const_iterator i=begin(); i!=end(); ++i)
  {
    i->toDB(Qry.get());
    Qry.get().Execute();
  }
}

void TGrpServiceAutoList::toDB(int grp_id) const
{
  clearDB(grp_id);
  TCachedQuery Qry("INSERT INTO pax_services_auto(pax_id, transfer_num, rfic, rfisc, service_quantity, ssr_code, service_name, emd_type, emd_no, emd_coupon) "
                   "VALUES(:pax_id, :transfer_num, :rfic, :rfisc, :service_quantity, :ssr_code, :service_name, :emd_type, :emd_no, :emd_coupon)",
                   QParams() << QParam("pax_id", otInteger)
                             << QParam("transfer_num", otInteger)
                             << QParam("rfic", otString)
                             << QParam("rfisc", otString)
                             << QParam("service_quantity", otInteger)
                             << QParam("ssr_code", otString)
                             << QParam("service_name", otString)
                             << QParam("emd_type", otString)
                             << QParam("emd_no", otString)
                             << QParam("emd_coupon", otInteger));
  for(TGrpServiceAutoList::const_iterator i=begin(); i!=end(); ++i)
  {
    i->toDB(Qry.get());
    Qry.get().Execute();
  }
}

void TGrpServiceList::copyDB(int grp_id_src, int grp_id_dest)
{
  clearDB(grp_id_dest);
  TCachedQuery Qry(
    "INSERT INTO pax_services(pax_id, transfer_num, list_id, rfisc, service_type, airline, service_quantity) "
    "SELECT dest.pax_id, "
    "       pax_services.transfer_num+src.seg_no-dest.seg_no, "
    "       pax_services.list_id, "
    "       pax_services.rfisc, "
    "       pax_services.service_type, "
    "       pax_services.airline, "
    "       pax_services.service_quantity "
    "FROM pax_services, "
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
    "WHERE src.tckin_id=dest.tckin_id AND "
    "      src.distance=dest.distance AND "
    "      pax_services.pax_id=src.pax_id AND "
    "      pax_services.transfer_num+src.seg_no-dest.seg_no>=0 ",
    QParams() << QParam("grp_id_src", otInteger, grp_id_src)
              << QParam("grp_id_dest", otInteger, grp_id_dest));
  Qry.get().Execute();
}

void TGrpServiceAutoList::copyDB(int grp_id_src, int grp_id_dest, bool not_clear)
{
  if (!not_clear) clearDB(grp_id_dest);
  TCachedQuery Qry(
    "INSERT INTO pax_services_auto(pax_id, transfer_num, rfic, rfisc, service_quantity, ssr_code, service_name, emd_type, emd_no, emd_coupon) "
    "SELECT dest.pax_id, "
    "       pax_services_auto.transfer_num+src.seg_no-dest.seg_no, "
    "       pax_services_auto.rfic, "
    "       pax_services_auto.rfisc, "
    "       pax_services_auto.service_quantity, "
    "       pax_services_auto.ssr_code, "
    "       pax_services_auto.service_name, "
    "       pax_services_auto.emd_type, "
    "       pax_services_auto.emd_no, "
    "       pax_services_auto.emd_coupon "
    "FROM pax_services_auto, "
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
    "WHERE src.tckin_id=dest.tckin_id AND "
    "      src.distance=dest.distance AND "
    "      pax_services_auto.pax_id=src.pax_id AND "
    "      pax_services_auto.transfer_num+src.seg_no-dest.seg_no>=0 ",
    QParams() << QParam("grp_id_src", otInteger, grp_id_src)
              << QParam("grp_id_dest", otInteger, grp_id_dest));
  Qry.get().Execute();
}

void TGrpServiceListWithAuto::split(int grp_id, TGrpServiceList& list1, TGrpServiceAutoList& list2) const
{
  list1.clear();
  list2.clear();
  TGrpServiceAutoList prior;
  prior.fromDB(grp_id, true);
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

void TPaidRFISCList::clearDB(int grp_id)
{
  TCachedQuery Qry("DELETE FROM (SELECT * FROM pax, paid_rfisc WHERE paid_rfisc.pax_id=pax.pax_id AND pax.grp_id=:grp_id)",
                   QParams() << QParam("grp_id", otInteger, grp_id));
  Qry.get().Execute();
}

void TPaidRFISCList::toDB(int grp_id) const
{
  clearDB(grp_id);
  TCachedQuery Qry("INSERT INTO paid_rfisc(pax_id, transfer_num, list_id, rfisc, service_type, airline, service_quantity, paid, need) "
                   "VALUES(:pax_id, :transfer_num, :list_id, :rfisc, :service_type, :airline, :service_quantity, :paid, :need)",
                   QParams() << QParam("pax_id", otInteger)
                             << QParam("transfer_num", otInteger)
                             << QParam("list_id", otInteger)
                             << QParam("rfisc", otString)
                             << QParam("service_type", otString)
                             << QParam("airline", otString)
                             << QParam("service_quantity", otInteger)
                             << QParam("paid", otInteger)
                             << QParam("need", otInteger));
  for(TPaidRFISCList::const_iterator i=begin(); i!=end(); ++i)
  {
    i->second.toDB(Qry.get());
    Qry.get().Execute();
  }

  updateExcess(grp_id);
}

void TPaidRFISCList::updateExcess(int grp_id)
{
  TCachedQuery Qry(
    "BEGIN "
    "  SELECT NVL(SUM(paid_rfisc.paid),0) INTO :excess "
    "  FROM paid_rfisc, pax "
    "  WHERE paid_rfisc.pax_id=pax.pax_id AND "
    "        pax.grp_id=:grp_id AND "
    "        paid_rfisc.transfer_num=0 AND "
    "        paid_rfisc.paid>0; "
    "  UPDATE pax_grp "
    "  SET excess_pc=:excess, excess=DECODE(NVL(piece_concept,0), 0, excess, :excess) "
    "  WHERE grp_id=:grp_id; "
    "END; ",
    QParams() << QParam("grp_id", otInteger, grp_id)
              << QParam("excess", otInteger, FNull));
  Qry.get().Execute();
}

void TPaidRFISCList::copyDB(int grp_id_src, int grp_id_dest)
{
  clearDB(grp_id_dest);
  TCachedQuery Qry(
    "INSERT INTO paid_rfisc(pax_id, transfer_num, list_id, rfisc, service_type, airline, service_quantity, paid, need) "
    "SELECT dest.pax_id, "
    "       paid_rfisc.transfer_num+src.seg_no-dest.seg_no, "
    "       paid_rfisc.list_id, "
    "       paid_rfisc.rfisc, "
    "       paid_rfisc.service_type, "
    "       paid_rfisc.airline, "
    "       paid_rfisc.service_quantity, "
    "       paid_rfisc.paid, "
    "       paid_rfisc.need "
    "FROM paid_rfisc, "
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
    "WHERE src.tckin_id=dest.tckin_id AND "
    "      src.distance=dest.distance AND "
    "      paid_rfisc.pax_id=src.pax_id AND "
    "      paid_rfisc.transfer_num+src.seg_no-dest.seg_no>=0 ",
    QParams() << QParam("grp_id_src", otInteger, grp_id_src)
              << QParam("grp_id_dest", otInteger, grp_id_dest));
  Qry.get().Execute();

  updateExcess(grp_id_dest);
}

void TGrpServiceList::addBagInfo(int grp_id,
                                 int tckin_seg_count,
                                 int trfer_seg_count)
{
  TCachedQuery Qry("SELECT ckin.get_bag_pool_pax_id(bag2.grp_id, bag2.bag_pool_num, 0) AS pax_id, "
                   "       0 AS transfer_num, "
                   "       bag2.list_id, "
                   "       bag2.rfisc, "
                   "       bag2.service_type, "
                   "       bag2.airline, "
                   "       bag2.amount AS service_quantity, "
                   "       bag2.pr_cabin "
                   "FROM bag2 "
                   "WHERE bag2.grp_id=:grp_id AND bag2.rfisc IS NOT NULL ",
                   QParams() << QParam("grp_id", otInteger, grp_id));
  Qry.get().Execute();
  for(; !Qry.get().Eof; Qry.get().Next())
  {
    if (Qry.get().FieldIsNULL("pax_id")) continue;
    TGrpServiceItem item;
    item.fromDB(Qry.get());
    bool pr_cabin=Qry.get().FieldAsInteger("pr_cabin")!=0;
    for(int trfer_num=0; trfer_num<trfer_seg_count; trfer_num++)
    {
      if (trfer_num>=tckin_seg_count && pr_cabin) continue;
      item.trfer_num=trfer_num;
      push_back(item);
    };
  }
}

void TGrpServiceList::prepareForSirena(int grp_id,
                                       int tckin_seg_count,
                                       int trfer_seg_count)
{
  clear();
  fromDB(grp_id, true);
  addBagInfo(grp_id, tckin_seg_count, trfer_seg_count);
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
    if (string((char*)itemNode->name)!="item") continue;
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

const TPaidRFISCItem& TPaidRFISCItem::toDB(TQuery &Qry) const
{
  TGrpServiceItem::toDB(Qry);
  Qry.SetVariable("paid",paid);
  Qry.SetVariable("need",need);
  return *this;
}

TPaidRFISCItem& TPaidRFISCItem::fromDB(TQuery &Qry)
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

boost::optional<TRFISCKey> TPaidRFISCList::getKeyIfSingleRFISC(int pax_id, const std::string &rfisc) const
{
  boost::optional<TRFISCKey> result=boost::none;
  for(TPaidRFISCList::const_iterator i=begin(); i!=end(); ++i)
  {
    const TPaidRFISCItem &item=i->second;
    if (item.pax_id==pax_id && item.RFISC==rfisc)
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

void TPaidRFISCItem::addStatusList(TPaidRFISCStatusList &list) const
{
  if (!service_quantity_valid()) return;

  TPaidRFISCItem item=*this;
  while(item.service_quantity>0)
  {
    if (item.need==ASTRA::NoExists ||
        item.paid==ASTRA::NoExists)
      list.push_back(TPaidRFISCStatus(item, TServiceStatus::Unknown));
    else if (item.need>0)
      list.push_back(TPaidRFISCStatus(item, TServiceStatus::Need));
    else if (item.paid>0)
      list.push_back(TPaidRFISCStatus(item, TServiceStatus::Paid));
    else
      list.push_back(TPaidRFISCStatus(item, TServiceStatus::Free));
    if (item.need!=ASTRA::NoExists) item.need--;
    if (item.paid!=ASTRA::NoExists) item.paid--;
    item.service_quantity--;
  };
}

void TPaidRFISCList::getStatusList(TPaidRFISCStatusList &list) const
{
  list.clear();
  for(TPaidRFISCList::const_iterator i=begin(); i!=end(); ++i)
    i->second.addStatusList(list);
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
    if (cat==TServiceCategory::Baggage ||
        cat==TServiceCategory::Both ||
        cat==TServiceCategory::BaggageWithOrigInfo ||
        cat==TServiceCategory::BothWithOrigInfo)
      return 0;
    if (cat==TServiceCategory::CarryOn ||
        cat==TServiceCategory::CarryOnWithOrigInfo)
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
      if (!i->first.list_item || !i->first.list_item.get().carry_on()) continue; //выводим только багаж
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
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT DISTINCT service_lists.rfisc_used, pax_service_lists.category "
    "FROM pax, pax_service_lists, service_lists "
    "WHERE pax.pax_id=pax_service_lists.pax_id AND "
    "      pax_service_lists.list_id=service_lists.id AND "
    "      pax.grp_id=:grp_id AND transfer_num=0 "
    "UNION "
    "SELECT DISTINCT service_lists.rfisc_used, grp_service_lists.category "
    "FROM grp_service_lists, service_lists "
    "WHERE grp_service_lists.list_id=service_lists.id AND "
    "      grp_service_lists.grp_id=:grp_id AND transfer_num=0";
  Qry.CreateVariable("grp_id", otInteger, grp_id);
  Qry.Execute();
  if (!Qry.Eof)
  {
    for(;!Qry.Eof;Qry.Next())
    {
      bool _rfisc_used=Qry.FieldAsInteger("rfisc_used");
      bool category=Qry.FieldAsInteger("category");
      if (_rfisc_used) rfisc_used=true;
      if (category==(int)TServiceCategory::Baggage ||
          category==(int)TServiceCategory::CarryOn)
      {
        _rfisc_used?pc=true:wt=true;
      };
    }
  }
  else
  {
    //!!! потом удалить!
    int bag_types_id=ASTRA::NoExists;
    GetBagConceptsCompatible(grp_id, pc, wt, bag_types_id);
    rfisc_used=bag_types_id!=ASTRA::NoExists;
  };
}

void GetBagConceptsCompatible(int grp_id, bool &pc, bool &wt, int &bag_types_id) //!!! потом удалить!
{
  pc=false;
  wt=false;
  bag_types_id=ASTRA::NoExists;
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT excess_wt, excess_pc, NVL(piece_concept,0) AS piece_concept, bag_types_id "
    "FROM pax_grp WHERE grp_id=:grp_id AND trfer_confirm<>0";
  Qry.CreateVariable("grp_id", otInteger, grp_id);
  Qry.Execute();
  if (Qry.Eof ||
      !Qry.FieldIsNULL("excess_wt") ||
      !Qry.FieldIsNULL("excess_pc") ||
      Qry.FieldIsNULL("piece_concept")) return;
  Qry.FieldAsInteger("piece_concept")!=0?pc=true:wt=true;
  bag_types_id=Qry.FieldIsNULL("bag_types_id")?ASTRA::NoExists:
                                               Qry.FieldAsInteger("bag_types_id");
}


