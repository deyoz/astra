#pragma once

#include "emd_request.h"
#include <etick/tick_data.h>


namespace edifact
{

class EmdDisassociateRequestParams: public EmdRequestParams
{
    Ticketing::TicketCpn_t m_etTickCpn;
    Ticketing::TicketCpn_t m_emdTickCpn;
    Ticketing::CpnStatAction::CpnStatAction_t m_statAction;

public:
    EmdDisassociateRequestParams(const Ticketing::OrigOfRequest& org,
                                 const std::string& ctxt,
                                 const edifact::KickInfo &kickInfo,
                                 const std::string& airline,
                                 const Ticketing::FlightNum_t& flNum,
                                 const Ticketing::TicketCpn_t& etTickCpn,
                                 const Ticketing::TicketCpn_t& emdTickCpn,
                                 const Ticketing::CpnStatAction::CpnStatAction_t statAction)
        : EmdRequestParams(org, ctxt, kickInfo, airline, flNum),
          m_etTickCpn(etTickCpn), m_emdTickCpn(emdTickCpn), m_statAction(statAction)
    {}

    const Ticketing::TicketCpn_t& etTickCpn() const { return m_etTickCpn; }
    const Ticketing::TicketCpn_t& emdTickCpn() const { return m_emdTickCpn; }
    Ticketing::CpnStatAction::CpnStatAction_t emdStatAction() const { return m_statAction; }
};

//-----------------------------------------------------------------------------

class EmdDisassociateRequest: public EmdRequest
{
    EmdDisassociateRequestParams m_params;

public:
    EmdDisassociateRequest(const EmdDisassociateRequestParams& params);

    virtual std::string mesFuncCode() const;
    virtual void collectMessage();
};

}//namespace edifact
