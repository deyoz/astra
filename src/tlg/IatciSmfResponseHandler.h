#pragma once

#include "IatciSeatmapResponseHandler.h"


namespace TlgHandling {

class IatciSmfResponseHandler : public IatciSeatmapResponseHandler
{
public:
    IatciSmfResponseHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                            const edilib::EdiSessRdData *edisess);

    virtual iatci::Result::Action_e action() const;

    virtual ~IatciSmfResponseHandler() {}
};

}//namespace TlgHandling
