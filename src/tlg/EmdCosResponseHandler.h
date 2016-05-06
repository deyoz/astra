#pragma once

#include "ResponseHandler.h"

namespace TlgHandling
{

class EmdCosResponseHandler : public AstraEdiResponseHandler
{
public:
    EmdCosResponseHandler(_EDI_REAL_MES_STRUCT_ *pmes,
                          const edilib::EdiSessRdData *edisess);

    void parse() {}
    void handle();

    virtual ~EmdCosResponseHandler() {}
};

}//namespace TlgHandling
