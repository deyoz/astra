#pragma once

#include "ResponseHandler.h"


namespace TlgHandling {

class EtRacResponseHandler: public AstraEdiResponseHandler
{
public:
    EtRacResponseHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                         const edilib::EdiSessRdData *edisess);

    void handle();
    void parse() {}

    virtual ~EtRacResponseHandler() {}
};

}//namespace TlgHandling
