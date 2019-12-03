#pragma once

#include "astra_ticket.h"
#include "exceptions.h"
#include "tlg/edi_tlg.h"

#include <etick/tick_data.h>

#include <boost/optional.hpp>

namespace Ticketing {


/*
 * рабочая копия билета
*/
struct WcTicket
{
    std::string            m_recloc;
    Ticketing::TicketNum_t m_tickNum;

    WcTicket(const std::string& recloc,
             const Ticketing::TicketNum_t& ticknum)
        : m_recloc(recloc),
          m_tickNum(ticknum)
    {}

    WcTicket()
    {}

    const std::string&             recloc() const { return m_recloc;  }
    const Ticketing::TicketNum_t& tickNum() const { return m_tickNum; }
};

//---------------------------------------------------------------------------------------

/*
 * рабочая копия купона
*/
struct WcCoupon
{
    std::string             m_recloc;
    Ticketing::TicketNum_t  m_tickNum;
    Ticketing::CouponNum_t  m_cpnNum;
    Ticketing::CouponStatus m_status;

    WcCoupon(const std::string& recloc,
             const Ticketing::TicketNum_t& tickNum,
             const Ticketing::CouponNum_t& cpnNum,
             const Ticketing::CouponStatus& status)
        : m_recloc(recloc),
          m_tickNum(tickNum),
          m_cpnNum(cpnNum),
          m_status(status)
    {}

    WcCoupon()
    {}

    const std::string&             recloc() const { return m_recloc;  }
    const Ticketing::TicketNum_t& tickNum() const { return m_tickNum; }
    const Ticketing::CouponNum_t&  cpnNum() const { return m_cpnNum;  }
    const Ticketing::CouponStatus& status() const { return m_status;  }
};

/*
 * рабочая копия образа pnr
*/
struct WcPnr
{
    std::string    m_recloc;
    Ticketing::Pnr m_pnr;

    WcPnr(const std::string& recloc,
          const Ticketing::Pnr& pnr)
        : m_recloc(recloc),
          m_pnr(pnr)
    {}

    const std::string& recloc() const { return m_recloc; }
    const Ticketing::Pnr& pnr() const { return m_pnr;    }
    Ticketing::Pnr&       pnr()       { return m_pnr;    }
};

//---------------------------------------------------------------------------------------

class WcCouponNotFound : public EXCEPTIONS::Exception
{
public:
    WcCouponNotFound(const Ticketing::TicketNum_t& tick,
                     const Ticketing::CouponNum_t& cpn);

    WcCouponNotFound(const std::string& recloc,
                     const Ticketing::TicketNum_t& tick,
                     const Ticketing::CouponNum_t& cpn);
};

//---------------------------------------------------------------------------------------

void saveWcPnr(const EdiPnr& ediPnr);

void loadWcEdiPnr(const std::string& recloc, boost::optional<EdiPnr>& ediPnr);
boost::optional<WcPnr> loadWcPnr(const std::string& recloc);
boost::optional<WcPnr> loadWcPnr(const Ticketing::TicketNum_t& tickNum);

boost::optional<WcPnr> loadWcPnrWithActualStatuses(const std::string& recloc);
boost::optional<WcPnr> loadWcPnrWithActualStatuses(const Ticketing::TicketNum_t& tickNum);

boost::optional<WcTicket> readWcTicket(const Ticketing::TicketNum_t& tickNum);

boost::optional<WcCoupon> readWcCoupon(const Ticketing::TicketNum_t& tickNum,
                                       const Ticketing::CouponNum_t& cpnNum);

WcCoupon readWcCouponByRl(const std::string& recloc,
                          const Ticketing::TicketNum_t& tickNum,
                          const Ticketing::CouponNum_t& cpnNum);

// смена статуса рабочей копии купона
// если контроль не у нас - throw AirportControlNotFound или return false
// если рабочей копии нет - throw WcCouponNotFound или return false
bool changeOfStatusWcCoupon(const Ticketing::TicketNum_t& ticknum,
                            const Ticketing::CouponNum_t& cpnnum,
                            const Ticketing::CouponStatus& newStatus,
                            bool throwErr = false);

// для совместимости
bool changeOfStatusWcCoupon(const std::string& ticknum,
                            unsigned cpnnum,
                            const Ticketing::CouponStatus& newStatus,
                            bool throwErr = false);

// попытка вернуть контроль над купоном
bool returnWcCoupon(const Ticketing::TicketNum_t& ticknum,
                    const Ticketing::CouponNum_t& cpnnum,
                    bool throwErr = false);

//---------------------------------------------------------------------------------------

class AstraPnrCallbacks
{
public:
    virtual ~AstraPnrCallbacks() {}

    virtual void afterReceiveAirportControl(const Ticketing::WcCoupon& cpn) = 0;
    virtual void afterReturnAirportControl(const Ticketing::WcCoupon& cpn) = 0;
};

}//namespace Ticketing
