#include "EtCosRequestHandler.h"
#include "view_edi_elements.h"
#include "read_edi_elements.h"
#include "astra_pnr.h"
#include "etick.h"
#include "edi_tlg.h"
#include "edi_msg.h"
#include "remote_system_context.h"
#include "AirportControl.h"

#include <edilib/edi_func_cpp.h>
#include <etick/tick_data.h>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace TlgHandling {

using namespace edifact;
using namespace edilib;
using namespace Ticketing;
using namespace Ticketing::TickReader;
using namespace Ticketing::TickExceptions;
using Ticketing::RemoteSystemContext::SystemContext;


namespace {

class CosParamsMaker
{
private:
    edifact::TktElem m_tkt;
    edifact::CpnElem m_cpn;

public:
    void setTkt(const boost::optional<edifact::TktElem>& tkt);
    void setCpn(const boost::optional<edifact::CpnElem>& cpn);

    CosParams makeParams() const;
};

//

void CosParamsMaker::setTkt(const boost::optional<edifact::TktElem>& tkt)
{
    ASSERT(tkt);
    m_tkt = *tkt;
}

void CosParamsMaker::setCpn(const boost::optional<edifact::CpnElem>& cpn)
{
    ASSERT(cpn);
    m_cpn = *cpn;
}

CosParams CosParamsMaker::makeParams() const
{
    return CosParams(m_tkt.m_ticketNum,
                     m_cpn.m_num);
}

//---------------------------------------------------------------------------------------

TktElem makeTkt(const TicketNum_t& tickNum)
{
    TktElem tkt;
    tkt.m_ticketNum      = tickNum;
    tkt.m_docType        = DocType::Ticket;
    tkt.m_conjunctionNum = 1;
    tkt.m_tickStatAction = TickStatAction::newtick;
    return tkt;
}

CpnElem makeCpn(const CouponNum_t& cpnNum)
{
    CpnElem cpn;
    cpn.m_num        = cpnNum;
    cpn.m_status     = CouponStatus(CouponStatus::OriginalIssue);
    cpn.m_media      = TicketMedia::Electronic;
    cpn.m_prevStatus = CouponStatus(CouponStatus::Airport);
    return cpn;
}

}//namespace

/////////////////////////////////////////////////////////////////////////////////////////

CosRequestHandler::CosRequestHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                                     const edilib::EdiSessRdData *edisess)
    : AstraRequestHandler(pMes, edisess)
{}

std::string CosRequestHandler::mesFuncCode() const
{
    return "142";
}

bool CosRequestHandler::fullAnswer() const
{
    return false;
}

void CosRequestHandler::parse()
{
    CosParamsMaker cosParamsMaker;

    SetEdiPointToSegGrG(pMes(), SegGrElement(1), "PROG_ERR");
    cosParamsMaker.setTkt(readEdiTkt(pMes()));
    SetEdiPointToSegGrG(pMes(), SegGrElement(2), "PROG_ERR");
    cosParamsMaker.setCpn(readEdiCpn(pMes()));

    m_cosParams = cosParamsMaker.makeParams();
}

void CosRequestHandler::handle()
{
    LogTrace(TRACE3) << __FUNCTION__;
    ASSERT(m_cosParams);

    try {
        Ticketing::returnWcCoupon(SystemContext::Instance(STDLOG).airlineImpl()->ida(),
                                  m_cosParams->m_tickNum,
                                  m_cosParams->m_cpnNum,
                                  true);
    } catch(const AirportControlCantBeReturned& e) {
        throw tick_soft_except(STDLOG, AstraErr::INV_COUPON_STATUS);
    } catch(const AirportControlNotFound& e) {
        throw tick_soft_except(STDLOG, AstraErr::INV_COUPON_STATUS);
    }
}

void CosRequestHandler::makeAnAnswer()
{
    LogTrace(TRACE3) << __FUNCTION__;
    ASSERT(m_cosParams);

    PushEdiPointW(pMesW());
    SetEdiSegGr(pMesW(), SegGrElement(1));
    SetEdiPointToSegGrW(pMesW(), SegGrElement(1), "SegGr1 not found");
    viewTktElement(pMesW(), makeTkt(m_cosParams->m_tickNum));

    PushEdiPointW(pMesW());
    SetEdiSegGr(pMesW(), SegGrElement(2));
    SetEdiPointToSegGrW(pMesW(), SegGrElement(2), "SegGr2 not found");
    viewCpnElement(pMesW(), makeCpn(m_cosParams->m_cpnNum));
}

void CosRequestHandler::saveErrorInfo(const Ticketing::ErrMsg_t& errCode,
                                      const std::string&/*не хотим класть текст ошибки*/)
{
    setEdiErrorCode(StrUtils::ToUpper(getErcErrByInner(errCode)));
}

}//namespace TlgHandling
