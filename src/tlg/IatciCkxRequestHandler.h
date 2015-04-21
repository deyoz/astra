#pragma once

#include "IatciRequestHandler.h"


namespace TlgHandling {

class IatciCkxRequestHandler: public IatciRequestHandler
{
public:
    IatciCkxRequestHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                           const edilib::EdiSessRdData *edisess);
    virtual void parse();
    virtual void handle();
    virtual void makeAnAnswer() {}
    virtual std::string respType() const;

    virtual ~IatciCkxRequestHandler() {}
};

}//namespace TlgHandling


