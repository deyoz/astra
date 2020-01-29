#ifndef SERVERLIB_RIP_H
#define SERVERLIB_RIP_H

#include <type_traits>

#include <boost/optional.hpp>
#include "makename_defs.h"
#include "exception.h"

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
        Exception() : ServerFramework::Exception(trait_t::errMsg()) {
            SIRENA_ABORT_OPT("RIP");
        }
        virtual ~Exception() throw () {}
    };

    typedef BaseParameter<trait_t, base_t> this_type;
    typedef base_t base_type;
public:
    explicit BaseParameter(const base_t& v) : val_(v) {
        if (!trait_t::validate(val_)) {
            throw Exception();
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
    return rv.get() == lv.get();
}

// operator !=
template <typename traits_t, typename base_t>
bool operator!= (const BaseParameter<traits_t, base_t>& lv, const BaseParameter<traits_t, base_t>& rv) {
    return !(rv == lv);
}

// operator <
template <typename traits_t, typename base_t>
bool operator< (const BaseParameter<traits_t, base_t>& lv, const BaseParameter<traits_t, base_t>& rv) {
    return lv.get() < rv.get();
}

// operator <=
template <typename traits_t, typename base_t>
bool operator<= (const BaseParameter<traits_t, base_t>& lv, const BaseParameter<traits_t, base_t>& rv) {
    return lv.get() <= rv.get();
}

// operator >
template <typename traits_t, typename base_t>
bool operator> (const BaseParameter<traits_t, base_t>& lv, const BaseParameter<traits_t, base_t>& rv) {
    return lv.get() > rv.get();
}

// operator >=
template <typename traits_t, typename base_t>
bool operator>= (const BaseParameter<traits_t, base_t>& lv, const BaseParameter<traits_t, base_t>& rv) {
    return lv.get() >= rv.get();
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

#define DECL_RIP(TypeName, BaseType) \
struct MakeNameWithLine2(TypeName, _RIP_AT_) \
{ \
    static const char* name () { return #TypeName; } \
    static const char* errMsg() { return "invalid " #TypeName; } \
    static bool validate(const BaseType&) { return true; } \
}; \
typedef rip::BaseParameter<MakeNameWithLine2(TypeName, _RIP_AT_), BaseType> TypeName

#define DECL_RIP2(TypeName, BaseType, ...) \
struct MakeNameWithLine2(TypeName, _RIP_AT_) \
{ \
    static const char* name () { return #TypeName; } \
    static const char* errMsg() { return "invalid " #TypeName; } \
    static bool validate(const BaseType& v) { return __VA_ARGS__(v); } \
}; \
typedef rip::BaseParameter<MakeNameWithLine2(TypeName, _RIP_AT_), BaseType> TypeName

#define DECL_RIP_LESS(TypeName, BaseType, Max) \
struct MakeNameWithLine2(TypeName, _RIP_AT_) \
{ \
    static const char* name () { return #TypeName; } \
    static const char* errMsg() { return "invalid " #TypeName; } \
    static bool validate(const BaseType& v) { return rip::validators::byLess<BaseType, Max>(v); } \
}; \
typedef rip::BaseParameter<MakeNameWithLine2(TypeName, _RIP_AT_), BaseType> TypeName

#define DECL_RIP_RANGED(TypeName, BaseType, Min, Max) \
struct MakeNameWithLine2(TypeName, _RIP_AT_) \
{ \
    static const char* name () { return #TypeName; } \
    static const char* errMsg() { return "invalid " #TypeName; } \
    static bool validate(const BaseType& v) { return rip::validators::byRange<BaseType, Min, Max>(v); } \
}; \
typedef rip::BaseParameter<MakeNameWithLine2(TypeName, _RIP_AT_), BaseType> TypeName

#define DECL_RIP_LENGTH(TypeName, BaseType, Min, Max) \
struct MakeNameWithLine2(TypeName, _RIP_AT_) \
{ \
    static const char* name () { return #TypeName; } \
    static const char* errMsg() { return "invalid " #TypeName; } \
    static bool validate(const BaseType& v) { return rip::validators::byLength<Min, Max>(v); } \
}; \
typedef rip::BaseParameter<MakeNameWithLine2(TypeName, _RIP_AT_), BaseType> TypeName

#define DECL_RIP_REGEX(TypeName, BaseType, reStr) \
struct MakeNameWithLine2(TypeName, _RIP_AT_) \
{ \
    static const char* name () { return #TypeName; } \
    static const char* errMsg() { return "invalid " #TypeName; } \
    static bool validate(const BaseType& v) { return rip::validators::byRegex(v, reStr); } \
}; \
typedef rip::BaseParameter<MakeNameWithLine2(TypeName, _RIP_AT_), BaseType> TypeName

#endif /* SERVERLIB_RIP_H */
