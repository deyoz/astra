#include "RequestHandler.h"
#include "remote_system_context.h"
#include "exceptions.h"
#include "edi_msg.h"

#include "exceptions.h"
#include <etick/exceptions.h>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>



namespace TlgHandling {

using namespace edifact;
using namespace Ticketing;
using namespace Ticketing::RemoteSystemContext;

AstraEdiRequestHandler::AstraEdiRequestHandler(_EDI_REAL_MES_STRUCT_* pMes,
                                               const edilib::EdiSessRdData *edisess)
    : edilib::EdiRequestHandler(pMes, edisess)
{
}

void AstraEdiRequestHandler::onParseError(const std::exception *e)
{
    // TODO
}

#define __CAST(type, var, c) const type *var = dynamic_cast<const type *>(c)

void AstraEdiRequestHandler::onHandlerError(const std::exception *e)
{
    edilib::EdiRequestHandler::onHandlerError(e);

    if(__CAST(TickExceptions::tick_exception, exc, e)) {
        ProgTrace(TRACE1, "errCode(): %s, errText(): %s", exc->errCode().c_str(),
                                                          exc->errText().c_str());
        if(exc->errCode() == AstraErr::EDI_PROC_ERR) {
            ProgError(STDLOG, "%s", exc->what());
        } else {
            WriteLog(STDLOG, "%s", exc->what());
        }
        saveErrorInfo(exc->errCode(), exc->errText());
    } else if(__CAST(EXCEPTIONS::Exception, exc, e)) {
        WriteLog(STDLOG, "%s", exc->what());
        saveErrorInfo(AstraErr::EDI_PROC_ERR, "Unable to process - internal error");
    } else {
        WriteLog(STDLOG,"std::exception: %s", e->what());
        saveErrorInfo(AstraErr::EDI_PROC_ERR, "");
    }
}

bool AstraEdiRequestHandler::needPutErrToQueue() const
{
    return false;
}

void AstraEdiRequestHandler::saveErrorInfo(const Ticketing::ErrMsg_t& errCode,
                                           const std::string& errText)
{
    setEdiErrorCode(StrUtils::ToUpper(getErcErrByInner(errCode)));
    setEdiErrorText(StrUtils::ToUpper(errText));
}

tlgnum_t AstraEdiRequestHandler::inboundTlgNum() const
{
    boost::optional<tlgnum_t> inbTlgNum = SystemContext::Instance(STDLOG).inbTlgInfo().tlgNum();
    ASSERT(inbTlgNum);
    return inbTlgNum.get();
}

}//namespace TlgHandling
