#include "events.h"
#include "basic.h"
#include "exceptions.h"
#include "oralib.h"
#include "stl_utils.h"
#include "xml_unit.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "astra_consts.h"
#include "docs.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

using namespace std;
using namespace BASIC;
using namespace AstraLocale;
using namespace EXCEPTIONS;
using namespace ASTRA;

void EventsInterface::GetEvents(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
    TReqInfo *reqInfo = TReqInfo::Instance();
    TQuery Qry(&OraSession);
    int point_id = NodeAsInteger("point_id",reqNode);
    TDateTime part_key = NoExists;
    if (GetNode( "part_key", reqNode )!=NULL)
      part_key = NodeAsDateTime( "part_key", reqNode );
    xmlNodePtr etNode = GetNode( "EventsTypes", reqNode );
    vector<string> eventTypes;
    bool disp_event=false;
    if (etNode!=NULL)
    {
      for(etNode=etNode->children; etNode!=NULL; etNode=etNode->next)
      {
        TEventType eventType=DecodeEventType(NodeAsString(etNode->children));
        switch(eventType)
        {
          case evtDisp: disp_event=true;
                        break;
       case evtUnknown: break;
               default: eventTypes.push_back(NodeAsString(etNode->children));
                        break;
        };
      };
    };
    
    int move_id = NoExists;
    if ( disp_event )
    {
      Qry.Clear();
    	if ( part_key != NoExists ) {
    		Qry.SQLText=
          "SELECT move_id FROM arx_points "
          "WHERE part_key=:part_key AND point_id=:point_id AND pr_del>=0";
    	  Qry.CreateVariable( "part_key", otDate, part_key );
    	}
  	  else {
        Qry.SQLText=
          "SELECT move_id FROM points "
          "WHERE point_id=:point_id AND pr_del>=0";
  	  }
      Qry.CreateVariable( "point_id", otInteger, point_id );
      Qry.Execute();
      if ( !Qry.Eof ) move_id = Qry.FieldAsInteger( "move_id" );
    };
    
    xmlNodePtr logNode = NewTextChild(resNode, "events_log");
    
    if (move_id != NoExists || !eventTypes.empty())
    {
      Qry.Clear();
      ostringstream sql;
      if (move_id != NoExists)
      {
        sql << "SELECT type, msg, time, id2 AS point_id, \n"
               "       DECODE(type,:evtPax,id2,:evtPay,id2,-1) AS reg_no, \n"
               "       DECODE(type,:evtPax,id3,:evtPay,id3,-1) AS grp_id, \n"
               "       ev_user, station, ev_order \n";
        if (part_key != NoExists)
          sql << "FROM arx_events \n"
                 "WHERE part_key=:part_key AND \n";
        else
          sql << "FROM events \n"
                 "WHERE \n";
        sql << " type=:evtDisp AND id1=:move_id \n";
        Qry.CreateVariable("evtDisp",otString,EncodeEventType(ASTRA::evtDisp));
        Qry.CreateVariable("move_id",otInteger,move_id);
      };
      
      if (move_id != NoExists && !eventTypes.empty()) sql << "UNION \n";
      
      if (!eventTypes.empty())
      {
        sql << "SELECT type, msg, time, id1 AS point_id, \n"
               "       DECODE(type,:evtPax,id2,:evtPay,id2,-1) AS reg_no, \n"
               "       DECODE(type,:evtPax,id3,:evtPay,id3,-1) AS grp_id, \n"
               "       ev_user, station, ev_order \n";
        if (part_key != NoExists)
          sql << "FROM arx_events \n"
                 "WHERE part_key=:part_key AND \n";
        else
          sql << "FROM events \n"
                 "WHERE \n";
        sql << " type IN " << GetSQLEnum(eventTypes) << " AND id1=:point_id \n";
        Qry.CreateVariable("point_id",otInteger,point_id);
      };
      Qry.CreateVariable("evtPax",otString,EncodeEventType(ASTRA::evtPax));
      Qry.CreateVariable("evtPay",otString,EncodeEventType(ASTRA::evtPay));
      if (part_key != NoExists)
        Qry.CreateVariable( "part_key", otDate, part_key );

      //ProgTrace(TRACE5, "GetEvents: SQL=\n%s", sql.str().c_str());
      Qry.SQLText=sql.str().c_str();
      Qry.Execute();

      for(;!Qry.Eof;Qry.Next())
      {
        xmlNodePtr rowNode=NewTextChild(logNode,"row");
        NewTextChild(rowNode,"point_id",Qry.FieldAsInteger("point_id"));
        NewTextChild(rowNode,"ev_user",Qry.FieldAsString("ev_user"));
        NewTextChild(rowNode,"station",Qry.FieldAsString("station"));

        TDateTime time = UTCToClient(Qry.FieldAsDateTime("time"),reqInfo->desk.tz_region);

        NewTextChild(rowNode,"time",DateTimeToStr(time));
        NewTextChild(rowNode,"fmt_time",DateTimeToStr(time, "dd.mm.yy hh:nn"));
        NewTextChild(rowNode,"grp_id",Qry.FieldAsInteger("grp_id"),-1);
        NewTextChild(rowNode,"reg_no",Qry.FieldAsInteger("reg_no"),-1);
        NewTextChild(rowNode,"msg",Qry.FieldAsString("msg"));
        NewTextChild(rowNode,"ev_order",Qry.FieldAsInteger("ev_order"));
      };
    };
    logNode = NewTextChild(resNode, "form_data");
    logNode = NewTextChild(logNode, "variables");
    if ( GetNode( "seasonvars", reqNode ) ) {
        SeasonListVars( point_id, 0, logNode, reqNode );
        NewTextChild(logNode, "caption", getLocaleText("CAP.DOC.SEASON_EVENTS_LOG",
                    LParams() << LParam("trip", NodeAsString("trip", logNode))));
    } else {
        TRptParams rpt_params(TReqInfo::Instance()->desk.lang);
        PaxListVars(point_id, rpt_params, logNode, part_key);
        NewTextChild(logNode, "caption", getLocaleText("CAP.DOC.EVENTS_LOG",
                    LParams() << LParam("flight", get_flight(logNode)) //!!!den param 100%error
                    << LParam("day_issue", NodeAsString("day_issue", logNode)
                        )));
    }
    NewTextChild(logNode, "cap_test", getLocaleText("CAP.TEST", TReqInfo::Instance()->desk.lang));
    get_new_report_form("EventsLog", reqNode, resNode);
    NewTextChild(logNode, "short_page_number_fmt", getLocaleText("CAP.SHORT_PAGE_NUMBER_FMT"));
};

void GetBagToLogInfo(int grp_id, TBagToLogGrpInfo &grpInfo)
{
  grpInfo.clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT "
    "       pax.pax_id, pax.reg_no, "
    "       pax.surname, pax.name, pax.pers_type, pax.refuse, "
    "       NVL(ckin.get_bagAmount2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) AS bag_amount, "
    "       NVL(ckin.get_bagWeight2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) AS bag_weight, "
    "       NVL(ckin.get_rkAmount2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) AS rk_amount, "
    "       NVL(ckin.get_rkWeight2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,rownum),0) AS rk_weight, "
    "       ckin.get_birks2(pax_grp.grp_id,pax.pax_id,pax.bag_pool_num,:lang) AS tags, "
    "       DECODE(pax_grp.bag_refuse,0,pax_grp.excess,0) AS excess "
    "FROM pax_grp, pax "
    "WHERE pax_grp.grp_id=pax.grp_id(+) AND pax_grp.grp_id=:grp_id";
  Qry.CreateVariable("grp_id",otInteger,grp_id);
  Qry.CreateVariable("lang",otString,AstraLocale::LANG_RU); //пока в лог пишем всегда на русском
  Qry.Execute();
  if (!Qry.Eof)
  {
    grpInfo.grp_id=grp_id;
    grpInfo.excess=Qry.FieldAsInteger("excess");
    for(;!Qry.Eof;Qry.Next())
    {
      TBagToLogPaxInfoKey paxInfoKey;
      paxInfoKey.pax_id=Qry.FieldIsNULL("pax_id")?NoExists:Qry.FieldAsInteger("pax_id");
      paxInfoKey.reg_no=Qry.FieldIsNULL("reg_no")?NoExists:Qry.FieldAsInteger("reg_no");
      TBagToLogPaxInfo paxInfo;
      paxInfo.surname=Qry.FieldAsString("surname");
      paxInfo.name=Qry.FieldAsString("name");
      paxInfo.pers_type=Qry.FieldAsString("pers_type");
      paxInfo.refuse=Qry.FieldAsString("refuse");
      paxInfo.bag_amount=Qry.FieldAsInteger("bag_amount");
      paxInfo.bag_weight=Qry.FieldAsInteger("bag_weight");
      paxInfo.rk_amount=Qry.FieldAsInteger("rk_amount");
      paxInfo.rk_weight=Qry.FieldAsInteger("rk_weight");
      paxInfo.tags=Qry.FieldAsString("tags");
      grpInfo.pax[paxInfoKey]=paxInfo;
    };

    Qry.Clear();
    Qry.SQLText=
      "SELECT bag_type, "
      "       SUM(amount) AS amount, "
      "       SUM(weight) AS weight "
      "FROM pax_grp,bag2 "
      "WHERE pax_grp.grp_id=bag2.grp_id AND "
      "      pax_grp.grp_id=:grp_id AND "
      "      ckin.bag_pool_refused(bag2.grp_id,bag2.bag_pool_num,pax_grp.class,pax_grp.bag_refuse)=0 "
      "GROUP BY bag_type";
    Qry.CreateVariable("grp_id",otInteger,grp_id);
    Qry.Execute();
    if (!Qry.Eof)
    {
      //багаж есть
      for(;!Qry.Eof;Qry.Next())
      {
        int bag_type=Qry.FieldIsNULL("bag_type")?-1:Qry.FieldAsInteger("bag_type");
        TBagToLogPaidInfo &paidInfo=grpInfo.paid[bag_type];
        paidInfo.bag_type=bag_type;
        paidInfo.bag_amount=Qry.FieldAsInteger("amount");
        paidInfo.bag_weight=Qry.FieldAsInteger("weight");
      };

      Qry.Clear();
      Qry.SQLText=
        "SELECT bag_type, weight FROM paid_bag WHERE grp_id=:grp_id";
      Qry.CreateVariable("grp_id",otInteger,grp_id);
      Qry.Execute();
      for(;!Qry.Eof;Qry.Next())
      {
        int bag_type=Qry.FieldIsNULL("bag_type")?-1:Qry.FieldAsInteger("bag_type");
        TBagToLogPaidInfo &paidInfo=grpInfo.paid[bag_type];
        paidInfo.bag_type=bag_type;
        paidInfo.paid_weight=Qry.FieldAsInteger("weight");
      };
    };
  };
};

void SaveBagToLog(int point_id, const TBagToLogGrpInfo &grpInfoBefore,
                                const TBagToLogGrpInfo &grpInfoAfter)
{
  if (grpInfoBefore==grpInfoAfter) return;
  
  int grp_id=grpInfoAfter.grp_id==NoExists?grpInfoBefore.grp_id:grpInfoAfter.grp_id;

  TReqInfo* reqInfo = TReqInfo::Instance();
  map< TBagToLogPaxInfoKey, TBagToLogPaxInfo>::const_iterator a=grpInfoAfter.pax.begin();
  map< TBagToLogPaxInfoKey, TBagToLogPaxInfo>::const_iterator b=grpInfoBefore.pax.begin();
  for(;a!=grpInfoAfter.pax.end() || b!=grpInfoBefore.pax.end();)
  {
    string bagStrAfter, bagStrBefore, paxStr;
    int reg_no=NoExists;
    if (a==grpInfoAfter.pax.end() ||
        a!=grpInfoAfter.pax.end() && b!=grpInfoBefore.pax.end() && b->first < a->first)
    {
      //пишем b
      bagStrBefore=b->second.getBagStr();
      paxStr=b->second.getPaxStr();
      reg_no=NoExists;//b->first.reg_no;
      b++;
    } else
    if (b==grpInfoBefore.pax.end() ||
        a!=grpInfoAfter.pax.end() && b!=grpInfoBefore.pax.end() && a->first < b->first)
    {
      //пишем a
      bagStrAfter=a->second.getBagStr();
      paxStr=a->second.getPaxStr();
      reg_no=a->first.reg_no;
      a++;
    } else
    if (a!=grpInfoAfter.pax.end() && b!=grpInfoBefore.pax.end() && a->first==b->first)
    {
      //пассажиры совпадают
      bagStrAfter=a->second.getBagStr();
      bagStrBefore=b->second.getBagStr();
      paxStr=a->second.getPaxStr();
      reg_no=a->first.reg_no;
      a++;
      b++;
    };

    if (bagStrAfter==bagStrBefore) continue;

    ostringstream msg;
    if (bagStrAfter.empty())
      msg << paxStr << ". Удалено: " << bagStrBefore;
    if (bagStrBefore.empty())
      msg << paxStr << ". Добавлено: " << bagStrAfter;
    if (!bagStrAfter.empty() && !bagStrBefore.empty())
      msg << paxStr << ". Изменено: " << bagStrAfter;

    reqInfo->MsgToLog(msg.str(), ASTRA::evtPax,
                      point_id, reg_no==NoExists?0:reg_no, grp_id);
  };
  

  if (grpInfoBefore.paid==grpInfoAfter.paid &&
      grpInfoBefore.excess==grpInfoAfter.excess) return;
  map< int/*bag_type*/, TBagToLogPaidInfo>::const_iterator p=grpInfoAfter.paid.begin();
  ostringstream msg;
  msg << "Опл. вес: " << grpInfoAfter.excess << " кг. "
      << "Багаж по типам (мест/вес/опл): ";
  if (!grpInfoAfter.paid.empty())
  {
    for(;p!=grpInfoAfter.paid.end();p++)
    {
      if (p!=grpInfoAfter.paid.begin()) msg << ", ";
      if (p->second.bag_type!=-1) msg << setw(2) << setfill('0') << p->second.bag_type << ":";
      msg << p->second.bag_amount << "/" << p->second.bag_weight << "/" << p->second.paid_weight;
    };
  }
  else msg << "нет";

  reqInfo->MsgToLog(msg.str(), ASTRA::evtPax, point_id, 0, grp_id);
};
/*
//запись багажа в лог
void CheckInInterface::SaveBagToLog(int point_id, int grp_id, xmlNodePtr bagtagNode)
{
  if (bagtagNode==NULL) return;

  xmlNodePtr paidBagNode=GetNode("paid_bags",bagtagNode);
  xmlNodePtr bagNode=GetNode("bags",bagtagNode);
  xmlNodePtr tagNode=GetNode("tags",bagtagNode);
  TReqInfo* reqInfo = TReqInfo::Instance();
  TLogMsg msg;
  msg.ev_type=ASTRA::evtPax;
  msg.id1=point_id;
  msg.id2=0;
  msg.id3=grp_id;
  TQuery Qry(&OraSession);
  if (bagNode!=NULL || tagNode!=NULL)
  {
    //строка по общему кол-ву багажа
    Qry.Clear();
    Qry.SQLText=
      "SELECT "
      "       NVL(ckin.get_bagAmount2(grp_id,NULL,NULL),0) AS bagAmount, "
      "       NVL(ckin.get_bagWeight2(grp_id,NULL,NULL),0) AS bagWeight, "
      "       NVL(ckin.get_rkAmount2(grp_id,NULL,NULL),0) AS rkAmount, "
      "       NVL(ckin.get_rkWeight2(grp_id,NULL,NULL),0) AS rkWeight, "
      "       ckin.get_birks2(grp_id,NULL,NULL,:lang) AS tags, "
      "       excess "
      "FROM pax_grp where grp_id=:grp_id";
    Qry.CreateVariable("grp_id",otInteger,grp_id);
    Qry.CreateVariable("lang",otString,AstraLocale::LANG_RU); //пока в лог пишем всегда на русском
    Qry.Execute();
    if (!Qry.Eof)
    {
      ostringstream msgh;
      msgh << "Багаж: " << Qry.FieldAsInteger("bagAmount") << "/" << Qry.FieldAsInteger("bagWeight") << ", "
           << "р/кладь: " << Qry.FieldAsInteger("rkAmount") << "/" << Qry.FieldAsInteger("rkWeight") << ". ";
      if (Qry.FieldAsInteger("excess")!=0)
        msgh << "Опл. вес: " << Qry.FieldAsInteger("excess") << " кг. ";
      if (!Qry.FieldIsNULL("tags"))
        msgh << "Бирки: " << Qry.FieldAsString("tags") << ". ";
      msg.msg=msgh.str();
      reqInfo->MsgToLog(msg);
    };
  };
  if (bagNode!=NULL || paidBagNode!=NULL)
  {
    //строка по типам багажа и оплачиваемому багажу
    Qry.Clear();
    Qry.SQLText=
      "SELECT LPAD(paid_bag.bag_type,2,'0' ) AS bag_type, "
      "       MAX(paid_bag.weight) AS paid_weight, "
      "       NVL(SUM(bag2.amount),0) AS bag_amount, "
      "       NVL(SUM(bag2.weight),0) AS bag_weight "
      "FROM paid_bag,bag2 "
      "WHERE paid_bag.grp_id=bag2.grp_id(+) AND  "
      "      NVL(paid_bag.bag_type,-1)=NVL( bag2.bag_type(+),-1) AND  "
      "      paid_bag.grp_id=:grp_id "
      "GROUP BY paid_bag.bag_type "
      "ORDER BY DECODE(paid_bag.bag_type,NULL,0,1),paid_bag.bag_type ";
    Qry.CreateVariable("grp_id",otInteger,grp_id);
    Qry.Execute();
    ostringstream msgh1,msgh2;
    for(;!Qry.Eof;Qry.Next())
    {
      if (Qry.FieldAsInteger("bag_amount")==0 &&
          Qry.FieldAsInteger("bag_weight")==0 &&
          Qry.FieldAsInteger("paid_weight")==0) continue;
      if (Qry.FieldIsNULL("bag_type"))
      {
        msgh1 << ", "
              << Qry.FieldAsInteger("bag_amount") << "/"
              << Qry.FieldAsInteger("bag_weight") << "/"
              << Qry.FieldAsInteger("paid_weight");
      }
      else
      {
        msgh2 << ", "
              << Qry.FieldAsInteger("bag_type") << ": "
              << Qry.FieldAsInteger("bag_amount") << "/"
              << Qry.FieldAsInteger("bag_weight") << "/"
              << Qry.FieldAsInteger("paid_weight");
      };
    };
    if (!msgh2.str().empty())
    {
      msgh1 << msgh2.str();
      msg.msg="Багаж по типам (мест/вес/опл): "+msgh1.str().substr(2);
      reqInfo->MsgToLog(msg);

    };
  };
  Qry.Close();
};*/
