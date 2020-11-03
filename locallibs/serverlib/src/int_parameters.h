#ifndef INT_PARAMETERS_H
#define INT_PARAMETERS_H

#include <string>
#include <iosfwd>
#include <limits>
#include <cstdlib>

#include "tcl_utils.h"
#include "exception.h"
#include "makename_defs.h"

namespace ParInt
{
template <typename Par> bool equalParInt(Par a,Par b)
{
    if(a.valid() && b.valid()){
        return a==b;
    }
    return false;
}
template<typename traits_t, typename base_t>
class BaseIntParam;
template <typename traits_t, typename base_t>
base_t& getRef(BaseIntParam<traits_t, base_t>& p);
template <typename traits_t, typename base_t>
const base_t& getRef(const BaseIntParam<traits_t, base_t>& p);

/**
 * @class BaseIntParam
 * */
template<typename traits_t, typename base_t>
class BaseIntParam {
public:
    class Exception : public ServerFramework::Exception {
    public:
        Exception(const std::string& msg) : ServerFramework::Exception(msg) {
            SIRENA_ABORT_OPT("PARINT");
        }
    };
    typedef traits_t traits_type;
    typedef base_t base_type;
public:
    constexpr BaseIntParam() : val_(std::numeric_limits<base_t>::max()) {}
    constexpr explicit BaseIntParam(const base_t& val) : val_(val) {}
    const base_t& get() const {
        check_init();
        return val_;
    }
    const char* name() const { return traits_t().name_; }
    bool valid() const noexcept { return is_init(); }

    explicit operator bool () const noexcept { return valid(); }
/**
 * these two functions are used to help valid bind/def 'ing via OciSelector
 * */
    base_t* never_use_except_in_OciCpp () noexcept
    {
        return &val_;
    }
    base_t const* never_use_except_in_OciCpp () const noexcept
    {
        return &val_;
    }

// operators
    BaseIntParam<traits_t, base_t>& operator += (const BaseIntParam<traits_t, base_t>& i)
    {
        return (operator += (i.get()));
    }
    BaseIntParam<traits_t, base_t>& operator -= (const BaseIntParam<traits_t, base_t>& i)
    {
        return (operator -= (i.get()));
    }
    BaseIntParam<traits_t, base_t>& operator += (base_t i)
    {
        check_init();
        (void)traits_t::allow_arithmetics;
        val_ += i;
        return *this;
    }
    BaseIntParam<traits_t, base_t>& operator -= (base_t i)
    {
        check_init();
        (void)traits_t::allow_arithmetics;
        val_ -= i;
        return *this;
    }
    BaseIntParam<traits_t, base_t> inline  operator + (const BaseIntParam<traits_t, base_t>& v2) const
    {
        check_init();
        (void)traits_t::allow_arithmetics;
        return BaseIntParam<traits_t, base_t>(get() + v2.get());
    }
    BaseIntParam<traits_t, base_t> inline  operator - (const BaseIntParam<traits_t, base_t>& v2) const
    {
        check_init();
        (void)traits_t::allow_arithmetics;
        return BaseIntParam<traits_t, base_t>(get() - v2.get());
    }

    friend std::ostream& operator<< (std::ostream& str, const BaseIntParam& val) {
        if(val.valid())
            return str << val.get();
        return str << std::string("<Undefined>");
    }

private:
    void check_init() const {
        if (!is_init()){
            throw Exception(std::string("undefined BaseIntParam: ") + traits_t().name_);
        }
    }
    bool is_init() const noexcept {
        return (val_ != std::numeric_limits<base_t>::max());
    }
    base_t val_;
    friend const base_t& getRef<traits_t, base_t>(const BaseIntParam<traits_t, base_t>&);
    friend base_t& getRef<traits_t, base_t>(BaseIntParam<traits_t, base_t>&);
};

template <typename traits_t, typename base_t>
base_t& getRef(BaseIntParam<traits_t, base_t>& p)
{
    return p.val_;
}

template <typename traits_t, typename base_t>
const base_t& getRef(const BaseIntParam<traits_t, base_t>& p)
{
    return p.val_;
}

// operator ==
template <typename traits_t, typename base_t>
bool operator== (const BaseIntParam<traits_t, base_t>& lv, const BaseIntParam<traits_t, base_t>& rv) {
    return (not lv and not rv) or (lv and rv and rv.get() == lv.get());
}

template <typename traits_t, typename base_t>
bool operator== (const base_t& lv, const BaseIntParam<traits_t, base_t>& rv) {
    (void)traits_t::can_compare_to_base_t;
    return rv and lv == rv.get();
}

template <typename traits_t, typename base_t>
bool operator== (const BaseIntParam<traits_t, base_t>& lv, const base_t& rv) {
    (void)traits_t::can_compare_to_base_t;
    return lv and lv.get() == rv;
}

// operator !=
template <typename traits_t, typename base_t>
bool operator!= (const BaseIntParam<traits_t, base_t>& lv, const BaseIntParam<traits_t, base_t>& rv) {
    return not operator==(lv, rv);
}

template <typename traits_t, typename base_t>
bool operator!= (const base_t& lv, const BaseIntParam<traits_t, base_t>& rv) {
    (void)traits_t::can_compare_to_base_t;
    return not operator==(lv, rv);
}

template <typename traits_t, typename base_t>
bool operator!= (const BaseIntParam<traits_t, base_t>& lv, const base_t& rv) {
    (void)traits_t::can_compare_to_base_t;
    return not operator==(lv, rv);
}

// operator <
template <typename traits_t, typename base_t>
bool operator< (const BaseIntParam<traits_t, base_t>& lv, const BaseIntParam<traits_t, base_t>& rv) {
    return lv and (not rv or lv.get() < rv.get());
}

template <typename traits_t, typename base_t>
bool operator< (const BaseIntParam<traits_t, base_t>& lv, const base_t& rv) {
    (void)traits_t::can_compare_to_base_t;
    return lv.get() < rv;
}

template <typename traits_t, typename base_t>
bool operator< (const base_t& lv, const BaseIntParam<traits_t, base_t>& rv) {
    (void)traits_t::can_compare_to_base_t;
    return lv < rv.get();
}

// operator >
template <typename traits_t, typename base_t>
bool operator> (const BaseIntParam<traits_t, base_t>& lv, const BaseIntParam<traits_t, base_t>& rv) {
    return rv < lv;
}

template <typename traits_t, typename base_t>
bool operator> (const BaseIntParam<traits_t, base_t>& lv, const base_t& rv) {
    (void)traits_t::can_compare_to_base_t;
    return lv.get() > rv;
}

template <typename traits_t, typename base_t>
bool operator> (const base_t& lv, const BaseIntParam<traits_t, base_t>& rv) {
    (void)traits_t::can_compare_to_base_t;
    return lv > rv.get();
}

// operator <=
template <typename traits_t, typename base_t>
bool operator<= (const BaseIntParam<traits_t, base_t>& lv, const BaseIntParam<traits_t, base_t>& rv) {
    return lv.get() <= rv.get();
}

template <typename traits_t, typename base_t>
bool operator<= (const BaseIntParam<traits_t, base_t>& lv, const base_t& rv) {
    (void)traits_t::can_compare_to_base_t;
    return lv.get() <= rv;
}

template <typename traits_t, typename base_t>
bool operator<= (const base_t& lv, const BaseIntParam<traits_t, base_t>& rv) {
    (void)traits_t::can_compare_to_base_t;
    return lv <= rv.get();
}

// operator >=
template <typename traits_t, typename base_t>
bool operator>= (const BaseIntParam<traits_t, base_t>& lv, const BaseIntParam<traits_t, base_t>& rv) {
    return lv.get() >= rv.get();
}

template <typename traits_t, typename base_t>
bool operator>= (const BaseIntParam<traits_t, base_t>& lv, const base_t& rv) {
    (void)traits_t::can_compare_to_base_t;
    return lv.get() >= rv;
}

template <typename traits_t, typename base_t>
bool operator>= (const base_t& lv, const BaseIntParam<traits_t, base_t>& rv) {
    (void)traits_t::can_compare_to_base_t;
    return lv >= rv.get();
}

template<typename traits_t>
class BaseStrParam;
template <typename traits_t>
std::string& getRef(BaseStrParam<traits_t>& p);
template <typename traits_t>
const std::string& getRef(const BaseStrParam<traits_t>& p);

/**
 * @class BaseStrParam
 * */
template<typename traits_t>
class BaseStrParam {
public:
    class Exception : public ServerFramework::Exception {
    public:
        Exception(const std::string& msg) : ServerFramework::Exception(msg) {
            SIRENA_ABORT_OPT("PARINT");
        }
    };
    typedef traits_t traits_type;
    typedef std::string base_type;
public:
    BaseStrParam() : val_(traits_t().defaul_value) {}
    explicit BaseStrParam(const std::string& val) : val_(val) {
        valid();
    }
    const std::string& get() const {
        check_init();
        return val_; 
    }
    const char* name() const { return traits_t().name; }
    bool valid() const { return is_init(); }

private:
    void check_init() const {
        if (!is_init()){
            throw Exception(std::string("undefined BaseStrParam: ") + traits_t().name);
        }
    }
    bool is_init() const {
        return traits_t::is_valid(val_);
    }
    std::string val_;
    friend const std::string& getRef<traits_t>
        (const BaseStrParam<traits_t>&);
    friend std::string& getRef<traits_t>(BaseStrParam<traits_t>&);
};

template <typename traits_t>
bool operator== (const BaseStrParam<traits_t>& lv, const BaseStrParam<traits_t>& rv) {
    return lv.get() == rv.get();
}

template <typename traits_t>
bool operator!= (const BaseStrParam<traits_t>& lv, const BaseStrParam<traits_t>& rv) {
    return lv.get() != rv.get();
}

template <typename traits_t>
bool operator< (const BaseStrParam<traits_t>& lv, const BaseStrParam<traits_t>& rv) {
    return lv.get() < rv.get();
}

template <typename traits_t>
bool operator<= (const BaseStrParam<traits_t>& lv, const BaseStrParam<traits_t>& rv) {
    return lv.get() <= rv.get();
}

template <typename traits_t>
bool operator> (const BaseStrParam<traits_t>& lv, const BaseStrParam<traits_t>& rv) {
    return lv.get() > rv.get();
}

template <typename traits_t>
bool operator>= (const BaseStrParam<traits_t>& lv, const BaseStrParam<traits_t>& rv) {
    return lv.get() >= rv.get();
}

template <typename traits_t>
std::ostream& operator<< (std::ostream& str, const BaseStrParam<traits_t>& val) {
    if (val.valid())
        str << val.get();
    else
        str << std::string("<Undefined>");
    return str;
}

template <typename traits_t>
std::string& getRef(BaseStrParam<traits_t>& p)
{
    return p.val_;
}

template <typename traits_t>
const std::string& getRef(const BaseStrParam<traits_t>& p)
{
    return p.val_;
}

template<size_t len>
struct DefaultStrValidator
{
    static bool is_valid(const std::string& v) {
        return !v.empty() && (len == v.length());
    }
};

template<size_t len>
struct LessThanStrValidator
{
    static bool is_valid(const std::string& v) {
        return !v.empty() && (v.length() <= len);
    }
};

} // namespace ParInt

#define MakeIntParamType(ruler, type) \
struct MakeNameWithLine2(ruler, _PARINT_AT_) \
{   const char* name_; \
    MakeNameWithLine2(ruler, _PARINT_AT_)() { name_ = #ruler; } \
}; \
typedef ParInt::BaseIntParam<MakeNameWithLine2(ruler, _PARINT_AT_), type> ruler;

#define MakeArithmeticsIntParamType(ruler, type) \
struct MakeNameWithLine2(ruler, _PARINT_AT_) \
{   const char* name_; \
    static const bool can_compare_to_base_t = true; \
    static const bool allow_arithmetics = true; \
    MakeNameWithLine2(ruler, _PARINT_AT_)() { name_ = #ruler; } \
}; \
typedef ParInt::BaseIntParam<MakeNameWithLine2(ruler, _PARINT_AT_), type> ruler;

#define MakeStrParamType(ruler, len, ...) \
template<typename T = ParInt::DefaultStrValidator<(len)> > \
struct MakeNameWithLine2(ruler, _PARINT_AT_) \
{   const char* name; \
    const char* defaul_value; \
    static bool is_valid(const std::string& v) { \
        return T::is_valid(v); } \
    MakeNameWithLine2(ruler, _PARINT_AT_)() { \
        name = #ruler; \
        defaul_value = ""; \
    } \
}; \
typedef ParInt::BaseStrParam<MakeNameWithLine2(ruler, _PARINT_AT_)<__VA_ARGS__> > ruler;

#define MakeStrParamType2(ruler, len) MakeStrParamType(ruler, (len), ParInt::LessThanStrValidator<(len)>)

#endif /* INT_PARAMETERS_H */

