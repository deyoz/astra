#pragma once

#include "astra_ticket.h"
#include "exceptions.h"
#include "tlg/edi_tlg.h"

#include <etick/tick_data.h>

#include <boost/optional.hpp>

namespace Ticketing {


/*
 * ࠡ��� ����� �����
*/
struct WcTicket
{
    std::string            m_recloc;
    Ticketing::Airline_t   m_airline;
    Ticketing::TicketNum_t m_tickNum;

    WcTicket(const std::string& recloc,
             const Ticketing::Airline_t& airline,
             const Ticketing::TicketNum_t& ticknum)
        : m_recloc(recloc),
          m_airline(airline),
          m_tickNum(ticknum)
    {}

    WcTicket()
    {}

    const std::string&             recloc() const { return m_recloc;  }
    const Ticketing::Airline_t&   airline() const { return m_airline; }
    const Ticketing::TicketNum_t& tickNum() const { return m_tickNum; }
};

//---------------------------------------------------------------------------------------

/*
 * ࠡ��� ����� �㯮��
*/
struct WcCoupon
{
    std::string             m_recloc;
    Ticketing::Airline_t    m_airline;
    Ticketing::TicketNum_t  m_tickNum;
    Ticketing::CouponNum_t  m_cpnNum;
    Ticketing::CouponStatus m_status;

    WcCoupon(const std::string& recloc,
             const Ticketing::Airline_t& airline,
             const Ticketing::TicketNum_t& tickNum,
             const Ticketing::CouponNum_t& cpnNum,
             const Ticketing::CouponStatus& status)
        : m_recloc(recloc),
          m_airline(airline),
          m_tickNum(tickNum),
          m_cpnNum(cpnNum),
          m_status(status)
    {}

    WcCoupon()
    {}

    const std::string&             recloc() const { return m_recloc;  }
    const Ticketing::Airline_t&   airline() const { return m_airline; }
    const Ticketing::TicketNum_t& tickNum() const { return m_tickNum; }
    const Ticketing::CouponNum_t&  cpnNum() const { return m_cpnNum;  }
    const Ticketing::CouponStatus& status() const { return m_status;  }
};

/*
 * ࠡ��� ����� ��ࠧ� pnr
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
    WcCouponNotFound(const Ticketing::Airline_t& airl,
                     const Ticketing::TicketNum_t& tick,
                     const Ticketing::CouponNum_t& cpn);

    WcCouponNotFound(const std::string& recloc,
                     const Ticketing::TicketNum_t& tick,
                     const Ticketing::CouponNum_t& cpn);
};

//---------------------------------------------------------------------------------------

struct EdiPnr
{
    std::string             m_ediText;
    edifact::EdiMessageType m_ediType;

    EdiPnr(const std::string& ediText, edifact::EdiMessageType ediType)
        : m_ediText(ediText),
          m_ediType(ediType)
    {}
};

std::ostream& operator<<(std::ostream& os, const EdiPnr& ediPnr);

//---------------------------------------------------------------------------------------

void saveWcPnr(const Ticketing::Airline_t& airline, const EdiPnr& ediPnr);

boost::optional<WcPnr> loadWcPnr(const std::string& recloc);
boost::optional<WcPnr> loadWcPnr(const Ticketing::Airline_t& airline,
                                 const Ticketing::TicketNum_t& tickNum);

boost::optional<WcPnr> loadWcPnrWithActualStatuses(const std::string& recloc);
boost::optional<WcPnr> loadWcPnrWithActualStatuses(const Ticketing::Airline_t& airline,
                                                   const Ticketing::TicketNum_t& tickNum);

boost::optional<WcTicket> readWcTicket(const Ticketing::Airline_t& airline,
                                       const Ticketing::TicketNum_t& tickNum);

boost::optional<WcCoupon> readWcCoupon(const Ticketing::Airline_t& airline,
                                       const Ticketing::TicketNum_t& tickNum,
                                       const Ticketing::CouponNum_t& cpnNum);

WcCoupon readWcCouponByRl(const std::string& recloc,
                          const Ticketing::TicketNum_t& tickNum,
                          const Ticketing::CouponNum_t& cpnNum);

// ᬥ�� ����� ࠡ�祩 ����� �㯮��
// �᫨ ����஫� �� � ��� - throw AirportControlNotFound ��� return false
// �᫨ ࠡ�祩 ����� ��� - throw WcCouponNotFound ��� return false
bool changeOfStatusWcCoupon(const Ticketing::Airline_t& airline,
                            const Ticketing::TicketNum_t& ticknum,
                            const Ticketing::CouponNum_t& cpnnum,
                            const Ticketing::CouponStatus& newStatus,
                            bool throwErr = false);

// ��� ᮢ���⨬���
bool changeOfStatusWcCoupon(const std::string& airline,
                            const std::string& ticknum,
                            unsigned cpnnum,
                            const Ticketing::CouponStatus& newStatus,
                            bool throwErr = false);

}//namespace Ticketing
