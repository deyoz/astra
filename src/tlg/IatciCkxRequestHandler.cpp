#include "IatciCkxRequestHandler.h"

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

void IatciCkxRequestHandler::handle()
{
    // TODO
    tst();
}

std::string IatciCkxRequestHandler::respType() const
{
    return "X";
}

}//namespace TlgHandling
