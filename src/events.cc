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

std::string TPaxToLogInfo::getBagStr() const
{
  std::ostringstream msg;
  if (bag_amount!=0 || bag_weight!=0)
  {
    if (!msg.str().empty()) msg << ", ";
    msg << "����� " << bag_amount << "/" << bag_weight;
  };
  if (rk_amount!=0 || rk_weight!=0)
  {
    if (!msg.str().empty()) msg << ", ";
    msg << "�/����� " << rk_amount << "/" << rk_weight;
  };
  if (!tags.empty())
  {
    if (!msg.str().empty()) msg << ", ";
    msg << "��ન " << tags;
  };
  return msg.str();
};

std::string TPaxToLogInfo::getPaxNameStr() const
{
  std::ostringstream msg;
  if (pers_type.empty())
    msg << "����� ��� ᮯ஢�������";
  else
    msg << "���ᠦ�� " << surname << (name.empty()?"":" ") << name
                       << " (" << pers_type << ")";
  return msg.str();
};

std::string TPaxToLogInfo::getNormStr() const
{
  std::ostringstream msg;
  if (norms.empty()) return "���";
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
  TQuery PaxDocQry(&OraSession);
  TQuery PaxDocoQry(&OraSession);
  TQuery NormQry(&OraSession);
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
  Qry.CreateVariable("lang",otString,AstraLocale::LANG_RU); //���� � ��� ��襬 �ᥣ�� �� ���᪮�
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
        LoadPaxDoc(paxInfoKey.pax_id, paxInfo.doc, PaxDocQry);
        LoadPaxDoco(paxInfoKey.pax_id, paxInfo.doco, PaxDocoQry);
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
        CheckIn::LoadPaxNorms(paxInfoKey.pax_id, norms, NormQry);
      else
        CheckIn::LoadGrpNorms(grp_id, norms, NormQry);
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
      //����� ����
      for(;!Qry.Eof;Qry.Next())
      {
        int bag_type=Qry.FieldIsNULL("bag_type")?-1:Qry.FieldAsInteger("bag_type");
        TPaidToLogInfo &paidInfo=grpInfo.paid[bag_type];
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
                  const TGrpToLogInfo &grpInfoAfter)
{
  bool SyncAODB=is_sync_aodb(point_id);

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
        a!=grpInfoAfter.pax.end() && b!=grpInfoBefore.pax.end() && b->first < a->first)
    {
      bPax=b;
      ++b;
    } else
    if (b==grpInfoBefore.pax.end() ||
        a!=grpInfoAfter.pax.end() && b!=grpInfoBefore.pax.end() && a->first < b->first)
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
      if (aPax->second.refuse.empty())
      {
        //���ᠦ�� �� ࠧॣ����஢��
        if (bPax!=grpInfoBefore.pax.end() && bPax->second.refuse.empty())
        {
          if (!aPax->second.cl.empty() &&
              !(aPax->second.surname==bPax->second.surname &&
                aPax->second.name==bPax->second.name &&
                aPax->second.pers_type==bPax->second.pers_type &&
                aPax->second.subcl==bPax->second.subcl &&
                aPax->second.seat_no==bPax->second.seat_no &&
                aPax->second.tkn==bPax->second.tkn &&
                aPax->second.doc==bPax->second.doc &&
                aPax->second.doco==bPax->second.doco))
          {
            //���ᠦ�� �������
            ostringstream msg;
            msg << aPax->second.getPaxNameStr() << ". "
                << "�������� ����� ���ᠦ��.";
            reqInfo->MsgToLog(msg.str(), ASTRA::evtPax, point_id, aPax->first.reg_no, grp_id);
            changed=true;
          };
          if (!(aPax->second.norms==bPax->second.norms))
          {
            ostringstream msg;
            msg << aPax->second.getPaxNameStr() << ". "
                << "���.����";
            if (aPax->second.pr_mark_norms && operFlt.airline!=markFlt.airline)
              msg << " (" << markFlt.airline << ")";
            msg << ": " << aPax->second.getNormStr();
            reqInfo->MsgToLog(msg.str(), ASTRA::evtPax, point_id, aPax->first.reg_no, grp_id);
            changed=true;
          };
        }
        else
        {
          //���ᠦ�� ��������
          ostringstream msg;
          msg << aPax->second.getPaxNameStr() << " ��ॣ����஢��";
          if (!aPax->second.cl.empty())
          {
            if (aPax->second.pr_exam) msg << ", ��襫 ��ᬮ��";
            if (aPax->second.pr_brd) msg << ", ��襫 ��ᠤ��";
            msg << ". �/�: " << aPax->second.airp_arv
                << ", �����: " << aPax->second.cl
                << ", �����: " << aPax->second.status
                << ", ����: " << (aPax->second.seat_no.empty()?"���":aPax->second.seat_no);
          }
          else
          {
            msg << ". �/�: " << aPax->second.airp_arv
                << ", �����: " << aPax->second.status;
          };
          msg << ". ���.����";
          if (aPax->second.pr_mark_norms && operFlt.airline!=markFlt.airline)
            msg << " (" << markFlt.airline << ")";
          msg << ": " << aPax->second.getNormStr();
          reqInfo->MsgToLog(msg.str(), ASTRA::evtPax, point_id, aPax->first.reg_no, grp_id);
          changed=true;
        };
      }
      else
      {
        //���ᠦ�� ࠧॣ����஢��
        if (!aPax->second.cl.empty())
        {
          if (bPax==grpInfoBefore.pax.end() || bPax->second.refuse.empty())
          {
            //࠭�� �� �뫨 ࠧॣ����஢���
            ostringstream msg;
            msg << aPax->second.getPaxNameStr() << " ࠧॣ����஢��. "
                << "��稭� �⪠�� � ॣ����樨: " << aPax->second.refuse << ". ";
            reqInfo->MsgToLog(msg.str(), ASTRA::evtPax, point_id, aPax->first.reg_no, grp_id);
            changed=true;
          }
          else
          {
            //࠭�� �뫨 ࠧॣ����஢���
            if (aPax->second.refuse!=bPax->second.refuse)
            {
              ostringstream msg;
              msg << aPax->second.getPaxNameStr() << ". "
                  << "�������� ��稭� �⪠�� � ॣ����樨: " << aPax->second.refuse << ". ";
              reqInfo->MsgToLog(msg.str(), ASTRA::evtPax, point_id, aPax->first.reg_no, grp_id);
              changed=true;
            };
          };
        }
        else
        {
          ostringstream msg;
          msg << aPax->second.getPaxNameStr() << " 㤠���";
          reqInfo->MsgToLog(msg.str(), ASTRA::evtPax, point_id, aPax->first.reg_no, grp_id);
          changed=true;
        };
      };
    }
    else
    {
      if (bPax==grpInfoBefore.pax.end()) continue;
      //���ᠦ�� 㤠���
      ostringstream msg;
      msg << bPax->second.getPaxNameStr() << " 㤠���";
      reqInfo->MsgToLog(msg.str(), ASTRA::evtPax, point_id, bPax->first.reg_no, grp_id);
      changed=true;
    };
    
    string bagStrAfter, bagStrBefore;
    if (aPax!=grpInfoAfter.pax.end() && aPax->second.refuse!=refuseAgentError)
      bagStrAfter=aPax->second.getBagStr();
    if (bPax!=grpInfoBefore.pax.end() && bPax->second.refuse!=refuseAgentError)
      bagStrBefore=bPax->second.getBagStr();
    if (bagStrAfter!=bagStrBefore)
    {
      //����� ���������
      ostringstream msg;
      msg << (aPax!=grpInfoAfter.pax.end()?aPax->second.getPaxNameStr():bPax->second.getPaxNameStr());
      
      if (bagStrAfter.empty())
        msg << ". �������: " << bagStrBefore;
      if (bagStrBefore.empty())
        msg << ". ���������: " << bagStrAfter;
      if (!bagStrAfter.empty() && !bagStrBefore.empty())
        msg << ". ������� �����. ��饥 ���-��: " << bagStrAfter;
        
      reqInfo->MsgToLog(msg.str(), ASTRA::evtPax,
                        point_id, aPax!=grpInfoAfter.pax.end()?aPax->first.reg_no:bPax->first.reg_no, grp_id);
      changed=true;
    };
    if (SyncAODB)
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
        if (changed) //�뫨 ��������� �� ॣ����樨
          update_aodb_pax_change( point_id, aodb_pax_id, aodb_reg_no, "�" );
        
        bool boardedAfter=false, boardedBefore=false;
        if (aPax!=grpInfoAfter.pax.end())
          boardedAfter=aPax->second.pr_brd;
        if (bPax!=grpInfoBefore.pax.end())
          boardedBefore=bPax->second.pr_brd;
        if (boardedAfter!=boardedBefore) //�뫨 ��������� � ��ᠤ���/��ᠤ���
          update_aodb_pax_change( point_id, aodb_pax_id, aodb_reg_no, "�" );
      };
    };
  };

  if (allGrpAgentError) return;

  if (grpInfoBefore.paid==grpInfoAfter.paid &&
      grpInfoBefore.excess==grpInfoAfter.excess) return;
  ostringstream msg;
  msg << "���. ���: " << grpInfoAfter.excess << " ��. "
      << "����� �� ⨯�� (����/���/���): ";
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
  else msg << "���";

  reqInfo->MsgToLog(msg.str(), ASTRA::evtPax, point_id, ASTRA::NoExists, grp_id);
};

