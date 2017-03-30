#pragma once

#include "IatciRequestHandler.h"


namespace TlgHandling {

class IatciCkiRequestHandler: public IatciRequestHandler
{
    boost::optional<iatci::CkiParams> m_ckiParamsNew;

public:
    IatciCkiRequestHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                           const edilib::EdiSessRdData *edisess);
    virtual void parse();
    virtual std::string respType() const;

    virtual ~IatciCkiRequestHandler() {}

protected:
    virtual const iatci::IBaseParams* paramsNew() const;

    virtual iatci::dcrcka::Result handleRequest() const;
    virtual edilib::EdiSessionId_t sendCascadeRequest() const;
};

}//namespace TlgHandling
