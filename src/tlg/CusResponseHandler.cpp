#include "CusResponseHandler.h"
#include "apis_edi_file.h"

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace TlgHandling
{

using namespace edilib;
using namespace edifact;


CusResponseHandler::CusResponseHandler(_EDI_REAL_MES_STRUCT_ *PMes,
                                       const edilib::EdiSessRdData *edisess)
    : AstraEdiResponseHandler(PMes, edisess)
{}

void CusResponseHandler::parse()
{
    Cusres cusres = readCUSRES(pMes());
}

}//namespace TlgHandling
