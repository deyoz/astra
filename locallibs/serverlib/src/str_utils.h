#ifndef _STR_UTILS_H_
#define _STR_UTILS_H_

#ifdef __cplusplus

#include <stdio.h>
#include <string>
#include <vector>
#include <stdint.h>
#include <sstream>
#include <map>
#include <functional>

#include "string_cast.h"

namespace StrUtils
{
bool IsUpperChar(unsigned char c);
bool IsLowerChar(unsigned char c);
bool IsUpper(const std::string &s);
bool IsLower(const std::string &s);

/**
 * @brief converts an input string to upper case
 * @brief сперто из yuran_tool
*/
std::string ToUpper( std::string s );

/**
 * @brief converts an input string to lower case
 * @brief сперто из yuran_tool
*/
std::string ToLower( std::string s );

/**
 * @brief capitalizes first letter and make others lower case
 */
std::string normalize(const std::string& in);
std::string normalize(std::string&& in);

bool IsLatChar(unsigned char ch);
bool IsRusChar(unsigned char ch);
bool IsLatOrRusChar(unsigned char ch);

bool isalnum_rus(char);
bool isalpha_rus(char);

std::string RPad(const std::string &str_in, std::string::size_type len, char c);
std::string LPad(const std::string &str_in, std::string::size_type len, char c);

/**
 * @brief returns string in specified base left padded up to 'len' with 'padch'
*/
std::string FromBase10Lpad(int64_t number, int target_base, std::string::size_type len, char padch = '0');

/**
 * @brief returns a string in 36 base (0-9A-Z) left padded up to 'len' with 'padch'
*/
std::string ToBase36Lpad(int64_t number, std::string::size_type len, char padch = '0');

std::string strncpy(const char* s, size_t n);



std::string replaceSubstrCopy(std::string in,
                    const std::string& search_for,
                    const std::string& replace_with);

//obsolete due to strangeNane
inline std::string ReplaceInStr(const std::string& in, const std::string& search_for, const std::string& replace_with)
{
    return replaceSubstrCopy(in,search_for,replace_with);
}
inline std::string delSpaces(const std::string &in)
{
    return replaceSubstrCopy(in, " ", "");
}

// Исключить из строки in все символы characters
std::string Filter(const std::string& in, const std::string& characters);

/// @brief Первая позиция без пробелов (слева)
/// @brief Вернет sir_in.size() если все пробелы
/// @brief Вернет string::npos если все пусто
size_t lNonSpacePos(const std::string &str_in, size_t start=0, size_t finish=std::string::npos);

/// @brief Первая позиция без пробелов (справа)
/// @brief Вернет string::npos если все пробелы
size_t rNonSpacePos(const std::string &str_in, size_t start=std::string::npos, size_t finish=std::string::npos);

inline std::string rtrim(const std::string &str_in)
{
    size_t i = rNonSpacePos(str_in);
    return str_in.substr(0, i !=std::string::npos ? i+1 : 0);
}
inline std::string ltrim(const std::string &str_in)
{
    size_t i = lNonSpacePos(str_in);
    return str_in.substr(i!=std::string::npos ? i : 0);
}
inline std::string trim(const std::string& str_in)
{
    return ltrim(rtrim(str_in));
}
std::string lpad(const std::string &str, size_t num, char chr=' ');
std::string rpad(const std::string &str, size_t num, char chr=' ');

void appendClause(std::string& s1, const std::string& s2);

bool isLatStr(const std::string& str);

class Placeholders {
public:
    template<class T>
    Placeholders& bind(const std::string& placeholder, const T& value)
    {
        std::ostringstream os;
        os << value;
        valueMap_[placeholder] = os.str();
        return *this;
    }

    Placeholders& merge(const Placeholders& phs);

    std::string replace(const std::string& mask) const;
private:
    std::map<std::string, std::string> valueMap_;
};

std::string url_encode(const std::string &value);

std::string translit(const std::string& str, bool useOldRules = false);

} // namespace StrUtils

extern "C"
{
#endif /* __cplusplus */
    /*****************************************************************************/
    /*****************        Extern "C" functions        ************************/
    /*****************************************************************************/
    char *matoi(int *num, const char *str_in);
    char *matol(long *num, const char *str_in);
    /* This function tries to read integer from str_in */
    /* Resulting integer is stored in *num */
    /* Returns NULL on succesful read, or pointer to first non-digit character */
    /* if failes */

    int atoiNVL_(const char *str_in, int NVL, const char* file, int line);
#define atoiNVL(str_in, NVL) atoiNVL_(str_in, NVL, __FILE__, __LINE__)

    int atoiNVLTrim(const char *str_in, int NVL);
    int atoinNVL(const char *str_in, int n, int NVL);

    int atoiChar(const char c);

    int getDate(const char *xmldate, char *rrmmdd);
    int setDate(const char *rrmmdd, char *xmldate);

    char *rtrim(char *str_in); /* str_in must be 0-terminated */
    char *ltrim(char *str_in); /* str_in must be 0-terminated */
    char *trim(char *str_in); /* str_in must be 0-terminated */
    char *rpad(char *str_in, int len, char c);  /* str_in must be 0-terminated */
    char *lpad(char *str_in, int len, char c);  /* str_in must be 0-terminated */

    char *UpperCaseN(char *InOutStr, const size_t InOutLen);
    char *UpperCase(char *InOutStr);
    char *LowerCaseN(char *InOutStr, const size_t InOutLen);
    char *LowerCase(char *InOutStr);

    int check_time(const char *str);

#ifdef __cplusplus
}

bool checkedAtoi(const char *str_in, int & result);

namespace StrUtils
{

void UtfUpperCase( std::string& InOutStr );

void UpperCase( std::string& InOutStr );
void LowerCase( std::string& InOutStr );

/*****************************************************************************/
/*****************      Non-'extern "C"' functions      **********************/
/*****************************************************************************/
int atoiTrim(const std::string& str_in);
int isEmptyStr(const char *str);
// returns 1 on true, 0 on false (checks by isspace())

void ParseStringToVecInt(std::vector<int> &vec, const std::string &str,
                         const char delim=',');
void ParseStringToVecStr(std::vector<std::string> &vec, const std::string &str, const char delim=',');
void split_string(std::vector<std::string>& vec, const std::string& str,
                  char delim=',', bool allow_empty_strings=false);
std::vector<std::string> str_parts(std::string const &str, size_t psz);
std::vector<std::string> str_parts_utf8(const std::string& str, size_t psz);

enum class KeepEmptyTokens { False, True };

template <typename T, typename C> auto push_back_or_insert(int  , C& c, T&& elem) -> decltype(c.push_back(elem)) { return c.push_back(std::forward<T>(elem)); }
template <typename T, typename C> auto push_back_or_insert(long , C& c, T&& elem) { return c.insert(std::forward<T>(elem)); }

template <typename T, typename SP>
T & split_string(T & res, const std::string &s, SP c, KeepEmptyTokens allow_empty_tokens = KeepEmptyTokens::True)
{
    res.clear();
    auto fill_with_empty = [&res,allow_empty_tokens](){ if(allow_empty_tokens == KeepEmptyTokens::True) push_back_or_insert(0, res, std::string()); };
    std::string::size_type l = 0, sz = s.size();
    for(std::string::size_type r = 0; l < sz; l = r + 1)
    {
        r = s.find_first_of(c, l);
        if(r == std::string::npos)
            r = sz;
        if (l == r) {
            fill_with_empty();
        } else {
            push_back_or_insert(0, res, s.substr(l, r - l));
        }
    }
    if (l == sz) {
        fill_with_empty();
    }
    return res;
}
template <template <typename...> class OutputContainer = std::vector, typename SP>
auto split_string(const std::string &s, SP c)
{
    OutputContainer<std::string> res;
    split_string(res,s,c,KeepEmptyTokens::False);
    return res;
}

template <typename T, typename SP>
T split_string(const std::string &s, SP c, KeepEmptyTokens allow_empty_tokens = KeepEmptyTokens::True)
{
    T res;
    split_string<T>(res,s,c,allow_empty_tokens);
    return res;
}

std::string StringRTrim(std::string str);
std::string StringLTrim(std::string str);
std::string StringTrim(std::string str);

std::string StringLTrimCh(const std::string &str, const char ch);

void StringRTrimCh(std::string &str, char ch);

void StringRTrim(std::string *str);
void StringLTrim(std::string *str);
void StringTrim(std::string *str);

std::string remove_escapes(std::string str);

std::string b64_encode(const char *in, int in_len); // закодировать в base64
std::string b64_encode(const std::string &in); // закодировать в base64
std::string b64_decode(const char *in, int in_len); // раскодировать из base64
std::string b64_decode(const std::string &in); // раскодировать из base64

void replaceSubstr(std::string &text, std::string const &what, std::string const &with);
std::string::size_type removeBetween(std::string &str, std::string const &s1, std::string const &s2);

bool containsLat(const std::string &str_in);
bool containsRus(const std::string &str_in);
bool isStrAllDigit(const std::string &str_in);
bool isStrAlphaRus(const std::string& str_in);
bool isStrAlphaNumeric(const std::string& str_in);
bool isStrAlphaNumericRus(const std::string& str_in);
bool isASCII(const std::string& str_in);

bool isNumericValue(const std::string &text, const std::string &dot = std::string());

std::string normalizeUCNames(const std::string &uc_name);
// Удаляет пробелы слева-справа, делает UpperCase первым буквам после пробела и минуса, LowerCase - остальным

struct ReplacementRule
{
    char charToReplace;
    std::function<bool(char)> prevCharRule;
    std::function<bool(char)> nextCharRule;
    std::string replacement_en;
    std::string replacement_ru;
};
std::string textCensor(const std::string& text,
        const std::function<bool(char)>& isAllowedChar,
        const std::vector<ReplacementRule>& replacementRules);

namespace details
{

template<typename T>
struct StringCaster
{
    std::string operator()(const T& t) const {
        return HelpCpp::string_cast(t);
    }
};

template<>
struct StringCaster<std::string>
{
    std::string operator()(const std::string& t) const {
        return t;
    }
};

template<>
struct StringCaster<const char*>
{
    std::string operator()(const char* t) const {
        return t;
    }
};

} // details

template<typename T, typename F>
std::string join(const char* d, const T* beg, const T* end, const F& func)
{
    if (beg == end)
        return std::string();
    const T* i = beg;
    std::string s;
    for (s += func(*i++); i != end; ++i)
        s += d + func(*i);
    return s;
}

template<typename T>
std::string join(const char* d, const T* beg, const T* end)
{
    return join<T>(d, beg, end, details::StringCaster<T>());
}

template<typename T, typename F>
std::string join(const char* d, const T& beg, const T& end, const F& func)
{
    if (beg == end)
        return std::string();
    T i = beg;
    std::ostringstream st;
    for (st << func(*i++); i != end; ++i)
        st << d << func(*i);
    return st.str();
}

template<typename T>
std::string join(const char* d, const T& beg, const T& end)
{
    return join<T>(d, beg, end, details::StringCaster<typename T::value_type>());
}

template<typename T, typename F>
std::string join(const char* d, const T& container, const F& func)
{
    return join(d, std::begin(container), std::end(container), func);
}

template<typename T> typename std::enable_if<not std::is_same<typename T::value_type, std::string>::value,std::string>::type
join(const char* d, const T& container)
{
    return join(d, std::begin(container), std::end(container), details::StringCaster<typename T::value_type>());
}

inline std::string strNvl(const std::string &a, const std::string &b)
{
  if(trim(a).empty())
    return b;
  return a;
}

template <typename C> typename std::enable_if<std::is_same<typename C::value_type, std::string>::value,std::string>::type join(const char* d, const C& container)
{
    if(container.empty())
        return std::string();
    std::string res;
    size_t z = container.size();
    for(auto& c : container)
        z += c.size();
    res.reserve(z);
    auto i = std::begin(container);
    res = *i++;
    for(auto e = std::end(container); i!= e; ++i)
        res.append(d).append(*i);
    return res;
}

template <typename C> typename std::enable_if<std::is_same<typename C::value_type, std::string>::value,std::string>::type join(const char d, const C& container)
{
    const char _d[2] = {d,0};
    return join(_d, container);
}

} // namespace StrUtils


// Copies min(strlen(src), dest_size-1) characters from src to dest
// and appends the terminating null character. Returns dest.
char *safe_strcpy(char *dest, size_t dest_size, const char *src);
inline char *safe_strcpy(char *dest, size_t dest_size, const std::string &src)
  { return safe_strcpy(dest, dest_size, src.c_str()); }

//Same as upper,but with 2 argumets
template <size_t N> void safe_strcpy(char (&dst)[N], const std::string &src, size_t up_to = std::string::npos)
{
    if(N>0) { dst[ src.copy(dst, std::min(up_to, N-1)) ] = '\0'; }
}

#endif /* __cplusplus */

#endif /*_STR_UTILS_H_*/
