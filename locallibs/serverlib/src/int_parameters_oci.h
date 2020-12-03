#pragma once
#include "string_cast.h"
#include "int_parameters.h"

#include "cursctl.h"
namespace OciCpp {

template <typename traits_t, typename base_t>
struct OciSelector<const ParInt::BaseIntParam<traits_t,base_t> >
{
    enum { canObind = 1 };
    static const int len = sizeof(base_t);
    static const int type = OciNumericTypeTraits<base_t>::otype;
    static const External::type data_type = External::wrapper;
    typedef const ParInt::BaseIntParam<traits_t,base_t> this_type;
    static bool canBind(const ParInt::BaseIntParam<traits_t,base_t>&) {return true;}
    static void * addr(this_type* a) {return const_cast<ParInt::BaseIntParam<traits_t, base_t>*>(a);}
    static void to(buf_ptr_t& dst, const void* src, indicator& ind)
    {
        if (not static_cast<this_type*>(src)->valid()) {
            ind = inull;
        }
        if (ind != inull) {
            dst.resize(len);
            memcpy(&dst[0], static_cast<this_type*>(src)->never_use_except_in_OciCpp(), len);
        } else {
            dst.clear();
        }
    }
    static int size(const void* /*addr*/) { return sizeof(base_t); }
    static void check(this_type const *a) {a->get();}
};

template <typename traits_t, typename base_t>
struct OciSelector<ParInt::BaseIntParam<traits_t, base_t> >
 : public OciSelector<const ParInt::BaseIntParam<traits_t,base_t> >
{
    enum{ canOdef = 1 };
    enum{ canBindout = 1 };
    typedef ParInt::BaseIntParam<traits_t, base_t> this_type;
    static const bool auto_null_value = true;
    static void from(char* out_ptr, const char* in_ptr, indicator ind)
    {
        if (ind == iok)
        {
            this_type a;
            base_t base = *reinterpret_cast<const base_t*>(in_ptr);
            a = this_type(base);
            reinterpret_cast<this_type*>(out_ptr)->operator =(a);
        }
    }
};

//-----------------------------------------------------------------------

template <typename traits_t>
struct OciSelector<const ParInt::BaseStrParam<traits_t> >
{
    typedef ParInt::BaseStrParam<traits_t> this_type;
    enum { canOdef  = 1 };
    enum { canObind = 1 };
    static const int len = -1;
    static const int type = SQLT_STR;
    static const External::type data_type = External::string;
    static bool canBind(const this_type&) {return true;}
    static void* addr(const this_type* a) {return &ParInt::getRef(*const_cast<this_type*>(a));}
    static void to(buf_ptr_t& dst, const void* src, indicator& ind)
    {
        const std::string& s = ParInt::getRef(*static_cast<const this_type*>(src));
        dst.resize(s.size() + 1);
        if (ind == iok) {
            s.copy(&dst[0], s.size());
        }
        dst[s.size()] = '\0';
    }
    static int size(const void* addr)
    {
        return strlen(static_cast<const char*>(addr)) + 1;
    }
    static void check(this_type const *){}
};


template <typename traits_t>
struct OciSelector< ParInt::BaseStrParam<traits_t> >
    : public OciSelector< const ParInt::BaseStrParam<traits_t> >
{
    static void from(char* out_ptr, const char* in_ptr, indicator ind)
    {
        auto sp = reinterpret_cast<ParInt::BaseStrParam<traits_t>*>(out_ptr);
        getRef(*sp).assign(in_ptr);
    }
    static const bool auto_null_value=true;
};

} // namespace OciCpp
