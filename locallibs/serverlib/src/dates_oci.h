#pragma once

#include <iosfwd>
#include <chrono>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "dates.h"
#ifdef ENABLE_ORACLE
#include "cursctl.h"
#endif

namespace OciCpp {

struct oracle_datetime
{
    enum { size = 7 };
    unsigned char value[size];
    oracle_datetime();
    oracle_datetime(int year, int month, int day, int hour = 0, int min = 0, int sec = 0);
};

std::ostream & operator << (std::ostream& os,const oracle_datetime &t);

boost::gregorian::date from_oracle_date(oracle_datetime const & od);
oracle_datetime to_oracle_datetime(boost::gregorian::date const & d);

boost::posix_time::ptime from_oracle_time(oracle_datetime const & od);
oracle_datetime to_oracle_datetime(boost::posix_time::ptime const & t);

}

namespace OciCpp
{

#ifdef ENABLE_ORACLE

template<> struct OciSelector<const oracle_datetime>
{
    enum { canOdef  = 1 };
    enum { canObind = 1 };
    static bool canBind(const oracle_datetime& ) { return true; }
    static void* addr(const oracle_datetime* a) {return const_cast<oracle_datetime*>(a);}
    static void to(buf_ptr_t& dst, const void* src, indicator& ind)
    {
        dst.resize(len);
        if (ind == iok) {
            memcpy(&dst[0], static_cast<const oracle_datetime*>(src)->value, len);
        }
    }
    static int size(const void* /*addr*/) { return oracle_datetime::size; }
    static void check( const oracle_datetime * ) {}
    static const int len = oracle_datetime::size;
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
    static void to(buf_ptr_t& dst, const void* src, indicator& indic);
    static constexpr int size(const void* /*addr*/) noexcept { return 7; }
    static void check(const boost::posix_time::ptime*) noexcept { }
    static const int len = oracle_datetime::size;
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
    static void to(buf_ptr_t& dst, const void* src, indicator& indic);
    static constexpr int size(const void* /*addr*/) noexcept { return 7; }
    static void check(const boost::gregorian::date*) noexcept { }
    static const int len = oracle_datetime::size;
    static const int type = SQLT_DAT;
    static const External::type data_type = External::wrapper;
};
template<> struct OciSelector<boost::gregorian::date> : public OciSelector<const boost::gregorian::date>
{
    enum { canBindout = 1 };
    static const bool auto_null_value = true;
    static void from(char* out_ptr, const char* in_ptr, indicator ind);
};

#endif // ENABLE_ORACLE

} // OciCpp
