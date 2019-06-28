#pragma once

#include "ResponseHandler.h"


namespace TlgHandling
{

class CusResponseHandler: public AstraEdiResponseHandler
{
public:
    CusResponseHandler(_EDI_REAL_MES_STRUCT_ *PMes,
                       const edilib::EdiSessRdData *edisess);

    virtual void fillFuncCodeRespStatus() {}
    virtual void fillErrorDetails() {}
    virtual void parse();
    virtual void handle() {}
    virtual void onTimeOut() {}
    virtual void onCONTRL() {}

    virtual ~CusResponseHandler() {}
};

}//namespace TlgHandling
