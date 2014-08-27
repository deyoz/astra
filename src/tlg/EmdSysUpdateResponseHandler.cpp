#include "EmdSysUpdateResponseHandler.h"

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace TlgHandling
{

EmdSysUpdateResponseHandler::EmdSysUpdateResponseHandler(_EDI_REAL_MES_STRUCT_ *pmes,
                                                         const edilib::EdiSessRdData *edisess)
    : AstraEdiResponseHandler(pmes, edisess)
{
}

}//namespace TlgHandling
