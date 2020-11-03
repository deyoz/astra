/*
 * Классы-обёртки для строк и целочисленных типов с валидирующим конструктором.
 *
 * От ParInt и smart_strings два существенных отличия:
 *
 * 1. Основное. Если объект существует, он обязательно валиден.
 *    Для хранения/передачи потенциально невалидных объектов пользуйтесь
 *    boost::optional - это понятней и надёжней, чем всякие вариации
 *    if (obj.isValid()) { ... }
 *    
 * 2. Некоторый минимализм в наборе методов.
 *    Например, функция set() отсутствует не случайно -
 *    при необходимости можно написать obj = Obj(raw_value);
 *
 * ---
 *
 * Примеры использования
 * ~~~~~~~~~~~~~~~~~~~~~
 *
 * DEFINE_STRING_WITH_LENGTH_VALIDATION(ETicket, 13, 13)
 * DEFINE_NATURAL_NUMBER_TYPE(Svcid, int)
 * DEFINE_NUMBER_WITH_RANGE_VALIDATION(CouponNum, unsigned, 1, 4)
 *
 * CouponNum cpn(0); // Летит std::runtime_error
 * CouponNum cpn(1); // OK
 * cpn = CouponNum(2); // OK, вместо специально отсутствующего метода set()
 * int x = cpn.get()
 *
 */

#ifndef __STRONG_TYPES_H
#define __STRONG_TYPES_H

#include <ostream>
#include <sstream>
#include <limits>
#include "lngv.h"
#include "exception.h"

namespace ServerFramework {

class ValidatorFailureException: public comtech::Exception {
public:
    ValidatorFailureException(
            const std::string& value,
            const std::string& lname,
            const std::string& rname):
        comtech::Exception(lname + ": validator failure for \"" + value + "\""),
        value_(value),
        lname_(lname),
        rname_(rname)
    {}
    ~ValidatorFailureException() = default;
    const std::string& value() const { return value_; }
    std::string typeName(Language lang) const {
        return lang == ENGLISH ?
            (lname_.empty() ? rname_ : lname_) :
            (rname_.empty() ? lname_ : rname_);
    }
private:
    std::string value_;
    std::string lname_;
    std::string rname_;
};

} /* namespace ServerFramework */

#define DECLARE_VALIDATOR_FAILURE_EXCEPTION(ExceptionClassName, lname, rname) \
    class ExceptionClassName: public ServerFramework::ValidatorFailureException { \
    public: \
        ExceptionClassName(const std::string& str): \
            ServerFramework::ValidatorFailureException(str, lname, rname) \
        {} \
    }

/*******************************************************************************
 * Строки
 ******************************************************************************/

#define DEFINE_STRING_WITH_LENGTH_VALIDATION(TypeName, min, max, lname, rname) \
    class TypeName { \
    public: \
        DECLARE_VALIDATOR_FAILURE_EXCEPTION(ValidatorFailure, lname, rname); \
        explicit TypeName(const std::string& s) : s_(s) {  validate();  } \
        explicit TypeName(std::string&& s) : s_(std::move(s)) {  validate();  } \
        ~TypeName() = default; \
        static bool validate(const std::string& s) { size_t z = s.size(); return z >= min and z <= max; } \
        static constexpr size_t minLength() { return min; } \
        static constexpr size_t maxLength() { return max; } \
        const std::string& str() const { return s_; } \
        bool operator==(const TypeName& rhp) const { return s_ == rhp.s_; } \
        bool operator!=(const TypeName& rhp) const { return s_ != rhp.s_; } \
        bool operator<(const TypeName& rhp) const { return s_ < rhp.s_; } \
        friend std::ostream& operator<<(std::ostream& os, const TypeName& obj) { return os << obj.str(); } \
    private: \
        void validate() const { \
             if (!validate(s_)) \
                throw ValidatorFailure(s_); \
        }\
        std::string s_; \
    };

/*******************************************************************************
 * Целочисленные типы
 ******************************************************************************/

#define DEFINE_NUMBER_WITH_RANGE_VALIDATION(TypeName, NumericType, min, max, lname, rname) \
    class TypeName { \
    public: \
        typedef NumericType UnderlyingType; \
        DECLARE_VALIDATOR_FAILURE_EXCEPTION(ValidatorFailure, lname, rname); \
        explicit TypeName(NumericType n): n_(n) { \
            if (!validate(n)) { \
                throw ValidatorFailure(std::to_string(n)); \
            } \
        } \
        static TypeName fromStr(const std::string& s); \
        static constexpr bool validate(NumericType n) noexcept { return n >= min and n <= max; } \
        static bool validateStr(const std::string& s); \
        static constexpr NumericType minValue() noexcept { return min; } \
        static constexpr NumericType maxValue() noexcept { return max; } \
        NumericType get() const noexcept { return n_; } \
        bool operator==(TypeName rhp) const noexcept { return n_ == rhp.n_; } \
        bool operator!=(TypeName rhp) const noexcept { return n_ != rhp.n_; } \
        bool operator<(TypeName rhp) const noexcept { return n_ < rhp.n_; } \
        friend std::ostream& operator<<(std::ostream& os, TypeName obj) { return os << obj.n_; } \
    private: \
        NumericType n_; \
    };

#define DEFINE_NATURAL_NUMBER_TYPE(TypeName, NumericType, lname, rname)\
    DEFINE_NUMBER_WITH_RANGE_VALIDATION(TypeName, NumericType, 1, std::numeric_limits<NumericType>::max(), lname, rname)

/*******************************************************************************
 * Булевские типы
 ******************************************************************************/

#define DEFINE_BOOL(TypeName) \
    class TypeName { \
    public: \
        explicit TypeName(bool flag): flag_(flag) {} \
        explicit TypeName(int flag): flag_(flag != 0) {} \
        int getInt() const noexcept { return flag_ ? 1 : 0; } \
        explicit operator bool() const noexcept { return flag_; } \
        bool operator==(const TypeName& x) const noexcept { return flag_ == x.flag_; } \
        bool operator!=(const TypeName& x) const noexcept { return flag_ != x.flag_; } \
        friend std::ostream& operator<<(std::ostream& os, TypeName obj) { return os << obj.flag_; } \
    private: \
        bool flag_; \
    }; \

#endif /* #ifndef __STRONG_TYPES_H */
