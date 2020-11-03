#pragma once

#include <functional>

#include "ssim_data_types.h"
#include "dei_subsets.h"

namespace res {
struct ScdPeriod;
}

namespace ssim { namespace details {

void setupSsimData(
    const ssim::ScdPeriod&,
    ssim::DeiInfo&, ssim::LegStuffList&, ssim::SegmentInfoList&,
    const ssim::PubOptions&, const supported_dei_fn&
);

} } //ssim::details
