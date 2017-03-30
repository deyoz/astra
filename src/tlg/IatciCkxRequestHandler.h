#pragma once

#include "IatciRequestHandler.h"


namespace TlgHandling {

class IatciCkxRequestHandler: public IatciRequestHandler
{
    boost::optional<iatci::CkxParams> m_ckxParamsNew;

public:
    IatciCkxRequestHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                           const edilib::EdiSessRdData *edisess);

    virtual bool fullAnswer() const;
    virtual void parse();
    virtual std::string respType() const;

    virtual ~IatciCkxRequestHandler() {}

protected:
    virtual const iatci::IBaseParams* paramsNew() const;

    virtual iatci::dcrcka::Result handleRequest() const;
    virtual edilib::EdiSessionId_t sendCascadeRequest() const;
};

}//namespace TlgHandling


