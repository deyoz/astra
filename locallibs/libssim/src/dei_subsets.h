#pragma once

#include <functional>
#include <coretypes/route.h>

#include "ssim_enums.h"

namespace ssim { namespace details {

enum DeiApplication {
    DA_Complex = 0,  //can be applied hierarchically
    DA_RouteInfo,    //Leg Information
    DA_SegInfo,      //Segment Information
    DA_ShortSegInfo  //Segment Information, applied only for short segments
};

DeiApplication getDeiApplication(ct::DeiCode);

using supported_dei_fn = std::function<bool (ct::DeiCode)>;

// DEIs can be stated in segment information for specified message type
supported_dei_fn getSegDeiSubset(ssim::SsmActionType);
supported_dei_fn getSegDeiSubset(ssim::AsmActionType);

// entire DEI subset for specified message type
supported_dei_fn getDeiSubset(ssim::SsmActionType);
supported_dei_fn getDeiSubset(ssim::AsmActionType);

} } //ssim::details
