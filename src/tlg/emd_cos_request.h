#pragma once

#include "emd_request.h"
#include <etick/tick_data.h>

#include <list>
#include <boost/optional.hpp>


namespace edifact
{

class EmdCOSParams: public EmdRequestParams
{
    friend class EmdCOSRequest;
    
    class EmdCOSItem
    {
    public:
        EmdCOSItem(const Ticketing::TicketCpn_t& tickCpn,
                   const Ticketing::CouponStatus& status,
                   const boost::optional<Ticketing::Itin>& itin = boost::optional<Ticketing::Itin>());
        
        const Ticketing::TicketNum_t& tickNum() const;
        const Ticketing::CouponNum_t& cpnNum() const;
        const Ticketing::CouponStatus& status() const;
        const boost::optional<Ticketing::Itin>& itinOpt() const;
        
    private:
        Ticketing::TicketCpn_t m_tickCpn;
        Ticketing::CouponStatus m_status;
        boost::optional<Ticketing::Itin> m_itin;
    };
    
public:
    /**
     * @brief Конструктор объекта параметров для смены статуса ОДНОГО купона EMD
    */
    EmdCOSParams(const Ticketing::OrigOfRequest& org,
                 const std::string& ctxt,
                 const edifact::KickInfo& kickInfo,
                 const std::string& airline,
                 const Ticketing::FlightNum_t& flNum,
                 const Ticketing::TicketNum_t& tickNum,
                 const Ticketing::CouponNum_t& cpnNum,
                 const Ticketing::CouponStatus& status,
                 const boost::optional<Ticketing::Itin>& itin = boost::optional<Ticketing::Itin>());
    
    /**
     * @brief Конструктор объекта параметров для смена статуса купонов EMD,
     *        добавляемых методом addCoupon
    */
    EmdCOSParams(const Ticketing::OrigOfRequest& org,
                 const std::string& ctxt,
                 const edifact::KickInfo& kickInfo,
                 const std::string& airline,
                 const Ticketing::FlightNum_t& flNum);
    
    void addCoupon(const Ticketing::TicketCpn_t& tickCpn,
                   const Ticketing::CouponStatus& status);
    
    const boost::optional<Ticketing::Itin>& globalItinOpt() const;
    
    size_t numberOfItems() const;
    
private:
    std::list<EmdCOSItem> m_cosItems;
    boost::optional<Ticketing::Itin> m_globalItin;
};

//-----------------------------------------------------------------------------

class EmdCOSRequest: public EmdRequest
{
    EmdCOSParams m_cosParams;
    
public:
    EmdCOSRequest(const EmdCOSParams& cosParams);

    virtual std::string mesFuncCode() const;
    virtual void collectMessage();

    virtual ~EmdCOSRequest() {}
};

}//namespace edifact
