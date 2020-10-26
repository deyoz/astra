#pragma once
#include <string>

#include "passenger.h"
#include "astra_misc.h"
#include "ckin_search.h"
#include "rfisc_sirena.h"

#include "jxtlib/JxtInterface.h"

namespace MobilePayment
{

class Response
{
  public:
    AstraLocale::OutputLang outputLang;

    Response() : outputLang(AstraLocale::OutputLang("", {AstraLocale::OutputLang::OnlyTrueIataCodes})) {}
    static void errorToXML(const std::exception& e, xmlNodePtr reqNode, xmlNodePtr resNode);
};

class SearchPassengersRequest
{
  public:

    class Depth
    {
      private:
        FlightFilter filter;
      public:
        std::string departure;
        int hours=ASTRA::NoExists;
        boost::optional<CheckIn::TPaxDocItem> doc;
        boost::optional<SurnameFilter> pax;

        Depth& fromXML(xmlNodePtr node);
        const FlightFilter& getFlightFilter() const { return filter; }
        bool completeForSearch() const
        {
          return !departure.empty() &&
                 hours!=ASTRA::NoExists &&
                 (doc || pax);
        }
    };

    class Segment : public TTripInfo
    {
      private:
        FlightFilter filter;
      public:
        TDateTime departure_date_scd=ASTRA::NoExists;
        std::string destination;

        Segment() { flt_no=ASTRA::NoExists; }

        Segment& fromXML(xmlNodePtr node);
        const FlightFilter& getFlightFilter() const { return filter; }
        bool completeForSearch() const
        {
          return !airline.empty() &&
                 flt_no!=ASTRA::NoExists &&
                 departure_date_scd!=ASTRA::NoExists &&
                 !airp.empty();
        }
    };

    class Barcode : public std::string
    {
      private:
        BarcodeFilter filter;
      public:
        Barcode& fromXML(xmlNodePtr node);
        const BarcodeFilter& getBarcodeFilter() const { return filter; }
        bool completeForSearch() const
        {
          return !filter.empty();
        }
    };

    std::string departure;
    boost::optional<Segment> oper;
    boost::optional<CheckIn::TPaxTknItem> tkn;
    boost::optional<TPnrAddrInfo> pnr;
    boost::optional<Depth> depth;
    boost::optional<Barcode> barcode;


    SearchPassengersRequest& fromXML(xmlNodePtr node);
};

class SearchFlightsRequest
{
  public:
    std::string departure;
    int depth_hours;
    TDateTime departure_datetime;
    std::string oper_carrier;

    SearchFlightsRequest& fromXML(xmlNodePtr node);
};

class Stages
{
  public:
    enum StageStatus {Open, Close};

    StageStatus checkin=Close;
    StageStatus webCheckin=Close;
    StageStatus boarding=Close;

    void set(int point_id);
    const Stages& toXML(xmlNodePtr node) const;
    static std::string statusStr(const StageStatus &status);
};

class Segment : public Stages
{
   public:
     TAdvTripRouteItem departure, arrival;

     void setStages(int point_id) { Stages::set(point_id); }
     const Segment& toXML(xmlNodePtr node,
                          const AstraLocale::OutputLang& lang) const;
};

class Flight : public TAdvTripInfo, public Stages
{
  public:
    Flight(const TAdvTripInfo& flt) : TAdvTripInfo(flt) {}

    TAdvTripRoute routeAfter;
    std::string check_in_desks, gates;

    void setStages(int point_id) { Stages::set(point_id); }
    const Flight& toXML(xmlNodePtr node,
                        const AstraLocale::OutputLang& lang) const;
};

class Passenger : public CheckIn::TSimplePaxItem
{
  public:
    TPaxSegmentPair segmentPair;
    CheckIn::TPaxDocItem doc;
    TPnrAddrs pnrAddrs;


    Passenger(const CheckIn::TSimplePaxItem& _pax,
              const TPaxSegmentPair& _segmentPair) :
      CheckIn::TSimplePaxItem(_pax), segmentPair(_segmentPair) {}
    const Passenger& toXML(xmlNodePtr node,
                           const Segment& segment,
                           const AstraLocale::OutputLang& lang) const;

    std::string categoryStr() const;
    std::string genderStr() const;
    std::string statusStr() const;
};

typedef ASTRA::Cache<TPaxSegmentPair, Segment> SegmentCache;

class SearchPassengersResponse : public Response,
                                 public SegmentCache,
                                 public PnrFlightsCache,
                                 public PaxGrpCache
{
  private:
    std::list<Passenger> passengers;
    void add(const CheckIn::TSimplePaxItem& pax,
             const std::string& reqDeparture);
    bool suitable(const Passenger& passenger,
                  const SearchPassengersRequest& req) const;
  public:
    void searchPassengers(const SearchPassengersRequest& req);
    void filterPassengers(const SearchPassengersRequest& req);
    const SearchPassengersResponse& toXML(xmlNodePtr node) const;
};

class SearchFlightsResponse : public Response
{
  private:
    std::list<Flight> flights;
    void add(const TAdvTripInfo& flt);
  public:
    void searchFlights(const SearchFlightsRequest& req);
    const SearchFlightsResponse& toXML(xmlNodePtr node) const;
};

class GetClientPermsResponse : public Response
{
  public:
    const GetClientPermsResponse& toXML(xmlNodePtr node) const;
};

class GetPassengerInfoResponse : public Response
{
  private:
    SirenaExchange::TEntityList entities;
  public:
    void prepareEntities(const PaxId_t& paxId);
    const GetPassengerInfoResponse& toXML(xmlNodePtr node) const;

    static boost::optional<std::pair<TAdvTripInfo, std::string>> getSegmentInfo(const CheckIn::TSimplePaxItem& pax);
};

} //namespace MobilePayment

class MobilePaymentInterface: public JxtInterface
{
    public:
        MobilePaymentInterface(): JxtInterface("", "MobilePayment")
        {
            AddEvent("search_passengers",    JXT_HANDLER(MobilePaymentInterface, searchPassengers));
            AddEvent("search_flights",       JXT_HANDLER(MobilePaymentInterface, searchFlights));
            AddEvent("get_client_perms",     JXT_HANDLER(MobilePaymentInterface, getClientPerms));
            AddEvent("get_passenger_info",   JXT_HANDLER(MobilePaymentInterface, getPassengerInfo));
        }

        void searchPassengers(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void searchFlights(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void getClientPerms(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void getPassengerInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};
