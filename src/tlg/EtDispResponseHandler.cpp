#include "EtDispResponseHandler.h"
#include "remote_system_context.h"
#include "etick.h"

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


EtDispResponseHandler::EtDispResponseHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                                             const edilib::EdiSessRdData *edisess)
    : AstraEdiResponseHandler(pMes, edisess)
{
}

void EtDispResponseHandler::handle()
{
  try
  {
      using namespace edifact;

      switch(respStatus().status())
      {
      case edilib::EdiRespStatus::successfully:
      case edilib::EdiRespStatus::notProcessed:
          if(remoteResults())
          {
              remoteResults()->setTlgSource(getTlgSrc());
              handleEtDispResponse(*remoteResults());
          }
          break;
      case edilib::EdiRespStatus::partial:
      case edilib::EdiRespStatus::rejected:
          tst();
          break;
      }
  }
  catch(std::exception &e)
  {
      ProgError(STDLOG, "EtDispResponseHandler::handle: %s", e.what());
  }
}

}//namespace TlgHandling
