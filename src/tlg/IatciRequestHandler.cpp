#include "IatciRequestHandler.h"
#include "postpone_edifact.h"
#include "view_edi_elements.h"
#include "remote_system_context.h"
#include "edi_msg.h"
#include "iatci_api.h"
#include "iatci_help.h"

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
    // TODO
//    if(nextParams())
//    {
//        if(!postponeHandling()) {
//            // TODO
//            //EdiSessionId_t sessId = sendCascadeRequest();
//            //throw TlgHandling::TlgToBePostponed(sessId);
//        } else {
//            // достаём данные, которые получили от другой DCS
//            loadDeferredData();
//        }
//    }

    // выполним обработку "у нас"
    m_lRes.push_front(handleRequest());
}

void IatciRequestHandler::makeAnAnswer()
{
    int curFlg = 0;
    for(const auto& res: m_lRes)
    {
        PushEdiPointW(pMesW());
        SetEdiSegGr(pMesW(), SegGrElement(1, curFlg));
        SetEdiPointToSegGrW(pMesW(), SegGrElement(1, curFlg), "SegGr1(flg) not found");

        viewFdrElement(pMesW(), res.flight());
        viewRadElement(pMesW(), respType(), res.statusAsString());
        if(res.cascade()) {
            viewChdElement(pMesW(), *res.cascade());
        }

        if(fullAnswer())
        {
            viewFsdElement(pMesW(), res.flight());

            int curPxg = 0;
            for(const auto& pxg: res.paxGroups())
            {
                PushEdiPointW(pMesW());
                SetEdiSegGr(pMesW(), SegGrElement(2, curPxg));
                SetEdiPointToSegGrW(pMesW(), SegGrElement(2, curPxg), "SegGr2(pxg) not found");

                if(pxg.infant()) {
                    viewPpdElement(pMesW(), pxg.pax(), *pxg.infant());
                } else {
                    viewPpdElement(pMesW(), pxg.pax());
                }

                if(pxg.reserv()) {
                    viewPrdElement(pMesW(), *pxg.reserv());
                }
                if(pxg.seat()) {
                    viewPfdElement(pMesW(), *pxg.seat(), pxg.infantSeat());
                }
                if(pxg.service()) {
                    viewPsiElement(pMesW(), *pxg.service());
                }
                if(pxg.baggage()) {
                    viewPbdElement(pMesW(), *pxg.baggage());
                }
                int curApg = 0;
                if(pxg.doc()) {
                    PushEdiPointW(pMesW());
                    SetEdiSegGr(pMesW(), SegGrElement(3, curApg));
                    SetEdiPointToSegGrW(pMesW(), SegGrElement(3, curApg), "SegGr3(apg) not found");

                    viewPapElement(pMesW(), *pxg.doc(), pxg.pax());

                    PopEdiPointW(pMesW());
                    curApg++;
                }

                if(pxg.infantDoc()) {
                    ASSERT(pxg.infant());
                    PushEdiPointW(pMesW());
                    SetEdiSegGr(pMesW(), SegGrElement(3, curApg));
                    SetEdiPointToSegGrW(pMesW(), SegGrElement(3, curApg), "SegGr3(apg) not found");

                    viewPapElement(pMesW(), *pxg.infantDoc(), *pxg.infant());

                    PopEdiPointW(pMesW());
                    curApg++;
                }

                PopEdiPointW(pMesW());
                curPxg++;
            }

        }
        PopEdiPointW(pMesW());

        curFlg++;
    }
}

void IatciRequestHandler::makeAnAnswerErr()
{
    PushEdiPointW(pMesW());
    SetEdiSegGr(pMesW(), SegGrElement(1));
    SetEdiPointToSegGrW(pMesW(), SegGrElement(1), "SegGr1(flg) not found");

    ASSERT(paramsNew());
    viewFdrElement(pMesW(), paramsNew()->outboundFlight());
    viewRadElement(pMesW(), respType(), "X");
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

bool IatciRequestHandler::postponeHandling() const
{
    return SystemContext::Instance(STDLOG).inbTlgInfo().repeatedlyProcessed();
}

void IatciRequestHandler::loadDeferredData()
{
    m_lRes = iatci::loadDeferredCkiData(inboundTlgNum());
    if(m_lRes.empty()) {
        throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR, "Empty result list!");
    } else if(m_lRes.size() == 1 &&
              (m_lRes.front().status() == iatci::dcrcka::Result::Failed ||
               m_lRes.front().status() == iatci::dcrcka::Result::RecoverableError)) {
        // что-то пошло не так - скорее всего, случился таймаут
        boost::optional<iatci::ErrorDetails> err = m_lRes.front().error();
        if(err) {
            throw tick_soft_except(STDLOG, err->errCode(), err->errDesc().c_str());
        } else {
            LogError(STDLOG) << "Warning: error detected but error information not found!";
        }
    }
}

}//namespace TlgHandling
