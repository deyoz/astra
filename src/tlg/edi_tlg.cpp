#include "edi_tlg.h"
#include "edilib/edi_func_cpp.h"
#include "edilib/edi_types.h"
#include "edilib/edi_astra_msg_types.h"
#include "edi_msg.h"
#include "etick/lang.h"
#include "etick_change_status.h"
#include "tlg.h"
#include "exceptions.h"
#include "oralib.h"
#include "cont_tools.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include "slogger.h"

using namespace edilib;
using namespace edilib::EdiSess;
using namespace Ticketing;
using namespace Ticketing::ChangeStatus;
using namespace Ticketing::CouponStatus;
using namespace jxtlib;
using namespace JxtContext;

static std::string edi_addr,edi_own_addr;

bool set_edi_addrs(string airline,int flt_no)
{
  TQuery Qry(&OraSession);
  Qry.SQLText=
    "SELECT edi_addr,edi_own_addr, "
    "       DECODE(airline,NULL,0,2)+ "
    "       DECODE(flt_no,NULL,0,1) AS priority "
    "FROM edi_addr_set "
    "WHERE airline=:airline AND "
    "      (flt_no IS NULL OR flt_no=:flt_no) "
    "ORDER BY priority DESC";
  Qry.CreateVariable("airline",otString,airline);
  if (flt_no!=-1)
    Qry.CreateVariable("flt_no",otInteger,flt_no);
  else
    Qry.CreateVariable("flt_no",otInteger,FNull);
  Qry.Execute();
  if (Qry.Eof)
  {
    edi_addr="";
    edi_own_addr="";
    return false;
  };
  edi_addr=Qry.FieldAsString("edi_addr");
  edi_own_addr=Qry.FieldAsString("edi_own_addr");
  return true;
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

static edi_loaded_char_sets edi_chrset[]=
{
    {"IATA", "\x3A\x2B,\x3F \x27" /* :+,? ' */},
    {"IATB", "\x1F\x1D,\x3F\x1D\x1C" /*��࣠ �����-�!*/},
    {"SIRE", "\x3A\x2B,\x3F \"\n"}
};

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
        UpdateEdiSession(&dynamic_cast<EdiSessRdData &>(*data->sessData()));
        ProgTrace(TRACE1,"Check edifact session - Ok");
        /* ��� ������, ���������� ... */
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
        /*�᫨ ��ࠡ��뢠�� �⢥�*/
        try{
            CommitEdiSession(data->sessData()->ediSession());
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

        // ������� ������ � ��
        CommitEdiSession(ed->sessData()->ediSession());
        DeleteMesOutgoing();

        ProgTrace(TRACE1,"tlg out: %s", tlg.c_str());
        sendTlg(get_canon_name(edi_addr).c_str(), OWN_CANON_NAME(), true, 20, tlg);
        registerHookAfter(sendCmdTlgSnd);
    }
    catch (edilib::Exception &x){
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

int confirm_notify_levb(const char *pult);

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

    if(CreateTemplateMessagesCur(LD,NULL)){
        return -1;
    }
    if(InitEdiTypes(edi_msg_proc, edi_proc_sz)){
        ProgError(STDLOG,"InitEdiTypes filed");
        return -2;
    }

    SetEdiTempServiceFunc(FuncAfterEdiParseErr,
                          FuncBeforeEdiProc,
                          FuncAfterEdiProc,
                          FuncAfterEdiProcErr,
                          FuncBeforeEdiSend,
                          FuncAfterEdiSend,
                          FuncAfterEdiSendErr);

    if(InitEdiCharSet(edi_chrset, sizeof(edi_chrset)/sizeof(edi_chrset[0]))){
        ProgError(STDLOG,"InitEdiCharSet() failed");
        return -3;
    }
    edilib::EdiSess::EdiSessLib::Instance()->
            setCallBacks(new edilib::EdiSess::EdiSessCallBack());

    EdiMesFuncs::init_messages(message_funcs,
                               sizeof(message_funcs)/sizeof(message_funcs[0]));
    return 0;
}

// ��ࠡ�⪠ EDIFACT
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
         throw edi_fatal_except(STDLOG, EdiErr::EDI_PROC_ERR, "�訡�� ��ࠡ�⪨");
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
        throw EXCEPTIONS::UserException("SendEdiMessage for change of status failed");
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
        throw EXCEPTIONS::UserException("SendEdiMessage DISPLAY failed");
    }
}

string prepareKickText(string iface)
{
    TReqInfo *reqInfo = TReqInfo::Instance();
    JxtCont *sysCont = getJxtContHandler()->sysContext();
 //   const char *iface = sysCont->readC("CUR_IFACE","");
    const char *handle= sysCont->readC("HANDLE","");
    const char *oper  = sysCont->readC("OPR","");

    string text("<?xml version=\"1.0\" encoding=\"CP866\"?>"
            "<term>");
    text = text + "<query handle=\"" + handle + "\" id=\"" +iface+
            "\" ver=\"0\" opr=\""+ oper +"\" screen=\""+reqInfo->screen.name+"\">"
            "<kick></kick>"
            "</query>"
            "</term>";
    return text;
}

void throw2UserLevel(const string &pult, const string &name, const string &text)
{
    JXTLib::Instance()->GetCallbacks()->
            initJxtContext(pult);

    JxtCont *sysCont = getJxtContHandler()->sysContext();
    sysCont->write(name,text);
    registerHookBefore(SaveContextsHook);
}
void saveTlgSource(const string &pult, const string &tlg)
{
    throw2UserLevel(pult, "ETDisplayTlg", tlg);
}

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
    if(!itin.operAirCode().empty()){
        tmp << ":" << itin.operAirCode();
    }
    tmp << "+";
    if(itin.flightnum())
        tmp << itin.flightnum();
    else
        tmp << ItinStatus::itin_status::Open;
    tmp << ":" << itin.classCode();
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

    // EQN = ���-�� ����⮢ � �����
    ProgTrace(TRACE2,"Tick.sizer()=%d", TickD.ltick().size());
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
                                      cpn.couponInfo().status().code());

            if(cpn.haveItin()){
                makeItin(pMes, cpn.itin(), cpn.couponInfo().num());
            }
        }
        PopEdiPointW(pMes);
        ResetEdiPointW(pMes);
    }
    if(TickD.org().pult() != "SYSTEM"){
/*        udata.ediHelp()->
                configForPerespros(prepareKickText().c_str(),
                                   TickD.org().pult().c_str());*/
        ServerFramework::getQueryRunner().getEdiHelpManager().
                configForPerespros(prepareKickText("ETStatus").c_str(),15);
    }
}

void ParseTKCRESchange_status(edi_mes_head *pHead, edi_udata &udata,
                              edi_common_data *data)
{
    ChngStatAnswer chngStatAns = ChngStatAnswer::readEdiTlg(GetEdiMesStruct());
    chngStatAns.Trace(TRACE2);
    if (chngStatAns.isGlobErr())
    {
        throw2UserLevel(udata.sessData()->ediSession()->pult, "ChangeOfStatusError", chngStatAns.globErr().second);
        return;
    }
    std::list<Ticket>::const_iterator currTick;
    TQuery Qry(&OraSession);
    for(currTick=chngStatAns.ltick().begin();currTick!=chngStatAns.ltick().end();currTick++)
    {
      ProgTrace(TRACE5,"ticket=%s error=%s",
                       currTick->ticknum().c_str(),
                       chngStatAns.err2Tick(currTick->ticknum()).c_str());
      if (!chngStatAns.err2Tick(currTick->ticknum()).empty()) {
        continue;
      }
      if (currTick->getCoupon().empty()) continue;

      ProgTrace(TRACE5,"ticket=%s coupon=%d",
                       currTick->ticknum().c_str(),
                       currTick->getCoupon().front().couponInfo().num());
      coupon_status status(currTick->getCoupon().front().couponInfo().status());
      Qry.Clear();
      if (status.codeInt()==Checked ||
          status.codeInt()==Boarded ||
          status.codeInt()==Flown)
      {
        //ᤥ���� update
        Qry.SQLText=
          "UPDATE etickets SET coupon_status=:status "
          "WHERE ticket_no=:ticket_no AND coupon_no=:coupon_no";
        Qry.CreateVariable("status",otString,status.dispCode());
      }
      else
      {
        //㤠���� �� ⠡����
        Qry.SQLText="DELETE FROM etickets WHERE ticket_no=:ticket_no AND coupon_no=:coupon_no";
      };
      Qry.CreateVariable("ticket_no",otString,currTick->ticknum());
      Qry.CreateVariable("coupon_no",otInteger,(int)currTick->getCoupon().front().couponInfo().num());
      Qry.Execute();
    };
}

void ProcTKCRESchange_status(edi_mes_head *pHead, edi_udata &udata,
                             edi_common_data *data)
{
    confirm_notify_levb(udata.sessData()->ediSession()->pult.c_str());
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
//     udata.ediHelp()->
//             configForPerespros(prepareKickText().c_str(),
//                                TickD.org().pult().c_str());
    ServerFramework::getQueryRunner().getEdiHelpManager().
            configForPerespros(prepareKickText("ETSearchForm").c_str(),15);
}

void ParseTKCRESdisplay(edi_mes_head *pHead, edi_udata &udata, edi_common_data *data)
{
}
void ProcTKCRESdisplay(edi_mes_head *pHead, edi_udata &udata, edi_common_data *data)
{
    // ������ ⥫��ࠬ�� � ���⥪��, ��� �裡 � obrzap'��
    // �맮� ������
    confirm_notify_levb(udata.sessData()->ediSession()->pult.c_str());

    edi_udata_rd &udata_rd = dynamic_cast<edi_udata_rd &>(udata);
    saveTlgSource(udata.sessData()->ediSession()->pult, udata_rd.tlgText());
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
        // �������� ���-��: edi_mes_head && EdiSession
        // �� ��ࢮ� ᮧ������ ���-� edifact ᮮ�饭��
        // �� ��ன ������ � ��

        const message_funcs_type &mes_funcs=
                EdiMesFuncs::GetEdiFunc(pHead->msg_type, ed->msgId());

        SetEdiSessMesAttrOnly(ed->sessDataWr());
        // ������� ���-�� EDIFACT
        if(::CreateMesByHead(ed->sessData()->edih()))
        {
            throw EdiExcept("Error in CreateMesByHead");
        }
        tst();

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

