#pragma once

#include "IatciRequestHandler.h"


namespace TlgHandling {

class IatciCkuRequestHandler: public IatciRequestHandler
{
    boost::optional<iatci::CkuParams> m_ckuParams;

public:
    IatciCkuRequestHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                           const edilib::EdiSessRdData *edisess);
    virtual void parse();
    virtual std::string respType() const;

    virtual ~IatciCkuRequestHandler() {}

protected:
    virtual const iatci::IBaseParams* paramsNew() const;

    virtual iatci::dcrcka::Result handleRequest() const;
    virtual edilib::EdiSessionId_t sendCascadeRequest() const;
};

}//namespace TlgHandling
