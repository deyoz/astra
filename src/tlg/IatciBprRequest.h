#pragma once

#include "EdifactRequest.h"
#include "iatci_types.h"


namespace edifact {

// Boarding Pass Reprint request
class BprRequest: public EdifactRequest
{
    iatci::BprParams m_params;
public:
    BprRequest(const iatci::BprParams& params,
               const std::string& pult,
               const std::string& ctxt,
               const KickInfo& kick);

    virtual std::string mesFuncCode() const;
    virtual std::string funcCode() const;
    virtual void collectMessage();

    virtual ~BprRequest() {}
};

//-----------------------------------------------------------------------------

edilib::EdiSessionId_t SendBprRequest(const iatci::BprParams& params,
                                      const std::string& pult = "SYSTEM",
                                      const std::string& ctxt = "",
                                      const KickInfo& kick = KickInfo());

}//namespace edifact
