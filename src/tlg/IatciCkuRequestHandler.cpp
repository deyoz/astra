#include "IatciCkuRequestHandler.h"

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

void IatciCkuRequestHandler::handle()
{
    // TODO
    tst();
}

std::string IatciCkuRequestHandler::respType() const
{
    return "U";
}

}//namespace TlgHandling
