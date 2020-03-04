#include "cuws_handlers.h"
#include "xml_unit.h"
#include "web_search.h"
#include "serverlib/str_utils.h"
#include <boost/regex.hpp>

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace ASTRA;

namespace CUWS {

// Классы CUWS
struct TBagSummary {
    const string summary_bags_tag    = "SummaryBags";
    const string tag_no_tag          = "BagTagNumber";
    const string bag_id_tag          = "BaggageId";
    const string pnr_tag             = "PNR";

    string BagTagNumber;
    string BaggageId;
    string PNR;

    void clear()
    {
        BagTagNumber.clear();
        BaggageId.clear();
        PNR.clear();
    }

    TBagSummary() { clear(); }

    void to_content(ostringstream &body) const;
};

struct TBagSummaryList {
    private:
        set<int> pax_ids;
        list<TBagSummary> items;
    public:
        void clear()
        {
            pax_ids.clear();
            items.clear();
        }
        TBagSummaryList() { clear(); }
        void add(int pax_id);
        void to_content(const string &parent, ostringstream &body);

};

void to_envelope(xmlNodePtr resNode, const string &data)
{
    to_content(resNode, "/cuws_envelope.xml", "BODY", data);
}

void wrap_empty(const string &tag, ostringstream &body)
{
    body << "<" << tag << "/>";
}

void wrap_begin(const string &tag, ostringstream &body)
{
    body << "<" << tag << ">";
}

void wrap_end(const string &tag, ostringstream &body)
{
    body << "</" << tag << ">";
}

void wrap(const string &tag, const string &data, ostringstream &body)
{
    if(data.empty())
        wrap_empty(tag, body);
    else {
        wrap_begin(tag, body);
        body << data;
        wrap_end(tag, body);
    }
}

void wrap(const string &tag, TDateTime data, ostringstream &body)
{
    string result;
    if(data != NoExists)
        result = DateTimeToStr(data, "yyyy-mm-dd'T'hh:nn:00");
    wrap(tag, result, body);
}

void wrap(const string &tag, int data, ostringstream &body)
{
    ostringstream result;
    if(data != NoExists)
        result << data;
    wrap(tag, result.str(), body);
}

struct TCUWSDateTime {
    TDateTime val;
    TCUWSDateTime() { clear(); }
    void clear() { val = NoExists; }
    void fromXML(xmlNodePtr dateTimeNode);
    void trace(ostringstream &ss);
};

struct TLegLocation {
    const string airp_tag = "AirportCode";
    const string scd_tag = "ScheduledDateTime";
    const string est_tag = "EstimatedDateTime";

    string AirportCode;
    TCUWSDateTime ScheduledDateTime;
    TCUWSDateTime EstimatedDateTime;

    void clear() {
        AirportCode.clear();
        ScheduledDateTime.clear();
        EstimatedDateTime.clear();
    }

    TLegLocation() { clear(); }
    void fromXML(xmlNodePtr legLocationNode);
    void trace(ostringstream &ss);
    void to_content(const string &parent, ostringstream &body) const;
};

void TLegLocation::to_content(const string &parent, ostringstream &body) const
{
    wrap_begin(parent, body);
    wrap(airp_tag, AirportCode, body);
    wrap(scd_tag, ScheduledDateTime.val, body);
    if(ScheduledDateTime.val != EstimatedDateTime.val)
        wrap(est_tag, EstimatedDateTime.val, body);
    wrap_end(parent, body);
}

struct TLeg {
    const string leg_tag        = "Leg";
    const string airline_tag    = "AirlineCode";
    const string flt_tag        = "FlightNumber";

    string AirlineCode;
    int FlightNumber;
    TLegLocation Origin;
    TLegLocation Arrival;

    void clear()
    {
        AirlineCode.clear();
        FlightNumber = NoExists;
        Origin.clear();
        Arrival.clear();
    }

    TLeg() { clear(); }

    void fromXML(xmlNodePtr legNode);
    string trace(ostringstream &ss);
    bool operator == (const TTrferRouteItem &seg) const;
    void to_content(ostringstream &body) const;
};

void TLeg::to_content(ostringstream &body) const
{
    wrap_begin(leg_tag, body);

    wrap(airline_tag, AirlineCode, body);
    wrap(flt_tag, FlightNumber, body);
    Origin.to_content("Origin", body);
    Arrival.to_content("Arrival", body);

    wrap_end(leg_tag, body);
}

struct TEligibleLeg {
    const string ref_tag = "LegReferenceId";
    const string seq_tag = "Sequence";

    TLeg Leg;
    string LegReferenceId;
    int sequence;

    void clear()
    {
        Leg.clear();
        LegReferenceId.clear();
        sequence = NoExists;
    }

    TEligibleLeg() { clear(); }
    void to_content(ostringstream &body) const;
};

void TEligibleLeg::to_content(ostringstream &body) const
{
    Leg.to_content(body);
    wrap(ref_tag, LegReferenceId, body);
    wrap(seq_tag, sequence, body);
}

struct TEligibleLegList: public list<TEligibleLeg> {
    const string container_tag = "EligibleLegs";

    void add(const TTrferRoute &route);
    void to_content(const string &parent, ostringstream &body);
};

void TEligibleLegList::to_content(const string &parent, ostringstream &body)
{
    if(empty())
            wrap_empty(parent, body);
    else {
        wrap_begin(parent, body);
        for(const auto &i: *this) {
            wrap_begin(container_tag, body);
            i.to_content(body);
            wrap_end(container_tag, body);
        }
        wrap_end(parent, body);
    }
}


void TEligibleLegList::add(const TTrferRoute &route)
{
    for(const auto &i: route) {
        TEligibleLeg eligible_leg;
        eligible_leg.Leg.AirlineCode = i.operFlt.airline;
        eligible_leg.Leg.FlightNumber = i.operFlt.flt_no;

        eligible_leg.Leg.Origin.AirportCode = i.operFlt.airp;
        eligible_leg.Leg.Origin.ScheduledDateTime.val = LocalToUTC(i.operFlt.scd_out, AirpTZRegion(i.operFlt.airp));
        eligible_leg.Leg.Origin.EstimatedDateTime.val = LocalToUTC(i.operFlt.est_scd_out(), AirpTZRegion(i.operFlt.airp));


        TTripRoute arrival;
        arrival.GetRouteAfter(NoExists, i.operFlt.point_id, trtNotCurrent, trtNotCancelled);
        if(arrival.empty())
            throw Exception("::add: arrival not found for point_id: %d", i.operFlt.point_id);

        eligible_leg.Leg.Arrival.AirportCode = i.airp_arv;
        TDateTime act_in;
        TTripInfo::get_times_in(
                arrival[0].point_id,
                eligible_leg.Leg.Arrival.ScheduledDateTime.val,
                eligible_leg.Leg.Arrival.EstimatedDateTime.val,
                act_in);

        if(eligible_leg.Leg.Arrival.EstimatedDateTime.val == NoExists)
            eligible_leg.Leg.Arrival.EstimatedDateTime.val = eligible_leg.Leg.Arrival.ScheduledDateTime.val;
        eligible_leg.LegReferenceId = IntToString(i.operFlt.point_id);
        eligible_leg.sequence = size();
        push_back(eligible_leg);
    }
}



bool TLeg::operator == (const TTrferRouteItem &seg) const
{
    TTripRoute arrival;
    arrival.GetRouteAfter(NoExists, seg.operFlt.point_id, trtNotCurrent, trtNotCancelled);
    if(arrival.empty())
        throw Exception("TLeg::operator ==: arrival not found for point_id: %d", seg.operFlt.point_id);

    TDateTime scd_in, est_in, act_in;
    TTripInfo::get_times_in(arrival[0].point_id, scd_in, est_in, act_in);
    if(est_in == NoExists) est_in = scd_in;

    return
        AirlineCode == seg.operFlt.airline and
        FlightNumber == seg.operFlt.flt_no and
        Origin.AirportCode == seg.operFlt.airp and
        Origin.ScheduledDateTime.val == LocalToUTC(seg.operFlt.scd_out, AirpTZRegion(seg.operFlt.airp)) and
        Origin.EstimatedDateTime.val == LocalToUTC(seg.operFlt.est_scd_out(), AirpTZRegion(seg.operFlt.airp)) and
        Arrival.AirportCode == seg.airp_arv and
        Arrival.ScheduledDateTime.val == scd_in and
        Arrival.EstimatedDateTime.val == est_in;
}

struct TPassengerDetails {
    string LastName;
    string FirstName;
    string PNR;
    TLeg NextLeg;

    void fromXML(xmlNodePtr actionNode);
    string trace();
};

void TLegLocation::trace(ostringstream &ss)
{
    ss
        << "AirportCode: " << AirportCode << endl
        << "ScheduledDateTime: ";
    ScheduledDateTime.trace(ss);
    ss << "EstimatedDateTime: ";
    EstimatedDateTime.trace(ss);
}

void TCUWSDateTime::trace(ostringstream &ss)
{
    ss << DateTimeToStr(val) << " (NoExists: " << (val == NoExists) << ")" << endl;
}

void TCUWSDateTime::fromXML(xmlNodePtr dateTimeNode)
{
    string  date_time;
    if(dateTimeNode) date_time = NodeAsString(dateTimeNode);
    if(date_time.empty()) return;
    static const boost::regex e("^.*(\\d{4}-\\d{2}-\\d{2}T\\d{2}:\\d{2}:\\d{2}).*$");
    boost::match_results<std::string::const_iterator> results;
    if(boost::regex_match(date_time, results, e)) {
        date_time = results[1];
        if(StrToDateTime(date_time.c_str(), "yyyy-mm-dd'T'hh:nn:ss", val) == EOF)
            throw Exception("StrToDateTime failed for %s", date_time.c_str());
    } else
        throw Exception("Unexpected date time: %s", date_time.c_str());
}

void TLegLocation::fromXML(xmlNodePtr legLocationNode)
{
    TElemFmt fmt;
    AirportCode = ElemToElemId(etAirp, NodeAsString(airp_tag.c_str(), legLocationNode), fmt);
    xmlNodePtr curNode = legLocationNode->children;
    ScheduledDateTime.fromXML(NodeAsNode(scd_tag.c_str(), legLocationNode));
    EstimatedDateTime.fromXML(NodeIsNULLFast(est_tag.c_str(), curNode, true) ? nullptr : NodeAsNode(est_tag.c_str(), legLocationNode));
    if(EstimatedDateTime.val == NoExists)
        EstimatedDateTime.val = ScheduledDateTime.val;
}

string TLeg::trace(ostringstream &ss)
{
    ss
        << "AirlineCode: " << AirlineCode << endl
        << "FlightNumber: " << FlightNumber << endl;
    Origin.trace(ss);
    Arrival.trace(ss);
    return ss.str();
}

void TLeg::fromXML(xmlNodePtr legNode)
{
    TElemFmt fmt;
    AirlineCode = ElemToElemId(etAirline, NodeAsString("AirlineCode", legNode), fmt);
    FlightNumber = NodeAsInteger("FlightNumber", legNode);
    Origin.fromXML(NodeAsNode("Origin", legNode));
    Arrival.fromXML(NodeAsNode("Arrival", legNode));
}

string TPassengerDetails::trace()
{
    ostringstream result;
    result
        << "LastName: " << LastName << endl
        << "FirstName: " << FirstName << endl
        << "PNR: " << PNR << endl;
    return NextLeg.trace(result);
}

void TPassengerDetails::fromXML(xmlNodePtr actionNode)
{
    LastName = NodeAsString("LastName", actionNode);
    FirstName = NodeAsString("FirstName", actionNode);
    PNR = NodeAsString("PNR", actionNode);
    NextLeg.fromXML(NodeAsNode("NextLeg", actionNode));
}

void Search_Bags_By_PassengerDetails(xmlNodePtr actionNode, xmlNodePtr resNode)
{
    TPassengerDetails pax;
    pax.fromXML(actionNode);
    LogTrace(TRACE5) << pax.trace();

    CheckIn::Search search;
    CheckIn::TSimplePaxList paxs;
    SurnameFilter surname_filter;
    surname_filter.surname = pax.LastName;
    FlightFilter flight_filter;
    flight_filter.airline = pax.NextLeg.AirlineCode;
    flight_filter.airp = pax.NextLeg.Origin.AirportCode;
    flight_filter.flt_no = pax.NextLeg.FlightNumber;
    flight_filter.scd_out = pax.NextLeg.Origin.ScheduledDateTime.val;
    search(paxs, surname_filter, flight_filter);
    LogTrace(TRACE5) << "paxs.size(): " << paxs.size();

    TBagSummaryList data;
    for(const auto &i: paxs)
        data.add(i.paxId());
    ostringstream body;
    data.to_content("ns:Search_Bags_By_PassengerDetailsResponse", body);
    to_envelope(resNode, body.str());
}

void TBagSummary::to_content(ostringstream &body) const
{
    wrap_begin(summary_bags_tag, body);

    wrap(tag_no_tag, BagTagNumber, body);
    wrap(bag_id_tag, BaggageId, body);
    wrap(pnr_tag, PNR, body);

    wrap_end(summary_bags_tag, body);
}

void TBagSummaryList::to_content(const string &parent, ostringstream &body)
{
    if(items.empty())
            wrap_empty(parent, body);
    else {
        wrap_begin(parent, body);
        for(const auto &i: items) i.to_content(body);
        wrap_end(parent, body);
    }
}

void TBagSummaryList::add(int pax_id)
{
    auto i = pax_ids.insert(pax_id);
    if(i.second) {
        CheckIn::TSimplePaxItem pax;
        pax.getByPaxId(pax_id);
        multiset<TBagTagNumber> tags;
        GetTagsByPool(pax.grp_id, pax.bag_pool_num, tags, true);
        if(not tags.empty()) {
            TPnrAddrs pnrs;
            pnrs.getByPaxId(pax_id);
            string pnr_addr;
            if(not pnrs.empty()) pnr_addr = pnrs.begin()->addr;
            for(const auto &i: tags) {
                TBagSummary item;
                item.BagTagNumber = i.str();
                item.PNR = pnr_addr;
                items.push_back(item);
            }
        }
    }
}

void Search_Bags_By_BCBP(xmlNodePtr actionNode, xmlNodePtr resNode)
{
    int point_id, reg_no, pax_id;
    bool isBoardingPass;
    SearchPaxByScanData(NodeAsString("BCBP", actionNode), point_id, reg_no, pax_id, isBoardingPass);
    ostringstream body;
    TBagSummaryList data;
    if(isBoardingPass)
        data.add(pax_id);
    data.to_content("ns:Search_Bags_By_BCBPResponse", body);
    to_envelope(resNode, body.str());
}

string getResource(string file_path)
{
    try
    {
        TCachedQuery Qry1(
            "select text from HTML_PAGES, HTML_PAGES_TEXT "
            "where "
            "   HTML_PAGES.name = :name and "
            "   HTML_PAGES.id = HTML_PAGES_TEXT.id "
            "order by "
            "   page_no",
            QParams()
            << QParam("name", otString, file_path)
        );
        Qry1.get().Execute();
        string result;
        for (; not Qry1.get().Eof; Qry1.get().Next())
            result += Qry1.get().FieldAsString("text");
        return result;
    }
    catch(Exception &E)
    {
        cout << __FUNCTION__ << " " << E.what() << endl;
        return "";
    }
}

void empty_response(xmlNodePtr resNode)
{
    SetProp( NewTextChild(resNode, "content"), "b64", true);
}

void to_content(xmlNodePtr resNode, const string &resource, const string &tag, const string &tag_data)
{
    string data = getResource(resource);
    if(not tag.empty())
        data = StrUtils::b64_encode(
                boost::regex_replace(
                    StrUtils::b64_decode(data),
                    boost::regex(tag), ConvertCodepage(tag_data, "CP866", "UTF8")));
    SetProp( NewTextChild(resNode, "content",  data), "b64", true);
}

void CUWSSuccess(xmlNodePtr resNode)
{
    to_content(resNode, "/cuws_success.xml");
}

void Get_EligibleBagLegs_By_TagNum(xmlNodePtr actionNode, xmlNodePtr resNode)
{
    double TagNum = NodeAsFloat("TagNum", actionNode);
    TLeg NextLeg;
    NextLeg.fromXML(NodeAsNode("NextLeg", actionNode));

    TCachedQuery Qry(
            "select grp_id from bag_tags where no = :no",
            QParams() << QParam("no", otFloat, TagNum));
    Qry.get().Execute();
    TEligibleLegList leg_list;
    for(; not Qry.get().Eof; Qry.get().Next()) {
        int grp_id = Qry.get().FieldAsInteger("grp_id");
        TTrferRoute route;
        if (!route.GetRoute(grp_id, trtWithFirstSeg) || route.empty())
            continue;
        if(NextLeg == route[0]) {
            CheckIn::TSimplePaxGrpItem grp;
            grp.getByGrpId(grp_id);
            if(grp.trfer_confirm)
                leg_list.add(route);
            break;
        }
    }
    ostringstream body;
    leg_list.to_content("ns:Get_EligibleBagLegs_By_TagNumResponse", body);
    to_envelope(resNode, body.str());
}

void Set_BagDetails_In_BagInfo(xmlNodePtr actionNode, xmlNodePtr resNode)
{
    empty_response(resNode);
}

void Set_Bag_as_Active(xmlNodePtr actionNode, xmlNodePtr resNode)
{
    empty_response(resNode);
}

void Search_FreeBagAllowanceOffer_By_BagType_BCBP(xmlNodePtr actionNode, xmlNodePtr resNode)
{
    ostringstream body;
    wrap_empty("ns:Search_FreeBagAllowanceOffer_By_BagType_BCBPResponse", body);
    to_envelope(resNode, body.str());
}

void Search_FreeBagAllowanceOffer_By_BagType_PaxDetails(xmlNodePtr actionNode, xmlNodePtr resNode)
{
    ostringstream body;
    wrap_empty("ns:Search_FreeBagAllowanceOffer_By_BagType_PaxDetailsResponse", body);
    to_envelope(resNode, body.str());
}

} //end namespace CUWS

