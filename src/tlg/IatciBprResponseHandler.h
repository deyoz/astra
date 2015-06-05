#pragma once

#include "IatciResponseHandler.h"

#include <list>

namespace TlgHandling {

class IatciBprResponseHandler : public IatciResponseHandler
{
public:
    IatciBprResponseHandler(_EDI_REAL_MES_STRUCT_* pMes,
                            const edilib::EdiSessRdData* edisess);

    virtual iatci::Result::Action_e action() const;

    virtual ~IatciBprResponseHandler() {}
};

}//namespace TlgHandling
