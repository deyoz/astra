#pragma once
#include <string>

#include "passenger.h"
#include "astra_misc.h"
#include "ckin_search.h"

#include "jxtlib/JxtInterface.h"

namespace MobilePayment
{

class SearchPassengersRequest
{
  public:

    class Depth
    {
      public:
        std::string departure;
        int hours;
        boost::optional<CheckIn::TPaxDocItem> doc;
        boost::optional<SurnameFilter> pax;

        Depth& fromXML(xmlNodePtr node);
        FlightFilter getFlightFilter() const;
    };

    class Segment : public TTripInfo
    {
      public:
        TDateTime departure_date_scd=ASTRA::NoExists;
        std::string destination;

        Segment& fromXML(xmlNodePtr node);
        FlightFilter getFlightFilter() const;
    };

    class Barcode : public std::string
    {
      private:
        BarcodeFilter filter;
      public:
        Barcode& fromXML(xmlNodePtr node);
        const BarcodeFilter& getBarcodeFilter() const { return filter; }
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
    CheckIn::TPaxSegmentPair segmentPair;
    CheckIn::TPaxDocItem doc;
    TPnrAddrs pnrAddrs;


    Passenger(const CheckIn::TSimplePaxItem& _pax,
              const CheckIn::TPaxSegmentPair& _segmentPair) :
      CheckIn::TSimplePaxItem(_pax), segmentPair(_segmentPair) {}
    const Passenger& toXML(xmlNodePtr node,
                           const Segment& segment,
                           const AstraLocale::OutputLang& lang) const;
    std::string categoryStr() const;
    std::string genderStr() const;
    std::string statusStr() const;
};

class SegmentCache
{
  private:
    mutable std::map<CheckIn::TPaxSegmentPair, Segment> segments;
  public:
    const Segment& getSegment(const CheckIn::TPaxSegmentPair& segmentPair) const;

    class NotFound : public EXCEPTIONS::Exception
    {
      public:
        NotFound() : EXCEPTIONS::Exception("SegmentCache: not found") {}
    };
};

class SearchPassengersResponse : public SegmentCache
{
  private:
//    std::map<int/*pnr_id*/, TAdvTripInfoList> flts; !!!vlad ͺνθ¨
//    std::map<int/*grp_id*/, CheckIn::TSimplePaxGrpItem> grps;
    std::list<Passenger> passengers;
    void add(const CheckIn::TSimplePaxItem& pax,
             const std::string& reqDeparture);
  public:
    void searchPassengers(const SearchPassengersRequest& req);
    const SearchPassengersResponse& toXML(xmlNodePtr node,
                                          const AstraLocale::OutputLang& lang) const;
};

class SearchFlightsResponse
{
  private:
    std::list<Flight> flights;
    void add(const TAdvTripInfo& flt);
  public:
    void searchFlights(const SearchFlightsRequest& req);
    const SearchFlightsResponse& toXML(xmlNodePtr node,
                                       const AstraLocale::OutputLang& lang) const;
};

class GetClientPermsResponse
{
  public:
    const GetClientPermsResponse& toXML(xmlNodePtr node,
                                        const AstraLocale::OutputLang& lang) const;
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
        }

        void searchPassengers(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void searchFlights(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
        void getClientPerms(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};
