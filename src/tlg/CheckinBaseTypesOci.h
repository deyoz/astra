#pragma once

#include "CheckinBaseTypes.h"
#include <serverlib/cursctl.h>
#include <serverlib/slogger_nonick.h>

#define OciSelectorParInt(base_t) template <Ticketing::IDS I> \
struct OciSelector<Ticketing::Value<I,base_t> > { \
    enum{ canOdef = 1}; \
    enum{ canObind = 1 }; \
    enum{ canBindout = 1 }; \
    enum{ bind_out_is_denied_for_this_type };\
    typedef Ticketing::Value<I,base_t> this_type;\
    static const bool auto_null_value=true;\
    static const int len=sizeof(base_t); \
    static const int type = OciNumericTypeTraits<base_t>::otype; \
    static const External::type data_type = External::wrapper;\
    static bool canBind(const Ticketing::Value<I,base_t>& ) {return true;} \
    static void * addr(this_type *a){return a;} \
    static char* to(const void* a, indicator& ind)\
    {\
        char* memory = new char[sizeof(base_t)];\
        if (!*((this_type*)a))\
        {\
            ind = inull;\
        }\
        if (ind == iok)\
        {\
            base_t val = ((this_type*)a)->get(); \
            memcpy(memory, &val, sizeof(base_t));\
        }\
        return memory;\
    }\
    static void check(this_type const *a){ a->get();} \
    static int size(const void* a)\
    {\
        return sizeof(base_t);\
    }\
    static void from(char* out_ptr, const char* in_ptr, indicator ind)\
    {\
        if (ind == iok)\
        {\
            this_type a;\
            base_t val = (*(base_t*)in_ptr);\
            a = this_type(val);\
            ((this_type*)out_ptr)->operator =(a);\
        }\
    }\
}; \
template <Ticketing::IDS I> struct OciSelector<const Ticketing::Value<I,base_t> > { \
    enum { canOdef = 0 }; \
    enum { canObind = 1 }; \
    static const int len=sizeof(base_t); \
    static const int type =SQLT_INT; \
    static const External::type data_type = External::wrapper;\
    typedef const Ticketing::Value<I,base_t> this_type;\
    static bool canBind(const Ticketing::Value<I,base_t>& ) {return true;} \
    static void * addr(const this_type *a){return (void*)a;}\
    static char* to(const void* a, indicator& ind)\
    {\
        char* memory = new char[sizeof(base_t)];\
        if (!((this_type*)a))\
        {\
            ind = inull;\
        }\
        if (ind == iok)\
        {\
            memset(memory, ((this_type*)a)->get(), sizeof(base_t));\
        }\
        return memory;\
    }\
    static int size(const void* a)\
    {\
        return sizeof(base_t);\
    }\
    static void check(this_type const *a){ a->get();} \
}

namespace OciCpp {
OciSelectorParInt(int);
OciSelectorParInt(long);
OciSelectorParInt(short);
OciSelectorParInt(unsigned int);
OciSelectorParInt(unsigned long);
OciSelectorParInt(unsigned short);
}//namespace OciCpp
