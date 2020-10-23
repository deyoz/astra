#include "IatciCkuRequestHandler.h"
#include "IatciCkuRequest.h"
#include "read_edi_elements.h"
#include "view_edi_elements.h"
#include "postpone_edifact.h"
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
public:

    class Pxg
    {
    private:
        edifact::PpdElem                  m_ppd;
        boost::optional<edifact::PrdElem> m_prd;
        boost::optional<edifact::PsdElem> m_psd;
        boost::optional<edifact::PbdElem> m_pbd;
        boost::optional<edifact::PsiElem> m_psi;

        boost::optional<edifact::UpdElem> m_upd;
        //boost::optional<edifact::UrdElem> m_urd;
        boost::optional<edifact::UsdElem> m_usd;
        boost::optional<edifact::UbdElem> m_ubd;
        boost::optional<edifact::UsiElem> m_usi;
        boost::optional<edifact::UapElem> m_uap;
        boost::optional<edifact::UapElem> m_uapInfant;
        boost::optional<edifact::AddElem> m_add;
        boost::optional<edifact::AddElem> m_addInfant;

    public:
        void setPpd(const boost::optional<edifact::PpdElem>& ppd);
        void setPrd(const boost::optional<edifact::PrdElem>& prd, bool required = false);
        void setPsd(const boost::optional<edifact::PsdElem>& psd, bool required = false);
        void setPbd(const boost::optional<edifact::PbdElem>& pbd, bool required = false);
        void setPsi(const boost::optional<edifact::PsiElem>& psi, bool required = false);

        void setUpd(const boost::optional<edifact::UpdElem>& upd, bool required = false);
        //void setUrd(const boost::optional<edifact::UrdElem>& urd, bool required = false);
        void setUsd(const boost::optional<edifact::UsdElem>& usd, bool required = false);
        void setUbd(const boost::optional<edifact::UbdElem>& ubd, bool required = false);
        void setUsi(const boost::optional<edifact::UsiElem>& usi, bool required = false);
        void addApg(const boost::optional<edifact::UapElem>& uap,
                    const boost::optional<edifact::AddElem>& add);

        iatci::dcqcku::PaxGroup makePaxGroup() const;

    protected:
        boost::optional<iatci::ReservationDetails> makeReserv() const;
        boost::optional<iatci::BaggageDetails>     makeBaggage() const;
        boost::optional<iatci::ServiceDetails>     makeService() const;
        boost::optional<iatci::PaxDetails>         makeInfant() const;

        boost::optional<iatci::UpdatePaxDetails>     makeUpdPax() const;
        boost::optional<iatci::UpdateSeatDetails>    makeUpdSeat() const;
        boost::optional<iatci::UpdateBaggageDetails> makeUpdBaggage() const;
        boost::optional<iatci::UpdateServiceDetails> makeUpdService() const;
        boost::optional<iatci::UpdateDocDetails>     makeUpdDoc() const;
        boost::optional<iatci::UpdateAddressDetails> makeUpdAddress() const;
        boost::optional<iatci::UpdateVisaDetails>    makeUpdVisa() const;
        boost::optional<iatci::UpdatePaxDetails>     makeUpdInfant() const;
        boost::optional<iatci::UpdateDocDetails>     makeUpdInfantDoc() const;
        boost::optional<iatci::UpdateAddressDetails> makeUpdInfantAddress() const;
        boost::optional<iatci::UpdateVisaDetails>    makeUpdInfantVisa() const;
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

    iatci::CkuParams makeParams() const;

protected:
    iatci::dcqcku::FlightGroup                 makeFlightGroup() const;
    boost::optional<iatci::CascadeHostDetails> makeCascade() const;
};

//---------------------------------------------------------------------------------------

IatciCkuRequestHandler::IatciCkuRequestHandler(_EDI_REAL_MES_STRUCT_* pMes,
                                               const edilib::EdiSessRdData *edisess)
    : IatciRequestHandler(pMes, edisess)
{
}

bool IatciCkuRequestHandler::fullAnswer() const
{
    return false;
}

void IatciCkuRequestHandler::parse()
{
    IatciCkuParamsMaker ckuParamsMaker;
    ckuParamsMaker.setLor(readEdiLor(pMes()));
    ckuParamsMaker.setChd(readEdiChd(pMes()));

    SetEdiPointToSegGrG(pMes(), SegGrElement(1), "PROG_ERR");
    ckuParamsMaker.setFdq(readEdiFdq(pMes()));

    int paxCount = GetNumSegGr(pMes(), 2);
    ASSERT(paxCount > 0);

    EdiPointHolder ph(pMes());
    for(int curPax = 0; curPax < paxCount; ++curPax)
    {
        SetEdiPointToSegGrG(pMes(), SegGrElement(2, curPax), "PROG_ERR");

        IatciCkuParamsMaker::Pxg pxg;
        pxg.setPpd(readEdiPpd(pMes()));
        pxg.setPrd(readEdiPrd(pMes()));
        pxg.setPsd(readEdiPsd(pMes()));
        pxg.setPbd(readEdiPbd(pMes()));
        pxg.setPsi(readEdiPsi(pMes()));

        pxg.setUpd(readEdiUpd(pMes()));
        //pxg.setUrd(readEdiUrd(pMes()));
        pxg.setUsd(readEdiUsd(pMes()));
        pxg.setUbd(readEdiUbd(pMes()));
        pxg.setUsi(readEdiUsi(pMes()));

        int apgCount = GetNumSegGr(pMes(), 3);
        for(int curApg = 0; curApg < apgCount; ++curApg)
        {
            EdiPointHolder apg_holder(pMes());
            SetEdiPointToSegGrG(pMes(), SegGrElement(3, curApg), "PROG_ERR");

            pxg.addApg(readEdiUap(pMes()), readEdiAdd(pMes()));
        }

        ckuParamsMaker.addPxg(pxg);

        PopEdiPoint_wdG(pMes());
    }

    m_ckuParams = ckuParamsMaker.makeParams();
}

std::string IatciCkuRequestHandler::respType() const
{
    return "U";
}

std::string IatciCkuRequestHandler::fcIndicator() const
{
    return "";
}

const iatci::IBaseParams* IatciCkuRequestHandler::params() const
{
    return m_ckuParams.get_ptr();
}

std::list<iatci::dcrcka::Result> IatciCkuRequestHandler::handleRequest() const
{
    LogTrace(TRACE3) << "Enter to " << __FUNCTION__;

    if(postponeHandling()) {
        LogTrace(TRACE3) << "postpone handling for tlg " << inboundTlgNum();

        return iatci::updateCheckin(inboundTlgNum());
    }

    ASSERT(m_ckuParams);
    return { iatci::updateCheckin(m_ckuParams.get()) };
}

//---------------------------------------------------------------------------------------

void IatciCkuParamsMaker::Pxg::setPpd(const boost::optional<edifact::PpdElem>& ppd)
{
    ASSERT(ppd);
    m_ppd = *ppd;
}

void IatciCkuParamsMaker::Pxg::setPrd(const boost::optional<edifact::PrdElem>& prd,
                                      bool required)
{
    if(required)
        ASSERT(prd);
    m_prd = prd;
}

void IatciCkuParamsMaker::Pxg::setPsd(const boost::optional<edifact::PsdElem>& psd,
                                      bool required)
{
    if(required)
        ASSERT(psd);
    m_psd = psd;
}

void IatciCkuParamsMaker::Pxg::setPbd(const boost::optional<edifact::PbdElem>& pbd,
                                      bool required)
{
    if(required)
        ASSERT(pbd);
    m_pbd = pbd;
}

void IatciCkuParamsMaker::Pxg::setPsi(const boost::optional<edifact::PsiElem>& psi,
                                      bool required)
{
    if(required)
        ASSERT(psi);
    m_psi = psi;
}

void IatciCkuParamsMaker::Pxg::setUpd(const boost::optional<edifact::UpdElem>& upd,
                                      bool required)
{
    if(required)
        ASSERT(upd);
    m_upd = upd;
}

void IatciCkuParamsMaker::Pxg::setUsd(const boost::optional<edifact::UsdElem>& usd,
                                      bool required)
{
    if(required)
        ASSERT(usd);
    m_usd = usd;
}

void IatciCkuParamsMaker::Pxg::setUbd(const boost::optional<edifact::UbdElem>& ubd,
                                      bool required)
{
    if(required)
        ASSERT(ubd);
    m_ubd = ubd;
}

void IatciCkuParamsMaker::Pxg::setUsi(const boost::optional<edifact::UsiElem>& usi,
                                      bool required)
{
    if(required)
        ASSERT(usi);
    m_usi = usi;
}

void IatciCkuParamsMaker::Pxg::addApg(const boost::optional<edifact::UapElem>& uap,
                                      const boost::optional<edifact::AddElem>& add)
{
    if(uap && uap->m_type == "IN") {
        m_uapInfant = uap;
        m_addInfant = add;
    } else {
        m_uap = uap;
        m_add = add;
    }
}

iatci::dcqcku::PaxGroup IatciCkuParamsMaker::Pxg::makePaxGroup() const
{
    return iatci::dcqcku::PaxGroup(iatci::makePax(m_ppd),
                                   makeReserv(),
                                   makeBaggage(),
                                   makeService(),
                                   makeInfant(),
                                   makeUpdPax(),
                                   makeUpdSeat(),
                                   makeUpdBaggage(),
                                   makeUpdService(),
                                   makeUpdDoc(),
                                   makeUpdAddress(),
                                   makeUpdVisa(),
                                   makeUpdInfant(),
                                   makeUpdInfantDoc(),
                                   makeUpdInfantAddress(),
                                   makeUpdInfantVisa());
}

boost::optional<iatci::ReservationDetails> IatciCkuParamsMaker::Pxg::makeReserv() const
{
    if(m_prd) {
        return iatci::makeReserv(*m_prd);
    }

    return boost::none;
}

boost::optional<iatci::BaggageDetails> IatciCkuParamsMaker::Pxg::makeBaggage() const
{
    if(m_pbd) {
        return iatci::makeBaggage(*m_pbd);
    }

    return boost::none;
}

boost::optional<iatci::ServiceDetails> IatciCkuParamsMaker::Pxg::makeService() const
{
    if(m_psi) {
        return iatci::makeService(*m_psi);
    }

    return boost::none;
}

boost::optional<iatci::PaxDetails> IatciCkuParamsMaker::Pxg::makeInfant() const
{
    if(m_ppd.m_withInftIndicator == "Y" && !m_ppd.m_inftName.empty()) {
        return iatci::makeInfant(m_ppd);
    }

    return boost::none;
}

boost::optional<iatci::UpdatePaxDetails> IatciCkuParamsMaker::Pxg::makeUpdPax() const
{
    if(m_upd) {
        return iatci::makeUpdPax(*m_upd);
    }

    return boost::none;
}

boost::optional<iatci::UpdateSeatDetails> IatciCkuParamsMaker::Pxg::makeUpdSeat() const
{
    if(m_usd) {
        return iatci::makeUpdSeat(*m_usd);
    }

    return boost::none;
}

boost::optional<iatci::UpdateBaggageDetails> IatciCkuParamsMaker::Pxg::makeUpdBaggage() const
{
    if(m_ubd) {
        return iatci::makeUpdBaggage(*m_ubd);
    }

    return boost::none;
}

boost::optional<iatci::UpdateServiceDetails> IatciCkuParamsMaker::Pxg::makeUpdService() const
{
    if(m_usi) {
        return iatci::makeUpdService(*m_usi);
    }

    return boost::none;
}

boost::optional<iatci::UpdateDocDetails> IatciCkuParamsMaker::Pxg::makeUpdDoc() const
{
    if(m_uap && !m_uap->isGroupHeader() && m_uap->findDoc()) {
        return iatci::makeUpdDoc(*m_uap);
    }

    return boost::none;
}

boost::optional<iatci::UpdateAddressDetails> IatciCkuParamsMaker::Pxg::makeUpdAddress() const
{
    if(m_add) {
        return iatci::makeUpdAddress(*m_add);
    }

    return boost::none;
}

boost::optional<iatci::UpdateVisaDetails> IatciCkuParamsMaker::Pxg::makeUpdVisa() const
{
    if(m_uap && !m_uap->isGroupHeader() && m_uap->findVisa()) {
        return iatci::makeUpdVisa(*m_uap);
    }

    return boost::none;
}

boost::optional<iatci::UpdatePaxDetails> IatciCkuParamsMaker::Pxg::makeUpdInfant() const
{
    if(m_upd) {
        return iatci::makeUpdPax(*m_upd);
    }

    return boost::none;
}

boost::optional<iatci::UpdateDocDetails> IatciCkuParamsMaker::Pxg::makeUpdInfantDoc() const
{
    if(m_uapInfant && !m_uapInfant->isGroupHeader() && m_uapInfant->findDoc()) {
        return iatci::makeUpdDoc(*m_uapInfant);
    }

    return boost::none;
}

boost::optional<iatci::UpdateAddressDetails> IatciCkuParamsMaker::Pxg::makeUpdInfantAddress() const
{
    if(m_addInfant) {
        return iatci::makeUpdAddress(*m_addInfant);
    }

    return boost::none;
}

boost::optional<iatci::UpdateVisaDetails> IatciCkuParamsMaker::Pxg::makeUpdInfantVisa() const
{
    if(m_uapInfant && !m_uapInfant->isGroupHeader() && m_uapInfant->findVisa()) {
        return iatci::makeUpdVisa(*m_uapInfant);
    }

    return boost::none;
}

//

void IatciCkuParamsMaker::setLor(const boost::optional<edifact::LorElem>& lor)
{
    ASSERT(lor);
    m_lor = *lor;
}

void IatciCkuParamsMaker::setChd(const boost::optional<edifact::ChdElem>& chd, bool required)
{
    if(required)
        ASSERT(chd);
    m_chd = chd;
}

void IatciCkuParamsMaker::setFdq(const boost::optional<edifact::FdqElem>& fdq)
{
    ASSERT(fdq);
    m_fdq = *fdq;
}

void IatciCkuParamsMaker::addPxg(const Pxg& pxg)
{
    m_lPxg.push_back(pxg);;
}

iatci::CkuParams IatciCkuParamsMaker::makeParams() const
{
    return iatci::CkuParams(iatci::makeOrg(m_lor),
                            makeCascade(),
                            makeFlightGroup());
}

iatci::dcqcku::FlightGroup IatciCkuParamsMaker::makeFlightGroup() const
{
    const auto paxGroups = algo::transform(m_lPxg, [](const Pxg& pxg) {
        return pxg.makePaxGroup();
    });
    return iatci::dcqcku::FlightGroup(iatci::makeOutboundFlight(m_fdq),
                                      iatci::makeInboundFlight(m_fdq),
                                      paxGroups);
}

boost::optional<iatci::CascadeHostDetails> IatciCkuParamsMaker::makeCascade() const
{
    if(m_chd) {
        return iatci::makeCascade(*m_chd);
    }

    return boost::none;
}

}//namespace TlgHandling
