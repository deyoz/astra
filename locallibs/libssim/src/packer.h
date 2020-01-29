#pragma once

#include <serverlib/json_packer.h>

namespace ssim { namespace mct {
struct Record;
} } //ssim::mct

namespace json_spirit {

JSON_PACK_UNPACK_DECL(ssim::mct::Record);

} //json_spirit
