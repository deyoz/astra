#include <string.h>
#include <edilib/edi_handler.h>
#include <edilib/edi_user_func.h>
#include <edilib/edi_except.h>
#include <edilib/edi_func_cpp.h>
#include <edilib/edi_logger.h>
#include <edilib/edi_sess.h>
#include <boost/scoped_ptr.hpp>
#include <edilib/edi_request_handler.h>
#include <edilib/edi_response_handler.h>

#define NICKNAME "ROMAN"
#include <serverlib/slogger.h>

namespace edilib
{

EdiHandlerManager::EdiHandlerManager(const hth::HthInfo *hth, const char *edifact_message)
    :hth(hth),
      edifact_message(edifact_message),
      pMes_(0),
      pTempMes(0),
      MsgType(0),
      SessionHandler(0),
      HasError(false)
{
}

EdiHandlerManager::~EdiHandlerManager()
{
    delete SessionHandler;
}

const EDI_REAL_MES_STRUCT *EdiHandlerManager::pMes() const
{
    return pMes_;
}

const EdiSessRdData *EdiHandlerManager::sessionHandler() const
{
    return SessionHandler;
}

const boost::optional<AnswerData> &EdiHandlerManager::answer() const
{
    return Answer;
}

bool EdiHandlerManager::hasError() const
{
    return HasError;
}

void EdiHandlerManager::onParseFailed(int err, const char *error_str)
{
    switch(err)
    {
    case EDI_MES_STRUCT_ERR:
        throw Exception(std::string("Error in message structure: ") +
                        EdiErrGetString_(pMes_));
    case EDI_MES_NOT_FND:
        throw  Exception(std::string("No message found in template: ") +
                         EdiErrGetString_(pMes_));
    case EDI_MES_ERR:
        throw Exception("Program error in ReadEdiMes()");
    }
    throw Exception("Unknown error in edifact parser");
}

void EdiHandlerManager::parseMessage()
{
    int ret = ReadEdiMessage(edifact_message);
    pMes_ = GetEdiMesStruct();
    if(ret) {
        const char *err_str = 0;
        if(ret == EDI_MES_STRUCT_ERR) {
            // Ошибка синтаксиса
            err_str = EdiErrGetString_(pMes_);
            EdiError(STDLOG, "%s", err_str ? err_str : "ReadEdiMessage: syntax error!");
        } else {
            EdiError(STDLOG,"ReadEdiMessage failed whith code = %d", ret);
        }

        onParseFailed(ret, err_str);

        // если из onParseFailed не вылетел exception, выкинем его сами.
        EdiHandlerManager::onParseFailed(ret, err_str);
    }

    pTempMes = GetEdiTemplateMessages();
}

void EdiHandlerManager::findMessageConfig()
{
    MessageName = GetEdiMes_(pMes_);
    MsgType = GetEdiMsgTypeStrByName_(pTempMes, MessageName.c_str());
    if(!MsgType) {
        throw Exception(std::string("Unable to find message struct for ") +
                        GetEdiMes_(pMes_));
    }
}

void EdiHandlerManager::readMessageHead(edi_mes_head &MsgHead)
{
    if(SetMesHead_(&MsgHead, pMes_, 1)) {
        throw Exception("GetMesAttr failed");
    }
}

void EdiHandlerManager::createAnswerFromReq()
{
    if(MsgType->answer_type == EDINOMES) {
        throw Exception("No answer type specified for request: " + MessageName);
    }

    // Ищем структуру ответного сообщения
    MsgType = GetEdiMsgTypeStrByType_(pTempMes, MsgType->answer_type);
    if(MsgType == NULL) {
        throw Exception("No types defined for answer on " + MessageName);
    }
    updateMsgHead(SessionHandler->edih());

    SessionHandler->CreateAnswerByAttr();

    if(CreateMesByHead(SessionHandler->edih()))
    {
        LogError(STDLOG) << "Failed to create message";
        throw Exception("Failed to create message");
    }
}

void EdiHandlerManager::updateMsgHead(edi_mes_head *MsgHead)
{
    MsgHead->msg_type_req = MsgType->query_type;
    MsgHead->answer_type  = MsgType->answer_type;
    MsgHead->msg_type     = MsgType->type;
    MsgHead->msg_type_str = MsgType;
    strcpy(MsgHead->code,   MsgType->code);
}

void EdiHandlerManager::makeAnswerText(EDI_REAL_MES_STRUCT *pMesW)
{
    Answer = AnswerData();
    Answer->MessageText = WriteEdiMessage(pMesW);
    if(SessionHandler->hth()) {
        Answer->hth = *SessionHandler->hth();
    }
}

void EdiHandlerManager::handleRequest(EdiRequestHandler *reqHandler)
{
    // Алгоритм обработки сообщения

    try {
        beforeProc(); // user overloaded function to call before proc()
        reqHandler->parse();
        reqHandler->handle();
    }
    catch (const std::exception &e) {
        LogWarning(STDLOG) << e.what();
        HasError = true;
        reqHandler->onHandlerError(&e);
        afterProcFailed(&e, reqHandler);
    }

    if(!HasError)
        afterProc(); // if OK

    // make answer structures
    createAnswerFromReq();
    reqHandler->setMesW(GetEdiMesStructW());

    beforeMakeAnswer();

    try {
        reqHandler->makeMSG();
        if(reqHandler->ediErrorCode().empty())
            reqHandler->makeAnAnswer();
        else
            reqHandler->makeAnAnswerErr();

        makeAnswerText(reqHandler->pMesW());

        afterMakeAnswer();
    }
    catch(const std::exception &e) {
        LogWarning(STDLOG) << e.what();
        HasError = true;
        afterMakeAnswerFailed(&e, reqHandler);
    }
}

EdiResponseHandler* EdiHandlerManager::makeResponseHandler()
{
    const EdiResponseHandlerFactory *resHandlerFactory;

    // getting factory
    resHandlerFactory = responseHandlerFactory();

    if(!resHandlerFactory) {
        throw Exception("Fatal: Unable to retrieve EdifactResponseHandlerFactory");
    }
    
    EdiResponseHandler* resHandler = resHandlerFactory->makeHandler(pMes_, sessionHandler()->edih()->msg_type,
                                                                    hth, sessionHandler());
    
    if(!resHandler) {
        throw Exception(std::string("There is no request handler for ") + MessageName);
    }
    
    return resHandler;
}

EdiRequestHandler* EdiHandlerManager::makeRequestHandler()
{
    const EdiRequestHandlerFactory *reqHandlerFactory;

    // getting factory
    reqHandlerFactory = requestHandlerFactory();

    if(!reqHandlerFactory) {
        // should never happen
        throw Exception("Fatal: Unable to retrieve EdifactRequestHandlerFactory");
    }

    // getting concrete edifact req handler
    EdiRequestHandler* reqHandler = reqHandlerFactory->makeHandler(pMes_, sessionHandler()->edih()->msg_type,
                                                                   hth, sessionHandler());

    if(!reqHandler) {
        throw Exception(std::string("There is no request handler for ") + MessageName);
    }
    
    return reqHandler;
}

void EdiHandlerManager::handleRequest()
{
    boost::scoped_ptr<EdiRequestHandler> reqHandler(makeRequestHandler());
    SessionHandler->UpdateEdiSession();
    
    handleRequest(reqHandler.get());
}

void EdiHandlerManager::handleResponse(EdiResponseHandler *resHandler)
{
    beforeProc(); // user overloaded function to call before proc()

    try {
        resHandler->fillErrorDetails();
        resHandler->fillFuncCodeRespStatus();

        resHandler->parse();
        resHandler->handle();
    }
    catch (const std::exception &e) {
        LogWarning(STDLOG) << e.what();
        HasError = true;
        resHandler->onHandlerError(&e);
        afterProcFailed(&e, resHandler);
        return;
    }

    afterProc();
}

void EdiHandlerManager::handleResponse()
{
    boost::scoped_ptr<EdiResponseHandler> resHandler(makeResponseHandler());
    if(resHandler->needDetectCarf()) {
        detectCarf();
    }
    SessionHandler->UpdateEdiSession();
    
    handleResponse(resHandler.get());
}

void EdiHandlerManager::handle()
{
    parseMessage();

    findMessageConfig();

    edi_mes_head MsgHead = {};
    readMessageHead(MsgHead);

    SessionHandler = makeSessionHandler(hth, MsgHead);
    if(!SessionHandler) {
        throw Exception("makeSessionHandler failed. Unable to continue w/o edifact session handler");
    }

    if(MsgType->query_type == QUERY) {
        handleRequest();
    } else {
        handleResponse();
    }

    SessionHandler->ediSession()->CommitEdiSession();
}

void EdiHandlerManager::detectCarf()
{
    if(!strlen(SessionHandler->edih()->acc_ref))
    {
        int num;
        std::string acc_ref = edilib::EdiSession::OurRefByFull(SessionHandler->edih()->our_ref, &num);
        strncpy(SessionHandler->edih()->acc_ref, acc_ref.c_str(), sizeof SessionHandler->edih()->acc_ref);
        LogTrace(TRACE1) << "acc_ref was updated by our_ref=" << acc_ref;
    }
}

}
