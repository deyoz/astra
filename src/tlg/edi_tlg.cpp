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
#include "tlg_source_edifact.h"
#include "remote_system_context.h"
#include "astra_tick_read_edi.h"
#include "etick.h"
#include "iatci_api.h"

// response handlers
#include "EmdDispResponseHandler.h"
#include "EmdSysUpdateResponseHandler.h"
#include "EmdCosResponseHandler.h"
#include "EtDispResponseHandler.h"
#include "EtCosResponseHandler.h"

#include "IatciCkiResponseHandler.h"
#include "IatciCkuResponseHandler.h"
#include "IatciCkxResponseHandler.h"
#include "IatciBprResponseHandler.h"
#include "IatciPlfResponseHandler.h"
#include "IatciSmfResponseHandler.h"

// request handlers
#include "IatciCkiRequestHandler.h"
#include "IatciCkuRequestHandler.h"
#include "IatciCkxRequestHandler.h"
#include "IatciBprRequestHandler.h"
#include "IatciPlfRequestHandler.h"
#include "IatciSmfRequestHandler.h"

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
#include <edilib/edi_astra_msg_types.h>
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

std::string NewAstraEdiSessWR::ourUnbAddr() const { return sysCont()->ourAddrEdifact(); }
std::string NewAstraEdiSessWR::unbAddr() const { return sysCont()->remoteAddrEdifact(); }

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
        if (ed->sessData()->ediSession()->pult()=="SYSTEM")
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


class AstraEdiSessCallBack : public edilib::EdiSessCallBack
{
  public:
    virtual void deleteDbEdiSession(edilib::EdiSessionId_t edisess)
    {
      edilib::EdiSessCallBack::deleteDbEdiSession(edisess);
      edilib::EdiSessionTimeOut::deleteDb(edisess); //чистим также таблицу edisession_timeouts
    }
};

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
    edilib::EdiSessLib::Instance()->
            setCallBacks(new AstraEdiSessCallBack());

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
    // здесь будут регистрироваться обработчики edifact-запросов
    __DECLARE_HANDLER__(IatciCkiRequestHandler,     DCQCKI, "");
    __DECLARE_HANDLER__(IatciCkuRequestHandler,     DCQCKU, "");
    __DECLARE_HANDLER__(IatciCkxRequestHandler,     DCQCKX, "");
    __DECLARE_HANDLER__(IatciBprRequestHandler,     DCQBPR, "");
    __DECLARE_HANDLER__(IatciPlfRequestHandler,     DCQPLF, "");
    __DECLARE_HANDLER__(IatciSmfRequestHandler,     DCQSMF, "");
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

    LogTrace(TRACE3) << "find response handler for msg " << msgid << " with func_code: " << func_code;
    // здесь будут регистрироваться обработчики edifact-ответов
    __DECLARE_HANDLER__(EtDispResponseHandler,              TKCRES, "131");
    __DECLARE_HANDLER__(EtCosResponseHandler,               TKCRES, "142");
    __DECLARE_HANDLER__(EmdDispResponseHandler,             TKCRES, "791");
    __DECLARE_HANDLER__(EmdCosResponseHandler,              TKCRES, "793");
    __DECLARE_HANDLER__(EmdSysUpdateResponseHandler,        TKCRES, "794");
    // IATCI
    __DECLARE_HANDLER__(IatciCkiResponseHandler,            DCRCKA, "I");
    __DECLARE_HANDLER__(IatciCkuResponseHandler,            DCRCKA, "U");
    __DECLARE_HANDLER__(IatciCkxResponseHandler,            DCRCKA, "X");
    __DECLARE_HANDLER__(IatciBprResponseHandler,            DCRCKA, "B");
    __DECLARE_HANDLER__(IatciPlfResponseHandler,            DCRCKA, "P");
    __DECLARE_HANDLER__(IatciSmfResponseHandler,            DCRSMF, "S");

    return 0;
}

class AstraEdiHandlerManager : public edilib::EdiHandlerManager
{
    boost::shared_ptr<TlgSourceEdifact> TlgSrc;
    boost::optional<TlgSourceEdifact> AnswerTlg;
    bool NeedNoAnswer;
    bool ProcSavePoint;

    void put2queue(const std::exception * e);
    void detectLang();
public:
    AstraEdiHandlerManager(boost::shared_ptr<TlgSourceEdifact> tlg) :
        edilib::EdiHandlerManager(tlg->h2h(), tlg->text().c_str()),
        TlgSrc(tlg), NeedNoAnswer(false), ProcSavePoint(false)
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
        return new AstraEdiSessRD(hth, Head);
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
//        Ticketing::RemoteSystemContext::SystemContext::initEdifact(
//                    sessionHandler()->edih()->from,
//                    sessionHandler()->edih()->to);
    }
    else
    {
//        Ticketing::RemoteSystemContext::SystemContext::initEdifactByAnswer(
//                    sessionHandler()->edih()->from,
//                    sessionHandler()->edih()->to);
    }
    RemoteSystemContext::SystemContext::initDummyContext();
    RemoteSystemContext::SystemContext::Instance(STDLOG).inbTlgInfo().setTlgSrc(TlgSrc->text());
    boost::optional<tlgnum_t> tlgNum = TlgSrc->tlgNum();
    ASSERT(tlgNum);
    RemoteSystemContext::SystemContext::Instance(STDLOG).inbTlgInfo().setTlgNum(*tlgNum);
    if(TlgHandling::isTlgPostponed(*tlgNum)) {
        RemoteSystemContext::SystemContext::Instance(STDLOG).inbTlgInfo().setRepeatedlyProcessed();
    }

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
//    /* ВСЕ ХОРОШО, ПРОДОЛЖАЕМ ... */
//    Utils::BeforeSoftError();
//    ProcSavePoint = true;
}

void AstraEdiHandlerManager::afterProc()
{
    /* Если обрабатываем ответ */
    if(sessionHandler()->edih()->msg_type_req == RESPONSE)
    {
        if(sessionHandler()->ediSession()->mustBeDeleted()) {
            TlgHandling::PostponeEdiHandling::deleteWaiting(sessionHandler()->ediSession()->ida());
        }
    }
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
//    // оставляем сессию в прежнем состоянии, даем отработать тайм ауту.
//    // ситуация с ошибкой обработки ответа приравниваем к неответу
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
