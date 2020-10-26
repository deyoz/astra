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
    : AstraEdiRequestHandler(pMes, edisess), m_ediErrorLevel("1")
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
    m_lRes = handleRequest();
}

void IatciRequestHandler::makeAnAnswer()
{
    int curFlg = 0;
    for(const auto& res: m_lRes)
    {
        PushEdiPointW(pMesW());
        SetEdiSegGr(pMesW(), SegGrElement(1, curFlg));
        SetEdiPointToSegGrW(pMesW(), SegGrElement(1, curFlg), "SegGr1(flg) not found");

        viewFdrElement(pMesW(), res.flight(), fcIndicator());
        viewRadElement(pMesW(), respType(), fullAnswer() ? "O" : "P");
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
                if(pxg.doc() || pxg.visa() || pxg.address()) {
                    PushEdiPointW(pMesW());
                    SetEdiSegGr(pMesW(), SegGrElement(3, curApg));
                    SetEdiPointToSegGrW(pMesW(), SegGrElement(3, curApg), "SegGr3(apg) not found");

                    if(pxg.doc() || pxg.visa()) {
                        viewPapElement(pMesW(), pxg.pax(), pxg.doc(), pxg.visa());
                    } else {
                        viewPapElement(pMesW(), false/*not-infant*/);
                    }

                    if(pxg.address()) {
                        viewAddElement(pMesW(), *pxg.address());
                    }

                    PopEdiPointW(pMesW());
                    curApg++;
                }

                if(pxg.infantDoc() || pxg.infantVisa() || pxg.infantAddress()) {
                    ASSERT(pxg.infant());
                    PushEdiPointW(pMesW());
                    SetEdiSegGr(pMesW(), SegGrElement(3, curApg));
                    SetEdiPointToSegGrW(pMesW(), SegGrElement(3, curApg), "SegGr3(apg) not found");

                    if(pxg.infantDoc()) {
                        viewPapElement(pMesW(), *pxg.infant(), pxg.infantDoc(), pxg.infantVisa());
                    } else {
                        viewPapElement(pMesW(), true/*infant*/);
                    }

                    if(pxg.infantAddress()) {
                        viewAddElement(pMesW(), *pxg.infantAddress());
                    }

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

    ASSERT(params());
    viewFdrElement(pMesW(), params()->outboundFlight(), "T");
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

}//namespace TlgHandling
