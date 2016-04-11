#pragma once

#include "et_request.h"

namespace edifact {

enum EtDispType_e {
    etDispByNum,
};

//---------------------------------------------------------------------------------------

class EtDispParams: public EtRequestParams
{
public:
    EtDispParams(const Ticketing::OrigOfRequest &org,
                 const std::string &ctxt,
                 const edifact::KickInfo &kickInfo,
                 const std::string& airline,
                 const Ticketing::FlightNum_t& flNum)
        : EtRequestParams(org, ctxt, kickInfo, airline, flNum)
    {}

    virtual ~EtDispParams() {}

    virtual EtDispType_e dispType() const = 0;
};

//---------------------------------------------------------------------------------------

class EtDispByNumParams: public EtDispParams
{
    Ticketing::TicketNum_t m_tickNum;
public:
    EtDispByNumParams(const Ticketing::OrigOfRequest &org,
                      const std::string &ctxt,
                      const edifact::KickInfo &kickInfo,
                      const std::string& airline,
                      const Ticketing::FlightNum_t& flNum,
                      const Ticketing::TicketNum_t& ticknum)
        : EtDispParams(org, ctxt, kickInfo, airline, flNum),
          m_tickNum(ticknum)
    {}

    const Ticketing::TicketNum_t& tickNum() const { return m_tickNum; }
    virtual EtDispType_e dispType() const { return etDispByNum; }
};

//---------------------------------------------------------------------------------------

class EtDispByNumRequest: public EtRequest
{
    EtDispByNumParams m_dispParams;
public:
    EtDispByNumRequest(const EtDispByNumParams& dispParams);

    virtual std::string mesFuncCode() const;
    virtual void collectMessage();

    virtual ~EtDispByNumRequest() {}
};

}//namespace edifact
