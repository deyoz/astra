#include "EmdDispResponseHandler.h"
#include "remote_system_context.h"
#include "emdoc.h"

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace TlgHandling
{

namespace
{
    inline std::string getTlgSrc()
    {
        return Ticketing::RemoteSystemContext::SystemContext::Instance(STDLOG).inbTlgInfo().tlgSrc();
    }
}//namespace


EmdDispResponseHandler::EmdDispResponseHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                                               const edilib::EdiSessRdData *edisess)
    : AstraEdiResponseHandler(pMes, edisess)
{
}

void EmdDispResponseHandler::handle()
{
  try
  {
    using namespace edifact;

    switch(respStatus().status())
    {
    case edilib::EdiRespStatus::successfully:
        if(remoteResults())
        {
            LogTrace(TRACE3) << "set tlg source: " << getTlgSrc();
            remoteResults()->setTlgSource(getTlgSrc());
            handleEmdDispResponse(*remoteResults());
        }
        break;
    case edilib::EdiRespStatus::partial:
    case edilib::EdiRespStatus::notProcessed:
    case edilib::EdiRespStatus::rejected:
        break;
    }
  }
  catch(std::exception &e)
  {
    ProgError(STDLOG, "EmdDispResponseHandler::handle: %s", e.what());
  };

}

}//namespace TlgHandling
