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

EmdCOSParams::EmdCOSItem::EmdCOSItem(const Ticketing::TicketCpn_t& tickCpn,
                                     const Ticketing::CouponStatus& status,
                                     const boost::optional<Ticketing::Itin>& itin)
    : m_tickCpn(tickCpn), m_status(status), m_itin(itin)
{
}

const Ticketing::TicketNum_t& EmdCOSParams::EmdCOSItem::tickNum() const
{
    return m_tickCpn.ticket();
}

const Ticketing::CouponNum_t& EmdCOSParams::EmdCOSItem::cpnNum() const
{
    return m_tickCpn.cpn();
}

const Ticketing::CouponStatus& EmdCOSParams::EmdCOSItem::status() const
{
    return m_status;
}

const boost::optional<Ticketing::Itin>& EmdCOSParams::EmdCOSItem::itinOpt() const
{
    return m_itin;
}

//-----------------------------------------------------------------------------

EmdCOSParams::EmdCOSParams(const Ticketing::OrigOfRequest& org,
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
    m_cosItems.push_back(EmdCOSParams::EmdCOSItem(Ticketing::TicketCpn_t(tickNum, cpnNum),
                                                  status));
}

EmdCOSParams::EmdCOSParams(const Ticketing::OrigOfRequest& org,
                           const std::string& ctxt,
                           const edifact::KickInfo& kickInfo,
                           const std::string& airline,
                           const Ticketing::FlightNum_t& flNum)
    : EmdRequestParams(org, ctxt, kickInfo, airline, flNum)
{
}

void EmdCOSParams::addCoupon(const Ticketing::TicketCpn_t& tickCpn,
                             const Ticketing::CouponStatus& status)
{
    m_cosItems.push_back(EmdCOSParams::EmdCOSItem(tickCpn, status));
}

const boost::optional<Ticketing::Itin>& EmdCOSParams::globalItinOpt() const
{
    return m_globalItin;
}

size_t EmdCOSParams::numberOfItems() const
{
    return m_cosItems.size();
}

//-----------------------------------------------------------------------------

EmdCOSRequest::EmdCOSRequest(const EmdCOSParams& cosParams)
    : EmdRequest(cosParams), m_cosParams(cosParams)
{
}

std::string EmdCOSRequest::mesFuncCode() const
{
    return "793";
}

static EqnElem getEqn(const EmdCOSParams& cosParams)
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

static TvlElem getTvl(const Ticketing::Itin& itin)
{
    TvlElem tvl;
    tvl.m_airline = itin.airCode();
    tvl.m_operAirline = itin.airCodeOper();
    tvl.m_flNum = Ticketing::FlightNum_t(itin.flightnum());
    tvl.m_operFlNum = Ticketing::FlightNum_t(itin.flightnumOper());
    tvl.m_depDate = itin.date1();
    tvl.m_depTime = itin.time1();
    tvl.m_arrDate = itin.date2();
    tvl.m_arrTime = itin.time2();
    tvl.m_depPoint = itin.depPointCode();
    tvl.m_arrPoint = itin.arrPointCode();
    return tvl;
}
    
void EmdCOSRequest::collectMessage()
{
    // ORG
    viewOrgElement(pMes(), m_cosParams.org());
    // EQN
    viewEqnElement(pMes(), getEqn(m_cosParams));
    
    if(m_cosParams.globalItinOpt()) {
        // global TVL
        viewTvlElement(pMes(), getTvl(*m_cosParams.globalItinOpt()));
    }
    
    edilib::PushEdiPointW(pMes());
    unsigned tnum = 0;
    BOOST_FOREACH(const EmdCOSParams::EmdCOSItem& item, m_cosParams.m_cosItems) {
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
            viewTvlElement(pMes(), getTvl(*item.itinOpt()));
        }
        edilib::PopEdiPointW(pMes());
        
        edilib::PopEdiPoint_wdW(pMes());
    }
    edilib::PopEdiPointW(pMes());
}

}//namespace edifact
