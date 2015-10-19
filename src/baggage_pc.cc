#include "baggage_pc.h"
#include <boost/crc.hpp>
#include "astra_context.h"
#include "httpClient.h"
#include <serverlib/xml_stuff.h>

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace BASIC;
using namespace EXCEPTIONS;
using namespace std;
using namespace AstraLocale;

namespace PieceConcept
{

TRFISCListItem& TRFISCListItem::fromXML(xmlNodePtr node)
{
  clear();

  try
  {
    if (node==NULL) throw Exception("node not defined");

    RFIC=NodeAsString("@rfic", node, "");
    if (RFIC.size()>1) throw Exception("Wrong @rfic='%s'", RFIC.c_str());

    RFISC=NodeAsString("@rfisc", node, "");
    if (RFISC.empty()) throw Exception("Unknown @rfisc");
    if (RFISC.size()>15) throw Exception("Wrong @rfisc='%s'", RFISC.c_str());

    service_type=NodeAsString("@service_type", node, "");
    if (service_type.empty()) throw Exception("Unknown @service_type");
    if (service_type.size()>1) throw Exception("Wrong @service_type='%s'", service_type.c_str());

    string xml_emd_type=NodeAsString("@emd_type", node, "");
    if (!xml_emd_type.empty())
    {
      if ( xml_emd_type == "EMD-A" ) emd_type = "A";
      else if
         ( xml_emd_type == "EMD-S" ) emd_type = "S";
      else if
         ( xml_emd_type == "OTHR" ) emd_type = "O";
      else
        throw Exception("Wrong @emd_type='%s'", xml_emd_type.c_str());
    };

    xmlNodePtr nameNode=node->children;
    for(; nameNode!=NULL; nameNode=nameNode->next)
    {
      if (string((char*)nameNode->name)!="name") continue;
      string lang=NodeAsString("@language", nameNode, "");
      if (lang.empty()) throw Exception("Unknown @language");
      if (lang=="ru" || lang=="en")
      {
        string &n=(lang=="ru"?name:name_lat);
        if (!n.empty()) throw Exception("Duplicate <name language='%s'>", lang.c_str());
        n=NodeAsString(nameNode);
        if (n.empty()) throw Exception("Unknown <name language='%s'>", lang.c_str());
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
      /*throw Exception("TRFISCListItem::fromXML: Duplicate @rfisc='%s'", item.RFISC.c_str())*/;  //!!!vlad service_type могут отличаться
  }
}

void TRFISCList::fromDB(int list_id)
{
  clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT service_type, rfic, rfisc, emd_type, name, name_lat "
    "FROM grp_rfisc_lists "
    "WHERE list_id=:list_id";
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
    "INSERT INTO grp_rfisc_lists(list_id,rfisc,service_type,rfic,emd_type,name,name_lat) "
    "VALUES(:list_id,:rfisc,:service_type,:rfic,:emd_type,:name,:name_lat)";
  Qry.CreateVariable( "list_id", otInteger, list_id );
  Qry.DeclareVariable( "rfisc", otString );
  Qry.DeclareVariable( "service_type", otString );
  Qry.DeclareVariable( "rfic", otString );
  Qry.DeclareVariable( "emd_type", otString );
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
    list.fromDB(list_id);
    if (*this==list) return list_id;
  };

  Qry.Clear();
  Qry.SQLText =
    "BEGIN "
    "  SELECT bag_types_lists__seq.nextval INTO :id FROM dual; "
    "  INSERT INTO bag_types_lists(id, crc) VALUES(:id, :crc); "
    "END;";
  Qry.CreateVariable("id", otInteger, FNull);
  Qry.CreateVariable("crc", otInteger, crc());
  Qry.Execute();
  int list_id=Qry.GetVariableAsInteger("id");
  toDB(list_id);
  return list_id;
}

void TPaxNormItem::fromXML(xmlNodePtr node, bool &piece_concept)
{
  clear();

  try
  {
    if (node==NULL) throw Exception("node not defined");
    for(node=node->children; node!=NULL; node=node->next)
    {
      if (string((char*)node->name)!="free_baggage_norm") continue;

      if (!empty()) throw Exception("<free_baggage_norm> tag duplicated" );

      if (NodeIsNULL("@piece_concept", node))
        throw Exception("Unknown @piece_concept");
      piece_concept=NodeAsBoolean("@piece_concept", node);

      xmlNodePtr textNode=node->children;
      for(; textNode!=NULL; textNode=textNode->next)
      {
        if (string((char*)textNode->name)!="text") continue;
        string lang=NodeAsString("@language", textNode, "");
        if (lang.empty()) throw Exception("Unknown @language");
        if (lang!="ru" && lang!="en") continue;

        TPaxNormTextItem item;
        item.lang=(lang=="ru"?LANG_RU:LANG_EN);
        item.text=NodeAsString(textNode);
        if (item.text.empty()) throw Exception("Unknown <text language='%s'>", item.lang.c_str());
        if (!insert(make_pair(item.lang, item)).second)
          throw Exception("Duplicate <text language='%s'>", item.lang.c_str());
      };

      if (find(LANG_RU)==end()) throw Exception("Not found <text language='%s'>", "ru");
      if (find(LANG_EN)==end()) throw Exception("Not found <text language='%s'>", "en");
    };
    if (empty()) throw Exception("<free_baggage_norm> tag not found" );
  }
  catch(Exception &e)
  {
    throw Exception("TPaxNormItem::fromXML: %s", e.what());
  };
}

void TPaxNormItem::fromDB(int pax_id)
{
  clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT lang, text FROM pax_norms_pc WHERE pax_id=:pax_id ORDER BY lang, page_no";
  Qry.CreateVariable( "pax_id", otInteger, pax_id );
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next())
  {
    TPaxNormTextItem item;
    item.lang=Qry.FieldAsString("lang");
    item.text=Qry.FieldAsString("text");

    TPaxNormItem::iterator i=find(item.lang);
    if (i!=end())
      i->second.text+=item.text;
    else
      insert(make_pair(item.lang, item));
  }
}

void TPaxNormItem::toDB(int pax_id) const
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "INSERT INTO pax_norms_pc(pax_id, lang, page_no, text) "
    "VALUES(:pax_id, :lang, :page_no, :text)";
  Qry.CreateVariable("pax_id", otInteger, pax_id);
  Qry.DeclareVariable("lang", otString);
  Qry.DeclareVariable("page_no", otInteger);
  Qry.DeclareVariable("text", otString);
  for(TPaxNormItem::const_iterator i=begin(); i!=end(); ++i)
  {
    Qry.SetVariable("lang", i->second.lang);
    longToDB(Qry, "text", i->second.text);
  }
}

void PaxNormsToStream(const CheckIn::TPaxItem &pax, ostringstream &s)
{
  TPaxNormItem norm;
  norm.fromDB(pax.id);
  TPaxNormItem::const_iterator i=norm.find(TReqInfo::Instance()->desk.lang);
  if (i==norm.end() && !norm.empty()) i=norm.begin(); //первая попавшаяся

  s << "#" << setw(3) << setfill('0') << pax.reg_no << " "
    << pax.full_name() << "(" << ElemIdToCodeNative(etPersType, EncodePerson(pax.pers_type)) << "):" << endl;
  if (i!=norm.end())
    s << i->second.text << endl
      << string(100,'=') << endl;  //!!!vlad потом докрутить разрегистрированных пассажиров
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
    if (i->status==bsUnknown ||
        i->status==bsPaid ||
        i->status==bsNeed) excess++;
    i->toDB(BagQry);
    BagQry.Execute();
  };

  BagQry.Clear();
  BagQry.SQLText="UPDATE pax_grp SET excess=:excess WHERE grp_id=:grp_id";
  BagQry.CreateVariable("grp_id", otInteger, grp_id);
  BagQry.CreateVariable("excess", otInteger, excess);
  BagQry.Execute();
};

void PaidBagFromDB(int grp_id, list<TPaidBagItem> &paid)
{
  paid.clear();
  TQuery BagQry(&OraSession);
  BagQry.Clear();
  BagQry.SQLText=
    "SELECT paid_bag_pc.pax_id, "
    "       paid_bag_pc.transfer_num, "
    "       paid_bag_pc.rfisc, "
    "       paid_bag_pc.status, "
    "       paid_bag_pc.pr_cabin "
    "FROM pax, paid_bag_pc "
    "WHERE pax.pax_id=paid_bag_pc.pax_id AND pax.grp_id=:grp_id";
  BagQry.CreateVariable("grp_id", otInteger, grp_id);
  BagQry.Execute();
  for(;!BagQry.Eof;BagQry.Next())
    paid.push_back(TPaidBagItem().fromDB(BagQry));
};

class TBagKey
{
  public:
    int pax_id;
    string rfisc;
    TBagKey()
    {
      pax_id=ASTRA::NoExists;
      rfisc.clear();
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

class TEMDNumber
{
  public:
    string no;
    int coupon;
    TEMDNumber(const CheckIn::TPaxASVCItem &item)
    {
      no=item.emd_no;
      coupon=item.emd_coupon;
    }
    TEMDNumber(const CheckIn::TPaidBagEMDItem &item)
    {
      no=item.emd_no;
      coupon=item.emd_coupon;
    }

    bool operator < (const TEMDNumber &emd) const
    {
      if (no!=emd.no)
        return no<emd.no;
      return coupon<emd.coupon;
    }

};

typedef map<TBagKey, TBagPcs> TBagMap;
typedef map<TBagKey, map<TEMDNumber, CheckIn::TPaxASVCItem> > TASVCMap;
typedef map<TEMDNumber, CheckIn::TPaidBagEMDItem> TPaidBagEmdMap;

void ConvertPaidBagEMD(const list<CheckIn::TPaidBagEMDItem> &emd_list,
                       TPaidBagEmdMap &emd_map)
{
  emd_map.clear();
  for(list<CheckIn::TPaidBagEMDItem>::const_iterator i=emd_list.begin(); i!=emd_list.end(); ++i)
    emd_map.insert(make_pair(TEMDNumber(*i), *i));
}

void ConvertPaidBagEMD(const TPaidBagEmdMap &emd_map,
                       list<CheckIn::TPaidBagEMDItem> &emd_list)
{
  emd_list.clear();
  for(TPaidBagEmdMap::const_iterator i=emd_map.begin(); i!=emd_map.end(); ++i)
    emd_list.push_back(i->second);
}

void GetBagAndASVCInfo(int grp_id,
                       TBagMap &bag_map,
                       TASVCMap &asvc_map)
{
  bag_map.clear();
  asvc_map.clear();
  //набираем багаж
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT rfisc, pr_cabin, amount, "
    "       ckin.get_bag_pool_pax_id(grp_id,bag_pool_num,0) AS pax_id "
    "FROM bag2 "
    "WHERE grp_id=:grp_id AND is_trfer=0 ";
  Qry.CreateVariable("grp_id", otInteger, grp_id);
  Qry.Execute();
  set<int/*pax_id*/> pax_ids;
  for(; !Qry.Eof; Qry.Next())
  {
    if (Qry.FieldIsNULL("pax_id")) continue;  //если пассажир разрегистрирован
    TBagKey key;
    key.pax_id=Qry.FieldAsInteger("pax_id");
    key.rfisc=Qry.FieldAsString("rfisc");
    TBagMap::iterator b=bag_map.find(key);
    if (b==bag_map.end())
      b=bag_map.insert(make_pair(key, TBagPcs())).first;
    (Qry.FieldAsInteger("pr_cabin")!=0?b->second.cabin:b->second.trunk)+=Qry.FieldAsInteger("amount");
    pax_ids.insert(key.pax_id);
  };

  //набираем ASVC пассажиров
  for(set<int>::const_iterator i=pax_ids.begin(); i!=pax_ids.end(); ++i)
  {
    vector<CheckIn::TPaxASVCItem> asvc;
    CheckIn::LoadPaxASVC(*i, asvc);
    for(vector<CheckIn::TPaxASVCItem>::const_iterator r=asvc.begin(); r!=asvc.end(); ++r)
    {
      std::set<ASTRA::TRcptServiceType> service_types;
      r->rcpt_service_types(service_types);
      if (service_types.find(ASTRA::rstExcess)==service_types.end() &&
          service_types.find(ASTRA::rstPaid)==service_types.end()) continue;
      TBagKey key;
      key.pax_id=*i;
      key.rfisc=r->RFISC;
      TASVCMap::iterator a=asvc_map.find(key);
      if (a==asvc_map.end())
        a=asvc_map.insert(make_pair(key, map<TEMDNumber, CheckIn::TPaxASVCItem>())).first;
      a->second.insert(make_pair(TEMDNumber(*r), *r));
    }
  };
}

void PreparePaidBagInfo(int grp_id,
                        int seg_count,
                        list<TPaidBagItem> &paid_bag)
{
  paid_bag.clear();

  TBagMap bag_map;
  TASVCMap asvc_map;
  GetBagAndASVCInfo(grp_id, bag_map, asvc_map);

  TPaidBagEmdMap curr_emd_map;
  list<CheckIn::TPaidBagEMDItem> curr_emd;
  CheckIn::PaidBagEMDFromDB(grp_id, curr_emd);
  ConvertPaidBagEMD(curr_emd, curr_emd_map);

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
        item.status=bsNone;
        item.pr_cabin=(b->second.trunk<=0);
        b->second.dec();
        paid_bag.push_back(item);
      };
    };
  }

  //подготовим распределение номеров билетов по pax_id
//  map<string/*emd_no*/, int/*pax_id*/> emd_owners;
//  for(TASVCMap::const_iterator a=asvc_map.begin(); a!=asvc_map.end(); ++a)
//  {
//    for(map<TEMDNumber, CheckIn::TPaxASVCItem>::const_iterator i=a->second.begin(); i!=a->second.end(); ++i)
//      emd_owners.insert(make_pair(i->first.no, a->first.pax_id));
//  }


//  for(TBagMap::iterator b=bag_map.begin(); b!=bag_map.end(); ++b)
//  {
//    if (b->second.zero()) continue;
//    TASVCMap::iterator a=asvc_map.find(b->first);
//    if (a==asvc_map.end()) continue;
//    for(map<TEMDNumber, CheckIn::TPaxASVCItem>::const_iterator i=a->second.begin(); i!=a->second.end(); ++i)
//    {
//      if (b->second.zero()) break;
//      TPaidBagEmdMap::iterator e=curr_emd_map.find(i->first);
//      if (e!=curr_emd_map.end())
//      {
//        TPaidBagItem item;
//        item.pax_id=a->first.pax_id;
//        item.trfer_num=e->second.trfer_num;
//        item.RFISC=a->first.rfisc;
//        item.status=bsPaid;
//        curr_emd_map.erase(e);
//        b->second.dec();
//      };
//    };
//  };





}

void SyncPaidBagEMDToDB(int grp_id,
                        const boost::optional< list<CheckIn::TPaidBagEMDItem> > &curr_emd)
{
  TBagMap bag_map;
  TASVCMap asvc_map;
  GetBagAndASVCInfo(grp_id, bag_map, asvc_map);

  TPaidBagEmdMap curr_emd_map, result_emd_map;
  if (!curr_emd)
  {
    list<CheckIn::TPaidBagEMDItem> prior_emd;
    CheckIn::PaidBagEMDFromDB(grp_id, prior_emd);
    ConvertPaidBagEMD(prior_emd, curr_emd_map);
  }
  else
  {
    ConvertPaidBagEMD(curr_emd.get(), curr_emd_map);
  };

  for(TBagMap::iterator b=bag_map.begin(); b!=bag_map.end(); ++b)
  {
    if (b->second.zero()) continue;
    TASVCMap::iterator a=asvc_map.find(b->first);
    if (a==asvc_map.end()) continue;
    for(map<TEMDNumber, CheckIn::TPaxASVCItem>::const_iterator i=a->second.begin(); i!=a->second.end(); ++i)
    {
      if (b->second.zero()) break;
      CheckIn::TPaidBagEMDItem item;
      item.rfisc=i->second.RFISC;
      item.trfer_num=0;
      item.emd_no=i->second.emd_no;
      item.emd_coupon=i->second.emd_coupon;
      item.weight=0;
      result_emd_map.insert(make_pair(TEMDNumber(item), item));
      curr_emd_map.erase(TEMDNumber(item));
      b->second.dec();
    };
  };

  for(TPaidBagEmdMap::const_iterator e=curr_emd_map.begin(); e!=curr_emd_map.end(); ++e)
  {
    if (e->second.trfer_num>0)
    {
      result_emd_map.insert(*e);
      continue;
    };
    for(TBagMap::iterator b=bag_map.begin(); b!=bag_map.end(); ++b)
    {
      if (b->first.rfisc!=e->second.rfisc) continue;
      if (b->second.zero()) continue;
      result_emd_map.insert(*e);
      b->second.dec();
      break;
    }
  }

  list<CheckIn::TPaidBagEMDItem> result_emd;
  ConvertPaidBagEMD(result_emd_map, result_emd);
  CheckIn::PaidBagEMDToDB(grp_id, result_emd);
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
      if (RFISC!=key.RFISC)
        return RFISC<key.RFISC;
      return trfer_num<key.trfer_num;
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
    const TPiadBagViewItem& toXML(xmlNodePtr node) const;
    const TPiadBagViewItem& toXML2(xmlNodePtr node, const TTrferRoute &trfer) const;
};

const TPiadBagViewItem& TPiadBagViewItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;

  xmlNodePtr rowNode=NewTextChild(node,"paid_bag");
  NewTextChild(rowNode,"bag_type",RFISC);
  NewTextChild(rowNode,"weight",pieces);
  NewTextChild(rowNode,"weight_calc",pieces);
  NewTextChild(rowNode,"rate_id");

  NewTextChild(rowNode,"bag_type_view",RFISC);
  NewTextChild(rowNode,"bag_number_view",bag_number);
  NewTextChild(rowNode,"weight_view",pieces);
  NewTextChild(rowNode,"weight_calc_view",pieces);

  return *this;
}

const TPiadBagViewItem& TPiadBagViewItem::toXML2(xmlNodePtr node, const TTrferRoute &trfer) const
{
  if (node==NULL) return *this;

  xmlNodePtr rowNode=NewTextChild(node, "row");
  xmlNodePtr colNode;
  colNode=NewTextChild(rowNode, "col", RFISC);
  colNode=NewTextChild(rowNode, "col", bag_number);
  //номер рейса для сегмента
  ostringstream s;
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

void PaidBagViewToXML(const TTrferRoute &trfer,
                      const std::list<TPaidBagItem> &paid,
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
  SetProp(colNode, "width", 130);
  SetProp(colNode, "align", taLeftJustify);
  colNode=NewTextChild(colsNode, "col");
  SetProp(colNode, "width", 35);
  SetProp(colNode, "align", taCenter);
  colNode=NewTextChild(colsNode, "col");
  SetProp(colNode, "width", 85);
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

  for(map<TPiadBagViewKey, TPiadBagViewItem>::const_iterator i=paid_view.begin(); i!=paid_view.end(); ++i)
  {
    if (i->second.trfer_num==0)
      i->second.toXML(paidNode);
    i->second.toXML2(rowsNode, trfer);
  };
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

const TSegItem& TSegItem::toXML(xmlNodePtr node, const std::string &lang) const
{
  if (node==NULL) return *this;

  SetProp(node, "id", id);
  if (markFlt)
  {
    SetProp(node, "company", ElemIdToPrefferedElem(etAirline, markFlt.get().airline, efmtCodeNative, lang));
    SetProp(node, "flight", flight(markFlt.get(), lang));
  };
  SetProp(node, "operating_company", ElemIdToPrefferedElem(etAirline, operFlt.airline, efmtCodeNative, lang));
  SetProp(node, "operating_flight", flight(operFlt, lang));
  SetProp(node, "departure", ElemIdToPrefferedElem(etAirp, operFlt.airp, efmtCodeNative, lang));
  SetProp(node, "arrival", ElemIdToPrefferedElem(etAirp, airp_arv, efmtCodeNative, lang));
  if (operFlt.scd_out!=ASTRA::NoExists)
    SetProp(node, "departure_time", DateTimeToStr(operFlt.scd_out, "yyyy-mm-ddThh:nn:ss")); //локальное время
  SetProp(node, "equipment", craft, "");

  return *this;
}

const TPaxSegItem& TPaxSegItem::toXML(xmlNodePtr node, const std::string &lang) const
{
  if (node==NULL) return *this;

  TSegItem::toXML(node, lang);
  SetProp(node, "subclass", ElemIdToPrefferedElem(etSubcls, subcl, efmtCodeNative, lang));
  for(list<CheckIn::TPnrAddrItem>::const_iterator i=pnrs.begin(); i!=pnrs.end(); ++i)
    SetProp(NewTextChild(node, "recloc", i->addr), "crs", ElemIdToPrefferedElem(etAirline, i->airline, efmtCodeNative, lang));
  for(vector<CheckIn::TPaxFQTItem>::const_iterator i=fqts.begin(); i!=fqts.end(); ++i)
    SetProp(NewTextChild(node, "ffp", i->no), "company", ElemIdToPrefferedElem(etAirline, i->airline, efmtCodeNative, lang));

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
  SetProp(node, "category", category(), "");
  if (doc.birth_date!=ASTRA::NoExists)
    SetProp(node, "birthdate", DateTimeToStr(doc.birth_date, "yyyy-mm-dd"));
  SetProp(node, "sex", sex(), "");
  //билет
  if (!tkn.no.empty())
    SetProp(NewTextChild(node, "ticket"), "number", tkn.no);

  for(TPaxSegMap::const_iterator i=segs.begin(); i!=segs.end(); ++i)
    i->second.toXML(NewTextChild(node, "segment"), LANG_EN);

  return *this;
}

std::string TPaxItem::category() const
{
  switch(pers_type)
  {
    case ASTRA::adult: return "ADT";
    case ASTRA::child: return "CNN";
     case ASTRA::baby: return seats>0?"INS":"INF";
              default: return "";
  }
}

std::string TPaxItem::sex() const
{
  int f=CheckIn::is_female(doc.gender, name);
  if (f==ASTRA::NoExists) return "";
  return (f==0?"male":"female");
}

const TBagItem& TBagItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;

  SetProp(node, "rfisc", RFISC);
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
    if (RFISC.empty()) throw Exception("Unknown @rfisc");
    if (RFISC.size()>15) throw Exception("Wrong @rfisc='%s'", RFISC.c_str());

    string xml_status=NodeAsString("@payment_status", node, "");
    if (xml_status.empty()) throw Exception("Unknown @payment_status");
    status=PieceConcept::DecodeBagStatus(xml_status);
    if (status==PieceConcept::bsNone)
      throw Exception("Wrong @payment_status='%s'", xml_status.c_str());

    if (NodeIsNULL("@hand_baggage", node, false))
      throw Exception("Unknown @hand_baggage");
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

void TExchange::build(std::string &content) const
{
  try
  {
    XMLDoc doc(isRequest()?"query":"answer");
    xmlNodePtr node=NewTextChild(NodeAsNode(isRequest()?"/query":"/answer", doc.docPtr()), exchangeId().c_str());
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

void TAvailabilityReq::toXML(xmlNodePtr node) const
{
  if (node==NULL) return;

  if (paxs.empty()) throw Exception("TAvailabilityReq::toXML: paxs.empty()");

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
      item.norm.fromXML(node, item.piece_concept);
    };
    if (empty()) throw Exception("empty()");
  }
  catch(Exception &e)
  {
    throw Exception("TAvailabilityRes::fromXML: %s", e.what());
  };
}

bool TAvailabilityRes::identical_piece_concept()
{
  TAvailabilityResMap::const_iterator prior=end();
  TAvailabilityResMap::const_iterator curr=begin();
  for(; curr!=end(); ++curr)
  {
    if (prior!=end())
    {
      if (prior->second.piece_concept!=curr->second.piece_concept) return false;
    };
    prior=curr;
  }
  if (prior==end()) return false;
  return true;
}

bool TAvailabilityRes::identical_rfisc_list()
{
  TAvailabilityResMap::const_iterator prior=end();
  TAvailabilityResMap::const_iterator curr=begin();
  for(; curr!=end(); ++curr)
  {
    if (prior!=end())
    {
      if (!(prior->second.rfisc_list==curr->second.rfisc_list)) return false;
    };
    prior=curr;
  }
  if (prior==end()) return false;
  return true;
}

void TAvailabilityRes::normsToDB(int seg_id)
{
  for(TAvailabilityResMap::const_iterator curr=begin(); curr!=end(); ++curr)
  {
    if (curr->first.seg_id!=seg_id) continue;  //!!!vlad потом докрутить возможно сохранение по разным сегментам
    curr->second.norm.toDB(curr->first.pax_id);
  }
}

void TPaymentStatusReq::toXML(xmlNodePtr node) const
{
  if (node==NULL) return;

  if (paxs.empty()) throw Exception("TPaymentStatusReq::toXML: paxs.empty()");
  if (bags.empty()) throw Exception("TPaymentStatusReq::toXML: bags.empty()");

  for(list<TPaxItem>::const_iterator p=paxs.begin(); p!=paxs.end(); ++p)
    p->toXML(NewTextChild(node, "passenger"), LANG_EN);

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
      if (string((char*)node->name)!="svc") continue;
      TPaxSegKey key;
      key.fromXML(node);
      pair<TPaxSegKey, TBagItem> &item=*(insert(end(), make_pair(TPaxSegKey(), TBagItem())));
      item.first.fromXML(node);
      item.second.fromXML(node);
    };
    if (empty()) throw Exception("empty()");
  }
  catch(Exception &e)
  {
    throw Exception("TPaymentStatusReq::fromXML: %s", e.what());
  };
}

void TPaymentStatusRes::convert(list<PieceConcept::TPaidBagItem> &paid) const
{
  paid.clear();
  for(TPaymentStatusList::const_iterator i=begin(); i!=end(); ++i)
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

void traceXML(const string& xml)
{
  size_t len=xml.size();
  int portion=4000;
  for(size_t pos=0; pos<len; pos+=portion)
    ProgTrace(TRACE5, "%s", xml.substr(pos,portion).c_str());
}

const char* SIRENA_HOST()
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

void SendRequest(const TExchange &request, TExchange &response)
{
  RequestInfo requestInfo;
  requestInfo.host = SIRENA_HOST();
  requestInfo.port = SIRENA_PORT();
  requestInfo.path = "/astra";
  request.build(requestInfo.content);
  requestInfo.using_ssl = false;
  requestInfo.timeout = SIRENA_REQ_TIMEOUT();
  traceXML(requestInfo.content);
  ResponseInfo responseInfo;
  httpClient_main(requestInfo, responseInfo);
  traceXML(XMLTreeToText(TextToXMLTree(responseInfo.content)));
  response.parse(responseInfo.content);
}



} //namespace SirenaExchange


void PieceConceptInterface::requestPieceConcept(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  xmlNodePtr node = GetNode( "content", reqNode);
  ProgTrace( TRACE5, "requestPieceConcept, %s", (char*)node->children->name );
  NewTextChild( resNode, "passenger_with_svc" );
}
