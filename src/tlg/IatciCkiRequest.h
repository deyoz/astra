#pragma once

#include "EdifactRequest.h"
#include "iatci_types.h"


namespace edifact {

// Initial Through Check-in Interchange request
class CkiRequest: public EdifactRequest
{
    iatci::CkiParams m_params;

public:
    CkiRequest(const iatci::CkiParams& params,
               const std::string& pult,
               const std::string& ctxt,
               const KickInfo& kick);

    virtual std::string mesFuncCode() const;
    virtual std::string funcCode() const;
    virtual void collectMessage();

    virtual ~CkiRequest() {}
};

//-----------------------------------------------------------------------------

edilib::EdiSessionId_t SendCkiRequest(const iatci::CkiParams& params,
                                      const std::string& pult = "SYSTEM",
                                      const std::string& ctxt = "",
                                      const KickInfo& kick = KickInfo());

}//namespace edifact
