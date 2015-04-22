#pragma once

#include "EdifactRequest.h"
#include "iatci_types.h"


namespace edifact {

// Passenger List Function Interchange request
class PlfRequest: public EdifactRequest
{
    iatci::PlfParams m_params;
public:
    PlfRequest(const iatci::PlfParams& params,
               const std::string& pult,
               const std::string& ctxt,
               const KickInfo& kick);

    virtual std::string mesFuncCode() const;
    virtual std::string funcCode() const;
    virtual void collectMessage();

    virtual ~PlfRequest() {}
};

//-----------------------------------------------------------------------------

edilib::EdiSessionId_t SendPlfRequest(const iatci::PlfParams& params,
                                      const std::string& pult = "SYSTEM",
                                      const std::string& ctxt = "",
                                      const KickInfo& kick = KickInfo());

}//namespace edifact
