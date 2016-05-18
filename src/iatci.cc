#include "iatci.h"
#include "iatci_api.h"
#include "iatci_help.h"
#include "edi_utils.h"
#include "astra_api.h"
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
#include <serverlib/savepoint.h>
#include <serverlib/dump_table.h>

#include <boost/foreach.hpp>

#define NICKNAME "ANTON"
#define NICKTRACE ANTON_TRACE
#include <serverlib/slogger.h>

using namespace astra_api;
using namespace astra_api::xml_entities;

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
                                   "SERGEI",
                                   boost::none);

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

    static TSegInfo segFromNode(xmlNodePtr segNode)
    {
        TSegInfo segInfo;
        segInfo.point_dep = NodeAsInteger("point_dep", segNode);
        segInfo.airp_dep  = NodeAsString("airp_dep", segNode);
        segInfo.point_arv = NodeAsInteger("point_arv", segNode);
        segInfo.airp_arv  = NodeAsString("airp_arv", segNode);

        xmlNodePtr fltNode = findNode(segNode, "mark_flight");
        if(!fltNode)
            fltNode = findNode(segNode, "pseudo_mark_flight");

        if(fltNode)
        {
            segInfo.fltInfo.airline = NodeAsString("airline", fltNode);
            segInfo.fltInfo.flt_no  = NodeAsInteger("flt_no", fltNode);
            segInfo.fltInfo.suffix  = NodeAsString("suffix", fltNode);
            segInfo.fltInfo.scd_out = NodeAsDateTime("scd", fltNode);
            segInfo.fltInfo.airp    = NodeAsString("airp_dep",fltNode);
        }

        return segInfo;
    }

    static TPaxSegInfo paxSegFromNode(xmlNodePtr segNode)
    {
        TPaxSegInfo paxSegInfo;
        paxSegInfo.seg = segFromNode(segNode);

        xmlNodePtr paxNode = NodeAsNode("passengers/pax", segNode);
        // пока можем обрабатывать только одного пассажира для Edifact
        paxSegInfo.pax.surname   = NodeAsString("surname", paxNode);
        paxSegInfo.pax.name      = NodeAsString("name", paxNode);
        paxSegInfo.pax.pers_type = DecodePerson(NodeAsString("pers_type", paxNode));
        paxSegInfo.pax.seat_no   = NodeAsString("seat_no", paxNode, "");

        return paxSegInfo;
    }

    std::vector<TSegInfo> getSegs(xmlNodePtr reqNode)
    {
        std::vector<TSegInfo> vSeg;
        xmlNodePtr segNode = NodeAsNode("segments/segment", reqNode);
        for(;segNode != NULL; segNode = segNode->next) {
            vSeg.push_back(segFromNode(segNode));
        }

        return vSeg;
    }

    std::vector<TPaxSegInfo> getPaxSegs(xmlNodePtr reqNode)
    {
        std::vector<TPaxSegInfo> vPaxSeg;
        xmlNodePtr segNode = NodeAsNode("segments/segment", reqNode);
        for(;segNode != NULL; segNode = segNode->next) {
            vPaxSeg.push_back(paxSegFromNode(segNode));
        }

        segNode = NodeAsNode("iatci_segments/segment", reqNode);
        if(segNode) {
            vPaxSeg.push_back(paxSegFromNode(segNode));
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

static boost::optional<iatci::CkiParams> getCkiParams(xmlNodePtr reqNode)
{
    // TODO переделать на CheckInTabs
    std::vector<TPaxSegInfo> vPaxSeg = getPaxSegs(reqNode);
    if(vPaxSeg.size() < 2) {
        ProgTrace(TRACE0, "%s: At least 2 segments must be present in the query for iatci, but there is %zu",
                           __FUNCTION__, vPaxSeg.size());
        return boost::none;
    }

    const TPaxSegInfo& lastPaxSeg   = vPaxSeg.at(vPaxSeg.size() - 1); // Последний сегмент в запросе
    const TPaxSegInfo& penultPaxSeg = vPaxSeg.at(vPaxSeg.size() - 2); // Предпоследний сегмент в запросе

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

static boost::optional<iatci::CkxParams> getCkxParams(xmlNodePtr reqNode)
{
    // TODO переделать на CheckInTabs
    std::vector<TPaxSegInfo> vPaxSeg = getPaxSegs(reqNode);
    if(vPaxSeg.size() < 2) {
        ProgTrace(TRACE0, "%s: At least 2 segments must be present in the query for iatci, but there is %zu",
                           __FUNCTION__, vPaxSeg.size());
        return boost::none;
    }

    const TPaxSegInfo& lastPaxSeg   = vPaxSeg.at(vPaxSeg.size() - 1); // Последний сегмент в запросе
    const TPaxSegInfo& penultPaxSeg = vPaxSeg.at(vPaxSeg.size() - 2); // Предпоследний сегмент в запросе

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

/////////////////////////////////////////////////////////////////////////////////////////

namespace {
class IatciPaxSeg
{
    iatci::FlightDetails m_seg;
    iatci::PaxDetails m_pax;

public:
    IatciPaxSeg(const iatci::FlightDetails& seg,
                const iatci::PaxDetails& pax)
        : m_seg(seg), m_pax(pax)
    {}

    static IatciPaxSeg read(int grpId)
    {
        LogTrace(TRACE3) << "read for grpId: " << grpId;
        std::list<iatci::FlightDetails> iatciSeg = iatci::IatciDb::readSeg(grpId);
        ASSERT(!iatciSeg.empty());
        std::list<iatci::PaxDetails>    iatciPax = iatci::IatciDb::readPax(grpId);
        ASSERT(iatciPax.size() == 1);

        return IatciPaxSeg(iatciSeg.front(), iatciPax.front());
    }

    const iatci::FlightDetails& seg() const { return m_seg; }
    const iatci::PaxDetails&    pax() const { return m_pax; }
};
} //namespace

/////////////////////////////////////////////////////////////////////////////////////////

static boost::optional<iatci::CkuParams> getCkuParams(xmlNodePtr reqNode)
{
    boost::optional<astra_entities::PaxInfo> changedPax;

    XmlCheckInTabs iatciTabs(findNodeR(reqNode, "iatci_segments"));
    for(const XmlCheckInTab& tab: iatciTabs.tabs()) {
        std::list<astra_entities::PaxInfo> lPax = tab.lPax();
        if(!lPax.empty())
        {
            tst();
            // пока умеем работать с одним пассажиром
            const astra_entities::PaxInfo& pax = lPax.front();

            // TODO пока обрабатываем только были ли изменения в документе
            if(pax.m_doc) {
                tst();
                changedPax = pax;
            }
        }
    }

    if(changedPax && changedPax->m_doc)
    {
        tst();
        XmlCheckInTab firstTab(NodeAsNode("segments/segment", reqNode));
        IatciPaxSeg iatciPaxSeg = IatciPaxSeg::read(firstTab.seg().m_grpId);

        iatci::UpdatePaxDetails::UpdateDocInfo
                updDoc(iatci::UpdateDetails::Replace,
                       changedPax->m_doc->m_type,
                       changedPax->m_doc->m_country,
                       changedPax->m_doc->m_num,
                       changedPax->m_doc->m_surname,
                       changedPax->m_doc->m_name,
                       changedPax->m_doc->m_gender,
                       changedPax->m_doc->m_citizenship,
                       changedPax->m_doc->m_birthDate,
                       changedPax->m_doc->m_expiryDate);

        iatci::UpdatePaxDetails
                updPax(iatci::UpdateDetails::Replace,
                       changedPax->m_surname,
                       changedPax->m_name,
                       updDoc);

        tst();
        return iatci::CkuParams(iatci::OriginatorDetails("ЮТ", "ДМД"), // TODO
                                iatciPaxSeg.pax(),
                                iatciPaxSeg.seg(),
                                boost::none,
                                updPax);
    }

    tst();
    return boost::none;
}

static iatci::PlfParams getPlfParams(int grpId)
{
    IatciPaxSeg iatciPaxSeg = IatciPaxSeg::read(grpId);
    return iatci::PlfParams(iatci::OriginatorDetails("ЮТ", "ДМД"), // TODO
                            iatciPaxSeg.pax(),
                            iatciPaxSeg.seg());
}

static void SaveIatciXmlResToDb(const std::list<iatci::Result>& lRes,
                                xmlNodePtr iatciResNode, int grpId,
                                iatci::Result::Action_e act)
{
    LogTrace(TRACE3) << "Enter to " << __FUNCTION__;
    std::string xmlData = XMLTreeToText(iatciResNode->doc);
    switch(act)
    {
    case iatci::Result::Checkin:
        tst();
        iatci::IatciXmlDb::add(grpId, xmlData);
        iatci::IatciDb::add(grpId, lRes);
        break;

    case iatci::Result::Update:
    case iatci::Result::Passlist:
        tst();
        iatci::IatciXmlDb::upd(grpId, xmlData);
        break;

    case iatci::Result::Cancel:
        tst();
        iatci::IatciXmlDb::del(grpId);
        break;

    default:
        break;
    }
}

static int SaveIatciPax(const std::list<iatci::Result>& lRes,
                        xmlNodePtr ediResNode, xmlNodePtr termReqNode, xmlNodePtr resNode)
{
    LogTrace(TRACE3) << "Enter to " << __FUNCTION__;
    boost::optional<iatci::Result::Action_e> act;
    xmlNodePtr node = NULL;
    if       ((node = findNodeR(ediResNode, "iatci_cki_result")) != NULL) {
        act = iatci::Result::Checkin;
    } else if((node = findNodeR(ediResNode, "iatci_ckx_result")) != NULL) {
        act = iatci::Result::Cancel;
    } else if((node = findNodeR(ediResNode, "iatci_cku_result")) != NULL) {
        act = iatci::Result::Update;
    } else if((node = findNodeR(ediResNode, "iatci_plf_result")) != NULL) {
        act = iatci::Result::Passlist;
    }

    if(node == NULL)
    {
        LogTrace(TRACE3) << "No one iatci pax";
        return 0;
    }

    ASSERT(act);

    int grpId = 0;

    if(act == iatci::Result::Checkin) {
        // при первичной регистрации grp_id появляется в ответном xml
        grpId = NodeAsInteger("grp_id", NodeAsNode("segments/segment", resNode));
    } else {
        // в остальных случаях grp_id уже содержится в запросе, либо её можно найти
        xmlNodePtr grpIdNode = findNodeR(termReqNode, "grp_id");
        if(grpIdNode != NULL && !NodeIsNULL(grpIdNode)) {
            grpId = NodeAsInteger(grpIdNode);
        } else {
            xmlNodePtr pointIdNode = findNodeR(termReqNode, "point_id");
            ASSERT(pointIdNode != NULL && !NodeIsNULL(pointIdNode));
            int pointId = NodeAsInteger(pointIdNode);
            xmlNodePtr regNoNode = findNodeR(termReqNode, "reg_no");
            if(regNoNode != NULL && !NodeIsNULL(regNoNode)) {
                int regNo = NodeAsInteger(regNoNode);
                grpId = astra_api::findGrpIdByRegNo(pointId, regNo);
            } else {
                xmlNodePtr paxIdNode = findNodeR(termReqNode, "pax_id");
                if(paxIdNode != NULL && !NodeIsNULL(paxIdNode)) {
                    int paxId = NodeAsInteger(paxIdNode);
                    grpId = astra_api::findGrpIdByPaxId(pointId, paxId);
                }
            }
        }
    }

    LogTrace(TRACE3) << "grpId=" << grpId;
    ASSERT(grpId > 0);

    // Пока сохраним информацию для всей группы,
    // но, возможно, в будущем потребуется сохранять для каждого пассажира
    SaveIatciXmlResToDb(lRes, node, grpId, *act);

    return grpId;
}

//---------------------------------------------------------------------------------------

IatciInterface::RequestType IatciInterface::ClassifyCheckInRequest(xmlNodePtr reqNode)
{
    xmlNodePtr segNode = NodeAsNode("segments/segment", reqNode);
    xmlNodePtr grpIdNode = findNode(segNode, "grp_id");
    if(grpIdNode == NULL) {
        LogTrace(TRACE3) << "Checkin request detected!";
        return IatciInterface::Cki;
    } else {
        xmlNodePtr paxesNode = findNode(segNode, "passengers");
        if(paxesNode != NULL) {
            xmlNodePtr paxNode = findNode(paxesNode, "pax");
            if(paxNode != NULL) {
                xmlNodePtr refuseNode = findNode(paxNode, "refuse");
                if(refuseNode != NULL && !NodeIsNULL(refuseNode)) {
                    LogTrace(TRACE3) << "Cancel request detected!";
                    return IatciInterface::Ckx;
                }
            }
        }

        LogTrace(TRACE3) << "Update request detected!";
        return IatciInterface::Cku;
    }

    TST();
    ASSERT(false); // throw always
}

bool IatciInterface::DispatchCheckInRequest(xmlNodePtr reqNode, xmlNodePtr ediResNode)
{
    RequestType reqType = ClassifyCheckInRequest(reqNode);
    switch(reqType)
    {
    case Cki:
        return InitialRequest(reqNode, ediResNode);
    case Cku:
        return UpdateRequest(reqNode, ediResNode);
    case Ckx:
        return CancelRequest(reqNode, ediResNode);
    default:
        break;
    }

    TST();
    ASSERT(false); // throw always

    return false;
}

// функция проверяет, будет ли послан iatci-запрос на регистрацию(checkin,update,cancel)
// причём запрос может быть не послан только в случае update, если изменения не затронули
// edifact-вкладки
bool IatciInterface::WillBeSentCheckInRequest(xmlNodePtr reqNode, xmlNodePtr ediResNode)
{
    RequestType reqType = ClassifyCheckInRequest(reqNode);
    if(reqType == Cku) {
        if(getCkuParams(reqNode)) {
            return true;
        } else {
            return false;
        }
    }

    return true;
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

bool IatciInterface::MayNeedSendIatci(xmlNodePtr reqNode)
{
    XmlCheckInTabs tabs(findNodeR(reqNode, "segments"));
    return tabs.containsEdiTab();
}

bool IatciInterface::InitialRequest(xmlNodePtr reqNode, xmlNodePtr ediResNode)
{
    boost::optional<iatci::CkiParams> ckiParams = getCkiParams(reqNode);
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

    return true; /*req was sent*/
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

bool IatciInterface::UpdateRequest(xmlNodePtr reqNode, xmlNodePtr ediResNode)
{
    // запрос на Update может не затронуть edifact-сегменты:
    // в этом случае ckuParams будут отсутствовать
    boost::optional<iatci::CkuParams> ckuParams = getCkuParams(reqNode);
    if(ckuParams)
    {
        edifact::KickInfo kickInfo = getIatciKickInfo(reqNode, ediResNode);
        edilib::EdiSessionId_t sessIda = edifact::SendCkuRequest(ckuParams.get(),
                                                                 getIatciPult(),
                                                                 getIatciRequestContext(kickInfo),
                                                                 kickInfo);

        if(TReqInfo::Instance()->api_mode)
        {
            LogTrace(TRACE3) << "throw TlgToBePostponed for edi_session=" << sessIda;
            throw TlgHandling::TlgToBePostponed(sessIda);
        }

        return true; /*req was sent*/
    }

    // выставим "was_sent_iatci" в значение false, т.к. запрос не был послан
    ReqParams(reqNode).setBoolParam("was_sent_iatci", false);

    return false; /*req was NOT sent*/
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

bool IatciInterface::CancelRequest(xmlNodePtr reqNode, xmlNodePtr ediResNode)
{
    tst();
    boost::optional<iatci::CkxParams> ckxParams = getCkxParams(reqNode);
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

    return true; /*req was sent*/
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

void IatciInterface::PasslistRequest(xmlNodePtr reqNode, int grpId)
{
    edifact::KickInfo kickInfo = getIatciKickInfo(reqNode, NULL);
    edilib::EdiSessionId_t sessIda = edifact::SendPlfRequest(getPlfParams(grpId),
                                                             getIatciPult(),
                                                             getIatciRequestContext(kickInfo),
                                                             kickInfo);

    if(TReqInfo::Instance()->api_mode)
    {
        LogTrace(TRACE3) << "throw TlgToBePostponed for edi_session=" << sessIda;
        throw TlgHandling::TlgToBePostponed(sessIda);
    }
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
    DoKickAction(reqNode, resNode, lRes, "iatci_cki_result", ActSavePax);
    FuncOut(CheckinKickHandler);
}

void IatciInterface::UpdateKickHandler(xmlNodePtr reqNode, xmlNodePtr resNode,
                                       const std::list<iatci::Result>& lRes)
{
    FuncIn(UpdateKickHandler);
    DoKickAction(reqNode, resNode, lRes, "iatci_cku_result", ActSavePax);
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
    DoKickAction(reqNode, resNode, lRes, "iatci_plf_result", ActLoadPax);
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
    AstraLocale::showProgError("MSG.DCS_CONNECT_ERROR");
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
        else
        {
            std::list<iatci::Result> lRes = iatci::loadCkiData(res->ediSession());
            ASSERT(!lRes.empty());
            iatci::Result::Action_e action = lRes.front().action();
            if(res->status() == RemoteStatus::Success)
            {
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

                switch(action)
                {
                case iatci::Result::Checkin:
                case iatci::Result::Cancel:
                case iatci::Result::Update:
                    // откат смены статуса, произошедшей ранее
                    DoKickAction(reqNode, resNode, lRes, "", ActRollbackStatus);
                    break;

                case iatci::Result::Reprint:
                case iatci::Result::Passlist:
                case iatci::Result::Seatmap:
                case iatci::Result::SeatmapForPassenger:
                    // ; NOP
                    break;

                default:
                    break;
                }
            }
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
            if(CheckInInterface::SavePax(termReqNode, ediResNode, resNode)) {
                int grpId = SaveIatciPax(lRes, ediResNode, termReqNode, resNode);
                if(grpId) {
                    CheckInInterface::LoadIatciPax(NULL, resNode, grpId, false);
                }
            } else {
                LogError(STDLOG) << "SavePax returns an error!";
            }
            break;

        case ActRollbackStatus:
            LogTrace(TRACE3) << "Call EtRollbackStatus()...";
            ETStatusInterface::ETRollbackStatus(ediResCtxt.docPtr(), false);
            break;

        case ActLoadPax:
            tst();
            ReqParams(termReqNode).setBoolParam("need_sync", false);
            SaveIatciPax(lRes, ediResNode, termReqNode, resNode);
            CheckInInterface::instance()->LoadPax(NULL, termReqNode, resNode);
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
