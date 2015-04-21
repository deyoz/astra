#pragma once

#include "RequestHandler.h"
#include "iatci_types.h"

namespace TlgHandling {

class IatciRequestHandler: public AstraRequestHandler
{
    std::string m_reqRef;
    std::string m_ediErrorLevel;

protected:
    std::list<iatci::Result> m_lRes;

public:
    IatciRequestHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                        const edilib::EdiSessRdData *edisess);
    virtual std::string mesFuncCode() const;
    virtual std::string respType() const = 0;    // Значение RAD:9868
    virtual void loadDeferredData();

    virtual void saveErrorInfo(const Ticketing::ErrMsg_t& errCode,
                               const std::string& errText);

    virtual void setEdiErrorLevel(const std::string& errLevel);
    virtual const std::string& ediErrorLevel() const;

    virtual ~IatciRequestHandler() {}
};

}//namespace TlgHandling
