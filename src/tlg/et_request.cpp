#include "et_request.h"
#include "remote_system_context.h"

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace edifact {

const Ticketing::RemoteSystemContext::SystemContext* EtRequestParams::readSysCont() const
{
    return Ticketing::RemoteSystemContext::EdsSystemContext::read(airline(),
                                                                  flightNum());
}

//---------------------------------------------------------------------------------------

EtRequest::EtRequest(const EtRequestParams& params)
    : EdifactRequest(params.org().pult(),
                     params.context(),
                     params.kickInfo(),
                     TKCREQ,
                     params.readSysCont())
{}

}//namespace edifact
