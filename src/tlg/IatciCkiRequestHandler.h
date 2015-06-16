#pragma once

#include "IatciRequestHandler.h"


namespace TlgHandling {

class IatciCkiRequestHandler: public IatciRequestHandler
{
    boost::optional<iatci::CkiParams> m_ckiParams;

public:
    IatciCkiRequestHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                           const edilib::EdiSessRdData *edisess);
    virtual void parse();
    virtual std::string respType() const;
    virtual ~IatciCkiRequestHandler() {}

protected:
    virtual boost::optional<iatci::BaseParams> params() const;
    virtual boost::optional<iatci::BaseParams> nextParams() const;
    virtual iatci::Result handleRequest() const;
    virtual edilib::EdiSessionId_t sendCascadeRequest() const;

private:
    const iatci::CkiParams& ckiParams() const;
    boost::optional<iatci::CkiParams> nextCkiParams() const;
};

}//namespace TlgHandling
