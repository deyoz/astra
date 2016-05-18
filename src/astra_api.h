#pragma once

#include "astra_consts.h"
#include "iatci_types.h"
#include "xml_unit.h"

#include <serverlib/xmllibcpp.h>

#include <list>
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
    int                            m_grpId;
    int                            m_pointDep;
    int                            m_pointArv;
    std::string                    m_airpDep;
    std::string                    m_airpArv;
    std::string                    m_cls;
    boost::optional<MarketingInfo> m_markFlight;

    SegmentInfo(int grpId,
                int pointDep,
                int pointArv,
                const std::string& airpDep,
                const std::string& airpArv,
                const std::string& cls,
                const boost::optional<MarketingInfo>& markFlight);
};

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
};

//---------------------------------------------------------------------------------------

struct PaxInfo
{
    int                      m_paxId;
    std::string              m_surname;
    std::string              m_name;
    ASTRA::TPerson           m_persType;
    std::string              m_ticketNum;
    unsigned                 m_couponNum;
    std::string              m_ticketRem;
    boost::optional<DocInfo> m_doc;

    PaxInfo(int paxId,
            const std::string& surname,
            const std::string& name,
            ASTRA::TPerson persType,
            const std::string& ticketNum,
            unsigned couponNum,
            const std::string& ticketRem,
            const boost::optional<DocInfo>& doc);
};

}//namespace astra_entities

/////////////////////////////////////////////////////////////////////////////////////////

namespace xml_entities {

class ReqParams
{
    xmlNodePtr m_rootNode;

public:
    ReqParams(xmlNodePtr rootNode);
    void setBoolParam(const std::string& param, bool val);
    bool getBoolParam(const std::string& param, bool nvl = false);
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
    std::string issueCountry;

    astra_entities::DocInfo toDoc() const;
};

//---------------------------------------------------------------------------------------

struct XmlRem
{
    std::string rem_code;
    std::string rem_text;
};

//---------------------------------------------------------------------------------------

struct XmlPax
{
    int         pax_id;
    std::string surname;
    std::string name;
    std::string pers_type;
    std::string seat_no;
    std::string seat_type;
    int         seats;
    boost::optional<std::string> refuse;
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
    boost::optional<XmlPaxDoc> doc;
    std::list<XmlRem> rems;


    XmlPax();

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

    XmlTrferSegment()
        : num(ASTRA::NoExists),
          trfer_permit(ASTRA::NoExists)
    {}
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

    // пока можем работать только с одним пассажиром
    XmlPax& pax();
    const XmlPax& pax() const;
};

//---------------------------------------------------------------------------------------

struct XmlTripHeader
{
    std::string flight;
    std::string airline;
    std::string aircode;
    std::string flt_no;
    std::string suffix;
    std::string airp;
    std::string scd_out_local;
    int         pr_etl_only;
    int         pr_etstatus;
    int         pr_no_ticket_check;

    XmlTripHeader()
        : pr_etl_only(ASTRA::NoExists),
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
    std::string airline;
    std::string flt_no;
    std::string suffix;
    std::string scd;
    std::string airp_dep;
    int         pr_mark_norms;

    XmlMarkFlight()
        : pr_mark_norms(ASTRA::NoExists)
    {}
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
    int           free_ok;
    int           free_goshow;
    int           nooccupy;

    XmlTripCounterItem()
        : point_arv(ASTRA::NoExists),
          noshow(ASTRA::NoExists),
          trnoshow(ASTRA::NoExists),
          show(ASTRA::NoExists),
          free_ok(ASTRA::NoExists),
          free_goshow(ASTRA::NoExists),
          nooccupy(ASTRA::NoExists)
    {}
};

//---------------------------------------------------------------------------------------

struct XmlSegment
{
    XmlTripHeader trip_header; // TODO optional?
    XmlTripData   trip_data;
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
    XmlMarkFlight mark_flight;
    std::list<XmlTripCounterItem> trip_counters;
    std::list<XmlPax> passengers;

    XmlSegment();

    astra_entities::SegmentInfo toSeg() const;
};

//---------------------------------------------------------------------------------------

struct XmlTrip
{
    int         point_id;
    std::string name;
    std::string airline;
    std::string flt_no;
    std::string scd;
    std::string airp_dep;
    std::string status;
    std::list<XmlPnr> pnrs;

    XmlTrip()
        : point_id(ASTRA::NoExists)
    {}

    // пока можем работать только с одним Pnr
    XmlPnr& pnr();
    const XmlPnr& pnr() const;
};

//---------------------------------------------------------------------------------------

class XmlCheckInTab
{
    XmlSegment m_seg;

public:
    XmlCheckInTab(xmlNodePtr tabNode);

    bool isEdi() const;
    astra_entities::SegmentInfo seg() const;
    std::list<astra_entities::PaxInfo> lPax() const;
};

//---------------------------------------------------------------------------------------

class XmlCheckInTabs
{
    std::list<XmlCheckInTab> m_tabs;

public:
    XmlCheckInTabs(xmlNodePtr tabsNode);
    size_t size() const;
    bool containsEdiTab() const;

    const std::list<XmlCheckInTab>& tabs() const;
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
    static std::list<XmlRem>             readRems(xmlNodePtr remsNode);

    static XmlPaxDoc                     readDoc(xmlNodePtr docNode);

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

    static XmlSegment                    readSeg(xmlNodePtr segNode);
    static std::list<XmlSegment>         readSegs(xmlNodePtr segsNode);
};

//---------------------------------------------------------------------------------------

struct SearchPaxXmlResult
{
    std::list<XmlTrip> lTrip;

    SearchPaxXmlResult(xmlNodePtr node);
};

//---------------------------------------------------------------------------------------

struct LoadPaxXmlResult
{
    std::list<XmlSegment> lSeg;

    iatci::Result toIatci(iatci::Result::Action_e action,
                          iatci::Result::Status_e status,
                          bool afterSavePax) const;

    LoadPaxXmlResult(xmlNodePtr node);
};

//---------------------------------------------------------------------------------------

struct PaxListXmlResult
{
    std::list<XmlPax> lPax;

    std::list<XmlPax> applyNameFilter(const std::string& surname,
                                      const std::string& name);

    PaxListXmlResult(xmlNodePtr node);
};

//---------------------------------------------------------------------------------------

struct GetAdvTripListXmlResult
{
    std::list<XmlTrip> lTrip;

    std::list<XmlTrip> applyFlightFilter(const std::string& flightName);

    GetAdvTripListXmlResult(xmlNodePtr node);
};

}//namespace xml_entities


/////////////////////////////////////////////////////////////////////////////////////////

class AstraEngine
{
private:
    mutable XMLDoc m_reqDoc;
    mutable XMLDoc m_resDoc;

protected:
    XMLRequestCtxt* getRequestCtxt() const;
    xmlNodePtr      getQueryNode() const;
    xmlNodePtr      getAnswerNode() const;

    void initReqInfo() const;

public:
    static AstraEngine& singletone();

    // просмотр списка зарегистрированных пассажиров на рейсе
    xml_entities::PaxListXmlResult PaxList(int depPointId);

    // поиск зарегистрированного пассажира по регистрационному номеру
    xml_entities::LoadPaxXmlResult LoadPax(int depPointId, int paxRegNo);

    // поиск НЕзарегистрированного пассажира на рейсе
    xml_entities::SearchPaxXmlResult SearchPax(int depPointId,
                                               const std::string& paxSurname,
                                               const std::string& paxName,
                                               const std::string& paxStatus);

    // сохранение информации о пассажире
    xml_entities::LoadPaxXmlResult SavePax(int depPointId, const xml_entities::XmlTrip& paxTrip);
    xml_entities::LoadPaxXmlResult SavePax(const xml_entities::XmlSegment& paxSeg);
    xml_entities::LoadPaxXmlResult SavePax(xmlNodePtr reqNode, xmlNodePtr ediResNode);

    // расширенный поиск рейса на дату
    xml_entities::GetAdvTripListXmlResult GetAdvTripList(const boost::gregorian::date& depDate);

};

//-----------------------------------------------------------------------------

/**
 * Найти Id вылетного пойнта
 * @return Id или tick_soft_except
*/
int findDepPointId(const std::string& depPort,
                   const std::string& airline,
                   unsigned flNum,
                   const boost::gregorian::date& depDate);

/**
 * Найти Id прилетного пойнта
 * @return Id или 0(если не найден)
*/
int findArvPointId(int pointDep,
                   const std::string& arvPort);

/**
 * Найти grp_id для вылетного пойнта и регистрационного номера пассажира
 * @return GrpId или 0(если не найден)
*/
int findGrpIdByRegNo(int pointDep, int regNo);

/**
 * Найти grp_id для вылетного пойнта и идентификатора пассажира
 * @return GrpId или 0(если не найден)
*/
int findGrpIdByPaxId(int pointDep, int paxId);

// регистрация
iatci::Result checkinIatciPax(const iatci::CkiParams& ckiParams);
iatci::Result checkinIatciPax(xmlNodePtr reqNode, xmlNodePtr ediResNode);

// отмена регистрации
iatci::Result cancelCheckinIatciPax(const iatci::CkxParams& ckxParams);
iatci::Result cancelCheckinIatciPax(xmlNodePtr reqNode, xmlNodePtr ediResNode);

// информация по пассажиру
iatci::Result fillPaxList(const iatci::PlfParams& plfParams);

}//namespace astra_api
