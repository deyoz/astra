#include "dei_subsets.h"

namespace ssim { namespace details {

using DeiRange = std::pair<int, int>;
using DeiRanges = std::vector<DeiRange>;

static bool contains(const DeiRanges& subset, ct::DeiCode dei)
{
    return std::any_of(subset.begin(), subset.end(), [dei] (const DeiRange& dr) {
        return dei.get() >= dr.first  && dei.get() <= dr.second;
    });
}

DeiApplication getDeiApplication(ct::DeiCode dei)
{
    //Segment Information
    static const DeiRanges any_seg_subset = {
        { 8, 8 },
        { 11, 11 },
        { 101, 102 },
        { 121, 122 },
        { 125, 125 },
        { 170, 173 },   //subset of traffic restiction (cannot be used separately?)
        { 198, 199 },
        { 201, 201 },
        { 220, 220 },
        { 507, 507 },
        { 710, 799 },   //subset of traffic restiction (cannot be used separately?)
        { 800, 999 },   //free format deis - limitation depends on meanings
    };

    //Flight/Period/Routing/Equipment Information
    static const DeiRanges complex_subset = {
        { 1, 6 },
        { 9, 9 },
    };

    if (contains(complex_subset, dei)) {
        return DA_Complex;
    }
    if (dei.get() == 7) {
        return DA_RouteInfo;
    }
    if (contains(any_seg_subset, dei)) {
        return DA_SegInfo;
    }
    return DA_ShortSegInfo;
}
//#############################################################################
static const DeiRanges& eqt_con_subset()
{
    static const DeiRanges subset = {
        { 2, 6 },
        { 9, 9 },
        { 101, 108 },
        { 113, 115 },
        { 127, 127 },
        { 800, 999 }
    };
    return subset;
}

static const DeiRanges& flt_subset()
{
    static const DeiRanges subset = {
        { 2, 6 },
        { 9, 9 },
        { 10, 10 },
        { 50, 50 },
        { 122, 122 },
        { 800, 999 }
    };
    return subset;
}

static const DeiRanges& tim_subset()
{
    static const DeiRanges subset = {
        { 7, 7 },
        { 97, 97 },
        { 109, 109 },
        { 111, 111 },
        { 800, 999 }
    };
    return subset;
}
//#############################################################################
static bool isDeiAllowed(const DeiRanges& subset, ct::DeiCode d, bool for_seg_info)
{
    if (!for_seg_info) {
        return contains(subset, d);
    }

    const DeiApplication app = getDeiApplication(d);
    if (app == DA_SegInfo || app == DA_ShortSegInfo) {
        return contains(subset, d);
    }
    return false;
}

static const DeiRanges& getDeiSubset_(ssim::SsmActionType t)
{
    static const DeiRanges noLimits = { { 1, 999 } };

    if (t == SSM_EQT || t == SSM_EQT) {
        return eqt_con_subset();
    } else if (t == SSM_TIM) {
        return tim_subset();
    } else if (t == SSM_FLT) {
        return flt_subset();
    }
    return noLimits;
}

static const DeiRanges& getDeiSubset_(ssim::AsmActionType t)
{
    ssim::SsmActionType st = SSM_NEW;
    if (t == ASM_EQT || t == ASM_CON) {
        st = SSM_EQT;
    } else if (t == ASM_TIM) {
        st = SSM_TIM;
    } else if (t == ASM_FLT) {
       st = SSM_FLT;
    }
    return getDeiSubset_(st);
}

supported_dei_fn getSegDeiSubset(ssim::SsmActionType t)
{
    const DeiRanges& subset = getDeiSubset_(t);
    return std::bind(isDeiAllowed, std::cref(subset), std::placeholders::_1, true);
}

supported_dei_fn getSegDeiSubset(ssim::AsmActionType t)
{
    const DeiRanges& subset = getDeiSubset_(t);
    return std::bind(isDeiAllowed, std::cref(subset), std::placeholders::_1, true);
}

supported_dei_fn getDeiSubset(ssim::SsmActionType t)
{
    const DeiRanges& subset = getDeiSubset_(t);
    return std::bind(isDeiAllowed, std::cref(subset), std::placeholders::_1, false);
}

supported_dei_fn getDeiSubset(ssim::AsmActionType t)
{
    const DeiRanges& subset = getDeiSubset_(t);
    return std::bind(isDeiAllowed, std::cref(subset), std::placeholders::_1, false);
}

} } //ssim::details
