#include "baggage.h"
#include "astra_consts.h"
#include "astra_locale.h"
#include "astra_utils.h"
#include "base_tables.h"
#include "basic.h"
#include "term_version.h"
#include "astra_misc.h"
#include "qrys.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace std;
using namespace AstraLocale;
using namespace EXCEPTIONS;

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

namespace CheckIn
{

const TValueBagItem& TValueBagItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  NewTextChild(node,"num",num);
  NewTextChild(node,"value",value);
  NewTextChild(node,"value_cur",value_cur);
  if (tax_id!=ASTRA::NoExists)
  {
    NewTextChild(node,"tax_id",tax_id);
    NewTextChild(node,"tax",tax);
    NewTextChild(node,"tax_trfer",(int)tax_trfer);
  }
  else
  {
    NewTextChild(node,"tax_id");
    NewTextChild(node,"tax");
    NewTextChild(node,"tax_trfer");
  };
  return *this;
};

TValueBagItem& TValueBagItem::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  num=NodeAsIntegerFast("num",node2);
  value=NodeAsFloatFast("value",node2);
  value_cur=NodeAsStringFast("value_cur",node2);
  if (!NodeIsNULLFast("tax_id",node2))
  {
    tax_id=NodeAsIntegerFast("tax_id",node2);
    tax=NodeAsFloatFast("tax",node2);
    tax_trfer=NodeAsIntegerFast("tax_trfer",node2,0)!=0;
  };
  return *this;
};

const TValueBagItem& TValueBagItem::toDB(TQuery &Qry) const
{
  Qry.SetVariable("num",num);
  Qry.SetVariable("value",value);
  Qry.SetVariable("value_cur",value_cur);
  if (tax_id!=ASTRA::NoExists)
  {
    Qry.SetVariable("tax_id",tax_id);
    Qry.SetVariable("tax",tax);
    Qry.SetVariable("tax_trfer",(int)tax_trfer);
  }
  else
  {
    Qry.SetVariable("tax_id",FNull);
    Qry.SetVariable("tax",FNull);
    Qry.SetVariable("tax_trfer",FNull);
  };
  return *this;
};

TValueBagItem& TValueBagItem::fromDB(TQuery &Qry)
{
  clear();
  num=Qry.FieldAsInteger("num");
  value=Qry.FieldAsFloat("value");
  value_cur=Qry.FieldAsString("value_cur");
  if (!Qry.FieldIsNULL("tax_id"))
  {
    tax_id=Qry.FieldAsInteger("tax_id");
    tax=Qry.FieldAsFloat("tax");
    tax_trfer=Qry.FieldAsInteger("tax_trfer")!=0;
  };
  return *this;
};

const TBagItem& TBagItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  NewTextChild(node,"id",id);
  NewTextChild(node,"num",num);
  NewTextChild(node,"bag_type",bag_type_str());
  NewTextChild(node,"pr_cabin",(int)pr_cabin);
  NewTextChild(node,"amount",amount);
  NewTextChild(node,"weight",weight);
  if (value_bag_num!=ASTRA::NoExists)
    NewTextChild(node,"value_bag_num",value_bag_num);
  else
    NewTextChild(node,"value_bag_num");
  NewTextChild(node,"pr_liab_limit",(int)pr_liab_limit);
  NewTextChild(node,"to_ramp",(int)to_ramp);
  NewTextChild(node,"using_scales",(int)using_scales);
  if (TReqInfo::Instance()->desk.compatible(PIECE_CONCEPT_VERSION2))
  {
    NewTextChild(node,"is_trfer",(int)is_trfer);
    NewTextChild(node,"handmade_trfer", (int)(is_trfer && handmade), (int)false);
  }
  else
  {
    if (is_trfer) ReplaceTextChild(node,"bag_type",99);
    NewTextChild(node,"is_trfer",(int)(is_trfer && !handmade));
  };
  if (TReqInfo::Instance()->desk.compatible(VERSION_WITH_BAG_POOLS))
    NewTextChild(node,"bag_pool_num",bag_pool_num);
  return *this;
};

TBagItem& TBagItem::fromXML(xmlNodePtr node, bool piece_concept)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  id=NodeAsIntegerFast("id",node2,ASTRA::NoExists);
  num=NodeAsIntegerFast("num",node2);
  if (!piece_concept)
  {
    if (!NodeIsNULLFast("bag_type",node2))
      bag_type=NodeAsIntegerFast("bag_type",node2);
  }
  else rfisc=NodeAsStringFast("bag_type",node2);
  pr_cabin=NodeAsIntegerFast("pr_cabin",node2)!=0;
  amount=NodeAsIntegerFast("amount",node2);
  weight=NodeAsIntegerFast("weight",node2);
  if (!NodeIsNULLFast("value_bag_num",node2))
    value_bag_num=NodeAsIntegerFast("value_bag_num",node2);
  pr_liab_limit=NodeAsIntegerFast("pr_liab_limit",node2)!=0;

  if (TReqInfo::Instance()->desk.compatible(BAG_TO_RAMP_VERSION))
    to_ramp=NodeAsIntegerFast("to_ramp",node2)!=0;

  if (TReqInfo::Instance()->desk.compatible(USING_SCALES_VERSION))
    using_scales=NodeAsIntegerFast("using_scales",node2)!=0;

  if (TReqInfo::Instance()->desk.compatible(PIECE_CONCEPT_VERSION2))
    is_trfer=NodeAsIntegerFast("is_trfer",node2)!=0;
  else
  {
    is_trfer=(bag_type==99);
    if (bag_type==99) bag_type=ASTRA::NoExists;
  };

  if (TReqInfo::Instance()->desk.compatible(VERSION_WITH_BAG_POOLS))
    bag_pool_num=NodeAsIntegerFast("bag_pool_num",node2);
  else
    bag_pool_num=1;
  return *this;
};

const TBagItem& TBagItem::toDB(TQuery &Qry) const
{
  if (id!=ASTRA::NoExists)
    Qry.SetVariable("id",id);
  else
    Qry.SetVariable("id",FNull);
  Qry.SetVariable("num",num);
  if (bag_type!=ASTRA::NoExists)
    Qry.SetVariable("bag_type",bag_type);
  else
    Qry.SetVariable("bag_type",FNull);
  Qry.SetVariable("rfisc",rfisc);
  Qry.SetVariable("pr_cabin",(int)pr_cabin);
  Qry.SetVariable("amount",amount);
  Qry.SetVariable("weight",weight);
  if (value_bag_num!=ASTRA::NoExists)
    Qry.SetVariable("value_bag_num",value_bag_num);
  else
    Qry.SetVariable("value_bag_num",FNull);
  Qry.SetVariable("pr_liab_limit",(int)pr_liab_limit);
  Qry.SetVariable("to_ramp",(int)to_ramp);
  Qry.SetVariable("using_scales",(int)using_scales);
  Qry.SetVariable("bag_pool_num",bag_pool_num);
  if (hall!=ASTRA::NoExists)
    Qry.SetVariable("hall",hall);
  else
    Qry.SetVariable("hall",FNull);
  if (user_id!=ASTRA::NoExists)
    Qry.SetVariable("user_id",user_id);
  else
    Qry.SetVariable("user_id",FNull);
  Qry.SetVariable("desk",desk);
  if (time_create!=ASTRA::NoExists)
    Qry.SetVariable("time_create",time_create);
  else
    Qry.SetVariable("time_create",FNull);
  Qry.SetVariable("is_trfer",(int)is_trfer);
  Qry.SetVariable("handmade",(int)handmade);
  return *this;
};

TBagItem& TBagItem::fromDB(TQuery &Qry)
{
  clear();
  id=Qry.FieldAsInteger("id");
  num=Qry.FieldAsInteger("num");
  if (!Qry.FieldIsNULL("bag_type"))
    bag_type=Qry.FieldAsInteger("bag_type");
  rfisc=Qry.FieldAsString("rfisc");
  pr_cabin=Qry.FieldAsInteger("pr_cabin")!=0;
  amount=Qry.FieldAsInteger("amount");
  weight=Qry.FieldAsInteger("weight");
  if (!Qry.FieldIsNULL("value_bag_num"))
    value_bag_num=Qry.FieldAsInteger("value_bag_num");
  pr_liab_limit=Qry.FieldAsInteger("pr_liab_limit")!=0;
  to_ramp=Qry.FieldAsInteger("to_ramp")!=0;
  using_scales=Qry.FieldAsInteger("using_scales")!=0;
  bag_pool_num=Qry.FieldAsInteger("bag_pool_num");
  if (!Qry.FieldIsNULL("hall"))
    hall=Qry.FieldAsInteger("hall");
  if (!Qry.FieldIsNULL("user_id"))
    user_id=Qry.FieldAsInteger("user_id");
  desk=Qry.FieldAsString("desk");
  if (!Qry.FieldIsNULL("time_create"))
    time_create=Qry.FieldAsDateTime("time_create");
  is_trfer=Qry.FieldAsInteger("is_trfer")!=0;
  if (Qry.FieldIsNULL("handmade"))
  {
    //���室�� ������  ��⮬ 㤠����!!!vlad
    if (bag_type==99)
    {
      //�� �࠭���� �����
      bag_type=ASTRA::NoExists;
      handmade=!is_trfer;
      is_trfer=true;
    }
    else handmade=true;
  }
  else handmade=Qry.FieldAsInteger("handmade")!=0;
  return *this;
};

std::string TBagItem::bag_type_str() const
{
  ostringstream s;
  if (rfisc.empty())
  {
    if (bag_type!=ASTRA::NoExists)
      s << setw(2) << setfill('0') << bag_type;
  }
  else s << rfisc;
  return s.str();
};

const TTagItem& TTagItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  NewTextChild(node,"num",num);
  NewTextChild(node,"tag_type",tag_type);
  NewTextChild(node,"no_len",no_len);
  NewTextChild(node,"no",no);
  NewTextChild(node,"color",color);
  if (bag_num!=ASTRA::NoExists)
    NewTextChild(node,"bag_num",bag_num);
  else
    NewTextChild(node,"bag_num");
  NewTextChild(node,"printable",(int)printable);
  NewTextChild(node,"pr_print",(int)pr_print);
  return *this;
};

TTagItem& TTagItem::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  num=NodeAsIntegerFast("num",node2);
  tag_type=NodeAsStringFast("tag_type",node2);
  no=NodeAsFloatFast("no",node2);
  color=NodeAsStringFast("color",node2);
  if (!NodeIsNULLFast("bag_num",node2))
    bag_num=NodeAsIntegerFast("bag_num",node2);
  pr_print=NodeAsIntegerFast("pr_print",node2)!=0;
  return *this;
};

const TTagItem& TTagItem::toDB(TQuery &Qry) const
{
  Qry.SetVariable("num",num);
  Qry.SetVariable("tag_type",tag_type);
  Qry.SetVariable("no",no);
  Qry.SetVariable("color",color);
  if (bag_num!=ASTRA::NoExists)
    Qry.SetVariable("bag_num",bag_num);
  else
    Qry.SetVariable("bag_num",FNull);
  Qry.SetVariable("pr_print",(int)pr_print);
  return *this;
};

TTagItem& TTagItem::fromDB(TQuery &Qry)
{
  clear();
  num=Qry.FieldAsInteger("num");
  tag_type=Qry.FieldAsString("tag_type");
  if (Qry.GetFieldIndex("no_len")>=0)
    no_len=Qry.FieldAsInteger("no_len");
  no=Qry.FieldAsFloat("no");
  color=Qry.FieldAsString("color");
  if (!Qry.FieldIsNULL("bag_num"))
    bag_num=Qry.FieldAsInteger("bag_num");
  if (Qry.GetFieldIndex("printable")>=0)
    printable=Qry.FieldAsInteger("printable")!=0;
  pr_print=Qry.FieldAsInteger("pr_print")!=0;
  return *this;
};

void GetNextTagNo(int grp_id, int tag_count, vector< pair<int,int> >& tag_ranges)
{
  tag_ranges.clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT points.airline, "
    "       pax_grp.point_dep,pax_grp.airp_dep,pax_grp.class, "
    "       NVL(transfer.airp_arv,pax_grp.airp_arv) AS airp_arv "
    "FROM points, pax_grp, transfer "
    "WHERE points.point_id=pax_grp.point_dep AND "
    "      pax_grp.grp_id=transfer.grp_id(+) AND transfer.pr_final(+)<>0 AND "
    "      pax_grp.grp_id=:grp_id";
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.Execute();
  if (Qry.Eof) throw EXCEPTIONS::Exception("CheckIn::GetNextTagNo: group not found (grp_id=%d)",grp_id);

  int point_id=Qry.FieldAsInteger("point_dep");
  string airp_dep=Qry.FieldAsString("airp_dep");
  string airp_arv=Qry.FieldAsString("airp_arv");
  string cl=Qry.FieldAsString("class");
  int aircode=-1;
  try
  {
    aircode=ToInt(base_tables.get("airlines").get_row("code",Qry.FieldAsString("airline")).AsString("aircode"));
    if (aircode<=0 || aircode>999) throw EXCEPTIONS::EConvertError("");
  }
  catch(EBaseTableError) { aircode=-1; }
  catch(EXCEPTIONS::EConvertError)   { aircode=-1; };

  if (aircode==-1) aircode=954;

  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  SELECT range INTO :range FROM last_tag_ranges2 WHERE aircode=:aircode FOR UPDATE; "
    "EXCEPTION "
    "  WHEN NO_DATA_FOUND THEN "
    "  BEGIN "
    "    :range:=9999; "
    "    INSERT INTO last_tag_ranges2(aircode,range) VALUES(:aircode,:range); "
    "  EXCEPTION "
    "    WHEN DUP_VAL_ON_INDEX THEN "
    "      SELECT range INTO :range FROM last_tag_ranges2 WHERE aircode=:aircode FOR UPDATE; "
    "  END; "
    "END;";
  Qry.CreateVariable("aircode",otInteger,aircode);
  Qry.CreateVariable("range",otInteger,FNull);
  Qry.Execute();
  //����稬 ��᫥���� �ᯮ�짮����� �������� (+��窠):
  int last_range=Qry.GetVariableAsInteger("range");

  int range = 0;
  int no = 0;
  bool use_new_range=false;
  while (tag_count>0)
  {
    if (!use_new_range)
    {
      int k;
      for(k=1;k<=5;k++)
      {
        Qry.Clear();
        Qry.CreateVariable("aircode",otInteger,aircode);
        ostringstream sql;

        sql << "SELECT range,no FROM tag_ranges2 ";
        if (k>=2) sql << ",points ";
        sql << "WHERE aircode=:aircode AND ";


        if (k==1)
        {
          sql <<
            "      point_id=:point_id AND airp_dep=:airp_dep AND airp_arv=:airp_arv AND "
            "      (class IS NULL AND :class IS NULL OR class=:class) ";
          Qry.CreateVariable("point_id",otInteger,point_id);
          Qry.CreateVariable("airp_dep",otString,airp_dep);
          Qry.CreateVariable("airp_arv",otString,airp_arv);
          Qry.CreateVariable("class",otString,cl);
        };

        if (k>=2)
        {
          sql <<
            "      tag_ranges2.point_id=points.point_id(+) AND "
            "      (points.point_id IS NULL OR "
            "       points.pr_del<>0 OR "
            "       NVL(points.act_out,NVL(points.est_out,points.scd_out))<:now_utc AND last_access<:now_utc-2/24 OR "
            "       NVL(points.act_out,NVL(points.est_out,NVL(points.scd_out,:now_utc+1)))>=:now_utc AND last_access<:now_utc-2) AND ";
          Qry.CreateVariable("now_utc",otDate,BASIC::NowUTC());
          if (k==2)
          {
            sql <<
              "      airp_dep=:airp_dep AND airp_arv=:airp_arv AND "
              "      (class IS NULL AND :class IS NULL OR class=:class) ";
            Qry.CreateVariable("airp_dep",otString,airp_dep);
            Qry.CreateVariable("airp_arv",otString,airp_arv);
            Qry.CreateVariable("class",otString,cl);
          };
          if (k==3)
          {
            sql <<
              "      airp_dep=:airp_dep AND airp_arv=:airp_arv ";
            Qry.CreateVariable("airp_dep",otString,airp_dep);
            Qry.CreateVariable("airp_arv",otString,airp_arv);
          };
          if (k==4)
          {
            sql <<
              "      airp_dep=:airp_dep ";
            Qry.CreateVariable("airp_dep",otString,airp_dep);
          };
          if (k==5)
          {
            sql <<
              "      last_access<:now_utc-1 ";
          };
        };
        sql << "ORDER BY last_access";


        Qry.SQLText=sql.str().c_str();
        Qry.Execute();
        if (!Qry.Eof)
        {
          range=Qry.FieldAsInteger("range");
          no=Qry.FieldAsInteger("no");
          break;
        };
      };
      if (k>5) //�।� 㦥 ��������� ���������� ��� ���室�饣� - ��६ ����
      {
        use_new_range=true;
        range=last_range;
      };
    };
    if (use_new_range)
    {
      range++;
      no=-1;
      if (range>9999)
      {
        range=0;
        no=0; //�㫥��� ��ઠ 000000 ����饭� IATA
      };


      if (range==last_range)
        throw EXCEPTIONS::Exception("CheckIn::GetNextTagNo: free range not found (aircode=%d)",aircode);

      Qry.Clear();
      Qry.SQLText="SELECT range FROM tag_ranges2 WHERE aircode=:aircode AND range=:range";
      Qry.CreateVariable("aircode",otInteger,aircode);
      Qry.CreateVariable("range",otInteger,range);
      Qry.Execute();
      if (!Qry.Eof) continue; //��� �������� �ᯮ������
    };

    pair<int,int> tag_range;
    tag_range.first=aircode*1000000+range*100+no+1; //��ࢠ� ���ᯮ�짮����� ��ઠ �� ��ண� ���������

    if (tag_count>=99-no)
    {
      tag_count-=99-no;
      no=99;
    }
    else
    {
      no+=tag_count;
      tag_count=0;
    };

    tag_range.second=aircode*1000000+range*100+no; //��᫥���� �ᯮ�짮����� ��ઠ ������ ���������
    if (tag_range.first<=tag_range.second) tag_ranges.push_back(tag_range);

    Qry.Clear();
    Qry.CreateVariable("aircode",otInteger,aircode);
    Qry.CreateVariable("range",otInteger,range);
    if (no>=99)
      Qry.SQLText="DELETE FROM tag_ranges2 WHERE aircode=:aircode AND range=:range";
    else
    {
      Qry.SQLText=
        "BEGIN "
        "  UPDATE tag_ranges2 "
        "  SET no=:no, airp_dep=:airp_dep, airp_arv=:airp_arv, class=:class, point_id=:point_id, "
        "      last_access=system.UTCSYSDATE "
        "  WHERE aircode=:aircode AND range=:range; "
        "  IF SQL%NOTFOUND THEN "
        "    INSERT INTO tag_ranges2(aircode,range,no,airp_dep,airp_arv,class,point_id,last_access) "
        "    VALUES(:aircode,:range,:no,:airp_dep,:airp_arv,:class,:point_id,system.UTCSYSDATE); "
        "  END IF; "
        "END;";
      Qry.CreateVariable("no",otInteger,no);
      Qry.CreateVariable("airp_dep",otString,airp_dep);
      Qry.CreateVariable("airp_arv",otString,airp_arv);
      Qry.CreateVariable("class",otString,cl);
      Qry.CreateVariable("point_id",otInteger,point_id);
    };
    Qry.Execute();
  };
  if (use_new_range)
  {
    Qry.Clear();
    Qry.SQLText=
      "BEGIN "
      "  UPDATE last_tag_ranges2 SET range=:range WHERE aircode=:aircode; "
      "  IF SQL%NOTFOUND THEN "
      "    INSERT INTO last_tag_ranges2(aircode,range) VALUES(:aircode,:range); "
      "  END IF; "
      "END;";
    Qry.CreateVariable("aircode",otInteger,aircode);
    Qry.CreateVariable("range",otInteger,range);
    Qry.Execute();
  };
};

void TGroupBagItem::setInboundTrfer(const TrferList::TGrpItem &grp)
{
  clear();
  int bag_weight=CalcWeightInKilos(grp.bag_weight, grp.weight_unit);
  int rk_weight=CalcWeightInKilos(grp.rk_weight, grp.weight_unit);
  if ((bag_weight==ASTRA::NoExists || bag_weight<=0 || grp.tags.empty()) &&
      (rk_weight==ASTRA::NoExists || rk_weight<=0)) return;
  int bag_num=1;
  if (bag_weight!=ASTRA::NoExists && bag_weight>0 && !grp.tags.empty())
  {
    TBagItem bag;
    bag.num=bag_num;
    bag.pr_cabin=false;
    bag.amount=grp.tags.size();
    bag.weight=bag_weight;
    bags[bag.num]=bag;
    int tag_num=1;
    for(multiset<TBagTagNumber>::const_iterator t=grp.tags.begin(); t!=grp.tags.end(); ++t, tag_num++)
    {
      TTagItem tag;
      tag.num=tag_num;
      tag.tag_type="BTM";
      tag.no_len=10;
      tag.no=t->numeric_part;
      tag.bag_num=bag_num;
      tag.printable=true;
      tag.pr_print=true;
      tags[tag.num]=tag;
    };
    bag_num++;
  };
  if (rk_weight!=ASTRA::NoExists && rk_weight>0)
  {
    TBagItem bag;
    bag.num=bag_num;
    bag.pr_cabin=true;
    bag.amount=1;
    bag.weight=rk_weight;
    bags[bag.num]=bag;
    bag_num++;
  };
};

void TGroupBagItem::setPoolNum(int bag_pool_num)
{
  for(map<int /*num*/, TBagItem>::iterator i=bags.begin();i!=bags.end();++i)
    i->second.bag_pool_num=bag_pool_num;
};

bool TGroupBagItem::trferExists() const
{
  for(map<int /*num*/, TBagItem>::const_iterator i=bags.begin();i!=bags.end();++i)
    if (i->second.is_trfer) return true;
  return false;
}

bool TGroupBagItem::fromXML(xmlNodePtr bagtagNode, bool piece_concept)
{
  clear();
  if (bagtagNode==NULL) return false;
  xmlNodePtr node;

  xmlNodePtr valueBagNode=GetNode("value_bags",bagtagNode);
  xmlNodePtr bagNode=GetNode("bags",bagtagNode);
  xmlNodePtr tagNode=GetNode("tags",bagtagNode);

  if (valueBagNode==NULL && bagNode==NULL && tagNode==NULL) return false;
  if (valueBagNode==NULL) throw Exception("TGroupBagItem::fromXML: valueBagNode=NULL");
  if (bagNode==NULL) throw Exception("TGroupBagItem::fromXML: bagNode=NULL");
  if (tagNode==NULL) throw Exception("TGroupBagItem::fromXML: tagNode=NULL");

  for(node=valueBagNode->children;node!=NULL;node=node->next)
  {
    TValueBagItem val;
    val.fromXML(node);
    if (vals.find(val.num)!=vals.end())
      throw Exception("TGroupBagItem::fromXML: vals[%d] duplicated", val.num);
    vals[val.num]=val;
  };

  for(node=bagNode->children;node!=NULL;node=node->next)
  {
    TBagItem bag;
    bag.fromXML(node, piece_concept);
    if (bags.find(bag.num)!=bags.end())
      throw Exception("TGroupBagItem::fromXML: bags[%d] duplicated", bag.num);
    if (bag.value_bag_num!=ASTRA::NoExists && vals.find(bag.value_bag_num)==vals.end())
      throw Exception("TGroupBagItem::fromXML: bags[%d].value_bag_num=%d not found", bag.num, bag.value_bag_num);
    bags[bag.num]=bag;
  };

  for(node=tagNode->children;node!=NULL;node=node->next)
  {
    TTagItem tag;
    tag.fromXML(node);
    if (tags.find(tag.num)!=tags.end())
      throw Exception("TGroupBagItem::fromXML: tags[%d] duplicated", tag.num);
    if (tag.bag_num!=ASTRA::NoExists && bags.find(tag.bag_num)==bags.end())
      throw Exception("TGroupBagItem::fromXML: tags[%d].bag_num=%d not found", tag.num, tag.bag_num);
    tags[tag.num]=tag;
  };

  int num=1;
  for(map<int, TValueBagItem>::const_iterator v=vals.begin();v!=vals.end();++v,num++)
  {
    if (v->second.num!=num)
      throw Exception("TGroupBagItem::fromXML: vals[%d] not found", v->second.num);
  };
  num=1;
  for(map<int, TBagItem>::const_iterator b=bags.begin();b!=bags.end();++b,num++)
  {
    if (b->second.num!=num)
      throw Exception("TGroupBagItem::fromXML: bags[%d] not found", b->second.num);
  };
  num=1;
  for(map<int, TTagItem>::const_iterator t=tags.begin();t!=tags.end();++t,num++)
  {
    if (t->second.num!=num)
      throw Exception("TGroupBagItem::fromXML: tags[%d] not found", t->second.num);
  };

  pr_tag_print=NodeAsInteger("@pr_print",tagNode)!=0;
  return true;
}

void TGroupBagItem::fromXMLadditional(int point_id, int grp_id, int hall)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  bool is_payment=reqInfo->client_type == ASTRA::ctTerm && reqInfo->screen.name != "AIR.EXE";

  map<int /*num*/, int> bound_tags;
  for(map<int, TBagItem>::const_iterator b=bags.begin();b!=bags.end();++b)
    bound_tags[b->second.num]=0;

  int unbound_tags=0;
  for(map<int, TTagItem>::const_iterator t=tags.begin();t!=tags.end();++t)
    if (t->second.bag_num!=ASTRA::NoExists)
      ++bound_tags[t->second.bag_num];
    else
      ++unbound_tags;

  int num;
  if (!is_payment)
  {
    //� ���� �� ����� �ਢ�뢠�� ��ન � ��������� ����� ���⮬� ������ �� ⮫쪮 ��� ॣ����樨
    if (unbound_tags>0 && !reqInfo->desk.compatible(VERSION_WITH_BAG_POOLS))
    {
      //��⮬���᪠� �ਢ離� ��ப ��� ����� �ନ�����
      for(map<int, TTagItem>::iterator t=tags.begin();t!=tags.end();++t)
      {
        if (t->second.bag_num!=ASTRA::NoExists) continue;
        for(map<int, TBagItem>::const_iterator b=bags.begin();b!=bags.end();b++)
        {
          if (!b->second.pr_cabin && bound_tags[b->second.num]<b->second.amount)
          {
            t->second.bag_num=b->second.num;
            ++bound_tags[b->second.num];
            --unbound_tags;
            break;
          };
        };
      };
    };

    if (unbound_tags>0)
    {
      if (unbound_tags==1)
      {
        if (tags.size()>1)
          throw UserException("MSG.LUGGAGE.ONE_OF_TAGS_NOT_ATTACHED");
        else
          throw UserException("MSG.LUGGAGE.TAG_NOT_ATTACHED");
      }
      else throw UserException("MSG.LUGGAGE.PART_OF_TAGS_NOT_ATTACHED");
    };

    int bagAmount=0;
    int rebind_tags=0;
    for(map<int, TBagItem>::const_iterator b=bags.begin();b!=bags.end();++b)
    {
      if (b->second.pr_cabin)
      {
        if (bound_tags[b->second.num]>0)
          throw UserException("MSG.LUGGAGE.CAN_NOT_ATTACH_TAG_TO_HAND_LUGGAGE");
      }
      else
      {
        bagAmount+=b->second.amount;
        num=bound_tags[b->second.num]-b->second.amount;
        if (num>0) rebind_tags+=num;
      };
    };
    if (rebind_tags>0)
    {
      if (rebind_tags==1)
        throw UserException("MSG.LUGGAGE.ONE_OF_TAGS_MUST_REBIND");
      else
        throw UserException("MSG.LUGGAGE.PART_OF_TAGS_MUST_REBIND");
    };

    //�����⠥� ���-�� ������ � ���. ��ப
    int tagCount=tags.size();
    TQuery Qry(&OraSession);
    if (bagAmount!=tagCount)
    {
      ProgTrace(TRACE5,"bagAmount=%d tagCount=%d",bagAmount,tagCount);
      if (pr_tag_print && tagCount<bagAmount )
      {
        map<int /*num*/, TTagItem> generated_tags;
        Qry.Clear();
        Qry.SQLText=
          "SELECT tag_type FROM trip_bt WHERE point_id=:point_id";
        Qry.CreateVariable("point_id",otInteger,point_id);
        Qry.Execute();
        if (Qry.Eof) throw UserException("MSG.CHECKIN.LUGGAGE_BLANK_NOT_SET");
        string tag_type = Qry.FieldAsString("tag_type");
        //����稬 ����� ���⠥��� ��ப
        vector< pair<int,int> > tag_ranges;
        GetNextTagNo(grp_id, bagAmount-tagCount, tag_ranges);
        num=tagCount+1;
        for(vector< pair<int,int> >::iterator r=tag_ranges.begin();r!=tag_ranges.end();++r)
        {
          for(int i=r->first;i<=r->second;i++,num++)
          {
            TTagItem tag;
            tag.num=num;
            tag.tag_type=tag_type;
            tag.no=i;
            generated_tags[tag.num]=tag;
          };
        };
        //�ਢ�뢠�� � ������
        map<int, TTagItem>::iterator t=generated_tags.begin();
        for(map<int, TBagItem>::const_iterator b=bags.begin();b!=bags.end();)
        {
          if (!b->second.pr_cabin && bound_tags[b->second.num]<b->second.amount)
          {
            if (t==generated_tags.end()) throw Exception("TGroupBagItem::fromXML: t==generated_tags.end()");
            t->second.bag_num=b->second.num;
            ++t;
            ++bound_tags[b->second.num];
          }
          else ++b;
        };
        if (t!=generated_tags.end()) throw Exception("TGroupBagItem::fromXML: t!=generated_tags.end()");
        tags.insert(generated_tags.begin(), generated_tags.end());
      }
      else throw UserException(1,"MSG.CHECKIN.COUNT_BIRKS_NOT_EQUAL_PLACES");
    };
  };

  TQuery BagQry(&OraSession);

  if (is_payment && !reqInfo->desk.compatible(VERSION_WITH_BAG_POOLS))
  {
    map<int /*num*/, TBagItem> old_bags;
    map<int /*num*/, bool> refused_bags;
    BagQry.Clear();
    if (reqInfo->desk.compatible(BAG_WITH_HALL_VERSION))
      BagQry.SQLText="SELECT * FROM bag2 WHERE grp_id=:grp_id FOR UPDATE";
    else
      //�맮� ckin.bag_pool_refused ��। ������� ������ �ப��뢠�� ⮫쪮 �᫨
      //��। �⨬ ��� ��������� � ��㯯� ���ᠦ�஢ (��� ����� ������� ���㠫쭮���)
      //�᫮��� ᮡ����� (�� ��ଠ�쭮) �᫨ ������ ������ �� ����� "����� ������"
      BagQry.SQLText=
        "SELECT bag2.*, "
        "       ckin.bag_pool_refused(bag2.grp_id, bag2.bag_pool_num, pax_grp.class, pax_grp.bag_refuse) AS refused "
        "FROM pax_grp,bag2 "
        "WHERE pax_grp.grp_id=bag2.grp_id AND bag2.grp_id=:grp_id FOR UPDATE";
    BagQry.CreateVariable("grp_id",otInteger,grp_id);
    BagQry.Execute();
    for(;!BagQry.Eof;BagQry.Next())
    {
      TBagItem bag;
      bag.fromDB(BagQry);
      old_bags[bag.num]=bag;
      if (!reqInfo->desk.compatible(BAG_WITH_HALL_VERSION))
        refused_bags[bag.num]=BagQry.FieldAsInteger("refused")!=0;
    };

    //��� ����� �ᯮ��㥬 old_bags, �� � ���⪮� � �����⠭�������� ��� value_bag_num
    if (reqInfo->desk.compatible(BAG_WITH_HALL_VERSION))
    {
      //ࠡ�⠥� �� �����䨪��ࠬ
      for(map<int, TBagItem>::iterator ob=old_bags.begin();ob!=old_bags.end();++ob)
      {
        map<int, TBagItem>::const_iterator nb=bags.begin();
        for(;nb!=bags.end();++nb)
          if (nb->second.id==ob->second.id) break;
        if (nb!=bags.end())
          ob->second.value_bag_num=nb->second.value_bag_num;
        else
          ob->second.value_bag_num=ASTRA::NoExists;
      };
    }
    else
    {
      //ࠡ�⠥� ���뢠� ���冷� � ࠧॣ����஢���� �����
      map<int, TBagItem>::const_iterator nb=bags.begin();
      for(map<int, TBagItem>::iterator ob=old_bags.begin();ob!=old_bags.end();++ob)
      {
        ob->second.value_bag_num=ASTRA::NoExists;
        if (refused_bags[ob->second.num]) continue;
        if (nb==bags.end()) throw Exception("TGroupBagItem::fromXML: nb==bags.end()");
        ob->second.value_bag_num=nb->second.value_bag_num;
        ++nb;
      };
      if (nb!=bags.end()) throw Exception("TGroupBagItem::fromXML: nb!=bags.end()");
    };
    bags=old_bags;
  }
  else
  {
    map<int /*num*/, TBagItem> old_bags;
    BagQry.Clear();
    BagQry.SQLText="SELECT * FROM bag2 WHERE grp_id=:grp_id FOR UPDATE";
    BagQry.CreateVariable("grp_id",otInteger,grp_id);
    BagQry.Execute();
    for(;!BagQry.Eof;BagQry.Next())
    {
      TBagItem bag;
      bag.fromDB(BagQry);
      old_bags[bag.num]=bag;
    };
    //�஡������� �� ���஢���� ���ᨢ�� � 楫�� ���������� ����� hall � user_id � ���ᨢ� ������ ������
    if (reqInfo->desk.compatible(BAG_WITH_HALL_VERSION))
    {
      for(map<int, TBagItem>::iterator nb=bags.begin();nb!=bags.end();++nb)
      {
        if (nb->second.id==ASTRA::NoExists)
        {
          //����� �������� �����
          nb->second.hall=hall;
          nb->second.user_id=reqInfo->user.user_id;
          nb->second.desk=reqInfo->desk.code;
          nb->second.time_create=BASIC::NowUTC();
        }
        else
        {
          //���� �����
          map<int, TBagItem>::const_iterator ob=old_bags.begin();
          for(;ob!=old_bags.end();++ob)
            if (ob->second.id==nb->second.id) break;
          if (ob!=old_bags.end())
          {
            nb->second.hall=ob->second.hall;
            nb->second.user_id=ob->second.user_id;
            nb->second.desk=ob->second.desk;
            nb->second.time_create=ob->second.time_create;
            if (!reqInfo->desk.compatible(BAG_TO_RAMP_VERSION))
              nb->second.to_ramp=ob->second.to_ramp;
            if (!reqInfo->desk.compatible(USING_SCALES_VERSION))
            {
              //��࠭塞 ���� using_scales ⮫쪮 �᫨ ob->second.weight==nb->second.weight
              if (ob->second.weight==nb->second.weight)
                nb->second.using_scales=ob->second.using_scales;
              else
                nb->second.using_scales=false;
            };
            if (!reqInfo->desk.compatible(PIECE_CONCEPT_VERSION2))
            {
              nb->second.is_trfer=ob->second.is_trfer;
              if (nb->second.is_trfer)
                nb->second.bag_type=ob->second.bag_type;
            };
            nb->second.handmade=ob->second.handmade;
          }
          else
          {
            nb->second.hall=hall;
            nb->second.user_id=reqInfo->user.user_id;
            nb->second.desk=reqInfo->desk.code;
            nb->second.time_create=BASIC::NowUTC();
          };
        };
      };
    }
    else
    {
      map<int, TBagItem>::const_iterator ob=old_bags.begin();
      for(map<int, TBagItem>::iterator nb=bags.begin();nb!=bags.end();++nb)
      {
        for(;ob!=old_bags.end();++ob)
        {
          if (ob->second.bag_type==nb->second.bag_type &&
              ob->second.pr_cabin==nb->second.pr_cabin &&
              ob->second.amount==  nb->second.amount &&
              ob->second.weight==  nb->second.weight) break;
        };
        if (ob!=old_bags.end())
        {
          nb->second.id=ob->second.id;
          nb->second.hall=ob->second.hall;
          nb->second.user_id=ob->second.user_id;
          nb->second.desk=ob->second.desk;
          nb->second.time_create=ob->second.time_create;
          if (!reqInfo->desk.compatible(BAG_TO_RAMP_VERSION))
            nb->second.to_ramp=ob->second.to_ramp;
          if (!reqInfo->desk.compatible(USING_SCALES_VERSION))
            //⠪ ��� ob->second.weight==nb->second.weight, � ᬥ�� ��࠭塞 ���� using_scales
            nb->second.using_scales=ob->second.using_scales;
          if (!reqInfo->desk.compatible(PIECE_CONCEPT_VERSION2))
            nb->second.is_trfer=ob->second.is_trfer;
          nb->second.handmade=ob->second.handmade;
          ++ob;
        }
        else
        {
          if (hall==ASTRA::NoExists)
            throw Exception("TGroupBagItem::fromXML: unknown hall");
          nb->second.hall=hall;
          nb->second.user_id=reqInfo->user.user_id;
          nb->second.desk=reqInfo->desk.code;
          nb->second.time_create=BASIC::NowUTC();
        };
      };
    };
  };
};

void TGroupBagItem::toDB(int grp_id) const
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  bool is_payment=reqInfo->client_type == ASTRA::ctTerm && reqInfo->screen.name != "AIR.EXE";

  TQuery BagQry(&OraSession);

  BagQry.Clear();
  BagQry.SQLText="DELETE FROM value_bag WHERE grp_id=:grp_id";
  BagQry.CreateVariable("grp_id",otInteger,grp_id);
  BagQry.Execute();
  BagQry.SQLText=
    "INSERT INTO value_bag(grp_id,num,value,value_cur,tax_id,tax,tax_trfer) "
    "VALUES(:grp_id,:num,:value,:value_cur,:tax_id,:tax,:tax_trfer)";
  BagQry.DeclareVariable("num",otInteger);
  BagQry.DeclareVariable("value",otFloat);
  BagQry.DeclareVariable("value_cur",otString);
  BagQry.DeclareVariable("tax_id",otInteger);
  BagQry.DeclareVariable("tax",otFloat);
  BagQry.DeclareVariable("tax_trfer",otInteger);
  for(map<int, TValueBagItem>::const_iterator v=vals.begin();v!=vals.end();++v)
  {
    v->second.toDB(BagQry);
    BagQry.Execute();
  };

  BagQry.Clear();
  BagQry.SQLText="DELETE FROM bag2 WHERE grp_id=:grp_id";
  BagQry.CreateVariable("grp_id",otInteger,grp_id);
  BagQry.Execute();
  BagQry.SQLText=
    "BEGIN "
    "  IF :id IS NULL THEN "
    "    SELECT cycle_id__seq.nextval INTO :id FROM dual; "
    "  END IF; "
    "  INSERT INTO bag2 (grp_id,num,id,bag_type,rfisc,pr_cabin,amount,weight,value_bag_num, "
    "    pr_liab_limit,to_ramp,using_scales,bag_pool_num,hall,user_id,desk,time_create,is_trfer,handmade) "
    "  VALUES (:grp_id,:num,:id,:bag_type,:rfisc,:pr_cabin,:amount,:weight,:value_bag_num, "
    "    :pr_liab_limit,:to_ramp,:using_scales,:bag_pool_num,:hall,:user_id,:desk,:time_create,:is_trfer,:handmade); "
    "END;";
  BagQry.DeclareVariable("num",otInteger);
  BagQry.DeclareVariable("id",otInteger);
  BagQry.DeclareVariable("bag_type",otInteger);
  BagQry.DeclareVariable("rfisc",otString);
  BagQry.DeclareVariable("pr_cabin",otInteger);
  BagQry.DeclareVariable("amount",otInteger);
  BagQry.DeclareVariable("weight",otInteger);
  BagQry.DeclareVariable("value_bag_num",otInteger);
  BagQry.DeclareVariable("pr_liab_limit",otInteger);
  BagQry.DeclareVariable("to_ramp",otInteger);
  BagQry.DeclareVariable("using_scales",otInteger);
  BagQry.DeclareVariable("bag_pool_num",otInteger);
  BagQry.DeclareVariable("hall",otInteger);
  BagQry.DeclareVariable("user_id",otInteger);
  BagQry.DeclareVariable("desk",otString);
  BagQry.DeclareVariable("time_create",otDate);
  BagQry.DeclareVariable("is_trfer",otInteger);
  BagQry.DeclareVariable("handmade",otInteger);
  for(map<int, TBagItem>::const_iterator nb=bags.begin();nb!=bags.end();++nb)
  {
    nb->second.toDB(BagQry);
    BagQry.Execute();
  };

  if (!is_payment)
  {
    BagQry.Clear();
    BagQry.SQLText="DELETE FROM bag_tags WHERE grp_id=:grp_id";
    BagQry.CreateVariable("grp_id",otInteger,grp_id);
    BagQry.Execute();
    BagQry.SQLText=
      "INSERT INTO bag_tags(grp_id,num,tag_type,no,color,bag_num,pr_print) "
      "VALUES (:grp_id,:num,:tag_type,:no,:color,:bag_num,:pr_print)";
    BagQry.DeclareVariable("num",otInteger);
    BagQry.DeclareVariable("tag_type",otString);
    BagQry.DeclareVariable("no",otFloat);
    BagQry.DeclareVariable("color",otString);
    BagQry.DeclareVariable("bag_num",otInteger);
    BagQry.DeclareVariable("pr_print",otInteger);
    for(map<int, TTagItem>::const_iterator t=tags.begin();t!=tags.end();++t)
    {
      t->second.toDB(BagQry);
      BagQry.Execute();
    };
  }; //!pr_payment
};

void TGroupBagItem::toLists(list<TValueBagItem> &vals_list,
                            list<TBagItem> &bags_list,
                            list<TTagItem> &tags_list) const
{
  for(map<int /*num*/, TValueBagItem>::const_iterator i=vals.begin();i!=vals.end();++i)
    vals_list.push_back(i->second);
  for(map<int /*num*/, TBagItem>::const_iterator i=bags.begin();i!=bags.end();++i)
    bags_list.push_back(i->second);
  for(map<int /*num*/, TTagItem>::const_iterator i=tags.begin();i!=tags.end();++i)
    tags_list.push_back(i->second);
};

void TGroupBagItem::fromLists(const list<TValueBagItem> &vals_list,
                              const list<TBagItem> &bags_list,
                              const list<TTagItem> &tags_list)
{
  for(list<TValueBagItem>::const_iterator i=vals_list.begin();i!=vals_list.end();++i)
    if (!vals.insert(make_pair(i->num, *i)).second)
      throw Exception("TGroupBagItem::fromLists: vals[%d] duplicated", i->num);
  for(list<TBagItem>::const_iterator i=bags_list.begin();i!=bags_list.end();++i)
    if (!bags.insert(make_pair(i->num, *i)).second)
      throw Exception("TGroupBagItem::fromLists: bags[%d] duplicated", i->num);
  for(list<TTagItem>::const_iterator i=tags_list.begin();i!=tags_list.end();++i)
    if (!tags.insert(make_pair(i->num, *i)).second)
      throw Exception("TGroupBagItem::fromLists: tags[%d] duplicated", i->num);
};

void TGroupBagItem::normalizeLists(int vals_first_num,
                                   int bags_first_num,
                                   int tags_first_num,
                                   list<TValueBagItem> &vals_list,
                                   list<TBagItem> &bags_list,
                                   list<TTagItem> &tags_list)
{
  int num;
  //㯫��塞 �������� 業�����
  num=vals_first_num;
  for(list<TValueBagItem>::iterator i=vals_list.begin();i!=vals_list.end();++i,num++)
  {
    if (num!=i->num)
    {
      for(list<TBagItem>::iterator j=bags_list.begin();j!=bags_list.end();++j)
        if (j->value_bag_num!=ASTRA::NoExists && j->value_bag_num==i->num) j->value_bag_num=num;
      i->num=num;
    };
  };
  //㯫��塞 �����
  num=bags_first_num;
  for(list<TBagItem>::iterator i=bags_list.begin();i!=bags_list.end();++i,num++)
  {
    if (num!=i->num)
    {
      for(list<TTagItem>::iterator j=tags_list.begin();j!=tags_list.end();++j)
        if (j->bag_num!=ASTRA::NoExists && j->bag_num==i->num) j->bag_num=num;
      i->num=num;
    };
  };
  //㯫��塞 ��ન
  num=tags_first_num;
  for(list<TTagItem>::iterator i=tags_list.begin();i!=tags_list.end();++i,num++)
  {
    if (num!=i->num) i->num=num;
  };
};

void TGroupBagItem::add(const TGroupBagItem &item)
{
  list<TValueBagItem> vals_tmp;
  list<TBagItem> bags_tmp;
  list<TTagItem> tags_tmp;
  item.toLists(vals_tmp, bags_tmp, tags_tmp);
  normalizeLists(vals.size()+1, bags.size()+1, tags.size()+1, vals_tmp, bags_tmp, tags_tmp);
  fromLists(vals_tmp, bags_tmp, tags_tmp);
};

void TGroupBagItem::filterPools(const set<int/*bag_pool_num*/> &pool_nums,
                                bool pool_nums_for_keep)
{
  if (pool_nums.empty()) return;

  list<TValueBagItem> vals_tmp;
  list<TBagItem> bags_tmp;
  list<TTagItem> tags_tmp;
  toLists(vals_tmp, bags_tmp, tags_tmp);

  for(list<TBagItem>::iterator i=bags_tmp.begin();i!=bags_tmp.end();)
  {
    set<int>::const_iterator b=pool_nums.find(i->bag_pool_num);
    if ((!pool_nums_for_keep && b!=pool_nums.end()) ||
        (pool_nums_for_keep && b==pool_nums.end()))
    {
      //㤠�塞 �易���� 業�����
      if (i->value_bag_num!=ASTRA::NoExists)
        for(list<TValueBagItem>::iterator j=vals_tmp.begin();j!=vals_tmp.end();)
        {
          if (j->num==i->value_bag_num) j=vals_tmp.erase(j); else ++j;
        };
      //㤠�塞 �易��� ��ન
      for(list<TTagItem>::iterator j=tags_tmp.begin();j!=tags_tmp.end();)
      {
        if (j->bag_num!=ASTRA::NoExists && j->bag_num==i->num) j=tags_tmp.erase(j); else ++j;
      };
      //㤠�塞 �����
      i=bags_tmp.erase(i);
    }
    else ++i;
  };

  normalizeLists(1, 1, 1, vals_tmp, bags_tmp, tags_tmp);

  clear();

  fromLists(vals_tmp, bags_tmp, tags_tmp);
};

void TGroupBagItem::fromDB(int grp_id, int bag_pool_num, bool without_refused)
{
  vals.clear();
  bags.clear();
  tags.clear();

  TQuery BagQry(&OraSession);
  BagQry.Clear();
  BagQry.SQLText="SELECT * FROM value_bag WHERE grp_id=:grp_id";
  BagQry.CreateVariable("grp_id",otInteger,grp_id);
  BagQry.Execute();
  for(;!BagQry.Eof;BagQry.Next())
  {
    TValueBagItem val;
    val.fromDB(BagQry);
    vals[val.num]=val;
  };

  BagQry.Clear();
  BagQry.SQLText="SELECT * FROM bag2 WHERE grp_id=:grp_id";
  BagQry.CreateVariable("grp_id",otInteger,grp_id);
  BagQry.Execute();
  for(;!BagQry.Eof;BagQry.Next())
  {
    TBagItem bag;
    bag.fromDB(BagQry);
    bags[bag.num]=bag;
  };

  BagQry.Clear();
  BagQry.SQLText=
    "SELECT num, tag_type, no, color, bag_num, pr_print, "
    "       no_len, NVL(printable, 1) AS printable "
    "FROM bag_tags,tag_types "
    "WHERE bag_tags.tag_type=tag_types.code AND grp_id=:grp_id";
  BagQry.CreateVariable("grp_id",otInteger,grp_id);
  BagQry.Execute();
  for(;!BagQry.Eof;BagQry.Next())
  {
    TTagItem tag;
    tag.fromDB(BagQry);
    tags[tag.num]=tag;
  };

  if (bag_pool_num!=ASTRA::NoExists)
  {
    set<int> keep_pools;
    keep_pools.insert(bag_pool_num);
    filterPools(keep_pools, true);
  };

  if (without_refused)
  {
    //���� �ନ��� �� �����ন���� �ਢ離� ������ � ࠧॣ����஢���� ���ᠦ�ࠬ
    //᫥����⥫쭮, �� ������ ���� ࠧॣ����஢���� �����
    BagQry.Clear();
    BagQry.SQLText="SELECT class, bag_refuse FROM pax_grp WHERE grp_id=:grp_id";
    BagQry.CreateVariable("grp_id",otInteger,grp_id);
    BagQry.Execute();
    if (BagQry.Eof) return;
    string cl=BagQry.FieldAsString("class");
    bool bag_refuse=BagQry.FieldAsInteger("bag_refuse")!=0;
    if (!bag_refuse)
    {
      //�筮 �� ���� ����� ࠧॣ����஢��
      BagQry.Clear();
      BagQry.SQLText=
        "SELECT ckin.bag_pool_refused(:grp_id, :bag_pool_num, :class, :bag_refuse) AS refused FROM dual";
      BagQry.CreateVariable("grp_id",otInteger,grp_id);
      BagQry.DeclareVariable("bag_pool_num",otInteger);
      BagQry.CreateVariable("class",otString,cl);
      BagQry.CreateVariable("bag_refuse",otInteger,(int)bag_refuse);
      set<int/*bag_pool_num*/> del_pools;
      for(map<int, TBagItem>::const_iterator i=bags.begin();i!=bags.end();++i)
      {
        if (del_pools.find(i->second.bag_pool_num)==del_pools.end())
        {
          BagQry.SetVariable("bag_pool_num",i->second.bag_pool_num);
          BagQry.Execute();
          if (BagQry.Eof || BagQry.FieldAsInteger("refused")!=0)
            del_pools.insert(i->second.bag_pool_num);
        };
      };
      filterPools(del_pools, false);
    };
  };
};

void TGroupBagItem::toXML(xmlNodePtr bagtagNode) const
{
  if (bagtagNode==NULL) return;

  xmlNodePtr node;
  node=NewTextChild(bagtagNode,"value_bags");
  for(map<int /*num*/, TValueBagItem>::const_iterator i=vals.begin();i!=vals.end();++i)
    i->second.toXML(NewTextChild(node,"value_bag"));

  node=NewTextChild(bagtagNode,"bags");
  for(map<int /*num*/, TBagItem>::const_iterator i=bags.begin();i!=bags.end();++i)
    i->second.toXML(NewTextChild(node,"bag"));

  node=NewTextChild(bagtagNode,"tags");
  for(map<int /*num*/, TTagItem>::const_iterator i=tags.begin();i!=tags.end();++i)
    i->second.toXML(NewTextChild(node,"tag"));
};

void TGroupBagItem::convertBag(std::multimap<int /*id*/, TBagItem> &result) const
{
  //multimap - ��⮬� �� �.�. id=NoExists �᫨ ����������� �ࠧ� ��᪮�쪮 ���� ������
  result.clear();
  for(multimap<int /*num*/, TBagItem>::const_iterator b=bags.begin(); b!=bags.end(); ++b)
    result.insert(make_pair(b->second.id, b->second));
}

const TPaidBagEMDItem& TPaidBagEMDItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  NewTextChild(node,"bag_type",bag_type_str());
  NewTextChild(node,"transfer_num",trfer_num);
  NewTextChild(node,"emd_no",emd_no);
  NewTextChild(node,"emd_coupon",emd_coupon);
  NewTextChild(node,"weight",weight);
  return *this;
};

TPaidBagEMDItem& TPaidBagEMDItem::fromXML(xmlNodePtr node, bool piece_concept)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;

  if (!piece_concept)
  {
    if (!NodeIsNULLFast("bag_type",node2))
      bag_type=NodeAsIntegerFast("bag_type",node2);
  }
  else rfisc=NodeAsStringFast("bag_type",node2);

  if (TReqInfo::Instance()->client_type==ASTRA::ctTerm && TReqInfo::Instance()->desk.compatible(PIECE_CONCEPT_VERSION))
    trfer_num=NodeAsIntegerFast("transfer_num",node2);
  else
    trfer_num=0;
  emd_no=NodeAsStringFast("emd_no",node2);
  emd_coupon=NodeAsIntegerFast("emd_coupon",node2);
  weight=NodeAsIntegerFast("weight",node2);
  return *this;
};

const TPaidBagEMDItem& TPaidBagEMDItem::toDB(TQuery &Qry) const
{
  if (bag_type!=ASTRA::NoExists)
    Qry.SetVariable("bag_type",bag_type);
  else
    Qry.SetVariable("bag_type",FNull);
  Qry.SetVariable("rfisc",rfisc);
  Qry.SetVariable("transfer_num",trfer_num);
  Qry.SetVariable("emd_no",emd_no);
  Qry.SetVariable("emd_coupon",emd_coupon);
  Qry.SetVariable("weight",weight);
  pax_id!=ASTRA::NoExists?Qry.SetVariable("pax_id",pax_id):
                          Qry.SetVariable("pax_id",FNull);
  return *this;
};

TPaidBagEMDItem& TPaidBagEMDItem::fromDB(TQuery &Qry)
{
  clear();
  if (!Qry.FieldIsNULL("bag_type"))
    bag_type=Qry.FieldAsInteger("bag_type");
  rfisc=Qry.FieldAsString("rfisc");
  trfer_num=Qry.FieldAsInteger("transfer_num");
  emd_no=Qry.FieldAsString("emd_no");
  emd_coupon=Qry.FieldAsInteger("emd_coupon");
  weight=Qry.FieldAsInteger("weight");
  if (!Qry.FieldIsNULL("pax_id"))
    pax_id=Qry.FieldAsInteger("pax_id");
  return *this;
};

std::string TPaidBagEMDItem::no_str() const
{
  ostringstream s;
  s << emd_no;
  if (emd_coupon!=ASTRA::NoExists)
    s << "/" << emd_coupon;
  return s.str();
};

std::string TPaidBagEMDItem::bag_type_str() const
{
  ostringstream s;
  if (rfisc.empty())
  {
    if (bag_type!=ASTRA::NoExists)
      s << setw(2) << setfill('0') << bag_type;
  }
  else s << rfisc;
  return s.str();
};

void PaidBagEMDFromXML(xmlNodePtr emdNode,
                       boost::optional< std::list<TPaidBagEMDItem> > &emd,
                       bool piece_concept)
{
  emd=boost::none;
  if (emdNode==NULL) return;
  emdNode=GetNode("paid_bag_emd",emdNode);
  if (emdNode!=NULL)
  {
    emd=list<TPaidBagEMDItem>();
    for(xmlNodePtr node=emdNode->children;node!=NULL;node=node->next)
    {
      TPaidBagEMDItem item;
      item.fromXML(node, piece_concept);
      if (/*item.trfer_num!=0 || !!!vlad */
          item.bag_type==99) continue;  //!!!vlad ��⮬ ��������
      if (piece_concept && item.rfisc.empty())
        throw UserException("MSG.EMD_ATTACHED_TO_UNKNOWN_RFISC", LParams()<<LParam("emd_no",item.no_str()));
      emd.get().push_back(item);
    };
  };
};

void PaidBagEMDToDB(int grp_id,
                    const boost::optional< std::list<TPaidBagEMDItem> > &emd)
{
  if (!emd) return;
  TQuery BagQry(&OraSession);
  BagQry.Clear();
  BagQry.SQLText="DELETE FROM paid_bag_emd WHERE grp_id=:grp_id";
  BagQry.CreateVariable("grp_id",otInteger,grp_id);
  BagQry.Execute();
  BagQry.SQLText=
    "INSERT INTO paid_bag_emd(grp_id,bag_type,rfisc,transfer_num,emd_no,emd_coupon,weight,pax_id) "
    "VALUES(:grp_id,:bag_type,:rfisc,:transfer_num,:emd_no,:emd_coupon,:weight,:pax_id)";
  BagQry.DeclareVariable("bag_type",otInteger);
  BagQry.DeclareVariable("rfisc",otString);
  BagQry.DeclareVariable("transfer_num",otInteger);
  BagQry.DeclareVariable("emd_no",otString);
  BagQry.DeclareVariable("emd_coupon",otInteger);
  BagQry.DeclareVariable("weight",otInteger);
  BagQry.DeclareVariable("pax_id",otInteger);
  for(list<TPaidBagEMDItem>::const_iterator i=emd.get().begin(); i!=emd.get().end(); ++i)
  {
    i->toDB(BagQry);
    try
    {
      BagQry.Execute();
    }
    catch(EOracleError E)
    {
      if (E.Code==1)
        throw UserException("MSG.DUPLICATED_EMD_NUMBER", LParams()<<LParam("emd_no",i->no_str()));
      else
        throw;
    };
  };
};

void PaidBagEMDFromDB(int grp_id,
                      std::list<TPaidBagEMDItem> &emd)
{
  TQuery BagQry(&OraSession);
  BagQry.Clear();
  BagQry.SQLText=
    "SELECT * FROM paid_bag_emd WHERE grp_id=:grp_id";
  BagQry.CreateVariable("grp_id",otInteger,grp_id);
  BagQry.Execute();
  for(;!BagQry.Eof;BagQry.Next())
    emd.push_back(TPaidBagEMDItem().fromDB(BagQry));

};

void PaidBagEMDToXML(const std::list<TPaidBagEMDItem> &emd,
                     xmlNodePtr emdNode)
{
  if (emdNode==NULL) return;

  xmlNodePtr node=NewTextChild(emdNode,"paid_bag_emd");
  for(list<TPaidBagEMDItem>::const_iterator i=emd.begin(); i!=emd.end(); ++i)
    i->toXML(NewTextChild(node,"emd"));
}

void PaidBagEMDPropsFromDB(int grp_id, TPaidBagEMDProps &props)
{
  props.clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT * FROM paid_bag_emd_props WHERE grp_id=:grp_id";
  Qry.CreateVariable("grp_id", otInteger, grp_id);
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next())
    props.insert(TPaidBagEMDPropsItem(Qry.FieldAsString("emd_no"),
                                      Qry.FieldAsInteger("emd_coupon"),
                                      Qry.FieldAsInteger("manual_bind")!=0));
}

void PaidBagEMDPropsToDB(int grp_id, const TPaidBagEMDProps &props)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  UPDATE paid_bag_emd_props "
    "  SET grp_id=:grp_id, manual_bind=:manual_bind "
    "  WHERE emd_no=:emd_no AND emd_coupon=:emd_coupon; "
    "  IF SQL%NOTFOUND THEN "
    "    INSERT INTO paid_bag_emd_props(grp_id, emd_no, emd_coupon, manual_bind) "
    "    VALUES(:grp_id, :emd_no, :emd_coupon, :manual_bind); "
    "  END IF; "
    "END;";
  Qry.CreateVariable("grp_id", otInteger, grp_id);
  Qry.DeclareVariable("emd_no", otString);
  Qry.DeclareVariable("emd_coupon", otInteger);
  Qry.DeclareVariable("manual_bind" ,otInteger);
  for(TPaidBagEMDProps::const_iterator i=props.begin(); i!=props.end(); ++i)
  {
    Qry.SetVariable("emd_no", i->emd_no);
    Qry.SetVariable("emd_coupon", i->emd_coupon);
    Qry.SetVariable("manual_bind" ,(int)i->manual_bind);
    Qry.Execute();
  };
}

} //namespace CheckIn

namespace WeightConcept
{

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
      result << " " << amount << getLocaleText("�", lang) << weight;
    else
      result << " " << weight;
    if (per_unit!=ASTRA::NoExists && per_unit!=0)
      result << getLocaleText("��/�", lang);
    else
      result << getLocaleText("��", lang);
  }
  else
  {
    if (amount!=ASTRA::NoExists)
      result << " " << amount << getLocaleText("�", lang);
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
  NewTextChild(node,"bag_type",bag_type_str());
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
  if (!NodeIsNULLFast("bag_type",node2))
    bag_type=NodeAsIntegerFast("bag_type",node2);
  if (!NodeIsNULLFast("norm_id",node2))
  {
    norm_id=NodeAsIntegerFast("norm_id",node2);
    norm_trfer=NodeAsIntegerFast("norm_trfer",node2,0)!=0;
  };
  return *this;
};

const TPaxNormItem& TPaxNormItem::toDB(TQuery &Qry) const
{
  if (bag_type!=ASTRA::NoExists)
    Qry.SetVariable("bag_type",bag_type);
  else
    Qry.SetVariable("bag_type",FNull);
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
  if (!Qry.FieldIsNULL("bag_type"))
    bag_type=Qry.FieldAsInteger("bag_type");
  if (!Qry.FieldIsNULL("norm_id"))
  {
    norm_id=Qry.FieldAsInteger("norm_id");
    norm_trfer=Qry.FieldAsInteger("norm_trfer")!=0;
  };
  handmade=(int)(Qry.FieldAsInteger("handmade")!=0);
  return *this;
};

std::string TPaxNormItem::bag_type_str() const
{
  ostringstream s;
  if (bag_type!=ASTRA::NoExists)
    s << setw(2) << setfill('0') << bag_type;
  return s.str();
};

bool PaxNormsFromDB(BASIC::TDateTime part_key, int pax_id, list< pair<TPaxNormItem, TNormItem> > &norms)
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
    if (norms.back().first.bag_type==99) norms.pop_back();  //!!!vlad ��⮬ 㤠����
  };

  return !norms.empty();
};

bool GrpNormsFromDB(BASIC::TDateTime part_key, int grp_id, list< pair<TPaxNormItem, TNormItem> > &norms)
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
    if (norms.back().first.bag_type==99) norms.pop_back();  //!!!vlad ��⮬ 㤠����
  };

  return !norms.empty();
};

void NormsToXML(const list< pair<TPaxNormItem, TNormItem> > &norms, const CheckIn::TGroupBagItem &group_bag, xmlNodePtr node)
{
  if (node==NULL) return;

  xmlNodePtr normsNode=NewTextChild(node,"norms");
  for(list< pair<TPaxNormItem, TNormItem> >::const_iterator i=norms.begin();i!=norms.end();++i)
  {
    xmlNodePtr normNode=NewTextChild(normsNode,"norm");
    i->first.toXML(normNode);
    i->second.toXML(normNode);
  };

  if (!TReqInfo::Instance()->desk.compatible(PIECE_CONCEPT_VERSION2) && group_bag.trferExists())
  {
    //������� ����।������� ���� �� 99 ������ ��� ����� �ନ�����
    xmlNodePtr normNode=NewTextChild(normsNode,"norm");
    TPaxNormItem paxNormItem;
    paxNormItem.bag_type=99;
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
    result.insert(make_pair(i->first.bag_type_str(), i->second));
  };
}

const TPaidBagItem& TPaidBagItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  NewTextChild(node,"bag_type",bag_type_str());
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
  return *this;
};

TPaidBagItem& TPaidBagItem::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  if (!NodeIsNULLFast("bag_type",node2))
    bag_type=NodeAsIntegerFast("bag_type",node2);
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
  if (bag_type!=ASTRA::NoExists)
    Qry.SetVariable("bag_type",bag_type);
  else
    Qry.SetVariable("bag_type",FNull);
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
  if (!Qry.FieldIsNULL("bag_type"))
    bag_type=Qry.FieldAsInteger("bag_type");
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

std::string TPaidBagItem::bag_type_str() const
{
  ostringstream s;
  if (bag_type!=ASTRA::NoExists)
    s << setw(2) << setfill('0') << bag_type;
  return s.str();
};

void PaidBagFromXML(xmlNodePtr paidbagNode,
                    boost::optional< list<TPaidBagItem> > &paid)
{
  paid=boost::none;
  if (paidbagNode==NULL) return;
  xmlNodePtr paidBagNode=GetNode("paid_bags",paidbagNode);
  if (paidBagNode!=NULL)
  {
    paid=list<TPaidBagItem>();
    for(xmlNodePtr node=paidBagNode->children;node!=NULL;node=node->next)
    {
      paid.get().push_back(TPaidBagItem().fromXML(node));
      if (!TReqInfo::Instance()->desk.compatible(PIECE_CONCEPT_VERSION2))
      {
        if (paid.get().back().bag_type==99) paid.get().pop_back();
      };
    };
  };
};

void PaidBagToDB(int grp_id,
                 const boost::optional< list<TPaidBagItem> > &paid)
{
  if (!paid) return;
  TQuery BagQry(&OraSession);
  BagQry.Clear();
  BagQry.SQLText="DELETE FROM paid_bag WHERE grp_id=:grp_id";
  BagQry.CreateVariable("grp_id",otInteger,grp_id);
  BagQry.Execute();
  BagQry.SQLText=
    "INSERT INTO paid_bag(grp_id,bag_type,weight,rate_id,rate_trfer,handmade) "
    "VALUES(:grp_id,:bag_type,:weight,:rate_id,:rate_trfer,:handmade)";
  BagQry.DeclareVariable("bag_type",otInteger);
  BagQry.DeclareVariable("weight",otInteger);
  BagQry.DeclareVariable("rate_id",otInteger);
  BagQry.DeclareVariable("rate_trfer",otInteger);
  BagQry.DeclareVariable("handmade",otInteger);
  int excess=0;
  for(list<TPaidBagItem>::const_iterator i=paid.get().begin(); i!=paid.get().end(); ++i)
  {
    excess+=i->weight;
    i->toDB(BagQry);
    BagQry.Execute();
  };

  BagQry.Clear();
  BagQry.SQLText="UPDATE pax_grp SET excess=:excess WHERE grp_id=:grp_id";
  BagQry.CreateVariable("grp_id", otInteger, grp_id);
  BagQry.CreateVariable("excess", otInteger, excess);
  BagQry.Execute();
};

void PaidBagFromDB(BASIC::TDateTime part_key, int grp_id, list<TPaidBagItem> &paid)
{
  paid.clear();
  const char* sql=
      "SELECT paid_bag.bag_type,paid_bag.weight,paid_bag.handmade, "
      "       paid_bag.rate_id,bag_rates.rate,bag_rates.rate_cur,paid_bag.rate_trfer "
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
  {
    paid.push_back(TPaidBagItem().fromDB(BagQry.get()));
    if (paid.back().bag_type==99) paid.pop_back();  //!!!vlad ��⮬ 㤠����
  };
}

void PaidBagToXML(const list<TPaidBagItem> &paid, const CheckIn::TGroupBagItem &group_bag, xmlNodePtr paidbagNode)
{
  if (paidbagNode==NULL) return;

  xmlNodePtr node=NewTextChild(paidbagNode,"paid_bags");
  for(list<TPaidBagItem>::const_iterator i=paid.begin(); i!=paid.end(); ++i)
  {
    xmlNodePtr paidBagNode=NewTextChild(node,"paid_bag");
    i->toXML(paidBagNode);
  };

  if (!TReqInfo::Instance()->desk.compatible(PIECE_CONCEPT_VERSION2) && group_bag.trferExists())
  {
    //������� �㫥��� ����� 99 ����� ��� ����� �ନ�����
    xmlNodePtr paidBagNode=NewTextChild(node,"paid_bag");
    TPaidBagItem item;
    item.bag_type=99;
    item.weight=0;
    item.handmade=false;
    item.toXML(paidBagNode);
  };
}

} //namespace WeightConcept


