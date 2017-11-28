#pragma once

#include "EdifactRequest.h"
#include "iatci_types.h"


namespace edifact {

// Seat Map Function Request
class SmfRequest: public EdifactRequest
{
    iatci::SmfParams m_params;
public:
    SmfRequest(const iatci::SmfParams& params,
               const std::string& pult,
               const std::string& ctxt,
               const KickInfo& kick);

    virtual std::string mesFuncCode() const;
    virtual std::string funcCode() const;
    virtual void collectMessage();

    virtual ~SmfRequest() {}
};

//-----------------------------------------------------------------------------

edilib::EdiSessionId_t SendSmfRequest(const iatci::SmfParams& params,
                                      const std::string& pult = "SYSTEM",
                                      const std::string& ctxt = "",
                                      const KickInfo& kick = KickInfo());

}//namespace edifact
