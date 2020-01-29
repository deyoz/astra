#pragma once

#include "lngv.h"
#ifdef __cplusplus

#include <iosfwd>
#include <string>

namespace boost
{
namespace date_time
{
template <class date_type,
          class CharT,
          class OutItrT>
class date_facet;
} // date_time

namespace gregorian
{
class date;
class date_duration;

typedef boost::date_time::date_facet<date, char, std::ostreambuf_iterator<char, std::char_traits<char> > > date_facet;

void fillDateTimeFacet(boost::gregorian::date_facet*, Language);
date from_normal_string(const std::string&);

std::ostream& operator<<(std::ostream&, const date&);
std::ostream& operator<<(std::ostream&, const date_duration&);
} // gregorian

namespace posix_time
{
class ptime;
class time_duration;
class minutes;

std::ostream& operator<<(std::ostream&, const ptime&);
std::ostream& operator<<(std::ostream&, const time_duration&);
} // posix_time
} // boost

namespace Dates
{

const int minutesInHour = 60;
const int minutesInDay = 1440;

using namespace boost::gregorian;
using namespace boost::posix_time;
typedef boost::gregorian::date Date_t;
typedef boost::posix_time::ptime DateTime_t;

int maketm_pos(const char *nickname,const char *filename,int line,const char *s,
  struct tm *tm);
#define maketm(s,tm) maketm_pos(NICKNAME,__FILE__,__LINE__,(s),(tm))

bool IsLeapYear( int Year );

/**
 * @brief �㭪�� �����頥� ������⢮ ���� ����� ��ன � ��ࢮ� ��⠬�,
 * @brief ����ᠭ�묨 � �ଠ� ��襩 �� ������ �� ���ᠬ date2 � date1 ᮮ�.
*/
int DateMinus(const char *date2,const char *date1);
long DateMinus(const boost::gregorian::date& date2, const boost::gregorian::date& date1);

typedef struct
{
    char name[4];
    int number;
    int ndays;
} M_mass;

extern  M_mass Mon[12];
extern  M_mass Mez[12];

/**
* @brief Normalize Year
* @param yy
* @return yyyy
*/
int NormalizeYear( int yy );
boost::posix_time::minutes sirena_int_time_to_minutes(int time);
boost::posix_time::time_duration sirena_int_time_to_time_duration(int time);
const char * getMonthName(const int num,char *txt,int lang);
unsigned getMonthNum(const std::string &mon_str);
/* �����頥� ����� �����, � ����� �������� ��� rrmmdd, � ���� */
int nMonthsDays(const char *rrmmdd);

void today_plus(int k, char *snd);
void today_minus(int k, char *snd);

int  Date2Int(const char *olddate);
int  aTime2int( const char *t);
void iTime2ascii( int in, char * s);
int  leap(int year);
long DayNum(int day, int month, int year);
long ADayNum(const char * s);
int  CorrectDate(int day, int mon, int year);
void MakeFaceDate(const char *src,char *dst);
int  CompleteDate(char *out, struct tm *ptm,int day,int mon);
int  CompleteDate1(char *out, struct tm *ptm,int day,int mon);

typedef void (*TodayPlusFuncPointer)(int, char*);
int VFDate(char *Bdate, const char *s, int l, TodayPlusFuncPointer today_plus_fp = today_plus);
/**
  * fast c-style function to print local time to buffer.
  * ��������: ��ப� �ଠ� ������ ᮦ�ঠ�� ᫥� �-��:
  * "%02d.%02d.%02d %02d:%02d:%02d.%06lu"
  * ���, ���, ����, ���, ���, ᥪ, ����ᥪ
  * @return > 0 - number of bytes written
  * @return = 0 - error
*/
size_t localtime_fullformat(char *buff, size_t size, const char *format);

int years_between(const boost::gregorian::date &d1, const boost::gregorian::date &d2);
int months_between(const boost::gregorian::date &d1, const boost::gregorian::date &d2);
const boost::gregorian::date& resolveSpecials(const boost::gregorian::date&);

std::string rrmmdd(const boost::gregorian::date &d);
std::string ddmmrr(const boost::gregorian::date &d, bool delimeter=false);
std::string ddmonrr(const boost::gregorian::date &d, Language);
std::string ddmon(const boost::gregorian::date &d, Language);
std::string ddmmyyyy(const boost::gregorian::date &d, bool delimeter=false);
std::string yyyymmdd(const boost::gregorian::date &d, bool delimeter=false);
std::string mmyy(const boost::gregorian::date&);
std::string yymm(const boost::gregorian::date&);
std::string to_iso_string(const boost::gregorian::date&);
std::string to_iso_extended_string(const boost::gregorian::date&);
std::string ddmonyyyy(const boost::gregorian::date&, Language lang);
std::string hh24mi(const boost::posix_time::time_duration& t, bool delimeter=true);
std::string hh24miss(const boost::posix_time::time_duration& t, bool delimeter=true);
std::string hhmi(const boost::posix_time::time_duration& time);
std::string hhmiss(const boost::posix_time::time_duration& time);
std::string hh24mi_ddmmyyyy(const boost::posix_time::ptime& t);
std::string hh24mi_ddmmyyyy(const boost::gregorian::date& d, const boost::posix_time::time_duration& t);
std::string to_iso_string(const boost::posix_time::ptime&);
std::string to_iso_extended_string(const boost::posix_time::ptime&);

boost::gregorian::date ddmmyyyy(const std::string& d);
boost::gregorian::date rrmmdd(const char* input);
boost::gregorian::date rrmmdd(const std::string &d);
boost::gregorian::date ddmmrr(const std::string &d);
boost::gregorian::date date_from_iso_string(const std::string&);

std::string ptime_to_undelimited_string(const boost::posix_time::ptime& t);
bool ptime_to_undelimited_string(const boost::posix_time::ptime& t, char* out);
boost::posix_time::ptime ptime_from_undelimited_string(const char* input);
boost::posix_time::ptime ptime_from_undelimited_string(const std::string& input);
boost::posix_time::time_duration hh24mi(const std::string& s);
boost::posix_time::time_duration duration_from_string(const std::string&);
boost::posix_time::ptime time_from_iso_string(const std::string&);

int getTotalMinutes(const boost::posix_time::time_duration& time);
//������� ᬥ饭�� � ���� �⭮�⥫쭮 ���� �뫥� ३��
boost::gregorian::date_duration daysOffset(const boost::posix_time::time_duration& time);
//������� ������⢮ ����� �⭮�⥫쭮 ��砫� ⥪��� ��⮪
boost::posix_time::time_duration dayTime(const boost::posix_time::time_duration& time);

boost::posix_time::ptime midnight(const boost::gregorian::date& d);
boost::posix_time::ptime midnight(const boost::posix_time::ptime& t);
boost::posix_time::ptime end_of_day(const boost::gregorian::date& d);
boost::posix_time::ptime end_of_day(const boost::posix_time::ptime& t);
std::string timeDurationToStr(const boost::posix_time::time_duration& td);

boost::gregorian::date currentDate();
boost::posix_time::ptime currentDateTime();
boost::posix_time::ptime currentDateTime_us();

/* �㭪�� �����뢠�� � ��ப� � ���ᮬ stout ���㪢����� ������祭��
   ��� ������ (�� ������ dn; 1 - '���'; 7 - '���') �� �몥 lan
   � ��砥 �訡�� �����頥� -1 */
int getdayname(char *stout,int dn,int lan);
/* �㭪�� �����뢠�� � ��ப� � ���ᮬ stout ����㪢����� ������祭��
   ��� ������ (�� ������ dn; 1 - '��'; 7 - '��') �� �몥 lan
   � ��砥 �訡�� �����頥� -1 */

/* �㭪�� �����뢠�� � ��ப� � ���ᮬ s ���㪢����� ᮪�饭�� ��������
   ��� ������ � ����஬ n. �����頥� -1 � ��砥 �訡�筮�� ����� ���
   ������ � 0 - � ��砥 �ࠢ��쭮��.
   �⫨砥��� �� getdayname() ���᪮���� ᮪�饭���*/
int  day_abb(char *s,int n,int lng);
std::string day_abb(int dayNo, Language, bool two_letter = false);
std::string day_abb(const boost::gregorian::date&, Language, bool two_letter = false);

std::string month_name(int monthNo, Language);
std::string month_name(const boost::gregorian::date&, Language);

int getdayname2(char *stout,int dn,int lan);
#define GetDayName3(x,y) getdayname((x),(y),agent.lang)
#define GetDayName2(x,y) getdayname2((x),(y),agent.lang)

/* �㭪�� �����뢠�� � ��ப� � ���ᮬ newdate ����, ����� �.�.
   �१ n ���� �⭮�⥫쭮 ����, ����ᠭ��� � ��ப� � ���ᮬ olddate   */
void DayShift(char *newdate,const char *olddate,int n);
// Add to date n seconds
void DayShiftSec(char *newdate,const char *olddate,int n);

/* � ��ப� � ���ᮬ snd  �����뢠���� ��� �।��饣� ��� �⭮�⥫쭮 ���
    � ��⮩, ����ᠭ��� � ��ப� � ���ᮬ td                               */
void prev_day( char *snd, const char *td);
/* � ��ப� � ���ᮬ snd  �����뢠���� ��� ᫥���饣� ��� �⭮�⥫쭮 ���
    � ��⮩, ����ᠭ��� � ��ப� � ���ᮬ td                               */
void next_day( char *snd, const char *td);

/* � ��ப� � ���ᮬ newfreq �����뢠���� 横���᪨ ᬥ饭��� �� n ����
   ���� �������� , ����ᠭ��� � ��ப� � ���ᮬ oldfreq                */
void FreqShift(char *newfreq,const char *oldfreq,int n);

/* �㭪�� ���������� ����� , ����ᠭ��� � ��ப� s ������ l, ��������
   � ����᭮� �ଠ� � ��९��뢠�� �� � ��ப� � ���ᮬ freq � �ଠ�
   ��襩 ��     */
int GetFreq(char *freq,const char *s,int l);

/* �㭪�� ���������� ����� , ����ᠭ��� � ��ப� freq ������ l, ��������
   �ଠ� ��襩 �� � ��९��뢠�� �� � ��ப� � ���ᮬ sf � ᮪�.�ଥ
   �����頥� ����� ��ப� freq */
int fan(const char *freq,char *sf,int lng);

/* DETE6=1 ����砥�, �� ��� �।�⠢����� � �ଠ� YYMMDD - 960228
   DATE6=0 ����砥�, �� ��� �।�⠢����� � �ଠ� YYYYMMDD - 19960228 */
extern int DATE6;
/* TIME2=1 ����砥�, �� �६� �।�⠢����� ⮫쪮 �ᠬ� �� - 12
   � ������ ������� �㫥�묨
   TIME2=0 ����砥�, �� �६� �।�⠢���� � ���� HHMI - 1203              */
extern int TIME2;

/* �����頥��� ࠧ����� ����� ���祭�ﬨ �६��� t2 � t1, ������묨  �
   ���� 楫�� �ᥫ, � ������ ����⪨ � ������� ������� ������⢮ �����,
   � ����� � �⭨ ������⢮ �ᮢ , ���ਬ�� TimeSub(1120,830)=240      */
int TimeSub(int t2,int t1);
/* �����頥��� c㬬� ���祭�� �६��� t2 � t1, ������묨 � ���� 楫�� �ᥫ,
   � ������ ����⪨ � ������� ������� ������⢮ �����, � ����� � �⭨
   ������⢮ �ᮢ , ���ਬ�� TimeAdd(1120,850)=2010                      */
int TimeAdd(int t2,int t1);

/* �㭪�� �����頥� ����� ��� ������ �� ᨬ���쭮�� ���/����-�㪢������ ����
   (1 - '���'; 7 - '���') �� ���/���� �몥. � ��砥 �訡�� �����頥� -1 */
int IsDayName(const char *stin,int stlen);

/* �㭪�� �஢����, ���� �� ��ப� �� ����� s ������  l ᨬ�����
   �६���� � �����⨬�� � ��襩 ��⥬� �ଠ�. �������: 0 - ���,1 -�� */
int Is_time(const char *s,int l);

/* �㭪�� �஢����, ���� �� ��ப� �� ����� s ������  l ᨬ�����
   ��⮩ � �����⨬�� � �ଠ� a-la Sirena 2M. �������: 0 - ���,1 -��    */
int Is_date(const char *s, int l);

/* �㭪�� �஢���� �� �ࠢ��쭮��� �६� � �ଠ� HH24MISS
   0 - �����४⭮� �६�, 1 - ��୮ */
int Is_fulltime(const char *s,int l);

/* � ��ப� s1 �����뢠���� ����� ��ப� s2 - ��᫥���� 2,�।��� 2,���� 2*/
void date_cat(char *s1,const char *s2);

/* � ��ப� s1 ��������� ��� � �ଠ� DDMONYY (�᫨ lng=ENGLISH)
   ��� � �ଠ� ������� (�᫨ lng=RUSSIAN) , ᮮ⢥������ ��� � s2 �
   �ଠ� ��  */
int  day_cpy(char *s1,const char *s2,int lng);

boost::gregorian::date MakeGregorianDate(unsigned year, unsigned month, unsigned day);
std::string makeDate6(const boost::gregorian::date&);
void makeDate6(char* dest, const boost::gregorian::date&);
std::string makeDate8(const boost::gregorian::date&);
void makeDate8 (char *dest, const boost::gregorian::date&);
std::string makeDateHuman6(boost::gregorian::date const &d, int lang);
std::string makeDateHuman8(boost::gregorian::date const &d, int lang);

/* �஢����, ���� �� ��� ����ᠭ��� � ��ப� � ���ᮬ s,
    ����஫��㥬��. �����頥� 1, �᫨ ����, 0 - �᫨ ���.             */
int DatNotCont(const char *s);
int DatNotCont(const boost::gregorian::date& d);

/* �㭪�� ���������� ���� � �ଠ� a-la WorldSpan, ����ᠭ��� � ��ப�
   �� ����� s ������ l, � �����뢠�� �� �� ����७��� �ଠ� �� ��⥬� �
   ��ப� � ���ᮬ Bdate. ������� 1 � ��砥 ��ଠ�쭮�� �����襭��
   � -1 � ��砥 �訡�� � ��室��� ����� ����                             */
int VerDate(char *Bdate,const char *s,int l);
int VecDate(char *Bdate,char *s,int l);

/* Transferred from Helpfunc.c */
/* Please, add comments !!! */
int D2_D1(const char *date2,const char *date1);

/* �� �㭪�� ������ ࠧ���� ����� date2 � date1 � ������ ������.
   date1,date2 - � �ଠ� RRMMDD    */
int MonMinus(const char *date2,const char *date1);
long MonMinus(const boost::gregorian::date& date2, const boost::gregorian::date& date1);
long MonMinus(const boost::posix_time::ptime& date2, const boost::posix_time::ptime& date1);

/* �㭪�� �����뢠�� � ��ப� � ���ᮬ newdate ����, ����� �.�.
   �१ n ���楢 �⭮�⥫쭮 ����, ����ᠭ��� � ��ப� � ���ᮬ olddate */
int MonShift(char *newdate,const char *olddate,int n);
boost::posix_time::ptime mon_shift(const boost::posix_time::ptime& t, const int m);
boost::gregorian::date mon_shift(const boost::gregorian::date& d, const int m);

// �ࠢ������ ��� yyyymmddhh24miss ���� � �����頥� -1/0/1
short CompareTimes(const char *dt1,const char *dt2);
// �ࠢ������ ��� rrmmddhh24miss ���� � �����頥� -1/0/1
short YYCompareTimes(const char *dt1,const char *dt2);

void fixTimeZone(int source_tz,int result_tz,
      int source_time,char const *source_date,
      int *result_time, char *result_date);

std::string get2CMonthNum(const std::string &mon_str);

boost::gregorian::date from_sirena_date(std::string const & date);
boost::gregorian::date from_sirena_date(const char *date);

//returns boost::posix_time::time_duration from HHMM string
boost::posix_time::time_duration from_sirena_time(std::string const &s);
boost::posix_time::time_duration from_sirena_time(char const *s);

//returns boost::posix_time::prime from sirena date dt and HHMM string
boost::posix_time::ptime from_sirena_date_and_time(const char *dt, const char *tm);

/* Adds to dt1 time (YYYYMMDDHH24MISS) delta seconds and write it into
   dt2 (YYYYMMDDHH24MISS)
   dt1 == dt2 - you can do it */
void ChangeTime(const char *dt1, int delta, char *dt2);

typedef boost::gregorian::date (YY_to_YYYY)(
        unsigned yy,
        unsigned month,
        unsigned day,
        boost::gregorian::date currentDate);

/*
 * If current year = 2000:
 * 99 -> 2099,
 * 01 -> 2001
 */
YY_to_YYYY YY2YYYY_UseCurrentCentury;

/*
 * �᫨ � १���� ���������� ⥪�饣� �⮫��� � ������஢��� ����
 * ����砥��� ��� > ⥪�饩 ����, �� ���� ���⠥��� 100.
 *
 * �ਬ�� (⥪�騩 ��� = 2000):
 * 99 -> 1999,
 * 01 -> 2001
 */
YY_to_YYYY YY2YYYY_WraparoundFutureDate;

/*
 * �᫨ � १���� ���������� ⥪�饣� �⮫��� � ������஢��� ����
 * ����砥��� ��� > ⥪�饩 ���� �� 50 � ����� ���, �� ���� ���⠥��� 100.
 *
 * �ਬ�� (⥪�騩 ��� = 2000):
 * 49 -> 2049,
 * 50 -> 1950,
 * 01 -> 2001,
 */
YY_to_YYYY YY2YYYY_Wraparound50YearsFutureDate;

boost::gregorian::date DateFromDDMONYY(
        const std::string& dateStr,
        YY_to_YYYY yy2yyyy,
        boost::gregorian::date currentDate);

boost::gregorian::date DateFromYYMMDD(
        const std::string& dateStr,
        YY_to_YYYY yy2yyyy,
        boost::gregorian::date currentDate);

boost::gregorian::date DateFromDDMMYY(
        const std::string& dateStr,
        YY_to_YYYY yy2yyyy,
        boost::gregorian::date currentDate);

/* Date from DD.MM.YY, like 05.03.15 */
boost::gregorian::date DateFromDD_MM_YY(
        const std::string& dateStr,
        YY_to_YYYY yy2yyyy,
        boost::gregorian::date currentDate);

boost::gregorian::date DateFromYYYYMMDD(const char* dateStr, unsigned size);
boost::gregorian::date DateFromYYYYMMDD(const std::string& dateStr);
boost::gregorian::date DateFromDDMONYYYY(const std::string& dateStr);
/* Date form DD.MM.YYYY, like 05.03.2015 */
boost::gregorian::date DateFromDD_MM_YYYY(const std::string& dateStr);
boost::posix_time::time_duration TimeFromHHMM(const std::string& timeStr);
boost::posix_time::time_duration TimeFromHHMM(const char* timeStr, unsigned size);

/* �᫨ � ��� �� 㪠��� ���, ��� ���� �����-� ��ࠧ�� 㧭��� */
struct GuessYear {
    virtual ~GuessYear() {}
    virtual boost::gregorian::date operator()(unsigned month, unsigned day, boost::gregorian::date currentDate) const = 0;
};

/* �ᯮ������ ��� �� ⥪�饩 ���� */
struct GuessYear_Current: public GuessYear {
    virtual boost::gregorian::date operator()(unsigned month, unsigned day, boost::gregorian::date currentDate) const;
};

/* �᫨ ��� �� �ᯮ�짮����� ⥪�饣� ���� ����뢠���� � ��諮�, �ᯮ������ ᫥���騩 ��� */
struct GuessYear_Future: public GuessYear {
    virtual boost::gregorian::date operator()(unsigned month, unsigned day, boost::gregorian::date currentDate) const;
};

/* �᫨ ��� �� �ᯮ�짮����� ⥪�饣� ���� ����뢠���� � ���饬, �ᯮ������ �।��騩 ��� */
struct GuessYear_Past: public GuessYear {
    virtual boost::gregorian::date operator()(unsigned month, unsigned day, boost::gregorian::date currentDate) const;
};

/*
 * ���樠�쭠� ����� ��� "㣠�뢠���" ���� ᥣ���⮢ ������.
 * ����� �ᯮ������ VFDate().
 * C��⢥����� GuessYear_Limit().limitPast(32).limitFuture(330)
 *
 * ��ࠬ��� currentDate ����������.
 */
struct GuessYear_Itin: public GuessYear {
    virtual boost::gregorian::date operator()(unsigned month, unsigned day, boost::gregorian::date currentDate) const;
};

boost::gregorian::date DateFromDDMON(
        const std::string& dateStr,
        const GuessYear& guessYear,
        boost::gregorian::date currentDate);

boost::gregorian::date DateFromMMDD(
        const std::string& dateStr,
        const GuessYear& guessYear,
        boost::gregorian::date currentDate);

boost::gregorian::date DateFromDDMM(
        const std::string& dateStr,
        const GuessYear& guessYear,
        boost::gregorian::date currentDate);

std::string dateToDDMMYY(const boost::gregorian::date& date);
std::string dateToYYMMDD(const boost::gregorian::date& date);
std::string durationToHHMM(const boost::posix_time::time_duration& dur);

int YYYYfromYY(int yy);
int YYYYfromYY(const char *yy);
std::string normalizeDate(const std::string &date);

/* 2 �㭪樨, ������騥 ᬥ饭�� �६.���� tz ��. GMT (� ������). */
int GmtDiffCityZone(const char *rrmmdd_date,const char *short_time,const char *tz);
int GmtDiffCityZone(const char *rrmmddhhmiss_date,const char *tz);

unsigned short day_of_week_ru(const boost::gregorian::date& d);
unsigned short day_of_week_ru(const boost::posix_time::ptime& t);
// �����頥��� ����� ��� ������ ���� [1-7], ����ᠭ��� � ��ப� � ���ᮬ date (rrmmdd)
// snatched from boost gregorian calendar
inline int week_day(const char* date)
{
    unsigned short year = date[0]*10+date[1]-11*'0';
    year += year < 50 ? 2000 : 1900;
    unsigned short month = date[2]*10+date[3]-11*'0';
    unsigned short day = date[4]*10+date[5]-11*'0';

    // dark boost witchery
    unsigned short a = (14 - month) / 12;
    unsigned short y = year - a;
    unsigned short m = month + 12*a - 2;
    unsigned short d = (day + y + (y/4) - (y/100) + (y/400) + (31*m)/12) % 7;
    return d == 0 ? 7 : d;
}

} // namespace Dates


#ifdef XP_TESTING
namespace xp_testing
{
bool setDateTimeOffset(long, const std::string& = std::string());
void fixDate(const boost::gregorian::date&);
void fixDateTime(const boost::posix_time::ptime&, const std::string& = std::string());
void unfixDate();
} // xp_testing
#endif // XP_TESTING

#endif /* __cplusplus */
