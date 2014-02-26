#include <boost/lexical_cast.hpp>
#include "ticket_types.h"
#include "etick/exceptions.h"

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

namespace Ticketing {

using namespace TickExceptions;
const std::string TicketNum_t::TchFormCode = "61";
const std::string TicketNum_t::AlOwnFormCode = "24";

Ticketing::TicketNum_t::TicketNum_t(const std::string & tick)
    :Ticket(tick)
{
    if (!tick.empty() && !isValid(tick))
    {
        throw Exception(std::string("Invalid ticket number: ") + tick);
    }
}

bool Ticketing::TicketNum_t::isValid(const std::string &tick)
{
    if (tick.length() != TicketNum_t::StrictLen)
        return false;
    else
        return true;
}

bool TicketNum_t::isTchControlled() const
{
    if(empty())
        return false;
    if(Ticket.substr(3,2) == TchFormCode)
        return true;
    else
        return false;
}

void TicketNum_t::check(const std::string & tick)
{
    if(!isValid(tick))
    {
        throw tick_soft_except(STDLOG, "EtsErr::INV_TICKNUM",
                               (std::string("Invalid ticket length: ")+tick).c_str());
    }
}

std::string TicketNum_t::cut_check_digit(const std::string & tick)
{
    if(tick.length() == TicketNum_t::StrictLen + 1)
        return tick.substr(0, TicketNum_t::StrictLen);
    else
        return tick;
}

std::string TicketNum_t::accNum() const
{
    return get().substr(0,3);
}

CouponNum_t::CouponNum_t(int n)
    :BaseCouponNum_t(n)
{
    if(!isValid(n))
    {
        throw tick_soft_except(STDLOG, "INV_COUPON",
                            (std::string ("Invalid coupon number: ") +
                             boost::lexical_cast<std::string>(get())).c_str());
    }
}

bool CouponNum_t::isValid(int n)
{
    if(n<1 || n>4)
    {
        return false;
    }
    else
    {
        return true;
    }
}

}
