#pragma once

#include "ResponseHandler.h"

namespace TlgHandling
{

class EmdCosResponseHandler : public AstraEdiResponseHandler
{
public:
    EmdCosResponseHandler(_EDI_REAL_MES_STRUCT_ *pmes,
                          const edilib::EdiSessRdData *edisess);

    void parse();
    void handle();
    void onTimeOut() {}
    void onCONTRL() {}

    virtual ~EmdCosResponseHandler() {}
};

}//namespace TlgHandling
