#pragma once

#include "IatciRequestHandler.h"


namespace TlgHandling {

class IatciCkxRequestHandler: public IatciRequestHandler
{
    boost::optional<iatci::CkxParams> m_ckxParams;

public:
    IatciCkxRequestHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                           const edilib::EdiSessRdData *edisess);

    virtual bool fullAnswer() const;
    virtual void parse();
    virtual std::string respType() const;
    virtual std::string fcIndicator() const;

    virtual ~IatciCkxRequestHandler() {}

protected:
    virtual const iatci::IBaseParams* params() const;

    virtual std::list<iatci::dcrcka::Result> handleRequest() const;
};

}//namespace TlgHandling


