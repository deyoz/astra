#include "period_joiner.h"
#include <serverlib/dates.h>

boost::optional<Periods> makePeriodsOpt(const std::set<boost::gregorian::date>& dts, bool allowBiweek);

struct PeriodHandlerStub {
    static Period getDatePeriod(const Period& obj) { return obj; }
    static void setDatePeriod(const Period& p, Period& obj) { obj = p; }
    static bool joinableDataStub(const Period&, const Period&) { return true; }
};

Periods join(const Periods& ps, bool allowBiweek)
{
    Periods periods(ps);
    std::sort(periods.begin(), periods.end());
    static DataPeriodJoiner<Period, PeriodHandlerStub> joiner(PeriodHandlerStub::joinableDataStub);
    joiner.join(periods, allowBiweek);
    return periods;
}

static boost::gregorian::date lastReasonableDate(const Periods& ps)
{
    boost::optional<boost::gregorian::date> dt;
    for (const Period& p : ps) {
        const auto d = (!p.end.is_pos_infinity() ? p.end : p.start + boost::gregorian::weeks(4));
        dt = std::max(dt.get_value_or(d), d);
    }
    return *dt;
}

Periods joinByDates(const Periods& in, bool allowBiweek)
{
    if (in.size() <= 1) {
        return in;
    }

    const boost::gregorian::date lrd = lastReasonableDate(in) + boost::gregorian::days(1);

    std::set<boost::gregorian::date> dts;
    for (const Period& p : in) {
        //for infinite periods assign reasonable end-date that will serve as marker of infinity
        const Period prd = (
            !p.end.is_infinity()
            ? p
            : *Period::create(p.start, lrd + boost::gregorian::weeks(2), p.freq, p.biweekly)
        );
        for (const boost::gregorian::date& dt : PeriodStl(prd)) {
            dts.emplace(dt);
        }
    }

    if (boost::optional<Periods> out = makePeriodsOpt(dts, allowBiweek)) {
        for (Period& p : *out) {
            if (p.end >= lrd) {
                //restore infinite periods
                p.end = boost::gregorian::date(boost::gregorian::pos_infin);
            }
        }
        return out->size() >= in.size() ? in : *out;
    }
    return in;
}
