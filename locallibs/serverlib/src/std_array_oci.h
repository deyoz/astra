#pragma once

#include "oci_selector.h"
#include <array>

namespace OciCpp {

template <class T, size_t N> struct OciSelector< const std::array<T,N> >
{
    using SType = std::enable_if_t<std::is_integral<T>::value, std::array<T,N>>;
    enum { canObind   = 1 };
    enum { canBindArray = 1 };
    static constexpr size_t len = (N) * sizeof(T);
    enum { type = SQLT_BIN };
    static const External::type data_type = External::wrapper;
    static bool canBind(const SType& /*val*/) noexcept { return true; }
    static void* addr(const SType* a){ return a ? const_cast<SType*>(a)->data() : nullptr; }
    static int size(const void* /*addr*/) noexcept { return len; }
    static char* to(const void * a, indicator& ind)
    {
        if(ind == iok)
        {
            auto memory = new char[len];
            memcpy(memory, static_cast<const SType*>(a)->data(), len);
            return memory;
        }
        return nullptr;
    }
    static void check(const SType* ) noexcept {}
};

template <class T, size_t N> struct OciSelector< std::array<T,N> > : public OciSelector< const std::array<T,N> >
{
    using typename OciSelector< const std::array<T,N> >::SType;
    using OciSelector< const std::array<T,N> >::len;
    enum{ canOdef = 1 };
    enum{ canBindout = 1 };
    static void from(char* out_ptr, const char* in_ptr, indicator ind)
    {
        if(ind == iok and in_ptr)
            memcpy(reinterpret_cast<char*>(reinterpret_cast<SType*>(out_ptr)->data()), in_ptr, len);
    }
};

} // namespace OciCpp
