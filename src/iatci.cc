#include "iatci.h"
#include "iatci_api.h"
#include "edi_utils.h"
#include "basic.h"
#include "checkin.h"
#include "passenger.h" // TPaxItem
#include "astra_context.h" // AstraContext
#include "tlg/IatciCkiRequest.h"
#include "tlg/IatciCkuRequest.h"
#include "tlg/IatciCkxRequest.h"
#include "tlg/IatciPlfRequest.h"
#include "tlg/IatciBprRequest.h"
#include "tlg/IatciSmfRequest.h"
#include "tlg/postpone_edifact.h"
#include "tlg/remote_results.h"
#include "tlg/remote_system_context.h"
#include "tlg/edi_msg.h"

#include <serverlib/dates.h>
#include <serverlib/cursctl.h>
#include <serverlib/dump_table.h>

#include <boost/foreach.hpp>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>


static iatci::CkiParams getDebugCkiParams()
{
    iatci::OriginatorDetails origin("UT", "SVO");

    iatci::FlightDetails prevFlight("UT",
                                    Ticketing::FlightNum_t(100),
                                    "SVO",
                                    "LED",
                                    Dates::rrmmdd("150220"),
                                    Dates::rrmmdd("150220"),
                                    Dates::hh24mi("0530"),
                                    Dates::hh24mi("1140"));

    iatci::FlightDetails flight("SU",
                                Ticketing::FlightNum_t(200),
                                "LED",
                                "AER",
                                Dates::rrmmdd("150221"),
                                Dates::rrmmdd("150221"));

    iatci::PaxDetails pax("PETROV",
                          "ALEX",
                          iatci::PaxDetails::Male,
                          boost::none,
                          "UT100");
    iatci::ReservationDetails reserv("Y");
    iatci::SeatDetails seat(iatci::SeatDetails::NonSmoking);
    iatci::BaggageDetails baggage(1, 20);
    boost::optional<iatci::CascadeHostDetails> cascadeDetails;
    iatci::ServiceDetails serviceDetails;
    serviceDetails.addSsrTkne("2981212121212", 1, false);
    serviceDetails.addSsr(iatci::ServiceDetails::SsrInfo("FQTV", "121313454", false, "I", "UT", 1));

    iatci::CkiParams ckiParams(origin,
                               pax,
                               flight,
                               prevFlight,
                               seat,
                               baggage,
                               reserv,
                               cascadeDetails,
                               serviceDetails);

    return ckiParams;
}

static iatci::CkxParams getDebugCkxParams()
{
    iatci::OriginatorDetails origin("UT", "SVO");

    iatci::FlightDetails flight("SU",
                                Ticketing::FlightNum_t(200),
                                "LED",
                                "AER",
                                Dates::rrmmdd("150221"),
                                Dates::rrmmdd("150221"));

    iatci::PaxDetails pax("IVANOV",
                          "SERGEI",
                          iatci::PaxDetails::Male,
                          boost::none,
                          "UT100");

    iatci::CkxParams ckxParams(origin,
                               pax,
                               flight);

    return ckxParams;
}

static iatci::CkuParams getDebugCkuParams()
{
    iatci::OriginatorDetails origin("UT", "SVO");

    iatci::FlightDetails flight("SU",
                                Ticketing::FlightNum_t(200),
                                "LED",
                                "AER",
                                Dates::rrmmdd("150221"),
                                Dates::rrmmdd("150221"));

    iatci::PaxDetails pax("IVANOV", "SERG", iatci::PaxDetails::Adult);

    iatci::UpdatePaxDetails updPax(iatci::UpdateDetails::Replace,
                                   "IVANOV",
                                   "SERGEI");

    iatci::UpdateSeatDetails updSeat(iatci::UpdateDetails::Replace,
                                     "15B");

    iatci::UpdateBaggageDetails baggage(iatci::UpdateDetails::Replace,
                                        1, 20);

    return iatci::CkuParams(origin,
                            pax,
                            flight,
                            boost::none,
                            updPax,
                            updSeat,
                            baggage);
}

static iatci::PlfParams getDebugPlfParams()
{
    iatci::OriginatorDetails origin("UT", "SVO");

    iatci::FlightDetails flight("SU",
                                Ticketing::FlightNum_t(200),
                                "LED",
                                "AER",
                                Dates::rrmmdd("150221"),
                                Dates::rrmmdd("150221"));

    iatci::PaxSeatDetails pax("IVANOV",
                              "SERGEI",
                              "Y",
                              "05A",
                              "21",
                              "RECLOC",
                              "2982145646345");

    iatci::PlfParams plfParams(origin,
                               pax,
                               flight);

    return plfParams;
}

static iatci::BprParams getDebugBprParams()
{
    iatci::OriginatorDetails origin("UT", "SVO");

    iatci::FlightDetails prevFlight("UT",
                                    Ticketing::FlightNum_t(100),
                                    "SVO",
                                    "LED",
                                    Dates::rrmmdd("150220"),
                                    Dates::rrmmdd("150220"),
                                    Dates::hh24mi("0530"),
                                    Dates::hh24mi("1140"));

    iatci::FlightDetails flight("SU",
                                Ticketing::FlightNum_t(200),
                                "LED",
                                "AER",
                                Dates::rrmmdd("150221"),
                                Dates::rrmmdd("150221"));

    iatci::PaxDetails pax("PETROV",
                          "ALEX",
                          iatci::PaxDetails::Male,
                          boost::none,
                          "UT100");
    iatci::ReservationDetails reserv("Y");
    iatci::SeatDetails seat(iatci::SeatDetails::NonSmoking);
    iatci::BaggageDetails baggage(1, 20);
    boost::optional<iatci::CascadeHostDetails> cascadeDetails;

    iatci::BprParams ckiParams(origin,
                               pax,
                               flight,
                               prevFlight,
                               seat,
                               baggage,
                               reserv,
                               cascadeDetails);

    return ckiParams;
}

static iatci::SmfParams getDebugSmfParams()
{
    iatci::OriginatorDetails origin("UT", "SVO");

    iatci::FlightDetails flight("SU",
                                Ticketing::FlightNum_t(200),
                                "LED",
                                "AER",
                                Dates::rrmmdd("150221"),
                                Dates::rrmmdd("150221"));

    iatci::SeatRequestDetails seatReq("F", iatci::SeatRequestDetails::NonSmoking);

    return iatci::SmfParams(origin, flight, seatReq);
}

/////////////////////////////////////////////////////////////////////////////////////////

namespace
{
    struct TPaxSegInfo
    {
        TSegInfo seg;
        CheckIn::TPaxItem pax;
    };

    bool isSeg4EdiTCkin(const TSegInfo& seg)
    {
        return (seg.point_dep == -1 && seg.point_arv == -1);
    }

    static TPaxSegInfo paxSegFromNode(xmlNodePtr segNode)
    {
        TPaxSegInfo paxSegInfo;
        paxSegInfo.seg.point_dep = NodeAsInteger("point_dep", segNode);
        paxSegInfo.seg.airp_dep  = NodeAsString("airp_dep", segNode);
        paxSegInfo.seg.point_arv = NodeAsInteger("point_arv", segNode);
        paxSegInfo.seg.airp_arv  = NodeAsString("airp_arv", segNode);

        xmlNodePtr fltNode = findNode(segNode, "mark_flight");
        if(!fltNode)
            fltNode = findNode(segNode, "pseudo_mark_flight");

        if(fltNode)
        {
            paxSegInfo.seg.fltInfo.airline = NodeAsString("airline", fltNode);
            paxSegInfo.seg.fltInfo.flt_no  = NodeAsInteger("flt_no", fltNode);
            paxSegInfo.seg.fltInfo.suffix  = NodeAsString("suffix", fltNode);
            paxSegInfo.seg.fltInfo.scd_out = NodeAsDateTime("scd", fltNode);
            paxSegInfo.seg.fltInfo.airp    = NodeAsString("airp_dep",fltNode);
        }

        xmlNodePtr paxNode = NodeAsNode("passengers/pax", segNode);
        // пока можем обрабатывать только одного пассажира для Edifact
        paxSegInfo.pax.surname   = NodeAsString("surname", paxNode);
        paxSegInfo.pax.name      = NodeAsString("name", paxNode);
        paxSegInfo.pax.pers_type = DecodePerson(NodeAsString("pers_type", paxNode));
        paxSegInfo.pax.seat_no   = NodeAsString("seat_no", paxNode, "");

        return paxSegInfo;
    }

    std::vector<TPaxSegInfo> getPaxSegs(xmlNodePtr reqNode, bool iatciFlag)
    {
        std::vector<TPaxSegInfo> vPaxSeg;
        xmlNodePtr segNode = NodeAsNode("segments/segment", reqNode);
        for(;segNode != NULL; segNode = segNode->next) {
            vPaxSeg.push_back(paxSegFromNode(segNode));
        }

        if(iatciFlag)
        {
            segNode = NodeAsNode("iatci_segments/segment", reqNode);
            if(segNode) {
                vPaxSeg.push_back(paxSegFromNode(segNode));
            }
        }

        return vPaxSeg;
    }

    iatci::PaxDetails::PaxType_e astra2iatci(ASTRA::TPerson personType)
    {
        switch(personType)
        {
        case ASTRA::adult:
            return iatci::PaxDetails::Adult;
        case ASTRA::child:
            return iatci::PaxDetails::Child;
        default:
            throw EXCEPTIONS::Exception("Unknown astra person type: %d", personType);
        }
    }

    struct PointInfo
    {
        std::string m_airline;
        std::string m_airport;

        PointInfo(const std::string& airline,
                  const std::string& airport)
            : m_airline(airline),
              m_airport(airport)
        {}
    };

    boost::optional<PointInfo> readPointInfo(int pointId)
    {
        LogTrace(TRACE5) << "Enter to " << __FUNCTION__ << " by pointId " << pointId;

        std::string airl, airp;
        OciCpp::CursCtl cur = make_curs(
"select AIRLINE, AIRP from POINTS where POINT_ID=:point_id");
        cur.bind(":point_id", pointId)
           .defNull(airl, "")
           .def(airp)
           .EXfet();
        if(cur.err() == NO_DATA_FOUND) {
            LogWarning(STDLOG) << "Point " << pointId << " not found!";
            return boost::none;
        }

        return PointInfo(airl, airp);
    }


}//namespace

//---------------------------------------------------------------------------------------

static std::string getIatciRequestContext(const edifact::KickInfo& kickInfo = edifact::KickInfo())
{
    // TODO fill whole context
    XMLDoc xmlCtxt("context");
    ASSERT(xmlCtxt.docPtr() != NULL);
    xmlNodePtr rootNode = NodeAsNode("/context", xmlCtxt.docPtr());
    NewTextChild(rootNode, "point_id");
    kickInfo.toXML(rootNode);
    SetProp(rootNode,"req_ctxt_id", kickInfo.reqCtxtId);
    return XMLTreeToText(xmlCtxt.docPtr());
}

static std::string getIatciPult()
{
    return TReqInfo::Instance()->desk.code;
}

static edifact::KickInfo getIatciKickInfo(xmlNodePtr reqNode, xmlNodePtr ediResNode)
{
    if(reqNode != NULL)
    {
        int termReqCtxtId = AstraContext::SetContext("TERM_REQUEST", XMLTreeToText(reqNode->doc));
        if(ediResNode != NULL) {
            AstraContext::SetContext("EDI_RESPONSE", termReqCtxtId, XMLTreeToText(ediResNode->doc));
        }
        return AstraEdifact::createKickInfo(termReqCtxtId, "IactiInterface");
    }

    return edifact::KickInfo();
}

static boost::optional<iatci::CkiParams> getCkiParams(xmlNodePtr reqNode, bool iatciFlag)
{
    std::vector<TPaxSegInfo> vPaxSeg = getPaxSegs(reqNode, iatciFlag);
    if(vPaxSeg.size() < 2) {
        ProgTrace(TRACE0, "%s: At least 2 segments must be present in the query for iatci, but there is %zu",
                           __FUNCTION__, vPaxSeg.size());
        return boost::none;
    }

    const TPaxSegInfo& lastPaxSeg   = vPaxSeg.at(vPaxSeg.size() - 1); // Последний сегмент в запросе
    const TPaxSegInfo& penultPaxSeg = vPaxSeg.at(vPaxSeg.size() - 2); // Предпоследний сегмент в запросе

    // последний сегмент запроса должен быть предназначен для Edifact iatci
    ASSERT(isSeg4EdiTCkin(lastPaxSeg.seg));
    // а предпоследний - нет
    ASSERT(!isSeg4EdiTCkin(penultPaxSeg.seg));

    iatci::OriginatorDetails origin(penultPaxSeg.seg.fltInfo.airline,
                                    penultPaxSeg.seg.airp_dep);

    iatci::PaxDetails pax(lastPaxSeg.pax.surname,
                          lastPaxSeg.pax.name,
                          astra2iatci(lastPaxSeg.pax.pers_type));

    iatci::FlightDetails flight(lastPaxSeg.seg.fltInfo.airline,
                                Ticketing::FlightNum_t(lastPaxSeg.seg.fltInfo.flt_no),
                                lastPaxSeg.seg.airp_dep,
                                lastPaxSeg.seg.airp_arv,
                                BASIC::DateTimeToBoost(lastPaxSeg.seg.fltInfo.scd_out).date());

    iatci::FlightDetails prevFlight(penultPaxSeg.seg.fltInfo.airline,
                                    Ticketing::FlightNum_t(penultPaxSeg.seg.fltInfo.flt_no),
                                    penultPaxSeg.seg.airp_dep,
                                    penultPaxSeg.seg.airp_arv,
                                    BASIC::DateTimeToBoost(penultPaxSeg.seg.fltInfo.scd_out).date());

    boost::optional<iatci::SeatDetails> seat;
    if(!lastPaxSeg.pax.seat_no.empty()) {
        seat = iatci::SeatDetails(lastPaxSeg.pax.seat_no);
    }

    return iatci::CkiParams(origin, pax, flight, prevFlight, seat);
}

static boost::optional<iatci::CkxParams> getCkxParams(xmlNodePtr reqNode, bool iatciFlag)
{
    std::vector<TPaxSegInfo> vPaxSeg = getPaxSegs(reqNode, iatciFlag);
    if(vPaxSeg.size() < 2) {
        ProgTrace(TRACE0, "%s: At least 2 segments must be present in the query for iatci, but there is %zu",
                           __FUNCTION__, vPaxSeg.size());
        return boost::none;
    }

    const TPaxSegInfo& lastPaxSeg   = vPaxSeg.at(vPaxSeg.size() - 1); // Последний сегмент в запросе
    const TPaxSegInfo& penultPaxSeg = vPaxSeg.at(vPaxSeg.size() - 2); // Предпоследний сегмент в запросе

    // последний сегмент запроса должен быть предназначен для Edifact iatci
    ASSERT(isSeg4EdiTCkin(lastPaxSeg.seg));
    // а предпоследний - нет
    ASSERT(!isSeg4EdiTCkin(penultPaxSeg.seg));

    std::string origAirline = penultPaxSeg.seg.fltInfo.airline;
    std::string origAirport = penultPaxSeg.seg.airp_dep;
    if(origAirline.empty() || origAirport.empty())
    {
        tst();
        if(penultPaxSeg.seg.point_dep != -1)
        {
            tst();
            boost::optional<PointInfo> pointInfo = readPointInfo(penultPaxSeg.seg.point_dep);
            if(pointInfo)
            {
                origAirline = pointInfo->m_airline;
                origAirport = pointInfo->m_airport;
            }
        }
    }

    iatci::OriginatorDetails origin(origAirline, origAirport);

    iatci::PaxDetails pax(lastPaxSeg.pax.surname,
                          lastPaxSeg.pax.name,
                          astra2iatci(lastPaxSeg.pax.pers_type));

    iatci::FlightDetails flight(lastPaxSeg.seg.fltInfo.airline,
                                Ticketing::FlightNum_t(lastPaxSeg.seg.fltInfo.flt_no),
                                lastPaxSeg.seg.airp_dep,
                                lastPaxSeg.seg.airp_arv,
                                BASIC::DateTimeToBoost(lastPaxSeg.seg.fltInfo.scd_out).date());

    return iatci::CkxParams(origin, pax, flight);
}

//---------------------------------------------------------------------------------------

void IatciInterface::DispatchRequest(xmlNodePtr reqNode, xmlNodePtr ediResNode)
{
    std::vector<TPaxSegInfo> vPaxSeg;
    xmlNodePtr segNode = NodeAsNode("segments/segment", reqNode);
    for(;segNode != NULL; segNode = segNode->next) {
        xmlNodePtr grpIdNode = findNode(segNode, "grp_id");
        if(grpIdNode == NULL) {
            LogTrace(TRACE3) << "Checkin request detected!";
            return InitialRequest(reqNode, ediResNode);
        } else {
            xmlNodePtr paxNode = NodeAsNode("passengers/pax", segNode);
            ASSERT(paxNode != NULL);
            xmlNodePtr refuseNode = findNode(paxNode, "refuse");
            if(refuseNode != NULL) {
                LogTrace(TRACE3) << "Cancel request detected!";
                return CancelRequest(reqNode, ediResNode);
            } else {
                LogTrace(TRACE3) << "Update request detected!";
                return UpdateRequest(reqNode, ediResNode);
            }
        }
    }

    TST();
    ASSERT(false); // throw always
}

void IatciInterface::InitialRequest(XMLRequestCtxt* ctxt,
                                    xmlNodePtr reqNode,
                                    xmlNodePtr resNode)
{
    // send edifact DCQCKI request
    edifact::SendCkiRequest(getDebugCkiParams(),
                            getIatciPult(),
                            getIatciRequestContext(),
                            AstraEdifact::createKickInfo(ASTRA::NoExists, "IactiInterface"));
}

bool IatciInterface::NeedSendIatciRequest(xmlNodePtr reqNode)
{
    std::vector<TPaxSegInfo> vPaxSeg = getPaxSegs(reqNode, false);
    if(vPaxSeg.size() < 2) {
        ProgTrace(TRACE1, "%s: At least 2 segments must be present in the query for iatci, but there is %zu",
                           __FUNCTION__, vPaxSeg.size());
        return boost::none;
    }

    const TPaxSegInfo& lastPaxSeg   = vPaxSeg.at(vPaxSeg.size() - 1); // Последний сегмент в запросе
    const TPaxSegInfo& penultPaxSeg = vPaxSeg.at(vPaxSeg.size() - 2); // Предпоследний сегмент в запросе

    // последний сегмент запроса должен быть предназначен для Edifact iatci, а предпоследний - нет
    return (isSeg4EdiTCkin(lastPaxSeg.seg) && !isSeg4EdiTCkin(penultPaxSeg.seg));
}

void IatciInterface::InitialRequest(xmlNodePtr reqNode, xmlNodePtr ediResNode)
{
    boost::optional<iatci::CkiParams> ckiParams = getCkiParams(reqNode, true);
    ASSERT(ckiParams);

    edifact::KickInfo kickInfo = getIatciKickInfo(reqNode, ediResNode);
    edilib::EdiSessionId_t sessIda = edifact::SendCkiRequest(ckiParams.get(),
                                                             getIatciPult(),
                                                             getIatciRequestContext(kickInfo),
                                                             kickInfo);

    if(TReqInfo::Instance()->api_mode)
    {
        LogTrace(TRACE3) << "throw TlgToBePostponed for edi_session=" << sessIda;
        throw TlgHandling::TlgToBePostponed(sessIda);
    }
}

void IatciInterface::UpdateRequest(XMLRequestCtxt* ctxt,
                                   xmlNodePtr reqNode,
                                   xmlNodePtr resNode)
{
    // send edifact DCQCKU request
    edifact::SendCkuRequest(getDebugCkuParams(),
                            getIatciPult(),
                            getIatciRequestContext(),
                            AstraEdifact::createKickInfo(ASTRA::NoExists, "IactiInterface"));
}

void IatciInterface::UpdateRequest(xmlNodePtr reqNode, xmlNodePtr ediResNode)
{
    // TODO
}

void IatciInterface::CancelRequest(XMLRequestCtxt* ctxt,
                                   xmlNodePtr reqNode,
                                   xmlNodePtr resNode)
{
    // send edifact DCQCKX request
    edifact::SendCkxRequest(getDebugCkxParams(),
                            getIatciPult(),
                            getIatciRequestContext(),
                            AstraEdifact::createKickInfo(ASTRA::NoExists, "IactiInterface"));
}

void IatciInterface::CancelRequest(xmlNodePtr reqNode, xmlNodePtr ediResNode)
{
    tst();
    boost::optional<iatci::CkxParams> ckxParams = getCkxParams(reqNode, true);
    ASSERT(ckxParams);

    edifact::KickInfo kickInfo = getIatciKickInfo(reqNode, ediResNode);
    edilib::EdiSessionId_t sessIda = edifact::SendCkxRequest(ckxParams.get(),
                                                             getIatciPult(),
                                                             getIatciRequestContext(kickInfo),
                                                             kickInfo);

    if(TReqInfo::Instance()->api_mode)
    {
        LogTrace(TRACE3) << "throw TlgToBePostponed for edi_session=" << sessIda;
        throw TlgHandling::TlgToBePostponed(sessIda);
    }
}

void IatciInterface::ReprintRequest(XMLRequestCtxt* ctxt,
                                    xmlNodePtr reqNode,
                                    xmlNodePtr resNode)
{
    // send edifact DCQBPR request
    edifact::SendBprRequest(getDebugBprParams(),
                            getIatciPult(),
                            getIatciRequestContext(),
                            AstraEdifact::createKickInfo(ASTRA::NoExists, "IactiInterface"));
}

void IatciInterface::PasslistRequest(XMLRequestCtxt* ctxt,
                                     xmlNodePtr reqNode,
                                     xmlNodePtr resNode)
{
    // send edifact DCQPLF request
    edifact::SendPlfRequest(getDebugPlfParams(),
                            getIatciPult(),
                            getIatciRequestContext(),
                            AstraEdifact::createKickInfo(ASTRA::NoExists, "IactiInterface"));
}

void IatciInterface::SeatmapRequest(XMLRequestCtxt* ctxt,
                                    xmlNodePtr reqNode,
                                    xmlNodePtr resNode)
{
    // send edifact DCQSMF request
    edifact::SendSmfRequest(getDebugSmfParams(),
                            getIatciPult(),
                            getIatciRequestContext(),
                            AstraEdifact::createKickInfo(ASTRA::NoExists, "IactiInterface"));
}

void IatciInterface::CheckinKickHandler(xmlNodePtr reqNode, xmlNodePtr resNode,
                                        const std::list<iatci::Result>& lRes)
{
    FuncIn(CheckinKickHandler);
    BOOST_FOREACH(const iatci::Result& res, lRes) {
        LogTrace(TRACE3) << "error: " << (res.errorDetails() ? res.errorDetails()->errCode() : "None");
    }

    DoKickAction(reqNode, resNode, lRes, "iatci_cki_result", ActSavePax);

    FuncOut(CheckinKickHandler);
}

void IatciInterface::UpdateKickHandler(xmlNodePtr reqNode, xmlNodePtr resNode,
                                       const std::list<iatci::Result>& lRes)
{
    FuncIn(UpdateKickHandler);
    FuncOut(UpdateKickHandler);
}

void IatciInterface::CancelKickHandler(xmlNodePtr reqNode, xmlNodePtr resNode,
                                       const std::list<iatci::Result>& lRes)
{
    FuncIn(CancelKickHandler);    
    DoKickAction(reqNode, resNode, lRes, "iatci_ckx_result", ActSavePax);
    FuncOut(CancelKickHandler);
}

void IatciInterface::ReprintKickHandler(xmlNodePtr reqNode, xmlNodePtr resNode,
                                        const std::list<iatci::Result>& lRes)
{
    FuncIn(ReprintKickHandler);
    FuncOut(ReprintKickHandler);
}

void IatciInterface::PasslistKickHandler(xmlNodePtr reqNode, xmlNodePtr resNode,
                                         const std::list<iatci::Result>& lRes)
{
    FuncIn(PasslistKickHandler);
    FuncOut(PasslistKickHandler);
}

void IatciInterface::SeatmapKickHandler(xmlNodePtr reqNode, xmlNodePtr resNode,
                                        const std::list<iatci::Result>& lRes)
{
    FuncIn(SeatmapKickHandler);
    FuncOut(SeatmapKickHandler);
}

void IatciInterface::SeatmapForPassengerKickHandler(xmlNodePtr reqNode, xmlNodePtr resNode,
                                                    const std::list<iatci::Result>& lRes)
{
    FuncIn(SeatmapForPassengerKickHandler);
    FuncOut(SeatmapForPassengerKickHandler);
}

void IatciInterface::TimeoutKickHandler(xmlNodePtr reqNode, xmlNodePtr resNode)
{
    FuncIn(TimeoutKickHandler);
    // TODO locale_messages
    AstraLocale::showProgError("MSG.ETS_CONNECT_ERROR");
    FuncOut(TimeoutKickHandler);
}

void IatciInterface::KickHandler(XMLRequestCtxt* ctxt,
                                 xmlNodePtr reqNode,
                                 xmlNodePtr resNode)
{
    using namespace edifact;
    FuncIn(KickHandler);

    pRemoteResults res = RemoteResults::readSingle();
    if(res) {
        LogTrace(TRACE3) << *res;

        if(res->status() == RemoteStatus::Timeout)
        {
            TimeoutKickHandler(reqNode, resNode);
        }
        else if(res->status() == RemoteStatus::Success)
        {
            std::list<iatci::Result> lRes = iatci::loadCkiData(res->ediSession());
            ASSERT(!lRes.empty());
            iatci::Result::Action_e action = lRes.front().action();
            switch(action)
            {
            case iatci::Result::Checkin:
                CheckinKickHandler(reqNode, resNode, lRes);
                break;
            case iatci::Result::Cancel:
                CancelKickHandler(reqNode, resNode, lRes);
                break;
            case iatci::Result::Update:
                UpdateKickHandler(reqNode, resNode, lRes);
                break;
            case iatci::Result::Reprint:
                ReprintKickHandler(reqNode, resNode, lRes);
                break;
            case iatci::Result::Passlist:
                PasslistKickHandler(reqNode, resNode, lRes);
                break;
            case iatci::Result::Seatmap:
                SeatmapKickHandler(reqNode, resNode, lRes);
                break;
            case iatci::Result::SeatmapForPassenger:
                SeatmapForPassengerKickHandler(reqNode, resNode, lRes);
                break;
            }
        }
        else
        {
            if(!res->remark().empty()) {
                AstraLocale::showProgError(res->remark());
            } else {
                if(!res->ediErrCode().empty()) {
                    AstraLocale::showProgError(getInnerErrByErd(res->ediErrCode()));
                } else {
                    AstraLocale::showProgError("Ошибка обработки в удалённой DCS");
                }
            }
            DoKickAction(reqNode, resNode, std::list<iatci::Result>(), "", ActRollbackStatus);
        }
    }
    FuncOut(KickHandler);
}

void IatciInterface::DoKickAction(xmlNodePtr reqNode, xmlNodePtr resNode,
                                  const std::list<iatci::Result>& lRes,
                                  const std::string& resNodeName,
                                  KickAction_e act)
{
    if(GetNode("@req_ctxt_id", reqNode) != NULL)
    {
        int reqCtxtId = NodeAsInteger("@req_ctxt_id", reqNode);
        LogTrace(TRACE3) << "reqCtxtId = " << reqCtxtId;


        if(!resNodeName.empty())
        {
            XMLDoc resCtxt;
            resCtxt.set(resNodeName.c_str());
            xmlNodePtr iatciResNode = NodeAsNode(std::string("/" + resNodeName).c_str(), resCtxt.docPtr());
            for(const iatci::Result& res: lRes) {
                res.toXml(iatciResNode);
            }

            AstraEdifact::addToEdiResponseCtxt(reqCtxtId, iatciResNode, "context");
        }


        XMLDoc termReqCtxt;
        AstraEdifact::getTermRequestCtxt(reqCtxtId, true, "IatciInterface::KickHandler", termReqCtxt);
        xmlNodePtr termReqNode = NodeAsNode("/term/query", termReqCtxt.docPtr())->children;
        ASSERT(termReqNode != NULL);

        XMLDoc ediResCtxt;
        AstraEdifact::getEdiResponseCtxt(reqCtxtId, true, "IatciInterface::KickHandler", ediResCtxt);
        xmlNodePtr ediResNode = NodeAsNode("/context", ediResCtxt.docPtr());
        ASSERT(ediResNode != NULL);

        switch(act)
        {
        case ActSavePax:
            CheckInInterface::SavePax(termReqNode, ediResNode, resNode);
            break;
        case ActRollbackStatus:
            ETStatusInterface::ETRollbackStatus(ediResCtxt.docPtr(), false);
            break;
        default:
            LogError(STDLOG) << "Nothing to do!";
        }
    }
    else
    {
        TST();
        LogError(STDLOG) << "Node 'req_ctxt_id'' not found in request!";
    }
}
