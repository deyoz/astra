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
#include "aodb.h"
#include "stat.h"

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
                    LParams() << LParam("flight", get_flight(logNode))
                    << LParam("day_issue", NodeAsString("day_issue", logNode)
                        )));
    }
    NewTextChild(logNode, "cap_test", getLocaleText("CAP.TEST", TReqInfo::Instance()->desk.lang));
    get_new_report_form("EventsLog", reqNode, resNode);
    NewTextChild(logNode, "short_page_number_fmt", getLocaleText("CAP.SHORT_PAGE_NUMBER_FMT"));
};

std::string TPaxToLogInfo::getBagStr() const
{
  std::ostringstream msg;
  if (bag_amount!=0 || bag_weight!=0)
  {
    if (!msg.str().empty()) msg << ", ";
    msg << "багаж " << bag_amount << "/" << bag_weight;
  };
  if (rk_amount!=0 || rk_weight!=0)
  {
    if (!msg.str().empty()) msg << ", ";
    msg << "р/кладь " << rk_amount << "/" << rk_weight;
  };
  if (!tags.empty())
  {
    if (!msg.str().empty()) msg << ", ";
    msg << "бирки " << tags;
  };
  return msg.str();
};

std::string TPaxToLogInfo::getPaxNameStr() const
{
  std::ostringstream msg;
  if (pers_type.empty())
    msg << "Багаж без сопровождения";
  else
    msg << (status!=EncodePaxStatus(psCrew)?"Пассажир ":"Член экипажа ")
        << surname
        << (name.empty()?"":" ") << name
        << " (" << pers_type << ")";
  return msg.str();
};

std::string TPaxToLogInfo::getNormStr() const
{
  std::ostringstream msg;
  if (norms.empty()) return "нет";
  std::map< int/*bag_type*/, CheckIn::TNormItem>::const_iterator n=norms.begin();
  for(;n!=norms.end();++n)
  {
    if (n!=norms.begin()) msg << ", ";
    if (n->first!=-1) msg << setw(2) << setfill('0') << n->first << ": ";
    msg << n->second.str();
  };
  return msg.str();
};

void GetGrpToLogInfo(int grp_id, TGrpToLogInfo &grpInfo)
{
  grpInfo.clear();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT "
    "       pax_grp.airp_arv, pax_grp.class, pax_grp.status, "
    "       pax_grp.pr_mark_norms, pax_grp.bag_refuse, "
    "       pax.pax_id, pax.reg_no, "
    "       pax.surname, pax.name, pax.pers_type, pax.refuse, pax.subclass, "
    "       salons.get_seat_no(pax.pax_id, pax.seats, pax_grp.status, pax_grp.point_dep, 'seats', rownum) seat_no, "
    "       pax.ticket_no, pax.coupon_no, pax.ticket_rem, 0 AS ticket_confirm, "
    "       pax.pr_brd, pax.pr_exam, "
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
      TPaxToLogInfoKey paxInfoKey;
      paxInfoKey.pax_id=Qry.FieldIsNULL("pax_id")?NoExists:Qry.FieldAsInteger("pax_id");
      paxInfoKey.reg_no=Qry.FieldIsNULL("reg_no")?NoExists:Qry.FieldAsInteger("reg_no");
      TPaxToLogInfo &paxInfo=grpInfo.pax[paxInfoKey];
      paxInfo.clear();
      paxInfo.airp_arv=Qry.FieldAsString("airp_arv");
      paxInfo.cl=Qry.FieldAsString("class");
      paxInfo.status=Qry.FieldAsString("status");
      paxInfo.pr_mark_norms=Qry.FieldAsInteger("pr_mark_norms")!=0;
      
      if (paxInfoKey.pax_id!=NoExists)
      {
        paxInfo.surname=Qry.FieldAsString("surname");
        paxInfo.name=Qry.FieldAsString("name");
        paxInfo.pers_type=Qry.FieldAsString("pers_type");
        paxInfo.refuse=Qry.FieldAsString("refuse");
        paxInfo.subcl=Qry.FieldAsString("subclass");
        paxInfo.seat_no=Qry.FieldAsString("seat_no");
        paxInfo.tkn.fromDB(Qry);
        paxInfo.pr_brd=paxInfo.refuse.empty() && !Qry.FieldIsNULL("pr_brd") && Qry.FieldAsInteger("pr_brd")!=0;
        paxInfo.pr_exam=paxInfo.refuse.empty() && !Qry.FieldIsNULL("pr_exam") && Qry.FieldAsInteger("pr_exam")!=0;
        LoadPaxDoc(paxInfoKey.pax_id, paxInfo.doc);
        LoadPaxDoco(paxInfoKey.pax_id, paxInfo.doco);
        LoadPaxDoca(paxInfoKey.pax_id, paxInfo.doca);
        LoadPaxRem(paxInfoKey.pax_id, true, paxInfo.rems);
        sort(paxInfo.rems.begin(), paxInfo.rems.end());
      }
      else
      {
        paxInfo.refuse=Qry.FieldAsInteger("bag_refuse")!=0?refuseAgentError:"";
      };
      
      paxInfo.bag_amount=Qry.FieldAsInteger("bag_amount");
      paxInfo.bag_weight=Qry.FieldAsInteger("bag_weight");
      paxInfo.rk_amount=Qry.FieldAsInteger("rk_amount");
      paxInfo.rk_weight=Qry.FieldAsInteger("rk_weight");
      paxInfo.tags=Qry.FieldAsString("tags");

      std::vector< std::pair<CheckIn::TPaxNormItem, CheckIn::TNormItem> > norms;
      if (paxInfoKey.pax_id!=NoExists)
        CheckIn::LoadPaxNorms(paxInfoKey.pax_id, norms);
      else
        CheckIn::LoadGrpNorms(grp_id, norms);
      paxInfo.norms.clear();
      std::vector< std::pair<CheckIn::TPaxNormItem, CheckIn::TNormItem> >::const_iterator i=norms.begin();
      for(; i!=norms.end(); ++i)
      {
        if (i->second.empty()) continue;
        int bag_type=i->first.bag_type==NoExists?-1:i->first.bag_type;
        paxInfo.norms[bag_type]=i->second;
      };
    };

    Qry.Clear();
    Qry.SQLText=
      "SELECT id, bag_type, pr_cabin, amount, weight, using_scales, "
      "       ckin.bag_pool_refused(bag2.grp_id,bag2.bag_pool_num,pax_grp.class,pax_grp.bag_refuse) AS refused "
      "FROM pax_grp,bag2 "
      "WHERE pax_grp.grp_id=bag2.grp_id AND "
      "      pax_grp.grp_id=:grp_id";
    Qry.CreateVariable("grp_id",otInteger,grp_id);
    Qry.Execute();
    if (!Qry.Eof)
    {
      //багаж есть
      for(;!Qry.Eof;Qry.Next())
      {
        int bag_type=Qry.FieldIsNULL("bag_type")?-1:Qry.FieldAsInteger("bag_type");
        TBagToLogInfo bagInfo;
        bagInfo.id=Qry.FieldAsInteger("id");
        bagInfo.pr_cabin=Qry.FieldAsInteger("pr_cabin")!=0;
        bagInfo.amount=Qry.FieldAsInteger("amount");
        bagInfo.weight=Qry.FieldAsInteger("weight");
        bagInfo.bag_type=Qry.FieldIsNULL("bag_type")?ASTRA::NoExists:Qry.FieldAsInteger("bag_type");
        bagInfo.using_scales=Qry.FieldAsInteger("using_scales")!=0;
        grpInfo.bag[bagInfo.id]=bagInfo;

        if (Qry.FieldAsInteger("refused")!=0) continue;

        std::map< int/*bag_type*/, TPaidToLogInfo>::iterator i=grpInfo.paid.find(bag_type);
        if (i!=grpInfo.paid.end())
        {
          i->second.bag_amount+=bagInfo.amount;
          i->second.bag_weight+=bagInfo.weight;
        }
        else
        {
          TPaidToLogInfo &paidInfo=grpInfo.paid[bag_type];
          paidInfo.bag_type=bag_type;
          paidInfo.bag_amount=bagInfo.amount;
          paidInfo.bag_weight=bagInfo.weight;
        };
      };

      Qry.Clear();
      Qry.SQLText=
        "SELECT bag_type, weight FROM paid_bag WHERE grp_id=:grp_id";
      Qry.CreateVariable("grp_id",otInteger,grp_id);
      Qry.Execute();
      for(;!Qry.Eof;Qry.Next())
      {
        int bag_type=Qry.FieldIsNULL("bag_type")?-1:Qry.FieldAsInteger("bag_type");
        TPaidToLogInfo &paidInfo=grpInfo.paid[bag_type];
        paidInfo.bag_type=bag_type;
        paidInfo.paid_weight=Qry.FieldAsInteger("weight");
      };
    };
  };
};

void SaveGrpToLog(int point_id,
                  const TTripInfo &operFlt,
                  const TTripInfo &markFlt,
                  const TGrpToLogInfo &grpInfoBefore,
                  const TGrpToLogInfo &grpInfoAfter,
                  TAgentStatInfo &agentStat)
{
  bool SyncPaxs=is_sync_paxs(point_id);
  
  bool auto_weighing=GetAutoWeighing(point_id, "Р");
  
  agentStat.clear();

  int grp_id=grpInfoAfter.grp_id==NoExists?grpInfoBefore.grp_id:grpInfoAfter.grp_id;

  TReqInfo* reqInfo = TReqInfo::Instance();
  map< TPaxToLogInfoKey, TPaxToLogInfo>::const_iterator a=grpInfoAfter.pax.begin();
  map< TPaxToLogInfoKey, TPaxToLogInfo>::const_iterator b=grpInfoBefore.pax.begin();
  bool allGrpAgentError=true;
  for(;a!=grpInfoAfter.pax.end() || b!=grpInfoBefore.pax.end();)
  {
    map< TPaxToLogInfoKey, TPaxToLogInfo>::const_iterator aPax=grpInfoAfter.pax.end();
    map< TPaxToLogInfoKey, TPaxToLogInfo>::const_iterator bPax=grpInfoBefore.pax.end();
  
    if (a==grpInfoAfter.pax.end() ||
        (a!=grpInfoAfter.pax.end() && b!=grpInfoBefore.pax.end() && b->first < a->first))
    {
      bPax=b;
      ++b;
    } else
    if (b==grpInfoBefore.pax.end() ||
        (a!=grpInfoAfter.pax.end() && b!=grpInfoBefore.pax.end() && a->first < b->first))
    {
      aPax=a;
      ++a;
    } else
    if (a!=grpInfoAfter.pax.end() && b!=grpInfoBefore.pax.end() && a->first==b->first)
    {
      aPax=a;
      bPax=b;
      ++a;
      ++b;
    };
    
    if (aPax!=grpInfoAfter.pax.end() && aPax->second.refuse!=refuseAgentError) allGrpAgentError=false;
    
    bool changed=false;
    if (aPax!=grpInfoAfter.pax.end())
    {
      bool is_crew=aPax->second.status==EncodePaxStatus(psCrew);
      bool is_unaccomp=aPax->second.cl.empty() && !is_crew;
      if (aPax->second.refuse.empty())
      {
        //пассажир не разрегистрирован
        if (bPax!=grpInfoBefore.pax.end() && bPax->second.refuse.empty())
        {
          if (!is_unaccomp &&
              !(aPax->second.surname==bPax->second.surname &&
                aPax->second.name==bPax->second.name &&
                aPax->second.pers_type==bPax->second.pers_type &&
                aPax->second.subcl==bPax->second.subcl &&
                aPax->second.seat_no==bPax->second.seat_no &&
                aPax->second.tkn==bPax->second.tkn &&
                aPax->second.doc==bPax->second.doc &&
                aPax->second.doco==bPax->second.doco &&
                aPax->second.doca==bPax->second.doca &&
                aPax->second.rems==bPax->second.rems))
          {
            //пассажир изменен
            ostringstream msg;
            msg << aPax->second.getPaxNameStr() << ". "
                << "Изменены данные "
                << (!is_crew?"пассажира.":"члена экипажа.");
            reqInfo->MsgToLog(msg.str(), ASTRA::evtPax, point_id, aPax->first.reg_no, grp_id);
            changed=true;
          };
          if (!(aPax->second.norms==bPax->second.norms))
          {
            ostringstream msg;
            msg << aPax->second.getPaxNameStr() << ". "
                << "Баг.нормы";
            if (aPax->second.pr_mark_norms && operFlt.airline!=markFlt.airline)
              msg << " (" << markFlt.airline << ")";
            msg << ": " << aPax->second.getNormStr();
            reqInfo->MsgToLog(msg.str(), ASTRA::evtPax, point_id, aPax->first.reg_no, grp_id);
            changed=true;
          };
        }
        else
        {
          //пассажир добавлен
          ostringstream msg;
          msg << aPax->second.getPaxNameStr() << " зарегистрирован";
          if (!is_unaccomp)
          {
            if (aPax->second.pr_exam) msg << ", прошел досмотр";
            if (aPax->second.pr_brd) msg << ", прошел посадку";
            msg << ". П/н: " << aPax->second.airp_arv;
            if (!is_crew)
              msg << ", класс: " << aPax->second.cl
                  << ", статус: " << aPax->second.status
                  << ", место: " << (aPax->second.seat_no.empty()?"нет":aPax->second.seat_no);
          }
          else
          {
            msg << ". П/н: " << aPax->second.airp_arv
                << ", статус: " << aPax->second.status;
          };
          msg << ". Баг.нормы";
          if (aPax->second.pr_mark_norms && operFlt.airline!=markFlt.airline)
            msg << " (" << markFlt.airline << ")";
          msg << ": " << aPax->second.getNormStr();
          reqInfo->MsgToLog(msg.str(), ASTRA::evtPax, point_id, aPax->first.reg_no, grp_id);
          changed=true;
        };
      }
      else
      {
        //пассажир разрегистрирован
        if (!is_unaccomp)
        {
          if (bPax==grpInfoBefore.pax.end() || bPax->second.refuse.empty())
          {
            //ранее не были разрегистрированы
            ostringstream msg;
            msg << aPax->second.getPaxNameStr() << " разрегистрирован. ";
            if (reqInfo->client_type!=ctTerm && aPax->second.refuse==refuseAgentError)
              msg << "Причина: отказ от регистрации с сайта. ";
            else
              msg << "Причина отказа в регистрации: " << aPax->second.refuse << ". ";
            reqInfo->MsgToLog(msg.str(), ASTRA::evtPax, point_id, aPax->first.reg_no, grp_id);
            changed=true;
          }
          else
          {
            //ранее были разрегистрированы
            if (aPax->second.refuse!=bPax->second.refuse)
            {
              ostringstream msg;
              msg << aPax->second.getPaxNameStr() << ". ";
              if (reqInfo->client_type!=ctTerm && aPax->second.refuse==refuseAgentError)
                msg << "Отказ от регистрации с сайта. ";
              else
                msg << "Изменена причина отказа в регистрации: " << aPax->second.refuse << ". ";
              reqInfo->MsgToLog(msg.str(), ASTRA::evtPax, point_id, aPax->first.reg_no, grp_id);
              changed=true;
            };
          };
        }
        else
        {
          ostringstream msg;
          msg << aPax->second.getPaxNameStr() << " удален";
          reqInfo->MsgToLog(msg.str(), ASTRA::evtPax, point_id, aPax->first.reg_no, grp_id);
          changed=true;
        };
      };
    }
    else
    {
      if (bPax==grpInfoBefore.pax.end()) continue;
      //пассажир удален
      ostringstream msg;
      msg << bPax->second.getPaxNameStr() << " удален";
      reqInfo->MsgToLog(msg.str(), ASTRA::evtPax, point_id, bPax->first.reg_no, grp_id);
      changed=true;
    };
    
    string bagStrAfter, bagStrBefore;
    int d=0;
    if (aPax!=grpInfoAfter.pax.end() && aPax->second.refuse!=refuseAgentError)
    {
      bagStrAfter=aPax->second.getBagStr();
      d++;
    };
    if (bPax!=grpInfoBefore.pax.end() && bPax->second.refuse!=refuseAgentError)
    {
      bagStrBefore=bPax->second.getBagStr();
      d--;
    };
    if (d>0) agentStat.dpax_amount.inc++;
    if (d<0) agentStat.dpax_amount.dec++;
    if (changed) agentStat.pax_amount++;
    
    if (bagStrAfter!=bagStrBefore)
    {
      //багаж изменился
      ostringstream msg;
      msg << (aPax!=grpInfoAfter.pax.end()?aPax->second.getPaxNameStr():bPax->second.getPaxNameStr());
      
      if (bagStrAfter.empty())
        msg << ". Удалено: " << bagStrBefore;
      if (bagStrBefore.empty())
        msg << ". Добавлено: " << bagStrAfter;
      if (!bagStrAfter.empty() && !bagStrBefore.empty())
        msg << ". Изменен багаж. Общее кол-во: " << bagStrAfter;
        
      reqInfo->MsgToLog(msg.str(), ASTRA::evtPax,
                        point_id, aPax!=grpInfoAfter.pax.end()?aPax->first.reg_no:bPax->first.reg_no, grp_id);
      changed=true;
    };
    if (SyncPaxs)
    {
      int aodb_pax_id=NoExists;
      int aodb_reg_no=NoExists;
      if (aPax!=grpInfoAfter.pax.end())
      {
        aodb_pax_id=aPax->first.pax_id;
        aodb_reg_no=aPax->first.reg_no;
      }
      else
      {
        aodb_pax_id=bPax->first.pax_id;
        aodb_reg_no=bPax->first.reg_no;
      };
      if (aodb_pax_id!=NoExists && aodb_reg_no!=NoExists)
      {
        if (changed) //были изменения по регистрации
          update_pax_change( point_id, aodb_pax_id, aodb_reg_no, "Р" );
        
        bool boardedAfter=false, boardedBefore=false;
        if (aPax!=grpInfoAfter.pax.end())
          boardedAfter=aPax->second.pr_brd;
        if (bPax!=grpInfoBefore.pax.end())
          boardedBefore=bPax->second.pr_brd;
        if (boardedAfter!=boardedBefore) //были изменения с посадкой/высадкой
          update_pax_change( point_id, aodb_pax_id, aodb_reg_no, "П" );
      };
    };
  };
  //багаж введенный вручную (using_scales=false):
  map<bool/*pr_cabin*/, map< int/*id*/, TBagToLogInfo> > man_weighing_bag_inc, man_weighing_bag_dec;
  //агентская статистика по изменениям багажа
  {
    std::map< int/*id*/, TBagToLogInfo>::const_iterator a=grpInfoAfter.bag.begin();
    std::map< int/*id*/, TBagToLogInfo>::const_iterator b=grpInfoBefore.bag.begin();
    for(;a!=grpInfoAfter.bag.end() || b!=grpInfoBefore.bag.end();)
    {
      std::map< int/*id*/, TBagToLogInfo>::const_iterator aBag=grpInfoAfter.bag.end();
      std::map< int/*id*/, TBagToLogInfo>::const_iterator bBag=grpInfoBefore.bag.end();

      if (a==grpInfoAfter.bag.end() ||
          (a!=grpInfoAfter.bag.end() && b!=grpInfoBefore.bag.end() && b->first < a->first))
      {
        bBag=b;
        ++b;
      } else
      if (b==grpInfoBefore.bag.end() ||
          (a!=grpInfoAfter.bag.end() && b!=grpInfoBefore.bag.end() && a->first < b->first))
      {
        aBag=a;
        ++a;
      } else
      if (a!=grpInfoAfter.bag.end() && b!=grpInfoBefore.bag.end() && a->first==b->first)
      {
        if (!(a->second==b->second))
        {
          aBag=a;
          bBag=b;
        };
        ++a;
        ++b;
      };

      if (aBag!=grpInfoAfter.bag.end())
      {
        if (aBag->second.pr_cabin)
        {
          agentStat.drk_amount.inc+=aBag->second.amount;
          agentStat.drk_weight.inc+=aBag->second.weight;
        }
        else
        {
          agentStat.dbag_amount.inc+=aBag->second.amount;
          agentStat.dbag_weight.inc+=aBag->second.weight;
        };
        if (auto_weighing && !aBag->second.using_scales)
          man_weighing_bag_inc[aBag->second.pr_cabin].insert(*aBag);
      };

      if (bBag!=grpInfoBefore.bag.end())
      {
        if (bBag->second.pr_cabin)
        {
          agentStat.drk_amount.dec+=bBag->second.amount;
          agentStat.drk_weight.dec+=bBag->second.weight;
        }
        else
        {
          agentStat.dbag_amount.dec+=bBag->second.amount;
          agentStat.dbag_weight.dec+=bBag->second.weight;
        };
        if (auto_weighing && !bBag->second.using_scales)
          man_weighing_bag_dec[bBag->second.pr_cabin].insert(*bBag);
      };

    };
  };

  for(int pass=0; pass<2; pass++)
  {
    map<bool/*pr_cabin*/, map< int/*id*/, TBagToLogInfo> >::const_iterator i=
      (pass==0)?man_weighing_bag_dec.begin():man_weighing_bag_inc.begin();
    for(;i!=(pass==0?man_weighing_bag_dec.end():man_weighing_bag_inc.end());++i)
    {
      if (i->second.empty()) continue;

      ostringstream msg;
      if (pass==0)
      {
        if (i->first) //pr_cabin
          msg << "Удалена р/к с весом, введенным вручную: ";
        else
          msg << "Удален багаж с весом, введенным вручную: ";
      }
      else
      {
        if (i->first) //pr_cabin
          msg << "Добавлена р/к с весом, введенным вручную: ";
        else
          msg << "Добавлен багаж с весом, введенным вручную: ";
      };
      for(map< int/*id*/, TBagToLogInfo>::const_iterator j=i->second.begin();
                                                         j!=i->second.end(); ++j)
      {
        if (j!=i->second.begin()) msg << ", ";
        if (j->second.bag_type!=ASTRA::NoExists)
          msg << setw(2) << setfill('0') << j->second.bag_type << ":";
        msg << j->second.amount << "/" << j->second.weight;
      };
      reqInfo->MsgToLog(msg.str(), ASTRA::evtPax, point_id, ASTRA::NoExists, grp_id);
    };
  };

  if (allGrpAgentError) return;

  if (grpInfoBefore.paid==grpInfoAfter.paid &&
      grpInfoBefore.excess==grpInfoAfter.excess) return;
  ostringstream msg;
  msg << "Опл. вес: " << grpInfoAfter.excess << " кг. "
      << "Багаж по типам (мест/вес/опл): ";
  if (!grpInfoAfter.paid.empty())
  {
    map< int/*bag_type*/, TPaidToLogInfo>::const_iterator p=grpInfoAfter.paid.begin();
    for(;p!=grpInfoAfter.paid.end();++p)
    {
      if (p!=grpInfoAfter.paid.begin()) msg << ", ";
      if (p->second.bag_type!=-1) msg << setw(2) << setfill('0') << p->second.bag_type << ":";
      msg << p->second.bag_amount << "/" << p->second.bag_weight << "/" << p->second.paid_weight;
    };
  }
  else msg << "нет";

  reqInfo->MsgToLog(msg.str(), ASTRA::evtPax, point_id, ASTRA::NoExists, grp_id);
};

//функция не только возвращает auto_weighing для пульта,
//но и пишет в лог, если для данного пульта изменилась настройка
bool GetAutoWeighing(int point_id, const string &work_mode)
{
  TReqInfo* reqInfo = TReqInfo::Instance();
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT trip_sets.auto_weighing, stations.using_scales "
    "FROM trip_sets, stations "
    "WHERE trip_sets.point_id=:point_id AND "
    "      stations.desk=:desk AND stations.work_mode=:work_mode";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.CreateVariable("desk", otString, reqInfo->desk.code);
  Qry.CreateVariable("work_mode", otString, work_mode);
  Qry.Execute();
  bool auto_weighing=false;
  if (!Qry.Eof)
    auto_weighing=Qry.FieldAsInteger("auto_weighing")!=0 &&
                  Qry.FieldAsInteger("using_scales")!=0;

  Qry.Clear();
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.CreateVariable("desk", otString, reqInfo->desk.code);
  if (auto_weighing)
  {
    Qry.SQLText=
      "INSERT INTO trip_auto_weighing(point_id, desk) VALUES(:point_id, :desk)";
    try
    {
      Qry.Execute();
      if (Qry.RowsProcessed()>0)
      {
        ostringstream msg;
        msg << "Установлен контроль автоматического взвешивания багажа для пульта "
            << reqInfo->desk.code;
        reqInfo->MsgToLog(msg.str(), ASTRA::evtFlt, point_id);
      };
    }
    catch(EOracleError E)
    {
      if (E.Code!=1) throw;
    };
  }
  else
  {
    Qry.SQLText=
      "DELETE FROM trip_auto_weighing WHERE point_id=:point_id AND desk=:desk";
    Qry.Execute();
    if (Qry.RowsProcessed()>0)
    {
      ostringstream msg;
      msg << "Отменен контроль автоматического взвешивания багажа для пульта "
          << reqInfo->desk.code;
      reqInfo->MsgToLog(msg.str(), ASTRA::evtFlt, point_id);
    };
  };

  return auto_weighing;
};


