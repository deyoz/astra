#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif

#include <boost/regex.hpp>

#include <serverlib/str_utils.h>

#include "predpoint.h"
#include <serverlib/a_ya_a_z_0_9.h>

#define NICKNAME "NONSTOP"
#include <serverlib/slogger.h>

namespace ct
{

PredPoint::PredPoint(const std::string& predPoint):
    predPoint_(predPoint)
{}

boost::optional<PredPoint> PredPoint::create(const std::string& s)
{
    static const boost::regex validator("^([0-9A-Z" A_YA "]{5,6})$");
    
    if (s.empty()) {
        LogTrace(TRACE5) << "empty PredPoint";
        return boost::optional<PredPoint>();
    }

    if (!boost::regex_match(s, validator)) {
        LogTrace(TRACE5) << "invalid PredPoint: [" << s << "]";
        return boost::optional<PredPoint>();
    }
    return PredPoint(s);
}

const std::string& PredPoint::str() const
{
    return predPoint_;
}

std::string PredPoint::airimpStr() const
{
    return predPoint_.size() == 6 ?
        cityPortStr() + '/' + airlineStr() :
        predPoint_;
}

std::string PredPoint::cityPortStr() const
{
    return predPoint_.substr(0, 3);
}

std::string PredPoint::airlineStr() const
{
    return predPoint_.substr(3);
}

bool PredPoint::operator<(const PredPoint& rhp) const
{
    return predPoint_ < rhp.predPoint_;
}

bool PredPoint::operator==(const PredPoint& rhp) const
{
    return predPoint_ == rhp.predPoint_;
}

std::ostream& operator<<(std::ostream& os, const PredPoint& predPoint)
{
    return os << predPoint.str();
}



} // ct
