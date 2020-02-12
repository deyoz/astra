#pragma once

#include "astra_types.h"
#include "astra_consts.h"
#include "iatci_types.h"
#include "date_time.h"
#include "xml_unit.h"
#include "term_version.h"

#include <serverlib/xmllibcpp.h>
#include <etick/tick_data.h>

#include <list>
#include <map>
#include <set>
#include <boost/optional.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>


class XMLRequestCtxt;

namespace astra_api {

namespace astra_entities {

struct MarketingInfo
{
    std::string            m_airline;
    unsigned               m_flightNum;
    std::string            m_flightSuffix;
    boost::gregorian::date m_scdDepDate;
    std::string            m_airpDep;

    MarketingInfo(const std::string& airline,
                  unsigned flightNum,
                  const std::string& flightSuffix,
                  const boost::gregorian::date& scdDepDate,
                  const std::string& airpDep);
};

//---------------------------------------------------------------------------------------

struct SegmentInfo
{
    int           m_grpId;
    int           m_pointDep;
    int           m_pointArv;
    std::string   m_airpDep;
    std::string   m_airpArv;
    std::string   m_cls;
    MarketingInfo m_markFlight;

    SegmentInfo(int grpId,
                int pointDep,
                int pointArv,
                const std::string& airpDep,
                const std::string& airpArv,
                const std::string& cls,
                const MarketingInfo& markFlight);
};

//---------------------------------------------------------------------------------------

struct Remark
{
    std::string m_remCode;
    std::string m_remText;

    Remark(const std::string& remCode,
           const std::string& remText);

    std::string id() const { return m_remCode + m_remText; }
    bool containsText(const std::string& text) const;
};

bool operator==(const Remark& left, const Remark& right);
bool operator!=(const Remark& left, const Remark& right);

//---------------------------------------------------------------------------------------

struct FqtRemark
{
    std::string m_remCode;
    std::string m_airline;
    std::string m_fqtNo;
    std::string m_tierLevel;

    FqtRemark(const std::string& remCode,
              const std::string& airline,
              const std::string& fqtNo,
              const std::string& tierLevel = "");

    std::string id() const { return m_remCode + m_airline + m_fqtNo; }
};

bool operator==(const FqtRemark& left, const FqtRemark& right);
bool operator!=(const FqtRemark& left, const FqtRemark& right);

//---------------------------------------------------------------------------------------

struct DocInfo
{
    std::string            m_type;
    std::string            m_country;
    std::string            m_num;
    boost::gregorian::date m_expiryDate;
    std::string            m_surname;
    std::string            m_name;
    std::string            m_secName;
    std::string            m_citizenship;
    boost::gregorian::date m_birthDate;
    std::string            m_gender;

    DocInfo(const std::string& type,
            const std::string& country,
            const std::string& num,
            const boost::gregorian::date& expiryDate,
            const std::string& surname,
            const std::string& name,
            const std::string& secName,
            const std::string& citizenship,
            const boost::gregorian::date& birthDate,
            const std::string& gender);

    bool isEmpty() const;
};

bool operator==(const DocInfo& left, const DocInfo& right);
bool operator!=(const DocInfo& left, const DocInfo& right);

//---------------------------------------------------------------------------------------

struct AddressInfo
{
    std::string m_type;
    std::string m_country;
    std::string m_address;
    std::string m_city;
    std::string m_region;
    std::string m_postalCode;

    AddressInfo(const std::string& type,
                const std::string& country,
                const std::string& address,
                const std::string& city,
                const std::string& region,
                const std::string& postalCode);

    bool isEmpty() const;

    std::string id() const { return m_type; }
};

bool operator==(const AddressInfo& left, const AddressInfo& right);
bool operator!=(const AddressInfo& left, const AddressInfo& right);

//---------------------------------------------------------------------------------------

struct Addresses
{
    std::list<AddressInfo> m_lAddrs;
};

bool operator==(const Addresses& left, const Addresses& right);
bool operator!=(const Addresses& left, const Addresses& right);

//---------------------------------------------------------------------------------------

struct VisaInfo
{
    std::string            m_type;
    std::string            m_country;
    std::string            m_num;
    std::string            m_placeOfIssue;
    boost::gregorian::date m_issueDate;
    boost::gregorian::date m_expiryDate;

    VisaInfo(const std::string& type,
             const std::string& country,
             const std::string& num,
             const std::string& placeOfIssue,
             const boost::gregorian::date& issueDate,
             const boost::gregorian::date& expiryDate);

    bool isEmpty() const;
};

bool operator==(const VisaInfo& left, const VisaInfo& right);
bool operator!=(const VisaInfo& left, const VisaInfo& right);

//---------------------------------------------------------------------------------------

struct Remarks
{
    std::list<Remark> m_lRems;
};

bool operator==(const Remarks& left, const Remarks& right);
bool operator!=(const Remarks& left, const Remarks& right);

//---------------------------------------------------------------------------------------

struct FqtRemarks
{
    std::list<FqtRemark> m_lFqtRems;
};

bool operator==(const FqtRemarks& left, const FqtRemarks& right);
bool operator!=(const FqtRemarks& left, const FqtRemarks& right);

//---------------------------------------------------------------------------------------

struct IatciBag
{
    int  num_of_pieces;
    int  weight;
    bool is_hand;

    IatciBag()
        : num_of_pieces(ASTRA::NoExists),
          weight(ASTRA::NoExists),
          is_hand(false)
    {}
};

//---------------------------------------------------------------------------------------

struct IatciBags
{
   std::list<IatciBag> bags;

   IatciBags()
   {}
   IatciBags(const std::list<IatciBag>& b)
       : bags(b)
   {}
};

//---------------------------------------------------------------------------------------

struct IatciBagTag
{
    std::string carrier_code;
    int         tag_num;
    int         qtty;
    std::string dest;
    int         accode;

    IatciBagTag()
        : tag_num(ASTRA::NoExists),
          qtty(ASTRA::NoExists),
          accode(ASTRA::NoExists)
    {}
};

//---------------------------------------------------------------------------------------

struct IatciBagTags
{
    std::list<IatciBagTag> tags;

    IatciBagTags()
    {}
    IatciBagTags(const std::list<IatciBagTag>& t)
        : tags(t)
    {}
};

//---------------------------------------------------------------------------------------

struct PaxInfo
{
    int                           m_paxId;
    std::string                   m_surname;
    std::string                   m_name;
    ASTRA::TPerson                m_persType;
    std::string                   m_ticketNum;
    unsigned                      m_couponNum;
    std::string                   m_ticketRem;
    std::string                   m_seatNo;
    std::string                   m_regNo;
    std::string                   m_iatciPaxId;
    Ticketing::SubClass           m_subclass;
    boost::optional<DocInfo>      m_doc;
    boost::optional<Addresses>    m_addrs;
    boost::optional<VisaInfo>     m_visa;
    boost::optional<Remarks>      m_rems;
    boost::optional<FqtRemarks>   m_fqtRems;
    boost::optional<IatciBags>    m_iatciBags;
    boost::optional<IatciBagTags> m_iatciBagTags;
    int                           m_bagPoolNum;
    int                           m_iatciParentId;

    PaxInfo(int paxId,
            const std::string& surname,
            const std::string& name,
            ASTRA::TPerson persType,
            const std::string& ticketNum,
            unsigned couponNum,
            const std::string& ticketRem,
            const std::string& seatNo,
            const std::string& regNo,
            const std::string& iatciPaxId,
            const Ticketing::SubClass& subclass,
            const boost::optional<DocInfo>& doc,
            const boost::optional<Addresses>& addrs,
            const boost::optional<VisaInfo>& visa,
            const boost::optional<Remarks>& rems = boost::none,
            const boost::optional<FqtRemarks>& fqtRems = boost::none,
            const boost::optional<IatciBags>& iatciBags = boost::none,
            const boost::optional<IatciBagTags>& iatciBagTags = boost::none,
            int bagPoolNum = 0,
            int iatciParentId = 0);

    int                          id() const { return m_paxId; }
    std::string              seatNo() const { return m_seatNo; }
    bool                   isInfant() const { return m_persType == ASTRA::baby; }
    boost::optional<Remark> ssrInft() const;
    std::string            fullName() const;
    int               iatciParentId() const { return m_iatciParentId; }
    Ticketing::TicketNum_t  tickNum() const;
    std::string             theirId() const;
    std::string               ourId() const;

    int                  bagPoolNum() const { return m_bagPoolNum; }
    void setBagPoolNum(int poolNum) { m_bagPoolNum = poolNum; }

    const boost::optional<IatciBags>& iatciBags() const { return m_iatciBags; }
    void setIatciBags(const boost::optional<IatciBags>& iatciBags) { m_iatciBags = iatciBags; }

    const boost::optional<IatciBagTags>& iatciBagTags() const { return m_iatciBagTags; }
    void setIatciBagTags(const boost::optional<IatciBagTags>& iatciBagTags) { m_iatciBagTags = iatciBagTags; }
};

bool operator==(const PaxInfo& left, const PaxInfo& right);
bool operator!=(const PaxInfo& left, const PaxInfo& right);

//---------------------------------------------------------------------------------------

struct BagPool
{
    int m_poolNum;
    int m_amount;
    int m_weight;

    BagPool(int poolNum, int amount = 0, int weight = 0);

    int poolNum() const { return m_poolNum; }
    int amount()  const { return m_amount;  }
    int weight()  const { return m_weight;  }

    BagPool  operator+ (const BagPool& pool);
    BagPool& operator+=(const BagPool& pool);
};

//---------------------------------------------------------------------------------------

struct BaggageTag
{
    std::string m_carrierCode;
    uint64_t    m_fullTag;
    unsigned    m_numOfConsecSerial;
    std::string m_destination;

    BaggageTag(uint64_t fullTag,
               unsigned numOfConsecSerial,
               const std::string& dest);

    BaggageTag(const std::string& carrierCode,
               uint64_t fullTag,
               unsigned numOfConsecSerial,
               const std::string& dest);

    const std::string&   carrierCode() const { return m_carrierCode;       }
    uint64_t                 fullTag() const { return m_fullTag;           }
    unsigned       numOfConsecSerial() const { return m_numOfConsecSerial; }
    const std::string&   destination() const { return m_destination;       }
};

//---------------------------------------------------------------------------------------

class Baggage
{
private:
    std::multimap<int, BagPool> m_bagPools;
    std::multimap<int, BagPool> m_handBagPools;
    std::set<int> m_poolNums;

public:
    Baggage() {}
    void addPool(const BagPool& p, bool handLuggage);
    void addPool(const BagPool& p);
    void addHandPool(const BagPool& p);
    BagPool totalByPoolNum(int poolNum) const;
    BagPool totalByHandPoolNum(int poolNum) const;
    const std::set<int>& poolNums() const;
};

}//namespace astra_entities

/////////////////////////////////////////////////////////////////////////////////////////

namespace xml_entities {

class PaxFilter;

//

class ReqParams
{
    xmlNodePtr m_rootNode;

public:
    ReqParams(xmlNodePtr rootNode);
    void setBoolParam(const std::string& param, bool val);
    bool getBoolParam(const std::string& param, bool nvl = false);

    void setStrParam(const std::string& param, const std::string& val);
    std::string getStrParam(const std::string& param);
};

//---------------------------------------------------------------------------------------

struct XmlPaxDoc
{
    std::string no;
    std::string type;
    std::string birth_date;
    std::string expiry_date;
    std::string surname;
    std::string first_name;
    std::string second_name;
    std::string nationality;
    std::string gender;
    std::string issue_country;

    astra_entities::DocInfo toDoc() const;
};

//---------------------------------------------------------------------------------------

struct XmlPaxAddress
{
    std::string type;
    std::string country;
    std::string address;
    std::string city;
    std::string region;
    std::string postal_code;

    astra_entities::AddressInfo toAddress() const;
};
bool operator==(const XmlPaxAddress& l, const XmlPaxAddress& r);

//---------------------------------------------------------------------------------------

struct XmlPaxAddresses
{
    std::list<XmlPaxAddress> addresses;

    astra_entities::Addresses toAddresses() const;
};

//---------------------------------------------------------------------------------------

struct XmlPaxVisa
{
    std::string type;
    std::string no;
    std::string issue_place;
    std::string issue_date;
    std::string expiry_date;
    std::string applic_country;

    astra_entities::VisaInfo toVisa() const;
};

//---------------------------------------------------------------------------------------

struct XmlRem
{
    std::string rem_code;
    std::string rem_text;

    astra_entities::Remark toRem() const;
};
bool operator==(const XmlRem& l, const XmlRem& r);
bool operator< (const XmlRem& l, const XmlRem& r);

//---------------------------------------------------------------------------------------

struct XmlFqtRem
{
    std::string rem_code;
    std::string airline;
    std::string no;
    std::string tier_level;

    astra_entities::FqtRemark toFqtRem() const;
};
bool operator==(const XmlFqtRem& l, const XmlFqtRem& r);
bool operator< (const XmlFqtRem& l, const XmlFqtRem& r);

//---------------------------------------------------------------------------------------

struct XmlRems
{
    std::set<XmlRem> rems;

    astra_entities::Remarks toRems() const;
};

//---------------------------------------------------------------------------------------

struct XmlFqtRems
{
    std::set<XmlFqtRem> rems;

    astra_entities::FqtRemarks toFqtRems() const;
};

//---------------------------------------------------------------------------------------

typedef astra_entities::IatciBag XmlIatciBag;
typedef astra_entities::IatciBags XmlIatciBags;
typedef astra_entities::IatciBagTag XmlIatciBagTag;
typedef astra_entities::IatciBagTags XmlIatciBagTags;

//---------------------------------------------------------------------------------------

struct XmlPax
{
    int         pax_id;
    std::string surname;
    std::string name;
    std::string second_name;
    std::string pers_type;
    std::string seat_no;
    std::string seat_type;
    int         seats;
    std::string refuse;
    int         reg_no;
    std::string subclass;
    int         bag_pool_num;
    int         tid;
    std::string ticket_no;
    int         coupon_no;
    std::string ticket_rem;
    int         ticket_confirm;
    int         pr_norec;
    int         pr_bp_print;
    int         grp_id;
    int         cl_grp_id;
    int         hall_id;
    int         point_arv;
    int         user_id;
    std::string airp_arv;
    std::string iatci_pax_id;
    int         iatci_parent_id;
    boost::optional<XmlPaxDoc> doc;
    boost::optional<XmlPaxAddresses> addrs;
    boost::optional<XmlPaxVisa> visa;
    boost::optional<XmlRems> rems;
    boost::optional<XmlFqtRems> fqt_rems;
    boost::optional<XmlIatciBags> iatci_bags;
    boost::optional<XmlIatciBagTags> iatci_bag_tags;

    XmlPax();

    bool equalName(const std::string& surname, const std::string& name) const;

    astra_entities::PaxInfo toPax() const;
};

//---------------------------------------------------------------------------------------

struct XmlPnrRecloc
{
    std::string airline;
    std::string recloc;
};

//---------------------------------------------------------------------------------------

struct XmlTrferSegment
{
    int         num;
    std::string airline;
    std::string flt_no;
    std::string local_date;
    std::string airp_dep;
    std::string airp_arv;
    std::string subclass;
    int         trfer_permit;

    std::string calc_status;

    XmlTrferSegment()
        : num(ASTRA::NoExists),
          trfer_permit(ASTRA::NoExists)
    {}

    void updateCalcStatus(const std::string& calc_status);
};

//---------------------------------------------------------------------------------------

struct XmlPnr
{
    int         pnr_id;
    std::string airp_arv;
    std::string subclass;
    std::string cls;
    std::list<XmlPax> passengers;
    std::list<XmlTrferSegment> trfer_segments;
    std::list<XmlPnrRecloc> pnr_reclocs;

    XmlPnr()
        : pnr_id(ASTRA::NoExists)
    {}


    std::list<XmlPax> filterPaxes(const std::string& surname,
                                  const std::string& name) const;

    std::list<XmlPax> filterPaxes(const PaxFilter& filter) const;
};

//---------------------------------------------------------------------------------------

struct XmlTripHeader
{
    std::string      flight;
    std::string      airline;
    std::string      aircode;
    int              flt_no;
    std::string      suffix;
    std::string      airp;
    BASIC::date_time::TDateTime scd_out_local;
    std::string      scd_brd_to_local;
    std::string      remote_gate;
    int              pr_etl_only;
    int              pr_etstatus;
    int              pr_no_ticket_check;

    XmlTripHeader()
        : flt_no(ASTRA::NoExists),
          scd_out_local(ASTRA::NoExists),
          pr_etl_only(ASTRA::NoExists),
          pr_etstatus(ASTRA::NoExists),
          pr_no_ticket_check(ASTRA::NoExists)
    {}
};

//---------------------------------------------------------------------------------------

struct XmlAirp
{
    int         point_id;
    std::string airp_code;
    std::string city_code;
    std::string target_view;

    XmlAirp()
        : point_id(ASTRA::NoExists)
    {}
};

//---------------------------------------------------------------------------------------

struct XmlClass
{
    std::string code;
    std::string class_view;
    int         cfg;

    XmlClass()
        : cfg(ASTRA::NoExists)
    {}
};

//---------------------------------------------------------------------------------------

struct XmlGate {};

//---------------------------------------------------------------------------------------

struct XmlHall {};

//---------------------------------------------------------------------------------------

struct XmlMarkFlight
{
    std::string      airline;
    int              flt_no;
    std::string      suffix;
    BASIC::date_time::TDateTime scd;
    std::string      airp_dep;
    int              pr_mark_norms;

    XmlMarkFlight()
        : flt_no(ASTRA::NoExists),
          scd(ASTRA::NoExists),
          pr_mark_norms(ASTRA::NoExists)
    {}

    astra_entities::MarketingInfo toMarkFlight() const;
};

//---------------------------------------------------------------------------------------

struct XmlTripData
{
    std::list<XmlAirp>  airps;
    std::list<XmlClass> classes;
    std::list<XmlGate>  gates;
    std::list<XmlHall>  halls;
    std::list<XmlMarkFlight> mark_flights;
};

//---------------------------------------------------------------------------------------

struct XmlTripCounterItem
{
    int           point_arv;
    std::string   cls;
    int           noshow;
    int           trnoshow;
    int           show;
    int           jmp_show;
    int           jmp_nooccupy;

    XmlTripCounterItem()
        : point_arv(ASTRA::NoExists),
          noshow(ASTRA::NoExists),
          trnoshow(ASTRA::NoExists),
          show(ASTRA::NoExists),
          jmp_show(ASTRA::NoExists),
          jmp_nooccupy(ASTRA::NoExists)
    {}
};

//---------------------------------------------------------------------------------------

struct XmlSegmentInfo
{
    int           grp_id;
    int           point_dep;
    std::string   airp_dep;
    int           point_arv;
    std::string   airp_arv;
    std::string   cls;
    std::string   status;
    std::string   bag_refuse;
    int           tid;
    std::string   city_arv;
    std::string   airline;

    XmlSegmentInfo()
        : grp_id(ASTRA::NoExists),
          point_dep(ASTRA::NoExists),
          point_arv(ASTRA::NoExists),
          tid(ASTRA::NoExists)
    {}
};

//---------------------------------------------------------------------------------------

struct XmlHostOrigin
{
    std::string origAirline;
    std::string origLocation;
};

//---------------------------------------------------------------------------------------

struct XmlHostDetails
{
    boost::optional<XmlHostOrigin> hostOrigin;
    std::list<std::string>         hostAirlines;
    boost::optional<unsigned>      maxRespFlights;
};

//---------------------------------------------------------------------------------------

struct XmlSegment
{
    XmlTripHeader  trip_header;
    XmlTripData    trip_data;
    XmlSegmentInfo seg_info;
    XmlMarkFlight  mark_flight;
    std::list<XmlTripCounterItem> trip_counters;
    std::list<XmlPax> passengers;
    boost::optional<XmlHostDetails> host_details;

    XmlSegment()
    {}

    astra_entities::SegmentInfo toSeg() const;

    std::list<XmlPax> findPaxesByName(const std::string& surname,
                                      const std::string& name) const;

    boost::optional<XmlPax> findPaxById(int paxId) const;
    boost::optional<XmlPax> findPaxByName(const std::string& surname,
                                          const std::string& name) const;

    XmlPax firstNonInfant() const;

    bool isIatci() const;

    std::string toKeyString() const;
};

//---------------------------------------------------------------------------------------

struct XmlBag
{
    std::string bag_type;
    std::string airline;
    int         id;
    int         num;
    bool        pr_cabin;
    int         amount;
    int         weight;
    int         bag_pool_num;
    std::string airp_arv_final;

    // ��� ���
    int         pax_id;

    XmlBag()
        : id(ASTRA::NoExists),
          num(ASTRA::NoExists),
          pr_cabin(false),
          amount(ASTRA::NoExists),
          weight(ASTRA::NoExists),
          bag_pool_num(ASTRA::NoExists),
          pax_id(ASTRA::NoExists)
    {}
};

std::ostream& operator<<(std::ostream& os, const XmlBag& bag);

//---------------------------------------------------------------------------------------

struct XmlBags
{
    std::list<XmlBag> bags;

    XmlBags()
    {}
    XmlBags(const std::list<XmlBag>& b)
        : bags(b)
    {}

    bool empty() const { return bags.empty(); }
    bool haveNotCabinBags() const;

    boost::optional<XmlBag> findBag(int paxId, int prCabin) const;

    size_t totalAmount() const;
};

//---------------------------------------------------------------------------------------

struct XmlBagTag
{
    int         num;
    std::string tag_type;
    uint64_t    no;
    int         bag_num;
    bool        pr_print;

    // ��� ���
    int         pax_id;

    XmlBagTag()
        : num(ASTRA::NoExists),
          no(ASTRA::NoExists),
          bag_num(ASTRA::NoExists),
          pr_print(false),
          pax_id(ASTRA::NoExists)
    {}
};

std::ostream& operator<<(std::ostream& os, const XmlBagTag& tag);

//---------------------------------------------------------------------------------------

struct XmlBagTags
{
    std::list<XmlBagTag> bagTags;

    XmlBagTags()
    {}
    XmlBagTags(const std::list<XmlBagTag>& bt)
        : bagTags(bt)
    {}

    bool empty() const { return bagTags.empty(); }
    bool containsTagForPax(int paxId) const;
};

//---------------------------------------------------------------------------------------

struct XmlTrip
{
    int         point_id;
    std::string name;
    std::string airline;
    int         flt_no;
    std::string scd;
    std::string airp_dep;
    std::string status;
    std::list<XmlPnr> pnrs;

    XmlTrip()
        : point_id(ASTRA::NoExists),
          flt_no(ASTRA::NoExists)
    {}

    // ���� ����� ࠡ���� ⮫쪮 � ����� Pnr
    XmlPnr& pnr();
    const XmlPnr& pnr() const;

    std::list<XmlPnr> filterPnrs(const std::string& surname,
                                 const std::string& name) const;

    std::list<XmlPnr> filterPnrs(const PaxFilter& filter) const;
};

//---------------------------------------------------------------------------------------

struct XmlFilterRouteItem
{
    int point_id;
    std::string airp;

    XmlFilterRouteItem()
        : point_id(ASTRA::NoExists)
    {}
};

//---------------------------------------------------------------------------------------

struct XmlServiceList
{
    int seg_no;
    int category;
    int list_id;

    XmlServiceList()
        : seg_no(ASTRA::NoExists),
          category(ASTRA::NoExists),
          list_id(ASTRA::NoExists)
    {}

    XmlServiceList(int segNo, int cat, int listId)
        : seg_no(segNo),
          category(cat),
          list_id(listId)
    {}
};

//---------------------------------------------------------------------------------------

struct XmlFilterRoutes
{
    int point_dep;
    int point_arv;
    std::list<XmlFilterRouteItem> items;

    XmlFilterRoutes() :
        point_dep(ASTRA::NoExists),
        point_arv(ASTRA::NoExists)
    {}

    XmlFilterRouteItem depItem() const;
    XmlFilterRouteItem arrItem() const;

protected:
    boost::optional<XmlFilterRouteItem> findItem(int pointId) const;
};

//---------------------------------------------------------------------------------------

struct XmlPlaceLayer
{
    std::string layer_type;
};

//---------------------------------------------------------------------------------------

struct XmlPlace
{
    int         x;
    int         y;
    std::string elem_type;
    std::string cls;
    std::string xname;
    std::string yname;
    std::list<XmlPlaceLayer> layers;

    XmlPlace()
        : x(ASTRA::NoExists),
          y(ASTRA::NoExists)
    {}
};

//---------------------------------------------------------------------------------------

struct XmlPlaceList
{
    int num;
    int xcount;
    int ycount;

    XmlFilterRoutes filterRoutes;
    std::list<XmlPlace> places;

    XmlPlaceList()
        : num(ASTRA::NoExists),
          xcount(ASTRA::NoExists),
          ycount(ASTRA::NoExists)
    {}

    std::vector<XmlPlace> yPlaces(int y) const;
    XmlPlace minYPlace() const;
    XmlPlace maxYPlace() const;
    boost::optional<XmlPlace> findPlace(int y, const std::string& xname) const;
};

//---------------------------------------------------------------------------------------

struct GetSeatmapXmlResult
{
    std::string trip;
    std::string craft;
    XmlFilterRoutes filterRoutes;

    std::list<XmlPlaceList> lPlacelist;

    iatci::dcrcka::Result toIatci(const iatci::FlightDetails& outbFlt) const;

    GetSeatmapXmlResult(xmlNodePtr node);
};

//---------------------------------------------------------------------------------------

struct XmlRouteSegment
{
    std::string trfer_permit;
    std::string tckin_permit;
    std::string flight;
    std::string classes;
    std::string total;
    std::string calc_status;
};

//---------------------------------------------------------------------------------------

struct XmlTCkinSegment
{
    XmlTripHeader      trip_header;
    XmlTripData        trip_data;
    XmlSegmentInfo     seg_info;
    std::list<XmlTrip> trips;
};

//---------------------------------------------------------------------------------------

class XmlCheckInTab
{
    XmlSegment m_seg;

public:
    XmlCheckInTab(xmlNodePtr tabNode);

    bool isEdi() const;
    const XmlSegment& xmlSeg() const;
    astra_entities::SegmentInfo seg() const;
    std::list<astra_entities::PaxInfo> lPax() const;
    boost::optional<astra_entities::PaxInfo> getPaxById(int paxId) const;
};

//---------------------------------------------------------------------------------------

class XmlCheckInTabs
{
    std::vector<XmlCheckInTab> m_tabs;

public:
    XmlCheckInTabs(xmlNodePtr tabsNode);
    size_t size() const;
    bool empty() const;
    bool containsEdiTab() const;

    const std::vector<XmlCheckInTab>& tabs() const;
    std::vector<XmlCheckInTab> ediTabs() const;
};

//---------------------------------------------------------------------------------------

class XmlEntityReader
{
public:
    static XmlTripHeader                 readTripHeader(xmlNodePtr tripHeaderNode);
    static XmlTripData                   readTripData(xmlNodePtr tripDataNode);

    static XmlAirp                       readAirp(xmlNodePtr airpNode);
    static std::list<XmlAirp>            readAirps(xmlNodePtr airpsNode);

    static XmlClass                      readClass(xmlNodePtr classNode);
    static std::list<XmlClass>           readClasses(xmlNodePtr classesNode);

    static XmlGate                       readGate(xmlNodePtr gateNode);
    static std::list<XmlGate>            readGates(xmlNodePtr gatesNode);

    static XmlHall                       readHall(xmlNodePtr hallNode);
    static std::list<XmlHall>            readHalls(xmlNodePtr hallsNode);

    static XmlMarkFlight                 readMarkFlight(xmlNodePtr flightNode);
    static std::list<XmlMarkFlight>      readMarkFlights(xmlNodePtr flightsNode);

    static XmlTripCounterItem            readTripCounterItem(xmlNodePtr itemNode);
    static std::list<XmlTripCounterItem> readTripCounterItems(xmlNodePtr tripCountersNode);

    static XmlRem                        readRem(xmlNodePtr remNode);
    static XmlRems                       readRems(xmlNodePtr remsNode);

    static XmlFqtRem                     readFqtRem(xmlNodePtr remNode);
    static XmlFqtRems                    readFqtRems(xmlNodePtr remsNode);

    static XmlPaxDoc                     readDoc(xmlNodePtr docNode);

    static XmlPaxAddress                 readAddress(xmlNodePtr addrNode);
    static XmlPaxAddresses               readAddresses(xmlNodePtr addrsNode);

    static XmlPaxVisa                    readVisa(xmlNodePtr visaNode);

    static XmlPax                        readPax(xmlNodePtr paxNode);
    static std::list<XmlPax>             readPaxes(xmlNodePtr paxesNode);

    static XmlPnrRecloc                  readPnrRecloc(xmlNodePtr reclocNode);
    static std::list<XmlPnrRecloc>       readPnrReclocs(xmlNodePtr reclocsNode);

    static XmlTrferSegment               readTrferSeg(xmlNodePtr trferSegNode);
    static std::list<XmlTrferSegment>    readTrferSegs(xmlNodePtr trferSegsNode);

    static XmlPnr                        readPnr(xmlNodePtr pnrNode);
    static std::list<XmlPnr>             readPnrs(xmlNodePtr pnrsNode);

    static XmlTrip                       readTrip(xmlNodePtr tripNode);
    static std::list<XmlTrip>            readTrips(xmlNodePtr tripsNode);

    static XmlServiceList                readServiceList(xmlNodePtr svcListNode);
    static std::list<XmlServiceList>     readServiceLists(xmlNodePtr svcListsNode);

    static XmlSegmentInfo                readSegInfo(xmlNodePtr segNode);

    static XmlSegment                    readSeg(xmlNodePtr segNode);
    static std::list<XmlSegment>         readSegs(xmlNodePtr segsNode);

    static XmlBag                        readBag(xmlNodePtr bagNode);
    static std::list<XmlBag>             readBags(xmlNodePtr bagsNode);

    static XmlIatciBag                   readIatciBag(xmlNodePtr iatciBagNode);
    static std::list<XmlIatciBag>        readIatciBags(xmlNodePtr iatciBagsNode);

    static XmlBagTag                     readBagTag(xmlNodePtr bagTagNode);
    static std::list<XmlBagTag>          readBagTags(xmlNodePtr bagTagsNode);

    static XmlIatciBagTag                readIatciBagTag(xmlNodePtr iatciBagTagNode);
    static std::list<XmlIatciBagTag>     readIatciBagTags(xmlNodePtr iatciBagTagsNode);

    static XmlPlaceLayer                 readPlaceLayer(xmlNodePtr layerNode);
    static std::list<XmlPlaceLayer>      readPlaceLayers(xmlNodePtr layersNode);

    static XmlPlace                      readPlace(xmlNodePtr placeNode);
    static std::list<XmlPlace>           readPlaces(xmlNodePtr placesNode);

    static XmlPlaceList                  readPlaceList(xmlNodePtr placelistNode);
    static std::list<XmlPlaceList>       readPlaceLists(xmlNodePtr salonsNode);

    static XmlFilterRouteItem            readFilterRouteItem(xmlNodePtr itemNode);
    static std::list<XmlFilterRouteItem> readFilterRouteItems(xmlNodePtr itemsNode);

    static XmlFilterRoutes               readFilterRoutes(xmlNodePtr filterRoutesNode);

    static XmlRouteSegment               readRouteSegment(xmlNodePtr routeNode);
    static std::list<XmlRouteSegment>    readRouteSegments(xmlNodePtr routesNode);

    static XmlTCkinSegment               readTCkinSegment(xmlNodePtr segNode);
    static std::list<XmlTCkinSegment>    readTCkinSegments(xmlNodePtr segsNode);

    static XmlHostOrigin                 readHostOrigin(xmlNodePtr hoNode);
    static XmlHostDetails                readHostDetails(xmlNodePtr hdNode);
};

//---------------------------------------------------------------------------------------

class XmlEntityViewer
{
public:
    static xmlNodePtr viewMarkFlight(xmlNodePtr node, const XmlMarkFlight& markFlight);

    static xmlNodePtr viewRem(xmlNodePtr node, const XmlRem& rem);
    static xmlNodePtr viewRems(xmlNodePtr node, const XmlRems& rems);

    static xmlNodePtr viewFqtRem(xmlNodePtr node, const XmlFqtRem& rem);
    static xmlNodePtr viewFqtRems(xmlNodePtr node, const boost::optional<XmlFqtRems>& rems);

    static xmlNodePtr viewDoc(xmlNodePtr node, const XmlPaxDoc& doc);

    static xmlNodePtr viewAddress(xmlNodePtr node, const XmlPaxAddress& addr);
    static xmlNodePtr viewAddresses(xmlNodePtr node, const XmlPaxAddresses& addrs);

    static xmlNodePtr viewVisa(xmlNodePtr node, const XmlPaxVisa& visa);

    static xmlNodePtr viewPax(xmlNodePtr node, const XmlPax& pax);
    static xmlNodePtr viewTrferPax(xmlNodePtr node, const XmlPax& pax);

    static xmlNodePtr viewSegInfo(xmlNodePtr node, const XmlSegmentInfo& segInfo);

    static xmlNodePtr viewSeg(xmlNodePtr node, const XmlSegment& seg);
    static xmlNodePtr viewTrferSeg(xmlNodePtr node, const XmlTrferSegment& seg);

    static xmlNodePtr viewBag(xmlNodePtr node, const XmlBag& bag);
    static xmlNodePtr viewBag(xmlNodePtr node, const XmlIatciBag& bag);
    static xmlNodePtr viewBagsHeader(xmlNodePtr node);
    static xmlNodePtr viewBags(xmlNodePtr node, const XmlBags& bags);
    static xmlNodePtr viewBags(xmlNodePtr node, const XmlIatciBags& bags);

    static xmlNodePtr viewBagTag(xmlNodePtr node, const XmlBagTag& tag);
    static xmlNodePtr viewBagTag(xmlNodePtr node, const XmlIatciBagTag& tag);
    static xmlNodePtr viewBagTagsHeader(xmlNodePtr node);
    static xmlNodePtr viewBagTags(xmlNodePtr node, const XmlBagTags& tags);
    static xmlNodePtr viewBagTags(xmlNodePtr node, const XmlIatciBagTags& tags);
    static xmlNodePtr viewValueBagsHeader(xmlNodePtr node);

    static xmlNodePtr viewServiceList(xmlNodePtr node, const XmlServiceList& svcList);

    static xmlNodePtr viewHostOrigin(xmlNodePtr node, const XmlHostOrigin& hostOrigin);
    static xmlNodePtr viewHostDetails(xmlNodePtr node, const XmlHostDetails& hostDetails);
};

//---------------------------------------------------------------------------------------

struct SearchPaxXmlResult
{
    std::list<XmlTrip> lTrip;

    std::list<XmlTrip> filterTrips(const std::string& surname,
                                   const std::string& name) const;

    std::list<XmlTrip> filterTrips(const PaxFilter& filter) const;


    SearchPaxXmlResult(xmlNodePtr node);
};

//---------------------------------------------------------------------------------------

struct LoadPaxXmlResult
{
    std::list<XmlSegment> lSeg;

    std::list<XmlBag>     lBag;
    std::list<XmlBagTag>  lBagTag;

    std::vector<iatci::dcrcka::Result> toIatci(iatci::dcrcka::Result::Action_e action) const;

    iatci::dcrcka::Result toIatciFirst(iatci::dcrcka::Result::Action_e action) const;


    void applyPaxFilter(const PaxFilter& filters);

    LoadPaxXmlResult(xmlNodePtr node);
    LoadPaxXmlResult(const std::list<XmlSegment>& lSeg,
                     const std::list<XmlBag>& lBag = std::list<XmlBag>(),
                     const std::list<XmlBagTag>& lBagTag = std::list<XmlBagTag>());

private:
    void finalizeBags();
    void finalizeBagTags();
};

//---------------------------------------------------------------------------------------

struct PaxListXmlResult
{
    std::list<XmlPax> lPax;

    std::list<XmlPax> applyNameFilter(const std::string& surname,
                                      const std::string& name) const;

    std::list<XmlPax> applyFilters(const PaxFilter& filters) const;

    PaxListXmlResult(xmlNodePtr node);
};

//---------------------------------------------------------------------------------------

struct GetAdvTripListXmlResult
{
    std::list<XmlTrip> lTrip;

    std::list<XmlTrip> applyFlightFilter(const std::string& flightName) const;

    GetAdvTripListXmlResult(xmlNodePtr node);
};

//---------------------------------------------------------------------------------------

struct CheckTCkinRoute1XmlResult
{
    std::list<XmlRouteSegment> lRouteSeg;

    CheckTCkinRoute1XmlResult(xmlNodePtr node);
};

//---------------------------------------------------------------------------------------

struct CheckTCkinRoute2XmlResult
{
    std::list<XmlTCkinSegment> lTCkinSeg;

    CheckTCkinRoute2XmlResult(xmlNodePtr node);
};

//---------------------------------------------------------------------------------------

struct NameFilter
{
    std::string m_surname;
    std::string m_name;

    NameFilter(const std::string& surname, const std::string& name)
        : m_surname(surname), m_name(name)
    {}

    inline bool operator()(const XmlPax& pax) const
    {
        return pax.equalName(m_surname, m_name);
    }
};

//---------------------------------------------------------------------------------------

struct TicknumFilter
{
    Ticketing::TicketNum_t m_ticknum;

    TicknumFilter(const Ticketing::TicketNum_t& ticknum)
        : m_ticknum(ticknum)
    {}

    inline bool operator()(const XmlPax& pax) const
    {
        return pax.ticket_no == m_ticknum.get();
    }
};

//---------------------------------------------------------------------------------------

struct IdFilter
{
    int m_id;

    IdFilter(int id)
        : m_id(id)
    {}

    inline bool operator()(const XmlPax& pax) const
    {
        return pax.pax_id == m_id;
    }
};

//---------------------------------------------------------------------------------------

struct PaxFilter
{
    boost::optional<NameFilter>    m_nameFilter;
    boost::optional<TicknumFilter> m_ticknumFilter;
    boost::optional<IdFilter>      m_idFilter;

    PaxFilter(const boost::optional<NameFilter>& nameFilter,
              const boost::optional<TicknumFilter>& ticknumFilter,
              const boost::optional<IdFilter>& idFilter);

    bool operator()(const XmlPax& pax) const;
    bool operator()(const XmlPnr& pnr) const;
    bool operator()(const XmlTrip& trip) const;
};


}//namespace xml_entities

/////////////////////////////////////////////////////////////////////////////////////////

class AstraEngine
{
private:
    mutable XMLDoc m_reqDoc;
    mutable XMLDoc m_resDoc;
    int            m_userId;

protected:
    int             getUserId() const;
    XMLRequestCtxt* getRequestCtxt() const;
    xmlNodePtr      getQueryNode() const;
    xmlNodePtr      getAnswerNode() const;

    void initReqInfo(const std::string& deskVersion = ASTRA_API_VERSION) const;

    AstraEngine();

    void CheckTCkinRoute(xmlNodePtr reqNode, xmlNodePtr resNode,
                         const PointId_t& pointDep,
                         const xml_entities::XmlTrip& paxTrip);

public:
    static AstraEngine& singletone();

    // ��ᬮ�� ᯨ᪠ ��ॣ����஢����� ���ᠦ�஢ �� ३�
    xml_entities::PaxListXmlResult PaxList(const PointId_t& depPointId);

    // ���� ��ॣ����஢������ ���ᠦ�� �� ॣ����樮����� ������
    xml_entities::LoadPaxXmlResult LoadPax(const PointId_t& depPointId,
                                           const RegNo_t& paxRegNo);

    xml_entities::LoadPaxXmlResult LoadGrp(const PointId_t& depPointId,
                                           const GrpId_t& grpId);

    // ���� ����ॣ����஢������ ���ᠦ�� �� ३�
    xml_entities::SearchPaxXmlResult SearchCheckInPax(const PointId_t& depPointId,
                                                      const Surname_t& paxSurname,
                                                      const Name_t& paxName);

    // �஢�ઠ ���������� ᪢����� ॣ����樨
    xml_entities::CheckTCkinRoute1XmlResult CheckTCkinRoute1(const PointId_t& pointDep,
                                                             const xml_entities::XmlTrip& paxTrip);

    xml_entities::CheckTCkinRoute2XmlResult CheckTCkinRoute2(const PointId_t& pointDep,
                                                             const xml_entities::XmlTrip& paxTrip);

    // ��࠭���� ���ଠ樨 � ���ᠦ��(��)
    xml_entities::LoadPaxXmlResult CheckinPax(const xml_entities::XmlSegment& paxSeg,
                                              boost::optional<xml_entities::XmlSegment> trferSeg);

    xml_entities::LoadPaxXmlResult UpdatePax(const xml_entities::XmlSegment& paxSeg,
                                             const std::list<xml_entities::XmlSegment>& trferSegs,
                                             boost::optional<xml_entities::XmlBags> bags,
                                             boost::optional<xml_entities::XmlBagTags> tags);

    xml_entities::LoadPaxXmlResult CancelPax(const xml_entities::XmlSegment& paxSeg,
                                             const std::list<xml_entities::XmlSegment>& trferSegs);

    xml_entities::LoadPaxXmlResult SavePax(xmlNodePtr reqNode, xmlNodePtr ediResNode);

    // ��������� ����
    void ReseatPax(const PointId_t& pointDep,
                   const xml_entities::XmlPax& pax,
                   boost::optional<xml_entities::XmlHostDetails> hostDetails);

    xml_entities::LoadPaxXmlResult Reseat(const xml_entities::XmlSegment& paxSeg);

    // ���७�� ���� ३� �� ����
    xml_entities::GetAdvTripListXmlResult GetAdvTripList(const boost::gregorian::date& depDate);

    // ��ᬮ�� ����� ���� ३�
    xml_entities::GetSeatmapXmlResult GetSeatmap(const PointId_t& depPointId);
};

//---------------------------------------------------------------------------------------

/**
 * ���� Id �뫥⭮�� �����
 * @return Id ��� tick_soft_except
*/
PointId_t findDepPointId(const std::string& depPort,
                         const std::string& airline,
                         unsigned flNum,
                         const boost::gregorian::date& depDate);

PointId_t findDepPointId(const std::string& depPort,
                         const std::string& airline,
                         const Ticketing::FlightNum_t& flNum,
                         const boost::gregorian::date& depDate);

/**
 * ���� Id �ਫ�⭮�� �����
 * @return Id ��� 0(�᫨ �� ������)
*/
PointId_t findArvPointId(const PointId_t& pointDep,
                         const std::string& arvPort);

/**
 * ���� grp_id ��� �뫥⭮�� ����� � ॣ����樮����� ����� ���ᠦ��
 * @return GrpId ��� 0(�᫨ �� ������)
*/
GrpId_t findGrpIdByRegNo(const PointId_t& pointDep,
                         const RegNo_t& regNo);

/**
 * ���� grp_id ��� �뫥⭮�� ����� � �����䨪��� ���ᠦ��
 * @return GrpId ��� 0(�᫨ �� ������)
*/
GrpId_t findGrpIdByPaxId(const PointId_t& pointDep,
                         const PaxId_t& paxId);

//---------------------------------------------------------------------------------------

struct IatciCheckinResult
{
    GrpId_t               m_grpId;
    iatci::dcrcka::Result m_iatciResult;
};

//---------------------------------------------------------------------------------------

// ��ࢨ筠� ॣ������
IatciCheckinResult checkinIatciPaxes(xmlNodePtr reqNode, xmlNodePtr ediResNode);
iatci::dcrcka::Result checkinIatciPaxes(const iatci::CkiParams& ckiParams);

// ���������� ॣ����樮���� ������
IatciCheckinResult updateIatciPaxes(xmlNodePtr reqNode, xmlNodePtr ediResNode);
iatci::dcrcka::Result updateIatciPaxes(const iatci::CkuParams& ckuParams);

// �⬥�� ॣ����樨
IatciCheckinResult cancelCheckinIatciPax(xmlNodePtr reqNode, xmlNodePtr ediResNode);
iatci::dcrcka::Result cancelCheckinIatciPaxes(const iatci::CkxParams& ckxParams);

// ���ଠ�� �� ���ᠦ���
iatci::dcrcka::Result fillPaxList(const iatci::PlfParams& plfParams);

// ���� ����
iatci::dcrcka::Result fillSeatmap(const iatci::SmfParams& smfParams);

// ����� ��ᠤ�筮�� ⠫���
iatci::dcrcka::Result printBoardingPass(const iatci::BprParams& bprParams);

}//namespace astra_api
