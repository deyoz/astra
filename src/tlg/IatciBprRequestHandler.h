#pragma once

#include "IatciRequestHandler.h"


namespace TlgHandling {

class IatciBprRequestHandler: public IatciRequestHandler
{
    boost::optional<iatci::BprParams> m_bprParams;
public:
    IatciBprRequestHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                           const edilib::EdiSessRdData *edisess);
    virtual void parse();
    virtual std::string respType() const;

    virtual ~IatciBprRequestHandler() {}

protected:
    virtual boost::optional<iatci::Params> params() const;
    virtual boost::optional<iatci::Params> nextParams() const;
    virtual iatci::Result handleRequest() const;
    virtual edilib::EdiSessionId_t sendCascadeRequest() const;

private:
    const iatci::BprParams& bprParams() const;
    boost::optional<iatci::BprParams> nextBprParams() const;
};

}//namespace TlgHandling
