#include "period.h"
#include "period_joiner.h"

#include <set>
#include <bitset>

#include "string_cast.h"
#include "dates.h"
#include "tcl_utils.h"
#include "exception.h"
#include "helpcpp.h"

#define NICKNAME "NONSTOP"
#include "slogger.h"

#ifdef TRACE5
#undef TRACE5
#endif // TRACE5
#define TRACE5  getRealTraceLev(99),STDLOG

Period::Period() : biweekly(false) {}

Period::Period(const boost::gregorian::date& d1, const boost::gregorian::date& d2, const std::string& fr)
    : start(d1), end(d2), biweekly(false), freq(fr) {}

Period::Period(const boost::gregorian::date& dt)
    : start(dt), end(dt), biweekly(false), freq(dt, dt)
{}

boost::optional<Period> Period::create(
        const boost::gregorian::date& d1, const boost::gregorian::date& d2,
        const Freq & fr, bool biw
    )
{
    Period pr(d1, d2, fr);
    if (!biw || pr.empty()) {
        return pr;
    }
    if (!fr.hasDayOfWeek(Dates::day_of_week_ru(d1))) {
        LogTrace(TRACE1) << "bad start date : " << d1 << " is " << Dates::day_of_week_ru(d1);
        return boost::none;
    }
    pr.biweekly = true;
    return pr;
}

bool Period::empty() const
{
    if (start.is_not_a_date() || end.is_not_a_date() 
        || start > end || freq.empty() || start.is_pos_infinity() || end.is_neg_infinity()) {
        return true;
    }
    boost::gregorian::date dt = start, twoWeek = start + boost::gregorian::days(14);
    bool hasDay = false;
    for (; dt <= end && dt <= twoWeek; dt += boost::gregorian::days(1)) {
        if (dateFromPeriod(*this, dt)) {
            hasDay = true;
            break;
        }
    }
    if (hasDay)
        return false;
    return true;//nothing was found so frequency and period don't overlap
}

bool Period::full() const
{
    if (freq.full()) {
        return true;
    }
    if (start.is_infinity() || end.is_infinity()) {
        return false;
    }
    if ((end - start).days() < 7) {
        for (boost::gregorian::date d = start; d <= end; d += boost::gregorian::days(1)) {
            if (!dateFromPeriod(*this, d)) {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool operator<(const Period& lv, const Period& rv)
{
    return (lv.start < rv.start)
        || (lv.start == rv.start && lv.end < rv.end)
        || (lv.start == rv.start && lv.end == rv.end && lv.freq < rv.freq);
}

bool operator==(const Period& lv, const Period& rv)
{
    return (lv.start == rv.start && lv.end == rv.end && lv.freq == rv.freq && lv.biweekly == rv.biweekly);
}

bool operator!=(const Period& lv, const Period& rv)
{
    return !( lv == rv );
}

// просто поменять для циклических периодов нельзя -- может потеряться инфа о том, что неделя с этим днем пропускается
void Period::changeStart(const boost::gregorian::date & dt)
{
    if (!this->biweekly) {
        this->start = dt;
        return;
    } 
    boost::gregorian::date st(dt);
    for (; st <= this->end && !dateFromPeriod(*this, st); st += boost::gregorian::days(1)) {}
    this->start = st;
}
// для циклических периодов -- сделать активными неделями те, что пропускались, а те, что были активными - пропускать
void Period::biweekInvert()
{
    if (!this->biweekly)
        return;
    assert(dateFromPeriod(*this, this->start));

    // находим начало следующей недели
    boost::gregorian::date newStart = this->start + boost::gregorian::days(7 - Dates::day_of_week_ru(this->start) + 1);
    // ок, нашли. теперь -- двигаемся до первого дня, который пропускается не из-за частоты, а из-за biweek
    while (newStart <= this->end && !dateFromPeriod(*this, newStart, false))
        newStart += boost::gregorian::days(1);
    // он и будет началом инвертированного периода
    this->start = newStart;
}

Period Period::shift(boost::gregorian::days d) const
{
    if (d.days() == 0 || empty()) {
        return *this;
    }
    return *Period::create(
        (start.is_neg_infinity() ? start : start + d),
        (end.is_pos_infinity() ? end : end + d),
        this->freq.shift(d.days()), this->biweekly
    );
}

Period operator&(const Period& p1, const Period& p2)
{
    boost::gregorian::date start = std::max(p1.start, p2.start);
    boost::gregorian::date end = std::min(p1.end, p2.end);
    Freq intersect(p1.freq & p2.freq);
    if (intersect.empty())
        return Period();
    if (!p1.biweekly && !p2.biweekly)
        return Period(start, end, intersect);
    // подрезаем начало и конец до тех пор, пока не попадем по частоте и цикличности
    for (; start <= end && !(dateFromPeriod(p1, start) && dateFromPeriod(p2, start)); start += boost::gregorian::days(1)) {}
    for (; end >= start && !(dateFromPeriod(p1, end) && dateFromPeriod(p2, end)); end -= boost::gregorian::days(1)) {}
    // может случиться, что дорежем до одного дня (end). так что create может вернуть пустой optional
    // например при пересечении периодов
    //  пвсчпсвп     в 
    // [.......1......] и 
    // [1......-------] (второй циклический) получим такую ситуацию : start == end == вск
    if (auto res = Period::create(start, end, intersect, true)) {
        return *res;
    }
    return Period();
}

Periods operator-(const Period& lp, const Period& rp)
{
    Periods ps;
    Period const ip = (lp & rp); // intersection period
    if (ip.empty()) {
        ps.push_back(lp);
    } else {
        if (ip.start >= lp.start) {
            Period p(lp);
            p.end = ip.start - boost::gregorian::days(1);
            if (!p.empty())
                ps.push_back(p);
        }
        if (ip.end <= lp.end) {
            Period p(lp);
            p.changeStart(ip.end + boost::gregorian::days(1));
            if (!p.empty())
                ps.push_back(p);
        }
        if (ip.start >= lp.start && ip.end <= lp.end) {
            Period p(lp);
            p.freq = lp.freq - ip.freq;
            p.end = ip.end;
            p.changeStart(ip.start);
            if (!p.empty())
                ps.push_back(p);
            // а еще если w2 в пересечении, то надо добавить каждый второй (пропущенный) блок
            if (ip.biweekly) {
                Period missed(ip);
                // нужно сдвинуть начало на неделю -- тогда активными будут как раз те недели, что были пропущены
                // при этом пересечение (если циклическое) -- точно правильное, т.е. первый день активный
                assert(dateFromPeriod(ip, ip.start));
                // частота и признак цикличности скопированы, а конец периода нам не особо важен
                missed.biweekInvert();
                missed = missed & lp;
                if (!missed.empty()) {
                    ps.push_back(missed);
                }
            }
        }
    }
    return ps;
}

std::ostream& operator<<(std::ostream& str, const Period& p)
{
    str << "[" << p.start << " " << p.end << " ";
    str << p.freq;
    str << "]";
    if (p.biweekly)
        str << "[W2]";
    return str;
}

static boost::gregorian::date firstDayOfPeriod(const Period& p)
{
    if (p.start.is_not_a_date() || p.end.is_not_a_date() 
        || p.start > p.end || p.freq.empty() || p.start.is_pos_infinity() || p.end.is_neg_infinity()) {
        return boost::gregorian::date();
    }

    boost::gregorian::date dt = p.start, twoWeek = p.start + boost::gregorian::days(14);
    for (; dt <= p.end && dt <= twoWeek; dt += boost::gregorian::days(1)) {
        if (p.freq.hasDayOfWeek(Dates::day_of_week_ru(dt))) {
            return dt;
        }
    }
    return boost::gregorian::date();
}

bool dateFromPeriod(const Period& p, const boost::gregorian::date& dt, bool checkBiweek)
{
    if (dt >= p.start && dt <= p.end) {
        if (dt.is_infinity()) {
            return true;
        }
        const bool freqMatch = p.freq.hasDayOfWeek(Dates::day_of_week_ru(dt));

        if (p.biweekly && checkBiweek && freqMatch) {
            // get first date of period 
            const boost::gregorian::date firstDate(firstDayOfPeriod(p));
            if (!firstDate.is_not_a_date()) {
                const int firstDateDayOfWeek(Dates::day_of_week_ru(firstDate) - 1);
                return ((dt - firstDate).days() + firstDateDayOfWeek) % 14 < 7;
            }
        } else {
            return freqMatch;
        }
    }
    return false;
}

std::string Period::periodForMask() const
{
    return Dates::ddmmrr(start) + ',' + Dates::ddmmrr(end) + ',' + freq.str();
}

static Period normalizeEdgeDates(const Period &p)
{
    assert(!p.start.is_not_a_date());
    assert(!p.end.is_not_a_date());
    boost::gregorian::date start = p.start, end = p.end;
    // режем начало пока не встретим актуальный день
    if (!start.is_special()) { //может быть бесконечность
        for (; start <= end && !dateFromPeriod(p, start); start += boost::gregorian::days(1)) {}
    }
    // то же самое с концом
    if (!end.is_special()) {
        for (; end >= start && !dateFromPeriod(p, end); end -= boost::gregorian::days(1)) {}
    }
    Period ret(p);
    ret.start = start;
    ret.end = end;
    if ((end - start).days() <= 7)
      ret.biweekly = false;
    return ret;
}

static Period normalizeBiweeklyPeriod(const Period& p)
{
    assert(p.biweekly);
    boost::gregorian::date start, end, dt;

    // найдем начало периода
    start = firstDayOfPeriod(p);
    if (start.is_not_a_date()) {
        return Period();
    }

    if (p.end.is_pos_infinity()) {
        return *Period::create(start, p.end, p.freq, true);
    }

    // найдем окончание периода
    short index = Dates::day_of_week_ru(start); //Mon is 1 !
    boost::gregorian::date w2begin(start - boost::gregorian::date_duration(index - 1));
    for (boost::gregorian::date dt = p.end; dt >= start; ) {
        if ((dt - w2begin).days() % 14 < 7) {
            if (p.freq.hasDayOfWeek(Dates::day_of_week_ru(dt))) {
                end = dt;
                break;
            }
        }
        dt -= boost::gregorian::date_duration(1);
    }

    // нормализуем частоту
    std::bitset<7> days;
    int weekCounter = 0;
    for (boost::gregorian::date dt = start; dt <= end; ) {
        if ((dt - w2begin).days() % 14 >= 7) {
            dt += boost::gregorian::date_duration(7);
            ++weekCounter;
        }
        else {
            if (weekCounter == 2) { // 2 weeks check is enough
                break;
            }
            const short index = Dates::day_of_week_ru(dt);
            if( p.freq.hasDayOfWeek(index) ) {
                days.set(index - 1);
            }
            dt += boost::gregorian::date_duration(1);
        }
    }

    // период в результате может оказаться обычным
    const bool biweekly((end - w2begin).days() /14 > 0);

    return *Period::create(start, end, Freq(days), biweekly);
}

static Period normalizeFreq(const Period &p)
{
    assert(!p.start.is_not_a_date());
    assert(!p.end.is_not_a_date());
    if (p.start.is_special() || p.end.is_special())
        return p;
    boost::gregorian::date d = p.start, 
        weekFromStart = p.start + boost::gregorian::date_duration(7);
    //if period is longer than week, there's no need to check entire period
    //so if p.end > weekFromStart, this is trivial copying of p.freq
    if ((p.end - p.start).days() > 7)
        return p;
    //if not, days not included in period'll be dropped from frequency.
    std::bitset<7> days;
    while( d <= p.end && d <= weekFromStart ) {
        const short index = Dates::day_of_week_ru(d); //Mon is 1 !
        if( p.freq.hasDayOfWeek(index) ) {
            days.set(index - 1);
        }
        d = d + boost::gregorian::date_duration(1);
    }
    return Period(p.start, p.end, Freq(days));
}

Period Period::normalize(const Period &p)
{
    if (p.empty())
        return Period();
    return p.biweekly ? normalizeBiweeklyPeriod(p) : normalizeEdgeDates(normalizeFreq(p));
}

Periods Period::normalize(const Periods & ps)
{
    Periods res;
    for( const Period & p:  ps ) {
        res.push_back(Period::normalize(p));
    }
    return res;
}


static unsigned countDays(const Period& period)
{
    using boost::gregorian::days;

    Period tmp(period);
    if (period.end.is_pos_infinity()) {
        static const boost::gregorian::date maxDate(boost::gregorian::date(2049,12, 31));
        tmp.end = maxDate;
    }
    tmp = Period::normalize(tmp);

    LogTrace(TRACE5) << "countDays: tmp=" << tmp;
    if (tmp.empty())
        return 0;

    if ((tmp.end - tmp.start).days() < 7)
        return tmp.freq.normalString().size();

    const unsigned daysInWeek(tmp.freq.normalString().size());

    Period aligned(Period::normalize(Period(tmp.start, tmp.end, Freq("1"))).start,
                   Period::normalize(Period(tmp.start, tmp.end, Freq("7"))).end, Freq("1.....7"));
    LogTrace(TRACE5) << "aligned=" << aligned;
    unsigned fullWeeks((aligned.end - aligned.start + days(1)).days() / 7);
    const Period left(Period::normalize(Period(tmp.start, aligned.empty() ? aligned.end : (aligned.start - days(1)), tmp.freq)));
    const Period right(Period::normalize(Period(aligned.empty() ? aligned.start : (aligned.end + days(1)), tmp.end, tmp.freq)));
    LogTrace(TRACE5) << "left = " << left << left.freq.normalString();
    LogTrace(TRACE5) << "right = " << right << right.freq.normalString();
    const unsigned delta(((aligned.start > tmp.start) ? left.freq.normalString().size() : 0) +
                         ((aligned.end < tmp.end) ? right.freq.normalString().size() : 0));
    if (tmp.biweekly)
        fullWeeks = aligned.start > tmp.start ? fullWeeks/2 : fullWeeks/2 + fullWeeks%2;

    LogTrace(TRACE5) << "daysInWeek=" << daysInWeek << " fullWeeks=" << fullWeeks << " delta=" << delta;
    return daysInWeek * fullWeeks + delta;
}

// пытается объединить непересекающиеся периоды
Period Period::join(const Period& lhs, const Period& rhs, bool allowBiweek)
{
    Period left(Period::normalize(lhs)), right(Period::normalize(rhs));
    if ((left & right).empty()) {
        LogTrace(TRACE5) << "try to join: "<< left << " - "<< right;
        boost::gregorian::date start = std::min(left.start, right.start);
        boost::gregorian::date end = std::max(left.end, right.end);
        const unsigned totalDays(countDays(left) + countDays(right));
        // trying biweekly at first "false", then "true":
        for (bool biw : { false, true }) {
            if (!allowBiweek && biw) {
                break;
            }

            LogTrace(TRACE5) << "biweekly: "<<biw;
            boost::optional<Period> p(Period::create(start, end, logicalOr(left.freq, right.freq), biw));
            if (p) {
                Period res(Period::normalize(p.get()));
                if (res == p.get()) {
                    if (totalDays==countDays(res) &&
                        left==Period::normalize(left & res) &&
                        right==Period::normalize(right & res)) {
                        // join is unsuccessfull if resulted period is biweekly and hasn't single freq
                        if (isOneDayFreqForBiweeklyPeriods() && res.biweekly && res.freq.normalString().size() != 1) {
                            return Period();
                        }
                        return res;
                    }
                }
            }
        }
    }
    return Period();
}

Period Period::bounds(const Periods& pss)
{
    if (pss.empty()) {
        return Period();
    }

    Period bound = Period::normalize(pss.front());
    for(const Period& p:  pss) {
        Period tmp(Period::normalize(p));
        bound = Period(
            std::min(bound.start, tmp.start),
            std::max(bound.end, tmp.end),
            logicalOr(bound.freq, tmp.freq)
        );
    }
    return Period::normalize(bound);
}

Periods Period::intersect(const Periods &lhs, const Periods &rhs)
{
    Periods ps;
    for(const Period& lp:  lhs) {
        for(const Period& rp:  rhs) {
            Period intersection = (lp & rp);
            if (!intersection.empty()) {
                ps.push_back(intersection);
            }
        }
    }
    return ps;
}

Periods Period::difference( Periods const &lhs, Periods const &rhs )
{
    Periods ret( lhs );
    for( Period const &rp:  rhs )
    {
        Periods tmp;
        for( Period const &tp:  ret )
        {
            Periods const ps( tp - rp );
            tmp.insert( tmp.end(), ps.begin(), ps.end() );
        }
        ret = tmp;
    }
    return ret;
}

static Periods symmetric_difference(const Period& x, const Period& y)
{
    Periods ld = Period::normalize(x - y);
    ld.erase(std::remove_if(ld.begin(), ld.end(), std::bind(&Period::empty, std::placeholders::_1)), ld.end());

    Periods rd = Period::normalize(y - x);
    rd.erase(std::remove_if(rd.begin(), rd.end(), std::bind(&Period::empty, std::placeholders::_1)), rd.end());

    std::set<Period> out(ld.begin(), ld.end());
    out.insert(rd.begin(), rd.end());
    return Periods(out.begin(), out.end());
}

Periods Period::combine(const Periods& lhs, const Periods& rhs) // FIXME: without grain
{
    if (Period::intersect(lhs, rhs).empty()) {
        std::set<Period> res(lhs.begin(), lhs.end());
        res.insert(rhs.begin(), rhs.end());
        return Periods(res.begin(), res.end());
    }

    std::set<Period> res;

    const Periods gr = Period::normalize(Period::grain(lhs, rhs));
    res.insert(gr.begin(), gr.end());

    const Periods gr2 = Period::normalize(Period::grain(rhs, lhs));
    res.insert(gr2.begin(), gr2.end());

    bool has_intersections = false;
    do {
        has_intersections = false;
        const Periods pss(res.begin(), res.end());
        for (Periods::const_iterator i = pss.begin(); i != pss.end(); ++i) {
            for (Periods::const_iterator j = HelpCpp::advance(i, 1); j != pss.end(); ++j) {
                const Period t = (*i & *j);
                if (!t.empty()) {
                    const Periods parts = symmetric_difference(*i, *j);
                    res.erase(*i);
                    res.erase(*j);
                    res.insert(parts.begin(), parts.end());
                    res.insert(Period::normalize(t));
                    has_intersections = true;
                }
            }
        }
    } while (has_intersections);

    return Periods(res.begin(), res.end());
}


static Periods grain(const Period& lhs, const Period& rhs)
{
    Periods ps;
    Period ip = lhs & rhs;
    if (ip.empty()) {
        ps.push_back(Period::normalize(lhs));
    } else {
        ps.push_back(Period::normalize(ip));
        for (const Period& p : Period::normalize(lhs - rhs)) {
            if (!p.empty()) {
                ps.push_back(p);
            }
        }
    }
    return ps;
}

static Periods innerGrain(size_t recLevel, const Period& p, const Periods& ps, size_t idx)
{
    const std::string indent(recLevel * 2, ' ');
    if (idx >= ps.size()) {
        LogTrace(TRACE5) << indent << "stop at " << idx;
        return Periods{p};
    }
    LogTrace(TRACE5) << indent << "grain " << p << " ps[" << idx << "] " << ps[idx];
    Periods res;
    for (const Period& gp : grain(p, Period::normalize(ps[idx]))) {
        Periods gps = innerGrain(recLevel + 1, gp, ps, idx + 1);
        res.insert(res.end(), gps.begin(), gps.end());
    }
    if (res.empty()) {
        res.push_back(p);
    }
    for (const Period& pr : res) {
        LogTrace(TRACE5) << indent << pr;
    }
    return res;
}

Periods Period::grain(const Periods &lhs, const Periods &rhs)
{
    std::set<Period> ps;
    for (const Period& lp:  lhs) {
        Periods gps = innerGrain(1, Period::normalize(lp), rhs, 0);
        ps.insert(gps.begin(), gps.end());
    }
    return Periods{ps.begin(), ps.end()};
}

Periods splitBiweekPeriod(const Period& in)
{
    assert(in.biweekly);

    Periods out;
    if (in.freq.normalString().size() != 1) {
        //several days per week - split by weeks
        for (boost::gregorian::date dt = boost::date_time::previous_weekday(in.start, boost::gregorian::greg_weekday(boost::date_time::Monday));
             dt <= in.end;)
        {
            const Period p = Period::normalize(Period(dt, dt + boost::gregorian::days(6)) & in);
            if (!p.empty()) {
                out.push_back(p);
            }
            dt += boost::gregorian::days(7);
        }
    } else {
        //one day per week - split by days
        for (const boost::gregorian::date& dt : PeriodStl(in)) {
            out.emplace_back(dt, dt, Freq(dt, dt));
        }
    }
    return out;
}

PeriodStl::Iterator &PeriodStl::Iterator::operator=( Iterator const &it ) 
{
    p_ = it.p_;
    np_ = it.np_;
    dt_ = it.dt_;
    return *this;
}

static boost::gregorian::date nextDate( boost::gregorian::date const &dt, Period const &p )
{
    static boost::gregorian::days const one( 1 );
    boost::gregorian::date ret( dt + one );

    while ( !p.freq.hasDayOfWeek( Dates::day_of_week_ru( ret ) ) )
        ret += one;

    if (p.biweekly) {
        assert(dateFromPeriod(p, p.start));
        while (true) {
            unsigned dayIn2week = ((ret-p.start).days() + Dates::day_of_week_ru(p.start) - 1) % 14;
            if (dayIn2week < 7) {
                return ret;
            }
            else {
                do { ret += one; }
                while ( !p.freq.hasDayOfWeek( Dates::day_of_week_ru( ret ) ) );
            }
        }
    }
    return ret;
}

PeriodStl::Iterator &PeriodStl::Iterator::operator++()
{
    if( !dt_.is_not_a_date() )
    {
        dt_ = nextDate( dt_, np_ );
        if( dt_ > np_.end )
            dt_ = boost::gregorian::date();
    }
    return *this;
}

PeriodStl::Iterator PeriodStl::Iterator::operator++( int )
{
    Iterator tmp( *this );
    ++( *this );
    return tmp;
}

boost::gregorian::date nextDate( boost::gregorian::date const &dt, Freq const &fr )
{
    static boost::gregorian::days const one( 1 );
    boost::gregorian::date ret( dt + one );

    while( !fr.hasDayOfWeek( Dates::day_of_week_ru( ret ) ) )
        ret += one;
    return ret;
}

bool PeriodStl::Iterator::operator==( Iterator const &it ) const
{
    assert( p_ == it.p_ );
    return dt_.is_not_a_date() && it.dt_.is_not_a_date() ? true : dt_ == it.dt_;
}


period::period(const boost::gregorian::date& b, const boost::gregorian::date& e, const frequency& f)
    : beg(b), end(e), fr(f) {}

bool period::intersects(const period& p) const
{
  using namespace boost::gregorian;
  using namespace std;

  if(!fr.intersects(p.fr))  return false;
  date_period in = date_period(beg,end).intersection(date_period(p.beg,p.end));
  if(in.length()<=days(0))  return false;
  frequency f = fr.intersection(p.fr);
  for(date d=in.begin(); d!=in.end() && d-in.begin() < days(7); d+=days(1))
      if(f.includes(d.day_of_week().as_number()))
          return true;
  return false;
}

DayRange::DayRange(const boost::gregorian::date& d)
    : start(d), end(d)
{
}

DayRange::DayRange(const boost::gregorian::date& b, const boost::gregorian::date& e)
    : start(b), end(e)
{
}

bool operator==(const DayRange& lhs, const DayRange& rhs)
{
    return lhs.start == rhs.start && lhs.end == rhs.end;
}

bool operator<(const DayRange& lhs, const DayRange& rhs)
{
    return lhs.start == rhs.start ? lhs.end < rhs.end : lhs.start < rhs.start;
}

std::ostream& operator<<(std::ostream& os, const DayRange& dr)
{
    if (dr.start == dr.end)
        return os << "[ " << dr.start <<" ]";
    return os << "[" << dr.start << " " << dr.end << "]";
}

bool isOneDayFreqForBiweeklyPeriods()
{
#ifndef XP_TESTING
    static
#endif
    int isW2HasOneDayFreq(readIntFromTcl("FREQ_W2_WITH_SINGLE_DAY_OF_WEEK", 1));

    return isW2HasOneDayFreq == 1;
}

//#############################################################################
#define TOO_LARGE_WEEK_OFFSET 3

static int weekOffset(const boost::gregorian::date& dt, const boost::gregorian::date& prv)
{
    const int d = (dt - prv).days();
    if (d < 0 || d > 14) {
        return TOO_LARGE_WEEK_OFFSET;
    }
    return (d / 7) + (Dates::day_of_week_ru(prv) > Dates::day_of_week_ru(dt) ? 1 : 0);
}

class JoinHelper
{
    bool allowBiweek;
    Periods ps;

    static Freq makeFreqUntilWeekday(unsigned short weekDay)
    {
        ASSERT(weekDay > 1);
        std::string s(".......");
        for (unsigned short i = 1; i < weekDay; ++i) {
            s[i] = i + '0';
        }
        return Freq(s);
    }

    static bool isSuitableDate(const Period& p, const boost::gregorian::date& dt)
    {
        const boost::gregorian::days oneDay(1);
        const boost::gregorian::days oneWeek(7);

        if (dt - p.end <= oneWeek) {
            if (dt - p.start < oneWeek) {
                return true;
            }
            if (p.freq.hasDayOfWeek(Dates::day_of_week_ru(dt))) {
                if (dt - p.end == oneDay) {
                    return true;
                }
                if ((p.freq & Freq(p.end + oneDay, dt - oneDay)).empty()) {
                    return true;
                }
            } else if (p.biweekly && dt - p.start < boost::gregorian::weeks(2)) {
                return true;
            }
        }
        return false;
    }

    Period* findVariant(const boost::gregorian::date& dt)
    {
        if (!allowBiweek) {
            if (Period* p = (ps.empty() ? nullptr : &ps.back())) {
                if (isSuitableDate(*p, dt)) {
                    return p;
                }
            }
            return nullptr;
        }

        const boost::gregorian::days twoWeek(14);
        const unsigned short weekDay = Dates::day_of_week_ru(dt);
        const bool oneDayFreq = isOneDayFreqForBiweeklyPeriods();
        for (auto i = ps.rbegin(), e = ps.rend(); i != e; ++i) {
            Period& p = *i;

            const int week_offset = weekOffset(dt, p.end);
            //week_offset can be:
            //  for normal period: [0, 1, (2)] (2 is a trigger for change normal period to w2)
            //  for w2 period (without restrictions): [0, 2]
            //  for w2 period (with 1-day frequency restriction): [2]

            if (week_offset >= TOO_LARGE_WEEK_OFFSET) {
                continue;
            }

            if (p.biweekly) {
                if ((oneDayFreq && week_offset != 2) || (!oneDayFreq && week_offset != 0 && week_offset != 2)) {
                    continue;
                }
            }

            if (week_offset == 2) {
                if (!p.biweekly && weekOffset(p.end, p.start) != 0) {
                    continue;
                }

                if (oneDayFreq) {
                    if (!p.freq.hasDayOfWeek(weekDay) || p.freq.normalString().size() != 1) {
                        continue;
                    }
                } else if (dt - p.start >= twoWeek) {
                    if (!p.freq.hasDayOfWeek(weekDay)) {
                        continue;
                    }
                    if (weekDay != 1 && !(p.freq & makeFreqUntilWeekday(weekDay)).empty()) {
                        continue;
                    }
                }
                p.biweekly = true;
                return &p;
            }

            if ((week_offset == 0 || week_offset == 1) && isSuitableDate(p, dt)) {
                return &p;
            }
        }
        return nullptr;
    }

public:
    explicit JoinHelper(bool biw) : allowBiweek(biw) {}

    void append(const boost::gregorian::date& dt)
    {
        if (Period* p = findVariant(dt)) {
            p->end = dt;
            p->freq = logicalOr(p->freq, Freq(dt, dt));
        } else {
            ps.emplace_back(Period(dt, dt, Freq(dt, dt)));
        }
    }

    Periods periods() const {
        return Period::normalize(ps);
    }
};

boost::optional<Periods> makePeriodsOpt(const std::set<boost::gregorian::date>& dts, bool allowBiweek)
{
    JoinHelper jh(allowBiweek);
    for (const boost::gregorian::date& dt : dts) {
        jh.append(dt);
    }

    const Periods out = jh.periods();

    size_t cnt = 0;
    for (const Period& p : out) {
        cnt += countDays(p);
    }
    if (cnt != dts.size()) {
#ifdef XP_TESTING
        ASSERT(cnt == dts.size() && "Failed to make periods");
#else
        LogError(STDLOG) << "Failed to make periods: " << cnt << " vs " << dts.size();
#endif
        return boost::none;
    }
    return out;
}

Periods makePeriods(const std::set<boost::gregorian::date>& dts, bool allowBiweek)
{
    const auto ps = makePeriodsOpt(dts, allowBiweek);
    ASSERT(ps && "Failed to make periods");
    return *ps;
}

//#############################################################################

#ifdef XP_TESTING
#include "checkunit.h"
#include "str_utils.h"

void init_period_tests()
{
}

namespace {
using namespace boost::gregorian;
static void print(const char* l, const Periods& ps)
{
    for(const Period& p:  ps) {
        LogTrace(TRACE5) << l << " " << p;
    }
}

static date getStartDate()
{
    // always start with nearest Monday
    date dt = day_clock::local_day();
    while (Dates::day_of_week_ru(dt) != 1
            || dt.day() != 1) {
        dt += days(1);;
    }
    return dt;
}

static Period makePeriod(int s, int e, const char* f)
{
    const date sd = getStartDate();
    return Period(sd + days(s), sd + days(e), f);
}

START_TEST(check_date_from_period)
{
    using boost::gregorian::date;
    const date startDate = getStartDate();

    Period biwPeriod(startDate + days(2), startDate + days(19), "12457");
    biwPeriod.biweekly = true;
    const int resDays[] = { 3, 4, 6, 14, 15, 17, 18 };
    std::set<int> daysFromPeriod(resDays, resDays + 7);
    for (int i = 0; i < 19; ++i) {
        if (daysFromPeriod.find(i) != daysFromPeriod.end())
            fail_unless(dateFromPeriod(biwPeriod, startDate + days(i)), "Day %d should belong to period", i);
        else
            fail_if(dateFromPeriod(biwPeriod, startDate + days(i)), "Day %d shouldn't belong to period", i);
    }
}
END_TEST

START_TEST(check_biweek_invert)
{
    using boost::gregorian::date;
    const date startDate = getStartDate();
    Period p1 = Period(startDate + days(2), startDate + days(18), Freq("35"));
    Period p2(p1);
    p2.biweekInvert();
    fail_if(p1 != p2);

    Period biwPeriod = *Period::create(startDate + days(2), startDate + days(18), Freq("35"), true);
    biwPeriod.biweekInvert();
    fail_unless(biwPeriod == *Period::create(startDate + days(9), startDate + days(18), Freq("35"), true));
    fail_unless(Period::normalize(biwPeriod) == Period(startDate + days(9), startDate + days(11), "35"));

    biwPeriod = *Period::create(startDate, startDate + days(20), Freq("13457"), true);
    biwPeriod.biweekInvert();
    fail_unless(biwPeriod == *Period::create(startDate + days(7), startDate + days(20), Freq("13457"), true));
    fail_unless(Period::normalize(biwPeriod) == Period(startDate + days(7), startDate + days(13), "13457"));

    biwPeriod = *Period::create(startDate, startDate + days(13), Freq("13457"), true);
    biwPeriod.biweekInvert();
    fail_unless(biwPeriod == *Period::create(startDate + days(7), startDate + days(13), Freq("13457"), true));
    fail_unless(Period::normalize(biwPeriod) == Period(startDate + days(7), startDate + days(13), "13457"));

}
END_TEST

START_TEST(check_intersect11)
{
    const date startDate = getStartDate();
    Periods p1;
    p1.push_back(Period(startDate, startDate + days(10), "1234567"));
    Periods p2;
    p2.push_back(Period(startDate + days(3), startDate + days(7), "12.4.67"));

    Periods pi = Period::intersect(p1, p2);
    print("intersect", pi);
    fail_unless(pi == p2);
}
END_TEST

START_TEST(check_intersect12)
{
    const date startDate = getStartDate();

    Periods p1;
    p1.push_back(Period(startDate, startDate + days(10), "123457"));

    Periods p2;
    p2.push_back(Period(startDate + days(2), startDate + days(5), "12467"));
    p2.push_back(Period(startDate + days(7), startDate + days(9), "35"));
    
    Periods inter;
    inter.push_back(Period(startDate + days(2), startDate + days(5), "1247"));
    inter.push_back(Period(startDate + days(7), startDate + days(9), "35"));

    Periods pi = Period::intersect(p1, p2);
    print("intersect", pi);
    fail_unless(pi == inter);
}
END_TEST

START_TEST(check_intersect21)
{
    const date startDate = getStartDate();
    Periods p1;
    p1.push_back(Period(startDate + days(2), startDate + days(4), ".23.567"));
    p1.push_back(Period(startDate + days(5), startDate + days(10), "1234567"));

    Periods p2;
    p2.push_back(Period(startDate, startDate + days(11), "12467"));

    Periods inter;
//    inter.push_back(Period(startDate + days(2), startDate + days(4), "267"));
    inter.push_back(Period(startDate + days(5), startDate + days(10), "12467"));

    Periods pi = Period::intersect(p1, p2);
    print("intersect", pi);
    fail_unless(pi == inter);
}
END_TEST

START_TEST(check_intersect22_w_int)
{
    const date startDate = getStartDate();
    Periods p1;
    p1.push_back(Period(startDate + days(2), startDate + days(5), ".23...7"));
    p1.push_back(Period(startDate + days(7), startDate + days(10), "1..4.6."));

    Periods p2;
    p2.push_back(Period(startDate, startDate + days(3), "123"));
    p2.push_back(Period(startDate + days(4), startDate + days(8), "1234567"));

    Periods inter;
    inter.push_back(Period(startDate + days(2), startDate + days(3), "23"));
//    inter.push_back(Period(startDate + days(4), startDate + days(5), "237")); // empty
    inter.push_back(Period(startDate + days(7), startDate + days(8), "146"));

    Periods pi = Period::intersect(p1, p2);
    print("intersect", pi);
    fail_unless(pi == inter);
}
END_TEST

START_TEST(check_intersect22_wo_int)
{
    const date startDate = getStartDate();
    Periods p1;
    p1.push_back(Period(startDate, startDate + days(2), "3..2..7"));
    p1.push_back(Period(startDate + days(6), startDate + days(9), "146"));

    Periods p2;
    p2.push_back(Period(startDate + days(4), startDate + days(5), "123"));
    p2.push_back(Period(startDate + days(10), startDate + days(12), "1234567"));

    Periods inter;

    Periods pi = Period::intersect(p1, p2);
    print("intersect", pi);
    fail_unless(pi == inter);
}
END_TEST

START_TEST(check_biweek_intersect)
{
    const date startDate = getStartDate();
    Period p1 = *Period::create(startDate, startDate + days(23), Freq("123...."), true); //4 тройки пн-вт-ср
    Period p2(startDate + days(6), startDate + days(10), "123....");
    Period inter = p1 & p2; // пересечение попадает на пропуск из-за w2
    fail_unless(inter.empty());
    p2 = Period(startDate + days(14), startDate + days(20), ".2.....");
    inter = p1 & p2; // нормальное пересечение, но только по 1 дню недели
    Period ref = *Period::create(startDate + days(15), startDate + days(15), Freq(".2....."), true);
    LogTrace(TRACE5) << inter << " <> " << ref;
    fail_unless(inter == ref);
    p2 = Period(startDate + days(1), startDate + days(25), Freq("123...."));
    inter = p1 & p2; // пересечение с выпадающим первым понедельником: 23-...-123-...-123-... etc
    ref = *Period::create(startDate + days(1), startDate + days(16), Freq("123...."), true); 
    //вообще здесь 4 тройки, но последняя выпадает из-за цикличности и поэтому подрезается
    LogTrace(TRACE5) << inter << " <> " << ref;
    fail_unless(inter == ref);
    // проверка транзитивности
    inter = p2 & p1;
    LogTrace(TRACE5) << inter << " <> " << ref;
    fail_unless(inter == ref);
    //и еще надо два периода с провалами пересечь
    p2 = *Period::create(startDate + days(7), startDate + days(23), Freq("1.3...."), true); //2 пары пн-ср
    inter = p1 & p2;
    fail_unless(inter.empty()); //попадаем на провал
    // и последнее -- два провальных с реальным пересечением
    p2 = *Period::create(startDate + days(16), startDate + days(23), Freq("..3...."), true); //2 ср
    inter = p1 & p2;
    ref = *Period::create(startDate + days(16), startDate + days(16), Freq("..3...."), true);
    LogTrace(TRACE5) << inter << " <> " << ref;
    fail_unless(inter == ref); //попадаем на провал
}
END_TEST

START_TEST(check_grain_simple)
{
    //day:  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21
    // p1:  1  2  3  4  5  6  7  1  2  3  4  5  6  7  1  2  3  4  5  6  7
    // p2:     2  3  4  5  6  7  1  2
    // p2:                                   5  6  7  1  2  3  4  5
    const Periods p1 = {
        makePeriod(0, 20, "1234567")
    };
    const Periods p2 = {
        makePeriod(1, 8, "1234567"),
        makePeriod(11, 18, "1234567")
    };

    const Periods pg = Period::grain(p1, p2);
    const Periods samplePg = {
        makePeriod(0, 0, "1"),
        makePeriod(1, 8, "1234567"),
        makePeriod(9, 10, "34"),
        makePeriod(11, 18, "1234567"),
        makePeriod(19, 20, "67")
    };
    print("grain    res", pg);
    print("grain sample", samplePg);
    fail_unless(samplePg == pg);
}END_TEST

START_TEST(check_grain11)
{
    //day:  1  2  3  4  5  6  7  8  9 10 11
    // p1:  1  2  3  4  5  6  7  1  2  3  4
    // p2:           -  -  6  7  1
    const Periods p1 = {
        makePeriod(0, 10, "1234567")
    };
    const Periods p2 = {
        makePeriod(3, 7, "12...67")
    };

    const Periods pg = Period::grain(p1, p2);
    const Periods samplePg = {
        makePeriod(0, 4, "12345"),
        makePeriod(5, 7, "1....67"),
        makePeriod(8, 10, "234")
    };
    print("grain    res", pg);
    print("grain sample", samplePg);
    fail_unless(samplePg == pg);
}END_TEST

START_TEST(check_grain12)
{
    //day:  1  2  3  4  5  6  7  8  9 10 11
    // p1:  1  2  3  4  5  -  7  1  2  3  4
    // p2:        -  4  -  6
    // p2:                       -  -  3
    const Periods p1 = {
        makePeriod(0, 10, "12345.7")
    };
    const Periods p2 = {
        makePeriod(2, 5, "12467"),
        makePeriod(7, 9, "35")
    };

    const Periods pg = Period::grain(p1, p2);
    const Periods samplePg = {
        makePeriod(0, 2, "123...."),
        makePeriod(3, 3, "...4..."),
        makePeriod(4, 4, "....5.."),
        makePeriod(6, 8, "12....7"),
        makePeriod(9, 9, "..3...."),
        makePeriod(10, 10, "...4...")
    };
    print("grain    res", pg);
    print("grain sample", samplePg);
    fail_unless(samplePg == pg);
}
END_TEST

START_TEST(check_grain21)
{
    //day:  1  2  3  4  5  6  7  8  9 10 11 12
    // p1:        3  -  5
    // p1:                 6  7  1  2  3  4
    // p2:  1  2  -  4  -  6  7  1  2  -  4  -
    const Periods p1 = {
        makePeriod(2, 4, ".23.567"),
        makePeriod(5, 10, "1234567")
    };
    const Periods p2 = {
        makePeriod(0, 11, "12467")
    };

    const Periods pg = Period::grain(p1, p2);
    const Periods samplePg = {
        makePeriod(2, 4, "3.5"),
        makePeriod(5, 10, "12.4.67"),
        makePeriod(9, 9, "3")
    };
    print("grain    res", pg);
    print("grain sample", samplePg);
    fail_unless(samplePg == pg);
}
END_TEST

START_TEST(check_grain22_w_int)
{
    //day:  1  2  3  4  5  6  7  8  9 10 11
    // p1:        3  -  -  -
    // p1:                       1  -  -  4
    // p2:  1  2  3  -
    // p2:              5  -  7  1  2
    const Periods p1 = {
        makePeriod(2, 5, ".23...7"),
        makePeriod(7, 10, "1..4.6.")
    };
    const Periods p2 = {
        makePeriod(0, 3, "123...."),
        makePeriod(4, 8, "12345.7")
    };

    const Periods pg = Period::grain(p1, p2);
    const Periods samplePg = {
        makePeriod(2, 2, "3"),
        makePeriod(7, 7, "1"),
        makePeriod(10, 10, "4")
    };
    print("grain    res", pg);
    print("grain sample", samplePg);
    fail_unless(samplePg == pg);
}END_TEST

START_TEST(check_grain22_wo_int)
{
    //day:  1  2  3  4  5  6  7  8  9 10 11 12 13
    // p1:  -  2  3
    // p1:                    -  1  -  -
    // p2:              -  -
    // p2:                                -  -  -
    const Periods p1 = {
        makePeriod(0, 2, "..23..7"),
        makePeriod(6, 9, "146")
    };
    const Periods p2 = {
        makePeriod(4, 5, "123"),
        makePeriod(10, 12, "1234567")
    };

    const Periods pg = Period::grain(p1, p2);
    const Periods samplePg = {
        makePeriod(1, 2, "23"),
        makePeriod(7, 7, "1")
    };
    print("grain    res", pg);
    print("grain sample", samplePg);
    fail_unless(samplePg == pg);
}END_TEST

START_TEST(check_freq)
{
    fail_unless(Freq().full(), "default is full freq");
    fail_unless(Freq("").empty(), "empty string must lead to empty frequency");

    auto f = freqFromStr("1234567");
    fail_unless(static_cast<bool>(f) == true, "freqFromStr failed: 1234567");
    fail_unless(f->full(), "expected full Freq");
    f = freqFromStr("1234567/2");
    fail_unless(static_cast<bool>(f) == false, "freqFromStr parsed invalid str");
    f = freqFromStr("123/2");
    fail_unless(static_cast<bool>(f) == false, "freqFromStr parsed invalid str");
    try {
        Freq("1234567/2");
        fail_if(true, "expected exception due to bad string: 1234567/2");
    } catch (const comtech::Exception&) {
    }
    try {
        Freq("123/2");
        fail_if(true, "expected exception due to bad string: 123/2");
    } catch (const comtech::Exception&) {
    }

    fail_unless(Freq("37") < Freq("125"));
    fail_unless(Freq("125") > Freq("37"));
    fail_unless(Freq("136") < Freq("245"));
    fail_unless(Freq("245") > Freq("136"));

    Freq fr1("12...67"), fr2("..345.."), fr3;
    LogTrace(TRACE5) << fr3 - fr1;;
    fail_unless((fr3 - fr1) == fr2);
    LogTrace(TRACE5) << fr3 - fr2;;
    fail_unless((fr3 - fr2) == fr1);
    Freq fr4(".23...7"), fr5("123...."), fr6("......7"), fr7(".23....");
    LogTrace(TRACE5) << (fr4 & fr5);
    fail_unless((fr4 & fr5) == fr7);
    LogTrace(TRACE5) << (fr4 - fr5);
    fail_unless((fr4 - fr5) == fr6);

    fail_unless(Freq("137").shift(-1) == Freq("267"));
    fail_unless(Freq("137").shift(1) == Freq("124"));

    fail_unless(Freq("27").shift(4) == Freq("46"));
    fail_unless(Freq("27").shift(-4) == Freq("35"));

    fail_unless(Freq("27").shift( 4 + 7*2) == Freq("46"));
    fail_unless(Freq("27").shift(-(4 + 7*2)) == Freq("35"));

    fail_unless(Freq("137").shift(10*7) == Freq("137"));
    fail_unless(Freq("137").shift(-10*7) == Freq("137"));
}END_TEST

START_TEST(check_grain12_2int)
{
    const date startDate = getStartDate() + days(2);
    Periods p1;
    p1.push_back(Period(startDate, startDate + days(20), Freq()));
    Periods p2;
    p2.push_back(Period(startDate + days(1), startDate + days(10), Freq()));
    p2.push_back(Period(startDate + days(1), startDate + days(5), Freq()));
    
    Periods pg = Period::grain(p1, p2);
    Periods samplePg;
    samplePg.push_back(Period(startDate, startDate, Freq()));
    samplePg.push_back(Period(startDate + days(1), startDate + days(5), Freq()));
    samplePg.push_back(Period(startDate + days(6), startDate + days(10), Freq()));
    samplePg.push_back(Period(startDate + days(11), startDate + days(20), Freq()));
    samplePg = Period::normalize(samplePg);
    std::sort(samplePg.begin(), samplePg.end());
    print("grain    res", pg);
    print("grain sample", samplePg);
    fail_unless(samplePg == pg);
}END_TEST

START_TEST(check_normalize)
{
    using boost::gregorian::date;

    date start = getStartDate();
    Period test1(start, start, "234");
    Period test2(start, start + days(6), "345");
    Period test3(start + days(3), start + days(6), "123456");
    Period test4(start, start + days(12), "237");
    Period norm1 = normalizeEdgeDates(test1);
    Period norm2 = normalizeEdgeDates(test2);
    Period norm3 = normalizeFreq(test3);
    Period norm4 = Period::normalize(test4);
    fail_unless(norm1.empty(), "One-day-period Monday should be empty");
    fail_unless(norm2.start == start + days(2) && norm2.end == start + days(4), "Period shoud've been shrinked");
    fail_unless(norm3.freq.normalString() == "456", "Frequency is %s should've been 456", norm3.freq.normalString().c_str());
    LogTrace(TRACE5) << "Before " << test4 << " After " << norm4;
    fail_unless(norm4.freq.normalString() == "237" && norm4.start == start + days(1) && norm4.end == start + days(9), 
            "Big period normalized incorrectly");

    // check biweekly period normalization
    Period biwPeriod = *Period::create(start, start + days(30), Freq("127"), true);
    fail_unless(Period::normalize(biwPeriod) ==  *Period::create(start, start + days(29), Freq("127"), true));

    biwPeriod = *Period::create(date(2013, 6, 21), date(2013, 6, 30), Freq("1234567"), true);
    fail_unless(Period::normalize(biwPeriod) == Period(date(2013, 6, 21), date(2013, 6, 23), "567"));

    biwPeriod = Period(date(2014, 9, 27), date(2014, 10, 13), Freq("2345"));
    biwPeriod.biweekly = true;
    fail_unless(Period::normalize(biwPeriod) == Period(date(2014, 9, 30), date(2014, 10, 3), "2345"));
}
END_TEST

START_TEST( checkPerStl )
{
    using boost::gregorian::date;
    auto get_tres = [](const auto& p){
        const PeriodStl pstl(p);
        std::vector<date> tres((p.end - p.start).days());
        auto e = std::copy(pstl.begin(), pstl.end(), tres.begin());
        tres.erase(e, tres.end());
        return tres;
    };
    const date start = getStartDate();
    {
    auto tres = get_tres(Period( start, start + days( 16 ), "17" ));
    std::vector< date > gres(5);
    gres[0] = start;
    gres[1] = start + days( 6 );
    gres[2] = start + days( 7 );
    gres[3] = start + days( 13 );
    gres[4] = start + days( 14 );
    fail_unless( tres == gres );
    }
    {
    auto tres = get_tres(Period::create( start + days(6), start + days( 20 ), Freq("17"), true ).get());
    std::vector< date > gres(3);
    gres[0] = start + days( 6 );
    gres[1] = start + days( 14 );
    gres[2] = start + days( 20 );
    fail_unless( tres == gres );
    }
    {
    auto tres = get_tres( *Period::create(date(2013, 11, 1), date(2013, 11, 29), Freq("5"), true));
    std::vector< date > gres(3);
    gres[0] = date( 2013, 11, 1 );
    gres[1] = date( 2013, 11, 15 );
    gres[2] = date( 2013, 11, 29 );
    fail_unless( tres == gres );
    }
}
END_TEST

START_TEST(period_minus)
{
    using boost::gregorian::date;

    {
        Period const rp( date( 2010, 10, 10 ), date( 2010, 10, 20 ), "135" );
        Period const lp( date( 2010, 10, 12 ), date( 2010, 10, 18 ), "356" );
        Periods const res( lp - rp );
        fail_unless( res.size() == 1 && res.front() == Period( date( 2010, 10, 12 ), date( 2010, 10, 18 ), "6" ), 
                "check period minus 1" );
    }

    {
        Period const rp( date( 2010, 10, 10 ), date( 2010, 10, 20 ), "34" );
        Period const lp( date( 2010, 10, 5 ), date( 2010, 10, 15 ), "1356" );
        Periods res( lp - rp );
        std::sort( res.begin(), res.end() );
        Periods orig;
        orig.push_back( Period( date( 2010, 10, 5 ), date( 2010, 10, 9 ), "1356" ) );
        orig.push_back( Period( date( 2010, 10, 10 ), date( 2010, 10, 15 ), "156" ) );
        fail_unless( res == orig, "check minus 2" );
    }

    {
        Period const rp( date( 2010, 10, 5 ), date( 2010, 10, 15 ), "267" );
        Period const lp( date( 2010, 10, 10 ), date( 2010, 10, 20 ), "137" );
        Periods res( lp - rp );
        std::sort( res.begin(), res.end() );
        Periods orig;
        orig.push_back( Period( date( 2010, 10, 10 ), date( 2010, 10, 15 ), "13" ) );
        orig.push_back( Period( date( 2010, 10, 16 ), date( 2010, 10, 20 ), "137" ) );
        fail_unless( res == orig, "check minus 3" );
    }

    {
        Period const rp( date( 2010, 10, 10 ), date( 2010, 10, 15 ), "12347" );
        Period const lp( date( 2010, 10, 5 ), date( 2010, 10, 20 ), "467" );
        Periods res( lp - rp );
        std::sort( res.begin(), res.end() );
        Periods orig;
        orig.push_back( Period( date( 2010, 10, 5 ), date( 2010, 10, 9 ), "467" ) );
        orig.push_back( Period( date( 2010, 10, 16 ), date( 2010, 10, 20 ), "467" ) );
        fail_unless( res == orig, "check minus 4" );
    }
}
END_TEST

START_TEST(pos_neg_inf)
{
    using boost::gregorian::pos_infin;
    using boost::gregorian::neg_infin;
    using boost::gregorian::date;
    const Period p1(date(2010, 10, 12), date(pos_infin), "135");
    const Period p2(date(neg_infin), date(2010, 10, 15), "237");
    const Period lp(date(2010, 10, 12), date(2010, 10, 18), "356");
    const Periods minus1(p1 - lp);

    fail_unless(minus1.size() == 2, "bad size %zd", minus1.size());
    fail_unless(minus1.front() == Period(date(2010, 10, 19), date(pos_infin), "135"), "check period minus 1");
    fail_unless(minus1.back() == Period(date(2010, 10, 12), date(2010, 10, 18), "1"), "check period minus 2");

    const Periods minus2(p2 - lp);
    fail_unless(minus2.size() == 2, "bad size %zd", minus2.size());
    fail_unless(minus2.front() == Period(date(neg_infin), date(2010, 10, 11), "237"), "check period minus 3");
    fail_unless(minus2.back() == Period(date(2010, 10, 12), date(2010, 10, 15), "27"), "check period minus 4");

    const Period inter1(p1 & lp), inter2(p2 & lp);
    fail_unless(inter1 == Period(date(2010, 10, 12), date(2010, 10, 18), "35"), "intersect 1");
    fail_unless(inter2 == Period(date(2010, 10, 12), date(2010, 10, 15), "3"), "intersect 2");

    fail_unless(Period::normalize(p1) == Period(date(2010,10,13), date(pos_infin), "135"), "normalize 1");
    fail_unless(Period::normalize(p2) == Period(date(neg_infin), date(2010,10,13),"237"), "normalize 2");

    const Period big(date(neg_infin), date(pos_infin), "127");
    const Periods minus3(big - lp);
    fail_unless(minus3.size() == 1 && minus3.front() == big, "minus 5");

    fail_unless((p1 & p2) == Period(date(2010,10,12), date(2010,10,15), "3"), "bad intersect 3");

    const Periods minus4(p1 - big);
    fail_unless(minus4.size() == 1 && minus4.front() == Period(date(2010, 10, 12), date(pos_infin), "35"), "minus 6");
}
END_TEST

START_TEST(check_biweek)
{
    const date startDate = getStartDate();
    Period p1 = *Period::create(startDate, startDate + days(16), Freq("123...."), true);
    Period p2(startDate, startDate + days(10), "12345..");
    
    Periods minus(p2 - p1);
    print("test", minus);
    fail_unless(minus.size() == 1);
    fail_unless(minus.front() == *Period::create(startDate + days(3), startDate + days(10), Freq("12345.."), false));
    
    minus = p1 - p1;
    fail_unless(minus.empty());

    minus = p1 - p2;
    print("test", minus);
    fail_unless(minus.size() == 1);
    fail_unless(minus.front() == *Period::create(startDate + days(14), startDate + days(16), Freq("123...."), true));

    p1 = Period(startDate + days(1), startDate + days(17), Freq());
    p2 = *Period::create(startDate, startDate + days(16), Freq("123...."), true);
    minus = p1 - p2;
    print("test", minus);
    fail_unless(minus.size() == 3);

    minus = join(Period::normalize(minus));
    Periods tmps;
    tmps.push_back(Period(startDate + days(3), startDate + days(13), Freq()));
    tmps.push_back(Period(startDate + days(17), startDate + days(17), Freq("...4...")));

    print("test", minus);
    fail_unless(minus.size() == 2);
    fail_unless(minus == tmps);

    p1 = Period::create( startDate, startDate + days(42), Freq("1234567"), true ).get();
    minus = p1 - p1;
    fail_unless(minus.empty());

    p1 = *Period::create(date(2013, 7, 3), date(2013, 7, 20), Freq("1234567"), true);
    p2 = Period(date(2013, 7, 8), date(2013, 7, 14), "234567");
    minus = p1 - p2;
    fail_unless(minus.size() == 1);
    fail_unless(minus.front() == p1);

    p1 = *Period::create(date(2013, 6, 21), date(2013, 8, 4), Freq("1234567"), true);
    p2 = *Period::create(date(2013, 7, 1), date(2013, 8, 1), Freq("1234567"), true);

    minus = Period::normalize(p1 - p2);
    print("test", minus);
    fail_unless(minus.size() == 2);
    fail_unless(minus.front() == Period(date(2013, 6, 21), date(2013, 6, 23),"567"));
    fail_unless(minus.back() == Period(date(2013, 8, 2), date(2013, 8, 4),"567"));

    p1 = Period(date(2014, 9, 12), date(2014, 11, 23), Freq("1567"));
    p2 = *Period::create(date(2014, 9, 11), date(2014, 11, 23), Freq("1234567"), true);

    setTclVar("FREQ_W2_WITH_SINGLE_DAY_OF_WEEK", "1");
    minus = join(Period::normalize(p2 - p1));
    tmps.clear();
    tmps.push_back(Period(date(2014, 9, 11), date(2014, 9, 11), Freq("...4...")));
    tmps.push_back(*Period::create(date(2014, 9, 23), date(2014, 11, 20), Freq(".234..."), true));
    fail_unless(minus == tmps);

    setTclVar("FREQ_W2_WITH_SINGLE_DAY_OF_WEEK", "0");
    minus = join(Period::normalize(p2 - p1));
    fail_unless( minus.size() == 1 );
    fail_unless( minus.front() == *Period::create(date(2014, 9, 11), date(2014, 11, 20), Freq("234"), true) );

    p1 = *Period::create(date(2014, 10, 6), date(2014, 12, 16), Freq("12"), true);
    p2 = *Period::create(date(2014, 10, 6), date(2014, 12, 17), Freq("123"), true);

    minus = join(Period::normalize(p2 - p1));
    fail_unless( minus.size() == 1 );
    fail_unless( minus.front() == *Period::create(date(2014, 10, 8), date(2014, 12, 17), Freq("3"), true) );

    // bound для biweek-периодов
    tmps.clear();
    tmps.push_back(*Period::create(startDate, startDate + days(10), Freq("123...."), true)); // на самом деле это три дня из-за w2
    tmps.push_back(Period(startDate, startDate + days(7), "...45.."));
    Period bnd = Period::bounds(tmps);
    LogTrace(TRACE5) << bnd;
    fail_unless(bnd == Period(startDate, startDate + days(4), Freq("12345..")));
}
END_TEST

START_TEST(check_count_days)
{
    using boost::gregorian::date;
    const date startDate = getStartDate();

    Period p1(startDate + days(1), startDate + days(18), "347");
    fail_unless(countDays(p1) == 8);

    Period biwPeriod = *Period::create(startDate + days(6), startDate + days(19), Freq("127"), true);
    fail_unless(countDays(biwPeriod) == 3);

    biwPeriod = *Period::create(startDate, startDate + days(30), Freq("127"), true);
    fail_unless(countDays(biwPeriod) == 8);

    biwPeriod = *Period::create(startDate + days(1), startDate + days(30), Freq("127"), true);
    fail_unless(countDays(biwPeriod) == 7);

}
END_TEST

START_TEST(check_can_join)
{
#define PERIOD(start,end,freq) Period(d1+days(start), d1+days(end), freq)
#define W2PERIOD(start,end,freq) Period::create(d1+days(start), d1+days(end), Freq(freq), true).get()

#define CHECK_CAN_JOIN(p1, p2, p3) { \
    Period joined(Period::join(p1, p2)); \
    LogTrace(TRACE5) << "join: p1=" << p1 << " p2=" << p2 << " joined=" << joined; \
    if (joined.empty()) { \
        fail_unless(0, "failed join [%s] [%s]", p1.periodForMask().c_str(), p2.periodForMask().c_str()); \
    } \
    fail_unless(joined.start == p3.start, "failed join: invalid start"); \
    fail_unless(joined.end == p3.end, "failed join: invalid end"); \
    fail_unless(joined.freq == p3.freq, "failed join: invalid freq"); \
    fail_unless(joined.biweekly == p3.biweekly, "failed join: invalid biweekly"); \
}
#define CHECK_CANNOT_JOIN(p1, p2) { \
    Period joined(Period::join(p1, p2)); \
    LogTrace(TRACE5) << "join: p1=" << p1 << " p2=" << p2 << " joined=" << joined; \
    fail_unless(joined.empty(), "invalid join [%s] [%s]", p1.periodForMask().c_str(), p2.periodForMask().c_str()); \
}

    const date d1 = getStartDate();
    Period p1, p2, p3;

    CHECK_CAN_JOIN( PERIOD(0,  13, "1234567"),   PERIOD(14,  20, "1234567"),   PERIOD(0, 20, "1234567") );
    CHECK_CAN_JOIN( PERIOD(14, 20, "1234567"),   PERIOD( 0,  13, "1234567"),   PERIOD(0, 20, "1234567") );
    CHECK_CAN_JOIN( PERIOD(0,   2, "123"),       PERIOD( 3,   6, "4567"),      PERIOD(0,  6, "1234567") );
    CHECK_CAN_JOIN( PERIOD(0,   7, "123"),       PERIOD( 1,   6, "4567"),      PERIOD(0,  7, "1234567") );
    CHECK_CAN_JOIN( PERIOD(0,  15, "1234567"),   PERIOD( 16, 26, "1234567"),   PERIOD(0, 26, "1234567") );
    CHECK_CAN_JOIN( PERIOD(0,   6, "7"),         PERIOD( 7,   9, "1"),         PERIOD(6,  7, "17") );

    CHECK_CANNOT_JOIN( PERIOD(0, 9, "123"), PERIOD(10, 13, "4567") );
    CHECK_CANNOT_JOIN( PERIOD(0, 2, "123"), PERIOD(10, 13, "4567") );
    CHECK_CANNOT_JOIN( PERIOD(0, 2, "123"), PERIOD( 2,  6, "34567") );
    CHECK_CANNOT_JOIN( PERIOD(0, 6, "135"), PERIOD( 2, 10, "267") );

    // biweekly related:
{
    setTclVar("FREQ_W2_WITH_SINGLE_DAY_OF_WEEK", "0");
    CHECK_CAN_JOIN( W2PERIOD(1,  15, "2"),   W2PERIOD(29, 43, "2"),   W2PERIOD(1, 43, "2") );
    CHECK_CAN_JOIN(   PERIOD(0,  6, "1234567"),   PERIOD(14, 18, "12345"),   W2PERIOD(0, 18, "1234567") );
    CHECK_CAN_JOIN( W2PERIOD(0, 18, "12345"),     PERIOD(28, 29, "12"),      W2PERIOD(0, 29, "12345") );
    CHECK_CAN_JOIN(   PERIOD(0,  6, "1234567"), W2PERIOD(14, 29, "1234567"), W2PERIOD(0, 29, "1234567") );
    CHECK_CAN_JOIN( W2PERIOD(0, 18, "12345"),     PERIOD( 7, 11, "12345"),     PERIOD(0, 18, "12345") );
    CHECK_CAN_JOIN( W2PERIOD(0, 18, "12345"),   W2PERIOD(28, 43, "12345"),   W2PERIOD(0, 43, "12345") );
    CHECK_CAN_JOIN( W2PERIOD(0, 18, "12345"),   W2PERIOD( 7, 25, "12345"),     PERIOD(0, 25, "12345") );
    CHECK_CAN_JOIN( Period::create(date(2013,6,5), date(2013,6,22),Freq("3456"),true).get(),
                    Period(date(2013,6,12), date(2013,6,15),"3456"), Period(date(2013,6,5), date(2013,6,22),"3456") );

    CHECK_CANNOT_JOIN( W2PERIOD(0, 18, "1345"),  W2PERIOD(28, 43, "12345") );
    CHECK_CANNOT_JOIN(   PERIOD(0,  9, "123"),     PERIOD(10, 13, "4567") );
    CHECK_CANNOT_JOIN( W2PERIOD(0, 18, "12345"), W2PERIOD( 7, 25, "1245") );
    CHECK_CANNOT_JOIN( W2PERIOD(0, 18, "12345"),   PERIOD(21, 35, "12345") );

    CHECK_CANNOT_JOIN(Period(date(2013, 6, 25), date(2013, 7, 4),"246"),
                      Period(date(2013, 7, 20), date(2013, 7, 23),"246") );
}
{
    setTclVar("FREQ_W2_WITH_SINGLE_DAY_OF_WEEK", "1");
    CHECK_CAN_JOIN(   W2PERIOD(1,  15, "2"),   W2PERIOD(29, 43, "2"),   W2PERIOD(1, 43, "2") );
    CHECK_CANNOT_JOIN(   PERIOD(0,  6, "1234567"),   PERIOD(14, 18, "12345") );
    CHECK_CANNOT_JOIN( W2PERIOD(0, 18, "12345"),     PERIOD(28, 29, "12") );
    CHECK_CANNOT_JOIN(   PERIOD(0,  6, "1234567"), W2PERIOD(14, 29, "1234567") );
    CHECK_CAN_JOIN( W2PERIOD(0, 18, "12345"),     PERIOD( 7, 11, "12345"),     PERIOD(0, 18, "12345") );
    CHECK_CANNOT_JOIN( W2PERIOD(0, 18, "12345"),   W2PERIOD(28, 43, "12345") );
    CHECK_CAN_JOIN( W2PERIOD(0, 18, "12345"),   W2PERIOD( 7, 25, "12345"),     PERIOD(0, 25, "12345") );
    CHECK_CAN_JOIN( Period::create(date(2013,6,5), date(2013,6,22),Freq("3456"),true).get(),
                    Period(date(2013,6,12), date(2013,6,15),"3456"), Period(date(2013,6,5), date(2013,6,22),"3456") );

    CHECK_CANNOT_JOIN( W2PERIOD(0, 18, "1345"),  W2PERIOD(28, 43, "12345") );
    CHECK_CANNOT_JOIN(   PERIOD(0,  9, "123"),     PERIOD(10, 13, "4567") );
    CHECK_CANNOT_JOIN( W2PERIOD(0, 18, "12345"), W2PERIOD( 7, 25, "1245") );
    CHECK_CANNOT_JOIN( W2PERIOD(0, 18, "12345"),   PERIOD(21, 35, "12345") );

    CHECK_CANNOT_JOIN(Period(date(2013, 6, 25), date(2013, 7, 4),"246"),
                      Period(date(2013, 7, 20), date(2013, 7, 23),"246") );
}
{
    Periods ps = { Period(date(2014, 11, 25), date(2015,  3, 24), "2"),
                   Period(date(2014, 11, 27), date(2014, 12, 11), "4"),
                   Period(date(2014, 12, 18), date(2014, 12, 25), "4"),
                   Period(date(2015,  1,  1), date(2015,  1,  8), "4"),
                   Period(date(2015,  1, 15), date(2015,  3, 26), "4") };

    fail_unless(join(ps) == Periods(1, Period(date(2014, 11, 25), date(2015, 3, 26), "24")));
}
    {
        Periods ps = {
            Period(date(2017, 12, 31), date(2017, 12, 31), "7"),
            Period(date(2018,  1,  4), date(2018,  1, 11), "4"),
            Period(date(2018,  1, 14), date(2018,  1, 18), "47"),
            Period(date(2018,  1, 21), date(2018,  1, 21), "7"),
        };

        Periods res = {
            Period(date(2017, 12, 31), date(2018,  1,  4), "47"),
            Period(date(2018,  1, 11), date(2018,  1, 21), "47")
        };

        fail_unless(join(ps) == res);
    }

} END_TEST

static Period periodFromStr(const std::string& periodStr)
{
    std::size_t delimPos = periodStr.find("-");
    assert(delimPos != std::string::npos);

    std::size_t freqSlash = periodStr.find("/", delimPos + 1);
    assert(freqSlash != std::string::npos);

    std::string startStr = periodStr.substr(0, delimPos);
    std::string endStr = periodStr.substr(delimPos + 1, freqSlash - delimPos - 1);

    std::size_t bwSlash = periodStr.find("/", freqSlash + 1);
    std::string freqStr = periodStr.substr(freqSlash + 1
                                           , bwSlash == std::string::npos ? bwSlash : bwSlash - freqSlash - 1);

    std::string bwStr;
    if (bwSlash != std::string::npos) {
        bwStr = periodStr.substr(bwSlash + 1);
    }

    return *Period::create(
        Dates::DateFromYYYYMMDD(startStr),
        (endStr == "+infinity" ? boost::gregorian::date(boost::gregorian::pos_infin) : Dates::DateFromYYYYMMDD(endStr)),
        Freq(freqStr == "8" ? "1234567" : freqStr),
        !bwStr.empty()
    );
}

START_TEST(check_grain_bad_data)
{
    Periods ps1 = {
        periodFromStr("20151202-20151207/13"),
        periodFromStr("20151209-20160323/13")
    };

    Periods ps2 = {
        periodFromStr("20151202-20160323/13"),
        periodFromStr("20160309-20160309/3"),
        periodFromStr("20160314-20160314/1"),
        periodFromStr("20160302-20160302/3"),
        periodFromStr("20160307-20160307/1"),
        periodFromStr("20160316-20160316/3"),
        periodFromStr("20160321-20160321/1"),
        periodFromStr("20160323-20160323/3"),
        periodFromStr("20160210-20160210/3"),
        periodFromStr("20160208-20160208/1"),
        periodFromStr("20160203-20160203/3"),
        periodFromStr("20160217-20160217/3"),
        periodFromStr("20160215-20160215/1"),
        periodFromStr("20160222-20160222/1"),
        periodFromStr("20160229-20160229/1"),
        periodFromStr("20160224-20160224/3"),
        periodFromStr("20160201-20160201/1"),
        periodFromStr("20151228-20151228/1"),
        periodFromStr("20151230-20151230/3"),
        periodFromStr("20160104-20160104/1"),
        periodFromStr("20160106-20160106/3"),
        periodFromStr("20160111-20160111/1"),
        periodFromStr("20160127-20160127/3"),
        periodFromStr("20160125-20160125/1"),
        periodFromStr("20160120-20160120/3"),
        periodFromStr("20160118-20160118/1"),
        periodFromStr("20160113-20160113/3"),
        periodFromStr("20151223-20151223/3"),
        periodFromStr("20151221-20151221/1"),
        periodFromStr("20151216-20151216/3"),
        periodFromStr("20151214-20151214/1"),
        periodFromStr("20151209-20151209/3")
    };

    Periods ps = Period::grain(ps1, ps2);
} END_TEST

START_TEST(check_multi_join)
{
    struct Case {
        std::vector<const char*> src;
        std::vector<const char*> dst;
        bool allowBiweek;
        bool w2_single_day;
    };

    const std::vector< Case > cases = {
        Case {
            {
                "20161030-20161227/12467",
                "20161102-20170104/35",
                "20161229-20170103/124",
                "20161231-20170226/467",
                "20170106-20170228/1235",
                "20170301-20170325/1234567"
            },
            {
                "20161030-20170325/1234567"
            },
            true, false
        },
        Case {
            {
                "20161107-20161113/125",
                "20161122-20161122/2"
            },
            {
                "20161107-20161113/125",
                "20161122-20161122/2"
            },
            true, false
        },
        Case {
            {
                "20161107-20161113/125",
                "20161121-20161121/1",
                "20161125-20161125/5"
            },
            {
                "20161107-20161121/125/W2",
                "20161125-20161125/5"
            },
            true, false
        },
        Case {
            {
                "20161107-20161113/125",
                "20161121-20161121/1",
                "20161125-20161125/5"
            },
            {
                "20161107-20161111/125",
                "20161121-20161125/15"
            },
            true, true
        },
        Case {
            {
                "20161103-20161103/4",
                "20161104-20161104/5",
                "20161109-20161109/3",
                "20161112-20161112/6",
                "20161120-20161121/17",
                "20161124-20161126/456",
                "20161107-+infinity/1",
                "20161108-+infinity/2"
            },
            {
                "20161103-20161109/12345",
                "20161112-20161115/126",
                "20161120-20161126/124567",
                "20161128-+infinity/12",
            },
            false, false
        },
        Case {
            {
                "20161107-+infinity/1",
                "20161129-+infinity/25",
                "20161109-+infinity/3"
            },
            {
                "20161107-20161128/13",
                "20161129-+infinity/1235"
            },
            true, false
        },
        Case {
            {
                "20161107-20161107/1",
                "20161121-20161121/1",
                "20161125-20161125/5",
                "20161205-20161205/1"
            },
            {
                "20161107-20161205/1/W2",
                "20161125-20161125/5"
            },
            true, true
        },
        Case {
            {
                "20161103-20161208/4",
                "20161222-20161229/4"
            },
            {
                "20161103-20161208/4",
                "20161222-20161229/4"
            },
            true, true
        },
        Case {
            {
                "20161103-20161208/4",
                "20161222-20161222/4",
                "20170105-20170105/4"
            },
            {
                "20161103-20161208/4",
                "20161222-20170105/4/W2"
            },
            true, true
        },
    };

    size_t cn = 0;
    for (const auto& c : cases) {
        LogTrace(TRACE1) << "start case " << ++cn;

        if (c.allowBiweek) {
            setTclVar("FREQ_W2_WITH_SINGLE_DAY_OF_WEEK", std::to_string(static_cast<int>(c.w2_single_day)));
        } else {
            setTclVar("FREQ_W2_WITH_SINGLE_DAY_OF_WEEK", "0");
        }

        Periods dst, src;
        for (const auto& v : c.dst) dst.push_back(periodFromStr(v));
        for (const auto& v : c.src) src.push_back(periodFromStr(v));

        CHECK_EQUAL_STRINGS(StrUtils::join("\n", dst), StrUtils::join("\n", joinByDates(src, c.allowBiweek)));
    }
}
END_TEST

START_TEST(check_week_offset)
{
    const auto cdt = Dates::currentDate();
    const auto edt = cdt + boost::gregorian::years(3);

    for (boost::gregorian::date dt = cdt; dt < edt; dt = dt + boost::gregorian::days(1)) {
        const int dow = Dates::day_of_week_ru(dt);

        const size_t we1 = (7 - dow);
        const size_t we2 = we1 + 7;
        for (size_t i = 0; i < 14; ++i) {
            const int wo = (i <= we1 ? 0 : (i <= we2 ? 1 : 2));
            fail_unless(weekOffset(dt + boost::gregorian::days(i), dt) == wo);
        }
    }
}
END_TEST

#define SUITENAME "Periods"
TCASEREGISTER(0, 0)
{
    ADD_TEST(check_freq)

    ADD_TEST(check_date_from_period);
    ADD_TEST(check_biweek_invert);
    ADD_TEST(check_intersect11);
    ADD_TEST(check_intersect12);
    ADD_TEST(check_intersect21);
    ADD_TEST(check_intersect22_w_int);
    ADD_TEST(check_intersect22_wo_int);
    ADD_TEST(check_biweek_intersect);

    ADD_TEST(check_grain_simple);
    ADD_TEST(check_grain11);
    ADD_TEST(check_grain12);
    ADD_TEST(check_grain21);
    ADD_TEST(check_grain22_w_int);
    ADD_TEST(check_grain22_wo_int);
    ADD_TEST(check_grain12_2int);
    ADD_TEST(check_grain_bad_data);

    ADD_TEST(check_normalize);
    ADD_TEST(checkPerStl)

    ADD_TEST(period_minus);
    ADD_TEST(pos_neg_inf);
    ADD_TEST(check_biweek);
    ADD_TEST(check_count_days);
    ADD_TEST(check_can_join);
    ADD_TEST(check_multi_join);
    ADD_TEST(check_week_offset);
}
TCASEFINISH
} // namespace

#endif /* XP_TESTING */
