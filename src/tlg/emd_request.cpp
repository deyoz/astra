#include "emd_request.h"
#include "remote_system_context.h"

#include <edilib/edi_astra_msg_types.h>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace edifact
{
using namespace Ticketing;

Ticketing::RemoteSystemContext::SystemContext* EmdRequestParams::readSysCont() const
{
    return Ticketing::RemoteSystemContext::EdsSystemContext::read(airline(),
                                                                  flightNum());
}

//---------------------------------------------------------------------------------------

EmdRequest::EmdRequest(const EmdRequestParams& params)
    : EdifactRequest(params.org().pult(),
                     params.context(),
                     params.kickInfo(),
                     TKCREQ,
                     params.readSysCont())
{}

}//namespace edifact
