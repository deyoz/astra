#include "AirportControl.h"
#include "basetables.h"
#include "tlg/CheckinBaseTypesOci.h"

#include "pg_session.h"
#include <serverlib/pg_cursctl.h>
#include <serverlib/pg_rip.h>

#include <serverlib/cursctl.h>
#include <serverlib/dates_oci.h>

#include <boost/scoped_ptr.hpp>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace Ticketing {

AirportControlNotFound::AirportControlNotFound(const Ticketing::TicketNum_t& tick,
                                               const Ticketing::CouponNum_t& cpn)
    : EXCEPTIONS::Exception("")
{
    std::string err = "AirportControl record not found: ";
    err += tick.get() + "/" + std::to_string(cpn.get());

    setMessage(err);
}

AirportControlNotFound::AirportControlNotFound(const std::string& rl)
    : EXCEPTIONS::Exception("")
{
    std::string err = "AirportControl record not found by recloc: " + rl;
    setMessage(err);
}

AirportControlCantBeReturned::AirportControlCantBeReturned(const Ticketing::TicketNum_t& tick,
                                                           const Ticketing::CouponNum_t& cpn)
    : EXCEPTIONS::Exception("")
{
    std::string err = "AirportControl can't be returned: ";
    err += tick.get() + "/" + std::to_string(cpn.get());

    setMessage(err);
}

/////////////////////////////////////////////////////////////////////////////////////////

class AirportControlMaker
{
public:
    AirportControlMaker() {}
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
       !m_airpControl.cpnnum())
    {
        LogTrace(TRACE1) << m_airpControl;
        throw EXCEPTIONS::Exception("AirportControlMaker: "
                "Incomplete AirportControl, can't create instance");
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

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

AirportControl* AirportControl::readDb(const Ticketing::TicketNum_t& tickNum,
                                       const Ticketing::CouponNum_t& cpnNum,
                                       bool throwNf)
{
    LogTrace(TRACE3) << __FUNCTION__ << " by "
                     << tickNum << "/" << cpnNum;

    PgCpp::CursCtl cur = get_pg_curs(
"select TICKNUM, CPNNUM, DATE_CR, RECLOC "
"from AIRPORT_CONTROLS "
"where TICKNUM=:tnum and CPNNUM=:cnum");

    cur.bind(":tnum", tickNum.get())
       .bind(":cnum", cpnNum.get());

    std::string rTick, rRecloc;
    unsigned rCpn;
    Dates::DateTime_t rDateCr;

    cur
            .def(rTick)
            .def(rCpn)
            .def(rDateCr)
            .def(rRecloc)
            .EXfet();

    if(cur.err() == PgCpp::NoDataFound)
    {
        if(throwNf) {
            throw AirportControlNotFound(tickNum, cpnNum);
        } else {
            return nullptr;
        }
    }

    AirportControlMaker mk;
    mk.setTicknum(Ticketing::TicketNum_t(rTick));
    mk.setCoupon(Ticketing::CouponNum_t(rCpn));
    mk.setDateCr(rDateCr);
    mk.setRecloc(rRecloc);

    return mk.getNewAirportControl();
}

void AirportControl::writeDb(const AirportControl& ac)
{
    LogTrace(TRACE3) << __FUNCTION__ << " " << ac;

    PgCpp::CursCtl cur = get_pg_curs(
"insert into AIRPORT_CONTROLS(TICKNUM, CPNNUM, DATE_CR, RECLOC) "
"values (:tnum, :cnum, :date_cr, :rl)");

    cur.
            noThrowError(PgCpp::ConstraintFail).      //CERR_U_CONSTRAINT
            bind(":tnum",    ac.ticknum().get()).
            bind(":cnum",    ac.cpnnum().get()).
            bind(":date_cr", Dates::second_clock::local_time()).
            bind(":rl",      ac.recloc()).
            exec();

    if(cur.err() == PgCpp::ConstraintFail)
    {
        throw AirportControlExists();
    }
}

void AirportControl::deleteDb(const AirportControl& ac)
{
    LogTrace(TRACE3) << __FUNCTION__ << " by "
                     << ac.ticknum() << "/" << ac.cpnnum();

    PgCpp::CursCtl cur = get_pg_curs(
"delete from AIRPORT_CONTROLS "
"where TICKNUM=:tnum and CPNNUM=:cnum");

    cur.bind(":tnum", ac.ticknum().get())
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
                                       const Ticketing::TicketNum_t& ticknum,
                                       const Ticketing::CouponNum_t& cpnnum)
{
    AirportControlMaker mk;
    mk.setRecloc(recloc);
    mk.setTicknum(ticknum);
    mk.setCoupon(cpnnum);
    return mk.getNewAirportControl();
}

//---------------------------------------------------------------------------------------

std::ostream& operator<<(std::ostream& s, const AirportControl& ac)
{
    s << "AirportControl:" <<
            " Recloc: " << ac.recloc() <<
            " Ticket: " << ac.ticknum() <<
            " Coupon: " << ac.cpnnum() <<
            " DateCr: " << ac.dateCr();
    return s;
}

bool existsAirportControl(const Ticketing::TicketNum_t& ticknum,
                          const Ticketing::CouponNum_t& cpnnum,
                          bool throwErr)
{
    boost::scoped_ptr<AirportControl> ac(AirportControl::readDb(ticknum,
                                                                cpnnum));
    if(!ac) {
        LogWarning(STDLOG) << "Airport control not found: "
                           << ticknum << "/" << cpnnum;
        if(throwErr) {
            throw AirportControlNotFound(ticknum, cpnnum);
        }
        return false;
    }
    return true;
}

bool existsAirportControl(const std::string& tick_no,
                          const int& coupon_no,
                          bool throwErr)
{
  return existsAirportControl(Ticketing::TicketNum_t(tick_no),
                              Ticketing::CouponNum_t(coupon_no),
                              throwErr);
}

}//namespace Ticketing
