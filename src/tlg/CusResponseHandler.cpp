#include "CusResponseHandler.h"
#include "apis_edi_file.h"
#include "apps_interaction.h"

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
    ProcessChinaCusres(*m_data);
}

}//namespace TlgHandling
