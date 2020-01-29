#ifndef SMARTSTRINGS_H
#define SMARTSTRINGS_H

#include <iosfwd>
#include <string>
#include "exception.h"

namespace smart_strings {

template<template <typename> class Validator, typename Traits>
class SmartString
{
public:
    typedef std::string base_type;
    class Exception
        : public comtech::Exception
    {
    public:
        Exception(const std::string& msg)
            : comtech::Exception(msg)
        {}
        virtual ~Exception() throw()
        {}
    };
public:
    /**
      * maximum length of element
    */
    static size_t maxLength() { return Traits::length; }

    SmartString() {}
    SmartString(const std::string& str) {
        this->set(str);
    }
    SmartString(const char* str) {
        this->set(str);
    }
    static bool validate(const std::string& str)
    {
        return Validator<Traits>::validate(str);
    }
    void set(const char* str)
    {
        if (!Traits::allowNullPtr)
            m_str = str;
        else
            m_str = (str) ? str : "";
        if (!validate(m_str)) {
            std::string msg(Traits::name);
            msg += ":bad value length: value=[";
            msg += m_str + "]";
            throw Exception(msg);
        }
    }
    void set(const std::string& str)
    {
        this->set(str.c_str());
    }
    const std::string& get() const
    {
        return m_str;
    }
    bool operator==(const SmartString<Validator, Traits>& st) const
    {
        return this->m_str == st.m_str;
    }
    bool operator < (const SmartString<Validator, Traits>& st) const
    {
        return this->m_str < st.m_str;
    }
private:
    std::string m_str;
};

template <typename Traits> struct LimitedLengthStringValidator
{
    static bool validate(const std::string& str)
    {
        if (str.length() > Traits::length)
            return false;
        return true;
    }
};

template <typename Traits> struct RangedLengthStringValidator
{
    static bool validate(const std::string& str)
    {
        if (str.length() > Traits::length || str.length() < Traits::minlength)
            return false;
        return true;
    }
};

template <typename Traits> struct FixedLengthStringValidator
{
    static bool validate(const std::string& str)
    {
        if (str.length() != Traits::length)
            return false;
        return true;
    }
};

template<template <typename> class Validator, typename Traits>
std::ostream& operator<<(std::ostream& st, const SmartString<Validator, Traits>& val) {
    st << val.get();
    return st;
}

} // namespace smart_strings


#endif /* SMARTSTRINGS_H */

