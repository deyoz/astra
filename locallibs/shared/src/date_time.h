#ifndef DATE_TIME_H
#define DATE_TIME_H

#include "misc.h"
#include "limits.h"
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/optional.hpp>

namespace BASIC {
    namespace date_time {

        typedef double TDateTime;

        const int DateDelphiDelta = 693594; // разница м/у   1/1/0001 и 12/31/1899
        const int MSecsPerDay = 24*60*60*1000;
        const int SecsPerDay = 24*60*60;
        extern const int  MonthDays[ 2 ][ 13 ];
        extern const char* ShortMonthNames[13];
        extern const char* ShortMonthNamesLat[13];

        extern const char ServerFormatDateTimeAsString[ 22 ];

        int IsLeapYear( int Year );
        void EncodeDate( int Year, int Month, int Day, TDateTime &VDate );
        void EncodeTime( int Hour, int Minute, int Second, TDateTime &VTime );
        void EncodeTime( int Hour, int Minute, int Second, int MSec, TDateTime &VTime );
        int DoEncodeDate( int Year, int Month, int Day, TDateTime &VDate );
        int DoEncodeTime( int Hour, int Minute, int Second, int MSec, TDateTime &VTime );
        void DecodeDate( TDateTime VDate, int &Year, int &Month, int &Day );
        void DecodeTime( TDateTime VDate, int &Hour, int &Min, int &Sec );
        boost::posix_time::ptime DateTimeToBoost( TDateTime VDateTime );

        TDateTime BoostToDateTime( const boost::posix_time::ptime &VDateTime );
        TDateTime BoostToDateTime( const boost::gregorian::date &d1 );

        TDateTime IncMonth(const TDateTime Date, int NumberOfMonths);
        int DayOfWeek(const TDateTime Date);
        int StrToDateTime( const char *StrDateTime, TDateTime &VDate );
        char* DateTimeToStr(TDateTime Value,  const char *format, char *buf, int pr_lat = 0);
        std::string DateTimeToStr(TDateTime Value, const std::string format, int pr_lat = 0);
        std::string DateTimeToStr( TDateTime Value, int pr_lat = 0 );
        int StrToDateTime(const char *buf, const char *format, TDateTime& Value, bool pr_lat = false );
        int StrToDateTime(const char *buf, const char *format, TDateTime upper_limit, TDateTime& Value, bool pr_lat = false );
        int StrToMonth(const char *buf, int &Month, bool pr_lat = false );

        enum TypeOfForcedConversion { BackwardWhenProblem, ForwardWhenProblem };

        // Time conversion functions
        TDateTime UTCToLocal(TDateTime date, const std::string& region);
        TDateTime LocalToUTC(TDateTime date, const std::string& region, int is_dst = INT_MIN);
        TDateTime LocalToUTC(TDateTime date, const std::string& region, TypeOfForcedConversion type);

        boost::gregorian::date UTCToLocal(const boost::gregorian::date& date, const std::string& region);

        // Offset from UTC
        int getDateOffsetMSec(TDateTime utcDate, const std::string& region);
        int getDateOffsetHour(TDateTime utcDate, const std::string& region);

        // Разница смещений от UTC для двух дат
        TDateTime getDatesOffsetsDiff(TDateTime first_day, TDateTime curr_day, const std::string& region);

        // Now functions
        TDateTime Now(const std::string& zone = std::string());
        TDateTime NowUTC();

        int Year(TDateTime dt);
        TDateTime setYear(TDateTime dt, int year);
        TDateTime LastSunday(int year, int month);

        // For debug
        std::string getTZDataVersion();

        boost::posix_time::ptime boostDateTimeFromAstraFormatStr(const std::string& str);
        std::string boostDateToAstraFormatStr(const boost::gregorian::date& d);

        class JulianDate
        {
          public:
            enum TDirection { before, after, everywhere };

          private:
            TDateTime _date;
            int _julian_date;
            int _year_last_digit;
            bool toDateTime(int julian_date, int year, TDateTime &date) const;
            std::string directionStr(TDirection direction) const;
            std::string dateTimeStr(TDateTime dateTime) const;
            void set(int julian_date,
                     const boost::optional<int> &year_last_digit,
                     TDateTime time_point,
                     TDirection direction);
            void set(TDateTime date);
          public:
            JulianDate(TDateTime date);
            JulianDate(int julian_date,
                       TDateTime time_point,
                       TDirection direction);
            JulianDate(int julian_date,
                       int year_last_digit,
                       TDateTime time_point,
                       TDirection direction);
            int getYearLastDigit() const { return _year_last_digit; }
            int getJulianDate() const { return _julian_date; }
            TDateTime getDateTime() const { return _date; }
            void trace( const std::string& where ) const;
        };
    }
}

#endif // DATE_TIME_H
