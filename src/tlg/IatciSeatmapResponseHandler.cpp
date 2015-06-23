#include "IatciSeatmapResponseHandler.h"
#include "read_edi_elements.h"
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

class IatciSeatmapResultMaker
{
private:
    edifact::FdrElem m_fdr;
    edifact::RadElem m_rad;
    boost::optional<edifact::ChdElem> m_chd;
    boost::optional<edifact::ErdElem> m_erd;
    boost::optional<edifact::WadElem> m_wad;
    boost::optional<edifact::SrpElem> m_srp;
    boost::optional<edifact::EqdElem> m_eqd;
    std::list<edifact::CbdElem> m_lCbd;
    std::list<edifact::RodElem> m_lRod;

protected:
    static std::list<iatci::SeatColumnDetails> makeSeatColumns(const edifact::CbdElem& cbd);
    static boost::optional<iatci::RowRange> makeSmokingRowRange(const edifact::CbdElem& cbd);
    static boost::optional<iatci::RowRange> makeOverwingRowRange(const edifact::CbdElem& cbd);
    static std::list<iatci::SeatOccupationDetails> makeSeatOccupations(const edifact::RodElem& rod);

public:
    void setFdr(const boost::optional<edifact::FdrElem>& fdr);
    void setRad(const boost::optional<edifact::RadElem>& rad);
    void setChd(const boost::optional<edifact::ChdElem>& chd, bool required = false);
    void setErd(const boost::optional<edifact::ErdElem>& erd, bool required = false);
    void setWad(const boost::optional<edifact::WadElem>& wad, bool required = false);
    void setSrp(const boost::optional<edifact::SrpElem>& srp, bool required = false);
    void setEqd(const boost::optional<edifact::EqdElem>& eqd, bool required = false);
    void addCbd(const boost::optional<edifact::CbdElem>& cbd);
    void addRod(const boost::optional<edifact::RodElem>& rod);
    iatci::Result makeResult() const;
};

//---------------------------------------------------------------------------------------

IatciSeatmapResponseHandler::IatciSeatmapResponseHandler(_EDI_REAL_MES_STRUCT_ *PMes,
                                                         const edilib::EdiSessRdData *edisess)
    : IatciResponseHandler(PMes, edisess)
{
}

void IatciSeatmapResponseHandler::parse()
{
    int flightsCount = GetNumSegGr(pMes(), 1); // Сколько рейсов в ответе
    ASSERT(flightsCount == 1); // Рейс должны быть ровно один

    EdiPointHolder flt_holder(pMes());
    SetEdiPointToSegGrG(pMes(), SegGrElement(1, 0), "PROG_ERR");
    IatciSeatmapResultMaker resultMaker;
    resultMaker.setFdr(readEdiFdr(pMes()));
    resultMaker.setRad(readEdiRad(pMes()));
    resultMaker.setChd(readEdiChd(pMes()));
    resultMaker.setWad(readEdiWad(pMes()));
    resultMaker.setEqd(readEdiEqd(pMes()));
    unsigned numCbd = GetNumSegment(pMes(), "CBD");
    for(unsigned n = 0; n < numCbd; ++n) {
        resultMaker.addCbd(readEdiCbd(pMes(), n));
    }

    unsigned numRod = GetNumSegment(pMes(), "ROD");
    for(unsigned n = 0; n < numRod; ++n) {
        resultMaker.addRod(readEdiRod(pMes(), n));
    }

    m_lRes.push_back(resultMaker.makeResult());
}

//---------------------------------------------------------------------------------------

void IatciSeatmapResultMaker::setFdr(const boost::optional<edifact::FdrElem>& fdr)
{
    ASSERT(fdr);
    m_fdr = *fdr;
}

void IatciSeatmapResultMaker::setRad(const boost::optional<edifact::RadElem>& rad)
{
    ASSERT(rad);
    m_rad = *rad;
}

void IatciSeatmapResultMaker::setChd(const boost::optional<edifact::ChdElem>& chd, bool required)
{
    if(required)
        ASSERT(chd);
    m_chd = chd;
}

void IatciSeatmapResultMaker::setErd(const boost::optional<edifact::ErdElem>& erd, bool required)
{
    if(required)
        ASSERT(erd);
    m_erd = erd;
}

void IatciSeatmapResultMaker::setWad(const boost::optional<edifact::WadElem>& wad, bool required)
{
    if(required)
        ASSERT(wad);
    m_wad = wad;
}

void IatciSeatmapResultMaker::setSrp(const boost::optional<edifact::SrpElem>& srp, bool required)
{
    if(required)
        ASSERT(srp);
    m_srp = srp;
}

void IatciSeatmapResultMaker::setEqd(const boost::optional<edifact::EqdElem>& eqd, bool required)
{
    if(required)
        ASSERT(eqd);
    m_eqd = eqd;
}

void IatciSeatmapResultMaker::addCbd(const boost::optional<edifact::CbdElem>& cbd)
{
    ASSERT(cbd);
    m_lCbd.push_back(*cbd);
}

void IatciSeatmapResultMaker::addRod(const boost::optional<edifact::RodElem>& rod)
{
    ASSERT(rod);
    m_lRod.push_back(*rod);
}

std::list<iatci::SeatColumnDetails> IatciSeatmapResultMaker::makeSeatColumns(const edifact::CbdElem& cbd)
{
    std::list<iatci::SeatColumnDetails> lRes;
    BOOST_FOREACH(const edifact::CbdElem::SeatColumn& seatColumn, cbd.m_lSeatColumns) {
        lRes.push_back(iatci::SeatColumnDetails(seatColumn.m_col,
                                                seatColumn.m_desc1,
                                                seatColumn.m_desc2));
    }

    return lRes;
}

boost::optional<iatci::RowRange> IatciSeatmapResultMaker::makeSmokingRowRange(const edifact::CbdElem& cbd)
{
    if(cbd.m_firstSmokingRow && cbd.m_lastSmokingRow) {
        return iatci::RowRange(cbd.m_firstSmokingRow, cbd.m_lastSmokingRow);
    }

    return boost::none;
}

boost::optional<iatci::RowRange> IatciSeatmapResultMaker::makeOverwingRowRange(const edifact::CbdElem& cbd)
{
    if(cbd.m_firstOverwingRow && cbd.m_lastOverwingRow) {
        return iatci::RowRange(cbd.m_firstOverwingRow, cbd.m_lastOverwingRow);
    }

    return boost::none;
}

std::list<iatci::SeatOccupationDetails> IatciSeatmapResultMaker::makeSeatOccupations(const edifact::RodElem& rod)
{
    std::list<iatci::SeatOccupationDetails> lRes;
    BOOST_FOREACH(const edifact::RodElem::SeatOccupation& seatOccup, rod.m_lSeatOccupations) {
        lRes.push_back(iatci::SeatOccupationDetails(seatOccup.m_col,
                                                    seatOccup.m_occup,
                                                    seatOccup.m_lCharacteristics));
    }

    return lRes;
}

iatci::Result IatciSeatmapResultMaker::makeResult() const
{
    iatci::FlightDetails flightDetails(m_fdr.m_airl,
                                       m_fdr.m_flNum,
                                       m_fdr.m_depPoint,
                                       m_fdr.m_arrPoint,
                                       m_fdr.m_depDate,
                                       m_fdr.m_arrDate,
                                       m_fdr.m_depTime,
                                       m_fdr.m_arrTime);

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

    boost::optional<iatci::SeatmapDetails> seatmapDetails;
    if(!m_lCbd.empty() || !m_lRod.empty()) {
        boost::optional<iatci::SeatRequestDetails> seatRequestDetails;
        if(m_srp) {
            seatRequestDetails = iatci::SeatRequestDetails(m_srp->m_cabinClass,
                                                           iatci::SeatRequestDetails::strToSmokeInd(m_srp->m_noSmokingInd));
        }

        std::list<iatci::CabinDetails> lCabinDetails;
        BOOST_FOREACH(const CbdElem& cbd, m_lCbd) {
            lCabinDetails.push_back(iatci::CabinDetails(
                                             cbd.m_cabinClass,
                                             iatci::RowRange(cbd.m_firstClassRow, cbd.m_lastClassRow),
                                             cbd.m_seatOccupDefIndic,
                                             makeSeatColumns(cbd),
                                             cbd.m_deck,
                                             makeSmokingRowRange(cbd),
                                             makeOverwingRowRange(cbd)));
        }

        std::list<iatci::RowDetails> lRowDetails;
        BOOST_FOREACH(const RodElem& rod, m_lRod) {
            lRowDetails.push_back(iatci::RowDetails(rod.m_row,
                                                    makeSeatOccupations(rod),
                                                    rod.m_characteristic));
        }

        seatmapDetails = iatci::SeatmapDetails(lCabinDetails,
                                               lRowDetails,
                                               seatRequestDetails);
    }

    ASSERT(seatmapDetails);
    return iatci::Result::makeSeatmapResult(iatci::Result::strToStatus(m_rad.m_status),
                                            flightDetails,
                                            *seatmapDetails,
                                            cascadeDetails,
                                            errorDetails,
                                            warningDetails,
                                            equipmentDetails);
}

}//namespace TlgHandling
