#pragma once

#include <type_traits>
#include <cmath>

#include <boost/optional.hpp>
#include "makename_defs.h"
#include "exception.h"
#include "string_cast.h"

// rip - right parameters - parameters always having valid values

// alternatives
// trup - trusty parameters
// cop - correct parameters
// sup - sure parameters
// cep - certain parameters
// sap/saf - safe parameters
// rep/rev - reliable parameters/values

namespace rip
{

template<typename trait_t, typename base_t>
class BaseParameter
{
public: 
    class Exception : public ServerFramework::Exception {
    public:
        Exception(const std::string& val) : ServerFramework::Exception(std::string(trait_t::errMsg()) + " val='" + val + "'") {
            SIRENA_ABORT_OPT("RIP");
        }
        virtual ~Exception() throw () {}
    };

    typedef BaseParameter<trait_t, base_t> this_type;
    typedef base_t base_type;
public:
    explicit BaseParameter(const base_t& v) : val_(v) {
        if (!trait_t::validate(val_)) {
            throw Exception(HelpCpp::string_cast(val_));
        }
    }
    const base_t& get() const {
        return val_;
    }

    const base_t& getRef() const { // mostly for OciCpp
        return val_;
    }

    static const char* name() {
        return trait_t::name();
    }
    static boost::optional<this_type> create(const base_t& v) {
        if (!trait_t::validate(v)) {
            return boost::optional<this_type>();
        } else {
            return this_type(v);
        }
    }
    static bool validate( base_t const &t )
    {
        return trait_t::validate( t );
    }
private:

    base_t val_;
};

// operator <<
template <typename traits_t, typename base_t>
std::ostream& operator<< (std::ostream& str, const BaseParameter<traits_t, base_t>& val) {
    return str << val.get();
}

// operator ==
template <typename traits_t, typename base_t>
bool operator== (const BaseParameter<traits_t, base_t>& lv, const BaseParameter<traits_t, base_t>& rv) {
    if constexpr (std::is_floating_point_v<base_t>) {
        return std::abs(lv.get() - rv.get()) <= std::numeric_limits<base_t>::epsilon();
    } else {
        return rv.get() == lv.get();
    }
}

// operator !=
template <typename traits_t, typename base_t>
bool operator!= (const BaseParameter<traits_t, base_t>& lv, const BaseParameter<traits_t, base_t>& rv) {
    return !(rv == lv);
}

// operator <
template <typename traits_t, typename base_t>
bool operator< (const BaseParameter<traits_t, base_t>& lv, const BaseParameter<traits_t, base_t>& rv) {
    if constexpr (std::is_floating_point_v<base_t>) {
        return (rv.get() - lv.get()) > std::numeric_limits<base_t>::epsilon();
    } else {
        return lv.get() < rv.get();
    }
}

// operator <=
template <typename traits_t, typename base_t>
bool operator<= (const BaseParameter<traits_t, base_t>& lv, const BaseParameter<traits_t, base_t>& rv) {
    if constexpr (std::is_floating_point_v<base_t>) {
        return lv.get() < rv.get() || lv == rv;
    } else {
        return lv.get() <= rv.get();
    }
}

// operator >
template <typename traits_t, typename base_t>
bool operator> (const BaseParameter<traits_t, base_t>& lv, const BaseParameter<traits_t, base_t>& rv) {
    if constexpr (std::is_floating_point_v<base_t>) {
        return (lv.get() - rv.get()) > std::numeric_limits<base_t>::epsilon();
    } else {
        return lv.get() > rv.get();
    }
}

// operator >=
template <typename traits_t, typename base_t>
bool operator>= (const BaseParameter<traits_t, base_t>& lv, const BaseParameter<traits_t, base_t>& rv) {
    if constexpr (std::is_floating_point_v<base_t>) {
        return lv.get() > rv.get() || lv == rv;
    } else {
        return lv.get() >= rv.get();
    }
}

template< typename T >
struct is_integral
{
    static constexpr bool value = false;
};

template< typename T, typename B >
struct is_integral< BaseParameter< T, B > >
{
    static constexpr bool value = std::is_integral< B >::value;
};

} // rip

// inner namespace rip_details is used to prevent ADL
#define DECL_RIP(TypeName, BaseType) \
namespace rip_details { \
struct MakeNameWithLine2(TypeName, _RIP_AT_) \
{ \
    static const char* name () { return #TypeName; } \
    static const char* errMsg() { return "invalid " #TypeName; } \
    static bool validate(const BaseType&) { return true; } \
}; } \
using TypeName = rip::BaseParameter<rip_details::MakeNameWithLine2(TypeName, _RIP_AT_), BaseType>;

#define DECL_RIP2(TypeName, BaseType, ...) \
namespace rip_details { \
struct MakeNameWithLine2(TypeName, _RIP_AT_) \
{ \
    static const char* name () { return #TypeName; } \
    static const char* errMsg() { return "invalid " #TypeName; } \
    static bool validate(const BaseType& v) { return __VA_ARGS__(v); } \
}; } \
using TypeName = rip::BaseParameter<rip_details::MakeNameWithLine2(TypeName, _RIP_AT_), BaseType>;

#define DECL_RIP_LESS(TypeName, BaseType, Max) \
namespace rip_details { \
struct MakeNameWithLine2(TypeName, _RIP_AT_) \
{ \
    static const char* name () { return #TypeName; } \
    static const char* errMsg() { return "invalid " #TypeName; } \
    static bool validate(const BaseType& v) { return rip::validators::byLess<BaseType, Max>(v); } \
}; } \
using TypeName = rip::BaseParameter<rip_details::MakeNameWithLine2(TypeName, _RIP_AT_), BaseType>;

#define DECL_RIP_RANGED(TypeName, BaseType, Min, Max) \
namespace rip_details { \
struct MakeNameWithLine2(TypeName, _RIP_AT_) \
{ \
    static const char* name () { return #TypeName; } \
    static const char* errMsg() { return "invalid " #TypeName; } \
    static bool validate(const BaseType& v) { return rip::validators::byRange<BaseType, Min, Max>(v); } \
}; } \
using TypeName = rip::BaseParameter<rip_details::MakeNameWithLine2(TypeName, _RIP_AT_), BaseType>;

#define DECL_RIP_LENGTH(TypeName, BaseType, Min, Max) \
namespace rip_details { \
struct MakeNameWithLine2(TypeName, _RIP_AT_) \
{ \
    static const char* name () { return #TypeName; } \
    static const char* errMsg() { return "invalid " #TypeName; } \
    static bool validate(const BaseType& v) { return rip::validators::byLength<Min, Max>(v); } \
}; } \
using TypeName = rip::BaseParameter<rip_details::MakeNameWithLine2(TypeName, _RIP_AT_), BaseType>;

#define DECL_RIP_REGEX(TypeName, BaseType, reStr) \
namespace rip_details { \
struct MakeNameWithLine2(TypeName, _RIP_AT_) \
{ \
    static const char* name () { return #TypeName; } \
    static const char* errMsg() { return "invalid " #TypeName; } \
    static bool validate(const BaseType& v) { return rip::validators::byRegex(v, reStr); } \
}; } \
using TypeName = rip::BaseParameter<rip_details::MakeNameWithLine2(TypeName, _RIP_AT_), BaseType>;

namespace rip
{
DECL_RIP(Int, int);

struct UInt_traits
{
    static const char* name () { return "UInt"; }
    static const char* errMsg() { return "invalid UInt"; }
    static bool validate( int const &v ) { return v >= 0; }
};
using UInt = rip::BaseParameter<UInt_traits, int>;

} // rip
