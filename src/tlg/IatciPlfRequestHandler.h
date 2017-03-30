#pragma once

#include "IatciRequestHandler.h"


namespace TlgHandling {

class IatciPlfRequestHandler: public IatciRequestHandler
{
    boost::optional<iatci::PlfParams> m_plfParams;

public:
    IatciPlfRequestHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                           const edilib::EdiSessRdData *edisess);

    virtual void parse();
    virtual std::string respType() const;

    virtual ~IatciPlfRequestHandler() {}

protected:
    virtual const iatci::IBaseParams* paramsNew() const;

    virtual iatci::dcrcka::Result handleRequest() const;
    virtual edilib::EdiSessionId_t sendCascadeRequest() const;
};

}//namespace TlgHandling
