#pragma once

#include "ResponseHandler.h"

namespace TlgHandling {

class EmdSysUpdateResponseHandler : public AstraEdiResponseHandler
{
public:
    EmdSysUpdateResponseHandler(_EDI_REAL_MES_STRUCT_* pMes,
                           const edilib::EdiSessRdData* edisess);

    void handle();
    void parse() {}

    virtual ~EmdSysUpdateResponseHandler() {}
};

}//namespace TlgHandling
