#include "baggage_pc.h"
#include <boost/crc.hpp>
#include "astra_context.h"
#include "httpClient.h"
#include "points.h"
#include "basic.h"
#include "astra_misc.h"
#include "misc.h"
#include "emdoc.h"
#include "baggage.h"
#include "astra_locale.h"
#include "qrys.h"
#include <boost/asio.hpp>
#include <serverlib/xml_stuff.h>

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace BASIC;
using namespace EXCEPTIONS;
using namespace std;
using namespace AstraLocale;
using namespace SirenaExchange;

/*const char* SIRENA_HOST()
{
  static string VAR;
  if ( VAR.empty() )
    VAR=getTCLParam("SIRENA_HOST",NULL);
  return VAR.c_str();
}

int SIRENA_PORT()
{
  static int VAR=ASTRA::NoExists;
  if (VAR==ASTRA::NoExists)
    VAR=getTCLParam("SIRENA_PORT", ASTRA::NoExists, ASTRA::NoExists, ASTRA::NoExists);
  return VAR;
};

int SIRENA_REQ_TIMEOUT()
{
  static int VAR=ASTRA::NoExists;
  if (VAR==ASTRA::NoExists)
    VAR=getTCLParam("SIRENA_REQ_TIMEOUT", 1000, 30000, 10000);
  return VAR;
};

int SIRENA_REQ_ATTEMPTS()
{
  static int VAR=ASTRA::NoExists;
  if (VAR==ASTRA::NoExists)
    VAR=getTCLParam("SIRENA_REQ_ATTEMPTS", 1, 10, 1);
  return VAR;
};*/

namespace PieceConcept
{

void TNodeList::apply()
{
    if(concept == ctAll) {
        for(TConceptList::iterator i = items.begin(); i != items.end(); i++) {
            if(i->second)
                NodeSetContent(i->first, std::string(NodeAsString(i->first))+AstraLocale::getLocaleText("м"));
            else
                NodeSetContent(i->first, std::string(NodeAsString(i->first))+AstraLocale::getLocaleText("кг"));
        }
    }
}

void TNodeList::set_concept(xmlNodePtr& node, bool val)
{   if(!must_work) return;
    if(node) {
        items.push_back(std::make_pair(node,val));
    }
    ConceptType tmp = (val == false ? ctWeight : ctPiece);
    if(concept == ctInitial) {
        concept = tmp;
    } else if(concept != ctAll and concept != tmp)
        concept = ctAll;
}


TRFISCListItem& TRFISCListItem::fromXML(xmlNodePtr node)
{
  clear();

  try
  {
    if (node==NULL) throw Exception("node not defined");

    RFIC=NodeAsString("@rfic", node, "");
    if (RFIC.size()>1) throw Exception("Wrong @rfic='%s'", RFIC.c_str());

    RFISC=NodeAsString("@rfisc", node, "");
    if (RFISC.empty()) throw Exception("Empty @rfisc");
    if (RFISC.size()>15) throw Exception("Wrong @rfisc='%s'", RFISC.c_str());

    service_type=NodeAsString("@service_type", node, "");
    if (service_type.empty()) throw Exception("Empty @service_type");
    if (service_type.size()>1) throw Exception("Wrong @service_type='%s'", service_type.c_str());

    string xml_emd_type=NodeAsString("@emd_type", node, "");
    if (!xml_emd_type.empty())
    {
      emd_type=DecodeEmdType(xml_emd_type);
      if (emd_type.empty())
        throw Exception("Wrong @emd_type='%s'", xml_emd_type.c_str());
    };

    if (NodeIsNULL("@carry_on", node))
      throw Exception("Empty @carry_on");
    pr_cabin=NodeAsBoolean("@carry_on", node);

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
  }
  catch(Exception &e)
  {
    throw Exception("TRFISCListItem::fromXML: %s", e.what());
  };
  return *this;
};

const TRFISCListItem& TRFISCListItem::toDB(TQuery &Qry) const
{
  Qry.SetVariable("rfic", RFIC);
  Qry.SetVariable("rfisc", RFISC);
  Qry.SetVariable("service_type", service_type);
  Qry.SetVariable("emd_type", emd_type);
  pr_cabin?Qry.SetVariable("pr_cabin", (int)pr_cabin.get()):
           Qry.SetVariable("pr_cabin", FNull);
  Qry.SetVariable("name", name);
  Qry.SetVariable("name_lat", name_lat);
  return *this;
};

TRFISCListItem& TRFISCListItem::fromDB(TQuery &Qry)
{
  clear();
  RFIC=Qry.FieldAsString("rfic");
  RFISC=Qry.FieldAsString("rfisc");
  service_type=Qry.FieldAsString("service_type");
  emd_type=Qry.FieldAsString("emd_type");
  if (!Qry.FieldIsNULL("pr_cabin"))
    pr_cabin=Qry.FieldAsInteger("pr_cabin")!=0;
  name=Qry.FieldAsString("name");
  name_lat=Qry.FieldAsString("name_lat");
  return *this;
};

void TRFISCList::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) throw Exception("TRFISCListItem::fromXML: node not defined");
  for(node=node->children; node!=NULL; node=node->next)
  {
    if (string((char*)node->name)!="svc") continue;
    TRFISCListItem item;
    item.fromXML(node);
    if (!insert(make_pair(item.RFISC, item)).second)
      ProgTrace(TRACE5, "TRFISCListItem::fromXML: Duplicate @rfisc='%s'", item.RFISC.c_str());
  }
  filter_baggage_rfiscs();
}

void TRFISCList::fromDB(int list_id)
{
  clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT * FROM grp_rfisc_lists WHERE list_id=:list_id";
  Qry.CreateVariable( "list_id", otInteger, list_id );
  Qry.Execute();
  for ( ;!Qry.Eof; Qry.Next())
  {
    TRFISCListItem item;
    item.fromDB(Qry);
    insert(make_pair(item.RFISC, item));
  }
}

void TRFISCList::toDB(int list_id) const
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "INSERT INTO grp_rfisc_lists(list_id,rfisc,service_type,rfic,emd_type,pr_cabin,name,name_lat) "
    "VALUES(:list_id,:rfisc,:service_type,:rfic,:emd_type,:pr_cabin,:name,:name_lat)";
  Qry.CreateVariable( "list_id", otInteger, list_id );
  Qry.DeclareVariable( "rfisc", otString );
  Qry.DeclareVariable( "service_type", otString );
  Qry.DeclareVariable( "rfic", otString );
  Qry.DeclareVariable( "emd_type", otString );
  Qry.DeclareVariable( "pr_cabin", otInteger );
  Qry.DeclareVariable( "name", otString );
  Qry.DeclareVariable( "name_lat", otString );
  for(TRFISCList::const_iterator i=begin(); i!=end(); ++i)
  {
    i->second.toDB(Qry);
    Qry.Execute();
  }
}

int TRFISCList::crc() const
{
  if (empty()) throw Exception("TRFISCList::crc: empty list");
  boost::crc_basic<32> crc32( 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true, true );
  crc32.reset();
  ostringstream buf;
  for(TRFISCList::const_iterator i=begin(); i!=end(); ++i)
    buf << i->first << ";";
  crc32.process_bytes( buf.str().c_str(), buf.str().size() );
  return crc32.checksum();
}

void TRFISCList::fromDBAdv(int list_id)
{
  clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT airline FROM bag_types_lists WHERE id=:list_id";
  Qry.CreateVariable( "list_id", otInteger, list_id );
  Qry.Execute();
  if (Qry.Eof) return;
  fromDB(list_id);
  airline=Qry.FieldAsString("airline");
}

int TRFISCList::toDBAdv() const
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText = "SELECT id FROM bag_types_lists WHERE crc=:crc";
  Qry.CreateVariable( "crc", otInteger, crc() );
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next())
  {
    int list_id=Qry.FieldAsInteger("id");
    TRFISCList list;
    list.fromDBAdv(list_id);
    if (*this==list && airline==list.airline) return list_id;
  };

  Qry.Clear();
  Qry.SQLText =
    "BEGIN "
    "  SELECT bag_types_lists__seq.nextval INTO :id FROM dual; "
    "  INSERT INTO bag_types_lists(id, airline, crc) VALUES(:id, :airline, :crc); "
    "END;";
  Qry.CreateVariable("id", otInteger, FNull);
  Qry.CreateVariable("airline", otString, airline);
  Qry.CreateVariable("crc", otInteger, crc());
  Qry.Execute();
  int list_id=Qry.GetVariableAsInteger("id");
  toDB(list_id);
  return list_id;
}

void TRFISCList::filter_baggage_rfiscs()
{
//  здесь, возможно, надо в будущем сделать фильтрацию RFISC, относящихся только к багажу
}

std::string TRFISCList::localized_name(const std::string& rfisc, const std::string& lang) const
{
  if (rfisc.empty()) return "";
  TRFISCList::const_iterator i=find(rfisc);
  if (i==end()) return "";
  return lang==AstraLocale::LANG_RU?i->second.name:i->second.name_lat;
}

void TRFISCList::check(const CheckIn::TBagItem &bag) const
{
  if (bag.rfisc.empty()) return;
  TRFISCList::const_iterator i=find(bag.rfisc);
  if (i==end())
    throw UserException("MSG.LUGGAGE.UNKNOWN_BAG_TYPE",
                        LParams() << LParam("bag_type", bag.rfisc));
  if (i->second.pr_cabin && bag.pr_cabin!=i->second.pr_cabin.get())
  {
    if (i->second.pr_cabin.get())
      throw UserException("MSG.LUGGAGE.BAG_TYPE_SHOULD_BE_ADDED_TO_CABIN",
                          LParams() << LParam("bag_type", bag.rfisc));
    else
      throw UserException("MSG.LUGGAGE.BAG_TYPE_SHOULD_BE_ADDED_TO_HOLD",
                          LParams() << LParam("bag_type", bag.rfisc));
  };
}

TRFISCSetting& TRFISCSetting::fromDB(TQuery &Qry)
{
  clear();
  RFISC=Qry.FieldAsString("code");
  if (!Qry.FieldIsNULL("priority"))
    priority=Qry.FieldAsInteger("priority");
  if (!Qry.FieldIsNULL("min_weight"))
    min_weight=Qry.FieldAsInteger("min_weight");
  if (!Qry.FieldIsNULL("max_weight"))
    max_weight=Qry.FieldAsInteger("max_weight");
  return *this;
}

void TRFISCSettingList::fromDB(const string& airline)
{
  clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT * FROM rfisc_types WHERE airline=:airline";
  Qry.CreateVariable( "airline", otString, airline );
  Qry.Execute();
  for ( ;!Qry.Eof; Qry.Next())
  {
    TRFISCSetting setting;
    setting.fromDB(Qry);
    insert(make_pair(setting.RFISC, setting));
  }
}

void TRFISCSettingList::check(const CheckIn::TBagItem &bag) const
{
  if (bag.rfisc.empty()) return;
  TRFISCSettingList::const_iterator i=find(bag.rfisc);
  if (i==end()) return;
  if ((i->second.min_weight!=ASTRA::NoExists &&
       bag.weight<i->second.min_weight*bag.amount) ||
      (i->second.max_weight!=ASTRA::NoExists &&
       bag.weight>i->second.max_weight*bag.amount))
  {
    ostringstream s;
    s << bag.weight << " " << getLocaleText("кг");
    throw UserException("MSG.LUGGAGE.WRONG_WEIGHT_FOR_BAG_TYPE",
                        LParams() << LParam("bag_type", bag.rfisc)
                                  << LParam("weight", s.str()));
  };
}

void TRFISCListWithSets::fromDB(int list_id)
{
  TRFISCList::fromDBAdv(list_id);
  if (!airline.empty())
    TRFISCSettingList::fromDB(airline);
}

void TRFISCListWithSets::check(const CheckIn::TBagItem &bag) const
{
  TRFISCList::check(bag);
  TRFISCSettingList::check(bag);
}

void TLocaleTextMap::fromXML(xmlNodePtr node)
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
      if (item.text.empty()) throw Exception("Empty <text language='%s'>", item.lang.c_str());
      if (!insert(make_pair(item.lang, item)).second)
        throw Exception("Duplicate <text language='%s'>", item.lang.c_str());
    };

    if (find(LANG_RU)==end()) throw Exception("Not found <text language='%s'>", "ru");
    if (find(LANG_EN)==end()) throw Exception("Not found <text language='%s'>", "en");
  }
  catch(Exception &e)
  {
    throw Exception("TLocaleTextMap::fromXML: %s", e.what());
  };
}

void TSimplePaxNormItem::fromXMLAdv(xmlNodePtr node, TBagConcept &concept, string &airline)
{
  clear();

  try
  {
    if (node==NULL) throw Exception("node not defined");
    for(node=node->children; node!=NULL; node=node->next)
    {
      if (string((char*)node->name)!="free_baggage_norm") continue;

      if (!empty()) throw Exception("<free_baggage_norm> tag duplicated" );

      string str = NodeAsString("@type", node);
      if (str.empty()) throw Exception("Empty @type");
      if ( str == "piece" ) concept=bcPiece;
      else if
          ( str == "weight" ) concept=bcWeight;
      else if
          ( str == "unknown" ) concept=bcUnknown;
      else
        throw Exception("Unknown @type='%s'", str.c_str());

      str = NodeAsString("@company", node);
      TElemFmt fmt;
      if (str.empty()) throw Exception("Empty @company");
      airline = ElemToElemId( etAirline, str, fmt );
      if (fmt==efmtUnknown) throw Exception("Unknown @company='%s'", str.c_str());

      fromXML(node);
    };
    if (empty()) throw Exception("<free_baggage_norm> tag not found" );
  }
  catch(Exception &e)
  {
    throw Exception("TSimplePaxNormItem::fromXMLAdv: %s", e.what());
  };
}

void TSimplePaxBrandItem::fromXMLAdv(xmlNodePtr node)
{
  clear();

  try
  {
    if (node==NULL) throw Exception("node not defined");
    for(node=node->children; node!=NULL; node=node->next)
    {
      if (string((char*)node->name)!="brand_info") continue;

      if (!empty()) throw Exception("<brand_info> tag duplicated" );

      fromXML(node);
    };
  }
  catch(Exception &e)
  {
    throw Exception("TSimplePaxBrandItem::fromXMLAdv: %s", e.what());
  };
}

void CopyPaxNorms(int grp_id_src, int grp_id_dest)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="DELETE FROM (SELECT * FROM pax, pax_norms_pc WHERE pax.pax_id=pax_norms_pc.pax_id AND pax.grp_id=:grp_id)";
  Qry.CreateVariable("grp_id", otInteger, grp_id_dest);
  Qry.Execute();
  Qry.Clear();
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

void PaxNormsToDB(int grp_id, const list<TPaxNormItem> &norms)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText="DELETE FROM (SELECT * FROM pax, pax_norms_pc WHERE pax.pax_id=pax_norms_pc.pax_id AND pax.grp_id=:grp_id)";
  Qry.CreateVariable("grp_id", otInteger, grp_id);
  Qry.Execute();
  Qry.Clear();
  Qry.SQLText=
    "INSERT INTO pax_norms_pc(pax_id, transfer_num, lang, page_no, text) "
    "VALUES(:pax_id, :transfer_num, :lang, :page_no, :text)";
  Qry.DeclareVariable("pax_id", otInteger);
  Qry.DeclareVariable("transfer_num", otInteger);
  Qry.DeclareVariable("lang", otString);
  Qry.DeclareVariable("page_no", otInteger);
  Qry.DeclareVariable("text", otString);
  for(list<TPaxNormItem>::const_iterator i=norms.begin(); i!=norms.end(); ++i)
  {
    Qry.SetVariable("pax_id", i->pax_id);
    Qry.SetVariable("transfer_num", i->trfer_num);
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

void PaxNormsToDB(const TCkinGrpIds &tckin_grp_ids, const list<TPaxNormItem> &norms)
{
  for(TCkinGrpIds::const_iterator i=tckin_grp_ids.begin(); i!=tckin_grp_ids.end(); ++i)
  {
    if (i==tckin_grp_ids.begin())
      PaxNormsToDB(*i, norms);
    else
      CopyPaxNorms(*tckin_grp_ids.begin(), *i);
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

void PaxNormsFromDB(int pax_id, list<TPaxNormItem> &norms)
{
  norms.clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT * FROM pax_norms_pc WHERE pax_id=:id ORDER BY pax_id, transfer_num, lang, page_no";
  Qry.CreateVariable("id", otInteger, pax_id);
  Qry.Execute();
  list<TPaxNormItem>::iterator iLast=norms.end();
  for(; !Qry.Eof; Qry.Next())
  {
    TPaxNormItem item;
    item.pax_id=Qry.FieldAsInteger("pax_id");
    item.trfer_num=Qry.FieldAsInteger("transfer_num");
    if (iLast==norms.end() ||
        iLast->pax_id!=item.pax_id || iLast->trfer_num!=item.trfer_num)
      iLast=norms.insert(norms.end(), item);

    TLocaleTextItem textItem;
    textItem.lang=Qry.FieldAsString("lang");
    textItem.text=Qry.FieldAsString("text");
    TPaxNormItem::iterator i=iLast->find(textItem.lang);
    if (i!=iLast->end())
      i->second.text+=textItem.text;
    else
      iLast->insert(make_pair(textItem.lang, textItem));
  }
}

void PaxBrandsFromDB(int pax_id, list<TPaxBrandItem> &norms)
{
  norms.clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT * FROM pax_brands WHERE pax_id=:id ORDER BY pax_id, transfer_num, lang, page_no";
  Qry.CreateVariable("id", otInteger, pax_id);
  Qry.Execute();
  list<TPaxBrandItem>::iterator iLast=norms.end();
  for(; !Qry.Eof; Qry.Next())
  {
    TPaxBrandItem item;
    item.pax_id=Qry.FieldAsInteger("pax_id");
    item.trfer_num=Qry.FieldAsInteger("transfer_num");
    if (iLast==norms.end() ||
        iLast->pax_id!=item.pax_id || iLast->trfer_num!=item.trfer_num)
      iLast=norms.insert(norms.end(), item);

    TLocaleTextItem textItem;
    textItem.lang=Qry.FieldAsString("lang");
    textItem.text=Qry.FieldAsString("text");
    TPaxBrandItem::iterator i=iLast->find(textItem.lang);
    if (i!=iLast->end())
      i->second.text+=textItem.text;
    else
      iLast->insert(make_pair(textItem.lang, textItem));
  }
}

void PaxBrandsNormsToStream(const TTrferRoute &trfer, const CheckIn::TPaxItem &pax, ostringstream &s)
{
  s << "#" << setw(3) << setfill('0') << pax.reg_no << " "
    << pax.full_name() << "(" << ElemIdToCodeNative(etPersType, EncodePerson(pax.pers_type)) << "):" << endl;

  list<TPaxNormItem> norms;
  PaxNormsFromDB(pax.id, norms);
  list<TPaxBrandItem> brands;
  PaxBrandsFromDB(pax.id, brands);

  for(int pass=0; pass<2; pass++)
  {
    string prior_text;
    int trfer_num=0;
    for(TTrferRoute::const_iterator t=trfer.begin(); ; ++t, trfer_num++)
    {
      //ищем норму, соответствующую сегменту
      string curr_text;
      if (t!=trfer.end())
      {
        if (pass==0)
        {
          //бренды
          for(list<TPaxBrandItem>::const_iterator b=brands.begin(); b!=brands.end(); ++b)
          {
            const TPaxBrandItem &brand=*b;
            if (brand.trfer_num!=trfer_num) continue;

            TSimplePaxBrandItem::const_iterator i=brand.find(TReqInfo::Instance()->desk.lang);
            if (i==brand.end() && !brand.empty()) i=brand.begin(); //первая попавшаяся

            if (i!=brand.end()) curr_text=i->second.text;
            break;
          };
        }
        else
        {
          //нормы
          for(list<TPaxNormItem>::const_iterator n=norms.begin(); n!=norms.end(); ++n)
          {
            const TPaxNormItem &norm=*n;
            if (norm.trfer_num!=trfer_num) continue;

            TSimplePaxNormItem::const_iterator i=norm.find(TReqInfo::Instance()->desk.lang);
            if (i==norm.end() && !norm.empty()) i=norm.begin(); //первая попавшаяся

            if (i!=norm.end()) curr_text=i->second.text;
            break;
          };
        };
      };

      if (t!=trfer.begin())
      {
        if (t==trfer.end() || curr_text!=prior_text)
        {
          if (pass==1 || !prior_text.empty())
          {
            s << ":" << endl //закончили секцию сегментов
              << (prior_text.empty()?getLocaleText(pass==0?"MSG.UNKNOWN_BRAND":
                                                           "MSG.LUGGAGE.UNKNOWN_BAG_NORM"):prior_text) << endl;  //записали соответствующую норму
          };
        }
        else
        {
          if (pass==1 || !prior_text.empty()) s << " -> ";
        }
      };

      if (t==trfer.end()) break;

      if (t==trfer.begin() || curr_text!=prior_text)
      {
        if (pass==1 || t==trfer.begin() || !prior_text.empty()) s << endl;
      };

      if (pass==1 || !curr_text.empty())
      {
        const TTrferRouteItem &item=*t;
        s << ElemIdToCodeNative(etAirline, item.operFlt.airline)
          << setw(2) << setfill('0') << item.operFlt.flt_no
          << ElemIdToCodeNative(etSuffix, item.operFlt.suffix)
          << " " << ElemIdToCodeNative(etAirp, item.operFlt.airp);
      };

      prior_text=curr_text;
    }
  }

  s << string(100,'=') << endl; //подведем черту :)
}

std::string DecodeEmdType(const std::string &s)
{
  if ( s == "EMD-A" ) return "A";
  else if
     ( s == "EMD-S" ) return "S";
  else if
     ( s == "OTHR" ) return "O";
  else return "";
}

std::string EncodeEmdType(const std::string &s)
{
  if ( s == "A" ) return "EMD-A";
  else if
     ( s == "S" ) return "EMD-S";
  else if
     ( s == "O" ) return "OTHR";
  else return "";
}

TBagStatus DecodeBagStatus(const std::string &s)
{
  if ( s == "unknown" ) return bsUnknown;
  else if
     ( s == "free" ) return bsFree;
  else if
     ( s == "paid" ) return bsPaid;
  else if
     ( s == "need" ) return bsNeed;
  return bsNone;
}

const std::string EncodeBagStatus(const TBagStatus &s)
{
  switch(s)
  {
    case bsUnknown: return "unknown";
       case bsFree: return "free";
       case bsPaid: return "paid";
       case bsNeed: return "need";
           default: return "";
  }
}

const TPaidBagItem& TPaidBagItem::toDB(TQuery &Qry) const
{
  Qry.SetVariable("pax_id", pax_id);
  Qry.SetVariable("transfer_num", trfer_num);
  Qry.SetVariable("rfisc", RFISC);
  Qry.SetVariable("status", EncodeBagStatus(status));
  Qry.SetVariable("pr_cabin", (int)pr_cabin);
  return *this;
};

TPaidBagItem& TPaidBagItem::fromDB(TQuery &Qry)
{
  clear();
  pax_id=Qry.FieldAsInteger("pax_id");
  trfer_num=Qry.FieldAsInteger("transfer_num");
  RFISC=Qry.FieldAsString("rfisc");
  status=DecodeBagStatus(Qry.FieldAsString("status"));
  pr_cabin=(int)(Qry.FieldAsInteger("pr_cabin")!=0);
  return *this;
};

void PaidBagToDB(int grp_id, const list<TPaidBagItem> &paid)
{
  TQuery BagQry(&OraSession);
  BagQry.Clear();
  BagQry.SQLText="DELETE FROM (SELECT * FROM pax, paid_bag_pc WHERE pax.pax_id=paid_bag_pc.pax_id AND pax.grp_id=:grp_id)";
  BagQry.CreateVariable("grp_id",otInteger,grp_id);
  BagQry.Execute();
  BagQry.Clear();
  BagQry.SQLText=
    "INSERT INTO paid_bag_pc(pax_id, transfer_num, rfisc, status, pr_cabin) "
    "VALUES(:pax_id, :transfer_num, :rfisc, :status, :pr_cabin)";
  BagQry.DeclareVariable("pax_id", otInteger);
  BagQry.DeclareVariable("transfer_num", otInteger);
  BagQry.DeclareVariable("rfisc", otString);
  BagQry.DeclareVariable("status", otString);
  BagQry.DeclareVariable("pr_cabin", otInteger);
  int excess=0;
  for(list<TPaidBagItem>::const_iterator i=paid.begin(); i!=paid.end(); ++i)
  {
    if (i->trfer_num==0 &&
        (i->status==bsUnknown ||
         i->status==bsPaid ||
         i->status==bsNeed)) excess++;
    i->toDB(BagQry);
    BagQry.Execute();
  };

  BagQry.Clear();
  BagQry.SQLText="UPDATE pax_grp SET excess=:excess WHERE grp_id=:grp_id";
  BagQry.CreateVariable("grp_id", otInteger, grp_id);
  BagQry.CreateVariable("excess", otInteger, excess);
  BagQry.Execute();
};

void PaidBagFromDB(int id, bool is_grp_id, list<TPaidBagItem> &paid)
{
  paid.clear();
  TQuery BagQry(&OraSession);
  BagQry.Clear();
  if (is_grp_id)
    BagQry.SQLText=
        "SELECT paid_bag_pc.pax_id, "
        "       paid_bag_pc.transfer_num, "
        "       paid_bag_pc.rfisc, "
        "       paid_bag_pc.status, "
        "       paid_bag_pc.pr_cabin "
        "FROM pax, paid_bag_pc "
        "WHERE pax.pax_id=paid_bag_pc.pax_id AND pax.grp_id=:id";
  else
    BagQry.SQLText=
        "SELECT * FROM paid_bag_pc WHERE pax_id=:id";
  BagQry.CreateVariable("id", otInteger, id);
  BagQry.Execute();
  for(;!BagQry.Eof;BagQry.Next())
    paid.push_back(TPaidBagItem().fromDB(BagQry));
};

class TBagKey
{
  public:
    int pax_id;
    string rfisc;
    string rfic;
    string emd_type;
    TBagKey()
    {
      pax_id=ASTRA::NoExists;
      rfisc.clear();
      rfic.clear();
      emd_type.clear();
    }

    bool operator < (const TBagKey &key) const
    {
      if (pax_id!=key.pax_id)
        return pax_id<key.pax_id;
      return rfisc<key.rfisc;
    }
};

class TBagPcs
{
  public:
    int cabin, trunk;
    TBagPcs()
    {
      cabin=0;
      trunk=0;
    }
    void dec()
    {
      if (trunk>0) trunk--;
      else if
         (cabin>0) cabin--;
    }
    bool zero() const
    {
      return cabin==0 && trunk==0;
    }
};

typedef map<TBagKey, TBagPcs> TBagMap;

void GetBagInfo(int grp_id, TBagMap &bag_map)
{
  bag_map.clear();
  //набираем багаж
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT b.rfisc, b.pr_cabin, b.amount, b.pax_id, "
    "       grp_rfisc_lists.rfic, grp_rfisc_lists.emd_type "
    "FROM grp_rfisc_lists, "
    "  (SELECT bag2.rfisc, bag2.pr_cabin, bag2.amount, pax_grp.bag_types_id, "
    "          ckin.get_bag_pool_pax_id(bag2.grp_id, bag2.bag_pool_num, 0) AS pax_id "
    "   FROM pax_grp, bag2 "
    "   WHERE pax_grp.grp_id=bag2.grp_id AND "
    "         bag2.grp_id=:grp_id AND bag2.is_trfer=0) b "
    "WHERE b.bag_types_id=grp_rfisc_lists.list_id(+) AND "
    "      b.rfisc=grp_rfisc_lists.rfisc(+) ";
  Qry.CreateVariable("grp_id", otInteger, grp_id);
  Qry.Execute();
  set<int/*pax_id*/> pax_ids;
  for(; !Qry.Eof; Qry.Next())
  {
    if (Qry.FieldIsNULL("pax_id")) continue;  //если пассажир разрегистрирован
    TBagKey key;
    key.pax_id=Qry.FieldAsInteger("pax_id");
    key.rfisc=Qry.FieldAsString("rfisc");
    key.rfic=Qry.FieldAsString("rfic");
    key.emd_type=EncodeEmdType(Qry.FieldAsString("emd_type"));
    TBagMap::iterator b=bag_map.find(key);
    if (b==bag_map.end())
      b=bag_map.insert(make_pair(key, TBagPcs())).first;
    (Qry.FieldAsInteger("pr_cabin")!=0?b->second.cabin:b->second.trunk)+=Qry.FieldAsInteger("amount");
    pax_ids.insert(key.pax_id);
  };
}

void CalcPaidStatus(list<CheckIn::TPaidBagEMDItem> &paid_bag_emd,
                    TPaidBagItem &item)
{
  for(list<CheckIn::TPaidBagEMDItem>::iterator i=paid_bag_emd.begin(); i!=paid_bag_emd.end(); ++i)
  {
    if (i->pax_id==item.pax_id &&
        i->trfer_num==item.trfer_num &&
        i->rfisc==item.RFISC)
    {
      item.status=bsPaid;
      i=paid_bag_emd.erase(i);
      break;
    }
  }
}

string GetBagRcptStr(int grp_id, int pax_id)
{
  vector<string> rcpts;
  CheckIn::PaidBagEMDList emd;
  PaxASVCList::GetBoundPaidBagEMD(grp_id, 0, emd);
  for(CheckIn::PaidBagEMDList::const_iterator i=emd.begin(); i!=emd.end(); ++i)
    if (i->second.pax_id==ASTRA::NoExists || pax_id==ASTRA::NoExists || i->second.pax_id==pax_id)
      rcpts.push_back(i->second.emd_no);
  if (!rcpts.empty())
  {
    sort(rcpts.begin(),rcpts.end());
    return ::GetBagRcptStr(rcpts);
  };
  return "";
}

bool BagPaymentCompleted(int grp_id, int pax_id, bool only_tckin_segs)
{
  int max_trfer_num=ASTRA::NoExists;
  if (only_tckin_segs)
  {
    TCachedQuery Qry("SELECT seg_no FROM tckin_segments WHERE grp_id=:grp_id AND pr_final<>0",
                     QParams() << QParam("grp_id", otInteger, grp_id));
    Qry.get().Execute();
    if (!Qry.get().Eof)
      max_trfer_num=Qry.get().FieldAsInteger("seg_no");
    else
      max_trfer_num=0;
  }

  list<TPaidBagItem> paid;
  PaidBagFromDB(pax_id, false, paid);
  for(list<TPaidBagItem>::const_iterator i=paid.begin(); i!=paid.end(); ++i)
  {
    if (max_trfer_num!=ASTRA::NoExists && i->trfer_num>max_trfer_num) continue;
    if (i->status==bsUnknown||
        i->status==bsNeed) return false;
  };
  return true;
}

void PreparePaidBagInfo(int grp_id,
                        int seg_count,
                        list<TPaidBagItem> &paid_bag)
{
  paid_bag.clear();

  TBagMap bag_map;
  GetBagInfo(grp_id, bag_map);

  list<CheckIn::TPaidBagEMDItem> paid_bag_emd;
  CheckIn::PaidBagEMDFromDB(grp_id, paid_bag_emd);

  for(int trfer_num=0; trfer_num<seg_count; trfer_num++)
  {
    TBagMap bag_map_tmp=bag_map;
    for(TBagMap::iterator b=bag_map_tmp.begin(); b!=bag_map_tmp.end(); ++b)
    {
      while (!b->second.zero())
      {
        TPaidBagItem item;
        item.pax_id=b->first.pax_id;
        item.trfer_num=trfer_num;
        item.RFISC=b->first.rfisc;
        item.RFIC=b->first.rfic;
        item.emd_type=b->first.emd_type;
        item.status=bsNone;
        item.pr_cabin=(b->second.trunk<=0);
        CalcPaidStatus(paid_bag_emd, item);
        b->second.dec();
        paid_bag.push_back(item);
      };
    };
  }
}

class TTmpKey
  {
    public:
      int pax_id;
      string rfisc;
      int trfer_num;
      TTmpKey(const PieceConcept::TPaidBagItem &item) : pax_id(item.pax_id), rfisc(item.RFISC), trfer_num(item.trfer_num) {}
      TTmpKey(const CheckIn::TPaidBagEMDItem &item) : pax_id(item.pax_id), rfisc(item.rfisc), trfer_num(item.trfer_num) {}
      TTmpKey(int _pax_id, string _rfisc, int _trfer_num) : pax_id(_pax_id), rfisc(_rfisc), trfer_num(_trfer_num) {}
      bool operator < (const TTmpKey &key) const
      {
        if (pax_id!=key.pax_id)
          return pax_id<key.pax_id;
        if (rfisc!=key.rfisc)
          return rfisc<key.rfisc;
        return trfer_num<key.trfer_num;
      }
  };

bool TryDelPaidBagEMD(const list<PieceConcept::TPaidBagItem> &curr_paid,
                      list<CheckIn::TPaidBagEMDItem> &curr_emds)
{
  bool modified=false;
  set< pair<int/*pax_id*/, string/*rfisc*/> > rfiscs;
  map< TTmpKey, int > tmp_paid;
  for(list<PieceConcept::TPaidBagItem>::const_iterator p=curr_paid.begin(); p!=curr_paid.end(); ++p)
    if (p->status==bsUnknown ||
        p->status==bsPaid ||
        p->status==bsNeed)
    {
      rfiscs.insert(make_pair(p->pax_id, p->RFISC));
      TTmpKey key(*p);
      map< TTmpKey, int >::iterator i=tmp_paid.find(key);
      if (i!=tmp_paid.end())
        i->second++;
      else
        tmp_paid.insert(make_pair(key, 1));
    };

  map< TTmpKey, int > tmp_emds;
  for(list<CheckIn::TPaidBagEMDItem>::iterator e=curr_emds.begin(); e!=curr_emds.end();)
    if (!e->rfisc.empty() && rfiscs.find(make_pair(e->pax_id, e->rfisc))==rfiscs.end())
    {
      e=curr_emds.erase(e);
      modified=true;
    }
    else
    {
      TTmpKey key(*e);
      map< TTmpKey, int >::iterator i=tmp_emds.find(key);
      if (i!=tmp_emds.end())
        i->second++;
      else
        tmp_emds.insert(make_pair(key, 1));
      ++e;
    };

  for(map< TTmpKey, int >::const_iterator e=tmp_emds.begin(); e!=tmp_emds.end(); ++e)
  {
    map< TTmpKey, int >::const_iterator p=tmp_paid.find(e->first);
    if (p==tmp_paid.end() || e->second>p->second)
      throw UserException("MSG.EXCESS_EMD_ATTACHED_FOR_BAG_TYPE",
                          LParams() << LParam("bag_type", e->first.rfisc));
  }

  return modified;
}

bool UpdatePaidBag(const CheckIn::TPaidBagEMDItem &emd,
                   list<TPaidBagItem> &paid_bag,
                   bool only_check)
{
  for(bool pr_cabin=only_check?true:false; ;pr_cabin=!pr_cabin)
  {
    for(list<TPaidBagItem>::iterator p=paid_bag.begin(); p!=paid_bag.end(); ++p)
    {
      if (p->pax_id!=ASTRA::NoExists && p->pax_id==emd.pax_id &&
          p->trfer_num!=ASTRA::NoExists && p->trfer_num==emd.trfer_num &&
          !p->RFISC.empty() && p->RFISC==emd.rfisc &&
          p->status==bsNeed &&
          (only_check || p->pr_cabin==pr_cabin))
      {
        if (!only_check) p->status=bsPaid;
        return true;
      };
    };
    if (pr_cabin) break;
  };
  return false;
}

bool Confirmed(const CheckIn::TPaidBagEMDItem &emd,
               const boost::optional< list<CheckIn::TPaidBagEMDItem> > &confirmed_emd)
{
  if (!confirmed_emd) return true;
  if (emd.trfer_num!=0) return true;
  for(list<CheckIn::TPaidBagEMDItem>::const_iterator i=confirmed_emd.get().begin(); i!=confirmed_emd.get().end(); ++i)
    if (emd.emd_no==i->emd_no &&
        emd.emd_coupon==i->emd_coupon &&
        emd.pax_id==i->pax_id) return true;
  return false;
}

bool TryAddPaidBagEMD(list<TPaidBagItem> &paid_bag,
                      list<CheckIn::TPaidBagEMDItem> &paid_bag_emd,
                      const CheckIn::TPaidBagEMDProps &paid_bag_emd_props,
                      const boost::optional< list<CheckIn::TPaidBagEMDItem> > &confirmed_emd)
{
  ProgTrace(TRACE5, "%s started", __FUNCTION__);

  if (confirmed_emd)
  {
    ostringstream s;
    for(list<CheckIn::TPaidBagEMDItem>::const_iterator i=confirmed_emd.get().begin(); i!=confirmed_emd.get().end(); ++i)
    {
      if (i!=confirmed_emd.get().begin()) s << ", ";
      s << i->no_str();
    }
    ProgTrace(TRACE5, "%s: confirmed_emd=%s", __FUNCTION__, s.str().c_str());
  };


  bool result=false;

  class TAddedEmdItem
  {
    public:
      string rfisc;
      string emd_no_base;
      int continuous_segs;
      bool manual_bind;
      list<CheckIn::TPaidBagEMDItem> coupons;

      TAddedEmdItem(): continuous_segs(0), manual_bind(false) {}
      TAddedEmdItem(const string& _rfisc, const string& _emd_no_base):
        rfisc(_rfisc), emd_no_base(_emd_no_base), continuous_segs(0), manual_bind(false) {}

      bool operator < (const TAddedEmdItem &item) const
      {
        if (continuous_segs!=item.continuous_segs)
          return continuous_segs<item.continuous_segs;
        if (manual_bind!=item.manual_bind)
          return manual_bind;
        return coupons.size()>item.coupons.size();
      }
      bool empty() const
      {
        return rfisc.empty() || emd_no_base.empty() || continuous_segs==0 || coupons.empty();
      }
      string traceStr() const
      {
        ostringstream s;
        s << "rfisc=" << rfisc << ", "
             "emd_no_base=" << emd_no_base << ", "
             "continuous_segs=" << continuous_segs << ", "
             "manual_bind=" << (manual_bind?"true":"false") << ", "
             "coupons=";
        for(list<CheckIn::TPaidBagEMDItem>::const_iterator i=coupons.begin(); i!=coupons.end(); ++i)
        {
          if (i!=coupons.begin()) s << ", ";
          s << i->no_str();
        };
        return s.str();
      }
  };

  class TBaseEmdMap : public map< string/*emd_no_base*/, set<string/*emd_no*/> >
  {
    public:
      string rfisc;
      TBaseEmdMap(const string& _rfisc) : rfisc(_rfisc) {}
      void add(const TPaxEMDItem& emd)
      {
        if (emd.RFISC!=rfisc) return;
        string emd_no_base=emd.emd_no_base.empty()?emd.emd_no:emd.emd_no_base;
        pair< TBaseEmdMap::iterator, bool > res=insert(make_pair(emd_no_base, set<string>()));
        if (res.first==end()) throw Exception("%s: res.first==end()!", __FUNCTION__);
        res.first->second.insert(emd.emd_no);
      }
      string traceStr() const
      {
        ostringstream s;
        for(TBaseEmdMap::const_iterator i=begin(); i!=end(); ++i)
        {
          if (i!=begin()) s << ", ";
          for(set<string>::const_iterator j=i->second.begin(); j!=i->second.end(); ++j)
          {
            if (j!=i->second.begin()) s << "/";
            s << *j;
          }
        }
        return s.str();
      }
  };

  typedef map<int/*pax_id*/, map<string/*RFISC*/, int/*кол-во*/> > TNeedMap;
  TNeedMap need;
  int max_trfer_num=0;
  for(list<TPaidBagItem>::iterator p=paid_bag.begin(); p!=paid_bag.end(); ++p)
  {
    if (max_trfer_num<p->trfer_num) max_trfer_num=p->trfer_num;
    if (p->RFISC.empty() || p->status!=bsNeed) continue;
    TNeedMap::iterator i=need.find(p->pax_id);
    if (i==need.end())
      i=need.insert(make_pair(p->pax_id, map<string, int>())).first;
    if (i==need.end()) throw Exception("%s: i==need.end()!", __FUNCTION__);
    pair<map<string/*RFISC*/, int/*кол-во*/>::iterator, bool> res=i->second.insert(make_pair(p->RFISC,0));
    if (res.first==i->second.end()) throw Exception("%s: res.first==i->second.end()!", __FUNCTION__);
    res.first->second++;
  };

  for(TNeedMap::iterator i=need.begin(); i!=need.end(); ++i)
  {
    multiset<TPaxEMDItem> emds;
    GetPaxEMD(i->first, emds);
    if (emds.empty()) continue; //по пассажиру нет ничего
    for(map<string, int>::iterator irfisc=i->second.begin(); irfisc!=i->second.end(); ++irfisc)
    {
      TBaseEmdMap base_emds(irfisc->first);
      for(multiset<TPaxEMDItem>::const_iterator e=emds.begin(); e!=emds.end(); ++e) base_emds.add(*e);

      ProgTrace(TRACE5, "%s: pax_id=%d, rfisc=%s(%d), emds: %s",
                __FUNCTION__, i->first, irfisc->first.c_str(), irfisc->second, base_emds.traceStr().c_str());

      for(int initial_trfer_num=0; initial_trfer_num<=max_trfer_num && irfisc->second>0; initial_trfer_num++)
      {
        for(;irfisc->second>0;)
        {
          TAddedEmdItem best_added;

          for(TBaseEmdMap::const_iterator be=base_emds.begin(); be!=base_emds.end(); ++be)
          {
            TAddedEmdItem curr_added(base_emds.rfisc, be->first);

            for(;curr_added.continuous_segs<=max_trfer_num;curr_added.continuous_segs++)
            {
              {
                //ищем среди привязанных
                list<CheckIn::TPaidBagEMDItem>::const_iterator e=paid_bag_emd.begin();
                for(; e!=paid_bag_emd.end(); ++e)
                  if (e->rfisc==curr_added.rfisc &&
                      e->trfer_num==curr_added.continuous_segs &&
                      e->pax_id==i->first &&
                      be->second.find(e->emd_no)!=be->second.end()) break;
                if (curr_added.continuous_segs< initial_trfer_num && e==paid_bag_emd.end()) continue;
                if (curr_added.continuous_segs>=initial_trfer_num && e!=paid_bag_emd.end()) continue;
              }
              {
                //ищем среди непривязанных
                multiset<TPaxEMDItem>::const_iterator e=emds.begin();
                for(; e!=emds.end(); ++e)
                  if (e->RFISC==curr_added.rfisc &&
                      e->trfer_num==curr_added.continuous_segs &&
                      be->second.find(e->emd_no)!=be->second.end()) break;
                if (curr_added.continuous_segs< initial_trfer_num && e==emds.end()) continue;
                if (curr_added.continuous_segs>=initial_trfer_num && e!=emds.end())
                {
                  CheckIn::TPaidBagEMDItem item;
                  item.rfisc=e->RFISC;
                  item.trfer_num=e->trfer_num;
                  item.emd_no=e->emd_no;
                  item.emd_coupon=e->emd_coupon;
                  item.weight=0;
                  item.pax_id=i->first;

                  if (Confirmed(item, confirmed_emd) &&
                      UpdatePaidBag(item, paid_bag, true))
                  {
                    if (paid_bag_emd_props.get(e->emd_no, e->emd_coupon).manual_bind)
                      curr_added.manual_bind=true;
                    else
                    {
                      curr_added.coupons.push_back(item);
                      continue;
                    }
                  };
                }
              }
              break; //сюда дошли - значит к данному сегменту не можем привязать
            }

            if (curr_added.coupons.empty()) continue;
            if (best_added<curr_added) best_added=curr_added;
          }; //emd_no

          if (best_added.empty()) break; //ни одного EMD из непривязанных не можем привязать по текущему RFISC

          ProgTrace(TRACE5, "%s: pax_id=%d, %s", __FUNCTION__, i->first, best_added.traceStr().c_str());

          for(list<CheckIn::TPaidBagEMDItem>::const_iterator e=best_added.coupons.begin(); e!=best_added.coupons.end(); ++e)
          {
            if (!UpdatePaidBag(*e, paid_bag, false)) throw Exception("%s: UpdatePaidBag strange situation!", __FUNCTION__);
            paid_bag_emd.push_back(*e);
            irfisc->second--;
            result=true;
          }
          base_emds.erase(best_added.emd_no_base);
        }
      }; //initial_trfer_num
    } //rfisc
  }

  return result;
}

class TPiadBagViewKey
{
  public:
    std::string RFISC;
    int trfer_num;
    TPiadBagViewKey(const TPaidBagItem &item)
    {
      RFISC=item.RFISC;
      trfer_num=item.trfer_num;
    }
    bool operator < (const TPiadBagViewKey &key) const
    {
      if (trfer_num!=key.trfer_num)
        return trfer_num<key.trfer_num;
      return RFISC<key.RFISC;
    }
};

class TPiadBagViewItem : public TPiadBagViewKey
{
  public:
    int bag_number;
    int pieces;
    int emd_pieces;
    TPiadBagViewItem(const TPaidBagItem &item) : TPiadBagViewKey(item)
    {
      bag_number=0;
      pieces=0;
      emd_pieces=0;
      add(item);
    }
    void add(const TPaidBagItem &item)
    {
      if (RFISC!=item.RFISC ||
          trfer_num!=item.trfer_num) return;
      bag_number++;
      if (item.status==bsUnknown ||
          item.status==bsPaid ||
          item.status==bsNeed) pieces++;
      if (item.status==bsPaid) emd_pieces++;
    }
    const TPiadBagViewItem& toXML(xmlNodePtr node,
                                  const TTrferRoute &trfer,
                                  const TRFISCList &rfisc_list) const;
};

const TPiadBagViewItem& TPiadBagViewItem::toXML(xmlNodePtr node,
                                                const TTrferRoute &trfer,
                                                const TRFISCList &rfisc_list) const
{
  if (node==NULL) return *this;

  xmlNodePtr rowNode=NewTextChild(node, "row");
  xmlNodePtr colNode;
  ostringstream s;
  //код и название RFISC
  s.str("");
  s << RFISC << ": " << lowerc(rfisc_list.localized_name(RFISC, TReqInfo::Instance()->desk.lang));
  colNode=NewTextChild(rowNode, "col", s.str());
  colNode=NewTextChild(rowNode, "col", bag_number);
  //номер рейса для сегмента
  s.str("");
  s << (trfer_num+1) << ": ";
  if (trfer_num<(int)trfer.size())
  {
    const TTrferRouteItem &item=trfer[trfer_num];
    s << ElemIdToCodeNative(etAirline, item.operFlt.airline)
      << setw(2) << setfill('0') << item.operFlt.flt_no
      << ElemIdToCodeNative(etSuffix, item.operFlt.suffix)
      << " " << ElemIdToCodeNative(etAirp, item.operFlt.airp);
  };

  colNode=NewTextChild(rowNode, "col", s.str());
  colNode=NewTextChild(rowNode, "col", pieces);
  if (pieces==0)
    SetProp(colNode, "font_style", "");
  else
  {
    if (pieces>emd_pieces)
    {
      SetProp(colNode, "font_color", "clInactiveAlarm");
      SetProp(colNode, "font_color_selected", "clInactiveAlarm");
    }
    else
    {
      SetProp(colNode, "font_color", "clInactiveBright");
      SetProp(colNode, "font_color_selected", "clInactiveBright");
    };
  };
  colNode=NewTextChild(rowNode, "col", emd_pieces);
  if (pieces==0 && emd_pieces==0)
    SetProp(colNode, "font_style", "");

  return *this;
}

class TSumPaidBagViewItem : public set<TPiadBagViewItem>
{
  public:
    const TSumPaidBagViewItem& toXML(xmlNodePtr node) const;
};

class TSumPaidBagViewMap : public map< string, TSumPaidBagViewItem >
{
  public:
    void add(const TPiadBagViewItem &item)
    {
      pair<TSumPaidBagViewMap::iterator, bool> i=insert(make_pair(item.RFISC, TSumPaidBagViewItem()));
      if (i.first==end()) throw Exception("%s: i.first==end()!", __FUNCTION__);
      i.first->second.insert(item);
    }
};

const TSumPaidBagViewItem& TSumPaidBagViewItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;

  if (empty()) return *this;
  int bag_number=0, pieces=begin()->pieces;
  bool different_pieces=false;
  ostringstream pieces_view;
  int trfer_num=0;
  for(TSumPaidBagViewItem::const_iterator i=begin(); i!=end();)
  {
    if (trfer_num>=i->trfer_num)
    {
      if (bag_number<i->bag_number) bag_number=i->bag_number;
      if (pieces!=i->pieces)
      {
        different_pieces=true;
        if (pieces<i->pieces) pieces=i->pieces;
      };
      if (!pieces_view.str().empty()) pieces_view << "/";
      pieces_view << i->pieces;
      if (trfer_num==i->trfer_num) trfer_num++;
      ++i;
    }
    else
    {
      different_pieces=true;
      if (!pieces_view.str().empty()) pieces_view << "/";
      pieces_view << "?";
      trfer_num++;
    };
  }

  if (!different_pieces)
  {
    pieces_view.str("");
    pieces_view << pieces;
  }

  xmlNodePtr rowNode=NewTextChild(node,"paid_bag");
  NewTextChild(rowNode, "bag_type", begin()->RFISC);
  NewTextChild(rowNode, "weight", pieces);
  NewTextChild(rowNode, "weight_calc", pieces);
  NewTextChild(rowNode, "rate_id");

  NewTextChild(rowNode, "bag_type_view", begin()->RFISC);
  NewTextChild(rowNode, "bag_number_view", bag_number);
  NewTextChild(rowNode, "weight_view", pieces_view.str());
  NewTextChild(rowNode, "weight_calc_view", pieces_view.str());

  return *this;
}

void PaidBagViewToXML(const TTrferRoute &trfer,
                      const std::list<TPaidBagItem> &paid,
                      const TRFISCList &rfisc_list,
                      xmlNodePtr node)
{
  if (trfer.empty()) throw Exception("%s: trfer.empty()", __FUNCTION__);
  map<TPiadBagViewKey, TPiadBagViewItem> paid_view;
  for(list<TPaidBagItem>::const_iterator p=paid.begin(); p!=paid.end(); p++)
  {
    TPiadBagViewKey key(*p);
    map<TPiadBagViewKey, TPiadBagViewItem>::iterator i=paid_view.find(key);
    if (i!=paid_view.end())
      i->second.add(*p);
    else
      paid_view.insert(make_pair(key, TPiadBagViewItem(*p)));

  };

  const string taLeftJustify="taLeftJustify";  //!!!vlad в astra_consts
  const string taRightJustify="taRightJustify";
  const string taCenter="taCenter";
  const string fsBold="fsBold";

  xmlNodePtr paidNode=NewTextChild(node, "paid_bags");

  xmlNodePtr paidViewNode=NewTextChild(node, "paid_bag_view");
  SetProp(paidViewNode, "font_size", 8);

  xmlNodePtr colNode;
  //секция описывающая столбцы
  xmlNodePtr colsNode=NewTextChild(paidViewNode, "cols");
  colNode=NewTextChild(colsNode, "col");
  SetProp(colNode, "width", 140);
  SetProp(colNode, "align", taLeftJustify);
  colNode=NewTextChild(colsNode, "col");
  SetProp(colNode, "width", 30);
  SetProp(colNode, "align", taCenter);
  colNode=NewTextChild(colsNode, "col");
  SetProp(colNode, "width", 80);
  SetProp(colNode, "align", taLeftJustify);
  colNode=NewTextChild(colsNode, "col");
  SetProp(colNode, "width", 40);
  SetProp(colNode, "align", taRightJustify);
  SetProp(colNode, "font_style", fsBold);
  colNode=NewTextChild(colsNode, "col");
  SetProp(colNode, "width", 40);
  SetProp(colNode, "align", taRightJustify);
  SetProp(colNode, "font_style", fsBold);

  //секция описывающая заголовок
  xmlNodePtr headerNode=NewTextChild(paidViewNode, "header");
  SetProp(headerNode, "font_size", 10);
  SetProp(headerNode, "font_style", "");
  SetProp(headerNode, "align", taLeftJustify);
  NewTextChild(headerNode, "col", getLocaleText("RFISC"));
  NewTextChild(headerNode, "col", getLocaleText("Кол."));
  NewTextChild(headerNode, "col", getLocaleText("Сегмент"));
  NewTextChild(headerNode, "col", getLocaleText("К опл."));
  NewTextChild(headerNode, "col", getLocaleText("Оплачено"));

  xmlNodePtr rowsNode = NewTextChild(paidViewNode, "rows");

  TSumPaidBagViewMap paid_view_sum;
  for(map<TPiadBagViewKey, TPiadBagViewItem>::const_iterator i=paid_view.begin(); i!=paid_view.end(); ++i)
  {
    paid_view_sum.add(i->second);
    i->second.toXML(rowsNode, trfer, rfisc_list);
  };
  for(TSumPaidBagViewMap::const_iterator i=paid_view_sum.begin(); i!=paid_view_sum.end(); ++i)
    i->second.toXML(paidNode);
}


//<... color=... color_selected=... font_color=... font_color_selected=... font_style=... font_size=... align=...> //секция таблицы
//  <cols>  //секция описания столбцов
//    </col width=... color=... color_selected=... font_color=... font_color_selected=... font_style=... font_size=... align=...>
//    ...
//    ...
//  </cols>
//  <header height=... color=... color_selected=... font_color=... font_color_selected=... font_style=... font_size=... align=...>  //секция заголовка
//    <col color=... color_selected=... font_color=... font_color_selected=... font_style=... font_size=... align=...>текст</col>
//    ...
//    ...
//  </header>
//  <rows>  //секция данных
//    <row height=... color=... color_selected=... font_color=... font_color_selected=... font_style=... font_size=... align=...> //секция строки
//      <col color=... color_selected=... font_color=... font_color_selected=... font_style=... font_size=... align=...>текст</col> //секция ячейки
//      ...
//      ...
//    </row>
//    ...
//    ...
//  </rows>
//</...>

} //namespace PieceConcept

namespace SirenaExchange
{

const std::string TAvailability::id="svc_availability";
const std::string TPaymentStatus::id="svc_payment_status";
const std::string TGroupInfo::id="group_svc_info";
const std::string TPassengers::id="passenger_with_svc";

string airlineToXML(const std::string &code, const std::string &lang)
{
  string result;
  result=ElemIdToPrefferedElem(etAirline, code, efmtCodeNative, lang);
  if (result.size()==3) //типа ИКАО
    result=ElemIdToPrefferedElem(etAirline, code, efmtCodeNative, lang==LANG_EN?LANG_RU:LANG_EN);
  return result;
}

string airpToXML(const std::string &code, const std::string &lang)
{
  string result;
  result=ElemIdToPrefferedElem(etAirp, code, efmtCodeNative, lang);
  if (result.size()==4) //типа ИКАО
    result=ElemIdToPrefferedElem(etAirp, code, efmtCodeNative, lang==LANG_EN?LANG_RU:LANG_EN);
  return result;
}

const TSegItem& TSegItem::toXML(xmlNodePtr node, const std::string &lang) const
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
  SetProp(node, "equipment", craft, "");

  return *this;
}

const TPaxSegItem& TPaxSegItem::toXML(xmlNodePtr node, const std::string &lang) const
{
  if (node==NULL) return *this;

  TSegItem::toXML(node, lang);
  SetProp(node, "subclass", ElemIdToPrefferedElem(etSubcls, subcl, efmtCodeNative, lang));
  SetProp(node, "coupon_num", tkn.coupon, ASTRA::NoExists);
  if (!tkn.no.empty())
  {
    xmlNodePtr tknNode=NewTextChild(node, "ticket");
    SetProp(tknNode, "number", tkn.no);
    SetProp(tknNode, "coupon_num", tkn.coupon, ASTRA::NoExists);
  }
  for(list<CheckIn::TPnrAddrItem>::const_iterator i=pnrs.begin(); i!=pnrs.end(); ++i)
    SetProp(NewTextChild(node, "recloc", i->addr), "crs", airlineToXML(i->airline, lang));
  for(vector<CheckIn::TPaxFQTItem>::const_iterator i=fqts.begin(); i!=fqts.end(); ++i)
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

const TPaxItem& TPaxItem::toXML(xmlNodePtr node, const std::string &lang) const
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
    i->second.toXML(NewTextChild(node, "segment"), lang);

  return *this;
}

std::string TPaxItem::sex() const
{
  int f=CheckIn::is_female(doc.gender, name);
  if (f==ASTRA::NoExists) return "";
  return (f==0?"male":"female");
}

const TPaxItem2& TPaxItem2::toXML(xmlNodePtr node, const std::string &lang) const
{
  if (node==NULL) return *this;

  NewTextChild( node, "surname", surname );
  NewTextChild( node, "name", name );
  NewTextChild( node, "category", category());
  NewTextChild( node, "group_id", grp_id );
  NewTextChild( node, "reg_no", reg_no );
  return *this;
}

TPaxItem2& TPaxItem2::fromDB(TQuery &Qry)
{
  clear();
  surname = Qry.FieldAsString( "surname" );
  name = Qry.FieldAsString( "name" );
  pers_type = DecodePerson( Qry.FieldAsString( "pers_type" ) );
  seats = Qry.FieldAsInteger( "seats" );
  reg_no = Qry.FieldAsInteger( "reg_no" );
  grp_id = Qry.FieldAsInteger( "grp_id" );
  return *this;
}

const TBagItem& TBagItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;

  SetProp(node, "rfisc", RFISC);
  SetProp(node, "rfic", RFIC, "");
  SetProp(node, "emd_type", emd_type, "");
  SetProp(node, "paid", status==PieceConcept::bsPaid?"true":"false", "false");
  SetProp(node, "hand_baggage", pr_cabin?"true":"false", "false");

  return *this;
}

TBagItem& TBagItem::fromXML(xmlNodePtr node)
{
  clear();

  try
  {
    if (node==NULL) throw Exception("node not defined");

    RFISC=NodeAsString("@rfisc", node, "");
    if (RFISC.empty()) throw Exception("Empty @rfisc");
    if (RFISC.size()>15) throw Exception("Wrong @rfisc='%s'", RFISC.c_str());

    string xml_status=NodeAsString("@payment_status", node, "");
    if (xml_status.empty()) throw Exception("Empty @payment_status");
    status=PieceConcept::DecodeBagStatus(xml_status);
    if (status==PieceConcept::bsNone)
      throw Exception("Wrong @payment_status='%s'", xml_status.c_str());

    if (NodeIsNULL("@hand_baggage", node, false))
      throw Exception("Empty @hand_baggage");
    pr_cabin=NodeAsBoolean("@hand_baggage", node, false);
  }
  catch(Exception &e)
  {
    throw Exception("TBagItem::fromXML: %s", e.what());
  };
  return *this;
}

const TPaxSegKey& TPaxSegKey::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;

  if (pax_id!=ASTRA::NoExists)
    SetProp(node, "passenger-id", pax_id);
  if (seg_id!=ASTRA::NoExists)
    SetProp(node, "segment-id", seg_id);
  return *this;
}

TPaxSegKey& TPaxSegKey::fromXML(xmlNodePtr node)
{
  clear();

  try
  {
    if (node==NULL) throw Exception("node not defined");

    pax_id=NodeAsInteger("@passenger-id", node);
    seg_id=NodeAsInteger("@segment-id", node);
  }
  catch(Exception &e)
  {
    throw Exception("TPaxSegKey::fromXML: %s", e.what());
  };
  return *this;
}

/*void TExchange::build(std::string &content) const
{
  try
  {
    XMLDoc doc(isRequest()?"query":"answer");
    xmlNodePtr node=NewTextChild(NodeAsNode(isRequest()?"/query":"/answer", doc.docPtr()), exchangeId().c_str());
    if (error())
      errorToXML(node);
    else
      toXML(node);
    content = ConvertCodepage( XMLTreeToText( doc.docPtr() ), "CP866", "UTF-8" );
  }
  catch(Exception &e)
  {
    throw Exception("TExchange::build: %s", e.what());
  };
}

void TExchange::parse(const std::string &content)
{
  try
  {
    XMLDoc doc(content);
    if (doc.docPtr()==NULL)
    {
      if (content.size()>500)
        throw Exception("Wrong XML %s...", content.substr(0,500).c_str());
      else
        throw Exception("Wrong XML %s", content.c_str());
    };
    xml_decode_nodelist(doc.docPtr()->children);
    xmlNodePtr node=NodeAsNode(isRequest()?"/query":"/answer", doc.docPtr());
    node=NodeAsNode(exchangeId().c_str(), node);
    errorFromXML(node);
    if (!error())
      fromXML(node);
  }
  catch(Exception &e)
  {
    throw Exception("TExchange::parse: %s", e.what());
  };
}

void TExchange::toXML(xmlNodePtr node) const
{
  throw Exception("TExchange::toXML: %s <%s> not implemented", isRequest()?"Request":"Response", exchangeId().c_str());
}

void TExchange::fromXML(xmlNodePtr node)
{
  throw Exception("TExchange::fromXML: %s <%s> not implemented", isRequest()?"Request":"Response", exchangeId().c_str());
}

void TErrorReference::toXML(xmlNodePtr node) const
{
  if (node==NULL) return;

  xmlNodePtr refNode=NewTextChild(node, "reference");
  SetProp(refNode, "path", path, "");
  SetProp(refNode, "value", value, "");
  SetProp(refNode, "passenger_id", pax_id, ASTRA::NoExists);
  SetProp(refNode, "segment_id", seg_id, ASTRA::NoExists);
}

void TErrorReference::fromXML(xmlNodePtr node)
{
  clear();

  xmlNodePtr refNode=GetNode("reference", node);

  path=NodeAsString("@path", refNode, "");
  value=NodeAsString("@value", refNode, "");
  pax_id=NodeAsInteger("@passenger_id", refNode, ASTRA::NoExists);
  seg_id=NodeAsInteger("@segment_id", refNode, ASTRA::NoExists);
}

std::string TErrorReference::traceStr() const
{
  ostringstream s;
  if (!path.empty())
    s << " path='" << path << "'";
  if (!value.empty())
    s << " value='" << value << "'";
  if (pax_id!=ASTRA::NoExists)
    s << " pax_id=" << pax_id;
  if (seg_id!=ASTRA::NoExists)
    s << " seg_id=" << seg_id;
  return s.str();
}*/

/*void TExchange::errorToXML(xmlNodePtr node) const
{
  if (node==NULL) return;

  xmlNodePtr errNode=NewTextChild(node, "error");
  SetProp(errNode, "code", error_code);
  SetProp(errNode, "message", error_message);
  if (!error_reference.empty())
    error_reference.toXML(node);
}

void TExchange::errorFromXML(xmlNodePtr node)
{
  error_code.clear();
  error_message.clear();
  error_reference.clear();

  xmlNodePtr errNode=GetNode("error", node);
  if (errNode==NULL) return;

  error_code=NodeAsString("@code", errNode, "");
  error_message=NodeAsString("@message", errNode, "");
  error_reference.fromXML(errNode);
}

bool TExchange::error() const
{
  return !error_code.empty() ||
         !error_message.empty() ||
         !error_reference.empty();
}

std::string TExchange::traceError() const
{
  ostringstream s;
  if (!error_code.empty())
    s << " code=" << error_code;
  if (!error_message.empty())
    s << " message='" << error_message << "'";
  if (!error_reference.empty())
    s << error_reference.traceStr();
  return s.str();
}*/

void TAvailabilityReq::toXML(xmlNodePtr node) const
{
  if (node==NULL) return;

  if (paxs.empty()) throw Exception("TAvailabilityReq::toXML: paxs.empty()");

  SetProp(node, "show_brand_info", "true");

  for(list<TPaxItem>::const_iterator p=paxs.begin(); p!=paxs.end(); ++p)
    p->toXML(NewTextChild(node, "passenger"), LANG_EN);
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
      TPaxSegKey key;
      key.fromXML(node);
      if (find(key)!=end())
        throw Exception("Duplicate passenger-id=%d segment-id=%d", key.pax_id, key.seg_id);
      TAvailabilityResItem &item=insert(make_pair(key, TAvailabilityResItem())).first->second;
      item.rfisc_list.fromXML(node);
      item.norm.fromXMLAdv(node, item.concept, item.rfisc_list.airline);
      item.brand.fromXMLAdv(node);
    };
    if (empty()) throw Exception("empty()");
  }
  catch(Exception &e)
  {
    throw Exception("TAvailabilityRes::fromXML: %s", e.what());
  };
}

bool TAvailabilityRes::identical_concept(int seg_id, boost::optional<TBagConcept> &concept) const
{
  concept=boost::none;
  for(TAvailabilityResMap::const_iterator i=begin(); i!=end(); ++i)
  {
    if (i->first.seg_id!=seg_id) continue;
    if (!concept)
      concept=i->second.concept;
    else
    {
      if (concept.get()!=i->second.concept)
      {
        concept=boost::none;
        return false;
      };
    };
  };
  return true;
}

bool TAvailabilityRes::identical_rfisc_list(int seg_id, boost::optional<PieceConcept::TRFISCList> &rfisc_list) const
{
  rfisc_list=boost::none;
  for(TAvailabilityResMap::const_iterator i=begin(); i!=end(); ++i)
  {
    if (i->first.seg_id!=seg_id) continue;
    if (!rfisc_list)
      rfisc_list=i->second.rfisc_list;
    else
    {
      if (rfisc_list.get()!=i->second.rfisc_list)
      {
        rfisc_list=boost::none;
        return false;
      }
    };
  };
  return true;
}

void TAvailabilityRes::normsToDB(const TCkinGrpIds &tckin_grp_ids) const
{
  list<PieceConcept::TPaxNormItem> normsList;
  for(TAvailabilityResMap::const_iterator i=begin(); i!=end(); ++i)
  {
    if (i->second.norm.empty()) continue;
    PieceConcept::TPaxNormItem item;
    static_cast<PieceConcept::TSimplePaxNormItem&>(item)=i->second.norm;
    item.pax_id=i->first.pax_id;
    item.trfer_num=i->first.seg_id;
    normsList.push_back(item);
  }
  PieceConcept::PaxNormsToDB(tckin_grp_ids, normsList);
}

void TAvailabilityRes::brandsToDB(const TCkinGrpIds &tckin_grp_ids) const
{
  list<PieceConcept::TPaxBrandItem> brandsList;
  for(TAvailabilityResMap::const_iterator i=begin(); i!=end(); ++i)
  {
    if (i->second.brand.empty()) continue;
    PieceConcept::TPaxBrandItem item;
    static_cast<PieceConcept::TSimplePaxBrandItem&>(item)=i->second.brand;
    item.pax_id=i->first.pax_id;
    item.trfer_num=i->first.seg_id;
    brandsList.push_back(item);
  }
  PieceConcept::PaxBrandsToDB(tckin_grp_ids, brandsList);
}

void TPaymentStatusReq::toXML(xmlNodePtr node) const
{
  if (node==NULL) return;

  if (paxs.empty()) throw Exception("TPaymentStatusReq::toXML: paxs.empty()");

  for(list<TPaxItem>::const_iterator p=paxs.begin(); p!=paxs.end(); ++p)
    p->toXML(NewTextChild(node, "passenger"), LANG_EN);

  if (bags.empty()) throw Exception("TPaymentStatusReq::toXML: bags.empty()");

  for(list< pair<TPaxSegKey, TBagItem> >::const_iterator b=bags.begin(); b!=bags.end(); ++b)
  {
    xmlNodePtr svcNode=NewTextChild(node, "svc");
    b->first.toXML(svcNode);
    b->second.toXML(svcNode);
  }
}

void TPaymentStatusRes::fromXML(xmlNodePtr node)
{
  clear();

  try
  {
    if (node==NULL) throw Exception("node not defined");
    for(node=node->children; node!=NULL; node=node->next)
    {
      if (string((char*)node->name)!="svc" &&
          string((char*)node->name)!="free_baggage_norm") continue;
      TPaxSegKey key;
      key.fromXML(node);
      if (string((char*)node->name)=="svc")
      {
        pair<TPaxSegKey, TBagItem> &item=
          *(bags.insert(bags.end(), make_pair(TPaxSegKey(), TBagItem())));
        item.first.fromXML(node);
        item.second.fromXML(node);
      };
      if (string((char*)node->name)=="free_baggage_norm")
      {
        pair<TPaxSegKey, PieceConcept::TSimplePaxNormItem> &item=
          *(norms.insert(norms.end(), make_pair(TPaxSegKey(), PieceConcept::TSimplePaxNormItem())));
        item.first.fromXML(node);
        item.second.fromXML(node);
      };
    };
    if (bags.empty()) throw Exception("bags.empty()");
    //if (norms.empty()) throw Exception("norms.empty()"); а надо ли?
  }
  catch(Exception &e)
  {
    throw Exception("TPaymentStatusRes::fromXML: %s", e.what());
  };
}

void TPaymentStatusRes::normsToDB(const TCkinGrpIds &tckin_grp_ids) const
{
  list<PieceConcept::TPaxNormItem> normsList;
  for(TPaxNormList::const_iterator i=norms.begin(); i!=norms.end(); ++i)
  {
    PieceConcept::TPaxNormItem item;
    static_cast<PieceConcept::TSimplePaxNormItem&>(item)=i->second;
    item.pax_id=i->first.pax_id;
    item.trfer_num=i->first.seg_id;
    normsList.push_back(item);
  }
  PieceConcept::PaxNormsToDB(tckin_grp_ids, normsList);
}

void TPaymentStatusRes::convert(list<PieceConcept::TPaidBagItem> &paid) const
{
  paid.clear();
  for(TBagList::const_iterator i=bags.begin(); i!=bags.end(); ++i)
  {
    PieceConcept::TPaidBagItem item;
    item.pax_id=i->first.pax_id;
    item.trfer_num=i->first.seg_id;
    item.RFISC=i->second.RFISC;
    item.status=i->second.status;
    item.pr_cabin=i->second.pr_cabin;
    paid.push_back(item);
  }
}

void TPaymentStatusRes::check_unknown_status(int seg_id, set<string> &rfiscs) const
{
  rfiscs.clear();
  for(TBagList::const_iterator i=bags.begin(); i!=bags.end(); ++i)
  {
    if (i->first.seg_id!=seg_id) continue;
    if (i->second.status==PieceConcept::bsUnknown)
        rfiscs.insert(i->second.RFISC);
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

    if ( BASIC::StrToInt( str.c_str(), flt_no ) == EOF ||
         flt_no > 99999 || flt_no <= 0 ) throw Exception("Wrong flight number <flight> '%s'", flight.c_str());

    str=suffix;
    if (!str.empty())
    {
      suffix = ElemToElemId( etSuffix, str, suffix_fmt );
      if (suffix_fmt==efmtUnknown) throw Exception("Unknown flight suffix <flight> '%s'", flight.c_str());
    };

    str=NodeAsString("departure_date", node);
    if (str.empty()) throw Exception("Empty <departure_date>");
    if ( BASIC::StrToDateTime(str.c_str(), "yyyy-mm-dd", scd_out) == EOF )
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
    p->toXML(NewTextChild(node, "passenger"), LANG_EN);
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
    p->toXML(NewTextChild(node, "passenger"), LANG_EN);

  if (bags.empty()) throw Exception("TGroupInfoRes::toXML: bags.empty()");

  for(list< pair<TPaxSegKey, TBagItem> >::const_iterator b=bags.begin(); b!=bags.end(); ++b)
  {
    xmlNodePtr svcNode=NewTextChild(node, "svc");
    b->first.toXML(svcNode);
    b->second.toXML(svcNode);
  }
}

/*void traceXML(const string& xml)
{
  size_t len=xml.size();
  int portion=4000;
  for(size_t pos=0; pos<len; pos+=portion)
    ProgTrace(TRACE5, "%s", xml.substr(pos,portion).c_str());
}

void SendRequest(const TExchange &request, TExchange &response,
                 RequestInfo &requestInfo, ResponseInfo &responseInfo)
{
  requestInfo.host = SIRENA_HOST();
  requestInfo.port = SIRENA_PORT();
  requestInfo.path = "/astra";
  request.build(requestInfo.content);
  requestInfo.using_ssl = false;
  requestInfo.timeout = SIRENA_REQ_TIMEOUT();
  int request_count = SIRENA_REQ_ATTEMPTS();
  traceXML(requestInfo.content);
  for(int pass=0; pass<request_count; pass++)
  {
    httpClient_main(requestInfo, responseInfo);
    if (!(!responseInfo.completed && responseInfo.error_code==boost::asio::error::eof && responseInfo.error_operation==toStatus))
      break;
    ProgTrace( TRACE5, "SIRENA DEADED, next request!" );
  };
  if (!responseInfo.completed ) throw Exception("%s: responseInfo.completed()=false", __FUNCTION__);

  xmlDocPtr doc=TextToXMLTree(responseInfo.content);
  if (doc!=NULL)
    traceXML(XMLTreeToText(doc));
  else
    traceXML(responseInfo.content);
  response.parse(responseInfo.content);
  if (response.error()) throw Exception("SIRENA ERROR: %s", response.traceError().c_str());
}

void SendRequest(const TExchange &request, TExchange &response)
{
  RequestInfo requestInfo;
  ResponseInfo responseInfo;
  SendRequest(request, response, requestInfo, responseInfo);
}*/

void fillPaxsBags(int first_grp_id, TExchange &exch, bool &pr_unaccomp, TCkinGrpIds &tckin_grp_ids);

/*void TLastExchangeInfo::toDB()
{
  if (grp_id==ASTRA::NoExists) return;
  AstraContext::ClearContext("pc_payment_req", grp_id);
  AstraContext::SetContext("pc_payment_req", grp_id, ConvertCodepage(pc_payment_req, "UTF-8", "CP866"));
  pc_payment_req_created=NowUTC();
  AstraContext::ClearContext("pc_payment_res", grp_id);
  AstraContext::SetContext("pc_payment_res", grp_id, ConvertCodepage(pc_payment_res, "UTF-8", "CP866"));
  pc_payment_res_created=NowUTC();
}

void TLastExchangeInfo::fromDB(int grp_id)
{
  clear();
  if (grp_id==ASTRA::NoExists) return;
  pc_payment_req_created=AstraContext::GetContext("pc_payment_req", grp_id, pc_payment_req);
  pc_payment_res_created=AstraContext::GetContext("pc_payment_res", grp_id, pc_payment_res);
  pc_payment_req=ConvertCodepage(pc_payment_req, "CP866", "UTF-8");
  pc_payment_res=ConvertCodepage(pc_payment_res, "CP866", "UTF-8");
}

void TLastExchangeInfo::cleanOldRecords()
{
  TDateTime d=NowUTC()-15/1440.0;
  AstraContext::ClearContext("pc_payment_req", d);
  AstraContext::ClearContext("pc_payment_res", d);
}

void TLastExchangeList::handle(const string& where)
{
  for(TLastExchangeList::iterator i=begin(); i!=end(); ++i)
    i->toDB();
}*/

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
      SirenaExchange::TPassengersReq req1;
      SirenaExchange::TPassengersRes res1;
      req1.fromXML(reqNode);
      procPassengers( req1, res1 );
      res1.toXML(resNode);
    }
    else if (exchangeId==SirenaExchange::TGroupInfo::id)
    {
      SirenaExchange::TGroupInfoReq req2;
      SirenaExchange::TGroupInfoRes res2;
      req2.fromXML(reqNode);
      procGroupInfo( req2, res2 );
      res2.toXML(resNode);
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
  list<TAdvTripInfo> flts;
  set<int> marketing_point_ids;
  SearchFlt( fltInfo, flts);
  SearchMktFlt( fltInfo, marketing_point_ids );
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT pax.pax_id, pax.name, pax.surname, pax_grp.grp_id, pax.pers_type, pax.seats, pax.reg_no "
    "FROM pax, pax_grp "
    "WHERE pax_grp.grp_id=pax.grp_id AND "
    "      point_dep=:point_id AND "
    "      pr_brd IS NOT NULL AND pax_grp.status NOT IN ('E') AND "
    "      piece_concept<>0 AND  "
    "      ckin.need_for_payment(pax_grp.grp_id, pax_grp.class, pax_grp.bag_refuse, "
    "                            pax_grp.piece_concept, pax_grp.excess, pax.pax_id)<>0 ";
  set<int> paxs;
  for ( list<TAdvTripInfo>::iterator iflt=flts.begin(); iflt!=flts.end(); iflt++ ) {
    Qry.CreateVariable( "point_id", otInteger, iflt->point_id );
    Qry.Execute();
    for ( ; !Qry.Eof; Qry.Next() ) {
      if (!paxs.insert( Qry.FieldAsInteger( "pax_id" ) ).second) continue;
      res.push_back( SirenaExchange::TPaxItem2().fromDB(Qry) );
    }
  }
  Qry.Clear();
  Qry.SQLText =
    "SELECT pax.pax_id, pax.name, pax.surname, pax_grp.grp_id, pax.pers_type, pax.seats, pax.reg_no "
    "FROM pax, pax_grp "
    "WHERE pax_grp.grp_id=pax.grp_id AND "
    "      pax_grp.point_id_mark=:point_id AND"
    "      pr_brd IS NOT NULL AND pax_grp.status NOT IN ('E') AND "
    "      piece_concept<>0 AND  "
    "      ckin.need_for_payment(pax_grp.grp_id, pax_grp.class, pax_grp.bag_refuse, "
    "                            pax_grp.piece_concept, pax_grp.excess, pax.pax_id)<>0 ";
  for ( set<int>::iterator iflt=marketing_point_ids.begin(); iflt!=marketing_point_ids.end(); iflt++ ) {
    Qry.CreateVariable( "point_id", otInteger, *iflt );
    Qry.Execute();
    for ( ; !Qry.Eof; Qry.Next() ) {
      if (!paxs.insert( Qry.FieldAsInteger( "pax_id" ) ).second) continue;
      res.push_back( SirenaExchange::TPaxItem2().fromDB(Qry) );
    }
  }
}

void PieceConceptInterface::procGroupInfo( const SirenaExchange::TGroupInfoReq &req, SirenaExchange::TGroupInfoRes &res )
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

  bool pr_unaccomp;
  TCkinGrpIds tckin_grp_ids;
  SirenaExchange::fillPaxsBags(req.grp_id, res, pr_unaccomp, tckin_grp_ids);
}

/*void SendTestRequest(const string &req)
{
  RequestInfo request;
  std::string proto;
  std::string query;
  request.host = SIRENA_HOST();
  request.port = SIRENA_PORT();
  request.timeout = SIRENA_REQ_TIMEOUT();
  request.headers.insert(make_pair("CLIENT-ID", "SIRENA"));
  request.headers.insert(make_pair("OPERATION", "piece_concept"));

  request.content = req;
  ProgTrace( TRACE5, "request.content=%s", request.content.c_str());
  request.using_ssl = false;
  ResponseInfo response;
  for(int pass=0; pass<SIRENA_REQ_ATTEMPTS(); pass++)
  {
    httpClient_main(request, response);
    if (!(!response.completed && response.error_code==boost::asio::error::eof && response.error_operation==toStatus))
      break;
    ProgTrace( TRACE5, "SIRENA DEADED, next request!" );
  };
  if (!response.completed ) throw Exception("%s: responseInfo.completed()=false", __FUNCTION__);

  ProgTrace( TRACE5, "response=%s", response.toString().c_str());
}*/

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
      "    <departure>РЩН</departure>"
      "  </passenger_with_svc>"
      "</query>");
    reqText = ConvertCodepage( reqText, "CP866", "UTF-8" );
    SendTestRequest(reqText);
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
    SendTestRequest(reqText);
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
  //PaxASVCList::printSQLs();

  return 0;
}

