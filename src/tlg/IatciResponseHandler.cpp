#include "IatciResponseHandler.h"
#include "read_edi_elements.h"
#include "postpone_edifact.h"
#include "iatci_api.h"
#include "edi_msg.h"

#include <edilib/edi_func_cpp.h>

#include <boost/foreach.hpp>

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace TlgHandling
{

using namespace edilib;
using namespace edifact;
using namespace Ticketing::TickReader;


IatciResponseHandler::IatciResponseHandler(_EDI_REAL_MES_STRUCT_ *PMes,
                                           const edilib::EdiSessRdData *edisess)
    : AstraEdiResponseHandler(PMes, edisess)
{
}

void IatciResponseHandler::fillFuncCodeRespStatus()
{
    readRemoteResults();
    if(pMes())
    {
        int flightsCount = GetNumSegGr(pMes(), 1);
        ASSERT(flightsCount > 0);

        std::string funcCode = "";
        int numFailedFlights = 0;

        for(int currFlight = 0; currFlight < flightsCount; ++currFlight)
        {
            EdiPointHolder flt_holder(pMes());
            SetEdiPointToSegGrG(pMes(), SegGrElement(1, currFlight), "PROG_ERR");
            boost::optional<edifact::RadElem> rad;
            rad = Ticketing::TickReader::readEdiRad(pMes());
            ASSERT(rad);
            if(funcCode.empty()) {
                funcCode = rad->m_respType;
            } else if(funcCode != rad->m_respType) {
                LogError(STDLOG) << "Different response types in message!";
            }

            if(iatci::Result::strToStatus(rad->m_status) == iatci::Result::Failed) {
                numFailedFlights++;
            }
        }

        LogTrace(TRACE3) << "set func code: " << funcCode;
        setFuncCode(funcCode);

        LogTrace(TRACE1) << "Failed " << numFailedFlights << " of " << flightsCount;

        if(numFailedFlights == 0)  { // ни одного рейса не зафейлилось
            setRespStatus(EdiRespStatus(EdiRespStatus::successfully));
        } else if(numFailedFlights == flightsCount) { // все рейсы зафейлились
            setRespStatus(EdiRespStatus(EdiRespStatus::notProcessed));
        } else { // часть рейсов зафейлилась
            setRespStatus(EdiRespStatus(EdiRespStatus::partial));
        }

        setRemoteResultStatus();
    }
}

void IatciResponseHandler::fillErrorDetails()
{
}

void IatciResponseHandler::handle()
{
    tlgnum_t postponeTlg = PostponeEdiHandling::findPostponeTlg(ediSessId());
    if(postponeTlg.num.valid()) {
        // сохранение данных для последующей обработки отложенной телеграммы
        iatci::saveDeferredCkiData(postponeTlg, m_lRes);
    } else {
        // сохранение данных для obrzap
        iatci::saveCkiData(ediSessId(), m_lRes);
    }
}

void IatciResponseHandler::onTimeOut()
{
    tlgnum_t postponeTlg = PostponeEdiHandling::findPostponeTlg(ediSessId());
    if(postponeTlg.num.valid()) {
        // в списке m_lRes должен лежать один элемент, информирующий о таймауте
        iatci::saveDeferredCkiData(postponeTlg, m_lRes);
    }
}

//---------------------------------------------------------------------------------------

void IatciResultMaker::setFdr(const boost::optional<edifact::FdrElem>& fdr)
{
    ASSERT(fdr);
    m_fdr = *fdr;
}

void IatciResultMaker::setRad(const boost::optional<edifact::RadElem>& rad)
{
    ASSERT(rad);
    m_rad = *rad;
}

void IatciResultMaker::setPpd(const boost::optional<edifact::PpdElem>& ppd, bool required)
{
    if(required)
        ASSERT(ppd);
    m_ppd = *ppd;
}

void IatciResultMaker::setPfd(const boost::optional<edifact::PfdElem>& pfd, bool required)
{
    if(required)
        ASSERT(pfd);
    m_pfd = pfd;
}

void IatciResultMaker::setFsd(const boost::optional<edifact::FsdElem>& fsd, bool required)
{
    if(required)
        ASSERT(fsd);
    m_fsd = fsd;
}

void IatciResultMaker::setChd(const boost::optional<edifact::ChdElem>& chd, bool required)
{
    if(required)
        ASSERT(chd);
    m_chd = chd;
}

void IatciResultMaker::setErd(const boost::optional<edifact::ErdElem>& erd, bool required)
{
    if(required)
        ASSERT(erd);
    m_erd = erd;
}

iatci::Result IatciResultMaker::makeResult() const
{
    iatci::FlightDetails flightDetails(m_fdr.m_airl,
                                       m_fdr.m_flNum,
                                       m_fdr.m_depPoint,
                                       m_fdr.m_arrPoint,
                                       m_fdr.m_depDate,
                                       m_fdr.m_arrDate,
                                       m_fdr.m_depTime,
                                       m_fdr.m_arrTime,
                                       m_fsd ? m_fsd->m_boardingTime : Dates::not_a_date_time);

    boost::optional<iatci::PaxDetails> paxDetails;
    if(m_ppd) {
        paxDetails = iatci::PaxDetails(m_ppd->m_passSurname,
                                       m_ppd->m_passName,
                                       iatci::PaxDetails::strToType(m_ppd->m_passType),
                                       m_ppd->m_passQryRef,
                                       m_ppd->m_passRespRef);
    }

    boost::optional<iatci::FlightSeatDetails> seatDetails;
    if(m_pfd) {
        seatDetails = iatci::FlightSeatDetails(m_pfd->m_seat,
                                               m_pfd->m_cabinClass,
                                               m_pfd->m_securityId,
                                               iatci::FlightSeatDetails::strToSmokeInd(m_pfd->m_noSmokingInd));
    }

    boost::optional<iatci::CascadeHostDetails> cascadeDetails;
    if(m_chd) {
        cascadeDetails = iatci::CascadeHostDetails(m_chd->m_origAirline,
                                                   m_chd->m_origPoint);
        BOOST_FOREACH(const std::string& hostAirline, m_chd->m_hostAirlines) {
            cascadeDetails->addHostAirline(hostAirline);
        }
    }

    boost::optional<iatci::ErrorDetails> errorDetails;
    if(m_erd) {
        errorDetails = iatci::ErrorDetails(edifact::getInnerErrByErd(m_erd->m_messageNumber),
                                           m_erd->m_messageText);
    }

    return iatci::Result::makeResult(iatci::Result::strToAction(m_rad.m_respType),
                                     iatci::Result::strToStatus(m_rad.m_status),
                                     flightDetails,
                                     paxDetails,
                                     seatDetails,
                                     cascadeDetails,
                                     errorDetails);
}

}//namespace TlgHandling
