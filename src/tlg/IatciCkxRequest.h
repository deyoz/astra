#pragma once

#include "EdifactRequest.h"
#include "iatci_types.h"


namespace edifact {

// Through Check-in Cancel request
class CkxRequest: public EdifactRequest
{
    iatci::CkxParams m_params;
public:
    CkxRequest(const iatci::CkxParams& params, const KickInfo& kick);

    virtual std::string mesFuncCode() const;
    virtual void collectMessage();

    virtual ~CkxRequest() {}
};

//-----------------------------------------------------------------------------

edilib::EdiSessionId_t SendCkxRequest(const iatci::CkxParams& params,
                                      const KickInfo& kick = KickInfo());

}//namespace edifact
