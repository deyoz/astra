#pragma once

#include "oci_selector.h"
#include "oci_default_ptr_t.h"
#include "ociexception.h"

namespace OciCpp {

template <unsigned L> struct OciSelector< const char [L] > {
    enum { canObind = 1 };
    enum { canBindArray = 1 };
    static constexpr unsigned len=L;
    enum { type = SQLT_STR };
    static constexpr External::type data_type = External::pod;
    static bool canBind(const char * val) {return (val != NULL);}
    static void* addr(const char (*a)[L]) {return const_cast<char(*)[L]>(a);}

    static void to(buf_ptr_t& dst, const void* src, indicator& ind)
    {
        if (ind != iok) {
            dst.clear();
            return;
        }
        const size_t a_len = strlen(static_cast<const char*>(src));
        dst.resize(a_len + 1);
        memcpy(&dst[0], src, a_len);
        dst[a_len] = '\0';
    }
    static int size(const void* /*addr*/) { return L; }
    static void check(const char (* /*addr*/)[L]){}
};

template <unsigned L> struct OciSelector< char [L] > : public OciSelector< const char [L] > {
    enum { canOdef    = 1 };
    enum { canBindout = 1};
    static constexpr bool auto_null_value=true;
    static void* addr(char (*a)[L]) {return a;}
    static void from(char* /*out_ptr*/, const char* /*in_ptr*/, indicator /*ind*/) {}
    template <size_t N>
    static default_ptr_t conv(const char (&nvl)[N]) {
        static_assert(L >= N, "the length of nvl is greater than the buffer capacity");
        struct { char v[N]; } v;
        memcpy(v.v, nvl, N);
        return default_ptr_t(v);
    }
    static default_ptr_t conv(const char* s) {
        size_t z = strlen(s);
        if(z >= L)
            throw ociexception("the length of nvl (" + std::to_string(z) + ") is greater than the buffer capacity (" + std::to_string(L) + "-1)");
        struct { char v[L]; } v;
        strncpy(v.v, s, L);
        return default_ptr_t(v);
    }
    template <class T> static default_ptr_t conv(T&& t) {
        static_assert(t == nullptr, "pretty bad default value type; you want to use a string literal instead");
        return default_ptr_t(1);
    }
    static default_ptr_t convnull() { return default_ptr_t(char(0)); }
};

template <> struct OciSelector<char const *> {
    enum { canObind = 1 };
    static const int len=-1;
    static const int type =SQLT_STR;
    static const External::type data_type = External::pod;
    static bool canBind(char const *val) {return (val != NULL);}
    static void* addr( char const * a){return const_cast <char *>(a);};
    static void* addr(char const* const* a) {return const_cast<char*>(*a);}
    static void to(buf_ptr_t& dst, const void* src, indicator& ind);
    static int size(const void* addr);
    static void check(char const * const*){}
};

template <> struct OciSelector<char *> : public OciSelector<char const *> {
    static void* addr(char const* a){return const_cast <char *>(a);};
    static void* addr(char const* const* a) {return const_cast<char*>(*a);}
    static void from(char* /*out_ptr*/, const char* /*in_ptr*/, indicator /*ind*/) {}
    template <size_t N>
    static default_ptr_t conv(const char (&nvl)[N]) {
        struct { char v[N]; } v;
        memcpy(v.v, nvl, N);
        return default_ptr_t(v);
    }
    template <class T> static default_ptr_t conv(T&& t) {
        static_assert(t == nullptr, "pretty bad default value type; you want to use a string literal instead");
        return default_ptr_t(1);
    }
};

template <> struct OciSelector<const char *const> : public OciSelector<char const *> {};
template <> struct OciSelector<char *const> : public OciSelector<char const *> {};



} // namespace OciCpp
