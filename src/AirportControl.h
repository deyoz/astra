#pragma once

#include "tlg/CheckinBaseTypes.h"
#include "ticket_types.h"
#include "astra_dates.h"
#include "exceptions.h"


namespace Ticketing {

class AirportControlNotFound : public EXCEPTIONS::Exception
{
public:
    AirportControlNotFound(const Ticketing::TicketNum_t& tick,
                           const Ticketing::CouponNum_t& cpn);
    AirportControlNotFound(const std::string& rl);
};

//---------------------------------------------------------------------------------------

class AirportControlExists : public EXCEPTIONS::Exception
{
public:
    AirportControlExists()
        : EXCEPTIONS::Exception("Duplicate AirportControl record")
    {}
};

//---------------------------------------------------------------------------------------

class AirportControlCantBeReturned : public EXCEPTIONS::Exception
{
public:
    AirportControlCantBeReturned(const Ticketing::TicketNum_t& tick,
                                 const Ticketing::CouponNum_t& cpn);
};

//---------------------------------------------------------------------------------------

class AirportControl
{
public:
    const Ticketing::TicketNum_t& ticknum() const;
    const Ticketing::CouponNum_t& cpnnum()  const;
    const Dates::DateTime_t&      dateCr()  const;
    const std::string&            recloc()  const;

    static AirportControl* readDb(const Ticketing::TicketNum_t& tickNum,
                                  const Ticketing::CouponNum_t& cpnNum,
                                  bool throwNf = false);

    static void deleteDb(const AirportControl& ac);

    static void writeDb(const AirportControl& ac);

    void writeDb() const;
    void deleteDb() const;

    static AirportControl* create(const std::string& recloc,
                                  const Ticketing::TicketNum_t& ticknum,
                                  const Ticketing::CouponNum_t& cpnnum);

protected:
    friend class AirportControlMaker;

    AirportControl() {}

private:
    Ticketing::TicketNum_t m_ticknum;
    Ticketing::CouponNum_t m_cpnnum;
    Dates::DateTime_t      m_dateCr;
    std::string            m_recloc;
};

std::ostream& operator<<(std::ostream& s, const AirportControl& ac);

bool existsAirportControl(const Ticketing::TicketNum_t& ticknum,
                          const Ticketing::CouponNum_t& cpnnum,
                          bool throwErr);

bool existsAirportControl(const std::string& tick_no,
                          const int& coupon_no,
                          bool throwErr);

}//namespace Ticketing
