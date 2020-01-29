#pragma once

#include <boost/optional.hpp>

// вспомогательная структура для обработки трех ситуаций:
// - значение существует и оно непустое
// - значение существует и оно пустое
// = значения не существует
// optional< ValOrNull<T> > более наглядно, чем optional< optional<T> >

template<typename T>
struct ValOrNull
{
    ValOrNull() {}
    ValOrNull(const T& t) : val(t) {}

    boost::optional<T> val;
};

template<typename T>
bool operator==(const ValOrNull<T>& lhs, const ValOrNull<T>& rhs)
{
    return lhs.val == rhs.val;
}
