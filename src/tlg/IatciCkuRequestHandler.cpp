#include "IatciCkuRequestHandler.h"
#include "IatciCkuRequest.h"
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


class IatciCkuParamsMaker
{
    edifact::LorElem m_lor;
    edifact::FdqElem m_fdq;
    edifact::PpdElem m_ppd;
    boost::optional<edifact::ChdElem> m_chd;
    boost::optional<edifact::UpdElem> m_upd;
    boost::optional<edifact::UsdElem> m_usd;
    boost::optional<edifact::UbdElem> m_ubd;

public:
    void setLor(const boost::optional<edifact::LorElem>& lor);
    void setFdq(const boost::optional<edifact::FdqElem>& fdq);
    void setPpd(const boost::optional<edifact::PpdElem>& ppd);
    void setChd(const boost::optional<edifact::ChdElem>& chd, bool required = false);
    void setUpd(const boost::optional<edifact::UpdElem>& upd, bool required = false);
    void setUsd(const boost::optional<edifact::UsdElem>& usd, bool required = false);
    void setUbd(const boost::optional<edifact::UbdElem>& ubd, bool required = false);
    iatci::CkuParams makeParams() const;
};

//---------------------------------------------------------------------------------------

IatciCkuRequestHandler::IatciCkuRequestHandler(_EDI_REAL_MES_STRUCT_* pMes,
                                               const edilib::EdiSessRdData *edisess)
    : IatciRequestHandler(pMes, edisess)
{
}

void IatciCkuRequestHandler::parse()
{
    IatciCkuParamsMaker ckuParamsMaker;
    ckuParamsMaker.setLor(readEdiLor(pMes()));
    ckuParamsMaker.setChd(readEdiChd(pMes()));

    SetEdiPointToSegGrG(pMes(), SegGrElement(1), "PROG_ERR");
    ckuParamsMaker.setFdq(readEdiFdq(pMes()));

    int paxCount = GetNumSegGr(pMes(), 2); // Сколько пассажиров регистрируется
    ASSERT(paxCount > 0); // Пассажиры должны быть обязательно

    if(paxCount > 1) {
        LogError(STDLOG) << "Warning: cku request for several passengers!";
    }

    EdiPointHolder grp_holder(pMes());
    SetEdiPointToSegGrG(pMes(), 2, 0, "PROG_ERR");
    ckuParamsMaker.setPpd(readEdiPpd(pMes())); /* PPD должен быть обязательно в Sg2 */
    ckuParamsMaker.setUpd(readEdiUpd(pMes()));
    ckuParamsMaker.setUsd(readEdiUsd(pMes()));
    ckuParamsMaker.setUbd(readEdiUbd(pMes()));

    m_ckuParams = ckuParamsMaker.makeParams();
}

std::string IatciCkuRequestHandler::respType() const
{
    return "U";
}

boost::optional<iatci::BaseParams> IatciCkuRequestHandler::params() const
{
    return ckuParams();
}

boost::optional<iatci::BaseParams> IatciCkuRequestHandler::nextParams() const
{
    if(nextCkuParams()) {
        return *nextCkuParams();
    }

    return boost::none;
}

iatci::Result IatciCkuRequestHandler::handleRequest() const
{
    return iatci::updateCheckin(ckuParams());
}

edilib::EdiSessionId_t IatciCkuRequestHandler::sendCascadeRequest() const
{
    ASSERT(nextCkuParams());
    return edifact::SendCkuRequest(*nextCkuParams());
}

const iatci::CkuParams& IatciCkuRequestHandler::ckuParams() const
{
    ASSERT(m_ckuParams);
    return m_ckuParams.get();
}

boost::optional<iatci::CkuParams> IatciCkuRequestHandler::nextCkuParams() const
{
    // TODO
    return boost::none;
}

//---------------------------------------------------------------------------------------

void IatciCkuParamsMaker::setLor(const boost::optional<edifact::LorElem>& lor)
{
    ASSERT(lor);
    m_lor = *lor;
}

void IatciCkuParamsMaker::setFdq(const boost::optional<edifact::FdqElem>& fdq)
{
    ASSERT(fdq);
    m_fdq = *fdq;
}

void IatciCkuParamsMaker::setPpd(const boost::optional<edifact::PpdElem>& ppd)
{
    ASSERT(ppd);
    m_ppd = *ppd;
}

void IatciCkuParamsMaker::setChd(const boost::optional<edifact::ChdElem>& chd, bool required)
{
    if(required)
        ASSERT(chd);
    m_chd = chd;
}

void IatciCkuParamsMaker::setUpd(const boost::optional<edifact::UpdElem>& upd, bool required)
{
    if(required)
        ASSERT(upd);
    m_upd = upd;
}

void IatciCkuParamsMaker::setUsd(const boost::optional<edifact::UsdElem>& usd, bool required)
{
    if(required)
        ASSERT(usd);
    m_usd = usd;
}

void IatciCkuParamsMaker::setUbd(const boost::optional<edifact::UbdElem>& ubd, bool required)
{
    if(required)
        ASSERT(ubd);
    m_ubd = ubd;
}

iatci::CkuParams IatciCkuParamsMaker::makeParams() const
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

    iatci::PaxDetails paxDetails(m_ppd.m_passSurname,
                                 m_ppd.m_passName,
                                 iatci::PaxDetails::strToType(m_ppd.m_passType),
                                 m_ppd.m_passQryRef,
                                 m_ppd.m_passRespRef);

    boost::optional<iatci::CascadeHostDetails> cascadeHostDetails;
    if(m_chd) {
        cascadeHostDetails = iatci::CascadeHostDetails(m_chd->m_origAirline,
                                                       m_chd->m_origPoint);
        BOOST_FOREACH(const std::string& hostAirline, m_chd->m_hostAirlines) {
            cascadeHostDetails->addHostAirline(hostAirline);
        }
    }

    boost::optional<iatci::UpdatePaxDetails> updatePaxDetails;
    if(m_upd) {
        updatePaxDetails = iatci::UpdatePaxDetails(iatci::UpdatePaxDetails::strToActionCode(m_upd->m_actionCode),
                                                   m_upd->m_surname,
                                                   m_upd->m_name,
                                                   m_upd->m_passQryRef);
    }

    boost::optional<iatci::UpdateSeatDetails> updateSeatDetails;
    if(m_usd) {
        updateSeatDetails = iatci::UpdateSeatDetails(iatci::UpdateSeatDetails::strToActionCode(m_usd->m_actionCode),
                                                     m_usd->m_seat,
                                                     iatci::UpdateSeatDetails::strToSmokeInd(m_usd->m_noSmokingInd));
    }

    boost::optional<iatci::UpdateBaggageDetails> updateBaggageDetails;
    if(m_ubd) {
        updateBaggageDetails = iatci::UpdateBaggageDetails(iatci::UpdateBaggageDetails::strToActionCode(m_ubd->m_actionCode),
                                                           m_ubd->m_numOfPieces, m_ubd->m_weight);
    }

    return iatci::CkuParams(origDetails,
                            paxDetails,
                            flight,
                            boost::none, // TODO
                            updatePaxDetails,
                            updateSeatDetails,
                            updateBaggageDetails);

}

}//namespace TlgHandling
