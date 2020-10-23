#include "emd_cos_request.h"
#include "view_edi_elements.h"

#include <etick/tick_data.h>
#include <edilib/edi_func_cpp.h>

#include <boost/foreach.hpp>

#define NICKNAME "ANTON"
#define NICK_TRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace edifact
{

EmdCosParams::EmdCOSItem::EmdCOSItem(const Ticketing::TicketCpn_t& tickCpn,
                                     const Ticketing::CouponStatus& status,
                                     const boost::optional<Ticketing::Itin>& itin)
    : m_tickCpn(tickCpn), m_status(status), m_itin(itin)
{
}

const Ticketing::TicketNum_t& EmdCosParams::EmdCOSItem::tickNum() const
{
    return m_tickCpn.ticket();
}

const Ticketing::CouponNum_t& EmdCosParams::EmdCOSItem::cpnNum() const
{
    return m_tickCpn.cpn();
}

const Ticketing::CouponStatus& EmdCosParams::EmdCOSItem::status() const
{
    return m_status;
}

const boost::optional<Ticketing::Itin>& EmdCosParams::EmdCOSItem::itinOpt() const
{
    return m_itin;
}

//-----------------------------------------------------------------------------

EmdCosParams::EmdCosParams(const Ticketing::OrigOfRequest& org,
                           const std::string& ctxt,
                           const edifact::KickInfo& kickInfo,
                           const std::string& airline,
                           const Ticketing::FlightNum_t& flNum,
                           const Ticketing::TicketNum_t& tickNum,
                           const Ticketing::CouponNum_t& cpnNum,
                           const Ticketing::CouponStatus& status,
                           const boost::optional<Ticketing::Itin>& itin)
    : EmdRequestParams(org, ctxt, kickInfo, airline, flNum), m_globalItin(itin)
{
    m_cosItems.push_back(EmdCosParams::EmdCOSItem(Ticketing::TicketCpn_t(tickNum, cpnNum),
                                                  status));
}

EmdCosParams::EmdCosParams(const Ticketing::OrigOfRequest& org,
                           const std::string& ctxt,
                           const edifact::KickInfo& kickInfo,
                           const std::string& airline,
                           const Ticketing::FlightNum_t& flNum)
    : EmdRequestParams(org, ctxt, kickInfo, airline, flNum)
{
}

void EmdCosParams::addCoupon(const Ticketing::TicketCpn_t& tickCpn,
                             const Ticketing::CouponStatus& status)
{
    m_cosItems.push_back(EmdCosParams::EmdCOSItem(tickCpn, status));
}

const boost::optional<Ticketing::Itin>& EmdCosParams::globalItinOpt() const
{
    return m_globalItin;
}

size_t EmdCosParams::numberOfItems() const
{
    return m_cosItems.size();
}

//-----------------------------------------------------------------------------

EmdCosRequest::EmdCosRequest(const EmdCosParams& cosParams)
    : EmdRequest(cosParams), m_cosParams(cosParams)
{
}

std::string EmdCosRequest::mesFuncCode() const
{
    return "793";
}

static EqnElem getEqn(const EmdCosParams& cosParams)
{
    return EqnElem(cosParams.numberOfItems(), "TD");
}

static TktElem getTkt(const Ticketing::TicketNum_t& tickNum)
{
    TktElem tkt;
    tkt.m_docType = Ticketing::DocType(Ticketing::DocType::EmdA);
    tkt.m_ticketNum = tickNum;
    tkt.m_tickStatAction = Ticketing::TickStatAction::newtick;
    return tkt;
}

static CpnElem getCpn(const Ticketing::CouponNum_t& cpnNum,
                      const Ticketing::CouponStatus& status)
{
    CpnElem cpn;
    cpn.m_num = cpnNum;
    cpn.m_status = status;
    return cpn;
}
    
void EmdCosRequest::collectMessage()
{
    BaseTables::Router rot(sysCont()->routerCanonName());
    // ORG
    viewOrgElement(pMes(), m_cosParams.org(), rot->translit());
    // EQN
    viewEqnElement(pMes(), getEqn(m_cosParams));
    
    if(m_cosParams.globalItinOpt()) {
        // global TVL
        viewItin2(pMes(), *m_cosParams.globalItinOpt(), rot->translit());
    }
    
    edilib::PushEdiPointW(pMes());
    unsigned tnum = 0;
    BOOST_FOREACH(const EmdCosParams::EmdCOSItem& item, m_cosParams.m_cosItems) {
        edilib::SetEdiSegGr(pMes(), edilib::SegGrElement(1, tnum));
        edilib::SetEdiPointToSegGrW(pMes(), edilib::SegGrElement(1, tnum++));
        // TKT
        viewTktElement(pMes(), getTkt(item.tickNum()));
        edilib::PushEdiPointW(pMes());
        edilib::SetEdiSegGr(pMes(), edilib::SegGrElement(2));
        edilib::SetEdiPointToSegGrW(pMes(), edilib::SegGrElement(2));
        // CPN
        viewCpnElement(pMes(), getCpn(item.cpnNum(), item.status()));
        if(item.itinOpt()) {
            // coupon TVL
            viewItin2(pMes(), *item.itinOpt(), rot->translit());
        }
        edilib::PopEdiPointW(pMes());
        
        edilib::PopEdiPoint_wdW(pMes());
    }
    edilib::PopEdiPointW(pMes());
}

}//namespace edifact
