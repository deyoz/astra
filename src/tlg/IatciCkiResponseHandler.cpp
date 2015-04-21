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

void IatciCkiResponseHandler::parse()
{
    int flightsCount = GetNumSegGr(pMes(), 1); // ����쪮 ३ᮢ � �⢥�
    ASSERT(flightsCount > 0); // ����� ������ ���� ��易⥫쭮

    for(int currFlight = 0; currFlight < flightsCount; ++currFlight)
    {
        EdiPointHolder flt_holder(pMes());
        IatciResultMaker ckiResultMaker;
        SetEdiPointToSegGrG(pMes(), SegGrElement(1, currFlight), "PROG_ERR");
        ckiResultMaker.setFdr(readEdiFdr(pMes()));
        ckiResultMaker.setRad(readEdiRad(pMes()));
        ckiResultMaker.setChd(readEdiChd(pMes()));
        ckiResultMaker.setFsd(readEdiFsd(pMes()));
        ckiResultMaker.setErd(readEdiErd(pMes()));

        if(GetNumSegGr(pMes(), 2) > 0)
        {
            EdiPointHolder pax_holder(pMes());
            SetEdiPointToSegGrG(pMes(), SegGrElement(2), "PROG_ERR");
            ckiResultMaker.setPpd(readEdiPpd(pMes()), true /*required*/);
            ckiResultMaker.setPfd(readEdiPfd(pMes()));
        }

        m_lRes.push_back(ckiResultMaker.makeResult());
    }
}

void IatciCkiResponseHandler::onTimeOut()
{
    m_lRes.push_back(iatci::Result::makeFailResult(iatci::Result::Checkin,
                                                   iatci::ErrorDetails(AstraErr::TIMEOUT_ON_HOST_3)));
    IatciResponseHandler::onTimeOut();
}

}//namespace TlgHandling
