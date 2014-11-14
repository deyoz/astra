#pragma once

#include "ResponseHandler.h"

namespace TlgHandling
{

class EmdDispResponseHandler : public AstraEdiResponseHandler
{
public:
    EmdDispResponseHandler(_EDI_REAL_MES_STRUCT_ *pmes,
                           const edilib::EdiSessRdData *edisess);

    void parse();
    void handle();
    void onTimeOut() {}
    void onCONTRL() {}

    virtual ~EmdDispResponseHandler() {}
};

}//namespace TlgHandling
