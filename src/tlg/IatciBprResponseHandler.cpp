#include "IatciBprResponseHandler.h"

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace TlgHandling {

IatciBprResponseHandler::IatciBprResponseHandler(_EDI_REAL_MES_STRUCT_* pMes,
                                                 const edilib::EdiSessRdData* edisess)
    : IatciResponseHandler(pMes, edisess)
{
}

iatci::Result::Action_e IatciBprResponseHandler::action() const
{
    return iatci::Result::Reprint;
}

}//namespace TlgHandling
