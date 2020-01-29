#ifndef _UTF2CP866_H__
#define _UTF2CP866_H__
#include <string>
std::string CP866toUTF8(const std::string& value);
bool CP866toUTF8(const char* buf, size_t bufsize, std::string& out);
std::string UTF8toCP866(const std::string& value);
std::string UTF8toCP866_no_throw(const std::string& s);
#endif // _UTF2CP866_H__
