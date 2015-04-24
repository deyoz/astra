#pragma once

#include "IatciRequestHandler.h"


namespace TlgHandling {

class IatciPlfRequestHandler: public IatciRequestHandler
{
    boost::optional<iatci::PlfParams> m_plfParams;

public:
    IatciPlfRequestHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                           const edilib::EdiSessRdData *edisess);

    virtual void parse();
    virtual std::string respType() const;

    virtual ~IatciPlfRequestHandler() {}

protected:
    virtual boost::optional<iatci::Params> params() const;
    virtual boost::optional<iatci::Params> nextParams() const;
    virtual iatci::Result handleRequest() const;
    virtual edilib::EdiSessionId_t sendCascadeRequest() const;

private:
    const iatci::PlfParams& plfParams() const;
    boost::optional<iatci::PlfParams> nextPlfParams() const;
};

}//namespace TlgHandling
