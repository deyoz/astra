#pragma once

#include "ResponseHandler.h"


namespace TlgHandling {

class EtDispResponseHandler : public AstraEdiResponseHandler
{
public:
    EtDispResponseHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                          const edilib::EdiSessRdData *edisess);

    void handle();
    void parse() {}

    virtual ~EtDispResponseHandler() {}
};

}//namespace TlgHandling
