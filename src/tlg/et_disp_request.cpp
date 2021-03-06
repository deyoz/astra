#include "et_disp_request.h"
#include "view_edi_elements.h"

#include <edilib/edi_func_cpp.h>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace edifact {

EtDispByNumRequest::EtDispByNumRequest(const EtDispByNumParams& dispParams)
    : EtRequest(dispParams), m_dispParams(dispParams)
{}

std::string EtDispByNumRequest::mesFuncCode() const
{
    return "131";
}

void EtDispByNumRequest::collectMessage()
{
    BaseTables::Router rot(sysCont()->routerCanonName());
    viewOrgElement(pMes(), m_dispParams.org(), rot->translit());
    edilib::SetEdiSegGr(pMes(), 1);
    edilib::SetEdiPointToSegGrW(pMes(), 1);
    TktElem tkt;
    tkt.m_ticketNum = m_dispParams.tickNum();
    viewTktElement(pMes(), tkt);
}

edilib::EdiSessionId_t SendEtDispByNumRequest(const EtDispByNumParams& dispParams)
{
    EtDispByNumRequest dispReq(dispParams);
    dispReq.sendTlg();
    return dispReq.ediSessId();
}

}//namespace edifact
