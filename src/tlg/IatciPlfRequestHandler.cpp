#include "IatciPlfRequestHandler.h"
#include "IatciPlfRequest.h"
#include "read_edi_elements.h"
#include "view_edi_elements.h"
#include "postpone_edifact.h"
#include "basetables.h"
#include "remote_system_context.h"
#include "iatci_types.h"
#include "iatci_help.h"
#include "iatci_api.h"
#include "astra_msg.h"

#include <edilib/edi_func_cpp.h>
#include <etick/exceptions.h>

#include <boost/optional.hpp>

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

public:
    boost::optional<iatci::CascadeHostDetails> makeCascade() const;

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

std::string IatciPlfRequestHandler::fcIndicator() const
{
    return "T";
}

std::list<iatci::dcrcka::Result> IatciPlfRequestHandler::handleRequest() const
{
    ASSERT(m_plfParams);
    return { iatci::fillPasslist(m_plfParams.get()) };
}

const iatci::IBaseParams* IatciPlfRequestHandler::params() const
{
    return m_plfParams.get_ptr();
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
    return iatci::PlfParams(iatci::makeOrg(m_lor),
                            iatci::makePersonal(m_spd),
                            iatci::makeOutboundFlight(m_fdq),
                            iatci::makeInboundFlight(m_fdq),
                            makeCascade());
}

boost::optional<iatci::CascadeHostDetails> IatciPlfParamsMaker::makeCascade() const
{
    if(m_chd) {
        return iatci::makeCascade(*m_chd);
    }

    return boost::none;
}

}//namespace TlgHandling
