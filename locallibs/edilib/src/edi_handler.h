#ifndef EDI_HANDLER_H
#define EDI_HANDLER_H

#include <string>
#include <libtlg/hth.h>
#include <boost/optional.hpp>
#include <edilib/edi_types.h>

namespace edilib
{

class EdiRequestHandler;
class EdiResponseHandler;
class EdiSessRdData;

/// @class EdiRequestHandlerFactory - should return an edifact request handler
class EdiRequestHandlerFactory
{
public:
    virtual EdiRequestHandler * makeHandler (EDI_REAL_MES_STRUCT *pMes, edi_msg_types_t req,
                                             const hth::HthInfo *hth,
                                             const edilib::EdiSessRdData * sessionHandler) const = 0;
    virtual ~EdiRequestHandlerFactory() {}
};

/// @class EdiResponseHandlerFactory - should return an edifact response handler
class EdiResponseHandlerFactory
{
public:
    virtual EdiResponseHandler * makeHandler (EDI_REAL_MES_STRUCT *pMes, edi_msg_types_t req,
                                              const hth::HthInfo *hth,
                                              const edilib::EdiSessRdData * sessionHandler) const = 0;
    virtual ~EdiResponseHandlerFactory() {}
};

struct AnswerData {
    std::string MessageText;
    boost::optional<hth::HthInfo> hth;
};

/**
  * @class EdiHandlerManager - edifact request handler wraper
  * @attention please remember - this is interface object, it is your responsibility to check \
  * wherever this interface is used. Change virtual functions carefully
*/
class EdiHandlerManager
{
    // hth of inbound message
    const hth::HthInfo *hth;
    const char *edifact_message;

    // parsed edifact message
    EDI_REAL_MES_STRUCT *pMes_;
    EDI_MESSAGES_STRUCT *pTempMes;
    EDI_MSG_TYPE *MsgType;
    EdiSessRdData * SessionHandler;

    std::string MessageName;

    boost::optional<AnswerData> Answer;

    bool HasError;

    void parseMessage();
    /// найти MsgType с настройками для данного сообщения
    void findMessageConfig();
    void readMessageHead(edi_mes_head &MsgHead);
    void handleRequest();
    void handleRequest(EdiRequestHandler *reqHandler);

    void handleResponse();
    void handleResponse(EdiResponseHandler *resHandler);

    void createAnswerFromReq();
    void updateMsgHead(edi_mes_head *MsgHead);

    void makeAnswerText(EDI_REAL_MES_STRUCT *pMesW);
    
    void detectCarf();
   
    /// (c)
    EdiHandlerManager(const EdiHandlerManager &);
    
    EdiResponseHandler * makeResponseHandler();
    EdiRequestHandler  * makeRequestHandler();
            
public:
    EdiHandlerManager(const hth::HthInfo *hth, const char *edifact_message);
    virtual ~EdiHandlerManager();

    const EDI_REAL_MES_STRUCT *pMes() const;

    const EdiSessRdData * sessionHandler() const;

    const boost::optional<AnswerData> &answer() const;

    /**
      * @brief returns true if exception was catched during handling
    */
    bool hasError() const;


    /// @brief handle whole edifact message (request or answer, we don't know at this point)
    virtual void handle();

    /**
      * @brief will be called in case of edifact parser error
      * @param err - edifact parser error code from edilib/edi_user_func.h
      * @param error_str - edifact error description text, can be null
     **/
    virtual void onParseFailed(int err, const char *error_str);

    /// @brief will be called before calling proc() function
    virtual void beforeProc() {}

    /// @brief will be called after proc() function succeed
    virtual void afterProc() {}

    /// @brief will be called after proc() function failed
    virtual void afterProcFailed(const std::exception *, const EdiRequestHandler *) {}
    virtual void afterProcFailed(const std::exception *, const EdiResponseHandler *) {}

    /// @brief will be called before makeAnAnswer() function
    virtual void beforeMakeAnswer() {}

    /// @brief will be called after makeAnAnswer() function succeed
    virtual void afterMakeAnswer() {}

    /// @brief will be called after makeAnAnswer() function failed
    virtual void afterMakeAnswerFailed(const std::exception *, const EdiRequestHandler *) {}

    /// @brief abstract factory to get edifact request handler
    virtual const EdiRequestHandlerFactory *requestHandlerFactory() = 0;

    /// @brief abstract factory to get edifact response handler
    virtual const EdiResponseHandlerFactory *responseHandlerFactory() = 0;

    /**
      * @brief here we expect to receive a new copy of application \n
      * specific edifact session handler
    */
    virtual EdiSessRdData * makeSessionHandler(const hth::HthInfo *hth, const edi_mes_head &MsgHead) = 0;
};

}

#endif // EDI_HANDLER_H
