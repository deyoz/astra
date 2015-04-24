#include "IatciCkxRequestHandler.h"
#include "IatciCkxRequest.h"
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


class IatciCkxParamsMaker
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
    iatci::CkxParams makeParams() const;
};

//---------------------------------------------------------------------------------------

IatciCkxRequestHandler::IatciCkxRequestHandler(_EDI_REAL_MES_STRUCT_* pMes,
                                               const edilib::EdiSessRdData *edisess)
    : IatciRequestHandler(pMes, edisess)
{
}

bool IatciCkxRequestHandler::fullAnswer() const
{
    return false;
}

void IatciCkxRequestHandler::parse()
{
    IatciCkxParamsMaker ckxParamsMaker;
    ckxParamsMaker.setLor(readEdiLor(pMes())); /* LOR должен быть обязательно */
    ckxParamsMaker.setChd(readEdiChd(pMes()));

    SetEdiPointToSegGrG(pMes(), SegGrElement(1), "PROG_ERR");
    ckxParamsMaker.setFdq(readEdiFdq(pMes())); /* FDQ должен быть обязательно */

    int paxCount = GetNumSegGr(pMes(), 2); // Сколько пассажиров регистрируется
    ASSERT(paxCount > 0); // Пассажиры должны быть обязательно

    if(paxCount > 1) {
        LogError(STDLOG) << "Warning: ckx request for several passengers!";
    }

    EdiPointHolder grp_holder(pMes());
    SetEdiPointToSegGrG(pMes(), 2, 0, "PROG_ERR");
    ckxParamsMaker.setPpd(readEdiPpd(pMes())); /* PPD должен быть обязательно в Sg2 */


    m_ckxParams = ckxParamsMaker.makeParams();
}

std::string IatciCkxRequestHandler::respType() const
{
    return "X";
}

boost::optional<iatci::Params> IatciCkxRequestHandler::params() const
{
    return ckxParams();
}

boost::optional<iatci::Params> IatciCkxRequestHandler::nextParams() const
{
    if(nextCkxParams()) {
        return *nextCkxParams();
    }

    return boost::none;
}

iatci::Result IatciCkxRequestHandler::handleRequest() const
{
   return iatci::cancelCheckin(ckxParams());
}

edilib::EdiSessionId_t IatciCkxRequestHandler::sendCascadeRequest() const
{
    ASSERT(nextCkxParams());
    return edifact::SendCkxRequest(*nextCkxParams());
}

const iatci::CkxParams IatciCkxRequestHandler::ckxParams() const
{
    ASSERT(m_ckxParams);
    return *m_ckxParams;
}

boost::optional<iatci::CkxParams> IatciCkxRequestHandler::nextCkxParams() const
{
    // TODO
    return boost::none;
}

//---------------------------------------------------------------------------------------

void IatciCkxParamsMaker::setLor(const boost::optional<edifact::LorElem>& lor)
{
    ASSERT(lor);
    m_lor = *lor;
}

void IatciCkxParamsMaker::setFdq(const boost::optional<edifact::FdqElem>& fdq)
{
    ASSERT(fdq);
    m_fdq = *fdq;
}

void IatciCkxParamsMaker::setPpd(const boost::optional<edifact::PpdElem>& ppd)
{
    ASSERT(ppd);
    m_ppd = *ppd;
}

void IatciCkxParamsMaker::setChd(const boost::optional<edifact::ChdElem>& chd, bool required)
{
    if(required)
        ASSERT(chd);
    m_chd = chd;
}

iatci::CkxParams IatciCkxParamsMaker::makeParams() const
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

    return iatci::CkxParams(origDetails,
                            flight,
                            paxDetails,
                            cascadeHostDetails);
}



}//namespace TlgHandling
