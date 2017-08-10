#include "et_rac_request.h"
#include "view_edi_elements.h"

#include <edilib/edi_func_cpp.h>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>



namespace edifact {

EtRacRequest::EtRacRequest(const EtRacParams& racParams)
    : EtRequest(racParams), m_racParams(racParams)
{}

std::string EtRacRequest::mesFuncCode() const
{
    return "734";
}

void EtRacRequest::collectMessage()
{
    viewOrgElement2(pMes(), m_racParams.org());
    viewEqnElement(pMes(), EqnElem(1, "TD"));

    edilib::SetEdiSegGr(pMes(), 1);
    edilib::SetEdiPointToSegGrW(pMes(), 1);
    TktElem tkt;
    tkt.m_ticketNum = m_racParams.tickNum();
    tkt.m_docType = Ticketing::DocType(Ticketing::DocType::Ticket);
    tkt.m_tickStatAction = Ticketing::TickStatAction::newtick;
    tkt.m_conjunctionNum = 1;
    viewTktElement(pMes(), tkt);

    edilib::SetEdiSegGr(pMes(), 2);
    edilib::SetEdiPointToSegGrW(pMes(), 2);
    CpnElem cpn;
    cpn.m_num = m_racParams.cpnNum();
    cpn.m_media = Ticketing::TicketMedia(Ticketing::TicketMedia::Electronic);
    // запрашивать контроль будем статусом A
    cpn.m_status = Ticketing::CouponStatus(Ticketing::CouponStatus::Airport);
    viewCpnElement(pMes(), cpn);
}

edilib::EdiSessionId_t SendEtRacRequest(const EtRacParams& racParams)
{
    EtRacRequest racReq(racParams);
    racReq.sendTlg();
    return racReq.ediSessId();
}

}//namespace edifact
