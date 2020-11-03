#include <bitset>
#include <unordered_map>
#include <array>

#include <serverlib/str_utils.h>
#include <serverlib/expected.h>
#include <serverlib/dates.h>
#include <serverlib/logopt.h>
#include <nsi/geozones.h>

#include "mct.h"
#include "mct_search.h"

#define NICKNAME "DAG"
#include <serverlib/slogger.h>

namespace ssim { namespace mct {

ENUM_NAMES_BEGIN(Status)
    (Status::Domestic, "D")
    (Status::International, "I")
ENUM_NAMES_END(Status)
ENUM_NAMES_END_AUX(Status)

ENUM_NAMES_BEGIN(AircraftBody)
    (AircraftBody::Any, "A")
    (AircraftBody::Narrow, "N")
    (AircraftBody::Wide, "W")
ENUM_NAMES_END(AircraftBody)
ENUM_NAMES_END_AUX(AircraftBody)

bool isEqual(const SegDesc& x, const SegDesc& y)
{
    return x.status == y.status
        && x.conxStation == y.conxStation
        && x.conxTerminal == y.conxTerminal
        && x.carrier == y.carrier
        && x.cshOprCarrier == y.cshOprCarrier
        && x.cshIndicator == y.cshIndicator
        && x.fltRange == y.fltRange
        && x.aircraftType == y.aircraftType
        && x.aircraftBodyType == y.aircraftBodyType
        && x.oppGeozone == y.oppGeozone
        && x.oppCountry == y.oppCountry
        && x.oppRegion == y.oppRegion
        && x.oppStation == y.oppStation
    ;
}

bool operator== (const SegDesc& x, const SegDesc& y)
{
    return isEqual(x, y);
}

std::ostream& operator<< (std::ostream& os, const SegDesc& sg)
{
    os  << enumToStr(sg.status)
        << "; station: " << LogOpt(sg.conxStation, "none") << '/' << LogOpt(sg.conxTerminal);

    if (sg.carrier || sg.cshOprCarrier || sg.cshIndicator) {
        os  << "; carrier: " << LogOpt(sg.carrier, "none")
            << '/' << LogOpt(sg.cshOprCarrier, "none")
            << '/' << sg.cshIndicator;
    }

    if (sg.fltRange) {
        os << "; range: [" << sg.fltRange->first << "; " << sg.fltRange->second << "]";
    }

    if (sg.aircraftType) {
        os  << "; aircraft: " << *sg.aircraftType;
    } else if (sg.aircraftBodyType != AircraftBody::Any) {
        os  << "; aircraft: " << enumToStr(sg.aircraftBodyType);
    }

    os  << LogOpt("; opposite region: ", sg.oppGeozone)
        << LogOpt("; opposite country: ", sg.oppCountry)
        << LogOpt("; opposite state: ", sg.oppRegion)
        << LogOpt("; opposite station: ", sg.oppStation);

    return os;
}

bool operator== (const Record& x, const Record& y)
{
    return x.arrival == y.arrival
        && x.departure == y.departure
        && x.effectiveFrom == y.effectiveFrom
        && x.effectiveTo == y.effectiveTo
        && x.suppressionIndicator == y.suppressionIndicator
        && x.suppressionGeozone == y.suppressionGeozone
        && x.suppressionCountry == y.suppressionCountry
        && x.suppressionRegion == y.suppressionRegion
        && x.time == y.time
    ;
}

bool isEquivalent(const Record& x, const Record& y)
{
    return x.arrival == y.arrival
        && x.departure == y.departure
        && x.effectiveFrom == y.effectiveFrom
        && x.effectiveTo == y.effectiveTo
        && x.suppressionIndicator == y.suppressionIndicator
        && x.suppressionGeozone == y.suppressionGeozone
        && x.suppressionCountry == y.suppressionCountry
        && x.suppressionRegion == y.suppressionRegion
    ;
}

std::ostream& operator<< (std::ostream& os, const Record& r)
{
    os << "time: " << r.time << std::endl;
    os << "arrival: " << r.arrival << std::endl;
    os << "departure: " << r.departure << std::endl;

    if (r.suppressionIndicator) {
        os << "suppresion: [";
        if (r.suppressionGeozone) {
            os << " region: " << *r.suppressionGeozone;
        }
        if (r.suppressionCountry) {
            os << " country: " << *r.suppressionCountry;
        }
        if (r.suppressionRegion) {
            os << " state: " << *r.suppressionRegion;
        }
        os << "]" << std::endl;
    } else {
        os << "no suppression" << std::endl;
    }
    return os;
}

static Message validateSeg(const Record& r, bool arrival)
{
    const SegDesc& sg = (arrival ? r.arrival : r.departure);
    const std::string tag = (arrival ? "arrival" : "departure");

    if (!r.suppressionIndicator && !sg.conxStation) {
        return Message(STDLOG, _("Invalid " + tag + " station"));
    }

    if (sg.aircraftType && sg.aircraftBodyType != AircraftBody::Any) {
        return Message(STDLOG, _("Invalid " + tag + " aircraft type/body combination"));
    }

    if (1 < ((sg.oppGeozone ? 1 : 0) + (sg.oppCountry ? 1 : 0) + (sg.oppRegion ? 1 : 0) + (sg.oppStation ? 1 : 0))) {
        const std::string t = (arrival ? "previous" : "next");
        return Message(STDLOG, _("Invalid " + t + " station/region/country/state combination"));
    }

    if (sg.fltRange) {
        if (!sg.carrier && !sg.cshOprCarrier) {
            return Message(STDLOG, _("Invalid " + tag + " flight number range"));
        }
        if (sg.fltRange->first <= 0 || sg.fltRange->first > sg.fltRange->second) {
            return Message(STDLOG, _("Invalid " + tag + " flight number range"));
        }
    }

    if (sg.cshOprCarrier && !sg.cshIndicator) {
        return Message(STDLOG, _("Invalid " + tag + " codeshare operating carrier"));
    }

    if (sg.cshIndicator && !(sg.carrier || sg.cshOprCarrier)) {
        return Message(STDLOG, _("Invalid " + tag + " codeshare indicator"));
    }

    return Message();
}

Message validate(const Record& r)
{
    CALL_MSG_RET(validateSeg(r, true));
    CALL_MSG_RET(validateSeg(r, false));

    if (!r.suppressionIndicator) {
        if (r.time.is_special()) {
            return Message(STDLOG, _("Invalid time"));
        }
        if (r.suppressionGeozone) {
            return Message(STDLOG, _("Invalid suppression region"));
        }
        if (r.suppressionCountry) {
            return Message(STDLOG, _("Invalid suppression country"));
        }
        if (r.suppressionRegion) {
            return Message(STDLOG, _("Invalid suppression state"));
        }
    } else {
        if (!r.time.is_pos_infinity()) {
            return Message(STDLOG, _("Invalid time"));
        }
        if (1 < ((r.suppressionGeozone ? 1 : 0) + (r.suppressionCountry ? 1 : 0) + (r.suppressionRegion ? 1 : 0))) {
            return Message(STDLOG, _("Invalid suppression region/country/state combination"));
        }
    }

    return Message();
}

//default values from SSIM 28th 8.9.2
boost::posix_time::time_duration defaultMct(
        nsi::PointId arrPt, Status arrSt,
        nsi::PointId depPt, Status depSt
    )
{
    if (arrPt != depPt) {
        return boost::posix_time::time_duration(4, 0, 0);
    }
    if (arrSt == Status::Domestic && depSt == Status::Domestic) {
        return boost::posix_time::time_duration(0, 20, 0);
    }
    return boost::posix_time::time_duration(1, 0, 0);
}
//#############################################################################
class GeoRelationsImpl
{
    using zones_t = boost::optional< std::set<nsi::GeozoneId> >;
    using country_t = nsi::CountryId;
    using reg_t = boost::optional<nsi::RegionId>;

    using geo_rel = std::tuple< zones_t, country_t, reg_t>;
    using geo_cache = std::map<nsi::PointId, geo_rel>;

    mutable geo_cache data_;

    static const geo_rel& get_geo_rel(const nsi::PointId& pt, geo_cache& gc, bool withZones)
    {
        auto i = gc.find(pt);
        if (i != gc.end()) {
            if (withZones) {
                zones_t& z = std::get<zones_t>(i->second);
                if (!z) {
                    z = nsi::getGeozones(nsi::Point(pt).cityId());
                }
            }
            return i->second;
        }

        const nsi::CityId cid = nsi::Point(pt).cityId();
        return gc.emplace(
            pt,
            geo_rel {
                (withZones ? zones_t(nsi::getGeozones(cid)) : boost::none ),
                nsi::City(cid).countryId(),
                nsi::City(cid).regionId()
            }
        ).first->second;
    }

public:
    bool belongsTo(const nsi::GeozoneId& gid, const nsi::PointId& pt) const
    {
        return std::get<zones_t>(get_geo_rel(pt, data_, true))->count(gid) > 0;
    }

    bool belongsTo(const nsi::CountryId& cid, const nsi::PointId& pt) const
    {
        return std::get<country_t>(get_geo_rel(pt, data_, false)) == cid;
    }

    bool belongsTo(const nsi::RegionId& rid, const nsi::PointId& pt) const
    {
        return std::get<reg_t>(get_geo_rel(pt, data_, false)) == rid;
    }
};

GeoRelations::GeoRelations()
    : impl_(std::make_unique<GeoRelationsImpl>())
{}

GeoRelations::~GeoRelations()
{}

bool GeoRelations::belongsTo(const nsi::GeozoneId& id, const nsi::PointId& pt) const
{
    return impl_->belongsTo(id, pt);
}

bool GeoRelations::belongsTo(const nsi::CountryId& id, const nsi::PointId& pt) const
{
    return impl_->belongsTo(id, pt);
}

bool GeoRelations::belongsTo(const nsi::RegionId& id, const nsi::PointId& pt) const
{
    return impl_->belongsTo(id, pt);
}
//#############################################################################
static bool is_matched(const SegDesc& rsg, const ScdSeg& sg, const GeoRelations& geos, bool& fuzzy)
{
    if (rsg.status != sg.status) {
        return false;
    }
    if (rsg.conxStation && rsg.conxStation != sg.conxStation) {
        return false;
    }

    if (rsg.conxTerminal && sg.conxTerminal && rsg.conxTerminal != sg.conxTerminal) {
        return false;
    }

    fuzzy = (fuzzy || (rsg.conxTerminal && !sg.conxTerminal));
    //-------------------------------------------------------------------------
    if (rsg.carrier || rsg.cshOprCarrier) {
        const bool mismatchedCarrier = (rsg.carrier && rsg.carrier != sg.flt.airline);

        if (rsg.cshIndicator) {
            if (!sg.oprFlt) {
                return false;
            }
            if (mismatchedCarrier) {
                return false;
            }
            if (rsg.cshOprCarrier && rsg.cshOprCarrier != sg.oprFlt->airline) {
                return false;
            }
        } else {
            if (mismatchedCarrier) {
                if (!sg.oprFlt) {
                    return false;
                }
                if (rsg.carrier != sg.oprFlt->airline) {
                    return false;
                }
            } else if (sg.oprFlt) {
                return false;
            }
        }
    }
    //-------------------------------------------------------------------------
    if (rsg.fltRange) {
        const unsigned int fno = (
            rsg.carrier
                ? sg.flt.number.get()
                : ((rsg.cshOprCarrier && sg.oprFlt) ? sg.oprFlt->number.get() : 10000)
        );

        if (fno < rsg.fltRange->first  || fno > rsg.fltRange->second) {
            return false;
        }
    }
    //-------------------------------------------------------------------------
    if (rsg.aircraftType && sg.aircraftType && rsg.aircraftType != sg.aircraftType) {
        return false;
    }

    fuzzy = (fuzzy || (rsg.aircraftType && !sg.aircraftType));

    if (rsg.aircraftBodyType != AircraftBody::Any && rsg.aircraftBodyType != AircraftBody::Any && rsg.aircraftBodyType != sg.aircraftBodyType) {
        return false;
    }

    fuzzy = (fuzzy || (rsg.aircraftBodyType != AircraftBody::Any && sg.aircraftBodyType == AircraftBody::Any));
    //-------------------------------------------------------------------------
    if (rsg.oppStation && rsg.oppStation != sg.oppositeStation) {
        return false;
    }
    if (rsg.oppGeozone && !geos.belongsTo(*rsg.oppGeozone, sg.oppositeStation)) {
        return false;
    }
    if (rsg.oppCountry && !geos.belongsTo(*rsg.oppCountry, sg.oppositeStation)) {
        return false;
    }
    if (rsg.oppRegion && !geos.belongsTo(*rsg.oppRegion, sg.oppositeStation)) {
        return false;
    }
    return true;
}

static bool is_matched(
        const Record& rec, const ScdSeg& arrSeg, const ScdSeg& depSeg,
        const boost::gregorian::date& dt, const GeoRelations& geos, bool& fuzzy
    )
{
    fuzzy = false;

    if (!is_matched(rec.arrival, arrSeg, geos, fuzzy)) {
        return false;
    }
    if (!is_matched(rec.departure, depSeg, geos, fuzzy)) {
        return false;
    }
    if (rec.effectiveFrom > dt || rec.effectiveTo < dt) {
        return false;
    }

    if (rec.suppressionIndicator) {
        if (rec.suppressionGeozone) {
            if (!geos.belongsTo(*rec.suppressionGeozone, arrSeg.conxStation)) {
                if (!geos.belongsTo(*rec.suppressionGeozone, depSeg.conxStation)) {
                    return false;
                }
            }
        }

        if (rec.suppressionCountry) {
            if (!geos.belongsTo(*rec.suppressionCountry, arrSeg.conxStation)) {
                if (!geos.belongsTo(*rec.suppressionCountry, depSeg.conxStation)) {
                    return false;
                }
            }
        }

        if (rec.suppressionRegion) {
            if (!geos.belongsTo(*rec.suppressionRegion, arrSeg.conxStation)) {
                if (!geos.belongsTo(*rec.suppressionRegion, depSeg.conxStation)) {
                    return false;
                }
            }
        }
    }

    return true;
}

bool isMatched(
        const Record& rec, const ScdSeg& arrSeg, const ScdSeg& depSeg,
        const boost::gregorian::date& dt, const GeoRelations& geos
    )
{
    bool fuzzy = false;
    return is_matched(rec, arrSeg, depSeg, dt, geos, fuzzy);
}
//#############################################################################
//reversed against priority: less weight - higher priority
enum DataElementWeight
{
    SuppressionRegion = 0,
    SuppressionCountry,
    SuppressonState,
    ArrivalStation,
    DepartureStation,
    EffectiveToDate,
    EffectiveFromDate,
    ArrivalAircraftBody,
    DepartureAircraftBody,
    ArrivalAircraftType,
    DepartureAircraftType,
    PreviousRegion,
    NextRegion,
    PreviousCountry,
    NextCountry,
    PreviousState,
    NextState,
    PreviousStation,
    NextStation,
    ArrivalTerminal,
    DepartureTerminal,
    ArrivalFlightNumberRange,
    DepartureFlightNumberRange,
    ArrivalCodeshareOperatingCarrier,
    ArrivalCarrier,
    ArrivalCodeshareIndicator,
    DepartureCodeshareOperatingCarrier,
    DepartureCarrier,
    DepartureCodeshareIndicator,

    WeightedDataElemenCount
};

using record_weight = std::bitset<WeightedDataElemenCount>;

static void setSegWeight(record_weight& wt, const SegDesc& sg, bool arrival)
{
    if (sg.conxStation) {
        wt.set(arrival ? ArrivalStation : DepartureStation);
    }
    if (sg.conxTerminal) {
        wt.set(arrival ? ArrivalTerminal : DepartureTerminal);
    }

    if (sg.aircraftType) {
        wt.set(arrival ? ArrivalAircraftType : DepartureAircraftType);
    } else if (sg.aircraftBodyType != AircraftBody::Any) {
        wt.set(arrival ? ArrivalAircraftBody : DepartureAircraftBody);
    }

    if (sg.oppGeozone) {
        wt.set(arrival ? PreviousRegion : NextRegion);
    } else if (sg.oppCountry) {
        wt.set(arrival ? PreviousCountry : NextCountry);
    } else if (sg.oppRegion) {
        wt.set(arrival ? PreviousState : NextState);
    } else if (sg.oppStation) {
        wt.set(arrival ? PreviousStation : NextStation);
    }

    if (sg.carrier) {
        wt.set(arrival ? ArrivalCarrier : DepartureCarrier);
    }
    if (sg.cshIndicator) {
        wt.set(arrival ? ArrivalCodeshareIndicator : DepartureCodeshareIndicator);
        if (sg.cshOprCarrier) {
            wt.set(arrival ? ArrivalCodeshareOperatingCarrier : DepartureCodeshareOperatingCarrier);
        }
    }
    if (sg.fltRange) {
        wt.set(arrival ? ArrivalFlightNumberRange : DepartureFlightNumberRange);
    }
}

static unsigned int getWeight(const Record& r)
{
    record_weight wt;

    if (r.suppressionGeozone) {
        wt.set(SuppressionRegion);
    }
    if (r.suppressionCountry) {
        wt.set(SuppressionCountry);
    }
    if (r.suppressionRegion) {
        wt.set(SuppressonState);
    }
    if (!r.effectiveTo.is_infinity()) {
        wt.set(EffectiveToDate);
    }
    if (!r.effectiveFrom.is_infinity()) {
        wt.set(EffectiveFromDate);
    }

    setSegWeight(wt, r.arrival, true);
    setSegWeight(wt, r.departure, false);

    return wt.to_ulong();
}

static bool isSubset(const FlightRange& x, const FlightRange& y)
{
    return y.first >= x.first
        && y.second <= x.second
        && (y.first != x.first || y.second != x.second);
}

static bool intersects(const FlightRange& x, const FlightRange& y)
{
    return x.first <= y.second && x.second >= y.first;
}

static nsi::GeozoneId getSpecialRegion(const std::string& code)
{
    const EncString c = EncString::fromDb(code);
    if (!nsi::Geozone::find(c)) {
        LogError(STDLOG) << "Missing NSI entry for region " << code;
        return nsi::GeozoneId(0);
    }
    return nsi::Geozone(c).id();
}

static nsi::GeozoneId schRegion()
{
    static const nsi::GeozoneId id = getSpecialRegion("SCH");
    return id;
}

static nsi::GeozoneId eurRegion()
{
    static const nsi::GeozoneId id = getSpecialRegion("EUR");
    return id;
}

static bool less(const boost::optional<nsi::GeozoneId>& x, const boost::optional<nsi::GeozoneId>& y)
{
    //to handle special case: SCH takes precedence over EUR

    if (!x || !y) {
        return x < y;
    }
    if (*x == schRegion() && *y == eurRegion()) {
        return true;
    }
    if (*x == eurRegion() && *y == schRegion()) {
        return false;
    }
    return x < y;
}

static bool less(const boost::optional<FlightRange>& x, const boost::optional<FlightRange>& y)
{
    if (x && y) {
        if (isSubset(*y, *x)) {
            return true;
        }
        if (isSubset(*x, *y)) {
            return false;
        }
    }
    return x < y;
}

static bool less(const SegDesc& x, const SegDesc& y)
{
    if (x.status != y.status) {
        return x.status < y.status;
    }
    if (x.conxStation != y.conxStation) {
        return x.conxStation < y.conxStation;
    }
    if (x.conxTerminal != y.conxTerminal) {
        return x.conxTerminal < y.conxTerminal;
    }
    if (x.carrier != y.carrier) {
        return x.carrier < y.carrier;
    }
    if (x.cshOprCarrier != y.cshOprCarrier) {
        return x.cshOprCarrier < y.cshOprCarrier;
    }
    if (x.fltRange != y.fltRange) {
        return less(x.fltRange, y.fltRange);
    }
    if (x.aircraftType != y.aircraftType) {
        return x.aircraftType < y.aircraftType;
    }
    if (x.aircraftBodyType != y.aircraftBodyType) {
        return x.aircraftBodyType > y.aircraftBodyType;
    }
    if (x.oppGeozone != y.oppGeozone) {
        return less(x.oppGeozone, y.oppGeozone);
    }
    if (x.oppCountry != y.oppCountry) {
        return x.oppCountry < y.oppCountry;
    }
    if (x.oppRegion != y.oppRegion) {
        return x.oppRegion < y.oppRegion;
    }
    return x.oppStation < y.oppStation;
}

bool hasPriority(const Record& x, const Record& y)
{
    const auto x_wt = getWeight(x);
    const auto y_wt = getWeight(y);

    //priority by specified fields
    if (x_wt != y_wt) {
        return x_wt > y_wt;
    }

    if (!isEqual(x.departure, y.departure)) {
        return less(x.departure, y.departure);
    }

    if (!isEqual(x.arrival, y.arrival)) {
        return less(x.arrival, y.arrival);
    }

    if (x.effectiveFrom != y.effectiveFrom) {
        return x.effectiveFrom > y.effectiveFrom;
    }

    if (x.effectiveTo != y.effectiveTo) {
        return x.effectiveTo < y.effectiveTo;
    }

    if (x.suppressionIndicator != y.suppressionIndicator) {
        return x.suppressionIndicator;
    }

    if (!x.suppressionIndicator) {
        return false;
    }

    if (x.suppressionGeozone != y.suppressionGeozone) {
        return less(x.suppressionGeozone, y.suppressionGeozone);
    }

    if (x.suppressionCountry != y.suppressionCountry) {
        return x.suppressionCountry < y.suppressionCountry;
    }

    return x.suppressionRegion < y.suppressionRegion;
}
//#############################################################################
static bool duplicated(const boost::optional<FlightRange>& x, const boost::optional<FlightRange>& y)
{
    if (x && y) {
        if (!intersects(*x, *y)) {
            return false;
        }
        if (isSubset(*y, *x)) {
            return false;
        }
        if (isSubset(*x, *y)) {
            return false;
        }
        return true;
    }
    return static_cast<bool>(x) == static_cast<bool>(y);
}

static bool duplicated(const SegDesc& x, const SegDesc& y)
{
    return x.status == y.status
        && x.conxStation == y.conxStation
        && x.conxTerminal == y.conxTerminal
        && x.carrier == y.carrier
        && x.cshIndicator == y.cshIndicator
        && x.cshOprCarrier == y.cshOprCarrier
        && duplicated(x.fltRange, y.fltRange)
        && x.aircraftType == y.aircraftType
        && x.aircraftBodyType == y.aircraftBodyType
        && x.oppGeozone == y.oppGeozone
        && x.oppCountry == y.oppCountry
        && x.oppRegion == y.oppRegion
        && x.oppStation == y.oppStation
    ;
}

bool isDuplicated(const Record& x, const Record& y)
{
    if (x.arrival.status != y.arrival.status) {
        return false;
    }
    if (x.departure.status != y.departure.status) {
        return false;
    }
    if (getWeight(x) != getWeight(y)) {
        return false;
    }
    if (!duplicated(x.arrival, y.arrival)) {
        return false;
    }
    if (!duplicated(x.departure, y.departure)) {
        return false;
    }

    if (!x.effectiveFrom.is_infinity() || !x.effectiveTo.is_infinity()) {
        if (!(x.effectiveFrom <= y.effectiveTo && x.effectiveTo >= y.effectiveFrom)) {
            return false;
        }
    }

    return x.suppressionIndicator == y.suppressionIndicator
        && x.suppressionGeozone == y.suppressionGeozone
        && x.suppressionCountry == y.suppressionCountry
        && x.suppressionRegion == y.suppressionRegion
    ;
}
//#############################################################################
struct sub_key
{
    unsigned int arr_conx_point;
    unsigned int arr_airline;
    unsigned int dep_airline;
};

static bool operator== (const sub_key& x, const sub_key& y)
{
    return x.arr_conx_point == y.arr_conx_point
        && x.arr_airline == y.arr_airline
        && x.dep_airline == y.dep_airline
    ;
}

class sub_key_hasher
{
    mutable std::vector<unsigned int> pts_;

    unsigned int normalized_point(unsigned int pt) const
    {
        if (pt == 0) {
            return pt;
        }
        for (unsigned int i = 0; i < pts_.size(); ++i) {
            if (pts_[i] == pt) {
                return i + 1;
            }
        }
        pts_.emplace_back(pt);
        return pts_.size();
    }

public:
    std::size_t operator() (const sub_key& k) const
    {
        return
            (normalized_point(k.arr_conx_point) << 28)
            | ((k.arr_airline % 0x3FFF) << 14) | (k.dep_airline % 0x3FFF)
        ;
    }
};

static size_t make_type_key(Status arr, Status dep)
{
    return ((arr == Status::International) ? 2 : 0)
        +  ((dep == Status::International) ? 1 : 0);
}

static size_t make_type_key(const Record& r)
{
    return make_type_key(r.arrival.status, r.departure.status);
}

static size_t make_type_key(const ScdSeg& arrSg, const ScdSeg& depSg)
{
    return make_type_key(arrSg.status, depSg.status);
}

static sub_key make_sub_key(int arr_conx_point, int arr_airline, int dep_airline)
{
    return sub_key {
        static_cast<unsigned int>(arr_conx_point),
        static_cast<unsigned int>(arr_airline),
        static_cast<unsigned int>(dep_airline),
    };
}

static int carrier_key(const SegDesc& sd)
{
    if (sd.carrier) {
        return sd.carrier->get();
    }
    if (sd.cshOprCarrier) {
        return sd.cshOprCarrier->get();
    }
    return 0;
}

static sub_key make_sub_key(const Record& r)
{
    return make_sub_key(
        r.arrival.conxStation.get_value_or(nsi::PointId(0)).get(),
        carrier_key(r.arrival),
        carrier_key(r.departure)
    );
}

static std::vector<sub_key> make_sub_keys(const ScdSeg& arrSg, const ScdSeg& depSg)
{
    std::vector<sub_key> out;
    for (unsigned int conxPt : { arrSg.conxStation.get(), 0 }) {
        out.emplace_back(make_sub_key(conxPt, arrSg.flt.airline.get(), depSg.flt.airline.get()));
        out.emplace_back(make_sub_key(conxPt, arrSg.flt.airline.get(), 0));
        out.emplace_back(make_sub_key(conxPt, 0, depSg.flt.airline.get()));
        out.emplace_back(make_sub_key(conxPt, 0, 0));

        if (arrSg.oprFlt) {
            out.emplace_back(make_sub_key(conxPt, arrSg.oprFlt->airline.get(), depSg.flt.airline.get()));
            out.emplace_back(make_sub_key(conxPt, arrSg.oprFlt->airline.get(), 0));
        }

        if (depSg.oprFlt) {
            out.emplace_back(make_sub_key(conxPt, arrSg.flt.airline.get(), depSg.oprFlt->airline.get()));
            out.emplace_back(make_sub_key(conxPt, 0, depSg.oprFlt->airline.get()));
        }

        if (depSg.oprFlt && arrSg.oprFlt) {
            out.emplace_back(make_sub_key(conxPt, arrSg.oprFlt->airline.get(), depSg.oprFlt->airline.get()));
        }
    }
    return out;
}
//#############################################################################
class SeekerImpl
{
    using rec_ref = const Record*;
    using rec_refs = std::vector<rec_ref>;
    using sub_data = std::unordered_map<sub_key, rec_refs, sub_key_hasher>;

    Seeker::FuzzyMatchPolicy fmp_;
    std::vector<Record> data_;
    std::array<sub_data, 4> refData_;

public:
    SeekerImpl(const SeekerImpl&) = delete;
    SeekerImpl(SeekerImpl&&) = delete;
    SeekerImpl& operator= (const SeekerImpl&) = delete;
    SeekerImpl& operator= (SeekerImpl&&) = delete;

    explicit SeekerImpl(const std::vector<Record>& rs, Seeker::FuzzyMatchPolicy fmp)
        : fmp_(fmp), data_(rs)
    {
        std::sort(data_.begin(), data_.end(), hasPriority);

        for (const Record& r : data_) {
            refData_[make_type_key(r)][make_sub_key(r)].emplace_back(&r);
        }
    }

    rec_ref search(const ScdSeg& arrSg, const ScdSeg& depSg, const boost::gregorian::date& dt) const
    {
        const sub_data& sd = refData_[make_type_key(arrSg, depSg)];

        std::pair<size_t, rec_ref> found { 0, nullptr };

        rec_refs fuzzyRecs;

        GeoRelations geos;
        for (const sub_key& sk : make_sub_keys(arrSg, depSg)) {
            auto ski = sd.find(sk);
            if (ski == sd.end()) {
                continue;
            }

            for (rec_ref r : ski->second) {
                bool fuzzy = false;
                if (is_matched(*r, arrSg, depSg, dt, geos, fuzzy)) {
                    if (fuzzy) {
                        fuzzyRecs.emplace_back(r);
                        continue;
                    }

                    const size_t pos = (r - data_.data());
                    if (!found.second || (found.second && found.first > pos)) {
                        found.first = pos;
                        found.second = r;
                    }
                    break;
                }
            }
        }

        if (fuzzyRecs.empty()) {
            if (found.second) {
                LogTrace(TRACE5) << "found MCT: " << *found.second;
            }
            return found.second;
        }

        rec_ref out = found.second;
        for (rec_ref r : fuzzyRecs) {
            if (!found.second || (found.second && found.first > static_cast<size_t>(r - data_.data()))) {
                if (!out) {
                    out = r;
                } else if (fmp_ == Seeker::Minimum) {
                    if (out->time > r->time) {
                        out = r;
                    }
                } else {
                    if (out->time < r->time) {
                        out = r;
                    }
                }
            }
        }

        if (out) {
            LogTrace(TRACE5) << "found MCT (fuzzy, "
                << (fmp_ == Seeker::Minimum ? "min" : "max") << "-mode): "
                << *out;
        }

        return out;
    }
};
//#############################################################################

Seeker::Seeker(const std::vector<Record>& rs, FuzzyMatchPolicy fmp)
    : impl_(std::make_unique<SeekerImpl>(rs, fmp))
{}

Seeker::~Seeker()
{}

const Record* Seeker::search(const ScdSeg& arrSg, const ScdSeg& depSg, const boost::gregorian::date& dt) const
{
    return impl_->search(arrSg, depSg, dt);
}

} } //ssim::mct

//#############################################################################
#ifdef XP_TESTING

#include <serverlib/checkunit.h>
#include <libnsi/callbacks.h>

#define NA boost::none
#define FLT(airline, fno) ct::Flight(airline, ct::FlightNum(fno))
#define TRM(t) nsi::TermId(t)

namespace {

static std::vector<ssim::mct::Record> testRecords()
{
    using namespace ssim::mct;

    using st = ssim::mct::Status;
    using ab = ssim::mct::AircraftBody;
    using bpm = boost::posix_time::minutes;

    const boost::gregorian::date pinf(boost::gregorian::pos_infin);
    const boost::gregorian::date ninf(boost::gregorian::neg_infin);
    const boost::posix_time::time_duration tdinf(boost::posix_time::pos_infin);

    const nsi::PointId svo = nsi::Point(EncString::fromDb("SVO")).id();

    const nsi::CompanyId ut = nsi::Company(EncString::fromDb("UT")).id();
    const nsi::CompanyId su = nsi::Company(EncString::fromDb("SU")).id();
    const nsi::CompanyId lh = nsi::Company(EncString::fromDb("LH")).id();
    const nsi::CompanyId u6 = nsi::Company(EncString::fromDb("U6")).id();

    const nsi::GeozoneId eur = nsi::Geozone(EncString::fromDb("EUR")).id();
    const nsi::GeozoneId sch = nsi::Geozone(EncString::fromDb("SCH")).id();
    const nsi::GeozoneId sea = nsi::Geozone(EncString::fromDb("SEA")).id();

    const nsi::AircraftTypeId a330 = nsi::AircraftType(EncString::fromDb("330")).id();

    const std::vector<ssim::mct::Record> rcs = {
        {
            { st::Domestic, svo, NA, ut, NA, false, NA, NA, ab::Any },
            { st::Domestic, svo, NA, NA, NA, false, NA, NA, ab::Any },
            ninf, pinf, false, NA, NA, NA, bpm(30)
        },
        {
            { st::Domestic, svo, NA, NA, NA, false, NA, NA, ab::Any },
            { st::Domestic, svo, NA, ut, NA, false, NA, NA, ab::Any },
            ninf, pinf, false, NA, NA, NA, bpm(31)
        },
        {
            { st::Domestic, svo, NA, ut, NA, true, NA, NA, ab::Any },
            { st::Domestic, svo, NA, ut, NA, false, NA, NA, ab::Any },
            ninf, pinf, false, NA, NA, NA, bpm(32)
        },
        {
            { st::Domestic, svo, NA, ut, su, true, NA, NA, ab::Any },
            { st::Domestic, svo, NA, ut, NA, false, NA, NA, ab::Any },
            ninf, pinf, false, NA, NA, NA, bpm(33)
        },
        {
            { st::Domestic, svo, NA, NA, u6, true, NA, NA, ab::Any },
            { st::Domestic, svo, NA, ut, NA, false, NA, NA, ab::Any },
            ninf, pinf, false, NA, NA, NA, bpm(34)
        },
        {
            { st::Domestic, svo, NA, NA, u6, true, FlightRange(50, 60), NA, ab::Any },
            { st::Domestic, svo, NA, ut, NA, false, NA, NA, ab::Any },
            ninf, pinf, false, NA, NA, NA, bpm(35)
        },
        {
            { st::Domestic, svo, NA, NA, u6, true, FlightRange(50, 70), NA, ab::Any },
            { st::Domestic, svo, NA, ut, NA, false, NA, NA, ab::Any },
            ninf, pinf, false, NA, NA, NA, bpm(36)
        },
        {
            { st::Domestic, svo, NA, lh, NA, false, FlightRange(1000, 2000), NA, ab::Any },
            { st::Domestic, svo, NA, su, NA, false, FlightRange(3000, 4000), NA, ab::Any },
            ninf, pinf, false, NA, NA, NA, bpm(37)
        },
        {
            { st::Domestic, svo, NA, lh, NA, false, FlightRange(1000, 2000), NA, ab::Any },
            { st::Domestic, svo, NA, su, NA, false, FlightRange(3500, 4000), NA, ab::Any },
            ninf, pinf, false, NA, NA, NA, bpm(38)
        },
        {
            { st::Domestic, svo, NA, lh, NA, false, FlightRange(1500, 2000), NA, ab::Any },
            { st::Domestic, svo, NA, su, NA, false, FlightRange(3000, 4000), NA, ab::Any },
            ninf, pinf, false, NA, NA, NA, bpm(39)
        },
        {
            { st::Domestic, svo, NA, lh, NA, false, FlightRange(1600, 2000), NA, ab::Any },
            { st::Domestic, svo, NA, su, NA, false, FlightRange(3600, 4000), NA, ab::Any },
            ninf, pinf, false, NA, NA, NA, bpm(40)
        },
        {
            { st::Domestic, svo, NA, NA, NA, false, NA, NA, ab::Any, sea },
            { st::International, svo, NA, NA, NA, false, NA, NA, ab::Any, eur },
            ninf, pinf, false, NA, NA, NA, bpm(41)
        },
        {
            { st::Domestic, svo, NA, NA, NA, false, NA, NA, ab::Any, sea },
            { st::Domestic, svo, NA, NA, NA, false, NA, NA, ab::Any, eur },
            ninf, pinf, false, NA, NA, NA, bpm(45)
        },
        {
            { st::Domestic, svo, NA, NA, NA, false, NA, NA, ab::Any, eur },
            { st::International, svo, NA, NA, NA, false, NA, NA, ab::Any, eur },
            ninf, pinf, false, NA, NA, NA, bpm(42)
        },
        {
            { st::Domestic, svo, NA, NA, NA, false, NA, NA, ab::Any, eur },
            { st::Domestic, svo, NA, NA, NA, false, NA, NA, ab::Any, eur },
            ninf, pinf, false, NA, NA, NA, bpm(46)
        },
        {
            { st::Domestic, svo, NA, NA, NA, false, NA, NA, ab::Any, eur },
            { st::International, svo, NA, NA, NA, false, NA, NA, ab::Any, sch },
            ninf, pinf, false, NA, NA, NA, bpm(43)
        },
        {
            { st::Domestic, svo, NA, NA, NA, false, NA, NA, ab::Any, eur },
            { st::Domestic, svo, NA, NA, NA, false, NA, NA, ab::Any, sch },
            ninf, pinf, false, NA, NA, NA, bpm(47)
        },
        {
            { st::Domestic, svo, NA, NA, NA, false, NA, NA, ab::Any },
            { st::Domestic, svo, NA, NA, NA, false, NA, NA, ab::Any },
            ninf, pinf, false, NA, NA, NA, bpm(44)
        },
        {
            { st::Domestic, NA, NA, u6, NA, false, NA, NA, ab::Any },
            { st::Domestic, NA, NA, NA, NA, false, NA, NA, ab::Any },
            ninf, pinf, true, NA, NA, NA, tdinf
        },
        {
            { st::Domestic, NA, NA, NA, NA, false, NA, NA, ab::Any },
            { st::Domestic, NA, NA, u6, NA, true, NA, NA, ab::Any },
            ninf, pinf, true, NA, NA, NA, tdinf
        },
        {
            { st::Domestic, svo, NA, ut, NA, false, NA, NA, ab::Any },
            { st::Domestic, svo, NA, NA, NA, false, NA, NA, ab::Any },
            Dates::currentDate() + boost::gregorian::days(15), pinf, false, NA, NA, NA, bpm(48)
        },
        {
            { st::Domestic, svo, NA, ut, NA, false, NA, NA, ab::Any },
            { st::Domestic, svo, NA, NA, NA, false, NA, NA, ab::Any },
            Dates::currentDate() + boost::gregorian::days(15), Dates::currentDate() + boost::gregorian::days(50), false, NA, NA, NA, bpm(56)
        },
        {
            { st::Domestic, svo, NA, NA, NA, false, NA, NA, ab::Any },
            { st::Domestic, svo, NA, ut, NA, false, NA, NA, ab::Any },
            Dates::currentDate() + boost::gregorian::days(15), pinf, false, NA, NA, NA, bpm(49)
        },
        {
            { st::Domestic, svo, NA, NA, NA, false, NA, NA, ab::Any },
            { st::Domestic, svo, TRM("D"), NA, NA, false, NA, NA, ab::Any },
            ninf, pinf, false, NA, NA, NA, bpm(50)
        },
        {
            { st::Domestic, svo, NA, NA, NA, false, NA, NA, ab::Any },
            { st::Domestic, svo, TRM("D"), NA, NA, false, NA, NA, ab::Wide },
            ninf, pinf, false, NA, NA, NA, bpm(51)
        },
        {
            { st::Domestic, svo, NA, NA, NA, false, NA, NA, ab::Any },
            { st::Domestic, svo, TRM("D"), NA, NA, false, NA, a330, ab::Any },
            ninf, pinf, false, NA, NA, NA, bpm(52)
        },
        {
            { st::Domestic, svo, NA, NA, NA, false, NA, NA, ab::Any },
            { st::Domestic, svo, TRM("D"), NA, NA, false, NA, a330, ab::Any },
            ninf, pinf, false, NA, NA, NA, bpm(53)
        },
        {
            { st::Domestic, svo, NA, NA, NA, false, NA, NA, ab::Any, NA, nsi::Country(EncString::fromUtf("RU")).id() },
            { st::Domestic, svo, TRM("D"), NA, NA, false, NA, a330, ab::Any },
            ninf, pinf, false, NA, NA, NA, bpm(54)
        },
        {
            { st::Domestic, svo, NA, NA, NA, false, NA, NA, ab::Any, NA, NA, NA, nsi::Point(EncString::fromUtf("LED")).id() },
            { st::Domestic, svo, TRM("D"), NA, NA, false, NA, a330, ab::Any },
            ninf, pinf, false, NA, NA, NA, bpm(55)
        },
    };
    //-------------------------------------------------------------------------
    {
        std::set<boost::posix_time::time_duration> tms;
        for (const auto& r : rcs) {
            ASSERT(!validate(r));
            if (!r.time.is_special()) {
                ASSERT(tms.emplace(r.time).second);
            }
        }
    }

    return rcs;
}

} //anonymous

START_TEST(search_matching)
{
    using namespace ssim::mct;

    using st = ssim::mct::Status;
    using ab = ssim::mct::AircraftBody;
    using bpm = boost::posix_time::minutes;

    const boost::posix_time::time_duration tdinf(boost::posix_time::pos_infin);

    const boost::gregorian::date someDt = Dates::currentDate() + boost::gregorian::days(10);

    const nsi::PointId svo = nsi::Point(EncString::fromDb("SVO")).id();
    const nsi::PointId dme = nsi::Point(EncString::fromDb("DME")).id();
    const nsi::PointId svx = nsi::Point(EncString::fromDb("SVX")).id();
    const nsi::PointId led = nsi::Point(EncString::fromDb("LED")).id();
    const nsi::PointId muc = nsi::Point(EncString::fromDb("MUC")).id();
    const nsi::PointId rov = nsi::Point(EncString::fromDb("ROV")).id();

    const nsi::CompanyId ut = nsi::Company(EncString::fromDb("UT")).id();
    const nsi::CompanyId su = nsi::Company(EncString::fromDb("SU")).id();
    const nsi::CompanyId lh = nsi::Company(EncString::fromDb("LH")).id();
    const nsi::CompanyId u6 = nsi::Company(EncString::fromDb("U6")).id();

    const nsi::AircraftTypeId a330 = nsi::AircraftType(EncString::fromDb("330")).id();
    const nsi::AircraftTypeId a320 = nsi::AircraftType(EncString::fromDb("320")).id();
    //-------------------------------------------------------------------------
    struct {
        ScdSeg arrSg;
        ScdSeg depSg;
        boost::gregorian::date dt;
        boost::optional<boost::posix_time::time_duration> tm;
        boost::optional<unsigned int> wt;
    } cases[] = {
        //departure priority
        {
            { st::Domestic, FLT(ut, 10), svo, svx, TRM("F"), a330, ab::Wide },
            { st::Domestic, FLT(ut, 20), svo, led, TRM("F"), a330, ab::Wide },
            someDt, bpm(31)
        },
        {
            { st::Domestic, FLT(ut, 10), svo, svx, TRM("F"), a330, ab::Wide },
            { st::Domestic, FLT(ut, 20), svo, led, TRM("F"), a330, ab::Wide },
            someDt + boost::gregorian::days(10), bpm(49)
        },
        //flight subsets
        {
            { st::Domestic, FLT(lh, 10), svo, svx, TRM("F"), a330, ab::Wide },
            { st::Domestic, FLT(su, 20), svo, led, TRM("F"), a330, ab::Wide },
            someDt, bpm(45)
        },
        {
            { st::Domestic, FLT(lh, 1501), svo, svx, TRM("F"), a330, ab::Wide },
            { st::Domestic, FLT(su, 3501), svo, led, TRM("F"), a330, ab::Wide },
            someDt, bpm(38)
        },
        {
            { st::Domestic, FLT(lh, 1601), svo, svx, TRM("F"), a330, ab::Wide },
            { st::Domestic, FLT(su, 3601), svo, led, TRM("F"), a330, ab::Wide },
            someDt, bpm(40)
        },
        {
            { st::Domestic, FLT(lh, 1601), svo, svx, TRM("F"), a330, ab::Wide },
            { st::Domestic, FLT(su, 3401), svo, led, TRM("F"), a330, ab::Wide },
            someDt, bpm(39)
        },
        //regions and relative priority
        {
            { st::Domestic, FLT(lh, 10), svo, svx, TRM("F"), a330, ab::Wide },
            { st::International, FLT(u6, 20), svo, muc, TRM("F"), a330, ab::Wide },
            someDt, bpm(41)
        },
        {
            { st::Domestic, FLT(lh, 10), svo, rov, TRM("F"), a330, ab::Wide },
            { st::International, FLT(u6, 20), svo, muc, TRM("F"), a330, ab::Wide },
            someDt, bpm(43)
        },
        {
            { st::Domestic, FLT(lh, 10), svo, rov, TRM("F"), a330, ab::Wide },
            { st::Domestic, FLT(u6, 20), svo, led, TRM("F"), a330, ab::Wide },
            someDt, bpm(46)
        },
        {
            { st::Domestic, FLT(lh, 10), svo, svx, TRM("F"), a330, ab::Wide },
            { st::Domestic, FLT(u6, 20), svo, led, TRM("F"), a330, ab::Wide },
            someDt, bpm(45)
        },
        //codeshare indicator matching
        {
            { st::Domestic, FLT(ut, 10), svo, svx, TRM("F"), a330, ab::Wide, FLT(su, 10) },
            { st::Domestic, FLT(ut, 20), svo, led, TRM("F"), a330, ab::Wide },
            someDt, bpm(33)
        },
        {
            { st::Domestic, FLT(ut, 10), svo, svx, TRM("F"), a330, ab::Wide, FLT(lh, 10) },
            { st::Domestic, FLT(ut, 20), svo, led, TRM("F"), a330, ab::Wide },
            someDt, bpm(32)
        },
        {
            { st::Domestic, FLT(su, 10), svo, svx, TRM("F"), a330, ab::Wide, FLT(u6, 10) },
            { st::Domestic, FLT(ut, 20), svo, led, TRM("F"), a330, ab::Wide },
            someDt, bpm(34)
        },
        //flight range applies to operating carrier
        {
            { st::Domestic, FLT(su, 10), svo, svx, TRM("F"), a330, ab::Wide, FLT(u6, 50) },
            { st::Domestic, FLT(ut, 20), svo, led, TRM("F"), a330, ab::Wide },
            someDt, bpm(35)
        },
        //no mct for UT as marketing at depature segment
        {
            { st::Domestic, FLT(ut, 10), svo, svx, TRM("F"), a330, ab::Wide },
            { st::Domestic, FLT(ut, 20), svo, led, TRM("F"), a330, ab::Wide, FLT(su, 20) },
            someDt, bpm(30)
        },
        //defaulting to operating carrier mct
        {
            { st::Domestic, FLT(ut, 10), svo, svx, TRM("F"), a330, ab::Wide },
            { st::Domestic, FLT(su, 20), svo, led, TRM("F"), a330, ab::Wide, FLT(ut, 20) },
            someDt, bpm(31)
        },
        //suppression
        {
            { st::Domestic, FLT(u6, 10), svo, rov, TRM("F"), a330, ab::Wide },
            { st::Domestic, FLT(su, 20), svo, led, TRM("F"), a330, ab::Wide },
            someDt, tdinf,
            record_weight().set(ArrivalCarrier).to_ulong()
        },
        {
            { st::Domestic, FLT(u6, 10), svo, rov, TRM("F"), a330, ab::Wide, FLT(lh, 10) },
            { st::Domestic, FLT(su, 20), svo, led, TRM("F"), a330, ab::Wide },
            someDt, bpm(46)
        },
        {
            { st::Domestic, FLT(su, 10), svo, rov, TRM("F"), a330, ab::Wide },
            { st::Domestic, FLT(u6, 20), svo, svx, TRM("F"), a330, ab::Wide, FLT(lh, 20) },
            someDt, tdinf,
            record_weight().set(DepartureCarrier).set(DepartureCodeshareIndicator).to_ulong()
        },
        {
            { st::Domestic, FLT(su, 10), svo, rov, TRM("F"), a330, ab::Wide },
            { st::Domestic, FLT(u6, 20), svo, svx, TRM("F"), a330, ab::Wide },
            someDt, bpm(44)
        },
        //misc
        {
            { st::Domestic, FLT(su, 10), svo, rov, TRM("F"), a330, ab::Wide },
            { st::Domestic, FLT(u6, 20), svo, svx, TRM("D"), a320, ab::Narrow },
            someDt, bpm(50)
        },
        {
            { st::Domestic, FLT(su, 10), svo, rov, TRM("F"), a330, ab::Wide },
            { st::Domestic, FLT(u6, 20), dme, svx, TRM("1"), a320, ab::Narrow },
            someDt, NA
        },
        {
            { st::Domestic, FLT(su, 10), svo, rov, TRM("F"), a320, ab::Narrow },
            { st::Domestic, FLT(u6, 20), svo, svx, TRM("D"), a320, ab::Wide },
            someDt, bpm(51)
        },
        {
            { st::Domestic, FLT(su, 10), svo, rov, TRM("F"), a320, ab::Narrow },
            { st::Domestic, FLT(u6, 20), svo, svx, TRM("D"), a330, ab::Wide },
            someDt, bpm(54)
        },
        {
            { st::Domestic, FLT(su, 10), svo, led, TRM("F"), a320, ab::Narrow },
            { st::Domestic, FLT(u6, 20), svo, svx, TRM("D"), a330, ab::Wide },
            someDt, bpm(55)
        },
        //fuzzy search (max)
        {
            { st::Domestic, FLT(su, 10), svo, led, NA, NA, ab::Any },
            { st::Domestic, FLT(u6, 20), svo, svx, NA, NA, ab::Any },
            someDt, bpm(55)
        },

    };
    //-------------------------------------------------------------------------
    Seeker skr(testRecords(), Seeker::Maximum);

    size_t cn = 0;
    for (const auto& c : cases) {
        ++cn;
        LogTrace(TRACE5) << "case " << cn;

        auto r = skr.search(c.arrSg, c.depSg, c.dt);
        fail_unless(static_cast<bool>(r) == static_cast<bool>(c.tm), "invalid result in case #%zd", cn);
        if (r) {
            fail_unless(r->time == c.tm, "invalid result in case #%zd", cn);
            if (c.wt) {
                fail_unless(getWeight(*r) == c.wt, "invalid result in case #%zd", cn);
            }
        }
    }

    //fuzzy search (min)
    Seeker skr_min(testRecords(), Seeker::Minimum);
    auto r = skr_min.search(
        { st::Domestic, FLT(su, 10), svo, led, NA, NA, ab::Any },
        { st::Domestic, FLT(u6, 20), svo, svx, NA, NA, ab::Any },
        someDt
    );
    fail_unless(r->time == bpm(44), "invalid result");


}
END_TEST

START_TEST(duplications)
{
    using namespace ssim::mct;

    using st = ssim::mct::Status;
    using ab = ssim::mct::AircraftBody;
    using bpm = boost::posix_time::minutes;

    const boost::gregorian::date pinf(boost::gregorian::pos_infin);
    const boost::gregorian::date ninf(boost::gregorian::neg_infin);
    const boost::posix_time::time_duration tdinf(boost::posix_time::pos_infin);

    const nsi::PointId svo = nsi::Point(EncString::fromDb("SVO")).id();

    const nsi::CompanyId ut = nsi::Company(EncString::fromDb("UT")).id();
    const nsi::CompanyId su = nsi::Company(EncString::fromDb("SU")).id();
    const nsi::CompanyId lh = nsi::Company(EncString::fromDb("LH")).id();
    //-------------------------------------------------------------------------
    struct {
        ssim::mct::Record rec;
        bool duplicated;
    } cases[] = {
        {
            {
                { st::Domestic, svo, NA, lh, NA, false, FlightRange(1600, 2000), NA, ab::Any },
                { st::Domestic, svo, NA, su, NA, false, FlightRange(3500, 3999), NA, ab::Any },
                ninf, pinf, false, NA, NA, NA, bpm(100)
            }, true
        },
        {
            {
                { st::Domestic, svo, NA, lh, NA, false, FlightRange(1600, 1999), NA, ab::Any },
                { st::Domestic, svo, NA, su, NA, false, FlightRange(3500, 3999), NA, ab::Any },
                ninf, pinf, false, NA, NA, NA, bpm(100)
            }, false
        },
        {
            {
                { st::Domestic, svo, NA, lh, NA, false, FlightRange(1600, 2001), NA, ab::Any },
                { st::Domestic, svo, NA, su, NA, false, FlightRange(3700, 3999), NA, ab::Any },
                ninf, pinf, false, NA, NA, NA, bpm(100)
            }, false
        },
        {
            {
                { st::Domestic, svo, NA, lh, NA, false, FlightRange(1600, 2001), NA, ab::Any },
                { st::Domestic, svo, NA, su, NA, false, FlightRange(3600, 4000), NA, ab::Any },
                ninf, pinf, false, NA, NA, NA, bpm(100)
            }, false
        },
        {
            {
                { st::Domestic, svo, NA, lh, NA, false, FlightRange(1599, 1999), NA, ab::Any },
                { st::Domestic, svo, NA, su, NA, false, FlightRange(3600, 4000), NA, ab::Any },
                ninf, pinf, false, NA, NA, NA, bpm(100)
            }, true
        },
        {
            {
                { st::Domestic, svo, NA, lh, NA, false, FlightRange(1599, 1999), NA, ab::Any },
                { st::Domestic, svo, NA, su, NA, false, FlightRange(3600, 4000), NA, ab::Any },
                Dates::currentDate(), pinf, false, NA, NA, NA, bpm(100)
            }, false
        },
        {
            {
                { st::Domestic, svo, NA, ut, NA, false, NA, NA, ab::Any },
                { st::Domestic, svo, NA, NA, NA, false, NA, NA, ab::Any },
                Dates::currentDate() + boost::gregorian::days(15), pinf, false, NA, NA, NA, bpm(100)
            }, true
        },
        {
            {
                { st::Domestic, svo, NA, ut, NA, false, NA, NA, ab::Any },
                { st::Domestic, svo, NA, NA, NA, false, NA, NA, ab::Any },
                Dates::currentDate(), pinf, false, NA, NA, NA, bpm(100)
            }, true
        },
        {
            {
                { st::Domestic, svo, NA, ut, NA, false, NA, NA, ab::Any },
                { st::Domestic, svo, NA, NA, NA, false, NA, NA, ab::Any },
                ninf, Dates::currentDate(), false, NA, NA, NA, bpm(100)
            }, false
        },
        {
            {
                { st::Domestic, svo, NA, ut, NA, false, NA, NA, ab::Any },
                { st::Domestic, svo, NA, NA, NA, false, NA, NA, ab::Any },
                Dates::currentDate() + boost::gregorian::days(15), Dates::currentDate() + boost::gregorian::days(30), false, NA, NA, NA, bpm(100)
            }, true
        },
        {
            {
                { st::Domestic, svo, NA, ut, NA, false, NA, NA, ab::Any },
                { st::Domestic, svo, NA, NA, NA, false, NA, NA, ab::Any },
                Dates::currentDate() + boost::gregorian::days(60), Dates::currentDate() + boost::gregorian::days(70), false, NA, NA, NA, bpm(100)
            }, false
        },
    };
    //-------------------------------------------------------------------------
    const std::vector<Record> exst = testRecords();

    size_t cn = 0;
    for (const auto& c : cases) {
        ++cn;
        LogTrace(TRACE5) << "case " << cn;

        const Record& rec = c.rec;

        const bool dup = std::any_of(exst.begin(), exst.end(), [&rec] (const Record& x) {
            return isDuplicated(rec, x);
        });

        fail_unless(dup == c.duplicated, "invalid result in case #%zd", cn);
    }
}
END_TEST

#define SUITENAME "mct"
TCASEREGISTER(nsi::setupTestNsi, nullptr)
{
    ADD_TEST(search_matching);
    ADD_TEST(duplications);
}
TCASEFINISH

#endif //XP_TESTING
