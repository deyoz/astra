#include "IatciSmfRequestHandler.h"
#include "IatciSmfRequest.h"
#include "read_edi_elements.h"
#include "view_edi_elements.h"
#include "iatci_api.h"

#include <boost/foreach.hpp>

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace TlgHandling {

using namespace edilib;
using namespace edifact;
using namespace Ticketing;
using namespace Ticketing::TickReader;
using namespace Ticketing::TickExceptions;
using namespace Ticketing::RemoteSystemContext;


class IatciSmfParamsMaker
{
private:
    edifact::LorElem m_lor;
    edifact::FdqElem m_fdq;
    boost::optional<edifact::SrpElem> m_srp;
    boost::optional<edifact::ChdElem> m_chd;

public:
    void setLor(const boost::optional<edifact::LorElem>& lor);
    void setFdq(const boost::optional<edifact::FdqElem>& fdq);
    void setSrp(const boost::optional<edifact::SrpElem>& srp, bool required = false);
    void setChd(const boost::optional<edifact::ChdElem>& chd, bool required = false);
    iatci::SmfParams makeParams() const;
};

//---------------------------------------------------------------------------------------

IatciSmfRequestHandler::IatciSmfRequestHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                                               const edilib::EdiSessRdData *edisess)
    : IatciSeatmapRequestHandler(pMes, edisess)
{
}

void IatciSmfRequestHandler::parse()
{
    IatciSmfParamsMaker smfParamsMaker;
    smfParamsMaker.setLor(readEdiLor(pMes()));
    smfParamsMaker.setFdq(readEdiFdq(pMes()));
    smfParamsMaker.setSrp(readEdiSrp(pMes()));
    smfParamsMaker.setChd(readEdiChd(pMes()));

    m_smfParams = smfParamsMaker.makeParams();
}

std::string IatciSmfRequestHandler::respType() const
{
    return "S";
}

boost::optional<iatci::BaseParams> IatciSmfRequestHandler::params() const
{
    return smfParams();
}

boost::optional<iatci::BaseParams> IatciSmfRequestHandler::nextParams() const
{
    if(nextSmfParams()) {
        return *nextSmfParams();
    }

    return boost::none;
}

iatci::Result IatciSmfRequestHandler::handleRequest() const
{
    return iatci::fillSeatmap(smfParams());
}

edilib::EdiSessionId_t IatciSmfRequestHandler::sendCascadeRequest() const
{
    ASSERT(nextSmfParams());
    return edifact::SendSmfRequest(*nextSmfParams());
}

const iatci::SmfParams& IatciSmfRequestHandler::smfParams() const
{
    ASSERT(m_smfParams);
    return m_smfParams.get();
}

boost::optional<iatci::SmfParams> IatciSmfRequestHandler::nextSmfParams() const
{
    boost::optional<iatci::FlightDetails> flightForNextHost;
    flightForNextHost = iatci::findCascadeFlight(smfParams().flight());
    if(!flightForNextHost) {
        tst();
        return boost::none;
    }

    iatci::CascadeHostDetails cascadeDetails(smfParams().origin().airline(),
                                             smfParams().origin().point());
    cascadeDetails.addHostAirline(smfParams().flight().airline());
    if(smfParams().flightFromPrevHost()) {
        cascadeDetails.addHostAirline(smfParams().flightFromPrevHost()->airline());
    }

    return iatci::SmfParams(iatci::OriginatorDetails(smfParams().flight().airline()),
                            *flightForNextHost,
                            smfParams().seatRequestDetails(),
                            smfParams().flight(),
                            cascadeDetails);
}

//---------------------------------------------------------------------------------------

void IatciSmfParamsMaker::setLor(const boost::optional<edifact::LorElem>& lor)
{
    ASSERT(lor);
    m_lor = *lor;
}

void IatciSmfParamsMaker::setFdq(const boost::optional<edifact::FdqElem>& fdq)
{
    ASSERT(fdq);
    m_fdq = *fdq;
}

void IatciSmfParamsMaker::setSrp(const boost::optional<edifact::SrpElem>& srp, bool required)
{
    if(required)
        ASSERT(srp);
    m_srp = srp;
}

void IatciSmfParamsMaker::setChd(const boost::optional<edifact::ChdElem>& chd, bool required)
{
    if(required)
        ASSERT(chd);
    m_chd = chd;
}

iatci::SmfParams IatciSmfParamsMaker::makeParams() const
{
    iatci::OriginatorDetails origDetails(m_lor.m_airline,
                                         m_lor.m_port);

    iatci::FlightDetails flight(m_fdq.m_outbAirl,
                                m_fdq.m_outbFlNum,
                                m_fdq.m_outbDepPoint,
                                m_fdq.m_outbArrPoint,
                                m_fdq.m_outbDepDate,
                                Dates::Date_t(),
                                m_fdq.m_outbDepTime);

    iatci::FlightDetails prevFlight(m_fdq.m_inbAirl,
                                    m_fdq.m_inbFlNum,
                                    m_fdq.m_inbDepPoint,
                                    m_fdq.m_inbArrPoint,
                                    m_fdq.m_inbDepDate,
                                    m_fdq.m_inbArrDate,
                                    m_fdq.m_inbDepTime,
                                    m_fdq.m_inbArrTime);

    boost::optional<iatci::SeatRequestDetails> seatRequestDetails;
    if(m_srp) {
        seatRequestDetails = iatci::SeatRequestDetails(m_srp->m_cabinClass,
                                                       iatci::SeatRequestDetails::strToSmokeInd(m_srp->m_noSmokingInd));
    }

    boost::optional<iatci::CascadeHostDetails> cascadeHostDetails;
    if(m_chd) {
        cascadeHostDetails = iatci::CascadeHostDetails(m_chd->m_origAirline,
                                                       m_chd->m_origPoint);
        BOOST_FOREACH(const std::string& hostAirline, m_chd->m_hostAirlines) {
            cascadeHostDetails->addHostAirline(hostAirline);
        }
    }

    return iatci::SmfParams(origDetails,
                            flight,
                            seatRequestDetails,
                            prevFlight,
                            cascadeHostDetails);
}

}//namespace TlgHandling
