#include "IatciPlfRequestHandler.h"
#include "IatciPlfRequest.h"
#include "read_edi_elements.h"
#include "view_edi_elements.h"
#include "postpone_edifact.h"
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


class IatciPlfParamsMaker
{
    edifact::LorElem m_lor;
    edifact::FdqElem m_fdq;
    edifact::SpdElem m_spd;
    boost::optional<edifact::ChdElem> m_chd;

public:
    void setLor(const boost::optional<edifact::LorElem>& lor);
    void setFdq(const boost::optional<edifact::FdqElem>& fdq);
    void setSpd(const boost::optional<edifact::SpdElem>& spd);
    void setChd(const boost::optional<edifact::ChdElem>& chd, bool required = false);
    iatci::PlfParams makeParams() const;
};

//---------------------------------------------------------------------------------------

IatciPlfRequestHandler::IatciPlfRequestHandler(_EDI_REAL_MES_STRUCT_ *pMes,
                                               const edilib::EdiSessRdData *edisess)
    : IatciRequestHandler(pMes, edisess)
{
}

void IatciPlfRequestHandler::parse()
{
    IatciPlfParamsMaker plfParamsMaker;
    plfParamsMaker.setLor(readEdiLor(pMes()));
    plfParamsMaker.setFdq(readEdiFdq(pMes()));
    plfParamsMaker.setSpd(readEdiSpd(pMes()));
    plfParamsMaker.setChd(readEdiChd(pMes()));
    m_plfParams = plfParamsMaker.makeParams();
}

std::string IatciPlfRequestHandler::respType() const
{
    return "P";
}

boost::optional<iatci::Params> IatciPlfRequestHandler::params() const
{
    return plfParams();
}

boost::optional<iatci::Params> IatciPlfRequestHandler::nextParams() const
{
    if(nextPlfParams()) {
        return *nextPlfParams();
    }

    return boost::none;
}

iatci::Result IatciPlfRequestHandler::handleRequest() const
{
    return iatci::fillPasslist(plfParams());
}

edilib::EdiSessionId_t IatciPlfRequestHandler::sendCascadeRequest() const
{
    ASSERT(nextPlfParams());
    return edifact::SendPlfRequest(*nextPlfParams());
}

const iatci::PlfParams& IatciPlfRequestHandler::plfParams() const
{
    ASSERT(m_plfParams);
    return m_plfParams.get();
}

boost::optional<iatci::PlfParams> IatciPlfRequestHandler::nextPlfParams() const
{
    // TODO
    return boost::none;
}

//---------------------------------------------------------------------------------------

void IatciPlfParamsMaker::setLor(const boost::optional<edifact::LorElem>& lor)
{
    ASSERT(lor);
    m_lor = *lor;
}

void IatciPlfParamsMaker::setFdq(const boost::optional<edifact::FdqElem>& fdq)
{
    ASSERT(fdq);
    m_fdq = *fdq;
}

void IatciPlfParamsMaker::setSpd(const boost::optional<edifact::SpdElem>& spd)
{
    ASSERT(spd);
    m_spd = *spd;
}

void IatciPlfParamsMaker::setChd(const boost::optional<edifact::ChdElem>& chd, bool required)
{
    if(required)
        ASSERT(chd);
    m_chd = chd;
}

iatci::PlfParams IatciPlfParamsMaker::makeParams() const
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

    iatci::PaxSeatDetails paxSeatDetails(m_spd.m_passSurname,
                                         m_spd.m_passName,
                                         m_spd.m_rbd,
                                         m_spd.m_passSeat,
                                         m_spd.m_securityId,
                                         m_spd.m_recloc,
                                         m_spd.m_tickNum,
                                         m_spd.m_passQryRef,
                                         m_spd.m_passRespRef);

    boost::optional<iatci::CascadeHostDetails> cascadeHostDetails;
    if(m_chd) {
        cascadeHostDetails = iatci::CascadeHostDetails(m_chd->m_origAirline,
                                                       m_chd->m_origPoint);
        BOOST_FOREACH(const std::string& hostAirline, m_chd->m_hostAirlines) {
            cascadeHostDetails->addHostAirline(hostAirline);
        }
    }

    return iatci::PlfParams(origDetails,
                            flight,
                            paxSeatDetails,
                            cascadeHostDetails);
}

}//namespace TlgHandling
