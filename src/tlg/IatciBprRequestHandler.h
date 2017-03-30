#pragma once

#include "IatciRequestHandler.h"


namespace TlgHandling {

class IatciBprRequestHandler: public IatciRequestHandler
{
    boost::optional<iatci::BprParams> m_bprParamsNew;

public:
    IatciBprRequestHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                           const edilib::EdiSessRdData *edisess);
    virtual void parse();
    virtual std::string respType() const;

    virtual ~IatciBprRequestHandler() {}

protected:
    virtual const iatci::IBaseParams* paramsNew() const;

    virtual iatci::dcrcka::Result handleRequest() const;
    virtual edilib::EdiSessionId_t sendCascadeRequest() const;
};

}//namespace TlgHandling
