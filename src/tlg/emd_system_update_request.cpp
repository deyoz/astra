#include "emd_system_update_request.h"
#include "remote_system_context.h"
#include "view_edi_elements.h"

#include <etick/tick_data.h>
#include <edilib/edi_func_cpp.h>
#include <edilib/edi_astra_msg_types.h>

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace edifact
{
using namespace Ticketing;

EmdDisassociateRequest::EmdDisassociateRequest(const EmdDisassociateRequestParams& params)
    : EmdRequest(params), m_params(params)
{
}

std::string EmdDisassociateRequest::mesFuncCode() const
{
    return "794";
}

static std::list<EqnElem> getEqnList(const EmdDisassociateRequestParams& params)
{
    std::list<EqnElem> lEqn;
    lEqn.push_back(EqnElem(1, "TD"));
    lEqn.push_back(EqnElem(1, "TF"));
    return lEqn;
}

static TktElem getTkt(const EmdDisassociateRequestParams& params)
{
    TktElem tkt;
    tkt.m_docType = DocType::EmdA;
    tkt.m_ticketNum = params.emdTickCpn().ticket();
    tkt.m_inConnectionTicketNum = params.etTickCpn().ticket();
    tkt.m_tickStatAction = TickStatAction::inConnectionWith;
    return tkt;
}

static CpnElem getCpn(const EmdDisassociateRequestParams& params)
{
    CpnElem cpn;
    cpn.m_num = params.emdTickCpn().cpn();
    cpn.m_connectedNum = params.etTickCpn().cpn();
    cpn.m_action = CpnStatAction::CpnActionStr(params.emdStatAction());
    return cpn;
}

void EmdDisassociateRequest::collectMessage()
{
    // ORG
    viewOrgElement(pMes(), m_params.org());
    // EQN
    viewEqnElement(pMes(), getEqnList(m_params));
    edilib::SetEdiSegGr(pMes(), 1);
    edilib::SetEdiPointToSegGrW(pMes(), 1);
    // TKT
    viewTktElement(pMes(), getTkt(m_params));
    edilib::SetEdiSegGr(pMes(), 2);
    edilib::SetEdiPointToSegGrW(pMes(), 2);
    // CPN
    viewCpnElement(pMes(), getCpn(m_params));
}

}//namespace edifact
