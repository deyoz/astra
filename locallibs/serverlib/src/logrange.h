#pragma once

#include <ostream>

// Utility class. Do not use outside this header
template<class InputIterator, class TransformT>
class LogRange_holder {
public:
    LogRange_holder(InputIterator first, InputIterator last, TransformT transform):
        first_(first), last_(last), transform_(transform)
    {}

    std::ostream& log(std::ostream& out) const
    {
        out << "[";
        for (InputIterator it = first_; it != last_; ++it) {
            out << " " << transform_(*it);
        }
        if (first_ != last_)
            out << " ";
        out << "]";
        return out;
    }
private:
    const InputIterator first_;
    const InputIterator last_;
    const TransformT transform_;
};

// Simple logger
// Usage:
//     std::vector<int> v = { 1, 2, 3 }
//     std::cout << LogRange(v.begin(), v.end());
// Output:
//     [ 1 2 3 ]
template<class InputIterator>
inline LogRange_holder<
    InputIterator,
    typename InputIterator::value_type (*)(const typename InputIterator::value_type&) >
LogRange(InputIterator first, InputIterator last)
{
    typedef typename InputIterator::value_type T;
    T (*identity)(const T&) = [](const T& t) -> T { return t; };
    return LogRange_holder<InputIterator, decltype(identity)>(first, last, identity);
}

// Advanced logger
// Usage:
//     std::vector<int> v = { 1, 2, 3 }
//     std::cout << LogRange(v.begin(), v.end(), [](int n) { return "(" + std::to_string(n) + ")"; });
// Output:
//     [ (1) (2) (3) ]
template<class InputIterator, class TransformT>
inline LogRange_holder<InputIterator, TransformT>
LogRange(InputIterator first, InputIterator last, TransformT transform)
{
    return LogRange_holder<InputIterator, TransformT>(first, last, transform);
}

template<class InputIterator, class TransformT>
inline std::ostream& operator<<(std::ostream& out, const LogRange_holder<InputIterator, TransformT>& holder)
{
    return holder.log(out);
}
