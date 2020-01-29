#pragma once

#include <boost/date_time/gregorian/greg_date.hpp>

#include "ssim_schedule.h"

namespace ssim {

boost::gregorian::date guessDate(const std::string&);
std::string printDate(const boost::gregorian::date&);

std::string pointCode(nsi::PointId);

struct ScdRouteView
{
    Period period;
    std::vector<Section> route;
};

Periods splitUtcScdByLeaps(const ScdRouteView&);

Period getSearchRangeForUtc(const Period&);

template <typename Legs>
std::pair<typename Legs::const_iterator, typename Legs::const_iterator>
    getLegsRange(const Legs& lgs, ct::SegNum sn)
{
    const ct::PointsPair pp = ct::pointsBySeg(sn);
    return { std::next(lgs.begin(), pp.first.get()), std::next(lgs.begin(), pp.second.get() - 1) };
}

boost::optional<ct::SegNum> segnum(const ssim::Legs&, const nsi::DepArrPoints&);
boost::optional<ct::LegNum> legnum(const ssim::Legs&, const nsi::DepArrPoints&);
nsi::DepArrPoints getSegDap(const ssim::Route&, ct::SegNum);

namespace details {
std::vector<size_t> findLegs(const ssim::Legs&, nsi::PointId, nsi::PointId);
std::vector<ct::SegNum> findSegs(const ssim::Route&, nsi::PointId, nsi::PointId);
} //details

} //ssim
