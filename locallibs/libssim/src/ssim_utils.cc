#include "ssim_utils.h"

#include <serverlib/dates.h>
#include <serverlib/localtime.h>
#include <coretypes/message_binders.h>
#include <nsi/time_utils.h>

#include "ssim_schedule.h"
#include "callbacks.h"

#include "ssim_data_types.h"

#define NICKNAME "ASH"
#include <serverlib/slogger.h>

namespace ssim {

boost::gregorian::date guessDate(const std::string& str)
{
    if (str.length() == 5) {
        if (str == "00XXX") {
            return boost::gregorian::date(boost::gregorian::pos_infin);
        }
        return Dates::DateFromDDMON(str, Dates::GuessYear_Itin(), Dates::currentDate());
    } else if (str.length() == 7) {
        if (str == "00XXX00") {
            return boost::gregorian::date(boost::gregorian::pos_infin);
        }
        return Dates::DateFromDDMONYY(str, Dates::YY2YYYY_UseCurrentCentury, Dates::currentDate());
    } else {
        return boost::gregorian::date();
    }
}

std::string printDate(const boost::gregorian::date & dt)
{
    if (dt.is_pos_infinity() || dt.is_neg_infinity()) {
        return "00XXX00";
    }
    return Dates::ddmonrr(dt, ENGLISH);
}

std::string pointCode(nsi::PointId pt)
{
    if (pt.get() == 0) {
        return "QQQ";
    }
    return nsi::Point(pt).code(ENGLISH).toUtf();
}

boost::gregorian::days getUtcOffset(const ssim::ScdPeriod& scp)
{
    const Section& s = scp.route.legs.front().s;
    const boost::gregorian::date& dt = scp.period.start;
    return nsi::timeCityToGmt(boost::posix_time::ptime(dt, s.dep), s.from).date() - dt;
}

boost::gregorian::days getScdOffset(const ssim::ScdPeriod& scp, bool timeIsLocal)
{
    if (timeIsLocal || scp.route.legs.empty()) {
        return boost::gregorian::days(0);
    }
    return ssim::getUtcOffset(scp);
}

Period getSearchRangeForUtc(const Period& prd)
{
    //выбираем с учетом того, что период указан в UTC
    //для поиска подходящих периодов в LT надо расширить исходный на 1 день в каждую сторону
    return Period(
        prd.start.is_neg_infinity() ? prd.start : prd.start - boost::gregorian::days(1),
        prd.end.is_pos_infinity() ? prd.end : prd.end + boost::gregorian::days(1)
    );
}

nsi::DepArrPoints getSegDap(const ssim::Route& r, ct::SegNum sn)
{
    auto legs = getLegsRange(r.legs, sn);
    return nsi::DepArrPoints(legs.first->s.from, legs.second->s.to);
}

boost::optional<ct::LegNum> legnum(const ssim::Legs& lgs, const nsi::DepArrPoints& dap)
{
    for (size_t i = 0; i < lgs.size(); ++i) {
        if (lgs.at(i).s.dap() == dap) {
            return ct::LegNum(i);
        }
    }
    return boost::none;
}

boost::optional<ct::SegNum> segnum(const ssim::Legs& lgs, const nsi::DepArrPoints& dap)
{
    return ct::segnum(lgs, [&dap] (const ssim::Leg& lg, bool byDep) {
        return byDep
            ? lg.s.from == dap.dep
            : lg.s.to == dap.arr;
    });
}

namespace details {

std::vector<size_t> findLegs(const ssim::Legs& legs, nsi::PointId p1, nsi::PointId p2)
{
    std::vector<size_t> ret;
    if (p1.get() == 0 && p2.get() == 0) {
        // все плечи
        for (size_t i = 0; i < legs.size(); ++i) {
            ret.push_back(i);
        }
    } else if (p1.get() != 0 && p2.get() == 0) {
        // плечо, начинающееся в p1
        for (size_t i = 0; i < legs.size(); ++i) {
            if (legs.at(i).s.from == p1) {
                ret.push_back(i);
            }
        }
    } else if (p1.get() == 0 && p2.get() != 0) {
        // плечо, кончающееся в p2
        for (size_t i = 0; i < legs.size(); ++i) {
            if (legs.at(i).s.to == p2) {
                ret.push_back(i);
            }
        }
    } else {
        for (size_t i = 0; i < legs.size(); ++i) {
            //NOTE В SSIM написано что et на сегмент не должен приходить. Но приходит.
            //поэтому мы можем здесь так оказаться, что p1 и p2 - это не плечо, а сегмент
            //будем искать по первой точке и возвращать первое плечо сегмента
            if (legs.at(i).s.from == p1/* && legs.at(i).section.to == p2*/) {
                ret.push_back(i);
            }
        }
    }
    return ret;
}

std::vector<ct::SegNum> findSegs(const ssim::Route& route, nsi::PointId p1, nsi::PointId p2)
{
    std::vector<ct::SegNum> ret;
    std::vector<ct::SegNum> allSegs;
    for (size_t i = 0; i < route.segCount(); ++i)
        allSegs.push_back(ct::SegNum(i));
    // QQQQQQ
    if (p1.get() == 0 && p2.get() == 0) {
        for (ct::SegNum s :allSegs)
            ret.push_back(s);
    } else if (p1.get() != 0 && p2.get() == 0) { //ABCQQQ
        for (ct::SegNum &s :allSegs) {
            if (route.getSegPoints(s).front() == p1)
                ret.push_back(s);
        }
    } else if (p1.get() == 0 && p2.get() != 0) { //QQQABC
        for (ct::SegNum s :allSegs) {
            if (route.getSegPoints(s).back() == p2)
                ret.push_back(s);
        }
    } else { //ABCDEF
        for (ct::SegNum s :allSegs) {
            std::vector<nsi::PointId> ps = route.getSegPoints(s);
            if (ps.front() == p1 && ps.back() == p2)
                ret.push_back(s);
        }
    }
    return ret;
}

ssim::ScdPeriods getSchedulesUsingTimeMode(const ct::Flight& flt, const Period& prd, const ProcContext& attr, bool timeIsLocal)
{
    if (timeIsLocal) {
        //NOTE: timeIsLocal can be different from global attr.timeIsLocal !
        return attr.callbacks->getSchedules(flt, prd);
    }

    const Period p = getSearchRangeForUtc(prd);

    //в любом случае возвращаем в LT
    //в идеале, конечно, было бы оптимально работать с расписаниями в виде:
    //достаем из базы -> перегоняем в нужный time mode -> проводим все изменения -> перегоняем в LT и сохраняем изменения в базе
    //однако т.к. сама по себе обработка увязана с окружением, то требуется обработку проводить всегда в LT
    ssim::ScdPeriods out;
    for (const ssim::ScdPeriod& scp : attr.callbacks->getSchedules(flt, p)) {
        if (!(scp.period.shift(getUtcOffset(scp)) & prd).empty()) {
            out.push_back(scp);
        }
    }
    return out;
}

} //details

static std::vector<ct::Flight> getFltsByOpr(
        const ProcContext& attr, const ct::Flight& flt, const Period& pr
    )
{
    const Period prd = attr.timeIsLocal ? pr : getSearchRangeForUtc(pr);

    std::set<ct::Flight> ret;
    for (const ssim::ScdPeriod& scp : attr.callbacks->getSchedulesWithOpr(attr.ourComp, flt, prd)) {
        const boost::gregorian::days offset(attr.timeIsLocal ? 0 : getUtcOffset(scp).days());
        if (!(scp.period.shift(offset) & pr).empty()) {
            ret.emplace(scp.flight);
        }
    }

    return std::vector<ct::Flight>(ret.begin(), ret.end());
}

Expected<ct::Flight> getActFlight(const ProcContext& attr, const ct::Flight& tflt, const Period& p, bool allowNotFound)
{
    if (!attr.inventoryHost || attr.ourComp == tflt.airline) {
        return tflt;
    }
    // возможно, это операторская телеграмма - поищем в базе соответствие
    const std::vector<ct::Flight> mans = getFltsByOpr(attr, tflt, p);
    if (mans.empty()) {
        // иногда это не ошибка
        if (allowNotFound) {
            return Message();
        }
        return Message(STDLOG, _("Marketing schedule for %1% not found")).bind(tflt);
    }
    if (mans.size() > 1) {
        return Message(STDLOG, _("Lots of different marketings for %1% on %2%")).bind(tflt).bind(p);
    }
    return mans.front();
}

static void appendPointLeaps(
        std::set<boost::gregorian::date>& leaps,
        const Period& basePrd,
        const nsi::PointId& pt, const boost::posix_time::time_duration& tm
    )
{
    const std::string tmz = nsi::City(nsi::Point(pt).cityId()).timezone();
    const boost::gregorian::days offset = Dates::daysOffset(tm);
    const Period p = Period::normalize(basePrd.shift(offset));
    for (const boost::gregorian::date& d : getZoneLeapDates(tmz, p.start, p.end)) {
        leaps.emplace(d - offset);
    }
}

static Periods splitPeriodByDate(const Period& prd, const boost::gregorian::date& dt)
{
    if (prd.start > dt || prd.end < dt) {
        return { prd };
    }

    Periods out;
    if (dateFromPeriod(prd, dt)) {
        const Period p(dt, dt, Freq(dt, dt));
        out = Period::normalize(prd - p);
        out.push_back(p);
    } else {
        const Period x = Period::normalize(Period(prd.start, dt, prd.freq));
        if (!x.empty()) {
            out.push_back(x);
        }
        const Period y = Period::normalize(Period(dt, prd.end, prd.freq));
        if (!y.empty()) {
            out.push_back(y);
        }
    }
    return out;
}

Periods splitUtcScdByLeaps(const ScdRouteView& in)
{
    if (in.period.start == in.period.end) {
        return { in.period };
    }

    std::set<boost::gregorian::date> leaps;
    for (const Section& sc : in.route) {
        appendPointLeaps(leaps, in.period, sc.from, sc.dep);
        appendPointLeaps(leaps, in.period, sc.to, sc.arr);
    }

    Periods out = { in.period };
    for (const boost::gregorian::date& dt : leaps) {
        Periods ps;
        for (const Period& p : out) {
            const Periods t = splitPeriodByDate(p, dt);
            ps.insert(ps.end(), t.begin(), t.end());
        }
        out.swap(ps);
    }
    return out;
}

static ssim::ScdPeriods splitByLeaps(const ssim::ScdPeriod& in)
{
    std::vector<Section> scs;
    for (const ssim::Leg& lg : in.route.legs) {
        scs.push_back(lg.s);
    }

    ssim::ScdPeriods out;
    for (const Period& prd : splitUtcScdByLeaps(ScdRouteView{ in.period, scs})) {
        out.push_back(in);
        out.back().period = prd;
    }
    return out;
}

ssim::ScdPeriods toLocalTime(const ssim::ScdPeriod& in)
{
    //UTC -> LT
    ssim::ScdPeriods out = splitByLeaps(in);
    for (ssim::ScdPeriod& scp : out) {
        boost::gregorian::days offset(0);
        const boost::gregorian::date baseDt = scp.period.start;
        for (auto lgi = scp.route.legs.begin(); lgi != scp.route.legs.end(); ++lgi) {
            const auto tm = nsi::timeGmtToCity(boost::posix_time::ptime(baseDt, lgi->s.dep), lgi->s.from);
            if (lgi == scp.route.legs.begin()) {
                offset = tm.date() - baseDt;
            }
            const boost::posix_time::ptime anchor(baseDt + offset);
            lgi->s.dep = tm - anchor;
            lgi->s.arr = nsi::timeGmtToCity(boost::posix_time::ptime(baseDt, lgi->s.arr), lgi->s.to) - anchor;
        }
        scp.period = scp.period.shift(offset);
    }
    return out;
}

} //ssim
