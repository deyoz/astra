#include <cstring>
#include <fstream>

#include <list>

#include <serverlib/dates.h>
#include <serverlib/period.h>
#include <serverlib/str_utils.h>
#include <nsi/time_utils.h>

#include "ssimexport.h"

#define NICKNAME "D.ZERNOV"
#include <serverlib/slogger.h>

static Period defaultPeriod()
{
    return Period(Dates::currentDate(), boost::gregorian::date(boost::gregorian::pos_infin));
}

static ssimexp::FlightRange defaultFlightRange()
{
    return ssimexp::FlightRange(ct::FlightNum(1), ct::FlightNum(9999));
}

namespace {

static std::string pointCode(nsi::PointId pt)
{
    return nsi::Point(pt).code(ENGLISH).to866();
}

class ItineraryVariation
{
    std::size_t number;
public:
    ItineraryVariation() : number(0) {}

    ItineraryVariation& operator++() {
        ++number;
        return *this;
    }

    std::string format() const {
        ASSERT(number > 0);
        return StrUtils::LPad(HelpCpp::string_cast(number % 100), 2, '0');
    }

    char overflow() const {
        ASSERT(number > 0);
        return (number < 100 ? ' ' : (number / 100) + '0');
    }
};

class RecordSerialNumber
{
    std::size_t number;
public:

    RecordSerialNumber() : number(2) {}

    RecordSerialNumber& operator++() {
        if ((++number) >= 1000000) {
            number = 2;
        }
        return *this;
    }

    std::string format() const {
        return StrUtils::LPad(HelpCpp::string_cast(number), 6, '0');
    }
};

class ScdExportFilter
{
    std::vector<ssimexp::FlightRange> flights_;
    bool iataOnly_;

    bool isSuitableFlight(const ct::Flight& flt) const {
        if (flights_.empty()) {
            return true;
        }
        for (const auto& fr : flights_) {
            if (flt.number >= fr.first && flt.number <= fr.second) {
                return true;
            }
        }
        return false;
    }

public:
    ScdExportFilter(const std::vector<ssimexp::FlightRange>& f, bool iataOnly)
        : flights_(f), iataOnly_(iataOnly)
    {}

    bool operator()(const ssim::ScdPeriod& scp) const
    {
        if (!isSuitableFlight(scp.flight)) {
            return true;
        }

        if (iataOnly_) {
            for (const ssim::Leg& leg : scp.route.legs) {
                const std::string dep = pointCode(leg.s.from);
                const std::string arr = pointCode(leg.s.to);
                if (dep.empty() || StrUtils::containsRus(dep) ||
                    arr.empty() || StrUtils::containsRus(arr))
                {
                    LogTrace(TRACE1) << "Skip schedule: " << scp.flight << " " << scp.period;
                    return true;
                }
            }
        }
        return false;
    }
};

static char dateVariation(const boost::posix_time::time_duration& tm)
{
    const boost::gregorian::days d = Dates::daysOffset(tm);
    return (d.is_negative() ? 'A' : d.days() + '0');
}

static std::string dateVariation(const ssim::Leg& leg)
{
    const char depVar = dateVariation(leg.s.dep);
    const char arrVar = dateVariation(leg.s.arr);

    if (depVar == '0' && arrVar == '0') {
        return std::string(2, ' ');
    }
    return std::string { depVar, arrVar };
}

static std::vector<ct::SegNum> segsFromLeg(ct::LegNum ln, size_t legCnt, bool onlyLong)
{
    std::vector<ct::SegNum> out;
    for (size_t i = ln.get(); i != legCnt; ++i) {
        if (onlyLong && static_cast<size_t>(ln.get()) == i) {
            continue;
        }
        out.push_back(ct::segnum(ct::PointNum(ln.get()), ct::PointNum(i + 1)));
    }
    return out;
}

struct Seg
{
    ssim::Section s;
    boost::optional<ct::RbdLayout> order;
    boost::optional<ssim::MealServiceInfo> meals;
    ssim::Restrictions restrictions;
};

static Seg seg(const ssim::Route& route, ct::SegNum sn)
{
    ASSERT(static_cast<size_t>(sn.get()) < route.segCount());

    const ct::PointsPair pp(ct::pointsBySeg(sn));
    auto flg = std::next(route.legs.begin(), pp.first.get());
    auto elg = std::next(route.legs.begin(), (pp.second.get() - 1));

    ssim::SegmentsProps::const_iterator it = route.segProps.find(sn);
    ssim::SegProperty sp = (it != route.segProps.end()) ? it->second : ssim::SegProperty();

    if (flg == elg) {
        return Seg { flg->s, flg->subclOrder, flg->meals, sp.restrictions };
    }

    return Seg {
        ssim::Section(flg->s.from, elg->s.to, flg->s.dep, elg->s.arr),
        sp.subclOrder,
        sp.meals,
        sp.restrictions
    };
}

static char pointIndicator(ct::PointNum pt)
{
    return 'A' + pt.get();
}

static std::string pointIndicators(ct::LegNum ln)
{
    const auto pts = ct::pointsByLeg(ln);
    return std::string({ pointIndicator(pts.first), pointIndicator(pts.second) });
}

static std::string pointIndicators(ct::SegNum sn)
{
    const auto pts = ct::pointsBySeg(sn);
    return std::string({ pointIndicator(pts.first), pointIndicator(pts.second) });
}

struct DeferredRecord
{
    std::string pointIndicators;
    std::string depPt;
    std::string arrPt;
    ct::DeiCode code;
    std::string content;
};
typedef std::list<DeferredRecord> DeferredRecordSet;

static std::string composeTrafficRestrictions(
        ct::LegNum ln, const ssim::Route& route, DeferredRecordSet& drs
    )
{
    /* Record type 3: 150-160 bytes (Traffic restriction code) + 161 (TR Leg overflow indicator)
       Each byte from 150 to 160 relates sequentially to the Off Points in the routing (max 12 legs).

       For extra long routing (> 12 legs) with TR byte 161 should be used.
       NOTE: Will not fill it now since we have no routes with more than 11 legs

       Routing: LHR-FCO-THR-DEL-BKK

                   150  151  152  153  ... 161
                   FCO  TRH  DEL  BKK  ...
       01 LHR-FCO   Z
       02 FCO-THR        Q
       03 THR-DEL
       04 DEL-BKK                  A

       + Record type 4 (due to LHR-FCO Z):
       AB170LHRFCOW
       AB171LHRFCOA
    */

    std::vector<std::string> offPts;
    for (const auto& lg : route.legs) {
        offPts.push_back(pointCode(lg.s.to));
    }

    std::string recTrs(12, ' ');

    for (size_t i = ln.get(), n = route.legs.size(); i < n; ++i) {
        const ct::SegNum sn = ct::segnum(ct::PointNum(ln.get()), ct::PointNum(i + 1));
        auto sg = seg(route, sn);
        const ssim::Restrictions& trs = sg.restrictions;
        if (trs.restrs.empty()) {
            continue;
        }

        ASSERT(i < recTrs.size() - 1);

        //restrictions for Record type 3
        const ssim::Restriction& tr = *trs.restrs.begin();
        if (trs.restrs.size() > 1 || tr.application) {
            recTrs[i] = 'Z';
        } else {
            recTrs[i] = nsi::Restriction(tr.code).code(ENGLISH).to866().front();
        }

        //additional deis for separate Record type 4
        for (const ssim::Restriction& r : trs.restrs) {
            if (r.application) {
                drs.emplace_back(DeferredRecord {
                    pointIndicators(sn), pointCode(sg.s.from), pointCode(sg.s.to),
                    *r.application, nsi::Restriction(r.code).code(ENGLISH).to866()
                });
            }
        }

        if (trs.qualifier) {
            drs.emplace_back(DeferredRecord { pointIndicators(sn), pointCode(sg.s.from), pointCode(sg.s.to), *trs.qualifier });
        }

        for (const auto& inf : trs.info) {
            drs.emplace_back(DeferredRecord {
                pointIndicators(sn), pointCode(sg.s.from), pointCode(sg.s.to), inf.first, inf.second
            });
        }
    }
    return recTrs;
}

static std::string mealsSsimString(const ssim::MealServicesMap& mls, const ct::Rbds& rbds)
{
    std::string out;
    for (ct::Rbd r : rbds) {
        auto i = mls.find(r);
        if (i != mls.end() && !i->second.empty()) {
            out.push_back(nsi::MealService(i->second.front()).code(ENGLISH).to866().front());
            if (i->second.size() > 1) {
                out.push_back(nsi::MealService(*std::next(i->second.begin())).code(ENGLISH).to866().front());
            } else {
                out.push_back(' ');
            }
        } else {
            out.append("  ");
        }
    }
    return out;
}

static std::string composeMeals(
        const ssim::Leg& leg, ct::LegNum ln,
        const ssim::Route& route, DeferredRecordSet& drs
    )
{
    /* Record Type 3: 101-110 bytes (Meal Service Note), 2 bytes (meal code) per class
       If Meal service note is applicable to more than 5 classes, "XX" will be stated on first 2 positions

       + Record Type 4 (due to "XX") with DEI 109
    */

    const ssim::MealServicesMap& mls = leg.meals.rbdMeals;

    std::string legMls(10, ' ');
    if (!mls.empty()) {
        const std::string m = mealsSsimString(mls, leg.subclOrder.rbds());
        if (m.size() > 10) {
            legMls.replace(0, 2, "XX");
            drs.push_back(DeferredRecord {
                pointIndicators(ln), pointCode(leg.s.from), pointCode(leg.s.to), ct::DeiCode(109), m
            });
        } else {
            legMls.replace(0, m.size(), m);
        }
    }
    //-------------------------------------------------------------------------
    // Record Type 4 with DEI 111 (Meal Service Segment Override)
    for (ct::SegNum sn : segsFromLeg(ln, route.legs.size(), true)) {
        const auto sg = seg(route, sn);
        if (sg.meals && !sg.meals->rbdMeals.empty() && sg.order) {
            const std::string m = mealsSsimString(sg.meals->rbdMeals, sg.order->rbds());
            if (!m.empty()) {
                drs.push_back(DeferredRecord {
                    pointIndicators(sn), pointCode(sg.s.from), pointCode(sg.s.to), ct::DeiCode(111), m
                });
            }
        }
    }

    return legMls;
}

static std::string makeStationInfo(
        nsi::PointId pt,
        const boost::gregorian::date& dt,
        const boost::posix_time::time_duration& tm,
        const boost::optional<nsi::TermId>& term
    )
{
    const std::string strTm = Dates::hh24mi(Dates::dayTime(tm), false);
    const std::string strTerm = (term ? term->get() : std::string());

    const boost::posix_time::ptime t(dt, tm);
    const int m = Dates::getTotalMinutes(t - nsi::timeCityToGmt(t, pt));
    const int utc_offset = (m / 60) * 100 + (m % 60);


    // Departure (Arrival) station
    // Passenger STD (STA)
    // Aircraft STD (STA)
    // UTC/Local time variation for departure (arrival)
    // Passenger terminal for departure (arrival)

    char buf[100];
    sprintf(buf, "%3.3s%4.4s%4.4s%+05d%-2.2s",
        pointCode(pt).c_str(),
        strTm.c_str(),
        strTm.c_str(),
        utc_offset,
        strTerm.c_str()
    );
    return std::string(buf);
}

static std::string fmtSegmentDataRecord(
        const std::string& legDesc,
        const std::string& indicators,
        const std::string& from, const std::string& to,
        unsigned int deiCode, const std::string& deiContent,
        const ItineraryVariation& ivi,
        RecordSerialNumber& rsn
    )
{
    char buf[200];
    memset(buf, ' ', sizeof(buf));
    ('4' + legDesc).copy(buf, legDesc.size() + 1);
    //Itinerary Variation Identifier Overflow
    buf[27] = ivi.overflow();
    // Board Point Indicator + Off Point Indicator + DEI + Board Point + Off Point
    sprintf(&buf[28], "%2.2s%03u%3.3s%3.3s", indicators.c_str(), deiCode, from.c_str(), to.c_str());
    //dei data
    buf[39] = ' ';
    deiContent.copy(&buf[39], deiContent.size());
    //record serial number
    (++rsn).format().copy(&buf[194], 6);
    return std::string(buf, 200);
}

struct FranchisePub
{
    std::string code;
    std::string name;
};

static boost::optional<FranchisePub> makeFranchisePub(const boost::optional<ssim::Franchise>& in)
{
    if (!in) {
        return boost::none;
    }

    const ssim::Franchise& fr = *in;

    std::string frAir;
    if (fr.code) {
        frAir = StrUtils::RPad(nsi::Company(*fr.code).code(ENGLISH).to866(), 3, ' ');
        if (StrUtils::containsRus(frAir)) {
            frAir.clear();
        }
    }

    FranchisePub out = { frAir, fr.name };
    if (out.code.empty() && fr.name.empty()) {
        return boost::none;
    }
    return out;
}

}

namespace ssimexp {

static std::map<ct::Flight, ssim::ScdPeriods> prepareForPublication(const ssim::ScdPeriods& scps, bool splitW2 = true)
{
    //skip adhocs
    //group schedule periods by Flight Designator (regardless operational suffix)
    //split W2-periods

    std::map<ct::Flight, ssim::ScdPeriods> out;

    for (const ssim::ScdPeriod& scp : scps) {
        if (scp.adhoc) {
            continue;
        }

        ssim::ScdPeriods& grp = out[ct::Flight(scp.flight.airline, scp.flight.number)];
        if (splitW2 && scp.period.biweekly) {
            for (const Period& p : splitBiweekPeriod(scp.period)) {
                grp.push_back(scp);
                grp.back().period = p;
            }
        } else {
            grp.push_back(scp);
        }
    }

    //finally sort periods to set IVI correctly
    for (auto& v : out) {
        std::sort(v.second.begin(), v.second.end(),
            [] (const ssim::ScdPeriod& x, const ssim::ScdPeriod& y) {
                return (x.period != y.period
                    ? x.period < y.period
                    : x.flight < y.flight
                );
            }
        );
    }

    return out;
}

static void addSsimLine(std::ostream& os, const char* data)
{
    const std::size_t LineSize = 210;
    char line[LineSize] = {0};
    snprintf( line, LineSize, "%200.200s\n", data );
    os << line;
}

void createSsimFile(
        std::ostream& os,
        const nsi::Company& awk,
        const std::vector<FlightRange>& ranges,
        const Period& ssimPeriod,
        bool iataOnly
    )
{

    const ScdExportFilter scdFilter(
        ranges.empty() ? std::vector<FlightRange>(1, defaultFlightRange()) : ranges,
        iataOnly
    );

    const Period reqPrd = (
        ssimPeriod.empty() ? defaultPeriod() : ssimPeriod
    );

    const std::string today = Dates::ddmonrr(Dates::currentDate(), ENGLISH);
    const std::string awk_code = StrUtils::RPad(awk.code(ENGLISH).to866(), 3, ' ');

    char buf[210];

    //Record type 1
    memset( buf, ' ', 200 );
    memcpy( buf, "1AIRLINE STANDARD SCHEDULE DATA SET", 35 );
    memcpy( &buf[191], "001000001", 9 );
    addSsimLine(os, buf);

    for( int i = 0; i < 4; ++i ) {
        memset( buf, '0', 200 );
        addSsimLine(os, buf);
    }

    //Record type 2
    memset( buf, ' ', 200 );
    buf[0] = '2';
    buf[1] = 'L'; //time mode
    awk_code.copy(&buf[2], awk_code.size());
    //Period of schedule validity
    Dates::ddmonrr(reqPrd.start, ENGLISH).copy(&buf[14], 7);
    if (reqPrd.end.is_infinity()) {
        memcpy(&buf[21], "00XXX00", 7);
    } else {
        Dates::ddmonrr(reqPrd.end, ENGLISH).copy(&buf[21], 7);
    }
    //Creation date
    today.copy(&buf[28], today.size());
    //Title of data
    memcpy( &buf[35], "SIRENA-2000 SSIM FILE EXPORT", 28 );
    //Release date
    today.copy(&buf[64], today.size());
    buf[71] = 'C'; //schedule status
    memcpy( &buf[188], "EN", 2 ); //eticket by default
    //creation time + record serial number
    time_t timer = time( NULL );
    struct tm *st = localtime( &timer );
    sprintf( &buf[190], "%02d%02d000002", st->tm_hour, st->tm_min );
    addSsimLine(os, buf);

    for( int i = 0; i < 4; ++i ) {
        memset( buf, '0', 200 );
        addSsimLine(os, buf);
    }

    RecordSerialNumber rsn;

    //load all airline schedules even if single flight requested due to suffix flight variants
    for (const auto& scdGrp : prepareForPublication(createScheduleCollection(awk.id(), reqPrd))) {
        ItineraryVariation ivi;
        for (const ssim::ScdPeriod& scp : scdGrp.second) {
            if (scdFilter(scp)) {
                continue;
            }

            const Period scdPrd = Period::normalize(scp.period & reqPrd);
            if (scdPrd.empty()) {
                LogTrace(TRACE1) << "Skip schedule: " << scp.flight << " " << scp.period;
                continue;
            }

            ++ivi;
            size_t legNo = 0;
            for (const ssim::Leg& leg : scp.route.legs) {
                DeferredRecordSet drs;

                const Period prd = scdPrd.shift(Dates::daysOffset(leg.s.dep));
                const std::string legDesc(
                    StrUtils::RPad(ct::suffixToString(scp.flight.suffix, ENGLISH), 1, ' ')
                            + awk_code
                            + StrUtils::LPad(HelpCpp::string_cast(scp.flight.number.get()), 4, ' ')
                            + ivi.format()
                            + StrUtils::LPad(HelpCpp::string_cast(legNo + 1), 2, '0')
                            + nsi::ServiceType(leg.serviceType).code(ENGLISH).to866()
                );

                //Record type 3
                memset( buf, ' ', 200 );
                strcpy(buf, ("3" + legDesc).c_str());
                strcpy(&buf[14], Dates::ddmonrr(prd.start, ENGLISH).c_str());
                strcpy(&buf[21], Dates::ddmonrr(prd.end, ENGLISH).c_str());
                strcpy(&buf[28], StrUtils::replaceSubstrCopy(prd.freq.str(), ".", " ").c_str());
                buf[35] = (prd.biweekly ? '2' : ' ');
                strcpy(&buf[36], makeStationInfo(leg.s.from, scdPrd.start, leg.s.dep, leg.depTerm).c_str());
                strcpy(&buf[54], makeStationInfo(leg.s.to, scdPrd.start, leg.s.arr, leg.arrTerm).c_str());
                nsi::AircraftType(leg.aircraftType).code(ENGLISH).to866().copy(&buf[72], 3);
                //-------------------------------------------------------------
                //PRBD
                const std::string prbd = ct::rbdsCode(leg.subclOrder.rbds(), ENGLISH);
                if (prbd.size() > 20) {
                    memcpy(&buf[75], "XX", 2);
                } else {
                    prbd.copy(&buf[75], prbd.size());
                }

                // ACV
                std::string acv = ct::toString(leg.subclOrder.config());
                if (!acv.empty()) {
                    if (acv.size() > 20) {
                        //too long ACV... remove seat counts to avoid DEI 108
                        acv = ct::cabinsCode(leg.subclOrder.cabins());
                    }
                    acv.copy(&buf[172], acv.size());
                }

                //Itinerary Variation Identifier Overflow
                buf[127] = ivi.overflow();
                //-------------------------------------------------------------
                //Operating Airline Disclosure
                std::string oprDisclosureTxt;
                const boost::optional<FranchisePub> frp = makeFranchisePub(leg.franchise);
                if (leg.oprFlt && frp) {
                    if (!frp->code.empty()) {
                        frp->code.copy(&buf[128], 3);  //Aircraft Owner
                        buf[148] = 'L'; //point to Aircraft Owner (Code Share)
                    } else {
                        oprDisclosureTxt = '/' + frp->name;
                        buf[148] = 'Z'; //point to DEI 127 (Code Share)
                    }
                } else if (leg.oprFlt) {
                    const std::string oprAir = StrUtils::RPad(nsi::Company(leg.oprFlt->airline).code(ENGLISH).to866(), 3, ' ');
                    oprAir.copy(&buf[128], 3);  //Aircraft Owner
                    buf[148] = 'L'; //point to Aircraft Owner (Code Share)
                } else if (frp) {
                    if (frp->name.empty() && !frp->code.empty()) {
                        frp->code.copy(&buf[128], 3);  //Aircraft Owner
                        buf[148] = 'S'; //point to Aircraft Owner (Wet Lease)
                    } else if (!frp->name.empty()) {
                        oprDisclosureTxt = StrUtils::trim(frp->code) + '/' + frp->name;
                        buf[148] = 'X'; //point to DEI 127 (Wet Lease)
                    }
                }
                //-------------------------------------------------------------
                //Meal Service Note
                composeMeals(leg, ct::LegNum(legNo), scp.route, drs)
                    .copy(&buf[100], 10);  //bytes 101-110
                //-------------------------------------------------------------
                //Secure Flight Indicator
                if (leg.secureFlight) {
                    buf[121] = 'S';
                }
                //-------------------------------------------------------------
                //Traffic Restriction Code + Traffic Restriction Code Leg Overflow Indicator
                composeTrafficRestrictions(ct::LegNum(legNo), scp.route, drs)
                    .copy(&buf[149], 12);  //bytes 150-161
                //-------------------------------------------------------------
                //Date Variation
                dateVariation(leg).copy(&buf[192], 2);

                //record serial number
                (++rsn).format().copy(&buf[194], 6);
                addSsimLine(os, buf);
                //-------------------------------------------------------------
                //Records type 4

                //PRBD exceeding max length
                if (prbd.size() > 20) {
                    addSsimLine(os, fmtSegmentDataRecord(
                        legDesc, pointIndicators(ct::LegNum(legNo)), pointCode(leg.s.from), pointCode(leg.s.to),
                        106, prbd, ivi, rsn).c_str()
                    );
                }
                //-------------------------------------------------------------
                //PRBD segment override
                for (ct::SegNum sn : segsFromLeg(ct::LegNum(legNo), scp.route.legs.size(), true)) {
                    auto sg = seg(scp.route, sn);
                    if (sg.order && sg.order->rbds() != leg.subclOrder.rbds()) {
                        addSsimLine(os, fmtSegmentDataRecord(
                            legDesc, pointIndicators(sn), pointCode(sg.s.from), pointCode(sg.s.to),
                            101, ct::rbdsCode(sg.order->rbds(), ENGLISH), ivi, rsn
                        ).c_str());
                    }
                }
                //-------------------------------------------------------------
                if (leg.et) {
                    addSsimLine(os, fmtSegmentDataRecord(
                        legDesc, pointIndicators(ct::LegNum(legNo)), pointCode(leg.s.from), pointCode(leg.s.to),
                        505, "ET", ivi, rsn).c_str()
                    );
                }
                //-------------------------------------------------------------
                const std::vector<ct::Flight> cshFlights = (
                    leg.oprFlt ? std::vector<ct::Flight>(1, *leg.oprFlt) : leg.manFlts
                );
                if (!cshFlights.empty()) {
                    std::string cshStr;
                    for (const ct::Flight& cshFlt : cshFlights) {
                        cshStr += StrUtils::RPad(nsi::Company(cshFlt.airline).code(ENGLISH).to866(), 3, ' ') +
                                StrUtils::LPad(HelpCpp::string_cast(cshFlt.number.get()), 4, ' ') +
                                StrUtils::RPad(ct::suffixToString(cshFlt.suffix, ENGLISH), 1, ' ') + "/";
                    }
                    addSsimLine( os, fmtSegmentDataRecord(
                        legDesc, pointIndicators(ct::LegNum(legNo)), pointCode(leg.s.from), pointCode(leg.s.to),
                        (leg.oprFlt ? 50 : 10),
                        cshStr.substr(0, cshStr.size() - 1),
                        ivi, rsn).c_str()
                    );
                }
                //-------------------------------------------------------------
                if (!oprDisclosureTxt.empty()) {
                    addSsimLine(os, fmtSegmentDataRecord(
                        legDesc, pointIndicators(ct::LegNum(legNo)), pointCode(leg.s.from), pointCode(leg.s.to),
                        127, oprDisclosureTxt, ivi, rsn).c_str()
                    );
                }
                //-------------------------------------------------------------
                //additional records
                for (const auto& v : drs) {
                    addSsimLine(os, fmtSegmentDataRecord(
                        legDesc, v.pointIndicators, v.depPt, v.arrPt, v.code.get(), v.content, ivi, rsn
                    ).c_str());
                }
                //-------------------------------------------------------------
                ++legNo;
            }
        }
    }
    //-------------------------------------------------------------------------
    //Record type 5
    memset( buf, ' ', 200 );
    buf[0] = '5';
    awk_code.copy(&buf[2], awk_code.size());
    today.copy(&buf[5], today.size());
    buf[12] = ' ';
    rsn.format().copy(&buf[187], 6);
    buf[193] = 'E'; //the end
    (++rsn).format().copy(&buf[194], 6);
    addSsimLine(os, buf);

    for( int i = 0; i < 14; ++i ) {
        memset( buf, '0', 200 );
        addSsimLine(os, buf);
    }
}

void createSsimFile(
        const std::string& filename,
        const nsi::Company& awk,
        const std::vector<FlightRange>& ranges,
        const Period& ssimPeriod,
        bool iataOnly
    )
{
    std::ofstream file(filename.c_str(), std::ios_base::ate);
    createSsimFile(file, awk, ranges, ssimPeriod, iataOnly);
}


void createSsimFile(const std::string& filename, const nsi::Company& awk, bool iataOnly)
{
    std::ofstream file(filename.c_str(), std::ios_base::ate);
    createSsimFile(file, awk, std::vector<FlightRange>(1, defaultFlightRange()), defaultPeriod(), iataOnly);
}

}
