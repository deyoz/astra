#include "UacRequestHandler.h"
#include "view_edi_elements.h"
#include "etick.h"
#include "edi_tlg.h"
#include "edi_msg.h"

#include <edilib/edi_func_cpp.h>

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace TlgHandling {

using namespace edifact;
using namespace edilib;
using namespace Ticketing;
using namespace Ticketing::TickExceptions;



UacRequestHandler::UacRequestHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                                     const edilib::EdiSessRdData *edisess)
    : AstraEdiRequestHandler(pMes, edisess)
{}

std::string UacRequestHandler::mesFuncCode() const
{
    return "733";
}

bool UacRequestHandler::fullAnswer() const
{
    return false;
}

void UacRequestHandler::handle()
{
    using Ticketing::RemoteSystemContext::SystemContext;
    handleEtUac(SystemContext::Instance(STDLOG).inbTlgInfo().tlgSrc());
}

}//namespace TlgHandling
