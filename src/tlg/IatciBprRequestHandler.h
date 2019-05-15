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
    virtual std::string fcIndicator() const;

    virtual ~IatciBprRequestHandler() {}

protected:
    virtual const iatci::IBaseParams* params() const;

    virtual std::list<iatci::dcrcka::Result> handleRequest() const;
};

}//namespace TlgHandling
