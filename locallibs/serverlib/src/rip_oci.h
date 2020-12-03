#pragma once

#include <string>
#include <typeinfo>
#include <string.h>

#include "slogger_nonick.h"

#include "rip.h"
#include "oci_selector.h"

namespace OciCpp
{

template<typename T>
struct OciSelectorRipHelper
{
    static const int len = sizeof(T);
    static const int type = OciSelector<T>::type; // OciNumericTypeTraits<T>::otype;
    static const External::type data_type = External::wrapper;
    static const int canBindout = 1;

    static int size(const void* ) { return sizeof(T); }

    template<typename RipType>
    static void* addr(const RipType* a) { return const_cast<RipType*>(a); }

    template<typename RipType>
    static void to(buf_ptr_t& dst, const void* src, indicator& ind, const RipType*) {
        ind = iok;
        dst.resize(sizeof(T));
        memcpy(&dst[0], &static_cast<const RipType*>(src)->get(), sizeof(T));
    }

    template<typename RipType>
    static void from(RipType* out_ptr, const char* in_ptr, indicator ind) {
        if (ind == iok) {
            out_ptr->operator=(RipType(*reinterpret_cast<const T*>(in_ptr)));
        } // TODO else throw
    }

    template<typename RipType>
    static void check(RipType const *a) { a->get(); }
};

template<>
struct OciSelectorRipHelper<std::string>
{
    static const int len = -1;
    static const int type = SQLT_STR;
    static const External::type data_type = External::string;
    static const int canBindout = 0;
    
    static int size(const void* addr) { return strlen(static_cast<const char*>(addr)) + 1; }

    template<typename RipType>
    static void* addr(RipType* a) { return const_cast<std::string*>(&a->getRef()); }

    template<typename RipType>
    static void to(buf_ptr_t& dst, const void* src, indicator& ind, const RipType*) {
        const std::string& s = static_cast<const RipType*>(src)->get();
        dst.resize(s.size() + 1);
        if (ind == iok) {
            s.copy(&dst[0], s.size());
        }
        dst[s.size()] = '\0';
    }
    template<typename RipType>
    static void from(RipType* out_ptr, const char* in_ptr, indicator ind) {
        if (ind == iok) {
            *out_ptr = RipType(in_ptr);
        } // TODO else throw
    }

    template<typename RipType>
    static void check(RipType const*) {}
};

template <typename traits_t, typename base_t>
struct OciSelector<const rip::BaseParameter<traits_t, base_t> >
{
    typedef rip::BaseParameter<traits_t, base_t> this_type;
    typedef OciSelectorRipHelper<base_t> helper_t;

    enum{ canOdef = 1 };
    enum{ canObind = 1 };
    static const int len = helper_t::len;
    static const int type = helper_t::type;
    static const External::type data_type = helper_t::data_type;
    static bool canBind(const this_type&) { return true; }
    static int size(const void* a) { return helper_t::size(a); }
    static void* addr(const this_type* a) { return helper_t::addr(a); }
    static void to(buf_ptr_t& dst, const void* src, indicator& ind) { helper_t::to(dst, src, ind, (this_type*)NULL); }
    static void check(this_type const *) { /* always valid */ }
};

template <typename traits_t, typename base_t>
struct OciSelector<rip::BaseParameter<traits_t, base_t> >
    : public OciSelector<const rip::BaseParameter<traits_t, base_t> >
{
    typedef rip::BaseParameter<traits_t, base_t> this_type;
    typedef OciSelectorRipHelper<base_t> helper_t;

    enum{ canBindout = helper_t::canBindout };
    static void from(char* out_ptr, const char* in_ptr, indicator ind) {
        return helper_t::from(reinterpret_cast<this_type*>(out_ptr), in_ptr, ind);
    }
};

} // OciCpp
