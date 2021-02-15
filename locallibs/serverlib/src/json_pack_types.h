#pragma once

#include "json_packer.h"
#include "lngv.h"
#include "bgnd.h"
#include "objid.h"
#include "rip.h"

namespace boost { namespace gregorian { class date; } }
namespace boost { namespace gregorian { class date_duration; } }
namespace boost { namespace posix_time { class time_duration; } }
namespace boost { namespace posix_time { class ptime; } }
class UserLanguage;

namespace json_spirit
{
JSON_PACK_UNPACK_DECL(int);
JSON_PACK_UNPACK_DECL(unsigned int);
JSON_PACK_UNPACK_DECL(short);
JSON_PACK_UNPACK_DECL(bool);
JSON_PACK_UNPACK_DECL(int64_t);
JSON_PACK_UNPACK_DECL(uint64_t);
JSON_PACK_UNPACK_DECL(uint8_t);
JSON_PACK_UNPACK_DECL(double);
JSON_PACK_UNPACK_DECL(std::string);
JSON_PACK_UNPACK_DECL(boost::gregorian::date);
JSON_PACK_UNPACK_DECL(boost::gregorian::date_duration);
JSON_PACK_UNPACK_DECL(boost::posix_time::time_duration);
JSON_PACK_UNPACK_DECL(boost::posix_time::ptime);
JSON_PACK_UNPACK_DECL(Language);
JSON_PACK_UNPACK_DECL(ct::ObjectId);
JSON_PACK_UNPACK_DECL(bgnd::BgndReqId);
JSON_PACK_UNPACK_DECL(ct::UserId);
JSON_PACK_UNPACK_DECL(ct::CommandId);
JSON_PACK_UNPACK_DECL(UserLanguage);
JSON_PACK_UNPACK_DECL(rip::Int);
JSON_PACK_UNPACK_DECL(rip::UInt);

} // json_spirit
