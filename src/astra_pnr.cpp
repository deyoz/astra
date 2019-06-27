#include "astra_pnr.h"
#include "recloc.h"
#include "etick.h"
#include "astra_utils.h"
#include "astra_tick_view_xml.h"
#include "AirportControl.h"
#include "basetables.h"
#include "tlg/CheckinBaseTypesOci.h"

#include <serverlib/cursctl.h>
#include <etick/tickmng.h>

#include <boost/scoped_ptr.hpp>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace Ticketing {

using namespace TickView;
using namespace TickMng;

static AirportControl* createAirportControlIfNeeded(const std::string& recloc,
                                                    const Ticketing::Airline_t& airline,
                                                    const Ticketing::Coupon& cpn)
{
    AirportControl* ac = nullptr;
    if(cpn.couponInfo().status() == Ticketing::CouponStatus::OriginalIssue ||
       cpn.couponInfo().status() == Ticketing::CouponStatus::Airport)
    {
        return AirportControl::create(recloc,
                                      airline,
                                      Ticketing::TicketNum_t(cpn.ticknum()),
                                      Ticketing::CouponNum_t(cpn.couponInfo().num()));
    }

    return ac;
}

static void saveCouponWc(const std::string& recloc,
                         const Ticketing::Airline_t& airline,
                         const Ticketing::Coupon& cpn)
{
    LogTrace(TRACE3) << __FUNCTION__ << " "
                     << recloc << "/"
                     << airline << "/"
                     << cpn.ticknum() << "/"
                     << cpn.couponInfo().num();

    make_curs(
"begin "
"delete from WC_COUPON where OWNER_AIRLINE=:owner_airline and TICKNUM=:ticknum and NUM=:num; "
"insert into WC_COUPON(RECLOC, OWNER_AIRLINE, TICKNUM, NUM, STATUS) "
"values (:recloc, :owner_airline, :ticknum, :num, :status);"
"end;")
            .stb()
            .bind(":recloc",        recloc)
            .bind(":owner_airline", airline)
            .bind(":ticknum",       cpn.ticknum())
            .bind(":num",           cpn.couponInfo().num())
            .bind(":status",        cpn.couponInfo().status()->toBaseType())
            .exec();

    boost::scoped_ptr<AirportControl> ac(createAirportControlIfNeeded(recloc,
                                                                      airline,
                                                                      cpn));
    if(ac) {
        try {
            ac->writeDb();
        } catch(const AirportControlExists&) {
            LogWarning(STDLOG) << "Airport control under coupon "
                               << airline << "/" << cpn.ticknum() << "/" << cpn.couponInfo().num()
                               << " already exists!";
            ac->deleteDb();
            ac->writeDb();
        }

        WcCoupon wcCpn(recloc,
                       airline,
                       Ticketing::TicketNum_t(cpn.ticknum()),
                       Ticketing::CouponNum_t(cpn.couponInfo().num()),
                       cpn.couponInfo().status());

        try {
          callbacks<AstraPnrCallbacks>()->afterReceiveAirportControl(wcCpn);
        } catch(...) {
          CallbacksExceptionFilter(STDLOG);
        }
    }
}

static void updateCouponWc(const Ticketing::Airline_t& airline,
                           const Ticketing::TicketNum_t& ticknum,
                           const Ticketing::CouponNum_t& cpnnum,
                           const Ticketing::CouponStatus& status)
{
    LogTrace(TRACE3) << __FUNCTION__ << " "
                     << airline << "/"
                     << ticknum << "/"
                     << cpnnum << "/"
                     << status;

    make_curs(
"update WC_COUPON set STATUS=:status "
"where OWNER_AIRLINE=:owner_airline and "
"TICKNUM=:ticknum and "
"NUM=:cpnnum")
            .stb()
            .bind(":owner_airline", airline)
            .bind(":ticknum",       ticknum.get())
            .bind(":cpnnum",        cpnnum.get())
            .bind(":status",        status->toBaseType())
            .exec();
}

static void saveTicketWc(const std::string& recloc,
                         const Ticketing::Airline_t& airline,
                         const Ticketing::Ticket& ticket)
{
    LogTrace(TRACE3) << __FUNCTION__ << " "
                     << recloc << "/"
                     << airline << "/"
                     << ticket.ticknum();

    for(const auto& cpn: ticket.getCoupon()) {
        saveCouponWc(recloc, airline, cpn);
    }

    make_curs(
"begin "
"delete from WC_TICKET where AIRLINE=:airline and TICKNUM=:ticknum; "
"insert into WC_TICKET(RECLOC, AIRLINE, TICKNUM) "
"values (:recloc, :airline, :ticknum); "
"end;")
            .bind(":recloc",  recloc)
            .bind(":airline", airline)
            .bind(":ticknum", ticket.ticknumt().get())
            .exec();
}

static void savePnrEdifact(const std::string& recloc,
                           const Ticketing::Airline_t& airline,
                           const std::string& ediText,
                           edifact::EdiMessageType ediType)
{
    const size_t PageSize = 1000;
    LogTrace(TRACE3) << __FUNCTION__ << " for recloc=" << recloc
                     << "; ediText size=" << ediText.size()
                     << "; pageSize=" << PageSize;

    std::string::const_iterator itb = ediText.begin(), ite = itb;
    for(size_t pageNo = 1; itb < ediText.end(); pageNo++)
    {
        ite = itb + PageSize;
        if(ite > ediText.end()) ite = ediText.end();
        std::string page(itb, ite);
        LogTrace(TRACE3) << "pageNo=" << pageNo << "; page=" << page;
        itb = ite;

        make_curs(
"insert into WC_PNR(RECLOC, AIRLINE, PAGE_NO, TLG_TEXT, TLG_TYPE) "
"values (:recloc, :airline, :page_no, :edi_text, :edi_type)")
        .bind(":recloc",   recloc)
        .bind(":airline",  airline)
        .bind(":page_no",  pageNo)
        .bind(":edi_text", page)
        .bind(":edi_type", static_cast<int>(ediType))
        .exec();
    }
}

static void saveWcPnr(const std::string& recloc,
                      const Ticketing::Airline_t& airline,
                      const EdiPnr& ediPnr)
{
    LogTrace(TRACE3) << __FUNCTION__ << " "
                     << recloc << "/"
                     << airline;

    LogTrace(TRACE3) << ediPnr;
    savePnrEdifact(recloc, airline, ediPnr.m_ediText, ediPnr.m_ediType);
}

static bool controlMayBeReturned(const WcCoupon& cpn)
{
    if(cpn.status() == Ticketing::CouponStatus::OriginalIssue ||
       cpn.status() == Ticketing::CouponStatus::Flown) {
        return true;
    }

    return false;
}

//---------------------------------------------------------------------------------------

std::ostream& operator<<(std::ostream& os, const EdiPnr& ediPnr)
{
    os << "Message type: " << ediPnr.m_ediType << "\n";
    os << "Message text: " << ediPnr.m_ediText;
    return os;
}

//---------------------------------------------------------------------------------------

WcCouponNotFound::WcCouponNotFound(const Ticketing::Airline_t& airl,
                                   const Ticketing::TicketNum_t& tick,
                                   const Ticketing::CouponNum_t& cpn)
    : EXCEPTIONS::Exception("")
{
    std::string err = "WcCoupon not found: ";
    err += BaseTables::Company(airl)->code() + "/" + tick.get() + "/" +
            std::to_string(cpn.get());

    setMessage(err);
}

WcCouponNotFound::WcCouponNotFound(const std::string& recloc,
                                   const Ticketing::TicketNum_t& tick,
                                   const Ticketing::CouponNum_t& cpn)
    : EXCEPTIONS::Exception("")
{
    std::string err = "WcCoupon not found: ";
    err += recloc + "/" + tick.get() + "/" + std::to_string(cpn.get());

    setMessage(err);
}

//---------------------------------------------------------------------------------------

void saveWcPnr(const Ticketing::Airline_t& airline, const EdiPnr& ediPnr)
{
    const std::string recloc = Recloc::GenerateRecloc();

    Ticketing::Pnr pnr = readPnr(ediPnr);
    Pnr::Trace(TRACE2, pnr);

    for(const auto& ticket: pnr.ltick()) {
        if(ticket.actCode() == TickStatAction::newtick) {
            saveTicketWc(recloc, airline, ticket);
        }
    }

    saveWcPnr(recloc, airline, ediPnr);
}

void loadWcEdiPnr(const std::string& recloc, boost::optional<EdiPnr>& ediPnr)
{
    ediPnr=boost::none;

    std::string res, page;
    int tlgType = 0;
    OciCpp::CursCtl cur = make_curs(
"select TLG_TEXT, TLG_TYPE from WC_PNR where RECLOC=:recloc "
"order by PAGE_NO");
    cur.bind(":recloc", recloc)
       .def(page)
       .def(tlgType)
       .exec();
    while(!cur.fen()) {
        res += page;
    }

    if(res.empty()) {
        return;
    }

    ediPnr=EdiPnr(res, static_cast<edifact::EdiMessageType>(tlgType));
}

boost::optional<WcPnr> loadWcPnr(const std::string& recloc)
{
  LogTrace(TRACE3) << "Enter to " << __FUNCTION__ << "; recloc=" << recloc;

  boost::optional<EdiPnr> ediPnr;
  loadWcEdiPnr(recloc, ediPnr);

  if (!ediPnr) return boost::none;

  return WcPnr(recloc, readPnr(ediPnr.get()));
}

boost::optional<WcPnr> loadWcPnr(const Ticketing::Airline_t& airline,
                                 const Ticketing::TicketNum_t& tickNum)
{
    boost::optional<WcTicket> ticket = readWcTicket(airline, tickNum);
    if(ticket) {
        return loadWcPnr(ticket->recloc());
    }

    return boost::none;
}

boost::optional<WcPnr> loadWcPnrWithActualStatuses(const std::string& recloc)
{
    boost::optional<WcPnr> wcPnr = loadWcPnr(recloc);
    if(!wcPnr) return wcPnr;

    for(auto& ticket: wcPnr->pnr().ltick()) {
        for(auto& coupon: ticket.getCoupon()) {
            WcCoupon wcCpn = readWcCouponByRl(recloc,
                                              Ticketing::TicketNum_t(coupon.ticknum()),
                                              Ticketing::CouponNum_t(coupon.couponInfo().num()));
            coupon.couponInfo().setStatus(wcCpn.status());
        }
    }

    return wcPnr;
}

boost::optional<WcPnr> loadWcPnrWithActualStatuses(const Ticketing::Airline_t& airline,
                                                   const Ticketing::TicketNum_t& tickNum)
{
    boost::optional<WcPnr> wcPnr = loadWcPnr(airline, tickNum);
    if(!wcPnr) return wcPnr;

    for(auto& ticket: wcPnr->pnr().ltick()) {
        for(auto& coupon: ticket.getCoupon()) {
            boost::optional<WcCoupon> wcCpn;
            wcCpn = readWcCoupon(airline,
                                 Ticketing::TicketNum_t(coupon.ticknum()),
                                 Ticketing::CouponNum_t(coupon.couponInfo().num()));
            ASSERT(wcCpn);
            coupon.couponInfo().setStatus(wcCpn->status());
        }
    }

    return wcPnr;
}

boost::optional<WcTicket> readWcTicket(const Ticketing::Airline_t& airline,
                                       const Ticketing::TicketNum_t& tickNum)
{
    OciCpp::CursCtl cur = make_curs(
"select RECLOC "
"from WC_TICKET "
"where AIRLINE=:airline and TICKNUM=:ticknum");

    std::string recloc;
    cur
            .stb()
            .def(recloc)
            .bind(":airline", airline)
            .bind(":ticknum", tickNum.get())
            .EXfet();

    if(cur.err() == NO_DATA_FOUND) {
        return boost::none;
    }

    return WcTicket(recloc, airline, tickNum);
}

boost::optional<WcCoupon> readWcCoupon(const Ticketing::Airline_t& airline,
                                       const Ticketing::TicketNum_t& tickNum,
                                       const Ticketing::CouponNum_t& cpnNum)
{
    OciCpp::CursCtl cur = make_curs(
"select RECLOC, STATUS "
"from WC_COUPON "
"where OWNER_AIRLINE=:airline and TICKNUM=:ticknum and NUM=:cpnnum");

    std::string recloc;
    int status = 0;
    cur
            .stb()
            .def(recloc)
            .def(status)
            .bind(":airline", airline)
            .bind(":ticknum", tickNum.get())
            .bind(":cpnnum",  cpnNum.get())
            .EXfet();

    if(cur.err() == NO_DATA_FOUND) {
        return boost::none;
    }

    return WcCoupon(recloc, airline, tickNum, cpnNum, CouponStatus(status));
}

WcCoupon readWcCouponByRl(const std::string& recloc,
                          const Ticketing::TicketNum_t& tickNum,
                          const Ticketing::CouponNum_t& cpnNum)
{
    OciCpp::CursCtl cur = make_curs(
"select OWNER_AIRLINE, STATUS "
"from WC_COUPON "
"where RECLOC=:recloc and TICKNUM=:ticknum and NUM=:cpnnum");

    Ticketing::Airline_t airline;
    int status = 0;
    cur
            .stb()
            .def(airline)
            .def(status)
            .bind(":recloc",  recloc)
            .bind(":ticknum", tickNum.get())
            .bind(":cpnnum",  cpnNum.get())
            .EXfet();

    if(cur.err() == NO_DATA_FOUND) {
        throw WcCouponNotFound(recloc, tickNum, cpnNum);
    }

    return WcCoupon(recloc, airline, tickNum, cpnNum, CouponStatus(status));
}

bool changeOfStatusWcCoupon(const Ticketing::Airline_t& airline,
                            const Ticketing::TicketNum_t& ticknum,
                            const Ticketing::CouponNum_t& cpnnum,
                            const Ticketing::CouponStatus& newStatus,
                            bool throwErr)
{
  if (!existsAirportControl(airline, ticknum, cpnnum, throwErr)) return false;

    boost::optional<WcCoupon> cpn = readWcCoupon(airline, ticknum, cpnnum);
    if(!cpn) {
        LogWarning(STDLOG) << "Coupon not found: "
                           << airline << "/" << ticknum << "/" << cpnnum;
        if(throwErr) {
            throw WcCouponNotFound(airline, ticknum, cpnnum);
        }
        return false;
    }

    updateCouponWc(airline, ticknum, cpnnum, newStatus);
    return true;
}

bool changeOfStatusWcCoupon(const std::string& airline,
                            const std::string& ticknum,
                            unsigned cpnnum,
                            const Ticketing::CouponStatus& newStatus,
                            bool throwErr)
{
    return changeOfStatusWcCoupon(BaseTables::Company(airline)->ida(),
                                  Ticketing::TicketNum_t(ticknum),
                                  Ticketing::CouponNum_t(cpnnum),
                                  newStatus,
                                  throwErr);
}

bool returnWcCoupon(const Ticketing::Airline_t& airline,
                    const Ticketing::TicketNum_t& ticknum,
                    const Ticketing::CouponNum_t& cpnnum,
                    bool throwErr)
{
    LogTrace(TRACE3) << __FUNCTION__ << " "
                     << airline << "/" << ticknum << "/" << cpnnum;

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

    boost::optional<WcCoupon> cpn = readWcCoupon(airline, ticknum, cpnnum);
    if(!cpn) {
        LogWarning(STDLOG) << "Coupon not found: "
                           << airline << "/" << ticknum << "/" << cpnnum;
        if(throwErr) {
            throw WcCouponNotFound(airline, ticknum, cpnnum);
        }
        return false;
    }

    if(!controlMayBeReturned(*cpn)) {
        LogWarning(STDLOG) << "Control "
                           << airline << "/" << ticknum << "/" << cpnnum
                           << " may NOT be returned due to status " << cpn->status();

        if(throwErr) {
            throw AirportControlCantBeReturned(airline, ticknum, cpnnum);
        }
        return false;
    }

    ac->deleteDb();
    try {
      callbacks<AstraPnrCallbacks>()->afterReturnAirportControl(*cpn);
    } catch(...) {
      CallbacksExceptionFilter(STDLOG);
    }
    return true;
}

}//namespace Ticketing
