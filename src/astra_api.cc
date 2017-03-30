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

XmlPax createCheckInPax(const XmlPax& basePax,
                        const std::string& seatNo,
                        const std::string& subclass)
{
    XmlPax ckiPax = basePax;
    ckiPax.pers_type = "ВЗ"; // TODO не всегда "ВЗ"
    ckiPax.seat_no   = seatNo;
    ckiPax.subclass  = subclass;
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

void savePrintBP(const LoadPaxXmlResult& loadPaxRes)
{
    ASSERT(!loadPaxRes.lSeg.empty());
    const XmlSegment& seg = loadPaxRes.lSeg.front();
    ASSERT(!seg.passengers.empty());
    DeskCodeReplacement dcr("IATCIP");
    for(const auto& pax: seg.passengers)
    {
        LogTrace(TRACE3) << __FUNCTION__
                         << " grp_id:" << seg.seg_info.grp_id
                         << " pax_id:" << pax.pax_id;


        PrintDataParser parser(seg.seg_info.grp_id, pax.pax_id, 0, NULL);
        parser.pts.confirm_print(true, ASTRA::dotPrnBP);
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
    }

    NewTextChild(savePaxNode, "excess");
    NewTextChild(savePaxNode, "hall", 1);

    NewTextChild(savePaxNode, "paid_bags");

    initReqInfo();

    LogTrace(TRACE3) << "save pax query:\n" << XMLTreeToText(reqNode->doc);
    CheckInInterface::instance()->SavePax(getRequestCtxt(), savePaxNode, resNode);
    LogTrace(TRACE3) << "save pax answer:\n" << XMLTreeToText(resNode->doc);
    return LoadPaxXmlResult(resNode);
}

LoadPaxXmlResult AstraEngine::SavePax(const xml_entities::XmlSegment& paxSeg)
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
        if(pax.rems) {
            XmlEntityViewer::viewRems(paxNode, *pax.rems);
        }
    }

    NewTextChild(segNode, "paid_bag_emd");

    NewTextChild(savePaxNode, "excess");
    NewTextChild(savePaxNode, "hall", 1);
    NewTextChild(savePaxNode, "bag_refuse");
    NewTextChild(savePaxNode, "paid_bags");

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

void AstraEngine::initReqInfo() const
{
    TReqInfo::Instance()->Initialize("МОВ");
    TReqInfo::Instance()->client_type  = ASTRA::ctTerm;
    TReqInfo::Instance()->desk.version = VERSION_WITH_BAG_POOLS;
    TReqInfo::Instance()->desk.lang    = AstraLocale::LANG_EN;
    TReqInfo::Instance()->api_mode     = true;
    JxtContext::JxtContHolder::Instance()
            ->setHandler(new JxtContext::JxtContHandlerSir(""));

}

//---------------------------------------------------------------------------------------

//С7300/29.05 ДМД
//СУ1204/28 ПЛК
//ЮТ103 СОЧ
static void parseTrip(const std::string& trip,
                      std::string& airline,
                      std::string& flNum,
                      std::string& scd)
{
    ASSERT(trip.length() > 4);
    std::vector<std::string> splitted1;
    StrUtils::split_string(splitted1, trip, ' ');
    ASSERT(!splitted1.empty());
    std::string left = splitted1.at(0);
    ASSERT(!left.empty());
    if(algo::contains(left, '/')) {
        std::vector<std::string> splitted2;
        StrUtils::split_string(splitted2, left, '/');
        ASSERT(splitted2.size() == 2);
        left = splitted2.at(0);
        scd = splitted2.at(1);
    }

    ASSERT(left.length() > 2);
    size_t airlLen = 2;
    if(StrUtils::IsLatOrRusChar(left[2])) {
        airlLen = 3; // чтобы обрабатывать 3-х символьный код компании
    }
    airline = left.substr(0, airlLen);
    flNum = left.substr(airlLen);

    LogTrace(TRACE3) << trip << " parsed to airline:" << airline << "; "
                     << "flNum:" << flNum << "; "
                     << "scd:" << scd;
}

static boost::optional<XmlPlaceLayer> findLayer(const XmlPlace& place, const std::string& layerType)
{
    for(const XmlPlaceLayer& layer: place.layers) {
        if(layer.layer_type == layerType) {
            return layer;
        }
    }
    return boost::none;
}

static iatci::FlightDetails createFlightDetails(const std::string& trip,
                                                const XmlFilterRoutes& filterRoutes)
{
    std::string airl, flNum, scd;
    parseTrip(trip, airl, flNum, scd);

    std::string airpDep = filterRoutes.depItem().airp,
                airpArv = filterRoutes.arrItem().airp;

    TDateTime now_local = UTCToLocal(NowUTC(), AirpTZRegion(airpDep));
    int now_day_local = ASTRA::NoExists,
        now_mon_local = ASTRA::NoExists,
        now_year_local = ASTRA::NoExists;
    DecodeDate(now_local,
                      now_year_local, now_mon_local, now_day_local);
    LogTrace(TRACE3) << "current local year: " << now_year_local << "; "
                     << "local month: " << now_mon_local << "; "
                     << "local day: " << now_day_local;

    if(scd.length() == 2) {
        // только день - берём текущие месяц и год
        ASSERT(sscanf(scd.c_str(), "%d", &now_day_local) == 1);
    } else if(scd.length() == 5 ) {
        // день.месяц - берём текущий год
        ASSERT(sscanf(scd.c_str(), "%d.%d", &now_day_local, &now_mon_local) == 2);
    }

    TDateTime scd_local = ASTRA::NoExists;
    EncodeDate(now_year_local, now_mon_local, now_day_local, scd_local);

    return iatci::FlightDetails(airl,
                                Ticketing::getFlightNum(flNum),
                                airpDep,
                                airpArv,
                                DateTimeToBoost(scd_local).date());
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

static bool PaxAlreadyCheckedIn(int pointDep,
                                const std::string& surname,
                                const std::string& name)
{
    PaxListXmlResult paxListXmlRes = AstraEngine::singletone().PaxList(pointDep);
    return !paxListXmlRes.applyNameFilter(surname, name).empty();
}

static LoadPaxXmlResult LoadPax__(int pointDep,
                                  const std::string& surname,
                                  const std::string& name)
{
    PaxListXmlResult paxListXmlRes = AstraEngine::singletone().PaxList(pointDep);
    std::list<XmlPax> wantedPaxes = paxListXmlRes.applyNameFilter(surname, name);
    if(wantedPaxes.empty()) {
        // не нашли пассажира в списке зарегистрированных
        throw tick_soft_except(STDLOG, AstraErr::PAX_SURNAME_NOT_CHECKED_IN);
    }

    if(wantedPaxes.size() > 1) {
        // нашшли несколько зарегистрированных пассажиров с одним именем/фамилией
        throw tick_soft_except(STDLOG, AstraErr::TOO_MANY_PAX_WITH_SAME_SURNAME);
    }

    // в этом месте в списке wantedPaxes ровно 1 пассажир
    const XmlPax& pax = wantedPaxes.front();
    LogTrace(TRACE3) << "Pax " << pax.surname << " " << pax.name << " "
                     << "has reg_no " << pax.reg_no;

    ASSERT(pax.reg_no != ASTRA::NoExists);

    LoadPaxXmlResult loadPaxXmlRes =
                AstraEngine::singletone().LoadPax(pointDep, pax.reg_no);

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

static void applyRemsUpdate(XmlPax& pax, const iatci::UpdateServiceDetails& updSvc)
{
    LogTrace(TRACE3) << __FUNCTION__;
    if(!pax.rems) {
        pax.rems = XmlRems();
    }

    for(const iatci::UpdateServiceDetails::UpdSsrInfo& updSsr: updSvc.lSsr())
    {
        XmlRem rem;
        rem.rem_code = updSsr.ssrCode();
        rem.rem_text = updSsr.freeText();

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
    for(const auto& paxGroup: paxGroups)
    {
        const std::string PaxSurname = paxGroup.pax().surname();
        const std::string PaxName    = paxGroup.pax().name();
        LogTrace(TRACE3) << "handle pax: " << PaxSurname << "/" << PaxName;

        if(PaxAlreadyCheckedIn(pointDep, PaxSurname, PaxName)) {
            throw tick_soft_except(STDLOG, AstraErr::PAX_ALREADY_CHECKED_IN);
        }

        SearchPaxXmlResult searchPaxXmlRes =
                    AstraEngine::singletone().SearchCheckInPax(pointDep,
                                                               PaxSurname,
                                                               PaxName);

        std::list<XmlTrip> tripsFiltered = searchPaxXmlRes.applyNameFilter(PaxSurname,
                                                                           PaxName);
        if(tripsFiltered.empty()) {
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

        ASSERT(!finalPnrs.empty());
        ASSERT(!finalPaxes.empty());

        if(finalPnrs.size() > 1 || finalPaxes.size() > 1) {
            // TODO здесь можно уточнить пакса по доп. признакам, например TKNE
            throw tick_soft_except(STDLOG, AstraErr::TOO_MANY_PAX_WITH_SAME_SURNAME);
        }

        const std::string seatNo = paxGroup.seat() ? paxGroup.seat()->seat()
                                                   : "";
        const std::string subcls = paxGroup.reserv() ? paxGroup.reserv()->subclass()->code(RUSSIAN)
                                                     : "";
        XmlPax pax = createCheckInPax(finalPaxes.front(), seatNo, subcls);

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

        paxSeg.passengers.push_back(pax);
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

iatci::dcrcka::Result updateIatciPaxes(const iatci::CkuParams& ckuParams)
{
    LogTrace(TRACE3) << __FUNCTION__;

    ASSERT(ckuParams.fltGroup().paxGroups().size() > 0);
    bool multiPax = ckuParams.fltGroup().paxGroups().size() > 1;
    if(multiPax) {
        throw tick_soft_except(STDLOG, AstraErr::UPDATE_SEPARATELY);
    }

    const iatci::FlightDetails& outbFlt = ckuParams.fltGroup().outboundFlight();
    int pointDep = astra_api::findDepPointId(outbFlt.depPort(),
                                             outbFlt.airline(),
                                             outbFlt.flightNum(),
                                             outbFlt.depDate());


    const auto& paxGroup = ckuParams.fltGroup().paxGroups().front();
    const std::string PaxSurname = paxGroup.pax().surname();
    const std::string PaxName    = paxGroup.pax().name();
    bool Reseat = paxGroup.updSeat();
    LogTrace(TRACE3) << "handle pax: " << PaxSurname << "/" << PaxName;

    LoadPaxXmlResult loadPaxXmlRes = LoadPax__(pointDep,
                                               PaxSurname,
                                               PaxName);

    std::list<XmlPax> paxesFiltered = loadPaxXmlRes.applyNameFilter(PaxSurname,
                                                                    PaxName);

    ASSERT(!paxesFiltered.empty());
    if(paxesFiltered.size() > 1) {
        // TODO здесь можно уточнить пакса по доп. признакам, например TKNE
        throw tick_soft_except(STDLOG, AstraErr::TOO_MANY_PAX_WITH_SAME_SURNAME);
    }

    XmlPax pax = paxesFiltered.front();
    // здесь в loadPaxXmlRes всегда один сегмент
    XmlSegment currSeg = loadPaxXmlRes.lSeg.front();
    ASSERT(currSeg.seg_info.grp_id != ASTRA::NoExists);


    XmlSegment paxSeg = makeSegment(currSeg);


    if(paxGroup.updSeat()) {
        applySeatUpdate(pax, *paxGroup.updSeat());
    }

    if(paxGroup.updDoc()) {
        applyDocUpdate(pax, *paxGroup.updDoc());
    }

    if(paxGroup.updService()) {
        applyRemsUpdate(pax, *paxGroup.updService());
    }

    paxSeg.passengers.push_back(pax);


    if(Reseat) {
        loadPaxXmlRes = AstraEngine::singletone().ReseatPax(paxSeg);
        return loadPaxXmlRes.toIatciFirst(iatci::dcrcka::Result::Update,
                                          iatci::dcrcka::Result::Ok);
    } else {
        loadPaxXmlRes = AstraEngine::singletone().SavePax(paxSeg);
        if(loadPaxXmlRes.lSeg.empty()) {
            // не смогли сохранить
            throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR, "Unable to update pax");
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

iatci::dcrcka::Result cancelCheckinIatciPaxes(const iatci::CkxParams& ckxParams)
{
    LogTrace(TRACE3) << __FUNCTION__;

    const iatci::FlightDetails& outbFlt = ckxParams.fltGroup().outboundFlight();
    int pointDep = astra_api::findDepPointId(outbFlt.depPort(),
                                             outbFlt.airline(),
                                             outbFlt.flightNum(),
                                             outbFlt.depDate());

    boost::optional<XmlSegment> paxSeg;
    const auto& paxGroups = ckxParams.fltGroup().paxGroups();
    for(const auto& paxGroup: paxGroups)
    {
        const std::string PaxSurname = paxGroup.pax().surname();
        const std::string PaxName    = paxGroup.pax().name();
        LogTrace(TRACE3) << "handle pax: " << PaxSurname << "/" << PaxName;


        LoadPaxXmlResult loadPaxXmlRes = LoadPax__(pointDep,
                                                   PaxSurname,
                                                   PaxName);

        std::list<XmlPax> paxesFiltered = loadPaxXmlRes.applyNameFilter(PaxSurname,
                                                                        PaxName);

        ASSERT(!paxesFiltered.empty());
        if(paxesFiltered.size() > 1) {
            // TODO здесь можно уточнить пакса по доп. признакам, например TKNE
            throw tick_soft_except(STDLOG, AstraErr::TOO_MANY_PAX_WITH_SAME_SURNAME);
        }

        XmlPax pax = createCancelPax(paxesFiltered.front());
        // здесь в loadPaxXmlRes всегда один сегмент
        XmlSegment currSeg = loadPaxXmlRes.lSeg.front();
        ASSERT(currSeg.seg_info.grp_id != ASTRA::NoExists);

        if(!paxSeg) {
            paxSeg = makeSegment(currSeg);
        }

        if(currSeg.seg_info.grp_id != paxSeg->seg_info.grp_id) {
            LogWarning(STDLOG) << "Attempt to cancel paxes from different groups at single request! "
                               << currSeg.seg_info.grp_id << "<>" << paxSeg->seg_info.grp_id;
            throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR);
        }
        paxSeg->passengers.push_back(pax);
    }

    ASSERT(paxSeg);
    // SavePax
    tst();
    LoadPaxXmlResult loadPaxXmlRes = AstraEngine::singletone().SavePax(*paxSeg);
    tst();

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

    int pointDep = astra_api::findDepPointId(plfParams.outboundFlight().depPort(),
                                             plfParams.outboundFlight().airline(),
                                             plfParams.outboundFlight().flightNum().get(),
                                             plfParams.outboundFlight().depDate());

    LoadPaxXmlResult loadPaxXmlRes = LoadPax__(pointDep,
                                               plfParams.personal().surname(),
                                               plfParams.personal().name());

    if(loadPaxXmlRes.lSeg.empty()) {
        tst();
        throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR, "Unable to load pax");
    }

    tst();
    return loadPaxXmlRes.toIatciFirst(iatci::dcrcka::Result::Passlist,
                                      iatci::dcrcka::Result::Ok);
}

iatci::dcrcka::Result fillSeatmap(const iatci::SmfParams& smfParams)
{
    LogTrace(TRACE3) << __FUNCTION__ << " by "
                     << "airp_dep[" << smfParams.outboundFlight().depPort() << "]; "
                     << "airline["  << smfParams.outboundFlight().airline() << "]; "
                     << "flight["   << smfParams.outboundFlight().flightNum() << "]; "
                     << "dep_date[" << smfParams.outboundFlight().depDate() << "]; ";

    int pointDep = astra_api::findDepPointId(smfParams.outboundFlight().depPort(),
                                             smfParams.outboundFlight().airline(),
                                             smfParams.outboundFlight().flightNum().get(),
                                             smfParams.outboundFlight().depDate());

    LogTrace(TRACE3) << "seatmap point id: " << pointDep;

    GetSeatmapXmlResult seatmapXmlRes = AstraEngine::singletone().GetSeatmap(pointDep);
    return seatmapXmlRes.toIatci();
}

iatci::dcrcka::Result printBoardingPass(const iatci::BprParams& bprParams)
{
    LogTrace(TRACE3) << __FUNCTION__;

    const iatci::FlightDetails& outbFlt = bprParams.fltGroup().outboundFlight();
    int pointDep = astra_api::findDepPointId(outbFlt.depPort(),
                                             outbFlt.airline(),
                                             outbFlt.flightNum(),
                                             outbFlt.depDate());

    boost::optional<XmlPax> firstPax;
    boost::optional<XmlSegment> paxSeg;
    const auto& paxGroups = bprParams.fltGroup().paxGroups();
    for(const auto& paxGroup: paxGroups)
    {
        const std::string PaxSurname = paxGroup.pax().surname();
        const std::string PaxName    = paxGroup.pax().name();
        LogTrace(TRACE3) << "handle pax: " << PaxSurname << "/" << PaxName;


        LoadPaxXmlResult loadPaxXmlRes = LoadPax__(pointDep,
                                                   PaxSurname,
                                                   PaxName);

        std::list<XmlPax> paxesFiltered = loadPaxXmlRes.applyNameFilter(PaxSurname,
                                                                        PaxName);

        ASSERT(!paxesFiltered.empty());
        if(paxesFiltered.size() > 1) {
            // TODO здесь можно уточнить пакса по доп. признакам, например TKNE
            throw tick_soft_except(STDLOG, AstraErr::TOO_MANY_PAX_WITH_SAME_SURNAME);
        }

        XmlPax pax = paxesFiltered.front();
        XmlSegment currSeg = loadPaxXmlRes.lSeg.front();
        ASSERT(currSeg.seg_info.grp_id != ASTRA::NoExists);

        // здесь в loadPaxXmlRes всегда один сегмент
        if(!paxSeg) {
            paxSeg = currSeg;
        }
        if(!firstPax) {
            firstPax = pax;
        }

        if(currSeg.seg_info.grp_id != paxSeg->seg_info.grp_id) {
            LogWarning(STDLOG) << "Attempt to print BP for paxes from different groups at single request! "
                               << currSeg.seg_info.grp_id << "<>" << paxSeg->seg_info.grp_id;
            throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR);
        }
    }

    if(!paxSeg) {
        tst();
        throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR, "Unable to find pax/group");
    }

    // все проверки прошли - можно брать любого пассажира из группы
    // и грузить данные по всей группе. Берём первого пассажира

    LoadPaxXmlResult loadPaxXmlRes = LoadPax__(pointDep,
                                               firstPax->surname,
                                               firstPax->name);

    savePrintBP(loadPaxXmlRes);

    return loadPaxXmlRes.toIatciFirst(iatci::dcrcka::Result::Passlist,
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

bool operator==(const XmlRem& l, const XmlRem& r)
{
    return (l.rem_code == r.rem_code &&
            l.rem_text == r.rem_text);
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
      user_id(ASTRA::NoExists)
{}

bool XmlPax::equalName(const std::string& surname, const std::string& name) const
{
    // возможно следует усложнить функцию сравнения (trim, translit)
    return (this->surname == surname &&
            this->name == name);
}

astra_entities::PaxInfo XmlPax::toPax() const
{
    boost::optional<astra_entities::DocInfo> paxDoc;
    if(doc) {
        paxDoc = doc->toDoc();
    }

    boost::optional<astra_entities::Remarks> paxRems;
    if(rems)
    {
        paxRems = astra_entities::Remarks();
        for(const XmlRem& rem: rems->rems) {
            paxRems->m_lRems.push_back(astra_entities::Remark(rem.rem_code,
                                                              rem.rem_text));
        }
    }

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
                                   paxDoc,
                                   paxRems);
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
    th.flight             = NodeAsString("flight",              thNode, "");
    th.airline            = NodeAsString("airline",             thNode, "");
    th.aircode            = NodeAsString("aircode",             thNode, "");
    th.flt_no             = NodeAsInteger("flt_no",             thNode, ASTRA::NoExists);
    th.suffix             = NodeAsString("suffix",              thNode, "");
    th.airp               = NodeAsString("airp",                thNode, "");
    th.scd_out_local      = NodeAsDateTime("scd_out_local",     thNode, ASTRA::NoExists);
    th.pr_etl_only        = NodeAsInteger("pr_etl_only",        thNode, ASTRA::NoExists);
    th.pr_etstatus        = NodeAsInteger("pr_etstatus",        thNode, ASTRA::NoExists);
    th.pr_no_ticket_check = NodeAsInteger("pr_no_ticket_check", thNode, ASTRA::NoExists);
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
    airp.point_id    = NodeAsInteger("point_id",   airpNode, ASTRA::NoExists);
    airp.airp_code   = NodeAsString("airp_code",   airpNode, "");
    airp.city_code   = NodeAsString("city_code",   airpNode, "");
    airp.target_view = NodeAsString("target_view", airpNode, "");
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
    cls.code       = NodeAsString("code",       classNode, "");
    cls.class_view = NodeAsString("class_view", classNode, "");
    if(cls.class_view.empty()) {
        cls.class_view = NodeAsString("name",   classNode, "");
    }
    cls.cfg        = NodeAsInteger("cfg", classNode, ASTRA::NoExists);
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
    flight.airline       = NodeAsString("airline",        flightNode, "");
    flight.flt_no        = NodeAsInteger("flt_no",        flightNode, ASTRA::NoExists);
    flight.suffix        = NodeAsString("suffix",         flightNode, "");
    flight.scd           = NodeAsDateTime("scd",          flightNode, ASTRA::NoExists);
    flight.airp_dep      = NodeAsString("airp_dep",       flightNode, "");
    flight.pr_mark_norms = NodeAsInteger("pr_mark_norms", flightNode, ASTRA::NoExists);
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
    item.point_arv   = NodeAsInteger("point_arv",   itemNode, ASTRA::NoExists);
    item.cls         = NodeAsString("class",        itemNode, "");
    item.noshow      = NodeAsInteger("noshow",      itemNode, ASTRA::NoExists);
    item.trnoshow    = NodeAsInteger("trnoshow",    itemNode, ASTRA::NoExists);
    item.show        = NodeAsInteger("show",        itemNode, ASTRA::NoExists);
    item.free_ok     = NodeAsInteger("free_ok",     itemNode, ASTRA::NoExists);
    item.free_goshow = NodeAsInteger("free_goshow", itemNode, ASTRA::NoExists);
    item.nooccupy    = NodeAsInteger("nooccupy",    itemNode, ASTRA::NoExists);
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
    rem.rem_code = NodeAsString("rem_code", remNode, "");
    rem.rem_text = NodeAsString("rem_text", remNode, "");
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

XmlPaxDoc XmlEntityReader::readDoc(xmlNodePtr docNode)
{
    ASSERT(docNode);

    XmlPaxDoc doc;
    doc.no = NodeAsString(docNode);
    if(doc.no.empty())
    {
        doc.no           = NodeAsString("no",            docNode, "");
        doc.birth_date   = NodeAsString("birth_date",    docNode, "");
        doc.surname      = NodeAsString("surname",       docNode, "");
        doc.first_name   = NodeAsString("first_name",    docNode, "");
        doc.second_name  = NodeAsString("second_name",   docNode, "");
        doc.expiry_date  = NodeAsString("expiry_date",   docNode, "");
        doc.type         = NodeAsString("type",          docNode, "");
        doc.nationality  = NodeAsString("nationality",   docNode, "");
        doc.gender       = NodeAsString("gender",        docNode, "");
        doc.issue_country= NodeAsString("issue_country", docNode, "");
    }
    return doc;
}

XmlPax XmlEntityReader::readPax(xmlNodePtr paxNode)
{
    ASSERT(paxNode);

    XmlPax pax;
    pax.pax_id         = NodeAsInteger("pax_id",         paxNode, ASTRA::NoExists);
    pax.grp_id         = NodeAsInteger("grp_id",         paxNode, ASTRA::NoExists);
    pax.cl_grp_id      = NodeAsInteger("cl_grp_id",      paxNode, ASTRA::NoExists);
    pax.surname        = NodeAsString("surname",         paxNode, "");
    pax.name           = NodeAsString("name",            paxNode, "");
    pax.airp_arv       = NodeAsString("airp_arv",        paxNode, "");
    pax.pers_type      = NodeAsString("pers_type",       paxNode, "");
    pax.seat_no        = NodeAsString("seat_no",         paxNode, "");
    pax.seat_type      = NodeAsString("seat_type",       paxNode, "");
    pax.seats          = NodeAsInteger("seats",          paxNode, ASTRA::NoExists);
    pax.refuse         = NodeAsString("refuse",          paxNode, "");
    pax.reg_no         = NodeAsInteger("reg_no",         paxNode, ASTRA::NoExists);
    pax.subclass       = NodeAsString("subclass",        paxNode, "");
    pax.bag_pool_num   = NodeAsInteger("bag_pool_num",   paxNode, ASTRA::NoExists);
    pax.tid            = NodeAsInteger("tid",            paxNode, ASTRA::NoExists);
    pax.ticket_no      = NodeAsString("ticket_no",       paxNode, "");
    pax.coupon_no      = NodeAsInteger("coupon_no",      paxNode, ASTRA::NoExists);
    pax.ticket_rem     = NodeAsString("ticket_rem",      paxNode, "");
    pax.ticket_confirm = NodeAsInteger("ticket_confirm", paxNode, ASTRA::NoExists);
    pax.pr_norec       = NodeAsInteger("pr_norec",       paxNode, ASTRA::NoExists);
    pax.pr_bp_print    = NodeAsInteger("pr_bp_print",    paxNode, ASTRA::NoExists);
    pax.hall_id        = NodeAsInteger("hall_id",        paxNode, ASTRA::NoExists);
    pax.point_arv      = NodeAsInteger("point_arv",      paxNode, ASTRA::NoExists);
    pax.user_id        = NodeAsInteger("user_id",        paxNode, ASTRA::NoExists);
    pax.iatci_pax_id   = NodeAsString("iatci_pax_id",    paxNode, "");

    // doc
    xmlNodePtr docNode = findNode(paxNode, "document");
    if(docNode != NULL && !isempty(docNode)) {
        pax.doc = XmlEntityReader::readDoc(docNode);
    }

    // remarks
    xmlNodePtr remsNode = findNode(paxNode, "rems");
    if(remsNode != NULL && !isempty(remsNode)) {
        pax.rems = XmlEntityReader::readRems(remsNode);
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
    recloc.airline = NodeAsString("airline", reclocNode);
    recloc.recloc  = NodeAsString("addr", reclocNode);
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
    trferSeg.num          = NodeAsInteger("num",          trferSegNode, ASTRA::NoExists);
    trferSeg.airline      = NodeAsString("airline",       trferSegNode, "");
    trferSeg.flt_no       = NodeAsString("flt_no",        trferSegNode, "");
    trferSeg.local_date   = NodeAsString("local_date",    trferSegNode, "");
    trferSeg.airp_dep     = NodeAsString("airp_dep",      trferSegNode, "");
    trferSeg.airp_arv     = NodeAsString("airp_arv",      trferSegNode, "");
    trferSeg.subclass     = NodeAsString("subclass",      trferSegNode, "");
    trferSeg.trfer_permit = NodeAsInteger("trfer_permit", trferSegNode, ASTRA::NoExists);
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
    pnr.pnr_id   = NodeAsInteger("pnr_id",  pnrNode, ASTRA::NoExists);
    pnr.airp_arv = NodeAsString("airp_arv", pnrNode, "");
    pnr.subclass = NodeAsString("subclass", pnrNode, "");
    pnr.cls      = NodeAsString("class",    pnrNode, "");

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
    trip.point_id = NodeAsInteger("point_id", tripNode, ASTRA::NoExists);
    trip.airline  = NodeAsString("airline",   tripNode, "");
    trip.flt_no   = NodeAsInteger("flt_no",   tripNode, ASTRA::NoExists);
    trip.scd      = NodeAsString("scd",       tripNode, "");
    trip.airp_dep = NodeAsString("airp_dep",  tripNode, "");

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

XmlSegmentInfo XmlEntityReader::readSegInfo(xmlNodePtr segNode)
{
    ASSERT(segNode);

    XmlSegmentInfo seg_info;
    seg_info.grp_id    = NodeAsInteger("grp_id",    segNode, ASTRA::NoExists);
    seg_info.point_dep = NodeAsInteger("point_dep", segNode, ASTRA::NoExists);
    seg_info.airp_dep  = NodeAsString("airp_dep",   segNode, "");
    seg_info.point_arv = NodeAsInteger("point_arv", segNode, ASTRA::NoExists);
    seg_info.airp_arv  = NodeAsString("airp_arv",   segNode, "");
    seg_info.cls       = NodeAsString("class",      segNode, "");
    seg_info.status    = NodeAsString("status",     segNode, "");
    seg_info.bag_refuse= NodeAsString("bag_refuse", segNode, "");
    seg_info.tid       = NodeAsInteger("tid",       segNode, ASTRA::NoExists);
    seg_info.city_arv  = NodeAsString("city_arv",   segNode, "");
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

XmlPlaceLayer XmlEntityReader::readPlaceLayer(xmlNodePtr layerNode)
{
    ASSERT(layerNode);

    XmlPlaceLayer layer;
    layer.layer_type = NodeAsString("layer_type", layerNode, "");
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
    place.x         = NodeAsInteger("x",        placeNode, ASTRA::NoExists);
    place.y         = NodeAsInteger("y",        placeNode, ASTRA::NoExists);
    place.elem_type = NodeAsString("elem_type", placeNode, "");
    place.cls       = NodeAsString("class",     placeNode, "");
    place.xname     = NodeAsString("xname",     placeNode, "");
    place.yname     = NodeAsString("yname",     placeNode, "");
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
    item.point_id = NodeAsInteger("point_id", itemNode, ASTRA::NoExists);
    item.airp     = NodeAsString("airp", itemNode, "");
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
    filterRoutes.point_dep = NodeAsInteger("point_dep", filterRoutesNode, ASTRA::NoExists);
    filterRoutes.point_arv = NodeAsInteger("point_arv", filterRoutesNode, ASTRA::NoExists);
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
    NewTextChild(paxNode, "seats",          1);
    NewTextChild(paxNode, "ticket_no",      pax.ticket_no);
    NewTextChild(paxNode, "coupon_no",      pax.coupon_no);
    NewTextChild(paxNode, "ticket_rem",     pax.ticket_rem);
    NewTextChild(paxNode, "ticket_confirm", 0);
    NewTextChild(paxNode, "subclass",       pax.subclass);
    NewTextChild(paxNode, "bag_pool_num");
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
}

LoadPaxXmlResult::LoadPaxXmlResult(const std::list<XmlSegment>& lSeg)
{
    this->lSeg = lSeg;
}

std::vector<iatci::dcrcka::Result> LoadPaxXmlResult::toIatci(iatci::dcrcka::Result::Action_e action,
                                                             iatci::dcrcka::Result::Status_e status) const
{
    std::vector<iatci::dcrcka::Result> lRes;
    for(auto& seg: lSeg)
    {
        std::list<iatci::dcrcka::PaxGroup> paxGroups;
        for(const XmlPax& pax: seg.passengers)
        {
            astra_entities::PaxInfo paxInfo = pax.toPax();

            LogTrace(TRACE3) << "handle pax " << paxInfo.m_surname << "/" << paxInfo.m_name;

            paxGroups.push_back(iatci::dcrcka::PaxGroup(iatci::makePax(paxInfo),
                                                        iatci::makeReserv(paxInfo),
                                                        iatci::makeFlightSeat(paxInfo),
                                                        iatci::makeBaggage(paxInfo),
                                                        iatci::makeService(paxInfo),
                                                        iatci::makeDoc(paxInfo),
                                                        iatci::makeAddress(paxInfo)));
        }

        lRes.push_back(iatci::dcrcka::Result::makeResult(action,
                                                         status,
                                                         iatci::makeFlight(seg),
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
    ASSERT(lRes.size() == 1);
    return lRes.front();
}

std::list<XmlPax> LoadPaxXmlResult::applyNameFilter(const std::string& surname,
                                                    const std::string& name) const
{
    std::list<XmlPax> res;
    if(lSeg.empty()) {
        return res;
    }

    ASSERT(lSeg.size() == 1);
    const XmlSegment& seg = lSeg.front();
    for(const XmlPax& pax: seg.passengers) {
        if(pax.equalName(surname, name)) {
            res.push_back(pax);
        }
    }

    return res;
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
        // TODO необходимо усложнить функцию сравнения (trim, translit)
        if(pax.equalName(surname, name)) {
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
    trip  = NodeAsString(findNodeR(node, "trip"));
    craft = NodeAsString(findNodeR(node, "craft"));
    xmlNodePtr salonsNode = findNodeR(node, "salons");
    if(salonsNode != NULL) {
        lPlacelist   = XmlEntityReader::readPlaceLists(salonsNode);
        xmlNodePtr filterRoutesNode = findNodeR(salonsNode, "filterRoutes");
        if(filterRoutesNode != NULL) {
            filterRoutes = XmlEntityReader::readFilterRoutes(filterRoutesNode);
        }
    }
}

iatci::dcrcka::Result GetSeatmapXmlResult::toIatci() const
{
    if(lPlacelist.empty() || filterRoutes.items.empty()) {
        LogError(STDLOG) << "Seatmap failed!";
        throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR);
    }

    return iatci::dcrcka::Result::makeSeatmapResult(iatci::dcrcka::Result::Ok,
                                                    createFlightDetails(trip, filterRoutes),
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

bool operator==(const AddressInfo& left, const AddressInfo& right)
{
    return true; // TODO
}

bool operator!=(const AddressInfo& left, const AddressInfo& right)
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
                 const boost::optional<Remarks>& rems)
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
      m_rems(rems)
{}

bool operator==(const PaxInfo& left, const PaxInfo& right)
{
    return (left.m_paxId     == right.m_paxId &&
            left.m_surname   == right.m_surname &&
            left.m_name      == right.m_name &&
            left.m_persType  == right.m_persType &&
            left.m_ticketNum == right.m_ticketNum &&
            left.m_couponNum == right.m_couponNum &&
            left.m_ticketRem == right.m_ticketRem &&
            left.m_seatNo    == right.m_seatNo &&
            left.m_regNo     == right.m_regNo &&
            left.m_iatciPaxId== right.m_iatciPaxId &&
            left.m_subclass  == right.m_subclass &&
            left.m_doc       == right.m_doc &&
            left.m_address   == right.m_address &&
            left.m_visa      == right.m_visa &&
            left.m_rems      == right.m_rems);
}

bool operator!=(const PaxInfo& left, const PaxInfo& right)
{
    return !(left == right);
}

}//namespace astra_entities

}//namespace astra_api
