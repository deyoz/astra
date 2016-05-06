#pragma once

#include "ResponseHandler.h"

namespace TlgHandling
{

class EtCosResponseHandler : public AstraEdiResponseHandler
{
public:
    EtCosResponseHandler(_EDI_REAL_MES_STRUCT_ *pmes,
                         const edilib::EdiSessRdData *edisess);

    void parse() {}
    void handle();

    virtual ~EtCosResponseHandler() {}
};

}//namespace TlgHandling
