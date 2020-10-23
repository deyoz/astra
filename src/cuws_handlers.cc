#include "cuws_handlers.h"
#include "xml_unit.h"
#include "web_search.h"
#include "serverlib/str_utils.h"
#include <boost/regex.hpp>
#include "html_pages.h"
#include "baggage_calc.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace ASTRA;

namespace CUWS {

class TWeightUnit {
    public:
        enum Enum {
            Kilograms,
            Pounds,
            Unknown
        };

        static const std::list< std::pair<Enum, std::string> >& pairs()
        {
            static std::list< std::pair<Enum, std::string> > l;
            if (l.empty())
            {
                l.push_back(std::make_pair(Kilograms,   "Kilograms"));
                l.push_back(std::make_pair(Pounds,      "Pounds"));
                l.push_back(std::make_pair(Unknown,     ""));
            }
            return l;
        }
};

class TWeightUnits : public ASTRA::PairList<TWeightUnit::Enum, std::string>
{
  private:
    virtual std::string className() const { return "TWeightUnits"; }
  public:
    TWeightUnits() : ASTRA::PairList<TWeightUnit::Enum, std::string>(TWeightUnit::pairs(),
                                                                       TWeightUnit::Unknown,
                                                                       boost::none) {}
};

const TWeightUnits& WeightUnits()
{
  static TWeightUnits weightUnits;
  return weightUnits;
}

class TLengthUnit {
    public:
        enum Enum {
            Meter,
            Inch,
            Unknown
        };

        static const std::list< std::pair<Enum, std::string> >& pairs()
        {
            static std::list< std::pair<Enum, std::string> > l;
            if (l.empty())
            {
                l.push_back(std::make_pair(Meter,   "Meter"));
                l.push_back(std::make_pair(Inch,    "Inch"));
                l.push_back(std::make_pair(Unknown, ""));
            }
            return l;
        }
};

class TLengthUnits : public ASTRA::PairList<TLengthUnit::Enum, std::string>
{
  private:
    virtual std::string className() const { return "TLengthUnits"; }
  public:
    TLengthUnits() : ASTRA::PairList<TLengthUnit::Enum, std::string>(TLengthUnit::pairs(),
                                                                       TLengthUnit::Unknown,
                                                                       boost::none) {}
};

const TLengthUnits& LengthUnits()
{
  static TLengthUnits lengthUnits;
  return lengthUnits;
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

void wrap_string(const string &tag, const string &data, ostringstream &body)
{
    if(data.empty())
        wrap_empty(tag, body);
    else {
        wrap_begin(tag, body);
        body << data;
        wrap_end(tag, body);
    }
}

void wrap_date(const string &tag, TDateTime data, ostringstream &body)
{
    string result;
    if(data != NoExists)
        result = DateTimeToStr(data, "yyyy-mm-dd'T'hh:nn:00");
    wrap_string(tag, result, body);
}

void wrap_int(const string &tag, int data, ostringstream &body)
{
    ostringstream result;
    if(data != NoExists)
        result << data;
    wrap_string(tag, result.str(), body);
}

void wrap_double(const string &tag, double data, ostringstream &body)
{
    ostringstream result;
    if(data != NoExists)
        result << fixed << setprecision(2) << data;
    wrap_string(tag, result.str(), body);
}

// Классы CUWS
struct TBagSummary {
    const string summary_bags_tag    = "SummaryBags";
    const string tag_no_tag          = "BagTagNumber";
    const string bag_id_tag          = "BaggageId";
    const string pnr_tag             = "PNR";

    boost::optional<TBagTagNumber> BagTagNumber;
    int BaggageId;
    string PNR;

    void clear()
    {
        BagTagNumber = boost::none;
        BaggageId = NoExists;
        PNR.clear();
    }

    TBagSummary() { clear(); }

    void to_content(const string &parent, ostringstream &body) const;
    void set_active(bool pr_active);
    void fromXML(xmlNodePtr summaryNode);
};

void TBagSummary::set_active(bool pr_active)
{
    if(BaggageId == NoExists)
        throw Exception("BaggageId not specified");
    TCachedQuery Qry("select * from cuws_tags where bag_id = :bag_id for update",
            QParams() << QParam("bag_id", otInteger, BaggageId));
    Qry.get().Execute();
    if(Qry.get().Eof)
        throw Exception("BaggageId not found");
    if(not Qry.get().FieldIsNULL("time_delete"))
        throw Exception("Baggage does not exist");
    if(pr_active) {
        if(not Qry.get().FieldIsNULL("time_activate"))
            throw Exception("Baggage is activated already");
    } else {
        if(Qry.get().FieldIsNULL("time_activate"))
            throw Exception("Baggage is not activated");
    }

    TCachedQuery updateQry(
             (string)"update cuws_tags set " + (pr_active ? "time_activate" : "time_delete" ) + " = :time where bag_id = :bag_id",
            QParams()
            << QParam("bag_id", otInteger, BaggageId)
            << QParam("time", otDate, NowUTC()));
    updateQry.get().Execute();
}

void TBagSummary::fromXML(xmlNodePtr summaryNode)
{
    xmlNodePtr curNode = summaryNode->children;
    xmlNodePtr tagNode = GetNodeFast(tag_no_tag.c_str(), curNode);
    if(tagNode)
        BagTagNumber = boost::in_place("", NodeAsFloat(tagNode));
    BaggageId = NodeAsIntegerFast(bag_id_tag.c_str(), curNode, NoExists);
    PNR = NodeAsStringFast(pnr_tag.c_str(), curNode, "");
}

struct TIndividual {
    const string Individual_tag     = "Individual";
    const string LastName_tag       = "LastName";
    const string FirstName_tag      = "FirstName";
    const string PassengerId_tag    = "PassengerId";

    string LastName;
    string FirstName;
    int PassengerId;
    void clear()
    {
        LastName.clear();
        FirstName.clear();
        PassengerId = NoExists;
    }
    TIndividual() { clear(); }
    bool empty()
    {
        return
            LastName.empty() and
            FirstName.empty() and
            PassengerId == NoExists;
    }
    void to_content(ostringstream &body);
    void fromXML(xmlNodePtr individualNode);
};

void TIndividual::fromXML(xmlNodePtr individualNode)
{
    xmlNodePtr curNode = individualNode->children;
    LastName = NodeAsStringFast(LastName_tag.c_str(), curNode, "");
    FirstName = NodeAsStringFast(FirstName_tag.c_str(), curNode, "");
    PassengerId = NodeAsIntegerFast(PassengerId_tag.c_str(), curNode, NoExists);
}

void TIndividual::to_content(ostringstream &body)
{
    wrap_begin(Individual_tag, body);
    if(not LastName.empty())
        wrap_string(LastName_tag, LastName, body);
    if(not FirstName.empty())
        wrap_string(FirstName_tag, FirstName, body);
    if(PassengerId != NoExists)
        wrap_int(PassengerId_tag, PassengerId, body);
    wrap_end(Individual_tag, body);
}

struct TBagWeight {
    const string Weight_tag = "Weight";
    const string Unit_tag = "Unit";

    double Weight;
    TWeightUnit::Enum Unit;
    void clear()
    {
        Weight = NoExists;
        Unit = TWeightUnit::Unknown;
    }
    bool empty()
    {
        return
            Weight == NoExists and
            Unit == TWeightUnit::Unknown;
    }
    TBagWeight() { clear(); }
    void fromXML(xmlNodePtr bagWeightNode);
    void operator = (const TBagWeight &val)
    {
        Weight = val.Weight;
        Unit = val.Unit;
    }
    void to_content(const string &parent, ostringstream &body);
};

struct TBagDimension {
    const string Dimension_tag  = "Dimension";
    const string Length_tag     = "Length";
    const string Height_tag     = "Height";
    const string Width_tag      = "Width";
    const string Unit_tag       = "Unit";

    double Length;
    double Height;
    double Width;
    TLengthUnit::Enum Unit;
    void clear()
    {
        Length = NoExists;
        Height = NoExists;
        Width = NoExists;
        Unit = TLengthUnit::Unknown;
    }
    bool empty()
    {
        return
            Length == NoExists and
            Height == NoExists and
            Width == NoExists and
            Unit == TLengthUnit::Unknown;
    }
    TBagDimension() { clear(); }
    void fromXML(xmlNodePtr dimNode);
    void operator = (const TBagDimension &val)
    {
        Length = val.Length;
        Height = val.Height;
        Width = val.Width;
        Unit = val.Unit;
    }
    void to_content(const string &parent, ostringstream &body);
};

struct TCostType {
    const string Value_tag = "Value";
    const string CurrencyCode_tag = "CurrencyCode";

    double Value;
    string CurrencyCode;
    void clear()
    {
        Value = NoExists;
        CurrencyCode.clear();
    }
    bool empty() const
    {
        return
            Value == NoExists and
            CurrencyCode.empty();
    }
    TCostType() { clear(); }
    TCostType(double _Value, const string _CurrencyCode):
        Value(_Value),
        CurrencyCode(_CurrencyCode)
    {}
    void to_content(const string &parent, ostringstream &body) const;
};

struct TCostList: public vector<TCostType> {
    void to_content(const string &parent, ostringstream &body);
};

struct TBagAllowance {
    const string BagAllowance_tag = "BagAllowance";
    const string BasicCode_tag = "BasicCode";
    const string AllowedNumber_tag = "AllowedNumber";

    int BasicCode;
    int AllowedNumber;
    TBagWeight MaximumWeight;
    TBagWeight TotalMaximumWeight;
    TBagDimension MaximumDimension;
    TCostList Price;
    void clear()
    {
        BasicCode = NoExists;
        AllowedNumber = NoExists;
        MaximumWeight.clear();
        TotalMaximumWeight.clear();
        MaximumDimension.clear();
        Price.clear();
    }
    bool empty()
    {
        return
            BasicCode == NoExists and
            AllowedNumber == NoExists and
            MaximumWeight.empty() and
            TotalMaximumWeight.empty() and
            MaximumDimension.empty() and
            Price.empty();
    }
    void to_content(ostringstream &body);
};

struct TBaggageItem {
    const string BaggageItem_tag = "BaggageItem";
    const string amount_tag = "amount";
    const string weight_tag = "weight";
    const string time_issue_tag = "time_issue";
    const string time_activate_tag = "time_activate";

    int amount;
    double weight;
    TDateTime time_issue;
    TDateTime time_activate;

    TBaggageItem(int _amount, double _weight, TDateTime _time_issue, TDateTime _time_activate):
        amount(_amount),
        weight(_weight),
        time_issue(_time_issue),
        time_activate(_time_activate)
    {}
    void to_content(ostringstream &body) const;
};

void TBaggageItem::to_content(ostringstream &body) const
{
    wrap_begin(BaggageItem_tag, body);
    wrap_int(amount_tag, amount, body);
    wrap_double(weight_tag, weight, body);
    if(time_issue != NoExists)
        wrap_date(time_issue_tag, time_issue, body);
    if(time_activate != NoExists)
        wrap_date(time_activate_tag, time_activate, body);
    wrap_end(BaggageItem_tag, body);
}

struct TBaggageList: public vector<TBaggageItem> {
    const string BaggageList_tag = "BaggageList";
    void fromDB(const CheckIn::TSimplePaxItem &pax);
    void to_content(ostringstream &body) const;
};

void TBaggageList::to_content(ostringstream &body) const
{
    if(empty())
        wrap_empty(BaggageList_tag, body);
    else {
        wrap_begin(BaggageList_tag, body);
        for(const auto &i: *this)
            i.to_content(body);
        wrap_end(BaggageList_tag, body);
    }
}

void TBaggageList::fromDB(const CheckIn::TSimplePaxItem &pax)
{
    CheckIn::TBagMap bag_map;
    bag_map.fromDB(pax.grp_id);
    for(const auto &i: bag_map) {
        if(i.second.bag_pool_num == pax.bag_pool_num)
            push_back(TBaggageItem(i.second.amount, i.second.weight, NoExists, NoExists));
    }
    TCachedQuery Qry(
            "select * from cuws_tags where pax_id = :pax_id and time_delete is null",
            QParams() << QParam("pax_id", otInteger, pax.id));
    Qry.get().Execute();
    for(; not Qry.get().Eof; Qry.get().Next())
        push_back(TBaggageItem(1,
                    Qry.get().FieldAsFloat("weight"),
                    Qry.get().FieldAsDateTime("time_issue"),
                    Qry.get().FieldIsNULL("time_activate") ? NoExists : Qry.get().FieldAsDateTime("time_activate")));
}

struct  TPassengerInfo {
    const string pnr_tag = "PNR";

    string PNR;
    TIndividual Individual;
    TBagAllowance BagAllowance;
    TBaggageList BaggageList;

    bool empty()
    {
        return
            Individual.empty() and
            BagAllowance.empty() and
            BaggageList.empty() and
            PNR.empty();
    }

    void clear()
    {
        PNR.clear();
        Individual.clear();
        BagAllowance.clear();
        BaggageList.clear();
    }
    TPassengerInfo() { clear(); }
    void fromDB(int pax_id);
    void to_content(const string &parent, ostringstream &body);
    void fromXML(xmlNodePtr paxNode);
};

void TPassengerInfo::fromXML(xmlNodePtr paxNode)
{
    xmlNodePtr curNode = paxNode->children;
    PNR = NodeAsStringFast(pnr_tag.c_str(), curNode, "");
    Individual.fromXML(NodeAsNodeFast(Individual.Individual_tag.c_str(), curNode));
}

void TPassengerInfo::to_content(const string &parent, ostringstream &body)
{
    if(empty())
        wrap_empty(parent, body);
    else {
        wrap_begin(parent, body);
        if(not PNR.empty())
            wrap_string(pnr_tag, PNR, body);
        Individual.to_content(body);
        BagAllowance.to_content(body);
        BaggageList.to_content(body);
        wrap_end(parent, body);
    }
}

void TPassengerInfo::fromDB(int pax_id)
{
    CheckIn::TSimplePaxItem pax;
    pax.getByPaxId(pax_id);
    TPnrAddrs pnrs;
    pnrs.getByPaxId(pax_id);
    string pnr_addr;
    if(not pnrs.empty()) PNR = pnrs.begin()->addr;
    Individual.FirstName = pax.name;
    Individual.LastName = pax.surname;
    Individual.PassengerId = pax_id;

    CheckIn::TPaxGrpItem grp_item;
    if(not grp_item.getByGrpIdWithBagConcepts(pax.grp_id))
        throw Exception("grp_item.getByGrpIdWithBagConcepts failed");

    TBagConcept::Enum bagAllowanceType = grp_item.getBagAllowanceType();
    boost::optional<::BagAllowance> bagAllowance;
    if (bagAllowanceType == TBagConcept::Piece)
        bagAllowance = PieceConcept::getBagAllowance(pax);
    else if (bagAllowanceType == TBagConcept::Weight)
    {
        bagAllowance = WeightConcept::getBagAllowance(pax);
        if (!bagAllowance) {
            TTripInfo trip_info;
            trip_info.getByPaxId(pax_id);
            bagAllowance = WeightConcept::calcBagAllowance(pax, grp_item, trip_info);
        }
    }
    if(bagAllowance) {
        if(bagAllowance->amount)
            BagAllowance.AllowedNumber = bagAllowance->amount.get();
        else
            BagAllowance.AllowedNumber = 1;
        if(bagAllowance->weight) {
            BagAllowance.MaximumWeight.Weight = bagAllowance->weight.get();
            BagAllowance.MaximumWeight.Unit = TWeightUnit::Kilograms;
        }
    }

    BaggageList.fromDB(pax);
}

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
    wrap_string(airp_tag, AirportCode, body);
    wrap_date(scd_tag, ScheduledDateTime.val, body);
    if(ScheduledDateTime.val != EstimatedDateTime.val)
        wrap_date(est_tag, EstimatedDateTime.val, body);
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

    wrap_string(airline_tag, AirlineCode, body);
    wrap_int(flt_tag, FlightNumber, body);
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
    wrap_string(ref_tag, LegReferenceId, body);
    wrap_int(seq_tag, sequence, body);
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

void TBagSummary::to_content(const string &parent, ostringstream &body) const
{
    wrap_begin(parent, body);

    if(BagTagNumber)
        wrap_string(tag_no_tag, BagTagNumber.get().str(), body);
    if(BaggageId != NoExists)
        wrap_int(bag_id_tag, BaggageId, body);
    if(not PNR.empty())
        wrap_string(pnr_tag, PNR, body);

    wrap_end(parent, body);
}

void TBagSummaryList::to_content(const string &parent, ostringstream &body)
{
    if(items.empty())
            wrap_empty(parent, body);
    else {
        wrap_begin(parent, body);
        for(const auto &i: items) i.to_content(i.summary_bags_tag, body);
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
                item.BagTagNumber = i;
                item.PNR = pnr_addr;
                items.push_back(item);
            }
        }
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
    TResHTTPParams rhp;
    rhp.hdrs[HTTP_HDR::CONTENT_TYPE] = "text/xml";
    rhp.toXML(resNode);
}

void CUWSSuccess(xmlNodePtr resNode)
{
    to_content(resNode, "/cuws_success.xml");
}

void TBagWeight::to_content(const string &parent, ostringstream &body)
{
    if(not empty()) {
        wrap_begin(parent, body);
        if(Weight != NoExists)
            wrap_double(Weight_tag, Weight, body);
        if(Unit != TWeightUnit::Unknown)
            wrap_string(Unit_tag, WeightUnits().encode(Unit), body);
        wrap_end(parent, body);
    }
}

void TBagWeight::fromXML(xmlNodePtr bagWeightNode)
{
    xmlNodePtr curNode = bagWeightNode->children;
    Weight = NodeAsFloatFast(Weight_tag.c_str(), curNode, NoExists);
    Unit = WeightUnits().decode(NodeAsStringFast(Unit_tag.c_str(), curNode, ""));
}

void TBagDimension::to_content(const string &parent, ostringstream &body)
{
    if(not empty()) {
        wrap_begin(parent, body);
        if(Length != NoExists)
            wrap_double(Length_tag, Length, body);
        if(Height != NoExists)
            wrap_double(Height_tag, Height, body);
        if(Width != NoExists)
            wrap_double(Width_tag, Width, body);
        if(Unit != TLengthUnit::Unknown)
            wrap_string(Unit_tag, LengthUnits().encode(Unit), body);
        wrap_end(parent, body);
    }
}

void TBagDimension::fromXML(xmlNodePtr dimNode)
{
    xmlNodePtr curNode = dimNode->children;
    Length = NodeAsFloatFast(Length_tag.c_str(), curNode, NoExists);
    Height = NodeAsFloatFast(Height_tag.c_str(), curNode, NoExists);
    Width = NodeAsFloatFast(Width_tag.c_str(), curNode, NoExists);
    Unit = LengthUnits().decode(NodeAsStringFast(Unit_tag.c_str(), curNode, ""));
}

struct TBagAspect {
    const string Aspect_tag                 = "Aspect";
    const string BasicCode_tag              = "BasicCode";
    const string ColorCode_tag              = "ColorCode";
    const string DescriptiveCode_tag        = "DescriptiveCode";
    const string PictureURI_tag             = "PictureURI";
    const string ExternalFeatureCode_tag    = "ExternalFeatureCode";
    const string MaterialCode_tag           = "MaterialCode";

    int BasicCode;
    string ColorCode;
    string DescriptiveCode;
    string PictureURI;
    string ExternalFeatureCode;
    string MaterialCode;
    void clear()
    {
        BasicCode = NoExists;
        ColorCode.clear();
        DescriptiveCode.clear();
        PictureURI.clear();
        ExternalFeatureCode.clear();
        MaterialCode.clear();
    }
    bool empty()
    {
        return
            BasicCode == NoExists and
            ColorCode.empty() and
            DescriptiveCode.empty() and
            PictureURI.empty() and
            ExternalFeatureCode.empty() and
            MaterialCode.empty();
    }
    TBagAspect() { clear(); }
    void fromXML(xmlNodePtr aspectNode);
};

void TBagAspect::fromXML(xmlNodePtr aspectNode)
{
    xmlNodePtr curNode = aspectNode->children;
    BasicCode = NodeAsIntegerFast(BasicCode_tag.c_str(), curNode, NoExists);
    ColorCode = NodeAsStringFast(ColorCode_tag.c_str(), curNode, "");
    DescriptiveCode = NodeAsStringFast(DescriptiveCode_tag.c_str(), curNode, "");
    PictureURI = NodeAsStringFast(PictureURI_tag.c_str(), curNode, "");
    ExternalFeatureCode = NodeAsStringFast(ExternalFeatureCode_tag.c_str(), curNode, "");
    MaterialCode = NodeAsStringFast(MaterialCode_tag.c_str(), curNode, "");
}

struct TBagDetail {
    TBagWeight Weight;
    TBagDimension Dimension;
    TBagAspect Aspect;
    void clear()
    {
        Weight.clear();
        Dimension.clear();
        Aspect.clear();
    }
    bool empty()
    {
        return
            Weight.empty() and
            Dimension.empty() and
            Aspect.empty();
    }
    TBagDetail() { clear(); }
    void fromXML(xmlNodePtr bagDetailNode);
    void toDB(const TPassengerInfo &pax, TBagSummary &Summary);
};

void TBagDetail::toDB(const TPassengerInfo &pax, TBagSummary &Summary)
{
    Summary.clear();
    QParams QryParams;
    QryParams
        << QParam("bag_id", otInteger)
        << QParam("pax_id", otInteger, pax.Individual.PassengerId)
        << QParam("time_issue", otDate, NowUTC())
        << QParam("bag_type", otInteger, Aspect.BasicCode);

    if(Weight.Weight == NoExists)
        QryParams << QParam("weight", otFloat, FNull);
    else
        QryParams << QParam("weight", otFloat, Weight.Weight);

    if(Dimension.Length == NoExists)
        QryParams << QParam("length", otFloat, FNull);
    else
        QryParams << QParam("length", otFloat, Dimension.Length);

    if(Dimension.Height == NoExists)
        QryParams << QParam("height", otFloat, FNull);
    else
        QryParams << QParam("height", otFloat, Dimension.Height);

    if(Dimension.Width == NoExists)
        QryParams << QParam("width", otFloat, FNull);
    else
        QryParams << QParam("width", otFloat, Dimension.Width);

    CheckIn::TSimplePaxItem pax_info;
    pax_info.getByPaxId(pax.Individual.PassengerId);
    TGeneratedTags generated;
    generated.generate(pax_info.grp_id, 1);
    if (generated.tags().size()!=1)
        throw Exception("generated.tags().size()!=1");
    Summary.BagTagNumber = *(generated.tags().begin());
    QryParams << QParam("tag_no", otFloat, Summary.BagTagNumber.get().numeric_part);


    TCachedQuery Qry(
            "begin "
            "   insert into cuws_tags( "
            "      bag_id, "
            "      pax_id, "
            "      time_issue, "
            "      tag_no, "
            "      bag_type, "
            "      weight, "
            "      length, "
            "      height, "
            "      width "
            "   ) values ( "
            "      id__seq.nextval, "
            "      :pax_id, "
            "      :time_issue, "
            "      :tag_no, "
            "      :bag_type, "
            "      :weight, "
            "      :length, "
            "      :height, "
            "      :width "
            "   ) returning bag_id into :bag_id; "
            "end; ",
        QryParams);

    Qry.get().Execute();
    Summary.BaggageId = Qry.get().GetVariableAsInteger("bag_id");
}

void TBagDetail::fromXML(xmlNodePtr bagDetailNode)
{
    xmlNodePtr curNode = bagDetailNode->children;
    Weight.fromXML(NodeAsNodeFast(Weight.Weight_tag.c_str(), curNode));
    xmlNodePtr dimNode = GetNodeFast(Dimension.Dimension_tag.c_str(), curNode);
    if(dimNode)
        Dimension.fromXML(dimNode);
    Aspect.fromXML(NodeAsNodeFast(Aspect.Aspect_tag.c_str(), curNode));
}

void TCostType::to_content(const string &parent, ostringstream &body) const
{
    if(not empty()) {
        wrap_begin(parent, body);
        if(Value != NoExists)
            wrap_double(Value_tag, Value, body);
        if(not CurrencyCode.empty())
            wrap_string(CurrencyCode_tag, CurrencyCode, body);
        wrap_end(parent, body);
    }
}

void TCostList::to_content(const string &parent, ostringstream &body)
{
    for(const auto &i: *this)
        i.to_content(parent, body);
}

void TBagAllowance::to_content(ostringstream &body)
{
    if(empty())
        wrap_empty(BagAllowance_tag, body);
    else {
        wrap_begin(BagAllowance_tag, body);
        if(BasicCode != NoExists)
            wrap_int(BasicCode_tag, BasicCode, body);
        if(AllowedNumber != NoExists)
            wrap_int(AllowedNumber_tag, AllowedNumber, body);
        MaximumWeight.to_content("MaximumWeight", body);
        TotalMaximumWeight.to_content("TotalMaximumWeight", body);
        MaximumDimension.to_content("MaximumDimension", body);
        Price.to_content("Price", body);
        wrap_end(BagAllowance_tag, body);
    }
}

void Get_PassengerInfo_By_BCBP(xmlNodePtr actionNode, xmlNodePtr resNode)
{
    int point_id, reg_no, pax_id;
    bool isBoardingPass;
    boost::optional<TSearchFltInfo> searchFltInfo;
    if(SearchPaxByScanData(NodeAsString("BCBP", actionNode), point_id, reg_no, pax_id, isBoardingPass, searchFltInfo)) {
        if(not isBoardingPass)
            throw Exception("Barcode is not BCBP");
        TPassengerInfo pax;
        pax.fromDB(pax_id);
        ostringstream body;
        pax.to_content("ns:PassengerInfo", body);
        to_envelope(resNode, body.str());
    } else
        throw Exception("Passenger not found");
}

void success(const string &tag, ostringstream &body)
{
    wrap_begin(tag, body);
    wrap_string("Result", "SUCCESS", body);
    wrap_end(tag, body);
}

void Set_Bag_as_Inactive(xmlNodePtr actionNode, xmlNodePtr resNode)
{
    TBagSummary bag;
    bag.fromXML(NodeAsNode("Bag", actionNode));
    bag.set_active(false);
    ostringstream body;
    success("ns:Set_Bag_as_InactiveResponse", body);
    to_envelope(resNode, body.str());
}

void Set_Bag_as_Active(xmlNodePtr actionNode, xmlNodePtr resNode)
{
    TBagSummary bag;
    bag.fromXML(NodeAsNode("Bag", actionNode));
    bag.set_active(true);
    ostringstream body;
    success("ns:Set_Bag_as_ActiveResponse", body);
    to_envelope(resNode, body.str());
}

void Issue_TagNumber(xmlNodePtr actionNode, xmlNodePtr resNode)
{
    TPassengerInfo pax;
    TBagDetail bag_detail;
    pax.fromXML(NodeAsNode("Passenger", actionNode));
    bag_detail.fromXML(NodeAsNode("BagDetail", actionNode));
    TBagSummary Summary;
    bag_detail.toDB(pax, Summary);
    ostringstream body;
    Summary.to_content("ns:BagSummary", body);
    to_envelope(resNode, body.str());
}

} //end namespace CUWS

