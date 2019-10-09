#include "config.h"
#include "edi_tlg.h"
#include "edi_utils.h"
#include "edi_msg.h"
#include "astra_msg.h"
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
#include "postpone_edifact.h"
#include "basetables.h"
#include "tlg_source_edifact.h"
#include "remote_system_context.h"
#include "astra_tick_read_edi.h"
#include "etick.h"
#include "iatci_api.h"
#include "EdiHandlersFactory.h"

#include <etick/lang.h>
#include <etick/exceptions.h>
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
#include <edilib/edi_handler.h>
#include <edilib/EdiSessionTimeOut.h>

#include <boost/scoped_ptr.hpp>

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

using namespace edilib;
using namespace edifact;
using namespace Ticketing;
using namespace Ticketing::ChangeStatus;
using namespace Ticketing::TickMng;
using namespace AstraLocale;
using namespace TlgHandling;
using namespace AstraEdifact;

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

AstraEdiSessWR::AstraEdiSessWR(const std::string &pult,
                               edi_mes_head *mhead,
                               const Ticketing::RemoteSystemContext::SystemContext* sysctxt)
    : Pult(pult),
      EdiHead(mhead),
      H2H(0),
      SysCtxt(sysctxt)
{
    BaseTables::Router rot(sysctxt->routerCanonName());
    ASSERT(rot);
    if(rot->isH2h())
    {
        if(!rot->h2hSrcAddr().empty() && !rot->h2hDestAddr().empty())
        {
            H2H = new hth::HthInfo;
            memset(H2H, 0, sizeof(*H2H));
            H2H->remAddrNum = rot->remAddrNum();
        }
        else
        {
            tst();
            LogError(STDLOG) << "Some of h2h addresses are empty! " <<
                    rot->h2hSrcAddr() << "/" << rot->h2hDestAddr();
        }
    }

}

hth::HthInfo* AstraEdiSessWR::hth()
{
    return H2H;
}

std::string AstraEdiSessWR::sndrHthAddr() const
{
    return BaseTables::Router(sysCont()->routerCanonName())->h2hSrcAddr();
}

std::string AstraEdiSessWR::rcvrHthAddr() const
{
    return BaseTables::Router(sysCont()->routerCanonName())->h2hDestAddr();
}

std::string AstraEdiSessWR::hthTpr() const
{
    if(ediSession()->ourCarf().length() > 4)
        return ediSession()->ourCarf().substr(ediSession()->ourCarf().length() - 4);
    else
        return ediSession()->ourCarf();
}

std::string AstraEdiSessWR::baseOurrefName() const
{
    return "ASTRA";
}

edi_mes_head* AstraEdiSessWR::edih()
{
    return EdiHead;
}

const edi_mes_head* AstraEdiSessWR::edih() const
{
    return EdiHead;
}

std::string AstraEdiSessWR::pult() const
{
    return Pult;
}

std::string AstraEdiSessWR::ourUnbAddr() const
{
    return sysCont()->ourAddrEdifact();
}

std::string AstraEdiSessWR::unbAddr() const
{
    return sysCont()->remoteAddrEdifact();
}

std::string AstraEdiSessWR::ourUnbAddrExt() const
{
    return sysCont()->ourAddrEdifactExt();
}

std::string AstraEdiSessWR::unbAddrExt() const
{
    return sysCont()->remoteAddrEdifactExt();
}

std::string AstraEdiSessWR::ctrlAgency() const
{
    return sysCont()->edifactProfile().ctrlAgency();
}

std::string AstraEdiSessWR::version() const
{
    return sysCont()->edifactProfile().version();
}

std::string AstraEdiSessWR::subVersion() const
{
    return sysCont()->edifactProfile().subVersion();
}

std::string AstraEdiSessWR::syntax() const
{
    return sysCont()->edifactProfile().syntaxName();
}

unsigned AstraEdiSessWR::syntaxVer() const
{
    return sysCont()->edifactProfile().syntaxVer();
}

edilib::EdiSession* AstraEdiSessWR::ediSession()
{
    return &EdiSess;
}

const edilib::EdiSession* AstraEdiSessWR::ediSession() const
{
    return &EdiSess;
}

const Ticketing::RemoteSystemContext::SystemContext* AstraEdiSessWR::sysCont() const
{
    return SysCtxt;
}

//---------------------------------------------------------------------------------------

static std::string last_session_ref;

std::string get_last_session_ref()
{
  return last_session_ref;
}

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
    LogTrace(TRACE3) << __FUNCTION__;
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
    LogTrace(TRACE3) << __FUNCTION__;
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
        /*Если обрабатываем ответ*/
        try{
            data->sessData()->ediSession()->CommitEdiSession();

            //Это важная строчка!
            //После очередного окончания обработки телеграммы в фоновом режиме
            //отправщику посылается уведомление о том, что он может отправить следующую с типом 'step by step'
            if (data->sessData()->ediSession()->pult()==TDesk::system_code)
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
    LogTrace(TRACE3) << __FUNCTION__;
    return 0;
}

int FuncAfterEdiSendErr(edi_mes_head *pHead, int ret, void *udata, int *err)
{
    LogTrace(TRACE3) << __FUNCTION__;
    if(*err == 0) {
        *err = 1;//EDI_PROC_ERR;
    }
    ret -= 140;
    return ret;
}

int FuncAfterEdiSend(edi_mes_head *pHead, void *udata, int *err)
{
    LogTrace(TRACE3) << __FUNCTION__;
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
        if (ed->sessData()->ediSession()->pult()==TDesk::system_code)
          queuePriority=qpOutAStepByStep;  //в фоновом режиме можем себе позволить не торопиться

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

int edifact::init_edifact()
{
    InitEdiLogger(ProgError,WriteLog,ProgTrace);

    if(CreateTemplateMessages()) {
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

    return 0;
}

// Обработка EDIFACT
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
    virtual AstraEdiResponseHandler * makeHandler (EDI_REAL_MES_STRUCT *pMes,
                                                   edi_msg_types_t msgid,
                                                   const hth::HthInfo *hth,
                                                   const edilib::EdiSessRdData *) const;

    static const AstraEdifactResponseHandlerFactory *instance();

    virtual ~AstraEdifactResponseHandlerFactory() {}
};

edilib::EdiRequestHandler *
    AstraEdifactRequestHandlerFactory::makeHandler(EDI_REAL_MES_STRUCT *pMes,
                                                   edi_msg_types_t msgid,
                                                   const hth::HthInfo *hth,
                                                   const edilib::EdiSessRdData *sessionHandler) const
{
    std::string func_code = edilib::GetDBFName(pMes,
                                               edilib::DataElement(1225), "",
                                               edilib::CompElement("C302"),
                                               edilib::SegmElement("MSG"));

    if(msgid == CUSRES || msgid == CUSUMS) {
        func_code = edilib::GetDBFName(pMes,
                                       edilib::DataElement(1001), "",
                                       edilib::CompElement("C002"),
                                       edilib::SegmElement("BGM"));
    }

    LogTrace(TRACE3) << "find request handler for msg " << msgid << " with func_code: " << func_code;

    return EdiReqHandlersFactory(pMes, msgid, func_code, sessionHandler);
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

AstraEdiResponseHandler *
    AstraEdifactResponseHandlerFactory::makeHandler(EDI_REAL_MES_STRUCT *pMes,
                                                    edi_msg_types_t msgid,
                                                    const hth::HthInfo *hth,
                                                    const edilib::EdiSessRdData *sessionHandler) const
{
    std::string func_code = edilib::GetDBFName(pMes,
                                               edilib::DataElement(1225), "",
                                               edilib::CompElement("C302"),
                                               edilib::SegmElement("MSG"));

    // надеемся на то, что RAD всех сегментных групп Sg1(flg) одинаковы
    if(msgid == DCRCKA || msgid == DCRSMF) {
        func_code = edilib::GetDBFName(pMes,
                                       edilib::DataElement(9868), "",
                                       edilib::CompElement(),
                                       edilib::SegmElement("RAD"),
                                       edilib::SegGrElement(1));
    }

    if(msgid == CUSRES) {
        func_code = edilib::GetDBFName(pMes,
                                       edilib::DataElement(1001), "",
                                       edilib::CompElement("C002"),
                                       edilib::SegmElement("BGM"));
    }

    LogTrace(TRACE3) << "find response handler for msg " << msgid << " with func_code: " << func_code;

    return EdiResHandlersFactory(pMes, msgid, func_code, sessionHandler);
}

class AstraEdiHandlerManager : public edilib::EdiHandlerManager
{
    boost::shared_ptr<TlgSourceEdifact> TlgSrc;
    boost::optional<TlgSourceEdifact> AnswerTlg;
    AstraEdiSessRD *AstraSessionHandler;
    bool NeedNoAnswer;
    bool ProcSavePoint;

    void put2queue(const std::exception * e);
    void detectLang();
public:
    AstraEdiHandlerManager(boost::shared_ptr<TlgSourceEdifact> tlg) :
        edilib::EdiHandlerManager(tlg->h2h(), tlg->text().c_str()),
        TlgSrc(tlg), AstraSessionHandler(0), NeedNoAnswer(false), ProcSavePoint(false)
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
        return AstraSessionHandler = new AstraEdiSessRD(hth, Head);
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
    if(sessionHandler()->edih()->msg_type_req == QUERY)
    {
        Ticketing::RemoteSystemContext::SystemContext::initEdifact(
                    sessionHandler()->edih()->from,
                    sessionHandler()->edih()->fromAddExt,
                    sessionHandler()->edih()->to,
                    sessionHandler()->edih()->toAddExt);
    }
    else
    {
        Ticketing::RemoteSystemContext::SystemContext::initEdifactByAnswer(
                    sessionHandler()->edih()->from,
                    sessionHandler()->edih()->fromAddExt,
                    sessionHandler()->edih()->to,
                    sessionHandler()->edih()->toAddExt);
    }

    RemoteSystemContext::SystemContext::Instance(STDLOG).inbTlgInfo().setTlgSrc(TlgSrc->text());
    boost::optional<tlgnum_t> tlgNum = TlgSrc->tlgNum();
    ASSERT(tlgNum);
    RemoteSystemContext::SystemContext::Instance(STDLOG).inbTlgInfo().setTlgNum(*tlgNum);
    if(TlgHandling::isTlgPostponed(*tlgNum)) {
        RemoteSystemContext::SystemContext::Instance(STDLOG).inbTlgInfo().setRepeatedlyProcessed();
    } else {
        // создаём savepoint до обработки только для случаев не postponed
        // Postponed-обработка создаёт свой собственный savepoint
        ASTRA::beforeSoftError();
        ProcSavePoint = true;
    }
}

void AstraEdiHandlerManager::afterProc()
{
    /* Если обрабатываем ответ */
    if(sessionHandler()->edih()->msg_type_req == RESPONSE)
    {
        if(sessionHandler()->ediSession()->mustBeDeleted()) {
            TlgHandling::PostponeEdiHandling::deleteWaiting(sessionHandler()->ediSession()->ida());
        }
        if (sessionHandler()->ediSession()->pult()==TDesk::system_code)
        {
          registerHookAfter(sendCmdTlgSndStepByStep);
          LogTrace(TRACE5) << __FUNCTION__ << ": registerHookAfter(sendCmdTlgSndStepByStep)";
        };
    }
}

void AstraEdiHandlerManager::afterProcFailed(const std::exception *e, const edilib::EdiRequestHandler *rh)
{
    LogTrace(TRACE1) << __FUNCTION__;
    if(ProcSavePoint) {
        ASTRA::afterSoftError();
    }
}

void AstraEdiHandlerManager::afterProcFailed(const std::exception *e, const edilib::EdiResponseHandler *rh)
{
    LogTrace(TRACE1) << __FUNCTION__;
    if(ProcSavePoint) {
        ASTRA::afterSoftError();
    }
//    // оставляем сессию в прежнем состоянии, даем отработать тайм ауту.
//    // ситуацию с ошибкой обработки ответа приравниваем к неответу
    AstraSessionHandler->ediSession()->EdiSessNotUpdate();
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

boost::optional<TlgSourceEdifact> proc_new_edifact(boost::shared_ptr<TlgHandling::TlgSourceEdifact> tlg)
{
    ProgTrace(TRACE4, "proc_new_edifact");
    boost::optional<TlgSourceEdifact> answTlg;
    try
    {
        AstraEdiHandlerManager megaHandler(tlg);
        megaHandler.handle();

        if(megaHandler.answer()) {
            answTlg = TlgSourceEdifact(megaHandler.answer()->MessageText);
            if(megaHandler.answer()->hth)
                answTlg->setH2h(*megaHandler.answer()->hth);
            LogTrace(TRACE1) << "tlgout: " << *answTlg;
        }
    }
    catch(std::exception &e)
    {
        LogError(STDLOG) << "Exception at this point tells something wrong";
        LogError(STDLOG) << e.what();
        sendCmdTlgSndStepByStep();
        throw Ticketing::TickExceptions::tick_fatal_except(STDLOG, AstraErr::EDI_PROC_ERR);
    }

    ProgTrace(TRACE2, "New Edifact done.");

    return answTlg;
}

int ProcEDIREQ (edi_mes_head *pHead, void *udata, void *data, int *err)
{
    ProgTrace(TRACE1, "Proc EDIREQ");
    return 0;
}

int CreateEDIREQ (edi_mes_head *pHead, void *udata, void *data, int *err)
{
    ProgTrace(TRACE1, "Create EDIREQ");
    return 0;
}

int ProcCONTRL(edi_mes_head *pHead, void *udata, void *data, int *err)
{
    ProgTrace(TRACE1, "Proc CONTRL");
    return 0;
}

int CreateCONTRL(edi_mes_head *pHead, void *udata, void *data, int *err)
{
    ProgTrace(TRACE4, "Create CONTRL");
    return 0;
}

int ProcEDIRES (edi_mes_head *pHead, void *udata, void *data, int *err)
{
    ProgTrace(TRACE4, "Proc EDIRES");
    return 0;
}

Ticketing::Pnr readPnr(const Ticketing::EdiPnr& ediPnr)
{
    int ret = ReadEdiMessage(ediPnr.ediText().c_str());
    if(ret == EDI_MES_STRUCT_ERR){
      throw EXCEPTIONS::Exception("Error in message structure: %s",EdiErrGetString());
    } else if( ret == EDI_MES_NOT_FND){
      throw EXCEPTIONS::Exception("No message found in template: %s",EdiErrGetString());
    } else if( ret == EDI_MES_ERR) {
      throw EXCEPTIONS::Exception("Edifact error ");
    }

    try {
      Pnr pnr = PnrRdr::doRead<Pnr>(PnrEdiRead(GetEdiMesStruct(), ediPnr.ediType()));
      Pnr::Trace(TRACE2, pnr);
      return pnr;
    }
    catch(edilib::EdiExcept &e)
    {
      throw EXCEPTIONS::Exception("edilib: %s", e.what());
    }
}

