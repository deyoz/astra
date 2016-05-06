#pragma once

#include "ResponseHandler.h"

namespace TlgHandling {

class EmdDispResponseHandler : public AstraEdiResponseHandler
{
public:
    EmdDispResponseHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                           const edilib::EdiSessRdData *edisess);

    void handle();
    void parse() {}

    virtual ~EmdDispResponseHandler() {}
};

}//namespace TlgHandling
