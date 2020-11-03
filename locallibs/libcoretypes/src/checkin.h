#ifndef CORETYPES_CHECKIN_H
#define CORETYPES_CHECKIN_H

#include <iosfwd>
#include <boost/optional.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <libnsi/nsi.h>

namespace ct
{

struct Checkin
{
    boost::optional<boost::posix_time::time_duration> time; // Время регистрации в порту
    boost::optional<unsigned> luggage;      // Норма багажа (< 200)
    boost::optional<nsi::TermId> dt;        // departure terminal
    boost::optional<nsi::TermId> at;        // arrival terminal
};
bool operator==(const Checkin&, const Checkin&);
std::ostream& operator<<(std::ostream& os, const Checkin&);

}// ct

#endif /* CORETYPES_CHECKIN_H */

