#include "astra_api.h"
#include "astra_msg.h"
#include "astra_misc.h"
#include "astra_utils.h"
#include "basic.h"
#include "points.h"
#include "checkin.h"
#include "tripinfo.h"

#include <serverlib/cursctl.h>
#include <serverlib/xml_tools.h>
#include <serverlib/xml_stuff.h>
#include <serverlib/dates_io.h>
#include <jxtlib/jxt_cont.h>
#include <jxtlib/jxt_cont_impl.h>
#include <etick/exceptions.h>

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


namespace astra_api
{

using namespace xml_entities;
using namespace Ticketing;
using namespace Ticketing::TickExceptions;

static int GetArvPointId(int pointDep, const XmlPnr& paxPnr);

namespace astra_entities
{

MarketingInfo::MarketingInfo(const std::string& airline,
                             unsigned flightNum,
                             const std::string& flightSuffix,
                             const boost::gregorian::date& scdDepDate,
                             const std::string& airpDep)
    : m_airline(airline), m_flightNum(flightNum), m_flightSuffix(flightSuffix),
      m_scdDepDate(scdDepDate), m_airpDep(airpDep)
{
}

//---------------------------------------------------------------------------------------

SegmentInfo::SegmentInfo(int pointDep, int pointArv,
                         const std::string& airpDep, const std::string& airpArv,
                         const std::string& cls,
                         const MarketingInfo& markFlight)
    : m_pointDep(pointDep), m_pointArv(pointArv),
      m_airpDep(airpDep), m_airpArv(airpArv),
      m_cls(cls), m_markFlight(markFlight)
{
}

//---------------------------------------------------------------------------------------

DocInfo::DocInfo(const std::string& docNum,
        const std::string& surname, const std::string& name,
        const boost::gregorian::date& birthDate)
    : m_docNum(docNum),
      m_surname(surname), m_name(name),
      m_birthDate(birthDate)
{
}

//---------------------------------------------------------------------------------------

PaxInfo::PaxInfo(int paxId,
                 const std::string& surname, const std::string& name,
                 ASTRA::TPerson persType,
                 const std::string& ticketNum, unsigned couponNum,
                 const std::string& ticketRem, const DocInfo &doc)
    : m_paxId(paxId), m_surname(surname), m_name(name),
      m_persType(persType), m_ticketNum(ticketNum), m_couponNum(couponNum),
      m_ticketRem(ticketRem), m_doc(doc)
{
}

}//namespace astra_entities

/////////////////////////////////////////////////////////////////////////////////////////

namespace {

/* FormatStr = 'DD.MM.YYYY HH24:MI:SS' */
boost::posix_time::ptime boostDateTimeFromAstraFormatStr(const std::string& str)
{
    if(str.length() != 19) { // DD.MM.YYYY HH24:MI:SS
        LogWarning(STDLOG) << "Invalid date/time: " << str;
        return boost::posix_time::ptime();
    }
    BASIC::TDateTime dt = ASTRA::NoExists;
    ASSERT(!BASIC::StrToDateTime(str.c_str(), dt));
    if(dt == ASTRA::NoExists) {
        LogWarning(STDLOG) << "Invalid date/time: " << str;
        return boost::posix_time::ptime();
    }
    return BASIC::DateTimeToBoost(dt);
}

void fillPaxTrip(XmlTrip& paxTrip, const iatci::CkiParams& ckiParams)
{
    paxTrip.status = "K"; // �ᥣ�� "K" ?
    paxTrip.pnr().pax().pers_type = "��";
    if(ckiParams.seat()) {
        paxTrip.pnr().pax().seat_no = ckiParams.seat()->seat();
    }
}

SearchPaxXmlResult parseSearchPaxAnswer(xmlNodePtr answerNode)
{
    SearchPaxXmlResult res;
    xmlNodePtr tripsNode = findNode(answerNode, "trips");
    if(tripsNode != NULL)
    {
        // 横� �� trip-��
        for(xmlNodePtr tripNode = tripsNode->children;
            tripNode != NULL; tripNode = tripNode->next)
        {
            XmlTrip trip;
            trip.point_id = NodeAsInteger("point_id", tripNode);
            trip.airline  = NodeAsString("airline", tripNode);
            trip.flt_no   = NodeAsString("flt_no", tripNode);
            trip.scd      = NodeAsString("scd", tripNode);
            trip.airp_dep = NodeAsString("airp_dep", tripNode);

            xmlNodePtr groupsNode = findNode(tripNode, "groups");
            if(groupsNode != NULL)
            {
                // 横� �� pnr-��
                for(xmlNodePtr pnrNode = groupsNode->children;
                    pnrNode != NULL; pnrNode = pnrNode->next)
                {
                    XmlPnr pnr;
                    pnr.pnr_id   = NodeAsInteger("pnr_id", pnrNode);
                    pnr.airp_arv = NodeAsString("airp_arv", pnrNode);
                    pnr.subclass = NodeAsString("subclass", pnrNode);
                    pnr.cls      = NodeAsString("class", pnrNode);

                    xmlNodePtr passengersNode = findNode(pnrNode, "passengers");
                    if(passengersNode != NULL)
                    {
                        // 横� �� pax-��
                        for(xmlNodePtr paxNode = passengersNode->children;
                            paxNode != NULL; paxNode = paxNode->next)
                        {
                            XmlPax pax;
                            pax.pax_id     = NodeAsInteger("pax_id", paxNode);
                            pax.surname    = NodeAsString("surname", paxNode);
                            pax.name       = NodeAsString("name", paxNode);
                            pax.ticket_no  = NodeAsString("ticket_no", paxNode, "");
                            pax.coupon_no  = NodeAsInteger("coupon_no", paxNode, ASTRA::NoExists);
                            pax.ticket_rem = NodeAsString("ticket_rem", paxNode, "");

                            xmlNodePtr docNode = findNode(paxNode, "document");
                            if(docNode != NULL)
                            {
                                XmlPaxDoc doc;
                                doc.no = NodeAsString(docNode);
                                if(doc.no.empty())
                                {
                                    doc.no          = NodeAsString("no", docNode, "");
                                    doc.type        = NodeAsString("type", docNode, "");
                                    doc.birth_date  = NodeAsString("birth_date", docNode, "");
                                    doc.expiry_date = NodeAsString("expiry_date", docNode, "");
                                    doc.surname     = NodeAsString("surname", docNode, "");
                                    doc.first_name  = NodeAsString("first_name", docNode, "");
                                }

                                pax.doc = doc;
                            }

                            xmlNodePtr remsNode = findNode(paxNode, "rems");
                            if(remsNode != NULL)
                            {
                                // 横� �� rems-��
                                for(xmlNodePtr remNode = remsNode->children;
                                    remNode != NULL; remNode = remNode->next)
                                {
                                    XmlRem rem;
                                    rem.rem_code = NodeAsString("rem_code", remNode);
                                    rem.rem_text = NodeAsString("rem_text", remNode);

                                    pax.rems.push_back(rem);
                                }
                            }

                            pnr.passengers.push_back(pax);
                        }
                    }

                    xmlNodePtr trferSegsNode = findNode(pnrNode, "transfer");
                    if(trferSegsNode != NULL)
                    {
                        // 横� �� ��몮����(�࠭����) ᥣ���⠬
                        for(xmlNodePtr trferSegNode = trferSegsNode->children;
                            trferSegNode != NULL; trferSegNode = trferSegNode->next)
                        {
                            XmlTrferSegment trferSeg;
                            trferSeg.num          = NodeAsInteger("num", trferSegNode);
                            trferSeg.airline      = NodeAsString("airline", trferSegNode);
                            trferSeg.flt_no       = NodeAsString("flt_no",  trferSegNode);
                            trferSeg.local_date   = NodeAsString("local_date", trferSegNode);
                            trferSeg.airp_dep     = NodeAsString("airp_dep", trferSegNode);
                            trferSeg.airp_arv     = NodeAsString("airp_arv", trferSegNode);
                            trferSeg.subclass     = NodeAsString("subclass", trferSegNode);
                            trferSeg.trfer_permit = NodeAsInteger("trfer_permit", trferSegNode);

                            pnr.trfer_segments.push_back(trferSeg);
                        }
                    }

                    xmlNodePtr pnrAddrsNode = findNode(pnrNode, "pnr_addrs");
                    if(pnrAddrsNode != NULL)
                    {
                        // 横� �� recloc-��
                        for(xmlNodePtr pnrReclocNode = pnrAddrsNode->children;
                            pnrReclocNode != NULL; pnrReclocNode = pnrReclocNode->next)
                        {
                            XmlPnrRecloc recloc;
                            recloc.airline = NodeAsString("airline", pnrReclocNode);
                            recloc.recloc  = NodeAsString("addr", pnrReclocNode);

                            pnr.pnr_reclocs.push_back(recloc);
                        }
                    }

                    trip.pnrs.push_back(pnr);
                }
            }

            res.lTrip.push_back(trip);
        }
    }

    return res;
}

LoadPaxXmlResult parseLoadPaxAnswer(xmlNodePtr answerNode)
{
    tst();
    LoadPaxXmlResult res;
    xmlNodePtr segsNode = findNode(answerNode, "segments");
    if(segsNode != NULL)
    {
        tst();
        // 横� �� segment-��
        for(xmlNodePtr segNode = segsNode->children;
            segNode != NULL; segNode = segNode->next)
        {
            tst();
            XmlSegment seg;

            // trip header
            xmlNodePtr tripHeaderNode = findNode(segNode, "tripheader");
            if(tripHeaderNode != NULL)
            {
                seg.tripHeader.flight  = NodeAsString("flight",  tripHeaderNode, "");
                seg.tripHeader.airline = NodeAsString("airline", tripHeaderNode);
                seg.tripHeader.aircode = NodeAsString("aircode", tripHeaderNode);
                seg.tripHeader.flt_no  = NodeAsString("flt_no",  tripHeaderNode);
                seg.tripHeader.suffix  = NodeAsString("suffix",  tripHeaderNode);
                seg.tripHeader.airp    = NodeAsString("airp",    tripHeaderNode);
                seg.tripHeader.scd_out_local = NodeAsString("scd_out_local", tripHeaderNode, "");
                seg.tripHeader.pr_etl_only   = NodeAsInteger("pr_etl_only",  tripHeaderNode, 0);
                seg.tripHeader.pr_etstatus   = NodeAsInteger("pr_etstatus",  tripHeaderNode, 0);
                seg.tripHeader.pr_no_ticket_check = NodeAsInteger("pr_no_ticket_check", tripHeaderNode, 0);
            }

            // trip data
            xmlNodePtr tripDataNode = findNode(segNode, "tripdata");
            if(tripDataNode == NULL) {
                // tripDataNode ���������, �᫨ ���ᨬ xml �����
                tripDataNode = segNode;
            }

            if(tripDataNode != NULL)
            {
                xmlNodePtr airpsNode = findNode(tripDataNode, "airps");
                if(airpsNode != NULL)
                {
                    // 横� �� airp-��
                    for(xmlNodePtr airpNode = airpsNode->children;
                        airpNode != NULL; airpNode = airpNode->next)
                    {
                        XmlAirp airp;
                        airp.point_id    = NodeAsInteger("point_id", airpNode);
                        airp.airp_code   = NodeAsString("airp_code", airpNode);
                        airp.city_code   = NodeAsString("city_code", airpNode);
                        airp.target_view = NodeAsString("target_view", airpNode, "");

                        seg.tripData.airps.push_back(airp);
                    }
                }

                xmlNodePtr classesNode = findNode(tripDataNode, "classes");
                if(classesNode != NULL)
                {
                    // 横� �� class-��
                    for(xmlNodePtr classNode = classesNode->children;
                        classNode != NULL; classNode = classNode->next)
                    {
                        XmlClass cls;
                        cls.code       = NodeAsString("code", classNode);
                        cls.class_view = NodeAsString("class_view", classNode, "");
                        if(cls.class_view.empty()) {
                            cls.class_view = NodeAsString("name", classNode, "");
                        }
                        cls.cfg        = NodeAsInteger("cfg", classNode);

                        seg.tripData.classes.push_back(cls);
                    }
                }

                xmlNodePtr markFlightsNode = findNode(tripDataNode, "mark_flights");
                if(markFlightsNode != NULL)
                {
                    // 横� �� flight-��
                    for(xmlNodePtr flightNode = markFlightsNode->children;
                        flightNode != NULL; flightNode = flightNode->next)
                    {
                        XmlMarkFlight flight;
                        flight.airline       = NodeAsString("airline", flightNode);
                        flight.flt_no        = NodeAsString("flt_no", flightNode);
                        flight.suffix        = NodeAsString("suffix", flightNode);
                        flight.scd           = NodeAsString("scd", flightNode);
                        flight.airp_dep      = NodeAsString("airp_dep", flightNode);
                        flight.pr_mark_norms = NodeAsInteger("pr_mark_norms", flightNode);

                        seg.tripData.mark_flights.push_back(flight);
                    }
                }

                seg.grp_id    = NodeAsInteger("grp_id", segNode);
                seg.point_dep = NodeAsInteger("point_dep", segNode);
                seg.airp_dep  = NodeAsString("airp_dep", segNode);
                seg.point_arv = NodeAsInteger("point_arv", segNode);
                seg.airp_arv  = NodeAsString("airp_arv", segNode);
                seg.cls       = NodeAsString("class", segNode);
                seg.status    = NodeAsString("status", segNode);
                seg.bag_refuse= NodeAsString("bag_refuse", segNode, "");
                seg.tid       = NodeAsInteger("tid", segNode);
                seg.city_arv  = NodeAsString("city_arv", segNode, "");

                // mark flight
                xmlNodePtr markFlightNode = findNode(segNode, "mark_flight");
                if(markFlightNode != NULL)
                {
                    seg.mark_flight.airline       = NodeAsString("airline", markFlightNode, "");
                    seg.mark_flight.flt_no        = NodeAsString("flt_no", markFlightNode, "");
                    seg.mark_flight.suffix        = NodeAsString("suffix", markFlightNode, "");
                    seg.mark_flight.scd           = NodeAsString("scd", markFlightNode, "");
                    seg.mark_flight.airp_dep      = NodeAsString("airp_dep", markFlightNode, "");
                    seg.mark_flight.pr_mark_norms = NodeAsInteger("pr_mark_norms", markFlightNode, ASTRA::NoExists);
                }

                xmlNodePtr passengersNode = findNode(segNode, "passengers");
                if(passengersNode != NULL)
                {
                    tst();
                    // 横� �� pax-��
                    for(xmlNodePtr paxNode = passengersNode->children;
                        paxNode != NULL; paxNode = paxNode->next)
                    {
                        tst();
                        XmlPax pax;
                        pax.pax_id         = NodeAsInteger("pax_id", paxNode);
                        pax.surname        = NodeAsString("surname", paxNode);
                        pax.name           = NodeAsString("name",    paxNode);
                        pax.pers_type      = NodeAsString("pers_type", paxNode);
                        pax.seat_no        = NodeAsString("seat_no", paxNode, "");
                        pax.seat_type      = NodeAsString("seat_type", paxNode, "");
                        pax.seats          = NodeAsInteger("seats", paxNode, ASTRA::NoExists);
                        pax.refuse         = NodeAsString("refuse", paxNode);
                        pax.reg_no         = NodeAsInteger("reg_no", paxNode, ASTRA::NoExists);
                        pax.subclass       = NodeAsString("subclass", paxNode);
                        pax.bag_pool_num   = NodeAsInteger("bag_pool_num", paxNode, 0);
                        pax.tid            = NodeAsInteger("tid", paxNode);
                        pax.ticket_no      = NodeAsString("ticket_no", paxNode);
                        pax.coupon_no      = NodeAsInteger("coupon_no", paxNode);
                        pax.ticket_rem     = NodeAsString("ticket_rem", paxNode);
                        pax.ticket_confirm = NodeAsInteger("ticket_confirm", paxNode);
                        pax.pr_norec       = NodeAsInteger("pr_norec", paxNode, ASTRA::NoExists);
                        pax.pr_bp_print    = NodeAsInteger("pr_bp_print", paxNode, ASTRA::NoExists);

                        // doc
                        xmlNodePtr docNode = findNode(paxNode, "document");
                        if(docNode != NULL)
                        {
                            XmlPaxDoc doc;
                            doc.no         = NodeAsString("no", docNode, "");
                            doc.birth_date = NodeAsString("birth_date", docNode, "");
                            doc.surname    = NodeAsString("surname", docNode, "");
                            doc.first_name = NodeAsString("first_name", docNode, "");
                            doc.expiry_date= NodeAsString("expiry_date", docNode, "");
                            doc.type       = NodeAsString("type", docNode, "");

                            pax.doc = doc;
                        }

                        xmlNodePtr remsNode = findNode(paxNode, "rems");
                        if(remsNode != NULL)
                        {
                            // 横� �� rems-��
                            for(xmlNodePtr remNode = remsNode->children;
                                remNode != NULL; remNode = remNode->next)
                            {
                                XmlRem rem;
                                rem.rem_code = NodeAsString("rem_code", remNode);
                                rem.rem_text = NodeAsString("rem_text", remNode);

                                pax.rems.push_back(rem);
                            }
                        }

                        seg.passengers.push_back(pax);
                    }
                }

                xmlNodePtr tripCountersNode = findNode(segNode, "tripcounters");
                if(tripCountersNode != NULL)
                {
                    // 横� �� tripcounter-��
                    for(xmlNodePtr itemNode = tripCountersNode->children;
                        itemNode != NULL; itemNode = itemNode->next)
                    {
                        XmlTripCounterItem item;
                        item.point_arv   = NodeAsInteger("point_arv", itemNode);
                        item.cls         = NodeAsString("class", itemNode);
                        item.noshow      = NodeAsInteger("noshow", itemNode);
                        item.trnoshow    = NodeAsInteger("trnoshow", itemNode);
                        item.show        = NodeAsInteger("show", itemNode);
                        item.free_ok     = NodeAsInteger("free_ok", itemNode);
                        item.free_goshow = NodeAsInteger("free_goshow", itemNode);
                        item.nooccupy    = NodeAsInteger("nooccupy", itemNode);

                        seg.trip_counters.push_back(item);
                    }
                }
            }

            res.lSeg.push_back(seg);
        }
    }

    return res;
}

GetAdvTripListXmlResult parseGetAdvTripListAnswer(xmlNodePtr answerNode)
{
    GetAdvTripListXmlResult res;
    xmlNodePtr tripsNode = findNode(answerNode, "trips");
    if(tripsNode != NULL)
    {
        // 横� �� trip-��
        for(xmlNodePtr tripNode = tripsNode->children;
            tripNode != NULL; tripNode = tripNode->next)
        {
            XmlTrip trip;
            trip.point_id = NodeAsInteger("point_id", tripNode);
            trip.name     = NodeAsString("name", tripNode);
            trip.scd      = NodeAsString("date", tripNode);
            trip.airp_dep = NodeAsString("airp", tripNode);

            res.lTrip.push_back(trip);
        }
    }

    return res;
}

PaxListXmlResult parsePaxListAnswer(xmlNodePtr answerNode)
{
    PaxListXmlResult res;
    xmlNodePtr passengersNode = findNode(answerNode, "passengers");
    if(passengersNode != NULL)
    {
        // 横� �� pax-��
        for(xmlNodePtr paxNode = passengersNode->children;
            paxNode != NULL; paxNode = paxNode->next)
        {
            XmlPax pax;
            //pax.doc // TODO
            pax.reg_no    = NodeAsInteger("reg_no", paxNode);
            pax.surname   = NodeAsString("surname", paxNode);
            pax.name      = NodeAsString("name", paxNode);
            pax.airp_arv  = NodeAsString("airp_arv", paxNode);
            pax.subclass  = NodeAsString("subclass", paxNode);
            pax.seat_no   = NodeAsString("seat_no", paxNode);
            pax.ticket_no = NodeAsString("ticket_no", paxNode);
            pax.ticket_rem= NodeAsString("ticket_rem", paxNode);
            pax.grp_id    = NodeAsInteger("grp_id", paxNode);
            pax.cl_grp_id = NodeAsInteger("cl_grp_id", paxNode);
            pax.hall_id   = NodeAsInteger("hall_id", paxNode);
            pax.point_arv = NodeAsInteger("point_arv", paxNode);
            pax.user_id   = NodeAsInteger("user_id", paxNode);

            res.lPax.push_back(pax);
        }
    }

    return res;
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
    return parsePaxListAnswer(resNode);
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
    return parseLoadPaxAnswer(resNode);
}


SearchPaxXmlResult AstraEngine::SearchPax(int pointId,
                                          const std::string& paxSurname,
                                          const std::string& paxName,
                                          const std::string& paxStatus)
{
    xmlNodePtr reqNode = getQueryNode(),
               resNode = getAnswerNode();

    std::string query = paxSurname + " " + paxName;

    xmlNodePtr searchPaxNode = newChild(reqNode, "SearchPax");
    NewTextChild(searchPaxNode, "point_dep", pointId);
    NewTextChild(searchPaxNode, "pax_status", paxStatus);
    NewTextChild(searchPaxNode, "query", query);

    initReqInfo();

    LogTrace(TRACE3) << "search pax query:\n" << XMLTreeToText(reqNode->doc);
    CheckInInterface::instance()->SearchPax(getRequestCtxt(), searchPaxNode, resNode);
    LogTrace(TRACE3) << "search pax answer:\n" << XMLTreeToText(resNode->doc);
    return parseSearchPaxAnswer(resNode);
}

LoadPaxXmlResult AstraEngine::SavePax(int depPointId, const XmlTrip& paxTrip)
{
    // ���� ���� ����� ��ࠡ��뢠�� ������ ���ᠦ��
    const XmlPnr& pnr = paxTrip.pnr();
    const XmlPax& pax = pnr.pax();

    xmlNodePtr reqNode = getQueryNode(),
               resNode = getAnswerNode();

    xmlNodePtr savePaxNode = newChild(reqNode, "TCkinSavePax");
    NewTextChild(savePaxNode, "agent_stat_period", -1);
    NewTextChild(savePaxNode, "transfer");
    xmlNodePtr segsNode = newChild(savePaxNode, "segments");
    // ���� ����� �������� ⮫쪮 ���� ᥣ����
    xmlNodePtr segNode = newChild(segsNode, "segment");
    NewTextChild(segNode, "point_dep", depPointId);
    NewTextChild(segNode, "point_arv", GetArvPointId(depPointId, pnr));
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

    // ���� ࠡ�⠥� � ����� ���ᠦ�஬
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
        //NewTextChild(paxNode, "document", pax.doc->no);
        xmlNodePtr docNode = newChild(paxNode, "document");
        NewTextChild(docNode, "type", pax.doc->type);
        NewTextChild(docNode, "issue_country");
        NewTextChild(docNode, "no", pax.doc->no);
        NewTextChild(docNode, "nationality");
        NewTextChild(docNode, "birth_date", pax.doc->birth_date);
        NewTextChild(docNode, "expiry_date", pax.doc->expiry_date);
        NewTextChild(docNode, "gender");
        NewTextChild(docNode, "surname", pax.doc->surname);
        NewTextChild(docNode, "first_name", pax.doc->first_name);
    }

    NewTextChild(paxNode, "doco");
    NewTextChild(paxNode, "addresses");
    NewTextChild(paxNode, "subclass", pnr.subclass);
    NewTextChild(paxNode, "bag_pool_num");
    NewTextChild(paxNode, "transfer");

    xmlNodePtr remsNode = newChild(paxNode, "rems");
    BOOST_FOREACH(const XmlRem& rem, pax.rems)
    {
        xmlNodePtr remNode = newChild(remsNode, "rem");
        NewTextChild(remNode, "rem_code", rem.rem_code);
        NewTextChild(remNode, "rem_text", rem.rem_text);
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
    return parseLoadPaxAnswer(resNode);
}

LoadPaxXmlResult AstraEngine::SavePax(int depPointId,
                                      const xml_entities::XmlSegment& paxSeg)
{
    xmlNodePtr reqNode = getQueryNode(),
               resNode = getAnswerNode();

    xmlNodePtr savePaxNode = newChild(reqNode, "TCkinSavePax");
    NewTextChild(savePaxNode, "agent_stat_period", -1);
    NewTextChild(savePaxNode, "transfer");
    xmlNodePtr segsNode = newChild(savePaxNode, "segments");
    // ���� ����� �������� ⮫쪮 ���� ᥣ����
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

    // ���� ࠡ�⠥� � ����� ���ᠦ�஬
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
        //NewTextChild(paxNode, "document", pax.doc->no);
        xmlNodePtr docNode = newChild(paxNode, "document");
        NewTextChild(docNode, "no", pax.doc->no);
        NewTextChild(docNode, "type", pax.doc->type);
        NewTextChild(docNode, "birth_date", pax.doc->birth_date);
        NewTextChild(docNode, "expiry_date", pax.doc->expiry_date);
        NewTextChild(docNode, "surname", pax.doc->surname);
        NewTextChild(docNode, "first_name", pax.doc->first_name);
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
    return parseLoadPaxAnswer(resNode);
}

LoadPaxXmlResult AstraEngine::SavePax(xmlNodePtr reqNode, xmlNodePtr ediResNode)
{
    xmlNodePtr resNode = getAnswerNode();
    LogTrace(TRACE3) << "save pax query:\n" << XMLTreeToText(reqNode->doc);
    CheckInInterface::instance()->SavePax(reqNode, ediResNode, resNode);
    LogTrace(TRACE3) << "save pax answer:\n" << XMLTreeToText(resNode->doc);
    return parseLoadPaxAnswer(resNode);
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
    return parseGetAdvTripListAnswer(resNode);
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
    TReqInfo::Instance()->Initialize("���");
    TReqInfo::Instance()->client_type  = ASTRA::ctTerm;
    TReqInfo::Instance()->desk.version = VERSION_WITH_BAG_POOLS;
    //TReqInfo::Instance()->api_mode     = true; // TODO !!!
    JxtContext::JxtContHolder::Instance()
            ->setHandler(new JxtContext::JxtContHandlerSir(""));

}

//---------------------------------------------------------------------------------------

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

static int GetArvPointId(int depPointId, const XmlPnr &paxPnr)
{
    FlightPoints flPoints;
    flPoints.Get(depPointId);
    int pointArv = flPoints.point_arv;
    // ����� point_id �ਡ��� �� ᥣ����, ᮮ⢥�����騩 �᪮���� ���ᠦ���
    for(FlightPoints::const_iterator it = flPoints.begin();
        it != flPoints.end(); ++it)
    {
        if(paxPnr.airp_arv == it->airp)
        {
            LogTrace(TRACE3) << "PointId[" << it->point_id << "] found by airport arv "
                             << "[" << paxPnr.airp_arv << "]";
            pointArv = it->point_id;
            break;
        }
    }
    return pointArv;
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

void getNextTrip(const std::string& depPort,
                 const std::string& airline,
                 unsigned flNum,
                 const boost::gregorian::date& depDate)
{
    LogTrace(TRACE3) << "Enter to " << __FUNCTION__;
    int depPoint = findDepPointId(depPort, airline, flNum, depDate);
    LogTrace(TRACE3) << "depPoint=" << depPoint;

    TAdvTripRoute route;
    route.GetRouteAfter(ASTRA::NoExists,
                        depPoint,
                        trtNotCurrent, trtNotCancelled);

    LogTrace(TRACE3) << "route.size()=" << route.size();

    if(!route.empty()) {
        LogTrace(TRACE3) << "next point: " << route.front().point_id << "; "
                                           << route.front().airp;
    }
}

//iatci::Result checkinIatciPax(const iatci::CkiParams& ckiParams)
//{
//    LogTrace(TRACE3) << __FUNCTION__ << " by "
//                     << "airp_dep[" << ckiParams.flight().depPort() << "]; "
//                     << "airline["  << ckiParams.flight().airline() << "]; "
//                     << "flight["   << ckiParams.flight().flightNum() << "]; "
//                     << "dep_date[" << ckiParams.flight().depDate() << "]; "
//                     << "surname["  << ckiParams.pax().surname() << "]; "
//                     << "name["     << ckiParams.pax().name() << "] ";

//    int depPointId = astra_api::findDepPointId(ckiParams.flight().depPort(),
//                                               ckiParams.flight().airline(),
//                                               ckiParams.flight().flightNum().get(),
//                                               ckiParams.flight().depDate());

//    LogTrace(TRACE3) << __FUNCTION__ << " point_dep[" << depPointId << "]";

//    SearchPaxXmlResult searchPaxXmlRes;
//    searchPaxXmlRes = AstraEngine::singletone().SearchPax(depPointId,
//                                                          ckiParams.pax().surname(),
//                                                          ckiParams.pax().name(),
//                                                          "�");

//    if(searchPaxXmlRes.lTrip.size() == 0) {
//        // ���ᠦ�� �� ������ � ᯨ᪥ ����ॣ����஢�����
//        // ⮣�� ���饬 ��� � ᯨ᪥ ��ॣ����஢�����
//        PaxListXmlResult paxListXmlRes;
//        paxListXmlRes = AstraEngine::singletone().PaxList(depPointId);
//        std::list<XmlPax> wantedPaxes = paxListXmlRes.applyNameFilter(ckiParams.pax().surname(),
//                                                                      ckiParams.pax().name());
//        if(!wantedPaxes.empty()) {
//            // ��諨 ���ᠦ�� � ᯨ᪥ ��ॣ����஢�����
//            throw tick_soft_except(STDLOG, AstraErr::PAX_ALREADY_CHECKED_IN);
//        } else {
//            throw tick_soft_except(STDLOG, AstraErr::PAX_SURNAME_NF);
//        }
//    } else if(searchPaxXmlRes.lTrip.size() > 1) {
//        // ������� ��᪮�쪮 ���ᠦ�஢
//        throw tick_soft_except(STDLOG, AstraErr::TOO_MANY_PAX_WITH_SAME_SURNAME);
//    }

//    XmlTrip& paxTrip = searchPaxXmlRes.lTrip.front();
//    fillPaxTrip(paxTrip, ckiParams);
//    // SavePax
//    LoadPaxXmlResult loadPaxXmlRes = AstraEngine::singletone().SavePax(depPointId,
//                                                                       paxTrip);
//    if(loadPaxXmlRes.lSeg.empty()) {
//        // �� ᬮ�� ��ॣ����஢���
//        throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR, "Unable to checkin pax");
//    }

//    return loadPaxXmlRes.toIatci(iatci::Result::Checkin,
//                                 iatci::Result::Ok,
//                                 true/*afterSavePax*/);
//}

//iatci::Result checkinIatciPax(xmlNodePtr reqNode, xmlNodePtr ediResNode)
//{
//    LoadPaxXmlResult loadPaxXmlRes = AstraEngine::singletone().SavePax(reqNode, ediResNode);
//    if(loadPaxXmlRes.lSeg.empty()) {
//        // �� ᬮ�� ��ॣ����஢���
//        throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR, "Unable to checkin pax");
//    }
//    return loadPaxXmlRes.toIatci(iatci::Result::Checkin,
//                                 iatci::Result::Ok,
//                                 true/*afterSavePax*/);
//}

//iatci::Result cancelCheckinIatciPax(const iatci::CkxParams& ckxParams)
//{
//    // �⬥�� ॣ����樨
//    // ࠡ�⠥� ���� �� �����/䠬����/३��
//    // ��, ᪮॥ �ᥣ�, ������ ࠡ���� �� id, ����祭���� ��᫥ ॣ����樨

//    LogTrace(TRACE3) << __FUNCTION__ << " by "
//                     << "airp_dep[" << ckxParams.flight().depPort() << "]; "
//                     << "airline["  << ckxParams.flight().airline() << "]; "
//                     << "flight["   << ckxParams.flight().flightNum() << "]; "
//                     << "dep_date[" << ckxParams.flight().depDate() << "]; "
//                     << "surname["  << ckxParams.pax().surname() << "]; "
//                     << "name["     << ckxParams.pax().name() << "] ";

//    int depPointId = astra_api::findDepPointId(ckxParams.flight().depPort(),
//                                               ckxParams.flight().airline(),
//                                               ckxParams.flight().flightNum().get(),
//                                               ckxParams.flight().depDate());

//    PaxListXmlResult paxListXmlRes;
//    paxListXmlRes = AstraEngine::singletone().PaxList(depPointId);
//    std::list<XmlPax> wantedPaxes = paxListXmlRes.applyNameFilter(ckxParams.pax().surname(),
//                                                                  ckxParams.pax().name());
//    if(wantedPaxes.empty()) {
//        // �� ��諨 ���ᠦ�� � ᯨ᪥ ��ॣ����஢�����
//        throw tick_soft_except(STDLOG, AstraErr::PAX_SURNAME_NOT_CHECKED_IN);
//    }

//    if(wantedPaxes.size() > 1) {
//        // ���諨 ��᪮�쪮 ��ॣ����஢����� ���ᠦ�஢ � ����� ������/䠬�����
//        throw tick_soft_except(STDLOG, AstraErr::TOO_MANY_PAX_WITH_SAME_SURNAME);
//    }

//    // � �⮬ ���� � ᯨ᪥ wantedPaxes ஢�� 1 ���ᠦ��
//    const XmlPax& pax = wantedPaxes.front();
//    LogTrace(TRACE3) << "Pax " << pax.surname << " " << pax.name << " "
//                     << "has reg_no " << pax.reg_no;

//    ASSERT(pax.reg_no != ASTRA::NoExists);

//    LoadPaxXmlResult loadPaxXmlRes;
//    loadPaxXmlRes = AstraEngine::singletone().LoadPax(depPointId, pax.reg_no);

//    if(loadPaxXmlRes.lSeg.size() != 1) {
//        LogError(STDLOG) << "Load pax failed! " << loadPaxXmlRes.lSeg.size() << " "
//                         << "segments found but should be 1";
//        throw tick_soft_except(STDLOG, AstraErr::EDI_PROC_ERR);
//    }

//    XmlSegment& paxSegForCancel = loadPaxXmlRes.lSeg.front();
//    ASSERT(paxSegForCancel.passengers.size() == 1);

//    XmlPax& paxForCancel = paxSegForCancel.passengers.front();
//    paxForCancel.refuse = "�"; // ���� ��稭� �⬥�� �ᥣ�� "� - �訡�� �����"

//    // SavePax
//    loadPaxXmlRes = AstraEngine::singletone().SavePax(depPointId,
//                                                      paxSegForCancel);

//    if(!loadPaxXmlRes.lSeg.empty()) {
//        tst();
//        return loadPaxXmlRes.toIatci(iatci::Result::Cancel,
//                                     iatci::Result::OkWithNoData,
//                                     true/*afterSavePax*/);
//    } else {
//        tst();
//        // � १���� ���� �������� ᥣ���� - ����� ��� ����� ����� �� ckxParams
//        return iatci::Result::makeCancelResult(iatci::Result::OkWithNoData,
//                                               ckxParams.flight());
//    }
//}

//iatci::Result cancelCheckinIatciPax(xmlNodePtr reqNode, xmlNodePtr ediResNode)
//{
//    tst();
//    LoadPaxXmlResult loadPaxXmlRes = AstraEngine::singletone().SavePax(reqNode, ediResNode);
//    if(!loadPaxXmlRes.lSeg.empty()) {
//        tst();
//        return loadPaxXmlRes.toIatci(iatci::Result::Cancel,
//                                     iatci::Result::OkWithNoData,
//                                     true/*afterSavePax*/);
//    } else {
//        tst();
//        // � १���� ���� �������� ᥣ���� - ����� ��� ����� ����� ⮫쪮 �� reqNode
//        loadPaxXmlRes = parseLoadPaxAnswer(reqNode);
//        return loadPaxXmlRes.toIatci(iatci::Result::Cancel,
//                                     iatci::Result::OkWithNoData,
//                                     false/*afterSavePax*/);
//    }
//}

void searchTrip(const iatci::FlightDetails& flight, const iatci::PaxDetails& pax)
{
    //AstraEngine::singletone().CallGetAdvTripList(flight.depDate());
    /*AstraEngine::singletone().CallSearchTripAndLoadPax(flight.toShortKeyString(),
                                                       flight.depDate(),
                                                       pax.surname(),
                                                       pax.name());*/
}

//---------------------------------------------------------------------------------------

//iatci::Result LoadPaxXmlResult::toIatci(iatci::Result::Action_e action,
//                                        iatci::Result::Status_e status,
//                                        bool afterSavePax) const
//{
//    // flight details
//    ASSERT(lSeg.size() == 1);
//    const XmlSegment& seg = lSeg.front();

//    std::string airl     = seg.tripHeader.airline;
//    std::string flNum    = seg.tripHeader.flt_no;
//    std::string scd_local= seg.tripHeader.scd_out_local;
//    std::string airp_dep = seg.airp_dep;
//    std::string airp_arv = seg.airp_arv;

//    if(airl.empty()) {
//        airl = seg.mark_flight.airline;
//    }
//    if(flNum.empty()) {
//        flNum = seg.mark_flight.flt_no;
//    }
//    if(scd_local.empty()) {
//        scd_local = seg.mark_flight.scd;
//    }

//    ASSERT(!scd_local.empty());
//    boost::posix_time::ptime scd_dep_date_time = boostDateTimeFromAstraFormatStr(scd_local);

//    boost::gregorian::date scd_dep_date = scd_dep_date_time.date();
//    boost::gregorian::date scd_arr_date = boost::gregorian::date();
//    boost::posix_time::time_duration scd_dep_time(boost::posix_time::not_a_date_time);
//    if(afterSavePax) {
//        scd_dep_time = scd_dep_date_time.time_of_day();
//    }
//    boost::posix_time::time_duration scd_arr_time(boost::posix_time::not_a_date_time);

//    iatci::FlightDetails flightDetails(airl,
//                                       Ticketing::getFlightNum(flNum),
//                                       airp_dep,
//                                       airp_arv,
//                                       scd_dep_date,
//                                       scd_arr_date,
//                                       scd_dep_time,
//                                       scd_arr_time);

//    // pax details
//    ASSERT(seg.passengers.size() == 1);
//    const XmlPax& pax = seg.passengers.front();

//    boost::optional<iatci::PaxDetails::DocInfo> paxDoc;
//    if(pax.doc)
//    {
//        boost::gregorian::date birthDate = boostDateTimeFromAstraFormatStr(pax.doc->birth_date).date(),
//                              expiryDate = boostDateTimeFromAstraFormatStr(pax.doc->expiry_date).date();
//        paxDoc = iatci::PaxDetails::DocInfo(pax.doc->type,
//                                            "",
//                                            pax.doc->no,
//                                            pax.doc->surname,
//                                            pax.doc->first_name,
//                                            "",
//                                            "",
//                                            birthDate,
//                                            expiryDate);
//    }

//    std::string surname  = pax.surname;
//    std::string name     = pax.name;
//    std::string pers_type= pax.pers_type;

//    iatci::PaxDetails paxDetails(surname,
//                                 name,
//                                 iatci::PaxDetails::Adult,// TODO
//                                 paxDoc);

//    boost::optional<iatci::FlightSeatDetails> seatDetails;
//    boost::optional<iatci::ServiceDetails> serviceDetails;
//    if(status != iatci::Result::OkWithNoData)
//    {
//        // seat details
//        ASSERT(!pax.seat_no.empty());
//        seatDetails = iatci::FlightSeatDetails(pax.seat_no,
//                                               pax.subclass,
//                                               "", // securityCode
//                                               iatci::SeatDetails::None); // non-smoking ind



//        serviceDetails = iatci::ServiceDetails();
//        if(pax.ticket_rem == "TKNE") {
//            serviceDetails->addSsrTkne(pax.ticket_no,
//                                       pax.coupon_no,
//                                       false);
//        } else {
//            serviceDetails->addSsr(pax.ticket_rem,
//                                   pax.ticket_no);
//        }
//    }

//    // TODO

//    return iatci::Result::makeResult(action,
//                                     status,
//                                     flightDetails,
//                                     paxDetails,
//                                     seatDetails,
//                                     boost::none,
//                                     boost::none,
//                                     boost::none,
//                                     boost::none,
//                                     boost::none,
//                                     serviceDetails);
//}

/////////////////////////////////////////////////////////////////////////////////////////

namespace xml_entities
{

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

std::list<XmlPax> PaxListXmlResult::applyNameFilter(const std::string& surname,
                                                    const std::string& name)
{
    std::list<XmlPax> res;
    BOOST_FOREACH(const XmlPax& pax, lPax) {
        // TODO ����室��� �᫮����� �㭪�� �ࠢ����� (trim, translit)
        if(surname == pax.surname && name == pax.name) {
            res.push_back(pax);
        }
    }

    return res;
}

//---------------------------------------------------------------------------------------

std::list<XmlTrip> GetAdvTripListXmlResult::applyFlightFilter(const std::string& flightName)
{
    std::list<XmlTrip> res;
    BOOST_FOREACH(const XmlTrip& trip, lTrip) {
        if(flightName == trip.name) {
            res.push_back(trip);
        }
    }

    return res;
}

}//namespace xml_entities

}//namespace astra_api
