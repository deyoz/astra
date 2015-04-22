#pragma once

#include "IatciResponseHandler.h"


namespace TlgHandling {

class IatciPlfResponseHandler : public IatciResponseHandler
{
public:
    IatciPlfResponseHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                            const edilib::EdiSessRdData *edisess);

    virtual iatci::Result::Action_e action() const;
    virtual void parse();

    virtual ~IatciPlfResponseHandler() {}
};


}//namespace TlgHandling
