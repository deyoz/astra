#include "CusRequestHandler.h"
#include "apis_edi_file.h"

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace TlgHandling
{

using namespace edilib;
using namespace edifact;


CusRequestHandler::CusRequestHandler(_EDI_REAL_MES_STRUCT_ *PMes,
                                     const edilib::EdiSessRdData *edisess)
    : AstraEdiRequestHandler(PMes, edisess)
{}

std::string CusRequestHandler::mesFuncCode() const
{
    return "132";
}

bool CusRequestHandler::fullAnswer() const
{
    return true;
}

void CusRequestHandler::parse()
{
    LogTrace(TRACE3) << readCUSRES(pMes());
}

}//namespace TlgHandling
