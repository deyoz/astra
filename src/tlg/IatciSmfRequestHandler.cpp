#include "IatciSmfRequestHandler.h"
#include "IatciSmfRequest.h"
#include "basetables.h"
#include "read_edi_elements.h"
#include "view_edi_elements.h"
#include "iatci_api.h"
#include "iatci_help.h"

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

protected:
    boost::optional<iatci::SeatRequestDetails> makeSeatReq() const;
    boost::optional<iatci::CascadeHostDetails> makeCascade() const;
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

std::list<iatci::dcrcka::Result> IatciSmfRequestHandler::handleRequest() const
{
    ASSERT(m_smfParams);
    return { iatci::fillSeatmap(m_smfParams.get()) };
}

const iatci::IBaseParams* IatciSmfRequestHandler::params() const
{
    return m_smfParams.get_ptr();
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
    return iatci::SmfParams(iatci::makeOrg(m_lor),
                            makeSeatReq(),
                            iatci::makeOutboundFlight(m_fdq),
                            iatci::makeInboundFlight(m_fdq),
                            makeCascade());
}

boost::optional<iatci::SeatRequestDetails> IatciSmfParamsMaker::makeSeatReq() const
{
    if(m_srp) {
        return iatci::makeSeatReq(*m_srp);
    }

    return boost::none;
}

boost::optional<iatci::CascadeHostDetails> IatciSmfParamsMaker::makeCascade() const
{
    if(m_chd) {
        return iatci::makeCascade(*m_chd);
    }

    return boost::none;
}

}//namespace TlgHandling
