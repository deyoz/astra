#include "CusRequestHandler.h"
#include "apis_edi_file.h"
#include "apps_interaction.h"
#include "view_edi_elements.h"

#include <edilib/edi_func_cpp.h>

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace TlgHandling
{

using namespace edilib;
using namespace edifact;


CusRequestHandler::CusRequestHandler(_EDI_REAL_MES_STRUCT_ *PMes,
                                     const edilib::EdiSessRdData *edisess)
    : AstraEdiRequestHandler(PMes, edisess)
{}

std::string CusRequestHandler::mesFuncCode() const
{
    return "";
}

bool CusRequestHandler::fullAnswer() const
{
    return true;
}

void CusRequestHandler::parse()
{
   m_data.reset(new edifact::Cusres(readCUSRES(pMes())));
}

void CusRequestHandler::handle()
{
    ASSERT(m_data);
    ProcessChinaCusres(*m_data);
}

static UngElem makeAnswerUng(const UngElem& ung)
{
    std::string senderId = ung.m_senderName,
              senderCode = ung.m_senderCarrierCode;
    std::string  rcpntId = ung.m_recipientName,
               rcpntCode = ung.m_recipientCarrierCode;

    UngElem answerUng = ung;
    answerUng.m_msgGroupName         = "CUSRES";
    answerUng.m_senderName           = rcpntId;
    answerUng.m_senderCarrierCode    = rcpntCode;
    answerUng.m_recipientName        = senderId;
    answerUng.m_recipientCarrierCode = senderCode;
    return answerUng;
}

static Cusres makeAnswerCusres(const Cusres& cusres)
{
    Cusres answer = cusres;
    if(answer.m_ung) {
        answer.m_ung = makeAnswerUng(answer.m_ung.get());
    }

    return answer;
}

void CusRequestHandler::makeAnAnswer()
{
    ASSERT(m_data);
    Cusres cusres = makeAnswerCusres(*m_data);

    viewBgmElement(pMesW(), BgmElem("312", ""));

    if(cusres.m_ung) {
        viewUngElement(pMesW(), cusres.m_ung.get());
    }

    if(cusres.m_rff) {
        viewRffElement(pMesW(), cusres.m_rff.get());
    }

    int curSegGr3 = 0;
    for(const Cusres::SegGr3& sg3 : cusres.m_vSegGr3)
    {
        SetEdiSegGr(pMesW(), SegGrElement(3, curSegGr3));

        PushEdiPointW(pMesW());
        SetEdiPointToSegGrW(pMesW(), SegGrElement(3, curSegGr3++));

        viewRffElement(pMesW(), sg3.m_rff);

        viewDtmElement(pMesW(), sg3.m_dtm1, 0);
        viewDtmElement(pMesW(), sg3.m_dtm2, 1);
        viewLocElement(pMesW(), sg3.m_loc1, 0);
        viewLocElement(pMesW(), sg3.m_loc2, 1);

        PopEdiPointW(pMesW());
    }

    int curSegGr4 = 0;
    for(const Cusres::SegGr4& sg4 : cusres.m_vSegGr4)
    {
        SetEdiSegGr(pMesW(), SegGrElement(4, curSegGr4));

        PushEdiPointW(pMesW());
        SetEdiPointToSegGrW(pMesW(), SegGrElement(4, curSegGr4++));

        SetEdiFullSegment(pMesW(), SegmElement("ERP"), sg4.m_erp.m_msgSectionCode);

        int rffNum = 0;
        for(const RffElem& rffElem : sg4.m_vRff)
        viewRffElement(pMesW(), rffElem, rffNum++);

        SetEdiFullSegment(pMesW(), SegmElement("ERC"), sg4.m_erc.m_errorCode);

        PopEdiPointW(pMesW());
    }

    if(cusres.m_une) {
        viewUneElement(pMesW(), cusres.m_une.get());
    }
}

}//namespace TlgHandling
