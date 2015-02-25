#include "EmdDispResponseHandler.h"
#include "remote_system_context.h"

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


EmdDispResponseHandler::EmdDispResponseHandler(_EDI_REAL_MES_STRUCT_ *pmes,
                                               const edilib::EdiSessRdData *edisess)
    : AstraEdiResponseHandler(pmes, edisess)
{
}

void EmdDispResponseHandler::parse()
{
}

void EmdDispResponseHandler::handle()
{
    using namespace edifact;

    switch(respStatus().status())
    {
    case edilib::EdiRespStatus::successfully:
        if(remoteResults())
        {
            LogTrace(TRACE3) << "set tlg source: " << getTlgSrc();
            remoteResults()->setTlgSource(getTlgSrc());
        }
        break;
    case edilib::EdiRespStatus::partial:        
    case edilib::EdiRespStatus::unsuccessfully:
        break;
    }
}

}//namespace TlgHandling
