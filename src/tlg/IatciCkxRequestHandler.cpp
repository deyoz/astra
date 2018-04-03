#include "IatciCkxRequestHandler.h"
#include "IatciCkxRequest.h"
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
#include <serverlib/algo.h>

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


class IatciCkxParamsMaker
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
        //boost::optional<edifact::CrdElem> m_crd;

    public:
        void setPpd(const boost::optional<edifact::PpdElem>& ppd);
        void setPrd(const boost::optional<edifact::PrdElem>& prd, bool required = false);
        void setPsd(const boost::optional<edifact::PsdElem>& psd, bool required = false);
        void setPbd(const boost::optional<edifact::PbdElem>& pbd, bool required = false);
        void setPsi(const boost::optional<edifact::PsiElem>& psi, bool required = false);

        iatci::dcqckx::PaxGroup makePaxGroup() const;

    protected:
        boost::optional<iatci::ReservationDetails> makeReserv() const;
        boost::optional<iatci::SeatDetails>        makeSeat() const;
        boost::optional<iatci::BaggageDetails>     makeBaggage() const;
        boost::optional<iatci::ServiceDetails>     makeService() const;
        boost::optional<iatci::PaxDetails>         makeInfant() const;
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

    iatci::CkxParams makeParams() const;

protected:
    iatci::dcqckx::FlightGroup                 makeFlightGroup() const;
    boost::optional<iatci::CascadeHostDetails> makeCascade() const;
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
    IatciCkxParamsMaker ckxParamsNewMaker;
    ckxParamsNewMaker.setLor(readEdiLor(pMes()));
    ckxParamsNewMaker.setChd(readEdiChd(pMes()));

    SetEdiPointToSegGrG(pMes(), SegGrElement(1), "PROG_ERR");
    ckxParamsNewMaker.setFdq(readEdiFdq(pMes()));

    int paxCount = GetNumSegGr(pMes(), 2);
    ASSERT(paxCount > 0);

    EdiPointHolder ph(pMes());
    for(int curPax = 0; curPax < paxCount; ++curPax)
    {
        SetEdiPointToSegGrG(pMes(), SegGrElement(2, curPax), "PROG_ERR");

        IatciCkxParamsMaker::Pxg pxg;
        pxg.setPpd(readEdiPpd(pMes()));
        pxg.setPrd(readEdiPrd(pMes()));
        pxg.setPsd(readEdiPsd(pMes()));
        pxg.setPbd(readEdiPbd(pMes()));
        pxg.setPsi(readEdiPsi(pMes()));

        ckxParamsNewMaker.addPxg(pxg);

        PopEdiPoint_wdG(pMes());
    }

    m_ckxParamsNew = ckxParamsNewMaker.makeParams();
}

std::string IatciCkxRequestHandler::respType() const
{
    return "X";
}

const iatci::IBaseParams* IatciCkxRequestHandler::paramsNew() const
{
    return m_ckxParamsNew.get_ptr();
}

iatci::dcrcka::Result IatciCkxRequestHandler::handleRequest() const
{
    LogTrace(TRACE3) << "Enter to " << __FUNCTION__;

    if(postponeHandling()) {
        LogTrace(TRACE3) << "postpone handling for tlg " << inboundTlgNum();

        return iatci::cancelCheckin(inboundTlgNum());
    }

    ASSERT(m_ckxParamsNew);
    return iatci::cancelCheckin(m_ckxParamsNew.get());
}

edilib::EdiSessionId_t IatciCkxRequestHandler::sendCascadeRequest() const
{
    throw "Not implemented!";
}

//---------------------------------------------------------------------------------------

void IatciCkxParamsMaker::Pxg::setPpd(const boost::optional<edifact::PpdElem>& ppd)
{
    ASSERT(ppd);
    m_ppd = *ppd;
}

void IatciCkxParamsMaker::Pxg::setPrd(const boost::optional<edifact::PrdElem>& prd,
                                      bool required)
{
    if(required)
        ASSERT(prd);
    m_prd = prd;
}

void IatciCkxParamsMaker::Pxg::setPsd(const boost::optional<edifact::PsdElem>& psd,
                                      bool required)
{
    if(required)
        ASSERT(psd);
    m_psd = psd;
}

void IatciCkxParamsMaker::Pxg::setPbd(const boost::optional<edifact::PbdElem>& pbd,
                                      bool required)
{
    if(required)
        ASSERT(pbd);
    m_pbd = pbd;
}

void IatciCkxParamsMaker::Pxg::setPsi(const boost::optional<edifact::PsiElem>& psi,
                                      bool required)
{
    if(required)
        ASSERT(psi);
    m_psi = psi;
}

iatci::dcqckx::PaxGroup IatciCkxParamsMaker::Pxg::makePaxGroup() const
{
    return iatci::dcqckx::PaxGroup(iatci::makePax(m_ppd),
                                   makeReserv(),
                                   makeSeat(),
                                   makeBaggage(),
                                   makeService(),
                                   makeInfant());
}

boost::optional<iatci::ReservationDetails> IatciCkxParamsMaker::Pxg::makeReserv() const
{
    if(m_prd) {
        return iatci::makeReserv(*m_prd);
    }

    return boost::none;
}

boost::optional<iatci::SeatDetails> IatciCkxParamsMaker::Pxg::makeSeat() const
{
    if(m_psd) {
        return iatci::makeSeat(*m_psd);
    }

    return boost::none;
}

boost::optional<iatci::BaggageDetails> IatciCkxParamsMaker::Pxg::makeBaggage() const
{
    if(m_pbd) {
        return iatci::makeBaggage(*m_pbd);
    }

    return boost::none;
}

boost::optional<iatci::ServiceDetails> IatciCkxParamsMaker::Pxg::makeService() const
{
    if(m_psi) {
        return iatci::makeService(*m_psi);
    }

    return boost::none;
}

boost::optional<iatci::PaxDetails> IatciCkxParamsMaker::Pxg::makeInfant() const
{
    if(m_ppd.m_withInftIndicator == "Y" && !m_ppd.m_inftName.empty()) {
        return iatci::makeInfant(m_ppd);
    }

    return boost::none;
}

//

void IatciCkxParamsMaker::setLor(const boost::optional<edifact::LorElem>& lor)
{
    ASSERT(lor);
    m_lor = *lor;
}

void IatciCkxParamsMaker::setChd(const boost::optional<edifact::ChdElem>& chd,
                                 bool required)
{
    if(required)
        ASSERT(chd);
    m_chd = chd;
}

void IatciCkxParamsMaker::setFdq(const boost::optional<edifact::FdqElem>& fdq)
{
    ASSERT(fdq);
    m_fdq = *fdq;
}

void IatciCkxParamsMaker::addPxg(const Pxg& pxg)
{
    m_lPxg.push_back(pxg);
}

iatci::CkxParams IatciCkxParamsMaker::makeParams() const
{
    return iatci::CkxParams(iatci::makeOrg(m_lor),
                            makeCascade(),
                            makeFlightGroup());
}

iatci::dcqckx::FlightGroup IatciCkxParamsMaker::makeFlightGroup() const
{
    const auto paxGroups = algo::transform(m_lPxg, [](const Pxg& pxg) {
        return pxg.makePaxGroup();
    });
    return iatci::dcqckx::FlightGroup(iatci::makeOutboundFlight(m_fdq),
                                      iatci::makeInboundFlight(m_fdq),
                                      paxGroups);
}

boost::optional<iatci::CascadeHostDetails> IatciCkxParamsMaker::makeCascade() const
{
    if(m_chd) {
        return iatci::makeCascade(*m_chd);
    }

    return boost::none;
}

}//namespace TlgHandling
