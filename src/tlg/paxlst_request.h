#pragma once

#include "EdifactRequest.h"
#include "apis_edi_file.h"

#include <memory>

namespace edifact {

class PaxlstReqParams: public RequestParams
{
    Paxlst::PaxlstInfo m_paxlstInfo;
public:
    PaxlstReqParams(const std::string& airline,
                    const Paxlst::PaxlstInfo& paxlstInfo);

    const Paxlst::PaxlstInfo& paxlst() const;

    virtual const Ticketing::RemoteSystemContext::SystemContext* readSysCont() const;

    virtual ~PaxlstReqParams() {}
};

//---------------------------------------------------------------------------------------

class PaxlstRequest: public EdifactRequest
{
    std::shared_ptr<Paxlst::PaxlstInfo> m_paxlstInfo;

public:
    PaxlstRequest(const PaxlstReqParams& params);

    virtual std::string mesFuncCode() const;
    virtual std::string funcCode() const;
    virtual void updateMesHead();
    virtual void collectMessage();
    virtual bool needRemoteResults() const;
    virtual bool needSaveEdiSessionContext() const;
    virtual bool needConfigAgentToWait() const;

    virtual ~PaxlstRequest() {}
};

}//namespace edifact
