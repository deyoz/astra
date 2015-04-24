#include "IatciCkuRequestHandler.h"
#include "IatciCkuRequest.h"

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace TlgHandling {

IatciCkuRequestHandler::IatciCkuRequestHandler(_EDI_REAL_MES_STRUCT_* pMes,
                                               const edilib::EdiSessRdData *edisess)
    : IatciRequestHandler(pMes, edisess)
{
}

void IatciCkuRequestHandler::parse()
{
    // TODO
    tst();
}

std::string IatciCkuRequestHandler::respType() const
{
    return "U";
}

boost::optional<iatci::Params> IatciCkuRequestHandler::params() const
{
    // TODO
    return boost::none;
}

boost::optional<iatci::Params> IatciCkuRequestHandler::nextParams() const
{
    // TODO
    return boost::none;
}

iatci::Result IatciCkuRequestHandler::handleRequest() const
{
    // TODO
    throw "";
}

edilib::EdiSessionId_t IatciCkuRequestHandler::sendCascadeRequest() const
{
    /*ASSERT(nextParams());
    return edifact::SendCkuRequest(*nextParams());*/
    // TODO
    throw "";
}

}//namespace TlgHandling
