#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif

#include <set>
#include <serverlib/exception.h>

#include "route.h"

#define NICKNAME "NONSTOP"
#include <serverlib/slogger.h>

// TODO needs refactoring
static void pointsBySeg(int segnum, int *city1, int *city2)
{
    int tmp = segnum;
    for (int i = 0; ; ++i) {
        tmp -= i;
        if (tmp == 0) {
            *city2 = i + 1;
            *city1 = i;
            return;
        }
        if (tmp > 0 && tmp <= i) {
            *city2 = i + 1;
            *city1 = tmp - 1;
            return;
        }
    }
}

namespace ct
{

PointsPair pointsByLeg(LegNum ln)
{
    return std::make_pair(PointNum(ln.get()), PointNum(ln.get() + 1));
}

PointsPair pointsBySeg(SegNum sn)
{
    int d = -1, a = -1;
    ::pointsBySeg(sn.get(), &d, &a);
    ASSERT(d >= 0 && a >= 0);
    return std::make_pair(PointNum(d), PointNum(a));
}

bool isLongSeg(ct::SegNum sn)
{
    const ct::PointsPair pp(pointsBySeg(sn));
    return pp.second.get() > pp.first.get() + 1;
}

SegNum segnum(PointNum p1, PointNum p2)
{
    ASSERT(p1 < p2);
    return SegNum((p2.get() - p1.get() == 1)
            ? segsCnt(p1.get())
            : segsCnt(p2.get() - 1) + p1.get() + 1);
}

int segsCnt(int legs)
{
    return (1 + legs) * legs / 2;
}

std::vector<SegNum> segsByLeg(LegNum leg, int legs)
{
    std::vector<SegNum> segs;
    if (leg.get() >= legs)
        return segs;
    const PointNum legDep(leg.get()), legArr(leg.get() + 1);
    for (int i = 0; i < segsCnt(legs); ++i) {
        const PointsPair cs(pointsBySeg(SegNum(i)));
        if ((cs.first <= legDep && cs.second >= legArr)
                || (legDep > cs.first && legDep < cs.second && legArr > cs.second)
                || (legArr > cs.first && legArr < cs.second && legDep < cs.first))
            segs.push_back(SegNum(i));
    }
    return segs;
}

std::vector<ct::LegNum> legsBySeg(ct::SegNum seg)
{
    const PointsPair cs(pointsBySeg(seg));
    std::vector< ct::LegNum > legs;
    for (PointNum::base_type i = cs.first.get(); i < cs.second.get(); ++i) {
        legs.push_back(LegNum(i));
    }
    return legs;
}

std::vector<ct::SegNum> crossingSegs(ct::SegNum sn, int legs)
{
    std::set<ct::SegNum> segs;
    for (ct::LegNum l : ct::legsBySeg(sn)) {
        for (ct::SegNum s : ct::segsByLeg(l, legs)) {
            if (s == sn) {
                continue;
            }
            segs.insert(s);
        }
    }
    return std::vector<ct::SegNum>(segs.begin(), segs.end());
}

} // ct
