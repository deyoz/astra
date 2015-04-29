#include "IatciPlfResponseHandler.h"
#include "read_edi_elements.h"
#include "postpone_edifact.h"
#include "iatci_api.h"

#include <edilib/edi_func_cpp.h>

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace TlgHandling {

using namespace edilib;
using namespace edifact;
using namespace Ticketing::TickReader;

IatciPlfResponseHandler::IatciPlfResponseHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                                                 const edilib::EdiSessRdData *edisess)
    : IatciResponseHandler(pMes, edisess)
{
}

iatci::Result::Action_e IatciPlfResponseHandler::action() const
{
    return iatci::Result::Passlist;
}

}//namespace TlgHandling
