#include "IatciCkxResponseHandler.h"
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

IatciCkxResponseHandler::IatciCkxResponseHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                                                 const edilib::EdiSessRdData *edisess)
    : IatciResponseHandler(pMes, edisess)
{
}

iatci::Result::Action_e IatciCkxResponseHandler::action() const
{
    return iatci::Result::Cancel;
}

void IatciCkxResponseHandler::parse()
{
    int flightsCount = GetNumSegGr(pMes(), 1); // Сколько рейсов в ответе
    ASSERT(flightsCount > 0); // Рейсы должны быть обязательно

    EdiPointHolder flt_holder(pMes());
    for(int currFlight = 0; currFlight < flightsCount; ++currFlight)
    {
        IatciResultMaker ckiResultMaker;
        SetEdiPointToSegGrG(pMes(), SegGrElement(1, currFlight), "PROG_ERR");
        ckiResultMaker.setFdr(readEdiFdr(pMes()));
        ckiResultMaker.setRad(readEdiRad(pMes()));
        ckiResultMaker.setChd(readEdiChd(pMes()));
        ckiResultMaker.setErd(readEdiErd(pMes()));

        m_lRes.push_back(ckiResultMaker.makeResult());
    }
}

}//namespace TlgHandling
