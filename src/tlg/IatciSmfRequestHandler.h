#pragma once

#include "IatciSeatmapRequestHandler.h"


namespace TlgHandling {

class IatciSmfRequestHandler : public IatciSeatmapRequestHandler
{
    boost::optional<iatci::SmfParams> m_smfParams;

public:
    IatciSmfRequestHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                           const edilib::EdiSessRdData *edisess);

    virtual void parse();
    virtual std::string respType() const;

    virtual ~IatciSmfRequestHandler() {}

protected:
    virtual const iatci::IBaseParams* paramsNew() const;

    virtual iatci::dcrcka::Result handleRequest() const;
    virtual edilib::EdiSessionId_t sendCascadeRequest() const;
};

}//namespace TlgHandling
