#include "edi_tlg.h"
#include "edilib/edi_func_cpp.h"
#include "edilib/edi_types.h"
#include "edilib/edi_astra_msg_types.h"
#include "edi_msg.h"
#include "etick/lang.h"
#include "etick_change_status.h"
#include "tlg.h"
#include "basic.h"
#include "exceptions.h"
#include "oralib.h"
#include "xml_unit.h"
#include "astra_locale.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "stl_utils.h"
#include "astra_context.h"
#include "jxtlib/cont_tools.h"
#include "jxtlib/xml_stuff.h"
#include "serverlib/cursctl.h"
#include "serverlib/ocilocal.h"
#include "serverlib/ehelpsig.h"
#include "serverlib/date_cast.h"
#include "serverlib/posthooks.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include "serverlib/slogger.h"

using namespace BASIC;
using namespace edilib;
using namespace Ticketing;
using namespace Ticketing::ChangeStatus;
using namespace AstraLocale;

static std::string edi_addr,edi_own_addr;

bool get_et_addr_set( string airline, int flt_no, pair<string,string> &addrs )
{
  addrs.first.clear();
  addrs.second.clear();
  TQuery Qry(&OraSession);
  Qry.SQLText=
    "SELECT edi_addr,edi_own_addr, "
    "       DECODE(airline,NULL,0,2)+ "
    "       DECODE(flt_no,NULL,0,1) AS priority "
    "FROM et_addr_set "
    "WHERE airline=:airline AND "
    "      (flt_no IS NULL OR flt_no=:flt_no) "
    "ORDER BY priority DESC";
  Qry.CreateVariable("airline",otString,airline);
  if (flt_no!=-1)
    Qry.CreateVariable("flt_no",otInteger,flt_no);
  else
    Qry.CreateVariable("flt_no",otInteger,FNull);
  Qry.Execute();
  if (!Qry.Eof)
  {
    addrs.first=Qry.FieldAsString("edi_addr");
    addrs.second=Qry.FieldAsString("edi_own_addr");
    return true;
  };
  return false;
};

void set_edi_addrs( const pair<string,string> &addrs )
{
  edi_addr=addrs.first;
  edi_own_addr=addrs.second;
};

std::string get_edi_addr()
{
  return edi_addr;
};

std::string get_edi_own_addr()
{
  return edi_own_addr;
};

std::string get_canon_name(std::string edi_addr)
{
  TQuery Qry(&OraSession);
  Qry.SQLText=
    "SELECT canon_name FROM edi_addrs WHERE addr=:addr";
  Qry.CreateVariable("addr",otString,edi_addr);
  Qry.Execute();
  if (Qry.Eof||Qry.FieldIsNULL("canon_name"))
    return ETS_CANON_NAME();
  return Qry.FieldAsString("canon_name");
};


const std::string EdiMess::Display = "131";
const std::string EdiMess::ChangeStat = "142";
static std::string last_session_ref;

std::string get_last_session_ref()
{
  return last_session_ref;
};

// static edi_loaded_char_sets edi_chrset[]=
// {
//     {"IATA", "\x3A\x2B,\x3F \x27" /* :+,? ' */},
//     {"IATB", "\x1F\x1D,\x3F\x1D\x1C" /*Пурга какая-то!*/},
//     {"SIRE", "\x3A\x2B,\x3F \"\n"},
//     {"UNOA", "\x3A\x2B.\x3F '\n" /* :+.? ' */},
// };

struct lsTKCREQ {
};
void lsTKCREC_destruct(void *data)
{

}

static EDI_MSG_TYPE edi_msg_proc[]=
{
#include "edilib/astra_msg_types.etp"
};
static int edi_proc_sz = sizeof(edi_msg_proc)/sizeof(edi_msg_proc[0]);


int FuncAfterEdiParseErr(int parse_ret, void *udata, int *err)
{
    if(parse_ret==EDI_MES_STRUCT_ERR)
    {
        //SendEdiTlgErrCONTRL(get_tlg_context()->rot_num, 0, *err);
    } else {
        *err=1;
    }

    return parse_ret;
}

int FuncBeforeEdiProc(edi_mes_head *pHead, void *udata, int *err)
{
    edi_udata * data = ((edi_udata *)udata);
    int ret=0;
    try{
        dynamic_cast<EdiSessRdData &>(*data->sessData()).UpdateEdiSession();
        ProgTrace(TRACE1,"Check edifact session - Ok");
        /* ВСЕ ХОРОШО, ПРОДОЛЖАЕМ ... */
        last_session_ref = pHead->our_ref;
    }
    catch(edilib::EdiExcept &e)
    {
        WriteLog(STDLOG, e.what());
        ret-=100;
        ProgTrace(TRACE2,"Read EDIFACT message / update EDIFACT session - failed");
    }
    return ret;
}

int FuncAfterEdiProc(edi_mes_head *pHead, void *udata, int *err)
{
    int ret=0;
    edi_udata * data = ((edi_udata *)udata);
    if(pHead->msg_type_req == RESPONSE){
        /*Если обрабатываем ответ*/
        try{
            data->sessData()->ediSession()->CommitEdiSession();
            
            if (data->sessData()->ediSession()->pult()=="SYSTEM")
              registerHookAfter(sendCmdTlgSndStepByStep);
        }
        catch (edilib::Exception &x){
            *err=ret=-110;
//             Utils::AfterSoftError();
//             SendEdiTlgErrCONTRL(get_tlg_context()->rot_num, 0, *err);
        }
        catch(...){
            ProgError(STDLOG, "UnknERR exception!");
        }
    }
    return ret;
}

int FuncAfterEdiProcErr(edi_mes_head *pHead, int ret, void *udata, int *err)
{
#if 0
    tst();
    if(ret < 0){
        Utils::AfterSoftError();
        if(*err == 0) {
            *err = 1;
        }
        SendEdiTlgErrCONTRL(get_tlg_context()->rot_num, 0, *err);
        ret -= 120;
    }
    return ret;
#endif
    return 0;
}

int FuncBeforeEdiSend(edi_mes_head *pHead, void *udata, int *err)
{

    tst();
    return 0;
}

int FuncAfterEdiSendErr(edi_mes_head *pHead, int ret, void *udata, int *err)
{
    tst();
    if(*err == 0) {
        *err = 1;//EDI_PROC_ERR;
    }
    ret -= 140;
    return ret;
}

int FuncAfterEdiSend(edi_mes_head *pHead, void *udata, int *err)
{
    int ret=0;
    edi_udata *ed=(edi_udata *) udata;

    try {
        std::string tlg = edilib::WriteEdiMessage(GetEdiMesStructW());
        last_session_ref = pHead->our_ref;

        // Создает запись в БД
        ed->sessData()->ediSession()->CommitEdiSession();
        DeleteMesOutgoing();

        ProgTrace(TRACE1,"tlg out: %s", tlg.c_str());
        TTlgQueuePriority queuePriority=qpOutA;
        if (ed->sessData()->ediSession()->pult()=="SYSTEM")
          queuePriority=qpOutAStepByStep;
        sendTlg(get_canon_name(edi_addr).c_str(),
                OWN_CANON_NAME(),
                queuePriority,
                20, tlg, ASTRA::NoExists, ASTRA::NoExists);
        registerHookAfter(sendCmdTlgSnd);
    }
    catch (std::exception &x){
        ProgError(STDLOG, "%s", x.what());
        *err=1;//PROG_ERR;
    }
    catch(...)
    {
        ProgError(STDLOG, "PROG ERR");
        *err=1;
    }

    if (*err){
        DeleteMesOutgoing();
        if(!ret) ret=-9;
    }
    return ret;
}

void confirm_notify_levb(const int edi_sess_id)
{
  ProgTrace(TRACE2,"confirm_notify_levb: called with edi_sess_id=%d",edi_sess_id);

  string hex_msg_id;
  if (AstraContext::GetContext("EDI_HELP_INTMSGID", edi_sess_id, hex_msg_id)==ASTRA::NoExists)
    return; //контекст не существует, значит и подвешивания запроса не было
  AstraContext::ClearContext("EDI_HELP_INTMSGID", edi_sess_id);
  if (hex_msg_id.empty())
    throw EXCEPTIONS::Exception("confirm_notify_levb: context EDI_HELP_INTMSGID empty for edi_sess_id=%d", edi_sess_id);
  ProgTrace(TRACE2,"confirm_notify_levb: edi_sess_id=%d, intmsgid=%s", edi_sess_id, hex_msg_id.c_str());

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  DELETE FROM edi_help WHERE date1<SYSDATE-1/1440; "
    "  DELETE FROM edi_help WHERE intmsgid=HEXTORAW(:id) AND rownum<2 "
    "    RETURNING address,text,1 INTO :address,:text,:exists; "
    "  SELECT COUNT(*) INTO :remained FROM edi_help "
    "    WHERE intmsgid=HEXTORAW(:id) AND rownum<2; "
    "END;";
  Qry.CreateVariable("id",otString,hex_msg_id);
  Qry.CreateVariable("address",otString,FNull);
  Qry.CreateVariable("text",otString,FNull);
  Qry.CreateVariable("exists",otInteger,0);
  Qry.CreateVariable("remained",otInteger,FNull);
  Qry.Execute();
  if (Qry.GetVariableAsInteger("exists")==0)
    throw EXCEPTIONS::Exception("confirm_notify_levb: nothing in edi_help for intmsgid=%s", hex_msg_id.c_str());
  if (Qry.GetVariableAsInteger("remained")==0)
  {
    string txt=Qry.GetVariableAsString("text");
    string str_msg_id;
    if (!HexToString(hex_msg_id,str_msg_id) || str_msg_id.size()!=sizeof(int)*3)
      throw EXCEPTIONS::Exception("confirm_notify_levb: wrong intmsgid=%s", hex_msg_id.c_str());
    ProgTrace(TRACE2,"confirm_notify_levb: prepare signal %s",txt.c_str());
    sethAfter(EdiHelpSignal((const int*)str_msg_id.c_str(),
                            Qry.GetVariableAsString("address"),
                            txt.c_str()));
  }
  else
  {
    ProgTrace(TRACE2,"confirm_notify_levb: more records in edi_help for intmsgid=%s", hex_msg_id.c_str());
  };
};

void ParseTKCRESdisplay(edi_mes_head *pHead, edi_udata &udata, edi_common_data *data);
void ProcTKCRESdisplay(edi_mes_head *pHead, edi_udata &udata, edi_common_data *data);
void CreateTKCREQdisplay(edi_mes_head *pHead, edi_udata &udata, edi_common_data *data);

void ParseTKCRESchange_status(edi_mes_head *pHead, edi_udata &udata, edi_common_data *data);
void ProcTKCRESchange_status(edi_mes_head *pHead, edi_udata &udata, edi_common_data *data);
void CreateTKCREQchange_status(edi_mes_head *pHead, edi_udata &udata, edi_common_data *data);

message_funcs_type message_TKCREQ[] =
{
    {EdiMess::Display.c_str(), ParseTKCRESdisplay,
            ProcTKCRESdisplay,
            CreateTKCREQdisplay,
            "Ticket display"},
    {EdiMess::ChangeStat.c_str(), ParseTKCRESchange_status,
            ProcTKCRESchange_status,
            CreateTKCREQchange_status,
            "Ticket change of status"},
};

message_funcs_str message_funcs[] =
{
    {TKCREQ, "Ticketing", message_TKCREQ, sizeof(message_TKCREQ)/sizeof(message_TKCREQ[0])},
    {TKCRES, "Ticketing", message_TKCREQ, sizeof(message_TKCREQ)/sizeof(message_TKCREQ[0])},
};

int init_edifact()
{
    InitEdiLogger(ProgError,WriteLog,ProgTrace);

    if(CreateTemplateMessagesCur(OciCpp::mainSession().getLd()/*  LD*/,NULL)){
        return -1;
    }
    if(InitEdiTypes(edi_msg_proc, edi_proc_sz)){
        ProgError(STDLOG,"InitEdiTypes failed");
        return -2;
    }

    SetEdiTempServiceFunc(FuncAfterEdiParseErr,
                          FuncBeforeEdiProc,
                          FuncAfterEdiProc,
                          FuncAfterEdiProcErr,
                          FuncBeforeEdiSend,
                          FuncAfterEdiSend,
                          FuncAfterEdiSendErr);

    /*if(InitEdiCharSet(edi_chrset, sizeof(edi_chrset)/sizeof(edi_chrset[0]))){
        ProgError(STDLOG,"InitEdiCharSet() failed");
        return -3;
    }*/
    edilib::EdiSessLib::Instance()->
            setCallBacks(new edilib::EdiSessCallBack());

    EdiMesFuncs::init_messages(message_funcs,
                               sizeof(message_funcs)/sizeof(message_funcs[0]));
    return 0;
}

// Обработка EDIFACT
void proc_edifact(const std::string &tlg)
{
    edi_udata_rd udata(new AstraEdiSessRD(), tlg);
    int err=0, ret;

    try{
        edi_mes_head edih;
        memset(&edih,0, sizeof(edih));
        udata.sessDataRd()->setMesHead(edih);

        ProgTrace(TRACE2, "Edifact Handle");
        ret = FullObrEdiMessage(tlg.c_str(),&edih,&udata,&err);
    }
    catch(...)
    {
        ProgError(STDLOG,"!!! UnknERR exception !!!");
        ret = -1;
    }
    if(ret){
         throw edi_fatal_except(STDLOG, EdiErr::EDI_PROC_ERR, "Ошибка обработки");
    }
    ProgTrace(TRACE2, "Edifact done.");
}


EdiMesFuncs::messages_map_t *EdiMesFuncs::messages_map;
const message_funcs_type &EdiMesFuncs::GetEdiFunc(
        edi_msg_types_t mes_type, const std::string &msg_code)
{
    messages_map_t::const_iterator iter = get_map()->find(mes_type);
    if(iter == get_map()->end())
    {
        throw edi_fatal_except(STDLOG,EdiErr::EDI_PROC_ERR,
                                "No such message type %d in message function array",
                                mes_type);
    }
    const types_map_t &tmap = iter->second;
    types_map_t::const_iterator iter2 = tmap.find(msg_code);
    if(iter2 == tmap.end()){
        //err
        throw edi_soft_except (STDLOG, EdiErr::EDI_INV_MESSAGE_F,
                                "UnknERR message function for message %d, code=%s",
                                mes_type, msg_code.c_str());
    }
    if(!iter2->second.parse || !iter2->second.proc || !iter2->second.collect_req)
    {
        throw edi_soft_except (STDLOG, EdiErr::EDI_NS_MESSAGE_F,
                                "Message function %s not supported", msg_code.c_str());
    }
    return iter2->second;

}

void SendEdiTlgTKCREQ_ChangeStat(ChngStatData &TChange)
{
    int err=0;
    edi_udata_wr ud(new AstraEdiSessWR(TChange.org().pult()), EdiMess::ChangeStat);

    tst();
    int ret = SendEdiMessage(TKCREQ, ud.sessData()->edih(), &ud, &TChange, &err);
    if(ret)
    {
        throw EXCEPTIONS::Exception("SendEdiMessage for change of status failed");
    }
}

void SendEdiTlgTKCREQ_Disp(TickDisp &TDisp)
{
    int err=0;
    edi_udata_wr ud(new AstraEdiSessWR(TDisp.org().pult()), EdiMess::Display);

    tst();
    int ret = SendEdiMessage(TKCREQ, ud.sessData()->edih(), &ud, &TDisp, &err);
    if(ret)
    {
        throw EXCEPTIONS::Exception("SendEdiMessage DISPLAY failed");
    }
}

xmlDocPtr prepareKickXMLDoc(string iface, int reqCtxtId)
{
  xmlDocPtr kickDoc=CreateXMLDoc(/*"CP866"*/"UTF-8","term");
  if (kickDoc==NULL)
    throw EXCEPTIONS::Exception("prepareKickXMLDoc failed");
  TReqInfo *reqInfo = TReqInfo::Instance();
  xmlNodePtr node=NodeAsNode("/term",kickDoc);
  node=NewTextChild(node,"query");
  SetProp(node,"handle","0");
  SetProp(node,"id",iface);
  SetProp(node,"ver","1");
  SetProp(node,"opr",reqInfo->user.login);
  SetProp(node,"screen",reqInfo->screen.name);
  SetProp(node,"lang",reqInfo->desk.lang);
  if (reqInfo->desk.term_id!=ASTRA::NoExists)
    SetProp(node,"term_id",FloatToString(reqInfo->desk.term_id,0));
  if (reqCtxtId!=ASTRA::NoExists)
    SetProp(NewTextChild(node,"kick"),"req_ctxt_id",reqCtxtId);
  else
    NewTextChild(node,"kick");
  return kickDoc;
};

string prepareKickText(string iface, int reqCtxtId)
{
  string res;
  xmlDocPtr kickDoc=prepareKickXMLDoc(iface, reqCtxtId);
  try
  {
    res=ConvertCodepage(XMLTreeToText(kickDoc),"CP866","UTF-8");
    xmlFreeDoc(kickDoc);
  }
  catch(...)
  {
    xmlFreeDoc(kickDoc);
    throw;
  };
  return res;
};

void makeItin(EDI_REAL_MES_STRUCT *pMes, const Itin &itin, int cpnnum=0)
{
    ostringstream tmp;

    LogTrace(TRACE3) << itin.date1() << " - " << itin.time1() ;

    tmp << (itin.date1().is_special()?"":
            HelpCpp::string_cast(itin.date1(), "%d%m%y"))
            << ":" <<
                    (itin.time1().is_special()?"":
            HelpCpp::string_cast(itin.time1(), "%H%M"))
            << "+" <<
            itin.depPointCode() << "+" <<
            itin.arrPointCode() << "+" <<
            itin.airCode();
    if(!itin.airCodeOper().empty()){
        tmp << ":" << itin.airCodeOper();
    }
    tmp << "+";
    if(itin.flightnum())
        tmp << itin.flightnum();
    else
        tmp << ItinStatus::Open;
    tmp << ":" << itin.classCodeStr();
    if (cpnnum)
        tmp << "++" << cpnnum;

    ProgTrace(TRACE3,"TVL: %s", tmp.str().c_str());
    SetEdiFullSegment(pMes, SegmElement("TVL"), tmp.str());

    return;
}

void CreateTKCREQchange_status(edi_mes_head *pHead, edi_udata &udata,
                               edi_common_data *data)
{
    EDI_REAL_MES_STRUCT *pMes = GetEdiMesStructW();
    ChngStatData &TickD = dynamic_cast<ChngStatData &>(*data);

    // EQN = кол-во билетов в запросе
    ProgTrace(TRACE2,"Tick.size()=%zu", TickD.ltick().size());
    SetEdiFullSegment(pMes, "EQN",0,
                      HelpCpp::string_cast(TickD.ltick().size())+":TD");
    int sg1=0;
    Ticket::Trace(TRACE4,TickD.ltick());
    if(TickD.isGlobItin()){
        makeItin(pMes, TickD.itin());
    }
    for(list<Ticket>::const_iterator i=TickD.ltick().begin();
        i!=TickD.ltick().end();i++,sg1++)
    {
        ProgTrace(TRACE2, "sg1=%d", sg1);
        const Ticket & tick  =  (*i);
        SetEdiSegGr(pMes, 1, sg1);
        SetEdiPointToSegGrW(pMes, 1, sg1);
        SetEdiFullSegment(pMes, "TKT",0, tick.ticknum()+":T");

        PushEdiPointW(pMes);
        int sg2=0;
        for(list<Coupon>::const_iterator j=tick.getCoupon().begin();
            j!= tick.getCoupon().end();j++, sg2++)
        {
            const Coupon &cpn = (*j);
            ProgTrace(TRACE2, "sg2=%d", sg2);
            SetEdiSegGr(pMes, 2, sg2);
            SetEdiPointToSegGrW(pMes, 2, sg2);

            SetEdiFullSegment(pMes, "CPN",0,
                              HelpCpp::string_cast(cpn.couponInfo().num()) + ":" +
                                      cpn.couponInfo().status()->code());

            if(cpn.haveItin()){
                makeItin(pMes, cpn.itin(), cpn.couponInfo().num());
            }
        }
        PopEdiPointW(pMes);
        ResetEdiPointW(pMes);
    }

    //запишем контексты
    AstraContext::SetContext("EDI_SESSION",
                             udata.sessData()->ediSession()->ida().get(),
                             TickD.context());

    if (TickD.req_ctxt_id()!=ASTRA::NoExists)
    {
        AstraContext::SetContext("EDI_HELP_INTMSGID",
                                 udata.sessData()->ediSession()->ida().get(),
                                 get_internal_msgid_hex());

        ServerFramework::getQueryRunner().getEdiHelpManager().
                configForPerespros(STDLOG,prepareKickText("ETStatus",TickD.req_ctxt_id()).c_str(),-1,15);
    };
}

void ChangeStatusToLog(const xmlNodePtr statusNode,
                       const bool repeated,
                       const string &msg_text,
                       const string &screen,
                       const string &user,
                       const string &desk)
{
  TLogMsg msg;
  msg.ev_type=ASTRA::evtPax;

  if (statusNode!=NULL)
  {
    xmlNodePtr node2=statusNode;

    msg.id1=NodeAsIntegerFast("point_id",node2);
    if (GetNodeFast("reg_no",node2)!=NULL)
    {
      msg.id2=NodeAsIntegerFast("reg_no",node2);
      msg.id3=NodeAsIntegerFast("grp_id",node2);
    };
    if (GetNodeFast("pax_full_name",node2)!=NULL &&
        GetNodeFast("pers_type",node2)!=NULL)
    {
      ostringstream pax;
      pax << "Пассажир " << NodeAsStringFast("pax_full_name",node2)
          << " (" << NodeAsStringFast("pers_type",node2) << "). ";
      msg.msg+=pax.str();
    };
  }
  msg.msg+=msg_text;

  if (!repeated) MsgToLog(msg,screen,user,desk);

  if (statusNode!=NULL)
  {
    SetProp(statusNode,"repeated",(int)repeated);
    xmlNodePtr eventNode=NewTextChild(statusNode,"event",msg.msg);
    if (!repeated &&
        msg.ev_time!=ASTRA::NoExists &&
        msg.ev_order!=ASTRA::NoExists)
    {
      SetProp(eventNode,"ev_time",DateTimeToStr(msg.ev_time, ServerFormatDateTimeAsString));
      SetProp(eventNode,"ev_order",msg.ev_order);
    };
  };
};

void ParseTKCRESchange_status(edi_mes_head *pHead, edi_udata &udata,
                              edi_common_data *data)
{
    string ctxt;
    AstraContext::GetContext("EDI_SESSION",
                             udata.sessData()->ediSession()->ida().get(),
                             ctxt);
    ctxt=ConvertCodepage(ctxt,"CP866","UTF-8");


    XMLDoc ediSessCtxt(ctxt);
    xmlNodePtr rootNode=NULL,ticketNode=NULL;
    int req_ctxt_id=ASTRA::NoExists;
    string screen,user,desk;
    if (ediSessCtxt.docPtr()!=NULL)
    {
      //для нормальной работы надо все дерево перевести в CP866:
      xml_decode_nodelist(ediSessCtxt.docPtr()->children);
      rootNode=NodeAsNode("/context",ediSessCtxt.docPtr());
      ticketNode=NodeAsNode("tickets",rootNode)->children;
      if (GetNode("@req_ctxt_id",rootNode))
        req_ctxt_id=NodeAsInteger("@req_ctxt_id",rootNode);
      screen=NodeAsString("@screen",rootNode);
      user=NodeAsString("@user",rootNode);
      desk=NodeAsString("@desk",rootNode);
    };

    TQuery Qry(&OraSession);
    Qry.SQLText=
      "UPDATE trip_sets SET pr_etstatus=0 "
      "WHERE point_id=:point_id AND pr_etstatus<0 ";
    Qry.DeclareVariable("point_id",otInteger);

    int point_id=-1;
    for(xmlNodePtr node=ticketNode;node!=NULL;node=node->next)
    {
      if (point_id!=NodeAsInteger("point_id",node))
      {
        point_id=NodeAsInteger("point_id",node);
        Qry.SetVariable("point_id",point_id);
        Qry.Execute();
        if (Qry.RowsProcessed()>0)
        {
          //запишем в лог
          MsgToLog( "Возвращен режим интерактива с СЭБ", ASTRA::evtFlt, point_id );
        };
      };
    };

    TQuery UpdQry(&OraSession);
    UpdQry.SQLText=
      "BEGIN "
      "  BEGIN "
      "    SELECT error,coupon_status "
      "    INTO :prior_error,:prior_status FROM etickets "
      "    WHERE ticket_no=:ticket_no AND coupon_no=:coupon_no; "
      "  EXCEPTION "
      "    WHEN NO_DATA_FOUND THEN NULL; "
      "  END; "
      "  IF :error IS NULL AND :coupon_status IS NULL THEN "
      "    DELETE FROM etickets "
      "    WHERE ticket_no=:ticket_no AND coupon_no=:coupon_no; "
      "  ELSE "
      "    UPDATE etickets "
      "    SET point_id=:point_id, airp_dep=:airp_dep, airp_arv=:airp_arv, "
      "        coupon_status=:coupon_status, error=:error "
      "    WHERE ticket_no=:ticket_no AND coupon_no=:coupon_no; "
      "    IF SQL%NOTFOUND THEN "
      "      INSERT INTO etickets(ticket_no,coupon_no,point_id,airp_dep,airp_arv,coupon_status,error) "
      "      VALUES(:ticket_no,:coupon_no,:point_id,:airp_dep,:airp_arv,:coupon_status,:error); "
      "    END IF; "
      "  END IF; "
      "END;";
    UpdQry.DeclareVariable("ticket_no",otString);
    UpdQry.DeclareVariable("coupon_no",otInteger);
    UpdQry.DeclareVariable("point_id",otInteger);
    UpdQry.DeclareVariable("airp_dep",otString);
    UpdQry.DeclareVariable("airp_arv",otString);
    UpdQry.DeclareVariable("coupon_status",otString);
    UpdQry.DeclareVariable("error",otString);
    UpdQry.DeclareVariable("prior_status",otString);
    UpdQry.DeclareVariable("prior_error",otString);

    ChngStatAnswer chngStatAns = ChngStatAnswer::readEdiTlg(GetEdiMesStruct());
    chngStatAns.Trace(TRACE2);
    if (chngStatAns.isGlobErr())
    {
        string err,err_locale;
        LexemaData err_lexeme;
        if (chngStatAns.globErr().second.empty())
        {
          err="ОШИБКА " + chngStatAns.globErr().first;
          err_lexeme.lexema_id="MSG.ETICK.ETS_ERROR";
          err_lexeme.lparams << LParam("msg", chngStatAns.globErr().first);
        }
        else
        {
          err=chngStatAns.globErr().second;
          err_lexeme.lexema_id="WRAP.ETS";
          err_lexeme.lparams << LParam("text", chngStatAns.globErr().second);
        };

        for(xmlNodePtr node=ticketNode;node!=NULL;node=node->next)
        {
          xmlNodePtr node2=node->children;

          ostringstream msgh;
          msgh << "Эл. билет "
               << NodeAsStringFast("ticket_no",node2) << "/"
               << NodeAsIntegerFast("coupon_no",node2)
               << ": " << err << ". ";
          xmlNodePtr errNode=NewTextChild(node,"global_error");
          LexemeDataToXML(err_lexeme, errNode);
          if (err.size()>100) err.erase(100);

          UpdQry.SetVariable("ticket_no",NodeAsStringFast("ticket_no",node2));
          UpdQry.SetVariable("coupon_no",NodeAsIntegerFast("coupon_no",node2));
          UpdQry.SetVariable("point_id",NodeAsIntegerFast("point_id",node2));
          UpdQry.SetVariable("airp_dep",NodeAsStringFast("airp_dep",node2));
          UpdQry.SetVariable("airp_arv",NodeAsStringFast("airp_arv",node2));
          UpdQry.SetVariable("coupon_status",FNull);
          UpdQry.SetVariable("error",err);
          UpdQry.SetVariable("prior_status",FNull);
          UpdQry.SetVariable("prior_error",FNull);
          UpdQry.Execute();
          //bool repeated=!UpdQry.VariableIsNULL("prior_error") &&
          //              UpdQry.GetVariableAsString("prior_error")==err;
          ChangeStatusToLog(errNode, /*repeated*/false, msgh.str(), screen, user, desk);
        };
    }
    else
    {
      std::list<Ticket>::const_iterator currTick;

      for(currTick=chngStatAns.ltick().begin();currTick!=chngStatAns.ltick().end();currTick++)
      {
        ostringstream msgh;

        //попробуем проанализировать ошибку уровня билета
        string err=chngStatAns.err2Tick(currTick->ticknum(), 0);
        if (!err.empty())
        {
          ProgTrace(TRACE5,"ticket=%s error=%s",
                           currTick->ticknum().c_str(), err.c_str());
          msgh << "Ошибка при изменении статуса эл. билета "
               << currTick->ticknum()
               << ": " << err << ". ";
               
          LexemaData err_lexeme;
          err_lexeme.lexema_id="MSG.ETICK.CHANGE_STATUS_ERROR";
          err_lexeme.lparams << LParam("ticknum",currTick->ticknum())
                             << LParam("error",err);

          if (ticketNode!=NULL)
          {
            //поищем все билеты
            for(xmlNodePtr node=ticketNode;node!=NULL;node=node->next)
            {
              xmlNodePtr node2=node->children;
              if (NodeAsStringFast("ticket_no",node2)==currTick->ticknum())
              {
                xmlNodePtr errNode=NewTextChild(node,"ticket_error");
                LexemeDataToXML(err_lexeme, errNode);
                if (err.size()>100) err.erase(100);
                //нашли билет
                UpdQry.SetVariable("ticket_no",NodeAsStringFast("ticket_no",node2));
                UpdQry.SetVariable("coupon_no",NodeAsIntegerFast("coupon_no",node2));
                UpdQry.SetVariable("point_id",NodeAsIntegerFast("point_id",node2));
                UpdQry.SetVariable("airp_dep",NodeAsStringFast("airp_dep",node2));
                UpdQry.SetVariable("airp_arv",NodeAsStringFast("airp_arv",node2));
                UpdQry.SetVariable("coupon_status",FNull);
                UpdQry.SetVariable("error",err);
                UpdQry.SetVariable("prior_status",FNull);
                UpdQry.SetVariable("prior_error",FNull);
                UpdQry.Execute();
                //bool repeated=!UpdQry.VariableIsNULL("prior_error") &&
                //              UpdQry.GetVariableAsString("prior_error")==err;
                ChangeStatusToLog(errNode, /*repeated*/false, msgh.str(), screen, user, desk);
              };
            };
          }
          else
          {
            ChangeStatusToLog(NULL, false, msgh.str(), screen, user, desk);
          };
          continue;
        };

        if (currTick->getCoupon().empty()) continue;

        //попробуем проанализировать ошибку уровня купона
        err = chngStatAns.err2Tick(currTick->ticknum(), currTick->getCoupon().front().couponInfo().num());
        if (!err.empty())
        {
          ProgTrace(TRACE5,"ticket=%s coupon=%d error=%s",
                    currTick->ticknum().c_str(),
                    currTick->getCoupon().front().couponInfo().num(),
                    err.c_str());
          msgh << "Ошибка при изменении статуса эл. билета "
               << currTick->ticknum() << "/"
               << currTick->getCoupon().front().couponInfo().num()
               << ": " << err << ". ";
               
          LexemaData err_lexeme;
          err_lexeme.lexema_id="MSG.ETICK.CHANGE_STATUS_ERROR";
          err_lexeme.lparams << LParam("ticknum",currTick->ticknum()+"/"+
                                                 IntToString(currTick->getCoupon().front().couponInfo().num()))
                             << LParam("error",err);
               
          if (ticketNode!=NULL)
          {
            //поищем все билеты
            for(xmlNodePtr node=ticketNode;node!=NULL;node=node->next)
            {
              xmlNodePtr node2=node->children;
              if (NodeAsStringFast("ticket_no",node2)==currTick->ticknum() &&
                  NodeAsIntegerFast("coupon_no",node2)==(int)currTick->getCoupon().front().couponInfo().num())
              {
                xmlNodePtr errNode=NewTextChild(node,"coupon_error");
                LexemeDataToXML(err_lexeme, errNode);
                if (err.size()>100) err.erase(100);
                //нашли билет
                UpdQry.SetVariable("ticket_no",NodeAsStringFast("ticket_no",node2));
                UpdQry.SetVariable("coupon_no",NodeAsIntegerFast("coupon_no",node2));
                UpdQry.SetVariable("point_id",NodeAsIntegerFast("point_id",node2));
                UpdQry.SetVariable("airp_dep",NodeAsStringFast("airp_dep",node2));
                UpdQry.SetVariable("airp_arv",NodeAsStringFast("airp_arv",node2));
                UpdQry.SetVariable("coupon_status",FNull);
                UpdQry.SetVariable("error",err);
                UpdQry.SetVariable("prior_status",FNull);
                UpdQry.SetVariable("prior_error",FNull);
                UpdQry.Execute();
                //bool repeated=!UpdQry.VariableIsNULL("prior_error") &&
                //              UpdQry.GetVariableAsString("prior_error")==err;
                ChangeStatusToLog(errNode, /*repeated*/false, msgh.str(), screen, user, desk);
              };
            };
          }
          else
          {
            ChangeStatusToLog(NULL, false, msgh.str(), screen, user, desk);
          };
          continue;
        };


        CouponStatus status(currTick->getCoupon().front().couponInfo().status());

        ProgTrace(TRACE5,"ticket=%s coupon=%d status=%s",
                         currTick->ticknum().c_str(),
                         currTick->getCoupon().front().couponInfo().num(),
                         status->dispCode());

        msgh << "Изменен статус эл. билета "
             << currTick->ticknum() << "/"
             << currTick->getCoupon().front().couponInfo().num()
             << ": " << status->dispCode() << ". ";

        if (ticketNode!=NULL)
        {
          //поищем все билеты
          for(xmlNodePtr node=ticketNode;node!=NULL;node=node->next)
          {
            xmlNodePtr node2=node->children;
            if (NodeAsStringFast("ticket_no",node2)==currTick->ticknum() &&
                NodeAsIntegerFast("coupon_no",node2)==(int)currTick->getCoupon().front().couponInfo().num())
            {
              //изменим статус в таблице etickets
              //нашли билет
              xmlNodePtr statusNode=NewTextChild(node,"coupon_status",status->dispCode());

              UpdQry.SetVariable("ticket_no",NodeAsStringFast("ticket_no",node2));
              UpdQry.SetVariable("coupon_no",NodeAsIntegerFast("coupon_no",node2));
              UpdQry.SetVariable("point_id",NodeAsIntegerFast("point_id",node2));
              UpdQry.SetVariable("airp_dep",NodeAsStringFast("airp_dep",node2));
              UpdQry.SetVariable("airp_arv",NodeAsStringFast("airp_arv",node2));
              if (status->codeInt()!=CouponStatus::OriginalIssue)
                UpdQry.SetVariable("coupon_status",status->dispCode());
              else
                UpdQry.SetVariable("coupon_status",FNull);
              UpdQry.SetVariable("error",FNull);
              UpdQry.SetVariable("prior_status",FNull);
              UpdQry.SetVariable("prior_error",FNull);
              UpdQry.Execute();
              bool repeated=(UpdQry.VariableIsNULL("prior_status") &&
                             status->codeInt()==CouponStatus::OriginalIssue) ||
                            UpdQry.GetVariableAsString("prior_status")==status->dispCode();
              ChangeStatusToLog(statusNode, repeated, msgh.str(), screen, user, desk);
            };
          };
        }
        else
        {
          ChangeStatusToLog(NULL, false, msgh.str(), screen, user, desk);
        };
      };
    };

    if (ticketNode!=NULL && req_ctxt_id!=ASTRA::NoExists)
    {
      AstraContext::GetContext("EDI_RESPONSE",
                               req_ctxt_id,
                               ctxt);
      XMLDoc ediResCtxt;
      if (!ctxt.empty())
      {
        ctxt=ConvertCodepage(ctxt,"CP866","UTF-8");
        ediResCtxt.set(ctxt);
        if (ediResCtxt.docPtr()!=NULL)
          xml_decode_nodelist(ediResCtxt.docPtr()->children);
      }
      else
      {
        ediResCtxt.set("UTF-8","context");
        if (ediResCtxt.docPtr()!=NULL)
          NewTextChild(NodeAsNode("/context",ediResCtxt.docPtr()),"tickets");
      };
      if (ediResCtxt.docPtr()!=NULL)
      {
        xmlNodePtr node=NodeAsNode("/context/tickets",ediResCtxt.docPtr());
        CopyNodeList(node,ticketNode->parent);
        ctxt=XMLTreeToText(ediResCtxt.docPtr());
        if (!ctxt.empty())
        {
          AstraContext::ClearContext("EDI_RESPONSE",req_ctxt_id);
          AstraContext::SetContext("EDI_RESPONSE",req_ctxt_id,ctxt);
        };
      };
    };
}

void ProcTKCRESchange_status(edi_mes_head *pHead, edi_udata &udata,
                             edi_common_data *data)
{
    AstraContext::ClearContext("EDI_SESSION",
                               udata.sessData()->ediSession()->ida().get());
    confirm_notify_levb(udata.sessData()->ediSession()->ida().get());
}

void CreateTKCREQdisplay(edi_mes_head *pHead, edi_udata &udata, edi_common_data *data)
{
    EDI_REAL_MES_STRUCT *pMes = GetEdiMesStructW();
    TickDisp &TickD = dynamic_cast<TickDisp &>(*data);

    switch(TickD.dispType())
    {
        case TickDispByTickNo:
        {
            TickDispByNum & TickDisp= dynamic_cast<TickDispByNum &>(TickD);

            SetEdiSegGr(pMes, 1);
            SetEdiPointToSegGrW(pMes, 1);
            SetEdiFullSegment(pMes, "TKT",0, TickDisp.tickNum());
        }
        break;
        default:
            throw EdiExcept("Unsupported dispType");
    }

    //запишем контексты
    AstraContext::SetContext("EDI_SESSION",
                             udata.sessData()->ediSession()->ida().get(),
                             TickD.context());

    if (TickD.req_ctxt_id()!=ASTRA::NoExists)
    {
      AstraContext::SetContext("EDI_HELP_INTMSGID",
                                 udata.sessData()->ediSession()->ida().get(),
                                 get_internal_msgid_hex());

      ServerFramework::getQueryRunner().getEdiHelpManager().
              configForPerespros(STDLOG,prepareKickText("ETSearchForm",TickD.req_ctxt_id()).c_str(),-1,15);
    };
}

void ParseTKCRESdisplay(edi_mes_head *pHead, edi_udata &udata, edi_common_data *data)
{
    string ctxt;
    AstraContext::GetContext("EDI_SESSION",
                             udata.sessData()->ediSession()->ida().get(),
                             ctxt);
    ctxt=ConvertCodepage(ctxt,"CP866","UTF-8");


    XMLDoc ediSessCtxt(ctxt);
    if (ediSessCtxt.docPtr()!=NULL)
    {
      //для нормальной работы надо все дерево перевести в CP866:
      xml_decode_nodelist(ediSessCtxt.docPtr()->children);
      xmlNodePtr rootNode=NodeAsNode("/context",ediSessCtxt.docPtr());

      int point_id=NodeAsInteger("point_id",rootNode);
      TQuery Qry(&OraSession);
      Qry.SQLText=
        "UPDATE trip_sets SET pr_etstatus=0 "
        "WHERE point_id=:point_id AND pr_etstatus<0 ";
      Qry.CreateVariable("point_id",otInteger,point_id);
      Qry.Execute();
      if (Qry.RowsProcessed()>0)
      {
        //запишем в лог
        MsgToLog( "Возвращен режим интерактива с СЭБ", ASTRA::evtFlt, point_id );
      };

      if (GetNode("@req_ctxt_id",rootNode))
      {
        int req_ctxt_id=NodeAsInteger("@req_ctxt_id",rootNode);
        edi_udata_rd &udata_rd = dynamic_cast<edi_udata_rd &>(udata);
        AstraContext::SetContext("EDI_RESPONSE",req_ctxt_id,udata_rd.tlgText());
      };
    };
}

void ProcTKCRESdisplay(edi_mes_head *pHead, edi_udata &udata, edi_common_data *data)
{
    AstraContext::ClearContext("EDI_SESSION",
                               (int)udata.sessData()->ediSession()->ida().get());
    confirm_notify_levb(udata.sessData()->ediSession()->ida().get());
}


int ProcEDIREQ (edi_mes_head *pHead, void *udata, void *data, int *err)
{
    ProgTrace(TRACE4, "ProcEDIREQ: tlg_in is %s", pHead->msg_type_str->code);

  try {
    edi_udata *ud = (edi_udata *)udata;
    const message_funcs_type &mes_funcs=
            EdiMesFuncs::GetEdiFunc(pHead->msg_type,
                                    edilib::GetDBFName(GetEdiMesStruct(),
                                    edilib::DataElement(1225),
                                    EdiErr::EDI_PROC_ERR,
                                    edilib::CompElement("C302"),
                                    edilib::SegmElement("MSG")));

    mes_funcs.parse(pHead, *ud, 0);
    mes_funcs.proc(pHead, *ud, 0);
  }
  catch(edi_exception &e)
  {
      ProgTrace(TRACE0,"EdiExcept: %s:%s", e.errCode().c_str(), e.what());
      *err=1;
      return -1;
  }
  catch(std::exception &e)
  {
      ProgError(STDLOG, "std::exception: %s", e.what());
      *err=2;
      return -1;
  }
  catch(...)
  {
      ProgError(STDLOG, "UnknERR error");
      *err=3;
      return -1;
  }
  return 0;
}

int CreateEDIREQ (edi_mes_head *pHead, void *udata, void *data, int *err)
{

    try{
        EDI_REAL_MES_STRUCT *pMes = GetEdiMesStructW();
        edi_udata_wr *ed=(edi_udata_wr *) udata;
        // Заполняет стр-ры: edi_mes_head && EdiSession
        // Из первой создастся стр-ра edifact сообщения
        // Из второй запись в БД
        const message_funcs_type &mes_funcs=
                EdiMesFuncs::GetEdiFunc(pHead->msg_type, ed->msgId());
        ed->sessDataWr()->SetEdiSessMesAttr();
        // Создает стр-ру EDIFACT
        if(::CreateMesByHead(ed->sessData()->edih()))
        {
            throw EdiExcept("Error in CreateMesByHead");
        }

        SetEdiFullSegment(pMes, "MSG",0, ":"+ed->msgId());
        edi_common_data *td = static_cast<edi_common_data *>(data);
        SetEdiFullSegment(pMes, "ORG",0,
                          td->org().airlineCode() + ":" +
                                  td->org().locationCode() +
                                  "+"+td->org().pprNumber()+
                                  "+++"+td->org().type()+
                                  "+::"+td->org().langStr()+
                                  "+"+td->org().pult());
        mes_funcs.collect_req(pHead, *ed, td);
    }
    catch(std::exception &e)
    {
        ProgError(STDLOG, e.what());
        *err = 1;
    }
    catch(...)
    {
        ProgError(STDLOG,"UnknERR exception");
        *err = 1;
    }
    return *err;
}

int ProcCONTRL(edi_mes_head *pHead, void *udata, void *data, int *err)
{
    ProgTrace(TRACE1, "Proc CONTRL");
    return 0;
}

int CreateCONTRL (edi_mes_head *pHead, void *udata, void *data, int *err)
{
    return 0;
}

