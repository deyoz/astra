#pragma once

#include "emd_request.h"


namespace edifact
{

class EmdDisassociateRequestParams: public EmdRequestParams
{
    Ticketing::TicketCpn_t m_etTickCpn;
    Ticketing::TicketCpn_t m_emdTickCpn;

public:
    EmdDisassociateRequestParams(const Ticketing::OrigOfRequest& org,
                                 const std::string& ctxt,
                                 const int reqCtxtId,
                                 const std::string& airline,
                                 const Ticketing::FlightNum_t& flNum,
                                 const Ticketing::TicketCpn_t& etTickCpn,
                                 const Ticketing::TicketCpn_t& emdTickCpn)
        : EmdRequestParams(org, ctxt, reqCtxtId, airline, flNum),
          m_etTickCpn(etTickCpn), m_emdTickCpn(emdTickCpn)
    {}

    const Ticketing::TicketCpn_t& etTickCpn() const { return m_etTickCpn; }
    const Ticketing::TicketCpn_t& emdTickCpn() const { return m_emdTickCpn; }
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
