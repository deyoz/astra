#pragma once

#include "ResponseHandler.h"

namespace TlgHandling
{

class EtCosResponseHandler : public AstraEdiResponseHandler
{
public:
    EtCosResponseHandler(_EDI_REAL_MES_STRUCT_ *pmes,
                         const edilib::EdiSessRdData *edisess);

    virtual void parse() {}
    virtual void handle();
    virtual void onTimeOut();

    virtual ~EtCosResponseHandler() {}
};

}//namespace TlgHandling
