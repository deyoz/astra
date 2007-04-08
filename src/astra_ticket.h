#ifndef _ASTRA_TICKET_H_
#define _ASTRA_TICKET_H_
#include <utility>
#include <string>
#include "etick/ticket.h"
#include "etick/tick_data.h"
#include "etick/tickmng.h"
#include "etick/ticket.h"
#include "astra_utils.h"

namespace Ticketing{
// NOTE: See eticklib for realization

typedef BaseCoupon_info Coupon_info;


// NOTE: See eticklib for realization
typedef BaseLuggage Luggage;

// NOTE: See eticklib for realization
typedef BaseFrequentPass FrequentPass;

// NOTE: See eticklib for realization
class Itin : public BaseItin<Luggage>
{
public:
    typedef boost::shared_ptr< Itin > SharedPtr;
    Itin(const string &air,
         const string &oper_air,
         int flight,
         int cls,
         const boost::gregorian::date  &date1,
         const boost::posix_time::time_duration &time1,
         const string &depPoint,
         const string &arrPoint)
    :
        BaseItin<Luggage>("",
                          air,
                          oper_air,
                          flight,
                          cls,
                          date1,
                          time1,
                          boost::gregorian::date(),
                          boost::posix_time::time_duration(),
                          depPoint,
                          arrPoint,
                          pair<boost::gregorian::date, boost::gregorian::date>(),
                          ItinStatus::itin_status(),
                          "",// Fare Basis
                          -1, // Version
                          Luggage())
    {
    }

    Itin(const string &tnum,
         const string &air,
         const string &oper_air,
         int flight,
         int cls,
         const boost::gregorian::date  &date1,
         const boost::posix_time::time_duration &time1,
         const boost::gregorian::date  &date2,
         const boost::posix_time::time_duration &time2,
         const string &depPoint,
         const string &arrPoint,
         const pair<boost::gregorian::date, boost::gregorian::date> &VldDates,
         const ItinStatus::itin_status &rpiStat,
         const string &Fare,
         int ver,
         const Luggage &lugg)
    :
            BaseItin<Luggage>(tnum,
                              air,
                              oper_air,
                              flight,
                              cls,
                              date1,
                              time1,
                              date2,
                              time2,
                              depPoint,
                              arrPoint,
                              VldDates,
                              rpiStat,
                              Fare,
                              ver, // Version
                              lugg)
    {
    }
};

// NOTE: See eticklib for realization
typedef BaseFreeTextInfo FreeTextInfo;

// NOTE: See eticklib for realization
class Coupon : public BaseCoupon<Coupon_info, Itin, FrequentPass, FreeTextInfo>
{
public:
    Coupon(const Coupon_info &ci)
    :BaseCoupon<Coupon_info, Itin, FrequentPass, FreeTextInfo>(ci,"")
    {
    }
    Coupon(const Coupon_info &ci,
           const Itin &i,
           const list<FrequentPass> &fPass,
           const std::string &tnum)
    :BaseCoupon<Coupon_info, Itin, FrequentPass, FreeTextInfo>
            (ci,i,fPass,tnum)
    {
    }

    Coupon(const Coupon_info &ci,
           const Itin &i)
    :BaseCoupon<Coupon_info, Itin, FrequentPass, FreeTextInfo>(ci,i,"")
    {
    }

};

// NOTE: See eticklib for realization
class Ticket : public BaseTicket<Coupon>
{
public:
    Ticket(const std::string &ticknum,const std::list<Coupon> &lcoup)
    : BaseTicket<Coupon>(ticknum, TickStatAction::newtick, 1, lcoup)
    {
    }
    Ticket(const std::string &ticknum,
           TickStatAction::TickStatAction_t tick_act,
           unsigned short num,
           const std::list<Coupon> &lcoup)
    : BaseTicket<Coupon>(ticknum, tick_act, num, lcoup)
    {
    }
};

// NOTE: See eticklib for realization
typedef BaseTaxDetails TaxDetails;

typedef BasePassenger Passenger;

typedef BaseMonetaryInfo MonetaryInfo;

typedef BaseFormOfPayment FormOfPayment;

typedef BaseResContrInfo ResContrInfo;

//typedef BaseOrigOfRequest OrigOfRequest ;
class OrigOfRequest : public BaseOrigOfRequest
{
public:
    OrigOfRequest(const std::string & airline,
                  const TReqInfo &req)
    : BaseOrigOfRequest(
                        airline,
                        "ŒŽ‚",
                        "","",//ppr,agn
                        req.desk.city,
                        'Y',
                        req.desk.code,
                        "",
                        Lang::RUSSIAN)
    {
    }

    OrigOfRequest(const std::string & airline)
    : BaseOrigOfRequest(
                        airline,
                        "ŒŽ‚",
                        "","",//ppr,agn
                        "ŒŽ‚",
                        'Y',
                        "SYSTEM",
                        "",
                        Lang::RUSSIAN)
    {
    }

    OrigOfRequest(const std::string & airline,
                  const std::string & location,
                  const std::string & ppr,
                  const std::string & agn,
                  const std::string & originLocation,
                  char type,
                  const std::string &pult,
                  const std::string &authCode,
                  Lang::Language lang=Lang::ENGLISH)
    :BaseOrigOfRequest(
                  airline,
                  location,
                  ppr,
                  agn,
                  originLocation,
                  type,
                  pult,
                  authCode,
                  lang)
    {
    }

};

typedef BasePnr< OrigOfRequest,
                 ResContrInfo,
                 Passenger,
                 Ticket,
                 TaxDetails,
                 MonetaryInfo,
                 FormOfPayment,
                 FreeTextInfo>  Pnr;

typedef BasePnrListItem<OrigOfRequest,
                        ResContrInfo,
                        Passenger,
                        Ticket,
                        FormOfPayment> PnrListItem;

typedef BasePnrList <PnrListItem> PnrList;
}
#endif /*_ASTRA_TICKET_H_*/
