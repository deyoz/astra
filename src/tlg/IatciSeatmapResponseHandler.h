#pragma once

#include "IatciResponseHandler.h"
#include "iatci_types.h"
#include "edi_elements.h"


namespace TlgHandling
{

class IatciSeatmapResponseHandler : public IatciResponseHandler
{
public:
    IatciSeatmapResponseHandler(_EDI_REAL_MES_STRUCT_ *PMes,
                                const edilib::EdiSessRdData *edisess);

    virtual void parse();

    virtual ~IatciSeatmapResponseHandler() {}
};

}//namespace TlgHandling
