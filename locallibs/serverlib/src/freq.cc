#include "freq.h"

#include <stdlib.h>
#include <boost/date_time/gregorian/gregorian_types.hpp>

#include "isdigit.h"
#include "dates.h"
#include "exception.h"

#define NICKNAME "NONSTOP"
#include "slogger.h"

boost::optional<Freq> freqFromStr(const std::string& str)
{
    if (str.find('/') != std::string::npos) {
        return boost::none;
    }
    return boost::optional<Freq>(Freq(str));
}

Freq logicalOr(const Freq& lhs, const Freq& rhs)
{
    const std::string lf(lhs.str()), rf(rhs.str());
    assert(lf.size() == 7 && rf.size() == 7);
    char buff[8] = ".......";
    for (unsigned i = 0; i < 7; ++i) {
        if (lf[i] != '.') {
            buff[i] = lf[i];
        } else if (rf[i] != '.') {
            buff[i] = rf[i];
        }
    }
    return Freq(buff);
}

static std::bitset<7> makeFreqDays(const std::string & fr)
{
    std::bitset<7> days;
    if( fr == "8" || fr == "…" || fr == "E" || fr == "…†„" || fr == "EVRD" ) {
        days.flip();
        return days;
    }
    const auto sz = fr.size();
    if (sz > 7) {
        throw comtech::Exception(BIGLOG, "invalid Freq string [" + fr + ']');
    }
    for (unsigned i = 0; i < sz && i < 7; ++i) {
        if ( fr[i] == '.') {
            continue;
        } else if (!ISDIGIT(fr[i]) || fr[i] == '0' || fr[i] == '8' || fr[i] == '9') {
            throw comtech::Exception(BIGLOG, "invalid Freq string [" + fr + ']');
        } else {
            days.set(fr[i] - '1', true);
        }
    }
    return days;
}

Freq::Freq()
{
    days_.flip();
}

Freq::Freq(const std::bitset<7>& days)
    : days_(days)
{
}

Freq::Freq(const std::string& fr)
{
    days_ = makeFreqDays(fr);
}

Freq::Freq(const boost::gregorian::date& date1, const boost::gregorian::date& date2)
{
    if (date2 - date1 >= boost::gregorian::days(7)) {
        days_.flip();
        return;
    }

    boost::gregorian::date_period prd(date1, date2 + boost::gregorian::days(1));

    for (boost::gregorian::day_iterator i = prd.begin(); i != prd.end(); ++i) {
        days_.set(Dates::day_of_week_ru(*i) - 1, true);
    }
}

bool Freq::empty() const
{
    return !days_.any();
}

bool Freq::full() const
{
    return days_.all();
}

std::string Freq::normalString() const
{
    std::stringstream ss;
    for (size_t i = 0; i < 7; ++i) {
        if (days_.test(i)) {
            ss << (i + 1);
        }
    }
    return ss.str();
}

std::string Freq::str() const
{
    std::stringstream ss;
    for (size_t i = 0; i < 7; ++i) {
        if (days_.test(i)) {
            ss << (i + 1);
        } else {
            ss << '.';
        }
    }
    return ss.str();
}

bool Freq::operator==(const Freq& fr) const
{
    return this->days_ == fr.days_;
}

bool Freq::operator<(const Freq& fr) const
{
    const auto lhsCount = days_.count();
    const auto rhsCount = fr.days_.count();
    if (lhsCount == rhsCount) {
        for (unsigned i = 0; i < 7; ++i) {
            const bool lhsVal = days_.test(i);
            const bool rhsVal = fr.days_.test(i);
            if (lhsVal != rhsVal) {
                return lhsVal > rhsVal;
            }
        }
    } else {
        return lhsCount < rhsCount;
    }
    return false;
}

bool Freq::operator>(const Freq& fr) const
{
    const auto lhsCount = days_.count();
    const auto rhsCount = fr.days_.count();
    if (lhsCount == rhsCount) {
        for (unsigned i = 0; i < 7; ++i) {
            const bool lhsVal = days_.test(i);
            const bool rhsVal = fr.days_.test(i);
            if (lhsVal != rhsVal) {
                return lhsVal < rhsVal;
            }
        }
    } else {
        return lhsCount > rhsCount;
    }
    return false;
}

Freq Freq::operator&(const Freq& fr) const
{
    std::bitset<7> days;
    for (unsigned i = 0; i < 7; ++i) {
        days.set(i, days_.test(i) && fr.days_.test(i));
    }
    return Freq(days);
}

Freq Freq::operator-(const Freq& fr) const
{
    std::bitset<7> days(days_);
    for (unsigned i = 0; i < 7; ++i) {
        if (days.test(i)) {
            days.set(i, !fr.days_.test(i));
        }
    }
    return Freq(days);
}

Freq Freq::shift(int delta) const
{
    if (delta == 0) {
        return *this;
    }
    const bool neg = (delta < 0);
    delta = (std::abs(delta) % 7);  //normalize offset
    if (neg) {
        delta = 7 - delta;
    }
    std::bitset<7> days;
    for (unsigned i = 0; i < 7; ++i) {
        if (days_.test(i)) {
            days.set((i + delta) % 7, true);
        }
    }
    return Freq(days);
}

std::ostream& operator<<(std::ostream& str, const Freq& fr)
{
    return str << fr.str();
}

bool Freq::hasDayOfWeek(unsigned short i) const
{
    return days_.test(i - 1);
}

