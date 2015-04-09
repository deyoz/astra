#include "RequestHandler.h"

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace TlgHandling {

AstraRequestHandler::AstraRequestHandler(_EDI_REAL_MES_STRUCT_* pMes,
                                         const edilib::EdiSessRdData *edisess)
    : edilib::EdiRequestHandler(pMes, edisess)
{
}

void AstraRequestHandler::onParseError(const std::exception *e)
{
    // TODO
}

void AstraRequestHandler::onHandlerError(const std::exception *e)
{
    // TODO
}

bool AstraRequestHandler::needPutErrToQueue() const
{
    return false;
}

}//namespace TlgHandling
