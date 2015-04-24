#include "IatciCkxRequestHandler.h"
#include "IatciCkxRequest.h"
#include "iatci_api.h"

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace TlgHandling {

IatciCkxRequestHandler::IatciCkxRequestHandler(_EDI_REAL_MES_STRUCT_* pMes,
                                               const edilib::EdiSessRdData *edisess)
    : IatciRequestHandler(pMes, edisess)
{
}

void IatciCkxRequestHandler::parse()
{
    // TODO
    tst();
}

std::string IatciCkxRequestHandler::respType() const
{
    return "X";
}

boost::optional<iatci::Params> IatciCkxRequestHandler::params() const
{
    return ckxParams();
}

boost::optional<iatci::Params> IatciCkxRequestHandler::nextParams() const
{
    if(nextCkxParams()) {
        return *nextCkxParams();
    }

    return boost::none;
}

iatci::Result IatciCkxRequestHandler::handleRequest() const
{
   return iatci::cancelCheckin(ckxParams());
}

edilib::EdiSessionId_t IatciCkxRequestHandler::sendCascadeRequest() const
{
    ASSERT(nextCkxParams());
    return edifact::SendCkxRequest(*nextCkxParams());
}

const iatci::CkxParams IatciCkxRequestHandler::ckxParams() const
{
    ASSERT(m_ckxParams);
    return *m_ckxParams;
}

boost::optional<iatci::CkxParams> IatciCkxRequestHandler::nextCkxParams() const
{
    // TODO
    return boost::none;
}


}//namespace TlgHandling
