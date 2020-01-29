#pragma once

#include <string.h>
#include <tuple>
#include <type_traits>

#include "rip.h"
#include "freq.h"

namespace dbcpp {

namespace details {

template<typename T, int N>
struct TypeForEnum
{
    typedef T type;
};

template<typename T>
struct TypeForEnum<T, 1>
{
    typedef int type;
};

template<typename T>
struct RowFieldTraits
{
    typedef typename TypeForEnum<T, std::is_enum<T>::value >::type placeholder_t;
};

template<typename trait_t, typename base_t>
struct RowFieldTraits<rip::BaseParameter<trait_t, base_t> >
{
    typedef base_t placeholder_t;
};

template<typename T>
struct RowFieldTraits<boost::optional<T> >
{
    typedef typename RowFieldTraits<T>::placeholder_t placeholder_t;
};

template<>
struct RowFieldTraits<Freq>
{
    typedef std::string placeholder_t;
};

struct no_type
{
};

template<typename T>
struct DefHelper
{
    template<typename CursCtl, typename placeholder_t>
    static void def(CursCtl& cr, placeholder_t& val, short*) {
        cr.def(val);
    }
};

template<>
struct DefHelper< no_type >
{
    template<typename CursCtl, typename placeholder_t>
    static void def(CursCtl& cr, placeholder_t& val, short*) {}
};

template<typename T>
struct DefHelper< boost::optional<T> >
{
    template<typename CursCtl, typename placeholder_t>
    static void def(CursCtl& cr, placeholder_t& val, short* ind) {
        cr.def(val, ind);
    }
};

template<typename T>
struct GetHelper
{
    template<typename placeholder_t>
    static T get(const placeholder_t& val, short) {
        return T(val);
    }
};

template<>
struct GetHelper<no_type>
{};

template<typename T>
struct GetHelper< boost::optional<T> >
{
    template<typename placeholder_t>
    static boost::optional<T> get(const placeholder_t& val, short ind) {
        return (ind == -1) ? boost::optional<T>() : boost::optional<T>(val);
    }
};

} // details

template<typename CursCtl>
class BaseRow
{
public:
    virtual ~BaseRow() {}
    virtual void def(CursCtl&) = 0;
};

template<typename CursCtl, typename... Args>
class Row : public BaseRow<CursCtl>
{
    static_assert(sizeof...(Args) > 0, "Useless dbcpp::Row usage");

    template <typename T, size_t N>
    void define(CursCtl& cr) {
        details::DefHelper<T>::def(cr, std::get<N>(row), &indicators[N]);
    }

    template <std::size_t... Is>
    void define_with_index(CursCtl& cr, std::index_sequence<Is...>) {
        int d[] = { (define<Args, Is>(cr), 0)... };
        (void)d;
    }

    typedef std::tuple<Args...> row_type;
    std::tuple<typename details::RowFieldTraits<Args>::placeholder_t...> row;
    short indicators[sizeof...(Args)];
public:
    Row() {
        memset(indicators, 0, sizeof(indicators));
    }

    virtual void def(CursCtl& cr) override {
        define_with_index(cr, std::index_sequence_for<Args...>());
    }

    template<int N>
    typename std::tuple_element<N, row_type>::type get() const {
        return details::GetHelper<typename std::tuple_element<N, row_type>::type>::get(std::get<N>(row), indicators[N]);
    }
};

} //dbcpp
