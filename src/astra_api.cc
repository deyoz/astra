#include "astra_api.h"
#include "astra_msg.h"
#include "astra_misc.h"
#include "astra_utils.h"
#include "basic.h"
#include "points.h"
#include "checkin.h"
#include "print.h"
#include "tripinfo.h"
#include "salonform.h"

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

namespace {

void fillPaxTrip(XmlTrip& paxTrip, const iatci::CkiParams& ckiParams)
{
    paxTrip.status = "K"; // TODO всегда "K" ?
    paxTrip.pnr().pax().pers_type = "ВЗ";
    if(ckiParams.seat()) {
        paxTrip.pnr().pax().seat_no = ckiParams.seat()->seat();
    }
}

void savePrintBP(const LoadPaxXmlResult& loadPaxRes)
{
    ASSERT(!loadPaxRes.lSeg.empty());
    const XmlSegment& seg = loadPaxRes.lSeg.front();
    ASSERT(!seg.passengers.empty());
    const XmlPax& pax = seg.passengers.front();

    LogTrace(TRACE3) << __FUNCTION__
                     << " grp_id:" << seg.grp_id
                     << " pax_id:" << pax.pax_id;

    TReqInfo::Instance()->desk.code = "IATCI";

    PrintDataParser parser(seg.grp_id, pax.pax_id, 0, NULL);
    parser.pts.save_bp_print(true);
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

    xmlNodePtr paxListNode = newChild(reqNode, "PaxList");
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

    xmlNodePtr loadPaxNode = newChild(reqNode, "TCkinLoadPax");
    NewTextChild(loadPaxNode, "point_id", pointId);
    NewTextChild(loadPaxNode, "reg_no", paxRegNo);

    initReqInfo();

    LogTrace(TRACE3) << "load pax query:\n" << XMLTreeToText(reqNode->doc);
    CheckInInterface::instance()->LoadPax(getRequestCtxt(), loadPaxNode, resNode);
    LogTrace(TRACE3) << "load pax answer:\n" << XMLTreeToText(resNode->doc);
    return LoadPaxXmlResult(resNode);
}


SearchPaxXmlResult AstraEngine::SearchPax(int pointDep,
                                          const std::string& paxSurname,
                                          const std::string& paxName,
                                          const std::string& paxStatus)
{
    xmlNodePtr reqNode = getQueryNode(),
               resNode = getAnswerNode();

    std::string query = paxSurname + " " + paxName;

    xmlNodePtr searchPaxNode = newChild(reqNode, "SearchPax");
    NewTextChild(searchPaxNode, "point_dep", pointDep);
    NewTextChild(searchPaxNode, "pax_status", paxStatus);
    NewTextChild(searchPaxNode, "query", query);

    initReqInfo();

    LogTrace(TRACE3) << "search pax query:\n" << XMLTreeToText(reqNode->doc);
    CheckInInterface::instance()->SearchPax(getRequestCtxt(), searchPaxNode, resNode);
    LogTrace(TRACE3) << "search pax answer:\n" << XMLTreeToText(resNode->doc);
    return SearchPaxXmlResult(resNode);
}

LoadPaxXmlResult AstraEngine::SavePax(int pointDep, const XmlTrip& paxTrip)
{
    // пока можем обрабатывать одного пассажира
    const XmlPnr& pnr = paxTrip.pnr();
    const XmlPax& pax = pnr.pax();

    xmlNodePtr reqNode = getQueryNode(),
               resNode = getAnswerNode();

    xmlNodePtr savePaxNode = newChild(reqNode, "TCkinSavePax");
    NewTextChild(savePaxNode, "agent_stat_period", -1);
    NewTextChild(savePaxNode, "transfer");
    xmlNodePtr segsNode = newChild(savePaxNode, "segments");
    // пока можем добавить только один сегмент
    xmlNodePtr segNode = newChild(segsNode, "segment");
    NewTextChild(segNode, "point_dep", pointDep);
    NewTextChild(segNode, "point_arv", findArvPointId(pointDep, pnr.airp_arv));
    NewTextChild(segNode, "airp_dep",  paxTrip.airp_dep);
    NewTextChild(segNode, "airp_arv",  pnr.airp_arv);
    NewTextChild(segNode, "class",     pnr.cls);
    NewTextChild(segNode, "status",    paxTrip.status);
    NewTextChild(segNode, "wl_type");

    xmlNodePtr markFlightNode = newChild(segNode, "mark_flight");
    NewTextChild(markFlightNode, "airline", paxTrip.airline);
    NewTextChild(markFlightNode, "flt_no",  paxTrip.flt_no);
    NewTextChild(markFlightNode, "suffix");
    NewTextChild(markFlightNode, "scd",     paxTrip.scd);
    NewTextChild(markFlightNode, "airp_dep",paxTrip.airp_dep);
    NewTextChild(markFlightNode, "pr_mark_norms", 0);

    // пока работаем с одним пассажиром
    xmlNodePtr passengersNode = newChild(segNode, "passengers");
    xmlNodePtr paxNode = newChild(passengersNode, "pax");
    NewTextChild(paxNode, "pax_id",  pax.pax_id);
    NewTextChild(paxNode, "surname", pax.surname);
    NewTextChild(paxNode, "name",    pax.name);
    NewTextChild(paxNode, "pers_type", pax.pers_type);
    NewTextChild(paxNode, "seat_no", pax.seat_no);
    NewTextChild(paxNode, "preseat_no");
    NewTextChild(paxNode, "seat_type");
    NewTextChild(paxNode, "seats", 1);
    NewTextChild(paxNode, "ticket_no", pax.ticket_no);
    NewTextChild(paxNode, "coupon_no", pax.coupon_no);
    NewTextChild(paxNode, "ticket_rem", pax.ticket_rem);
    NewTextChild(paxNode, "ticket_confirm", 0);
    if(pax.doc)
    {
        xmlNodePtr docNode = newChild(paxNode, "document");
        NewTextChild(docNode, "type", pax.doc->type);
        NewTextChild(docNode, "issue_country", pax.doc->issue_country);
        NewTextChild(docNode, "no", pax.doc->no);
        NewTextChild(docNode, "nationality", pax.doc->nationality);
        NewTextChild(docNode, "birth_date", pax.doc->birth_date);
        NewTextChild(docNode, "expiry_date", pax.doc->expiry_date);
        NewTextChild(docNode, "gender", pax.doc->gender);
        NewTextChild(docNode, "surname", pax.doc->surname);
        NewTextChild(docNode, "first_name", pax.doc->first_name);
        NewTextChild(docNode, "second_name", pax.doc->second_name);
    }

    NewTextChild(paxNode, "doco");
    NewTextChild(paxNode, "addresses");
    NewTextChild(paxNode, "subclass", pnr.subclass);
    NewTextChild(paxNode, "bag_pool_num");
    NewTextChild(paxNode, "transfer");

    if(pax.rems)
    {
        xmlNodePtr remsNode = newChild(paxNode, "rems");
        for(const XmlRem& rem: pax.rems->rems)
        {
            xmlNodePtr remNode = newChild(remsNode, "rem");
            NewTextChild(remNode, "rem_code", rem.rem_code);
            NewTextChild(remNode, "rem_text", rem.rem_text);
        }
    }

    NewTextChild(segNode, "paid_bag_emd");

    NewTextChild(savePaxNode, "excess");
    NewTextChild(savePaxNode, "hall", 1);
    xmlNodePtr paidBagsNode = newChild(savePaxNode, "paid_bags");
    xmlNodePtr paidBagNode = newChild(paidBagsNode, "paid_bag");
    NewTextChild(paidBagNode, "bag_type");
    NewTextChild(paidBagNode, "weight");
    NewTextChild(paidBagNode, "rate_id");
    NewTextChild(paidBagNode, "rate_trfer");

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

    xmlNodePtr savePaxNode = newChild(reqNode, "TCkinSavePax");
    NewTextChild(savePaxNode, "agent_stat_period", -1);
    NewTextChild(savePaxNode, "transfer");
    xmlNodePtr segsNode = newChild(savePaxNode, "segments");
    // пока можем добавить только один сегмент
    xmlNodePtr segNode = newChild(segsNode, "segment");
    NewTextChild(segNode, "point_dep", paxSeg.point_dep);
    NewTextChild(segNode, "point_arv", paxSeg.point_arv);
    NewTextChild(segNode, "airp_dep",  paxSeg.airp_dep);
    NewTextChild(segNode, "airp_arv",  paxSeg.airp_arv);
    NewTextChild(segNode, "class",     paxSeg.cls);
    NewTextChild(segNode, "status",    paxSeg.status);
    NewTextChild(segNode, "grp_id",    paxSeg.grp_id);
    NewTextChild(segNode, "tid",       paxSeg.tid);

    xmlNodePtr markFlightNode = newChild(segNode, "mark_flight");
    NewTextChild(markFlightNode, "airline",       paxSeg.mark_flight.airline);
    NewTextChild(markFlightNode, "flt_no",        paxSeg.mark_flight.flt_no);
    NewTextChild(markFlightNode, "suffix",        paxSeg.mark_flight.suffix);
    NewTextChild(markFlightNode, "scd",           paxSeg.mark_flight.scd);
    NewTextChild(markFlightNode, "airp_dep",      paxSeg.mark_flight.airp_dep);
    NewTextChild(markFlightNode, "pr_mark_norms", paxSeg.mark_flight.pr_mark_norms);

    // пока работаем с одним пассажиром
    ASSERT(paxSeg.passengers.size() == 1);
    const XmlPax& pax = paxSeg.passengers.front();

    xmlNodePtr passengersNode = newChild(segNode, "passengers");
    xmlNodePtr paxNode = newChild(passengersNode, "pax");
    NewTextChild(paxNode, "pax_id",  pax.pax_id);
    NewTextChild(paxNode, "surname", pax.surname);
    NewTextChild(paxNode, "name",    pax.name);
    NewTextChild(paxNode, "pers_type", pax.pers_type);
    if(pax.refuse) {
        LogTrace(TRACE3) << "refuse = " << *pax.refuse;
        NewTextChild(paxNode, "refuse", *pax.refuse);
    }

    NewTextChild(paxNode, "ticket_no", pax.ticket_no);
    NewTextChild(paxNode, "coupon_no", pax.coupon_no);
    NewTextChild(paxNode, "ticket_rem", pax.ticket_rem);
    NewTextChild(paxNode, "ticket_confirm", 0);
    if(pax.doc)
    {
        xmlNodePtr docNode = newChild(paxNode, "document");
        NewTextChild(docNode, "no", pax.doc->no);
        NewTextChild(docNode, "type", pax.doc->type);
        NewTextChild(docNode, "birth_date", pax.doc->birth_date);
        NewTextChild(docNode, "expiry_date", pax.doc->expiry_date);
        NewTextChild(docNode, "surname", pax.doc->surname);
        NewTextChild(docNode, "first_name", pax.doc->first_name);
        NewTextChild(docNode, "second_name", pax.doc->second_name);
        NewTextChild(docNode, "nationality", pax.doc->nationality);
        NewTextChild(docNode, "gender", pax.doc->gender);
        NewTextChild(docNode, "issue_country", pax.doc->issue_country);
    }

    if(pax.rems)
    {
        xmlNodePtr remsNode = newChild(paxNode, "rems");
        for(const XmlRem& rem: pax.rems->rems)
        {
            xmlNodePtr remNode = newChild(remsNode, "rem");
            NewTextChild(remNode, "rem_code", rem.rem_code);
            NewTextChild(remNode, "rem_text", rem.rem_text);
        }
    }

    NewTextChild(paxNode, "doco");
    NewTextChild(paxNode, "addresses");
    NewTextChild(paxNode, "bag_pool_num");
    NewTextChild(paxNode, "transfer");
    NewTextChild(paxNode, "subclass", pax.subclass);
    NewTextChild(paxNode, "tid", pax.tid);

    NewTextChild(segNode, "paid_bag_emd");

    NewTextChild(savePaxNode, "excess");
    NewTextChild(savePaxNode, "hall", 1);
    NewTextChild(savePaxNode, "bag_refuse");
    xmlNodePtr paidBagsNode = newChild(savePaxNode, "paid_bags");
    xmlNodePtr paidBagNode = newChild(paidBagsNode, "paid_bag");
    NewTextChild(paidBagNode, "bag_type");
    NewTextChild(paidBagNode, "weight", 0);
    NewTextChild(paidBagNode, "rate_id");
    NewTextChild(paidBagNode, "rate_trfer");

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
    xmlNodePtr reseatNode = newChild(reqNode, "Reseat");
    NewTextChild(reseatNode, "trip_id", paxSeg.point_dep);
    NewTextChild(reseatNode, "pax_id",  pax.pax_id);
    NewTextChild(reseatNode, "xname",   seat.col());
    NewTextChild(reseatNode, "yname",   seat.row());
    NewTextChild(reseatNode, "tid",     pax.tid);
    NewTextChild(reseatNode, "question_reseat", "");

    initReqInfo();

    LogTrace(TRACE3) << "reseat pax query:\n" << XMLTreeToText(reqNode->doc);
    SalonFormInterface::instance()->Reseat(getRequestCtxt(), reseatNode, resNode);
    LogTrace(TRACE3) << "reseat pax answer:\n" << XMLTreeToText(resNode->doc);
    return LoadPax(paxSeg.point_dep, pax.reg_no);
}


GetAdvTripListXmlResult AstraEngine::GetAdvTripList(const boost::gregorian::date& depDate)
{
    xmlNodePtr reqNode = getQueryNode(),
               resNode = getAnswerNode();

    xmlNodePtr advTripListNode = newChild(reqNode, "GetAdvTripList");
    NewTextChild(advTripListNode, "date", HelpCpp::string_cast(depDate,
                                                               "%d.%m.%Y 00:00:00"));
    xmlNodePtr filterNode = newChild(advTripListNode, "filter");
    NewTextChild(filterNode, "pr_takeoff", 1); // TODO what is it?

    xmlNodePtr viewNode = newChild(advTripListNode, "codes_fmt");
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

    xmlNodePtr showNode = newChild(reqNode, "Show");
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
    airline = left.substr(0, airlLen); // TODO авк может иметь трёхсимвольный код
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

    BASIC::TDateTime now_local = UTCToLocal(BASIC::NowUTC(), AirpTZRegion(airpDep));
    int now_day_local = ASTRA::NoExists,
        now_mon_local = ASTRA::NoExists,
        now_year_local = ASTRA::NoExists;
    BASIC::DecodeDate(now_local,
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

    BASIC::TDateTime scd_local = ASTRA::NoExists;
    BASIC::EncodeDate(now_year_local, now_mon_local, now_day_local, scd_local);

    return iatci::FlightDetails(airl,
                                Ticketing::getFlightNum(flNum),
                                airpDep,
                                airpArv,
                                BASIC::DateTimeToBoost(scd_local).date());
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
    filter.scd_out  = BASIC::BoostToDateTime(depDateTime);
    return filter;
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

static void applyDocUpdate(XmlPax& pax, const iatci::UpdatePaxDetails::UpdateDocInfo& updDoc)
{
    LogTrace(TRACE3) << __FUNCTION__ << " with act code: " << updDoc.actionCodeAsString();
    XmlPaxDoc newDoc;
    newDoc.no = updDoc.no();
    newDoc.type = updDoc.docType();
    newDoc.birth_date = BASIC::boostDateToAstraFormatStr(updDoc.birthDate());
    newDoc.expiry_date = BASIC::boostDateToAstraFormatStr(updDoc.expiryDate());
    newDoc.surname = updDoc.surname();
    newDoc.first_name = updDoc.name();
    newDoc.second_name = updDoc.secondName();
    newDoc.nationality = updDoc.nationality();
    newDoc.issue_country = updDoc.issueCountry();
    newDoc.gender = updDoc.gender();
    pax.doc = newDoc;
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

int findArvPointId(int pointDep, const std::string& arvPort)
{
    FlightPoints flPoints;
    flPoints.Get(pointDep);
    int pointArv = 0;
    // найдём point_id прибытия на сегменте, соответствующий искомому пассажиру
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

iatci::Result checkinIatciPax(const iatci::CkiParams& ckiParams)
{
    LogTrace(TRACE3) << __FUNCTION__ << " by "
                     << "airp_dep[" << ckiParams.flight().depPort() << "]; "
                     << "airline["  << ckiParams.flight().airline() << "]; "
                     << "flight["   << ckiParams.flight().flightNum() << "]; "
                     << "dep_date[" << ckiParams.flight().depDate() << "]; "
                     << "surname["  << ckiParams.pax().surname() << "]; "
                     << "name["     << ckiParams.pax().name() << "] ";

    int pointDep = astra_api::findDepPointId(ckiParams.flight().depPort(),
                                             ckiParams.flight().airline(),
                                             ckiParams.flight().flightNum().get(),
                                             ckiParams.flight().depDate());

    LogTrace(TRACE3) << __FUNCTION__ << " point_dep[" << pointDep << "]";

    SearchPaxXmlResult searchPaxXmlRes =
            AstraEngine::singletone().SearchPax(pointDep,
                                                ckiParams.pax().surname(),
                                                ckiParams.pax().name(),
                                                "К");

    if(searchPaxXmlRes.lTrip.size() == 0) {
        // пассажир не найден в списке незарегистрированных
        // тогда поищем его в списке зарегистрированных
        PaxListXmlResult paxListXmlRes = AstraEngine::singletone().PaxList(pointDep);
        std::list<XmlPax> wantedPaxes = paxListXmlRes.applyNameFilter(ckiParams.pax().surname(),
                                                                      ckiParams.pax().name());
        if(!wantedPaxes.empty()) {
            // нашли пассажира в списке зарегистрированных
            throw tick_soft_except(STDLOG, AstraErr::PAX_ALREADY_CHECKED_IN);
        } else {
            throw tick_soft_except(STDLOG, AstraErr::PAX_SURNAME_NF);
        }
    } else if(searchPaxXmlRes.lTrip.size() > 1) {
        // найдено несколько пассажиров
        throw tick_soft_except(STDLOG, AstraErr::TOO_MANY_PAX_WITH_SAME_SURNAME);
    }

    XmlTrip& paxTrip = searchPaxXmlRes.lTrip.front();
    fillPaxTrip(paxTrip, ckiParams);
    // SavePax
    LoadPaxXmlResult loadPaxXmlRes = AstraEngine::singletone().SavePax(pointDep,
                                                                       paxTrip);
    if(loadPaxXmlRes.lSeg.empty()) {
        // не смогли зарегистрировать
        throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR, "Unable to checkin pax");
    }

    return loadPaxXmlRes.toIatciFirst(iatci::Result::Checkin,
                                      iatci::Result::Ok);
}

iatci::Result checkinIatciPax(xmlNodePtr reqNode, xmlNodePtr ediResNode)
{
    LoadPaxXmlResult loadPaxXmlRes = AstraEngine::singletone().SavePax(reqNode, ediResNode);
    if(loadPaxXmlRes.lSeg.empty()) {
        // не смогли зарегистрировать
        throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR, "Unable to checkin pax");
    }
    return loadPaxXmlRes.toIatciFirst(iatci::Result::Checkin,
                                      iatci::Result::Ok);
}

iatci::Result updateIatciPax(const iatci::CkuParams& ckuParams)
{
    int pointDep = astra_api::findDepPointId(ckuParams.flight().depPort(),
                                             ckuParams.flight().airline(),
                                             ckuParams.flight().flightNum().get(),
                                             ckuParams.flight().depDate());

    LoadPaxXmlResult loadPaxXmlRes = LoadPax__(pointDep,
                                               ckuParams.pax().surname(),
                                               ckuParams.pax().name());

    XmlSegment& paxSegForUpdate = loadPaxXmlRes.lSeg.front();
    ASSERT(paxSegForUpdate.passengers.size() == 1);

    XmlPax& paxForUpdate = paxSegForUpdate.passengers.front();
    if(ckuParams.updSeat()) {
        tst();
        applySeatUpdate(paxForUpdate, *ckuParams.updSeat());
    }

    if(ckuParams.updPax()) {
        tst();
        // Doc
        if(ckuParams.updPax()->doc()) {
            tst();
            applyDocUpdate(paxForUpdate, *ckuParams.updPax()->doc());
        }
    }

    // Rems
    if(ckuParams.updService()) {
        tst();
        applyRemsUpdate(paxForUpdate, *ckuParams.updService());
    }

    if(ckuParams.updSeat()) {
        loadPaxXmlRes = AstraEngine::singletone().ReseatPax(paxSegForUpdate);
        return loadPaxXmlRes.toIatciFirst(iatci::Result::Update,
                                          iatci::Result::Ok);
    } else {
        // SavePax
        loadPaxXmlRes = AstraEngine::singletone().SavePax(paxSegForUpdate);
        if(loadPaxXmlRes.lSeg.empty()) {
            // не смогли зарегистрировать
            throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR, "Unable to checkin pax");
        }

        return loadPaxXmlRes.toIatciFirst(iatci::Result::Update,
                                          iatci::Result::Ok);
    }

}

iatci::Result cancelCheckinIatciPax(const iatci::CkxParams& ckxParams)
{
    // отмена регистрации
    // работает пока по имени/фамилии/рейсу
    // но, скорее всего, должна работать по id, полученному после регистрации

    LogTrace(TRACE3) << __FUNCTION__ << " by "
                     << "airp_dep[" << ckxParams.flight().depPort() << "]; "
                     << "airline["  << ckxParams.flight().airline() << "]; "
                     << "flight["   << ckxParams.flight().flightNum() << "]; "
                     << "dep_date[" << ckxParams.flight().depDate() << "]; "
                     << "surname["  << ckxParams.pax().surname() << "]; "
                     << "name["     << ckxParams.pax().name() << "] ";

    int pointDep = astra_api::findDepPointId(ckxParams.flight().depPort(),
                                             ckxParams.flight().airline(),
                                             ckxParams.flight().flightNum().get(),
                                             ckxParams.flight().depDate());

    LoadPaxXmlResult loadPaxXmlRes = LoadPax__(pointDep,
                                               ckxParams.pax().surname(),
                                               ckxParams.pax().name());

    XmlSegment& paxSegForCancel = loadPaxXmlRes.lSeg.front();
    ASSERT(paxSegForCancel.passengers.size() == 1);

    XmlPax& paxForCancel = paxSegForCancel.passengers.front();
    paxForCancel.refuse = "А"; // TODO пока причина отмены всегда "А - Ошибка агента"

    // SavePax
    loadPaxXmlRes = AstraEngine::singletone().SavePax(paxSegForCancel);

    if(!loadPaxXmlRes.lSeg.empty()) {
        tst();
        return loadPaxXmlRes.toIatciFirst(iatci::Result::Cancel,
                                          iatci::Result::OkWithNoData);
    }

    tst();
    // в результат надо положить сегмент - взять его здесь можно из ckxParams
    return iatci::Result::makeCancelResult(iatci::Result::OkWithNoData,
                                           ckxParams.flight());
}

iatci::Result cancelCheckinIatciPax(xmlNodePtr reqNode, xmlNodePtr ediResNode)
{
    tst();
    LoadPaxXmlResult loadPaxXmlRes = AstraEngine::singletone().SavePax(reqNode, ediResNode);
    if(!loadPaxXmlRes.lSeg.empty()) {
        tst();
        return loadPaxXmlRes.toIatciFirst(iatci::Result::Cancel,
                                          iatci::Result::OkWithNoData);
    }

    tst();
    // в результат надо положить сегмент - достать его здесь можно, например, из reqNode
    return LoadPaxXmlResult(reqNode).toIatciFirst(iatci::Result::Cancel,
                                                  iatci::Result::OkWithNoData);
}

iatci::Result fillPaxList(const iatci::PlfParams& plfParams)
{
    LogTrace(TRACE3) << __FUNCTION__ << " by "
                     << "airp_dep[" << plfParams.flight().depPort() << "]; "
                     << "airline["  << plfParams.flight().airline() << "]; "
                     << "flight["   << plfParams.flight().flightNum() << "]; "
                     << "dep_date[" << plfParams.flight().depDate() << "]; "
                     << "surname["  << plfParams.pax().surname() << "]; "
                     << "name["     << plfParams.pax().name() << "] ";

    int pointDep = astra_api::findDepPointId(plfParams.flight().depPort(),
                                             plfParams.flight().airline(),
                                             plfParams.flight().flightNum().get(),
                                             plfParams.flight().depDate());

    LoadPaxXmlResult loadPaxXmlRes = LoadPax__(pointDep,
                                               plfParams.pax().surname(),
                                               plfParams.pax().name());

    if(loadPaxXmlRes.lSeg.empty()) {
        tst();
        throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR, "Unable to load pax");
    }

    tst();
    return loadPaxXmlRes.toIatciFirst(iatci::Result::Passlist,
                                      iatci::Result::Ok);
}

iatci::Result fillSeatmap(const iatci::SmfParams& smfParams)
{
    LogTrace(TRACE3) << __FUNCTION__ << " by "
                     << "airp_dep[" << smfParams.flight().depPort() << "]; "
                     << "airline["  << smfParams.flight().airline() << "]; "
                     << "flight["   << smfParams.flight().flightNum() << "]; "
                     << "dep_date[" << smfParams.flight().depDate() << "]; ";

    int pointDep = astra_api::findDepPointId(smfParams.flight().depPort(),
                                             smfParams.flight().airline(),
                                             smfParams.flight().flightNum().get(),
                                             smfParams.flight().depDate());

    LogTrace(TRACE3) << "seatmap point id: " << pointDep;

    GetSeatmapXmlResult seatmapXmlRes = AstraEngine::singletone().GetSeatmap(pointDep);
    return seatmapXmlRes.toIatci();
}

iatci::Result printBoardingPass(const iatci::BprParams& bprParams)
{
    LogTrace(TRACE3) << __FUNCTION__ << " by "
                     << "airp_dep[" << bprParams.flight().depPort() << "]; "
                     << "airline["  << bprParams.flight().airline() << "]; "
                     << "flight["   << bprParams.flight().flightNum() << "]; "
                     << "dep_date[" << bprParams.flight().depDate() << "]; "
                     << "surname["  << bprParams.pax().surname() << "]; "
                     << "name["     << bprParams.pax().name() << "] ";

    int pointDep = astra_api::findDepPointId(bprParams.flight().depPort(),
                                             bprParams.flight().airline(),
                                             bprParams.flight().flightNum().get(),
                                             bprParams.flight().depDate());

    LoadPaxXmlResult loadPaxXmlRes = LoadPax__(pointDep,
                                               bprParams.pax().surname(),
                                               bprParams.pax().name());

    if(loadPaxXmlRes.lSeg.empty()) {
        tst();
        throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR, "Unable to find pax");
    }

    savePrintBP(loadPaxXmlRes);

    tst();
    return loadPaxXmlRes.toIatciFirst(iatci::Result::Passlist,
                                      iatci::Result::Ok);
}

/////////////////////////////////////////////////////////////////////////////////////////

namespace xml_entities {

ReqParams::ReqParams(xmlNodePtr rootNode)
    : m_rootNode(rootNode)
{}

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

astra_entities::DocInfo XmlPaxDoc::toDoc() const
{
    return astra_entities::DocInfo(type,
                                   issue_country,
                                   no,
                                   BASIC::boostDateTimeFromAstraFormatStr(expiry_date).date(),
                                   surname,
                                   first_name,
                                   second_name,
                                   nationality,
                                   BASIC::boostDateTimeFromAstraFormatStr(birth_date).date(),
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
                                   paxDoc,
                                   paxRems);
}

//---------------------------------------------------------------------------------------

XmlSegment::XmlSegment()
    : grp_id(ASTRA::NoExists),
      point_dep(ASTRA::NoExists),
      point_arv(ASTRA::NoExists),
      tid(ASTRA::NoExists)
{}

astra_entities::SegmentInfo XmlSegment::toSeg() const
{
    return astra_entities::SegmentInfo(grp_id,
                                       point_dep,
                                       point_arv,
                                       airp_dep,
                                       airp_arv,
                                       cls,
                                       boost::none); // TODO
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
    return (m_seg.point_dep < 0 &&
            m_seg.point_arv < 0 &&
            m_seg.grp_id < 0);
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

//---------------------------------------------------------------------------------------

XmlCheckInTabs::XmlCheckInTabs(xmlNodePtr tabsNode)
{
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
    for(auto tab: tabs()) { if(tab.isEdi()) return true; }
    return false;
}

const std::vector<XmlCheckInTab>& XmlCheckInTabs::tabs() const
{
    return m_tabs;
}

//---------------------------------------------------------------------------------------

XmlTripHeader XmlEntityReader::readTripHeader(xmlNodePtr tripHeaderNode)
{
    ASSERT(tripHeaderNode);

    XmlTripHeader tripHeader;
    tripHeader.flight             = NodeAsString("flight",  tripHeaderNode, "");
    tripHeader.airline            = NodeAsString("airline", tripHeaderNode, "");
    tripHeader.aircode            = NodeAsString("aircode", tripHeaderNode, "");
    tripHeader.flt_no             = NodeAsString("flt_no",  tripHeaderNode, "");
    tripHeader.suffix             = NodeAsString("suffix",  tripHeaderNode, "");
    tripHeader.airp               = NodeAsString("airp",    tripHeaderNode, "");
    tripHeader.scd_out_local      = NodeAsString("scd_out_local", tripHeaderNode, "");
    tripHeader.pr_etl_only        = NodeAsInteger("pr_etl_only",  tripHeaderNode, ASTRA::NoExists);
    tripHeader.pr_etstatus        = NodeAsInteger("pr_etstatus",  tripHeaderNode, ASTRA::NoExists);
    tripHeader.pr_no_ticket_check = NodeAsInteger("pr_no_ticket_check", tripHeaderNode, ASTRA::NoExists);
    return tripHeader;
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
    airp.point_id    = NodeAsInteger("point_id", airpNode, ASTRA::NoExists);
    airp.airp_code   = NodeAsString("airp_code", airpNode, "");
    airp.city_code   = NodeAsString("city_code", airpNode, "");
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
    cls.code       = NodeAsString("code", classNode, "");
    cls.class_view = NodeAsString("class_view", classNode, "");
    if(cls.class_view.empty()) {
        cls.class_view = NodeAsString("name", classNode, "");
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
    flight.airline       = NodeAsString("airline", flightNode, "");
    flight.flt_no        = NodeAsString("flt_no", flightNode, "");
    flight.suffix        = NodeAsString("suffix", flightNode, "");
    flight.scd           = NodeAsString("scd", flightNode, "");
    flight.airp_dep      = NodeAsString("airp_dep", flightNode, "");
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
    item.point_arv   = NodeAsInteger("point_arv", itemNode, ASTRA::NoExists);
    item.cls         = NodeAsString("class", itemNode, "");
    item.noshow      = NodeAsInteger("noshow", itemNode, ASTRA::NoExists);
    item.trnoshow    = NodeAsInteger("trnoshow", itemNode, ASTRA::NoExists);
    item.show        = NodeAsInteger("show", itemNode, ASTRA::NoExists);
    item.free_ok     = NodeAsInteger("free_ok", itemNode, ASTRA::NoExists);
    item.free_goshow = NodeAsInteger("free_goshow", itemNode, ASTRA::NoExists);
    item.nooccupy    = NodeAsInteger("nooccupy", itemNode, ASTRA::NoExists);
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
        doc.no          = NodeAsString("no", docNode, "");
        doc.birth_date  = NodeAsString("birth_date", docNode, "");
        doc.surname     = NodeAsString("surname", docNode, "");
        doc.first_name  = NodeAsString("first_name", docNode, "");
        doc.second_name = NodeAsString("second_name", docNode, "");
        doc.expiry_date = NodeAsString("expiry_date", docNode, "");
        doc.type        = NodeAsString("type", docNode, "");
        doc.nationality = NodeAsString("nationality", docNode, "");
        doc.gender      = NodeAsString("gender", docNode, "");
        doc.issue_country= NodeAsString("issue_country", docNode, "");
    }
    return doc;
}

XmlPax XmlEntityReader::readPax(xmlNodePtr paxNode)
{
    ASSERT(paxNode);

    XmlPax pax;
    pax.pax_id         = NodeAsInteger("pax_id", paxNode, ASTRA::NoExists);
    pax.grp_id         = NodeAsInteger("grp_id", paxNode, ASTRA::NoExists);
    pax.cl_grp_id      = NodeAsInteger("cl_grp_id", paxNode, ASTRA::NoExists);
    pax.surname        = NodeAsString("surname", paxNode, "");
    pax.name           = NodeAsString("name",    paxNode, "");
    pax.airp_arv       = NodeAsString("airp_arv", paxNode, "");
    pax.pers_type      = NodeAsString("pers_type", paxNode, "");
    pax.seat_no        = NodeAsString("seat_no", paxNode, "");
    pax.seat_type      = NodeAsString("seat_type", paxNode, "");
    pax.seats          = NodeAsInteger("seats", paxNode, ASTRA::NoExists);
    pax.refuse         = NodeAsString("refuse", paxNode, "");
    pax.reg_no         = NodeAsInteger("reg_no", paxNode, ASTRA::NoExists);
    pax.subclass       = NodeAsString("subclass", paxNode, "");
    pax.bag_pool_num   = NodeAsInteger("bag_pool_num", paxNode, ASTRA::NoExists);
    pax.tid            = NodeAsInteger("tid", paxNode, ASTRA::NoExists);
    pax.ticket_no      = NodeAsString("ticket_no", paxNode, "");
    pax.coupon_no      = NodeAsInteger("coupon_no", paxNode, ASTRA::NoExists);
    pax.ticket_rem     = NodeAsString("ticket_rem", paxNode, "");
    pax.ticket_confirm = NodeAsInteger("ticket_confirm", paxNode, ASTRA::NoExists);
    pax.pr_norec       = NodeAsInteger("pr_norec", paxNode, ASTRA::NoExists);
    pax.pr_bp_print    = NodeAsInteger("pr_bp_print", paxNode, ASTRA::NoExists);
    pax.hall_id        = NodeAsInteger("hall_id", paxNode, ASTRA::NoExists);
    pax.point_arv      = NodeAsInteger("point_arv", paxNode, ASTRA::NoExists);
    pax.user_id        = NodeAsInteger("user_id", paxNode, ASTRA::NoExists);

    // doc
    xmlNodePtr docNode = findNode(paxNode, "document");
    if(docNode != NULL) {
        pax.doc = XmlEntityReader::readDoc(docNode);
    }

    // remarks
    xmlNodePtr remsNode = findNode(paxNode, "rems");
    if(remsNode != NULL) {
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
    trferSeg.num          = NodeAsInteger("num", trferSegNode, ASTRA::NoExists);
    trferSeg.airline      = NodeAsString("airline", trferSegNode, "");
    trferSeg.flt_no       = NodeAsString("flt_no",  trferSegNode, "");
    trferSeg.local_date   = NodeAsString("local_date", trferSegNode, "");
    trferSeg.airp_dep     = NodeAsString("airp_dep", trferSegNode, "");
    trferSeg.airp_arv     = NodeAsString("airp_arv", trferSegNode, "");
    trferSeg.subclass     = NodeAsString("subclass", trferSegNode, "");
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
    pnr.pnr_id   = NodeAsInteger("pnr_id", pnrNode, ASTRA::NoExists);
    pnr.airp_arv = NodeAsString("airp_arv", pnrNode, "");
    pnr.subclass = NodeAsString("subclass", pnrNode, "");
    pnr.cls      = NodeAsString("class", pnrNode, "");

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
    trip.airline  = NodeAsString("airline", tripNode, "");
    trip.flt_no   = NodeAsString("flt_no", tripNode, "");
    trip.scd      = NodeAsString("scd", tripNode, "");
    trip.airp_dep = NodeAsString("airp_dep", tripNode, "");

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
    seg.grp_id    = NodeAsInteger("grp_id", segNode, ASTRA::NoExists);
    seg.point_dep = NodeAsInteger("point_dep", segNode, ASTRA::NoExists);
    seg.airp_dep  = NodeAsString("airp_dep", segNode, "");
    seg.point_arv = NodeAsInteger("point_arv", segNode, ASTRA::NoExists);
    seg.airp_arv  = NodeAsString("airp_arv", segNode, "");
    seg.cls       = NodeAsString("class", segNode, "");
    seg.status    = NodeAsString("status", segNode, "");
    seg.bag_refuse= NodeAsString("bag_refuse", segNode, "");
    seg.tid       = NodeAsInteger("tid", segNode, ASTRA::NoExists);
    seg.city_arv  = NodeAsString("city_arv", segNode, "");

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
    place.x         = NodeAsInteger("x", placeNode, ASTRA::NoExists);
    place.y         = NodeAsInteger("y", placeNode, ASTRA::NoExists);
    place.elem_type = NodeAsString("elem_type", placeNode, "");
    place.cls       = NodeAsString("class", placeNode, "");
    place.xname     = NodeAsString("xname", placeNode, "");
    place.yname     = NodeAsString("yname", placeNode, "");
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

SearchPaxXmlResult::SearchPaxXmlResult(xmlNodePtr node)
{
    xmlNodePtr tripsNode = findNode(node, "trips");
    if(tripsNode != NULL) {
        this->lTrip = XmlEntityReader::readTrips(tripsNode);
    }
}

std::list<XmlPax> SearchPaxXmlResult::applyNameFilter(const std::string& surname,
                                                      const std::string& name)
{
    std::list<XmlPax> res;
    for(const XmlTrip& trip: lTrip) {
        for(const XmlPnr& pnr: trip.pnrs) {
            for(const XmlPax& pax: pnr.passengers) {
                // TODO необходимо усложнить функцию сравнения (trim, translit)
                if(surname == pax.surname && name == pax.name) {
                    res.push_back(pax);
                }
            }
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

std::vector<iatci::Result> LoadPaxXmlResult::toIatci(iatci::Result::Action_e action,
                                                     iatci::Result::Status_e status) const
{
    std::vector<iatci::Result> lRes;
    for(auto& seg: lSeg)
    {
        // flight details
        std::string airl     = seg.trip_header.airline;
        std::string flNum    = seg.trip_header.flt_no;
        std::string scd_local= seg.trip_header.scd_out_local;
        std::string airp_dep = seg.airp_dep;
        std::string airp_arv = seg.airp_arv;

        if(airl.empty()) {
            airl = seg.mark_flight.airline;
        }
        if(flNum.empty()) {
            flNum = seg.mark_flight.flt_no;
        }
        if(scd_local.empty()) {
            scd_local = seg.mark_flight.scd;
        }

        ASSERT(!scd_local.empty());
        boost::posix_time::ptime scd_dep_date_time = BASIC::boostDateTimeFromAstraFormatStr(scd_local);

        boost::gregorian::date scd_dep_date = scd_dep_date_time.date();
        boost::gregorian::date scd_arr_date = boost::gregorian::date();

        iatci::FlightDetails flightDetails(airl,
                                           Ticketing::getFlightNum(flNum),
                                           airp_dep,
                                           airp_arv,
                                           scd_dep_date,
                                           scd_arr_date,
                                           boost::posix_time::time_duration(boost::posix_time::not_a_date_time),
                                           boost::posix_time::time_duration(boost::posix_time::not_a_date_time));

        // pax details
        ASSERT(seg.passengers.size() == 1);
        const XmlPax& pax = seg.passengers.front();

        boost::optional<iatci::PaxDetails::DocInfo> paxDoc;
        if(pax.doc)
        {
            boost::gregorian::date birthDate = BASIC::boostDateTimeFromAstraFormatStr(pax.doc->birth_date).date(),
                                  expiryDate = BASIC::boostDateTimeFromAstraFormatStr(pax.doc->expiry_date).date();
            paxDoc = iatci::PaxDetails::DocInfo(pax.doc->type,
                                                pax.doc->issue_country,
                                                pax.doc->no,
                                                pax.doc->surname,
                                                pax.doc->first_name,
                                                pax.doc->second_name,
                                                pax.doc->gender,
                                                pax.doc->nationality,
                                                birthDate,
                                                expiryDate);
        }

        std::string surname  = pax.surname;
        std::string name     = pax.name;
        std::string pers_type= pax.pers_type;

        iatci::PaxDetails paxDetails(surname,
                                     name,
                                     iatci::PaxDetails::Adult,// TODO
                                     paxDoc);

        boost::optional<iatci::FlightSeatDetails> seatDetails;
        boost::optional<iatci::ServiceDetails> serviceDetails;
        if(status != iatci::Result::OkWithNoData)
        {
            // seat details
            ASSERT(!pax.seat_no.empty());
            seatDetails = iatci::FlightSeatDetails(pax.seat_no,
                                                   pax.subclass,
                                                   "", // securityCode
                                                   iatci::SeatDetails::None); // non-smoking ind



            serviceDetails = iatci::ServiceDetails();
            if(pax.ticket_rem == "TKNE") {
                serviceDetails->addSsrTkne(pax.ticket_no,
                                           pax.coupon_no,
                                           false);
            } else {
                serviceDetails->addSsr(pax.ticket_rem,
                                       pax.ticket_no);
            }

            // rems
            if(pax.rems)
            {
                for(const XmlRem& rem: pax.rems->rems)
                {
                    serviceDetails->addSsr(rem.rem_code,
                                           rem.rem_text);
                }
            }
        }
        // TODO

        lRes.push_back(iatci::Result::makeResult(action,
                                                 status,
                                                 flightDetails,
                                                 paxDetails,
                                                 seatDetails,
                                                 boost::none,
                                                 boost::none,
                                                 boost::none,
                                                 boost::none,
                                                 boost::none,
                                                 serviceDetails));
    }

    return lRes;
}

iatci::Result LoadPaxXmlResult::toIatciFirst(iatci::Result::Action_e action,
                                             iatci::Result::Status_e status) const
{
    std::vector<iatci::Result> lRes = toIatci(action, status);
    ASSERT(!lRes.empty());
    return lRes.front();
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
                                                    const std::string& name)
{
    std::list<XmlPax> res;
    for(const XmlPax& pax: lPax) {
        // TODO необходимо усложнить функцию сравнения (trim, translit)
        if(surname == pax.surname && name == pax.name) {
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

std::list<XmlTrip> GetAdvTripListXmlResult::applyFlightFilter(const std::string& flightName)
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

iatci::Result GetSeatmapXmlResult::toIatci() const
{
    if(lPlacelist.empty() || filterRoutes.items.empty()) {
        LogError(STDLOG) << "Seatmap failed!";
        throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR);
    }

    return iatci::Result::makeSeatmapResult(iatci::Result::Ok,
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
                         const boost::optional<MarketingInfo>& markFlight)
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
            left.m_doc       == right.m_doc &&
            left.m_rems     == right.m_rems);
}

bool operator!=(const PaxInfo& left, const PaxInfo& right)
{
    return !(left == right);
}

}//namespace astra_entities

}//namespace astra_api
