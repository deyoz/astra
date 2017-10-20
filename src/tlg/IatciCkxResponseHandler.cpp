#include "IatciCkxResponseHandler.h"

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace TlgHandling {

IatciCkxResponseHandler::IatciCkxResponseHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                                                 const edilib::EdiSessRdData *edisess)
    : IatciResponseHandler(pMes, edisess)
{
}

iatci::dcrcka::Result::Action_e IatciCkxResponseHandler::action() const
{
    return iatci::dcrcka::Result::Cancel;
}

}//namespace TlgHandling
