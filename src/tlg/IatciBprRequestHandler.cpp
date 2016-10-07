#include "IatciBprRequestHandler.h"
#include "IatciBprRequest.h"
#include "read_edi_elements.h"
#include "view_edi_elements.h"
#include "basetables.h"
#include "remote_system_context.h"
#include "iatci_types.h"
#include "iatci_api.h"
#include "astra_msg.h"

#include <edilib/edi_func_cpp.h>
#include <etick/exceptions.h>

#include <boost/optional.hpp>
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


class IatciBprParamsMaker
{
private:
    edifact::LorElem m_lor;
    edifact::FdqElem m_fdq;
    edifact::PpdElem m_ppd;
    boost::optional<edifact::ChdElem> m_chd;

public:
    void setLor(const boost::optional<edifact::LorElem>& lor);
    void setFdq(const boost::optional<edifact::FdqElem>& fdq);
    void setPpd(const boost::optional<edifact::PpdElem>& ppd);
    void setChd(const boost::optional<edifact::ChdElem>& chd, bool required = false);
    iatci::BprParams makeParams() const;
};

//---------------------------------------------------------------------------------------

IatciBprRequestHandler::IatciBprRequestHandler(_EDI_REAL_MES_STRUCT_* pMes,
                                               const edilib::EdiSessRdData *edisess)
    : IatciRequestHandler(pMes, edisess)
{
}

void IatciBprRequestHandler::parse()
{
    IatciBprParamsMaker bprParamsMaker;
    bprParamsMaker.setLor(readEdiLor(pMes())); /* LOR должен быть обязательно */
    bprParamsMaker.setChd(readEdiChd(pMes()));

    SetEdiPointToSegGrG(pMes(), SegGrElement(1), "PROG_ERR");
    bprParamsMaker.setFdq(readEdiFdq(pMes())); /* FDQ должен быть обязательно */

    int paxCount = GetNumSegGr(pMes(), 2);
    ASSERT(paxCount > 0);

    if(paxCount > 1) {
        LogError(STDLOG) << "Warning: bpr request for several passengers!";
    }

    EdiPointHolder grp_holder(pMes());
    SetEdiPointToSegGrG(pMes(), 2, 0, "PROG_ERR");
    bprParamsMaker.setPpd(readEdiPpd(pMes())); /* PPD должен быть обязательно в Sg2 */

    m_bprParams = bprParamsMaker.makeParams();
}

std::string IatciBprRequestHandler::respType() const
{
    return "B";
}

iatci::Result IatciBprRequestHandler::handleRequest() const
{
    return iatci::reprintBoardingPass(bprParams());
}

edilib::EdiSessionId_t IatciBprRequestHandler::sendCascadeRequest() const
{
    ASSERT(nextBprParams());
    return edifact::SendBprRequest(*nextBprParams());
}

boost::optional<iatci::BaseParams> IatciBprRequestHandler::params() const
{
    return bprParams();
}

boost::optional<iatci::BaseParams> IatciBprRequestHandler::nextParams() const
{
    if(nextBprParams()) {
        return *nextBprParams();
    }

    return boost::none;
}

const iatci::BprParams& IatciBprRequestHandler::bprParams() const
{
    ASSERT(m_bprParams);
    return m_bprParams.get();
}

boost::optional<iatci::BprParams> IatciBprRequestHandler::nextBprParams() const
{
    boost::optional<iatci::FlightDetails> flightForNextHost;
    flightForNextHost = iatci::findCascadeFlight(bprParams().flight());
    if(!flightForNextHost) {
        tst();
        return boost::none;
    }

    iatci::PaxDetails pax = iatci::PaxDetails(bprParams().pax().surname(),
                                              bprParams().pax().name(),
                                              bprParams().pax().type(),
                                              boost::none,
                                              bprParams().flight().toShortKeyString());

    iatci::CascadeHostDetails cascadeDetails(bprParams().origin().airline(),
                                             bprParams().origin().port());
    cascadeDetails.addHostAirline(bprParams().flight().airline());
    if(bprParams().flightFromPrevHost()) {
        cascadeDetails.addHostAirline(bprParams().flightFromPrevHost()->airline());
    }

    return iatci::BprParams(iatci::OriginatorDetails(bprParams().flight().airline()),
                            pax,
                            *flightForNextHost,
                            bprParams().flight(),
                            cascadeDetails);
}

//---------------------------------------------------------------------------------------

void IatciBprParamsMaker::setLor(const boost::optional<edifact::LorElem>& lor)
{
    ASSERT(lor);
    m_lor = *lor;
}

void IatciBprParamsMaker::setFdq(const boost::optional<edifact::FdqElem>& fdq)
{
    ASSERT(fdq);
    m_fdq = *fdq;
}

void IatciBprParamsMaker::setPpd(const boost::optional<edifact::PpdElem>& ppd)
{
    ASSERT(ppd);
    m_ppd = *ppd;
}

void IatciBprParamsMaker::setChd(const boost::optional<edifact::ChdElem>& chd, bool required)
{
    if(required)
        ASSERT(chd);
    m_chd = chd;
}

iatci::BprParams IatciBprParamsMaker::makeParams() const
{
    iatci::OriginatorDetails origDetails(m_lor.m_airline.empty() ? ""
                                    : BaseTables::Company(m_lor.m_airline)->rcode(),
                                         m_lor.m_port.empty() ? ""
                                    : BaseTables::Port(m_lor.m_port)->rcode());

    iatci::FlightDetails flight(BaseTables::Company(m_fdq.m_outbAirl)->rcode(),
                                m_fdq.m_outbFlNum,
                                BaseTables::Port(m_fdq.m_outbDepPoint)->rcode(),
                                BaseTables::Port(m_fdq.m_outbArrPoint)->rcode(),
                                m_fdq.m_outbDepDate,
                                Dates::Date_t(),
                                m_fdq.m_outbDepTime);

    iatci::FlightDetails prevFlight(m_fdq.m_inbAirl.empty() ? ""
                                : BaseTables::Company(m_fdq.m_inbAirl)->rcode(),
                                    m_fdq.m_inbFlNum,
                                    m_fdq.m_inbDepPoint.empty() ? ""
                                : BaseTables::Port(m_fdq.m_inbDepPoint)->rcode(),
                                    m_fdq.m_inbArrPoint.empty() ? ""
                                : BaseTables::Port(m_fdq.m_inbArrPoint)->rcode(),
                                    m_fdq.m_inbDepDate,
                                    m_fdq.m_inbArrDate,
                                    m_fdq.m_inbDepTime,
                                    m_fdq.m_inbArrTime);

    iatci::PaxDetails paxDetails(m_ppd.m_passSurname,
                                 m_ppd.m_passName,
                                 iatci::PaxDetails::strToType(m_ppd.m_passType),
                                 boost::none,
                                 m_ppd.m_passQryRef,
                                 m_ppd.m_passRespRef);

    boost::optional<iatci::CascadeHostDetails> cascadeHostDetails;
    if(m_chd) {
        cascadeHostDetails = iatci::CascadeHostDetails(m_chd->m_origAirline.empty() ? ""
                                                    : BaseTables::Company(m_chd->m_origAirline)->rcode(),
                                                       m_chd->m_origPoint.empty() ? ""
                                                    : BaseTables::Port(m_chd->m_origPoint)->rcode());
        BOOST_FOREACH(const std::string& hostAirline, m_chd->m_hostAirlines) {
            cascadeHostDetails->addHostAirline(BaseTables::Company(hostAirline)->rcode());
        }
    }

    return iatci::BprParams(origDetails,
                            paxDetails,
                            flight,
                            prevFlight,
                            cascadeHostDetails);
}

}//namespace TlgHandling
