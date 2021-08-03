#pragma once

#include "CheckinBaseTypes.h"
#ifdef ENABLE_ORACLE
#include <serverlib/cursctl.h>
#endif
#include <serverlib/slogger_nonick.h>
#ifdef ENABLE_ORACLE
#define OciSelectorParInt(base_t) \
template <Ticketing::IDS I> struct OciSelector<const Ticketing::Value<I,base_t> > { \
    enum { canOdef = 0 }; \
    enum { canObind = 1 }; \
    static constexpr int len = sizeof(base_t); \
    static constexpr int type = OciNumericTypeTraits<base_t>::otype; \
    static constexpr External::type data_type = External::wrapper;\
    typedef const Ticketing::Value<I,base_t> this_type;\
    static constexpr bool canBind(const Ticketing::Value<I,base_t>& ) {return true;} \
    static constexpr void * addr(const this_type *a){ return const_cast<this_type*>(a); }\
    static void to(buf_ptr_t& dst, const void* src, indicator& ind)\
    {\
        if (!static_cast<const this_type*>(src)->valid())\
        {\
            ind = inull;\
        }\
        if (ind == iok)\
        {\
            dst.resize(len);\
            memcpy(&dst[0], static_cast<const this_type*>(src)->never_use_except_in_OciCpp(), len);\
        } else {\
            dst.clear();\
        }\
    }\
    static constexpr int size(const void* a) { return sizeof(base_t); }\
    static void check(this_type const *a){ a->get();} \
}; \
template <Ticketing::IDS I> struct OciSelector<Ticketing::Value<I,base_t>> : OciSelector<const Ticketing::Value<I,base_t>> { \
    enum{ canOdef = 1}; \
    enum{ canBindout = 1 }; \
    enum{ bind_out_is_denied_for_this_type = 1 };\
    typedef Ticketing::Value<I,base_t> this_type;\
    static constexpr bool auto_null_value=true;\
    static void from(char* out_ptr, const char* in_ptr, indicator ind)\
    {\
        if (ind == iok)\
        {\
            this_type a;\
            base_t val = (*(const base_t*)in_ptr);\
            a = this_type(val);\
            ((this_type*)out_ptr)->operator =(a);\
        }\
    }\
};

namespace OciCpp {
OciSelectorParInt(int);
OciSelectorParInt(long);
OciSelectorParInt(short);
OciSelectorParInt(unsigned int);
OciSelectorParInt(unsigned long);
OciSelectorParInt(unsigned short);
}//namespace OciCpp

#endif //ENABLE_ORACLE
