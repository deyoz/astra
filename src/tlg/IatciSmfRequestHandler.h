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
    virtual boost::optional<iatci::BaseParams> params() const; // TODO убрать optional потом
    virtual boost::optional<iatci::BaseParams> nextParams() const;
    virtual iatci::Result handleRequest() const;
    virtual edilib::EdiSessionId_t sendCascadeRequest() const;

private:
    const iatci::SmfParams& smfParams() const;
    boost::optional<iatci::SmfParams> nextSmfParams() const;
};

}//namespace TlgHandling
