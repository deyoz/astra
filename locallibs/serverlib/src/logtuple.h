////////////////////////////////////////////////////////////////////////////////
// example:
//
// const auto t = std::make_tuple(1, 2);
// std::cout << LogTuple(t) << std::endl;
//
// output:
// [ 1, 2 ]
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <tuple>
#include <ostream>

/* 
 * Recursive template for printing tuple elements.
 * Initial value of I must be equal to tuple size.
 */
template<size_t I, class TupleT>
struct LogTuple_recursive_printer
{
    static void log(std::ostream& out, const TupleT& t)
    {
        constexpr size_t index = std::tuple_size<TupleT>::value - I;
        if (index > 0)
            out << ", ";
        out << std::get<index>(t);
        LogTuple_recursive_printer<I - 1, TupleT>::log(out, t);
    }
};

/* recursion terminator */
template<class TupleT>
struct LogTuple_recursive_printer<0, TupleT>
{
    static void log(std::ostream& out, const TupleT& t) {}
};

/* operator << is defined for this class */
template<class TupleT>
struct LogTuple_holder
{
    LogTuple_holder(const TupleT& value): value(value) {}
    const TupleT& value;
};

/* creates an instance of LogTuple_holder */
template<class TupleT>
inline LogTuple_holder<TupleT> LogTuple(const TupleT& t)
{
    return LogTuple_holder<TupleT>(t);
}

template<class TupleT>
std::ostream& operator<<(std::ostream& out, const LogTuple_holder<TupleT>& t)
{
    out << "[ ";
    LogTuple_recursive_printer<
            std::tuple_size<TupleT>::value,
            TupleT>
        ::log(out, t.value);
    out << " ]";
    return out;
}
