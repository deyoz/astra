//
// C++ Interface: edi_request
//
// Description: Makes an EDIFACT request structure
//
//
// Author: Komtech-N <rom@sirena2000.ru>, (C) 2007
//
//
#ifndef _EDILIB_EDIFACTREQUEST_H_
#define _EDILIB_EDIFACTREQUEST_H_
#include <string>
#include <vector>
#include "edilib/edi_types.h"
#include "edilib/fse_types.h"
#include "edilib/edi_session.h"
#include "edilib/EdiSessionId_t.h"

struct _edi_mes_head_;
struct _EDI_REAL_MES_STRUCT_;

namespace edilib
{
class EdiSessWrData;

/**
 * @class EdifactRequest
 * @brief Makes an EDIFACT request structure
 */
class EdifactRequest
{
    _edi_mes_head_ *mhead;
    EDI_MSG_TYPE *pEType;
    EdiSessWrData *EdiSess;
    bool WaitForAnswer;

    EdifactRequest(const EdifactRequest &);
public:
    /**
     * @brief EdifactRequest
     * @param msg_type тип сообщения
     */
    EdifactRequest(edi_msg_types_t msg_type);

    _EDI_REAL_MES_STRUCT_ *pMes();

    /**
     * @brief set edifact session controller
     * @param EdiSess
     */
    void setEdiSessionController(EdiSessWrData *EdiSess);

    /**
     * @brief sets edifact message attributes (char set, edifact version, etc)
     * @param EdiSess
     */
    void setEdiSessMesAttr();

    /**
     * @brief returns edifact msg head
     * @return
     */
    _edi_mes_head_ *msgHead() { return mhead; }

    /**
     * @brief returns edifact msg head
     * @return
     */
    const _edi_mes_head_ *msgHead() const { return mhead; }

    /**
     * @brief Собрать ытелеграмму, используя данные из pMes()
     */
    virtual std::string makeEdifactText();

    /**
     * Данные по edifact сесси
     * @return
     */
    const EdiSessWrData *ediSess() const;
    EdiSessWrData *ediSess();

    /**
     * @brief edifact session id
     * @return
     */
    edilib::EdiSessionId_t ediSessId() const;

    /**
     * @brief edifact unique reference
    */
    std::string unReference() const;

    /**
     * @brief MSG+ function code, return "" if you don't want MSG in your message
    */
    virtual std::string mesFuncCode() const = 0;

    /**
      * @brief MGS subfunction code. empty by default.
      * @brief MSG+1:77::_1_ <-- ITAREQ with smart lookup and "1" - cancel subfunction
    */
    virtual std::vector<std::string> mesFuncSubcode() const { return std::vector<std::string>(); }

    /**
     * @brief MSG+ type of service code, returns "" by default
    */
    virtual std::string mesTypeOfService() const { return ""; }

    virtual std::string mesFuncCodeAnswer() const { return mesFuncCode(); }

    virtual void drawMsgFuncCode();

    virtual void setWaitForAnswer(bool val) { WaitForAnswer = val; }

    virtual bool waitForAnswer() const { return WaitForAnswer; }

    /**
     *  @brief functional service element(FSE) type
    */
    virtual EdiFse fseType() const { return EdiFse(); }

    virtual ~EdifactRequest();
};
} // namespace edilib

#endif /*_EDILIB_EDIFACTREQUEST_H_*/
