#pragma once

#include "IatciRequestHandler.h"


namespace TlgHandling {

class IatciCkxRequestHandler: public IatciRequestHandler
{
    boost::optional<iatci::CkxParams> m_ckxParams;

public:
    IatciCkxRequestHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                           const edilib::EdiSessRdData *edisess);

    virtual bool fullAnswer() const;
    virtual void parse();
    virtual std::string respType() const;

    virtual ~IatciCkxRequestHandler() {}

protected:
    virtual boost::optional<iatci::BaseParams> params() const;
    virtual boost::optional<iatci::BaseParams> nextParams() const;
    virtual iatci::Result handleRequest() const;
    virtual edilib::EdiSessionId_t sendCascadeRequest() const;

private:
    const iatci::CkxParams ckxParams() const;
    boost::optional<iatci::CkxParams> nextCkxParams() const;
};

}//namespace TlgHandling


