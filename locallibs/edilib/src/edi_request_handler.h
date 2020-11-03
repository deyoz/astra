#ifndef EDI_REQUEST_HANDLER_H
#define EDI_REQUEST_HANDLER_H

#include <boost/shared_ptr.hpp>
#include <edilib/edi_types.h>
#include <edilib/edi_session.h>
#include <edilib/edi_response_handler.h>

struct _EDI_REAL_MES_STRUCT_;

namespace edilib {
/**
 * @class EdifactRequestHandler
 * @brief parse edifact, handle and make an answer!
*/
class EdiRequestHandler
{
    _EDI_REAL_MES_STRUCT_ *PMesSelect;
    _EDI_REAL_MES_STRUCT_ *PMesInsert;
    const edilib::EdiSessRdData *EdiSess;

    std::string EdiErrorCode;
    std::string EdiErrorText;

    // edifact response status
    edilib::EdiRespStatus RespStatus;
public:
    /**
     * @brief sets an edifact response status (handling result)
     */
    void setRespStatus(edilib::EdiRespStatus::RespStatus_t respStatus);

    /**
     * @brief edifact message struct. Getting data from edifact
     * @return
     */
    _EDI_REAL_MES_STRUCT_ *pMes() const;

    /**
     * @brief edifact message struct. Inserting data into edifact
     * @brief returns NULL before makeAnAnswer
     * @return
     */
    _EDI_REAL_MES_STRUCT_ *pMesW() const;

    /**
     * @brief set pMesW, right before makeAnAnswer
    */
    void setMesW(_EDI_REAL_MES_STRUCT_ *val);

    /**
     * @brief Edifact session ID
     * @return
     */
    edilib::EdiSessionId_t ediSessId() const;

    /**
     * @brief Edifact session
     * @return
     */
    const edilib::EdiSessRdData * ediSess() const;

    /**
     * @brief parsing
     * should be overloaded
     */
    virtual void parse() = 0;

    /**
     * @brief Handle after parse was succeed
     */
    virtual void handle() = 0;

    /**
     * @brief send an answer after handle() was succeed
     */
    virtual void makeAnAnswer() = 0;

    /**
     * @brief edifact function code: MSG tag. Specify "" if it is not needed.
    */
    virtual std::string mesFuncCode() const = 0;

    /**
     * @brief mes func for answer
    */
    virtual std::string mesFuncCodeAnswer() const { return mesFuncCode(); }

    /**
     * @brief will be called if parse() throwed an exception
    */
    virtual void onParseError(const std::exception *e) {}

    /**
     * @brief will be called if handler throwed an exception
     * @brief you should set EdiErrorCode/EdiErrorText in it
    */
    virtual void onHandlerError(const std::exception *e);

    /**
     * @brief makes an ERC and IFT codes
    */
    virtual void makeAnAnswerErr();

    /**
     * @brief makes MSG edifact tag. Uses RespStatus to detect handling result.
    */
    virtual void makeMSG();

    /**
     * @brief edifact error code
    */
    virtual void setEdiErrorCode(const std::string &val) { EdiErrorCode = val; }
    /**
    * @brief edifact error text
    */
    virtual void setEdiErrorText(const std::string &val) { EdiErrorText = val; }

    virtual const std::string &ediErrorCode() const { return EdiErrorCode; }
    virtual const std::string &ediErrorText() const { return EdiErrorText; }

    EdiRequestHandler(_EDI_REAL_MES_STRUCT_ *PMes, const EdiSessRdData *edisess);


    virtual ~EdiRequestHandler();
};

} // namespace edilib
#endif // EDI_REQUEST_HANDLER_H
