#pragma once

#include "ResponseHandler.h"

namespace TlgHandling
{

class EmdSysUpdateResponseHandler : public AstraEdiResponseHandler
{
public:
    EmdSysUpdateResponseHandler(_EDI_REAL_MES_STRUCT_ *pmes,
                           const edilib::EdiSessRdData *edisess);

    void parse();
    void handle();
    void onTimeOut();
    void onCONTRL();

    virtual ~EmdSysUpdateResponseHandler() {}
};

}//namespace TlgHandling
