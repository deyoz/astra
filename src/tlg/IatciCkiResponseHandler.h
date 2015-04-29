#pragma once

#include "IatciResponseHandler.h"

#include <list>

namespace TlgHandling {

class IatciCkiResponseHandler : public IatciResponseHandler
{
public:
    IatciCkiResponseHandler(_EDI_REAL_MES_STRUCT_* pMes,
                            const edilib::EdiSessRdData* edisess);

    virtual iatci::Result::Action_e action() const;

    virtual ~IatciCkiResponseHandler() {}
};

}//namespace TlgHandling
