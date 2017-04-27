#include "IatciResponseHandler.h"
#include "read_edi_elements.h"
#include "postpone_edifact.h"
#include "basetables.h"
#include "iatci_api.h"
#include "iatci_help.h"

#include <edilib/edi_func_cpp.h>
#include <serverlib/algo.h>

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
public:

    class Pxg
    {
    private:
        edifact::PpdElem                  m_ppd;
        boost::optional<edifact::PrdElem> m_prd;
        boost::optional<edifact::PfdElem> m_pfd;
        boost::optional<edifact::PsiElem> m_psi;
        boost::optional<edifact::PapElem> m_pap;
        boost::optional<edifact::PapElem> m_papInfant;
        //boost::optional<edifact::AddElem> m_add;
    public:
        void setPpd(const boost::optional<PpdElem>& ppd);
        void setPrd(const boost::optional<edifact::PrdElem>& prd, bool required = false);
        void setPsi(const boost::optional<edifact::PsiElem>& psi, bool required = false);
        void setPfd(const boost::optional<edifact::PfdElem>& pfd, bool required = false);
        void addPap(const boost::optional<edifact::PapElem>& pap);

        iatci::dcrcka::PaxGroup makePaxGroup() const;

    protected:
        boost::optional<iatci::ReservationDetails> makeReserv() const;
        boost::optional<iatci::FlightSeatDetails>  makeSeat() const;
        boost::optional<iatci::ServiceDetails>     makeService() const;
        boost::optional<iatci::BaggageDetails>     makeBaggage() const;
        boost::optional<iatci::DocDetails>         makeDoc() const;
        boost::optional<iatci::AddressDetails>     makeAddress() const;
        boost::optional<iatci::PaxDetails>         makeInfant() const;
        boost::optional<iatci::DocDetails>         makeInfantDoc() const;
        boost::optional<iatci::FlightSeatDetails>  makeInfantSeat() const;
    };

private:
    edifact::FdrElem m_fdr;
    edifact::RadElem m_rad;
    boost::optional<edifact::ChdElem> m_chd;        
    boost::optional<edifact::FsdElem> m_fsd;
    boost::optional<edifact::ErdElem> m_erd;
    boost::optional<edifact::WadElem> m_wad;
    boost::optional<edifact::EqdElem> m_eqd;
    std::list<Pxg>                    m_lPxg;

public:
    void setFdr(const boost::optional<edifact::FdrElem>& fdr);
    void setRad(const boost::optional<edifact::RadElem>& rad);        
    void setChd(const boost::optional<edifact::ChdElem>& chd, bool required = false);
    void setFsd(const boost::optional<edifact::FsdElem>& fsd, bool required = false);
    void setErd(const boost::optional<edifact::ErdElem>& erd, bool required = false);
    void setWad(const boost::optional<edifact::WadElem>& wad, bool required = false);
    void setEqd(const boost::optional<edifact::EqdElem>& eqd, bool required = false);
    void addPxg(const Pxg& pxg);

    iatci::dcrcka::Result makeResult() const;

protected:
    boost::optional<iatci::CascadeHostDetails> makeCascade() const;
    boost::optional<iatci::EquipmentDetails>   makeEquipment() const;
    boost::optional<iatci::ErrorDetails>       makeError() const;
    boost::optional<iatci::WarningDetails>     makeWarning() const;
};

//---------------------------------------------------------------------------------------

IatciResponseHandler::IatciResponseHandler(_EDI_REAL_MES_STRUCT_ *PMes,
                                           const edilib::EdiSessRdData *edisess)
    : AstraEdiResponseHandler(PMes, edisess)
{
}

void IatciResponseHandler::fillFuncCodeRespStatus()
{
    using iatci::dcrcka::Result;
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

            if(Result::strToStatus(rad->m_status) == Result::Failed ||
               Result::strToStatus(rad->m_status) == Result::RecoverableError)
            {
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
    if(pMes())
    {
        PushEdiPointG(pMes());
        if(SetEdiPointToSegGrG(pMes(), 1))
        {
            PushEdiPointG(pMes());
            if(SetEdiPointToSegmentG(pMes(), SegmElement("ERD")))
            {
                PushEdiPointG(pMes());
                SetEdiPointToCompositeG(pMes(), CompElement("C056"));

                setEdiErrCode(GetDBFName(pMes(), DataElement(9845), ""));
                setEdiErrText(GetDBFName(pMes(), DataElement(4440), ""));

                PopEdiPointG(pMes());
            }

            PopEdiPointG(pMes());
        }
        PopEdiPointG(pMes());
    }
}

void IatciResponseHandler::parse()
{
    int flightsCount = GetNumSegGr(pMes(), 1); // Сколько рейсов в ответе
    ASSERT(flightsCount > 0); // Рейсы должны быть обязательно

    for(int currFlight = 0; currFlight < flightsCount; ++currFlight)
    {
        EdiPointHolder flg_holder(pMes());
        SetEdiPointToSegGrG(pMes(), SegGrElement(1, currFlight), "PROG_ERR");
        IatciResultMaker resultMaker;
        resultMaker.setFdr(readEdiFdr(pMes()));
        resultMaker.setRad(readEdiRad(pMes()));
        resultMaker.setChd(readEdiChd(pMes()));
        resultMaker.setFsd(readEdiFsd(pMes()));
        resultMaker.setErd(readEdiErd(pMes()));

        int paxCount = GetNumSegGr(pMes(), 2);
        for(int currPax = 0; currPax < paxCount; ++currPax)
        {
            EdiPointHolder pxg_holder(pMes());
            SetEdiPointToSegGrG(pMes(), SegGrElement(2, currPax), "PROG_ERR");

            IatciResultMaker::Pxg pxg;
            pxg.setPpd(readEdiPpd(pMes()));
            pxg.setPrd(readEdiPrd(pMes()));
            pxg.setPfd(readEdiPfd(pMes()));
            pxg.setPsi(readEdiPsi(pMes()));

            int apgCount = GetNumSegGr(pMes(), 3);
            for(int curApg = 0; curApg < apgCount; ++curApg)
            {
                EdiPointHolder apg_holder(pMes());
                SetEdiPointToSegGrG(pMes(), SegGrElement(3, curApg), "PROG_ERR");

                pxg.addPap(readEdiPap(pMes()));
            }
            
            resultMaker.addPxg(pxg);
        }

        m_lRes.push_back(resultMaker.makeResult());
    }
}

void IatciResponseHandler::handle()
{
    boost::optional<tlgnum_t> postponeTlgNum = PostponeEdiHandling::findPostponeTlg(ediSessId());
    if(postponeTlgNum) {
        // сохранение данных для последующей обработки отложенной телеграммы
        iatci::saveDeferredCkiData(*postponeTlgNum, m_lRes);
    } else {
        // сохранение данных для obrzap
        iatci::saveCkiData(ediSessId(), m_lRes);
    }
}

//---------------------------------------------------------------------------------------

void IatciResultMaker::Pxg::setPpd(const boost::optional<edifact::PpdElem>& ppd)
{
    ASSERT(ppd);
    m_ppd = *ppd;
}

void IatciResultMaker::Pxg::setPrd(const boost::optional<edifact::PrdElem>& prd,
                                   bool required)
{
    if(required)
        ASSERT(prd);
    m_prd = prd;
}

void IatciResultMaker::Pxg::setPsi(const boost::optional<edifact::PsiElem>& psi,
                                   bool required)
{
    if(required)
        ASSERT(psi);
    m_psi = psi;
}

void IatciResultMaker::Pxg::setPfd(const boost::optional<edifact::PfdElem>& pfd,
                                   bool required)
{
    if(required)
        ASSERT(pfd);
    m_pfd = pfd;
}

void IatciResultMaker::Pxg::addPap(const boost::optional<edifact::PapElem>& pap)
{
    if(pap && pap->m_type == "IN") {
        m_papInfant = pap;
    } else {
        m_pap = pap;
    }
}

iatci::dcrcka::PaxGroup IatciResultMaker::Pxg::makePaxGroup() const
{
    return iatci::dcrcka::PaxGroup(iatci::makePax(m_ppd),
                                   makeReserv(),
                                   makeSeat(),
                                   makeBaggage(),
                                   makeService(),
                                   makeDoc(),
                                   makeAddress(),
                                   makeInfant(),
                                   makeInfantDoc(),
                                   makeInfantSeat());
}

boost::optional<iatci::ReservationDetails> IatciResultMaker::Pxg::makeReserv() const
{
    if(m_prd) {
        return iatci::makeReserv(*m_prd);
    }

    return boost::none;
}

boost::optional<iatci::FlightSeatDetails>  IatciResultMaker::Pxg::makeSeat() const
{
    if(m_pfd) {
        return iatci::makeSeat(*m_pfd);
    }

    return boost::none;
}

boost::optional<iatci::ServiceDetails> IatciResultMaker::Pxg::makeService() const
{
    if(m_psi) {
        return iatci::makeService(*m_psi);
    }

    return boost::none;
}

boost::optional<iatci::BaggageDetails> IatciResultMaker::Pxg::makeBaggage() const
{
    // TODO
    return boost::none;
}

boost::optional<iatci::DocDetails> IatciResultMaker::Pxg::makeDoc() const
{
    if(m_pap) {
        return iatci::makeDoc(*m_pap);
    }

    return boost::none;
}

boost::optional<iatci::AddressDetails> IatciResultMaker::Pxg::makeAddress() const
{
    return boost::none;
}

boost::optional<iatci::PaxDetails> IatciResultMaker::Pxg::makeInfant() const
{
    if(m_ppd.m_withInftIndicator == "Y") {
        return iatci::makeInfant(m_ppd);
    }

    return boost::none;
}

boost::optional<iatci::DocDetails> IatciResultMaker::Pxg::makeInfantDoc() const
{
    if(m_papInfant) {
        return iatci::makeDoc(*m_papInfant);
    }

    return boost::none;
}

boost::optional<iatci::FlightSeatDetails> IatciResultMaker::Pxg::makeInfantSeat() const
{
    if(m_pfd) {
        return iatci::makeInfantSeat(*m_pfd);
    }

    return boost::none;
}

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

void IatciResultMaker::setChd(const boost::optional<edifact::ChdElem>& chd, bool required)
{
    if(required)
        ASSERT(chd);
    m_chd = chd;
}

void IatciResultMaker::setFsd(const boost::optional<edifact::FsdElem>& fsd, bool required)
{
    if(required)
        ASSERT(fsd);
    m_fsd = fsd;
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

void IatciResultMaker::addPxg(const IatciResultMaker::Pxg& pxg)
{
    m_lPxg.push_back(pxg);
}

iatci::dcrcka::Result IatciResultMaker::makeResult() const
{
    using iatci::dcrcka::Result;
    const auto paxGroups = algo::transform(m_lPxg, [](const Pxg& pxg) {
        return pxg.makePaxGroup();
    });

    return Result::makeResult(Result::strToAction(m_rad.m_respType),
                              Result::strToStatus(m_rad.m_status),
                              iatci::makeFlight(m_fdr, m_fsd),
                              paxGroups,
                              boost::none,
                              makeCascade(),
                              makeError(),
                              makeWarning(),
                              makeEquipment());
}

boost::optional<iatci::CascadeHostDetails> IatciResultMaker::makeCascade() const
{
    if(m_chd) {
        return iatci::makeCascade(*m_chd);
    }

    return boost::none;
}

boost::optional<iatci::EquipmentDetails> IatciResultMaker::makeEquipment() const
{
    if(m_eqd) {
        return iatci::makeEquipment(*m_eqd);
    }

    return boost::none;
}

boost::optional<iatci::ErrorDetails> IatciResultMaker::makeError() const
{
    if(m_erd) {
        return iatci::makeError(*m_erd);
    }

    return boost::none;
}

boost::optional<iatci::WarningDetails> IatciResultMaker::makeWarning() const
{
    if(m_wad) {
        return iatci::makeWarning(*m_wad);
    }

    return boost::none;
}

}//namespace TlgHandling
