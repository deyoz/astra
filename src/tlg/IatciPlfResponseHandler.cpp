#include "IatciPlfResponseHandler.h"

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace TlgHandling {

IatciPlfResponseHandler::IatciPlfResponseHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                                                 const edilib::EdiSessRdData *edisess)
    : IatciResponseHandler(pMes, edisess)
{
}

iatci::dcrcka::Result::Action_e IatciPlfResponseHandler::action() const
{
    return iatci::dcrcka::Result::Passlist;
}

}//namespace TlgHandling
