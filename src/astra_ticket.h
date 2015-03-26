#ifndef _ASTRA_TICKET_H_
#define _ASTRA_TICKET_H_
#include <utility>
#include <string>
#include "etick/ticket.h"
#include "etick/tick_data.h"
#include "etick/tickmng.h"
#include "etick/ticket.h"
#include "astra_utils.h"
#include "astra_dates.h"
#include "ticket_types.h"
#include "xml_unit.h"
#include "tlg/CheckinBaseTypes.h"

namespace Ticketing{
// NOTE: See eticklib for realization

typedef BaseCoupon_info Coupon_info;

typedef BaseFormOfId FormOfId;

// NOTE: See eticklib for realization
typedef BaseLuggage Luggage;

// NOTE: See eticklib for realization
typedef BaseFrequentPass FrequentPass;

// NOTE: See eticklib for realization
class Itin : public BaseItin<Luggage>
{
public:
    typedef boost::shared_ptr< Itin > SharedPtr;
    Itin(const std::string &air,
         const std::string &oper_air,
         int flight,
         int oper_flight,
         const SubClass &cls,
         const boost::gregorian::date  &date1,
         const boost::posix_time::time_duration &time1,
         const std::string &depPoint,
         const std::string &arrPoint)
    :
        BaseItin<Luggage>("",
                          air,
                          oper_air,
                          flight,
                          oper_flight,
                          cls,
                          date1,
                          time1,
                          boost::gregorian::date(),
                          boost::posix_time::time_duration(),
                          depPoint,
                          arrPoint,
                          std::pair<boost::gregorian::date, boost::gregorian::date>(),
                          ItinStatus(),
                          "",// Fare Basis
                          -1, // Version
                          Luggage())
    {
    }

    Itin(const std::string &tnum,
         const std::string &air,
         const std::string &oper_air,
         int flight,
         int oper_flight,
         const SubClass &cls,
         const boost::gregorian::date  &date1,
         const boost::posix_time::time_duration &time1,
         const boost::gregorian::date  &date2,
         const boost::posix_time::time_duration &time2,
         const std::string &depPoint,
         const std::string &arrPoint,
         const std::pair<boost::gregorian::date, boost::gregorian::date> &VldDates,
         const ItinStatus &rpiStat,
         const std::string &Fare,
         int ver,
         const Luggage &lugg)
    :
            BaseItin<Luggage>(tnum,
                              air,
                              oper_air,
                              flight,
                              oper_flight,
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

    Itin(const std::string &airline,
        const std::string &operAirline,
        const FlightNum_t &flNum,
        const FlightNum_t &operFlNum,
        const Dates::Date_t &depDate,
        const Dates::time_duration &depTime,
        const Dates::Date_t &arrDate,
        const Dates::time_duration &arrTime,
        const std::string &depPoint,
        const std::string &arrPoint,
        int ver = 1)
        :
          BaseItin<Luggage>("",
               airline,
               operAirline,
               flNum ? flNum.get() : 0,
               operFlNum ? operFlNum.get() : 0,
               SubClass(),
               depDate,
               depTime,
               arrDate,
               arrTime,
               depPoint,
               arrPoint,
               std::make_pair(Dates::Date_t(), Dates::Date_t()),
               ItinStatus(),
               "",
               ver,
               Luggage())
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
           const std::list<FrequentPass> &fPass,
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

    Coupon(const Coupon_info &ci, const std::string ticknum)
    :BaseCoupon<Coupon_info, Itin, FrequentPass, FreeTextInfo>(ci, ticknum)
    {
    }
};

// NOTE: See eticklib for realization
class Ticket : public BaseTicket<Coupon>
{
    Ticketing::TicketNum_t ConnectedDocNum;
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
    Ticketing::TicketNum_t ticknumt() const { return Ticketing::TicketNum_t(ticknum()); }
    void setConnectedDocNum(const Ticketing::TicketNum_t &ticknum) { ConnectedDocNum = ticknum; }
    Ticketing::TicketNum_t connectedDocNum() const { return ConnectedDocNum; }
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
    OrigOfRequest(const std::string &airline,
                  const TReqInfo &req)
    : BaseOrigOfRequest(airline,
                        "ŒŽ‚",
                        "","",//ppr,agn
                        req.desk.city,
                        'Y',
                        req.desk.code,
                        "",
                        req.desk.lang == AstraLocale::LANG_RU?RUSSIAN:ENGLISH)
    {
    }

    OrigOfRequest(const std::string &airline)
    : BaseOrigOfRequest(airline,
                        "ŒŽ‚",
                        "","",//ppr,agn
                        "ŒŽ‚",
                        'Y',
                        "SYSTEM",
                        "",
                        RUSSIAN)
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
                  Language lang)
    : BaseOrigOfRequest(
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

    static void toXML(const OrigOfRequest &orig,
                      const FlightNum_t &flNum,
                      xmlNodePtr node)
    {
      if (node==NULL) return;
      xmlNodePtr origNode=NewTextChild(node, "orig_of_request");
      NewTextChild(origNode, "airline", orig.airlineCode());
      flNum?NewTextChild(origNode, "flt_no", (int)flNum.get()):
            NewTextChild(origNode, "flt_no");
      NewTextChild(origNode, "desk_city", orig.originLocationCode());
      NewTextChild(origNode, "desk_code", orig.pult());
      NewTextChild(origNode, "desk_lang", orig.lang()==RUSSIAN?AstraLocale::LANG_RU:AstraLocale::LANG_EN);
    };

    static void fromXML(xmlNodePtr node,
                        OrigOfRequest &orig,
                        FlightNum_t &flNum)
    {
      if (node==NULL) return;
      xmlNodePtr origNode=GetNode("orig_of_request",node);
      if (origNode==NULL) return;
      std::string airline=NodeAsString("airline", origNode);
      if (!NodeIsNULL("flt_no", origNode))
        flNum=FlightNum_t(NodeAsInteger("flt_no", origNode));
      else
        flNum=FlightNum_t();
      std::string originLocation=NodeAsString("desk_city", origNode);
      std::string pult=NodeAsString("desk_code", origNode);
      Language lang=NodeAsString("desk_lang", origNode) == AstraLocale::LANG_RU?RUSSIAN:ENGLISH;

      orig=OrigOfRequest(airline,
                         "ŒŽ‚",
                         "", "",
                         originLocation,
                         'Y',
                         pult,
                         "",
                         lang);
    }

};

typedef BasePnr< OrigOfRequest,
                 ResContrInfo,
                 Passenger,
                 Ticket,
                 TaxDetails,
                 MonetaryInfo,
                 FormOfPayment,
                 FreeTextInfo,
                 FormOfId>  Pnr;

typedef BasePnrListItem<OrigOfRequest,
                        ResContrInfo,
                        Passenger,
                        Ticket,
                        FormOfPayment> PnrListItem;

typedef BasePnrList <PnrListItem> PnrList;
}
#endif /*_ASTRA_TICKET_H_*/
