#ifndef SERVERLIB_STRING_CAST_H
#define SERVERLIB_STRING_CAST_H

#include <sstream>

namespace HelpCpp {

template<typename T> auto string_cast(T&& val) -> decltype(std::string(val))
{
    return std::string(std::forward<T>(val));
}

template<typename N> std::string string_cast(N const& n)
{
    std::stringstream s;
    s << n;
    return s.str();
}

/// a set of fast snprintf based int to string convertors
std::string string_cast(int n);
std::string string_cast(long n);
std::string string_cast(unsigned n);
std::string string_cast(unsigned long n);
std::string string_cast(unsigned long long n);
std::string string_cast(long n);
std::string string_cast(long long n);
std::string string_cast(double n);
std::string string_cast(bool b);
std::string string_cast(char c);


} // namespace HelpCpp

#endif /* SERVERLIB_STRING_CAST_H */
