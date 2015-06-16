#include "IatciRequestHandler.h"
#include "postpone_edifact.h"
#include "view_edi_elements.h"
#include "remote_system_context.h"
#include "edi_msg.h"
#include "iatci_api.h"

#include <serverlib/str_utils.h>
#include <edilib/edi_func_cpp.h>
#include <etick/exceptions.h>

#include <boost/foreach.hpp>

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace TlgHandling {

using namespace edifact;
using namespace edilib;
using namespace Ticketing;
using namespace Ticketing::TickExceptions;
using namespace Ticketing::RemoteSystemContext;


IatciRequestHandler::IatciRequestHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                                         const edilib::EdiSessRdData *edisess)
    : AstraRequestHandler(pMes, edisess), m_ediErrorLevel("1")
{
}

std::string IatciRequestHandler::mesFuncCode() const
{
    return ""; // No MSG segment at IATCI
}

bool IatciRequestHandler::fullAnswer() const
{
    return true;
}

void IatciRequestHandler::handle()
{
    if(nextParams())
    {
        if(!SystemContext::Instance(STDLOG).inbTlgInfo().repeatedlyProcessed()) {
            EdiSessionId_t sessId = sendCascadeRequest();
            throw TlgHandling::TlgToBePostponed(sessId);
        } else {
            // достаём данные, которые получили от другой DCS
            loadDeferredData();
        }
    }

    // выполним обработку "у нас"
    m_lRes.push_front(handleRequest());
}

void IatciRequestHandler::makeAnAnswer()
{
    int curSg1 = 0;
    BOOST_FOREACH(const iatci::Result& res, m_lRes)
    {
        PushEdiPointW(pMesW());
        SetEdiSegGr(pMesW(), SegGrElement(1, curSg1));
        SetEdiPointToSegGrW(pMesW(), SegGrElement(1, curSg1), "SegGr1(flg) not found");

        viewFdrElement(pMesW(), res.flight());
        viewRadElement(pMesW(), respType(), res.statusAsString());
        if(res.cascadeDetails())
            viewChdElement(pMesW(), *res.cascadeDetails());

        if(fullAnswer())
        {
            viewFsdElement(pMesW(), res.flight());

            PushEdiPointW(pMesW());
            SetEdiSegGr(pMesW(), SegGrElement(2));
            SetEdiPointToSegGrW(pMesW(), SegGrElement(2), "SegGr2(pxg) not found" );

            ASSERT(res.pax());
            viewPpdElement(pMesW(), *res.pax());
            if(res.seat())
                viewPfdElement(pMesW(), *res.seat());

            PopEdiPointW(pMesW());
        }
        PopEdiPointW(pMesW());

        curSg1++;
    }
}

void IatciRequestHandler::makeAnAnswerErr()
{
    PushEdiPointW(pMesW());
    SetEdiSegGr(pMesW(), SegGrElement(1));
    SetEdiPointToSegGrW(pMesW(), SegGrElement(1), "SegGr1(flg) not found");

    viewFdrElement(pMesW(), requestParams().flight());
    viewRadElement(pMesW(), respType(), "F");
    viewErdElement(pMesW(), ediErrorLevel(), ediErrorCode(), ediErrorText());

    PopEdiPointW(pMesW());
}

void IatciRequestHandler::saveErrorInfo(const Ticketing::ErrMsg_t& errCode,
                                        const std::string& errText)
{
    setEdiErrorCode(StrUtils::ToUpper(getErdErrByInner(errCode)));
    setEdiErrorText(StrUtils::ToUpper(errText));
}

void IatciRequestHandler::setEdiErrorLevel(const std::string& errLevel)
{
    m_ediErrorLevel = errLevel;
}

const std::string& IatciRequestHandler::ediErrorLevel() const
{
    return m_ediErrorLevel;
}

void IatciRequestHandler::loadDeferredData()
{
    m_lRes = iatci::loadDeferredCkiData(SystemContext::Instance(STDLOG).inbTlgInfo().tlgNum());
    if(m_lRes.empty()) {
        throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR, "Empty result list!");
    } else if(m_lRes.size() == 1 && m_lRes.front().status() == iatci::Result::Failed) {
        // что-то пошло не так - скорее всего, случился таймаут
        boost::optional<iatci::ErrorDetails> err = m_lRes.front().errorDetails();
        if(err) {
            throw tick_soft_except(STDLOG, err->errCode(), err->errDesc().c_str());
        } else {
            LogError(STDLOG) << "Warning: error detected but error information not found!";
        }
    }
}

iatci::BaseParams IatciRequestHandler::requestParams() const
{
    ASSERT(params());
    return *params();
}

}//namespace TlgHandling
