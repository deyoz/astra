#include "RequestHandler.h"
#include "exceptions.h"
#include "edi_msg.h"

#include <etick/exceptions.h>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>



namespace TlgHandling {

using namespace Ticketing;

AstraRequestHandler::AstraRequestHandler(_EDI_REAL_MES_STRUCT_* pMes,
                                         const edilib::EdiSessRdData *edisess)
    : edilib::EdiRequestHandler(pMes, edisess)
{
}

void AstraRequestHandler::onParseError(const std::exception *e)
{
    // TODO
}

#define __CAST(type, var, c) const type *var = dynamic_cast<const type *>(c)

void AstraRequestHandler::onHandlerError(const std::exception *e)
{
    edilib::EdiRequestHandler::onHandlerError(e);

    if(__CAST(TickExceptions::tick_exception, exc, e)) {
        ProgTrace(TRACE1, "errCode(): %s", exc->errCode().c_str());
        if(exc->errCode() == AstraErr::EDI_PROC_ERR) {
            ProgError(STDLOG, "%s", exc->what());
        } else {
            WriteLog(STDLOG, "%s", exc->what());
        }
        saveErrorInfo(exc->errCode(), exc->what());
    } else if(__CAST(EXCEPTIONS::Exception, exc, e)) {
        ProgError(STDLOG, "%s", exc->what());
        saveErrorInfo(AstraErr::EDI_PROC_ERR, exc->what());
    } else {
        ProgError(STDLOG,"std::exception: %s", e->what());
        saveErrorInfo(AstraErr::EDI_PROC_ERR, "");
    }
}

bool AstraRequestHandler::needPutErrToQueue() const
{
    return false;
}

}//namespace TlgHandling
