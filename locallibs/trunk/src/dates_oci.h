#pragma once

#include <iosfwd>
#include <chrono>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "dates.h"

namespace OciCpp {

    struct oracle_datetime
    {
        unsigned char value[7];
        oracle_datetime();
        oracle_datetime(int year, int month, int day, int hour = 0, int min = 0, int sec = 0);
    };

   std::ostream & operator << (std::ostream& os,const oracle_datetime &t);

boost::gregorian::date from_oracle_date(oracle_datetime const & od);
oracle_datetime to_oracle_datetime(boost::gregorian::date const & d);

boost::posix_time::ptime from_oracle_time(oracle_datetime const & od);
oracle_datetime to_oracle_datetime(boost::posix_time::ptime const & t);

}

#include "cursctl.h"
namespace OciCpp
{

template<> struct OciSelector<const oracle_datetime>
{
    enum { canOdef  = 1 };
    enum { canObind = 1 };
    static bool canBind(const oracle_datetime& ) { return true; }
    static void* addr(const oracle_datetime* a) {return const_cast<oracle_datetime*>(a);}
    static char* to(const void* a, indicator& ind)
    {
        char* memory = new char[7];
        if (ind == iok)
        {
            memcpy(memory, static_cast<const oracle_datetime*>(a)->value, 7);
        }
        return memory;
    }
    static int size(const void* /*addr*/) { return 7; }
    static void check( const oracle_datetime * ) {}
    static const int len = 7;
    static const int type = SQLT_DAT;
    static const External::type data_type = External::pod;
};
template<> struct OciSelector<oracle_datetime> : public OciSelector<const oracle_datetime>
{
    enum { canBindout = 1 };
    static void from(char* /*out_ptr*/, const char* /*in_ptr*/, indicator /*ind*/) {}
};

template<> struct OciSelector<const boost::posix_time::ptime>
{
    enum { canOdef = 1 };
    enum { canObind = 1 };
    static constexpr bool canBind(const boost::posix_time::ptime&) noexcept { return true; }
    static void* addr(const boost::posix_time::ptime* a) { return const_cast<boost::posix_time::ptime*>(a); }
    static char* to(const void* a, indicator& indic);
    static constexpr int size(const void* /*addr*/) noexcept { return 7; }
    static void check(const boost::posix_time::ptime*) noexcept { }
    static const int len = 7;
    static const int type = SQLT_DAT;
    static const External::type data_type = External::wrapper;
};
template<> struct OciSelector<boost::posix_time::ptime> : public OciSelector<const boost::posix_time::ptime>
{
    enum { canBindout = 1 };
    static const bool auto_null_value = true;
    static void from(char* out_ptr, const char* in_ptr, indicator ind);
};

template<> struct OciSelector<const boost::gregorian::date>
{
    enum { canOdef = 1 };
    enum { canObind = 1 };
    static constexpr bool canBind(const boost::gregorian::date&) noexcept { return true; }
    static void* addr(const boost::gregorian::date* a) { return const_cast<boost::gregorian::date*>(a); }
    static char* to(const void* a, indicator& indic);
    static constexpr int size(const void* /*addr*/) noexcept { return 7; }
    static void check(const boost::gregorian::date*) noexcept { }
    static const int len = 7;
    static const int type = SQLT_DAT;
    static const External::type data_type = External::wrapper;
};
template<> struct OciSelector<boost::gregorian::date> : public OciSelector<const boost::gregorian::date>
{
    enum { canBindout = 1 };
    static const bool auto_null_value = true;
    static void from(char* out_ptr, const char* in_ptr, indicator ind);
};

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
#if 0
template <> struct OciSelector<const std::chrono::system_clock::time_point>
{
    typedef unsigned char __base_type_[22];
    enum{ canObind = 1 };
    enum { canBindArray = 1 };
    static constexpr int len = sizeof(__base_type_);
    enum { type = SQLT_VNU/*OciNumericTypeTraits<__base_type_>::otype*/ };
    static constexpr External::type data_type = External::wrapper;
    static bool canBind(const std::chrono::system_clock::time_point&) { return true; }
    static char* to(const void* a, indicator& ind);
    static void* addr(const std::chrono::system_clock::time_point* a) { return a; }
    static int size(const void* a) { return  len; }
    static void check(std::chrono::system_clock::time_point const *){}//{a->get();}
    static buf_ptr_t conv (const std::chrono::system_clock::time_point *p);
    static buf_ptr_t convnull();
};

template <> struct OciSelector<std::chrono::system_clock::time_point> : public OciSelector<const std::chrono::system_clock::time_point>
{
    enum{ canOdef = 1 };
    enum{ canBindout = 1 };
    static void from(char* out_ptr, const char* in_ptr, indicator ind);
};
#endif // 0
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------


} // OciCpp
