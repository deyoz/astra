#include "IatciBprRequestHandler.h"
#include "IatciBprRequest.h"
#include "read_edi_elements.h"
#include "view_edi_elements.h"
#include "basetables.h"
#include "remote_system_context.h"
#include "iatci_types.h"
#include "iatci_help.h"
#include "iatci_api.h"
#include "astra_msg.h"

#include <serverlib/algo.h>
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


class IatciBprParamsNewMaker
{
public:

    class Pxg
    {
    private:
        edifact::PpdElem                  m_ppd;
        boost::optional<edifact::PrdElem> m_prd;
        boost::optional<edifact::PsdElem> m_psd;
        boost::optional<edifact::PbdElem> m_pbd;
        boost::optional<edifact::PsiElem> m_psi;

    public:
        void setPpd(const boost::optional<edifact::PpdElem>& ppd);
        void setPrd(const boost::optional<edifact::PrdElem>& prd, bool required = false);
        void setPsd(const boost::optional<edifact::PsdElem>& psd, bool required = false);
        void setPbd(const boost::optional<edifact::PbdElem>& pbd, bool required = false);
        void setPsi(const boost::optional<edifact::PsiElem>& psi, bool required = false);

        iatci::dcqbpr::PaxGroup makePaxGroup() const;

    protected:
        boost::optional<iatci::ReservationDetails> makeReserv() const;
        boost::optional<iatci::SeatDetails>        makeSeat() const;
        boost::optional<iatci::BaggageDetails>     makeBaggage() const;
        boost::optional<iatci::ServiceDetails>     makeService() const;
    };

    //-----------------------------------------------------------------------------------

private:
    edifact::LorElem                  m_lor;
    boost::optional<edifact::ChdElem> m_chd;
    edifact::FdqElem                  m_fdq;
    std::list<Pxg>                    m_lPxg;

public:
    void setLor(const boost::optional<edifact::LorElem>& lor);
    void setChd(const boost::optional<edifact::ChdElem>& chd, bool required = false);
    void setFdq(const boost::optional<edifact::FdqElem>& fdq);
    void addPxg(const Pxg& pxg);

    iatci::BprParams makeParams() const;

protected:
    iatci::dcqbpr::FlightGroup                 makeFlightGroup() const;
    boost::optional<iatci::CascadeHostDetails> makeCascade() const;
};

//---------------------------------------------------------------------------------------

IatciBprRequestHandler::IatciBprRequestHandler(_EDI_REAL_MES_STRUCT_* pMes,
                                               const edilib::EdiSessRdData *edisess)
    : IatciRequestHandler(pMes, edisess)
{
}

void IatciBprRequestHandler::parse()
{
    IatciBprParamsNewMaker bprParamsNewMaker;
    bprParamsNewMaker.setLor(readEdiLor(pMes()));
    bprParamsNewMaker.setChd(readEdiChd(pMes()));

    SetEdiPointToSegGrG(pMes(), SegGrElement(1), "PROG_ERR");
    bprParamsNewMaker.setFdq(readEdiFdq(pMes()));

    int paxCount = GetNumSegGr(pMes(), 2);
    ASSERT(paxCount > 0);

    EdiPointHolder ph(pMes());
    for(int curPax = 0; curPax < paxCount; ++curPax)
    {
        SetEdiPointToSegGrG(pMes(), SegGrElement(2, curPax), "PROG_ERR");

        IatciBprParamsNewMaker::Pxg pxg;
        pxg.setPpd(readEdiPpd(pMes()));
        pxg.setPrd(readEdiPrd(pMes()));
        pxg.setPsd(readEdiPsd(pMes()));
        pxg.setPbd(readEdiPbd(pMes()));
        pxg.setPsi(readEdiPsi(pMes()));

        bprParamsNewMaker.addPxg(pxg);

        PopEdiPoint_wdG(pMes());
    }

    m_bprParamsNew = bprParamsNewMaker.makeParams();
}

std::string IatciBprRequestHandler::respType() const
{
    return "B";
}

iatci::dcrcka::Result IatciBprRequestHandler::handleRequest() const
{
    ASSERT(m_bprParamsNew);
    return iatci::reprintBoardingPass(m_bprParamsNew.get());
}

edilib::EdiSessionId_t IatciBprRequestHandler::sendCascadeRequest() const
{
    throw "Not implemented!";
}

const iatci::IBaseParams* IatciBprRequestHandler::paramsNew() const
{
    return m_bprParamsNew.get_ptr();
}

//---------------------------------------------------------------------------------------

void IatciBprParamsNewMaker::Pxg::setPpd(const boost::optional<edifact::PpdElem>& ppd)
{
    ASSERT(ppd);
    m_ppd = *ppd;
}

void IatciBprParamsNewMaker::Pxg::setPrd(const boost::optional<edifact::PrdElem>& prd,
                                         bool required)
{
    if(required)
        ASSERT(prd);
    m_prd = prd;
}

void IatciBprParamsNewMaker::Pxg::setPsd(const boost::optional<edifact::PsdElem>& psd,
                                         bool required)
{
    if(required)
        ASSERT(psd);
    m_psd = psd;
}

void IatciBprParamsNewMaker::Pxg::setPbd(const boost::optional<edifact::PbdElem>& pbd,
                                         bool required)
{
    if(required)
        ASSERT(pbd);
    m_pbd = pbd;
}

void IatciBprParamsNewMaker::Pxg::setPsi(const boost::optional<edifact::PsiElem>& psi,
                                         bool required)
{
    if(required)
        ASSERT(psi);
    m_psi = psi;
}

iatci::dcqbpr::PaxGroup IatciBprParamsNewMaker::Pxg::makePaxGroup() const
{
    return iatci::dcqbpr::PaxGroup(iatci::makePax(m_ppd),
                                   makeReserv(),
                                   makeBaggage(),
                                   makeService());
}

boost::optional<iatci::ReservationDetails> IatciBprParamsNewMaker::Pxg::makeReserv() const
{
    if(m_prd) {
        return iatci::makeReserv(*m_prd);
    }

    return boost::none;
}

boost::optional<iatci::SeatDetails> IatciBprParamsNewMaker::Pxg::makeSeat() const
{
    if(m_psd) {
        return iatci::makeSeat(*m_psd);
    }

    return boost::none;
}

boost::optional<iatci::BaggageDetails> IatciBprParamsNewMaker::Pxg::makeBaggage() const
{
    if(m_pbd) {
        return iatci::makeBaggage(*m_pbd);
    }

    return boost::none;
}

boost::optional<iatci::ServiceDetails> IatciBprParamsNewMaker::Pxg::makeService() const
{
    if(m_psi) {
        return iatci::makeService(*m_psi);
    }

    return boost::none;
}

//

void IatciBprParamsNewMaker::setLor(const boost::optional<edifact::LorElem>& lor)
{
    ASSERT(lor);
    m_lor = *lor;
}

void IatciBprParamsNewMaker::setChd(const boost::optional<edifact::ChdElem>& chd,
                                    bool required)
{
    if(required)
        ASSERT(chd);
    m_chd = chd;
}

void IatciBprParamsNewMaker::setFdq(const boost::optional<edifact::FdqElem>& fdq)
{
    ASSERT(fdq);
    m_fdq = *fdq;
}

void IatciBprParamsNewMaker::addPxg(const Pxg& pxg)
{
    m_lPxg.push_back(pxg);
}

iatci::BprParams IatciBprParamsNewMaker::makeParams() const
{
    return iatci::BprParams(iatci::makeOrg(m_lor),
                            makeCascade(),
                            makeFlightGroup());
}

iatci::dcqbpr::FlightGroup IatciBprParamsNewMaker::makeFlightGroup() const
{
    const auto paxGroups = algo::transform(m_lPxg, [](const Pxg& pxg) {
        return pxg.makePaxGroup();
    });
    return iatci::dcqbpr::FlightGroup(iatci::makeOutboundFlight(m_fdq),
                                      iatci::makeInboundFlight(m_fdq),
                                      paxGroups);
}

boost::optional<iatci::CascadeHostDetails> IatciBprParamsNewMaker::makeCascade() const
{
    if(m_chd) {
        return iatci::makeCascade(*m_chd);
    }

    return boost::none;
}

}//namespace TlgHandling
