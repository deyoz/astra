#pragma once

#include "IatciResponseHandler.h"


namespace TlgHandling {

class IatciCkuResponseHandler : public IatciResponseHandler
{
public:
    IatciCkuResponseHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                            const edilib::EdiSessRdData *edisess);

    virtual iatci::Result::Action_e action() const;
    virtual void parse();

    virtual ~IatciCkuResponseHandler() {}
};

}//namespace TlgHandling
