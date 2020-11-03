/*	2006 by Roman Kovalev 	*/
/*	roman@pike.dev.sirena2000.ru		*/
#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif

#include <serverlib/dates_io.h>
#include "etick/tick_data.h"
#include "etick/edi_cast.h"
#include "etick/exceptions.h"
#include "etick/lang.h"
#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/test.h>
#include <serverlib/slogger.h>

namespace Ticketing{
namespace EdiCast {
    CouponStatus CoupStatCast::operator () (const BaseFDE &de)
    {
        try {
            if(de.dataStr() == "G")
#warning hardcode for 1A ETKT - G->708
                return CouponStatus("708"); // exchanged/FIM
            else
                return CouponStatus(de.dataStr());
        }
        catch (NoSuchCode &e){
            throw BadCast(de, "Coupon Status", Err);
        }
    }

    ItinStatus ItinStatCast::operator () (const BaseFDE &de)
    {
        try{
            return ItinStatus(de.dataStr());
        }
        catch(NoSuchCode &e) {
            if(MaybyNumSeat &&
               de.dataStr().size() <= ItinStatus::Maxlen)
            {
                ItinStatus stat(ItinStatus::Confirmed);
                stat.setSeats(de.dataStr());
                return stat;
            } else {
                throw BadCast(de, "Itinerary status code", Err);
            }
        }
    }

boost::gregorian::date DateCast::operator () (const BaseFDE &de)
{
    try{
        if(de.dataStr().size()==0){
            return boost::gregorian::date();
        } else {
            boost::gregorian::date date1 = HelpCpp::date_cast(de.dataStr().c_str(),
                                            DateForm, ENGLISH);
            return date1;
        }
    }
    catch (std::ios_base::failure &e){
        ProgTrace(TRACE1, "%s", e.what());
        throw BadCast (de, ("Date/Time: "+string(DateForm)), Err);
    }
}

boost::posix_time::time_duration TimeCast::operator () (const BaseFDE &de)
{
    try{
        if(de.dataStr().size()==0){
            return boost::posix_time::time_duration(boost::date_time::not_a_date_time);
        } else {
            boost::posix_time::time_duration time1 = HelpCpp::timed_cast(de.dataStr().c_str(),
                    TimeForm, ENGLISH);
            return time1;
        }
    }
    catch (std::ios_base::failure &e){
        ProgTrace(TRACE1, "%s", e.what());
        throw BadCast (de, ("Time: "+string(TimeForm)), Err);
    }
}

SubClass RBDCast::operator () (const BaseFDE &de)
{
    try {
        if(de.dataStr().empty()) {
            return SubClass();
        } else {
            return SubClass(de.dataStr());
        }
    }
    catch(NoSuchCode &e)
    {
        ProgTrace(TRACE1, "%s", e.what());
        throw BadCast (de, "Reservation booking designator", Err);
    }
}

TickStatAction::TickStatAction_t TickActCast::operator () (const BaseFDE &de)
{
    try {
        if(de.dataStr().size()==0){
            return TickStatAction::newtick;
        }
        return TickStatAction::GetTickAction(de.dataStr().c_str());
    }
    catch(TickExceptions::tick_exception &e)
    {
        ProgTrace(TRACE1, "%s", e.what());
        throw BadCast(de, "Ticket action code", Err);
    }
}

CpnStatAction::CpnStatAction_t CpnActCast::operator () (const BaseFDE &de)
{
    try {
        return CpnStatAction::GetCpnAction(de.dataStr().c_str());
    }
    catch(TickExceptions::tick_exception &e)
    {
        ProgTrace(TRACE1, "%s", e.what());
        throw BadCast(de, "Coupon action code", Err);
    }
}

TaxAmount::Amount amountFromString(const std::string &amount,
                                   TaxAmount::Amount::AmountType_e Type)
{
    std::string str = amount;
    if(!str.empty() && str[str.length() - 1] == 'A') {
        // Amadeus присылает признак 'A' (additional fare) в елементе T
        str = str.substr(0, str.length() - 1);
    }
    TaxAmount::Amount am(str, Type);
    return am;
}

TaxAmount::Amount AmountCast::operator () (const BaseFDE &de)
{
    try
    {
        return amountFromString(de.dataStr(), Type);
    }
    catch (TickExceptions::tick_exception &e)
    {
        ProgTrace(TRACE1, "%s", e.what());
        if(!Err.empty())
            throw BadCast(de, "Amount", Err);
        else
            return TaxAmount::Amount();
    }
}

PassengerType PassTypeCast::operator () (const BaseFDE &de)
{
    try
    {
        if(de.dataStr().empty())
            return PassengerType();
        else
            return PassengerType(de.dataStr());
    }
    catch (NoSuchCode &e)
    {
        ProgTrace(TRACE1, "%s", e.what());
        throw BadCast (de, "Passenger type", Err);
    }
}

AmountCode AmountCodeCast::operator () (const BaseFDE &de)
{
    try{
        return AmountCode(de.dataStr());
    }
    catch (NoSuchCode &e)
    {
        ProgTrace(TRACE1, "%s", e.what());
        throw BadCast (de, "Amount Code", Err);
    }
}

FoidType EdiCast::FoidTypeCast::operator ( )(const BaseFDE & de)
{
    try
    {
        return FoidType(de.dataStr());
    }
    catch(NoSuchCode &e)
    {
        ProgTrace(TRACE1, "%s", e.what());
        throw BadCast (de, "Form of ID type", Err);
    }
}

FopIndicator FopIndicatorCast::operator ( )(const BaseFDE & de)
{
    try
    {
        if(de.dataStr().empty())
            return FopIndicator(defVal);
        else
            return FopIndicator(de.dataStr());
    }
    catch(NoSuchCode &e)
    {
        LogTrace(TRACE1) << e.what();
        throw BadCast (de, "Fop Indicator", Err);
    }

}

TaxCategory TaxCategoryCast::operator ( )(const BaseFDE & de)
{
    try
    {
        if(de.dataStr().empty())
            return TaxCategory(defVal);
        else
            return TaxCategory(de.dataStr());
    }
    catch(NoSuchCode &e)
    {
        LogTrace(TRACE1) << e.what();
        throw BadCast (de, "Tax category", Err);
    }
}

TicketMedia EdiCast::TicketMediaCast::operator () (const BaseFDE & de)
{
    try
    {
        if(de.dataStr().empty())
            return TicketMedia();
        return TicketMedia(de.dataStr());
    }
    catch(NoSuchCode &e)
    {
        LogTrace(TRACE1) << e.what();
        throw BadCast (de, TicketMediaElem::ElemName, Err);
    }
}

DocType DocTypeCast::operator ()(const BaseFDE &de)
{
    try {
        if(de.dataStr().empty())
            return DocType();
        return DocType(de.dataStr());
    }
    catch(NoSuchCode &e) {
        LogTrace(TRACE1) << e.what();
        throw BadCast(de, TicketMediaElem::ElemName, Err);
    }
}

RficType RficTypeCast::operator () (const BaseFDE &de)
{
    try {
        if(de.dataStr().empty())
            return RficType();
        return RficType(de.dataStr());
    }
    catch(NoSuchCode &e) {
        LogTrace(TRACE1) << e.what();
        throw BadCast(de, RficTypeList::ElemName, Err);
    }
}

PricingTicketingIndicator PricingTicketingIndicatorCast::operator () (const BaseFDE &de)
{
    try {
        ProgTrace(TRACE1, "dataStr = %s", de.dataStr().c_str());
        if(de.dataStr().empty())
            return PricingTicketingIndicator();
        else
            return PricingTicketingIndicator(de.dataStr());
    }
    catch(NoSuchCode &e) {
        LogTrace(TRACE1) << e.what();
        throw BadCast(de, PricingTicketingIndicatorElem::ElemName, Err);
    }
}

PricingTicketingSellType PricingTicketingSellTypeCast::operator () (const BaseFDE &de)
{
    try {
        ProgTrace(TRACE1, "dataStr = %s", de.dataStr().c_str());
        if(de.dataStr().empty())
            return PricingTicketingSellType();
        else
            return PricingTicketingSellType(de.dataStr());
    }
    catch(NoSuchCode &e) {
        LogTrace(TRACE1) << e.what();
        throw BadCast(de, PricingTicketingSellTypeElem::ElemName, Err);
    }
}

} // namespace EdiCast
} // namespace Ticketing
