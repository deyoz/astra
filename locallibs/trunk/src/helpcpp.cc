#if HAVE_CONFIG_H
#endif

#include <glob.h>
#include <errno.h>
#include <cstdio>
#include <locale>
#include <sstream>
#include <typeinfo>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>
#include <string>
#include <vector>
#include <stdarg.h>

#include "helpcpp.h"
#include "bool_operator.h"
#include "text_codec.h"
#include "dates_io.h"
#include "string_cast.h"
#include "lngv.h"
#include "lngv_user.h"
#include "str_utils.h"
#include "oci_seq.h"
#include "va_holder.h"

#define NICKNAME "SYSTEM"
#include "slogger.h"

using namespace std;
using namespace HelpCpp;

namespace boost
{
namespace date_time
{
template <class date_type>
date_type from_normal_string(const std::string& s)
{
    //typename date_type::ymd_type ymd; // default now
    unsigned short y=0, m=0, d=0;
    int offsets[] = {2, 2, 4};
    int pos = 0;
    boost::offset_separator osf(offsets, offsets+3);
    boost::tokenizer<boost::offset_separator> tok(s, osf);
    for(boost::tokenizer<boost::offset_separator>::iterator ti=tok.begin(); ti!=tok.end(); ++ti)
    {
        unsigned short i = boost::lexical_cast<unsigned short>(*ti);
        switch(pos)
        {
        case 0: d = i; break;
        case 1: m = i; break;
        case 2: y = Dates::NormalizeYear(i); break;
        }
        ++pos;
    }
    return date_type(y, m, d);
}
} // date_time

namespace gregorian
{
void fillDateTimeFacet(boost::gregorian::date_facet *fac, Language lang);

date from_normal_string(const std::string& s) 
{
    return date_time::from_normal_string<date>(s);
}
} // gregorian
} // boost

namespace{
const char * short_eng_month[12]= {"JAN","FEB","MAR","APR",
             "MAY","JUN", "JUL", "AUG",
             "SEP", "OCT","NOV","DEC"};

const char * short_rus_month[12]= {"ЯНВ","ФЕВ","МАР", "АПР",
             "МАЙ","ИЮН","ИЮЛ","АВГ","СЕН","ОКТ","НОЯ","ДЕК"};

const char *long_wd_e[7]={"MON","TUE","WED","THU","FRI","SAT","SUN"};
const char *long_wd_r[7]= {"ПОН","ВТР","СРД","ЧЕТ","ПТН","СУБ","ВСК"};
const char *short_wd_e[7]={"MO","TU","WE","TH","FR","SA","SU"};
const char *short_wd_r[7]={"ПН","ВТ","СР","ЧТ","ПТ","СБ","ВС"};


const char *long_rus_months[12] = {"ЯНВАРЬ","ФЕВРАЛЬ","МАРТ","АПРЕЛЬ","МАЙ",
                             "ИЮНЬ","ИЮЛЬ","АВГУСТ","СЕНТЯБРЬ","ОКТЯБРЬ",
                             "НОЯБРЬ","ДЕКАБРЬ"};
const char *long_eng_months[12] = {"JANUARY","FEBRUARY","MARCH","APRIL","MAY",
                             "JUNE","JULY","AUGUST","SEPTEMBER",
                             "OCTOBER",
                             "NOVEMBER","DECEMBER"};

struct Lang {
const char  *(*long_wd)[7];
const char  *(*short_wd)[7];
const char  *(*long_mon)[12];
const char  *(*short_mon)[12];
};

Lang nullLang()
{
    Lang l;
    Zero(l);
    return l;
}
Lang rus()
{
    Lang l;
    l.long_wd=&long_wd_r;
    l.short_wd=&short_wd_r;
    l.long_mon=&long_rus_months;
    l.short_mon=&short_rus_month;
    return l;
}
Lang eng()
{
    Lang l;
    l.long_wd=&long_wd_e;
    l.short_wd=&short_wd_e;
    l.long_mon=&long_eng_months;
    l.short_mon=&short_eng_month;
    return l;
}



using boost::posix_time::time_facet;
using boost::gregorian::date_facet;
using boost::posix_time::time_input_facet;
using boost::gregorian::date_input_facet;
using boost::posix_time::ptime;
using boost::gregorian::date;


template <typename FACET>
FACET  *dateTimeFacet(const char * format,Lang l)
{
    FACET *fac=new FACET();
    if(format && *format){
        fac->format(format);
    }

    if(l.short_mon){
        typename FACET::input_collection_type short_months, long_months,
                                      short_weekdays, long_weekdays;


        short_months.assign(l.short_mon[0],l.short_mon[0]+12);

        long_months.assign(l.long_mon[0],l.long_mon[0]+12);

        short_weekdays.push_back(l.short_wd[0][6]);
        short_weekdays.insert(short_weekdays.end(),
                l.short_wd[0],l.short_wd[0]+6);
        long_weekdays.push_back(l.long_wd[0][6]);
        long_weekdays.insert(long_weekdays.end(),l.long_wd[0],l.long_wd[0]+6);
        fac->short_month_names(short_months);
        fac->long_month_names(long_months);
        fac->short_weekday_names(short_weekdays);
        fac->long_weekday_names(long_weekdays);
    }
    return fac ;
}

Lang int2Lang(int l)
{
    switch (l) {
        case -1:
            return nullLang();
        case 0:
            return eng();
        default:
            return rus();
    }
}

}
/*
ENUM_NAMES_BEGIN(Language)
    (ENGLISH, "en")
    (RUSSIAN, "ru")
ENUM_NAMES_END(Language)
ENUM_NAMES_END_AUX(Language)
*/
Language languageFromStr(const std::string& l)
{
    return l == "ru" ? RUSSIAN : ENGLISH;
}

std::ostream & operator << (std::ostream& os, const Language &x)
{
    os<<"'"<<(x==RUSSIAN?"RUSSIAN":x==ENGLISH?"ENGLISH":"???")
        <<"'("<<(int)x<<")";
    return os;
}

static bool validate(const std::string& language)
{
    static const std::set<std::string> languages = {"en_US", "ru_RU", "es_ES"};
    if (languages.find(language) == languages.end()) {
        return false;
    }
    return true;
}

UserLanguage UserLanguage::en_US()
{
    return UserLanguage("en_US");
}

UserLanguage UserLanguage::ru_RU()
{
    return UserLanguage("ru_RU");
}

UserLanguage UserLanguage::es_ES()
{
    return UserLanguage("es_ES");
}

UserLanguage::UserLanguage(const std::string& lang)
{
    if (!validate(lang)) {
        LogError(STDLOG) << "Unknown language: " + lang << " - ENGLISH language applied";
        langCode_ = "en_US";
    } else {
        langCode_ = lang;
    }
}

std::string UserLanguage::translate(const std::string& msg, const UserLanguage& l, const Dictionary& d)
{
    auto it = d.find(msg);
    if (it == d.end()) {
        return msg;
    }

    auto translationIt = it->second.find(l);
    if (translationIt == it->second.end() || translationIt->second.empty()) {
        auto engTranslationIt = it->second.find(UserLanguage::en_US());
        ASSERT(engTranslationIt != it->second.end());
        return engTranslationIt->second;
    }
    return translationIt->second;
}

bool UserLanguage::operator<(const UserLanguage& that) const
{
    return this->langCode_ < that.langCode_;
}

bool UserLanguage::operator == (const UserLanguage &that) const
{
    return this->langCode_ == that.langCode_;
}

std::ostream& operator<<(std::ostream& s, const UserLanguage& userLanguage)
{
    s << "Language: " << userLanguage.langCode_;
    return s;
}

std::string UserLanguagePackerHelper::pack(const UserLanguage& l)
{
    return l.langCode_;
}

UserLanguage UserLanguagePackerHelper::unpack(const std::string& s)
{
    return UserLanguage(s);
}

UserLanguage UserLanguagePackerHelper::unpack(Language l)
{
    return l == RUSSIAN ? UserLanguage::ru_RU() : UserLanguage::en_US();
}

namespace HelpCpp {

CallStackRegisterManager & CallStackRegisterManager::getInstance()
{
    static CallStackRegisterManager *p=0;
    if(!p){
        p=new CallStackRegisterManager;
    }
    return *p;
}
std::string CallStackRegisterManager::dump()
{
    string res;
    for (list<CallStackRegisterManager::point>::iterator i=l.begin();
            i!=l.end();++i)
    {
       if(i!=l.begin())
           res.append("\n\t\t");
       res.append(i->nick).append(":").append(i->file).append(":").append(string_cast(i->line)).append(":")
          .append(i->func);

       for (list<string>::iterator j=i->messages.begin();j!=i->messages.end();++j){
            res.append("\n").append(*j);
       }
    }
    res.append("\nEND OF STACKTRACE");
    return res;
}


    

date_facet  *dateFacet(const char * format,int l)
{
   return dateTimeFacet<date_facet>(format,int2Lang(l));
}
time_facet  *timeFacet(const char * format,int l)
{
   return dateTimeFacet<time_facet>(format,int2Lang(l));
}

date_input_facet *dateInputFacet(const char * format,int l)
{
    return dateTimeFacet<date_input_facet>(format,int2Lang(l));
}
time_input_facet *timeInputFacet(const char * format,int l)
{
    return dateTimeFacet<time_input_facet>(format,int2Lang(l));
}

std::string date6to8(std::string const &six)
{
      if(six.length()!=6)
          throw comtech::Exception(std::string("bad date length:")+six);
      int year=boost::lexical_cast<int>(six.substr(0,2));
      if(year>91){
         year+=1900;
      }else{
          year+=2000;
      }
      return string_cast(year)+six.substr(2);
}


const char *checkLen(const char *s, size_t len)
{
    if (strlen(checkNull(s)) > len) {
        abort();
    }
    return s;
}

void abortError()
{
    ProgError(STDLOG, "modification of constant object ");
    abort();
}

std::string vsprintf_string_ap(char const* format, va_list ap)
{
    va_list aq;
    __va_copy(aq, ap);
    auto size = vsnprintf(nullptr, 0, format, aq);
    va_end(aq);

    if(size < 0)
        throw comtech::Exception("Error while vsnprintf");

    std::string msg(++size, '\0');
    vsnprintf(const_cast<char*>(msg.data()), size, format, ap);
    msg.resize(size-1);
    return msg;
}

std::string vsprintf_string(char const *str , ...)
{
    VA_HOLDER(ap, str);
    return vsprintf_string_ap(str, ap);
}

bool cstring_eq::operator()(const char *a, const char *b) const
{
    return strcmp(a, b)==0;
}
bool cstring_not_eq::operator()(const char *a, const char *b) const
{
    return strcmp(a, b)!=0;
}
bool cstring_less::operator()(const char *a, const char *b) const
{
    return strcmp(a, b)<0;
}

boost::gregorian::date_facet  *dateFacet(const char * format,int l);
boost::posix_time::time_facet  *timeFacet(const char * format,int l);
inline boost::posix_time::time_facet  *timeDurationFacet(const char * format)
{
    boost::posix_time::time_facet *fac=new boost::posix_time::time_facet();
    if(format && *format){
        fac->time_duration_format(format);
    }
    return fac;
}
boost::gregorian::date_input_facet *dateInputFacet(const char * format,int l);
boost::posix_time::time_input_facet  *timeInputFacet(const char * format,int l);
inline boost::posix_time::time_input_facet  *timeDurationInputFacet(const char * format)
{
    boost::posix_time::time_input_facet *fac=new boost::posix_time::time_input_facet();
    if(format && *format){
        fac->time_duration_format(format);
    }
    return fac;
}


template <typename T> struct DateTime {};

template <>
struct DateTime<boost::gregorian::date>{
    typedef boost::gregorian::date_facet Facet;
    static Facet *getFacet(const char * format,int l)
    {
        return dateFacet(format,l);
    }
};

template <>
struct DateTime<boost::posix_time::ptime>{
    typedef boost::posix_time::time_facet Facet;
    static Facet *getFacet(const char * format,int l)
    {
        return timeFacet(format,l);
    }
};

template <>
struct DateTime<boost::posix_time::time_duration>{
    typedef boost::posix_time::time_facet Facet;
    static Facet *getFacet(const char * format,int l)
    {
        return timeDurationFacet(format);
    }
    static Facet *getFacet(const char * format)
    {
        return timeDurationFacet(format);
    }
};


template <typename T> struct InputDateTime {};

template <>
struct InputDateTime<boost::gregorian::date>{
typedef boost::gregorian::date_input_facet Facet;
    static Facet *getFacet(const char * format,int l)
    {
        return dateInputFacet(format,l);
    }
};

template <>
struct InputDateTime<boost::posix_time::ptime>{
    typedef boost::posix_time::time_input_facet Facet;
    static Facet *getFacet(const char * format,int l)
    {
        return timeInputFacet(format,l);
    }
};

template <>
struct InputDateTime<boost::posix_time::time_duration>{
    typedef boost::posix_time::time_input_facet Facet;
    static Facet *getFacet(const char * format,int l)
    {
        return timeDurationInputFacet(format);
    }
    static Facet *getFacet(const char * format)
    {
        return timeDurationInputFacet(format);
    }
};

template <typename DateType>
static const std::locale& getCachedLocale(const std::string& format, int lang)
{
    typedef boost::shared_ptr<std::locale> LocalePtr;
    typedef std::map<std::pair<std::string,int>, LocalePtr> Cache;

    static Cache cache;
    const Cache::key_type key(format, lang);
    Cache::iterator it = cache.find(key);
    if (cache.end() == it) {
        LocalePtr loc(new std::locale(std::locale(), DateType::getFacet(format.c_str(), lang)));
        it = cache.insert(std::make_pair(key, loc)).first;
    }
    return *(it->second);
}


template <typename T>
static std::string string_cast__(T const &d,char const *format, int l=-1)
{
    std::ostringstream s;
    s.imbue(getCachedLocale<DateTime<T> >(std::string(format), l));
    s << d;
    return s.str();
}

std::string string_cast(const boost::gregorian::date& d, char const *format, int l)
{
    return string_cast__(d, format, l);
}

std::string string_cast(const boost::posix_time::ptime& t, char const *format, int l)
{
    return string_cast__(t, format, l);
}

std::string string_cast(const boost::posix_time::time_duration& td, char const *format, int l)
{
    return string_cast__(td, format, l);
}

template <typename T>
static T date_time_cast(const char *data, const char *format="", int l=-1, bool strict=true)
{
    std::stringstream s(data);
    s.imbue(getCachedLocale<InputDateTime<T> >(std::string(format), l));
    s.exceptions(std::ios_base::failbit);
    T D(boost::gregorian::not_a_date_time);
    s >> D;
    if(strict && string_cast(D,format,l)!=data) {
        std::stringstream msg;
        msg  <<("Parse failed. Invalid Date/Time data=")
            << " data=" << data << " format=" << format 
            << " l=" << l << " strict=" << strict;
        throw comtech::Exception(msg.str());
    }
    return D;
}

boost::gregorian::date date_cast(const char *data, const char *format, int l, bool strict)
{
    return date_time_cast<boost::gregorian::date>(data, format, l, strict);
}
boost::posix_time::ptime time_cast(const char *data, const char *format, int l, bool strict)
{
    return date_time_cast<boost::posix_time::ptime>(data, format, l, strict);
}
boost::posix_time::time_duration timed_cast(const char *data, const char *format, int l, bool strict)
{
    return date_time_cast<boost::posix_time::time_duration>(data, format, l, strict);
}

// do not throw 
template <typename T>
static T simple_date_time_cast__(const char *data, const char *format="", int l=-1)
{
    std::stringstream s(data);
    s.imbue(getCachedLocale<InputDateTime<T> >(std::string(format), l));
    T D(boost::gregorian::not_a_date_time);
    s >> D;
    return D;
}

boost::gregorian::date simple_date_cast(const char *data, const char *format, int l)
{
    return simple_date_time_cast__<boost::gregorian::date>(data, format, l);
}

boost::posix_time::ptime simple_time_cast(const char *data, const char *format, int l)
{
    return simple_date_time_cast__<boost::posix_time::ptime>(data, format, l);
}

boost::posix_time::time_duration simple_timed_cast(const char *data, const char *format, int l)
{
    return simple_date_time_cast__<boost::posix_time::time_duration>(data, format, l);
}

std::string memdump(const void *aBuf_void, size_t aBufLen, int markIndex)
{
    if(not aBuf_void)
        return "NULL * " + std::to_string(aBufLen) + " bytes\n";
    auto aBuf = static_cast<const uint8_t*>(aBuf_void);
    constexpr uint8_t  BytesPerLine=8;
    std::string res;
    for(unsigned i=0;i<aBufLen/BytesPerLine+1;i++)
    {
      char temp2[3*16+1+2];
      char temp3[16+1];
      int  len3=0;

      temp2[0] = markIndex < 0 or size_t(markIndex) != i * BytesPerLine ? ' ' : '[';
      size_t len2 = 1;
      for(size_t Offs=i*BytesPerLine; Offs<(i+1)*BytesPerLine && Offs<aBufLen; Offs++)
      {
          len2 += snprintf(temp2+len2, sizeof(temp2)-len2, "%02X", aBuf[Offs]);
          temp2[len2++] = markIndex>0 and size_t(markIndex) - 1 == Offs ? '['
                        : markIndex >= 0 and size_t(markIndex) == Offs  ? ']'
                        : ' ';
          len3 += snprintf(temp3+len3, sizeof(temp3)-len3, "%c", aBuf[Offs]<' '?'.':aBuf[Offs]);
      }
      res.append(temp2,len2);
      if(BytesPerLine*3 >= len2)
          res.append(BytesPerLine*3+1-len2, ' ');
      res.append(temp3,len3);
      res+='\n';
    }
    return res;
}

ObjIdType objId()
{
    return boost::lexical_cast<ObjIdType>(
        OciCpp::Sequence(OciCpp::mainSession(), "SEQ_OBJECT").nextval<std::string>(STDLOG));
}

static void bigdiv(const std::string& s, unsigned divider, std::string& res, unsigned& reminder)
{
    std::string src = s;
    res.clear();
    reminder = 0;
    unsigned temp = 0;

    size_t i = 0;
    for (;; ++i) {
        if (i == src.size()) {
            reminder = temp;
            break;
        }
        temp *= 10;
        temp += (src.at(i) - '0');
        if (divider > temp) {
            if (!res.empty()) {
                res.push_back('0');
            }
        } else {
            res.push_back(temp / divider + '0');
            unsigned rem = temp % divider;
            temp = rem;
        }
    }
}

std::string convertToId(const std::string& src, unsigned len, const std::string& prefix, const std::string& dict)
{
    std::string current(src);
    std::string res;
    unsigned const dsz = dict.size();
    while (!current.empty()) {
        unsigned rem;
        bigdiv(current, dsz, current, rem);
        res.push_back(dict[rem]);
    }
    std::reverse(res.begin(), res.end());
    return prefix + std::string(len - res.length(), '0') + res;
}

std::string convertToId(const std::string& src, unsigned len, const std::string& prefix)
{
    return convertToId(boost::lexical_cast<uint64_t>(src), len, prefix);
}

std::string convertToId(const uint64_t& src, unsigned len, const std::string& prefix)
{
    static const uint64_t mask = 0xFCLL << 0x34; // 0x0FC0000000000000
    // Алфавит упорядочен в соответствии с ASCII и един для всех функций.
    // Изменение алфавита (упорядоченности или содержимого) требует изменения функций convertToId/convertFromId
    static const char mappingTable[] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
        'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '_',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
        't', 'u', 'v', 'w', 'x', 'y', 'z'};

    std::string res(prefix);
    res.reserve(len + prefix.size());
    res += mappingTable[(src & (mask << 0x06)) >> 0x3C]; //Крайний левый разряд. 60 - 63 биты
    res += mappingTable[(src & mask) >> 0x36];           //9 разряд результата. 54 - 59 биты
    res += mappingTable[(src & (mask >> 0x06)) >> 0x30]; //8 разряд результата. 48 - 53 биты
    res += mappingTable[(src & (mask >> 0x0C)) >> 0x2A]; //7 разряд результата. 42 - 47 биты
    res += mappingTable[(src & (mask >> 0x12)) >> 0x24]; //6 разряд результата. 36 - 41 биты
    res += mappingTable[(src & (mask >> 0x18)) >> 0x1E]; //5 разряд результата. 30 - 35 биты
    res += mappingTable[(src & (mask >> 0x1E)) >> 0x18]; //4 разряд результата. 24 - 29 биты
    res += mappingTable[(src & (mask >> 0x24)) >> 0x12]; //3 разряд результата. 18 - 23 биты
    res += mappingTable[(src & (mask >> 0x2A)) >> 0x0C]; //2 разряд результата. 12 - 17 биты
    res += mappingTable[(src & (mask >> 0x30)) >> 0x06]; //1 разряд результата. 6 - 11 биты
    res += mappingTable[src & (mask >> 0x36)];           //0 разряд результата. 0 - 5 биты
    if ((len + prefix.size()) < res.size()) { // Если нужно обрезаем строку
        res.erase(res.begin() + prefix.size(), res.end() - len);
    } else {                                  // Или наоборот, расширяем
        res.insert(res.begin() + prefix.size(), len + prefix.size() - res.size(), '0');
    }

    return res;
}

uint64_t convertFromId(const std::string& src, unsigned prefixLength)
{
    static const unsigned BLOCK_SIZE = 6; // Количество бит, соответствующих одному символу алфавита
    //Количество символов алфавита, которые нужно разобрать, чтобы заполнить res
    static const unsigned BLOCK_COUNT = ((CHAR_BIT * sizeof(uint64_t)) / BLOCK_SIZE) + 1;
    // Зеркало алфавита из предыдущей функции.
    // Индексы в массиве соответствуют выражению: (ASCII код символа из алфавита - ASCII код символа '0')
    // Значения по этим идексам соответствуют индексу данного символа в исходном алфавите, т.е. числу
    // в 6 битах исходного числа
    static const uint64_t table[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
        34, 35, 36, 0, 0, 0, 0, 37, 0, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
        49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63};
    static const uint64_t* mappingTable = table - static_cast<ptrdiff_t>('0');

    uint64_t res = 0; //Результат преобразования
    unsigned j = 0;   //Номер блока из 6 бит. Нумерация возрастает от младших битов к старшим
    std::string::const_reverse_iterator i = src.rbegin(); // Начиная с младшего символа исходной последовательности
    std::string::const_reverse_iterator end = src.rend() - prefixLength;

    ASSERT(static_cast<unsigned>(*i) < ('0' + (sizeof(table) / sizeof(uint64_t))));
    ASSERT('0' <= *i);

    while ((i != end) && (j < BLOCK_COUNT)) { // Пока не закончились символы и не заполнены все разряды результата
        // Получаем исходные 6 бит, в которые сконвертировался текущий символ *i
        // и помещаем их в соответствующие позиции
        res |= mappingTable[static_cast<unsigned char>(*i)] << (j * BLOCK_SIZE);
        ++i; // К следующему символу
        ++j; // К следующему блоку битов
    }

    return res;
}

} //namespace HelpCpp
vector<string> getFilenamesByMask(string const &mask)
{
    vector<string> res;
    glob_t namelist;
    int ret=glob(mask.c_str(),GLOB_ERR|GLOB_MARK,0,&namelist);
    if(ret==GLOB_NOMATCH){
        return res;
    }
    if(ret!=0){
        throw runtime_error("glob failed ["+mask +"] :"+ strerror(errno));
    }
    
    res.assign(&namelist.gl_pathv[0],&namelist.gl_pathv[namelist.gl_pathc]);
    globfree(&namelist);
    return res;
}

#ifdef XP_TESTING
#include <boost/bind.hpp>
#include "tcl_utils.h"
#include "enum.h"
#include "time_caching.h"
#include "call_with_fallback.h"
#include "expected.h"
#include "either.h"
#include "checkunit.h"

namespace {
using namespace std;
using namespace HelpCpp;
using namespace boost::gregorian;
using namespace boost::posix_time;
START_TEST(to_date)
{
    try
    {
        date d= date_time_cast<date>("2007-Dec-21");
        ProgTrace(TRACE3,"Date: %s", string_cast(d,"%Y%m%d",0).c_str());
        fail_unless("071221"==string_cast(d,"%y%m%d",0),"fail formatting");
        fail_unless("211207"==string_cast(d,"%d%m%y",0),"fail formatting");
        fail_unless(d.year() == 2007, "bad year");
        fail_unless(d.month() == 12, "bad month");
        fail_unless(d.day() == 21, "bad day to die");

        d = date_cast("020309", "%d%m%y");
        fail_unless("090302"==string_cast(d,"%y%m%d",0),"fail formatting");
        fail_unless("020309"==string_cast(d,"%d%m%y",0),"fail formatting");
        fail_unless(d.year() == 2009, "bad year");
        fail_unless(d.month() == 3, "bad month");
        fail_unless(d.day() == 2, "bad day to die");
    }
    catch (const comtech::Exception &e)
    {
        fail_unless(0,"%s",e.what());
    }

}END_TEST

START_TEST(simple_date_time)
{
    try
    {
        char date_str[7]={" "};
        date d= simple_date_cast(date_str);        
        fail_unless(d.is_not_a_date(),"fail not-a-date-time");

        ptime t= simple_time_cast(date_str);        
        fail_unless(t.is_not_a_date_time(),"fail not-a-date-time");
 
        ptime t1 = simple_time_cast("20091211T172550", "%Y%m%dT%H%M%S");
        fail_unless("172550"==string_cast(t1,"%H%M%S",0),"fail formatting");
        fail_unless("20091211"==string_cast(t1,"%Y%m%d",0),"fail formatting");
  
        ptime t2 = simple_time_cast("20091210", "%Y%m%d");
        fail_unless("20091210"==string_cast(t2,"%Y%m%d",0),"fail formatting");
        
        date d3 = simple_date_cast("20071101", "%Y%m%d");
        fail_unless("20071101"==string_cast(d3,"%Y%m%d",0),"fail formatting");
  
    }
    catch (const comtech::Exception &e)
    {
        fail_unless(0,"%s",e.what());
    }
}END_TEST


START_TEST(to_date_no_day) //Expair date for Credit card
{
    try
    {
        date d = date_cast("0309", "%m%y");

        ProgTrace(TRACE3,"NoDate: %s", string_cast(d,"%Y%m%d",0).c_str());
        fail_unless("090301"==string_cast(d,"%y%m%d",0),"fail formatting");
        fail_unless("010309"==string_cast(d,"%d%m%y",0),"fail formatting");
        fail_unless(d.year() == 2009, "bad year");
        fail_unless(d.month() == 3, "bad month");
        fail_unless(d.day() == 1, "bad day to die");
    }
    catch (const comtech::Exception &e)
    {
        fail_unless(0,"%s",e.what());
    }

}END_TEST

START_TEST(to_date_no_year) // date of flight
{
    try
    {
        date d = date_cast("2212", "%d%m");

        d = date( greg_year(2006), d.month(), d.day() );
        ProgTrace(TRACE3,"NoYear: %s", string_cast(d,"%Y%m%d",0).c_str());
        fail_unless("061222"==string_cast(d,"%y%m%d",0),"fail formatting");
        fail_unless("221206"==string_cast(d,"%d%m%y",0),"fail formatting");
        fail_unless(d.year() == greg_year(2006), "bad year");
        fail_unless(d.month() == 12, "bad month");
        fail_unless(d.day() == 22, "bad day to die");
    }
    catch(const comtech::Exception &e)
    {
        fail_unless(0,"%s",e.what());
    }

}END_TEST


START_TEST(to_time)
{
    try
    {
        ptime t = time_cast("235859", "%H%M%S");

        ProgTrace(TRACE3,"Time: %s", string_cast(t,"%H%M%S",0).c_str());
        fail_unless("235859"==string_cast(t,"%H%M%S",0),"fail formatting");
        fail_unless("595823"==string_cast(t,"%S%M%H",0),"fail formatting");
        fail_unless("592358"==string_cast(t,"%S%H%M",0),"fail formatting");
        fail_unless(t.time_of_day().hours() == 23, "bad hours");
        fail_unless(t.time_of_day().minutes() == 58, "bad minutes");
        fail_unless(t.time_of_day().seconds() == 59, "seconds");
    }
    catch(const comtech::Exception &e)
    {
        fail_unless(0,"%s",e.what());
    }

}END_TEST

START_TEST(to_time_dura)
{
    try
    {
        time_duration td = date_time_cast<time_duration>
                ("235859", "%H%M%S");

        ProgTrace(TRACE3,"TimeDuration: %s", string_cast(td,"%H%M%S",0).c_str());
        fail_unless("235859"==string_cast(td,"%H%M%S",0),"fail formatting");
        fail_unless("595823"==string_cast(td,"%S%M%H",0),"fail formatting");
        fail_unless("592358"==string_cast(td,"%S%H%M",0),"fail formatting");
        fail_unless(td.hours() == 23, "bad hours");
        fail_unless(td.minutes() == 58, "bad minutes");
        fail_unless(td.seconds() == 59, "seconds");
    }
    catch(const comtech::Exception &e)
    {
        fail_unless(0,"%s",e.what());
    }

}END_TEST


START_TEST(to_time_no_seconds)
{
    try
    {
        ptime t = time_cast("2252", "%H%M");

        ProgTrace(TRACE3,"Time: %s", string_cast(t,"%H%M%S",0).c_str());
        fail_unless("225200"==string_cast(t,"%H%M%S",0),"fail formatting");
        fail_unless("005222"==string_cast(t,"%S%M%H",0),"fail formatting");
        fail_unless("002252"==string_cast(t,"%S%H%M",0),"fail formatting");
        fail_unless(t.time_of_day().hours() == 22, "bad hours");
        fail_unless(t.time_of_day().minutes() == 52, "bad minutes");
        fail_unless(t.time_of_day().seconds() == 0, "seconds");    }
        catch(const comtech::Exception &e)
        {
            fail_unless(0,"%s",e.what());
        }

}END_TEST


START_TEST(date_string)
{
    try {
        date d(greg_year(2006),
                greg_month(4),greg_day(15));

        ProgTrace(TRACE3,"Date: %s", string_cast(d,"%Y%m%d",0).c_str());
        fail_unless("20060415"==string_cast(d,"%Y%m%d",0),"fail formatting");

        ProgTrace(TRACE3,"Date: %s", string_cast(d,"%Y-%b-%d",1).c_str());
        fail_unless(string("2006-АПР-15")
                ==string_cast(d,"%Y-%b-%d",1),"fail formatting");

        ProgTrace(TRACE3,"Date: %s", string_cast(d,"%m%y",1).c_str());
        fail_unless(string("0406")
                ==string_cast(d,"%m%y",1),"fail formatting");

    }
    catch (const comtech::Exception &e)
    {
        fail_unless(0,"%s",e.what());
    }
}
END_TEST

START_TEST(time_string)
{
    try {
        ptime t(date(greg_year(2006),greg_month(4),greg_day(15)),
                time_duration(23,11,15));
        ProgTrace(TRACE3,"Date: %s", string_cast(t,"%Y%m%d%H%M%S",0).c_str());
        fail_unless("20060415231115"==string_cast(t,"%Y%m%d%H%M%S",0),
                "fail formatting");
        ProgTrace(TRACE3,"Date: %s", string_cast(t,"%Y-%b-%d 23:11:15",1).c_str());
        fail_unless(string("2006-АПР-15 23:11:15")
                ==string_cast(t,"%Y-%b-%d %H:%M:%S",1),
        "fail formatting");
    }
    catch(const comtech::Exception &e)
    {
        fail_unless(0,"%s",e.what());
    }
}
END_TEST

START_TEST(to_time_mbf) //must be fail
{
    try
    {
        const char *date = "170606";
        const char *time = "296088";
        time_duration td = timed_cast(time, "%H%M%S");
        ptime pt(date_cast(date,"%d%m%y"), td);
        ProgError(STDLOG,"DATE?!:%s", string_cast(pt, "%d%m%Y %H%M%S",-1).c_str());
        fail_unless(0,string(string("Construction date/time duration from ") + date + "/"
                + time + " must be fail").c_str());
    }
    catch (const comtech::Exception& e)
    {
        ProgTrace(TRACE3, "%s", e.what());
    }

}END_TEST

START_TEST(to_time_mbf3) //must be fail
{
    try
    {
        ptime pt = time_cast("28022006 255961","%d%m%Y %H%M%S");
        ProgError(STDLOG,"DATE?!:%s", string_cast(pt, "%d%m%Y %H%M%S",-1).c_str());
        fail_unless(0,"Must be fail!");
    }
    catch (const comtech::Exception& e)
    {
        ProgTrace(TRACE3, "%s", e.what());
    }

}END_TEST

std::string vsprintf_string_with_check(char const *str , ...)
{
    VA_HOLDER(ap, str);
    return vsprintf_string_ap(str, ap);
}

START_TEST(vsprintf_string_c)
{
    std::string res=vsprintf_string_with_check("%s %s %s", "10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000", "1000", "1000");
    fail_unless(res==                                      "10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 1000 1000",
            "vsprintf_string_check failed:\n<%s> of size %zu", res.c_str(), res.size());
}END_TEST
START_TEST(vsprintf_string_w)
{
    std::string res=vsprintf_string("%s %s", "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000", "1000");
    fail_unless(res==                        "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 1000",
            "vsprintf_string_work failed:\n<%s>", res.c_str());
}END_TEST


class A{
int p;
    public:
A(int pp):p(pp){}
BOOL_OPERATOR(p!=15)

};

class B{
int p;
    public:
B(int pp):p(pp){}
BOOL_OPERATOR_DECL;
};
BOOL_OPERATOR_DEF(B,p!=0);

START_TEST(test_bool_opr)
{
    A a(15);
    if(a){
        fail_unless(0,"bool operator returned true");
    }
    A a1(10);
    if(!a1){
        fail_unless(0,"bool operator returned false");
    }
    B b(0);
    if(b){
        fail_unless(0,"bool operator returned true");
    }
    B b1(10);
    if(!b1){
        fail_unless(0,"bool operator returned true");
    }

    
}
END_TEST

START_TEST(boolRead)
{
    struct {
        const char* cval;
        int bval;
    } Tst[] = {
        { "0", 0 }, { "1", 1 }, {"Yes", 1 }, { "On", 1 }, { "Off", 0 },
        { "no", 0 }, { "on", 1 }, { "YES", 1 }, {"NO", 0}, { "OFF", 0 },
        { "True", 1 }, { "False", 0 }, { "true", 1 }, { "FALSE", 0 },
        { "N", 0 }, { "Y", 1 }
    };

    const unsigned int count = sizeof(Tst) / sizeof(Tst[0]);

    for (unsigned int i = 2; i < count; ++i) {
        std::string varName("FAKE_VAR_");
        varName += HelpCpp::string_cast(i);

        setTclVar(varName, Tst[i].cval);

        fail_unless( getVariableStaticBool(varName.c_str(), 0, -1) == Tst[i].bval, "invalid bool value");
    }
    //-------------------------------------------------------------------------
    const char* missVar = "MISSING_FAKE_VAR";
    static int flag = -1;

    fail_unless( getVariableStaticBool(missVar, &flag, 1) == 1, "invalid missing variable handling 1" );
    fail_unless( flag == 1, "invalid static value" );

    flag = 0;
    fail_unless( getVariableStaticBool(missVar, &flag, 1) == 0, "invalid missing variable handling 2");
}
END_TEST
/*
struct Foo
{
    int fld1, fld2;
    Foo()
        : fld1(0), fld2(0)
    {}
};

static void fooLoader(int i, Foo& foo)
{
    static bool alreadyEntered = false;
    foo.fld1 = i;
    foo.fld2 = -i;
    if (alreadyEntered) {
        throw "we must be here only once";
    } else {
        alreadyEntered = true;
    }
}
*/
START_TEST(getFilenamesByMask_test)
{
    system("rm -f /tmp/getFilenamesByMask_test*.*");
    system("touch /tmp/getFilenamesByMask_test01.TXT");
    system("touch /tmp/getFilenamesByMask_test.TRG");
    vector<string> files=getFilenamesByMask("/t?p/getFilenamesByMask_test*.*");
    LogTrace(TRACE1) << files.at(0);
    LogTrace(TRACE1) << files.at(1);
    fail_unless(files.at(0)=="/tmp/getFilenamesByMask_test.TRG");
    fail_unless(files.at(1)=="/tmp/getFilenamesByMask_test01.TXT");

} END_TEST

START_TEST(check_bitdump)
{
#define CHECK_BITDUMP(val, res) { \
    const std::string s(bitdump(val)); \
    fail_unless(s == res, "bitdump failed: [%s] != [%s]", s.c_str(), res); \
}
    CHECK_BITDUMP((short)(0x1), "0000000000000001");
    CHECK_BITDUMP((int)(0x100), "00000000000000000000000100000000");
    CHECK_BITDUMP((char)(0xA), "00001010");
    CHECK_BITDUMP((unsigned int)(-1), "11111111111111111111111111111111");
    CHECK_BITDUMP((unsigned long long int)(-1), "1111111111111111111111111111111111111111111111111111111111111111");
#undef CHECK_BITDUMP
} END_TEST

START_TEST(check_slice)
{
    std::vector<int> v1;
    for (int i = 0; i < 10; ++i) {
        v1.push_back(i);
    }
#define CHECK_SLICE(res, start, stop) { \
    std::vector<int> tmpRes(HelpCpp::slice(v1, start, stop)); \
    const std::string tmpStr(StrUtils::join(" ", tmpRes)); \
    fail_unless(res == tmpStr, "[%s] != [%s]", res, tmpStr.c_str()); \
}
    ProgTrace(TRACE5, "v1.size=%zd", v1.size());
    CHECK_SLICE("5 6 7 8 9", 5, 0);
    CHECK_SLICE("5 6 7 8 9", 5, 300);
    CHECK_SLICE("0 1 2 3 4 5 6 7", 0, 8);
    CHECK_SLICE("0 1 2 3 4 5 6 7", 0, -2);
    CHECK_SLICE("2 3 4 5 6", 2, 7);
    CHECK_SLICE("", 5, -8);
    CHECK_SLICE("6", 6, 6);
    CHECK_SLICE("6", 6, -4);
    CHECK_SLICE("", 10, 20);
    CHECK_SLICE("9", 9, 10);
    CHECK_SLICE("9", 9, 9);
} END_TEST

START_TEST(check_arch_seq)
{
    std::string res;
    unsigned rem;
    bigdiv("122", 11, res, rem);
    fail_unless(res == "11" && rem == 1);
    bigdiv("10101", 100, res, rem);
    fail_unless(res == "101" && rem == 1);
    bigdiv("534229", 782, res, rem);
    fail_unless(res == "683" && rem == 123);
    bigdiv("3063685872235597", 627, res, rem);
    fail_unless(res == "4886261359227" && rem == 268);
    fail_unless(convertToId(123LL, 3, "") == "01v") ;
    fail_unless(convertToId(12LL, 3, "") == "00B") ;
    fail_unless(convertToId(12LL, 0, "prefix") == "prefix") ;
    fail_unless(convertToId(0xFFFFFFFFFFFFFFFFLL, 11, "") == "Ezzzzzzzzzz");
    fail_unless(convertToId(0xEFFFFFFFFFFFFFFFLL, 10, "") == "zzzzzzzzzz");
    fail_unless(convertToId(uint64_t(0x0000000000000001), 10, "") == "0000000001");
    fail_unless(convertToId(4294967296LL, 10, "") == "0000400000");
    fail_unless(convertToId(65536LL, 15, "") == "000000000000F00");
    fail_unless(convertToId(24210133LL, 15, "") == "00000000001RLfK");
    fail_unless(convertToId(24210133LL, 15, "") == convertToId("24210133", 15, ""));
    fail_unless(convertToId(0xFFFFFFFFFFFFFFFFLL, 11, "") == convertToId("18446744073709551615", 11, ""));
    fail_unless(convertFromId("00000000001RLfK") == 24210133);
    fail_unless(convertFromId("000000000000F00") == 65536);
    fail_unless(convertFromId("000000000000F00") == 65536);
    fail_unless(convertFromId("Ezzzzzzzzzz") == 0xFFFFFFFFFFFFFFFF);
}
END_TEST

enum TestEnum { TE_1 = 1, TE_2 = 10, TE_3 = 3, TE_4 = 4 };

ENUM_NAMES_DECL(TestEnum);

ENUM_NAMES_BEGIN(TestEnum)
    (TE_1, "te1")
    (TE_2, "te2")
    (TE_3, "te3")
    (TE_4, "te4")
ENUM_NAMES_END(TestEnum);
ENUM_NAMES_END_AUX(TestEnum);

struct Foo2
{
enum TestEnum { TE_1 = 10, TE_2 = 100, TE_3 = 30 };
};

ENUM_NAMES_DECL2(Foo2, TestEnum);

ENUM_NAMES_BEGIN2(Foo2, TestEnum)
    (Foo2::TE_1, "fte1")
    (Foo2::TE_2, "fte2")
    (Foo2::TE_3, "fte3")
ENUM_NAMES_END2(Foo2, TestEnum);

START_TEST(enumNames)
{
    fail_unless(strcmp("te1", enumToStr(TE_1)) == 0);
    fail_unless(strcmp("te2", enumToStr(TE_2)) == 0);
    fail_unless(strcmp("te3", enumToStr(TE_3)) == 0);
    try { enumToStr(static_cast<TestEnum>(5)); fail_unless("must throw" == 0); }
    catch (const comtech::Exception& e) { /* pass */ }

    TestEnum v1(TE_3);
    fail_unless(enumFromStr(v1, "te1") == true);
    fail_unless(v1 == TE_1);
    fail_unless(enumFromStr(v1, "te5") == false);

    fail_unless(enumFromStr2("te2", TE_3) == TE_2);
    fail_unless(enumFromStr2("te5", TE_3) == TE_3);

    std::vector<TestEnum> v;
    v.push_back(TE_1);
    v.push_back(TE_3);
    v.push_back(TE_4);
    v.push_back(TE_2);
    fail_unless(enumSorted<TestEnum>() == v);

    std::vector<Foo2::TestEnum> v2;
    v2.push_back(Foo2::TE_1);
    v2.push_back(Foo2::TE_3);
    v2.push_back(Foo2::TE_2);
    fail_unless(enumSorted<Foo2::TestEnum>() == v2, "<%s>", StrUtils::join("-", v2).c_str());
    fail_unless(enumFromStr2("fte3", Foo2::TE_1) == Foo2::TE_3);
    fail_unless(not enumFromStr(v1, enumToStr(Foo2::TE_1)));           // задолбало
    fail_unless(enumToStr2(static_cast<TestEnum>(5)) == nullptr);      // читать
    fail_unless(enumToStr2(static_cast<Foo2::TestEnum>(5)) == nullptr);// целый
    auto te = Foo2::TE_1;                                              // экран
    fail_unless(not enumFromStr(te, "te3"));                           // ворнингов
} END_TEST

static int timeCachingReadInt()
{
    static int i = 0;
    return ++i;
}

START_TEST(time_caching)
{
    HelpCpp::TimeCaching<int> v1(std::chrono::milliseconds(500), timeCachingReadInt);
    fail_unless(v1.get() == 1, "expected 1");
    fail_unless(v1.get() == 1, "expected 1");
    sleep(1);
    fail_unless(v1.get() == 2, "expected 2");
    fail_unless(v1.get() == 2, "expected 2");
} END_TEST

static std::vector<std::string> callstack;

static int callWithFallbackGoodFunc()
{
    callstack.push_back("good func");
    return 1;
}

static int callWithFallbackBadFunc()
{
    callstack.push_back("bad func");
    throw std::runtime_error("badFunc");
}

static int callWithFallbackVeryBadFunc()
{
    callstack.push_back("very bad func");
    throw "veryBadFunc";
}

static void callWithFallbackOnFail(const std::exception& e)
{
    callstack.push_back("on fail");
}

START_TEST(call_with_fallback)
{
    callstack.clear();
    fail_unless(HelpCpp::CallWithFallback<int>(
                callWithFallbackGoodFunc,
                callWithFallbackBadFunc,
                callWithFallbackOnFail).get() == 1, "expected 1");
    fail_unless(callstack.size() == 1);
    fail_unless(callstack[0] == "good func");

    callstack.clear();
    fail_unless(HelpCpp::CallWithFallback<int>(
                callWithFallbackBadFunc,
                callWithFallbackGoodFunc,
                callWithFallbackOnFail).get() == 1, "expected 1");
    fail_unless(callstack.size() == 3);
    fail_unless(callstack[0] == "bad func");
    fail_unless(callstack[1] == "on fail");
    fail_unless(callstack[2] == "good func");

    callstack.clear();
    fail_unless(HelpCpp::CallWithFallback<int>(
                callWithFallbackVeryBadFunc,
                callWithFallbackGoodFunc,
                callWithFallbackOnFail).get() == 1, "expected 1");
    fail_unless(callstack.size() == 3);
    fail_unless(callstack[0] == "very bad func");
    fail_unless(callstack[1] == "on fail");
    fail_unless(callstack[2] == "good func");
} END_TEST

START_TEST(memdump)
{
    auto m = HelpCpp::memdump("1234567890", 10, 2);
    const char* p = " 31 32[33]34 35 36 37 38 12345678\n 39 30                   90\n";
    ck_assert_str_eq(m, p);
}
END_TEST

static std::vector<std::string> funcs;

#define MARK_FUNC(f) LogTrace(TRACE5) << f; funcs.push_back(f)

struct GoodValue
{
    GoodValue(int arg1, int arg2) {
        MARK_FUNC("GoodValue()");
    }
    GoodValue(const GoodValue& that) {
        ASSERT(this != &that);
        MARK_FUNC("GoodValue(GoodValue)");
    }
    ~GoodValue() {
        MARK_FUNC("~GoodValue()");
    }
};

struct BadValue
{
    BadValue(int arg1, int arg2) {
        MARK_FUNC("BadValue()");
    }
    BadValue(const BadValue& that) {
        ASSERT(this != &that);
        MARK_FUNC("BadValue(BadValue)");
    }
    ~BadValue() {
        MARK_FUNC("~BadValue()");
    }
};

#undef MARK_FUNC

START_TEST(expected)
{
    typedef std::shared_ptr<GoodValue> GoodValuePtr;
    typedef Expected<GoodValuePtr, BadValue> Exp;
    funcs.clear();
    Exp e1{std::make_shared<GoodValue>(1, 2)};
    fail_unless(e1.valid(), "with GoodValue must be valid");
    fail_unless(static_cast<bool>(e1), "with GoodValue must be valid");

    fail_unless(funcs.size() == 1);
    fail_unless(funcs.at(0) == "GoodValue()");

    Exp e2{BadValue(3, 4)};

    fail_unless(funcs.size() == 4);
    fail_unless(funcs.at(1) == "BadValue()");
    fail_unless(funcs.at(2) == "BadValue(BadValue)");
    fail_unless(funcs.at(3) == "~BadValue()");

    e1 = e2;

    fail_unless(funcs.size() == 6);
    fail_unless(funcs.at(4) == "~GoodValue()");
    fail_unless(funcs.at(5) == "BadValue(BadValue)");

    try {
        Exp(std::make_shared<GoodValue>(1, 2)).err();
        fail_if(true, "Expected with good value - err must throw");
    } catch (const ServerFramework::EitherExc< BadValue >&) {
    }

    try {
        *Exp(BadValue(5, 6));
        fail_if(true, "Expected with bad value - get must throw");
    } catch (const ServerFramework::EitherExc< GoodValuePtr >&) {
    }

} END_TEST

#define SUITENAME "Serverlib"
TCASEREGISTER(0,0)
{
    ADD_TEST(date_string);
    ADD_TEST(time_string);
    ADD_TEST(to_date);
    ADD_TEST(simple_date_time);
    ADD_TEST(to_date_no_day);
    ADD_TEST(to_time);
    ADD_TEST(to_date_no_year);
    ADD_TEST(to_time_dura);
    ADD_TEST(to_time_no_seconds);
    ADD_TEST(to_time_mbf);
    /*    ADD_TEST(to_time_mbf2);*/
    ADD_TEST(to_time_mbf3);
    ADD_TEST(check_bitdump);
    ADD_TEST(check_slice);
    ADD_TEST(check_arch_seq);
    ADD_TEST(getFilenamesByMask_test);
    ADD_TEST(enumNames)
    ADD_TEST(expected)
    ADD_TEST(time_caching)
    ADD_TEST(call_with_fallback)
    ADD_TEST(vsprintf_string_w);
    ADD_TEST(vsprintf_string_c);
    ADD_TEST(test_bool_opr);
    ADD_TEST(memdump)
}
TCASEFINISH

#undef SUITENAME
#define SUITENAME "cfgread"
TCASEREGISTER(0,0)
{
    ADD_TEST(boolRead)
}
TCASEFINISH

} // namespace
#endif //XP_TESTING
