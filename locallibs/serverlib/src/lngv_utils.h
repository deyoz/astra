#ifndef SERVERLIB_LNGV_UTILS_H
#define SERVERLIB_LNGV_UTILS_H

#include <string>
#include <iosfwd>
#include "lngv.h"

std::ostream & operator << (std::ostream& os, const Language &x);

inline std::string LangStr(Language lng)
{
    return (lng==ENGLISH?"EN":"RU");
}

#endif /* SERVERLIB_LNGV_UTILS_H */

