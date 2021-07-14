#include "baggage_base.h"
#include "astra_locale.h"
#include "qrys.h"
#include "baggage_ckin.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace std;
using namespace AstraLocale;
using namespace EXCEPTIONS;
using namespace BASIC::date_time;

const TServiceCategories& ServiceCategories()
{
  static TServiceCategories serviceCategories;
  return serviceCategories;
}

const TBagConcepts& BagConcepts()
{
  static TBagConcepts bagConcepts;
  return bagConcepts;
};

void TBagUnit::set(const Ticketing::Baggage::Baggage_t &value)
{
  unit=value;
}

void TBagUnit::set(const std::string &value)
{
  if      (value=="PC") unit=Ticketing::Baggage::NumPieces;
  else if (value=="KG") unit=Ticketing::Baggage::WeightKilo;
  else if (value=="LB") unit=Ticketing::Baggage::WeightPounds;
  else                  unit=Ticketing::Baggage::Nil;
}

Ticketing::Baggage::Baggage_t TBagUnit::get() const
{
  return unit;
}

std::string TBagUnit::get_db_form() const
{
  switch(unit)
  {
    case Ticketing::Baggage::NumPieces:    return "PC";
    case Ticketing::Baggage::WeightKilo:   return "KG";
    case Ticketing::Baggage::WeightPounds: return "LB";
    case Ticketing::Baggage::Nil:          return "";
  }
  return "";
}

std::string TBagUnit::get_lexeme_form() const
{
  switch(unit)
  {
    case Ticketing::Baggage::NumPieces:    return "MSG.BAGGAGE_UNIT.PC";
    case Ticketing::Baggage::WeightKilo:   return "MSG.BAGGAGE_UNIT.KG";
    case Ticketing::Baggage::WeightPounds: return "MSG.BAGGAGE_UNIT.LB";
    case Ticketing::Baggage::Nil:          return "";
  }
  return "";
}

std::string TBagUnit::get_events_form() const
{
  switch(unit)
  {
    case Ticketing::Baggage::NumPieces:    return "EVT.BAGGAGE_UNIT.PC";
    case Ticketing::Baggage::WeightKilo:   return "EVT.BAGGAGE_UNIT.KG";
    case Ticketing::Baggage::WeightPounds: return "EVT.BAGGAGE_UNIT.LB";
    case Ticketing::Baggage::Nil:          return "";
  }
  return "";
}

std::string TBagQuantity::view(const AstraLocale::OutputLang &lang, const bool &unitRequired) const
{
  ostringstream s;
  s << quantity;
  if (unitRequired)
    s << lowerc(AstraLocale::getLocaleText(unit.get_lexeme_form(), lang.get()));
  return s.str();
}

TBagQuantity& TBagQuantity::operator += (const TBagQuantity &item)
{

  if (unit.get()!=item.unit.get())
    throw Exception("%s: impossible to summarize (%s+=%s)",
                    __FUNCTION__,
                    view(AstraLocale::OutputLang(LANG_EN)).c_str(),
                    item.view(AstraLocale::OutputLang(LANG_EN)).c_str());
  quantity+=item.quantity;
  return *this;
}

std::set<int> getServiceListIdSet(ServiceGetItemWay way, int id, int transfer_num, int bag_pool_num,
                                  boost::optional<TServiceCategory::Enum> category)
{
  ostringstream sql;
  QParams QryParams;
  std::string table_name;
  if (way == ServiceGetItemWay::Unaccomp) {
    table_name = "GRP_SERVICE_LISTS";
    sql << "SELECT DISTINCT list_id \n"
           "FROM grp_service_lists \n"
           "WHERE grp_id=:id AND \n"
           "      transfer_num=:transfer_num";
  } else if (way == ServiceGetItemWay::ByGrpId) {
    table_name = "PAX_SERVICE_LISTS";
    sql << "SELECT DISTINCT list_id \n"
           "FROM pax_service_lists \n"
           "WHERE grp_id=:id AND \n"
           "      transfer_num=:transfer_num";
  } else if (way==ServiceGetItemWay::ByPaxId || way==ServiceGetItemWay::ByBagPool) {
    table_name = "PAX_SERVICE_LISTS";
    sql << "SELECT DISTINCT list_id \n"
           "FROM pax_service_lists \n"
           "WHERE pax_id=:id AND \n"
           "      transfer_num=:transfer_num";
  }

  if (category)
  {
    if (way==ServiceGetItemWay::Unaccomp) {
      sql << "      AND grp_service_lists.category=:category \n";
    } else {
      sql << "      AND pax_service_lists.category=:category \n";
    }
    QryParams << QParam("category", otInteger, (int)category.get());
  }

  if (way==ServiceGetItemWay::ByBagPool)
  {
    const std::optional<PaxId_t> pax_id = CKIN::get_bag_pool_pax_id(GrpId_t(id), bag_pool_num, std::nullopt);
    QryParams << QParam("id", otInteger, pax_id ? pax_id->get() : ASTRA::NoExists);
  } else {
    QryParams << QParam("id", otInteger, id);
  }
  QryParams << QParam("transfer_num", otInteger, transfer_num);

  std::set<int> result;
  DB::TCachedQuery Qry(PgOra::getROSession(table_name), sql.str(), QryParams, STDLOG);
  Qry.get().Execute();
  for(;!Qry.get().Eof; Qry.get().Next()) {
    result.insert(Qry.get().FieldAsInteger("list_id"));
  }
  return result;
}

std::set<int> getServiceListIdSet(int crc, bool rfisc_used)
{
  std::set<int> result;
  DB::TQuery Qry(PgOra::getROSession("SERVICE_LISTS"), STDLOG);
  Qry.SQLText = "SELECT id FROM service_lists "
                "WHERE crc=:crc "
                "AND rfisc_used=:rfisc_used";
  Qry.CreateVariable("rfisc_used", otInteger, int(rfisc_used));
  Qry.CreateVariable("crc", otInteger, crc);
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next()) {
    result.insert(Qry.FieldAsInteger("id"));
  }
  return result;
}

int saveServiceLists(int crc, bool rfisc_used)
{
  const int list_id = PgOra::getSeqNextVal("SERVICE_LISTS__SEQ");
  DB::TQuery QryIns(PgOra::getRWSession("SERVICE_LISTS"), STDLOG);
  QryIns.SQLText =
    "INSERT INTO service_lists("
    "id, crc, rfisc_used, time_create"
    ") VALUES("
    ":id, :crc, :rfisc_used, :datetime"
    ") ";
  QryIns.CreateVariable("id", otInteger, list_id);
  QryIns.CreateVariable("crc", otInteger, crc);
  QryIns.CreateVariable("rfisc_used", otInteger, int(rfisc_used));
  QryIns.CreateVariable("datetime", otDate, NowUTC());
  QryIns.Execute();
  return list_id;
}

namespace Sirena
{

TLocaleTextMap& TLocaleTextMap::fromSirenaXML(xmlNodePtr node)
{
  clear();

  try
  {
    if (node==NULL) throw Exception("node not defined");

    xmlNodePtr textNode=node->children;
    for(; textNode!=NULL; textNode=textNode->next)
    {
      if (string((const char*)textNode->name)!="text") continue;
      string lang=NodeAsString("@language", textNode, "");
      if (lang.empty()) throw Exception("Empty @language");
      if (lang!="ru" && lang!="en") continue;

      TLocaleTextItem item;
      item.lang=(lang=="ru"?LANG_RU:LANG_EN);
      item.text=NodeAsString(textNode);
      //if (item.text.empty()) throw Exception("Empty <text language='%s'>", item.lang.c_str());
      if (!insert(make_pair(item.lang, item)).second)
        throw Exception("Duplicate <text language='%s'>", item.lang.c_str());
    };

    if (find(LANG_RU)==end()) throw Exception("Not found <text language='%s'>", "ru");
    if (find(LANG_EN)==end()) throw Exception("Not found <text language='%s'>", "en");
  }
  catch(Exception &e)
  {
    throw Exception("TLocaleTextMap::fromSirenaXML: %s", e.what());
  };
  return *this;
}

TLocaleTextMap& TLocaleTextMap::add(const TLocaleTextMap &_map)
{
  for(TLocaleTextMap::const_iterator i=_map.begin(); i!=_map.end(); ++i)
  {
    pair<TLocaleTextMap::iterator, bool> res=insert(*i);
    if (!res.second)
    {
      if (res.first->second.lang!=i->second.lang)
        throw Exception("%s: strange situation res.first->second.lang!=i->second.lang", __FUNCTION__);
      res.first->second.text+='\n';
      res.first->second.text+=i->second.text;
    }
  }
  return *this;
}

const TPaxSegKey& TPaxSegKey::toSirenaXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;

  if (pax_id!=ASTRA::NoExists)
    SetProp(node, "passenger-id", pax_id);
  if (trfer_num!=ASTRA::NoExists)
    SetProp(node, "segment-id", trfer_num);
  return *this;
}

TPaxSegKey& TPaxSegKey::fromSirenaXML(xmlNodePtr node)
{
  clear();

  try
  {
    if (node==NULL) throw Exception("node not defined");

    pax_id=NodeAsInteger("@passenger-id", node);
    trfer_num=NodeAsInteger("@segment-id", node);
  }
  catch(Exception &e)
  {
    throw Exception("TPaxSegKey::fromSirenaXML: %s", e.what());
  };
  return *this;
}

const TPaxSegKey& TPaxSegKey::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  NewTextChild(node, "pax_id", pax_id);
  NewTextChild(node, "transfer_num", trfer_num);
  return *this;
}

TPaxSegKey& TPaxSegKey::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  pax_id=NodeAsIntegerFast("pax_id", node2);
  trfer_num=NodeAsIntegerFast("transfer_num", node2);
  return *this;
}

const TPaxSegKey& TPaxSegKey::toDB(TQuery &Qry) const
{
  Qry.SetVariable("pax_id", pax_id);
  Qry.SetVariable("transfer_num", trfer_num);
  return *this;
}

const TPaxSegKey& TPaxSegKey::toDB(DB::TQuery &Qry) const
{
  Qry.SetVariable("pax_id", pax_id);
  Qry.SetVariable("transfer_num", trfer_num);
  return *this;
}

TPaxSegKey& TPaxSegKey::fromDB(TQuery &Qry)
{
  clear();
  pax_id=Qry.FieldAsInteger("pax_id");
  trfer_num=Qry.FieldAsInteger("transfer_num");
  return *this;
}

TPaxSegKey& TPaxSegKey::fromDB(DB::TQuery &Qry)
{
  clear();
  pax_id=Qry.FieldAsInteger("pax_id");
  trfer_num=Qry.FieldAsInteger("transfer_num");
  return *this;
}

TPaxNormListKey& TPaxNormListKey::fromSirenaXML(xmlNodePtr node)
{
  clear();

  try
  {
    if (node==NULL) throw Exception("node not defined");

    carry_on=(string((const char*)node->name)=="free_carry_on_norm");

    TPaxSegKey::fromSirenaXML(node);
  }
  catch(Exception &e)
  {
    throw Exception("TPaxNormListKey::fromSirenaXML: %s", e.what());
  };
  return *this;
}

const TPaxNormListKey& TPaxNormListKey::toDB(DB::TQuery &Qry) const
{
  TPaxSegKey::toDB(Qry);
  Qry.SetVariable("carry_on", (int)carry_on);
  return *this;
}

TPaxNormListKey& TPaxNormListKey::fromDB(DB::TQuery &Qry)
{
  clear();
  TPaxSegKey::fromDB(Qry);
  carry_on=Qry.FieldAsInteger("carry_on")!=0;
  return *this;
}

TSimplePaxNormItem &TSimplePaxNormItem::fromSirenaXML(xmlNodePtr node)
{
  clear();

  try
  {
    if (node==NULL) throw Exception("node not defined");

    carry_on=(string((const char*)node->name)=="free_carry_on_norm");

    string str = NodeAsString("@type", node);
    if (str.empty()) throw Exception("Empty @type");
    try
    {
      concept=BagConcepts().decode(str);
    }
    catch(const EConvertError&)
    {
      throw Exception("Unknown @type='%s'", str.c_str());
    };

    str = NodeAsString("@company", node);
    TElemFmt fmt;
    if (str.empty()) throw Exception("Empty @company");
    airline = ElemToElemId( etAirline, str, fmt );
    if (fmt==efmtUnknown) throw Exception("Unknown @company='%s'", str.c_str());

    rfiscs = NodeAsString("@rfiscs", node, "");

    TLocaleTextMap::fromSirenaXML(node);
  }
  catch(Exception &e)
  {
    throw Exception("TSimplePaxNormItem::fromSirenaXML: %s", e.what());
  };
  return *this;
}

void TSimplePaxNormItem::fromSirenaXMLAdv(xmlNodePtr node, bool carry_on)
{
  clear();

  try
  {
    if (node==NULL) throw Exception("node not defined");
    string tagName=carry_on?"free_carry_on_norm":"free_baggage_norm";
    for(node=node->children; node!=NULL; node=node->next)
    {
      string nodeName=(const char*)node->name;
      if (nodeName!=tagName) continue;

      if (!empty()) throw Exception("<%s> tag duplicated", tagName.c_str());

      fromSirenaXML(node);
    };
    if (empty()) throw Exception("<%s> tag not found", tagName.c_str());
  }
  catch(Exception &e)
  {
    throw Exception("TSimplePaxNormItem::fromSirenaXMLAdv: %s", e.what());
  };
}

const TSimplePaxNormItem& TSimplePaxNormItem::toDB(DB::TQuery &Qry) const
{
  Qry.SetVariable("carry_on", (int)carry_on);
  Qry.SetVariable("airline", airline);
  Qry.SetVariable("concept", BagConcepts().encode(concept));
  Qry.SetVariable("rfiscs", rfiscs.substr(0,50));
  return *this;
}

TSimplePaxNormItem& TSimplePaxNormItem::fromDB(DB::TQuery &Qry)
{
  clear();
  carry_on=Qry.FieldAsInteger("carry_on")!=0;
  airline=Qry.FieldAsString("airline");
  concept=BagConcepts().decode(Qry.FieldAsString("concept"));
  rfiscs=Qry.FieldAsString("rfiscs");
  return *this;
}

void TSimplePaxBrandItem::fromSirenaXMLAdv(xmlNodePtr node)
{
  clear();

  try
  {
    if (node==NULL) throw Exception("node not defined");
    for(node=node->children; node!=NULL; node=node->next)
    {
      if (string((const char*)node->name)!="brand_info") continue;

      if (!empty()) throw Exception("<brand_info> tag duplicated" );

      fromSirenaXML(node);
    };
  }
  catch(Exception &e)
  {
    throw Exception("TSimplePaxBrandItem::fromSirenaXMLAdv: %s", e.what());
  };
}

void CopyPaxNorms(const GrpId_t& grp_id_src, const GrpId_t& grp_id_dest)
{
  LogTrace(TRACE6) << __func__
                     << ": grp_id_src=" << grp_id_src
                     << ", grp_id_dest=" << grp_id_dest;
  TPaxNormList norms_dest;
  TPaxNormList norms_src;
  PaxNormsFromDB(grp_id_src, norms_src);
  for (const auto& norm: norms_src) {
    const TPaxNormListKey& key = norm.first;
    const TSimplePaxNormItem& item = norm.second;
    const std::vector<PaxGrpRoute> routes = PaxGrpRoute::load(PaxId_t(key.pax_id),
                                                              key.trfer_num,
                                                              grp_id_src,
                                                              grp_id_dest);
    for (const PaxGrpRoute& route: routes) {
      const int pax_id = route.dest.pax_id.get();
      const int transfer_num = key.trfer_num + route.src.seg_no - route.dest.seg_no;
      const Sirena::TPaxSegKey new_paxseg_key(pax_id, transfer_num);
      norms_dest.emplace(TPaxNormListKey(new_paxseg_key, key.carry_on), item);
    }
  }
  PaxNormsToDB(grp_id_dest, norms_dest);
}

void CopyPaxBrands(const GrpId_t& grp_id_src, const GrpId_t& grp_id_dest)
{
  LogTrace(TRACE6) << __func__
                     << ": grp_id_src=" << grp_id_src
                     << ", grp_id_dest=" << grp_id_dest;
  TPaxBrandList brands_dest;
  TPaxBrandList brands_src;
  PaxBrandsFromDB(grp_id_src, brands_src);
  for (const auto& brand: brands_src) {
    const TPaxSegKey& key = brand.first;
    const TSimplePaxBrandItem& item = brand.second;
    const std::vector<PaxGrpRoute> routes = PaxGrpRoute::load(PaxId_t(key.pax_id),
                                                              key.trfer_num,
                                                              grp_id_src,
                                                              grp_id_dest);
    for (const PaxGrpRoute& route: routes) {
      const int pax_id = route.dest.pax_id.get();
      const int transfer_num = key.trfer_num + route.src.seg_no - route.dest.seg_no;
      const Sirena::TPaxSegKey new_paxseg_key(pax_id, transfer_num);
      brands_dest.emplace(new_paxseg_key, item);
    }
  }
  PaxBrandsToDB(grp_id_dest, brands_dest);
}

void DeletePaxNorms(const GrpId_t& grp_id)
{
  DB::TQuery Qry(PgOra::getRWSession("PAX_NORMS_TEXT"), STDLOG);
  Qry.SQLText="DELETE FROM pax_norms_text "
              "WHERE grp_id=:grp_id";
  Qry.CreateVariable("grp_id", otInteger, grp_id.get());
  Qry.Execute();
}

void MakeQry_PaxNormsToDB(DB::TQuery& Qry, const GrpId_t& grp_id)
{
  Qry.SQLText=
    "INSERT INTO pax_norms_text ("
    "grp_id, pax_id, transfer_num, carry_on, lang, page_no, airline, concept, rfiscs, text"
    ") VALUES ("
    ":grp_id, :pax_id, :transfer_num, :carry_on, :lang, :page_no, :airline, :concept, :rfiscs, :text"
    ")";
  Qry.CreateVariable("grp_id", otInteger, grp_id.get());
  Qry.DeclareVariable("pax_id", otInteger);
  Qry.DeclareVariable("transfer_num", otInteger);
  Qry.DeclareVariable("carry_on", otInteger);
  Qry.DeclareVariable("lang", otString);
  Qry.DeclareVariable("page_no", otInteger);
  Qry.DeclareVariable("airline", otString);
  Qry.DeclareVariable("concept", otString);
  Qry.DeclareVariable("rfiscs", otString);
  Qry.DeclareVariable("text", otString);
}

void PaxNormsToDB(const GrpId_t& grp_id, const TPaxNormList& norms)
{
  DeletePaxNorms(grp_id);
  DB::TQuery Qry(PgOra::getRWSession("PAX_NORMS_TEXT"), STDLOG);
  MakeQry_PaxNormsToDB(Qry, grp_id);
  for(TPaxNormList::const_iterator i=norms.begin(); i!=norms.end(); ++i)
  {
    Qry.SetVariable("pax_id", i->first.pax_id);
    Qry.SetVariable("transfer_num", i->first.trfer_num);
    i->first.toDB(Qry);
    i->second.toDB(Qry);
    for(TPaxNormItem::const_iterator j=i->second.begin(); j!=i->second.end(); ++j)
    {
      Qry.SetVariable("lang", j->second.lang);
      longToDB(Qry, "text", j->second.text);
    }
  }
}

void PaxNormsToDB(const GrpId_t& grp_id, const list<TPaxNormItem> &norms)
{
  DeletePaxNorms(grp_id);
  DB::TQuery Qry(PgOra::getRWSession("PAX_NORMS_TEXT"), STDLOG);
  MakeQry_PaxNormsToDB(Qry, grp_id);
  for(list<TPaxNormItem>::const_iterator i=norms.begin(); i!=norms.end(); ++i)
  {
    Qry.SetVariable("pax_id", i->pax_id);
    Qry.SetVariable("transfer_num", i->trfer_num);
    i->toDB(Qry);
    for(TPaxNormItem::const_iterator j=i->begin(); j!=i->end(); ++j)
    {
      Qry.SetVariable("lang", j->second.lang);
      longToDB(Qry, "text", j->second.text);
    }
  }
}

void MakeQry_PaxBrandsToDB(DB::TQuery& Qry, const GrpId_t& grp_id)
{
  Qry.SQLText=
    "INSERT INTO pax_brands ("
    "grp_id, pax_id, transfer_num, lang, page_no, text"
    ") VALUES ("
    ":grp_id, :pax_id, :transfer_num, :lang, :page_no, :text"
    ")";
  Qry.CreateVariable("grp_id", otInteger, grp_id.get());
  Qry.DeclareVariable("pax_id", otInteger);
  Qry.DeclareVariable("pax_id", otInteger);
  Qry.DeclareVariable("transfer_num", otInteger);
  Qry.DeclareVariable("lang", otString);
  Qry.DeclareVariable("page_no", otInteger);
  Qry.DeclareVariable("text", otString);
}

void DeletePaxBrands(const GrpId_t& grp_id)
{
  DB::TQuery Qry(PgOra::getRWSession("PAX_BRANDS"), STDLOG);
  Qry.SQLText="DELETE FROM pax_brands "
              "WHERE grp_id=:grp_id";
  Qry.CreateVariable("grp_id", otInteger, grp_id.get());
  Qry.Execute();
}

void PaxBrandsToDB(const GrpId_t& grp_id, const TPaxBrandList& norms)
{
  DeletePaxBrands(grp_id);
  DB::TQuery Qry(PgOra::getRWSession("PAX_BRANDS"), STDLOG);
  MakeQry_PaxBrandsToDB(Qry, grp_id);
  for(TPaxBrandList::const_iterator i=norms.begin(); i!=norms.end(); ++i)
  {
    Qry.SetVariable("pax_id", i->first.pax_id);
    Qry.SetVariable("transfer_num", i->first.trfer_num);
    for(TSimplePaxBrandItem::const_iterator j=i->second.begin(); j!=i->second.end(); ++j)
    {
      Qry.SetVariable("lang", j->second.lang);
      longToDB(Qry, "text", j->second.text);
    }
  }
}

void PaxBrandsToDB(const GrpId_t& grp_id, const list<TPaxBrandItem> &norms)
{
  DeletePaxBrands(grp_id);
  DB::TQuery Qry(PgOra::getRWSession("PAX_BRANDS"), STDLOG);
  MakeQry_PaxBrandsToDB(Qry, grp_id);
  for(list<TPaxBrandItem>::const_iterator i=norms.begin(); i!=norms.end(); ++i)
  {
    Qry.SetVariable("pax_id", i->pax_id);
    Qry.SetVariable("transfer_num", i->trfer_num);
    for(TPaxBrandItem::const_iterator j=i->begin(); j!=i->end(); ++j)
    {
      Qry.SetVariable("lang", j->second.lang);
      longToDB(Qry, "text", j->second.text);
    }
  }
}

void PaxNormsToDB(const TCkinGrpIds &tckinGrpIds, const list<TPaxNormItem> &norms)
{
  for(TCkinGrpIds::const_iterator i=tckinGrpIds.begin(); i!=tckinGrpIds.end(); ++i)
  {
    if (i==tckinGrpIds.begin())
      PaxNormsToDB(*i, norms);
    else
      CopyPaxNorms(*tckinGrpIds.begin(), *i);
  }
}

void PaxBrandsToDB(const TCkinGrpIds &tckinGrpIds, const list<TPaxBrandItem> &norms)
{
  for(TCkinGrpIds::const_iterator i=tckinGrpIds.begin(); i!=tckinGrpIds.end(); ++i)
  {
    if (i==tckinGrpIds.begin())
      PaxBrandsToDB(*i, norms);
    else
      CopyPaxBrands(*tckinGrpIds.begin(), *i);
  }
}

void PaxNormsFromDB(DB::TQuery& Qry, TPaxNormList &norms)
{
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next())
  {
    TPaxNormItem item;
    item.fromDB(Qry);
    TPaxNormListKey key;
    key.fromDB(Qry);
    TPaxNormList::iterator n=norms.insert(make_pair(key, item)).first;

    TLocaleTextItem textItem;
    textItem.lang=Qry.FieldAsString("lang");
    textItem.text=Qry.FieldAsString("text");
    TPaxNormItem::iterator i=n->second.find(textItem.lang);
    if (i!=n->second.end()) {
      i->second.text+=textItem.text;
    } else {
      n->second.insert(make_pair(textItem.lang, textItem));
    }
  }
}

void PaxNormsFromDB(const GrpId_t& grp_id, TPaxNormList &norms)
{
  norms.clear();
  DB::TQuery Qry(PgOra::getROSession("PAX_NORMS_TEXT"), STDLOG);
  Qry.SQLText="SELECT * FROM pax_norms_text "
              "WHERE grp_id=:grp_id "
              "ORDER BY page_no";
  Qry.CreateVariable("grp_id", otInteger, grp_id.get());
  PaxNormsFromDB(Qry, norms);
}

void PaxNormsFromDB(const PaxId_t& pax_id, TPaxNormList &norms)
{
  norms.clear();
  DB::TQuery Qry(PgOra::getROSession("PAX_NORMS_TEXT"), STDLOG);
  Qry.SQLText="SELECT * FROM pax_norms_text "
              "WHERE pax_id=:pax_id "
              "ORDER BY page_no";
  Qry.CreateVariable("pax_id", otInteger, pax_id.get());
  PaxNormsFromDB(Qry, norms);
}

void PaxBrandsFromDB(DB::TQuery& Qry, TPaxBrandList &brands)
{
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next())
  {
    TPaxBrandItem item;
    item.fromDB(Qry);
    TPaxBrandList::iterator b=brands.insert(make_pair(item, item)).first;

    TLocaleTextItem textItem;
    textItem.lang=Qry.FieldAsString("lang");
    textItem.text=Qry.FieldAsString("text");
    TPaxBrandItem::iterator i=b->second.find(textItem.lang);
    if (i!=b->second.end()) {
      i->second.text+=textItem.text;
    } else {
      b->second.emplace(textItem.lang, textItem);
    }
  }
}

void PaxBrandsFromDB(const GrpId_t& grp_id, TPaxBrandList &brands)
{
  brands.clear();
  DB::TQuery Qry(PgOra::getROSession("PAX_BRANDS"), STDLOG);
  Qry.SQLText=
    "SELECT * FROM pax_brands "
    "WHERE grp_id=:grp_id "
    "ORDER BY page_no";
  Qry.CreateVariable("grp_id", otInteger, grp_id.get());
  PaxBrandsFromDB(Qry, brands);
}

void PaxBrandsFromDB(const PaxId_t& pax_id, TPaxBrandList &brands)
{
  brands.clear();
  DB::TQuery Qry(PgOra::getROSession("PAX_BRANDS"), STDLOG);
  Qry.SQLText=
    "SELECT * FROM pax_brands "
    "WHERE pax_id=:pax_id "
    "ORDER BY page_no";
  Qry.CreateVariable("pax_id", otInteger, pax_id.get());
  PaxBrandsFromDB(Qry, brands);
}

std::string getRFISCsFromBaggageNorm(const PaxId_t& pax_id)
{
  TPaxNormList norms;
  PaxNormsFromDB(pax_id, norms);

  TPaxNormListKey key(TPaxSegKey(pax_id.get(), 0), false);
  TPaxNormList::const_iterator n=norms.find(key);
  if (n!=norms.end()) return n->second.rfiscs;

  return "";
}

} //namespace Sirena

#include "term_version.h"

std::string BagTypeFromXML(const std::string& bag_type)
{
  if (TReqInfo::Instance()->client_type==ASTRA::ctTerm &&
      !TReqInfo::Instance()->desk.compatible(PAX_SERVICE_VERSION) && !bag_type.empty())
  {
    ostringstream s;
    s << setw(2) << setfill('0') <<  bag_type;
    return s.str();
  }
  return bag_type;
}

std::string BagTypeFromDB(TQuery &Qry)
{
  if (Qry.GetFieldIndex("bag_type_str")>=0 &&
      !Qry.FieldIsNULL("bag_type_str"))
    return Qry.FieldAsString("bag_type_str");

  if (!Qry.FieldIsNULL("bag_type"))
  {
    ostringstream s;
    s << setw(2) << setfill('0') << Qry.FieldAsInteger("bag_type");
    return s.str();
  };
  return "";
}

std::string BagTypeFromDB(DB::TQuery &Qry)
{
  if (Qry.GetFieldIndex("bag_type_str")>=0 &&
      !Qry.FieldIsNULL("bag_type_str"))
    return Qry.FieldAsString("bag_type_str");

  if (!Qry.FieldIsNULL("bag_type"))
  {
    ostringstream s;
    s << setw(2) << setfill('0') << Qry.FieldAsInteger("bag_type");
    return s.str();
  };
  return "";
}

void BagTypeToDB(TQuery &Qry, const std::string& bag_type, const std::string &where)
{
  if (bag_type.empty())
    Qry.SetVariable("bag_type",FNull);
  else
  {
    int result;
    if (StrToInt(bag_type.c_str(), result)==EOF)
      throw EConvertError("%s: can't convert bag_type='%s' to int", where.c_str(), bag_type.c_str());
    Qry.SetVariable("bag_type",result);
  }
  if (Qry.GetVariableIndex("bag_type_str")>=0)
    Qry.SetVariable("bag_type_str",bag_type);
}

void BagTypeToDB(DB::TQuery &Qry, const std::string& bag_type, const std::string &where)
{
  if (bag_type.empty())
    Qry.SetVariable("bag_type",FNull);
  else
  {
    int result;
    if (StrToInt(bag_type.c_str(), result)==EOF)
      throw EConvertError("%s: can't convert bag_type='%s' to int", where.c_str(), bag_type.c_str());
    Qry.SetVariable("bag_type",result);
  }
  if (Qry.GetVariableIndex("bag_type_str")>=0)
    Qry.SetVariable("bag_type_str",bag_type);
}

int get_max_tckin_num(int grp_id)
{
  DB::TCachedQuery Qry(
        PgOra::getROSession("TCKIN_SEGMENTS"),
        "SELECT seg_no FROM tckin_segments "
        "WHERE grp_id=:grp_id AND pr_final<>0",
        QParams() << QParam("grp_id", otInteger, grp_id),
        STDLOG);
  Qry.get().Execute();
  if (!Qry.get().Eof) {
    return Qry.get().FieldAsInteger("seg_no");
  }
  return 0;
}


