#pragma once

#include "IatciRequestHandler.h"

#include <list>
#include <boost/optional.hpp>


namespace TlgHandling {

class IatciCkiRequestHandler: public IatciRequestHandler
{
    boost::optional<iatci::CkiParams> m_ckiParams;
    std::list<iatci::Result> m_lRes;

protected:
    iatci::CkiParams ckiParams() const;

    boost::optional<iatci::CkiParams> nextCkiParams(const iatci::FlightDetails& flightFromCurrHost) const;

public:
    IatciCkiRequestHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                           const edilib::EdiSessRdData *edisess);
    virtual void parse();
    virtual void handle();
    virtual void makeAnAnswer();
    virtual std::string respType() const;

    virtual ~IatciCkiRequestHandler() {}
};

}//namespace TlgHandling
