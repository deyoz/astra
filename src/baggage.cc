#include "baggage.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "base_tables.h"
#include "term_version.h"
#include "astra_misc.h"
#include "qrys.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

//какая-то фигня с определением кодировки этого файла, поэтому я вставил эту фразу

using namespace std;
using namespace AstraLocale;
using namespace EXCEPTIONS;
using namespace BASIC::date_time;

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
  if (TReqInfo::Instance()->desk.compatible(PAX_SERVICE_VERSION))
  {
    if (pc) pc.get().toXML(node);
    else if (wt) wt.get().toXML(node);
  }
  else
    NewTextChild(node,"bag_type",key_str_compatible());
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
  if (TReqInfo::Instance()->desk.compatible(PIECE_CONCEPT_VERSION))
  {
    NewTextChild(node,"is_trfer",(int)is_trfer);
    NewTextChild(node,"handmade_trfer", (int)(is_trfer && handmade), (int)false);
  }
  else
  {
    if (is_trfer) ReplaceTextChild(node,"bag_type",WeightConcept::OLD_TRFER_BAG_TYPE);
    NewTextChild(node,"is_trfer",(int)(is_trfer && !handmade));
  };
  if (TReqInfo::Instance()->desk.compatible(VERSION_WITH_BAG_POOLS))
    NewTextChild(node,"bag_pool_num",bag_pool_num);
  return *this;
};

TBagItem& TBagItem::fromXML(xmlNodePtr node, bool baggage_pc)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  id=NodeAsIntegerFast("id",node2,ASTRA::NoExists);
  num=NodeAsIntegerFast("num",node2);
  if (id==ASTRA::NoExists)
  {
    //только для вновь введенного багажа
    if (TReqInfo::Instance()->desk.compatible(PAX_SERVICE_VERSION))
    {
      if (GetNodeFast("rfisc",node2)!=NULL)
      {
        pc=TRFISCKey();
        pc.get().fromXML(node);
      }
      else
      {
        wt=TBagTypeKey();
        wt.get().fromXML(node);
      };
    }
    else
    {
      if (baggage_pc)
      {
        pc=TRFISCKey();
        pc.get().fromXMLcompatible(node);
      }
      else
      {
        wt=TBagTypeKey();
        wt.get().fromXMLcompatible(node);
      };
    };

    if (TReqInfo::Instance()->desk.compatible(PIECE_CONCEPT_VERSION))
      is_trfer=NodeAsIntegerFast("is_trfer",node2)!=0;
    else
    {
      is_trfer=false;
      if (wt && wt.get().bag_type==WeightConcept::OLD_TRFER_BAG_TYPE)
      {
        is_trfer=true;
        wt.get().bag_type.clear();
      };
    };
  };
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
  if (!pc) TRFISCKey().toDB(Qry);   //устанавливаем FNull
  if (!wt) TBagTypeKey().toDBcompatible(Qry, "TBagItem::toDB"); //устанавливаем FNull
  if (pc) pc.get().toDB(Qry);
  else if (wt) wt.get().toDBcompatible(Qry, "TBagItem::toDB");
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
  if (!Qry.FieldIsNULL("rfisc"))
  {
    pc=TRFISCKey();
    pc.get().fromDB(Qry);
    pc.get().getListItem();
  }
  else
  {
    wt=TBagTypeKey();
    wt.get().fromDBcompatible(Qry);
    wt.get().getListItem();
  }
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
  handmade=Qry.FieldAsInteger("handmade")!=0;
  return *this;
};

void TBagItem::check(const TRFISCBagPropsList &list) const
{
  if (!pc) return;
  TRFISCBagPropsList::const_iterator i=list.find(pc.get().key());
  if (i==list.end()) return;
  if ((i->second.min_weight!=ASTRA::NoExists &&
       weight<i->second.min_weight*amount) ||
      (i->second.max_weight!=ASTRA::NoExists &&
       weight>i->second.max_weight*amount))
  {
    ostringstream s;
    s << weight << " " << getLocaleText("кг");
    throw UserException("MSG.LUGGAGE.WRONG_WEIGHT_FOR_BAG_TYPE",
                        LParams() << LParam("bag_type", pc.get().key().str())
                                  << LParam("weight", s.str()));
  };
}

void TBagItem::check(TRFISCListWithPropsCache &lists) const
{
  if (!pc) return;
  if (amount!=1)
    throw UserException("MSG.LUGGAGE.EACH_PIECE_SHOULD_WEIGHTED_SEPARATELY");
  if (!pc.get().list_item ||
      !pc.get().list_item.get().carry_on())
    throw UserException("MSG.LUGGAGE.UNKNOWN_BAG_TYPE",
                        LParams() << LParam("bag_type", pc.get().key().str()));

  bool carry_on=pc.get().list_item.get().carry_on().get();
  if (pr_cabin!=carry_on)
  {
    if (carry_on)
      throw UserException("MSG.LUGGAGE.BAG_TYPE_SHOULD_BE_ADDED_TO_CABIN",
                          LParams() << LParam("bag_type", pc.get().key().str()));
    else
      throw UserException("MSG.LUGGAGE.BAG_TYPE_SHOULD_BE_ADDED_TO_HOLD",
                          LParams() << LParam("bag_type", pc.get().key().str()));
  }
  //сюда дошли, значит проверяем bagProps
  check(lists.getBagProps(pc.get().list_id));
}

std::string TBagItem::key_str(const std::string& lang) const
{
  if (pc)
    return pc.get().str(lang);
  else if (wt)
    return wt.get().str(lang);
  else
    return "";
}

std::string TBagItem::key_str_compatible() const
{
  if (pc)
    return pc.get().RFISC;
  else if (wt)
    return wt.get().bag_type;
  else
    return "";
};

void TBagMap::procInboundTrferFromDBTmp()
{
  for(TBagMap::iterator i=begin(); i!=end(); i++)
  {
    TBagItem &bag=i->second;
    if (bag.pc && bag.pc.get().list_id<0)
    {
      int grp_id=-bag.pc.get().list_id;
      bag.pc.get().list_id=ASTRA::NoExists;
      TQuery Qry(&OraSession);
      Qry.Clear();
      Qry.SQLText=
          "SELECT bag_types_id FROM pax_grp WHERE grp_id=:grp_id";
      Qry.CreateVariable("grp_id", otInteger, grp_id);
      Qry.Execute();
      if (Qry.Eof || Qry.FieldIsNULL("bag_types_id")) return;
      TRFISCList list;
      list.fromDB(Qry.FieldAsInteger("bag_types_id"), true);
      bag.pc.get().list_id=list.toDBAdv(false);
    }
    if (bag.wt && bag.wt.get().list_id<0)
    {
      int grp_id=-bag.wt.get().list_id;
      bag.wt.get().list_id=ASTRA::NoExists;
      TBagTypeList list;
      list.createWithCurrSegBagAirline(grp_id); //procInboundTrferFromDBTmp - checked!
      bag.wt.get().list_id=list.toDBAdv();
    }
  };
}

void TBagMap::procInboundTrferFromBTM(const TrferList::TGrpItem &grp)
{
  for(TBagMap::iterator i=begin(); i!=end(); i++)
  {
    TBagItem &bag=i->second;
    if (bag.wt && bag.wt.get().airline.empty())
    {
      bag.wt.get().list_id=ASTRA::NoExists;
      TQuery Qry(&OraSession);
      Qry.Clear();
      Qry.SQLText=
          "SELECT airline FROM tlg_trips WHERE point_id=:point_id";
      Qry.CreateVariable("point_id", otInteger, grp.point_id);
      Qry.Execute();
      if (Qry.Eof || Qry.FieldIsNULL("airline")) return;
      TBagTypeList list;
      list.createForInboundTrferFromBTM(Qry.FieldAsString("airline"));
      bag.wt.get().airline=Qry.FieldAsString("airline");
      bag.wt.get().list_id=list.toDBAdv();
    }
  };
}

void TBagMap::dump(const std::string &where)
{
  for(TBagMap::iterator i=begin(); i!=end(); i++)
  {
    TBagItem &bag=i->second;
    if (bag.pc)
      ProgTrace(TRACE5, "%s: id=%s, %s", where.c_str(), bag.id==ASTRA::NoExists?"NoExists":IntToString(bag.id).c_str(), bag.pc.get().traceStr().c_str());
    if (bag.wt)
      ProgTrace(TRACE5, "%s: id=%s, %s", where.c_str(), bag.id==ASTRA::NoExists?"NoExists":IntToString(bag.id).c_str(), bag.wt.get().traceStr().c_str());
  };
}

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

const TUnaccompInfoItem& TUnaccompInfoItem::toXML(xmlNodePtr node) const
{
  if (node==NULL || isEmpty() ) return *this;
  NewTextChild(node,"num",num);
  if ( !original_tag_no.empty() ) {
    NewTextChild(node,"original_tag_no",original_tag_no);
  }
  if ( !name.empty() ) {
    NewTextChild(node,"name",name);
  }
  if ( !surname.empty() ) {
    NewTextChild(node,"surname",surname);
  }
  if ( !airline.empty() ) {
    NewTextChild(node,"airline",airline);
  }
  if ( flt_no != ASTRA::NoExists ) {
    NewTextChild(node,"flt_no",flt_no);
  }
  if ( !suffix.empty() ) {
    NewTextChild(node,"suffix",suffix);
  }
  if ( scd != ASTRA::NoExists ) {
    NewTextChild(node,"scd",DateTimeToStr( scd, ServerFormatDateTimeAsString ));
  }
  return *this;
}

TUnaccompInfoItem& TUnaccompInfoItem::fromXML(xmlNodePtr node)
{
  clear();
  if (node==NULL) return *this;
  xmlNodePtr node2=node->children;
  num = NodeAsIntegerFast("num",node2,ASTRA::NoExists);
  original_tag_no = NodeAsStringFast("original_tag_no",node2,"");
  name = NodeAsStringFast("name",node2,"");
  surname = NodeAsStringFast("surname",node2,"");
  airline = NodeAsStringFast("airline",node2,"");
  flt_no = NodeAsIntegerFast("flt_no",node2,ASTRA::NoExists);
  suffix = NodeAsStringFast("suffix",node2,"");
  scd = NodeAsDateTimeFast("scd",node2,ASTRA::NoExists);
  return *this;
}

const TUnaccompInfoItem& TUnaccompInfoItem::toDB(TQuery &Qry) const
{
  if ( isEmpty() ) {
    return *this;
  }
  Qry.SetVariable("num",num);
  Qry.SetVariable("original_tag_no",original_tag_no);
  Qry.SetVariable("surname",surname);
  Qry.SetVariable("name",name);
  Qry.SetVariable("airline",airline);
  Qry.SetVariable("flt_no",flt_no==ASTRA::NoExists?FNull:flt_no);
  Qry.SetVariable("suffix",suffix);
  if ( scd==ASTRA::NoExists ) {
    Qry.SetVariable("scd",FNull);
  }
  else {
    Qry.SetVariable("scd",scd);
  }
  return *this;
}

TUnaccompInfoItem& TUnaccompInfoItem::fromDB(TQuery &Qry)
{
  clear();
  num=Qry.FieldAsInteger("num");
  original_tag_no=Qry.FieldAsString("original_tag_no");
  surname=Qry.FieldAsString("surname");
  name=Qry.FieldAsString("name");
  airline=Qry.FieldAsString("airline");
  flt_no=Qry.FieldIsNULL("flt_no")?ASTRA::NoExists:Qry.FieldAsInteger("flt_no");
  suffix=Qry.FieldAsString("suffix");
  scd=Qry.FieldIsNULL("scd")?ASTRA::NoExists:Qry.FieldAsDateTime("scd");
  return *this;
}

const TUnaccompRuleItem& TUnaccompRuleItem::toXML(xmlNodePtr node) const
{
  if (node==NULL ) return *this;
  NewTextChild(node,"fieldname",fieldname);
  NewTextChild(node,"empty",empty);
  if ( max_len != ASTRA::NoExists ) {
    NewTextChild(node,"max_len",max_len);
  }
  if ( min_len != ASTRA::NoExists ) {
    NewTextChild(node,"min_len",min_len);
  }
  return *this;
}

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
          Qry.CreateVariable("now_utc",otDate, NowUTC());
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
    bag.wt=TBagTypeKey();
    bag.num=bag_num;
    bag.pr_cabin=false;
    bag.amount=grp.tags.size();
    bag.weight=bag_weight;
    bags111[bag.num]=bag;
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
    bag.wt=TBagTypeKey();
    bag.num=bag_num;
    bag.pr_cabin=true;
    bag.amount=1;
    bag.weight=rk_weight;
    bags111[bag.num]=bag;
    bag_num++;
  };
};

void TGroupBagItem::setPoolNum(int bag_pool_num)
{
  for(TBagMap::iterator i=bags111.begin();i!=bags111.end();++i)
    i->second.bag_pool_num=bag_pool_num;
};

bool TGroupBagItem::trferExists() const
{
  for(TBagMap::const_iterator i=bags111.begin();i!=bags111.end();++i)
    if (i->second.is_trfer) return true;
  return false;
}

bool TGroupBagItem::fromXML(xmlNodePtr bagtagNode, int grp_id, int hall, bool is_unaccomp, bool baggage_pc)
{
  clear();
  if (bagtagNode==NULL) return false;
  xmlNodePtr node;

  xmlNodePtr valueBagNode=GetNode("value_bags",bagtagNode);
  xmlNodePtr bagNode=GetNode("bags",bagtagNode);
  xmlNodePtr tagNode=GetNode("tags",bagtagNode);
  xmlNodePtr unaccompsNode=GetNode("unaccomps",bagtagNode);

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
    bag.fromXML(node, baggage_pc);
    if (bags111.find(bag.num)!=bags111.end())
      throw Exception("TGroupBagItem::fromXML: bags[%d] duplicated", bag.num);
    if (bag.value_bag_num!=ASTRA::NoExists && vals.find(bag.value_bag_num)==vals.end())
      throw Exception("TGroupBagItem::fromXML: bags[%d].value_bag_num=%d not found", bag.num, bag.value_bag_num);
    bags111[bag.num]=bag;
  };

  for(node=tagNode->children;node!=NULL;node=node->next)
  {
    TTagItem tag;
    tag.fromXML(node);
    if (tags.find(tag.num)!=tags.end())
      throw Exception("TGroupBagItem::fromXML: tags[%d] duplicated", tag.num);
    if (tag.bag_num!=ASTRA::NoExists && bags111.find(tag.bag_num)==bags111.end())
      throw Exception("TGroupBagItem::fromXML: tags[%d].bag_num=%d not found", tag.num, tag.bag_num);
    tags[tag.num]=tag;
  };

  if ( unaccompsNode != NULL ) {
    for(node=unaccompsNode->children;node!=NULL;node=node->next)
    {
      TUnaccompInfoItem unaccomp;
      unaccomp.fromXML(node);
      if (unaccomps.find(unaccomp.num)!=unaccomps.end())
        throw Exception("TGroupBagItem::fromXML: unaccomps[%d] duplicated", unaccomp.num);
      if (bags111.find(unaccomp.num)==bags111.end())
        throw Exception("TGroupBagItem::fromXML: bags[%d] not found", unaccomp.num);
      unaccomps[unaccomp.num]=unaccomp;
    };
  }


  int num=1;
  for(map<int, TValueBagItem>::const_iterator v=vals.begin();v!=vals.end();++v,num++)
  {
    if (v->second.num!=num)
      throw Exception("TGroupBagItem::fromXML: vals[%d] not found", v->second.num);
  };
  num=1;
  for(TBagMap::const_iterator b=bags111.begin();b!=bags111.end();++b,num++)
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

  fromXMLcompletion(grp_id, hall, is_unaccomp);

  return true;
}

void TGroupBagItem::checkAndGenerateTags(int point_id, int grp_id)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  bool is_payment=reqInfo->client_type == ASTRA::ctTerm && reqInfo->screen.name != "AIR.EXE";

  map<int /*num*/, int> bound_tags;
  for(TBagMap::const_iterator b=bags111.begin();b!=bags111.end();++b)
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
        for(TBagMap::const_iterator b=bags111.begin();b!=bags111.end();b++)
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
    for(TBagMap::const_iterator b=bags111.begin();b!=bags111.end();++b)
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
        for(TBagMap::const_iterator b=bags111.begin();b!=bags111.end();)
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
};

void TGroupBagItem::fromXMLcompletion(int grp_id, int hall, bool is_unaccomp)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  bool is_payment=reqInfo->client_type == ASTRA::ctTerm && reqInfo->screen.name != "AIR.EXE";

  TBagMap old_bags;
  if (grp_id!=ASTRA::NoExists) old_bags.fromDB(grp_id);

  bags111.dump("TGroupBagItem::fromXMLcompletion: new_bags:");
  old_bags.dump("TGroupBagItem::fromXMLcompletion: old_bags:");

  if (is_payment && !reqInfo->desk.compatible(VERSION_WITH_BAG_POOLS))
  {
    //для кассы используем old_bags, но с очисткой и переустановлением всех value_bag_num
    //работаем по идентификаторам
    for(TBagMap::iterator ob=old_bags.begin();ob!=old_bags.end();++ob)
    {
      TBagMap::const_iterator nb=bags111.begin();
      for(;nb!=bags111.end();++nb)
        if (nb->second.id==ob->second.id) break;
      if (nb!=bags111.end())
        ob->second.value_bag_num=nb->second.value_bag_num;
      else
        ob->second.value_bag_num=ASTRA::NoExists;
    };

    bags111=old_bags;
  }
  else
  {
    //пробегаемся по сортированным массивам с целью заполнения старых hall и user_id в массиве нового багажа
    for(TBagMap::iterator nb=bags111.begin();nb!=bags111.end();++nb)
    {
      if (nb->second.id==ASTRA::NoExists)
      {
        //вновь введенный багаж
        nb->second.hall=hall;
        nb->second.user_id=reqInfo->user.user_id;
        nb->second.desk=reqInfo->desk.code;
        nb->second.time_create= NowUTC();
      }
      else
      {
        //старый багаж
        TBagMap::const_iterator ob=old_bags.begin();
        for(;ob!=old_bags.end();++ob)
          if (ob->second.id==nb->second.id) break;
        if (ob!=old_bags.end())
        {
          nb->second.pc=ob->second.pc;
          nb->second.wt=ob->second.wt;
          nb->second.is_trfer=ob->second.is_trfer;

          nb->second.hall=ob->second.hall;
          nb->second.user_id=ob->second.user_id;
          nb->second.desk=ob->second.desk;
          nb->second.time_create=ob->second.time_create;
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
          nb->second.handmade=ob->second.handmade;
        }
        else throw Exception("%s: strange situation ob==old_bags.end()! (id=%d)", __FUNCTION__, nb->second.id);
      };
    };
  };

  if (!reqInfo->desk.compatible(PAX_SERVICE_VERSION))
    getAllListKeys(grp_id, is_unaccomp, 0);
};

void TBagMap::toDB(int grp_id) const
{
  TQuery BagQry(&OraSession);
  BagQry.Clear();
  BagQry.SQLText="DELETE FROM bag2 WHERE grp_id=:grp_id";
  BagQry.CreateVariable("grp_id",otInteger,grp_id);
  BagQry.Execute();
  BagQry.SQLText=
    "BEGIN "
    "  IF :id IS NULL THEN "
    "    SELECT cycle_id__seq.nextval INTO :id FROM dual; "
    "  END IF; "
    "  INSERT INTO bag2 (grp_id, num, id, list_id, bag_type, bag_type_str, rfisc, service_type, airline, "
    "    pr_cabin,amount,weight,value_bag_num, "
    "    pr_liab_limit,to_ramp,using_scales,bag_pool_num,hall,user_id,desk,time_create,is_trfer,handmade) "
    "  VALUES (:grp_id, :num, :id, :list_id, :bag_type, :bag_type_str, :rfisc, :service_type, :airline, "
    "    :pr_cabin,:amount,:weight,:value_bag_num, "
    "    :pr_liab_limit,:to_ramp,:using_scales,:bag_pool_num,:hall,:user_id,:desk,:time_create,:is_trfer,:handmade); "
    "END;";
  BagQry.DeclareVariable("num",otInteger);
  BagQry.DeclareVariable("id",otInteger);
  BagQry.DeclareVariable("list_id", otInteger);
  BagQry.DeclareVariable("bag_type", otInteger);
  BagQry.DeclareVariable("bag_type_str", otString);
  BagQry.DeclareVariable("rfisc",otString);
  BagQry.DeclareVariable("service_type", otString);
  BagQry.DeclareVariable("airline", otString);
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
  for(TBagMap::const_iterator nb=begin();nb!=end();++nb)
  {
    nb->second.toDB(BagQry);
    BagQry.Execute();
  };
}

void TGroupBagItem::bagToDB(int grp_id) const
{
  TQuery BagQry(&OraSession);
  BagQry.Clear();
  BagQry.SQLText="DELETE FROM unaccomp_bag_info WHERE grp_id=:grp_id";
  BagQry.CreateVariable("grp_id",otInteger,grp_id);
  BagQry.Execute();

  bags111.toDB(grp_id);

  BagQry.Clear();
  BagQry.SQLText=
    "INSERT INTO unaccomp_bag_info(grp_id,num,original_tag_no,surname,name,airline,flt_no,suffix,scd) "
    "VALUES(:grp_id,:num,:original_tag_no,:surname,:name,:airline,:flt_no,:suffix,:scd) ";
  BagQry.CreateVariable("grp_id",otInteger,grp_id);
  BagQry.DeclareVariable("num",otInteger);
  BagQry.DeclareVariable("original_tag_no",otString);
  BagQry.DeclareVariable("surname",otString);
  BagQry.DeclareVariable("name",otString);
  BagQry.DeclareVariable("airline",otString);
  BagQry.DeclareVariable("flt_no",otInteger);
  BagQry.DeclareVariable("suffix",otString);
  BagQry.DeclareVariable("scd",otDate);
  for(map<int, TUnaccompInfoItem>::const_iterator ub=unaccomps.begin();ub!=unaccomps.end();++ub)
  {
    ub->second.toDB(BagQry);
    BagQry.Execute();
  };

}

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

  bagToDB(grp_id);

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
  for(TBagMap::const_iterator i=bags111.begin();i!=bags111.end();++i)
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
    if (!bags111.insert(make_pair(i->num, *i)).second)
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
  normalizeLists(vals.size()+1, bags111.size()+1, tags.size()+1, vals_tmp, bags_tmp, tags_tmp);
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

void TBagMap::fromDB(int grp_id)
{
  clear();

  TQuery BagQry(&OraSession);
  BagQry.Clear();
  BagQry.SQLText="SELECT * FROM bag2 WHERE grp_id=:grp_id AND list_id IS NOT NULL"; //!!!list_id IS NOT NULL потом удалить
  BagQry.CreateVariable("grp_id",otInteger,grp_id);
  BagQry.Execute();
  for(;!BagQry.Eof;BagQry.Next())
  {
    TBagItem bag;
    bag.fromDB(BagQry);
    insert(make_pair(bag.num, bag));
  };

  if (empty()) //!!!потом удалить
  {
    string airline_wt=WeightConcept::GetCurrSegBagAirline(grp_id); //TBagMap::fromDB - checked!

    TCachedQuery Qry("SELECT bag2.grp_id, bag2.num, bag2.bag_type, bag2.pr_cabin, bag2.amount, bag2.weight, "
                     "       bag2.value_bag_num, bag2.pr_liab_limit, bag2.bag_pool_num, bag2.id, bag2.hall, "
                     "       bag2.user_id, bag2.to_ramp, bag2.using_scales, bag2.is_trfer, bag2.handmade, "
                     "       bag2.rfisc, bag2.desk, bag2.time_create, "
                     "       -bag2.grp_id AS list_id, "
                     "       bag2.bag_type_str, "
                     "       :airline_wt AS airline, "
                     "       NULL AS service_type "
                     "FROM bag2 "
                     "WHERE bag2.grp_id=:grp_id AND bag2.rfisc IS NULL "
                     "UNION "
                     "SELECT bag2.grp_id, bag2.num, bag2.bag_type, bag2.pr_cabin, bag2.amount, bag2.weight, "
                     "       bag2.value_bag_num, bag2.pr_liab_limit, bag2.bag_pool_num, bag2.id, bag2.hall, "
                     "       bag2.user_id, bag2.to_ramp, bag2.using_scales, bag2.is_trfer, bag2.handmade, "
                     "       bag2.rfisc, bag2.desk, bag2.time_create, "
                     "       DECODE(rfisc_list_items.airline, NULL, TO_NUMBER(NULL), -bag2.grp_id) AS list_id, "
                     "       bag2.bag_type_str, "
                     "       rfisc_list_items.airline, "
                     "       rfisc_list_items.service_type "
                     "FROM bag2, "
                     "     (SELECT bag_types_lists.airline, grp_rfisc_lists.service_type, grp_rfisc_lists.rfisc "
                     "      FROM pax_grp, bag_types_lists, grp_rfisc_lists "
                     "      WHERE pax_grp.bag_types_id=bag_types_lists.id AND "
                     "            bag_types_lists.id=grp_rfisc_lists.list_id AND "
                     "            pax_grp.grp_id=:grp_id) rfisc_list_items "
                     "WHERE bag2.grp_id=:grp_id AND bag2.rfisc IS NOT NULL AND "
                     "      bag2.rfisc=rfisc_list_items.rfisc(+) ",
                     QParams() << QParam("grp_id", otInteger, grp_id)
                               << QParam("airline_wt", otString, airline_wt));
    Qry.get().Execute();
    for(; !Qry.get().Eof; Qry.get().Next())
    {
      if (!Qry.get().FieldIsNULL("rfisc") && Qry.get().FieldAsInteger("is_trfer")==0 &&
          (Qry.get().FieldIsNULL("airline") || Qry.get().FieldIsNULL("service_type")))
        throw Exception("TGroupBagItem::fromDB: wrong data (grp_id=%d, rfisc=%s)",
                        grp_id, Qry.get().FieldAsString("rfisc"));
      if (Qry.get().FieldIsNULL("rfisc") && Qry.get().FieldIsNULL("airline"))
        throw Exception("TGroupBagItem::fromDB: wrong data (grp_id=%d, bag_type=%s)",
                        grp_id, Qry.get().FieldAsString("bag_type_str"));
      TBagItem bag;
      bag.fromDB(Qry.get());
      if (bag.pc && bag.is_trfer && bag.pc.get().airline.empty())
        bag.pc.get().getPseudoListIdForInboundTrfer(grp_id);
      insert(make_pair(bag.num, bag));
    };
  };
}

void TGroupBagItem::bagFromDB(int grp_id)
{
  bags111.fromDB(grp_id);

  TQuery BagQry(&OraSession);
  BagQry.Clear();
  BagQry.SQLText=
    "SELECT * FROM unaccomp_bag_info "
    "WHERE grp_id=:grp_id";
  BagQry.CreateVariable("grp_id",otInteger,grp_id);
  BagQry.Execute();
  for(;!BagQry.Eof;BagQry.Next())
  {
    TUnaccompInfoItem unaccomp;
    unaccomp.fromDB(BagQry);
    unaccomps[unaccomp.num]=unaccomp;
  };
}

void TGroupBagItem::fromDB(int grp_id, int bag_pool_num, bool without_refused)
{
  vals.clear();
  bags111.clear();
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

  bagFromDB(grp_id);

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

  //!!!djek
  fillUnaccompRules();

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
      for(TBagMap::const_iterator i=bags111.begin();i!=bags111.end();++i)
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
  for(TBagMap::const_iterator i=bags111.begin();i!=bags111.end();++i)
    i->second.toXML(NewTextChild(node,"bag"));

  node=NewTextChild(bagtagNode,"tags");
  for(map<int /*num*/, TTagItem>::const_iterator i=tags.begin();i!=tags.end();++i)
    i->second.toXML(NewTextChild(node,"tag"));
  node=NewTextChild(bagtagNode,"unaccomps_rules");
  for(set<TUnaccompRuleItem>::const_iterator i=unaccomp_rules.begin();i!=unaccomp_rules.end();++i)
    i->toXML(NewTextChild(node,"rule"));
  node=NewTextChild(bagtagNode,"unaccomps_data");
  for(map<int /*num*/, TUnaccompInfoItem>::const_iterator i=unaccomps.begin();i!=unaccomps.end();++i)
    i->second.toXML(NewTextChild(node,"unnacomp"));
};

void TGroupBagItem::convertBag(std::multimap<int /*id*/, TBagItem> &result) const
{
  //multimap - потому что м.б. id=NoExists если добавляются сразу несколько мест багажа
  result.clear();
  for(TBagMap::const_iterator b=bags111.begin(); b!=bags111.end(); ++b)
    result.insert(make_pair(b->second.id, b->second));
}

void TGroupBagItem::fillUnaccompRules()
{
  unaccomp_rules.clear();
  unaccomp_rules.insert( TUnaccompRuleItem("original_tag_no",false,10,15) );
  unaccomp_rules.insert( TUnaccompRuleItem("surname",false,ASTRA::NoExists,64) );
  unaccomp_rules.insert( TUnaccompRuleItem("name",true,ASTRA::NoExists,64) );
  unaccomp_rules.insert( TUnaccompRuleItem("flight",true,ASTRA::NoExists,9) );
  unaccomp_rules.insert( TUnaccompRuleItem("scd",true,ASTRA::NoExists,5) );
}

void TGroupBagItem::getAllListKeys(const int grp_id, const bool is_unaccomp, const int transfer_num)
{
  for(TBagMap::iterator b=bags111.begin(); b!=bags111.end(); ++b)
  {
    TBagItem &bag=b->second;
    if (grp_id==ASTRA::NoExists) throw UserException("MSG.BAGGAGE_INPUT_ONLY_AFTER_CHECKIN");
    TServiceCategory::Enum category=bag.pr_cabin?TServiceCategory::CarryOn:TServiceCategory::Baggage;
    try
    {
      if (bag.pc)
      {
        if (is_unaccomp)
          bag.pc.get().getListKeyUnaccomp(grp_id, transfer_num, category, "TGroupBagItem");
        else
          bag.pc.get().getListKeyByGrpId(grp_id, transfer_num, category, "TGroupBagItem");
          //bag.pc.get().getListKeyByBagPool(grp_id, transfer_num, bag.bag_pool_num, category); //это более правильно, но не работает пока не проапдейтится pax.bag_pool_num
      }
      else if (bag.wt)
      {
        if (is_unaccomp)
          bag.wt.get().getListKeyUnaccomp(grp_id, transfer_num, category, "TGroupBagItem");
        else
          bag.wt.get().getListKeyByGrpId(grp_id, transfer_num, category, "TGroupBagItem");
          //bag.wt.get().getListKeyByBagPool(grp_id, transfer_num, bag.bag_pool_num, category); //это более правильно, но не работает пока не проапдейтится pax.bag_pool_num
      };
    }
    catch(EConvertError &e)
    {
      throw;  //входящий трансфер не обрабатываем, поскольку он не может вводиться через терминал
    };
  }
}

void TGroupBagItem::getAllListItems(const int grp_id, const bool is_unaccomp, const int transfer_num)
{
  for(TBagMap::iterator b=bags111.begin(); b!=bags111.end(); ++b)
  {
    TBagItem &bag=b->second;
    if (grp_id==ASTRA::NoExists) throw UserException("MSG.BAGGAGE_INPUT_ONLY_AFTER_CHECKIN");
    TServiceCategory::Enum category=bag.pr_cabin?TServiceCategory::CarryOn:TServiceCategory::Baggage;
    try
    {
      if (bag.pc)
      {
        if (is_unaccomp)
          bag.pc.get().getListItemUnaccomp(grp_id, transfer_num, category, "TGroupBagItem");
        else
        {
          if (TReqInfo::Instance()->desk.compatible(VERSION_WITH_BAG_POOLS))
            bag.pc.get().getListItemByBagPool(grp_id, transfer_num, bag.bag_pool_num, category, "TGroupBagItem");
          else
            bag.pc.get().getListItemByGrpId(grp_id, transfer_num, category, "TGroupBagItem");
        };
      }
      else if (bag.wt)
      {
        if (is_unaccomp)
          bag.wt.get().getListItemUnaccomp(grp_id, transfer_num, category, "TGroupBagItem");
        else
        {
          if (TReqInfo::Instance()->desk.compatible(VERSION_WITH_BAG_POOLS))
            bag.wt.get().getListItemByBagPool(grp_id, transfer_num, bag.bag_pool_num, category, "TGroupBagItem");
          else
            bag.wt.get().getListItemByGrpId(grp_id, transfer_num, category, "TGroupBagItem");
        };
      };
    }
    catch(EConvertError &e)
    {
      if (!bag.is_trfer) throw;
      if (bag.pc)
      {
        bag.pc.get().getListItemInboundTrferTmp("TGroupBagItem");
      }
      else if (bag.wt)
      {
        bag.wt.get().getListItemInboundTrferTmp("TGroupBagItem");
      };
    };
  }
}

} //namespace CheckIn

void GridInfoToXML(const TTrferRoute &trfer,
                   const TPaidRFISCViewMap &paidRFISC,
                   const TPaidBagViewMap &paidBag,
                   const TPaidBagViewMap &trferBag,
                   xmlNodePtr node)
{
  class TGridCol
  {
    public:
      std::string name1, name2;
      int width;
      TAlignment::Enum align;
    TGridCol(const std::string& _name,
             const int _width,
             const TAlignment::Enum _align) : name1(_name), width(_width), align(_align) {}
    TGridCol(const std::string& _name1,
             const std::string& _name2,
             const int _width,
             const TAlignment::Enum _align) : name1(_name1), name2(_name2), width(_width), align(_align) {}
  };

  typedef std::list<TGridCol> TGridColList;

  bool pc=!paidRFISC.empty();
  bool wt=!paidBag.empty() || !trferBag.empty();
  TGridColList cols;
  if (wt)
  {
    if (pc)
    {
      cols.push_back(TGridCol("RFISC","Тип багажа", 140, TAlignment::LeftJustify));
      cols.push_back(TGridCol("Кол.",                30, TAlignment::Center));
      cols.push_back(TGridCol("Норма","Сегмент",     80, TAlignment::LeftJustify));
      cols.push_back(TGridCol("К опл.",              40, TAlignment::RightJustify));
      cols.push_back(TGridCol("Оплачено",            40, TAlignment::RightJustify));
    }
    else
    {
      cols.push_back(TGridCol("Тип багажа", 120, TAlignment::LeftJustify));
      cols.push_back(TGridCol("Кол.",        30, TAlignment::Center));
      cols.push_back(TGridCol("Норма",       80, TAlignment::LeftJustify));
      cols.push_back(TGridCol("Трфр",        25, TAlignment::Center));
      cols.push_back(TGridCol("К опл.",      35, TAlignment::RightJustify));
      cols.push_back(TGridCol("Оплачено",    40, TAlignment::RightJustify));
    }
  }
  else
  {
    cols.push_back(TGridCol("RFISC"   , 140, TAlignment::LeftJustify));
    cols.push_back(TGridCol("Кол."    ,  30, TAlignment::Center));
    cols.push_back(TGridCol("Сегмент" ,  80, TAlignment::LeftJustify));
    cols.push_back(TGridCol("К опл."  ,  40, TAlignment::RightJustify));
    cols.push_back(TGridCol("Оплачено",  40, TAlignment::RightJustify));
  }



  xmlNodePtr paidViewNode=NewTextChild(node, "paid_bag_view");
  SetProp(paidViewNode, "font_size", 8);

  //секция описывающая столбцы
  xmlNodePtr colsNode=NewTextChild(paidViewNode, "cols");
  for(TGridColList::const_iterator i=cols.begin(); i!=cols.end(); ++i)
  {
    xmlNodePtr colNode=NewTextChild(colsNode, "col");
    SetProp(colNode, "width", i->width);
    SetProp(colNode, "align", Alignments().encode(i->align));
  }

  //секция описывающая заголовок
  xmlNodePtr headerNode=NewTextChild(paidViewNode, "header");
  SetProp(headerNode, "font_size", 10);
  SetProp(headerNode, "font_style", "");
  SetProp(headerNode, "align", Alignments().encode(TAlignment::LeftJustify));
  for(TGridColList::const_iterator i=cols.begin(); i!=cols.end(); ++i)
  {
    ostringstream s;
    s << getLocaleText(i->name1);
    if (!i->name2.empty())
      s << "/" << getLocaleText(i->name2);
    NewTextChild(headerNode, "col", s.str());
  };

  xmlNodePtr rowsNode = NewTextChild(paidViewNode, "rows");

  for(TPaidRFISCViewMap::const_iterator i=paidRFISC.begin(); i!=paidRFISC.end(); ++i)
    i->second.GridInfoToXML(rowsNode, trfer);
  for(TPaidBagViewMap::const_iterator i=paidBag.begin(); i!=paidBag.end(); ++i)
    i->second.GridInfoToXML(rowsNode, !pc && wt);
  for(TPaidBagViewMap::const_iterator i=trferBag.begin(); i!=trferBag.end(); ++i)
    i->second.GridInfoToXML(rowsNode, !pc && wt);
}

TExcessNodeList::TExcessNodeList(): concept(ctInitial) {
    must_work = true;
    if (TReqInfo::Instance()->client_type != ASTRA::ctTerm || ! TReqInfo::Instance()->desk.compatible(PIECE_CONCEPT_VERSION))
        must_work = false;
}

void TExcessNodeList::apply()
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

void TExcessNodeList::set_concept(xmlNodePtr& node, bool val)
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

