#include <serverlib/dates.h>

#include "ssim_schedule.h"
#include "ssim_utils.h"

#define NICKNAME "ASH"
#include <serverlib/slogger.h>

namespace ssim {

Section::Section(
        nsi::PointId d, nsi::PointId a,
        const boost::posix_time::time_duration& dtm, const boost::posix_time::time_duration& atm
    )
    : from(d), to(a), dep(dtm), arr(atm)
{}

nsi::DepArrPoints Section::dap() const
{
    return nsi::DepArrPoints(from, to);
}

Leg::Leg(const Section& s_, nsi::ServiceTypeId srv, nsi::AircraftTypeId tts, const ct::RbdLayout& ord)
    : s(s_), serviceType(srv), aircraftType(tts), subclOrder(ord), et(false), secureFlight(false)
{ }


size_t Route::segCount() const
{
    return ct::segsCnt(legs.size());
}

std::vector<nsi::PointId> Route::getSegPoints(ct::SegNum sn) const
{
    std::vector<nsi::PointId> res;
    ASSERT(sn.get() < ct::segsCnt(legs.size()));
    const ct::PointsPair pp(ct::pointsBySeg(sn));
    for (int i = pp.first.get(); i < pp.second.get(); ++i) {
        res.push_back(legs[i].s.from);
    }
    res.push_back(legs[pp.second.get()-1].s.to);
    ASSERT(res.size() > 1);
    return res;
}

bool SegProperty::empty() const
{
    return !et && !subclOrder && !meals && restrictions.empty();
}

ScdPeriod::ScdPeriod(const ct::Flight& f)
    : flight(f), adhoc(false), cancelled(false)
{}

std::ostream& operator<< (std::ostream& out, const Section& s)
{
    return out << nsi::Point( s.from ).code(ENGLISH) << " -> " << nsi::Point( s.to ).code(ENGLISH)
               << "; " << Dates::hh24mi(s.dep) << " - " << Dates::hh24mi(s.arr);
}

std::ostream& operator<< (std::ostream& out, const Franchise& f)
{
    if (f.code) {
        out << nsi::Company(*f.code).code(ENGLISH);
    }
    return out << '/' << f.name;
}

std::ostream& operator<< (std::ostream& out, const SegProperty& sp)
{
    if (sp.subclOrder) {
        out << " rbds: " << sp.subclOrder->toString() << ';';
    }
    if (sp.et) {
        out << ' ' <<  (*sp.et ? "ET" : "EN") << ';';
    }
    if (sp.meals) {
        out << " meals: " << mealsToString(*sp.meals) << ';';
    }
    if (!sp.restrictions.empty()) {
        out << " trs: " << sp.restrictions;
    }
    return out;
}

std::ostream& operator<< (std::ostream& s, const Leg& l)
{
    s << "Leg: " << l.s << ' ';
    if (l.depTerm) {
        s << " d:" << *l.depTerm;
    }
    if (l.arrTerm) {
        s << " a:" << *l.arrTerm;
    }
    if (l.oprFlt) {
        s << " opr:" << *l.oprFlt;
    }
    if (!l.manFlts.empty()) {
        s << " man: [ " << LogCont(" ", l.manFlts) << " ]";
    }
    s << (l.et ? " et" : " en");
    if (l.franchise) {
        s << ' ' << *l.franchise;
    }
    if (l.secureFlight) {
        s << " sf";
    }
    return s;
}

std::ostream& operator<< (std::ostream& s, const Route& r)
{
    for (const Leg& l :r.legs) {
        s << l << std::endl;
    }
    if (!r.segProps.empty()) {
        s << "seg properties:" << std::endl;
        for (const SegmentsProps::value_type& sp :r.segProps) {
            const std::vector<nsi::PointId> pts = r.getSegPoints(sp.first);
            s << "  #" << sp.first;
            if (!pts.empty()) {
                s << '(' << nsi::Point(pts.front()).code(ENGLISH) << " -> " << nsi::Point(pts.back()).code(ENGLISH) << ')';
            }
            s << ": " << sp.second << std::endl;
        }
    }
    return s;
}

std::ostream& operator<< (std::ostream& s, const ExtraSegInfo& x)
{
    return s
        << pointCode(x.depPt.get_value_or(nsi::PointId(0)))
        << pointCode(x.arrPt.get_value_or(nsi::PointId(0)))
        << ' ' << x.code << '/' << x.content;
}

std::ostream& operator<< (std::ostream& out, const ScdPeriod& scp)
{
    out << "ScdPeriod: " << scp.flight << " " << scp.period << (scp.adhoc ? " ADHOC" : "")
        << "; actual: " << std::boolalpha << !scp.cancelled << std::endl;
    out << "route: " << std::endl << scp.route;
    out << LogCont("\n", scp.auxData);
    return out;
}

bool operator< (const ScdPeriod& lhs, const ScdPeriod& rhs)
{
    if (lhs.flight != rhs.flight) {
        return lhs.flight < rhs.flight;
    }
    if (lhs.period != rhs.period) {
        return lhs.period < rhs.period;
    }
    return lhs.adhoc < rhs.adhoc;
}

static bool is_equal_mans(const std::vector<ct::Flight>& x, const std::vector<ct::Flight>& y)
{
    if (x.size() != y.size()) {
        return false;
    }
    for (const ct::Flight& vx :x) {
        if (y.end() == std::find(y.begin(), y.end(), vx)) {
            return false;
        }
    }
    return true;
}

bool operator== (const Section& lhs, const Section& rhs)
{
    return lhs.from == rhs.from
        && lhs.to == rhs.to
        && lhs.dep == rhs.dep
        && lhs.arr == rhs.arr;
}

bool operator== (const Franchise& lhs, const Franchise& rhs)
{
    return lhs.code == rhs.code && lhs.name == rhs.name;
}

bool operator== (const Leg& lhs, const Leg& rhs)
{
    return lhs.subclOrder == rhs.subclOrder
        && lhs.et == rhs.et
        && lhs.meals == rhs.meals
        && lhs.services == rhs.services
        && lhs.s == rhs.s
        && lhs.depTerm == rhs.depTerm
        && lhs.arrTerm == rhs.arrTerm
        && lhs.aircraftType == rhs.aircraftType
        && lhs.oprFlt == rhs.oprFlt
        && is_equal_mans(lhs.manFlts, rhs.manFlts)
        && lhs.franchise == rhs.franchise
        && lhs.serviceType == rhs.serviceType
        && lhs.secureFlight == rhs.secureFlight;
}

bool operator== (const SegProperty& lhs, const SegProperty& rhs)
{
    return lhs.subclOrder == rhs.subclOrder
        && lhs.et == rhs.et
        && lhs.meals == rhs.meals
        && lhs.restrictions == rhs.restrictions;
}

bool equalShort(const SegProperty& lhs, const SegProperty& rhs)
{
     return lhs.restrictions == rhs.restrictions;
}

bool equalLong(const SegProperty& lhs, const SegProperty& rhs)
{
     return lhs == rhs;
}

static bool is_equal_segprops(const SegmentsProps& spx, const SegmentsProps& spy)
{
    std::set<ct::SegNum> sns;
    for (const auto& vx : spx) {
        sns.insert(vx.first);
    }
    for (const auto& vy : spy) {
        sns.insert(vy.first);
    }

    for (ct::SegNum sn : sns) {
        auto vxi = spx.find(sn);
        auto vyi = spy.find(sn);

        const bool is_long_seg = ct::isLongSeg(sn);
        const bool isEqual = (is_long_seg ? equalLong(vxi == spx.end() ? SegProperty() : vxi->second,
                                                      vyi == spy.end() ? SegProperty() : vyi->second)
                                          : equalShort(vxi == spx.end() ? SegProperty() : vxi->second,
                                                       vyi == spy.end() ? SegProperty() : vyi->second));
        if (!isEqual) {
            return false;
        }
    }
    return true;
}

bool operator==(const Route& lhs, const Route& rhs)
{
    return lhs.legs == rhs.legs && is_equal_segprops(lhs.segProps, rhs.segProps);
}

bool operator== (const ExtraSegInfo& lhs, const ExtraSegInfo& rhs)
{
    return lhs.depPt == rhs.depPt
        && lhs.arrPt == rhs.arrPt
        && lhs.code == rhs.code
        && lhs.content == rhs.content;
}

static bool is_equal_extras(const ExtraSegInfos& x, const ExtraSegInfos& y)
{
    if (x.size() != y.size()) {
        return false;
    }
    for (const ExtraSegInfo& vx : x) {
        if (y.end() == std::find(y.begin(), y.end(), vx)) {
            return false;
        }
    }
    return true;
}

bool operator== (const ssim::ScdPeriod& lhs, const ssim::ScdPeriod& rhs)
{
    return lhs.flight == rhs.flight
        && lhs.period == rhs.period
        && lhs.cancelled == rhs.cancelled
        && lhs.adhoc == rhs.adhoc
        && lhs.route == rhs.route
        && is_equal_extras(lhs.auxData, rhs.auxData);
}

boost::optional<ct::Flight> operatingFlight(const Route& r)
{
    //TODO: а может на любом плече?
    return r.legs.front().oprFlt;
}

} //ssim
