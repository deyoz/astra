#include "IatciCkiRequestHandler.h"
#include "IatciCkiRequest.h"
#include "read_edi_elements.h"
#include "view_edi_elements.h"
#include "remote_system_context.h"
#include "basetables.h"
#include "iatci_types.h"
#include "iatci_api.h"
#include "astra_msg.h"
#include "astra_utils.h"
#include "astra_api.h" // TODO dropme

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


class IatciCkiParamsMaker
{
private:
    edifact::LorElem m_lor;
    edifact::FdqElem m_fdq;
    edifact::PpdElem m_ppd;
    boost::optional<edifact::PrdElem> m_prd;
    boost::optional<edifact::PsdElem> m_psd;
    boost::optional<edifact::PbdElem> m_pbd;
    boost::optional<edifact::ChdElem> m_chd;

public:
    void setLor(const boost::optional<edifact::LorElem>& lor);
    void setFdq(const boost::optional<edifact::FdqElem>& fdq);
    void setPpd(const boost::optional<edifact::PpdElem>& ppd);
    void setPrd(const boost::optional<edifact::PrdElem>& prd, bool required = false);
    void setPsd(const boost::optional<edifact::PsdElem>& psd, bool required = false);
    void setPbd(const boost::optional<edifact::PbdElem>& pbd, bool required = false);
    void setChd(const boost::optional<edifact::ChdElem>& chd, bool required = false);
    iatci::CkiParams makeParams() const;
};

//---------------------------------------------------------------------------------------

IatciCkiRequestHandler::IatciCkiRequestHandler(_EDI_REAL_MES_STRUCT_* pMes,
                                               const edilib::EdiSessRdData *edisess)
    : IatciRequestHandler(pMes, edisess)
{
}

void IatciCkiRequestHandler::parse()
{
    IatciCkiParamsMaker ckiParamsMaker;
    ckiParamsMaker.setLor(readEdiLor(pMes())); /* LOR ������ ���� ��易⥫쭮 */
    ckiParamsMaker.setChd(readEdiChd(pMes()));

    SetEdiPointToSegGrG(pMes(), SegGrElement(1), "PROG_ERR");
    ckiParamsMaker.setFdq(readEdiFdq(pMes())); /* FDQ ������ ���� ��易⥫쭮 */

    int paxCount = GetNumSegGr(pMes(), 2); // ����쪮 ���ᠦ�஢ ॣ���������
    ASSERT(paxCount > 0); // ���ᠦ��� ������ ���� ��易⥫쭮

    if(paxCount > 1) {
        LogError(STDLOG) << "Warning: cki request for several passengers!";
    }

    EdiPointHolder grp_holder(pMes());
    SetEdiPointToSegGrG(pMes(), 2, 0, "PROG_ERR");
    ckiParamsMaker.setPpd(readEdiPpd(pMes())); /* PPD ������ ���� ��易⥫쭮 � Sg2 */
    ckiParamsMaker.setPrd(readEdiPrd(pMes()));
    ckiParamsMaker.setPsd(readEdiPsd(pMes()));
    ckiParamsMaker.setPbd(readEdiPbd(pMes()));

    m_ckiParams = ckiParamsMaker.makeParams();
}

std::string IatciCkiRequestHandler::respType() const
{
    return "I";
}

iatci::Result IatciCkiRequestHandler::handleRequest() const
{
    LogTrace(TRACE3) << "Enter to " << __FUNCTION__;

    if(postponeHandling()) {
        LogTrace(TRACE3) << "postpone handling for tlg " << inboundTlgNum();

        return iatci::checkinPax(inboundTlgNum());
    }

    return iatci::checkinPax(ckiParams());
}

edilib::EdiSessionId_t IatciCkiRequestHandler::sendCascadeRequest() const
{
    ASSERT(nextCkiParams());
    return edifact::SendCkiRequest(*nextCkiParams());
}

boost::optional<iatci::BaseParams> IatciCkiRequestHandler::params() const
{
    return ckiParams();
}

boost::optional<iatci::BaseParams> IatciCkiRequestHandler::nextParams() const
{
    if(nextCkiParams()) {
        return *nextCkiParams();
    }

    return boost::none;
}

const iatci::CkiParams& IatciCkiRequestHandler::ckiParams() const
{
    ASSERT(m_ckiParams);
    return m_ckiParams.get();
}

boost::optional<iatci::CkiParams> IatciCkiRequestHandler::nextCkiParams() const
{
    boost::optional<iatci::FlightDetails> flightForNextHost;
    flightForNextHost = iatci::findCascadeFlight(ckiParams().flight());
    if(!flightForNextHost) {
        tst();
        return boost::none;
    }

    iatci::PaxDetails pax = iatci::PaxDetails(ckiParams().pax().surname(),
                                              ckiParams().pax().name(),
                                              ckiParams().pax().type(),
                                              boost::none,
                                              ckiParams().flight().toShortKeyString());

    iatci::CascadeHostDetails cascadeDetails(ckiParams().origin().airline(),
                                             ckiParams().origin().port());
    cascadeDetails.addHostAirline(ckiParams().flight().airline());
    if(ckiParams().flightFromPrevHost()) {
        cascadeDetails.addHostAirline(ckiParams().flightFromPrevHost()->airline());
    }

    return iatci::CkiParams(iatci::OriginatorDetails(ckiParams().flight().airline()),
                            pax,
                            *flightForNextHost,
                            ckiParams().flight(),
                            ckiParams().seat(),
                            ckiParams().baggage(),
                            ckiParams().reserv(),
                            cascadeDetails);
}

//---------------------------------------------------------------------------------------

void IatciCkiParamsMaker::setLor(const boost::optional<edifact::LorElem>& lor)
{
    ASSERT(lor);
    m_lor = *lor;
}

void IatciCkiParamsMaker::setFdq(const boost::optional<edifact::FdqElem>& fdq)
{
    ASSERT(fdq);
    m_fdq = *fdq;
}

void IatciCkiParamsMaker::setPpd(const boost::optional<edifact::PpdElem>& ppd)
{
    ASSERT(ppd);
    m_ppd = *ppd;
}

void IatciCkiParamsMaker::setPrd(const boost::optional<edifact::PrdElem>& prd, bool required)
{
    if(required)
        ASSERT(prd);
    m_prd = prd;
}

void IatciCkiParamsMaker::setPsd(const boost::optional<edifact::PsdElem>& psd, bool required)
{
    if(required)
        ASSERT(psd);
    m_psd = psd;
}

void IatciCkiParamsMaker::setPbd(const boost::optional<edifact::PbdElem>& pbd, bool required)
{
    if(required)
        ASSERT(pbd);
    m_pbd = pbd;
}

void IatciCkiParamsMaker::setChd(const boost::optional<edifact::ChdElem>& chd, bool required)
{
    if(required)
        ASSERT(chd);
    m_chd = chd;
}

iatci::CkiParams IatciCkiParamsMaker::makeParams() const
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

    boost::optional<iatci::ReservationDetails> reservDetails;
    if(m_prd) {
        reservDetails = iatci::ReservationDetails(m_prd->m_rbd);
    }

    boost::optional<iatci::SeatDetails> seatDetails;
    if(m_psd) {
        seatDetails = iatci::SeatDetails(m_psd->m_seat,
                                         iatci::SeatDetails::strToSmokeInd((m_psd->m_noSmokingInd)));
    }

    boost::optional<iatci::BaggageDetails> baggageDetails;
    if(m_pbd) {
        baggageDetails = iatci::BaggageDetails(m_pbd->m_numOfPieces,
                                               m_pbd->m_weight);
    }

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

    return iatci::CkiParams(origDetails,
                            paxDetails,
                            flight,
                            prevFlight,
                            seatDetails,
                            baggageDetails,
                            reservDetails,
                            cascadeHostDetails);
}

}//namespace TlgHandling
