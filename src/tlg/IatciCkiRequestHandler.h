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
    virtual std::string fcIndicator() const;

    virtual ~IatciCkiRequestHandler() {}

protected:
    virtual const iatci::IBaseParams* params() const;

    virtual std::list<iatci::dcrcka::Result> handleRequest() const;
};

}//namespace TlgHandling
