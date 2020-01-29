#ifndef HTH_H
#define HTH_H

/**
 * @file
 * @brief HOST-TO-HOST(HTH protocol implementation due
 * "SYSTEMS AND COMMUNICATIONS REFERENCE (VOLUME 2)
 * HOST-TO-HOST PROTOCOL: STANDARD AND IMPLEMENTATION GUIDE 
 * Version 5.1" by IATA
 * */

#ifdef __cplusplus

#include <string>

/**
 * максимальный размер части HTH телеграмы */
#define MAX_HTH_PART_LEN    4096
/**
 * максимальный размер заголовка HTH телеграмы */
#define MAX_HTH_HEAD_SIZE   100

namespace hth
{

/**
 * максимальный размер заголовка HTH телеграмы */
const size_t MaxHthHeadSize = 100;

const size_t Request  = 0x48;
const size_t Response = 0x44;
const size_t AddrLength = 18;
const size_t TprLength = 19;
struct HthInfo
{
    char type;
    char sender[AddrLength + 1];
    char receiver[AddrLength + 1];
    char tpr[TprLength + 1];
    char why[3];
    char part;
    char end;
    char qri5;
    char qri6;
    char remAddrNum; // <- layer 5 octet 2
};
/**
 * comparison operator
 * */
bool operator==(const HthInfo& lv, const HthInfo& rv);

/**
 * output to stream operator
 * */
std::ostream& operator<<(std::ostream& os, const HthInfo& hth);

/**
 * joins hthInfo and tlgBody - adds special HTH symbols
 * */
void createTlg(char* tlgBody, const HthInfo& hthInfo);
/**
 * parses string str and fills HthInfo
 * return size of HTH header (0  means nothing was parsed)
 * */
size_t fromString(const std::string& str, HthInfo& hth);
size_t fromString(const char* str, HthInfo& hth);
std::string toString(const HthInfo& hth);
std::string toStringOnTerm(const HthInfo& hth);

/**
 * HTH body can contain spec symbols i.e. ETX
 * clear text from spec symbols
 * */
void removeSpecSymbols(char* str);
void trace(int logLevel, const char* nick, const char* file, int line, const HthInfo& hth);

} // namespace hth

#endif // __cplusplus

#endif /* HTH_H */

