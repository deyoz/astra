#include "astra_api.h"
#include "astra_msg.h"
#include "astra_misc.h"
#include "astra_utils.h"
#include "date_time.h"
#include "points.h"
#include "checkin.h"
#include "print.h"
#include "tripinfo.h"
#include "salonform.h"
#include "iatci_help.h"

#include <serverlib/cursctl.h>
#include <serverlib/xml_tools.h>
#include <serverlib/xml_stuff.h>
#include <serverlib/dates_io.h>
#include <serverlib/str_utils.h>
#include <serverlib/algo.h>
#include <jxtlib/jxt_cont.h>
#include <jxtlib/jxt_cont_impl.h>
#include <etick/exceptions.h>

#include <boost/lexical_cast.hpp>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace astra_api {

using namespace xml_entities;
using namespace Ticketing;
using namespace Ticketing::TickExceptions;
using namespace BASIC::date_time;

namespace {

TDateTime strToAstraDateTime(const std::string& serverFormatDateTimeString)
{
    TDateTime ret = ASTRA::NoExists;
    if(!serverFormatDateTimeString.empty()) {
        ASSERT(!StrToDateTime(serverFormatDateTimeString.c_str(), ret));
    }
    return ret;
}

TDateTime getDateTimeFromXml(xmlNodePtr node, const std::string& childName, const TDateTime& def)
{
    return NodeAsDateTime(childName.c_str(), node, def);
}

bool getBoolFromXml(xmlNodePtr node, const std::string& childName, bool def)
{
    return NodeAsBoolean(childName.c_str(), node, def);
}

XmlPaxDoc createCheckInDoc(const iatci::DocDetails& doc)
{
    XmlPaxDoc ckiDoc;
    ckiDoc.no         = doc.no();
    ckiDoc.type       = doc.docType();
    if(!doc.birthDate().is_not_a_date()) {
        ckiDoc.birth_date = BASIC::date_time::boostDateToAstraFormatStr(doc.birthDate());
    }
    if(!doc.expiryDate().is_not_a_date()) {
        ckiDoc.expiry_date = BASIC::date_time::boostDateToAstraFormatStr(doc.expiryDate());
    }
    ckiDoc.surname       = doc.surname();
    ckiDoc.first_name    = doc.name();
    ckiDoc.second_name   = doc.secondName();
    ckiDoc.nationality   = doc.nationality();
    ckiDoc.issue_country = doc.issueCountry();
    return ckiDoc;
}

XmlPaxAddresses createCheckInAddrs(const iatci::AddressDetails& addrs)
{
    XmlPaxAddresses ckiAddrs;
    for(const auto& addr: addrs.lAddr()) {
        XmlPaxAddress ckiAddr;
        ckiAddr.type        = addr.type();
        ckiAddr.country     = addr.country();
        ckiAddr.address     = addr.address();
        ckiAddr.city        = addr.city();
        ckiAddr.region      = addr.region();
        ckiAddr.postal_code = addr.postalCode();
        ckiAddrs.addresses.push_back(ckiAddr);
    }
    return ckiAddrs;
}

XmlRem createCheckInRem(const iatci::ServiceDetails::SsrInfo& ssr)
{
    XmlRem rem;
    rem.rem_code = ssr.ssrCode();
    rem.rem_text = ssr.freeText();

    // хак
    if(rem.rem_text.empty()) {
        rem.rem_text = rem.rem_code;
    }

    return rem;
}

XmlFqtRem createCheckInFqtRem(const iatci::ServiceDetails::SsrInfo& ssr)
{
    XmlFqtRem fqtRem;
    fqtRem.rem_code = ssr.ssrCode();
    fqtRem.airline  = ssr.airline();
    fqtRem.no       = ssr.ssrText();
    if(ssr.quantity()) {
        fqtRem.tier_level = std::to_string(ssr.quantity());
    }

    return fqtRem;
}

void addCheckInRems(XmlPax& pax, const iatci::ServiceDetails& service)
{
    for(const auto& ssr: service.lSsr()) {
        if(ssr.isTkne()) continue;

        if(ssr.isFqt()) {
            if(!pax.fqt_rems)
                pax.fqt_rems = XmlFqtRems();
            pax.fqt_rems->rems.push_back(createCheckInFqtRem(ssr));
        } else {
            if(!pax.rems)
                pax.rems = XmlRems();
            pax.rems->rems.push_back(createCheckInRem(ssr));
        }
    }
}

XmlPax createCheckInPax(const XmlPax& basePax,
                        const iatci::PaxDetails& pax,
                        const boost::optional<iatci::SeatDetails>& seat,
                        const boost::optional<iatci::ReservationDetails>& reserv,
                        const boost::optional<iatci::ServiceDetails>& service,
                        const boost::optional<iatci::DocDetails>& doc,
                        const boost::optional<iatci::AddressDetails>& addrs)
{
    XmlPax ckiPax = basePax;
    ckiPax.seats     = pax.isInfant() ? 0 : 1;
    ckiPax.pers_type = EncodePerson(iatci::iatci2astra(pax.type()));
    ckiPax.seat_no   = seat ? seat->seat() : "";
    ckiPax.subclass  = reserv ? reserv->subclass()->code(RUSSIAN) : "";
    if(doc) {
        ckiPax.doc = createCheckInDoc(*doc);
    }
    if(addrs) {
        ckiPax.addrs = createCheckInAddrs(*addrs);
    }
    if(service) {
        addCheckInRems(ckiPax, *service);
    }
    return ckiPax;
}

XmlPax createCancelPax(const XmlPax& basePax)
{
    XmlPax ckxPax = basePax;
    ckxPax.refuse = ASTRA::refuseAgentError; // TODO пока причина отмены всегда "А - Ошибка агента"
    return ckxPax;
}

XmlTripHeader makeTripHeader(const XmlTrip& trip)
{
    XmlTripHeader th;
    th.airline       = trip.airline;
    th.flt_no        = trip.flt_no;
    th.scd_out_local = strToAstraDateTime(trip.scd);
    th.airp          = trip.airp_dep;
    return th;
}

XmlMarkFlight makeMarkFlight(const XmlTrip& trip)
{
    XmlMarkFlight mf;
    mf.airline       = trip.airline;
    mf.flt_no        = trip.flt_no;
    mf.scd           = strToAstraDateTime(trip.scd);
    mf.airp_dep      = trip.airp_dep;
    mf.pr_mark_norms = 0;
    return mf;
}

XmlSegment makeSegment(const XmlSegment& baseSeg)
{
    XmlSegment seg = baseSeg;
    seg.passengers.clear();
    return seg;
}

//---------------------------------------------------------------------------------------

class DeskCodeReplacement
{
private:
    std::string m_oldDescCode;

public:
    DeskCodeReplacement(const std::string& descCode)
    {
        m_oldDescCode = TReqInfo::Instance()->desk.code;
        TReqInfo::Instance()->desk.code = descCode;
    }

    ~DeskCodeReplacement()
    {
        TReqInfo::Instance()->desk.code = m_oldDescCode;
    }
};

//---------------------------------------------------------------------------------------

void savePrintBP(int grpId, const std::list<XmlPax>& paxes)
{
    ASSERT(!paxes.empty());
    DeskCodeReplacement dcr("IATCIP");
    for(const auto& pax: paxes)
    {
        LogTrace(TRACE3) << __FUNCTION__
                         << " grp_id:" << grpId
                         << " pax_id:" << pax.pax_id;


        PrintDataParser parser(ASTRA::TDevOper::PrnBP, grpId, pax.pax_id, 0, NULL);
        parser.pts.confirm_print(true, ASTRA::TDevOper::PrnBP);
    }
}

bool StatusOfAllTicketsChanged(xmlNodePtr ediResNode)
{
    xmlNodePtr ticketsNode = findNodeR(ediResNode, "tickets");
    for(xmlNodePtr ticketNode = ticketsNode->children;
        ticketNode != NULL; ticketNode = ticketNode->next)
    {
        if(!findNode(ticketNode, "coupon_status")) {
            tst();
            return false;
        }
    }

    tst();
    return true;
}

}//namespace

/////////////////////////////////////////////////////////////////////////////////////////

AstraEngine& AstraEngine::singletone()
{
    static AstraEngine inst;
    return inst;
}

PaxListXmlResult AstraEngine::PaxList(int pointId)
{
    LogTrace(TRACE3) << __FUNCTION__ << " by point_id=" << pointId;
    xmlNodePtr reqNode = getQueryNode(),
               resNode = getAnswerNode();

    xmlNodePtr paxListNode = NewTextChild(reqNode, "PaxList");
    NewTextChild(paxListNode, "point_id", pointId);

    initReqInfo();

    LogTrace(TRACE3) << "pax list query:\n" << XMLTreeToText(reqNode->doc);
    CheckInInterface::instance()->PaxList(getRequestCtxt(), paxListNode, resNode);
    LogTrace(TRACE3) << "pax list answer:\n" << XMLTreeToText(resNode->doc);
    return PaxListXmlResult(resNode);
}

LoadPaxXmlResult AstraEngine::LoadPax(int pointId, int paxRegNo)
{
    LogTrace(TRACE3) << __FUNCTION__ << " by point_id=" << pointId
                                     << " and reg_no=" << paxRegNo;
    xmlNodePtr reqNode = getQueryNode(),
               resNode = getAnswerNode();

    xmlNodePtr loadPaxNode = NewTextChild(reqNode, "TCkinLoadPax");
    NewTextChild(loadPaxNode, "point_id", pointId);
    NewTextChild(loadPaxNode, "reg_no", paxRegNo);

    initReqInfo();

    LogTrace(TRACE3) << "load pax query:\n" << XMLTreeToText(reqNode->doc);
    CheckInInterface::instance()->LoadPax(getRequestCtxt(), loadPaxNode, resNode);
    LogTrace(TRACE3) << "load pax answer:\n" << XMLTreeToText(resNode->doc);
    return LoadPaxXmlResult(resNode);
}

xml_entities::LoadPaxXmlResult AstraEngine::LoadGrp(int pointId, int grpId)
{
    LogTrace(TRACE3) << __FUNCTION__ << " by point_id=" << pointId
                                     << " and grp_id=" << grpId;
    xmlNodePtr reqNode = getQueryNode(),
               resNode = getAnswerNode();

    xmlNodePtr loadPaxNode = NewTextChild(reqNode, "TCkinLoadPax");
    NewTextChild(loadPaxNode, "point_id", pointId);
    NewTextChild(loadPaxNode, "grp_id", grpId);

    initReqInfo();

    LogTrace(TRACE3) << "load pax query:\n" << XMLTreeToText(reqNode->doc);
    CheckInInterface::instance()->LoadPax(getRequestCtxt(), loadPaxNode, resNode);
    LogTrace(TRACE3) << "load pax answer:\n" << XMLTreeToText(resNode->doc);
    return LoadPaxXmlResult(resNode);
}

SearchPaxXmlResult AstraEngine::SearchCheckInPax(int pointDep,
                                                 const std::string& paxSurname,
                                                 const std::string& paxName)
{
    xmlNodePtr reqNode = getQueryNode(),
               resNode = getAnswerNode();

    std::string query = paxSurname + " " + paxName;

    xmlNodePtr searchPaxNode = NewTextChild(reqNode, "SearchPax");
    NewTextChild(searchPaxNode, "point_dep", pointDep);
    NewTextChild(searchPaxNode, "pax_status", "K"); // "K" - CheckIn status
    NewTextChild(searchPaxNode, "query", query);

    initReqInfo();

    LogTrace(TRACE3) << "search pax query:\n" << XMLTreeToText(reqNode->doc);
    CheckInInterface::instance()->SearchPax(getRequestCtxt(), searchPaxNode, resNode);
    LogTrace(TRACE3) << "search pax answer:\n" << XMLTreeToText(resNode->doc);
    return SearchPaxXmlResult(resNode);
}

LoadPaxXmlResult AstraEngine::SavePax(int pointDep, const XmlTrip& paxTrip)
{
    const XmlPnr& pnr = paxTrip.pnr();

    xmlNodePtr reqNode = getQueryNode(),
               resNode = getAnswerNode();

    xmlNodePtr savePaxNode = NewTextChild(reqNode, "TCkinSavePax");
    NewTextChild(savePaxNode, "agent_stat_period", -1);
    NewTextChild(savePaxNode, "transfer");

    xmlNodePtr segsNode = NewTextChild(savePaxNode, "segments");

    xmlNodePtr segNode = NewTextChild(segsNode, "segment");
    NewTextChild(segNode, "point_dep", pointDep);
    NewTextChild(segNode, "point_arv", findArvPointId(pointDep, pnr.airp_arv));
    NewTextChild(segNode, "airp_dep",  paxTrip.airp_dep);
    NewTextChild(segNode, "airp_arv",  pnr.airp_arv);
    NewTextChild(segNode, "class",     pnr.cls);
    NewTextChild(segNode, "status",    paxTrip.status);
    NewTextChild(segNode, "wl_type");
    NewTextChild(segNode, "paid_bag_emd");

    XmlEntityViewer::viewMarkFlight(segNode, makeMarkFlight(paxTrip));

    // бежим по пассажирам
    xmlNodePtr passengersNode = NewTextChild(segNode, "passengers");
    for(const XmlPax& pax: pnr.passengers)
    {
        xmlNodePtr paxNode = XmlEntityViewer::viewPax(passengersNode, pax);
        if(pax.doc) {
            XmlEntityViewer::viewDoc(paxNode, *pax.doc);
        }
        if(pax.rems) {
            XmlEntityViewer::viewRems(paxNode, *pax.rems);
        }

        XmlEntityViewer::viewFqtRems(paxNode, pax.fqt_rems);
    }

    NewTextChild(savePaxNode, "excess");
    NewTextChild(savePaxNode, "hall", 1);
    NewTextChild(savePaxNode, "value_bags");
    NewTextChild(savePaxNode, "paid_bags");

    initReqInfo();

    LogTrace(TRACE3) << "save pax query:\n" << XMLTreeToText(reqNode->doc);
    CheckInInterface::instance()->SavePax(getRequestCtxt(), savePaxNode, resNode);
    LogTrace(TRACE3) << "save pax answer:\n" << XMLTreeToText(resNode->doc);
    return LoadPaxXmlResult(resNode);
}

LoadPaxXmlResult AstraEngine::SavePax(const xml_entities::XmlSegment& paxSeg,
                                      boost::optional<xml_entities::XmlBags> bags,
                                      boost::optional<xml_entities::XmlBagTags> tags)
{
    xmlNodePtr reqNode = getQueryNode(),
               resNode = getAnswerNode();

    xmlNodePtr savePaxNode = NewTextChild(reqNode, "TCkinSavePax");
    NewTextChild(savePaxNode, "agent_stat_period", -1);

    xmlNodePtr segsNode = NewTextChild(savePaxNode, "segments");
    xmlNodePtr segNode = XmlEntityViewer::viewSeg(segsNode, paxSeg);

    XmlEntityViewer::viewMarkFlight(segNode, paxSeg.mark_flight);

    xmlNodePtr passengersNode = NewTextChild(segNode, "passengers");

    for(const XmlPax& pax: paxSeg.passengers)
    {
        xmlNodePtr paxNode = XmlEntityViewer::viewPax(passengersNode, pax);
        if(pax.doc) {
            XmlEntityViewer::viewDoc(paxNode, *pax.doc);
        }
        NewTextChild(paxNode, "doco");
        if(pax.addrs) {
            XmlEntityViewer::viewAddresses(paxNode, *pax.addrs);
        }
        if(pax.rems) {
            XmlEntityViewer::viewRems(paxNode, *pax.rems);
        }
        XmlEntityViewer::viewFqtRems(paxNode, pax.fqt_rems);
    }

    NewTextChild(segNode, "paid_bag_emd");

    NewTextChild(savePaxNode, "excess");
    NewTextChild(savePaxNode, "hall", 1);
    NewTextChild(savePaxNode, "bag_refuse");
    NewTextChild(savePaxNode, "value_bags");

    if(bags) {

        XmlEntityViewer::viewBags(savePaxNode, *bags);
        if(bags->haveNotCabinBags()) {
            ASSERT(tags); // есть сумки (не р.кладь) - ожидаем и бирки*/
            XmlEntityViewer::viewBagTags(savePaxNode, *tags);
        } else {
            XmlEntityViewer::viewBagTagsHeader(savePaxNode);
        }
    } else {
        XmlEntityViewer::viewBagsHeader(savePaxNode);
        XmlEntityViewer::viewBagTagsHeader(savePaxNode);
    }

    initReqInfo();

    LogTrace(TRACE3) << "save pax query:\n" << XMLTreeToText(reqNode->doc);
    CheckInInterface::instance()->SavePax(getRequestCtxt(), savePaxNode, resNode);
    LogTrace(TRACE3) << "save pax answer:\n" << XMLTreeToText(resNode->doc);
    return LoadPaxXmlResult(resNode);
}

LoadPaxXmlResult AstraEngine::SavePax(xmlNodePtr reqNode, xmlNodePtr ediResNode)
{
    xmlNodePtr resNode = getAnswerNode();
    LogTrace(TRACE3) << "save pax query:\n" << XMLTreeToText(reqNode->doc);
    CheckInInterface::instance()->SavePax(reqNode, ediResNode, resNode);
    LogTrace(TRACE3) << "save pax answer:\n" << XMLTreeToText(resNode->doc);
    return LoadPaxXmlResult(resNode);
}

LoadPaxXmlResult AstraEngine::ReseatPax(const xml_entities::XmlSegment& paxSeg)
{
    xmlNodePtr reqNode = getQueryNode(),
               resNode = getAnswerNode();

    // пока работаем с одним пассажиром
    ASSERT(paxSeg.passengers.size() == 1);
    const XmlPax& pax = paxSeg.passengers.front();

    iatci::Seat seat = iatci::Seat::fromStr(pax.seat_no);
    xmlNodePtr reseatNode = NewTextChild(reqNode, "Reseat");
    NewTextChild(reseatNode, "trip_id",         paxSeg.seg_info.point_dep);
    NewTextChild(reseatNode, "pax_id",          pax.pax_id);
    NewTextChild(reseatNode, "xname",           seat.col());
    NewTextChild(reseatNode, "yname",           seat.row());
    NewTextChild(reseatNode, "tid",             pax.tid);
    NewTextChild(reseatNode, "question_reseat", "");

    initReqInfo();

    LogTrace(TRACE3) << "reseat pax query:\n" << XMLTreeToText(reqNode->doc);
    SalonFormInterface::instance()->Reseat(getRequestCtxt(), reseatNode, resNode);
    LogTrace(TRACE3) << "reseat pax answer:\n" << XMLTreeToText(resNode->doc);
    return LoadPax(paxSeg.seg_info.point_dep, pax.reg_no);
}


GetAdvTripListXmlResult AstraEngine::GetAdvTripList(const boost::gregorian::date& depDate)
{
    xmlNodePtr reqNode = getQueryNode(),
               resNode = getAnswerNode();

    xmlNodePtr advTripListNode = NewTextChild(reqNode, "GetAdvTripList");
    NewTextChild(advTripListNode, "date", HelpCpp::string_cast(depDate,
                                                               "%d.%m.%Y 00:00:00"));
    xmlNodePtr filterNode = NewTextChild(advTripListNode, "filter");
    NewTextChild(filterNode, "pr_takeoff", 1); // TODO what is it?

    xmlNodePtr viewNode = NewTextChild(advTripListNode, "codes_fmt");
    NewTextChild(viewNode, "codes_fmt", 5); // TODO what is it?

    initReqInfo();

    LogTrace(TRACE3) << "adv trip list query:\n" << XMLTreeToText(reqNode->doc);
    TripsInterface::instance()->GetTripList(getRequestCtxt(), advTripListNode, resNode);
    LogTrace(TRACE3) << "adv trip list answer:\n" << XMLTreeToText(resNode->doc);
    return GetAdvTripListXmlResult(resNode);
}

GetSeatmapXmlResult AstraEngine::GetSeatmap(int depPointId)
{
    xmlNodePtr reqNode = getQueryNode(),
               resNode = getAnswerNode();

    xmlNodePtr showNode = NewTextChild(reqNode, "Show");
    NewTextChild(showNode, "trip_id", depPointId);

    initReqInfo();

    LogTrace(TRACE3) << "show seatmap query:\n" << XMLTreeToText(reqNode->doc);
    SalonFormInterface::instance()->Show(getRequestCtxt(), showNode, resNode);
    LogTrace(TRACE3) << "show seatmap answer:\n" << XMLTreeToText(resNode->doc);

    return GetSeatmapXmlResult(resNode);
}

AstraEngine::AstraEngine()
{
    m_userId = getUserId();
}

int AstraEngine::getUserId() const
{
    int usrId = -1;
    OciCpp::CursCtl cur = make_curs(
"select USER_ID from USERS2 where LOGIN=:api_login");
    cur.bind(":api_login", "IATCIUSR")
       .def(usrId)
       .EXfet();
    if(cur.err() == NO_DATA_FOUND) {
        LogError(STDLOG) << "Unable to find IATCI user!";
    }

    return usrId;
}

XMLRequestCtxt* AstraEngine::getRequestCtxt() const
{
    return NULL;
}

xmlNodePtr AstraEngine::getQueryNode() const
{
    m_reqDoc.set("query");
    ASSERT(m_reqDoc.docPtr() != NULL);
    return NodeAsNode("/query", m_reqDoc.docPtr());
}

xmlNodePtr AstraEngine::getAnswerNode() const
{
    m_resDoc.set("answer");
    ASSERT(m_reqDoc.docPtr() != NULL);
    return NodeAsNode("/answer", m_resDoc.docPtr());
}

void AstraEngine::initReqInfo(const std::string& deskVersion) const
{
    TReqInfo::Instance()->Initialize("МОВ");
    TReqInfo::Instance()->screen.name  = "AIR.EXE";
    TReqInfo::Instance()->client_type  = ASTRA::ctTerm;
    //TReqInfo::Instance()->desk.code = "IATCIP";
    TReqInfo::Instance()->desk.version = deskVersion;
    TReqInfo::Instance()->desk.lang    = AstraLocale::LANG_EN;
    TReqInfo::Instance()->api_mode     = true;
    TReqInfo::Instance()->user.user_id = m_userId;
    JxtContext::JxtContHolder::Instance()
            ->setHandler(new JxtContext::JxtContHandlerSir(""));
}

//---------------------------------------------------------------------------------------

static boost::optional<XmlPlaceLayer> findLayer(const XmlPlace& place, const std::string& layerType)
{
    for(const XmlPlaceLayer& layer: place.layers) {
        if(layer.layer_type == layerType) {
            return layer;
        }
    }
    return boost::none;
}

static iatci::CabinDetails createCabinDetails(const XmlPlaceList& placelist)
{
    ASSERT(!placelist.places.empty());

    const std::string cls = placelist.places.front().cls;
    XmlPlace minYPlace = placelist.minYPlace(),
             maxYPlace = placelist.maxYPlace();
    const iatci::RowRange rowRange(boost::lexical_cast<unsigned>(minYPlace.yname),
                                   boost::lexical_cast<unsigned>(maxYPlace.yname));

    // берём первый упомянутый ряд кресел
    std::vector<XmlPlace> rowPlaces = placelist.yPlaces(minYPlace.y);

    std::list<iatci::SeatColumnDetails> seatColumns;
    std::vector<XmlPlace>::const_iterator prev = rowPlaces.begin();
    for(std::vector<XmlPlace>::const_iterator curr = rowPlaces.begin();
        curr != rowPlaces.end(); ++curr)
    {
        bool aisle = false;
        if((curr->x - prev->x) > 1) {
            aisle = true;
        }
        if(aisle) {
            // обновляем предыдущее кресло в шаблоне
            seatColumns.back().setAisle();
        }
        seatColumns.push_back(iatci::SeatColumnDetails(curr->xname));
        if(aisle) {
            // обновляем текущее кресло в шаблоне
            seatColumns.back().setAisle();
        }
        prev = curr;
    }

    return iatci::CabinDetails(cls, rowRange, "F", seatColumns);
}

static boost::optional<iatci::RowDetails> createFilledRowDetails(const XmlPlaceList& placelist,
                                                                 int row)
{
    std::vector<XmlPlace> rowPlaces = placelist.yPlaces(row);
    std::list<iatci::SeatOccupationDetails> rowOccupations;
    bool atLeastOnePlaceOccupied = false;
    for(const XmlPlace& place: rowPlaces) {
        rowOccupations.push_back(iatci::SeatOccupationDetails(place.xname));
        if(findLayer(place, "CHECKIN")) {
            rowOccupations.back().setOccupied();
            atLeastOnePlaceOccupied = true;
        }
    }

    if(atLeastOnePlaceOccupied) {
        return iatci::RowDetails(rowPlaces.front().yname, rowOccupations);
    }

    return boost::none;
}

static iatci::SeatmapDetails createSeatmapDetails(const std::list<XmlPlaceList>& lPlacelist)
{
    std::list<iatci::RowDetails> lRow;
    std::list<iatci::CabinDetails> lCabin;
    for(const XmlPlaceList& placelist: lPlacelist)
    {
        iatci::CabinDetails cabin = createCabinDetails(placelist);
        lCabin.push_back(cabin);
        XmlPlace minYPlace = placelist.minYPlace(),
                 maxYPlace = placelist.maxYPlace();
        for(int curRow = minYPlace.y; curRow <= maxYPlace.y; ++curRow)
        {
            boost::optional<iatci::RowDetails> row = createFilledRowDetails(placelist, curRow);
            if(row) {
                lRow.push_back(*row);
            }
        }
    }

    return iatci::SeatmapDetails(lCabin, lRow);
}

static TSearchFltInfo MakeSearchFltFilter(const std::string& depPort,
                                          const std::string& airline,
                                          unsigned flNum,
                                          const boost::posix_time::ptime& depDateTime)
{
    TSearchFltInfo filter;
    filter.airp_dep = depPort;
    filter.airline  = airline;
    filter.flt_no   = flNum;
    filter.scd_out  = BoostToDateTime(depDateTime);
    return filter;
}

static PaxFilter getSearchPaxFilter(const iatci::PaxDetails& pax,
                                          const boost::optional<iatci::ServiceDetails>& service = boost::none)
{
    NameFilter nmf(pax.surname(), pax.name());
    boost::optional<TicknumFilter> tnf;
    if(service) {
        auto tickcpn = service->findTicketCpn(pax.isInfant());
        if(tickcpn) {
            tnf = TicknumFilter(tickcpn->ticket());
        }
    }
    boost::optional<IdFilter> idf;
    if(!pax.respRef().empty()) {
        idf = IdFilter(std::stoi(pax.respRef()));
    }

    return PaxFilter(nmf, tnf, idf);
}

static bool PaxAlreadyCheckedIn(int pointDep,
                                const iatci::PaxDetails& pax,
                                const boost::optional<iatci::ServiceDetails>& service)
{
    PaxListXmlResult paxListXmlRes = AstraEngine::singletone().PaxList(pointDep);
    PaxFilter filter = getSearchPaxFilter(pax, service);
    return !paxListXmlRes.applyFilters(filter).empty();
}

static LoadPaxXmlResult LoadPax__(int pointDep, const iatci::PaxDetails& pax)
{
    PaxListXmlResult paxListXmlRes = AstraEngine::singletone().PaxList(pointDep);
    PaxFilter filter = getSearchPaxFilter(pax);
    std::list<XmlPax> wantedPaxes = paxListXmlRes.applyFilters(filter);
    if(wantedPaxes.empty()) {
        // не нашли пассажира в списке зарегистрированных
        throw tick_soft_except(STDLOG, AstraErr::PAX_SURNAME_NOT_CHECKED_IN);
    }

    if(wantedPaxes.size() > 1) {
        // нашли слишком много пассажиров
        throw tick_soft_except(STDLOG, AstraErr::TOO_MANY_PAX_WITH_SAME_SURNAME);
    }

    // в этом месте в списке wantedPaxes ровно 1 пассажир
    const XmlPax& wantedPax = wantedPaxes.front();
    LogTrace(TRACE3) << "Pax " << wantedPax.surname << " " << wantedPax.name << " "
                     << "has reg_no " << wantedPax.reg_no;

    ASSERT(wantedPax.reg_no != ASTRA::NoExists);

    LoadPaxXmlResult loadPaxXmlRes =
                AstraEngine::singletone().LoadPax(pointDep, wantedPax.reg_no);

    if(loadPaxXmlRes.lSeg.size() != 1) {
        LogError(STDLOG) << "Load pax failed! " << loadPaxXmlRes.lSeg.size() << " "
                         << "segments found but should be 1";
        throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR);
    }

    loadPaxXmlRes.applyPaxFilter(filter);

    return loadPaxXmlRes;
}

static LoadPaxXmlResult LoadGrp__(int pointDep, int grpId)
{
    LoadPaxXmlResult loadPaxXmlRes =
                AstraEngine::singletone().LoadGrp(pointDep, grpId);

    if(loadPaxXmlRes.lSeg.size() != 1) {
        LogError(STDLOG) << "Load pax failed! " << loadPaxXmlRes.lSeg.size() << " "
                         << "segments found but should be 1";
        throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR);
    }

    return loadPaxXmlRes;
}

//---------------------------------------------------------------------------------------

static void applyDocUpdate(XmlPax& pax, const iatci::UpdateDocDetails& updDoc)
{
    LogTrace(TRACE3) << __FUNCTION__ << " with act code: " << updDoc.actionCodeAsString();
    XmlPaxDoc newDoc;
    newDoc.no            = updDoc.no();
    newDoc.type          = updDoc.docType();
    newDoc.birth_date    = boostDateToAstraFormatStr(updDoc.birthDate());
    newDoc.expiry_date   = boostDateToAstraFormatStr(updDoc.expiryDate());
    newDoc.surname       = updDoc.surname();
    newDoc.first_name    = updDoc.name();
    newDoc.second_name   = updDoc.secondName();
    newDoc.nationality   = updDoc.nationality();
    newDoc.issue_country = updDoc.issueCountry();
    newDoc.gender        = updDoc.gender();
    pax.doc              = newDoc;
}

//---------------------------------------------------------------------------------------

static void applyAddressUpdate(XmlPax& pax,
                               iatci::UpdateDetails::UpdateActionCode_e actCode,
                               const iatci::UpdateAddressDetails::AddrInfo& a)
{
    LogTrace(TRACE3) << __FUNCTION__;
    if(!pax.addrs) {
        pax.addrs = XmlPaxAddresses();
    }

    XmlPaxAddress addr;
    addr.type        = a.type();
    addr.country     = a.country();
    addr.address     = a.address();
    addr.city        = a.city();
    addr.region      = a.region();
    addr.postal_code = a.postalCode();

    switch(actCode)
    {
    case iatci::UpdateDetails::Add:
    case iatci::UpdateDetails::Replace:
    {
        LogTrace(TRACE3) << "add/modify address " << addr.type;
        pax.addrs->addresses.push_back(addr);
        break;
    }
    case iatci::UpdateDetails::Cancel:
    {
        LogTrace(TRACE3) << "cancel address: " << addr.type;
        pax.addrs->addresses.remove(addr);
        break;
    }
    default:
        break;
    }
}

static void applyAddressesUpdate(XmlPax& pax, const iatci::UpdateAddressDetails& updAddress)
{
    LogTrace(TRACE3) << __FUNCTION__ << " with act code: " << updAddress.actionCodeAsString();
    XmlPaxAddresses newAddrs;
    for(const auto& addr: updAddress.lAddr()) {
        applyAddressUpdate(pax, updAddress.actionCode(), addr);
    }
}

//---------------------------------------------------------------------------------------

static XmlBag createPaxBag(int paxId, int amount, int weight)
{
    XmlBag bag;
    bag.amount   = amount;
    bag.weight   = weight;
    bag.pr_cabin = 0;
    bag.pax_id   = paxId;
    return bag;
}

static XmlBag createPaxHandBag(int paxId, int amount, int weight)
{
    XmlBag bag = createPaxBag(paxId, amount, weight);
    bag.pr_cabin = 1;
    return bag;
}

static XmlBagTag createPaxBagTag(int paxId, uint64_t tagNo)
{
    XmlBagTag bagTag;
    bagTag.tag_type = "RUCH";// TODO ITCI ?!!
    bagTag.pr_print = 0;
    bagTag.no       = tagNo;
    bagTag.pax_id   = paxId;
    return bagTag;
}

//---------------------------------------------------------------------------------------

static void applyBaggageUpdate(XmlBags& newBags, XmlBags& delBags,
                               XmlBagTags& newBagTags,
                               XmlPax& pax, const iatci::UpdateBaggageDetails& updBaggage)
{
    LogTrace(TRACE3) << __FUNCTION__ << " with act code: " << updBaggage.actionCodeAsString();

    // багаж
    if(updBaggage.bag()) {
        XmlBag bag = createPaxBag(pax.pax_id,
                                  updBaggage.bag()->numOfPieces(),
                                  updBaggage.bag()->weight());
        if(bag.amount && bag.weight) {
            newBags.bags.push_back(bag);
        } else {
            delBags.bags.push_back(bag);
        }
    }

    // ручная кладь
    if(updBaggage.handBag()) {
        XmlBag handBag = createPaxHandBag(pax.pax_id,
                                          updBaggage.handBag()->numOfPieces(),
                                          updBaggage.handBag()->weight());
        if(handBag.amount && handBag.weight) {
            newBags.bags.push_back(handBag);
        } else {
            delBags.bags.push_back(handBag);
        }
    }

    // бирки
    for(const auto& updBagTag: updBaggage.bagTagsExpanded()) {
        XmlBagTag bagTag = createPaxBagTag(pax.pax_id,
                                           updBagTag.fullTag());
        newBagTags.bagTags.push_back(bagTag);
    }
}

//---------------------------------------------------------------------------------------

static void applyXmlRem(XmlPax& pax, const iatci::UpdateServiceDetails::UpdSsrInfo& updSsr)
{
    LogTrace(TRACE3) << __FUNCTION__;

    if(!pax.rems) {
        pax.rems = XmlRems();
    }

    XmlRem rem;
    rem.rem_code = updSsr.ssrCode();
    rem.rem_text = updSsr.freeText();

    // хак
    if(rem.rem_text.empty()) {
        rem.rem_text = rem.rem_code;
    }

    switch(updSsr.actionCode())
    {
    case iatci::UpdateDetails::Add:
    case iatci::UpdateDetails::Replace:
    {
        LogTrace(TRACE3) << "add/modify remark: " << updSsr.ssrCode() <<
                            "/" << updSsr.ssrText() << " (" << updSsr.freeText() << ")";
        pax.rems->rems.push_back(rem);
        break;
    }
    case iatci::UpdateDetails::Cancel:
    {
        LogTrace(TRACE3) << "cancel remark: " << updSsr.ssrCode() <<
                            "/" << updSsr.ssrText() << " (" << updSsr.freeText() << ")";
        pax.rems->rems.remove(rem);
        break;
    }
    default:
        break;
    }
}

static void applyFqtXmlRem(XmlPax& pax, const iatci::UpdateServiceDetails::UpdSsrInfo& updSsr)
{
    LogTrace(TRACE3) << __FUNCTION__;

    if(!pax.fqt_rems) {
        pax.fqt_rems = XmlFqtRems();
    }

    XmlFqtRem rem;
    rem.rem_code = updSsr.ssrCode();
    rem.airline  = BaseTables::Company(updSsr.airline())->rcode();
    rem.no       = updSsr.ssrText();
    if(updSsr.quantity()) {
        rem.tier_level = std::to_string(updSsr.quantity());
    }

    switch(updSsr.actionCode())
    {
    case iatci::UpdateDetails::Add:
    case iatci::UpdateDetails::Replace:
    {
        LogTrace(TRACE3) << "add/modify fqt remark: " << updSsr.ssrCode()
                         << "/" << updSsr.airline() << "/" << updSsr.ssrText()
                         << "/" << updSsr.quantity();
        pax.fqt_rems->rems.push_back(rem);
        break;
    }
    case iatci::UpdateDetails::Cancel:
    {
        LogTrace(TRACE3) << "cancel fqt remark: " << updSsr.ssrCode()
                         << "/" << updSsr.airline() << "/" << updSsr.ssrText()
                         << "/" << updSsr.quantity();
        pax.fqt_rems->rems.remove(rem);
        break;
    }
    default:
        break;
    }
}


static void applyRemsUpdate(XmlPax& pax, const iatci::UpdateServiceDetails& updSvc)
{
    LogTrace(TRACE3) << __FUNCTION__;

    for(const iatci::UpdateServiceDetails::UpdSsrInfo& updSsr: updSvc.lSsr())
    {
        if(updSsr.isTkne()) continue;

        if(updSsr.isFqt()) {
            applyFqtXmlRem(pax, updSsr);
        } else {
            applyXmlRem(pax, updSsr);
        }
    }
}

//---------------------------------------------------------------------------------------

static void applySeatUpdate(XmlPax& pax, const iatci::UpdateSeatDetails& updSeat)
{
    LogTrace(TRACE3) << __FUNCTION__ << " with act code: " << updSeat.actionCodeAsString();
    pax.seat_no = updSeat.seat();
}

//---------------------------------------------------------------------------------------

int findDepPointId(const std::string& depPort,
                   const std::string& airline,
                   unsigned flNum,
                   const boost::gregorian::date& depDate)
{
    TSearchFltInfo filter = MakeSearchFltFilter(depPort,
                                                airline,
                                                flNum,
                                                boost::posix_time::ptime(depDate));
    std::list<TAdvTripInfo> lFlts;
    SearchFlt(filter, lFlts);

    LogTrace(TRACE3) << "Found " << lFlts.size() << " flights";

    if(!lFlts.empty()) {
        tst();
        return lFlts.front().point_id;
    }

    throw tick_soft_except(STDLOG, AstraErr::INV_FLIGHT_DATE);
}

int findDepPointId(const std::string& depPort,
                   const std::string& airline,
                   const Ticketing::FlightNum_t& flNum,
                   const boost::gregorian::date& depDate)
{
    return findDepPointId(depPort,
                          airline,
                          flNum.get(),
                          depDate);
}

int findArvPointId(int pointDep, const std::string& arvPort)
{
    FlightPoints flPoints;
    flPoints.Get(pointDep);
    int pointArv = 0;
    // найдём point_id прибытия на сегменте
    for(FlightPoints::const_iterator it = flPoints.begin();
        it != flPoints.end(); ++it)
    {
        if(arvPort == it->airp)
        {
            LogTrace(TRACE3) << "PointId[" << it->point_id << "] found by airport arv "
                             << "[" << arvPort << "]";
            pointArv = it->point_id;
            break;
        }
    }
    return pointArv;
}

int findGrpIdByRegNo(int pointDep, int regNo)
{
    OciCpp::CursCtl cur = make_curs(
"select PAX_GRP.GRP_ID from PAX, PAX_GRP "
"where PAX.GRP_ID = PAX_GRP.GRP_ID "
"and PAX_GRP.POINT_DEP = :point_dep "
"and PAX.REG_NO = :regno "
"and PAX.REFUSE is null "
"and PAX_GRP.STATUS not in ('E')");
    int grpId = 0;
    cur.def(grpId)
       .bind(":point_dep", pointDep)
       .bind(":regno",     regNo)
       .EXfet();

    LogTrace(TRACE3) << "grp_id:" << grpId << " "
                     << "found by point_dep:" << pointDep << " and "
                     << "reg_no:" << regNo;
    return grpId;
}

int findGrpIdByPaxId(int pointDep, int paxId)
{
    OciCpp::CursCtl cur = make_curs(
"select PAX_GRP.GRP_ID from PAX, PAX_GRP "
"where PAX.GRP_ID = PAX_GRP.GRP_ID "
"and PAX_GRP.POINT_DEP = :point_dep "
"and PAX.PAX_ID = :pax_id "
"and PAX.REFUSE is null "
"and PAX_GRP.STATUS not in ('E')");
    int grpId = 0;
    cur.def(grpId)
       .bind(":point_dep", pointDep)
       .bind(":pax_id",   paxId)
       .EXfet();

    LogTrace(TRACE3) << "grp_id:" << grpId << " "
                     << "found by point_dep:" << pointDep << " and "
                     << "pax_id:" << paxId;
    return grpId;
}

iatci::dcrcka::Result checkinIatciPaxes(xmlNodePtr reqNode, xmlNodePtr ediResNode)
{
    if(!StatusOfAllTicketsChanged(ediResNode)) {
        tst();
        throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR, "Ets exchange error");
    }

    LoadPaxXmlResult loadPaxXmlRes = AstraEngine::singletone().SavePax(reqNode, ediResNode);
    if(loadPaxXmlRes.lSeg.empty()) {
        // не смогли зарегистрировать
        throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR, "Unable to checkin pax");
    }
    return loadPaxXmlRes.toIatciFirst(iatci::dcrcka::Result::Checkin,
                                      iatci::dcrcka::Result::Ok);
}

static void handleIatciCkiPax(int pointDep,
                              boost::optional<XmlTripHeader>& tripHeader,
                              XmlSegment& paxSeg,
                              const iatci::PaxDetails& pax,
                              const boost::optional<iatci::SeatDetails>& seat,
                              const boost::optional<iatci::ReservationDetails>& reserv,
                              const boost::optional<iatci::ServiceDetails>& service,
                              const boost::optional<iatci::DocDetails>& doc,
                              const boost::optional<iatci::AddressDetails>& addr
                              )
{
    const std::string PaxSurname = pax.surname();
    const std::string PaxName    = pax.name();
    LogTrace(TRACE3) << "handle pax: " << PaxSurname << "/" << PaxName;

    if(PaxAlreadyCheckedIn(pointDep, pax, service)) {
        throw tick_soft_except(STDLOG, AstraErr::PAX_ALREADY_CHECKED_IN);
    }

    SearchPaxXmlResult searchPaxXmlRes =
                AstraEngine::singletone().SearchCheckInPax(pointDep,
                                                           PaxSurname,
                                                           PaxName);

    std::list<XmlTrip> tripsFiltered = searchPaxXmlRes.applyNameFilter(PaxSurname,
                                                                       PaxName);
    if(tripsFiltered.empty()) {
        tst();
        throw tick_soft_except(STDLOG, AstraErr::PAX_SURNAME_NF);
    }

    if(tripsFiltered.size() > 1) {
        LogError(STDLOG) << "Warning: too many trips found!";
    }

    const XmlTrip& trip = tripsFiltered.front();

    std::list<XmlPnr> pnrsFiltered = trip.applyNameFilter(PaxSurname,
                                                          PaxName);
    ASSERT(!pnrsFiltered.empty());

    std::list<XmlPnr> finalPnrs;
    std::list<XmlPax> finalPaxes;
    for(const auto& pnr: pnrsFiltered) {
        std::list<XmlPax> paxesFiltered = pnr.applyNameFilter(PaxSurname,
                                                              PaxName);
        if(!paxesFiltered.empty()) {
            finalPnrs.push_back(pnr);
            algo::append(finalPaxes, paxesFiltered);
        }
    }

    if(finalPnrs.size() > 1 || finalPaxes.size() > 1) {
        if(service) {
            auto tickCpnOpt = service->findTicketCpn(pax.isInfant());
            if(tickCpnOpt) {
                Ticketing::TicketNum_t tickNum = tickCpnOpt->ticket();
                finalPnrs.clear();
                finalPaxes.clear();
                pnrsFiltered = trip.applyTickNumFilter(tickNum);
                for(const auto& pnr: pnrsFiltered) {
                    std::list<XmlPax> paxesFiltered = pnr.applyTickNumFilter(tickNum);
                    if(!paxesFiltered.empty()) {
                        finalPnrs.push_back(pnr);
                        algo::append(finalPaxes, paxesFiltered);
                    }
                }
            }
        }
    }

    if(finalPnrs.size() > 1 || finalPaxes.size() > 1) {
        throw tick_soft_except(STDLOG, AstraErr::TOO_MANY_PAX_WITH_SAME_SURNAME);
    }

    ASSERT(!finalPnrs.empty());
    ASSERT(!finalPaxes.empty());

    if(!tripHeader) {
        tripHeader = makeTripHeader(trip);

        paxSeg.seg_info.airp_dep  = trip.airp_dep;
        paxSeg.seg_info.airp_arv  = finalPnrs.front().airp_arv;
        paxSeg.seg_info.cls       = finalPnrs.front().cls;
        paxSeg.seg_info.status    = "K"; // CheckIn status
        paxSeg.seg_info.point_dep = pointDep;
        paxSeg.seg_info.point_arv = findArvPointId(pointDep, finalPnrs.front().airp_arv);

        paxSeg.mark_flight = makeMarkFlight(trip);
    }

    paxSeg.passengers.push_back(createCheckInPax(finalPaxes.front(),
                                                 pax,
                                                 seat,
                                                 reserv,
                                                 service,
                                                 doc,
                                                 addr));
}

iatci::dcrcka::Result checkinIatciPaxes(const iatci::CkiParams& ckiParams)
{
    LogTrace(TRACE3) << __FUNCTION__;

    const iatci::FlightDetails& outbFlt = ckiParams.fltGroup().outboundFlight();
    int pointDep = astra_api::findDepPointId(outbFlt.depPort(),
                                             outbFlt.airline(),
                                             outbFlt.flightNum(),
                                             outbFlt.depDate());

    boost::optional<XmlTripHeader> tripHeader;
    XmlSegment paxSeg;
    const auto& paxGroups = ckiParams.fltGroup().paxGroups();
    for(const auto& paxGroup: paxGroups) {
        handleIatciCkiPax(pointDep, tripHeader, paxSeg,
                          paxGroup.pax(),
                          paxGroup.seat(),
                          paxGroup.reserv(),
                          paxGroup.service(),
                          paxGroup.doc(),
                          paxGroup.address());
        if(paxGroup.infant()) {
            // докинем инфанта
            handleIatciCkiPax(pointDep, tripHeader, paxSeg,
                              paxGroup.infant().get(),
                              paxGroup.seat(),
                              paxGroup.reserv(),
                              paxGroup.service(),
                              paxGroup.infantDoc(),
                              paxGroup.infantAddress());
        }
    }

    if(tripHeader) {
        paxSeg.trip_header = tripHeader.get();
    }

    // SavePax
    LoadPaxXmlResult loadPaxXmlRes = AstraEngine::singletone().SavePax(paxSeg);
    if(loadPaxXmlRes.lSeg.empty()) {
        // не смогли зарегистрировать
        throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR, "Unable to checkin pax");
    }

    return loadPaxXmlRes.toIatciFirst(iatci::dcrcka::Result::Checkin,
                                      iatci::dcrcka::Result::Ok);
}

static boost::optional<XmlBags> makeBags(const XmlBags& oldBags,
                                         const XmlBags& delBags,
                                         const XmlBags& newBags)
{
    if(oldBags.empty() && delBags.empty() && newBags.empty()) {
        tst();
        return boost::none;
    }

    XmlBags ret;

    LogTrace(TRACE3) << "new_bags.size = " << newBags.bags.size();
    for(const auto& newBag: newBags.bags) {
        ASSERT(newBag.pax_id != ASTRA::NoExists);

        ret.bags.push_back(newBag);
    }

    LogTrace(TRACE3) << "old_bags.size = " << oldBags.bags.size();
    for(const auto& oldBag: oldBags.bags) {
        ASSERT(oldBag.pax_id != ASTRA::NoExists);

        if(!newBags.findBag(oldBag.pax_id, oldBag.pr_cabin)) {
            if(!delBags.findBag(oldBag.pax_id, oldBag.pr_cabin)) {
                ret.bags.push_back(oldBag);
            }
        }
    }

    LogTrace(TRACE3) << "del_bags.size = " << delBags.bags.size();
    for(const auto& delBag: delBags.bags) {
        ASSERT(delBag.pax_id != ASTRA::NoExists);

        if(!oldBags.findBag(delBag.pax_id, delBag.pr_cabin)) {
            LogWarning(STDLOG) << "Warning: attempt to remove nonexistent"
                               << (delBag.pr_cabin ? " hand " : " ") << "bag!";
        }
    }

    LogTrace(TRACE2) << "res_bags.size = " << ret.bags.size();

    return ret;
}

static boost::optional<XmlBagTags> makeBagTags(const XmlBagTags& oldBagTags,
                                               const XmlBagTags& newBagTags,
                                               const XmlBags& delBags)
{
    if(oldBagTags.empty() && newBagTags.empty()) {
        tst();
        return boost::none;
    }

    XmlBagTags ret;

    LogTrace(TRACE3) << "new_bag_tags.size = " << newBagTags.bagTags.size();
    for(const auto& newBagTag: newBagTags.bagTags) {
        ASSERT(newBagTag.pax_id != ASTRA::NoExists);

        ret.bagTags.push_back(newBagTag);
    }

    LogTrace(TRACE3) << "old_bag_tags.size = " << oldBagTags.bagTags.size();
    for(const auto& oldBagTag: oldBagTags.bagTags) {
        ASSERT(oldBagTag.pax_id != ASTRA::NoExists);

        if(!newBagTags.containsTagForPax(oldBagTag.pax_id)) {
            if(!delBags.findBag(oldBagTag.pax_id, 0)) {
                ret.bagTags.push_back(oldBagTag);
            }
        }
    }

    LogTrace(TRACE2) << "res_bag_tags.size = " << ret.bagTags.size();

    return ret;
}

static void handleIatciCkuPax(int pointDep,
                              bool& needMakeSeg,
                              XmlSegment& paxSeg,
                              XmlBags& newBags,
                              XmlBags& delBags,
                              XmlBagTags& newBagTags,
                              const iatci::PaxDetails& pax,
                              const boost::optional<iatci::UpdatePaxDetails>& updPax,
                              const boost::optional<iatci::UpdateDocDetails>& updDoc,
                              const boost::optional<iatci::UpdateAddressDetails>& updAddress,
                              const boost::optional<iatci::UpdateSeatDetails>& updSeat,
                              const boost::optional<iatci::UpdateServiceDetails>& updService,
                              const boost::optional<iatci::UpdateBaggageDetails>& updBaggage)
{
    LogTrace(TRACE3) << "handle pax: " << pax.surname() << "/" << pax.name();

    if(updPax) {
        // Обновление инфы по паксу пока? не позволяем
        throw tick_soft_except(STDLOG, AstraErr::FUNC_NOT_SUPPORTED);
    }

    LoadPaxXmlResult loadPaxXmlRes = LoadPax__(pointDep, pax);

    const XmlSegment& currSeg = loadPaxXmlRes.lSeg.front();
    XmlPax newPax = currSeg.passengers.front();

    ASSERT(currSeg.seg_info.grp_id != ASTRA::NoExists);

    if(needMakeSeg) {
        paxSeg = makeSegment(currSeg);
        needMakeSeg = false;
    }

    if(updSeat) {
        applySeatUpdate(newPax, *updSeat);
    }

    if(updDoc) {
        applyDocUpdate(newPax, *updDoc);
    }

    if(updAddress) {
        applyAddressesUpdate(newPax, *updAddress);
    }

    if(updService) {
        applyRemsUpdate(newPax, *updService);
    }

    if(updBaggage) {
        applyBaggageUpdate(newBags, delBags, newBagTags, newPax, *updBaggage);
    }

    if(currSeg.seg_info.grp_id != paxSeg.seg_info.grp_id) {
        LogWarning(STDLOG) << "Attempt to update paxes from different groups at single request! "
                           << currSeg.seg_info.grp_id << "<>" << paxSeg.seg_info.grp_id;
        throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR);
    }

    paxSeg.passengers.push_back(newPax);
}

static void updateBaggageQuery(XmlSegment& seg, XmlBags& bags, XmlBagTags& bagTags)
{
    LogTrace(TRACE3) << __FUNCTION__;

    std::set<int> bagPaxIds;
    std::multimap<int, XmlBag*> paxBags;
    for(auto& bag: bags.bags)
    {
        bagPaxIds.emplace(bag.pax_id);
        paxBags.emplace(bag.pax_id, &bag);
        LogTrace(TRACE3) << "associate pax " << bag.pax_id << " with " << bag;
    }

    std::set<int> tagPaxIds;
    std::multimap<int, XmlBagTag*> paxBagTags;
    for(auto& bagTag: bagTags.bagTags)
    {
        tagPaxIds.emplace(bagTag.pax_id);
        paxBagTags.emplace(bagTag.pax_id, &bagTag);
        LogTrace(TRACE3) << "associate pax " << bagTag.pax_id << " with " << bagTag;
    }

    LoadPaxXmlResult loadGrpXmlRes = AstraEngine::singletone().LoadGrp(seg.seg_info.point_dep,
                                                                       seg.seg_info.grp_id);

    ASSERT(loadGrpXmlRes.lSeg.size());
    std::list<XmlPax> grpPaxes = loadGrpXmlRes.lSeg.front().passengers;
    for(const auto& pax: grpPaxes) {
        if(pax.bag_pool_num != ASTRA::NoExists && !seg.findPaxById(pax.pax_id)) {
            seg.passengers.push_back(pax);
        }
    }

    int curBagNum = 0, curBagPoolNum = 0;
    for(int paxId: bagPaxIds)
    {
        auto range = paxBags.equal_range(paxId);
        ++curBagPoolNum;
        for(auto it = range.first; it != range.second; ++it)
        {
            it->second->num          = ++curBagNum;
            it->second->airline      = seg.mark_flight.airline;
            it->second->bag_pool_num = curBagPoolNum;
        }

        for(auto& pax: seg.passengers) {
            if(pax.pax_id == paxId) {
                pax.bag_pool_num = curBagPoolNum;
            }
        }
    }

    // контрольный выстрел по bag_pool_num
    for(auto& pax: seg.passengers) {
        if(!algo::contains(bagPaxIds, pax.pax_id)) {
            pax.bag_pool_num = ASTRA::NoExists;
        }
    }

    // бирки
    if(bagTags.empty()) {
        LogTrace(TRACE3) << "Bag tags are empty!";
        return;
    }


    unsigned bagTagNum = 0;
    for(int paxId: tagPaxIds)
    {
        LogTrace(TRACE3) << "handle bag tag for pax_id=" << paxId;
        auto bagsRange = paxBags.equal_range(paxId);
        auto bagTagsRange = paxBagTags.equal_range(paxId);

        size_t bagsTotalAmount = 0;
        std::list<XmlBag*> paxBags;
        LogTrace(TRACE3) << "pax bags:";
        for(auto it = bagsRange.first; it != bagsRange.second; ++it)
        {
            if(!it->second->pr_cabin)
            {
                paxBags.push_back(it->second);
                bagsTotalAmount += it->second->amount;
                LogTrace(TRACE3) << *(it->second);
            }
        }
        LogTrace(TRACE3) << "pax bags total amount=" << bagsTotalAmount;

        std::list<XmlBagTag*> paxBagTags;
        LogTrace(TRACE3) << "pax bag tags:";
        for(auto it = bagTagsRange.first; it != bagTagsRange.second; ++it)
        {
            paxBagTags.push_back(it->second);
            LogTrace(TRACE3) << *(it->second);
        }
        LogTrace(TRACE3) << "pax bag tags total count=" << paxBagTags.size();

        // в этом месте количество сумок багажа равно количеству бирок
        ASSERT(bagsTotalAmount == paxBagTags.size());

        auto bagIt = paxBags.begin();
        auto bagTagIt = paxBagTags.begin();
        // биркам пассажира проставим bag_num
        for(; bagIt != paxBags.end(); ++bagIt)
        {
            for(int i = 0; i < (*bagIt)->amount; ++i, ++bagTagIt) {
                (*bagTagIt)->num = ++bagTagNum;
                (*bagTagIt)->bag_num = (*bagIt)->num;
                LogTrace(TRACE3) << "set num=" << (*bagTagIt)->num
                                 << ", bag_num=" << (*bagTagIt)->bag_num
                                 << " for " << *(*bagTagIt);
            }
        }
    }
}

//---------------------------------------------------------------------------------------

static BaseTables::Company awkByAccode(const std::string& accode)
{
    int awk_ida;
    OciCpp::CursCtl cur = make_curs(
"select ID from AIRLINES where AIRCODE=:accode and PR_DEL=0");
    cur.def(awk_ida)
       .bind(":accode", accode)
       .EXfet();

    ASSERT(cur.err() != NO_DATA_FOUND);
    return BaseTables::Company(Ticketing::Airline_t(awk_ida));
}

//---------------------------------------------------------------------------------------

iatci::dcrcka::Result updateIatciPaxes(const iatci::CkuParams& ckuParams)
{
    LogTrace(TRACE3) << __FUNCTION__;

    const iatci::FlightDetails& outbFlt = ckuParams.fltGroup().outboundFlight();
    int pointDep = astra_api::findDepPointId(outbFlt.depPort(),
                                             outbFlt.airline(),
                                             outbFlt.flightNum(),
                                             outbFlt.depDate());

    const auto& paxGroups = ckuParams.fltGroup().paxGroups();
    ASSERT(!paxGroups.empty());
    //    bool multiPax = paxGroups > 1;
    //    if(multiPax) {
    //        throw tick_soft_except(STDLOG, AstraErr::UPDATE_SEPARATELY);
    //    }
    bool Reseat = paxGroups.front().updSeat();

    XmlBags newBags, delBags;
    XmlBagTags newBagTags;

    XmlSegment paxSeg;
    bool first = true;
    for(const auto& paxGroup: paxGroups) {
        handleIatciCkuPax(pointDep,
                          first, paxSeg, newBags, delBags,
                          newBagTags,
                          paxGroup.pax(),
                          paxGroup.updPax(),
                          paxGroup.updDoc(),
                          paxGroup.updAddress(),
                          paxGroup.updSeat(),
                          paxGroup.updService(),
                          paxGroup.updBaggage());
        if(paxGroup.infant()) {
            handleIatciCkuPax(pointDep,
                              first, paxSeg, newBags, delBags,
                              newBagTags,
                              paxGroup.infant().get(),
                              paxGroup.updInfant(),
                              paxGroup.updInfantDoc(),
                              paxGroup.updInfantAddress(),
                              boost::none,  // infant seat
                              boost::none,  // infant service
                              boost::none); // infant baggage
        }
    }

    if(Reseat) {
        LoadPaxXmlResult loadPaxXmlRes = AstraEngine::singletone().ReseatPax(paxSeg);
        return loadPaxXmlRes.toIatciFirst(iatci::dcrcka::Result::Update,
                                          iatci::dcrcka::Result::Ok);
    } else {
        LoadPaxXmlResult oldLoadPaxXmlRes = LoadPax__(pointDep, paxGroups.front().pax());

        boost::optional<XmlBags> bags = makeBags(oldLoadPaxXmlRes.lBag,
                                                 delBags,
                                                 newBags);
        boost::optional<XmlBagTags> bagTags;
        if(bags && !bags->empty()) {
            bagTags = makeBagTags(oldLoadPaxXmlRes.lBagTag,
                                  newBagTags,
                                  delBags);
            ASSERT(bagTags);
            ASSERT(bags->totalAmount() == bagTags->bagTags.size());
            updateBaggageQuery(paxSeg, bags.get(), bagTags.get());
        }

        LoadPaxXmlResult loadPaxXmlRes = AstraEngine::singletone().SavePax(paxSeg,
                                                                           bags,
                                                                           bagTags);
        if(loadPaxXmlRes.lSeg.empty()) {
            // не смогли сохранить
            throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR,
                                   "Unable to update pax grp");
        }

        return loadPaxXmlRes.toIatciFirst(iatci::dcrcka::Result::Update,
                                          iatci::dcrcka::Result::Ok);
    }
}

iatci::dcrcka::Result cancelCheckinIatciPax(xmlNodePtr reqNode, xmlNodePtr ediResNode)
{
    if(!StatusOfAllTicketsChanged(ediResNode)) {
        tst();
        throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR, "Ets exchange error");
    }

    LoadPaxXmlResult loadPaxXmlRes = AstraEngine::singletone().SavePax(reqNode, ediResNode);
    if(!loadPaxXmlRes.lSeg.empty()) {
        tst();
        return loadPaxXmlRes.toIatciFirst(iatci::dcrcka::Result::Cancel,
                                          iatci::dcrcka::Result::OkWithNoData);
    }

    tst();
    // в результат надо положить сегмент - достать его здесь можно, например, из reqNode
    return LoadPaxXmlResult(reqNode).toIatciFirst(iatci::dcrcka::Result::Cancel,
                                                  iatci::dcrcka::Result::OkWithNoData);
}

static void handleIatciCkxPax(int pointDep,
                              bool& needMakeSeg,
                              XmlSegment& paxSeg,
                              const iatci::PaxDetails& pax)
{
    LogTrace(TRACE3) << "handle pax: " << pax.surname() << "/" << pax.name();

    LoadPaxXmlResult loadPaxXmlRes = LoadPax__(pointDep, pax);

    const XmlSegment& currSeg = loadPaxXmlRes.lSeg.front();
    const XmlPax& currPax = currSeg.passengers.front();
    ASSERT(currSeg.seg_info.grp_id != ASTRA::NoExists);

    if(needMakeSeg) {
        paxSeg = makeSegment(currSeg);
        needMakeSeg = false;
    }

    if(currSeg.seg_info.grp_id != paxSeg.seg_info.grp_id) {
        LogWarning(STDLOG) << "Attempt to cancel paxes from different groups at single request! "
                           << currSeg.seg_info.grp_id << "<>" << paxSeg.seg_info.grp_id;
        throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR);
    }
    paxSeg.passengers.push_back(createCancelPax(currPax));
}

iatci::dcrcka::Result cancelCheckinIatciPaxes(const iatci::CkxParams& ckxParams)
{
    LogTrace(TRACE3) << __FUNCTION__;

    const iatci::FlightDetails& outbFlt = ckxParams.fltGroup().outboundFlight();
    int pointDep = astra_api::findDepPointId(outbFlt.depPort(),
                                             outbFlt.airline(),
                                             outbFlt.flightNum(),
                                             outbFlt.depDate());

    XmlSegment paxSeg;
    bool first = true;
    const auto& paxGroups = ckxParams.fltGroup().paxGroups();
    for(const auto& paxGroup: paxGroups) {
        handleIatciCkxPax(pointDep, first, paxSeg,
                          paxGroup.pax());

        if(paxGroup.infant()) {
            handleIatciCkxPax(pointDep, first, paxSeg,
                              paxGroup.infant().get());
        }
    }

    // SavePax
    LoadPaxXmlResult loadPaxXmlRes = AstraEngine::singletone().SavePax(paxSeg);

    if(!loadPaxXmlRes.lSeg.empty()) {
        tst();
        return loadPaxXmlRes.toIatciFirst(iatci::dcrcka::Result::Cancel,
                                          iatci::dcrcka::Result::OkWithNoData);
    }

    tst();
    return iatci::dcrcka::Result::makeCancelResult(iatci::dcrcka::Result::OkWithNoData,
                                                   outbFlt);
}

iatci::dcrcka::Result fillPaxList(const iatci::PlfParams& plfParams)
{
    LogTrace(TRACE3) << __FUNCTION__ << " by "
                     << "airp_dep[" << plfParams.outboundFlight().depPort() << "]; "
                     << "airline["  << plfParams.outboundFlight().airline() << "]; "
                     << "flight["   << plfParams.outboundFlight().flightNum() << "]; "
                     << "dep_date[" << plfParams.outboundFlight().depDate() << "]; "
                     << "surname["  << plfParams.personal().surname() << "]; "
                     << "name["     << plfParams.personal().name() << "] ";

    throw tick_soft_except(STDLOG, AstraErr::FUNC_NOT_SUPPORTED);

//    int pointDep = astra_api::findDepPointId(plfParams.outboundFlight().depPort(),
//                                             plfParams.outboundFlight().airline(),
//                                             plfParams.outboundFlight().flightNum().get(),
//                                             plfParams.outboundFlight().depDate());

//    LoadPaxXmlResult loadPaxXmlRes = LoadPax__(pointDep,
//                                               plfParams.personal().surname(),
//                                               plfParams.personal().name());

//    if(loadPaxXmlRes.lSeg.empty()) {
//        tst();
//        throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR, "Unable to load pax");
//    }

//    tst();
//    return loadPaxXmlRes.toIatciFirst(iatci::dcrcka::Result::Passlist,
//                                      iatci::dcrcka::Result::Ok);
}

iatci::dcrcka::Result fillSeatmap(const iatci::SmfParams& smfParams)
{
    const iatci::FlightDetails& outbFlt = smfParams.outboundFlight();
    LogTrace(TRACE3) << __FUNCTION__ << " by "
                     << "airp_dep[" << outbFlt.depPort() << "]; "
                     << "airline["  << outbFlt.airline() << "]; "
                     << "flight["   << outbFlt.flightNum() << "]; "
                     << "dep_date[" << outbFlt.depDate() << "]; ";

    int pointDep = astra_api::findDepPointId(outbFlt.depPort(),
                                             outbFlt.airline(),
                                             outbFlt.flightNum().get(),
                                             outbFlt.depDate());

    LogTrace(TRACE3) << "seatmap point id: " << pointDep;

    GetSeatmapXmlResult seatmapXmlRes = AstraEngine::singletone().GetSeatmap(pointDep);
    return seatmapXmlRes.toIatci(outbFlt);
}

static void handleIatciBprPax(int pointDep,
                              bool& needMakeSeg,
                              XmlSegment& paxSeg,
                              const iatci::PaxDetails& pax)
{
    LogTrace(TRACE3) << "handle pax: " << pax.surname() << "/" << pax.name();

    LoadPaxXmlResult loadPaxXmlRes = LoadPax__(pointDep, pax);

    const XmlSegment& currSeg = loadPaxXmlRes.lSeg.front();
    const XmlPax& currPax = currSeg.passengers.front();

    if(needMakeSeg) {
        paxSeg = makeSegment(currSeg);
        needMakeSeg = false;
    }

    if(currSeg.seg_info.grp_id != paxSeg.seg_info.grp_id) {
        LogWarning(STDLOG) << "Attempt to print BP for paxes from different groups at single request! "
                           << currSeg.seg_info.grp_id << "<>" << paxSeg.seg_info.grp_id;
        throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR);
    }
    paxSeg.passengers.push_back(currPax);
}

iatci::dcrcka::Result printBoardingPass(const iatci::BprParams& bprParams)
{
    LogTrace(TRACE3) << __FUNCTION__;

    const iatci::FlightDetails& outbFlt = bprParams.fltGroup().outboundFlight();
    int pointDep = astra_api::findDepPointId(outbFlt.depPort(),
                                             outbFlt.airline(),
                                             outbFlt.flightNum(),
                                             outbFlt.depDate());

    XmlSegment paxSeg;
    bool first = true;
    const auto& paxGroups = bprParams.fltGroup().paxGroups();
    for(const auto& paxGroup: paxGroups) {
        handleIatciBprPax(pointDep, first, paxSeg,
                          paxGroup.pax());

        if(paxGroup.infant()) {
            handleIatciBprPax(pointDep, first, paxSeg,
                              paxGroup.infant().get());
        }
    }

    savePrintBP(paxSeg.seg_info.grp_id, paxSeg.passengers);

    LoadPaxXmlResult loadGrpXmlRes = LoadGrp__(pointDep, paxSeg.seg_info.grp_id);
    if(loadGrpXmlRes.lSeg.empty()) {
        tst();
        throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR, "Unable to load grp");
    }

    tst();
    return loadGrpXmlRes.toIatciFirst(iatci::dcrcka::Result::Reprint,
                                      iatci::dcrcka::Result::Ok);
}

/////////////////////////////////////////////////////////////////////////////////////////

namespace xml_entities {

ReqParams::ReqParams(xmlNodePtr node)
{
    ASSERT(node->doc->children);
    ASSERT(node->doc->children->children);
    ASSERT(node->doc->children->children->children);
    m_rootNode = node->doc->children->children->children;
}

void ReqParams::setBoolParam(const std::string& param, bool val)
{
    xmlNodePtr node = findNodeR(m_rootNode, param);
    if(node) {
        xmlNodeSetContent(node, val);
    } else {
        xmlNewBoolChild(m_rootNode, param, val);
    }
}

bool ReqParams::getBoolParam(const std::string& param, bool nvl)
{
    return xmlbool(findNodeR(m_rootNode, param), nvl);
}

void ReqParams::setStrParam(const std::string& param, const std::string& val)
{
    xmlNodePtr node = findNodeR(m_rootNode, param);
    if(node) {
        xmlNodeSetContent(node, val);
    } else {
        NewTextChild(m_rootNode, param.c_str(), val);
    }
}

std::string ReqParams::getStrParam(const std::string& param)
{
    return getStrFromXml(findNodeR(m_rootNode, param));
}

//---------------------------------------------------------------------------------------

astra_entities::Remark XmlRem::toRem() const
{
    return astra_entities::Remark(rem_code,
                                  rem_text);
}

bool operator==(const XmlRem& l, const XmlRem& r)
{
    return (l.rem_code == r.rem_code &&
            l.rem_text == r.rem_text);
}

//---------------------------------------------------------------------------------------

astra_entities::FqtRemark XmlFqtRem::toFqtRem() const
{
    return astra_entities::FqtRemark(rem_code,
                                     airline,
                                     no,
                                     tier_level);
}

bool operator==(const XmlFqtRem& l, const XmlFqtRem& r)
{
    return (l.rem_code   == r.rem_code &&
            l.airline    == r.airline &&
            l.no         == r.no &&
            l.tier_level == r.tier_level);
}

//---------------------------------------------------------------------------------------

astra_entities::Remarks XmlRems::toRems() const
{
    astra_entities::Remarks res;
    for(const XmlRem& rem: rems) {
        res.m_lRems.push_back(rem.toRem());
    }
    return res;
}

//---------------------------------------------------------------------------------------

astra_entities::FqtRemarks XmlFqtRems::toFqtRems() const
{
    astra_entities::FqtRemarks res;
    for(const XmlFqtRem& rem: rems) {
        res.m_lFqtRems.push_back(rem.toFqtRem());
    }
    return res;
}

//---------------------------------------------------------------------------------------

astra_entities::MarketingInfo XmlMarkFlight::toMarkFlight() const
{
    return astra_entities::MarketingInfo(airline,
                                         flt_no,
                                         suffix,
                                         scd != ASTRA::NoExists ? DateTimeToBoost(scd).date()
                                                                : boost::gregorian::date(),
                                         airp_dep);
}

//---------------------------------------------------------------------------------------

astra_entities::DocInfo XmlPaxDoc::toDoc() const
{
    return astra_entities::DocInfo(type,
                                   issue_country,
                                   no,
                                   boostDateTimeFromAstraFormatStr(expiry_date).date(),
                                   surname,
                                   first_name,
                                   second_name,
                                   nationality,
                                   boostDateTimeFromAstraFormatStr(birth_date).date(),
                                   gender);
}

//---------------------------------------------------------------------------------------

astra_entities::AddressInfo XmlPaxAddress::toAddress() const
{
    return astra_entities::AddressInfo(type,
                                       country,
                                       address,
                                       city,
                                       region,
                                       postal_code);
}

bool operator==(const XmlPaxAddress& l, const XmlPaxAddress& r)
{
    return (l.type == r.type);
}

//---------------------------------------------------------------------------------------

astra_entities::Addresses XmlPaxAddresses::toAddresses() const
{
    astra_entities::Addresses res;
    for(const XmlPaxAddress& addr: addresses) {
        res.m_lAddrs.push_back(addr.toAddress());
    }
    return res;
}

//---------------------------------------------------------------------------------------

XmlPax::XmlPax()
    : pax_id(ASTRA::NoExists),
      seats(ASTRA::NoExists),
      reg_no(ASTRA::NoExists),
      bag_pool_num(ASTRA::NoExists),
      tid(ASTRA::NoExists),
      coupon_no(ASTRA::NoExists),
      ticket_confirm(ASTRA::NoExists),
      pr_norec(ASTRA::NoExists),
      pr_bp_print(ASTRA::NoExists),
      grp_id(ASTRA::NoExists),
      cl_grp_id(ASTRA::NoExists),
      hall_id(ASTRA::NoExists),
      point_arv(ASTRA::NoExists),
      user_id(ASTRA::NoExists),
      iatci_parent_id(ASTRA::NoExists)
{}

bool XmlPax::equalName(const std::string& surname, const std::string& name) const
{
    // возможно следует усложнить функцию сравнения (trim, translit)
    return (this->surname == surname &&
            this->name == name);
}

astra_entities::PaxInfo XmlPax::toPax() const
{
    return astra_entities::PaxInfo(pax_id,
                                   surname,
                                   name,
                                   DecodePerson(pers_type.c_str()),
                                   ticket_no,
                                   coupon_no,
                                   ticket_rem,
                                   seat_no,
                                   reg_no != ASTRA::NoExists ? std::to_string(reg_no) : "",
                                   iatci_pax_id,
                                   !subclass.empty() ? Ticketing::SubClass(subclass)
                                                     : Ticketing::SubClass(),
                                   doc ? doc->toDoc() : boost::optional<astra_entities::DocInfo>(),
                                   addrs ? addrs->toAddresses() : boost::optional<astra_entities::Addresses>(),
                                   rems ? rems->toRems() : boost::optional<astra_entities::Remarks>(),
                                   fqt_rems ? fqt_rems->toFqtRems() : boost::optional<astra_entities::FqtRemarks>(),
                                   bag_pool_num != ASTRA::NoExists ? bag_pool_num : 0,
                                   iatci_parent_id != ASTRA::NoExists ? iatci_parent_id : 0);
}

//---------------------------------------------------------------------------------------

astra_entities::SegmentInfo XmlSegment::toSeg() const
{
    return astra_entities::SegmentInfo(seg_info.grp_id,
                                       seg_info.point_dep,
                                       seg_info.point_arv,
                                       seg_info.airp_dep,
                                       seg_info.airp_arv,
                                       seg_info.cls,
                                       mark_flight.toMarkFlight());
}

std::list<XmlPax> XmlSegment::findPaxesByName(const std::string& surname,
                                              const std::string& name) const
{
    std::list<XmlPax> res;
    for(const XmlPax& pax: passengers) {
        if(pax.equalName(surname, name)) {
            res.push_back(pax);
        }
    }
    return res;
}

boost::optional<XmlPax> XmlSegment::findPaxById(int paxId) const
{
    for(const XmlPax& pax: passengers) {
        if(pax.pax_id == paxId){
            return pax;
        }
    }
    return boost::none;
}

//---------------------------------------------------------------------------------------

std::ostream& operator<<(std::ostream& os, const XmlBag& bag)
{
    os << "bag: "
       << bag.amount << ":" << bag.weight
       << (bag.pr_cabin ? "(cabin)" : "");
    return os;
}

//---------------------------------------------------------------------------------------

boost::optional<XmlBag> XmlBags::findBag(int paxId, int prCabin) const
{
    for(const auto& bag: bags) {
        if(bag.pax_id == paxId && bag.pr_cabin == prCabin) {
            return bag;
        }
    }
    return boost::none;
}

bool XmlBags::haveNotCabinBags() const
{
    for(const auto& bag: bags) {
        if(!bag.pr_cabin) {
            return true;
        }
    }

    return false;
}

size_t XmlBags::totalAmount() const
{
    size_t total = 0;
    for(const auto& bag: bags) {
        if(!bag.pr_cabin) {
            ASSERT(bag.amount != ASTRA::NoExists);
            total += (size_t)bag.amount;
        }
    }
    return total;
}

//---------------------------------------------------------------------------------------

std::ostream& operator<<(std::ostream& os, const XmlBagTag& tag)
{
    os << "bag tag: " << tag.no;
    return os;
}

//---------------------------------------------------------------------------------------

bool XmlBagTags::containsTagForPax(int paxId) const
{
    for(const auto& bagTag: bagTags) {
        if(bagTag.pax_id == paxId) {
            return true;
        }
    }
    return false;
}

//---------------------------------------------------------------------------------------

XmlPnr& XmlTrip::pnr()
{
    ASSERT(!pnrs.empty());
    return pnrs.front();
}

const XmlPnr& XmlTrip::pnr() const
{
    ASSERT(!pnrs.empty());
    return pnrs.front();
}

std::list<XmlPnr> XmlTrip::applyNameFilter(const std::string& surname,
                                           const std::string& name) const
{
    std::list<XmlPnr> res;
    for(const XmlPnr& pnr: pnrs) {
        for(const XmlPax& pax: pnr.passengers) {
            if(pax.equalName(surname, name)) {
                res.push_back(pnr);
            }
        }
    }
    return res;
}

std::list<XmlPnr> XmlTrip::applyTickNumFilter(const TicketNum_t& ticknum) const
{
    std::list<XmlPnr> res;
    for(const XmlPnr& pnr: pnrs) {
        for(const XmlPax& pax: pnr.passengers) {
            if(pax.ticket_no == ticknum.get()) {
                res.push_back(pnr);
            }
        }
    }
    return res;
}

//---------------------------------------------------------------------------------------

XmlPax& XmlPnr::pax()
{
    ASSERT(!passengers.empty());
    return passengers.front();
}

const XmlPax& XmlPnr::pax() const
{
    ASSERT(!passengers.empty());
    return passengers.front();
}

std::list<XmlPax> XmlPnr::applyNameFilter(const std::string& surname,
                                          const std::string& name) const
{
    std::list<XmlPax> res;
    for(const XmlPax& pax: passengers) {
        if(pax.equalName(surname, name)) {
            res.push_back(pax);
        }
    }
    return res;
}

std::list<XmlPax> XmlPnr::applyTickNumFilter(const TicketNum_t& ticknum) const
{
    std::list<XmlPax> res;
    for(const XmlPax& pax: passengers) {
        if(pax.ticket_no == ticknum.get()) {
            res.push_back(pax);
        }
    }
    return res;
}

//---------------------------------------------------------------------------------------

XmlFilterRouteItem XmlFilterRoutes::depItem() const
{
    boost::optional<XmlFilterRouteItem> item = findItem(point_dep);
    ASSERT(item);
    return item.get();
}

XmlFilterRouteItem XmlFilterRoutes::arrItem() const
{
    boost::optional<XmlFilterRouteItem> item = findItem(point_arv);
    ASSERT(item);
    return item.get();
}

boost::optional<XmlFilterRouteItem> XmlFilterRoutes::findItem(int pointId) const
{
    ASSERT(pointId != ASTRA::NoExists);
    for(auto& item: items) {
        if(item.point_id == pointId)
            return item;
    }
    return boost::none;
}

//---------------------------------------------------------------------------------------

std::vector<XmlPlace> XmlPlaceList::yPlaces(int y) const
{
    std::vector<XmlPlace> res;
    for(auto& place: places) {
        if(place.y == y) {
            res.push_back(place);
        }
    }

    ASSERT(!res.empty());
    return algo::sort(res, [](const XmlPlace& l, const XmlPlace& r) { return r.x > l.x; });
}

XmlPlace XmlPlaceList::minYPlace() const
{
    std::vector<XmlPlace> lPlaces = algo::transform<std::vector>(places, [](const XmlPlace& p) { return p; });
    lPlaces = algo::sort(lPlaces, [](const XmlPlace& l, const XmlPlace& r) { return r.y > l.y; });
    return lPlaces.front();
}

XmlPlace XmlPlaceList::maxYPlace() const
{
    std::vector<XmlPlace> lPlaces = algo::transform<std::vector>(places, [](const XmlPlace& p) { return p; });
    lPlaces = algo::sort(lPlaces, [](const XmlPlace& l, const XmlPlace& r) { return l.y > r.y; });
    return lPlaces.front();
}

//---------------------------------------------------------------------------------------

XmlCheckInTab::XmlCheckInTab(xmlNodePtr tabNode)
{
    m_seg = XmlEntityReader::readSeg(tabNode);
}

bool XmlCheckInTab::isEdi() const
{
    return (m_seg.seg_info.point_dep < 0 &&
            m_seg.seg_info.point_arv < 0 &&
            m_seg.seg_info.grp_id < 0);
}

const XmlSegment& XmlCheckInTab::xmlSeg() const
{
    return m_seg;
}

astra_entities::SegmentInfo XmlCheckInTab::seg() const
{
    return m_seg.toSeg();
}

std::list<astra_entities::PaxInfo> XmlCheckInTab::lPax() const
{
    std::list<astra_entities::PaxInfo> lPaxes;
    for(const XmlPax& pax: m_seg.passengers) {
        lPaxes.push_back(pax.toPax());
    }
    return lPaxes;
}

boost::optional<astra_entities::PaxInfo> XmlCheckInTab::getPaxById(int paxId) const
{
    LogTrace(TRACE5) << __FUNCTION__ << " by paxId: " << paxId;
    for(const auto& pax: lPax()) {
        if(pax.id() == paxId) {
            return pax;
        }
    }

    return boost::none;
}

//---------------------------------------------------------------------------------------

XmlCheckInTabs::XmlCheckInTabs(xmlNodePtr tabsNode)
{
    ASSERT(tabsNode);
    for(xmlNodePtr tabNode = tabsNode->children;
        tabNode != NULL; tabNode = tabNode->next)
    {
        m_tabs.push_back(XmlCheckInTab(tabNode));
    }
}

size_t XmlCheckInTabs::size() const
{
    return tabs().size();
}

bool XmlCheckInTabs::empty() const
{
    return (size() == 0);
}

bool XmlCheckInTabs::containsEdiTab() const
{
    return !ediTabs().empty();
}

const std::vector<XmlCheckInTab>& XmlCheckInTabs::tabs() const
{
    return m_tabs;
}

std::vector<XmlCheckInTab> XmlCheckInTabs::ediTabs() const
{
    std::vector<XmlCheckInTab> res;
    for(const auto& tab: tabs()) {
        if(tab.isEdi()) {
            res.push_back(tab);
        }
    }
    return res;
}

//---------------------------------------------------------------------------------------

XmlTripHeader XmlEntityReader::readTripHeader(xmlNodePtr thNode)
{
    ASSERT(thNode);

    XmlTripHeader th;
    th.flight             = getStrFromXml(thNode, "flight");
    th.airline            = getStrFromXml(thNode, "airline");
    th.aircode            = getStrFromXml(thNode, "aircode");
    th.flt_no             = getIntFromXml(thNode, "flt_no", 		ASTRA::NoExists);
    th.suffix             = getStrFromXml(thNode, "suffix");
    th.airp               = getStrFromXml(thNode, "airp");
    th.scd_out_local      = getDateTimeFromXml(thNode, "scd_out_local", ASTRA::NoExists);
    th.pr_etl_only        = getIntFromXml(thNode, "pr_etl_only",        ASTRA::NoExists);
    th.pr_etstatus        = getIntFromXml(thNode, "pr_etstatus",        ASTRA::NoExists);

    // вспомогательные iatci поля
    th.scd_brd_to_local   = getStrFromXml(thNode, "scd_brd_to_local");

    return th;
}

XmlTripData XmlEntityReader::readTripData(xmlNodePtr tripDataNode)
{
    ASSERT(tripDataNode);

    XmlTripData tripData;

    // airps
    xmlNodePtr airpsNode = findNode(tripDataNode, "airps");
    if(airpsNode != NULL) {
        tripData.airps = XmlEntityReader::readAirps(airpsNode);
    }

    // classes
    xmlNodePtr classesNode = findNode(tripDataNode, "classes");
    if(classesNode != NULL) {
        tripData.classes = XmlEntityReader::readClasses(classesNode);
    }

    // gates
    xmlNodePtr gatesNode = findNode(tripDataNode, "gates");
    if(gatesNode != NULL) {
        tripData.gates = XmlEntityReader::readGates(gatesNode);
    }

    // halls
    xmlNodePtr hallsNode = findNode(tripDataNode, "halls");
    if(hallsNode != NULL) {
        tripData.halls = XmlEntityReader::readHalls(hallsNode);
    }

    // marketing flights
    xmlNodePtr markFlightsNode = findNode(tripDataNode, "mark_flights");
    if(markFlightsNode != NULL) {
        tripData.mark_flights = XmlEntityReader::readMarkFlights(markFlightsNode);
    }

    return tripData;
}

XmlAirp XmlEntityReader::readAirp(xmlNodePtr airpNode)
{
    ASSERT(airpNode);

    XmlAirp airp;
    airp.point_id    = getIntFromXml(airpNode, "point_id", ASTRA::NoExists);
    airp.airp_code   = getStrFromXml(airpNode, "airp_code");
    airp.city_code   = getStrFromXml(airpNode, "city_code");
    airp.target_view = getStrFromXml(airpNode, "target_view");
    return airp;
}

std::list<XmlAirp> XmlEntityReader::readAirps(xmlNodePtr airpsNode)
{
    ASSERT(airpsNode);

    std::list<XmlAirp> airps;
    for(xmlNodePtr airpNode = airpsNode->children;
        airpNode != NULL; airpNode = airpNode->next)
    {
        airps.push_back(XmlEntityReader::readAirp(airpNode));
    }
    return airps;
}

XmlClass XmlEntityReader::readClass(xmlNodePtr classNode)
{
    ASSERT(classNode);

    XmlClass cls;
    cls.code       = getStrFromXml(classNode, "code");
    cls.class_view = getStrFromXml(classNode, "class_view");
    if(cls.class_view.empty()) {
        cls.class_view = getStrFromXml(classNode, "name");
    }
    cls.cfg        = getIntFromXml(classNode, "cfg", ASTRA::NoExists);
    return cls;
}

std::list<XmlClass> XmlEntityReader::readClasses(xmlNodePtr classesNode)
{
    ASSERT(classesNode);

    std::list<XmlClass> classes;
    for(xmlNodePtr classNode = classesNode->children;
        classNode != NULL; classNode = classNode->next)
    {
        classes.push_back(XmlEntityReader::readClass(classNode));
    }
    return classes;
}

XmlGate XmlEntityReader::readGate(xmlNodePtr gateNode)
{
    ASSERT(gateNode);
    // TODO
    XmlGate gate;
    return gate;
}

std::list<XmlGate> XmlEntityReader::readGates(xmlNodePtr gatesNode)
{
    ASSERT(gatesNode);
    // TODO
    std::list<XmlGate> gates;
    return gates;
}

XmlHall XmlEntityReader::readHall(xmlNodePtr hallNode)
{
    ASSERT(hallNode);
    // TODO
    XmlHall hall;
    return hall;
}

std::list<XmlHall> XmlEntityReader::readHalls(xmlNodePtr hallsNode)
{
    ASSERT(hallsNode);
    // TODO
    std::list<XmlHall> halls;
    return halls;
}

XmlMarkFlight XmlEntityReader::readMarkFlight(xmlNodePtr flightNode)
{
    ASSERT(flightNode);

    XmlMarkFlight flight;
    flight.airline       = getStrFromXml(flightNode, "airline");
    flight.flt_no        = getIntFromXml(flightNode, "flt_no", ASTRA::NoExists);
    flight.suffix        = getStrFromXml(flightNode, "suffix");
    flight.scd           = getDateTimeFromXml(flightNode, "scd", ASTRA::NoExists);
    flight.airp_dep      = getStrFromXml(flightNode, "airp_dep");
    flight.pr_mark_norms = getIntFromXml(flightNode, "pr_mark_norms", ASTRA::NoExists);
    return flight;
}

std::list<XmlMarkFlight> XmlEntityReader::readMarkFlights(xmlNodePtr flightsNode)
{
    ASSERT(flightsNode);

    std::list<XmlMarkFlight> flights;
    for(xmlNodePtr flightNode = flightsNode->children;
        flightNode != NULL; flightNode = flightNode->next)
    {
        flights.push_back(XmlEntityReader::readMarkFlight(flightNode));
    }
    return flights;
}

XmlTripCounterItem XmlEntityReader::readTripCounterItem(xmlNodePtr itemNode)
{
    ASSERT(itemNode);

    XmlTripCounterItem item;
    item.point_arv    = getIntFromXml(itemNode, "point_arv",    ASTRA::NoExists);
    item.cls          = getStrFromXml(itemNode, "class");
    item.noshow       = getIntFromXml(itemNode, "noshow",       ASTRA::NoExists);
    item.trnoshow     = getIntFromXml(itemNode, "trnoshow",     ASTRA::NoExists);
    item.show         = getIntFromXml(itemNode, "show",         ASTRA::NoExists);
    item.jmp_show     = getIntFromXml(itemNode, "jmp_show",     ASTRA::NoExists);
    item.jmp_nooccupy = getIntFromXml(itemNode, "jmp_nooccupy", ASTRA::NoExists);
    return item;
}

std::list<XmlTripCounterItem> XmlEntityReader::readTripCounterItems(xmlNodePtr tripCountersNode)
{
    ASSERT(tripCountersNode);

    std::list<XmlTripCounterItem> items;
    for(xmlNodePtr itemNode = tripCountersNode->children;
        itemNode != NULL; itemNode = itemNode->next)
    {
        items.push_back(XmlEntityReader::readTripCounterItem(itemNode));
    }
    return items;
}

XmlRem XmlEntityReader::readRem(xmlNodePtr remNode)
{
    ASSERT(remNode);

    XmlRem rem;
    rem.rem_code = getStrFromXml(remNode, "rem_code");
    rem.rem_text = getStrFromXml(remNode, "rem_text");
    return rem;
}

XmlRems XmlEntityReader::readRems(xmlNodePtr remsNode)
{
    ASSERT(remsNode);

    XmlRems rems;
    for(xmlNodePtr remNode = remsNode->children;
        remNode != NULL; remNode = remNode->next)
    {
        rems.rems.push_back(XmlEntityReader::readRem(remNode));
    }
    return rems;
}

XmlFqtRem XmlEntityReader::readFqtRem(xmlNodePtr remNode)
{
    ASSERT(remNode);

    XmlFqtRem rem;
    rem.rem_code   = getStrFromXml(remNode, "rem_code");
    rem.airline    = getStrFromXml(remNode, "airline");
    rem.no         = getStrFromXml(remNode, "no");
    rem.tier_level = getStrFromXml(remNode, "tier_level");
    return rem;
}

XmlFqtRems XmlEntityReader::readFqtRems(xmlNodePtr remsNode)
{
    ASSERT(remsNode);

    XmlFqtRems rems;
    for(xmlNodePtr remNode = remsNode->children;
        remNode != NULL; remNode = remNode->next)
    {
        rems.rems.push_back(XmlEntityReader::readFqtRem(remNode));
    }
    return rems;
}

XmlPaxDoc XmlEntityReader::readDoc(xmlNodePtr docNode)
{
    ASSERT(docNode);

    XmlPaxDoc doc;
    doc.no = getStrFromXml(docNode);
    if(doc.no.empty())
    {
        doc.no           = getStrFromXml(docNode, "no");
        doc.birth_date   = getStrFromXml(docNode, "birth_date");
        doc.surname      = getStrFromXml(docNode, "surname");
        doc.first_name   = getStrFromXml(docNode, "first_name");
        doc.second_name  = getStrFromXml(docNode, "second_name");
        doc.expiry_date  = getStrFromXml(docNode, "expiry_date");
        doc.type         = getStrFromXml(docNode, "type");
        doc.nationality  = getStrFromXml(docNode, "nationality");
        doc.gender       = getStrFromXml(docNode, "gender");
        doc.issue_country= getStrFromXml(docNode, "issue_country");
    }
    return doc;
}

XmlPaxAddress XmlEntityReader::readAddress(xmlNodePtr addrNode)
{
    ASSERT(addrNode);

    XmlPaxAddress addr;
    addr.type        = getStrFromXml(addrNode, "type");
    addr.country     = getStrFromXml(addrNode, "country");
    addr.address     = getStrFromXml(addrNode, "address");
    addr.city        = getStrFromXml(addrNode, "city");
    addr.region      = getStrFromXml(addrNode, "region");
    addr.postal_code = getStrFromXml(addrNode, "postal_code");
    return addr;
}

XmlPaxAddresses XmlEntityReader::readAddresses(xmlNodePtr addrsNode)
{
    ASSERT(addrsNode);

    XmlPaxAddresses addrs;
    for(xmlNodePtr addrNode = addrsNode->children;
        addrNode != NULL; addrNode = addrNode->next)
    {
        addrs.addresses.push_back(XmlEntityReader::readAddress(addrNode));
    }
    return addrs;
}

XmlPax XmlEntityReader::readPax(xmlNodePtr paxNode)
{
    ASSERT(paxNode);

    XmlPax pax;
    pax.pax_id          = getIntFromXml(paxNode, "pax_id",         ASTRA::NoExists);
    pax.grp_id          = getIntFromXml(paxNode, "grp_id",         ASTRA::NoExists);
    pax.cl_grp_id       = getIntFromXml(paxNode, "cl_grp_id",      ASTRA::NoExists);
    pax.surname         = getStrFromXml(paxNode, "surname");
    pax.name            = getStrFromXml(paxNode, "name");
    pax.airp_arv        = getStrFromXml(paxNode, "airp_arv");
    pax.pers_type       = getStrFromXml(paxNode, "pers_type");
    pax.seat_no         = getStrFromXml(paxNode, "seat_no");
    pax.seat_type       = getStrFromXml(paxNode, "seat_type");
    pax.seats           = getIntFromXml(paxNode, "seats",          ASTRA::NoExists);
    pax.refuse          = getStrFromXml(paxNode, "refuse");
    pax.reg_no          = getIntFromXml(paxNode, "reg_no",         ASTRA::NoExists);
    pax.subclass        = getStrFromXml(paxNode, "subclass");
    pax.bag_pool_num    = getIntFromXml(paxNode, "bag_pool_num",   ASTRA::NoExists);
    pax.tid             = getIntFromXml(paxNode, "tid",            ASTRA::NoExists);
    pax.ticket_no       = getStrFromXml(paxNode, "ticket_no");
    pax.coupon_no       = getIntFromXml(paxNode, "coupon_no",      ASTRA::NoExists);
    pax.ticket_rem      = getStrFromXml(paxNode, "ticket_rem");
    pax.ticket_confirm  = getIntFromXml(paxNode, "ticket_confirm", ASTRA::NoExists);
    pax.pr_norec        = getIntFromXml(paxNode, "pr_norec",       ASTRA::NoExists);
    pax.pr_bp_print     = getIntFromXml(paxNode, "pr_bp_print",    ASTRA::NoExists);
    pax.hall_id         = getIntFromXml(paxNode, "hall_id",        ASTRA::NoExists);
    pax.point_arv       = getIntFromXml(paxNode, "point_arv",      ASTRA::NoExists);
    pax.user_id         = getIntFromXml(paxNode, "user_id",        ASTRA::NoExists);
    pax.iatci_pax_id    = getStrFromXml(paxNode, "iatci_pax_id");
    pax.iatci_parent_id = getIntFromXml(paxNode, "iatci_parent_pax_id");

    // doc
    xmlNodePtr docNode = findNode(paxNode, "document");
    if(docNode != NULL && !isempty(docNode)) {
        pax.doc = XmlEntityReader::readDoc(docNode);
    }

    xmlNodePtr addrsNode = findNode(paxNode, "addresses");
    if(addrsNode != NULL) {
        pax.addrs = XmlEntityReader::readAddresses(addrsNode);
    }

    // remarks
    xmlNodePtr remsNode = findNode(paxNode, "rems");
    if(remsNode != NULL) {
        pax.rems = XmlEntityReader::readRems(remsNode);
    }

    // fqt remarks
    xmlNodePtr fqtRemsNode = findNode(paxNode, "fqt_rems");
    if(fqtRemsNode != NULL) {
        pax.fqt_rems = XmlEntityReader::readFqtRems(fqtRemsNode);
    }

    return pax;
}

std::list<XmlPax> XmlEntityReader::readPaxes(xmlNodePtr paxesNode)
{
    ASSERT(paxesNode);

    std::list<XmlPax> paxes;
    for(xmlNodePtr paxNode = paxesNode->children;
        paxNode != NULL; paxNode = paxNode->next)
    {
        paxes.push_back(XmlEntityReader::readPax(paxNode));
    }
    return paxes;
}

XmlPnrRecloc XmlEntityReader::readPnrRecloc(xmlNodePtr reclocNode)
{
    ASSERT(reclocNode);

    XmlPnrRecloc recloc;
    recloc.airline = getStrFromXml(reclocNode, "airline");
    recloc.recloc  = getStrFromXml(reclocNode, "addr");
    return recloc;
}

std::list<XmlPnrRecloc> XmlEntityReader::readPnrReclocs(xmlNodePtr reclocsNode)
{
    ASSERT(reclocsNode);

    std::list<XmlPnrRecloc> reclocs;
    for(xmlNodePtr reclocNode = reclocsNode->children;
        reclocNode != NULL; reclocNode = reclocNode->next)
    {
        reclocs.push_back(XmlEntityReader::readPnrRecloc(reclocNode));
    }
    return reclocs;
}

XmlTrferSegment XmlEntityReader::readTrferSeg(xmlNodePtr trferSegNode)
{
    ASSERT(trferSegNode);

    XmlTrferSegment trferSeg;
    trferSeg.num          = getIntFromXml(trferSegNode, "num", ASTRA::NoExists);
    trferSeg.airline      = getStrFromXml(trferSegNode, "airline");
    trferSeg.flt_no       = getStrFromXml(trferSegNode, "flt_no");
    trferSeg.local_date   = getStrFromXml(trferSegNode, "local_date");
    trferSeg.airp_dep     = getStrFromXml(trferSegNode, "airp_dep");
    trferSeg.airp_arv     = getStrFromXml(trferSegNode, "airp_arv");
    trferSeg.subclass     = getStrFromXml(trferSegNode, "subclass");
    trferSeg.trfer_permit = getIntFromXml(trferSegNode, "trfer_permit", ASTRA::NoExists);
    return trferSeg;
}

std::list<XmlTrferSegment> XmlEntityReader::readTrferSegs(xmlNodePtr trferSegsNode)
{
    ASSERT(trferSegsNode);

    std::list<XmlTrferSegment> trferSegs;
    for(xmlNodePtr trferSegNode = trferSegsNode->children;
        trferSegNode != NULL; trferSegNode = trferSegNode->next)
    {
        trferSegs.push_back(XmlEntityReader::readTrferSeg(trferSegNode));
    }
    return trferSegs;
}

XmlPnr XmlEntityReader::readPnr(xmlNodePtr pnrNode)
{
    ASSERT(pnrNode);

    XmlPnr pnr;
    pnr.pnr_id   = getIntFromXml(pnrNode, "pnr_id", ASTRA::NoExists);
    pnr.airp_arv = getStrFromXml(pnrNode, "airp_arv");
    pnr.subclass = getStrFromXml(pnrNode, "subclass");
    pnr.cls      = getStrFromXml(pnrNode, "class");

    // paxes
    xmlNodePtr paxesNode = findNode(pnrNode, "passengers");
    if(paxesNode != NULL) {
        pnr.passengers = XmlEntityReader::readPaxes(paxesNode);
    }

    // transfer segments
    xmlNodePtr trferSegsNode = findNode(pnrNode, "transfer");
    if(trferSegsNode != NULL) {
        pnr.trfer_segments = XmlEntityReader::readTrferSegs(trferSegsNode);
    }

    // pnr reclocs
    xmlNodePtr pnrAddrsNode = findNode(pnrNode, "pnr_addrs");
    if(pnrAddrsNode != NULL) {
        pnr.pnr_reclocs = XmlEntityReader::readPnrReclocs(pnrAddrsNode);
    }

    return pnr;
}

std::list<XmlPnr> XmlEntityReader::readPnrs(xmlNodePtr pnrsNode)
{
    ASSERT(pnrsNode);

    std::list<XmlPnr> pnrs;
    for(xmlNodePtr pnrNode = pnrsNode->children;
        pnrNode != NULL; pnrNode = pnrNode->next)
    {
        pnrs.push_back(XmlEntityReader::readPnr(pnrNode));
    }
    return pnrs;
}

XmlTrip XmlEntityReader::readTrip(xmlNodePtr tripNode)
{
    ASSERT(tripNode);

    XmlTrip trip;
    trip.point_id = getIntFromXml(tripNode, "point_id", ASTRA::NoExists);
    trip.airline  = getStrFromXml(tripNode, "airline");
    trip.flt_no   = getIntFromXml(tripNode, "flt_no",   ASTRA::NoExists);
    trip.scd      = getStrFromXml(tripNode, "scd");
    trip.airp_dep = getStrFromXml(tripNode, "airp_dep");

    xmlNodePtr groupsNode = findNode(tripNode, "groups");
    if(groupsNode != NULL) {
        trip.pnrs = XmlEntityReader::readPnrs(groupsNode);
    }
    return trip;
}

std::list<XmlTrip> XmlEntityReader::readTrips(xmlNodePtr tripsNode)
{
    ASSERT(tripsNode);

    std::list<XmlTrip> trips;
    for(xmlNodePtr tripNode = tripsNode->children;
        tripNode != NULL; tripNode = tripNode->next)
    {
        trips.push_back(XmlEntityReader::readTrip(tripNode));
    }
    return trips;
}

XmlServiceList XmlEntityReader::readServiceList(xmlNodePtr svcListNode)
{
    ASSERT(svcListNode);

    XmlServiceList svcList;
    xmlGetPropInt(svcListNode, "seg_no",   &svcList.seg_no);
    xmlGetPropInt(svcListNode, "category", &svcList.category);
    xmlGetPropInt(svcListNode, "list_id",  &svcList.list_id);
    return svcList;
}

std::list<XmlServiceList> XmlEntityReader::readServiceLists(xmlNodePtr svcListsNode)
{
    ASSERT(svcListsNode);

    std::list<XmlServiceList> svcLists;
    for(xmlNodePtr svcListNode = svcListsNode->children;
        svcListNode != NULL; svcListNode = svcListNode->next)
    {
        svcLists.push_back(XmlEntityReader::readServiceList(svcListNode));
    }
    return svcLists;
}

XmlSegmentInfo XmlEntityReader::readSegInfo(xmlNodePtr segNode)
{
    ASSERT(segNode);

    XmlSegmentInfo seg_info;
    seg_info.grp_id    = getIntFromXml(segNode, "grp_id",    ASTRA::NoExists);
    seg_info.point_dep = getIntFromXml(segNode, "point_dep", ASTRA::NoExists);
    seg_info.airp_dep  = getStrFromXml(segNode, "airp_dep");
    seg_info.point_arv = getIntFromXml(segNode, "point_arv", ASTRA::NoExists);
    seg_info.airp_arv  = getStrFromXml(segNode, "airp_arv");
    seg_info.cls       = getStrFromXml(segNode, "class");
    seg_info.status    = getStrFromXml(segNode, "status");
    seg_info.bag_refuse= getStrFromXml(segNode, "bag_refuse");
    seg_info.tid       = getIntFromXml(segNode, "tid",       ASTRA::NoExists);
    seg_info.city_arv  = getStrFromXml(segNode, "city_arv");
    seg_info.airline   = getStrFromXml(segNode, "airline");
    return seg_info;
}

XmlSegment XmlEntityReader::readSeg(xmlNodePtr segNode)
{
    ASSERT(segNode);

    XmlSegment seg;

    // trip header
    xmlNodePtr tripHeaderNode = findNode(segNode, "tripheader");
    if(tripHeaderNode != NULL) {
        seg.trip_header = XmlEntityReader::readTripHeader(tripHeaderNode);
    }

    // trip data
    xmlNodePtr tripDataNode = findNode(segNode, "tripdata");
    if(tripDataNode == NULL) {
        // tripDataNode отсутствует, если парсим xml запрос
        tripDataNode = segNode;
    }
    seg.trip_data = XmlEntityReader::readTripData(tripDataNode);

    // general information
    seg.seg_info = XmlEntityReader::readSegInfo(segNode);

    // mark flight
    xmlNodePtr markFlightNode = findNode(segNode, "mark_flight");
    if(markFlightNode == NULL) {
        markFlightNode = findNode(segNode, "pseudo_mark_flight");
    }

    if(markFlightNode != NULL) {
        seg.mark_flight = XmlEntityReader::readMarkFlight(markFlightNode);
    }

    // paxes
    xmlNodePtr paxesNode = findNode(segNode, "passengers");
    if(paxesNode != NULL) {
        seg.passengers = XmlEntityReader::readPaxes(paxesNode);
    }

    // trip counters
    xmlNodePtr tripCountersNode = findNode(segNode, "tripcounters");
    if(tripCountersNode != NULL) {
        seg.trip_counters = XmlEntityReader::readTripCounterItems(tripCountersNode);
    }

    return seg;
}

std::list<XmlSegment> XmlEntityReader::readSegs(xmlNodePtr segsNode)
{
    ASSERT(segsNode);

    std::list<XmlSegment> segs;
    for(xmlNodePtr segNode = segsNode->children;
        segNode != NULL; segNode = segNode->next)
    {
        segs.push_back(XmlEntityReader::readSeg(segNode));
    }
    return segs;
}

XmlBag XmlEntityReader::readBag(xmlNodePtr bagNode)
{
    ASSERT(bagNode);

    XmlBag bag;
    bag.bag_type       = getStrFromXml(bagNode, "bag_type");
    bag.airline        = getStrFromXml(bagNode, "airline");
    bag.id             = getIntFromXml(bagNode, "id",           ASTRA::NoExists);
    bag.num            = getIntFromXml(bagNode, "num",          ASTRA::NoExists);
    bag.pr_cabin       = getBoolFromXml(bagNode,"pr_cabin",     false);
    bag.amount         = getIntFromXml(bagNode, "amount",       ASTRA::NoExists);
    bag.weight         = getIntFromXml(bagNode, "weight",       ASTRA::NoExists);
    bag.bag_pool_num   = getIntFromXml(bagNode, "bag_pool_num", ASTRA::NoExists);
    bag.airp_arv_final = getStrFromXml(bagNode, "airp_arv_final");
    return bag;
}

std::list<XmlBag> XmlEntityReader::readBags(xmlNodePtr bagsNode)
{
    ASSERT(bagsNode);

    std::list<XmlBag> bags;
    for(xmlNodePtr bagNode = bagsNode->children;
        bagNode != NULL; bagNode = bagNode->next)
    {
        bags.push_back(XmlEntityReader::readBag(bagNode));
    }
    return bags;
}

XmlBagTag XmlEntityReader::readBagTag(xmlNodePtr bagTagNode)
{
    ASSERT(bagTagNode);

    XmlBagTag bagTag;

    bagTag.num           = getIntFromXml(bagTagNode, "num",      ASTRA::NoExists);
    bagTag.tag_type      = getStrFromXml(bagTagNode, "tag_type");
    std::string bagTagNo = getStrFromXml(bagTagNode, "no");
    bagTag.no            = std::strtoull(bagTagNo.c_str(), NULL, 10);
    bagTag.bag_num       = getIntFromXml(bagTagNode, "bag_num",  ASTRA::NoExists);
    bagTag.pr_print      = getBoolFromXml(bagTagNode,"pr_print", false);
    return bagTag;
}

std::list<XmlBagTag> XmlEntityReader::readBagTags(xmlNodePtr bagTagsNode)
{
    ASSERT(bagTagsNode);

    std::list<XmlBagTag> bagTags;
    for(xmlNodePtr bagTagNode = bagTagsNode->children;
        bagTagNode != NULL; bagTagNode = bagTagNode->next)
    {
        bagTags.push_back(XmlEntityReader::readBagTag(bagTagNode));
    }
    return bagTags;
}

XmlPlaceLayer XmlEntityReader::readPlaceLayer(xmlNodePtr layerNode)
{
    ASSERT(layerNode);

    XmlPlaceLayer layer;
    layer.layer_type = getStrFromXml(layerNode, "layer_type");
    return layer;
}

std::list<XmlPlaceLayer> XmlEntityReader::readPlaceLayers(xmlNodePtr layersNode)
{
    ASSERT(layersNode);

    std::list<XmlPlaceLayer> layers;
    for(xmlNodePtr layerNode = layersNode->children;
        layerNode != NULL; layerNode = layerNode->next)
    {
        layers.push_back(XmlEntityReader::readPlaceLayer(layerNode));
    }
    return layers;
}

XmlPlace XmlEntityReader::readPlace(xmlNodePtr placeNode)
{
    ASSERT(placeNode);

    XmlPlace place;
    place.x         = getIntFromXml(placeNode, "x", ASTRA::NoExists);
    place.y         = getIntFromXml(placeNode, "y", ASTRA::NoExists);
    place.elem_type = getStrFromXml(placeNode, "elem_type");
    place.cls       = getStrFromXml(placeNode, "class");
    place.xname     = getStrFromXml(placeNode, "xname");
    place.yname     = getStrFromXml(placeNode, "yname");

    xmlNodePtr layersNode = findNode(placeNode, "layers");
    if(layersNode != NULL) {
        place.layers    = readPlaceLayers(layersNode);
    }
    return place;
}

std::list<XmlPlace> XmlEntityReader::readPlaces(xmlNodePtr placesNode)
{
    ASSERT(placesNode);

    std::list<XmlPlace> places;
    for(xmlNodePtr placeNode = placesNode->children;
        placeNode != NULL; placeNode = placeNode->next)
    {
        places.push_back(XmlEntityReader::readPlace(placeNode));
    }
    return places;
}

XmlPlaceList XmlEntityReader::readPlaceList(xmlNodePtr placelistNode)
{
    ASSERT(placelistNode);

    XmlPlaceList placelist;
    placelist.num    = getIntPropFromXml(placelistNode, "num", ASTRA::NoExists);
    placelist.xcount = getIntPropFromXml(placelistNode, "xcount", ASTRA::NoExists);
    placelist.ycount = getIntPropFromXml(placelistNode, "ycount", ASTRA::NoExists);
    placelist.places = readPlaces(placelistNode);
    return placelist;
}

std::list<XmlPlaceList> XmlEntityReader::readPlaceLists(xmlNodePtr salonsNode)
{
    ASSERT(salonsNode);

    std::list<XmlPlaceList> placelists;
    for(xmlNodePtr node = salonsNode->children;
        node != NULL; node = node->next)
    {
        if(!strcmp((const char*)node->name, "placelist")) {
            placelists.push_back(readPlaceList(node));
        } else {
            LogTrace(TRACE5) << "Node '" << (const char*)node->name << "' is not a 'placelist' node! Skip it...";
        }
    }

    return placelists;
}

XmlFilterRouteItem XmlEntityReader::readFilterRouteItem(xmlNodePtr itemNode)
{
    XmlFilterRouteItem item;
    item.point_id = getIntFromXml(itemNode, "point_id", ASTRA::NoExists);
    item.airp     = getStrFromXml(itemNode, "airp");
    return item;
}

std::list<XmlFilterRouteItem> XmlEntityReader::readFilterRouteItems(xmlNodePtr itemsNode)
{
    std::list<XmlFilterRouteItem> items;
    for(xmlNodePtr node = itemsNode->children;
        node != NULL; node = node->next)
    {
        items.push_back(readFilterRouteItem(node));
    }
    return items;
}

XmlFilterRoutes XmlEntityReader::readFilterRoutes(xmlNodePtr filterRoutesNode)
{
    XmlFilterRoutes filterRoutes;
    filterRoutes.point_dep = getIntFromXml(filterRoutesNode, "point_dep", ASTRA::NoExists);
    filterRoutes.point_arv = getIntFromXml(filterRoutesNode, "point_arv", ASTRA::NoExists);
    filterRoutes.items     = readFilterRouteItems(findNode(filterRoutesNode, "items"));
    return filterRoutes;
}

//---------------------------------------------------------------------------------------

xmlNodePtr XmlEntityViewer::viewMarkFlight(xmlNodePtr node, const XmlMarkFlight& markFlight)
{
    xmlNodePtr markFlightNode = NewTextChild(node, "mark_flight");
    NewTextChild(markFlightNode, "airline",       markFlight.airline);
    NewTextChild(markFlightNode, "flt_no",        markFlight.flt_no);
    NewTextChild(markFlightNode, "suffix",        markFlight.suffix);
    NewTextChild(markFlightNode, "scd",           DateTimeToStr(markFlight.scd, ServerFormatDateTimeAsString));
    NewTextChild(markFlightNode, "airp_dep",      markFlight.airp_dep);
    NewTextChild(markFlightNode, "pr_mark_norms", markFlight.pr_mark_norms);
    return markFlightNode;
}

xmlNodePtr XmlEntityViewer::viewRem(xmlNodePtr node, const XmlRem& rem)
{
    xmlNodePtr remNode = NewTextChild(node, "rem");
    NewTextChild(remNode, "rem_code", rem.rem_code);
    NewTextChild(remNode, "rem_text", rem.rem_text);
    return remNode;
}

xmlNodePtr XmlEntityViewer::viewRems(xmlNodePtr node, const XmlRems& rems)
{
    xmlNodePtr remsNode = NewTextChild(node, "rems");
    for(const XmlRem& rem: rems.rems) {
        XmlEntityViewer::viewRem(remsNode, rem);
    }
    return remsNode;
}

xmlNodePtr XmlEntityViewer::viewFqtRem(xmlNodePtr node, const XmlFqtRem& rem)
{
    xmlNodePtr remNode = NewTextChild(node, "fqt_rem");
    NewTextChild(remNode, "rem_code", rem.rem_code);
    NewTextChild(remNode, "airline",  rem.airline);
    NewTextChild(remNode, "no",       rem.no);
    if(!rem.tier_level.empty()) {
        NewTextChild(remNode, "tier_level", rem.tier_level);
    }
    return remNode;
}

xmlNodePtr XmlEntityViewer::viewFqtRems(xmlNodePtr node, const boost::optional<XmlFqtRems>& rems)
{
    xmlNodePtr remsNode = NewTextChild(node, "fqt_rems");
    if(!rems) return remsNode;

    for(const XmlFqtRem& rem: rems->rems) {
        XmlEntityViewer::viewFqtRem(remsNode, rem);
    }
    return remsNode;
}

xmlNodePtr XmlEntityViewer::viewDoc(xmlNodePtr node, const XmlPaxDoc& doc)
{
    xmlNodePtr docNode = NewTextChild(node, "document");
    NewTextChild(docNode, "type",          doc.type);
    NewTextChild(docNode, "issue_country", doc.issue_country);
    NewTextChild(docNode, "no",            doc.no);
    NewTextChild(docNode, "nationality",   doc.nationality);
    NewTextChild(docNode, "birth_date",    doc.birth_date);
    NewTextChild(docNode, "expiry_date",   doc.expiry_date);
    NewTextChild(docNode, "gender",        doc.gender);
    NewTextChild(docNode, "surname",       doc.surname);
    NewTextChild(docNode, "first_name",    doc.first_name);
    NewTextChild(docNode, "second_name",   doc.second_name);
    return docNode;
}

xmlNodePtr XmlEntityViewer::viewAddress(xmlNodePtr node, const XmlPaxAddress& addr)
{
    xmlNodePtr addrNode = NewTextChild(node, "doca");
    NewTextChild(addrNode, "type",        addr.type);
    NewTextChild(addrNode, "country",     addr.country);
    NewTextChild(addrNode, "address",     addr.address);
    NewTextChild(addrNode, "city",        addr.city);
    NewTextChild(addrNode, "region",      addr.region);
    NewTextChild(addrNode, "postal_code", addr.postal_code);
    return addrNode;
}

xmlNodePtr XmlEntityViewer::viewAddresses(xmlNodePtr node, const XmlPaxAddresses& addrs)
{
    xmlNodePtr addrsNode = NewTextChild(node, "addresses");
    for(const XmlPaxAddress& addr: addrs.addresses) {
        XmlEntityViewer::viewAddress(addrsNode, addr);
    }
    return addrsNode;
}

xmlNodePtr XmlEntityViewer::viewPax(xmlNodePtr node, const XmlPax& pax)
{
    xmlNodePtr paxNode = NewTextChild(node, "pax");
    NewTextChild(paxNode, "pax_id",         pax.pax_id);
    NewTextChild(paxNode, "surname",        pax.surname);
    NewTextChild(paxNode, "name",           pax.name);
    NewTextChild(paxNode, "pers_type",      pax.pers_type);
    NewTextChild(paxNode, "seat_no",        pax.seat_no);
    NewTextChild(paxNode, "preseat_no");
    NewTextChild(paxNode, "seat_type");
    NewTextChild(paxNode, "seats",          pax.seats);
    NewTextChild(paxNode, "ticket_no",      pax.ticket_no);
    NewTextChild(paxNode, "coupon_no",      pax.coupon_no);
    NewTextChild(paxNode, "ticket_rem",     pax.ticket_rem);
    NewTextChild(paxNode, "ticket_confirm", 0);
    NewTextChild(paxNode, "subclass",       pax.subclass);
    if(pax.bag_pool_num != ASTRA::NoExists) {
        NewTextChild(paxNode, "bag_pool_num", pax.bag_pool_num);
    } else {
        NewTextChild(paxNode, "bag_pool_num");
    }

    /*xmlNodePtr trferNode = */NewTextChild(paxNode, "transfer");
//    xmlNodePtr trferSegNode = NewTextChild(trferNode, "segment");
//    NewTextChild(trferSegNode, "subclass", pax.subclass);
    if(!pax.refuse.empty()) {
        NewTextChild(paxNode, "refuse", pax.refuse);
    } else {
        NewTextChild(paxNode, "refuse"); // без этого, как минимум, не обновлялась инф-я о документе (паспорте)
    }
    if(pax.tid != ASTRA::NoExists) {
        NewTextChild(paxNode, "tid",      pax.tid);
    }

    NewTextChild(paxNode, "doco");      // TODO отрисовать визы
    NewTextChild(paxNode, "addresses"); // TODO отрисовать адреса

    return paxNode;
}

xmlNodePtr XmlEntityViewer::viewSegInfo(xmlNodePtr segNode, const XmlSegmentInfo& seg_info)
{
    NewTextChild(segNode, "point_dep", seg_info.point_dep);
    NewTextChild(segNode, "point_arv", seg_info.point_arv);
    NewTextChild(segNode, "airp_dep",  seg_info.airp_dep);
    NewTextChild(segNode, "airp_arv",  seg_info.airp_arv);
    NewTextChild(segNode, "class",     seg_info.cls);
    NewTextChild(segNode, "status",    seg_info.status);
    NewTextChild(segNode, "wl_type");
    if(seg_info.grp_id != ASTRA::NoExists) {
        NewTextChild(segNode, "grp_id", seg_info.grp_id);
    }
    if(seg_info.tid != ASTRA::NoExists) {
        NewTextChild(segNode, "tid", seg_info.tid);
    }
    return segNode;
}

xmlNodePtr XmlEntityViewer::viewSeg(xmlNodePtr node, const XmlSegment& seg)
{
    xmlNodePtr segNode = NewTextChild(node, "segment");
    XmlEntityViewer::viewSegInfo(segNode, seg.seg_info);
    return segNode;
}

xmlNodePtr XmlEntityViewer::viewBag(xmlNodePtr node, const XmlBag& bag)
{
    xmlNodePtr bagNode = NewTextChild(node, "bag");
    NewTextChild(bagNode, "bag_type",      bag.bag_type);
    NewTextChild(bagNode, "airline",       bag.airline);
    NewTextChild(bagNode, "num",           bag.num);
    NewTextChild(bagNode, "pr_cabin",      bag.pr_cabin ? 1 : 0);
    NewTextChild(bagNode, "amount",        bag.amount);
    NewTextChild(bagNode, "weight",        bag.weight);
    NewTextChild(bagNode, "value_bag_num");
    NewTextChild(bagNode, "pr_liab_limit", 0);
    NewTextChild(bagNode, "to_ramp",       0);
    NewTextChild(bagNode, "using_scales",  0);
    NewTextChild(bagNode, "is_trfer",      1);
    NewTextChild(bagNode, "bag_pool_num",  bag.bag_pool_num);
    return bagNode;
}

xmlNodePtr XmlEntityViewer::viewBagsHeader(xmlNodePtr node)
{
    xmlNodePtr bagsNode = NewTextChild(node, "bags");
    return bagsNode;
}

xmlNodePtr XmlEntityViewer::viewBags(xmlNodePtr node, const XmlBags& bags)
{
    xmlNodePtr bagsNode = viewBagsHeader(node);
    for(const XmlBag& bag: bags.bags) {
        XmlEntityViewer::viewBag(bagsNode, bag);
    }

    return bagsNode;
}

xmlNodePtr XmlEntityViewer::viewBagTag(xmlNodePtr node, const XmlBagTag& tag)
{
    xmlNodePtr bagTagNode = NewTextChild(node, "tag");
    NewTextChild(bagTagNode, "num",      tag.num);
    NewTextChild(bagTagNode, "tag_type", tag.tag_type);
    NewTextChild(bagTagNode, "no",       FloatToString(tag.no, 0));
    NewTextChild(bagTagNode, "color");
    NewTextChild(bagTagNode, "bag_num",  tag.bag_num);
    NewTextChild(bagTagNode, "pr_print", tag.pr_print ? 1 : 0);
    return bagTagNode;
}

xmlNodePtr XmlEntityViewer::viewBagTagsHeader(xmlNodePtr node)
{
    xmlNodePtr bagTagsNode = NewTextChild(node, "tags");
    xmlSetProp(bagTagsNode, "pr_print", "0");
    return bagTagsNode;
}

xmlNodePtr XmlEntityViewer::viewBagTags(xmlNodePtr node, const XmlBagTags& tags)
{
    xmlNodePtr bagTagsNode = viewBagTagsHeader(node);
    for(const XmlBagTag& tag: tags.bagTags) {
        XmlEntityViewer::viewBagTag(bagTagsNode, tag);
    }

    return bagTagsNode;
}

xmlNodePtr XmlEntityViewer::viewServiveList(xmlNodePtr node, const XmlServiceList& svcList)
{
    xmlNodePtr svcListNode = NewTextChild(node, "service_list");
    if(svcList.seg_no != ASTRA::NoExists) {
        xmlSetProp(svcListNode, "seg_no", std::to_string(svcList.seg_no));
    }
    if(svcList.category != ASTRA::NoExists) {
        xmlSetProp(svcListNode, "category", std::to_string(svcList.category));
    }
    if(svcList.list_id != ASTRA::NoExists) {
        xmlSetProp(svcListNode, "list_id", std::to_string(svcList.list_id));
    }
    return svcListNode;
}

//---------------------------------------------------------------------------------------

SearchPaxXmlResult::SearchPaxXmlResult(xmlNodePtr node)
{
    xmlNodePtr tripsNode = findNode(node, "trips");
    if(tripsNode != NULL) {
        this->lTrip = XmlEntityReader::readTrips(tripsNode);
    }
}

std::list<XmlTrip> SearchPaxXmlResult::applyNameFilter(const std::string& surname,
                                                       const std::string& name) const
{
    std::list<XmlTrip> res;
    for(const XmlTrip& trip: lTrip) {
        bool tripSuitable = false;
        for(const XmlPnr& pnr: trip.pnrs) {
            std::list<XmlPax> paxes = pnr.applyNameFilter(surname, name);
            for(const XmlPax& pax: paxes) {
                if(pax.equalName(surname, name)) {
                    tripSuitable = true;
                    break;
                }
            }
        }

        if(tripSuitable) {
            res.push_back(trip);
        }
    }

    return res;
}

//---------------------------------------------------------------------------------------

LoadPaxXmlResult::LoadPaxXmlResult(xmlNodePtr node)
{
    xmlNodePtr segsNode = findNode(node, "segments");
    if(segsNode != NULL) {
        this->lSeg = XmlEntityReader::readSegs(segsNode);
    }

    xmlNodePtr bagsNode = findNode(node, "bags");
    if(bagsNode != NULL) {
        this->lBag = XmlEntityReader::readBags(bagsNode);
    }

    xmlNodePtr bagTagsNode = findNode(node, "tags");
    if(bagTagsNode != NULL) {
        this->lBagTag = XmlEntityReader::readBagTags(bagTagsNode);
    }

    finalizeBags();
    finalizeBagTags();
}

LoadPaxXmlResult::LoadPaxXmlResult(const std::list<XmlSegment>& lSeg,
                                   const std::list<XmlBag>& lBag,
                                   const std::list<XmlBagTag>& lBagTag)
{
    this->lSeg    = lSeg;
    this->lBag    = lBag;
    this->lBagTag = lBagTag;
}

void LoadPaxXmlResult::finalizeBags()
{
    for(auto& bag: lBag) {
        if(bag.bag_pool_num != ASTRA::NoExists) {
            for(const auto& p: lSeg.front().passengers) {
                if(p.bag_pool_num == bag.bag_pool_num) {
                    bag.pax_id = p.pax_id;
                }
            }
        }
    }
}

void LoadPaxXmlResult::finalizeBagTags()
{
    for(auto& bagTag: lBagTag) {
        for(const auto& p: lSeg.front().passengers) {
            for(const XmlBag& bag: lBag) {
                if(bagTag.bag_num == bag.num) {
                    if(bag.pax_id == p.pax_id) {
                        bagTag.pax_id = bag.pax_id;
                    }
                }
            }
        }
    }
}

static std::list<astra_entities::PaxInfo> convertNotInfants(const std::list<XmlPax>& lPax)
{
    std::list<astra_entities::PaxInfo> lNotInfants;
    for(const XmlPax& xmlPax: lPax) {
        astra_entities::PaxInfo paxInfo = xmlPax.toPax();
        if(!paxInfo.isInfant()) {
            lNotInfants.push_back(paxInfo);
        }
    }
    return lNotInfants;
}

static std::list<astra_entities::PaxInfo> convertInfants(const std::list<XmlPax>& lPax)
{
    std::list<astra_entities::PaxInfo> lInfants;
    for(const XmlPax& xmlPax: lPax) {
        astra_entities::PaxInfo paxInfo = xmlPax.toPax();
        if(paxInfo.isInfant()) {
            lInfants.push_back(paxInfo);
        }
    }
    return lInfants;
}

static boost::optional<astra_entities::PaxInfo> findInfant(const std::list<astra_entities::PaxInfo>& lInfants,
                                                           const astra_entities::Remark& ssrInft)
{
    for(const auto& infant: lInfants) {
        if(ssrInft.containsText(infant.fullName())) {
            return infant;
        }
    }
    return boost::none;
}

//---------------------------------------------------------------------------------------

PaxFilter::PaxFilter(const boost::optional<NameFilter>& nameFilter,
                                 const boost::optional<TicknumFilter>& ticknumFilter,
                                 const boost::optional<IdFilter>& idFilter)
    : m_nameFilter(nameFilter),
      m_ticknumFilter(ticknumFilter),
      m_idFilter(idFilter)
{
    ASSERT(m_nameFilter || m_ticknumFilter || m_idFilter);

    LogTrace(TRACE3) << "name filter: ";
    if(m_nameFilter) {
        LogTrace(TRACE3) << nameFilter->m_surname << "/" << nameFilter->m_name;
    }

    LogTrace(TRACE3) << "ticknum filter: ";
    if(m_ticknumFilter) {
        LogTrace(TRACE3) << ticknumFilter->m_ticknum;
    }

    LogTrace(TRACE3) << "id filter: ";
    if(m_idFilter) {
        LogTrace(TRACE3) << idFilter->m_id;
    }
}

bool PaxFilter::operator()(const XmlPax& pax) const
{
    return ((m_nameFilter && (*m_nameFilter)(pax)) || !m_nameFilter) &&
           ((m_ticknumFilter && (*m_ticknumFilter)(pax)) || !m_ticknumFilter) &&
           ((m_idFilter && (*m_idFilter)(pax)) || !m_idFilter);
}

//---------------------------------------------------------------------------------------

std::vector<iatci::dcrcka::Result> LoadPaxXmlResult::toIatci(iatci::dcrcka::Result::Action_e action,
                                                             iatci::dcrcka::Result::Status_e status) const
{
    std::vector<iatci::dcrcka::Result> lRes;
    for(auto& seg: lSeg)
    {
        iatci::FlightDetails flight = iatci::makeFlight(seg);
        std::list<iatci::dcrcka::PaxGroup> paxGroups;
        std::list<astra_entities::PaxInfo> lNonInfants = convertNotInfants(seg.passengers);
        std::list<astra_entities::PaxInfo> lInfants = convertInfants(seg.passengers);

        for(const auto& paxInfo: lNonInfants) {
            LogTrace(TRACE3) << "current pax " << paxInfo.m_surname << "/" << paxInfo.m_name;

            boost::optional<astra_entities::PaxInfo> inft;

            boost::optional<iatci::PaxDetails> infant;
            boost::optional<iatci::DocDetails> infantDoc;
            boost::optional<iatci::AddressDetails> infantAddress;
            boost::optional<iatci::FlightSeatDetails> infantSeat;
            boost::optional<astra_entities::Remark> ssrInft = paxInfo.ssrInft();
            if(ssrInft) {
                inft = findInfant(lInfants, *ssrInft);
                if(inft) {
                    infant = iatci::makeRespPax(*inft);
                    infantDoc = iatci::makeDoc(*inft);
                    infantAddress = iatci::makeAddress(*inft);
                    infantSeat = iatci::makeFlightSeat(*inft);
                }
            }

            std::list<astra_entities::BagPool> bags, handBags;
            for(const auto& b: lBag) {
                if(b.pr_cabin) {
                    handBags.push_back(astra_entities::BagPool(b.bag_pool_num,
                                                               b.amount,
                                                               b.weight));
                } else {
                    bags.push_back(astra_entities::BagPool(b.bag_pool_num,
                                                           b.amount,
                                                           b.weight));
                }
            }

            std::list<astra_entities::BaggageTag> bagTags;
            for(const auto& bt: lBagTag) {
                if(bt.pax_id == paxInfo.id() || (inft && bt.pax_id == inft->id())) {
                    bagTags.push_back(astra_entities::BaggageTag(bt.no,
                                                                 1,
                                                                 flight.arrPort())); //TODO
                }
            }

            paxGroups.push_back(iatci::dcrcka::PaxGroup(iatci::makeRespPax(paxInfo, inft),
                                                        iatci::makeReserv(paxInfo),
                                                        iatci::makeFlightSeat(paxInfo),
                                                        iatci::makeBaggage(paxInfo, bags, handBags, bagTags),
                                                        iatci::makeService(paxInfo, inft),
                                                        iatci::makeDoc(paxInfo),
                                                        iatci::makeAddress(paxInfo),
                                                        infant,
                                                        infantDoc,
                                                        infantAddress,
                                                        infantSeat));
        }

        lRes.push_back(iatci::dcrcka::Result::makeResult(action,
                                                         status,
                                                         flight,
                                                         paxGroups,
                                                         boost::none,
                                                         boost::none,
                                                         boost::none,
                                                         boost::none,
                                                         boost::none));
    }

    return lRes;
}

iatci::dcrcka::Result LoadPaxXmlResult::toIatciFirst(iatci::dcrcka::Result::Action_e action,
                                                     iatci::dcrcka::Result::Status_e status) const
{
    std::vector<iatci::dcrcka::Result> lRes = toIatci(action, status);
    ASSERT(!lRes.empty());
    return lRes.front();
}

void LoadPaxXmlResult::applyPaxFilter(const PaxFilter& filter)
{
    ASSERT(!lSeg.empty());
    std::list<XmlPax> res;
    for(const XmlPax& pax: lSeg.front().passengers) {
        if(filter(pax)) {
            res.push_back(pax);
        }
    }
    lSeg.front().passengers = res;
}

//---------------------------------------------------------------------------------------

PaxListXmlResult::PaxListXmlResult(xmlNodePtr node)
{
    xmlNodePtr paxesNode = findNode(node, "passengers");
    if(paxesNode != NULL) {
        this->lPax = XmlEntityReader::readPaxes(paxesNode);
    }
}

std::list<XmlPax> PaxListXmlResult::applyNameFilter(const std::string& surname,
                                                    const std::string& name) const
{
    std::list<XmlPax> res;
    for(const XmlPax& pax: lPax) {
        if(pax.equalName(surname, name)) {
            res.push_back(pax);
        }
    }
    return res;
}

std::list<XmlPax> PaxListXmlResult::applyFilters(const PaxFilter& filter) const
{
    std::list<XmlPax> res;
    for(const XmlPax& pax: lPax) {
        if(filter(pax)) {
            res.push_back(pax);
        }
    }
    return res;
}

//---------------------------------------------------------------------------------------

GetAdvTripListXmlResult::GetAdvTripListXmlResult(xmlNodePtr node)
{
    xmlNodePtr tripsNode = findNode(node, "trips");
    if(tripsNode != NULL) {
        this->lTrip = XmlEntityReader::readTrips(tripsNode);
    }
}

std::list<XmlTrip> GetAdvTripListXmlResult::applyFlightFilter(const std::string& flightName) const
{
    std::list<XmlTrip> res;
    for(const XmlTrip& trip: lTrip) {
        if(flightName == trip.name) {
            res.push_back(trip);
        }
    }

    return res;
}

//---------------------------------------------------------------------------------------

GetSeatmapXmlResult::GetSeatmapXmlResult(xmlNodePtr node)
{
    trip  = getStrFromXml(node, "trip");
    craft = getStrFromXml(node, "craft");
    xmlNodePtr salonsNode = findNodeR(node, "salons");
    if(salonsNode != NULL) {
        lPlacelist   = XmlEntityReader::readPlaceLists(salonsNode);
        xmlNodePtr filterRoutesNode = findNodeR(salonsNode, "filterRoutes");
        if(filterRoutesNode != NULL) {
            filterRoutes = XmlEntityReader::readFilterRoutes(filterRoutesNode);
        }
    }
}

iatci::dcrcka::Result GetSeatmapXmlResult::toIatci(const iatci::FlightDetails& outbFlt) const
{
    if(lPlacelist.empty() || filterRoutes.items.empty()) {
        tst();
        throw tick_soft_except(STDLOG, AstraErr::INV_FLIGHT_DATE);
    }

    return iatci::dcrcka::Result::makeSeatmapResult(iatci::dcrcka::Result::Ok,
                                                    outbFlt,
                                                    createSeatmapDetails(lPlacelist));

}

}//namespace xml_entities

/////////////////////////////////////////////////////////////////////////////////////////

namespace astra_entities {

MarketingInfo::MarketingInfo(const std::string& airline,
                             unsigned flightNum,
                             const std::string& flightSuffix,
                             const boost::gregorian::date& scdDepDate,
                             const std::string& airpDep)
    : m_airline(airline),
      m_flightNum(flightNum),
      m_flightSuffix(flightSuffix),
      m_scdDepDate(scdDepDate),
      m_airpDep(airpDep)
{}

//---------------------------------------------------------------------------------------

SegmentInfo::SegmentInfo(int grpId,
                         int pointDep,
                         int pointArv,
                         const std::string& airpDep,
                         const std::string& airpArv,
                         const std::string& cls,
                         const MarketingInfo& markFlight)
    : m_grpId(grpId),
      m_pointDep(pointDep),
      m_pointArv(pointArv),
      m_airpDep(airpDep),
      m_airpArv(airpArv),
      m_cls(cls),
      m_markFlight(markFlight)
{}

//---------------------------------------------------------------------------------------

Remark::Remark(const std::string& remCode,
               const std::string& remText)
    : m_remCode(remCode),
      m_remText(remText)
{}

bool Remark::containsText(const std::string& text) const
{
    return m_remText.find(text) != std::string::npos;
}

bool operator==(const Remark& left, const Remark& right)
{
    return (left.m_remCode == right.m_remCode &&
            left.m_remText == right.m_remText);
}

bool operator!=(const Remark& left, const Remark& right)
{
    return !(left == right);
}

//---------------------------------------------------------------------------------------

FqtRemark::FqtRemark(const std::string& remCode,
                     const std::string& airline,
                     const std::string& fqtNo,
                     const std::string& tierLevel)
    : m_remCode(remCode),
      m_airline(airline),
      m_fqtNo(fqtNo),
      m_tierLevel(tierLevel)
{}

bool operator==(const FqtRemark& left, const FqtRemark& right)
{
    return (left.m_remCode   == right.m_remCode &&
            left.m_airline   == right.m_airline &&
            left.m_fqtNo     == right.m_fqtNo &&
            left.m_tierLevel == right.m_tierLevel);
}

bool operator!=(const FqtRemark& left, const FqtRemark& right)
{
    return !(left == right);
}

//---------------------------------------------------------------------------------------

DocInfo::DocInfo(const std::string& type,
                 const std::string& country,
                 const std::string& num,
                 const boost::gregorian::date& expiryDate,
                 const std::string& surname,
                 const std::string& name,
                 const std::string& secName,
                 const std::string& citizenship,
                 const boost::gregorian::date& birthDate,
                 const std::string& gender)
    : m_type(type),
      m_country(country),
      m_num(num),
      m_expiryDate(expiryDate),
      m_surname(surname),
      m_name(name),
      m_secName(secName),
      m_citizenship(citizenship),
      m_birthDate(birthDate),
      m_gender(gender)
{}

bool operator==(const DocInfo& left, const DocInfo& right)
{
    return (left.m_type        == right.m_type &&
            left.m_country     == right.m_country &&
            left.m_num         == right.m_num &&
            left.m_expiryDate  == right.m_expiryDate &&
            left.m_surname     == right.m_surname &&
            left.m_name        == right.m_name &&
            left.m_secName     == right.m_secName &&
            left.m_citizenship == right.m_citizenship &&
            left.m_birthDate   == right.m_birthDate &&
            left.m_gender      == right.m_gender);
}

bool operator!=(const DocInfo& left, const DocInfo& right)
{
    return !(left == right);
}

//---------------------------------------------------------------------------------------

AddressInfo::AddressInfo(const std::string& type,
                         const std::string& country,
                         const std::string& address,
                         const std::string& city,
                         const std::string& region,
                         const std::string& postalCode)
    : m_type(type),
      m_country(country),
      m_address(address),
      m_city(city),
      m_region(region),
      m_postalCode(postalCode)
{}

bool AddressInfo::isEmpty() const
{
    return (m_country.empty() &&
            m_address.empty() &&
            m_city.empty()    &&
            m_region.empty()  &&
            m_postalCode.empty());
}

bool operator==(const AddressInfo& left, const AddressInfo& right)
{
    return (left.m_type       == right.m_type &&
            left.m_country    == right.m_country &&
            left.m_address    == right.m_address &&
            left.m_city       == right.m_city &&
            left.m_region     == right.m_region &&
            left.m_postalCode == right.m_postalCode);
}

bool operator!=(const AddressInfo& left, const AddressInfo& right)
{
    return !(left == right);
}

//---------------------------------------------------------------------------------------

bool operator==(const Addresses& left, const Addresses& right)
{
    return (left.m_lAddrs == right.m_lAddrs);
}

bool operator!=(const Addresses& left, const Addresses& right)
{
    return !(left == right);
}

//---------------------------------------------------------------------------------------

bool operator==(const VisaInfo& left, const VisaInfo& right)
{
    return true; // TODO
}

bool operator!=(const VisaInfo& left, const VisaInfo& right)
{
    return !(left == right);
}

//---------------------------------------------------------------------------------------

bool operator==(const Remarks& left, const Remarks& right)
{
    return (left.m_lRems == right.m_lRems);
}

bool operator!=(const Remarks& left, const Remarks& right)
{
    return !(left == right);
}

//---------------------------------------------------------------------------------------

bool operator==(const FqtRemarks& left, const FqtRemarks& right)
{
    return (left.m_lFqtRems == right.m_lFqtRems);
}

bool operator!=(const FqtRemarks& left, const FqtRemarks& right)
{
    return !(left == right);
}

//---------------------------------------------------------------------------------------

PaxInfo::PaxInfo(int paxId,
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
                 const boost::optional<Remarks>& rems,
                 const boost::optional<FqtRemarks>& fqtRems,
                 int bagPoolNum,
                 int iatciParentId)
    : m_paxId(paxId),
      m_surname(surname),
      m_name(name),
      m_persType(persType),
      m_ticketNum(ticketNum),
      m_couponNum(couponNum),
      m_ticketRem(ticketRem),
      m_seatNo(seatNo),
      m_regNo(regNo),
      m_iatciPaxId(iatciPaxId),
      m_subclass(subclass),
      m_doc(doc),
      m_addrs(addrs),
      m_rems(rems),
      m_fqtRems(fqtRems),
      m_bagPoolNum(bagPoolNum),
      m_iatciParentId(iatciParentId)
{}

boost::optional<Remark> PaxInfo::ssrInft() const
{
    if(!m_rems) {
        return boost::none;
    }

    for(const auto& rem: m_rems->m_lRems) {
        if(rem.m_remCode == "INFT") {
            return rem;
        }
    }
    return boost::none;
}

std::string PaxInfo::fullName() const
{
    return m_surname + "/" + m_name;
}

Ticketing::TicketNum_t PaxInfo::tickNum() const
{
    return Ticketing::TicketNum_t(m_ticketNum);
}

std::string PaxInfo::theirId() const
{
    return m_iatciPaxId;
}

std::string PaxInfo::ourId() const
{
    if(m_paxId <= 0) return "";

    std::ostringstream os;
    os << std::setw(10) << std::setfill('0') << m_paxId;
    return os.str();
}

bool operator==(const PaxInfo& left, const PaxInfo& right)
{
    return (left.m_paxId         == right.m_paxId &&
            left.m_surname       == right.m_surname &&
            left.m_name          == right.m_name &&
            left.m_persType      == right.m_persType &&
            left.m_ticketNum     == right.m_ticketNum &&
            left.m_couponNum     == right.m_couponNum &&
            left.m_ticketRem     == right.m_ticketRem &&
            left.m_seatNo        == right.m_seatNo &&
            left.m_regNo         == right.m_regNo &&
            left.m_iatciPaxId    == right.m_iatciPaxId &&
            left.m_subclass      == right.m_subclass &&
            left.m_doc           == right.m_doc &&
            left.m_addrs         == right.m_addrs &&
            left.m_visa          == right.m_visa &&
            left.m_rems          == right.m_rems &&
            left.m_fqtRems       == right.m_fqtRems &&
            left.m_bagPoolNum    == right.m_bagPoolNum &&
            left.m_iatciParentId == right.m_iatciParentId);
}

bool operator!=(const PaxInfo& left, const PaxInfo& right)
{
    return !(left == right);
}

//---------------------------------------------------------------------------------------

BagPool::BagPool(int poolNum, int amount, int weight)
    : m_poolNum(poolNum),
      m_amount(amount),
      m_weight(weight)
{}

BagPool BagPool::operator+(const BagPool& pool)
{
    return BagPool(this->m_poolNum,
                   this->m_amount + pool.m_amount,
                   this->m_weight + pool.m_weight);
}

BagPool& BagPool::operator+=(const BagPool& pool)
{
    this->m_amount += pool.m_amount;
    this->m_weight += pool.m_weight;
    return *this;
}

//---------------------------------------------------------------------------------------

BaggageTag::BaggageTag(uint64_t fullTag,
                       unsigned numOfConsecSerial,
                       const std::string& dest)
    : m_fullTag(fullTag),
      m_numOfConsecSerial(numOfConsecSerial),
      m_destination(dest)
{
    m_carrierCode = awkByAccode(std::to_string(iatci::getTagAccodeByTag(fullTag)))->code(/*lang*/);
}

BaggageTag::BaggageTag(const std::string& carrierCode,
                       uint64_t fullTag,
                       unsigned numOfConsecSerial,
                       const std::string& dest)
    : m_carrierCode(carrierCode),
      m_fullTag(fullTag),
      m_numOfConsecSerial(numOfConsecSerial),
      m_destination(dest)
{}

//---------------------------------------------------------------------------------------

void Baggage::addPool(const BagPool& p, bool handLuggage)
{
    if(handLuggage) {
        addHandPool(p);
    } else {
        addPool(p);
    }
}

void Baggage::addPool(const BagPool& p)
{
    m_bagPools.insert(std::make_pair(p.m_poolNum, p));
    m_poolNums.insert(p.m_poolNum);
}

void Baggage::addHandPool(const BagPool& p)
{
    m_handBagPools.insert(std::make_pair(p.m_poolNum, p));
    m_poolNums.insert(p.m_poolNum);
}

static BagPool getTotalByPoolNum(const std::multimap<int, BagPool>& bagPools, int poolNum)
{
    BagPool total(poolNum);
    auto range = bagPools.equal_range(poolNum);
    for(auto it = range.first; it != range.second; ++it) {
        total += it->second;
    }
    return total;
}

BagPool Baggage::totalByPoolNum(int poolNum) const
{
    return getTotalByPoolNum(m_bagPools, poolNum);
}

BagPool Baggage::totalByHandPoolNum(int poolNum) const
{
    return getTotalByPoolNum(m_handBagPools, poolNum);
}

const std::set<int>& Baggage::poolNums() const
{
    return m_poolNums;
}

}//namespace astra_entities

}//namespace astra_api
