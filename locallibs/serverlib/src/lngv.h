#ifndef _SERVERLIB_LNGV_H
#define _SERVERLIB_LNGV_H

#include <string>

// #include "enum.h"

#define ENG   0
#define RUS   1
enum Language : int { ENGLISH = 0, RUSSIAN = 1 };

Language languageFromStr(const std::string&);
// ENUM_NAMES_DECL(Language);

#endif /*_SERVERLIB_LNGV_H*/
