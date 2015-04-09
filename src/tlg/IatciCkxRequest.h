#pragma once

#include "EdifactRequest.h"
#include "iatci_types.h"


namespace edifact {

// Through Check-in Cancel request
class CkxRequest: public EdifactRequest
{
    iatci::CkxParams m_params;
public:
    CkxRequest(const iatci::CkxParams& params,
               const std::string& pult,
               const std::string& ctxt,
               const KickInfo& kick);

    virtual std::string mesFuncCode() const;
    virtual std::string funcCode() const;
    virtual void collectMessage();

    virtual ~CkxRequest() {}
};

//-----------------------------------------------------------------------------

edilib::EdiSessionId_t SendCkxRequest(const iatci::CkxParams& params,
                                      const std::string& pult = "SYSTEM",
                                      const std::string& ctxt = "",
                                      const KickInfo& kick = KickInfo());

}//namespace edifact
