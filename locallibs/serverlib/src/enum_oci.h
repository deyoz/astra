#pragma once

#include <type_traits>
#include "oci_selector.h"

namespace OciCpp
{

template <typename T>
struct OciSelector<T, typename std::enable_if_t<std::is_enum<T>::value>>
{
    using underlying_type = std::underlying_type_t<T>;
    typedef OciSelector<underlying_type> helper_t;

    enum { canOdef = helper_t::canOdef };
    enum { canObind = helper_t::canObind };
    static const int len = helper_t::len;
    enum { type = helper_t::type };
    static const External::type data_type = helper_t::data_type;
    static bool canBind(const T &) { return true; }
    static void* addr(T *a) { return const_cast<void*>((const void*)a); }
    static void to(buf_ptr_t& dst, const void* src, indicator& ind) { helper_t::to(dst, src, ind); }
    static int size(const void* addr) { return helper_t::size(addr); }
    static void check(const T *) { /* always valid */ }
    enum { canBindout = helper_t::canBindout };
    static void from(char* out_ptr, const char* in_ptr, indicator ind) {
        helper_t::from(out_ptr, in_ptr, ind);
    }
};

} // OciCpp
