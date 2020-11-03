
#include <string>
#include <memory> // auto_ptr
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <time.h>

#include "exceptions.h"
#include "memory_manager.h"
#include <serverlib/dates_io.h>

#include "unicode/timezone.h"
#include "unicode/gregocal.h"
#include <boost/date_time.hpp>

#include "date_time.h"
#include "misc.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

using namespace std;

namespace BASIC {
    namespace date_time {

    using namespace EXCEPTIONS;

        namespace {
            // Заставляем примениться опцию
            // Изменение опций (set Skipped/Repeated WallTImeOption) применяется
            // только после изменения поля времени
            //
            inline void force_update(Calendar& cal, bool _get = false) {
                cal.set(UCAL_MILLISECOND, 0);
                if (!_get) return;

                UErrorCode success = U_ZERO_ERROR;
                cal.get(UCAL_MILLISECOND, success);
            }

            void DateTime2Calendar(TDateTime dt, Calendar& cal) {
                int y, m, d, hh, mm, ss;
                DecodeDate(dt, y, m, d);
                DecodeTime(dt, hh, mm, ss);
                cal.set(y, m - 1, d, hh, mm, ss);
            }

            TDateTime Calendar2DateTime(Calendar& cal) {
                UErrorCode success = U_ZERO_ERROR;
                int year = cal.get(UCAL_YEAR, success);
                int month = cal.get(UCAL_MONTH, success) + 1;
                int day = cal.get(UCAL_DATE, success);

                int hour = cal.get(UCAL_HOUR_OF_DAY, success);
                int minute = cal.get(UCAL_MINUTE, success);
                int second = cal.get(UCAL_SECOND, success);
                int millisec = cal.get(UCAL_MILLISECOND, success);

                TDateTime date, time;
                EncodeDate(year, month, day, date);
                EncodeTime(hour, minute, second, millisec, time);

                return date + time;
            }

            int32_t getOffset(Calendar& cal, int32_t& gmt_offset, int32_t& dst_offset) {
                UErrorCode success = U_ZERO_ERROR;

                force_update(cal);
                cal.getTimeZone().getOffset(cal.getTime(success), false, gmt_offset, dst_offset, success);
                return gmt_offset + dst_offset;
            }

            bool isSkippedTime(Calendar& cal) {
                int32_t gmt1, gmt2, dst1, dst2;

                cal.setSkippedWallTimeOption(UCAL_WALLTIME_FIRST);
                getOffset(cal, gmt1, dst1);

                cal.setSkippedWallTimeOption(UCAL_WALLTIME_LAST);
                getOffset(cal, gmt2, dst2);

                return !(gmt1 == gmt2 && dst1 == dst2);
            }

            bool isRepeatedTime(Calendar& cal) {
                int32_t gmt1, gmt2, dst1, dst2;

                cal.setRepeatedWallTimeOption(UCAL_WALLTIME_FIRST);
                getOffset(cal, gmt1, dst1);

                cal.setRepeatedWallTimeOption(UCAL_WALLTIME_LAST);
                getOffset(cal, gmt2, dst2);

                return !(gmt1 == gmt2 && dst1 == dst2);
            }

            void setDst(Calendar& cal, int isDst) {
                switch (isDst) {
                case 0:
                    //cal.setSkippedWallTimeOption(UCAL_WALLTIME_FIRST);
                    cal.setRepeatedWallTimeOption(UCAL_WALLTIME_LAST);
                    break;
                case 1:
                    //cal.setSkippedWallTimeOption(UCAL_WALLTIME_LAST);
                    cal.setRepeatedWallTimeOption(UCAL_WALLTIME_FIRST);
                    break;
                default:
                    return;
                }
                force_update(cal, true);
            }
        }

        TDateTime LocalToUTCInner(TDateTime dt, const std::string& region, int isDst = INT_MIN) {
            static std::auto_ptr<TimeZone> TimeZoneUTC(TimeZone::createTimeZone("Etc/GMT"));

            if (region.empty())
                throw EXCEPTIONS::Exception("Region not specified");

            UErrorCode success = U_ZERO_ERROR;
            TimeZone *tz = TimeZone::createTimeZone(region.c_str());
            // Календарь управляет временем жизни tz
            GregorianCalendar gc(tz, success);
            DateTime2Calendar(dt, gc);

            if (isSkippedTime(gc))
                throw boost::local_time::time_label_invalid(); // invalid time

            if (INT_MIN != isDst)
                setDst(gc, isDst);
            else
                if (isRepeatedTime(gc))
                    throw boost::local_time::ambiguous_result(); // ambiguous time

            gc.setTimeZone(*TimeZoneUTC);

            return Calendar2DateTime(gc);
        }

        TDateTime LocalToUTC(TDateTime dt, const std::string& region, TypeOfForcedConversion type)
        {
          int direction = (type==BackwardWhenProblem?-1:1);
          for(int i=0; i<=48; i++)
            try
            {
              TDateTime utc=LocalToUTCInner(dt+direction*i/48.0, region);
              return utc;
            }
            catch(const boost::local_time::time_label_invalid&)
            {
              //следующая итерация
            }
            catch(const boost::local_time::ambiguous_result&)
            {
              TDateTime utc1=LocalToUTC(dt, region, 0);
              TDateTime utc2=LocalToUTC(dt, region, 1);
              if (utc1<utc2)
                return type==BackwardWhenProblem?utc1:utc2;
              else
                return type==BackwardWhenProblem?utc2:utc1;
            }

          //вышли из цикла и не нашли UTC :(
          return LocalToUTC(dt, region);
        }



        TDateTime LocalToUTC(TDateTime dt, const std::string& region, int isDst) {

          try
          {
            TDateTime utc=LocalToUTCInner(dt, region, isDst);
            return utc;
          }
          catch(const boost::local_time::time_label_invalid&)
          {
            throw EXCEPTIONS::Exception("%s: invalid time (%s : %s)",
                                        __func__,
                                        DateTimeToStr(dt, "dd.mm.yyyy hh:nn").c_str(),
                                        region.c_str());
          }
          catch(const boost::local_time::ambiguous_result&)
          {
            throw EXCEPTIONS::Exception("%s: ambiguous time (%s : %s)",
                                        __func__,
                                        DateTimeToStr(dt, "dd.mm.yyyy hh:nn").c_str(),
                                        region.c_str());
          }
        }

        TDateTime UTCToLocal(TDateTime dt, const std::string& region) {

            TimeZone* TimeZoneUTC = TimeZone::createTimeZone("Etc/GMT");

            if (region.empty())
                throw EXCEPTIONS::Exception("Region not specified");

            UErrorCode success = U_ZERO_ERROR;
            std::auto_ptr<TimeZone> region_tz(TimeZone::createTimeZone(region.c_str()));
            // Календарь управляет временем жизни TimeZoneUTC
            GregorianCalendar gc(TimeZoneUTC, success);
            DateTime2Calendar(dt, gc);

            force_update(gc, true);
            gc.setTimeZone(*region_tz);

            return Calendar2DateTime(gc);
        }

        boost::gregorian::date UTCToLocal(const boost::gregorian::date& dateUtc,
                                          const std::string& region)
        {
            if(dateUtc.is_special()) return dateUtc;

            TDateTime dtUtc = BoostToDateTime(dateUtc);
            TDateTime dtLocal = UTCToLocal(dtUtc, region);
            return DateTimeToBoost(dtLocal).date();
        }

        // Получаем смещение актуальное на дату utcDate
        // в регионе region
        int getDateOffsetMSec(TDateTime utcDate, const std::string& region) {

            if (region.empty())
                throw EXCEPTIONS::Exception("Region not specified");

            UErrorCode success = U_ZERO_ERROR;

            GregorianCalendar gc(TimeZone::createTimeZone("Etc/GMT"), success);
            DateTime2Calendar(utcDate, gc);

            int32_t rawOffset, dstOffset;
            TimeZone* region_tz = TimeZone::createTimeZone(region.c_str());
            region_tz->getOffset(gc.getTime(success), false, rawOffset, dstOffset, success);
            delete region_tz;

            return rawOffset + dstOffset;
        }

        int getDateOffsetHour(TDateTime utcDate, const std::string& region) {
            return getDateOffsetMSec(utcDate, region) / (3600 * 1000);
        }

        TDateTime getDatesOffsetsDiff(TDateTime first_day, TDateTime curr_day, const std::string &region)
        {
            TDateTime res = 0;
            int diff = getDateOffsetMSec(first_day, region) -
                getDateOffsetMSec(curr_day, region);

            if (!diff)
                return res;

            bool sign = diff < 0;
            diff = sign ? -diff : diff;

            int total_sec = diff / 1000;
            //int s = total_sec % 60;
            int m = (total_sec / 60) % 60;
            int h = (total_sec / 3600) % 24;

            EncodeTime(h, m, 0, 0, res);

            return sign ? -res : res;
        }

        TDateTime Now(const std::string& zone) {
            UErrorCode success = U_ZERO_ERROR;

            TDateTime res;
            if (zone.empty()) {
                GregorianCalendar gc(success);
                res = Calendar2DateTime(gc);
            }
            else {
                GregorianCalendar gc(TimeZone::createTimeZone(zone.c_str()), success);
                res = Calendar2DateTime(gc);
            }

            return U_SUCCESS(success) ? res : INT_MIN;
        }

        TDateTime NowUTC() {
            return Now("Etc/GMT");
        }

        int Year(TDateTime dt) {
            int y, m, d;
            DecodeDate(dt, y, m, d);
            return y;
        }

        TDateTime setYear(TDateTime dt, int year) {
            int y, m, d;
            TDateTime date, stub;

            DecodeDate(dt, y, m, d);
            EncodeDate(year, m, d, date);
            return date + modf(dt, &stub);
        }

        TDateTime LastSunday(int year, int month) {
            UErrorCode success = U_ZERO_ERROR;
            GregorianCalendar gc(year, month - 1, 1, success);
            gc.set(UCAL_DAY_OF_WEEK, UCAL_SUNDAY);
            gc.set(UCAL_DAY_OF_WEEK_IN_MONTH, -1);

            return Calendar2DateTime(gc);
        }

        std::string getTZDataVersion() {
            UErrorCode success = U_ZERO_ERROR;
            return TimeZone::getTZDataVersion(success);
        }

        const int  MonthDays[2][13] =
            { { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },  \
              { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 } };

        const char* ShortMonthNames[13] =
            { "","ЯНВ","ФЕВ","МАР","АПР","МАЙ","ИЮН","ИЮЛ","АВГ","СЕН","ОКТ","НОЯ","ДЕК" };
        const char* ShortMonthNamesLat[13] =
            { "","JAN","FEB","MAR","APR","MAY","JUN","JUL","AUG","SEP","OCT","NOV","DEC" };

        const char ServerFormatDateTimeAsString[22] = "dd.mm.yyyy hh:nn:ss";

        char const SDateEncodeError[] = "Invalid argument to date encode";
        char const STimeEncodeError[] = "Invalid argument to time encode";
        char const SStrToIntError[] = "Invalid argument to int convert";

        int IsLeapYear(int Year)
        {
            return ((!(Year % 4)) && ((Year % 100) || (!(Year % 400))));
        }

        int DoEncodeDate(int Year, int Month, int Day, TDateTime &VDate)
        {
            const int *TableDays;
            TableDays = MonthDays[IsLeapYear(Year)];
            int Result = EOF;
            if ((Year >= 1) && (Year <= 9999) && (Month >= 1) && (Month <= 12) && \
                (Day >= 1) && (Day <= TableDays[Month]))
            {
                for (int i = 1; i < Month; i++)
                    Day = Day + TableDays[i];

                Year--;
                VDate = Year * 365 + Year / 4 - Year / 100 + Year / 400 + Day - DateDelphiDelta;
                Result = 0;
            }

            return Result;
        }

        void EncodeDate(int Year, int Month, int Day, TDateTime &VDate)
        {
            if (DoEncodeDate(Year, Month, Day, VDate))
                throw EConvertError((char*)SDateEncodeError);
            return;
        }

        int DoEncodeTime(int Hour, int Minute, int Second, int MSec, TDateTime &VTime)
        {
            int Result = EOF;
            if ((Hour < 24) && (Hour >= 0) && (Minute < 60) && (Minute >= 0) && (Second < 60) && (Second >= 0) && (MSec < 1000) && (MSec >= 0))
            {
                VTime = ((double)Hour*3600000.0 + (double)Minute*60000.0 + (double)Second*1000.0 + (double)MSec) / (double)MSecsPerDay;
                Result = 0;
            }

            return Result;
        }

        void EncodeTime(int Hour, int Minute, int Second, TDateTime &VTime)
        {
            if (DoEncodeTime(Hour, Minute, Second, 0, VTime))
                throw EConvertError((char*)STimeEncodeError);
            return;
        }

        void EncodeTime(int Hour, int Minute, int Second, int MSec, TDateTime &VTime)
        {
            if (DoEncodeTime(Hour, Minute, Second, MSec, VTime))
                throw EConvertError((char*)STimeEncodeError);
            return;
        }

        void DecodeDate(TDateTime VDate, int &Year, int &Month, int &Day)
        {
            int SS = (int)floor(fabs(modf(VDate, &VDate)*(double)MSecsPerDay) + 0.5); //кол-во миллисекунд
                                                                                      // патамушта modf вносить свой погрешност 24:00 - день
            if (SS == MSecsPerDay) VDate += 1.0; //!!!

            int D1 = 365;
            int D4 = D1 * 4 + 1;
            int D100 = D4 * 25 - 1;
            int D400 = D100 * 4 + 1;
            const int *TableDays;
            int I;

            //error
            Day = (int)VDate - 1 + DateDelphiDelta;
            if (Day < 0)
            {
                Day = 0;
                Month = 0;
                Year = 0;
            }
            else
            {
                Year = 1; // начинается отсчет с 01.01.01
                I = Day / D400; // определяем целое число 4-х-сотлетий
                Day -= I*D400;
                Year += I * 400; // кол-во лет
                I = Day / D100; // определяем целое число 100-летий

                if (I == 4) I--; // нет полных 4 столетия, полные только 3

                Day -= I*D100;
                Year += I * 100;
                I = Day / D4; // при делении на 4 можно не делать проверку на целых 25 4-хлетия
                Day -= I*D4; // т.к. 25*D4 > D100
                Year += I * 4;
                I = Day / D1; // определяем целое число лет

                if (I == 4) I--; // получили 4 года, но не хватает на 4-хлетие

                Year += I; // кол-во лет с начала
                Day -= I*D1; // остаток дней меньше D1
                TableDays = MonthDays[IsLeapYear(Year)];
                Month = 0;

                while (1)
                {
                    I = TableDays[Month];
                    if (Day < I) break;
                    Day -= I;
                    Month++;
                }

                Day++;
            }
        }

        void DecodeTime(TDateTime VDate, int &Hour, int &Min, int &Sec)
        {
            int SS = (int)floor(fabs(modf(VDate, &VDate)*(double)MSecsPerDay) + 0.5); //кол-во миллисекунд
                                                                                      // патамушта modf вносить свой погрешность 24:00 - день
            if (SS == MSecsPerDay) SS = 0;
            Sec = (int)SS / 1000;
            Hour = (int)(Sec / (60.0*60.0));
            Sec -= Hour * 60 * 60;
            Min = (int)(Sec / (60.0));
            Sec -= Min * 60;
        }

        boost::posix_time::ptime DateTimeToBoost(TDateTime VDateTime)
        {
            int Year, Month, Day;
            DecodeDate(VDateTime, Year, Month, Day);
            int Hour, Min, Sec;
            DecodeTime(VDateTime, Hour, Min, Sec);
            return boost::posix_time::ptime(boost::gregorian::date(Year, Month, Day),
                boost::posix_time::time_duration(Hour, Min, Sec));
        }

        TDateTime BoostToDateTime(const boost::posix_time::ptime &VDateTime)
        {
            TDateTime d2, t2;
            boost::gregorian::date d1(VDateTime.date());
            EncodeDate(d1.year(), d1.month(), d1.day(), d2);
            boost::posix_time::time_duration t1(VDateTime.time_of_day());
            EncodeTime(t1.hours(), t1.minutes(), t1.seconds(),
                t1.fractional_seconds() / (boost::posix_time::time_duration::ticks_per_second() / 1000), t2);
            return d2 + t2;
        }

        TDateTime BoostToDateTime(const boost::gregorian::date &d1)
        {
            TDateTime d2;
            EncodeDate(d1.year(), d1.month(), d1.day(), d2);
            return d2;
        }

        void ReplaceTime(TDateTime &DateTime, const TDateTime NewTime)
        {
            double ipart;
            modf(DateTime, &DateTime);
            if (DateTime >= 0)
                DateTime += fabs(modf(NewTime, &ipart));
            else
                DateTime -= fabs(modf(NewTime, &ipart));
        }

        TDateTime IncMonth(const TDateTime Date, int NumberOfMonths)
        {
            const int *TableDays;
            int Year, Month, Day, Sign;
            TDateTime Result;

            if (NumberOfMonths >= 0)
                Sign = 1;
            else
                Sign = -1;

            DecodeDate(Date, Year, Month, Day);
            Year = Year + (NumberOfMonths / 12);
            NumberOfMonths = NumberOfMonths % 12;
            Month += NumberOfMonths;

            if (Month < 1 || Month > 12)
            {
                Year += Sign;
                Month += -12 * Sign;
            }

            TableDays = MonthDays[IsLeapYear(Year)];
            if (Day > TableDays[Month])
                Day = TableDays[Month];
            DoEncodeDate(Year, Month, Day, Result);
            ReplaceTime(Result, Date);
            return Result;
        }

        int DayOfWeek(const TDateTime Date)
        {
            int Year, Month, Day;
            DecodeDate(Date, Year, Month, Day);
            boost::gregorian::date d(Year, Month, Day);
            int wday = d.day_of_week();

            if (wday == 0) wday = 7;
            return wday;
        }

        int StrToDateTime(const char *StrDateTime, TDateTime &VDate)
        {
            /* FormatStr = 'DD.MM.YYYY HH24:MI:SS' */
            int Temp, Year = -1, Month = -1, Day = -1, Hour = -1, Minute = -1, Second = -1;
            char *charstop;
            char *strvalue;
            int enddate = 0;
            TDateTime VTime = 0.0;
            VDate = 0.0;

            if (StrDateTime == NULL) return EOF;

            TMemoryManager mem(STDLOG);
            strvalue = (char*)mem.malloc(strlen(StrDateTime) + 1, STDLOG);

            if (strvalue == NULL)
                EMemoryError("Can't allocate memory");

            try {
                strcpy(strvalue, StrDateTime);
                Trim(strvalue);
                char *p = strvalue;

                while (*p != '\0')
                {
                    errno = 0;
                    Temp = strtol(p, &charstop, 10);

                    if (errno == ERANGE) break;

                    if ((*charstop == '.') || (*charstop == ' ') || ((*charstop == '\0') && (!enddate)))
                    {
                        if (Year != -1)
                            break;
                        else if (Month != -1)
                            Year = Temp;
                        else if (Day != -1)
                            Month = Temp;
                        else
                            Day = Temp;

                        if (*charstop == ' ') enddate = 1;
                    }

                    if ((*charstop == ':') || ((*charstop == '\0') && (enddate)))
                    {
                        enddate = 1;
                        if (Second != -1)
                            break;
                        else if (Minute != -1)
                            Second = Temp;
                        else if (Hour != -1)
                            Minute = Temp;
                        else
                            Hour = Temp;
                    }

                    if ((*charstop != '.') && (*charstop != ' ') && (*charstop != ':'))
                        break;

                    p = strchr(p, *charstop) + 1;
                }
            }
            catch (...) {
                mem.free(strvalue, STDLOG);
                throw;
            }

            if (*charstop != 0)
            {
                mem.free(strvalue, STDLOG);
                return EOF;
            }

            mem.free(strvalue, STDLOG);

            try {
                if (Year == -1)     Year = 1;
                if (Month == -1)    Month = 1;
                if (Day == -1)      Day = 1;
                if (Hour == -1)     Hour = 0;
                if (Minute == -1)   Minute = 0;
                if (Second == -1)   Second = 0;

                EncodeDate(Year, Month, Day, VDate);
                EncodeTime(Hour, Minute, Second, VTime);

                if (VDate < 0)
                    VDate -= VTime;
                else
                    VDate += VTime;

                return 0;
            }
            catch (...) {
                return EOF;
            }
        }

        char* DateTimeToStr(TDateTime Value, const char *format, char *buf, int pr_lat)
        {
            char str[5], *p, *pfmt;
            bool pr_quote = false;
            int YearNow, Year, Month, Day, Hour, Min, Sec;

            if (buf == NULL) return NULL;

            DecodeDate(Value, YearNow, Month, Day);
            DecodeTime(Value, Hour, Min, Sec);

            for (pfmt = (char*)format, p = (char*)buf; pfmt != NULL&&*pfmt != 0;)
            {
                if (!pr_quote)
                {
                    switch (*pfmt)
                    {
                    case 'y':
                        Year = YearNow;
                        if (strstr(pfmt, "yyyy") == pfmt)
                        {
                            sprintf(str, "%04d", Year);
                            strcpy(p, str);
                            pfmt += 4;
                            p += strlen(str);
                            break;
                        }
                        Year = YearNow % 100;

                    case 'm':
                        if (strstr(pfmt, "mmm") == pfmt)
                        {
                            sprintf(str, "%s", (pr_lat ? ShortMonthNamesLat[Month] : ShortMonthNames[Month]));
                            strcpy(p, str);
                            pfmt += 3;
                            p += strlen(str);
                            break;
                        }

                    case 'd':
                    case 'h':
                    case 'n':
                    case 's':
                        int *pint;
                        switch (*pfmt)
                        {
                        case 'y': pint = &Year;  break;
                        case 'm': pint = &Month; break;
                        case 'd': pint = &Day;   break;
                        case 'h': pint = &Hour;  break;
                        case 'n': pint = &Min;   break;
                        default: pint = &Sec;   break;
                        }

                        if (*(pfmt + 1) == *pfmt)
                        {
                            sprintf(str, "%02d", *pint);
                            strcpy(p, str);
                            pfmt += 2;
                        }
                        else
                        {
                            sprintf(str, "%d", *pint);
                            strcpy(p, str);
                            pfmt++;
                        }

                        p += strlen(str);
                        break;

                    case '\'':
                        if (*(pfmt + 1) == *pfmt)
                        {
                            *p = *pfmt;
                            pfmt += 2;
                            p++;
                        }
                        else
                        {
                            pfmt++;
                            pr_quote = true;
                        }
                        break;

                    default:
                        *p = *pfmt;
                        pfmt++;
                        p++;
                    }
                }
                else
                {
                    if (*pfmt != '\'')
                    {
                        *p = *pfmt;
                        pfmt++;
                        p++;
                    }
                    else
                    {
                        pfmt++;
                        pr_quote = false;
                    }
                }
            }

            *p = 0;
            return buf;
        }

        std::string DateTimeToStr(TDateTime Value, const std::string format, int pr_lat)
        {
            TMemoryManager mem(STDLOG);
            char *buf = (char*)mem.malloc(format.size() * 2 + 1, STDLOG);

            if (buf == NULL)
                throw EMemoryError("Can't allocate memory");

            DateTimeToStr(Value, format.c_str(), buf, pr_lat);

            std::string result = buf;
            mem.free(buf, STDLOG);
            return result;
        }

        std::string DateTimeToStr(TDateTime Value, int pr_lat) {
            return DateTimeToStr(Value, ServerFormatDateTimeAsString, pr_lat);
        }

        int StrToDateTime(const char *buf, const char *format, TDateTime& Value, bool pr_lat)
        {
            TDateTime upper_limit = IncMonth(NowUTC(), 12 * 50); //+50 лет от текущей даты
            return StrToDateTime(buf, format, upper_limit, Value, pr_lat);
        }

        int StrToDateTime(const char *buf, const char *format, TDateTime upper_limit, TDateTime& Value, bool pr_lat)
        {
            TDateTime day_part, time_part;
            char str[5];
            char *p;
            char *pfmt;
            char *strvalue = NULL;

            bool pr_quote = false;

            int YearNow,
                Year,
                Month = 1,
                Day = 1,
                Hour = 0,
                Min = 0,
                Sec = 0,
                i;

            TMemoryManager mem(STDLOG);
            try
            {
                if (buf == NULL || format == NULL) throw false;

                strvalue = (char*)mem.malloc(strlen(buf) + 1, STDLOG);
                if (strvalue == NULL) EMemoryError("Can't allocate memory");

                strcpy(strvalue, buf);
                Trim(strvalue);

                time_t curr_time_t = time(NULL);
                struct tm *curr_time;
                curr_time = localtime(&curr_time_t);
                YearNow = curr_time->tm_year + 1900;
                Year = YearNow;
                bool check_limit = false;

                for (pfmt = (char*)format, p = strvalue; (*p != 0 || pr_quote) && *pfmt != 0;)
                {
                    if (!pr_quote)
                    {
                        switch (*pfmt)
                        {
                        case 'y':
                            if (strstr(pfmt, "yyyy") == pfmt)
                            {
                                if (sscanf(p, "%4[0-9]", str) == 0 || strlen(str) != 4)
                                    throw false;

                                Year = strtol(str, NULL, 10);
                                pfmt += 4;
                                p += strlen(str);
                                check_limit = false;
                                break;
                            }

                        case 'm':
                            if (strstr(pfmt, "mmm") == pfmt)
                            {
                                strncpy(str, p, 3);
                                str[3] = 0;

                                for (i = 1; i <= 12; i++)
                                    if ((!pr_lat && strcmp(str, ShortMonthNames[i]) == 0) ||
                                          (pr_lat && strcmp(str, ShortMonthNamesLat[i]) == 0))
                                        break;

                                if (!pr_lat && i > 12)
                                    if (strcmp("МАЯ", buf) == 0)
                                        i = 5;
                                    else
                                        throw false;

                                Month = i;
                                pfmt += 3;
                                p += strlen(str);

                                break;
                            }

                        case 'd':
                        case 'h':
                        case 'n':
                        case 's':
                            int *pint;
                            switch (*pfmt)
                            {
                            case 'y': pint = &Year;  break;
                            case 'm': pint = &Month; break;
                            case 'd': pint = &Day;   break;
                            case 'h': pint = &Hour;  break;
                            case 'n': pint = &Min;   break;
                            default: pint = &Sec;    break;
                            }

                            if (*(pfmt + 1) == *pfmt)
                            {
                                if (sscanf(p, "%2[0-9]", str) == 0 || strlen(str) != 2)
                                    throw false;

                                *pint = strtol(str, NULL, 10);
                                pfmt += 2;
                            }
                            else
                            {
                                if (sscanf(p, "%2[0-9]", str) == 0)
                                    throw false;

                                *pint = strtol(str, NULL, 10);
                                pfmt++;
                            }

                            p += strlen(str);

                            if (pint == &Year)
                                check_limit = true;
                            break;

                        case '\'':
                            if (*(pfmt + 1) == *pfmt)
                            {
                                if (*p != *pfmt) throw false;
                                pfmt += 2;
                                p++;
                            }
                            else
                            {
                                pfmt++;
                                pr_quote = true;
                            }
                            break;

                        default:
                            if (*p != *pfmt) throw false;
                            pfmt++;
                            p++;
                        }
                    }
                    else
                    {
                        if (*pfmt != '\'')
                        {
                            if (*p != *pfmt) throw false;
                            pfmt++;
                            p++;
                        }
                        else
                        {
                            pfmt++;
                            pr_quote = false;
                        }
                    }
                }

                if (*p != 0 || *pfmt != 0 || pr_quote) throw false;

                if (check_limit)
                {
                    if (Year < 0 || Year>99) throw false;
                    Year = (YearNow / 100) * 100 + Year;
                }

                if (DoEncodeTime(Hour, Min, Sec, 0, time_part)) throw false;

                for (int k = (check_limit ? 1 : 0); k >= -1; k--)
                {
                    //три прохода, если check_limit, иначе один проход
                    if (DoEncodeDate(Year + k * 100, Month, Day, day_part))
                    {
                        if (check_limit) continue; else throw false;
                    }

                    if (day_part >= 0)
                        Value = day_part + time_part;
                    else
                        Value = day_part - time_part;
                    if (!check_limit || Value <= upper_limit) throw true;
                }
            }
            catch (bool res)
            {
                if (strvalue != NULL) mem.free(strvalue, STDLOG);
                if (res)
                    return 0;
                else
                    return EOF;
            }
            catch (...)
            {
                if (strvalue != NULL) mem.free(strvalue, STDLOG);
                throw;
            }

            if (strvalue != NULL) mem.free(strvalue, STDLOG);
            return EOF;
        }

        int StrToMonth(const char *buf, int &Month, bool pr_lat)
        {
            if (buf == NULL) return EOF;

            for (int i = 1; i <= 12; ++i)
                if ((!pr_lat && strcmp(ShortMonthNames[i], buf) == 0) ||
                    (pr_lat && strcmp(ShortMonthNamesLat[i], buf) == 0))
                {
                    Month = i;
                    return 0;
                }

            if (!pr_lat && strcmp("МАЯ", buf) == 0)
            {
                Month = 5;
                return 0;
            }

            return EOF;
        }

        /* FormatStr = 'DD.MM.YYYY HH24:MI:SS' */
        boost::posix_time::ptime boostDateTimeFromAstraFormatStr(const std::string& str)
        {
            if(str.length() != 19) { // DD.MM.YYYY HH24:MI:SS
                LogWarning(STDLOG) << "Invalid date/time: " << str;
                return boost::posix_time::ptime();
            }
            TDateTime dt;
            if(StrToDateTime(str.c_str(), dt)) {
                LogWarning(STDLOG) << "Invalid date/time: " << str;
                return boost::posix_time::ptime();
            }
            return DateTimeToBoost(dt);
        }

        /* FormatStr = 'DD.MM.YYYY HH24:MI:SS' */
        std::string boostDateToAstraFormatStr(const boost::gregorian::date& d)
        {
            if(d.is_not_a_date()) {
                return "";
            }
            return HelpCpp::string_cast(d, "%d.%m.%Y 00:00:00");
        }

        //---------------------JulianDate---------------------//

        bool JulianDate::toDateTime(int julian_date, int year, TDateTime &date) const
        {
            try
            {
                TDateTime result;
                EncodeDate(year, 1, 1, result);
                result += julian_date - 1;

                int Year, Month, Day;
                DecodeDate(result, Year, Month, Day);

                if (year != Year)
                    throw EConvertError("wrong julian_date");

                date = result;
                return true;
            }
            catch (EConvertError &e)
            {
                return false;
            }
        }

        std::string JulianDate::directionStr(TDirection direction) const
        {
            switch (direction)
            {
            case before:    return "TDirection::before";
            case after:     return "TDirection::after";
            case everywhere:return "TDirection::everywhere";
            }
            return "";
        }

        std::string JulianDate::dateTimeStr(TDateTime dateTime) const
        {
            try
            {
                return DateTimeToStr(dateTime, "dd.mm.yy hh:nn:ss");
            }
            catch (EConvertError)
            {
                return "error";
            }
        }

        void JulianDate::set(TDateTime date)
        {
            int Year, Month, Day;
            DecodeDate(date, Year, Month, Day);

            TDateTime firstYearDate, truncatedDate;
            EncodeDate(Year, 1, 1, firstYearDate);
            EncodeDate(Year, Month, Day, truncatedDate);

            _date = date;
            _julian_date = (int)round(truncatedDate - firstYearDate) + 1;
            _year_last_digit = Year % 10;

        }

        void JulianDate::set(int julian_date,
            const boost::optional<int> &year_last_digit,
            TDateTime time_point,
            JulianDate::TDirection direction)
        {
            if (julian_date < 1 || julian_date>366)
                throw EConvertError("wrong julian_date");

            if (year_last_digit && (year_last_digit.get() < 0 || year_last_digit.get() > 9))
                throw EConvertError("wrong julian_date");

            int Year, Month, Day;
            DecodeDate(time_point, Year, Month, Day);

            TDateTime truncatedPoint;
            EncodeDate(Year, Month, Day, truncatedPoint);

            boost::optional<TDateTime> result;
            int lower_offset = 0, upper_offset = 0;

            if (year_last_digit)
            {
                //+-40 лет достаточно, чтобы гарантированно попасть на високосный год c последней цифрой 0
                if (direction == before || direction == everywhere)
                    lower_offset = (julian_date == 366 ? 40 : 10);

                if (direction == after || direction == everywhere)
                    upper_offset = (julian_date == 366 ? 40 : 10);
            }
            else
            {
                //+-8 лет достаточно, чтобы гарантированно попасть на високосный год
                if (direction == before || direction == everywhere)
                    lower_offset = (julian_date == 366 ? 8 : 1);

                if (direction == after || direction == everywhere)
                    upper_offset = (julian_date == 366 ? 8 : 1);
            }

            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

            for (int y = Year - lower_offset; y <= Year + upper_offset; ++y)
            {
                if (year_last_digit && y % 10 != year_last_digit.get()) continue;

                if (julian_date == 366 && !IsLeapYear(y)) continue;

                TDateTime d;
                if (!toDateTime(julian_date, y, d)) continue;

                if ((direction == before && d > truncatedPoint) || (direction == after && d < truncatedPoint))
                    continue;

                if (!result || fabs(result.get() - truncatedPoint) > fabs(d - truncatedPoint))
                    result = d;
            }

            if (!result) throw EConvertError("impossible");
            set(result.get());
            #pragma GCC diagnostic pop

            if (julian_date != _julian_date)
                throw EConvertError("internal JulianDate error! (julian_date!=_julian_date)");

            if (year_last_digit && year_last_digit.get() != _year_last_digit)
                throw EConvertError("internal JulianDate error! (year_last_digit!=_year_last_digit)");
        }

        JulianDate::JulianDate(TDateTime date)
        {
            try
            {
                set(date);
            }
            catch (EConvertError &e)
            {
                throw EConvertError("JulianDate::JulianDate(%s): %s",
                    dateTimeStr(date).c_str(),
                    e.what());
            }
        }

        JulianDate::JulianDate(int julian_date, TDateTime time_point, TDirection direction)
        {
            try
            {
                set(julian_date, boost::none, time_point, direction);
            }
            catch (EConvertError &e)
            {
                throw EConvertError("JulianDate::JulianDate(%d, %s, %s): %s",
                    julian_date,
                    dateTimeStr(time_point).c_str(),
                    directionStr(direction).c_str(),
                    e.what());
            }
        }

        JulianDate::JulianDate(int julian_date, int year_last_digit, TDateTime time_point, TDirection direction)
        {
            try {
                set(julian_date, year_last_digit, time_point, direction);
            }
            catch (EConvertError &e) {
                throw EConvertError("JulianDate::JulianDate(%d, %d, %s, %s): %s",
                    julian_date,
                    year_last_digit,
                    dateTimeStr(time_point).c_str(),
                    directionStr(direction).c_str(),
                    e.what());
            }
        }

        void JulianDate::trace(const std::string &where) const
        {
            ProgTrace(TRACE5,
                "%s: date=%s julian_date=%d year_last_digit=%d",
            where.c_str(),
                DateTimeToStr(_date, "dd.mm.yy hh:nn:ss").c_str(),
                _julian_date,
                _year_last_digit);
        }
    }
}
