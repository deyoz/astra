#include "AirportControl.h"
#include "basetables.h"
#include "tlg/CheckinBaseTypesOci.h"

#include <serverlib/cursctl.h>
#include <serverlib/dates_oci.h>

#include <boost/scoped_ptr.hpp>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace Ticketing {

AirportControlNotFound::AirportControlNotFound(const Ticketing::Airline_t& airl,
                                               const Ticketing::TicketNum_t& tick,
                                               const Ticketing::CouponNum_t& cpn)
    : EXCEPTIONS::Exception("")
{
    std::string err = "AirportControl record not found: ";
    err += BaseTables::Company(airl)->code() + "/" + tick.get() + "/" +
            std::to_string(cpn.get());

    setMessage(err);
}

AirportControlNotFound::AirportControlNotFound(const std::string& rl)
    : EXCEPTIONS::Exception("")
{
    std::string err = "AirportControl record not found by recloc: " + rl;
    setMessage(err);
}

AirportControlCantBeReturned::AirportControlCantBeReturned(const Ticketing::Airline_t& airl,
                                                           const Ticketing::TicketNum_t& tick,
                                                           const Ticketing::CouponNum_t& cpn)
    : EXCEPTIONS::Exception("")
{
    std::string err = "AirportControl can't be returned: ";
    err += BaseTables::Company(airl)->code() + "/" + tick.get() + "/" +
            std::to_string(cpn.get());

    setMessage(err);
}

/////////////////////////////////////////////////////////////////////////////////////////

class AirportControlMaker
{
public:
    AirportControlMaker() {}
    void setAirline(const Ticketing::Airline_t& airl) {
        m_airpControl.m_airline = airl;
    }
    void setTicknum(const Ticketing::TicketNum_t& tnum) {
        m_airpControl.m_ticknum = tnum;
    }
    void setCoupon(const Ticketing::CouponNum_t& cpnnum) {
        m_airpControl.m_cpnnum = cpnnum;
    }
    void setDateCr(const Dates::DateTime_t& date) {
        m_airpControl.m_dateCr = date;
    }
    void setRecloc(const std::string& rl) {
        m_airpControl.m_recloc = rl;
    }

    AirportControl* getNewAirportControl() const;
    AirportControl  getAirportControl() const;

protected:
    void check() const;

private:
    AirportControl m_airpControl;
};

//

AirportControl* AirportControlMaker::getNewAirportControl() const
{
    check();
    return new AirportControl(m_airpControl);
}

AirportControl  AirportControlMaker::getAirportControl() const
{
   check();
   return m_airpControl;
}

void AirportControlMaker::check() const
{
    if(m_airpControl.ticknum().empty() ||
       !m_airpControl.cpnnum() ||
       !m_airpControl.airline())
    {
        LogTrace(TRACE1) << m_airpControl;
        throw EXCEPTIONS::Exception("AirportControlMaker: "
                "Incomplete AirportControl, can't create instance");
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

const Ticketing::Airline_t& AirportControl::airline() const
{
    return m_airline;
}

const Ticketing::TicketNum_t& AirportControl::ticknum() const
{
    return m_ticknum;
}

const Ticketing::CouponNum_t& AirportControl::cpnnum()  const
{
    return m_cpnnum;
}

const Dates::DateTime_t& AirportControl::dateCr()  const
{
    return m_dateCr;
}

const std::string& AirportControl::recloc()  const
{
    return m_recloc;
}

AirportControl* AirportControl::readDb(const Ticketing::Airline_t& airline,
                                       const Ticketing::TicketNum_t& tickNum,
                                       const Ticketing::CouponNum_t& cpnNum,
                                       bool throwNf)
{
    LogTrace(TRACE3) << __FUNCTION__ << " by "
                     << airline << "/" << tickNum << "/" << cpnNum;

    OciCpp::CursCtl cur = make_curs(
"select AIRLINE, TICKNUM, CPNNUM, DATE_CR, RECLOC "
"from AIRPORT_CONTROLS "
"where AIRLINE=:airl and TICKNUM=:tnum and CPNNUM=:cnum");

    cur.bind(":airl", airline)
       .bind(":tnum", tickNum.get())
       .bind(":cnum", cpnNum.get());

    Ticketing::Airline_t rAirl;
    std::string rTick, rRecloc;
    unsigned rCpn;
    Dates::DateTime_t rDateCr;

    cur
            .def(rAirl)
            .def(rTick)
            .def(rCpn)
            .def(rDateCr)
            .def(rRecloc)
            .EXfet();

    if(cur.err() == NO_DATA_FOUND)
    {
        if(throwNf) {
            throw AirportControlNotFound(airline, tickNum, cpnNum);
        } else {
            return nullptr;
        }
    }

    AirportControlMaker mk;
    mk.setAirline(rAirl);
    mk.setTicknum(Ticketing::TicketNum_t(rTick));
    mk.setCoupon(Ticketing::CouponNum_t(rCpn));
    mk.setDateCr(rDateCr);
    mk.setRecloc(rRecloc);

    return mk.getNewAirportControl();
}

AirportControl* AirportControl::readDb(const std::string& airline,
                                       const Ticketing::TicketNum_t& tickNum,
                                       const Ticketing::CouponNum_t& cpnNum,
                                       bool throwNf)
{
    return readDb(BaseTables::Company(airline)->ida(),
                  tickNum,
                  cpnNum,
                  throwNf);
}

void AirportControl::writeDb(const AirportControl& ac)
{
    LogTrace(TRACE3) << __FUNCTION__ << " " << ac;

    OciCpp::CursCtl cur = make_curs(
"insert into AIRPORT_CONTROLS(AIRLINE, TICKNUM, CPNNUM, DATE_CR, RECLOC) "
"values (:airl, :tnum, :cnum, :date_cr, :rl)");

    cur.
            noThrowError(CERR_U_CONSTRAINT).
            bind(":airl",    ac.airline()).
            bind(":tnum",    ac.ticknum().get()).
            bind(":cnum",    ac.cpnnum().get()).
            bind(":date_cr", Dates::second_clock::local_time()).
            bind(":rl",      ac.recloc()).
            exec();

    if(cur.err() == CERR_U_CONSTRAINT)
    {
        throw AirportControlExists();
    }
}

void AirportControl::deleteDb(const AirportControl& ac)
{
    LogTrace(TRACE3) << __FUNCTION__ << " by "
                     << ac.airline() << "/" << ac.ticknum() << "/" << ac.cpnnum();

    OciCpp::CursCtl cur = make_curs(
"delete from AIRPORT_CONTROLS "
"where AIRLINE=:airl and TICKNUM=:tnum and CPNNUM=:cnum");

    cur.bind(":airl", ac.airline())
       .bind(":tnum", ac.ticknum().get())
       .bind(":cnum", ac.cpnnum().get())
       .exec();

}

void AirportControl::writeDb() const
{
    writeDb(*this);
}

void AirportControl::deleteDb() const
{
    deleteDb(*this);
}

AirportControl* AirportControl::create(const std::string& recloc,
                                       const Ticketing::Airline_t& airline,
                                       const Ticketing::TicketNum_t& ticknum,
                                       const Ticketing::CouponNum_t& cpnnum)
{
    AirportControlMaker mk;
    mk.setRecloc(recloc);
    mk.setAirline(airline);
    mk.setTicknum(ticknum);
    mk.setCoupon(cpnnum);
    return mk.getNewAirportControl();
}

//---------------------------------------------------------------------------------------

std::ostream& operator<<(std::ostream& s, const AirportControl& ac)
{
    s << "AirportControl:" <<
            " Recloc: " << ac.recloc() <<
            " Airl: "   << ac.airline() <<
            " Ticket: " << ac.ticknum() <<
            " Coupon: " << ac.cpnnum() <<
            " DateCr: " << ac.dateCr();
    return s;
}

bool existsAirportControl(const Ticketing::Airline_t& airline,
                          const Ticketing::TicketNum_t& ticknum,
                          const Ticketing::CouponNum_t& cpnnum,
                          bool throwErr)
{
    boost::scoped_ptr<AirportControl> ac(AirportControl::readDb(airline,
                                                                ticknum,
                                                                cpnnum));
    if(!ac) {
        LogWarning(STDLOG) << "Airport control not found: "
                           << airline << "/" << ticknum << "/" << cpnnum;
        if(throwErr) {
            throw AirportControlNotFound(airline, ticknum, cpnnum);
        }
        return false;
    }
    return true;
}

bool existsAirportControl(const std::string& airline,
                          const std::string& tick_no,
                          const int& coupon_no,
                          bool throwErr)
{
  return existsAirportControl(BaseTables::Company(airline)->ida(),
                              Ticketing::TicketNum_t(tick_no),
                              Ticketing::CouponNum_t(coupon_no),
                              throwErr);
}

}//namespace Ticketing
