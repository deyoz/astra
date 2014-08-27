#pragma once

#include "edi_tkt_request.h"
#include "EdifactRequest.h"


namespace edifact
{

class EmdRequestParams: public edi_common_data
{
    std::string m_airline;
    Ticketing::FlightNum_t m_flNum;

public:
    EmdRequestParams(const Ticketing::OrigOfRequest& org,
                     const std::string& ctxt,
                     const int reqCtxtId,
                     const std::string& airline,
                     const Ticketing::FlightNum_t& flNum)
        : edi_common_data(org, ctxt, reqCtxtId),
          m_airline(airline), m_flNum(flNum)
    {}


    const std::string& airline() const { return m_airline; }
    const Ticketing::FlightNum_t& flightNum() const { return m_flNum; }
};

//-----------------------------------------------------------------------------

class EmdRequest: public EdifactRequest
{
public:
    EmdRequest(const EmdRequestParams& params);
    virtual ~EmdRequest() {}
};

}//namespace edifact
