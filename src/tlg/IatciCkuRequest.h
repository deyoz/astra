#pragma once

#include "EdifactRequest.h"
#include "iatci_types.h"


namespace edifact {

// Through Check-in Cancel request
class CkuRequest: public EdifactRequest
{
    iatci::CkuParams m_params;
public:
    CkuRequest(const iatci::CkuParams& params,
               const std::string& pult,
               const std::string& ctxt,
               const KickInfo& kick);

    virtual std::string mesFuncCode() const;
    virtual std::string funcCode() const;
    virtual void collectMessage();

    virtual ~CkuRequest() {}
};

//-----------------------------------------------------------------------------

edilib::EdiSessionId_t SendCkuRequest(const iatci::CkuParams& params,
                                      const std::string& pult = "SYSTEM",
                                      const std::string& ctxt = "",
                                      const KickInfo& kick = KickInfo());

}//namespace edifact
