#include "baggage.h"
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
  Qry.SetVariable("bag_pool_num",bag_pool_num);
  if (hall!=ASTRA::NoExists)
    Qry.SetVariable("hall",hall);
  else
    Qry.SetVariable("hall",FNull);
  Qry.SetVariable("user_id",user_id);
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
  bag_pool_num=Qry.FieldAsInteger("bag_pool_num");
  if (!Qry.FieldIsNULL("hall"))
    hall=Qry.FieldAsInteger("hall");
  user_id=Qry.FieldAsInteger("user_id");
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
  Qry.SetVariable("seg_no",seg_no);
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
  seg_no=Qry.FieldAsInteger("seg_no");
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
  if (Qry.Eof) throw EXCEPTIONS::Exception("CheckInInterface::GetNextTagNo: group not found (grp_id=%d)",grp_id);

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
        throw EXCEPTIONS::Exception("CheckInInterface::GetNextTagNo: free range not found (aircode=%d)",aircode);

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

void SaveBag(int point_id, int grp_id, int hall, xmlNodePtr bagtagNode)
{
  if (bagtagNode==NULL) return;
  xmlNodePtr node;

  xmlNodePtr valueBagNode=GetNode("value_bags",bagtagNode);
  xmlNodePtr bagNode=GetNode("bags",bagtagNode);
  xmlNodePtr tagNode=GetNode("tags",bagtagNode);
  
  if (valueBagNode==NULL && bagNode==NULL && tagNode==NULL) return;
  if (valueBagNode==NULL) throw Exception("CheckIn::SaveBag: valueBagNode=NULL");
  if (bagNode==NULL) throw Exception("CheckIn::SaveBag: bagNode=NULL");
  if (tagNode==NULL) throw Exception("CheckIn::SaveBag: tagNode=NULL");
  
  TReqInfo *reqInfo = TReqInfo::Instance();
  
  bool is_payment=reqInfo->screen.name != "AIR.EXE";

  map<int /*num*/, TValueBagItem> vals;
  map<int /*num*/, TBagItem> bags;
  map<int /*num*/, int> bound_tags;
  map<int /*num*/, TTagItem> tags;
  map<int /*num*/, TTagItem> generated_tags;

  for(node=valueBagNode->children;node!=NULL;node=node->next)
  {
    TValueBagItem val;
    val.fromXML(node);
    if (vals.find(val.num)!=vals.end())
      throw Exception("CheckIn::SaveBag: vals[%d] duplicated", val.num);
    vals[val.num]=val;
  };

  for(node=bagNode->children;node!=NULL;node=node->next)
  {
    TBagItem bag;
    bag.fromXML(node);
    if (bags.find(bag.num)!=bags.end())
      throw Exception("CheckIn::SaveBag: bags[%d] duplicated", bag.num);
    if (bag.value_bag_num!=ASTRA::NoExists && vals.find(bag.value_bag_num)==vals.end())
      throw Exception("CheckIn::SaveBag: bags[%d].value_bag_num=%d not found", bag.num, bag.value_bag_num);
    bags[bag.num]=bag;
  };

  for(node=tagNode->children;node!=NULL;node=node->next)
  {
    TTagItem tag;
    tag.fromXML(node);
    if (tags.find(tag.num)!=tags.end())
      throw Exception("CheckIn::SaveBag: tags[%d] duplicated", tag.num);
    if (tag.bag_num!=ASTRA::NoExists && bags.find(tag.bag_num)==bags.end())
      throw Exception("CheckIn::SaveBag: tags[%d].bag_num=%d not found", tag.num, tag.bag_num);
    tags[tag.num]=tag;
  };
  
  int num=1;
  for(map<int, TValueBagItem>::const_iterator v=vals.begin();v!=vals.end();++v,num++)
  {
    if (v->second.num!=num)
      throw Exception("CheckIn::SaveBag: vals[%d] not found", v->second.num);
  };
  num=1;
  for(map<int, TBagItem>::const_iterator b=bags.begin();b!=bags.end();++b,num++)
  {
    if (b->second.num!=num)
      throw Exception("CheckIn::SaveBag: bags[%d] not found", b->second.num);
    bound_tags[b->second.num]=0;
  };
  num=1;
  int unbound_tags=0;
  for(map<int, TTagItem>::const_iterator t=tags.begin();t!=tags.end();++t,num++)
  {
    if (t->second.num!=num)
      throw Exception("CheckIn::SaveBag: tags[%d] not found", t->second.num);
    if (t->second.bag_num!=ASTRA::NoExists)
      ++bound_tags[t->second.bag_num];
    else
      ++unbound_tags;
  };
  
  if (!is_payment)
  {
    //� ���� �� ����� �ਢ�뢠�� ��ન � ��������� ����� ���⮬� ������ �� ⮫쪮 ��� ॣ����樨
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
    bool pr_tag_print=NodeAsInteger("@pr_print",tagNode)!=0;
    TQuery Qry(&OraSession);
    if (bagAmount!=tagCount)
    {
      ProgTrace(TRACE5,"bagAmount=%d tagCount=%d",bagAmount,tagCount);
      if (pr_tag_print && tagCount<bagAmount )
      {
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
            if (t==generated_tags.end()) throw Exception("CheckIn::SaveBag: t==generated_tags.end()");
            t->second.bag_num=b->second.num;
            ++t;
            ++bound_tags[b->second.num];
          }
          else ++b;
        };
        if (t!=generated_tags.end()) throw Exception("CheckIn::SaveBag: t!=generated_tags.end()");
      }
      else throw UserException(1,"MSG.CHECKIN.COUNT_BIRKS_NOT_EQUAL_PLACES");
    };
  };

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
        "SELECT bag2.grp_id, "
        "       bag2.num, "
        "       bag2.bag_type, "
        "       bag2.pr_cabin, "
        "       bag2.amount, "
        "       bag2.weight, "
        "       bag2.value_bag_num, "
        "       bag2.pr_liab_limit, "
        "       bag2.bag_pool_num, "
        "       bag2.id, "
        "       bag2.hall, "
        "       bag2.user_id, "
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
        if (nb==bags.end()) throw Exception("CheckIn::SaveBag: nb==bags.end()");
        ob->second.value_bag_num=nb->second.value_bag_num;
        ++nb;
      };
      if (nb!=bags.end()) throw Exception("CheckIn::SaveBag: nb!=bags.end()");
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
          ++ob;
        }
        else
        {
          if (hall==ASTRA::NoExists)
            throw Exception("CheckIn::SaveBag: unknown hall");
          nb->second.hall=hall;
          nb->second.user_id=reqInfo->user.user_id;
        };
      };
    };
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
    "  INSERT INTO bag2 (grp_id,num,id,bag_type,pr_cabin,amount,weight,value_bag_num,pr_liab_limit,bag_pool_num,hall,user_id) "
    "  VALUES (:grp_id,:num,:id,:bag_type,:pr_cabin,:amount,:weight,:value_bag_num,:pr_liab_limit,:bag_pool_num,:hall,:user_id); "
    "END;";
  BagQry.DeclareVariable("num",otInteger);
  BagQry.DeclareVariable("id",otInteger);
  BagQry.DeclareVariable("bag_type",otInteger);
  BagQry.DeclareVariable("pr_cabin",otInteger);
  BagQry.DeclareVariable("amount",otInteger);
  BagQry.DeclareVariable("weight",otInteger);
  BagQry.DeclareVariable("value_bag_num",otInteger);
  BagQry.DeclareVariable("pr_liab_limit",otInteger);
  BagQry.DeclareVariable("bag_pool_num",otInteger);
  BagQry.DeclareVariable("hall",otInteger);
  BagQry.DeclareVariable("user_id",otInteger);
  for(map<int, TBagItem>::const_iterator nb=bags.begin();nb!=bags.end();++nb)
  {
    nb->second.toDB(BagQry);
    BagQry.Execute();
  };
    
  if (!is_payment)
  {
    BagQry.Clear();
    BagQry.SQLText="SELECT seg_no FROM tckin_pax_grp WHERE grp_id=:grp_id";
    BagQry.CreateVariable("grp_id",otInteger,grp_id);
    BagQry.Execute();
    int seg_no=1;
    if (!BagQry.Eof)
      seg_no=BagQry.FieldAsInteger("seg_no");

    BagQry.Clear();
    BagQry.SQLText="DELETE FROM bag_tags WHERE grp_id=:grp_id";
    BagQry.CreateVariable("grp_id",otInteger,grp_id);
    BagQry.Execute();
    BagQry.SQLText=
      "INSERT INTO bag_tags(grp_id,num,tag_type,no,color,seg_no,bag_num,pr_print) "
      "VALUES (:grp_id,:num,:tag_type,:no,:color,:seg_no,:bag_num,:pr_print)";
    BagQry.DeclareVariable("num",otInteger);
    BagQry.DeclareVariable("tag_type",otString);
    BagQry.DeclareVariable("no",otFloat);
    BagQry.DeclareVariable("color",otString);
    BagQry.DeclareVariable("seg_no",otInteger);
    BagQry.DeclareVariable("bag_num",otInteger);
    BagQry.DeclareVariable("pr_print",otInteger);
    tags.insert(generated_tags.begin(), generated_tags.end());
    for(map<int, TTagItem>::iterator t=tags.begin();t!=tags.end();++t)
    {
      t->second.seg_no=seg_no;
      t->second.toDB(BagQry);
      try
      {
        BagQry.Execute();
      }
      catch(EOracleError E)
      {
        if (E.Code==1)
          throw UserException("MSG.CHECKIN.BIRK_ALREADY_CHECKED",
                              LParams()<<LParam("tag_type",t->second.tag_type)
                                       <<LParam("color",t->second.color)
                                       <<LParam("no",t->second.no));
        else
          throw;
      };
    };
  }; //!pr_payment
};

void LoadBag(int grp_id, xmlNodePtr bagtagNode)
{
  if (bagtagNode==NULL) return;

  TQuery BagQry(&OraSession);
  
  vector<TValueBagItem> vals;
  vector<TBagItem> bags;
  vector<TTagItem> tags;

  BagQry.Clear();
  BagQry.SQLText="SELECT * FROM value_bag WHERE grp_id=:grp_id";
  BagQry.CreateVariable("grp_id",otInteger,grp_id);
  BagQry.Execute();
  for(;!BagQry.Eof;BagQry.Next())
    vals.push_back(TValueBagItem().fromDB(BagQry));

  BagQry.Clear();
  BagQry.SQLText="SELECT * FROM bag2 WHERE grp_id=:grp_id";
  BagQry.CreateVariable("grp_id",otInteger,grp_id);
  BagQry.Execute();
  for(;!BagQry.Eof;BagQry.Next())
    bags.push_back(TBagItem().fromDB(BagQry));

  BagQry.Clear();
  BagQry.SQLText=
    "SELECT num,tag_type,no_len,no,color,bag_num,printable,pr_print,seg_no "
    "FROM bag_tags,tag_types "
    "WHERE bag_tags.tag_type=tag_types.code AND grp_id=:grp_id";
  BagQry.CreateVariable("grp_id",otInteger,grp_id);
  BagQry.Execute();
  for(;!BagQry.Eof;BagQry.Next())
    tags.push_back(TTagItem().fromDB(BagQry));
  
  sort(vals.begin(), vals.end());
  sort(bags.begin(), bags.end());
  sort(tags.begin(), tags.end());
  
  if (!TReqInfo::Instance()->desk.compatible(VERSION_WITH_BAG_POOLS))
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
      map<int/*bag_pool_num*/, bool/*refused*/> del_pools;
      bool del_pool_exists=false;
      for(vector<TBagItem>::iterator i=bags.begin();i!=bags.end();)
      {
        if (del_pools.find(i->bag_pool_num)==del_pools.end())
        {
          BagQry.SetVariable("bag_pool_num",i->bag_pool_num);
          BagQry.Execute();
          del_pools[i->bag_pool_num]=BagQry.Eof || BagQry.FieldAsInteger("refused")!=0;
        };
        if (del_pools[i->bag_pool_num])
        {
          del_pool_exists=true;
          //㤠�塞 �易���� 業�����
          if (i->value_bag_num!=ASTRA::NoExists)
            for(vector<TValueBagItem>::iterator j=vals.begin();j!=vals.end();)
            {
              if (j->num==i->value_bag_num) j=vals.erase(j); else ++j;
            };
          //㤠�塞 �易��� ��ન
          for(vector<TTagItem>::iterator j=tags.begin();j!=tags.end();)
          {
            if (j->bag_num!=ASTRA::NoExists && j->bag_num==i->num) j=tags.erase(j); else ++j;
          };
          //㤠�塞 �����
          i=bags.erase(i);
        }
        else ++i;
      };
      if (del_pool_exists)
      {
        int num;
        //㯫��塞 �������� 業�����
        num=1;
        for(vector<TValueBagItem>::iterator i=vals.begin();i!=vals.end();++i,num++)
        {
          if (num!=i->num)
          {
            for(vector<TBagItem>::iterator j=bags.begin();j!=bags.end();++j)
              if (j->value_bag_num!=ASTRA::NoExists && j->value_bag_num==i->num) j->value_bag_num=num;
            i->num=num;
          };
        };
        //㯫��塞 �����
        num=1;
        for(vector<TBagItem>::iterator i=bags.begin();i!=bags.end();++i,num++)
        {
          if (num!=i->num)
          {
            for(vector<TTagItem>::iterator j=tags.begin();j!=tags.end();++j)
              if (j->bag_num!=ASTRA::NoExists && j->bag_num==i->num) j->bag_num=num;
            i->num=num;
          };
        };
        //㯫��塞 ��ન
        num=1;
        for(vector<TTagItem>::iterator i=tags.begin();i!=tags.end();++i,num++)
        {
          if (num!=i->num) i->num=num;
        };
      };
    };
  };
  
  xmlNodePtr node;
  node=NewTextChild(bagtagNode,"value_bags");
  for(vector<TValueBagItem>::const_iterator i=vals.begin();i!=vals.end();++i)
    i->toXML(NewTextChild(node,"value_bag"));
  
  node=NewTextChild(bagtagNode,"bags");
  for(vector<TBagItem>::const_iterator i=bags.begin();i!=bags.end();++i)
    i->toXML(NewTextChild(node,"bag"));
    
  node=NewTextChild(bagtagNode,"tags");
  for(vector<TTagItem>::const_iterator i=tags.begin();i!=tags.end();++i)
    i->toXML(NewTextChild(node,"tag"));
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

std::string TNormItem::str() const
{
  //!!!��� ��������樨
  if (empty()) return "";
  ostringstream result;
  result << lowerc(norm_type);
  if (weight!=ASTRA::NoExists)
  {
    if (amount!=ASTRA::NoExists)
      result << " " << amount << "�" << weight;
    else
      result << " " << weight;
    if (per_unit!=ASTRA::NoExists && per_unit!=0)
      result << "��/�";
    else
      result << "��";
  }
  else
  {
    if (amount!=ASTRA::NoExists)
      result << " " << amount << "�";
  };
  return result.str();
};

};


    
    
