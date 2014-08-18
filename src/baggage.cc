#include "baggage.h"
#include "astra_consts.h"
#include "astra_locale.h"
#include "astra_utils.h"
#include "base_tables.h"
#include "basic.h"
#include "term_version.h"
#include "astra_misc.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace std;
using namespace AstraLocale;
using namespace EXCEPTIONS;

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
  if (bag_type!=ASTRA::NoExists)
    NewTextChild(node,"bag_type",bag_type);
  else
    NewTextChild(node,"bag_type");
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
  NewTextChild(node,"is_trfer",(int)is_trfer);
  if (TReqInfo::Instance()->desk.compatible(VERSION_WITH_BAG_POOLS))
    NewTextChild(node,"bag_pool_num",bag_pool_num);
  return *this;
};

TBagItem& TBagItem::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  id=NodeAsIntegerFast("id",node2,ASTRA::NoExists);
  num=NodeAsIntegerFast("num",node2);
  if (!NodeIsNULLFast("bag_type",node2))
    bag_type=NodeAsIntegerFast("bag_type",node2);
  pr_cabin=NodeAsIntegerFast("pr_cabin",node2)!=0;
  amount=NodeAsIntegerFast("amount",node2);
  weight=NodeAsIntegerFast("weight",node2);
  if (!NodeIsNULLFast("value_bag_num",node2))
    value_bag_num=NodeAsIntegerFast("value_bag_num",node2);
  pr_liab_limit=NodeAsIntegerFast("pr_liab_limit",node2)!=0;
  
  if (TReqInfo::Instance()->desk.compatible(BAG_TO_RAMP_VERSION))
    to_ramp=NodeAsIntegerFast("to_ramp",node2)!=0;
  else
    to_ramp=false;
    
  if (TReqInfo::Instance()->desk.compatible(USING_SCALES_VERSION))
    using_scales=NodeAsIntegerFast("using_scales",node2)!=0;
  else
    using_scales=false;
    
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
  Qry.SetVariable("is_trfer",(int)is_trfer);
  return *this;
};

TBagItem& TBagItem::fromDB(TQuery &Qry)
{
  clear();
  id=Qry.FieldAsInteger("id");
  num=Qry.FieldAsInteger("num");
  if (!Qry.FieldIsNULL("bag_type"))
    bag_type=Qry.FieldAsInteger("bag_type");
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
  is_trfer=Qry.FieldAsInteger("is_trfer")!=0;
  return *this;
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
  //получим последний использованный диапазон (+лочка):
  int last_range=Qry.GetVariableAsInteger("range");

  int range;
  int no;
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
      if (k>5) //среди уже существующих диапазонов нет подходящего - берем новый
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
        no=0; //нулевая бирка 000000 запрещена IATA
      };


      if (range==last_range)
        throw EXCEPTIONS::Exception("CheckIn::GetNextTagNo: free range not found (aircode=%d)",aircode);

      Qry.Clear();
      Qry.SQLText="SELECT range FROM tag_ranges2 WHERE aircode=:aircode AND range=:range";
      Qry.CreateVariable("aircode",otInteger,aircode);
      Qry.CreateVariable("range",otInteger,range);
      Qry.Execute();
      if (!Qry.Eof) continue; //этот диапазон используется
    };

    pair<int,int> tag_range;
    tag_range.first=aircode*1000000+range*100+no+1; //первая неиспользовання бирка от старого диапазона

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

    tag_range.second=aircode*1000000+range*100+no; //последняя использовання бирка нового диапазона
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

bool TGroupBagItem::fromXML(xmlNodePtr bagtagNode, bool &pr_tag_print)
{
  vals.clear();
  bags.clear();
  tags.clear();
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
    bag.fromXML(node);
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

void TGroupBagItem::fromXMLadditional(int point_id, int grp_id, int hall, bool pr_tag_print)
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
    //в кассе не можем привязывать бирки и добавлять багаж поэтому делаем это только для регистрации
    if (unbound_tags>0 && !reqInfo->desk.compatible(VERSION_WITH_BAG_POOLS))
    {
      //автоматическая привязка бирок для старых терминалов
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

    //подсчитаем кол-во багажа и баг. бирок
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
        //получим номера печатаемых бирок
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
        //привязываем к багажу
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
      //вызов ckin.bag_pool_refused перед записью багажа прокатывает только если
      //перед этим нет изменений в группе пассажиров (пулы могут потерять актуальность)
      //условие соблюдается (все нормально) если запись багажа из модуля "Оплата багажа"
      BagQry.SQLText=
        "SELECT bag2.grp_id, "
        "       bag2.num, "
        "       bag2.bag_type, "
        "       bag2.pr_cabin, "
        "       bag2.amount, "
        "       bag2.weight, "
        "       bag2.value_bag_num, "
        "       bag2.pr_liab_limit, "
        "       bag2.to_ramp, "
        "       bag2.using_scales, "
        "       bag2.bag_pool_num, "
        "       bag2.id, "
        "       bag2.hall, "
        "       bag2.user_id, "
        "       bag2.is_trfer, "
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

    //для кассы используем old_bags, но с очисткой и переустановлением всех value_bag_num
    if (reqInfo->desk.compatible(BAG_WITH_HALL_VERSION))
    {
      //работаем по идентификаторам
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
      //работаем учитывая порядок и разрегистрированный багаж
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
    //пробегаемся по сортированным массивам с целью заполнения старых hall и user_id в массиве нового багажа
    if (reqInfo->desk.compatible(BAG_WITH_HALL_VERSION))
    {
      for(map<int, TBagItem>::iterator nb=bags.begin();nb!=bags.end();++nb)
      {
        if (nb->second.id==ASTRA::NoExists)
        {
          //вновь введенный багаж
          nb->second.hall=hall;
          nb->second.user_id=reqInfo->user.user_id;
        }
        else
        {
          //старый багаж
          map<int, TBagItem>::const_iterator ob=old_bags.begin();
          for(;ob!=old_bags.end();++ob)
            if (ob->second.id==nb->second.id) break;
          if (ob!=old_bags.end())
          {
            nb->second.hall=ob->second.hall;
            nb->second.user_id=ob->second.user_id;
            if (!reqInfo->desk.compatible(BAG_TO_RAMP_VERSION))
              nb->second.to_ramp=ob->second.to_ramp;
            if (!reqInfo->desk.compatible(USING_SCALES_VERSION))
            {
              //сохраняем старый using_scales только если ob->second.weight==nb->second.weight
              if (ob->second.weight==nb->second.weight)
                nb->second.using_scales=ob->second.using_scales;
              else
                nb->second.using_scales=false;
            };
            nb->second.is_trfer=ob->second.is_trfer;
          }
          else
          {
            nb->second.hall=hall;
            nb->second.user_id=reqInfo->user.user_id;
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
          if (!reqInfo->desk.compatible(BAG_TO_RAMP_VERSION))
            nb->second.to_ramp=ob->second.to_ramp;
          if (!reqInfo->desk.compatible(USING_SCALES_VERSION))
            //так как ob->second.weight==nb->second.weight, то смело сохраняем старый using_scales
            nb->second.using_scales=ob->second.using_scales;
          nb->second.is_trfer=ob->second.is_trfer;
          ++ob;
        }
        else
        {
          if (hall==ASTRA::NoExists)
            throw Exception("TGroupBagItem::fromXML: unknown hall");
          nb->second.hall=hall;
          nb->second.user_id=reqInfo->user.user_id;
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
    "  INSERT INTO bag2 (grp_id,num,id,bag_type,pr_cabin,amount,weight,value_bag_num, "
    "    pr_liab_limit,to_ramp,using_scales,bag_pool_num,hall,user_id,is_trfer) "
    "  VALUES (:grp_id,:num,:id,:bag_type,:pr_cabin,:amount,:weight,:value_bag_num, "
    "    :pr_liab_limit,:to_ramp,:using_scales,:bag_pool_num,:hall,:user_id,:is_trfer); "
    "END;";
  BagQry.DeclareVariable("num",otInteger);
  BagQry.DeclareVariable("id",otInteger);
  BagQry.DeclareVariable("bag_type",otInteger);
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
  BagQry.DeclareVariable("is_trfer",otInteger);
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

void SaveBag(int point_id, int grp_id, int hall, xmlNodePtr bagtagNode)
{
  TGroupBagItem grp;
  bool pr_tag_print;
  if (grp.fromXML(bagtagNode, pr_tag_print))
  {
    grp.fromXMLadditional(point_id, grp_id, hall, pr_tag_print);
    grp.toDB(grp_id);
  };
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
  //уплотняем объявленную ценность
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
  //уплотняем багаж
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
  //уплотняем бирки
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
      //удаляем связанную ценность
      if (i->value_bag_num!=ASTRA::NoExists)
        for(list<TValueBagItem>::iterator j=vals_tmp.begin();j!=vals_tmp.end();)
        {
          if (j->num==i->value_bag_num) j=vals_tmp.erase(j); else ++j;
        };
      //удаляем связанные бирки
      for(list<TTagItem>::iterator j=tags_tmp.begin();j!=tags_tmp.end();)
      {
        if (j->bag_num!=ASTRA::NoExists && j->bag_num==i->num) j=tags_tmp.erase(j); else ++j;
      };
      //удаляем багаж
      i=bags_tmp.erase(i);
    }
    else ++i;
  };

  normalizeLists(1, 1, 1, vals_tmp, bags_tmp, tags_tmp);

  clear();

  fromLists(vals_tmp, bags_tmp, tags_tmp);
};

//void TrferBagFromDB(int grp_id,

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
    //старый терминал не поддерживает привязку багажа к разрегистрированным пассажирам
    //следовательно, мы должны убрать разрегистрированный багаж
    BagQry.Clear();
    BagQry.SQLText="SELECT class, bag_refuse FROM pax_grp WHERE grp_id=:grp_id";
    BagQry.CreateVariable("grp_id",otInteger,grp_id);
    BagQry.Execute();
    if (BagQry.Eof) return;
    string cl=BagQry.FieldAsString("class");
    bool bag_refuse=BagQry.FieldAsInteger("bag_refuse")!=0;
    if (!bag_refuse)
    {
      //точно не весь багаж разрегистрирован
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

void LoadBag(int grp_id, xmlNodePtr bagtagNode)
{
  if (bagtagNode==NULL) return;
  
  TGroupBagItem grp;

  grp.fromDB(grp_id, ASTRA::NoExists, !TReqInfo::Instance()->desk.compatible(VERSION_WITH_BAG_POOLS));
  grp.toXML(bagtagNode);
};

const TNormItem& TNormItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  NewTextChild(node,"norm_type", norm_type);
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
  norm_type=Qry.FieldAsString("norm_type");
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
  result << ElemIdToPrefferedElem(etBagNormType, norm_type, efmtCodeNative, lang);
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

  prmenum.prms << PrmElem<string>("", etBagNormType, norm_type, efmtCodeNative)
               << PrmSmpl<string>("", " ");

  if (weight!=ASTRA::NoExists)
  {
    if (amount!=ASTRA::NoExists)
      prmenum.prms << PrmSmpl<int>("", amount) << PrmLexema("", "EVT.P") << PrmSmpl<int>("", weight);
    else
      prmenum.prms << PrmSmpl<int>("", weight);
    if (per_unit!=ASTRA::NoExists && per_unit!=0)
      prmenum.prms << PrmLexema("", "EVT.KG_P");
    else
      prmenum.prms << PrmLexema("", "EVT.KG");
  }
  else
  {
    if (amount!=ASTRA::NoExists)
      prmenum.prms << PrmSmpl<int>("", amount) << PrmLexema("", "EVT.P");
  };
}

const TPaxNormItem& TPaxNormItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  if (bag_type!=ASTRA::NoExists)
    NewTextChild(node,"bag_type",bag_type);
  else
    NewTextChild(node,"bag_type");
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
  return *this;
};

const TPaidBagItem& TPaidBagItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  if (bag_type!=ASTRA::NoExists)
    NewTextChild(node,"bag_type",bag_type);
  else
    NewTextChild(node,"bag_type");
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
  return *this;
};

bool PaidBagFromXML(xmlNodePtr paidbagNode,
                    list<TPaidBagItem> &paid)
{
  paid.clear();
  if (paidbagNode==NULL) return false;
  xmlNodePtr paidBagNode=GetNode("paid_bags",paidbagNode);
  if (paidBagNode!=NULL)
  {
    for(xmlNodePtr node=paidBagNode->children;node!=NULL;node=node->next)
      paid.push_back(TPaidBagItem().fromXML(node));
    return true;
  };
  return false;
};

void PaidBagToDB(int grp_id,
                 const list<TPaidBagItem> &paid)
{
  TQuery BagQry(&OraSession);
  BagQry.Clear();
  BagQry.SQLText="DELETE FROM paid_bag WHERE grp_id=:grp_id";
  BagQry.CreateVariable("grp_id",otInteger,grp_id);
  BagQry.Execute();
  BagQry.SQLText=
    "INSERT INTO paid_bag(grp_id,bag_type,weight,rate_id,rate_trfer) "
    "VALUES(:grp_id,:bag_type,:weight,:rate_id,:rate_trfer)";
  BagQry.DeclareVariable("bag_type",otInteger);
  BagQry.DeclareVariable("weight",otInteger);
  BagQry.DeclareVariable("rate_id",otInteger);
  BagQry.DeclareVariable("rate_trfer",otInteger);
  for(list<TPaidBagItem>::const_iterator i=paid.begin(); i!=paid.end(); ++i)
  {
    i->toDB(BagQry);
    BagQry.Execute();
  };  
};

void SavePaidBag(int grp_id, xmlNodePtr paidbagNode)
{
  list<TPaidBagItem> paid;
  if (PaidBagFromXML(paidbagNode, paid))
    PaidBagToDB(grp_id, paid);
};

void LoadPaidBag(int grp_id, xmlNodePtr paidbagNode)
{
  if (paidbagNode==NULL) return;

  xmlNodePtr node=NewTextChild(paidbagNode,"paid_bags");
  TQuery BagQry(&OraSession);
  BagQry.Clear();
  BagQry.SQLText=
    "SELECT paid_bag.bag_type,paid_bag.weight, "
    "       rate_id,rate,rate_cur,rate_trfer "
    "FROM paid_bag,bag_rates "
    "WHERE paid_bag.rate_id=bag_rates.id(+) AND grp_id=:grp_id";
  BagQry.CreateVariable("grp_id",otInteger,grp_id);
  BagQry.Execute();
  for(;!BagQry.Eof;BagQry.Next())
  {
    xmlNodePtr paidBagNode=NewTextChild(node,"paid_bag");
    TPaidBagItem().fromDB(BagQry).toXML(paidBagNode);
  };
};

const TPaidBagEMDItem& TPaidBagEMDItem::toXML(xmlNodePtr node) const
{
  if (node==NULL) return *this;
  if (bag_type!=ASTRA::NoExists)
    NewTextChild(node,"bag_type",bag_type);
  else
    NewTextChild(node,"bag_type");
  NewTextChild(node,"emd_no",emd_no);
  NewTextChild(node,"emd_coupon",emd_coupon);
  NewTextChild(node,"weight",weight);
  return *this;
};

TPaidBagEMDItem& TPaidBagEMDItem::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  if (!NodeIsNULLFast("bag_type",node2))
    bag_type=NodeAsIntegerFast("bag_type",node2);
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
  Qry.SetVariable("emd_no",emd_no);
  Qry.SetVariable("emd_coupon",emd_coupon);
  Qry.SetVariable("weight",weight);
  return *this;
};

TPaidBagEMDItem& TPaidBagEMDItem::fromDB(TQuery &Qry)
{
  clear();
  if (!Qry.FieldIsNULL("bag_type"))
    bag_type=Qry.FieldAsInteger("bag_type");
  emd_no=Qry.FieldAsString("emd_no");
  emd_coupon=Qry.FieldAsInteger("emd_coupon");
  weight=Qry.FieldAsInteger("weight");
  return *this;
};

bool PaidBagEMDFromXML(xmlNodePtr emdNode,
                       std::list<TPaidBagEMDItem> &emd)
{
  emd.clear();
  if (emdNode==NULL) return false;
  emdNode=GetNode("paid_bag_emd",emdNode);
  if (emdNode!=NULL)
  {
    for(xmlNodePtr node=emdNode->children;node!=NULL;node=node->next)
      emd.push_back(TPaidBagEMDItem().fromXML(node));
    return true;
  };
  return false;
};

void PaidBagEMDToDB(int grp_id,
                    const std::list<TPaidBagEMDItem> &emd)
{
  TQuery BagQry(&OraSession);
  BagQry.Clear();
  BagQry.SQLText="DELETE FROM paid_bag_emd WHERE grp_id=:grp_id";
  BagQry.CreateVariable("grp_id",otInteger,grp_id);
  BagQry.Execute();
  BagQry.SQLText=
    "INSERT INTO paid_bag_emd(grp_id,bag_type,emd_no,emd_coupon,weight) "
    "VALUES(:grp_id,:bag_type,:emd_no,:emd_coupon,:weight)";
  BagQry.DeclareVariable("bag_type",otInteger);
  BagQry.DeclareVariable("emd_no",otString);
  BagQry.DeclareVariable("emd_coupon",otInteger);
  BagQry.DeclareVariable("weight",otInteger);
  for(list<TPaidBagEMDItem>::const_iterator i=emd.begin(); i!=emd.end(); ++i)
  {
    i->toDB(BagQry);
    BagQry.Execute();
  };
};

void SavePaidBagEMD(int grp_id, xmlNodePtr emdNode)
{
  list<TPaidBagEMDItem> emd;
  if (PaidBagEMDFromXML(emdNode, emd))
    PaidBagEMDToDB(grp_id, emd);
};

void LoadPaidBagEMD(int grp_id, xmlNodePtr emdNode)
{
  if (emdNode==NULL) return;

  xmlNodePtr node=NewTextChild(emdNode,"paid_bag_emd");
  TQuery BagQry(&OraSession);
  BagQry.Clear();
  BagQry.SQLText=
    "SELECT bag_type, emd_no, emd_coupon, weight FROM paid_bag_emd WHERE grp_id=:grp_id";
  BagQry.CreateVariable("grp_id",otInteger,grp_id);
  BagQry.Execute();
  for(;!BagQry.Eof;BagQry.Next())
    TPaidBagEMDItem().fromDB(BagQry).toXML(NewTextChild(node,"emd"));
};

}; //namespace CheckIn

namespace BagPayment
{

class TBagNormFields
{
  public:
    //string name;
    int amount;
    bool filtered;
    int fieldIdx;
    otFieldType fieldType;
    string value;
    TBagNormFields(/*string pname,*/ int pamount, bool pfiltered):
                   /*name(pname),*/amount(pamount),filtered(pfiltered)
    {};
};

void GetPaxBagNorm(TQuery &Qry,
                   const bool use_mixed_norms,
                   const TPaxInfo &pax,
                   const bool onlyCategory,
                   pair<CheckIn::TPaxNormItem, CheckIn::TNormItem> &norm)
{
  norm.first.clear();
  norm.second.clear();

  static map<string,TBagNormFields> BagNormFields;
  if (BagNormFields.empty())
  {
    BagNormFields.insert(make_pair("city_arv",TBagNormFields(  1, false)));
    BagNormFields.insert(make_pair("pax_cat", TBagNormFields(100, false)));
    BagNormFields.insert(make_pair("subclass",TBagNormFields( 50, false)));
    BagNormFields.insert(make_pair("class",   TBagNormFields(  1, false)));
    BagNormFields.insert(make_pair("flt_no",  TBagNormFields(  1, true)));
    BagNormFields.insert(make_pair("craft",   TBagNormFields(  1, true)));
  };

  map<string,TBagNormFields>::iterator i;

  for(i=BagNormFields.begin();i!=BagNormFields.end();i++)
  {
    i->second.fieldIdx=Qry.FieldIndex(i->first);
    i->second.fieldType=Qry.FieldType(i->second.fieldIdx);
    i->second.value.clear();
  };

  if ((i=BagNormFields.find("pax_cat"))!=BagNormFields.end())
    i->second.value=pax.pax_cat;
  if ((i=BagNormFields.find("subclass"))!=BagNormFields.end())
    i->second.value=pax.subcl;
  if ((i=BagNormFields.find("class"))!=BagNormFields.end())
    i->second.value=pax.cl;

  bool pr_basic=(!Qry.Eof && Qry.FieldIsNULL("airline"));
  int max_amount=ASTRA::NoExists;
  int curr_amount=ASTRA::NoExists;
  pair<CheckIn::TPaxNormItem, CheckIn::TNormItem> max_norm,curr_norm;

  int pr_trfer=(int)(!pax.final_target.empty());
  for(;pr_trfer>=0;pr_trfer--)
  {
    if ((i=BagNormFields.find("city_arv"))!=BagNormFields.end())
    {
      i->second.value.clear();
      if (pr_trfer!=0)
        i->second.value=pax.final_target;
      else
        i->second.value=pax.target;
    };

    for(;!Qry.Eof;Qry.Next())
    {
      curr_norm.first.clear();
      if (!Qry.FieldIsNULL("bag_type"))
        curr_norm.first.bag_type=Qry.FieldAsInteger("bag_type");
      curr_norm.first.norm_id=Qry.FieldAsInteger("id");
      curr_norm.first.norm_trfer=pr_trfer!=0;

      curr_norm.second.fromDB(Qry);

      if (Qry.FieldIsNULL("airline") && !pr_basic) continue;
      if (!Qry.FieldIsNULL("pr_trfer") && (Qry.FieldAsInteger("pr_trfer")!=0)!=(pr_trfer!=0)) continue;
      if (curr_norm.first.bag_type!=norm.first.bag_type) continue;
      if (onlyCategory && Qry.FieldAsString("pax_cat")!=pax.pax_cat) continue;

      curr_amount=0;
      i=BagNormFields.begin();
      for(;i!=BagNormFields.end();i++)
      {
        if (Qry.FieldIsNULL(i->second.fieldIdx)) continue;
        if (i->second.filtered)
          curr_amount+=i->second.amount;
        else
          if (!i->second.value.empty())
          {
            if (i->second.value==Qry.FieldAsString(i->second.fieldIdx))
              curr_amount+=i->second.amount;
            else
              break;
          }
          else break;
      };
      if (i==BagNormFields.end())
      {
        if (max_amount==ASTRA::NoExists || curr_amount>max_amount)
        {
          max_norm=curr_norm;
          max_amount=curr_amount;
        };
      };
    };
    if ((pr_trfer!=0 && max_amount!=ASTRA::NoExists) || !use_mixed_norms) break;
  };
  norm=max_norm;
};

};
    
    
