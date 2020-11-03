#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif

#include <boost/regex.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>

#include <serverlib/enumset.h>
#include <serverlib/logopt.h>
#include <serverlib/str_utils.h>

#include "traffic_restriction.h"

#define NICKNAME "DAG"
#include <serverlib/slogger.h>

static nsi::RestrictionId trFromStr(const std::string& s)
{
    return nsi::Restriction(EncString::fromUtf(s)).id();
}

static std::string trToStr(const nsi::RestrictionId& r)
{
    return nsi::Restriction(r).code(ENGLISH).toUtf();
}

namespace ssim {

struct ServiceRestriction
{
    nsi::RestrictionId code;
    boost::optional<ct::DeiCode> application;   //DEI 170-173
    boost::optional<ct::DeiCode> qualifier;     //DEI 710-712
};

std::ostream& operator<< (std::ostream& out, const ssim::Restriction& r)
{
    out << trToStr(r.code);
    if (r.application) {
        out << '/' << *r.application;
    }
    return out;
}

bool Restrictions::empty() const
{
    return restrs.empty() && !qualifier && info.empty();
}

std::ostream& operator<< (std::ostream& out, const Restrictions& x)
{
    out << LogCont(" ", x.restrs);
    out << LogOpt(" Q", x.qualifier);
    if (!x.info.empty()) {
        out << " [";
        out << StrUtils::join("; ", x.info, [] (const auto& v) {
            return std::to_string(v.first.get()) + '/' + v.second;
        });
        out << "]";
    }
    return out;
}

bool operator== (const Restriction& lhs, const Restriction& rhs)
{
    return lhs.code == rhs.code && lhs.application == rhs.application;
}

bool operator== (const Restrictions& lhs, const Restrictions& rhs)
{
    return lhs.restrs == rhs.restrs
        && lhs.qualifier == rhs.qualifier
        && lhs.info == rhs.info;
}

} //ssim

namespace {

enum class TrLimit
{
    Domestic = 0,
    International,
    Online,
    Connex,
    Stopover,
    Qualified
};

using TrLimits = EnumSet<TrLimit>;

static bool is_applicable(ct::DeiCode code, ssim::TrafficType tt)
{
    switch (code.get()) {
        case 170:   return (tt == ssim::TrafficType::Passengers);
        case 171:   return (tt == ssim::TrafficType::Cargo || tt == ssim::TrafficType::Mail);
        case 172:   return (tt == ssim::TrafficType::Cargo);
        case 173:   return (tt == ssim::TrafficType::Mail);
    }
    return false;
}

static boost::optional<ssim::ServiceRestriction> select_applicable(
        const ssim::Restrictions& trs, ssim::TrafficType tt
    )
{
    for (const ssim::Restriction& tr : trs.restrs) {
        if (!tr.application || is_applicable(*tr.application, tt)) {
            return ssim::ServiceRestriction { tr.code, tr.application, trs.qualifier };
        }
    }
    return boost::none;
}

static const TrLimits* getLimits(const nsi::RestrictionId& tr)
{
    using TL = TrLimit;

    //this limits map for check trip with several segments (connecting or stopover)
    //so "Connecting or Stopover" => any

    //FIXME: There are several traffic restrictions that should be treated as another code for cargo/mail traffic
    //       e.g. T -> A, N -> A, Q -> O (stopovers not allowed?)
    //       It is not supported now (no needs to show carge/mail traffic)

    static const std::map<nsi::RestrictionId, TrLimits> trMap = {
        { trFromStr("C"), { TL::Domestic, TL::Connex } },
        { trFromStr("D"), { TL::Qualified, TL::International, TL::Online } },
        { trFromStr("E"), { TL::Qualified, TL::Online } },
        { trFromStr("F"), { TL::Online, TL::Connex } },
        { trFromStr("G"), { TL::Qualified, TL::Online, TL::Connex } },
        { trFromStr("K"), { TL::Connex } },
        { trFromStr("M"), { TL::International, TL::Online, TL::Stopover } },
        { trFromStr("N"), { TL::International, TL::Connex } },
        { trFromStr("O"), { TL::International, TL::Online, TL::Connex } },
        { trFromStr("Q"), { TL::International, TL::Online } },
        { trFromStr("T"), { TL::Online, TL::Stopover } },
        { trFromStr("V"), { } },
        { trFromStr("W"), { TL::International } },
        { trFromStr("X"), { TL::Online } },
        { trFromStr("Y"), { TL::Online, TL::Connex } }
    };

    auto i = trMap.find(tr);
    if (i != trMap.end()) {
        return &i->second;
    }
    return nullptr;
}


static bool is_adjacent_suitable(
        const ssim::tr::Seg& sg, const TrLimits& opts,
        const ssim::tr::Seg& asg, bool preceding
    )
{
    if (opts.contains(TrLimit::Domestic) && asg.international) {
        return false;
    }
    if (opts.contains(TrLimit::International) && !asg.international) {
        return false;
    }
    if (opts.contains(TrLimit::Online) && sg.airline != asg.airline) {
        return false;
    }
    if (opts.contains(TrLimit::Connex) || opts.contains(TrLimit::Stopover)) {
        //FIXME: treat stopover as waiting more then 24 hours but actually it may be more complex concept
        const boost::posix_time::time_duration wt(
            preceding ? sg.depTm - asg.arrTm : asg.depTm - sg.arrTm
        );
        if (opts.contains(TrLimit::Connex) && wt > boost::posix_time::hours(24)) {
            return false;
        }
        if (opts.contains(TrLimit::Stopover) && wt <= boost::posix_time::hours(24)) {
            return false;
        }
    }
    return true;
}

static bool is_seg_restricted__(
        const ssim::tr::Segs& trip, ssim::tr::Segs::const_iterator si,
        const ssim::ServiceRestriction& r, const TrLimits& opts
    )
{
    const bool have_preceding = (std::distance(trip.begin(), si) > 0);
    const bool have_following = (std::next(si) != trip.end());

    if (r.qualifier == ct::DeiCode(710)) {
        //at Board Point
        if (!have_preceding) {
            return true;
        }
        return !is_adjacent_suitable(*si, opts, *std::prev(si), true);
    }
    if (r.qualifier == ct::DeiCode(711)) {
        //at Off Point
        if (!have_following) {
            return true;
        }
        return !is_adjacent_suitable(*si, opts, *std::next(si), false);
    }
    if (r.qualifier == ct::DeiCode(712)) {
        //at Board AND Off Point
        if (!have_preceding || !have_following) {
            return true;
        }
        return !(
            is_adjacent_suitable(*si, opts, *std::prev(si), true)
            &&
            is_adjacent_suitable(*si, opts, *std::next(si), false)
        );
    }

    //by default - at Board OR Off Point
    if (have_preceding && is_adjacent_suitable(*si, opts, *std::prev(si), true)) {
        return false;
    }
    if (have_following && is_adjacent_suitable(*si, opts, *std::next(si), false)) {
        return false;
    }
    return true;
}

static boost::optional<ssim::ServiceRestriction> get_qualified_restriction(
        const ssim::Restrictions& trs, ssim::TrafficType tt
    )
{
    if (auto r = select_applicable(trs, tt)) {
        auto os = getLimits(r->code);
        if (os && os->contains(TrLimit::Qualified)) {
            return r;
        }
    }
    return boost::none;
}

static bool is_seg_qualified_restricted__(
        const ssim::tr::Segs& trip, const nsi::CompanyId& airline, ssim::TrafficType tt
    )
{
    //trip will be invalid if any qualified (D, E, G) restriction exists into and out of all online connect points
    for (auto i = trip.begin(); i != trip.end(); ++i) {
        if (i->airline != airline) {
            continue;
        }
        auto j = std::next(i);
        if (j != trip.end() && j->airline == airline) {
            //found online connection point
            auto ir = get_qualified_restriction(i->trs, tt);
            auto jr = get_qualified_restriction(j->trs, tt);
            if (!ir || !jr) {
                return false;
            }
            //both online segment have qualified restriction
            if (ir->qualifier == ct::DeiCode(710)) {
                //no restriction INTO connection point
                return false;
            }
            if (jr->qualifier == ct::DeiCode(711)) {
                //no restriction OUT connection point
                return false;
            }
        }
    }
    return true;
}

static bool is_seg_restricted(
        const ssim::tr::Segs& trip, ssim::tr::Segs::const_iterator si, ssim::TrafficType tt
    )
{
    const auto r = select_applicable(si->trs, tt);
    if (!r) {
        return false;
    }

    const std::string trf = trToStr(r->code);
    if (trf.empty()) {
        return false;
    }

    //FIXME: Can A, B, H, I restrictions have qualifiers (at board and/or off point)?
    static const std::string denied("ABHI");
    if (denied.find(trf.front()) != std::string::npos) {
        LogTrace(TRACE5) << "Seg #" << std::distance(trip.begin(), si) << " restricted by " << trf;
        return true;
    }

    const TrLimits* opts = getLimits(r->code);
    if (!opts) {
        return false;
    }

    //simple check by adjacent segments
    if (is_seg_restricted__(trip, si, *r, *opts)) {
        LogTrace(TRACE5) << "Seg #" << std::distance(trip.begin(), si) << " restricted by " << trf;
        return true;
    }

    //more complex check for qualified restrictions
    if (opts->contains(TrLimit::Qualified) && is_seg_qualified_restricted__(trip, si->airline, tt)) {
        LogTrace(TRACE5) << "Seg #" << std::distance(trip.begin(), si) << " restricted by " << trf;
        return true;
    }

    return false;
}

} //anonymous
//#############################################################################
namespace ssim {

bool operator< (const Restriction& lhs, const Restriction& rhs)
{
    return lhs.code != rhs.code
        ? lhs.code < rhs.code
        : lhs.application < rhs.application;
}

bool isSegmentRestricted(const Restrictions& trs, TrafficType tt)
{
    //FIXME: for passenger application
    //  DEMQTVWX - Displayed, but must be accompanied by appropriate text, eg. ONLINE CONNEX/STPVR TFC ONLY
    //  ... but what we can do with this segment?
    static const std::string denied("ADEGHIKMNOQTVWXY");

    const auto r = select_applicable(trs, tt);
    if (!r) {
        return false;
    }
    const std::string trf = trToStr(r->code);
    if (trf.empty()) {
        return false;
    }
    if (denied.find(trf.front()) != std::string::npos) {
        LogTrace(TRACE5) << "Seg restricted by " << trf;
        return true;
    }
    return false;
}

bool isTripRestricted(const tr::Segs& trip, TrafficType tt)
{
    for (auto si = trip.begin(); si != trip.end(); ++si) {
        if (is_seg_restricted(trip, si, tt)) {
            return true;
        }
    }
    return false;
}

boost::optional<Restrictions> parseTrNote(const std::string& deiCnt)
{
    // One DEI 8 per segment samples:
    // 8/Y
    // 8/Z/170/Q
    // 8/Y/710
    // 8/Q/782/TEXT

    // Complex sample - 4 DEI 8 per one seg (no one knows how it should be in a right way)
    // 8/Z/170/Y
    // 8/Z/171/A
    // 8/Z/782/TEXT
    // 8/Z/710

    const std::string s = StrUtils::trim(deiCnt);
    const nsi::RestrictionId z = nsi::Restriction(EncString::from866("Z")).id();

    static const boost::regex rx("^([A-Z])(?:/([0-9]{3})(?:/(.{1,}))?)?$");
    boost::smatch m;
    if (!boost::regex_match(s, m, rx)) {
        return boost::none;
    }

    if (!nsi::Restriction::find(EncString::from866(m[1]))) {
        return boost::none;
    }

    const nsi::RestrictionId trc = nsi::Restriction(EncString::from866(m[1])).id();

    if (!m[2].matched) {
        if (trc != z) {
            return ssim::Restrictions { { ssim::Restriction { trc } } };
        }
    } else {
        const int dcode = std::stoi(m[2]);
        if (dcode >= 170 && dcode <= 173) {
            //application
            if (trc == z && m[3].matched) {
                if (!nsi::Restriction::find(EncString::from866(m[3]))) {
                    return boost::none;
                }
                const nsi::RestrictionId tr = nsi::Restriction(EncString::from866(m[3])).id();
                if (tr != z) {
                    return ssim::Restrictions { { ssim::Restriction { tr, ct::DeiCode(dcode) } } };
                }
            }
        } else if (dcode >= 710 && dcode <= 712) {
            //qualifier
            if (!m[3].matched) {
                return ssim::Restrictions { { ssim::Restriction { trc } }, ct::DeiCode(dcode) };
            }
        } else if (dcode >= 713 && dcode <= 799) {
            //comment
            if (m[3].matched) {
                return ssim::Restrictions {
                    { ssim::Restriction { trc } },
                    boost::none,
                    { { ct::DeiCode(dcode), m[3] } }
                };
            }
        }
    }

    return boost::none;
}

boost::optional<Restrictions> mergeTrNotes(const std::vector<Restrictions>& trs)
{
    static const nsi::RestrictionId z = nsi::Restriction(EncString::from866("Z")).id();

    if (trs.empty()) {
        return boost::none;
    }

    ssim::Restrictions out;
    for (const ssim::Restrictions& x : trs) {
        for (const ssim::Restriction& r : x.restrs) {
            if (r.code != z) {
                out.restrs.emplace(r);
            }
        }

        if (!out.qualifier) {
            out.qualifier = x.qualifier;
        } else if (x.qualifier && out.qualifier != x.qualifier) {
            return boost::none;
        }

        for (const auto& v : x.info) {
            out.info.emplace(v.first, v.second);
        }
    }
    return out;
}

void setupTlgRestrictions(
        const std::function<void (ct::DeiCode, const std::string&)>& setupFn,
        const Restrictions& trs
    )
{
    if (trs.empty()) {
        return;
    }

    bool is_qualifier_appended = false;
    bool is_explain_appended = false;
    bool is_multi_app = false;

    for (const ssim::Restriction& r : trs.restrs) {
        const std::string trc = nsi::Restriction(r.code).code(ENGLISH).toUtf();
        if (r.application) {
            //XXXYYY 8/Z/170/Y
            setupFn(ct::DeiCode(8), "Z/" + std::to_string(r.application->get()) + "/" + trc);
            is_multi_app = true;
        } else {
            ASSERT(trs.restrs.size() == 1);

            if (trs.qualifier) {
                //XXXYYY 8/Y/710
                setupFn(ct::DeiCode(8), trc + "/" + std::to_string(trs.qualifier->get()));
                is_qualifier_appended = true;
            } else if (!trs.info.empty()) {
                //XXXYYY 8/Y/782/BLA-BLA
                setupFn(ct::DeiCode(8),
                    trc + "/" + std::to_string(trs.info.begin()->first.get())
                    + "/" + trs.info.begin()->second
                );
                is_explain_appended = true;
            } else {
                //XXXYYY 8/Y
                setupFn(ct::DeiCode(8), trc);
            }
        }
    }

    if (trs.qualifier && !is_qualifier_appended) {
        ASSERT(is_multi_app);
        setupFn(ct::DeiCode(8), "Z/" + std::to_string(trs.qualifier->get()));
    }

    for (auto i = is_explain_appended ? std::next(trs.info.begin()) : trs.info.begin();
         i != trs.info.end(); ++i)
    {
        setupFn(ct::DeiCode(8),
            (is_multi_app ? std::string("Z") : nsi::Restriction(trs.restrs.begin()->code).code(ENGLISH).toUtf())
            + "/" + std::to_string(i->first.get()) + "/" + i->second
        );
    }
}

} //ssim
//#############################################################################
#ifdef XP_TESTING

#include <serverlib/xp_test_utils.h>
#include <serverlib/checkunit.h>
#include <serverlib/tscript_diff.h>
#include <serverlib/dates.h>
#include <libnsi/callbacks.h>

namespace {

static void initTests()
{
    nsi::setupTestNsi();
}

ssim::Restrictions operator"" _tr(const char* s, size_t l)
{
    ssim::Restrictions out;
    for (const std::string& t : StrUtils::split_string< std::vector<std::string> >(s, ' ', StrUtils::KeepEmptyTokens::False)) {
        auto v = StrUtils::split_string< std::vector<std::string> >(t, '/', StrUtils::KeepEmptyTokens::True);
        ASSERT(!v.empty() && v.size() <= 3);

        auto qualifier = boost::make_optional(false, ct::DeiCode(1));
        if(v.size() > 1 && !v[1].empty())
            qualifier = ct::DeiCode(std::atoi(v[1].c_str()));
        auto application = boost::make_optional(false, ct::DeiCode(2));
        if(v.size() > 2 && !v[2].empty())
            application = ct::DeiCode(std::atoi(v[2].c_str()));

        ASSERT(out.restrs.empty() || out.qualifier == qualifier);
        out.qualifier = qualifier;
        auto code = nsi::Restriction(EncString::fromUtf(v[0])).id();
        auto inserted = out.restrs.emplace(ssim::Restriction { code, application });
        ASSERT(inserted.second);
    }
    return out;
}

START_TEST(traffic_restrictions)
{
    using namespace ssim;

    using td = boost::posix_time::time_duration;
    using day = boost::gregorian::days;
    using TT = ssim::TrafficType;

    const nsi::CompanyId airUT = nsi::Company(EncString::from866("UT")).id();
    const nsi::CompanyId airSU = nsi::Company(EncString::from866("SU")).id();

    const boost::gregorian::date bdt = Dates::currentDate() + day(1);

    const boost::posix_time::time_duration tms[] = {
        td( 9, 0, 0), td(10, 0, 0), td(11, 0, 0), td(12, 0, 0),
        td(13, 0, 0), td(14, 0, 0), td(15, 0, 0), td(16, 0, 0)
    };

    struct {
        tr::Segs trip;
        TrafficType tt;
        bool shouldBeRestricted;
    } cases[] = {
/*01*/  { { tr::Seg { airUT, { bdt, tms[0] }, { bdt, tms[4] }, "A"_tr , false } }, TT::Passengers, true },
        { { tr::Seg { airUT, { bdt, tms[0] }, { bdt, tms[4] }, "A/170"_tr, false } }, TT::Passengers, true },
        { { tr::Seg { airUT, { bdt, tms[0] }, { bdt, tms[4] }, "A/171"_tr, false } }, TT::Passengers, false },
        { { tr::Seg { airUT, { bdt, tms[0] }, { bdt, tms[4] }, "A/171 Y/170"_tr, false } }, TT::Passengers, true },
/*05*/  { { tr::Seg { airUT, { bdt, tms[0] }, { bdt, tms[4] }, "B"_tr, false } }, TT::Passengers, false },
        { { tr::Seg { airUT, { bdt, tms[0] }, { bdt, tms[4] }, "C"_tr, false } }, TT::Passengers, false },
        { { tr::Seg { airUT, { bdt, tms[0] }, { bdt, tms[3] }, { } , false },
            tr::Seg { airUT, { bdt, tms[4] }, { bdt, tms[7] }, "B"_tr, false } }, TT::Passengers, true },
        { { tr::Seg { airUT, { bdt, tms[0] }, { bdt, tms[3] }, { } , false },
            tr::Seg { airUT, { bdt, tms[4] }, { bdt, tms[7] }, "B/171"_tr, false } }, TT::Passengers, false },
        { { tr::Seg { airUT, { bdt, tms[0] }, { bdt, tms[3] }, { } , false },
            tr::Seg { airUT, { bdt, tms[4] }, { bdt, tms[7] }, "Y"_tr, false } }, TT::Passengers, false },
/*10*/  { { tr::Seg { airSU, { bdt, tms[0] }, { bdt, tms[3] }, { } , false },
            tr::Seg { airUT, { bdt, tms[4] }, { bdt, tms[7] }, "Y"_tr, false } }, TT::Passengers, true },
        { { tr::Seg { airUT, { bdt, tms[0] }, { bdt, tms[3] }, { } , false },
            tr::Seg { airUT, { bdt + day(1), tms[4] }, { bdt, tms[7] }, "Y"_tr, false } }, TT::Passengers, true },
        { { tr::Seg { airUT, { bdt, tms[0] }, { bdt, tms[3] }, { } , false },
            tr::Seg { airUT, { bdt + day(1), tms[4] }, { bdt, tms[7] }, "X"_tr, false } }, TT::Passengers, false },
        { { tr::Seg { airUT, { bdt, tms[0] }, { bdt, tms[3] }, { } , false },
            tr::Seg { airUT, { bdt, tms[4] }, { bdt, tms[7] }, "Y//710"_tr, false } }, TT::Passengers, false },
        { { tr::Seg { airUT, { bdt, tms[0] }, { bdt, tms[3] }, { } , false },
            tr::Seg { airUT, { bdt, tms[4] }, { bdt, tms[7] }, "Y//711"_tr, false } }, TT::Passengers, true },
/*15*/  { { tr::Seg { airUT, { bdt, tms[0] }, { bdt, tms[3] }, "Y//711"_tr , false },
            tr::Seg { airUT, { bdt, tms[4] }, { bdt, tms[7] }, { }, false } }, TT::Passengers, false },
        { { tr::Seg { airUT, { bdt, tms[0] }, { bdt, tms[3] }, { } , false },
            tr::Seg { airUT, { bdt, tms[4] }, { bdt, tms[7] }, "Y//712"_tr, false } }, TT::Passengers, true },
        { { tr::Seg { airUT, { bdt, tms[0] }, { bdt, tms[3] }, { } , false },
            tr::Seg { airUT, { bdt, tms[4] }, { bdt, tms[7] }, "Y//712"_tr, false },
            tr::Seg { airUT, { bdt + day(1), tms[0] }, { bdt, tms[2] }, { }, false } }, TT::Passengers, false },
        { { tr::Seg { airUT, { bdt, tms[0] }, { bdt, tms[3] }, { } , false },
            tr::Seg { airUT, { bdt, tms[4] }, { bdt, tms[7] }, "T"_tr, false } }, TT::Passengers, true },
        { { tr::Seg { airSU, { bdt, tms[0] }, { bdt, tms[3] }, { } , true },
            tr::Seg { airUT, { bdt, tms[4] }, { bdt, tms[7] }, "V"_tr, false } }, TT::Passengers, false },
/*20*/  { { tr::Seg { airSU, { bdt, tms[0] }, { bdt, tms[3] }, { } , true },
            tr::Seg { airUT, { bdt, tms[4] }, { bdt, tms[7] }, "W"_tr, false } }, TT::Passengers, false },
        { { tr::Seg { airSU, { bdt, tms[0] }, { bdt, tms[3] }, { } , false },
            tr::Seg { airUT, { bdt, tms[4] }, { bdt, tms[7] }, "W"_tr, true } }, TT::Passengers, true },
        { { tr::Seg { airSU, { bdt, tms[0] }, { bdt, tms[3] }, { } , false },
            tr::Seg { airUT, { bdt, tms[4] }, { bdt, tms[7] }, "C"_tr, true } }, TT::Passengers, false },
        { { tr::Seg { airSU, { bdt, tms[0] }, { bdt, tms[3] }, { } , true },
            tr::Seg { airUT, { bdt, tms[4] }, { bdt, tms[7] }, "C"_tr, true } }, TT::Passengers, true },
        { { tr::Seg { airSU, { bdt, tms[0] }, { bdt, tms[3] }, "H"_tr , false },
            tr::Seg { airUT, { bdt, tms[4] }, { bdt, tms[7] }, "C"_tr, true } }, TT::Passengers, true },
/*25*/  { { tr::Seg { airUT, { bdt, tms[0] }, { bdt, tms[3] }, "G//711"_tr, false },
            tr::Seg { airUT, { bdt, tms[4] }, { bdt, tms[7] }, "G//710"_tr, false } }, TT::Passengers, true },
        { { tr::Seg { airUT, { bdt, tms[0] }, { bdt, tms[3] }, "G//711"_tr, false },
            tr::Seg { airUT, { bdt, tms[4] }, { bdt, tms[7] }, "G"_tr, false } }, TT::Passengers, true },
        { { tr::Seg { airUT, { bdt, tms[0] }, { bdt, tms[3] }, "G"_tr, false },
            tr::Seg { airUT, { bdt, tms[4] }, { bdt, tms[7] }, "E"_tr, false } }, TT::Passengers, true },
        { { tr::Seg { airUT, { bdt, tms[0] }, { bdt, tms[3] }, "G//711"_tr, false },
            tr::Seg { airUT, { bdt, tms[4] }, { bdt, tms[7] }, "G//711"_tr, false },
            tr::Seg { airUT, { bdt + day(1), tms[0] }, { bdt, tms[2] }, "G//710"_tr, false } }, TT::Passengers, false },
        { { tr::Seg { airUT, { bdt, tms[0] }, { bdt, tms[3] }, "G//711"_tr, false },
            tr::Seg { airUT, { bdt, tms[4] }, { bdt, tms[7] }, "G"_tr, false },
            tr::Seg { airUT, { bdt + day(1), tms[0] }, { bdt, tms[2] }, "G//710"_tr, false } }, TT::Passengers, true },
/*30*/  { { tr::Seg { airUT, { bdt, tms[0] }, { bdt, tms[3] }, "Y//711"_tr, false },
            tr::Seg { airUT, { bdt, tms[4] }, { bdt, tms[7] }, "Y"_tr, false },
            tr::Seg { airUT, { bdt + day(1), tms[0] }, { bdt, tms[2] }, "Y//710"_tr, false } }, TT::Passengers, false },
        { { tr::Seg { airUT, { bdt, tms[0] }, { bdt, tms[3] }, { }, false },
            tr::Seg { airUT, { bdt, tms[4] }, { bdt, tms[7] }, { }, true },
            tr::Seg { airUT, { bdt + day(1), tms[0] }, { bdt, tms[2] }, "Q//710"_tr, false } }, TT::Passengers, false },
        { { tr::Seg { airUT, { bdt, tms[0] }, { bdt, tms[3] }, "G//712"_tr, false },
            tr::Seg { airUT, { bdt, tms[4] }, { bdt, tms[7] }, "G//712"_tr, false },
            tr::Seg { airUT, { bdt + day(1), tms[0] }, { bdt, tms[2] }, { }, false } }, TT::Passengers, true },
        { { tr::Seg { airUT, { bdt, tms[0] }, { bdt, tms[3] }, "Y//711"_tr, false },
            tr::Seg { airUT, { bdt, tms[4] }, { bdt, tms[7] }, "G//712"_tr, false },
            tr::Seg { airUT, { bdt + day(1), tms[0] }, { bdt, tms[2] }, "G//710"_tr, false } }, TT::Passengers, false },
        { { tr::Seg { airSU, { bdt, tms[0] }, { bdt, tms[3] }, { }, false },
            tr::Seg { airUT, { bdt, tms[4] }, { bdt, tms[7] }, "G//711"_tr, false },
            tr::Seg { airUT, { bdt + day(1), tms[0] }, { bdt, tms[2] }, "G//710"_tr, false } }, TT::Passengers, true },
/*35*/  { { tr::Seg { airSU, { bdt, tms[0] }, { bdt, tms[3] }, { }, false },
            tr::Seg { airUT, { bdt, tms[4] }, { bdt, tms[7] }, "Y//711"_tr, false },
            tr::Seg { airUT, { bdt + day(1), tms[0] }, { bdt, tms[2] }, "Y//710"_tr, false } }, TT::Passengers, false },
        { { tr::Seg { airSU, { bdt, tms[0] }, { bdt, tms[3] }, { }, false },
            tr::Seg { airUT, { bdt, tms[4] }, { bdt, tms[7] }, "G//711"_tr, false },
            tr::Seg { airUT, { bdt + day(1), tms[0] }, { bdt, tms[2] }, { }, false } }, TT::Passengers, false },
    };

    size_t cnt = 0;
    for (const auto& c : cases) {
        ++cnt;
        const bool cres = (c.trip.size() > 1
            ? isTripRestricted(c.trip, c.tt)
            : isSegmentRestricted(c.trip.front().trs, c.tt)
        );
        fail_unless(cres == c.shouldBeRestricted, "invalid result in case #%zd", cnt);
    }
}
END_TEST


#define SUITENAME "ssim"
TCASEREGISTER(initTests, 0)
{
    ADD_TEST(traffic_restrictions);
}
TCASEFINISH

} //anonymous

#endif //XP_TESTING
