#pragma once

#include "IatciRequestHandler.h"


namespace TlgHandling {

class IatciCkuRequestHandler: public IatciRequestHandler
{
public:
    IatciCkuRequestHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                           const edilib::EdiSessRdData *edisess);
    virtual void parse();
    virtual void handle();
    virtual std::string respType() const;

    virtual ~IatciCkuRequestHandler() {}
};

}//namespace TlgHandling


