#ifndef CORETYPES_ROUTE_H
#define CORETYPES_ROUTE_H

#include <vector>
#include <map>
#include <serverlib/rip.h>
#include <serverlib/rip_validators.h>

namespace ct
{

DECL_RIP_RANGED(LegNum, int, 0, 19);
DECL_RIP_RANGED(SegNum, int, 0, 209);
DECL_RIP_RANGED(PointNum, int, 0, 20);
DECL_RIP_RANGED(DeiCode, int, 1, 999);

typedef std::pair<PointNum, PointNum> PointsPair;

PointsPair pointsByLeg(LegNum);
PointsPair pointsBySeg(SegNum);
SegNum segnum(PointNum, PointNum);

int segsCnt(int legs);
std::vector<ct::SegNum> segsByLeg(ct::LegNum leg, int legs);
std::vector<ct::LegNum> legsBySeg(ct::SegNum seg);
std::vector<ct::SegNum> crossingSegs(ct::SegNum, int legs);

bool isLongSeg(ct::SegNum);

template <typename Legs>
boost::optional<ct::SegNum> segnum(
        const Legs& route,
        const std::function<bool (const typename Legs::value_type&, bool)>& matched
    )
{
    int i = 0;
    int p1 = -1, p2 = -1;
    for (const auto& lg : route) {
        if (p1 < 0 && matched(lg, true)) {
            p1 = i;
        }
        if (p1 >= 0 && p2 < 0 && matched(lg, false)) {
            p2 = i + 1;
            break;
        }
        ++i;
    }
    if (p1 >= 0 && p2 >= 0 && p1 < p2) {
        if (std::none_of(std::next(route.begin(), p2), route.end(), [&matched] (const auto& x) { return matched(x, false); })) {
            return ct::segnum(ct::PointNum(p1), ct::PointNum(p2));
        }
    }
    return boost::none;
}

} // ct

#endif /* CORETYPES_ROUTE_H */

