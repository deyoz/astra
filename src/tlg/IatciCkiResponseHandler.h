#pragma once

#include "IatciResponseHandler.h"

#include <list>

namespace TlgHandling {

class IatciCkiResponseHandler : public IatciResponseHandler
{
public:
    IatciCkiResponseHandler(_EDI_REAL_MES_STRUCT_* pMes,
                            const edilib::EdiSessRdData* edisess);

    virtual void parse();
    virtual void onTimeOut();

    virtual ~IatciCkiResponseHandler() {}
};

}//namespace TlgHandling
