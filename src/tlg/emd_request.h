#pragma once

#include "EdifactRequest.h"


namespace edifact {

class EmdRequestParams: public RequestParams
{
public:
    EmdRequestParams(const Ticketing::OrigOfRequest& org,
                     const std::string& ctxt,
                     const edifact::KickInfo &kickInfo,
                     const std::string& airline,
                     const Ticketing::FlightNum_t& flNum,
                     const boost::optional<std::string>& specBaseOurrefName = boost::none)
        : RequestParams(org, ctxt, kickInfo, airline, flNum, specBaseOurrefName)
    {}

    virtual ~EmdRequestParams() {}

    virtual Ticketing::RemoteSystemContext::SystemContext* readSysCont() const;
};

//-----------------------------------------------------------------------------

class EmdRequest: public EdifactRequest
{
public:
    EmdRequest(const EmdRequestParams& params);
    virtual ~EmdRequest() {}
};

}//namespace edifact
