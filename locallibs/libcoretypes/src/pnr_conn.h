#ifndef CORETYPES_PNR_CONN_H
#define CORETYPES_PNR_CONN_H

#include <boost/optional.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>

#include <nsi/nsi.h>

#include "segtime.h"

#include <vector>

namespace ct
{
namespace connections
{

struct SegData
{
    boost::optional<nsi::PointId> dep, arr;
    boost::optional<nsi::TermId> depTerm, arrTerm;
    boost::optional<boost::posix_time::ptime> depTime, arrTime;
    bool arnk;
    SegData();
};

struct CheckMode
{
    boost::posix_time::time_duration sameCityTime, samePortTime, sameTermTime;
    bool checkTime;
    CheckMode();
};

enum Reason { CR_BadCities = 1, CR_BadTimes };

struct Error
{
    unsigned seg1, seg2;
    Reason r;
    Error(unsigned i_seg1, unsigned i_seg2, Reason reason);
};

boost::optional<Error> check(const std::vector<SegData>&, const CheckMode&);

} // connections
} // ct

#endif
#ifndef CORETYPES_PNR_CONN_H
#define CORETYPES_PNR_CONN_H

#include <boost/optional.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include <nsi/nsi.h>

#include "segtime.h"

#include <vector>

namespace ct
{
namespace connections
{

struct SegData
{
    boost::optional<nsi::PointId> dep, arr;
    boost::optional<nsi::TermId> depTerm, arrTerm;
    boost::optional<boost::posix_time::ptime> depTime, arrTime;
    bool arnk;
    SegData();
};

struct CheckMode
{
    boost::posix_time::time_duration sameCityTime, samePortTime, sameTermTime;
    bool checkTime;
    CheckMode();
};

enum Reason { CR_BadCities = 1, CR_BadTimes };

struct Error
{
    unsigned seg1, seg2;
    Reason r;
    Error(unsigned i_seg1, unsigned i_seg2, Reason reason);
};

boost::optional<Error> check(const std::vector<SegData>&, const CheckMode&);

} // connections
} // ct

#endif
