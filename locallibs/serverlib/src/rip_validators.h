#pragma once

#include <string>
#include "encstring.h"

namespace rip
{
namespace validators {

template<typename T, T max>
bool byLess(const T& v)
{
    return v < max;
}

template<typename T, T min, T max>
bool byRange(const T& v)
{
    return v >= min && v <= max;
}

template<size_t min, size_t max>
bool byLength(const std::string& v)
{
    return v.length() >= min && v.length() <= max;
}

template<size_t min, size_t max>
bool byLength(const EncString& v)
{
    return v.to866().length() >= min && v.to866().length() <= max;
}

bool byRegex(const std::string& v, const std::string& reStr);

} // validators

} // rip
