#pragma once

#include "IatciRequestHandler.h"


namespace TlgHandling {

class IatciCkuRequestHandler: public IatciRequestHandler
{
    boost::optional<iatci::CkuParams> m_ckuParams;
public:
    IatciCkuRequestHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                           const edilib::EdiSessRdData *edisess);
    virtual void parse();
    virtual std::string respType() const;

    virtual ~IatciCkuRequestHandler() {}

protected:
    virtual boost::optional<iatci::Params> params() const;
    virtual boost::optional<iatci::Params> nextParams() const;
    virtual iatci::Result handleRequest() const;
    virtual edilib::EdiSessionId_t sendCascadeRequest() const;

private:
    const iatci::CkuParams& ckuParams() const;
    boost::optional<iatci::CkuParams> nextCkuParams() const;
};

}//namespace TlgHandling
