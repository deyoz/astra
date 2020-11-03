#ifndef CORETYPES_SEGTIME_H
#define CORETYPES_SEGTIME_H

#include <string>
#include <iosfwd>

#include <boost/optional.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace ct
{

/*
 * Для функции create и makeText используется такое представление:
 *     "HHMM HHMM"    day change = 0
 *     "HHMM HHMM/1"  day change = +1
 *     "HHMM HHMM/M1" day change = -1
 */
class SegTime {
public:
    SegTime(const boost::posix_time::time_duration&, const boost::posix_time::time_duration&, int dayChange);

    static boost::optional<SegTime> create(const std::string&);
    static boost::optional<SegTime> create(const boost::posix_time::time_duration&,
            const boost::posix_time::time_duration&, int dayChange);
    static boost::optional<SegTime> create(const boost::posix_time::ptime&, const boost::posix_time::ptime&);

    const boost::posix_time::time_duration& dep() const;
    const boost::posix_time::time_duration& arr() const;
    int dayChange() const;

    bool operator==(const SegTime& rhp) const;
    bool operator!=(const SegTime& rhp) const;

    std::string makeText() const;
private:
    boost::posix_time::time_duration depTime_;
    boost::posix_time::time_duration arrTime_;
    int dayChange_;
};
std::ostream& operator<<(std::ostream& os, const SegTime&);

} // ct

#endif /* CORETYPES_SEGTIME_H */

