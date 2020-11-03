//
// C++ Interface: edi_response
//
// Description: Базовый класс обработки edifact ответов
//
//
// Author: Kovalev Roman <rom@sirena2000.ru>, (C) 2008
//
//
#ifndef _EDILIB_EDI_RESPONSE_H_
#define _EDILIB_EDI_RESPONSE_H_

#include <string>
#include <edilib/EdiSessionId_t.h>
#include <edilib/edi_session.h>

struct _EDI_REAL_MES_STRUCT_;

/**
 * @file edi_response.h
 * @brief base class for edifact response handling
 */

namespace edilib
{

/**
 * @class EdiRespStatus
 * @brief Статус ответа
 */
class EdiRespStatus
{
public:
    /**
     * @enum RespStatus_t
     * @brief Успешно или нет тлг обработана удаленным хостом
     */
    enum RespStatus_t
    {
        /// successfully handled by remote
        successfully,
        /// unsuccessfully handled by remote
        notProcessed,
        rejected,
        /// particularly handled
        partial
    };

    explicit EdiRespStatus(RespStatus_t Stat);
    explicit EdiRespStatus(const std::string &Stat);

    /**
     * @brief charecter status code
     * @return
     */
    std::string code() const;
    /**
     * @brief status RespStatus_t
     * @return
     */
    RespStatus_t status() const;
private:
    RespStatus_t Status;
    std::string Code;
};



/**
 * @class EdiResponseHandler
 *
 *common usage
 * resHandler->readRemoteResults();
 * resHandler->fillFuncCodeRespStatus();
 * resHandler->fillErrorDetails();
 * resHandler->updateRemoteResults();
 * resHandler->setTlgSource(ud->tlgText());
 * resHandler->handle();
 * resHandler->archiveTlg();
 * resHandler->agentNotify();
 *
*/
class EdiResponseHandler
{
    _EDI_REAL_MES_STRUCT_ *PMes;
    std::string EdiErrCode;
    std::string EdiErrText;

    const edilib::EdiSessRdData *EdiSess;
    EdiRespStatus RespStatus;
    std::string FuncCode;
        
public:
    /**
     * @brief edifact message struct
     * @return
     */
    _EDI_REAL_MES_STRUCT_ *pMes() const;

    /**
     * @brief Edifact session ID
     * @return
     */
    edilib::EdiSessionId_t ediSessId() const;

    const std::string &ediErrCode() const;

    const std::string &ediErrText() const;

    void setEdiErrCode(const std::string &val);

    void setEdiErrText(const std::string &val);

    /**
     * @brief Edifact session
     * @return
     */
    const edilib::EdiSessRdData * ediSess() const;

    /**
     * @brief Response status
     * @brief Успешно или нет тлг обработана удаленным хостом
     * @return
     */
    const EdiRespStatus &respStatus() const;

    /**
     * @brief sets response status
     * @param
     */
    void setRespStatus(const EdiRespStatus &rstat);

    /**
     * @brief message function code
     * @return
     */
    const std::string funcCode() const;

    /**
     * @brief unique edifact reference. Same as in EdiRequest::unReference()
    */
    std::string unReference() const;

    void setFuncCode(const std::string &fc);

    /**
      * @brief load to internal structures
      * should be overloaded
    */
    virtual void parse() = 0;

    /**
     * @brief handling
     * should be overloaded
     */
    virtual void handle() = 0;

    /**
     * @brief On time out
     */
    virtual void onTimeOut() = 0;

    /**
     * @brief On CONTRL
     */
    virtual void onCONTRL() = 0;

    /**
      * @brief set Function Code and Response status code
    */
    virtual void fillFuncCodeRespStatus() = 0;

    virtual void fillErrorDetails();

    /**
     * @brief will be called in case of exception from parse() or handler()
     */
    virtual void onHandlerError(const std::exception *);
    
    /**
     * @brief this method indicates that empty acc_ref in edi_mes_head will be restored by other our_ref in edi_mes_head
     */
    virtual bool needDetectCarf() const;

    EdiResponseHandler(_EDI_REAL_MES_STRUCT_ *PMes, const edilib::EdiSessRdData *EdiSess);
    virtual ~EdiResponseHandler() {}
};


} // namespace edilib

#endif /*_EDILIB_EDI_RESPONSE_H_*/
