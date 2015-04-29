#include "IatciCkiResponseHandler.h"
#include "read_edi_elements.h"
#include "astra_msg.h"

#include <edilib/edi_func_cpp.h>

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace TlgHandling {

using namespace edilib;
using namespace edifact;
using namespace Ticketing;
using namespace Ticketing::TickReader;


IatciCkiResponseHandler::IatciCkiResponseHandler(_EDI_REAL_MES_STRUCT_* pMes,
                                                 const edilib::EdiSessRdData* edisess)
    : IatciResponseHandler(pMes, edisess)
{
}

iatci::Result::Action_e IatciCkiResponseHandler::action() const
{
    return iatci::Result::Checkin;
}

}//namespace TlgHandling
