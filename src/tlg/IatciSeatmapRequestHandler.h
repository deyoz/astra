#pragma once

#include "IatciRequestHandler.h"


namespace TlgHandling {

class IatciSeatmapRequestHandler : public IatciRequestHandler
{
public:
    IatciSeatmapRequestHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                               const edilib::EdiSessRdData *edisess);

    virtual void makeAnAnswer();

    virtual ~IatciSeatmapRequestHandler() {}
};

}//namespace TlgHandling
