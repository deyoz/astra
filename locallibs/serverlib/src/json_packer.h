#pragma once

#include <string>
#include <deque>
#include <iosfwd>

#include "expected.h"
#include "noncopyable.h"
#include "json_spirit_fwd.h"

namespace json_spirit
{

template<typename T>
struct Traits
{};

template<typename T>
struct Traits<const T> : public Traits<T>
{};

struct UnpackError
{
    std::string nick;
    std::string file;
    int line;

    std::string text;
    std::deque<std::string> path;

    UnpackError propagate(const std::string& tag) const;
};
std::ostream& operator<<(std::ostream&, const UnpackError&);

bool operator==( UnpackError const &lhs, UnpackError const &rhs );

template<typename T>
using UnpackResult = Expected<T, UnpackError>;

void maskCardNums(mValue&);

std::string convertObjToUTF8(const std::string& obj);

template<typename T>
struct allow_packInt{ enum { allow = 1 }; };
template<typename T>
struct allow_unpackInt{ enum { allow = 1 }; };
template<typename T>
struct allow_packExt{ enum { allow = 1 }; };
template<typename T>
struct allow_unpackExt{ enum { allow = 1 }; };

} // json_spirit

#define JSON_ASSERT_TYPE(Type, val, json_type) { \
if (val.type() != json_type) { \
    return json_spirit::UnpackError{"NONSTOP", __FILE__, __LINE__, \
        std::string(#Type " got invalid type(") + HelpCpp::string_cast(val.type()) + ")" \
        " expected " #json_type "(" + HelpCpp::string_cast(json_type) + ")" }; \
} \
}

#define JSON_PACK_UNPACK_DECL(Type) \
template<> struct Traits< Type > \
{ \
    static mValue packInt(const Type& i); \
    static UnpackResult<Type> unpackInt(const mValue& v); \
    static mValue packExt(const Type& i); \
    static UnpackResult<Type> unpackExt(const mValue& v); \
}

#define JSON_DESC_TYPE_DECL(Type) JSON_PACK_UNPACK_DECL(Type)

#define JSON_DISABLE_OP(Type, Op) \
template<> \
struct allow_##Op<Type> { enum { allow = 0 }; };
