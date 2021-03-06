#include "IatciCkiRequestHandler.h"
#include "IatciCkiRequest.h"
#include "read_edi_elements.h"
#include "view_edi_elements.h"
#include "remote_system_context.h"
#include "basetables.h"
#include "iatci_types.h"
#include "iatci_help.h"
#include "iatci_api.h"
#include "astra_msg.h"
#include "astra_utils.h"

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


class IatciCkiParamsMaker
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
        boost::optional<edifact::PapElem> m_pap;
        boost::optional<edifact::PapElem> m_papInfant;
        boost::optional<edifact::AddElem> m_add;
        boost::optional<edifact::AddElem> m_addInfant;

    public:
        void setPpd(const boost::optional<edifact::PpdElem>& ppd);
        void setPrd(const boost::optional<edifact::PrdElem>& prd, bool required = false);
        void setPsd(const boost::optional<edifact::PsdElem>& psd, bool required = false);
        void setPbd(const boost::optional<edifact::PbdElem>& pbd, bool required = false);
        void setPsi(const boost::optional<edifact::PsiElem>& psi, bool required = false);
        void addApg(const boost::optional<edifact::PapElem>& pap,
                    const boost::optional<edifact::AddElem>& add);

        iatci::dcqcki::PaxGroup makePaxGroup() const;

    protected:
        boost::optional<iatci::ReservationDetails> makeReserv() const;
        boost::optional<iatci::SeatDetails>        makeSeat() const;
        boost::optional<iatci::BaggageDetails>     makeBaggage() const;
        boost::optional<iatci::ServiceDetails>     makeService() const;
        boost::optional<iatci::DocDetails>         makeDoc() const;
        boost::optional<iatci::AddressDetails>     makeAddress() const;
        boost::optional<iatci::VisaDetails>        makeVisa() const;
        boost::optional<iatci::PaxDetails>         makeInfant() const;
        boost::optional<iatci::DocDetails>         makeInfantDoc() const;
        boost::optional<iatci::AddressDetails>     makeInfantAddress() const;
        boost::optional<iatci::VisaDetails>        makeInfantVisa() const;
    };

    //-----------------------------------------------------------------------------------

private:
    edifact::LorElem                  m_lor;
    boost::optional<edifact::ChdElem> m_chd;
    boost::optional<edifact::DmcElem> m_dmc;
    edifact::FdqElem                  m_fdq;
    std::list<Pxg>                    m_lPxg;

public:
    void setLor(const boost::optional<edifact::LorElem>& lor);
    void setChd(const boost::optional<edifact::ChdElem>& chd, bool required = false);
    void setDmc(const boost::optional<edifact::DmcElem>& dmc, bool required = false);
    void setFdq(const boost::optional<edifact::FdqElem>& fdq);    
    void addPxg(const Pxg& pxg);

    iatci::CkiParams makeParams() const;

protected:
    iatci::dcqcki::FlightGroup                 makeFlightGroup() const;
    boost::optional<iatci::CascadeHostDetails> makeCascade() const;
    iatci::MessageDetails                      makeMessageDetails() const;

};

//---------------------------------------------------------------------------------------

IatciCkiRequestHandler::IatciCkiRequestHandler(_EDI_REAL_MES_STRUCT_* pMes,
                                               const edilib::EdiSessRdData *edisess)
    : IatciRequestHandler(pMes, edisess)
{
}

void IatciCkiRequestHandler::parse()
{
    IatciCkiParamsMaker ckiParamsNewMaker;
    ckiParamsNewMaker.setLor(readEdiLor(pMes()));
    ckiParamsNewMaker.setDmc(readEdiDmc(pMes()));
    ckiParamsNewMaker.setChd(readEdiChd(pMes()));

    SetEdiPointToSegGrG(pMes(), SegGrElement(1), "PROG_ERR");
    ckiParamsNewMaker.setFdq(readEdiFdq(pMes()));

    int paxCount = GetNumSegGr(pMes(), 2);
    ASSERT(paxCount > 0);

    for(int curPax = 0; curPax < paxCount; ++curPax)
    {
        EdiPointHolder pxg_holder(pMes());
        SetEdiPointToSegGrG(pMes(), SegGrElement(2, curPax), "PROG_ERR");

        IatciCkiParamsMaker::Pxg pxg;
        pxg.setPpd(readEdiPpd(pMes()));
        pxg.setPrd(readEdiPrd(pMes()));
        pxg.setPsd(readEdiPsd(pMes()));
        pxg.setPbd(readEdiPbd(pMes()));
        pxg.setPsi(readEdiPsi(pMes()));

        int apgCount = GetNumSegGr(pMes(), 3);
        for(int curApg = 0; curApg < apgCount; ++curApg)
        {
            EdiPointHolder apg_holder(pMes());
            SetEdiPointToSegGrG(pMes(), SegGrElement(3, curApg), "PROG_ERR");

            pxg.addApg(readEdiPap(pMes()), readEdiAdd(pMes()));
        }

        ckiParamsNewMaker.addPxg(pxg);

        PopEdiPoint_wdG(pMes());
    }

    m_ckiParams = ckiParamsNewMaker.makeParams();
}

std::string IatciCkiRequestHandler::respType() const
{
    return "I";
}

std::string IatciCkiRequestHandler::fcIndicator() const
{
    return "T";
}

std::list<iatci::dcrcka::Result> IatciCkiRequestHandler::handleRequest() const
{
    LogTrace(TRACE3) << "Enter to " << __FUNCTION__;

    if(postponeHandling()) {
        LogTrace(TRACE3) << "postpone handling for tlg " << inboundTlgNum();

        return iatci::checkin(inboundTlgNum());
    }

    ASSERT(m_ckiParams);
    return { iatci::checkin(m_ckiParams.get()) };
}

const iatci::IBaseParams* IatciCkiRequestHandler::params() const
{
    return m_ckiParams.get_ptr();
}

//---------------------------------------------------------------------------------------

void IatciCkiParamsMaker::Pxg::setPpd(const boost::optional<edifact::PpdElem>& ppd)
{
    ASSERT(ppd);
    m_ppd = *ppd;
}

void IatciCkiParamsMaker::Pxg::setPrd(const boost::optional<edifact::PrdElem>& prd,
                                      bool required)
{
    if(required)
        ASSERT(prd);
    m_prd = prd;
}

void IatciCkiParamsMaker::Pxg::setPsd(const boost::optional<edifact::PsdElem>& psd,
                                      bool required)
{
    if(required)
        ASSERT(psd);
    m_psd = psd;
}

void IatciCkiParamsMaker::Pxg::setPbd(const boost::optional<edifact::PbdElem>& pbd,
                                      bool required)
{
    if(required)
        ASSERT(pbd);
    m_pbd = pbd;
}

void IatciCkiParamsMaker::Pxg::setPsi(const boost::optional<edifact::PsiElem>& psi,
                                      bool required)
{
    if(required)
        ASSERT(psi);
    m_psi = psi;
}

void IatciCkiParamsMaker::Pxg::addApg(const boost::optional<edifact::PapElem>& pap,
                                      const boost::optional<edifact::AddElem>& add)
{
    if(pap && pap->m_type == "IN") {
        m_papInfant = pap;
        m_addInfant = add;
    } else {
        m_pap = pap;
        m_add = add;
    }
}

iatci::dcqcki::PaxGroup IatciCkiParamsMaker::Pxg::makePaxGroup() const
{
    return iatci::dcqcki::PaxGroup(iatci::makePax(m_ppd),
                                   makeReserv(),
                                   makeSeat(),
                                   makeBaggage(),
                                   makeService(),
                                   makeDoc(),
                                   makeAddress(),
                                   makeVisa(),
                                   makeInfant(),
                                   makeInfantDoc(),
                                   makeInfantAddress(),
                                   makeInfantVisa());
}

boost::optional<iatci::ReservationDetails> IatciCkiParamsMaker::Pxg::makeReserv() const
{
    if(m_prd) {
        return iatci::makeReserv(*m_prd);
    }

    return boost::none;
}

boost::optional<iatci::SeatDetails> IatciCkiParamsMaker::Pxg::makeSeat() const
{
    if(m_psd) {
        return iatci::makeSeat(*m_psd);
    }

    return boost::none;
}

boost::optional<iatci::BaggageDetails> IatciCkiParamsMaker::Pxg::makeBaggage() const
{
    if(m_pbd) {
        return iatci::makeBaggage(*m_pbd);
    }

    return boost::none;
}

boost::optional<iatci::ServiceDetails> IatciCkiParamsMaker::Pxg::makeService() const
{
    if(m_psi) {
        return iatci::makeService(*m_psi);
    }

    return boost::none;
}

boost::optional<iatci::DocDetails> IatciCkiParamsMaker::Pxg::makeDoc() const
{
    if(m_pap && m_pap->findDoc()) {
        return iatci::makeDoc(*m_pap);
    }

    return boost::none;
}

boost::optional<iatci::AddressDetails> IatciCkiParamsMaker::Pxg::makeAddress() const
{
    if(m_add) {
        return iatci::makeAddress(*m_add);
    }

    return boost::none;
}

boost::optional<iatci::VisaDetails> IatciCkiParamsMaker::Pxg::makeVisa() const
{
    if(m_pap && m_pap->findVisa()) {
        return iatci::makeVisa(*m_pap);
    }

    return boost::none;
}

boost::optional<iatci::PaxDetails> IatciCkiParamsMaker::Pxg::makeInfant() const
{
    if(m_ppd.m_withInftIndicator == "Y") {
        return iatci::makeInfant(m_ppd);
    }

    return boost::none;
}

boost::optional<iatci::DocDetails> IatciCkiParamsMaker::Pxg::makeInfantDoc() const
{
    if(m_papInfant && m_papInfant->findDoc()) {
        return iatci::makeDoc(*m_papInfant);
    }

    return boost::none;
}

boost::optional<iatci::AddressDetails> IatciCkiParamsMaker::Pxg::makeInfantAddress() const
{
    if(m_addInfant) {
        return iatci::makeAddress(*m_addInfant);
    }

    return boost::none;
}

boost::optional<iatci::VisaDetails> IatciCkiParamsMaker::Pxg::makeInfantVisa() const
{
    if(m_papInfant && m_papInfant->findVisa()) {
        return iatci::makeVisa(*m_papInfant);
    }

    return boost::none;
}

//

void IatciCkiParamsMaker::setLor(const boost::optional<edifact::LorElem>& lor)
{
    ASSERT(lor);
    m_lor = *lor;
}

void IatciCkiParamsMaker::setChd(const boost::optional<edifact::ChdElem>& chd,
                                 bool required)
{
    if(required)
        ASSERT(chd);
    m_chd = chd;
}

void IatciCkiParamsMaker::setDmc(const boost::optional<edifact::DmcElem>& dmc,
                                 bool required)
{
    if(required)
        ASSERT(dmc);
    m_dmc = dmc;
}

void IatciCkiParamsMaker::setFdq(const boost::optional<edifact::FdqElem>& fdq)
{
    ASSERT(fdq);
    m_fdq = *fdq;
}

void IatciCkiParamsMaker::addPxg(const Pxg& pxg)
{
    m_lPxg.push_back(pxg);;
}

iatci::CkiParams IatciCkiParamsMaker::makeParams() const
{
    return iatci::CkiParams(iatci::makeOrg(m_lor),
                            makeCascade(),
                            makeMessageDetails(),
                            makeFlightGroup());
}

iatci::dcqcki::FlightGroup IatciCkiParamsMaker::makeFlightGroup() const
{
    const auto paxGroups = algo::transform(m_lPxg, [](const Pxg& pxg) {
        return pxg.makePaxGroup();
    });
    return iatci::dcqcki::FlightGroup(iatci::makeOutboundFlight(m_fdq),
                                      iatci::makeInboundFlight(m_fdq),
                                      paxGroups);
}

boost::optional<iatci::CascadeHostDetails> IatciCkiParamsMaker::makeCascade() const
{
    if(m_chd) {
        return iatci::makeCascade(*m_chd);
    }

    return boost::none;
}

iatci::MessageDetails IatciCkiParamsMaker::makeMessageDetails() const
{
    if(m_dmc) {
        return iatci::makeMessageDetails(*m_dmc);
    }

    return iatci::MessageDetails::createDefault();
}

}//namespace TlgHandling
