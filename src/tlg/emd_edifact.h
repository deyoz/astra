#pragma once

#include <edilib/edi_types.h>

#include "astra_ticket.h"
#include "astra_emd.h"
#include "tlg/edi_elements.h"


namespace Ticketing {

struct CpnEdifact
{
    boost::optional<edifact::CpnElem> cpn_;
    boost::optional<edifact::PtsElem> pts_;
    boost::optional<edifact::FtiElem> fti_;
    boost::optional<edifact::TvlElem> tvl_;
    boost::optional<edifact::EbdElem> ebd_;
    edifact::IftElements              ift_;
    boost::optional<Ticketing::TicketNum_t>  assocTickNum_;
};

//-----------------------------------------------------------------------------

struct TktEdifact
{
    boost::optional<edifact::TktElem> tkt_;
    std::list<CpnEdifact>             cpn_;
};

//-----------------------------------------------------------------------------

struct EmdEdifact
{
    friend class EmdEdifactReader;

    boost::optional<OrigOfRequest>           org_;
    boost::optional<ResContrInfo>            rci_;
    boost::optional<TicketingAgentInfo_t>    tai_;
    boost::optional<TourCode_t>              ati_;
    boost::optional<Passenger>               tif_;
    //boost::optional<edifact::PtkElem>        ptk_;
    boost::optional<edifact::PtsElem>        pts_;

    edifact::MonElements              mon_;
    edifact::FopElements              fop_;
    edifact::TxdElements              txd_;
    edifact::IftElements              ift_;

    /// Билеты
    std::list<TktEdifact>                    lTkt_;
    /// Привязки
    std::list<TktEdifact>                    lTktConnect_;

    static EmdEdifact read(_EDI_REAL_MES_STRUCT_ *pMes);

    Ticketing::DocType getDocType() const;

    Emd makeEmd() const;

private:
    EmdEdifact() {}

    std::list<EmdTicket> makeTkt() const;
    std::list<MonetaryInfo> makeMon() const;
    std::list<EmdCoupon> makeCpn(const std::list<CpnEdifact>& lcpn, const TicketNum_t& tickNum,
                                 DocType docType, TickStatAction::TickStatAction_t tickAct) const;

    void readLevel0(_EDI_REAL_MES_STRUCT_ *pMes, int currtif);
    void readLevel3(_EDI_REAL_MES_STRUCT_ *pMes);
    void readLevel4(_EDI_REAL_MES_STRUCT_ *pMes, TktEdifact &ticket);

    void applyConnections();
    void applyAssociations4EmdA();
    void applyConnections4EmdS();
};

//-----------------------------------------------------------------------------

class EmdEdifactReader
{
public:
    static std::list<Emd> readList(_EDI_REAL_MES_STRUCT_ *pMes);
    static std::list<Emd> readList(const std::string& tlgSource);
};

}//namespace Ticketing
