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
using namespace Ticketing;
using namespace Ticketing::TickReader;


class IatciResultMaker
{
private:
    edifact::FdrElem m_fdr;
    edifact::RadElem m_rad;
    boost::optional<edifact::PpdElem> m_ppd;
    boost::optional<edifact::ChdElem> m_chd;
    boost::optional<edifact::PfdElem> m_pfd;
    boost::optional<edifact::PsiElem> m_psi;
    boost::optional<edifact::FsdElem> m_fsd;
    boost::optional<edifact::ErdElem> m_erd;
    boost::optional<edifact::WadElem> m_wad;
    boost::optional<edifact::EqdElem> m_eqd;

public:
    void setFdr(const boost::optional<edifact::FdrElem>& fdr);
    void setRad(const boost::optional<edifact::RadElem>& rad);
    void setPpd(const boost::optional<edifact::PpdElem>& ppd, bool required = false);
    void setPfd(const boost::optional<edifact::PfdElem>& pfd, bool required = false);
    void setPsi(const boost::optional<edifact::PsiElem>& psi, bool required = false);
    void setFsd(const boost::optional<edifact::FsdElem>& fsd, bool required = false);
    void setChd(const boost::optional<edifact::ChdElem>& chd, bool required = false);
    void setErd(const boost::optional<edifact::ErdElem>& erd, bool required = false);
    void setWad(const boost::optional<edifact::WadElem>& wad, bool required = false);
    void setEqd(const boost::optional<edifact::EqdElem>& eqd, bool required = false);
    iatci::Result makeResult() const;
};

//---------------------------------------------------------------------------------------

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

void IatciResponseHandler::parse()
{
    int flightsCount = GetNumSegGr(pMes(), 1); // Сколько рейсов в ответе
    ASSERT(flightsCount > 0); // Рейсы должны быть обязательно

    for(int currFlight = 0; currFlight < flightsCount; ++currFlight)
    {
        EdiPointHolder flt_holder(pMes());
        SetEdiPointToSegGrG(pMes(), SegGrElement(1, currFlight), "PROG_ERR");
        IatciResultMaker resultMaker;
        resultMaker.setFdr(readEdiFdr(pMes()));
        resultMaker.setRad(readEdiRad(pMes()));
        resultMaker.setChd(readEdiChd(pMes()));
        resultMaker.setFsd(readEdiFsd(pMes()));
        resultMaker.setErd(readEdiErd(pMes()));

        if(GetNumSegGr(pMes(), 2) > 0)
        {
            EdiPointHolder pax_holder(pMes());
            SetEdiPointToSegGrG(pMes(), SegGrElement(2), "PROG_ERR");
            resultMaker.setPpd(readEdiPpd(pMes()), true /*required*/);
            resultMaker.setPfd(readEdiPfd(pMes()));
            resultMaker.setPsi(readEdiPsi(pMes()));
        }

        m_lRes.push_back(resultMaker.makeResult());
    }
}

void IatciResponseHandler::handle()
{
    boost::optional<tlgnum_t> postponeTlg = PostponeEdiHandling::findPostponeTlg(ediSessId());
    if(postponeTlg) {
        // сохранение данных для последующей обработки отложенной телеграммы
        iatci::saveDeferredCkiData(*postponeTlg, m_lRes);
    } else {
        // сохранение данных для obrzap
        iatci::saveCkiData(ediSessId(), m_lRes);
    }
}

void IatciResponseHandler::onTimeOut()
{
    // в список m_lRes положим один элемент, информирующий о таймауте
    m_lRes.push_back(iatci::Result::makeFailResult(action(),
                                                   iatci::ErrorDetails(AstraErr::TIMEOUT_ON_HOST_3)));
    boost::optional<tlgnum_t> postponeTlg = PostponeEdiHandling::findPostponeTlg(ediSessId());
    if(postponeTlg) {
        iatci::saveDeferredCkiData(*postponeTlg, m_lRes);
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

void IatciResultMaker::setPsi(const boost::optional<edifact::PsiElem>& psi, bool required)
{
    if(required)
        ASSERT(psi);
    m_psi = psi;
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

void IatciResultMaker::setWad(const boost::optional<edifact::WadElem>& wad, bool required)
{
    if(required)
        ASSERT(wad);
    m_wad = wad;
}

void IatciResultMaker::setEqd(const boost::optional<edifact::EqdElem>& eqd, bool required)
{
    if(required)
        ASSERT(eqd);
    m_eqd = eqd;
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

    boost::optional<iatci::WarningDetails> warningDetails;
    if(m_wad) {
        warningDetails = iatci::WarningDetails(edifact::getInnerErrByErd(m_wad->m_messageNumber),
                                               m_wad->m_messageText);
    }

    boost::optional<iatci::EquipmentDetails> equipmentDetails;
    if(m_eqd) {
        equipmentDetails = iatci::EquipmentDetails(m_eqd->m_equipment);
    }

    boost::optional<iatci::ServiceDetails> serviceDetails;
    if(m_psi) {
        serviceDetails = iatci::ServiceDetails(m_psi->m_osi);
        BOOST_FOREACH(const PsiElem::SsrDetails& ssr, m_psi->m_lSsr) {
            serviceDetails->addSsr(iatci::ServiceDetails::SsrInfo(ssr.m_ssrCode,
                                                                  ssr.m_ssrText,
                                                                  ssr.m_qualifier == "INF",
                                                                  ssr.m_freeText,
                                                                  ssr.m_airline,
                                                                  ssr.m_numOfPieces));
        }
    }

    return iatci::Result::makeResult(iatci::Result::strToAction(m_rad.m_respType),
                                     iatci::Result::strToStatus(m_rad.m_status),
                                     flightDetails,
                                     paxDetails,
                                     seatDetails,
                                     boost::none,
                                     cascadeDetails,
                                     errorDetails,
                                     warningDetails,
                                     equipmentDetails,
                                     serviceDetails);
}

}//namespace TlgHandling
