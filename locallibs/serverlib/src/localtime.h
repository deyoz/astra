#pragma once

#include <vector>
#include <string>

namespace boost {
    namespace posix_time {
        class ptime;
        class time_duration;
    }
    namespace gregorian {
        class date;
    }
}

/* Класс LocalTime предоставляет набор необходимых методов и
 * статических функций для конвертации времен между временными зонами.
 * Класс умеет работать со следующими форматами времени:
 * 1) boost::posix_time::ptime  
 * 2) сишная строка формата YYYYMMDDHHMMSS (14 символов)
 * 3) int64_t, количество секунд с 01.01.1970 0:00:00 (если время
 *    локальное, то количество секунд с 01.01.1970 0:00:00, которое
 *    прошло бы в GMT, если бы там было такое время на часах)
 *    
 * Также класс содержит ряд статических функций для конвертации разных
 * форматов времени друг в друга.
 */

/***********************************************************
  EXAMPLES:
1) Дано московское время, найти время в Берлине:
   int berlin_time = LocalTime(moscow_time,"Europe/Moscow").setZone("Europe/Berlin").getLocalTime();
   или так:
   boost::posix_time::ptime berlin_time = LocalTime(moscow_time,"Europe/Moscow")
                                            .setZone("Europe/Berlin").getBoostLocalTime();

2) Дано время отправления по времени Токио и время прибытия по времени Сиднея, найти время в пути:
   LocalTime tokyo(tokyo_time,"Asia/Tokyo");
   LocalTime sydney(sydney_time,"Australia/Sydney");
   int travel_time = sydney - tokyo;
   
***********************************************************/

/* В связи с переводом часов, во-первых, не всегда можно локальному
   времени поставить в соответствие только одно время по GMT
   (например, локальное время 2:30 встречается дважды, если часы в
   3:00 переводят на час назад) и не каждое локальное время вообще
   возможно (например, невозможно время 2:30 если часы в 2:00 были
   переведены на час вперед).

   Если время переводят на час вперед в 2:00:00, то после 2:00:00 идет
   3:00:01, а локальное время в промежутке [2:00:01, 3:00:00] невозможно.

   Если время переводят на час назад в 3:00:00, то после 3:00:00 идет
   2:00:01, а любое локальное время в промежутке [2:00:01, 3:00:00] 
   может соответствовать двум временам по GMT.

   В неоднозначных ситуациях выбрано следующее поведение:

   пусть 28.03.2010 в 2:00:00 время переводят на час вперед, т.е. до
   3:00:00 (т. е. gmtoff сдвигается с +3 до +4 в 23:00:00 27.03 по GMT)

   **local --> GMT**
   28.03 2:00:00 --> 27.03 23:00:00 (GMT+3) // в 2:00:00 время еще старое
   28.03 3:00:01 --> 27.03 23:00:01 (GMT+4) // а через секунду -- уже новое

   // несмотря на невозможность времен [2:00:01, 3:00:00], такие
   // входные данные обрабатываются. Например, некорректное время 2:05 -- это
   // время, которое на 5 минут больше, чем абсолютно корректное 2:00.
   28.03 2:00:01 --> 27.03 23:00:01 (GMT+3)
   28.03 3:00:00 --> 28.03  0:00:00 (GMT+3) // !!! 3:00:00 почти на час *больше*, чем 3:00:01 !!!


   пусть 31.10.2010 в 3:00:00 время переводят на час назад, т.е. до 2:00:00
   (т. е. gmtoff сдвигается с +4 до +3 в 23:00:00 30.10 по GMT)

   **local --> GMT**
   31.10 2:00:00 --> 30.10 22:00:00 (GMT+4) // в 2:00:00 время еще старое, а через секунду
                                            // будет новое и возникнет неоднозначность
   31.10 2:00:01 --> 30.10 23:00:01 (GMT+3) // при неоднозначности выбирается время *после* перевода стрелок
   31.10 3:00:01 --> 31.10  0:00:01 (GMT+3) // здесь неоднозначности уже нет

   **GMT  --> local**
   30.10 22:59:59 --> 31.10 2:59:59 (GMT+4) // setLocalTime(31.10 2:59:59) не даст 30.10 22:59:59
   30.10 23:00:00 --> 31.10 3:00:00 (GMT+4) // аналогично
   30.10 23:00:01 --> 31.10 2:00:01 (GMT+3)
*/

class TimeZone;
bool checkTimeZone(const std::string& zone);

class LocalTime
{
    LocalTime(int64_t a, const TimeZone * p) : zone(p), gmt(a) {}; 
private:
    const TimeZone * zone;
    int64_t gmt; // время в UTC/GMT
public: 
    // Конструкторы принимают в качестве аргумента локальное время и
    // строку-идентификатор временной зоны
    // если строка не указана, предполагается, что имеется в виду GMT
    LocalTime(const int64_t time = 0);
    LocalTime(const boost::posix_time::ptime &);
    LocalTime(const char * YYYYMMDDHHMMSS);
    LocalTime(const std::string& YYYYMMDDHHMMSS);
    LocalTime(const int64_t, const char * tzStr);

    LocalTime(const int64_t, const std::string& tzStr);
    LocalTime(const boost::posix_time::ptime & time, const char * tzStr);
    LocalTime(const boost::posix_time::ptime& time, const std::string& tzStr);

    LocalTime(const char * YYYYMMDDHHMMSS, const char * s);
    LocalTime(const std::string& YYYYMMDDHHMMSS, const std::string& s);

    // разница между двумя датами в секундах
    friend int64_t operator-(const LocalTime &, const LocalTime &); 

    // уменьшить дату на заданное количество секунд или на boost::posix_time::time_duration
    LocalTime operator-(const boost::posix_time::time_duration &) const;
    LocalTime operator-(const int64_t) const;
    LocalTime & operator-=(const boost::posix_time::time_duration &);
    LocalTime & operator-=(const int64_t);

    // увеличить дату на заданное количество секунд или на boost::posix_time::time_duration
    LocalTime operator+(const boost::posix_time::time_duration &) const;
    LocalTime operator+(const int64_t) const; 
    LocalTime & operator+=(const boost::posix_time::time_duration &);
    LocalTime & operator+=(const int64_t);

    friend bool operator<(const LocalTime &, const LocalTime &); 
    friend bool operator==(const LocalTime &, const LocalTime &); 

    // изменение временной зоны (setZone) не влияет на хранимое внутри время,
    // а влияет *только* на поведение методов семейства s
    LocalTime & setZone(const char *);
    LocalTime & setZone(const std::string&);

    LocalTime & setLocalTime(const boost::posix_time::ptime &);
    LocalTime & setLocalTime(const char * YYYYMMDDHHMMSS);
    LocalTime & setLocalTime(const std::string& YYYYMMDDHHMMSS);
    LocalTime & setLocalTime(const int64_t);
    LocalTime & setGMTime(const boost::posix_time::ptime &);
    LocalTime & setGMTime(const char * YYYYMMDDHHMMSS);
    LocalTime & setGMTime(const std::string& YYYYMMDDHHMMSS);
    LocalTime & setGMTime(const int64_t);

    boost::posix_time::ptime getBoostLocalTime() const;
    int64_t getLocalTime() const;
    std::string getLocalTimeStr() const; // пишет время в виде YYYYMMDDHHMMSS
    boost::posix_time::ptime getBoostGMTime() const; 
    int64_t getGMTime() const { return gmt; }
    std::string getGMTimeStr() const;// пишет время в виде YYYYMMDDHHMMSS
};

// функции, конвертирующие разные форматы представления времени друг в друга
// естественно, что при этом они ни коим образом не учитывают никакие временные зоны
int64_t strToTime(const char * YYYYMMDDHHMMSS);
int64_t strToTime(const std::string& YYYYMMDDHHMMSS);
int64_t boostToTime(const boost::posix_time::ptime &);
std::string timeToStr(int64_t time); // пишет время в виде YYYYMMDDHHMMSS
boost::posix_time::ptime timeToBoost(int64_t time);

std::string City2City(const std::string& from_time, const std::string& from_zone,const std::string& to_zone);
boost::posix_time::ptime City2City(boost::posix_time::ptime const &from_time, const std::string& from_zone,const std::string& to_zone);
std::string City2GMT(const std::string& time, const std::string& zone);
boost::posix_time::ptime City2GMT(const boost::posix_time::ptime& time, const std::string& zone);
std::vector<boost::gregorian::date> getZoneLeapDates(const std::string& zone,
        const boost::gregorian::date& start, const boost::gregorian::date& end, bool inUTC = true);

class TimeZoneCacheController
{
public:
    virtual ~TimeZoneCacheController();

    virtual bool needRefresh();
    virtual const std::string& tzdir();
};

void setupTimeZoneController(TimeZoneCacheController*);
