#if HAVE_CONFIG_H
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <map>
#include <iostream>
#include <dirent.h>
#include <vector>
#include <errno.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include "exception.h"
#include "tcl_utils.h"
#include "localtime.h"
#include "dates.h"

#define NICKNAME "LocalTime"
#include "logger.h"
#include "slogger.h"

class BadLocalTime : public comtech::Exception {
public:
    BadLocalTime(const char*, const char*, int, const char *, const std::string &);
};

TimeZoneCacheController::~TimeZoneCacheController()
{}

bool TimeZoneCacheController::needRefresh()
{
    return false;
}

const std::string& TimeZoneCacheController::tzdir()
{
    static std::string tzdir = readStringFromTcl("SIRENA_TZDIR", "/usr/share/zoneinfo");
    return tzdir;
}

std::unique_ptr<TimeZoneCacheController> timeZoneCacheCtrl(new TimeZoneCacheController);

void setupTimeZoneController(TimeZoneCacheController* c)
{
    timeZoneCacheCtrl.reset(c);
}

class TimeZone
{
    class DataFile;

    struct Transition // перевод часов
    {
        int64_t when; // когда случился
        int64_t gmtoff; // новый оффсет от GMT
        Transition(const int64_t t = 0, const int64_t off = 0) : when(t), gmtoff(off) {};
        bool operator<(const Transition & b) const { return when < b.when; }
    };

    class LocalCompare
    { // функтор для поиска релевантного перевода часов, когда дано локальное время
    public:
        bool operator()(const Transition &, const Transition &) const;
    };

    int64_t baseoff; // оффсет, который был до первого известного перевода часов
    std::vector<Transition> leaps; // массив переводов

    static std::string findZoneName(const char * cstr, const std::string& tzdir);
    static void resolveTZName(std::string &,std::vector<std::string> &,const std::string &, const std::string& tzdir);

    TimeZone(){};
    bool setTimeZone(const char *);
public:
    static const TimeZone* getTimeZone(const char * time_zone);
    static bool checkTimeZone(const char * tzname); // check whether tzname is valid
    static std::map<std::string, std::shared_ptr<TimeZone>> cache;

    std::vector<int64_t> getLeapTimes() const;
    boost::posix_time::ptime localToGMT(const boost::posix_time::ptime &) const; 
    int64_t localToGMT(const char * YYYYMMDDHHMMSS) const; 
    int64_t localToGMT(int64_t time) const;
    boost::posix_time::ptime GMTtoLocal(const boost::posix_time::ptime &) const; 
    int64_t GMTtoLocal(const char * YYYYMMDDHHMMSS) const;
    int64_t GMTtoLocal(int64_t time) const;
};

class TimeZone::DataFile 
{ 
    std::vector<char> vec;
    std::string path; 
    off_t pos;
    off_t size;
public:
    DataFile() : pos(0), size(0) {};
    bool setDataFile(const std::string & zone);
    bool setPos(off_t a);
    bool skip(off_t a);
    off_t getSize() const { return size; }
    off_t getPos() const { return pos; }
    bool eof() { return pos >= size; }
    const std::string & getPath() { return path; } 
    template <typename T> bool readValue(T *);
    char get();
};

static bool isBigEndian()
{
    static const bool res = (htonl((uint32_t) 1) == ((uint32_t) 1));
    return res;
}

bool checkTimeZone(const std::string& zone)
{
    return TimeZone::checkTimeZone(zone.c_str());
}

template <typename T> bool TimeZone::DataFile::readValue(T * val)
{ 
    int valSize = sizeof(T);
    if ( pos + valSize > size ) 
    {
        pos += valSize; // чтобы все последующие обращения гарантированно возвращали false
        return false;
    }
    if (isBigEndian())
    {
        char * begin = (char *) val;
        for (int i = 0; i < valSize; ++i, ++begin)
            *begin = get();
        return true;
    }
    char * begin = (char *) val;
    char * p = begin + valSize - 1;
    for ( ; begin <= p; --p ) 
        *p = get();
    return true;
}

struct Date
{   // Удобочитаемый формат представления даты
    int year; // напр. 1812
    int mon;  // 1-12
    int day;  // 1-31
    int hour; // 0-23
    int min;  // 0-59
    int sec;  // 0-59
    
    Date(){};
    Date(int64_t time) { setDate(time); }
    Date(const char * ch) { setDate(ch); }
    int64_t getTime(); 
    void setDate(const int64_t);
    void setDate(const char * YYYYMMDDHHMMSS);
    void setDate(const std::string& date);
    void dateToStr(char * buf) const;// пишет время в виде YYYYMMDDHHMMSS в буфер
    std::string dateToStr() const;
    static bool isYearLeap(int);
    static const int monthes[][2];
};

BadLocalTime::BadLocalTime(const char* a, const char* b, int c, const char * d, 
                           const std::string & e) : comtech::Exception(a, b, c, d, e) {};

std::string City2City(const std::string& from_time, const std::string& from_zone, const std::string& to_zone)
{
     return LocalTime(from_time,from_zone).setZone(to_zone).getLocalTimeStr();
}

boost::posix_time::ptime City2City(const boost::posix_time::ptime& time, const std::string& zone1, const std::string& zone2)
{
    if(zone1.empty() or zone2.empty())
        return time;
    char c_time[15];
    if(not Dates::ptime_to_undelimited_string(time, c_time))
        return time;
    return LocalTime(c_time,zone1).setZone(zone2).getBoostLocalTime();
}

std::string City2GMT(const std::string& time, const std::string& zone)
{
    return City2City(time,zone,"GMT");
}

boost::posix_time::ptime City2GMT(const boost::posix_time::ptime& time, const std::string& zone)
{
    return City2City(time,zone,"GMT");
}

std::vector<boost::gregorian::date> getZoneLeapDates(const std::string& zone,
        const boost::gregorian::date& start, const boost::gregorian::date& end, bool inUTC)
{
    boost::gregorian::date_period activePeriod(start, end);
    std::vector<boost::gregorian::date> leapDates;
    for (int64_t leapTime : TimeZone::getTimeZone(zone.c_str())->getLeapTimes()) {
        const boost::gregorian::date leapDate(
            inUTC
            ? timeToBoost(leapTime).date()
            : LocalTime().setGMTime(leapTime).setZone(zone).getBoostLocalTime().date()
        );
        if (activePeriod.contains(leapDate)) {
            leapDates.push_back(leapDate);
        }
    }
    return leapDates;
}

std::map<std::string, std::shared_ptr<TimeZone>> TimeZone::cache;

std::string TimeZone::findZoneName(const char * cstr, const std::string& tzdir)
{ // convert wrong-cased (uppercased) filename to right-cased
    std::vector<std::string> fileName;
    if (cstr == NULL) {
        return "";
    }
    const char * end = cstr;
    while (*end != '\0')
    {
        end = strchrnul(cstr, '/');
        fileName.push_back(std::string(cstr, end));
        cstr = end + 1;
    } 

    DIR * dir = opendir(tzdir.c_str());
    if (dir == NULL) {
        return "";
    }

    dirent * entry = NULL;
    std::string currentPath;

    for (std::vector<std::string>::const_iterator it = fileName.begin(); it != fileName.end(); ++it) 
    {
        while (true)
        {
            entry = readdir(dir);
            if (entry == NULL)
            {
                closedir(dir);
                return "";
            }
            if (strcasecmp(entry->d_name, it->c_str()) == 0)
            {
                if (currentPath.size() != 0) {
                    currentPath += '/';
                }
                currentPath += entry->d_name;
                break;
            }
        }
        closedir(dir);
        if (it + 1 != fileName.end())
        {
            dir = opendir((tzdir + '/' + currentPath).c_str());
            if (dir == NULL) {
                return "";
            }
        }
    }
    return currentPath;
}

// пишет в fsName полный путь до tz-файла, а в cacheNames все "псевдонимы" для временной
// зоны с именем inZoneStr
void TimeZone::resolveTZName(std::string & fsName,std::vector<std::string> & cacheNames, 
              const std::string & inZoneStr, const std::string& tzdir)
{
    fsName = TimeZone::findZoneName(inZoneStr.c_str(), tzdir);
    if (fsName.size() == 0) {
        throw BadLocalTime(STDLOG, "resolveTZName", 
                           std::string("Cannot find file for '") + tzdir + '/' + inZoneStr + '\'');
    }
    fsName = tzdir + '/' + fsName;
    // возможно, вместо уникального tzfile'а мы имели дело с линком на другой tzfile,
    // так что целесообразно было бы добавить в кеш с одинаковым указателем все остальные
    // строки-идентификаторы, которые ссылаются на тот же самый конечный файл.
    struct stat st;
    if (lstat(fsName.c_str(), &st) != 0) {
        return;
    }
    std::string fullPath = fsName;
    bool isLink = S_ISLNK(st.st_mode);
    while (isLink) // если рассматриваемый файл -- ссылка, то найдем, на что он ссылается
    {
        //LogTrace(TRACE5) << "file '" << fullPath << "' is symlink";
        static const int BUF_SIZE = 512;
        char buf[BUF_SIZE]; // буфер для чтения симлинка

        int placed = readlink(fullPath.c_str(), buf, BUF_SIZE); 
        if (placed >= BUF_SIZE)
            break;
        buf[placed] = '\0';
        //LogTrace(TRACE5) << "link: '" << buf << '\'';
        if (buf[0] != '/') // если ссылка содержит относительный, а не абсолютный путь
        {
            fullPath.erase(fullPath.rfind('/') + 1);
            fullPath += buf; // получаем абсолютный путь
        }
        else
        {
            fullPath = buf; // получаем абсолютный путь
        }
        //LogTrace(TRACE5) << "link: '" << fullPath << '\'';
        std::string::size_type temp;
        while ((temp = fullPath.find("/../")) != std::string::npos)
        { // убираем возвраты типа /usr/share/../zoneinfo
            fullPath.erase(fullPath.begin() + fullPath.rfind('/', temp - 1), 
                           fullPath.begin() + fullPath.find('/', temp + 1));
        }
        //LogTrace(TRACE5) << "normalized link: '" << fullPath << '\'';
        if (tzdir.size() <= fullPath.size() &&
            strncmp(fullPath.c_str(), tzdir.c_str(), tzdir.size()) == 0)
        { // если абсолютный путь файла находится в той же папке(tzdir), что и линк, то добавим в кеш
            // ########################################
            // прежде чем добавлять в кэш новую пару, надо проапперкейсить tzname
            // Если в сирену таки будет добавлена папка с временными зонами, чьи
            // названия будут уже апперкейснуты, нижеследующее преобразование нужно
            // будет удалить
            // ########################################
            std::string temp(fullPath.c_str() + tzdir.size() + 1);
            transform(temp.begin(),temp.end(),temp.begin(),toupper);
            cacheNames.push_back(temp);
            //LogTrace(TRACE5) << "name added into cacheNames: '" << temp << '\'';
            if (lstat(fullPath.c_str(), &st) == 0) // если новый адрес тоже относится 
                isLink = S_ISLNK(st.st_mode);      // к ссылке, то повторяем процедуру
            else
                isLink = false;
        }
        else
        {
            isLink = false;
        } // тем не менее, если, например, в /usr/share/zoneinfo/posix есть несколько линков, ссылающихся
          // на один и тот же файл в /usr/share/zoneinfo, то в кеш они пойдут по отдельности
    }
}

const TimeZone* TimeZone::getTimeZone(const char * inZone) {
    if (timeZoneCacheCtrl->needRefresh()) {
        TimeZone::cache.clear();
    }
    const std::string& tzdir = timeZoneCacheCtrl->tzdir();
    /*
      проапперкейсить
    */
    // ########################################
    // Входящая строка явно приводится к верхнему регистру.
    // Если в сирену таки будет добавлена папка с временными зонами, чьи
    // названия будут уже апперкейснуты, нижеследующее преобразование нужно
    // будет удалить
    // ########################################
    std::string inZoneStr = inZone;
    // раз уж у нас в сирене зоны хранятся в верхнем регистре, то нет необходимости
    // в этом преобразовании. только надо еще в тестах везде перейти на исключительно верхний регистр
    //transform(inZoneStr.begin(),inZoneStr.end(),inZoneStr.begin(),toupper);
    /*
      пошукать в кеше
    */
    auto it = cache.find(inZoneStr);
    if (it != cache.end())
    {
        //LogTrace(TRACE5) << "TZ '" << inZoneStr << "' found in cache";
        return it->second.get();
    }
    //LogTrace(TRACE5) << "TZ '" << inZoneStr << "' not found in cache";
    
    /*
      составить список возможных имен в кеше
    */
    std::string fsName;
    std::vector<std::string> cacheNames;
    resolveTZName(fsName, cacheNames, inZoneStr, tzdir);
    /*
      пошукать в кеше
    */
    for (auto& k : cacheNames) {
        it = cache.find(k);
        if (it != cache.end())
        {
            //LogTrace(TRACE5) << "TZ '" << *k << "' found in cache";
            for (auto& l : cacheNames) {
                cache.emplace(l,it->second);
            }
            cache.emplace(inZoneStr,it->second);
            return it->second.get();
        }
        //LogTrace(TRACE5) << "TZ '" << *k << "' not found in cache";
    }
    /*
      создать объект
    */
    std::shared_ptr<TimeZone> tzp(new TimeZone);
    if (tzp->setTimeZone(fsName.c_str()) == false)
    {
        throw BadLocalTime(STDLOG, "getTimeZone", 
                                 std::string("Cannot find valid TZFile for '") + inZone + '\'');
    }

    cache.emplace(inZoneStr, tzp);
    //LogTrace(TRACE5) << "pair added into cache: '" << inZoneStr << "' " << tzp;    
    for(auto& k : cacheNames) {
        cache.emplace(k, tzp);
        //LogTrace(TRACE5) << "pair added into cache: '" << *k << "' " << tzp;    
    }
    return tzp.get();
}

bool TimeZone::checkTimeZone(const char * const tzname) {
    const char * begin = tzname;
    while(isspace(*begin)) {
        ++begin;
    }
    const char * end = begin;
    while(*end != '\0') {
        ++end;
    }
    --end;
    do {
        while(isspace(*end) && end > begin) {
            --end;
        }
    } while( end > tzname && (end[-1] == '\033' || end[0] == '\033') && --end);
    std::string newName(begin, end - begin + 1);
    const std::string& tzdir = timeZoneCacheCtrl->tzdir();
    newName = findZoneName(newName.c_str(), tzdir);
    newName = tzdir + '/' + newName;
    TimeZone tz;
    return tz.setTimeZone(newName.c_str());
}

bool TimeZone::setTimeZone(const char * zone)  
{
    TimeZone::DataFile file;
    if (file.setDataFile(zone) == false)
        return false;

    if ( ! ( file.get() == 'T' &&
             file.get() == 'Z' &&
             file.get() == 'i' &&
             file.get() == 'f' ) )
    {
        LogError(STDLOG) << "Magic string is not TZif in: '" << zone << "'";
        return false;
    }

    file.setPos(20); 
    int32_t tzh_ttisgmtcnt; file.readValue(&tzh_ttisgmtcnt);
    int32_t tzh_ttisstdcnt; file.readValue(&tzh_ttisstdcnt);
    int32_t tzh_leapcnt; file.readValue(&tzh_leapcnt);
    int32_t tzh_timecnt; file.readValue(&tzh_timecnt);
    int32_t tzh_typecnt; file.readValue(&tzh_typecnt);
    int32_t tzh_charcnt; file.readValue(&tzh_charcnt);

    if (file.eof())
    {
        LogError(STDLOG) << "Unexpected end of file: '" << zone << "'";
        return false;
    }

    file.skip(5 * tzh_timecnt + 6 * tzh_typecnt + tzh_charcnt + 
              8 * tzh_leapcnt + tzh_ttisgmtcnt + tzh_ttisstdcnt);

    if ( ! ( file.get() == 'T' &&
             file.get() == 'Z' &&
             file.get() == 'i' &&
             file.get() == 'f' ) )
    {
        LogError(STDLOG) << "Second magic string is not TZif in: '" << zone << "'";
        return false;
    }

    file.skip(28);
    file.readValue(&tzh_timecnt);
    file.readValue(&tzh_typecnt); 
    file.skip(4);

    std::vector<int64_t> transitions(tzh_timecnt);
    for (int i = 0; i < tzh_timecnt; ++i)
        file.readValue(&transitions[i]);
    std::vector<unsigned char> indices(tzh_timecnt);
    for (int i = 0; i < tzh_timecnt; ++i)
        file.readValue(&indices[i]);
    std::vector<int32_t> offsets(tzh_typecnt);
    for (int i = 0; i < tzh_typecnt; ++i)
    {
        file.readValue(&offsets[i]);
        if (file.skip(2) == false)
        {
            LogError(STDLOG) << "Invalid file: '" << zone << "'";
            return false;
        }
    }
    // метки для local/UTC и Std/Wall по всей видимости не должны использоваться, так
    // что их игнорируем
    baseoff = offsets[0];
    leaps.resize(tzh_timecnt);
    for (int i = 0; i < tzh_timecnt; ++i)
    {
        leaps[i].when = transitions[i];
        leaps[i].gmtoff = offsets[indices[i]];
    }
    return true;
}

std::string timeToStr(int64_t t)
{
    return Date(t).dateToStr();
}

bool TimeZone::LocalCompare::operator()(const Transition & a, const Transition & b) const
{
    return (a.when + a.gmtoff) < (b.when + b.gmtoff);
}

bool TimeZone::DataFile::setDataFile(const std::string & zone) 
{
    path = zone;

    struct stat st;
    if ( !(stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode)) )
    {
//        LogTrace(TRACE5) << "file '" << zone << "' not found in FS";
        return false;
    }
//    LogTrace(TRACE5) << "file '" << zone << "' found in FS";

    size = st.st_size;
    vec.resize(size);
    int file = open(path.c_str(), O_RDONLY);
    if ( file < 0 )
    {
        LogError(STDLOG) << "Cannot open file '" << zone << "': " << strerror(errno);
        return false;
    }
//    LogTrace(TRACE5) << "file '" << zone << "' opened";
    if ( read(file, &vec[0], size) != size ) 
    {
        LogError(STDLOG) << "file '" << zone << "' cannot be read: " << strerror(errno);
        if (close(file) != 0) 
            LogError(STDLOG) << "file '" << zone << "' cannot be closed: " << strerror(errno);
//        else
//            LogTrace(TRACE5) << "file '" << zone << "' closed";
        return false;
    }
    pos = 0;
    if (close(file) != 0) 
        LogError(STDLOG) << "file '" << zone << "' cannot be closed: " << strerror(errno);
//    else
//        LogTrace(TRACE5) << "file '" << zone << "' closed";
    return true;
}

bool TimeZone::DataFile::setPos(off_t a) 
{
    pos = a;
    if (a <= size)
        return true;
    else
        return false;
}

bool TimeZone::DataFile::skip(off_t a) 
{
    pos += a;
    if (pos <= size)
        return true;
    else
        return false;
}

char TimeZone::DataFile::get() 
{ 
    if (eof())
        return '\0';
    else
        return vec[pos++]; 
}

const int Date::monthes[][2] = { // количество дней в году до начала месяца
    { 0, 0 }, // <- чтобы monthes[1][i] соответствовало январю
    { 1, 0 },
    { 2, 31 },
    { 3, 59 },
    { 4, 90 },
    { 5, 120 },
    { 6, 151 },
    { 7, 181 },
    { 8, 212 },
    { 9, 243 },
    { 10, 273 },
    { 11, 304 },
    { 12, 334 }
};

int64_t Date::getTime()
{
    int year_epoch = 1970;
//  int mon_epoch = 1;
    int day_epoch = 1;

    int temp = year / 100;
    int a_leap = (year >> 2) - temp + (temp >> 2);

    // если номер месяца меньше трех и год високосный
    if ( mon < 3 &&  !((year & 3) || (100 * temp == year && (temp & 3))))
        --a_leap;
    int epoch_leap = 477; //(year_epoch / 4) - (year_epoch / 100) + (year_epoch / 400);

    int64_t ret = 365*(year - year_epoch) + (a_leap - epoch_leap) + 
        (day  + monthes[mon][1] - day_epoch); // days
    ret *= 24; 
    ret += hour; // hours
    ret *= 60;
    ret += min; // minutes
    ret *= 60;
    return (ret + sec); // seconds
}

bool Date::isYearLeap(int year) 
{
    // номер года делится на четыре и
    // либо год не делится на 100, либо результат деления на 100 делится еще и на 4
    // (т. е. год делится на 400)
    int temp = year / 100;
    return  !((year & 3) || (100 * temp == year && (temp & 3)));
}

void Date::setDate(int64_t t)
{
#define GET_POSITIVE_FRACTION(T, D, F) {  F = T % D;  T /= D;  if(F < 0) {  F += D;  T -= 1;  }  }
    GET_POSITIVE_FRACTION(t, 60, sec)
    GET_POSITIVE_FRACTION(t, 60, min)
    GET_POSITIVE_FRACTION(t, 24, hour)
#undef GET_POSITIVE_FRACTION
    int all_days = t;
    year = 1970 + all_days / 365.2425;
    int temp = (year - 1) / 100; // нас интересует сколько високосных годов было до начала year,
                                 // является ли сам year високосным, пока не важно
    int leap = ((year - 1) >> 2) - temp + (temp >> 2);
    all_days -= (365 * (year - 1970) + (leap - 477));

    while (all_days < 0)
    {
        --year;
        all_days += isYearLeap(year) ? 366 : 365;
    }
    while (true)
    {
        int corr = isYearLeap(year) ? 1 : 0;
        if (all_days >= 365 + corr)
        {
            ++year; 
            all_days -= (365 + corr);
            continue;
        }
        for (mon = 1; mon <= 12; ++mon)
        {
            if ((mon < 3 && all_days < monthes[mon][1]) || 
                (mon >= 3 && all_days < monthes[mon][1] + corr))
                break;
        }
        --mon;
        day = all_days - monthes[mon][1] + 1; // если с начала месяца прошел один день,
                                              // то должно быть 2-е число, а не 1-е
        if (mon > 2)
            day -= corr;
        break;
    }
}

void Date::setDate(const char * a)
{
    static Dates::M_mass Mon[12]= {{"JAN", 1,31},{"FEB", 2,29},{"MAR", 3,31},{"APR", 4,30},
                 {"MAY", 5,31},{"JUN", 6,30},{"JUL", 7,31},{"AUG", 8,31},
                 {"SEP", 9,30},{"OCT",10,31},{"NOV",11,30},{"DEC",12,31}};

    const char * b = a;
    bool isStringCorrect = true;
    for (int i = 0; i < 14; ++i, ++b)
        if (!isdigit(*b))
            isStringCorrect = false;
    year = 1000*a[0] + 100*a[1] + 10*a[2] + a[3] - 1111*'0';
    mon = 10*a[4] + a[5] - 11*'0';
    day = 10*a[6] + a[7] - 11*'0';
    hour = 10*a[8] + a[9] - 11*'0';
    min = 10*a[10] + a[11] - 11*'0';
    sec = 10*a[12] + a[13] - 11*'0';
    // check whether DateString is valid
    if (mon > 12 || mon == 0 || day == 0 || hour > 23 || min > 59 || sec > 59 ||
        day > Mon[mon - 1].ndays || (mon == 2 && !isYearLeap(year) && day > 28))
        isStringCorrect = false; 
    if (!isStringCorrect)
        throw BadLocalTime(STDLOG, "setDate", "Incorrect DateString: '" + std::string(a, 14) + '\'');
}

void Date::dateToStr(char * buf) const
{
    if ( snprintf(buf, 15, "%04d%02d%02d%02d%02d%02d", year, mon, 
                  day, hour, min, sec)  >= 15 )
        throw BadLocalTime(STDLOG, "dateToStr", "invalid date");
}    
std::string Date::dateToStr() const
{
    char buf[20] = {};
    dateToStr(buf);
    return buf;
}

int64_t strToTime(const char * a)
{
    Date b(a);
    return b.getTime();
}

int64_t strToTime(const std::string& str)
{
    return strToTime(str.c_str());
}

std::vector<int64_t> TimeZone::getLeapTimes() const
{
    std::vector<int64_t> leapTimes;
    for(const Transition& leap:  leaps) {
        leapTimes.push_back(leap.when);
    }
    return leapTimes;
}

int64_t TimeZone::localToGMT(const char * inp) const // YYYYMMDDHHMMSS
{
    return localToGMT(strToTime(inp));
}

int64_t TimeZone::localToGMT(int64_t time) const
{
    typedef std::vector<Transition>::const_iterator Iterator;    
    // нужный нам offset находится перед первым переводом, случившимся после
    // переданной в качестве параметра даты
    Iterator it = lower_bound(leaps.begin(), leaps.end(), time, LocalCompare());
    if (it == leaps.begin())
    {
        return time - baseoff;
    }
    else
    {
        --it;
        return time - it->gmtoff;
    }
}

boost::posix_time::ptime TimeZone::localToGMT(const boost::posix_time::ptime & inTime) const
{
    return timeToBoost(localToGMT(boostToTime(inTime)));
}

int64_t TimeZone::GMTtoLocal(const int64_t inTime) const
{
    typedef std::vector<Transition>::const_iterator Iterator;
    // нужный нам offset находится перед первым переводом, случившимся после
    // переданной в качестве параметра даты
    Iterator it = lower_bound(leaps.begin(), leaps.end(), inTime); 
    if (it == leaps.begin()) 
    {
        return inTime + baseoff;
    }
    else
    {
        --it;
        return inTime + it->gmtoff;
    }
}

boost::posix_time::ptime TimeZone::GMTtoLocal(const boost::posix_time::ptime & inTime) const
{
    return timeToBoost(GMTtoLocal(boostToTime(inTime)));
}

int64_t TimeZone::GMTtoLocal(const char * inTime) const 
{
    return GMTtoLocal(strToTime(inTime));
}

int64_t boostToTime(const boost::posix_time::ptime& inTime)
{
    if (inTime.is_special()) {
        throw BadLocalTime(STDLOG, "boostToTime", "Uninitialized boost::posix_time::ptime argument");
    }
    static const boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
    boost::posix_time::time_duration diff(inTime - epoch);
    return (diff.ticks() / diff.ticks_per_second());
}

boost::posix_time::ptime timeToBoost(int64_t inTime)
{
    boost::posix_time::seconds time_t_leap(0);
    while(inTime > std::numeric_limits<int32_t>::max())
    {
        time_t_leap += boost::posix_time::seconds(std::numeric_limits<int32_t>::max());
        inTime -= std::numeric_limits<int32_t>::max();
    }
    return boost::posix_time::from_time_t(inTime) + time_t_leap;
}

LocalTime::LocalTime(const int64_t t, const char * tzStr)
    : zone(TimeZone::getTimeZone(tzStr)), gmt(zone->localToGMT(t)) {}

LocalTime::LocalTime(const int64_t t, const std::string& tzStr) 
    : zone(TimeZone::getTimeZone(tzStr.c_str())), gmt(zone->localToGMT(t)) {}

LocalTime::LocalTime(const boost::posix_time::ptime & time,
                     const char * tzStr)
    : zone(TimeZone::getTimeZone(tzStr)), 
      gmt(zone->localToGMT(boostToTime(time))) {}

LocalTime::LocalTime(const boost::posix_time::ptime& time,
        const std::string& tzStr)
    : zone(TimeZone::getTimeZone(tzStr.c_str())),
      gmt(zone->localToGMT(boostToTime(time))) {}

LocalTime::LocalTime(const char * time, const char * city) 
    : zone(TimeZone::getTimeZone(city)), gmt(zone->localToGMT(time)) {}

LocalTime::LocalTime(const int64_t time) : zone(TimeZone::getTimeZone("GMT")), gmt(time) {}

LocalTime::LocalTime(const boost::posix_time::ptime & time) 
    : zone(TimeZone::getTimeZone("GMT")), gmt(boostToTime(time)) {}

LocalTime::LocalTime(const char * YYYYMMDDHHMMSS)
    : zone(TimeZone::getTimeZone("GMT")), gmt(strToTime(YYYYMMDDHHMMSS)) {}

LocalTime::LocalTime(const std::string& YYYYMMDDHHMMSS)
    : zone(TimeZone::getTimeZone("GMT")), gmt(strToTime(YYYYMMDDHHMMSS)) {}

LocalTime::LocalTime(const std::string& time, const std::string& city) 
    : zone(TimeZone::getTimeZone(city.c_str())), gmt(zone->localToGMT(time.c_str())) {}

int64_t operator-(const LocalTime & a, const LocalTime & b)
{
    return a.gmt - b.gmt;
}

LocalTime LocalTime::operator-(const boost::posix_time::time_duration & t) const
{
    if (t.is_special()) {
        throw BadLocalTime(STDLOG, "LocalTime::operator-","Uninitialized boost::posix_time::time_duration argument");
    }
    return LocalTime(gmt - t.total_seconds(), zone);
}

LocalTime LocalTime::operator-(const int64_t t) const
{
    return LocalTime(gmt - t, zone);
}

LocalTime & LocalTime::operator-=(const boost::posix_time::time_duration & t)
{
    if (t.is_special()) {
        throw BadLocalTime(STDLOG, "LocalTime::operator-=","Uninitialized boost::posix_time::time_duration argument");
    }
    gmt -= t.total_seconds();
    return *this;
}

LocalTime & LocalTime::operator-=(const int64_t t)
{
    gmt -= t;
    return *this;
}

LocalTime LocalTime::operator+(const boost::posix_time::time_duration & t) const
{
    if (t.is_special()) {
        throw BadLocalTime(STDLOG, "LocalTime::operator+","Uninitialized boost::posix_time::time_duration argument");
    }
    return LocalTime(gmt + t.total_seconds(), zone);
}

LocalTime LocalTime::operator+(const int64_t t) const
{
    return LocalTime(gmt + t, zone);
}

LocalTime & LocalTime::operator+=(const boost::posix_time::time_duration & t)
{
    if (t.is_special()) {
        throw BadLocalTime(STDLOG, "LocalTime::operator+=","Uninitialized boost::posix_time::time_duration argument");
    }
    gmt += t.total_seconds();
    return *this;
}

LocalTime & LocalTime::operator+=(const int64_t t)
{
    gmt += t;
    return *this;
}

bool operator<(const LocalTime & a, const LocalTime & b)
{
    return a.gmt < b.gmt;
}

bool operator==(const LocalTime & a, const LocalTime & b)
{
    return a.gmt == b.gmt;
}

LocalTime & LocalTime::setZone(const char * city)
{
    zone = TimeZone::getTimeZone(city);
    return *this;
}

LocalTime & LocalTime::setZone(const std::string& city)
{
    return setZone(city.c_str());
}

LocalTime & LocalTime::setLocalTime(const boost::posix_time::ptime & t) 
{
    gmt = zone->localToGMT(boostToTime(t));
    return *this;
}

LocalTime & LocalTime::setLocalTime(const char * time)
{
    gmt = zone->localToGMT(time);
    return *this;
}

LocalTime & LocalTime::setLocalTime(const std::string& time)
{
    return setLocalTime(time.c_str());
}

LocalTime & LocalTime::setLocalTime(const int64_t time)
{
    gmt = zone->localToGMT(time);
    return *this;
}

LocalTime & LocalTime::setGMTime(const boost::posix_time::ptime & t)
{
    gmt = boostToTime(t);
    return *this;
}

LocalTime & LocalTime::setGMTime(const int64_t t)
{
    gmt = t;
    return *this;
}

LocalTime & LocalTime::setGMTime(const char * time)
{
    gmt = strToTime(time);
    return *this;
}

LocalTime & LocalTime::setGMTime(const std::string& time)
{
    return setGMTime(time.c_str());
}

boost::posix_time::ptime LocalTime::getBoostLocalTime() const
{
    return timeToBoost(zone->GMTtoLocal(gmt));
}

boost::posix_time::ptime LocalTime::getBoostGMTime() const
{
    return timeToBoost(gmt);
}

int64_t LocalTime::getLocalTime() const
{
    return zone->GMTtoLocal(gmt);
}

std::string LocalTime::getLocalTimeStr() const 
{
    return timeToStr(zone->GMTtoLocal(gmt));
}

std::string LocalTime::getGMTimeStr() const 
{
    return timeToStr(gmt);
}

#ifdef XP_TESTING
#include "xp_test_utils.h"
#include "checkunit.h"
#include "sirenaproc.h"

void init_localtime_tests()
{
}

using boost::posix_time::from_iso_string;

START_TEST (conversions)
{
    LocalTime loc_time1;
    LocalTime loc_time2;

    loc_time1 = LocalTime("20111111100000", "EUROPE/MOSCOW");
    loc_time2.setZone("GMT").setLocalTime("20111111060000");
    fail_unless(loc_time1 == loc_time2);

    loc_time1.setZone("ANTARCTICA/VOSTOK").setLocalTime(from_iso_string("20111111T110000"));
    loc_time2.setZone("EUROPE/VATICAN").setLocalTime("20111111060000");
    fail_unless(loc_time1.getGMTime() == loc_time2.getGMTime());
 
    loc_time1.setZone("EUROPE/MOSCOW").setLocalTime("20100328010000");
    loc_time2.setZone("GMT").setLocalTime("20100327220000");
    fail_unless(loc_time1 == loc_time2);

    loc_time1.setZone("EUROPE/MOSCOW").setLocalTime("20100328020000");
    loc_time2.setZone("GMT").setLocalTime("20100327230000");
    fail_unless(loc_time1 == loc_time2);

    loc_time1.setZone("EUROPE/MOSCOW").setLocalTime("20100328020001");
    loc_time2.setZone("GMT").setLocalTime("20100327230001");
    fail_unless(loc_time1 == loc_time2);

    loc_time1.setZone("EUROPE/MOSCOW").setLocalTime("20100328030000");
    loc_time2.setZone("GMT").setLocalTime("20100328000000");
    fail_unless(loc_time1 == loc_time2);

    loc_time1.setZone("EUROPE/MOSCOW").setLocalTime("20100328030001");
    loc_time2.setZone("GMT").setLocalTime("20100327230001");
    fail_unless(loc_time1 == loc_time2);

    loc_time1.setZone("EUROPE/MOSCOW").setLocalTime("20100328040001");
    loc_time2.setZone("GMT").setLocalTime("20100328000001");
    fail_unless(loc_time1 == loc_time2);

    loc_time1.setZone("EUROPE/MOSCOW").setLocalTime("20101031010000");
    loc_time2.setZone("GMT").setLocalTime("20101030210000");
    fail_unless(loc_time1 == loc_time2);

    loc_time1.setZone("EUROPE/MOSCOW").setLocalTime("20101031020000");
    loc_time2.setZone("GMT").setLocalTime("20101030220000");
    fail_unless(loc_time1 == loc_time2);

    loc_time1.setZone("EUROPE/MOSCOW").setLocalTime("20101031020001");
    loc_time2.setZone("GMT").setLocalTime("20101030230001");
    fail_unless(loc_time1 == loc_time2);

    loc_time1.setZone("EUROPE/MOSCOW").setLocalTime("20101031030001");
    loc_time2.setZone("GMT").setLocalTime("20101031000001");
    fail_unless(loc_time1 == loc_time2);
    
    loc_time1.setZone("EUROPE/MOSCOW").setGMTime("20101030225959");
    fail_unless(loc_time1.getBoostLocalTime() == from_iso_string("20101031T025959"));

    loc_time1.setZone("EUROPE/MOSCOW").setGMTime("20101030230000");
    fail_unless(loc_time1.getBoostLocalTime() == from_iso_string("20101031T030000"));

    loc_time1.setZone("EUROPE/MOSCOW").setGMTime("20101030230001");
    fail_unless(loc_time1.getBoostLocalTime() == from_iso_string("20101031T020001"));

    loc_time1 = LocalTime("20110313020000", "AMERICA/NEW_YORK");
    fail_unless(loc_time1.getBoostGMTime() == from_iso_string("20110313T070000"));

    loc_time1 = LocalTime(from_iso_string("20110313T020001"), "AMERICA/NEW_YORK");
    fail_unless(loc_time1.getBoostGMTime() == from_iso_string("20110313T070001"));

    loc_time1 = LocalTime(boostToTime(from_iso_string("20110313T030001")), "AMERICA/NEW_YORK");
    fail_unless(loc_time1.getBoostGMTime() == from_iso_string("20110313T070001"));

    loc_time1 = LocalTime(from_iso_string("20111106T010000"), "AMERICA/NEW_YORK");
    fail_unless(loc_time1.getBoostGMTime() == from_iso_string("20111106T050000"));

    loc_time1 = LocalTime(boostToTime(from_iso_string("20111106T010001")), "AMERICA/NEW_YORK");
    fail_unless(loc_time1.getBoostGMTime() == from_iso_string("20111106T060001"));
                
    loc_time1 = LocalTime(boostToTime(from_iso_string("20111106T055959")));
    loc_time1.setZone("AMERICA/NEW_YORK");
    fail_unless(loc_time1.getBoostLocalTime() == from_iso_string("20111106T015959"));

    loc_time1 = LocalTime(from_iso_string("20111106T060000"));
    loc_time1.setZone("AMERICA/NEW_YORK");
    fail_unless(loc_time1.getBoostLocalTime() == from_iso_string("20111106T020000"));

    loc_time1 = LocalTime("20111106060001");
    loc_time1.setZone("AMERICA/NEW_YORK");
    fail_unless(loc_time1.getBoostLocalTime() == from_iso_string("20111106T010001"));

    loc_time1.setZone("EUROPE/PARIS").setLocalTime("20110327020000");
    fail_unless(loc_time1.getBoostGMTime() == from_iso_string("20110327T010000"));

    loc_time1.setZone("EUROPE/PARIS").setLocalTime("20110327020001");
    fail_unless(loc_time1.getBoostGMTime() == from_iso_string("20110327T010001"));

    loc_time1.setZone("EUROPE/PARIS").setLocalTime("20110327030001");
    fail_unless(loc_time1.getBoostGMTime() == from_iso_string("20110327T010001"));

    loc_time1.setZone("EUROPE/PARIS").setLocalTime("20110327030000");
    fail_unless(loc_time1.getBoostGMTime() == from_iso_string("20110327T020000"));

    loc_time1.setZone("EUROPE/PARIS").setLocalTime("20111030020000");
    fail_unless(loc_time1.getBoostGMTime() == from_iso_string("20111030T000000"));

    loc_time1.setZone("EUROPE/PARIS").setLocalTime("20111030020001");
    fail_unless(loc_time1.getBoostGMTime() == from_iso_string("20111030T010001"));
                
    loc_time1.setZone("EUROPE/PARIS").setGMTime("20111030005959");
    fail_unless(loc_time1.getBoostLocalTime() == from_iso_string("20111030T025959"));

    loc_time1.setZone("EUROPE/PARIS").setGMTime("20111030010000");
    fail_unless(loc_time1.getBoostLocalTime() == from_iso_string("20111030T030000"));

    loc_time1.setZone("EUROPE/PARIS").setGMTime("20111030010001");
    fail_unless(loc_time1.getBoostLocalTime() == from_iso_string("20111030T020001"));
}
END_TEST

START_TEST(constructors)
{
    fail_unless(strToTime("19700101000000") == 0);
    fail_unless(strToTime("19800228123030") == 320589030);
    fail_unless(strToTime("19800229121314") == 320674394);
    fail_unless(strToTime("19800301000000") == 320716800);
    fail_unless(strToTime("19600128000000") == -313286400);
    fail_unless(timeToBoost(strToTime("20000228000000")) == from_iso_string("20000228T000000"));
    fail_unless(timeToBoost(strToTime("20000301121314")) == from_iso_string("20000301T121314"));
    fail_unless(timeToBoost(strToTime("20390901040000")) == from_iso_string("20390901T040000"));
    fail_unless(timeToBoost(strToTime("30390901040000")) == from_iso_string("30390901T040000"));
    fail_unless(timeToBoost(strToTime("19390901040000")) == from_iso_string("19390901T040000"));
    fail_unless(timeToBoost(strToTime("19400410200000")) == from_iso_string("19400410T200000"));
    fail_unless(strToTime("20000113161718") == boostToTime(from_iso_string("20000113T161718")));
    fail_unless(strToTime("20001231210000") == boostToTime(from_iso_string("20001231T210000")));
    fail_unless(strToTime("19500802020000") == boostToTime(from_iso_string("19500802T020000")));

    fail_unless(LocalTime(1).setZone("EUROPE/KIEV").setLocalTime(1234567) == LocalTime("19700115125607", "ASIA/TOKYO"));
    fail_unless(LocalTime(from_iso_string("19911121T125520")).getLocalTime() == 
                (from_iso_string("19911121T125520") - from_iso_string("19700101T000000")).total_seconds());
    fail_unless(strToTime("19720101000000") == 63072000);
    fail_unless(LocalTime(63072000) == LocalTime("19720101000000"));
    fail_unless(LocalTime(63071999) == LocalTime("19711231235959"));
}
END_TEST

START_TEST(string_compare)
{
    std::string s = timeToStr(strToTime("19841130121213"));
    fail_unless(s == "19841130121213");
    s = timeToStr(strToTime("19330512000202"));
    fail_unless(s == "19330512000202");
    s = LocalTime("20250606050403").setZone("ASIA/GAZA").getLocalTimeStr();
    fail_unless(s == "20250606080403");
    s = LocalTime(12345678).setZone("AUSTRALIA/SYDNEY").getGMTimeStr();
    fail_unless(s == "19700523212118");
    s = LocalTime(63072000).getGMTimeStr();
    fail_unless(s == "19720101000000");
    s = LocalTime(63071999).getGMTimeStr();
    fail_unless(s == "19711231235959");
    s = LocalTime(68256000).getGMTimeStr();
    fail_unless(s == "19720301000000");
    s = LocalTime(68255999).getGMTimeStr();
    fail_unless(s == "19720229235959");
    s = LocalTime(32503680000LL).getGMTimeStr();
    fail_unless(s == "30000101000000");
    s = LocalTime(32503679999LL).getGMTimeStr();
    fail_unless(s == "29991231235959");
    s = LocalTime(32508777600LL).getGMTimeStr();
    fail_unless(s == "30000301000000");
    s = LocalTime(32508777599LL).getGMTimeStr();
    fail_unless(s == "30000228235959");
    s = LocalTime(-4986057600LL).getGMTimeStr();
    fail_unless(s == "18120101000000");
    s = LocalTime(-4986057601LL).getGMTimeStr();
    fail_unless(s == "18111231235959");
    s = LocalTime(-4980873600LL).getGMTimeStr();
    fail_unless(s == "18120301000000");
    s = LocalTime(-4980873601LL).getGMTimeStr();
    fail_unless(s == "18120229235959");
}
END_TEST

START_TEST(time_arithmetic)
{
    fail_unless(LocalTime(1000) - LocalTime("19691231230000", "GMT") == 4600);
    fail_unless(LocalTime("19690707030000", "GMT") - LocalTime("19710707030000", "GMT") == -63072000);
    fail_unless(LocalTime(from_iso_string("19700510T070000")) - boost::posix_time::time_duration(3102, 0, 0) == LocalTime(3600, "GMT"));
    fail_unless(LocalTime(from_iso_string("20111111T144337")) - 2400 == LocalTime("20111111140337", "gmt"));
    fail_unless((LocalTime("20300601010203") -= boost::posix_time::time_duration(720, 0, 0)) == LocalTime("20300502010203"));
    fail_unless((LocalTime("20200301010203") -= 2592000) == LocalTime("20200131010203"));
    fail_unless(LocalTime("20130915020800") + boost::posix_time::time_duration(1, 30, 20) == LocalTime("20130915033820"));
    fail_unless(LocalTime("20130915020800") + 5420 == LocalTime("20130915033820"));
    fail_unless((LocalTime("20161215230800") += boost::posix_time::time_duration(1, 30, 20)) == LocalTime("20161216003820"));
    fail_unless((LocalTime("20000615020800") += 5420) == LocalTime("20000615033820"));
    LocalTime a("20020120034521");
    LocalTime b("19170223013245");
    fail_unless(!(a < b));
    fail_unless(b < a);
    fail_unless(!(a == b));
    fail_unless(a == a);
}
END_TEST

START_TEST(huge_date)
{
    fail_unless(LocalTime("22000228120000") + 86400 == LocalTime("22000301120000"));
    fail_unless(LocalTime("23000505231020", "TURKEY") == LocalTime(from_iso_string("23000505T231020"), "TURKEY"));
    fail_unless(LocalTime("24000302200000", "AFRICA/ALGIERS") - LocalTime("24000227170000", "JAMAICA") == 334800);
}
END_TEST

START_TEST(TZ_misc)
{
    bool (&check)(const char * const) = TimeZone::checkTimeZone;
    fail_unless(check("EeT"));
    fail_unless(check("EurOpe/MosCOW       "));
    fail_unless(check("Europe/MosCOW      \033a  "));
    fail_unless(check("Europe/MosCOW      \033  "));
    fail_unless(!check("XXX"));
    fail_unless(!check("Europe/Moscow\n?"));
    fail_unless(!check("Europe/MoscowФЫА"));
    
    fail_unless(TimeZone::getTimeZone("EUROPE/MOSCOW") == TimeZone::getTimeZone("EUROPE/MOSCOW"));
    fail_unless(TimeZone::getTimeZone("GMT") == TimeZone::getTimeZone("GMT"));
    // как оказалось, не на всех системах Portugal -- это симлинк на Europe/Lisbon 
    //fail_unless(getzone("PORTUGAL") == getzone("EUROPE/LISBON"));
    fail_unless(TimeZone::getTimeZone("UTC") != TimeZone::getTimeZone("W-SU"));
}
END_TEST

START_TEST(TZ_wrappers)
{
    fail_unless(std::string("20120209120800") == City2City("20120209200800","asia/vladivostok","europe/kaliningrad"));
    fail_unless(std::string("20120209090800") == City2GMT("20120209200800","Asia/Vladivostok"));
}
END_TEST

START_TEST(zone_leaps)
{
    fail_unless(std::vector<boost::gregorian::date>() ==
            getZoneLeapDates("EUROPE/MOSCOW", boost::gregorian::date(2013,1,1), boost::gregorian::date(2012,1,1)));
    fail_unless(std::vector<boost::gregorian::date>() ==
            getZoneLeapDates("EUROPE/MOSCOW", boost::gregorian::date(2012,1,30), boost::gregorian::date(2013,1,30)));
    fail_unless(std::vector<boost::gregorian::date>(1, boost::gregorian::date(2012,3,25)) ==
            getZoneLeapDates("EUROPE/BERLIN", boost::gregorian::date(2012,2,20), boost::gregorian::date(2012,7,20)));
    std::vector<boost::gregorian::date> chicagoLeaps;
    chicagoLeaps.push_back(boost::gregorian::date(2012,3,11));
    chicagoLeaps.push_back(boost::gregorian::date(2012,11,4));
    fail_unless(chicagoLeaps ==
            getZoneLeapDates("AMERICA/CHICAGO", boost::gregorian::date(2012,1,1), boost::gregorian::date(2013,1,1)));
    fail_unless(std::vector<boost::gregorian::date>() == getZoneLeapDates("GMT", boost::gregorian::date(2012,1,1),
                boost::gregorian::date(2013,2,2)));
}
END_TEST

#define SUITENAME "TZConversions"
TCASEREGISTER(0, 0)
{
    ADD_TEST(conversions);
    ADD_TEST(constructors);
    ADD_TEST(time_arithmetic);
    ADD_TEST(string_compare);
    ADD_TEST(huge_date);
    ADD_TEST(TZ_misc);
    ADD_TEST(TZ_wrappers);
    ADD_TEST(zone_leaps);
}
TCASEFINISH

#endif // XP_TESTING

