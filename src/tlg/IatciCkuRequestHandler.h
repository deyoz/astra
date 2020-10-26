#pragma once

#include "IatciRequestHandler.h"


namespace TlgHandling {

class IatciCkuRequestHandler: public IatciRequestHandler
{
    boost::optional<iatci::CkuParams> m_ckuParams;

public:
    IatciCkuRequestHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                           const edilib::EdiSessRdData *edisess);

    virtual bool fullAnswer() const;
    virtual void parse();
    virtual std::string respType() const;
    virtual std::string fcIndicator() const;

    virtual ~IatciCkuRequestHandler() {}

protected:
    virtual const iatci::IBaseParams* params() const;

    virtual std::list<iatci::dcrcka::Result> handleRequest() const;
};

}//namespace TlgHandling
