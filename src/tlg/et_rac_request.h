#pragma once

#include "et_request.h"


namespace edifact {

class EtRacParams: public EtRequestParams
{
    Ticketing::TicketNum_t m_tickNum;
    Ticketing::CouponNum_t m_cpnNum;
public:
    EtRacParams(const Ticketing::OrigOfRequest& org,
                const std::string& ctxt,
                const edifact::KickInfo& kickInfo,
                const std::string& airline,
                const Ticketing::FlightNum_t& flNum,
                const Ticketing::TicketNum_t& tickNo,
                const Ticketing::CouponNum_t& cpnNo)
        : EtRequestParams(org, ctxt, kickInfo, airline, flNum),
          m_tickNum(tickNo), m_cpnNum(cpnNo)
    {}

    const Ticketing::TicketNum_t& tickNum() const { return m_tickNum; }
    const Ticketing::CouponNum_t&  cpnNum() const { return m_cpnNum; }
};

//---------------------------------------------------------------------------------------

class EtRacRequest: public EtRequest
{
    EtRacParams m_racParams;
public:
    EtRacRequest(const EtRacParams& racParams);

    virtual std::string mesFuncCode() const;
    virtual void collectMessage();

    virtual ~EtRacRequest() {}
};

edilib::EdiSessionId_t SendEtRacRequest(const EtRacParams& racParams);

}//namespace edifact
