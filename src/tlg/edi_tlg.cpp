#include "config.h"
#include "edi_tlg.h"
#include "edi_utils.h"
#include "edi_msg.h"
#include "etick_change_status.h"
#include "tlg.h"
#include "exceptions.h"
#include "oralib.h"
#include "xml_unit.h"
#include "astra_locale.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "stl_utils.h"
#include "astra_context.h"
#include "emd_disp_request.h"
#include "tlg_source_edifact.h"
#include "remote_system_context.h"
#include "astra_tick_read_edi.h"
#include "etick.h"
#include "date_time.h"

// handlers
#include "EmdDispResponseHandler.h"
#include "EmdSysUpdateResponseHandler.h"
#include "EmdCosResponseHandler.h"

#include <etick/lang.h>
#include <jxtlib/cont_tools.h>
#include <jxtlib/xml_stuff.h>
#include <serverlib/cursctl.h>
#include <serverlib/ocilocal.h>
#include <serverlib/ehelpsig.h>
#include <serverlib/dates_io.h>
#include <serverlib/posthooks.h>
#include <serverlib/testmode.h>
#include <serverlib/EdiHelpManager.h>
#include <edilib/edi_func_cpp.h>
#include <edilib/edi_types.h>
#include <edilib/edi_astra_msg_types.h>
#include <edilib/edi_handler.h>
#include <edilib/EdiSessionTimeOut.h>

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

using namespace edilib;
using namespace Ticketing;
using namespace Ticketing::ChangeStatus;
using namespace Ticketing::TickMng;
using namespace AstraLocale;
using namespace TlgHandling;
using namespace AstraEdifact;
using namespace BASIC::date_time;

static std::string edi_addr,edi_own_addr;

void set_edi_addrs( const pair<string,string> &addrs )
{
  edi_addr=addrs.first;
  edi_own_addr=addrs.second;
}

std::string get_edi_addr()
{
  return edi_addr;
}

std::string get_edi_own_addr()
{
  return edi_own_addr;
}

//---------------------------------------------------------------------------------------

std::string NewAstraEdiSessWR::ourUnbAddr() const { return sysCont()->ourAddrEdifact(); }
std::string NewAstraEdiSessWR::unbAddr() const { return sysCont()->remoteAddrEdifact(); }

//---------------------------------------------------------------------------------------

const std::string EdiMess::Display = "131";
const std::string EdiMess::ChangeStat = "142";
static std::string last_session_ref;

std::string get_last_session_ref()
{
  return last_session_ref;
}

// static edi_loaded_char_sets edi_chrset[]=
// {
//     {"IATA", "\x3A\x2B,\x3F \x27" /* :+,? ' */},
//     {"IATB", "\x1F\x1D,\x3F\x1D\x1C" /*��࣠ �����-�!*/},
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
        /* ��� ������, ���������� ... */
        last_session_ref = pHead->our_ref;
    }
    catch(edilib::EdiExcept &e)
    {
        LogWarning(STDLOG) << e.what();
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
            data->sessData()->ediSession()->CommitEdiSession();

            //�� ������ ���窠!
            //��᫥ ��।���� ����砭�� ��ࠡ�⪨ ⥫��ࠬ�� � 䮭���� ०���
            //��ࠢ騪� ���뫠���� 㢥�������� � ⮬, �� �� ����� ��ࠢ��� ᫥������ � ⨯�� 'step by step'
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

        // ������� ������ � ��
        ed->sessData()->ediSession()->CommitEdiSession();
        DeleteMesOutgoing();

        ProgTrace(TRACE1,"tlg out: %s", tlg.c_str());
        TTlgQueuePriority queuePriority=qpOutA;
        if (ed->sessData()->ediSession()->pult()=="SYSTEM")
          queuePriority=qpOutAStepByStep;  //� 䮭���� ०��� ����� ᥡ� ��������� �� �ய�����

        sendTlg(get_canon_name(edi_addr).c_str(),
                OWN_CANON_NAME(),
                queuePriority,
                20, tlg, ASTRA::NoExists, ASTRA::NoExists);
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

class AstraEdiSessCallBack : public edilib::EdiSessCallBack
{
  public:
    virtual void deleteDbEdiSession(edilib::EdiSessionId_t edisess)
    {
      edilib::EdiSessCallBack::deleteDbEdiSession(edisess);
      edilib::EdiSessionTimeOut::deleteDb(edisess); //��⨬ ⠪�� ⠡���� edisession_timeouts
    }
};

int edifact::init_edifact()
{
    InitEdiLogger(ProgError,WriteLog,ProgTrace);

    if(CreateTemplateMessages(get_connect_string(),NULL)) {
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
            setCallBacks(new AstraEdiSessCallBack());

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
        ret = FullObrEdiMessage(tlg.c_str(),udata.sessDataRd()->edih(),&udata,&err);
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

/*************************/
class AstraEdifactRequestHandlerFactory : public edilib::EdiRequestHandlerFactory
{
    static AstraEdifactRequestHandlerFactory *Instance;
public:
    AstraEdifactRequestHandlerFactory() {}

    /**
      * @brief interface method to get edifact request handler
      * @overload
    */
    virtual edilib::EdiRequestHandler * makeHandler (EDI_REAL_MES_STRUCT *pMes,
                                                     edi_msg_types_t req,
                                                     const hth::HthInfo *hth,
                                                     const edilib::EdiSessRdData *) const;

    static const AstraEdifactRequestHandlerFactory *instance();

    virtual ~AstraEdifactRequestHandlerFactory() {}
};

/// Ets concrete factory
class AstraEdifactResponseHandlerFactory : public edilib::EdiResponseHandlerFactory
{
    static AstraEdifactResponseHandlerFactory *Instance;
public:
    AstraEdifactResponseHandlerFactory()
    {
        LogTrace(TRACE3) << "Enter AstraEdifactResponseHandlerFactory()";
    }
    /**
      * @brief interface method to get edifact request handler
      * @overload
    */
    virtual edilib::EdiResponseHandler * makeHandler (EDI_REAL_MES_STRUCT *pMes,
                                                      edi_msg_types_t msgid,
                                                      const hth::HthInfo *hth,
                                                      const edilib::EdiSessRdData *) const;

    static const AstraEdifactResponseHandlerFactory *instance();

    virtual ~AstraEdifactResponseHandlerFactory() {}
};

#define __DECLARE_HANDLER__(handler, msg__, func_code__) \
        if(msg__ == msgid && (func_code == func_code__ || !*func_code__))\
        {\
            return new handler(pMes, sessionHandler);\
        }

edilib::EdiRequestHandler *
    AstraEdifactRequestHandlerFactory::makeHandler(EDI_REAL_MES_STRUCT *pMes,
                                                   edi_msg_types_t msgid,
                                                   const hth::HthInfo *hth,
                                                   const edilib::EdiSessRdData *sessionHandler) const
{
    const std::string func_code = edilib::GetDBFName(pMes,
                                                     edilib::DataElement(1225), "",
                                                     edilib::CompElement("C302"),
                                                     edilib::SegmElement("MSG"));
    // ����� ���� ॣ����஢����� ��ࠡ��稪� edifact-����ᮢ
    return 0;
}

AstraEdifactRequestHandlerFactory *AstraEdifactRequestHandlerFactory::Instance = 0;
const AstraEdifactRequestHandlerFactory *AstraEdifactRequestHandlerFactory::instance()
{
    if(!Instance)
        Instance = new AstraEdifactRequestHandlerFactory();

    return Instance;
}

AstraEdifactResponseHandlerFactory *AstraEdifactResponseHandlerFactory::Instance = 0;
const AstraEdifactResponseHandlerFactory *AstraEdifactResponseHandlerFactory::instance()
{
    if(!Instance)
        Instance = new AstraEdifactResponseHandlerFactory();

    return Instance;
}

edilib::EdiResponseHandler *
    AstraEdifactResponseHandlerFactory::makeHandler(EDI_REAL_MES_STRUCT *pMes,
                                                    edi_msg_types_t msgid,
                                                    const hth::HthInfo *hth,
                                                    const edilib::EdiSessRdData *sessionHandler) const
{
    tst();
    const std::string func_code = edilib::GetDBFName(pMes,
                                                     edilib::DataElement(1225), "",
                                                     edilib::CompElement("C302"),
                                                     edilib::SegmElement("MSG"));

    LogTrace(TRACE3) << "find response handler for msg with func_code: " << func_code;
    // ����� ���� ॣ����஢����� ��ࠡ��稪� edifact-�⢥⮢
    __DECLARE_HANDLER__(EmdDispResponseHandler,             TKCRES, "791");
    __DECLARE_HANDLER__(EmdCosResponseHandler,              TKCRES, "793");
    __DECLARE_HANDLER__(EmdSysUpdateResponseHandler,        TKCRES, "794");

    return 0;
}

class AstraEdiHandlerManager : public edilib::EdiHandlerManager
{
    TlgSourceEdifact *TlgSrc;
    boost::optional<TlgSourceEdifact> AnswerTlg;
    AstraEdiSessRD *astraSessionHandler;
    bool NeedNoAnswer;
    bool ProcSavePoint;

    void put2queue(const std::exception * e);
    void detectLang();
public:
    AstraEdiHandlerManager(TlgSourceEdifact *tlg) :
        edilib::EdiHandlerManager(tlg->h2h(), tlg->text().c_str()),
        TlgSrc(tlg), astraSessionHandler(0), NeedNoAnswer(false), ProcSavePoint(false)
    {
    }

    /// @brief abstract factory to get edifact request handler
    /// @overload
    virtual const edilib::EdiRequestHandlerFactory *requestHandlerFactory() {
        return AstraEdifactRequestHandlerFactory::instance();
    }

    /// @brief abstract factory to get edifact response handler
    /// @overload
    virtual const edilib::EdiResponseHandlerFactory *responseHandlerFactory() {
        return AstraEdifactResponseHandlerFactory::instance();
    }

    /**
     * a new copy of application specific edifact session handler
    */
    virtual edilib::EdiSessRdData * makeSessionHandler(const hth::HthInfo *hth,
                                                       const edi_mes_head &Head)
    {
        return astraSessionHandler = new AstraEdiSessRD(hth, Head);
    }

    bool needNoAnswer() const { return NeedNoAnswer; }

    // callbacks
    void beforeProc();
    void afterProc();
    void afterProcFailed(const std::exception * e, const edilib::EdiRequestHandler * rh);
    void afterProcFailed(const std::exception * e, const edilib::EdiResponseHandler * rh);
    void afterMakeAnswer();
    void afterMakeAnswerFailed(const std::exception *, const edilib::EdiRequestHandler *rh);
};

void AstraEdiHandlerManager::put2queue(const std::exception *e)
{
//#define __CAST(type, var, c) const type *var = dynamic_cast<const type *>(c)
//#define UNUSED(x) (void)(x)

//    if(__CAST(RemoteSystemContext::UnknownSystAddrs, exc, e)) {
//        UNUSED(exc);
//        TlgHandling::ErrTlgQueue::putTlg2ErrQueue(qn_unknown_system_address,
//                                                  TlgSrc->tlgNum(),
//                                                  Tr(EtsErr::UNKN_SYSTEM_ADDR));
//    }
//    else if(__CAST(edilib::Exception, exc, e)) {
//        std::string errCode = (exc->hasErrCode() ? exc->errCode() : EtsErr::EDI_PROC_ERR);
//        TlgHandling::ErrTlgQueue::putTlg2ErrQueue(qn_edifact_soft_err,
//                                                  TlgSrc->tlgNum(),
//                                                  Tr(errCode));
//    }
//    else if(__CAST(TickExceptions::tick_soft_except, exc, e)) {
//        TlgHandling::ErrTlgQueue::putTlg2ErrQueue(qn_edifact_soft_err,
//                                                  TlgSrc->tlgNum(),
//                                                  Tr(exc->errCode()));
//    }
//    else if(__CAST(std::exception, exc, e)) {
//        UNUSED(exc);
//        TlgHandling::ErrTlgQueue::putTlg2ErrQueue(qn_edifact_fatal_err,
//                                                  TlgSrc->tlgNum(),
//                                                  Tr(EtsErr:EtsEdifactResponseHandlerFactory:EDI_PROC_ERR));
//    }
//#undef __CAST
//#undef UNUSED
}

void AstraEdiHandlerManager::detectLang()
{
    // Get language
//    const char *lng = GetDBFName(GetEdiMesStruct(),
//                                 edilib::DataElement(3453), "",
//                                 edilib::CompElement("C354"),
//                                 edilib::SegmElement("ORG"));

//    Language Lang = RemoteSystemContext::SystemContext::Instance(STDLOG).lang();

//    LogTrace(TRACE3) << "Lang = " << lng;

//    if(!strcmp(lng, "RU"))
//    {
//        Lang = RUSSIAN;
//    }
//    else if(!strcmp(lng, "EN"))
//    {
//        Lang = ENGLISH;
//    }
//    else
//    {
//        LogTrace(TRACE3) << "use default lang for remote system: " <<
//                            (Lang == RUSSIAN?"RUS":"ENG");
//    }

    //RemoteSystemContext::SystemContext::Instance(STDLOG).setLang(Lang);
}

void AstraEdiHandlerManager::beforeProc()
{
//    if(sessionHandler()->edih()->msg_type_req == QUERY)
//    {
//        Ticketing::RemoteSystemContext::SystemContext::initEdifact(
//                    sessionHandler()->edih()->from,
//                    sessionHandler()->edih()->to);
//    }
//    else
//    {
//        Ticketing::RemoteSystemContext::SystemContext::initEdifactByAnswer(
//                    sessionHandler()->edih()->from,
//                    sessionHandler()->edih()->to);
//    }
    RemoteSystemContext::SystemContext::initDummyContext();
    RemoteSystemContext::SystemContext::Instance(STDLOG).inbTlgInfo().setTlgSrc(TlgSrc->text());

//    etsSessionHandler->setSysCont(&RemoteSystemContext::SystemContext::Instance(STDLOG));

//    LogTrace(TRACE1) << "Request/Response from " <<
//              RemoteSystemContext::SystemContext::Instance(STDLOG).description();


//    detectLang();

//    if(sessionHandler()->edih()->msg_type_req == RESPONSE
//            && sessionHandler()->ediSession()->msgId().num.valid())
//    {
//        TlgSource::setAnswerTlgNumDb(sessionHandler()->ediSession()->msgId(),
//                                     TlgSrc->tlgNum());
//    }

//    ProgTrace(TRACE1,"Check edifact session - Ok");
//    /* ��� ������, ���������� ... */
//    Utils::BeforeSoftError();
//    ProcSavePoint = true;
}

void AstraEdiHandlerManager::afterProc()
{
//    if(sessionHandler()->edih()->msg_type_req == RESPONSE) {
//        /* �᫨ ��ࠡ��뢠�� �⢥� */
//        if(sessionHandler()->ediSession()->mustBeDeleted()) {
//            PostponedTlgHandling::deleteWaiting(sessionHandler()->ediSession()->ida());
//        }
//    }
}

void AstraEdiHandlerManager::afterProcFailed(const std::exception *e, const edilib::EdiRequestHandler *rh)
{
//    if(dynamic_cast<const PostponedTlgExcpt *>(e)) {
//        NeedNoAnswer = true;
//        TlgSrc->setPostponed();
//    } else {
//        if(ProcSavePoint)
//            Utils::AfterSoftError();

//        const EtsEdifactRequestHandler *reqHandler = dynamic_cast<const EtsEdifactRequestHandler *>(rh);

//        if(!reqHandler->needPutErrToQueue()) {
//            // no error to queue
//            return;
//        }

//        this->put2queue(e);
//    }
}

void AstraEdiHandlerManager::afterProcFailed(const std::exception *e, const edilib::EdiResponseHandler *rh)
{
//    LogTrace(TRACE1) << "EtsEdiHandlerManager::afterProcFailed";
//    if(ProcSavePoint)
//        Utils::AfterSoftError();
//    // ��⠢�塞 ���� � �०��� ���ﭨ�, ���� ��ࠡ���� ⠩� ����.
//    // ����� � �訡��� ��ࠡ�⪨ �⢥� ��ࠢ������ � ���⢥��
//    etsSessionHandler->ediSession()->EdiSessNotUpdate();

//    this->put2queue(e);
}

void AstraEdiHandlerManager::afterMakeAnswer()
{
}

void AstraEdiHandlerManager::afterMakeAnswerFailed(const std::exception *e, const edilib::EdiRequestHandler *rh)
{
//    LogError(STDLOG) << e->what();
//    TlgHandling::ErrTlgQueue::putTlg2ErrQueue(qn_edifact_fatal_err,
//                                              TlgSrc->tlgNum(),
//                                              Tr(EtsErr::EDI_PROC_ERR));
}

void proc_new_edifact(const std::string& tlg)
{
    try
    {
        AstraEdiHandlerManager megaHandler(new TlgSourceEdifact(tlg));
        megaHandler.handle();

//        if(!megaHandler.needNoAnswer() && megaHandler.answer()) {
//            TlgSourceEdifact *tlg;
//            tlg_info.setTlg(tlg = new TlgSourceEdifact(megaHandler.answer()->MessageText));
//            if(megaHandler.answer()->hth) {
//                tlg->setH2h(*megaHandler.answer()->hth);
//            }
//            tlg->setHandMade(tlgSrc()->handMade());
//            LogTrace(TRACE1) << "tlgout: " << *tlg;
//        }
    }
    catch(std::exception &e)
    {
        LogError(STDLOG) << "Exception at this point tells something wrong";
        LogError(STDLOG) << e.what();
        // TODO throw normal typified exception here
        //throw EdifactTlgHandleErr(Tr(EtsErr::EDI_PROC_ERR, ENGLISH));
        throw EXCEPTIONS::Exception("EDI_PROC_ERR");
    }

    ProgTrace(TRACE2, "New Edifact done.");
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

    // EQN = ���-�� ����⮢ � �����
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

    //����襬 ���⥪���
    AstraContext::SetContext("EDI_SESSION",
                             udata.sessData()->ediSession()->ida().get(),
                             TickD.context());

    if (!TickD.kickInfo().background_mode())
    {
        ServerFramework::getQueryRunner().getEdiHelpManager().
                configForPerespros(STDLOG,
                                   make_xml_kick(TickD.kickInfo()).c_str(),
                                   udata.sessData()->ediSession()->ida().get(),
                                   15);
    };
}

void ChangeStatusToLog(const xmlNodePtr statusNode,
                       const bool repeated,
                       const string lexema_id,
                       LEvntPrms& params,
                       const string &screen,
                       const string &user,
                       const string &desk)
{
  TLogLocale locale;
  locale.ev_type=ASTRA::evtPax;

  if (statusNode!=NULL)
  {
    xmlNodePtr node2=statusNode;

    locale.id1=NodeAsIntegerFast("point_id",node2);
    if (GetNodeFast("reg_no",node2)!=NULL)
    {
      locale.id2=NodeAsIntegerFast("reg_no",node2);
      locale.id3=NodeAsIntegerFast("grp_id",node2);
    };
    if (GetNodeFast("pax_full_name",node2)!=NULL &&
        GetNodeFast("pers_type",node2)!=NULL)
    {
      locale.lexema_id = "EVT.PASSENGER_DATA";
      locale.prms << PrmSmpl<string>("pax_name", NodeAsStringFast("pax_full_name",node2))
                  << PrmElem<string>("pers_type", etPersType, NodeAsStringFast("pers_type",node2));
      PrmLexema lexema("param", lexema_id);
      lexema.prms = params;
      locale.prms << lexema;
    }
  }
  else
  {
    locale.lexema_id = lexema_id;
    locale.prms = params;
  }
  if (!repeated) locale.toDB(screen, user, desk);

  if (statusNode!=NULL)
  {
    SetProp(statusNode,"repeated",(int)repeated);
    xmlNodePtr eventNode = NewTextChild(statusNode, "event");
    LocaleToXML (eventNode, locale.lexema_id, locale.prms);
    if (!repeated &&
        locale.ev_time!=ASTRA::NoExists &&
        locale.ev_order!=ASTRA::NoExists)
    {
      SetProp(eventNode,"ev_time",DateTimeToStr(locale.ev_time, ServerFormatDateTimeAsString));
      SetProp(eventNode,"ev_order",locale.ev_order);
    };
  };
}

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
      //��� ��ଠ�쭮� ࠡ��� ���� �� ��ॢ� ��ॢ��� � CP866:
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
          //����襬 � ���
          TReqInfo::Instance()->LocaleToLog("EVT.RETURNED_INTERACTIVE_WITH_ETC", ASTRA::evtFlt, point_id);
        };
      };
    };

    ChngStatAnswer chngStatAns = ChngStatAnswer::readEdiTlg(GetEdiMesStruct());
    chngStatAns.Trace(TRACE2);
    if (chngStatAns.isGlobErr())
    {
        string err,err_locale;
        LexemaData err_lexeme;
        if (chngStatAns.globErr().second.empty())
        {
          err="������ " + chngStatAns.globErr().first;
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
          LEvntPrms params;
          params << PrmSmpl<std::string>("ticket_no", NodeAsStringFast("ticket_no",node2))
                 << PrmSmpl<int>("coupon_no", NodeAsIntegerFast("coupon_no",node2))
                 << PrmSmpl<std::string>("err", err);
          xmlNodePtr errNode=NewTextChild(node,"global_error");
          LexemeDataToXML(err_lexeme, errNode);

          TETickItem ETickItem;
          ETickItem.et_no=NodeAsStringFast("ticket_no",node2);
          ETickItem.et_coupon=NodeAsIntegerFast("coupon_no",node2);
          ETickItem.point_id=NodeAsIntegerFast("point_id",node2);
          ETickItem.airp_dep=NodeAsStringFast("airp_dep",node2);
          ETickItem.airp_arv=NodeAsStringFast("airp_arv",node2);
          ETickItem.change_status_error=err;
          ETickItem.toDB(TETickItem::ChangeOfStatus);

          ChangeStatusToLog(errNode, false, "EVT.ETICKET", params, screen, user, desk);
        };
    }
    else
    {
      std::list<Ticket>::const_iterator currTick;

      for(currTick=chngStatAns.ltick().begin();currTick!=chngStatAns.ltick().end();currTick++)
      {
        //���஡㥬 �஠������஢��� �訡�� �஢�� �����
        string err=chngStatAns.err2Tick(currTick->ticknum(), 0);
        if (!err.empty())
        {
          ProgTrace(TRACE5,"ticket=%s error=%s",
                           currTick->ticknum().c_str(), err.c_str());
          LEvntPrms params;
          params << PrmSmpl<std::string>("tick_num", currTick->ticknum())
                 << PrmSmpl<std::string>("err", err);

          LexemaData err_lexeme;
          err_lexeme.lexema_id="MSG.ETICK.CHANGE_STATUS_ERROR";
          err_lexeme.lparams << LParam("ticknum",currTick->ticknum())
                             << LParam("error",err);

          if (ticketNode!=NULL)
          {
            //���饬 �� ������
            for(xmlNodePtr node=ticketNode;node!=NULL;node=node->next)
            {
              xmlNodePtr node2=node->children;
              if (NodeAsStringFast("ticket_no",node2)==currTick->ticknum())
              {
                xmlNodePtr errNode=NewTextChild(node,"ticket_error");
                LexemeDataToXML(err_lexeme, errNode);
                //��諨 �����
                TETickItem ETickItem;
                ETickItem.et_no=NodeAsStringFast("ticket_no",node2);
                ETickItem.et_coupon=NodeAsIntegerFast("coupon_no",node2);
                ETickItem.point_id=NodeAsIntegerFast("point_id",node2);
                ETickItem.airp_dep=NodeAsStringFast("airp_dep",node2);
                ETickItem.airp_arv=NodeAsStringFast("airp_arv",node2);
                ETickItem.change_status_error=err;
                ETickItem.toDB(TETickItem::ChangeOfStatus);

                ChangeStatusToLog(errNode, false, "EVT.ETICKET_CHANGE_STATUS_MISTAKE", params, screen, user, desk);
              };
            };
          }
          else
          {
            ChangeStatusToLog(NULL, false, "EVT.ETICKET_CHANGE_STATUS_MISTAKE", params, screen, user, desk);
          };
          continue;
        };

        if (currTick->getCoupon().empty()) continue;

        //���஡㥬 �஠������஢��� �訡�� �஢�� �㯮��
        err = chngStatAns.err2Tick(currTick->ticknum(), currTick->getCoupon().front().couponInfo().num());
        if (!err.empty())
        {
          ProgTrace(TRACE5,"ticket=%s coupon=%d error=%s",
                    currTick->ticknum().c_str(),
                    currTick->getCoupon().front().couponInfo().num(),
                    err.c_str());
          LEvntPrms params;
          ostringstream msgh;
          msgh << currTick->ticknum() << "/" << currTick->getCoupon().front().couponInfo().num();
          params << PrmSmpl<std::string>("tick_num", msgh.str())
                 << PrmSmpl<std::string>("err", err);

          LexemaData err_lexeme;
          err_lexeme.lexema_id="MSG.ETICK.CHANGE_STATUS_ERROR";
          err_lexeme.lparams << LParam("ticknum",currTick->ticknum()+"/"+
                                                 IntToString(currTick->getCoupon().front().couponInfo().num()))
                             << LParam("error",err);

          if (ticketNode!=NULL)
          {
            //���饬 �� ������
            for(xmlNodePtr node=ticketNode;node!=NULL;node=node->next)
            {
              xmlNodePtr node2=node->children;
              if (NodeAsStringFast("ticket_no",node2)==currTick->ticknum() &&
                  NodeAsIntegerFast("coupon_no",node2)==(int)currTick->getCoupon().front().couponInfo().num())
              {
                xmlNodePtr errNode=NewTextChild(node,"coupon_error");
                LexemeDataToXML(err_lexeme, errNode);
                //��諨 �����
                TETickItem ETickItem;
                ETickItem.et_no=NodeAsStringFast("ticket_no",node2);
                ETickItem.et_coupon=NodeAsIntegerFast("coupon_no",node2);
                ETickItem.point_id=NodeAsIntegerFast("point_id",node2);
                ETickItem.airp_dep=NodeAsStringFast("airp_dep",node2);
                ETickItem.airp_arv=NodeAsStringFast("airp_arv",node2);
                ETickItem.change_status_error=err;
                ETickItem.toDB(TETickItem::ChangeOfStatus);

                ChangeStatusToLog(errNode, false, "EVT.ETICKET_CHANGE_STATUS_MISTAKE", params, screen, user, desk);
              };
            };
          }
          else
          {
            ChangeStatusToLog(NULL, false, "EVT.ETICKET_CHANGE_STATUS_MISTAKE", params, screen, user, desk);
          };
          continue;
        };


        CouponStatus status(currTick->getCoupon().front().couponInfo().status());

        ProgTrace(TRACE5,"ticket=%s coupon=%d status=%s",
                         currTick->ticknum().c_str(),
                         currTick->getCoupon().front().couponInfo().num(),
                         status->dispCode());

        LEvntPrms params;
        params << PrmSmpl<std::string>("ticket_no", currTick->ticknum())
               << PrmSmpl<int>("coupon_no", currTick->getCoupon().front().couponInfo().num())
               << PrmSmpl<std::string>("disp_code", status->dispCode());

        if (ticketNode!=NULL)
        {
          //���饬 �� ������
          for(xmlNodePtr node=ticketNode;node!=NULL;node=node->next)
          {
            xmlNodePtr node2=node->children;
            if (NodeAsStringFast("ticket_no",node2)==currTick->ticknum() &&
                NodeAsIntegerFast("coupon_no",node2)==(int)currTick->getCoupon().front().couponInfo().num())
            {
              //������� ����� � ⠡��� etickets
              //��諨 �����
              xmlNodePtr statusNode=NewTextChild(node,"coupon_status",status->dispCode());

              TETickItem ETickItem;
              ETickItem.et_no=NodeAsStringFast("ticket_no",node2);
              ETickItem.et_coupon=NodeAsIntegerFast("coupon_no",node2);
              ETickItem.point_id=NodeAsIntegerFast("point_id",node2);
              ETickItem.airp_dep=NodeAsStringFast("airp_dep",node2);
              ETickItem.airp_arv=NodeAsStringFast("airp_arv",node2);
              if (status->codeInt()!=CouponStatus::OriginalIssue)
                ETickItem.status=CouponStatus(status);
              else
                ETickItem.status=CouponStatus(CouponStatus::Unavailable);

              TETickItem priorETickItem;
              priorETickItem.fromDB(ETickItem.et_no, ETickItem.et_coupon, TETickItem::ChangeOfStatus, true);

              ETickItem.toDB(TETickItem::ChangeOfStatus);

              bool repeated=(priorETickItem.status==ETickItem.status);
              ChangeStatusToLog(statusNode, repeated, "EVT.ETICKET_CHANGE_STATUS", params, screen, user, desk);
            };
          };
        }
        else
        {
            ChangeStatusToLog(NULL, false, "EVT.ETICKET_CHANGE_STATUS", params, screen, user, desk);
        };
      };
    };

    addToEdiResponseCtxt(req_ctxt_id, ticketNode, "tickets");
}

void ProcTKCRESchange_status(edi_mes_head *pHead, edi_udata &udata,
                             edi_common_data *data)
{
    AstraContext::ClearContext("EDI_SESSION",
                               udata.sessData()->ediSession()->ida().get());
    confirm_notify_levb(udata.sessData()->ediSession()->ida().get(), false);
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

    //����襬 ���⥪���
    AstraContext::SetContext("EDI_SESSION",
                             udata.sessData()->ediSession()->ida().get(),
                             TickD.context());

    if (!TickD.kickInfo().background_mode())
    {
      ServerFramework::getQueryRunner().getEdiHelpManager().
              configForPerespros(STDLOG,
                                 make_xml_kick(TickD.kickInfo()).c_str(),
                                 udata.sessData()->ediSession()->ida().get(),
                                 15);
    };
}

void ParseTKCRESdisplay(edi_mes_head *pHead, edi_udata &udata, edi_common_data *data)
{
  edi_udata_rd &udata_rd = dynamic_cast<edi_udata_rd &>(udata);
  boost::optional<Ticketing::Pnr> pnr;
  try
  {
    if (!pnr) pnr = readPnr(udata_rd.tlgText());
    ETDisplayToDB(pnr.get());
  }
  catch(std::exception &e)
  {
    ProgTrace(TRACE5, ">>>> %s: %s", __FUNCTION__, e.what());
  }
  catch(...)
  {
    ProgTrace(TRACE5, ">>>> %s: unknown error", __FUNCTION__);
  }
  //��� 䮭����� ०��� ��� � �� �த������ � ���⥪�⮬
  if (udata.sessData()->ediSession()->pult()=="SYSTEM") return;

  string ctxt;
  AstraContext::GetContext("EDI_SESSION",
                           udata.sessData()->ediSession()->ida().get(),
                           ctxt);
  ctxt=ConvertCodepage(ctxt,"CP866","UTF-8");

  XMLDoc ediSessCtxt(ctxt);
  if (ediSessCtxt.docPtr()!=NULL)
  {
    //��� ��ଠ�쭮� ࠡ��� ���� �� ��ॢ� ��ॢ��� � CP866:
    xml_decode_nodelist(ediSessCtxt.docPtr()->children);
    xmlNodePtr rootNode=NodeAsNode("/context",ediSessCtxt.docPtr());
    int req_ctxt_id=NodeAsInteger("@req_ctxt_id",rootNode);
    int point_id=NodeAsInteger("point_id",rootNode);

    TQuery Qry(&OraSession);
    Qry.SQLText=
        "UPDATE trip_sets SET pr_etstatus=0 "
        "WHERE point_id=:point_id AND pr_etstatus<0 ";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.Execute();
    if (Qry.RowsProcessed()>0)
    {
      //����襬 � ���
      TReqInfo::Instance()->LocaleToLog("EVT.RETURNED_INTERACTIVE_WITH_ETC", ASTRA::evtFlt, point_id );
    };

    string purpose=NodeAsString("@purpose",rootNode);

    if (purpose=="EMDDisplay" ||
        purpose=="EMDRefresh")
    {
      XMLDoc ediResCtxt("context");
      if (ediResCtxt.docPtr()==NULL)
        throw EXCEPTIONS::Exception("%s: CreateXMLDoc failed", __FUNCTION__);
      xmlNodePtr ediResCtxtNode=NodeAsNode("/context",ediResCtxt.docPtr());

      try
      {
        if (!pnr) pnr = readPnr(udata_rd.tlgText());

        edifact::KickInfo kickInfo;
        kickInfo.fromXML(rootNode);
        kickInfo.parentSessId=udata.sessData()->ediSession()->ida().get();
        OrigOfRequest org("");
        string airline;
        Ticketing::FlightNum_t flNum;
        OrigOfRequest::fromXML(rootNode, org, airline, flNum);
        set<Ticketing::TicketNum_t> emds;
        for(list<Ticket>::const_iterator i=pnr.get().ltick().begin(); i!=pnr.get().ltick().end(); ++i)
          if (i->actCode() == TickStatAction::inConnectionWith)
          {
            emds.insert(i->connectedDocNum());
            //ProgTrace(TRACE5, "%s: %s", __FUNCTION__, i->connectedDocNum().get().c_str());
          };
        Ticket::Trace(TRACE5, pnr.get().ltick());
        SearchEMDsByTickNo(emds, kickInfo, org, airline, flNum);
      }
      catch(AstraLocale::UserException &e)
      {
        //��� ��⠫��� �訡�� ������
        ProcEdiError(e.getLexemaData(), ediResCtxtNode, true);
      };
      addToEdiResponseCtxt(req_ctxt_id, ediResCtxtNode->children, "");
    };

    if (purpose=="ETDisplay")
    {
      AstraContext::SetContext("EDI_RESPONSE",req_ctxt_id,udata_rd.tlgText());
    };
  };
}

void ProcTKCRESdisplay(edi_mes_head *pHead, edi_udata &udata, edi_common_data *data)
{
    AstraContext::ClearContext("EDI_SESSION",
                               (int)udata.sessData()->ediSession()->ida().get());
    confirm_notify_levb(udata.sessData()->ediSession()->ida().get(), false);
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
        ed->sessDataWr()->SetEdiSessMesAttr();
        // ������� ���-�� EDIFACT
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
        LogError(STDLOG) << e.what();
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


