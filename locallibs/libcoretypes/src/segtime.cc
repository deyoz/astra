#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif

#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <serverlib/exception.h>
#include <serverlib/dates.h>

#include "segtime.h"

#define NICKNAME "NONSTOP"
#include <serverlib/slogger.h>

static bool IsValidDayChange(int dayChange)
{
    return dayChange >= -9 && dayChange <= 9;
}

static int DayChangeIndicator(const boost::gregorian::date& depDate, const boost::gregorian::date& arrDate)
{
    if (depDate == arrDate)
        return 0;
    else if (depDate < arrDate)
        return (int)boost::gregorian::date_period(depDate, arrDate).length().days();
    else
        return -(int)boost::gregorian::date_period(arrDate, depDate).length().days();
}

static std::string MakeDayChangeIndicatorText(int dayChange)
{
    std::ostringstream os;

    if (dayChange == 0)
        return std::string();
    else if (dayChange > 0)
        os << "/" << dayChange;
    else
        os << "/M" << -dayChange;

    return os.str();
}

namespace ct
{

SegTime::SegTime(const boost::posix_time::time_duration& depTime,
        const boost::posix_time::time_duration& arrTime, int dayChange):
    depTime_(depTime),
    arrTime_(arrTime),
    dayChange_(dayChange)
{
    ASSERT(!depTime.is_not_a_date_time());
    ASSERT(!arrTime.is_not_a_date_time());
    ASSERT(IsValidDayChange(dayChange));
}

boost::optional<SegTime> SegTime::create(const std::string& text)
{
    static const boost::regex re(" *([0-9]{4}) +([0-9]{4}) *(?: */ *(M)?([0-9]) *)?");
    boost::smatch m;
    if (!boost::regex_match(text, m, re))
        return boost::optional<SegTime>();

    /* Время вылета */
    const boost::posix_time::time_duration depTime = Dates::TimeFromHHMM(m[1]);
    if (depTime.is_not_a_date_time())
        return boost::optional<SegTime>();
    /* Время прибытия */
    const boost::posix_time::time_duration arrTime = Dates::TimeFromHHMM(m[2]);
    if (arrTime.is_not_a_date_time())
        return boost::optional<SegTime>();
    /* Индикатор смены даты */
    int dayChange = 0;
    if (m[4].matched) {
        dayChange = boost::lexical_cast<int>(m[4]);
        if (m[3].matched)
            dayChange *= -1;
    }
    if (!IsValidDayChange(dayChange))
        return boost::optional<SegTime>();

    return SegTime(depTime, arrTime, dayChange);
}

boost::optional<SegTime> SegTime::create(const boost::posix_time::time_duration& depTime,
        const boost::posix_time::time_duration& arrTime, int dayChange)
{
    if (depTime.is_not_a_date_time()) {
        return boost::optional<SegTime>();
    }
    if (arrTime.is_not_a_date_time()) {
        return boost::optional<SegTime>();
    }
    if (!IsValidDayChange(dayChange)) {
        return boost::optional<SegTime>();
    }
    return SegTime(depTime, arrTime, dayChange);
}

boost::optional<SegTime> SegTime::create(const boost::posix_time::ptime& depDateTime,
        const boost::posix_time::ptime& arrDateTime)
{
    if (depDateTime.is_not_a_date_time()) {
        return boost::optional<SegTime>();
    }
    if (arrDateTime.is_not_a_date_time()) {
        return boost::optional<SegTime>();
    }

    const boost::posix_time::time_duration depTime(depDateTime.time_of_day()),
        arrTime(arrDateTime.time_of_day());
    const int dayChange(DayChangeIndicator(depDateTime.date(), arrDateTime.date()));
    if (!IsValidDayChange(dayChange)) {
        return boost::optional<SegTime>();
    }
    return SegTime(depTime, arrTime, dayChange);
}


const boost::posix_time::time_duration& SegTime::dep() const
{
    return depTime_;
}

const boost::posix_time::time_duration& SegTime::arr() const
{
    return arrTime_;
}

int SegTime::dayChange() const
{
    return dayChange_;
}

bool SegTime::operator==(const SegTime& rhp) const
{
    return depTime_ == rhp.depTime_
        && arrTime_ == rhp.arrTime_
        && dayChange_ == rhp.dayChange_;
}

bool SegTime::operator!=(const SegTime& rhp) const
{
    return !(*this == rhp);
}

std::string SegTime::makeText() const
{
    return Dates::hhmi(depTime_)
        + ' '
        + Dates::hhmi(arrTime_)
        + MakeDayChangeIndicatorText(dayChange_);
}

std::ostream& operator<<(std::ostream& os, const SegTime& segTime)
{
    return os << segTime.makeText();
}

} // ct

// FIXME need tests from cls_segtime
