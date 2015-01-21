#include "astra_emd.h"

#include <etick/exceptions.h>

#include <boost/foreach.hpp>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace Ticketing {

RfiscDescription::RfiscDescription(const std::string& rfisc, const std::string& desc)
    : rfisc_(rfisc), description_(desc)
{
    if(rfisc_.empty() || description_.empty())
        throw TickExceptions::tick_soft_except(STDLOG, "EtsErr::INV_RFISC");
}

std::ostream& operator<<(std::ostream& os, const RfiscDescription& rfisc)
{
    os << rfisc.rfisc() << " " << rfisc.description();
    return os;
}

//---------------------------------------------------------------------------------------

EmdCoupon::EmdCoupon(DocType docType, const CouponNum_t& num, const CouponStatus& status,
                     const TicketNum_t& tnum, unsigned version)
    : Coupon(Coupon_info(num.get(), status, TicketMedia::Electronic), tnum.get()), docType_(docType),
      tickNum_(tnum), associated_(false), quantity_(1),
      consumed_(false), version_(version)
{
}

EmdCoupon EmdCoupon::makeEmdSCoupon(const CouponNum_t& num, const CouponStatus& status,
                                    const TicketNum_t& tnum, unsigned version)
{
    return EmdCoupon(DocType::EmdS, num, status, tnum, version);
}

EmdCoupon EmdCoupon::makeEmdACoupon(const CouponNum_t& num, const CouponStatus& status,
                                    const TicketNum_t& tnum,
                                    const CouponNum_t& numAssoc, const TicketNum_t& tnumAssoc,
                                    bool associated, unsigned version)
{
    EmdCoupon cpn(DocType::EmdA, num, status, tnum, version);
    cpn.associatedNum_ = numAssoc;
    cpn.associatedTickNum_ = tnumAssoc;
    cpn.associated_ = associated;
    return cpn;
}

std::ostream& operator<<(std::ostream& os, const EmdCoupon& cpn)
{
    os << "Coupon num: " << cpn.num();
    if(cpn.associatedNum())
        os << "[" << cpn.associatedNum() << "]";
    if(!cpn.associatedTickNum().empty())
        os << " in conn tick num: " << cpn.associatedTickNum() << "\n";
    os << " Is associated: " << cpn.associated() << "\n";
    os << " Status: " << cpn.couponInfo().status();
    if(cpn.amount())
        os << " Value: " << cpn.amount()->amStr();
    //os << " DoS: " << cpn.dateOfService() << "\n";

    if(cpn.rfisc())
        os << "\nRFISC: " << cpn.rfisc()->rfisc() << " - " << cpn.rfisc()->description();
    os << "\nQuantity: " << cpn.quantity()
       << " consumed: " << cpn.consumed();

    if(cpn.haveItin())
        os << "\nItin: " << cpn.itin() << " Oper Airline: " << cpn.itin().airCodeOper();

    if(!cpn.lfreqPass().empty())
        os << "\nCPN FREQ: " << cpn.lfreqPass().front();
    os << "\n\n";

    return os;
}

//---------------------------------------------------------------------------------------

EmdTicket::EmdTicket(DocType docType, const TicketNum_t &tickNum, TickStatAction::TickStatAction_t tac)
    : docType_(docType), tickNum_(tickNum), tickActCode_(tac), num_(0)
{
}

EmdTicket EmdTicket::makeEmdSTicket(const TicketNum_t& tickNum, const TicketNum_t& tickNumConnect,
                                    TickStatAction::TickStatAction_t tac)
{
    EmdTicket tick(DocType::EmdS, tickNum, tac);
    ASSERT(!tickNumConnect.empty());
    tick.tickNumConnect_ = tickNumConnect;
    return tick;
}

EmdTicket EmdTicket::makeEmdATicket(const TicketNum_t& tickNum, TickStatAction::TickStatAction_t tac)
{
    return EmdTicket(DocType::EmdA, tickNum, tac);
}

void EmdTicket::setLCpn(const std::list<EmdCoupon>& l)
{
    lCpn_ = l;
}

void EmdTicket::addCpn(const EmdCoupon& cpn)
{
    lCpn_.push_back(cpn);
}

std::ostream& operator<<(std::ostream& os, const EmdTicket& tkt)
{
    os << tkt.tickNum();
    if(tkt.tickNumConnect())
        os << " in connection: " << *tkt.tickNumConnect();
    os << "\n";

    BOOST_FOREACH(const EmdCoupon &cpn, tkt.lCpn())
        os << "CPN: " << cpn;

    return os;
}

//---------------------------------------------------------------------------------------

Emd::Emd(const DocType& docType,
         const RficType& rfic,
         const OrigOfRequest& org,
         const ResContrInfo& rci,
         const Passenger& pass,
         const std::list<TaxDetails>& lTax,
         const std::list<MonetaryInfo>& lMon,
         const std::list<FormOfPayment>& lFop,
         const std::list<FreeTextInfo> & lIft,
         const std::list<FormOfId>& lFoid,
         const std::list<EmdTicket>& lTicket)
    : docType_(docType),
      rficType_(rfic),
      org_(org),
      rci_(rci),
      pass_(pass),
      lTax_(lTax),
      lMon_(lMon),
      lFop_(lFop),
      lIft_(lIft),
      lFoid_(lFoid),
      lTicket_(lTicket)
{
}

std::ostream& operator<<(std::ostream& os, const Emd &emd)
{
    os << emd.type() << " " << emd.lTicket().front().tickNum() << "\n";
    os << "Issued by: " << emd.org() << "\n";
    os << "Reclocs information: " << emd.rci();
    os << "Pass: " << emd.pass().name() << "/" << emd.pass().surname() << "\n";
    os << "Rfic: " << emd.rfic();
    if(emd.tourCode())
        os << "\nTour code: " << emd.tourCode()->code();
    if(emd.agentInfo())
        os << "\nAgent info: " << emd.agentInfo()->companyId()
           << "/" << emd.agentInfo()->agentId() << "/" << emd.agentInfo()->agentType();
    os << "\nTXD: ";
    BOOST_FOREACH(const TaxDetails& txd, emd.lTax())
        os << txd << " ";
    os << "\nMON: ";
    BOOST_FOREACH(const MonetaryInfo& mon, emd.lMon())
        os << mon << " ";
    os << "\nFOP: ";
    BOOST_FOREACH(const FormOfPayment& fop, emd.lFop())
        os << (fop.fopCode().size() ? fop.fopCode() : "???") << fop.amValue().amStr();
    os << "\nIFT: ";
    BOOST_FOREACH(const FreeTextInfo& ift, emd.lIft())
        os << ift.qualifier() << ":" << ift.type() << ":" << ift.fullText();
    os << "\nFOID: ";
    BOOST_FOREACH(const FormOfId& foid, emd.lFoid())
        os << foid << "\n";
    os << "\nTKT: ";
    BOOST_FOREACH(const EmdTicket& tkt, emd.lTicket())
        os << tkt << "\n";

    return os;
}

}//namespace Ticketing
