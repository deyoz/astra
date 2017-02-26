#include "baggage_base.h"
#include "astra_locale.h"
#include "qrys.h"

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

void TBagNormUnit::set(const Ticketing::Baggage::Baggage_t &value)
{
  unit=value;
}

void TBagNormUnit::set(const std::string &value)
{
  if      (value=="PC") unit=Ticketing::Baggage::NumPieces;
  else if (value=="KG") unit=Ticketing::Baggage::WeightKilo;
  else if (value=="LB") unit=Ticketing::Baggage::WeightPounds;
  else                  unit=Ticketing::Baggage::Nil;
}

Ticketing::Baggage::Baggage_t TBagNormUnit::get() const
{
  return unit;
}

std::string TBagNormUnit::get_db_form() const
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

std::string TBagNormUnit::get_lexeme_form() const
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
      if (string((char*)textNode->name)!="text") continue;
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

TPaxSegKey& TPaxSegKey::fromDB(TQuery &Qry)
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

    carry_on=(string((char*)node->name)=="free_carry_on_norm");

    TPaxSegKey::fromSirenaXML(node);
  }
  catch(Exception &e)
  {
    throw Exception("TPaxNormListKey::fromSirenaXML: %s", e.what());
  };
  return *this;
}

const TPaxNormListKey& TPaxNormListKey::toDB(TQuery &Qry) const
{
  TPaxSegKey::toDB(Qry);
  Qry.SetVariable("carry_on", (int)carry_on);
  return *this;
}

TPaxNormListKey& TPaxNormListKey::fromDB(TQuery &Qry)
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

    carry_on=(string((char*)node->name)=="free_carry_on_norm");

    string str = NodeAsString("@type", node);
    if (str.empty()) throw Exception("Empty @type");
    try
    {
      concept=BagConcepts().decode(str);
    }
    catch(EConvertError)
    {
      throw Exception("Unknown @type='%s'", str.c_str());
    };

    str = NodeAsString("@company", node);
    TElemFmt fmt;
    if (str.empty()) throw Exception("Empty @company");
    airline = ElemToElemId( etAirline, str, fmt );
    if (fmt==efmtUnknown) throw Exception("Unknown @company='%s'", str.c_str());

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
      string nodeName=(char*)node->name;
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

const TSimplePaxNormItem& TSimplePaxNormItem::toDB(TQuery &Qry) const
{
  Qry.SetVariable("carry_on", (int)carry_on);
  Qry.SetVariable("airline", airline);
  Qry.SetVariable("concept", BagConcepts().encode(concept));
  return *this;
}

TSimplePaxNormItem& TSimplePaxNormItem::fromDB(TQuery &Qry)
{
  clear();
  carry_on=Qry.FieldAsInteger("carry_on")!=0;
  airline=Qry.FieldAsString("airline");
  concept=BagConcepts().decode(Qry.FieldAsString("concept"));
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
      if (string((char*)node->name)!="brand_info") continue;

      if (!empty()) throw Exception("<brand_info> tag duplicated" );

      fromSirenaXML(node);
    };
  }
  catch(Exception &e)
  {
    throw Exception("TSimplePaxBrandItem::fromSirenaXMLAdv: %s", e.what());
  };
}

void CopyPaxNorms(int grp_id_src, int grp_id_dest, bool old_version)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  if (old_version)
    Qry.SQLText="DELETE FROM (SELECT * FROM pax, pax_norms_pc WHERE pax.pax_id=pax_norms_pc.pax_id AND pax.grp_id=:grp_id)";
  else
    Qry.SQLText="DELETE FROM (SELECT * FROM pax, pax_norms_text WHERE pax.pax_id=pax_norms_text.pax_id AND pax.grp_id=:grp_id)";
  Qry.CreateVariable("grp_id", otInteger, grp_id_dest);
  Qry.Execute();
  Qry.Clear();
  if (old_version)
    Qry.SQLText=
      "INSERT INTO pax_norms_pc(pax_id, transfer_num, lang, page_no, text) "
      "SELECT dest.pax_id, "
      "       pax_norms_pc.transfer_num+src.seg_no-dest.seg_no, "
      "       pax_norms_pc.lang, "
      "       pax_norms_pc.page_no, "
      "       pax_norms_pc.text "
      "FROM pax_norms_pc, "
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
      "      pax_norms_pc.pax_id=src.pax_id AND "
      "      pax_norms_pc.transfer_num+src.seg_no-dest.seg_no>=0 ";
  else
    Qry.SQLText=
      "INSERT INTO pax_norms_text(pax_id, transfer_num, carry_on, lang, page_no, airline, concept, text) "
      "SELECT dest.pax_id, "
      "       pax_norms_text.transfer_num+src.seg_no-dest.seg_no, "
      "       pax_norms_text.carry_on, "
      "       pax_norms_text.lang, "
      "       pax_norms_text.page_no, "
      "       pax_norms_text.airline, "
      "       pax_norms_text.concept, "
      "       pax_norms_text.text "
      "FROM pax_norms_text, "
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
      "      pax_norms_text.pax_id=src.pax_id AND "
      "      pax_norms_text.transfer_num+src.seg_no-dest.seg_no>=0 ";
  Qry.CreateVariable("grp_id_src", otInteger, grp_id_src);
  Qry.CreateVariable("grp_id_dest", otInteger, grp_id_dest);
  Qry.Execute();
}

void CopyPaxBrands(int grp_id_src, int grp_id_dest)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="DELETE FROM (SELECT * FROM pax, pax_brands WHERE pax.pax_id=pax_brands.pax_id AND pax.grp_id=:grp_id)";
  Qry.CreateVariable("grp_id", otInteger, grp_id_dest);
  Qry.Execute();
  Qry.Clear();
  Qry.SQLText=
    "INSERT INTO pax_brands(pax_id, transfer_num, lang, page_no, text) "
    "SELECT dest.pax_id, "
    "       pax_brands.transfer_num+src.seg_no-dest.seg_no, "
    "       pax_brands.lang, "
    "       pax_brands.page_no, "
    "       pax_brands.text "
    "FROM pax_brands, "
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
    "      pax_brands.pax_id=src.pax_id AND "
    "      pax_brands.transfer_num+src.seg_no-dest.seg_no>=0 ";
  Qry.CreateVariable("grp_id_src", otInteger, grp_id_src);
  Qry.CreateVariable("grp_id_dest", otInteger, grp_id_dest);
  Qry.Execute();
}

void PaxNormsToDB(int grp_id, const list<TPaxNormItem> &norms, bool old_version)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  if (old_version)
    Qry.SQLText="DELETE FROM (SELECT * FROM pax, pax_norms_pc WHERE pax.pax_id=pax_norms_pc.pax_id AND pax.grp_id=:grp_id)";
  else
    Qry.SQLText="DELETE FROM (SELECT * FROM pax, pax_norms_text WHERE pax.pax_id=pax_norms_text.pax_id AND pax.grp_id=:grp_id)";
  Qry.CreateVariable("grp_id", otInteger, grp_id);
  Qry.Execute();
  Qry.Clear();
  if (old_version)
    Qry.SQLText=
      "INSERT INTO pax_norms_pc(pax_id, transfer_num, lang, page_no, text) "
      "VALUES(:pax_id, :transfer_num, :lang, :page_no, :text)";
  else
    Qry.SQLText=
      "INSERT INTO pax_norms_text(pax_id, transfer_num, carry_on, lang, page_no, airline, concept, text) "
      "VALUES(:pax_id, :transfer_num, :carry_on, :lang, :page_no, :airline, :concept, :text)";
  Qry.DeclareVariable("pax_id", otInteger);
  Qry.DeclareVariable("transfer_num", otInteger);
  Qry.DeclareVariable("lang", otString);
  Qry.DeclareVariable("page_no", otInteger);
  Qry.DeclareVariable("text", otString);
  if (!old_version)
  {
    Qry.DeclareVariable("carry_on", otInteger);
    Qry.DeclareVariable("airline", otString);
    Qry.DeclareVariable("concept", otString);
  };
  for(list<TPaxNormItem>::const_iterator i=norms.begin(); i!=norms.end(); ++i)
  {
    Qry.SetVariable("pax_id", i->pax_id);
    Qry.SetVariable("transfer_num", i->trfer_num);
    if (!old_version) i->toDB(Qry);
    for(TPaxNormItem::const_iterator j=i->begin(); j!=i->end(); ++j)
    {
      Qry.SetVariable("lang", j->second.lang);
      longToDB(Qry, "text", j->second.text);
    };
  };
};

void PaxBrandsToDB(int grp_id, const list<TPaxBrandItem> &norms)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="DELETE FROM (SELECT * FROM pax, pax_brands WHERE pax.pax_id=pax_brands.pax_id AND pax.grp_id=:grp_id)";
  Qry.CreateVariable("grp_id", otInteger, grp_id);
  Qry.Execute();
  Qry.Clear();
  Qry.SQLText=
    "INSERT INTO pax_brands(pax_id, transfer_num, lang, page_no, text) "
    "VALUES(:pax_id, :transfer_num, :lang, :page_no, :text)";
  Qry.DeclareVariable("pax_id", otInteger);
  Qry.DeclareVariable("transfer_num", otInteger);
  Qry.DeclareVariable("lang", otString);
  Qry.DeclareVariable("page_no", otInteger);
  Qry.DeclareVariable("text", otString);
  for(list<TPaxBrandItem>::const_iterator i=norms.begin(); i!=norms.end(); ++i)
  {
    Qry.SetVariable("pax_id", i->pax_id);
    Qry.SetVariable("transfer_num", i->trfer_num);
    for(TPaxBrandItem::const_iterator j=i->begin(); j!=i->end(); ++j)
    {
      Qry.SetVariable("lang", j->second.lang);
      longToDB(Qry, "text", j->second.text);
    };
  };
};

void PaxNormsToDB(const TCkinGrpIds &tckin_grp_ids, const list<TPaxNormItem> &norms, bool old_version)
{
  for(TCkinGrpIds::const_iterator i=tckin_grp_ids.begin(); i!=tckin_grp_ids.end(); ++i)
  {
    if (i==tckin_grp_ids.begin())
      PaxNormsToDB(*i, norms, old_version);
    else
      CopyPaxNorms(*tckin_grp_ids.begin(), *i, old_version);
  }
}

void PaxBrandsToDB(const TCkinGrpIds &tckin_grp_ids, const list<TPaxBrandItem> &norms)
{
  for(TCkinGrpIds::const_iterator i=tckin_grp_ids.begin(); i!=tckin_grp_ids.end(); ++i)
  {
    if (i==tckin_grp_ids.begin())
      PaxBrandsToDB(*i, norms);
    else
      CopyPaxBrands(*tckin_grp_ids.begin(), *i);
  }
}

void PaxNormsFromDB(int pax_id, TPaxNormList &norms)
{
  norms.clear();
  TQuery Qry(&OraSession);
  for(int pass=0; pass<2; pass++)
  {
    if (!norms.empty()) break; //!!! потом удалить второй проход
    Qry.Clear();
    Qry.SQLText=pass==0?"SELECT * FROM pax_norms_text WHERE pax_id=:id ORDER BY page_no":
                        "SELECT pax_norms_pc.*, 0 AS carry_on, "
                        "       bag_types_lists.airline, "
                        "       DECODE(NVL(pax_grp.piece_concept,0), 0, 'weight', 'piece') AS concept "
                        "FROM pax_grp, pax, pax_norms_pc, bag_types_lists "
                        "WHERE pax_grp.grp_id=pax.grp_id AND "
                        "      pax.pax_id=pax_norms_pc.pax_id AND "
                        "      pax_grp.bag_types_id=bag_types_lists.id(+) AND "
                        "      pax_norms_pc.pax_id=:id "
                        "ORDER BY page_no";
    Qry.CreateVariable("id", otInteger, pax_id);
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
      if (i!=n->second.end())
        i->second.text+=textItem.text;
      else
        n->second.insert(make_pair(textItem.lang, textItem));
    }
  }
}

void PaxBrandsFromDB(int pax_id, TPaxBrandList &brands)
{
  brands.clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT * FROM pax_brands WHERE pax_id=:id ORDER BY page_no";
  Qry.CreateVariable("id", otInteger, pax_id);
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
    if (i!=b->second.end())
      i->second.text+=textItem.text;
    else
      b->second.insert(make_pair(textItem.lang, textItem));
  }
}

} //namespace Sirena

#include "term_version.h"

std::string BagTypeFromXML(const std::string& bag_type)
{
  if (!TReqInfo::Instance()->desk.compatible(PAX_SERVICE_VERSION) && !bag_type.empty())
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

int get_max_tckin_num(int grp_id)
{
  TCachedQuery Qry("SELECT seg_no FROM tckin_segments WHERE grp_id=:grp_id AND pr_final<>0",
                   QParams() << QParam("grp_id", otInteger, grp_id));
  Qry.get().Execute();
  if (!Qry.get().Eof)
    return Qry.get().FieldAsInteger("seg_no");
  else
    return 0;
}



