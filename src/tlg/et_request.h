#pragma once

#include "EdifactRequest.h"


namespace edifact {

class EtRequestParams: public RequestParams
{
public:
    EtRequestParams(const Ticketing::OrigOfRequest& org,
                    const std::string& ctxt,
                    const edifact::KickInfo& kickInfo,
                    const std::string& airline,
                    const Ticketing::FlightNum_t& flNum)
        : RequestParams(org, ctxt, kickInfo, airline, flNum)
    {}

    virtual ~EtRequestParams() {}

    virtual const Ticketing::RemoteSystemContext::SystemContext* readSysCont() const;
};

//---------------------------------------------------------------------------------------

class EtRequest: public EdifactRequest
{
public:
    EtRequest(const EtRequestParams& params);
    virtual ~EtRequest() {}
};

}//namespace edifact
