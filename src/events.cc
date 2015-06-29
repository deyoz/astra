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

    if(part_key != NoExists && ARX_EVENTS_DISABLED())
        throw UserException("MSG.ERR_MSG.ARX_EVENTS_DISABLED");

    if (move_id != NoExists || !eventTypes.empty())
    {
      Qry.Clear();
      ostringstream sql;
      if (move_id != NoExists)
      {
        sql << "SELECT msg, time, id2 AS point_id, \n"
               "       DECODE(type,:evtPax,id2,:evtPay,id2,-1) AS reg_no, \n"
               "       DECODE(type,:evtPax,id3,:evtPay,id3,-1) AS grp_id, \n"
               "       ev_user, station, ev_order, NVL(part_num, 1) AS part_num \n";
        if (part_key != NoExists)
          sql << "FROM arx_events \n"
                 "WHERE part_key=:part_key AND (lang=:lang OR lang=:lang_undef) AND \n";
        else
          sql << "FROM events_bilingual \n"
                 "WHERE lang=:lang AND \n";
        sql << " type=:evtDisp AND id1=:move_id \n";
        Qry.CreateVariable("evtDisp",otString,EncodeEventType(ASTRA::evtDisp));
        Qry.CreateVariable("move_id",otInteger,move_id);
      };

      if (move_id != NoExists && !eventTypes.empty()) sql << "UNION \n";

      if (!eventTypes.empty())
      {
        sql << "SELECT msg, time, id1 AS point_id, \n"
               "       DECODE(type,:evtPax,id2,:evtPay,id2,-1) AS reg_no, \n"
               "       DECODE(type,:evtPax,id3,:evtPay,id3,-1) AS grp_id, \n"
               "       ev_user, station, ev_order, NVL(part_num, 1) AS part_num \n";
        if (part_key != NoExists)
          sql << "FROM arx_events \n"
                 "WHERE part_key=:part_key AND (lang=:lang OR lang=:lang_undef) AND \n";
        else
          sql << "FROM events_bilingual \n"
                 "WHERE lang=:lang AND \n";
        sql << " type IN " << GetSQLEnum(eventTypes) << " AND id1=:point_id \n";
        Qry.CreateVariable("point_id",otInteger,point_id);
      };
      Qry.CreateVariable("lang", otString, TReqInfo::Instance()->desk.lang);
      Qry.CreateVariable("evtPax",otString,EncodeEventType(ASTRA::evtPax));
      Qry.CreateVariable("evtPay",otString,EncodeEventType(ASTRA::evtPay));
      if (part_key != NoExists) {
        Qry.CreateVariable( "part_key", otDate, part_key );
        Qry.CreateVariable("lang_undef", otString, "ZZ");
      }

      //ProgTrace(TRACE5, "GetEvents: SQL=\n%s", sql.str().c_str());
      Qry.SQLText=sql.str().c_str();
      Qry.Execute();

      if (!Qry.Eof)
      {
        int col_msg=Qry.FieldIndex("msg");
        int col_time=Qry.FieldIndex("time");
        int col_point_id=Qry.FieldIndex("point_id");
        int col_reg_no=Qry.FieldIndex("reg_no");
        int col_grp_id=Qry.FieldIndex("grp_id");
        int col_ev_user=Qry.FieldIndex("ev_user");
        int col_station=Qry.FieldIndex("station");
        int col_ev_order=Qry.FieldIndex("ev_order");
        int col_part_num=Qry.FieldIndex("part_num");

        for(;!Qry.Eof;Qry.Next())
        {
          xmlNodePtr rowNode=NewTextChild(logNode,"row");
          NewTextChild(rowNode,"point_id",Qry.FieldAsInteger(col_point_id));
          NewTextChild(rowNode,"ev_user",Qry.FieldAsString(col_ev_user));
          NewTextChild(rowNode,"station",Qry.FieldAsString(col_station));

          TDateTime time = UTCToClient(Qry.FieldAsDateTime(col_time),reqInfo->desk.tz_region);

          NewTextChild(rowNode,"time",DateTimeToStr(time));
          NewTextChild(rowNode,"fmt_time",DateTimeToStr(time, "dd.mm.yy hh:nn"));
          NewTextChild(rowNode,"grp_id",Qry.FieldAsInteger(col_grp_id),-1);
          NewTextChild(rowNode,"reg_no",Qry.FieldAsInteger(col_reg_no),-1);
          NewTextChild(rowNode,"msg",Qry.FieldAsString(col_msg));
          NewTextChild(rowNode,"ev_order",Qry.FieldAsInteger(col_ev_order));
          NewTextChild(rowNode,"part_num",Qry.FieldAsInteger(col_part_num),1);
        };
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
}

void TPaxToLogInfo::getBag(PrmEnum& param) const
{
  bool empty = true;
  if (bag_amount!=0 || bag_weight!=0)
  {
      param.prms << PrmLexema("", "EVT.LUGGAGE") << PrmSmpl<string>("", " ")
                 << PrmSmpl<int>("", bag_amount) << PrmSmpl<string>("", "/")
                 << PrmSmpl<int>("", bag_weight);
      empty = false;
  };
  if (rk_amount!=0 || rk_weight!=0)
  {
    if (!empty) param.prms << PrmSmpl<string>("", ", ");
    param.prms << PrmLexema("", "EVT.CABIN_LUGGAGE") << PrmSmpl<string>("", " ")
               << PrmSmpl<int>("", rk_amount) << PrmSmpl<string>("", "/")
               << PrmSmpl<int>("", rk_weight);
    empty = false;
  };

  if (!tags.empty())
  {
    if (!empty) param.prms << PrmSmpl<string>("", ", ");
    param.prms << PrmLexema("", "EVT.TAGS") << PrmSmpl<string>("", " ")
               << PrmSmpl<string>("", tags);
  };
}

void logPaxName(const string &status, const string &surname,
                const string &name, const string &pers_type,
                LEvntPrms& params)
{
  if (pers_type.empty())
    params << PrmLexema("pax_name", "EVT.UNACCOMPANIED_LUGGAGE");
  else {
    PrmLexema lexema("pax_name", "EVT.PASSENGER");
    if (status == EncodePaxStatus(psCrew)) lexema.ChangeLexemaId("EVT.CREW_MEMBER");
    lexema.prms << PrmSmpl<std::string>("surname", surname)
                << PrmSmpl<std::string>("name", (name.empty()?"":" ") + name)
                << PrmElem<std::string>("pers_type", etPersType, pers_type);
    params << lexema;
  }
}

void TPaxToLogInfo::getPaxName(LEvntPrms& params) const
{
  logPaxName(status, surname, name, pers_type, params);
};

void TPaxToLogInfo::getNorm(PrmEnum& param) const
{
  if (norms.empty()) {
    param.prms << PrmBool("", false);
    return;
  }
  std::map< int/*bag_type*/, CheckIn::TNormItem>::const_iterator n=norms.begin();
  for(;n!=norms.end();++n)
  {
    std::ostringstream msg;
    if (n!=norms.begin()) param.prms << PrmSmpl<string>("", ", ");
    if (n->first!=-1) {
      msg << setw(2) << setfill('0') << n->first << ": ";
      param.prms << PrmSmpl<string>("", msg.str());
    }
    n->second.GetNorms(param);
  };
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
    "       pax.surname, pax.name, pax.pers_type, pax.refuse, pax.subclass, pax.is_female, "
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
        if (!Qry.FieldIsNULL("is_female"))
          paxInfo.is_female=(int)(Qry.FieldAsInteger("is_female")!=0);
        else
          paxInfo.is_female=ASTRA::NoExists;
        paxInfo.tkn.fromDB(Qry);
        paxInfo.pr_brd=paxInfo.refuse.empty() && !Qry.FieldIsNULL("pr_brd") && Qry.FieldAsInteger("pr_brd")!=0;
        paxInfo.pr_exam=paxInfo.refuse.empty() && !Qry.FieldIsNULL("pr_exam") && Qry.FieldAsInteger("pr_exam")!=0;
        paxInfo.apis.fromDB(paxInfoKey.pax_id);
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
      "SELECT id, bag_type, pr_cabin, amount, weight, using_scales, is_trfer, "
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
        bagInfo.is_trfer=Qry.FieldAsInteger("is_trfer")!=0;
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

    Qry.Clear();
    Qry.SQLText="SELECT * FROM paid_bag_emd WHERE grp_id=:grp_id";
    Qry.CreateVariable("grp_id",otInteger,grp_id);
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next())
      grpInfo.emd.insert(CheckIn::TPaidBagEMDItem().fromDB(Qry));
  };
};

void GetAPISLogMsgs(const CheckIn::TAPISItem &apisBefore,
                    const CheckIn::TAPISItem &apisAfter,
                    list<pair<string, string> > &msgs)
{
  msgs.clear();

  bool manualInputBefore=(apisBefore.doc.scanned_attrs & apisBefore.doc.getNotEmptyFieldsMask()) != apisBefore.doc.getNotEmptyFieldsMask();
  bool manualInputAfter=(apisAfter.doc.scanned_attrs & apisAfter.doc.getNotEmptyFieldsMask()) != apisAfter.doc.getNotEmptyFieldsMask();
  if (!(apisAfter.doc.equalAttrs(apisBefore.doc) && manualInputBefore==manualInputAfter))
  {
    //изменения по документу
    ostringstream msg;
    string id;
    msg << "DOCS: "
        << apisAfter.doc.type << "/"
        << apisAfter.doc.issue_country << "/"
        << apisAfter.doc.no << "/"
        << apisAfter.doc.nationality << "/"
        << (apisAfter.doc.birth_date!=ASTRA::NoExists?DateTimeToStr(apisAfter.doc.birth_date, "ddmmmyy", true):"") << "/"
        << apisAfter.doc.gender << "/"
        << (apisAfter.doc.expiry_date!=ASTRA::NoExists?DateTimeToStr(apisAfter.doc.expiry_date, "ddmmmyy", true):"") << "/"
        << apisAfter.doc.surname << "/"
        << apisAfter.doc.first_name << "/"
        << apisAfter.doc.second_name << ".";
    id = (manualInputAfter?"EVT.APIS_LOG_MANUAL_INPUT":"EVT.APIS_LOG_SCANNING");
    msgs.push_back(make_pair(id, msg.str()));
  };

  manualInputBefore=(apisBefore.doco.scanned_attrs & apisBefore.doco.getNotEmptyFieldsMask()) != apisBefore.doco.getNotEmptyFieldsMask();
  manualInputAfter=(apisAfter.doco.scanned_attrs & apisAfter.doco.getNotEmptyFieldsMask()) != apisAfter.doco.getNotEmptyFieldsMask();
  if (!(apisAfter.doco.equalAttrs(apisBefore.doco) && manualInputBefore==manualInputAfter))
  {
    //изменения по визе
    ostringstream msg;
    string id;
    msg << "DOCO: "
        << apisAfter.doco.birth_place << "/"
        << apisAfter.doco.type << "/"
        << apisAfter.doco.no << "/"
        << apisAfter.doco.issue_place << "/"
        << (apisAfter.doco.issue_date!=ASTRA::NoExists?DateTimeToStr(apisAfter.doco.issue_date, "ddmmmyy", true):"") << "/"
        << apisAfter.doco.applic_country << "/"
        << (apisAfter.doco.expiry_date!=ASTRA::NoExists?DateTimeToStr(apisAfter.doco.expiry_date, "ddmmmyy", true):"") << ".";
        id = (manualInputAfter?"EVT.APIS_LOG_MANUAL_INPUT":"EVT.APIS_LOG_SCANNING");
    msgs.push_back(make_pair(id, msg.str()));
  };

  CheckIn::TPaxDocaItem docaBefore[3];
  for(list<CheckIn::TPaxDocaItem>::const_iterator d=apisBefore.doca.begin(); d!=apisBefore.doca.end(); ++d)
  {
    if (d->type=="B") docaBefore[0]=*d;
    if (d->type=="R") docaBefore[1]=*d;
    if (d->type=="D") docaBefore[2]=*d;
  };

  CheckIn::TPaxDocaItem docaAfter[3];
  for(list<CheckIn::TPaxDocaItem>::const_iterator d=apisAfter.doca.begin(); d!=apisAfter.doca.end(); ++d)
  {
    if (d->type=="B") docaAfter[0]=*d;
    if (d->type=="R") docaAfter[1]=*d;
    if (d->type=="D") docaAfter[2]=*d;
  };

  for(int pass=0; pass<3; pass++)
  {
    if (docaAfter[pass]==docaBefore[pass]) continue;

    ostringstream msg;
    if (pass==0) msg << "DOCA(B): ";
    if (pass==1) msg << "DOCA(R): ";
    if (pass==2) msg << "DOCA(D): ";
    //изменения адреса
    msg << docaAfter[pass].type << "/"
        << docaAfter[pass].country << "/"
        << docaAfter[pass].address << "/"
        << docaAfter[pass].city << "/"
        << docaAfter[pass].region << "/"
        << docaAfter[pass].postal_code;
    msgs.push_back(make_pair("EVT.APIS_LOG", msg.str()));
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
  bool apis_control=GetAPISControl(point_id);

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
                (apis_control ||
                 (aPax->second.apis.doc.equalAttrs(bPax->second.apis.doc) &&
                  aPax->second.apis.doco.equalAttrs(bPax->second.apis.doco) &&
                  aPax->second.apis.doca==bPax->second.apis.doca)) &&
                aPax->second.rems==bPax->second.rems))
          {
            //пассажир изменен
            string lexema_id;
            if (!is_crew) lexema_id = "EVT.CHANGED_PASSENGER_DATA";
            else lexema_id = "EVT.CHANGED_CREW_MEMBER_DATA";
            LEvntPrms params;
            aPax->second.getPaxName(params);
            reqInfo->LocaleToLog(lexema_id, params, ASTRA::evtPax, point_id, aPax->first.reg_no, grp_id);
            changed=true;
          };
          if (!(aPax->second.norms==bPax->second.norms))
          {
            LEvntPrms params;
            aPax->second.getPaxName(params);
            if (aPax->second.pr_mark_norms && operFlt.airline!=markFlt.airline) {
              PrmEnum prmenum("airline", "");
              prmenum.prms << PrmSmpl<string>("", " (") << PrmElem<string>("", etAirline, markFlt.airline)
                           << PrmSmpl<string>("", ") ");
              params << prmenum;
            }
            else
              params << PrmSmpl<string>("airline", "");
            PrmEnum norms("norm", "");
            aPax->second.getNorm(norms);
            params << norms;
            reqInfo->LocaleToLog("EVT.BAGG_NORMS", params, ASTRA::evtPax, point_id, aPax->first.reg_no, grp_id);
            changed=true;
          };
          if (!is_unaccomp && apis_control)
          {
            list<pair<string, string> > msgs;
            GetAPISLogMsgs(bPax->second.apis, aPax->second.apis, msgs);
            for(list<pair<string, string> >::const_iterator m=msgs.begin(); m!=msgs.end(); ++m)
            {
              LEvntPrms params;
              aPax->second.getPaxName(params);
              reqInfo->LocaleToLog(m->first, params << PrmSmpl<string>("params", m->second),
                                               ASTRA::evtPax, point_id, aPax->first.reg_no, grp_id);
              changed=true;
            };
          };
        }
        else
        {
          //пассажир добавлен
          LEvntPrms params;
          aPax->second.getPaxName(params);
          params << PrmElem<string>("airp", etAirp, aPax->second.airp_arv);
          if (!is_unaccomp)
          {
            if (aPax->second.pr_exam)
              params << PrmLexema("exam", "EVT.EXAMED");
            else
              params << PrmSmpl<string>("exam", "");
            if (aPax->second.pr_brd)
              params << PrmLexema("board", "EVT.BOARDED");
            else
              params << PrmSmpl<string>("board", "");
            if (!is_crew) {
              PrmLexema lexema("params" ,"EVT.CLASS_STATUS_SEAT_NO");
              lexema.prms << PrmElem<string>("cls", etClass, aPax->second.cl)
                          << PrmElem<string>("status", etGrpStatusType, aPax->second.status, efmtNameLong);
              if (!aPax->second.seat_no.empty())
                lexema.prms << PrmSmpl<string>("seat_no", aPax->second.seat_no);
              else
                lexema.prms << PrmBool("seat_no", false);
              params << lexema;
            }
            else
              params << PrmSmpl<string>("params", "");
          }
          else
          {
            params << PrmSmpl<string>("exam", "") << PrmLexema("board", "EVT.BOARDED");
            PrmLexema lexema("params" ,"EVT.STATUS");
            lexema.prms << PrmElem<string>("status", etGrpStatusType, aPax->second.status, efmtNameLong);
            params << lexema;
          };
          if (aPax->second.pr_mark_norms && operFlt.airline!=markFlt.airline) {
            PrmEnum prmenum("airline", "");
            prmenum.prms << PrmSmpl<string>("", " (") << PrmElem<string>("", etAirline, markFlt.airline)
                         << PrmSmpl<string>("", ") ");
            params << prmenum;
          }
          else
            params << PrmSmpl<string>("airline", "");
          PrmEnum norms("norm", "");
          aPax->second.getNorm(norms);
          params << norms;
          reqInfo->LocaleToLog("EVT.PASSENGER.CHECKEDIN", params, ASTRA::evtPax, point_id, aPax->first.reg_no, grp_id);

          if (!is_unaccomp && apis_control)
          {
            list<pair<string, string> > msgs;
            GetAPISLogMsgs(CheckIn::TAPISItem(), aPax->second.apis, msgs);
            for(list<pair<string, string> >::const_iterator m=msgs.begin(); m!=msgs.end(); ++m)
            {
              LEvntPrms params;
              aPax->second.getPaxName(params);
              reqInfo->LocaleToLog(m->first, params << PrmSmpl<string>("params", m->second),
                                               ASTRA::evtPax, point_id, aPax->first.reg_no, grp_id);
            };
          };

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
            std::string lexema_id;
            LEvntPrms params;
            aPax->second.getPaxName(params);
            if (reqInfo->client_type!=ctTerm && aPax->second.refuse==refuseAgentError)
              lexema_id = "EVT.UNREGISTRATION_CANCEL";
            else {
              lexema_id = "EVT.UNREGISTRATION_REFUSE";
              params << PrmElem<std::string>("refuse", etRefusalType, aPax->second.refuse, efmtNameLong);
            }
            reqInfo->LocaleToLog(lexema_id, params, ASTRA::evtPax, point_id, aPax->first.reg_no, grp_id);
            changed=true;
          }
          else
          {
            //ранее были разрегистрированы
            if (aPax->second.refuse!=bPax->second.refuse)
            {
                std::string lexema_id;
                LEvntPrms params;
                aPax->second.getPaxName(params);
                if (reqInfo->client_type!=ctTerm && aPax->second.refuse==refuseAgentError)
                  lexema_id = "EVT.REGISTRATION_CANCEL";
                else {
                  lexema_id = "EVT.REGISTRATION_REFUSE";
                  params << PrmElem<std::string>("refuse", etRefusalType, aPax->second.refuse, efmtNameLong);
                }
                reqInfo->LocaleToLog(lexema_id, params, ASTRA::evtPax, point_id, aPax->first.reg_no, grp_id);
              changed=true;
            };
          };
        }
        else
        {
          LEvntPrms params;
          aPax->second.getPaxName(params);
          reqInfo->LocaleToLog("EVT.PASSENGER_DELETED", params, ASTRA::evtPax, point_id, aPax->first.reg_no, grp_id);
          changed=true;
        };
      };
    }
    else
    {
      if (bPax==grpInfoBefore.pax.end()) continue;
      //пассажир удален
      LEvntPrms params;
      bPax->second.getPaxName(params);
      reqInfo->LocaleToLog("EVT.PASSENGER_DELETED", params, ASTRA::evtPax, point_id, bPax->first.reg_no, grp_id);
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
      string lexema_id;
      LEvntPrms params;
      PrmEnum prmenum("bag", "");
      (aPax!=grpInfoAfter.pax.end()?aPax->second.getPaxName(params):bPax->second.getPaxName(params));

      if (bagStrAfter.empty()) {
        lexema_id = "EVT.LUGGAGE_DELETED";
        bPax->second.getBag(prmenum);
      }
      else if (bagStrBefore.empty()) {
        lexema_id = "EVT.LUGGAGE_ADDED";
        aPax->second.getBag(prmenum);
      }
      else if (!bagStrAfter.empty() && !bagStrBefore.empty()) {
        lexema_id = "EVT.LUGGAGE_MODIFIED";
        aPax->second.getBag(prmenum);
      }
      params << prmenum;
      reqInfo->LocaleToLog(lexema_id, params, ASTRA::evtPax, point_id,
                           aPax!=grpInfoAfter.pax.end()?aPax->first.reg_no:bPax->first.reg_no, grp_id);
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

      PrmEnum prmenum("bag", ", ");
      for(map< int/*id*/, TBagToLogInfo>::const_iterator j=i->second.begin();
                                                         j!=i->second.end(); ++j)
      {
        ostringstream msg;
        if (j->second.bag_type!=ASTRA::NoExists)
          msg << setw(2) << setfill('0') << j->second.bag_type << ":";
        msg << j->second.amount << "/" << j->second.weight;
        prmenum.prms << PrmSmpl<std::string>("", msg.str());
      };

      if (pass==0)
      {
        if (i->first) //pr_cabin
          reqInfo->LocaleToLog("EVT.REMOVE_CABIN_LUGGAGE_MAN_ENTERED_WEIGHT", LEvntPrms() << prmenum,
                                         ASTRA::evtPax, point_id, ASTRA::NoExists, grp_id);
        else
          reqInfo->LocaleToLog("EVT.REMOVE_LUGGAGE_MAN_ENTERED_WEIGHT", LEvntPrms() << prmenum,
                                         ASTRA::evtPax, point_id, ASTRA::NoExists, grp_id);
      }
      else
      {
        if (i->first) //pr_cabin
          reqInfo->LocaleToLog("EVT.ENTER_CABIN_LUGGAGE_MAN_ENTERED_WEIGHT", LEvntPrms() << prmenum,
                                         ASTRA::evtPax, point_id, ASTRA::NoExists, grp_id);
        else
          reqInfo->LocaleToLog("EVT.ENTER_LUGGAGE_MAN_ENTERED_WEIGHT", LEvntPrms() << prmenum,
                                         ASTRA::evtPax, point_id, ASTRA::NoExists, grp_id);
      };
    };
  };

  for(int pass=0; pass<2; pass++)
  {
    multiset<CheckIn::TPaidBagEMDItem, TPaidEMDToLogComparator> emd;
    if (pass==0)
      set_difference(grpInfoBefore.emd.begin(), grpInfoBefore.emd.end(),
                     grpInfoAfter.emd.begin(), grpInfoAfter.emd.end(),
                     inserter(emd, emd.end()),
                     TPaidEMDToLogComparator());
    else
      set_difference(grpInfoAfter.emd.begin(), grpInfoAfter.emd.end(),
                     grpInfoBefore.emd.begin(), grpInfoBefore.emd.end(),
                     inserter(emd, emd.end()),
                     TPaidEMDToLogComparator());
    if (!emd.empty())
    {
      PrmEnum prmenum("emd", "");
      for(multiset<CheckIn::TPaidBagEMDItem, TPaidEMDToLogComparator>::const_iterator e=emd.begin(); e!=emd.end(); ++e)
      {
        std::ostringstream msg;
        if (e!=emd.begin()) prmenum.prms << PrmSmpl<string>("", ", ");
        if (e->bag_type!=ASTRA::NoExists)
          msg << setw(2) << setfill('0') << e->bag_type << ":";
        msg << e->emd_no;
        if (e->emd_coupon!=ASTRA::NoExists)
          msg << "/" << e->emd_coupon;
        prmenum.prms << PrmSmpl<string>("", msg.str());
        if (e->weight!=ASTRA::NoExists)
          prmenum.prms << PrmSmpl<string>("", ":") << PrmSmpl<int>("", e->weight) << PrmLexema("", "EVT.KG");
      };
      LEvntPrms params;
      params << prmenum;
      reqInfo->LocaleToLog(pass==0?"EVT.EMD_DELETED":"EVT.EMD_ADDED",
                           params, ASTRA::evtPay, point_id, ASTRA::NoExists, grp_id);
    };
  };

  if (allGrpAgentError) return;

  if (grpInfoBefore.paid==grpInfoAfter.paid &&
      grpInfoBefore.excess==grpInfoAfter.excess) return;

  LEvntPrms lprms;
  lprms << PrmSmpl<int>("weight", grpInfoAfter.excess);

  if (!grpInfoAfter.paid.empty())
  {
    PrmEnum prmenum("bag", ", ");
    map< int/*bag_type*/, TPaidToLogInfo>::const_iterator p=grpInfoAfter.paid.begin();
    for(;p!=grpInfoAfter.paid.end();++p)
    {
      ostringstream msg;
      if (p->second.bag_type!=-1) msg << setw(2) << setfill('0') << p->second.bag_type << ":";
      msg << p->second.bag_amount << "/" << p->second.bag_weight << "/" << p->second.paid_weight;
      prmenum.prms << PrmSmpl<std::string>("", msg.str());
    };
    lprms << prmenum;
  }
  else lprms << PrmBool("bag", false);

  reqInfo->LocaleToLog("EVT.LUGGAGE.PAID_WEIGHT", lprms, ASTRA::evtPax, point_id, ASTRA::NoExists, grp_id);
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
        reqInfo->LocaleToLog("EVT.SET_LUGGAGE_AUTO_WEIGHTING_CONTROL",
                             LEvntPrms() << PrmSmpl<std::string>("desk", reqInfo->desk.code), ASTRA::evtFlt, point_id);
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
      reqInfo->LocaleToLog("EVT.CANCEL_LUGGAGE_AUTO_WEIGHTING_CONTROL",
                           LEvntPrms() << PrmSmpl<std::string>("desk", reqInfo->desk.code), ASTRA::evtFlt, point_id);
  };

  return auto_weighing;
};

bool GetAPISControl(int point_id)
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT apis_control FROM trip_sets WHERE point_id=:point_id";
  Qry.CreateVariable("point_id", otInteger, point_id);
  Qry.Execute();
  if (Qry.Eof) return false;
  return Qry.FieldAsInteger("apis_control")!=0;
};
