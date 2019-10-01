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
    m_data.reset(new edifact::Cusres(readCUSRES(pMes())));
}

void CusResponseHandler::handle()
{
    ASSERT(m_data);
    try {
        callbacks<CusresCallbacks>()->onCusRequestHandle(TRACE5, *m_data);
    } catch(...) {
        CallbacksExceptionFilter(STDLOG);
    }
}

}//namespace TlgHandling
