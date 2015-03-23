#include "emd_edifact.h"
#include "read_edi_elements.h"
#include "astra_tick_read_edi.h"

#include <edilib/edi_func_cpp.h>

#include <boost/foreach.hpp>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace Ticketing {

namespace {

struct FindTktByNum: public std::binary_function< TktEdifact, TicketNum_t, bool >
{
    bool operator()(const TktEdifact& l, const TicketNum_t& r) const
    {
        return (l.tkt_ && l.tkt_->m_ticketNum == r);
    }
};

struct FindCpnByNum: public std::binary_function< CpnEdifact, CouponNum_t, bool >
{
    bool operator()(const CpnEdifact& l, const CouponNum_t& r) const
    {
        return (l.cpn_ && l.cpn_->m_num == r);
    }
};

TktEdifact& findTktByNum_throwNotFound(std::list<TktEdifact>& ltkt, TicketNum_t num)
{
    std::list<TktEdifact>::iterator it = find_if(ltkt.begin(), ltkt.end(),
                                                 bind2nd(FindTktByNum(), num));
    if(it == ltkt.end()) {
        throw EXCEPTIONS::ExceptionFmt(STDLOG) << "Ticket " << num << " not found at EMD!";
    }

    return *it;
}

CpnEdifact& findCpnByNum_throwNotFound(std::list<CpnEdifact>& lcpn, CouponNum_t num)
{
    std::list<CpnEdifact>::iterator it = find_if(lcpn.begin(), lcpn.end(),
                                                 bind2nd(FindCpnByNum(), num));
    if(it == lcpn.end()) {
        throw EXCEPTIONS::ExceptionFmt(STDLOG) << "Coupon " << num << " not found at EMD!";
    }

    return *it;
}

}//namespace

///////////////////////////////////////////////////////////////////////////////

void EmdEdifact::readLevel0(_EDI_REAL_MES_STRUCT_ *pMes, int currtif)
{
    edilib::EdiPointHolder grp(pMes);
    edilib::SetEdiPointToSegGrG(pMes, 3, currtif);

    org_ = TickReader::readOrigOfRequest(pMes);
    tai_ = TickReader::readEdiTicketAgnInfoCurrOr0(pMes);
    ati_ = TickReader::readEdiTourCodeCurrOr0(pMes);
    rci_ = TickReader::readResContrInfo(pMes);
    tif_ = TickReader::readPassenger(pMes);
    mon_ = TickReader::readEdiMonCurrAnd0(pMes);
    fop_ = TickReader::readEdiFopCurrAnd0(pMes);
    txd_ = TickReader::readEdiTxdCurrAnd0(pMes);
    ift_ = TickReader::readEdiIftCurrAnd0(pMes, 1);
//    ptk_ = TickReader::readEdiPtkCurrOr0(pMes);
    pts_ = TickReader::readEdiPts(pMes);

    readLevel3(pMes);

    applyConnections();
}

void EmdEdifact::readLevel3(_EDI_REAL_MES_STRUCT_ *pMes)
{
    const unsigned segGr = 4;
    unsigned numTkt = edilib::GetNumSegGr(pMes, segGr, "EtsErr::INV_TICKNUM");
    unsigned numTktNew = 0;

    edilib::EdiPointHolder grph(pMes);
    for(unsigned currTkt = 0; currTkt < numTkt; currTkt++)
    {
        edilib::SetEdiPointToSegGrG(pMes, segGr, currTkt);

        TktEdifact tkt;
        tkt.tkt_ = TickReader::readEdiTkt(pMes);
        ASSERT(tkt.tkt_);
        readLevel4(pMes, tkt);
        if(tkt.tkt_->m_tickStatAction == TickStatAction::inConnectionWith) {
            ASSERT(tkt.tkt_->m_inConnectionTicketNum);
            lTktConnect_.push_back(tkt);
        } else if(tkt.tkt_->m_tickStatAction != TickStatAction::oldtick) {
            lTkt_.push_back(tkt);
        }

        if(tkt.tkt_->m_tickStatAction == TickStatAction::newtick)
            numTktNew++;

        grph.popNoDel();
    }

    if(numTktNew > 4)
    {
        throw TickExceptions::tick_soft_except(STDLOG, "EtsErr::TOO_MANY_TICKETS",
                                               "Invalid number of conjunction tickets (%d),"
                                               " 4 maximum", numTktNew);
    }
}

void EmdEdifact::readLevel4(_EDI_REAL_MES_STRUCT_ *pMes, TktEdifact &ticket)
{
    ASSERT(ticket.tkt_);
    const unsigned segGr = 5;

    unsigned numCpn = edilib::GetNumSegGr(pMes, segGr);
    if(!numCpn && ticket.tkt_->m_tickStatAction != TickStatAction::inConnectionWith) {
        throw TickExceptions::tick_soft_except(STDLOG, "EtsErr::INV_COUPON", "Missing coupon information!");
    }
    edilib::EdiPointHolder grph(pMes);
    for(unsigned currcpn = 0; currcpn < numCpn; currcpn ++)
    {
        edilib::SetEdiPointToSegGrG(pMes, segGr, currcpn);

        CpnEdifact cpn;

        cpn.cpn_ = TickReader::readEdiCpn(pMes);
        ASSERT(cpn.cpn_);
        if(!cpn.cpn_->m_num)
            cpn.cpn_->m_num = CouponNum_t(numCpn + 1);

        cpn.tvl_ = TickReader::readEdiTvl(pMes);
        cpn.pts_ = TickReader::readEdiPts(pMes);
        cpn.ebd_ = TickReader::readEdiEbd(pMes);
        cpn.ift_ = TickReader::readEdiIft(pMes, segGr - 1
                                          /*,ticket.tkt_->m_ticketNum, cpn.cpn_->m_num*/);

        ticket.cpn_.push_back(cpn);
        grph.popNoDel();
    }
}

std::list<EmdTicket> EmdEdifact::makeTkt() const
{
    DocType doctype;
    std::list<EmdTicket> lticket;

    BOOST_FOREACH(const TktEdifact& tkt, lTkt_)
    {
        if(!doctype)
            doctype = tkt.tkt_->m_docType;

        if(doctype && doctype != tkt.tkt_->m_docType)
            throw TickExceptions::tick_soft_except(STDLOG, "EtsErr::INV_DOC_TYPE");

        TickStatAction::TickStatAction_t tickAct = tkt.tkt_->m_tickStatAction ? *tkt.tkt_->m_tickStatAction
                                                                              : TickStatAction::newtick;
        if(doctype == DocType::EmdS) {
            // Тикет EMD-S с привязкой к ЭБ через TickNumConnect
            lticket.push_back(EmdTicket::makeEmdSTicket(tkt.tkt_->m_ticketNum,
                                                        tkt.tkt_->m_inConnectionTicketNum,
                                                        tickAct));
        } else {
            // Для тикетов EMD-A в TickNumConnect нет необходимости, т.к. ассоциации с ЭБ на уровне купонов
            lticket.push_back(EmdTicket::makeEmdATicket(tkt.tkt_->m_ticketNum,
                                                        tickAct));
        }

        lticket.back().setLCpn(makeCpn(tkt.cpn_, tkt.tkt_->m_ticketNum, doctype, tickAct));
    }

    return lticket;
}

static Itin makeItin(const edifact::TvlElem& tvl,
                     const boost::optional<edifact::PtsElem>& pts,
                     const boost::optional<edifact::EbdElem>& ebd)
{
    Itin itin(tvl.m_airline,
              tvl.m_operAirline,
              tvl.m_flNum,
              tvl.m_operFlNum,
              tvl.m_depDate,
              tvl.m_depTime,
              tvl.m_arrDate,
              tvl.m_arrTime,
              tvl.m_depPoint,
              tvl.m_arrPoint);

    if(pts)
        itin.setFareBasis(pts->m_fareBasis);
    if(ebd)
        itin.setLuggage(Luggage(ebd->m_quantity, ebd->m_charge, ebd->m_measure));

    return itin;
}

static std::string findRfiscDescription(const std::list<FreeTextInfo>& lift)
{
    BOOST_FOREACH(const FreeTextInfo &ift, lift) {
        if(ift.fTType() == FreeTextType::RfiscDescription) {
            return ift.fullText();
        }
    }
    return "";
}

std::list<EmdCoupon> EmdEdifact::makeCpn(const std::list<CpnEdifact>& lcpn,
                                         const TicketNum_t& tickNum,
                                         DocType docType,
                                         TickStatAction::TickStatAction_t tickAct) const
{
    std::list<EmdCoupon> result;
    BOOST_FOREACH(const CpnEdifact& c, lcpn) {
        //checkCoupon(c, docType, tickAct);

        if(docType == DocType::EmdS) {
            result.push_back(EmdCoupon::makeEmdSCoupon(c.cpn_->m_num, c.cpn_->m_status, tickNum));
        } else {

            result.push_back(EmdCoupon::makeEmdACoupon(c.cpn_->m_num, c.cpn_->m_status, tickNum,
                                                       c.cpn_->m_connectedNum, *c.assocTickNum_,
                                                       c.cpn_->m_action == "702" /*CpnStatAction::associate*/));
        }

        EmdCoupon& cpn = result.back();
        cpn.setAmount(c.cpn_->m_amount);
        cpn.setConsumed(c.cpn_->m_action == "6" /*CpnStatAction::consumedAtIssuance*/);

        if(tickAct == TickStatAction::newtick)
        {
            if(docType == DocType::EmdA) {
                cpn.resetItin(Itin::SharedPtr(new Itin(makeItin(*c.tvl_, c.pts_, c.ebd_))));
                //cpn.setDateOfService(c.tvl_->m_depDate); // будем считать датой сервиса в купоне дату вылета
            }

            if(!c.pts_ || c.pts_->m_rfisc.empty())
                throw TickExceptions::tick_soft_except(STDLOG, "EtsErr::INV_RFISC");
            cpn.setRfisc(RfiscDescription(c.pts_->m_rfisc, findRfiscDescription(c.ift_.m_lIft)));
        }
    }

    return result;
}


std::list<MonetaryInfo> EmdEdifact::makeMon() const
{
    std::list<MonetaryInfo> result;
    TickReader::readEdiMonetaryInfo(mon_.m_lMon, result);
    return result;
}

Emd EmdEdifact::makeEmd() const
{
    std::list<FormOfId> lFoid;
        // TODO fill FOID list
    //for(size_t i = 0; i < *rci_.Foid.size(); i++)
    //    lFoid.push_back(rci.Foid[i]);

    Emd emd(getDocType(),
            pts_->m_rfic,
            *org_,
            *rci_,
            *tif_,
            txd_.m_lTax,
            makeMon(),
            fop_.m_lFop,
            ift_.m_lIft,
            lFoid,
            makeTkt());

    if(tai_)
        emd.setAgentInfo(tai_.get());
    if(ati_)
        emd.setTourCode(ati_.get());

    return emd;
}

Ticketing::DocType EmdEdifact::getDocType() const
{
    Ticketing::DocType doctype;
    BOOST_FOREACH(const TktEdifact& tkt, lTkt_) {
        if(!doctype)
            doctype = tkt.tkt_->m_docType;

        if(doctype && doctype != tkt.tkt_->m_docType)
            throw TickExceptions::tick_soft_except(STDLOG, "EtsErr::INV_DOC_TYPE");
    }

    return doctype;
}

void EmdEdifact::applyConnections()
{
    if(getDocType() == DocType::EmdA) {
        applyAssociations4EmdA();
    } else {
        applyConnections4EmdS();
    }
}

void EmdEdifact::applyConnections4EmdS()
{
    std::map<TicketNum_t, TicketNum_t> inconnTickets;
    BOOST_FOREACH(const TktEdifact tkt, lTktConnect_) {
        inconnTickets.insert(std::make_pair(tkt.tkt_->m_ticketNum, *tkt.tkt_->m_inConnectionTicketNum));
    }

    BOOST_FOREACH(TktEdifact& tkt, lTkt_) {
        size_t numTicketsConnected = inconnTickets.count(tkt.tkt_->m_ticketNum);
        LogTrace(TRACE3) << "EmdS NUM[" << tkt.tkt_->m_ticketNum << "] has " << numTicketsConnected << " ET connected (max 1)";
        if(numTicketsConnected == 1)
            tkt.tkt_->m_inConnectionTicketNum = inconnTickets.at(tkt.tkt_->m_ticketNum);
    }
}

void EmdEdifact::applyAssociations4EmdA()
{
    BOOST_FOREACH(const TktEdifact& tktAssoc, lTktConnect_) {
        TktEdifact& tkt = findTktByNum_throwNotFound(lTkt_, tktAssoc.tkt_->m_ticketNum);
        BOOST_FOREACH(const CpnEdifact& cpnAssoc, tktAssoc.cpn_) {
            CpnEdifact& cpn = findCpnByNum_throwNotFound(tkt.cpn_, cpnAssoc.cpn_->m_num);
            cpn.cpn_->m_connectedNum  = cpnAssoc.cpn_->m_connectedNum;
            cpn.cpn_->m_action        = cpnAssoc.cpn_->m_action;
            cpn.assocTickNum_         = tktAssoc.tkt_->m_inConnectionTicketNum;
            LogTrace(TRACE3) << "Coupon association [" << tkt.tkt_->m_ticketNum << "/" << cpn.cpn_->m_num << "]"
                             << " <--> " << "[" << *cpn.assocTickNum_ << "/" << cpn.cpn_->m_connectedNum << "]";
            if(cpnAssoc.pts_) {
                if(!cpn.pts_)
                    cpn.pts_ = edifact::PtsElem();
                cpn.pts_->m_fareBasis = cpnAssoc.pts_->m_fareBasis;
            }
        }
    }
}

//---------------------------------------------------------------------------------------

std::list<Emd> EmdEdifactReader::readList(_EDI_REAL_MES_STRUCT_ *pMes)
{
    std::list<Emd> lEmd;
    unsigned numTif = edilib::GetNumSegGr(pMes, 3, "Need surname");
    for(unsigned currTif = 0; currTif < numTif; currTif++)
    {
        EmdEdifact ediEmd;
        ediEmd.readLevel0(pMes, currTif);
        lEmd.push_back(ediEmd.makeEmd());
    }

    return lEmd;
}

std::list<Emd> EmdEdifactReader::readList(const std::string& tlgSource)
{
    return readList(edilib::ReadEdifactMessage(tlgSource.c_str()));
}

}//namespace Ticketing
