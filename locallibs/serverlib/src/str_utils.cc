#if HAVE_CONFIG_H
#endif

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <string>
#include <exception>
#include <vector>
#include <cstdio>
#include <iomanip>
#include <set>
#include <errno.h>
#include <limits.h>

#include "str_utils.h"
#include "isdigit.h"
#include "helpcpp.h"
#include "string_cast.h"
#include "base64.h"

#define NICKNAME "SYSTEM"
#include "slogger.h"

using namespace std;
using namespace StrUtils;
static char LatSml[]="abcdefghijklmnopqrstuvwxyz";
static char LatBig[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static char RusSml[]="абвгдеёжзийклмнопрстуфхцчшщьъыэюя";
static char RusBig[]="АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЬЪЫЭЮЯ";

namespace StrUtils
{

void UtfUpperCase( std::string& InOutStr )
{
    InOutStr = EncString::from866(ToUpper(EncString::fromUtf(InOutStr).to866())).toUtf();
}

void UpperCase( std::string& InOutStr )
{
    size_t  RusLen=strlen(RusSml);

    for( size_t i=0; i<InOutStr.size(); i++)
    {
        void *ptr;

        if (InOutStr[i]>='a' && InOutStr[i]<='z')
        {
            InOutStr[i]=toupper(InOutStr[i]);
            continue;
        };
        if ((unsigned)InOutStr[i]<='\177')
            continue;
        if ((ptr=memchr(RusSml,InOutStr[i],RusLen))!=NULL)
            InOutStr[i]=RusBig[(char*)ptr-RusSml];
    };
}

void LowerCase( std::string& InOutStr )
{
    size_t  RusLen=strlen(RusSml);

    for(size_t i=0; i<InOutStr.size(); i++)
    {
        void *ptr;

        if (InOutStr[i]>='A' && InOutStr[i]<='Z')
        {
            InOutStr[i]=tolower(InOutStr[i]);
            continue;
        };
        if ((unsigned)InOutStr[i]<='\177')
            continue;
        if ((ptr=memchr(RusBig,InOutStr[i],RusLen))!=NULL)
            InOutStr[i]=RusSml[(char*)ptr-RusBig];
    };
}

std::string RPad(const std::string &str_in, std::string::size_type len, char c)
{
    std::string::size_type padlen =
        len > str_in.length() ? len - str_in.length() : 0;
    return str_in + std::string(padlen, c);
}

std::string LPad(const std::string &str_in, std::string::size_type len, char c)
{
    std::string::size_type padlen =
        len > str_in.length() ? len - str_in.length() : 0;
    return std::string(padlen, c) + str_in;
}

static std::string FromBase10(int64_t number, int target_base)
{
    static const char ZERO_NINE_A_Z[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    constexpr int max_target_base = sizeof(ZERO_NINE_A_Z)-1;
    if (target_base < 2 || target_base > max_target_base /*36*/)
    {
        LogTrace(TRACE3) << "target_base: " << target_base <<
                            " > max_target_base: " << max_target_base;
        return std::string();
    }
    if (target_base == 10)
        return HelpCpp::string_cast(number);

    int64_t current = number;
    int64_t rest;
    string res;

    while (current > 0)
    {
        rest = current % target_base;
        current = current / target_base;

        res = ZERO_NINE_A_Z[rest] + res;
    }

    return res;
}

std::string FromBase10Lpad(int64_t number, int target_base, std::string::size_type len, char padch)
{
    return LPad(FromBase10(number, target_base), len, padch);
}

/**
 * @brief returns a string in 36 base (0-9A-Z)
*/
#define ToBase36(number) FromBase10((number), 36)

std::string ToBase36Lpad(int64_t number, std::string::size_type len, char padch)
{
    return LPad(ToBase36(number), len, padch);
}

void split_string(vector<string>& vec, const string& str, char delim, bool allow_empty_strings)
{
    split_string<std::vector<std::string>>(vec,str,delim,allow_empty_strings ? StrUtils::KeepEmptyTokens::True : StrUtils::KeepEmptyTokens::False);
}

std::vector<std::string> str_parts(std::string const &str, size_t psz)
{
    std::vector<std::string> ret;
    size_t const sz = str.size();
    for (size_t pos = 0; pos < sz; pos += psz) {
        ret.push_back(str.substr(pos, psz));
    }
    return ret;
}

std::vector<std::string> str_parts_utf8(const std::string& str, const size_t psz)
{
    const static auto utf8StartMask = '\xC0';
    const static auto utf8ContIndic = '\x80';

    std::vector<std::string> ret;

    const auto sz = str.size();
    for (size_t pos = 0; pos < sz;) {
        auto endPos = pos + psz;
        while ((endPos < str.size()) && ((str[endPos] & utf8StartMask) == utf8ContIndic)) {
            endPos -= 1;
        }

        ret.push_back(str.substr(pos, endPos - pos));
        pos = endPos;
    }

    return ret;
}

std::string StringRTrim(std::string tmp)
{
    StringRTrim(&tmp);
    return tmp;
}

std::string StringLTrim(std::string tmp)
{
    StringLTrim(&tmp);
    return tmp;
}

std::string StringTrim(std::string tmp)
{
    StringTrim(&tmp);
    return tmp;
}

void StringRTrim(string *str)
{
    int i = str->size() - 1;
    for(; i>=0 && ISSPACE((*str)[i]); i--);
    str->erase(i + 1);
}

void StringLTrim(string *str)
{
    string::size_type i=0;
    for(; i<str->size() && ISSPACE((*str)[i]); i++);
    str->erase(0, i);
}

void StringTrim(string *str)
{
    StringLTrim(str);
    StringRTrim(str);
}

std::string StringLTrimCh(const std::string &str, const char ch)
{
    for (std::string::size_type i = 0; i < str.size(); ++i) {
        if (str[i] != ch) {
            return str.substr(i);
        }
    }
    return std::string();
}

void StringRTrimCh(std::string &str, char ch)
{
    int i = str.size() - 1;
    for(; i>=0 && str[i]==ch; i--);
    str.erase(i + 1);
}

std::string remove_escapes(std::string ret)
{
    for(std::string::size_type i=ret.find('\033'); i!=std::string::npos;
      i=ret.find('\033',i))
    {
        if(i+1 >= ret.size())
            ret.erase(i);
        else if(ret[i+1]=='w')
        {
            const std::string::size_type x = ret.find("\033x",i);
            ret.erase(i, x==std::string::npos ? std::string::npos : x+2-i);
        }
        else if(ret[i+1]=='Y')
            ret.erase(i,4);
        else
            ret.erase(i,2);
    }
    return ret;
}

void replaceSubstr(std::string &text, std::string const &what, std::string const &with)
{
    if(what.empty())
        return;
    for(string::size_type index=0; index=text.find(what, index), index!=string::npos;)
    {
        text.replace(index, what.length(), with);
        index+=with.length();
    }
}

std::string::size_type removeBetween(std::string &str, std::string const & s1, std::string const &s2)
{
    auto ret = std::string::npos;
    for(auto pos=str.find(s1); pos!=str.npos; pos=str.find(s1,pos))
    {
        string::size_type fin=str.find(s2,pos+s1.size());
        if(fin==str.npos)
            break;
        str.replace(pos, fin-pos+s2.size(), std::string());
        ret = pos;
    }
    return ret;
}

void ParseStringToVecStr(std::vector<std::string> &vec, const std::string &str, const char delim)
{
    vec.clear();

    string::size_type pos=0, prev_pos=0;
    while(1)
    {
        pos=str.substr(prev_pos).find(delim);
        if(pos==string::npos)
        {
            vec.push_back(str.substr(prev_pos));
      //res+=str_in.substr(prev_pos);
            break;
        }
    //res+=str_in.substr(prev_pos,pos);
        vec.push_back(str.substr(prev_pos,pos));
        prev_pos+=pos+1;
    }
}

void ParseStringToVecInt(vector<int> &vec, const string &str, const char delim)
{
    string::size_type pos=0, prev_pos=0;
    while(1)
    {
        pos=str.substr(prev_pos).find(delim);
        if(pos==string::npos)
        {
            vec.push_back(atoi(str.substr(prev_pos).c_str()));
      //res+=str_in.substr(prev_pos);
            break;
        }
    //res+=str_in.substr(prev_pos,pos);
        vec.push_back(atoi(str.substr(prev_pos,pos).c_str()));
        prev_pos+=pos+1;
    }
}

bool isStrAllDigit(const std::string &str)
{
    for(std::string::const_iterator i=str.begin();i!=str.end();++i)
    {
        if(!ISDIGIT(*i))
            return false;
    }
    return true;
}

bool isStrAlphaRus(const std::string& str)
{
    for (std::string::const_iterator i = str.begin(); i != str.end(); ++i) {
        if (!isalpha_rus(*i))
            return false;
    }
    return true;
}

bool isStrAlphaNumeric(const std::string& str)
{
    for (std::string::const_iterator i = str.begin(); i != str.end(); ++i) {
        if (!isalnum(*i))
            return false;
    }
    return true;
}

bool isStrAlphaNumericRus(const std::string& str)
{
    for (std::string::const_iterator i = str.begin(); i != str.end(); ++i) {
        if (!isalnum_rus(*i))
            return false;
    }
    return true;
}

bool isASCII(const std::string& str)
{
    return std::all_of(str.cbegin(), str.cend(), isascii);
}

bool isNumericValue(const std::string &text, const string &dot)
{
    size_t dotCount = 0;
    for (std::string::const_iterator i = text.begin(); i != text.end(); ++i) {
        if (!ISDIGIT(*i)) {
            if (!dot.empty() && dotCount < 1 && dot.find(*i) != std::string::npos) {
                ++dotCount;
                continue;
            }
            return false;
        }
    }
    return true;
}

int isEmptyStr(const char *str)
{
    if(!str)
        return 1;
    for(int i=strlen(str)-1;i>=0;i--)
        if(!ISSPACE(str[i]))
            return 0;
    return 1;
}

bool containsLat(const string &str_in)
{
    for(size_t i = 0; i < str_in.size(); ++i)
    {
        if(IsLatChar(str_in[i]))
        {
            return true;
        }
    }
    return false;
}

bool containsRus(const string &str_in)
{
    for(size_t i = 0; i < str_in.size(); ++i)
    {
        if(IsRusChar(str_in[i]))
        {
            return true;
        }
    }
    return false;
}


int atoiTrim(const string& str)
{
    string aaa=StringTrim(str);
    return atoi(aaa.c_str());
}
string b64_encode(const char *in, int in_len)
{
    string res;
    base64_encode((const unsigned char*)in, in_len, res);
    return res;
}

string b64_encode(const std::string &in)
{
  return b64_encode(in.c_str(),in.length());
}

string b64_decode(const char *in, int in_len)
{
    std::vector<unsigned char> tmp;
    base64_decode(in, in_len, tmp);
    return std::string(tmp.begin(), tmp.end());
}

string b64_decode(const std::string &in)
{
  return b64_decode(in.c_str(),in.length());
}

namespace
{
unsigned char tup(unsigned char c)
{
  if(c>=(unsigned char)'а' && c<=(unsigned char)'п')      return c-32;
  else if(c>=(unsigned char)'р' && c<=(unsigned char)'я') return c-80;
  else if(c==(unsigned char)'ё')                          return 'Ё';
  else                                                    return toupper(c);
}

unsigned char tlo(unsigned char c)
{
  if(c>=(unsigned char)'А' && c<=(unsigned char)'П')      return c+32;
  else if(c>=(unsigned char)'Р' && c<=(unsigned char)'Я') return c+80;
  else if(c==(unsigned char)'Ё')                          return 'ё';
  else                                                    return tolower(c);
}

} // anonymous namespace

bool IsUpperChar(unsigned char c)
{
  return c==tup(c);
}

bool IsLowerChar(unsigned char c)
{
  return c==tlo(c);
}

bool IsUpper(const std::string &s)
{
  for(auto c : s)
    if(!IsUpperChar(c))
      return false;
  return true;
}

bool IsLower(const std::string &s)
{
  for(auto c : s)
    if(!IsLowerChar(c))
      return false;
  return true;
}

std::string ToUpper( std::string s )
{
    for(size_t i = 0; i < s.length(); ++i)
      s[i]=tup(s[i]);
    return s;
}
std::string ToLower( std::string s )
{
    for ( size_t i = 0 ; i < s.length() ; ++i )
      s[i]=tlo(s[i]);
    return s;
}

std::string normalize(std::string&& in)
{
    const auto e = in.end();
    auto i = in.begin();
    if(i != e)
    {
        *i = tup(*i);
        for(++i ; i != e; ++i)
            *i = tlo(*i);
    }
    return std::move(in);
}

std::string normalize(const std::string& in)
{
    return normalize(std::string(in));
}

bool IsRusChar(unsigned char ch)
{
  return ch!='\0'  &&  (strchr(RusSml, ch) || strchr(RusBig, ch));
}

bool IsLatChar(unsigned char ch)
{
  return ch!='\0'  &&  (strchr(LatSml, ch) || strchr(LatBig, ch));
}

bool IsLatOrRusChar(unsigned char ch)
{
  return IsLatChar(ch) || IsRusChar(ch);
}

static bool isrusal(char ch)
{
    return (ch != '\0' && strchr(RusBig, ch));
}

bool isalnum_rus(char ch)
{
    return isalnum((unsigned char)ch) || isrusal(ch);
}

bool isalpha_rus(char ch)
{
    return isalpha((unsigned char)ch) || isrusal(ch);
}

std::string normalizeUCNames(const std::string &uc_name)
{
  std::string res;

  bool needUP=true;
  bool eat_space=false;
  bool add_space=false;
  for(std::string::const_iterator i=uc_name.begin();i!=uc_name.end();++i)
  {
    if(ISSPACE(*i))
    {
      if(res.empty() || eat_space)
        continue;
      eat_space=true;
      add_space=true;
      needUP=true;
      continue;
    }
    else
    {
      eat_space=false;
      if(add_space)
      {
        res.append(" ");
        add_space=false;
      }
    }
    if(*i=='-' || *i=='/' || *i=='\'')
    {
      res.append(string(1,*i));
      needUP=true;
      continue;
    }
    if(needUP)
    {
      res.append(1,tup(*i));
      needUP=false;
      continue;
    }
    res.append(1,tlo(*i));
  }
  replaceSubstr(res, "-На-", "-на-");
  return res;
}

namespace {

Language textLanguage(const std::string& text)
{
    for (char c : text) {
        if (StrUtils::IsRusChar(c)) {
            return RUSSIAN;
        }
    }
    return ENGLISH;
}

std::string smartReplace(char charToReplace, char prevChar, char nextChar, Language language,
        const std::vector<StrUtils::ReplacementRule>& replacementRules)
{
    for (const StrUtils::ReplacementRule& rule: replacementRules)
    {
        if (rule.charToReplace == charToReplace
                && rule.prevCharRule(prevChar) && rule.nextCharRule(nextChar)) {
            return language == RUSSIAN ? rule.replacement_ru : rule.replacement_en;
        }
    }
    return " ";
}

}

std::string textCensor(const std::string& text, const std::function<bool(char)>& isAllowedChar,
        const std::vector<ReplacementRule>& replacementRules)
{
    const std::string upperText = ToUpper(text);
    std::string censoredText;

    for (size_t i = 0; i < upperText.size(); ++i)
    {
        const char currChar = upperText.at(i);
        if (isAllowedChar(currChar))
        {
            censoredText += currChar;
            continue;
        }
        const char prev = i > 0 ? upperText.at(i - 1) : '\0';
        const char next = i + 1 < upperText.size() ? upperText.at(i + 1) : '\0';
        censoredText += smartReplace(currChar, prev, next, textLanguage(upperText), replacementRules);
    }
    censoredText = trim(censoredText);
    //удалить двойные пробелы
    censoredText.erase(std::unique(censoredText.begin(), censoredText.end(),
                                   [](char one, char two) { return one == ' ' && two == ' '; }),
                       censoredText.end());
    return censoredText;
}

/// ======= moved from tick_serv/src/utils.h
size_t lNonSpacePos(const string &str_in, size_t start, size_t finish)
{
    size_t i=0;
    if(start>=str_in.size())
    {
        return string::npos;
    }
    finish = std::min(finish, str_in.size());
    for(i=start; i<finish; ++i)
    {
        if(!ISSPACE(str_in.at(i)))
            return i;
    }
    return finish;
}

size_t rNonSpacePos(const string &str_in, size_t start, size_t finish)
{
    if(start==string::npos)
    {
        start = str_in.size()-1;
    }
    if(start>=str_in.size())
    {
        return string::npos;
    }
    for(size_t i=start; i!=finish; --i)
    {
        if(!ISSPACE(str_in.at(i)))
            return i;
    }
    return finish;
}

std::string strncpy(const char* s, size_t n)
{
    char buff[n + 1];
    buff[n] = '\0';
    return ::strncpy(buff, s, n);
}


std::string replaceSubstrCopy(std::string s,
                    const string& search_for,
                    const string& replace_with)
{
    replaceSubstr(s, search_for, replace_with);
    return s;
}

std::string Filter(const std::string& in, const std::string& characters)
{
    std::string result;
    for (size_t i = 0; i < in.size(); ++i) {
        const char currChar = in.at(i);
        if (characters.find(currChar) == std::string::npos) {
            result += currChar;
        }
    }
    return result;
}

std::string lpad(const std::string & str, size_t num, char chr)
{
    if(str.length() >= num)
    {
        return str;
    }
    std::string res(num-str.length(), chr);
    res += str;
    return res;
}

std::string rpad(const std::string &str, size_t num, char chr)
{
    if(str.length() >= num)
    {
        return str;
    }
    std::string fill(num-str.length(), chr);
    std::string res;
    res += str;
    res += fill;
    return res;
}
/// end of ======= moved from tick_serv/src/utils.h

void appendClause(std::string& s1, const std::string& s2)
{
    if (s1.empty())
        s1 = s2;
    else
        s1 += " AND " + s2;
}

bool isLatStr(const std::string& str)
{
    for (size_t i = 0, length = str.size(); i < length; ++i) {
        if (!StrUtils::IsLatChar(str[i])) {
            return false;
        }
    }
    return true;
}

/* Возвращает число замен для данного placeholder */
namespace {
    void ReplacePlaceholder(
            string& mask, /* in/out */
            const string& placeholder,
            const string& value)
    {
        size_t pos = string::npos, searchFrom = 0;
        do {
            pos = mask.find(placeholder, searchFrom);
            if (pos != string::npos) {
                mask.replace(pos, placeholder.size(), value);
                searchFrom = pos + value.size();
            }
        } while (pos != string::npos);
    }
}

/*********************************************************************
 * Placeholders
 ********************************************************************/

Placeholders& Placeholders::merge(const Placeholders& phs)
{
    for (map<string, string>::const_iterator it = phs.valueMap_.begin();
         it != phs.valueMap_.end();
         ++it)
    {
        valueMap_.insert(*it);
    }
    return *this;
}

static bool sortPlaceholderValues(const std::pair<std::string, std::string>& p1,
        const std::pair<std::string, std::string>& p2)
{
    return ((p1.first.length() > p2.first.length())
        || (p1.first.length() == p2.first.length() && p1.first < p2.first));
}

std::string Placeholders::replace(const std::string& mask) const
{
    string result = mask;
    typedef std::set<std::pair<string, string>,
            bool(*)(const std::pair<std::string, std::string>&, const std::pair<std::string, std::string>&)> set_t;
    set_t values(valueMap_.begin(), valueMap_.end(), sortPlaceholderValues);
    for (set_t::const_iterator it = values.begin(); it != values.end(); ++it) {
        ReplacePlaceholder(result, it->first, it->second);
    }
    return result;
}

std::string url_encode(const std::string &value)
{
  std::ostringstream escaped;
  escaped.fill('0');
  escaped << std::hex;

  for(std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i)
  {
    string::value_type c = (*i);
    if(isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
    {
      escaped << c;
    }
    else if(c == ' ')
    {
      escaped << '+';
    }
    else
    {
      char x[10]="";
      sprintf(x,"%%%hhX",c);
      escaped<<x;
      //escaped << '%' << std::setw(2) << ((int) c & 0xff) << std::setw(0);
    }
  }

  return escaped.str();
}

static const char* trtab_pre_22520[256] = {
             "",     "\x01",     "\x02",     "\x03",
         "\x04",     "\x05",     "\x06",     "\x07",
         "\x08",     "\x09",     "\x0a",     "\x0b",
         "\x0c",     "\x0d",     "\x0e",     "\x0f",
         "\x10",     "\x11",     "\x12",     "\x13",
         "\x14",     "\x15",     "\x16",     "\x17",
         "\x18",     "\x19",     "\x1a",     "\x1b",
         "\x1c",     "\x1d",     "\x1e",     "\x1f",
         "\x20",     "\x21",     "\x22",     "\x23",
         "\x24",     "\x25",     "\x26",     "\x27",
         "\x28",     "\x29",     "\x2a",     "\x2b",
         "\x2c",     "\x2d",     "\x2e",     "\x2f",
         "\x30",     "\x31",     "\x32",     "\x33",
         "\x34",     "\x35",     "\x36",     "\x37",
         "\x38",     "\x39",     "\x3a",     "\x3b",
         "\x3c",     "\x3d",     "\x3e",     "\x3f",
         "\x40",     "\x41",     "\x42",     "\x43",
         "\x44",     "\x45",     "\x46",     "\x47",
         "\x48",     "\x49",     "\x4a",     "\x4b",
         "\x4c",     "\x4d",     "\x4e",     "\x4f",
         "\x50",     "\x51",     "\x52",     "\x53",
         "\x54",     "\x55",     "\x56",     "\x57",
         "\x58",     "\x59",     "\x5a",     "\x5b",
         "\x5c",     "\x5d",     "\x5e",     "\x5f",
         "\x60",     "\x61",     "\x62",     "\x63",
         "\x64",     "\x65",     "\x66",     "\x67",
         "\x68",     "\x69",     "\x6a",     "\x6b",
         "\x6c",     "\x6d",     "\x6e",     "\x6f",
         "\x70",     "\x71",     "\x72",     "\x73",
         "\x74",     "\x75",     "\x76",     "\x77",
         "\x78",     "\x79",     "\x7a",     "\x7b",
         "\x7c",     "\x7d",     "\x7e",     "\x7f",
         "\x41",     "\x42",     "\x56",     "\x47",
         "\x44",     "\x45", "\x5a\x48",     "\x5a",
         "\x49",     "\x59",     "\x4b",     "\x4c",
         "\x4d",     "\x4e",     "\x4f",     "\x50",
         "\x52",     "\x53",     "\x54",     "\x55",
         "\x46", "\x4b\x48", "\x54\x53", "\x43\x48",
     "\x53\x48", "\x53\x48",         "",     "\x59",
             "",     "\x45", "\x59\x55", "\x59\x41",
         "\x61",     "\x62",     "\x76",     "\x67",
         "\x64",     "\x65", "\x7a\x68",     "\x7a",
         "\x69",     "\x79",     "\x6b",     "\x6c",
         "\x6d",     "\x6e",     "\x6f",     "\x70",
         "\xb0",     "\xb1",     "\xb2",     "\xb3",
         "\xb4",     "\xb5",     "\xb6",     "\xb7",
         "\xb8",     "\xb9",     "\xba",     "\xbb",
         "\xbc",     "\xbd",     "\xbe",     "\xbf",
         "\xc0",     "\xc1",     "\xc2",     "\xc3",
         "\xc4",     "\xc5",     "\xc6",     "\xc7",
         "\xc8",     "\xc9",     "\xca",     "\xcb",
         "\xcc",     "\xcd",     "\xce",     "\xcf",
         "\xd0",     "\xd1",     "\xd2",     "\xd3",
         "\xd4",     "\xd5",     "\xd6",     "\xd7",
         "\xd8",     "\xd9",     "\xda",     "\xdb",
         "\xdc",     "\xdd",     "\xde",     "\xdf",
         "\x72",     "\x73",     "\x74",     "\x75",
         "\x66", "\x6b\x68", "\x74\x73", "\x63\x68",
     "\x73\x68", "\x73\x68",         "",     "\x79",
             "",     "\x65", "\x79\x75", "\x79\x61",
     "\x59\x4f", "\x79\x6f",     "\xf2",     "\xf3",
         "\xf4",     "\xf5",     "\xf6",     "\xf7",
         "\xf8",     "\xf9",     "\xfa",     "\xfb",
         "\xfc",     "\xfd",     "\xfe",     "\xff"
};

/* Индексом в массиве является код символа (CP866) */
static const char* trtab[256] = {
             "",     "\x01",     "\x02",     "\x03",
         "\x04",     "\x05",     "\x06",     "\x07",
         "\x08",     "\x09",     "\x0a",     "\x0b",
         "\x0c",     "\x0d",     "\x0e",     "\x0f",
         "\x10",     "\x11",     "\x12",     "\x13",
         "\x14",     "\x15",     "\x16",     "\x17",
         "\x18",     "\x19",     "\x1a",     "\x1b",
         "\x1c",     "\x1d",     "\x1e",     "\x1f",
         "\x20",     "\x21",     "\x22",     "\x23",
         "\x24",     "\x25",     "\x26",     "\x27",
         "\x28",     "\x29",     "\x2a",     "\x2b",
         "\x2c",     "\x2d",     "\x2e",     "\x2f",
         "\x30",     "\x31",     "\x32",     "\x33",
         "\x34",     "\x35",     "\x36",     "\x37",
         "\x38",     "\x39",     "\x3a",     "\x3b",
         "\x3c",     "\x3d",     "\x3e",     "\x3f",
         "\x40",     "\x41",     "\x42",     "\x43",
         "\x44",     "\x45",     "\x46",     "\x47",
         "\x48",     "\x49",     "\x4a",     "\x4b",
         "\x4c",     "\x4d",     "\x4e",     "\x4f",
         "\x50",     "\x51",     "\x52",     "\x53",
         "\x54",     "\x55",     "\x56",     "\x57",
         "\x58",     "\x59",     "\x5a",     "\x5b",
         "\x5c",     "\x5d",     "\x5e",     "\x5f",
         "\x60",     "\x61",     "\x62",     "\x63",
         "\x64",     "\x65",     "\x66",     "\x67",
         "\x68",     "\x69",     "\x6a",     "\x6b",
         "\x6c",     "\x6d",     "\x6e",     "\x6f",
         "\x70",     "\x71",     "\x72",     "\x73",
         "\x74",     "\x75",     "\x76",     "\x77",
         "\x78",     "\x79",     "\x7a",     "\x7b",
         "\x7c",     "\x7d",     "\x7e",     "\x7f",
            "A",        "B",        "V",        "G",
            "D",        "E",       "ZH",        "Z",
            "I",        "I",        "K",        "L",
            "M",        "N",        "O",        "P",
            "R",        "S",        "T",        "U",
            "F",       "KH",       "TS",       "CH",
           "SH",     "SHCH",       "IE",        "Y",
             "",        "E",       "IU",       "IA",
            "a",        "b",        "v",        "g",
            "d",        "e",       "zh",        "z",
            "i",        "i",        "k",        "l",
            "m",        "n",        "o",        "p",
         "\xb0",     "\xb1",     "\xb2",     "\xb3",
         "\xb4",     "\xb5",     "\xb6",     "\xb7",
         "\xb8",     "\xb9",     "\xba",     "\xbb",
         "\xbc",     "\xbd",     "\xbe",     "\xbf",
         "\xc0",     "\xc1",     "\xc2",     "\xc3",
         "\xc4",     "\xc5",     "\xc6",     "\xc7",
         "\xc8",     "\xc9",     "\xca",     "\xcb",
         "\xcc",     "\xcd",     "\xce",     "\xcf",
         "\xd0",     "\xd1",     "\xd2",     "\xd3",
         "\xd4",     "\xd5",     "\xd6",     "\xd7",
         "\xd8",     "\xd9",     "\xda",     "\xdb",
         "\xdc",     "\xdd",     "\xde",     "\xdf",
            "r",        "s",        "t",        "u",
            "f",       "kh",       "ts",       "ch",
           "sh",     "shch",       "ie",        "y",
             "",        "e",       "iu",       "ia",
            "E",        "e",     "\xf2",     "\xf3",
         "\xf4",     "\xf5",     "\xf6",     "\xf7",
         "\xf8",     "\xf9",     "\xfa",     "\xfb",
         "\xfc",     "\xfd",     "\xfe",     "\xff"
};

std::string translit(const std::string& str, bool useOldRules)
{
    std::string result;
    // Оставим запас в несколько символов, т.к. длина строки может увеличиться при транслитерации
    result.reserve(str.size() + 10);

    if (useOldRules) {
        for (std::string::const_iterator c = str.begin(); c != str.end(); ++c)
            result += trtab_pre_22520[(unsigned char)*c];
    }
    else {
        for (std::string::const_iterator c = str.begin(); c != str.end(); ++c)
            result += trtab[(unsigned char)*c];
    }

    return result;
}

} // namespace StrUtils

char *safe_strcpy(char *dest, size_t dest_size, const char *src)
{
  if(dest_size==0)
    return dest;
  size_t ind = 0;
  while(src[ind]!='\0' && ind+1<dest_size)
  {
    dest[ind] = src[ind];
    ++ind;
  }
  dest[ind] = '\0';
  return dest;
}

char *UpperCaseN(char *InOutStr, const size_t InOutLen)
{
    const size_t RusLen=strlen(RusSml);

    for(size_t i=0; i<InOutLen; i++)
    {
        void *ptr;

        if (InOutStr[i]>='a' && InOutStr[i]<='z')
        {
            InOutStr[i]=toupper(InOutStr[i]);
            continue;
        };
        if ((unsigned)InOutStr[i]<='\177')
            continue;
        if ((ptr=memchr(RusSml,InOutStr[i],RusLen))!=NULL)
            InOutStr[i]=RusBig[(char*)ptr-RusSml];
    };
    return InOutStr;
}


char *UpperCase(char *InOutStr)
{
    return UpperCaseN(InOutStr,strlen(InOutStr));
}

char *LowerCaseN(char *InOutStr, const size_t InOutLen)
{
    const size_t RusLen=strlen(RusSml);

    for(size_t i=0; i<InOutLen; i++)
    {
        void *ptr;

        if (InOutStr[i]>='A' && InOutStr[i]<='Z')
        {
            InOutStr[i]=tolower(InOutStr[i]);
            continue;
        };
        if ((unsigned)InOutStr[i]<='\177')
            continue;
        if ((ptr=memchr(RusBig,InOutStr[i],RusLen))!=NULL)
            InOutStr[i]=RusSml[(char*)ptr-RusBig];
    };
    return InOutStr;
}

char *LowerCase(char *InOutStr)
{
    return LowerCaseN(InOutStr,strlen(InOutStr));
}

bool checkedAtoi__(const char *str_in, long int &ret, char* &ptr )
{
    if(!str_in)
        return false;
      
  // не считаем пустую строку ошибкой
    if (str_in!=nullptr && str_in[0]=='\0')
    {
      ret=0;
      ptr=const_cast<char*>(str_in);
      return true;
    }
    
    errno = 0;    /* To distinguish success/failure after call */      
    long int tmp = strtol(str_in,&ptr,10);
    if (ptr[0]!='\0'
      || (errno == ERANGE && (tmp == LONG_MAX || tmp == LONG_MIN))
      || (errno != 0 && tmp == 0))
    {
        return false;
    }

    ret = tmp;
    return true;
}

bool checkedAtoi(const char *str_in, int &ret )
{
  char *ptr=nullptr;
  long int tmp=0;
  if (!checkedAtoi__(str_in, tmp, ptr ))
    return false;
  ret=tmp;
  return true;
}

template<typename T>
char *mato_t(T *num, const char *str_in)
{
    if(!str_in || !num)
        return nullptr;

    char *ptr=nullptr;
    long int tmp=0;
    if (!checkedAtoi__(str_in, tmp, ptr ))
      return ptr;

    *num=tmp;  
    return nullptr;
}

char *matoi(int *num,const char *str_in)
{
    return mato_t(num, str_in);
}

char *matol(long int *num,const char *str_in)
{
    return mato_t(num, str_in);
}


int atoiNVL_(const char *str_in, int NVL, const char* file, int line)
{
    int res = 0;
    res = checkedAtoi( str_in, res ) ? res : NVL;
    if(str_in && !*str_in and NVL) {
        WriteLog(STDLOG, "25368_atoiNVL: %s:%d, default=%d, result=%d", file, line, NVL, res);
    }
    return res;
}

int atoiNVLTrim(const char *str_in, int NVL)
{
    if(!str_in)
        return NVL;
    std::string aaa(str_in);
    StringTrim(&aaa);
    return atoiNVL(aaa.c_str(),NVL);
}

int atoinNVL(const char *str_in, int n, int NVL)
{
    std::string tmp(str_in,n);
    return atoiNVL(tmp.c_str(),NVL);
}

int getDate(const char *xmldate, char *rrmmdd)
{
    struct tm *tim;
    char date[13];
    int len=strlen(xmldate);
    if(len<4 || len>15)
        return 1;

    /* дата, которую нам присылает xml-терминал, отличается от time_t большей */
    /* точностью - в ней указаны миллисекунды. Мы их отрезаем. */
    strncpy(date,xmldate,len-3);
    date[len-3]='\0';
    errno = 0;    /* To distinguish success/failure after call */      
    
    int i=0;
    if (!checkedAtoi(date, i ))
        return 1;      /* символы, что указывает на наличие неверных данных */

    time_t t=i;
    tim=localtime(&t); /* эта функция заполняет структуру tm */
    /* tm_year - количество годов, начиная с 1900 */
    /* tm_mon - номер месяца (от 0 до 11) */
    /* tm_day - номер дня месяца (от 1 до 31) */
    if(!tim)
        return 1;
    sprintf(rrmmdd,"%02d%02d%02d",tim->tm_year%100,tim->tm_mon+1,tim->tm_mday);

    return 0;
}

int setDate(const char *rrmmdd, char *xmldate)
{
    struct tm tim;
    time_t t;
    char buf[3];
    int q;

    ProgTrace(TRACE1,"rrmmdd='%s'",rrmmdd);
    if(!rrmmdd)
        return 1;

    sprintf(buf,"%2.2s",rrmmdd);
    q=atoi(buf);
    tim.tm_year=(q<50)?100+q:q;

    sprintf(buf,"%2.2s",rrmmdd+2);
    tim.tm_mon=atoi(buf)-1;

    sprintf(buf,"%2.2s",rrmmdd+4);
    tim.tm_mday=atoi(buf);
    ProgTrace(TRACE1,"rr=%i,mm=%i,dd=%i",tim.tm_year,tim.tm_mon,tim.tm_mday);

    tim.tm_sec=tim.tm_min=tim.tm_hour=0;
    t=mktime(&tim);
    if( t == -1 )
        ProgError(STDLOG, "mktime() failed!");
    ProgTrace(TRACE1,"rrmmdd='%s', t=%li",rrmmdd,t);

    sprintf(xmldate,"%li000",t);
    return 0;
}

char *rtrim(char *str_in) /* str_in must be 0-terminated */
{
    if(!str_in)
        return nullptr;
    for(char* p = str_in + strlen(str_in) - 1; p >= str_in; p--)
    {
        if(not ISSPACE(*p))
            break;
        *p = '\0';
    }
    return str_in;
}

char *ltrim(char *str_in) /* str_in must be 0-terminated */
{
    if(!str_in)
        return nullptr;
    size_t len = strlen(str_in);
    auto s = std::find_if(str_in, str_in+len, [](char c){ return not ISSPACE(c); });
    return static_cast<char*>(memmove(str_in, s, len - (s-str_in) + 1));
}

char *trim(char *str_in) /* str_in must be 0-terminated */
{
    return ltrim(rtrim(str_in));
}

char *rpad(char *str_in, int len, char c)  /* str_in must be 0-terminated */
{
    if(!str_in)
        return NULL;

    strcpy(str_in, RPad(std::string(str_in), len, c).c_str());

    return str_in;
}

char *lpad(char *str_in, int len, char c)  /* str_in must be 0-terminated */
{
    if(!str_in)
        return NULL;

    strcpy(str_in, LPad(std::string(str_in), len, c).c_str());

    return str_in;
}

int check_time(const char *str)
{
    char tmp[3];
    int i;
    if(!str || strlen(str)!=4)
        return 1;
    sprintf(tmp,"%2.2s",str);
    if((i=atoiNVL(tmp,-1))<0 || i>23)
        return 2;
    if((i=atoiNVL(str+2,-1))<0 || i>59)
        return 3;
    return 0;
}

int atoiChar(const char c)
{
    if(!ISDIGIT(c))
        return -1;
    return atoi(string(1,c).c_str());
}

#include "checkunit.h"
#ifdef XP_TESTING
namespace
{
using namespace StrUtils;

START_TEST(is_upper_lower)
{
  fail_unless(IsUpperChar('F')==true,  "IsUpperChar() failed");
  fail_unless(IsUpperChar('Ж')==true,  "IsUpperChar() failed");
  fail_unless(IsUpperChar('Ё')==true,  "IsUpperChar() failed");
  fail_unless(IsUpperChar('?')==true,  "IsUpperChar() failed");
  fail_unless(IsUpperChar('5')==true,  "IsUpperChar() failed");
  fail_unless(IsUpperChar('f')==false, "IsUpperChar() failed");
  fail_unless(IsUpperChar('ж')==false, "IsUpperChar() failed");
  fail_unless(IsUpperChar('ё')==false, "IsUpperChar() failed");

  fail_unless(IsLowerChar('F')==false, "IsLowerChar() failed");
  fail_unless(IsLowerChar('Ж')==false, "IsLowerChar() failed");
  fail_unless(IsLowerChar('Ё')==false, "IsLowerChar() failed");
  fail_unless(IsLowerChar('?')==true,  "IsLowerChar() failed");
  fail_unless(IsLowerChar('5')==true,  "IsLowerChar() failed");
  fail_unless(IsLowerChar('f')==true,  "IsLowerChar() failed");
  fail_unless(IsLowerChar('ж')==true,  "IsLowerChar() failed");
  fail_unless(IsLowerChar('ё')==true,  "IsLowerChar() failed");

  fail_unless(IsUpper("HELLO ВОРЛД !!! 5")==true,  "IsUpper() failed");
  fail_unless(IsUpper("Hello Ворлд !!! 5")==false, "IsUpper() failed");
  fail_unless(IsUpper("HeLlO ВоРлД !!! 5")==false, "IsUpper() failed");
  fail_unless(IsUpper("hello ворлд !!! 5")==false, "IsUpper() failed");

  fail_unless(IsLower("HELLO ВОРЛД !!! 5")==false, "IsLower() failed");
  fail_unless(IsLower("Hello Ворлд !!! 5")==false, "IsLower() failed");
  fail_unless(IsLower("HeLlO ВоРлД !!! 5")==false, "IsLower() failed");
  fail_unless(IsLower("hello ворлд !!! 5")==true,  "IsLower() failed");
}
END_TEST

START_TEST(chk_to_upper)
{
    fail_unless(ToUpper("jopa") == "JOPA","inv upper");
    fail_unless(ToUpper("жопа") == "ЖОПА","inv upper");
    fail_unless(ToUpper("hello ворлд") == "HELLO ВОРЛД","inv upper");
    fail_unless(ToUpper("hello ворлд 123 ЁёЁ lala !*.#;.,(;*)") ==
            "HELLO ВОРЛД 123 ЁЁЁ LALA !*.#;.,(;*)","inv upper");
}
END_TEST;

START_TEST(normalize_string)
{
  fail_unless(normalize("IVANOV")=="Ivanov",(normalize("IVANOV")+" != Ivanov").c_str());
  fail_unless(normalize("ivanov")=="Ivanov",(normalize("ivanov")+" != Ivanov").c_str());
  fail_unless(normalize("IvANoV")=="Ivanov",(normalize("IvANoV")+" != Ivanov").c_str());
  fail_unless(normalize("iVANOv")=="Ivanov",(normalize("iVANOv")+" != Ivanov").c_str());
  fail_unless(normalize("ИВАНОВ")=="Иванов",(normalize("ИВАНОВ")+" != Иванов").c_str());
  fail_unless(normalize("иванов")=="Иванов",(normalize("иванов")+" != Иванов").c_str());
  fail_unless(normalize("ИвАНоВ")=="Иванов",(normalize("ИвАНоВ")+" != Иванов").c_str());
  fail_unless(normalize("иВАНОв")=="Иванов",(normalize("иВАНОв")+" != Иванов").c_str());
}
END_TEST

START_TEST(normalize_UCname)
{
  fail_unless(normalizeUCNames("санкт-петербург")=="Санкт-Петербург",(normalizeUCNames("санкт-петербург")+" != Санкт-Петербург (1)").c_str());
  fail_unless(normalizeUCNames("САНКТ-ПЕТЕРБУРГ")=="Санкт-Петербург",(normalizeUCNames("САНКТ-ПЕТЕРБУРГ")+" != Санкт-Петербург (2)").c_str());
  fail_unless(normalizeUCNames("САНКТ - ПЕТЕРБУРГ НА НЕВЕ ")=="Санкт - Петербург На Неве",(normalizeUCNames("САНКТ - ПЕТЕРБУРГ НА НЕВЕ ")+" != Санкт - Петербург На Неве (3)").c_str());
  fail_unless(normalizeUCNames("РОСТОВ-НА-ДОНУ")=="Ростов-на-Дону",(normalizeUCNames("РОСТОВ-НА-ДОНУ")+" != Ростов-на-Дону (4)").c_str());
}
END_TEST

START_TEST(check_is_rus_lat_char)
{
    fail_unless(IsRusChar('И') == true, "'is rus char' lozhaet");
    fail_unless(IsRusChar('I') == false, "'is rus char' lozhaet");
    fail_unless(IsRusChar('ё') == true, "'is rus char' lozhaet");
    fail_unless(IsRusChar('\0') == false, "IsRusChar() failed");

    fail_unless(IsLatChar('Ж') == false, "IsLatChar() failed");
    fail_unless(IsLatChar('j') == true, "IsLatChar() failed");
    fail_unless(IsLatChar('\0') == false, "IsLatChar() failed");

    fail_unless(IsLatOrRusChar('ц') == true, "IsLatOrRusChar() failed");
    fail_unless(IsLatOrRusChar('Ё') == true, "IsLatOrRusChar() failed");
    fail_unless(IsLatOrRusChar('L') == true, "IsLatOrRusChar() failed");
    fail_unless(IsLatOrRusChar('\0') == false, "IsLatOrRusChar() failed");
}
END_TEST

START_TEST(check_fromBase10)
{
    fail_unless(FromBase10(16,16) == "10", "FromBase10 failed");
    fail_unless(FromBase10(16777215, 16) == "FFFFFF", "FromBase10 failed");
    fail_unless(FromBase10(2, 2) == "10", "FromBase10 failed");
    fail_unless(FromBase10(8, 2) == "1000", "FromBase10 failed");
    fail_unless(ToBase36(35) == "Z", "FromBase10 failed");
    fail_unless(FromBase10(69, 36) == "1X", "FromBase10 failed");
    fail_unless(FromBase10Lpad(69, 36, 6, '0') == "00001X", "FromBase10Lpad failed");
    fail_unless(ToBase36Lpad(69, 6) == "00001X", "FromBase10Lpad failed");
    LogTrace(TRACE3) << "FromBase10Lpad: " << FromBase10Lpad(2821109907455LL /*36 ^ 8 - 1*/, 36, 10);
    fail_unless(FromBase10Lpad(2821109907455LL /*36 ^ 8 - 1*/, 36, 10) == "00ZZZZZZZZ",
                                                            "FromBase10Lpad failed");

    // max signed int
    LogTrace(TRACE3) << "FromBase10Lpad: " << ToBase36(2147483647);
    fail_unless(ToBase36(2147483647) == "ZIK0ZJ", "FromBase10Lpad failed");

    // max unsigned int
    LogTrace(TRACE3) << "FromBase10Lpad: " << ToBase36(4294967295U);
    fail_unless(ToBase36(4294967295U) == "1Z141Z3", "FromBase10Lpad failed");

    LogTrace(TRACE3) << "FromBase10Lpad: 36^10 - 1: " << ToBase36(3656158440062975LL);
    LogTrace(TRACE3) << "FromBase10Lpad: 10^16 - 1: " << ToBase36( 999999999999999LL);
}
END_TEST

START_TEST(check_padding)
{
    char lpadres[10] = "123";
    char rpadres[10] = "123";
    char lpadres2[10] = "123";
    char rpadres2[10] = "123";
    fail_unless(::lpad(lpadres, 5, '.') == std::string("..123"),"::lpad failed");
    fail_unless(::rpad(rpadres, 5, '.') == std::string("123.."),"::rpad failed");
    fail_unless(::lpad(rpadres2, 3, '.') == std::string("123"),"::rpad failed");
    fail_unless(::lpad(lpadres2, 2, '.') == std::string("123"),"::rpad failed");
    fail_unless(LPad("AA", 3, '?') == "?AA", "lpad failed");
    fail_unless(RPad("VV", 5, 'W') == "VVWWW", "rpad failed");
    fail_unless(RPad("VVV", 3, 'W') == "VVV", "rpad failed");
    fail_unless(LPad("VVV", 2, 'W') == "VVV", "lpad failed");
}
END_TEST
START_TEST(test_replaceSubstr)
{
    std::string s;
    s = "aaBBBccDDDDeF";
    replaceSubstr(s,"aaB","cc");
    fail_unless(s=="ccBBccDDDDeF","%s",s.c_str() );
    s = "aaBBBccDDDDcc";
    replaceSubstr(s,"cc","xx");
    fail_unless(s=="aaBBBxxDDDDxx","%s",s.c_str() );

    s = "aaaaccDDaaDDeF";
    replaceSubstr(s,"aa","cc");
    fail_unless(s=="ccccccDDccDDeF","%s",s.c_str() );

    s = "aaBBBccaaBDDDDeFaaB";
    replaceSubstr(s,"aaB","");
    fail_unless(s=="BBccDDDDeF","%s",s.c_str() );
}
END_TEST
START_TEST(test_ReplaceInStr)
{
    std::string s;
    s = "aaBBBccDDDDeF";
    fail_unless(ReplaceInStr(s,"aaB","cc")=="ccBBccDDDDeF","%s",s.c_str() );
    s = "aaBBBccDDDDcc";
    fail_unless(ReplaceInStr(s,"cc","xx")=="aaBBBxxDDDDxx","%s",s.c_str() );

    s = "aaaaccDDaaDDeF";
    ReplaceInStr(s,"aa","cc");
    fail_unless(ReplaceInStr(s,"aa","cc")=="ccccccDDccDDeF","%s",s.c_str() );

    s = "aaBBBccaaBDDDDeFaaB";
    ReplaceInStr(s,"aaB","");
    fail_unless(ReplaceInStr(s,"aaB","")=="BBccDDDDeF","%s",s.c_str() );
}
END_TEST
START_TEST(test_replaceSubstrCopy)
{
    std::string s( "aaBBBccDDDDeF");
    fail_unless(replaceSubstrCopy(s,"aaB","cc")=="ccBBccDDDDeF","%s",s.c_str() );
    s = "aaBBBccDDDDcc";
    fail_unless(replaceSubstrCopy(s,"cc","xx")=="aaBBBxxDDDDxx","%s",s.c_str() );

    s = "aaaaccDDaaDDeF";
    replaceSubstrCopy(s,"aa","cc");
    fail_unless(replaceSubstrCopy(s,"aa","cc")=="ccccccDDccDDeF","%s",s.c_str() );

    s = "aaBBBccaaBDDDDeFaaB";
    replaceSubstrCopy(s,"aaB","");
    fail_unless(replaceSubstrCopy(s,"aaB","")=="BBccDDDDeF","%s",s.c_str() );
}
END_TEST

START_TEST(split1)
{
    vector<string> r1;
    string s="a/bb/ccc";
    r1.push_back("a");
    r1.push_back("bb");
    r1.push_back("ccc");
    vector<string> r2=split_string<vector<string> >(s,'/');
    fail_unless(r1==r2,"a/bb/ccc");
}
END_TEST
START_TEST(split2)
{
    vector<string> r1;
    string s="/a/bb/ccc/dddd";
    r1.push_back("");
    r1.push_back("a");
    r1.push_back("bb");
    r1.push_back("ccc");
    r1.push_back("dddd");
    vector<string> r2=split_string<vector<string> >(s,'/');
    fail_unless(r1==r2,"/a/bb/ccc/dddd");
}
END_TEST
START_TEST(split3)
{
    vector<string> r1;
    string s="//1bb,2ccc,3dddd/";
    r1.push_back("");
    r1.push_back("");
    r1.push_back("1bb");
    r1.push_back("2ccc");
    r1.push_back("3dddd");
    r1.push_back("");
    vector<string> r2=split_string<vector<string> >(s,",/");
//    for(int i=0;i<r2.size();++i){
//        cerr << "<" << r2[i] << ">\n";
//    }
    fail_unless(r1==r2,"//1bb,2ccc,3dddd/");
}
END_TEST

START_TEST(split4)
{
    vector<string> r1;
    string s="a/bb/ccc//";
    r1.push_back("a");
    r1.push_back("bb");
    r1.push_back("ccc");
    r1.push_back("");
    r1.push_back("");
    vector<string> r2=split_string<vector<string> >(s,'/');
    fail_unless(r1==r2,"a/bb/ccc//");
}
END_TEST
START_TEST(split5)
{
    vector<string> r1;
    string s="a";
    r1.push_back("a");
    vector<string> r2=split_string<vector<string> >(s,'/');
    fail_unless(r1==r2,"a");
}
END_TEST
START_TEST(split6)
{
    vector<string> r1;
    const string s = "";
    r1.push_back("");
    vector<string> r2 = split_string<vector<string> >(s, '/', StrUtils::KeepEmptyTokens::True);
    fail_unless(r1 == r2, "empty");
}
END_TEST
START_TEST(split7)
{
    vector<string> r1;
    string s="/";
    r1.push_back("");
    r1.push_back("");
    vector<string> r2=split_string<vector<string> >(s,'/');
    fail_unless(r1==r2,"(empty)(empty)");
}
END_TEST
START_TEST(split_false1)
{
    vector<string> r1;
    string s="a/bb/ccc";
    r1.push_back("a");
    r1.push_back("bb");
    r1.push_back("ccc");
    vector<string> r2=split_string<vector<string> >(s,'/', StrUtils::KeepEmptyTokens::False);
    fail_unless(r1==r2,"a/bb/ccc");
}
END_TEST
START_TEST(split_false2)
{
    vector<string> r1;
    string s="/a/bb/ccc/dddd";
    r1.push_back("a");
    r1.push_back("bb");
    r1.push_back("ccc");
    r1.push_back("dddd");
    vector<string> r2=split_string<vector<string> >(s,'/', StrUtils::KeepEmptyTokens::False);
    fail_unless(r1==r2,"/a/bb/ccc/dddd");
}
END_TEST
START_TEST(split_false3)
{
    vector<string> r1;
    string s="//1bb,2ccc,3dddd/";
    r1.push_back("1bb");
    r1.push_back("2ccc");
    r1.push_back("3dddd");
    vector<string> r2=split_string<vector<string> >(s,",/", StrUtils::KeepEmptyTokens::False);
//    for(int i=0;i<r2.size();++i){
//        cerr << "<" << r2[i] << ">\n";
//    }
    fail_unless(r1==r2,"//1bb,2ccc,3dddd/");
}
END_TEST

START_TEST(split_false4)
{
    vector<string> r1;
    string s="a/bb/ccc//";
    r1.push_back("a");
    r1.push_back("bb");
    r1.push_back("ccc");
    vector<string> r2=split_string<vector<string> >(s,'/', StrUtils::KeepEmptyTokens::False);
    fail_unless(r1==r2,"a/bb/ccc//");
}
END_TEST
START_TEST(split_false5)
{
    vector<string> r1;
    string s="a";
    r1.push_back("a");
    vector<string> r2=split_string<vector<string> >(s,'/', StrUtils::KeepEmptyTokens::False);
    fail_unless(r1==r2,"a");
}
END_TEST
START_TEST(split_false6)
{
    vector<string> r1;
    const string s = "";
    vector<string> r3 = split_string<vector<string> >(s, '/', StrUtils::KeepEmptyTokens::False);
    fail_unless(r3.empty(), "must be empty");
}
END_TEST
START_TEST(split_false7)
{
    vector<string> r1;
    string s="/";
    vector<string> r2=split_string<vector<string> >(s,'/', StrUtils::KeepEmptyTokens::False);
    fail_unless(r1==r2,"must be empty");
}
END_TEST

START_TEST(Check_Placeholders1)
{
    fail_unless(Placeholders()
            .bind(":a", 2)
            .bind(":b", 3)
            .bind(":c", "10")
            .replace(":a * (:a + :b) = :c") == "2 * (2 + 3) = 10");
}
END_TEST;

START_TEST(Check_Placeholders2)
{
    fail_unless(Placeholders()
            .bind("a", "a")
            .bind("b", "")
            .replace("aabc") == "aac");
}
END_TEST;

START_TEST(Check_atoiNVL_Default)
{
    fail_unless(atoiNVL(nullptr, -1) == -1);
    fail_unless(atoiNVL("", -1) == 0);
    fail_unless(atoiNVL("A", -1) == -1);
} END_TEST;

START_TEST(check_textCensor)
{
    using namespace StrUtils;
    auto Any  = [](char c) -> bool { return true; };
    auto IsLetter = [](char c) -> bool { return IsLatChar(c); };

    struct {
        std::string text;
        std::function<bool(char)> isAllowedChar;
        std::vector<ReplacementRule> replacementRules;
        std::string censoredText;
    } checks[] {
        { "111 TEST 100500 string 111 + thrush", IsLatChar, {}, "TEST STRING THRUSH" },
        { "111 TEST 100500 string 111 + thrush", IsLatOrRusChar,
            {{'+', Any, Any, "PLUS", "ПЛЮС"}},
            "TEST STRING PLUS THRUSH" },
        { "111 ТЕСТ 100500 строка 111 + дрозд", IsLatOrRusChar,
            {{'+', Any, Any, "PLUS", "ПЛЮС"}},
            "ТЕСТ СТРОКА ПЛЮС ДРОЗД" },
        { "SPARROW+THRUSH+++WOLF", IsLatChar,
            {{'+', IsLetter, IsLetter, " PLUS ", " PLUS "},
             {'+', Any, Any, " SUPERPLUS ", " SUPERPLUS "}},
            "SPARROW PLUS THRUSH SUPERPLUS SUPERPLUS SUPERPLUS WOLF" }
    };
    for (const auto& c: checks) {
        const std::string actualCensoredText = textCensor(c.text, c.isAllowedChar, c.replacementRules);
        fail_unless(c.censoredText == actualCensoredText,
                "expected censored text: %s, actual censored text: %s",
                c.censoredText.c_str(), actualCensoredText.c_str());
    }
}
END_TEST;

#define SUITENAME "Serverlib"
TCASEREGISTER(0,0)
{
    ADD_TEST(is_upper_lower);
    ADD_TEST(chk_to_upper);
    ADD_TEST(normalize_string);
    ADD_TEST(normalize_UCname);
    ADD_TEST(check_is_rus_lat_char);
    ADD_TEST(check_fromBase10);
    ADD_TEST(check_padding);
    ADD_TEST(test_replaceSubstr);
    ADD_TEST(test_ReplaceInStr);
    ADD_TEST(test_replaceSubstrCopy);
    ADD_TEST(split1);
    ADD_TEST(split2);
    ADD_TEST(split3);
    ADD_TEST(split4);
    ADD_TEST(split5);
    ADD_TEST(split6);
    ADD_TEST(split7);
    ADD_TEST(split_false1);
    ADD_TEST(split_false2);
    ADD_TEST(split_false3);
    ADD_TEST(split_false4);
    ADD_TEST(split_false5);
    ADD_TEST(split_false6);
    ADD_TEST(split_false7);
    ADD_TEST(Check_Placeholders1);
    ADD_TEST(Check_Placeholders2);
    ADD_TEST(Check_atoiNVL_Default);
    ADD_TEST(check_textCensor);
}
TCASEFINISH;
} /* namespace */
#endif /*XP_TESTING*/

