#if HAVE_CONFIG_H
#endif

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <cstdio>

#include "dates.h"
#include "localtime.h"
#include "dates_io.h"
#include "str_utils.h"
#include "exception.h"
#include <unistd.h>

#define NICKNAME "ROMAN"
#include "slogger.h"

#ifdef XP_TESTING

namespace  {

// Sets offset from NOW for currentDateTime()
class TNowOffset
{
    static long *get_offset_ptr();
    static std::string& tz_string();
  public:
    static long get();
    static const std::string& get_tz_string();
    static void set_tz_string(const std::string&);
    // Set new offset in seconds.
    // Returns true if offset changed
    static bool set(long offs);
    // Set new offset by needed current time.
    static void set(const boost::posix_time::ptime& new_now);
};

long* TNowOffset::get_offset_ptr()
{
  static long offset=0;
  return &offset;
}

std::string& TNowOffset::tz_string()
{
    static std::string tz = "GMT";
    return tz;
}

long TNowOffset::get()
{
    return *TNowOffset::get_offset_ptr();
}

const std::string& TNowOffset::get_tz_string()
{
    return tz_string();
}

void TNowOffset::set_tz_string(const std::string& tz)
{
    tz_string() = tz;
}

// returns true when offset changed
bool TNowOffset::set(long offs)
{
    long *p = TNowOffset::get_offset_ptr();
    if (*p != offs)
    {
        *p = offs;
        return true;
    }
    return false;
}

// Sets new offset (`new_now' will be current date/time).
void TNowOffset::set(const boost::posix_time::ptime& new_now)
{
    LocalTime is_now(boost::posix_time::second_clock::local_time(), get_tz_string());
    LocalTime be_now(new_now, get_tz_string());
    TNowOffset::set( be_now - is_now);
}

} // namespace


#endif // XP_TESTING

namespace {

#define D_LONG_DATE    14     /* Date 'YYYYMMDDHHMISS' len */
#define D_SHORT_DATE    6     /* Date 'DDMMRR' len */
#define D_MIN_DATE "19500101000000"
#define D_SPACE    ' '

    int months_between_dates_where_the_first_one_is_lesser(
                const boost::gregorian::date &d1,
                const boost::gregorian::date &d2
            )
    {
        return (d2.year() - d1.year()) * 12 +
               (d2.month() - d1.month()) -
               (d2.day() < d1.day() ? 1 : 0);
    }
}

namespace Dates
{
int DATE6=1;
int TIME2=0;
static const int LOOK_BACK = 32;

using namespace std;

M_mass Mon[12]= {{"JAN", 1,31},{"FEB", 2,29},{"MAR", 3,31},{"APR", 4,30},
             {"MAY", 5,31},{"JUN", 6,30},{"JUL", 7,31},{"AUG", 8,31},
             {"SEP", 9,30},{"OCT",10,31},{"NOV",11,30},{"DEC",12,31}};

M_mass Mez[12]= {{"ЯНВ", 1,31},{"ФЕВ", 2,29},{"МАР", 3,31},{"АПР", 4,30},
             {"МАЙ", 5,31},{"ИЮН", 6,30},{"ИЮЛ", 7,31},{"АВГ", 8,31},
             {"СЕН", 9,30},{"ОКТ",10,31},{"НОЯ",11,30},{"ДЕК",12,31}};

static const char *DofW3[2][7]={{"MON","TUE","WED","THU","FRI","SAT","SUN"},
                        {"ПОН","ВТР","СРД","ЧЕТ","ПТН","СУБ","ВСК"}};
static const char *DofW2[2][7]={{"MO","TU","WE","TH","FR","SA","SU"},
                         {"ПН","ВТ","СР","ЧТ","ПТ","СБ","ВС"}};

static const char* Days_Abb[2][7] = {
    { "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN" },
    { "ПНД", "ВТР", "СРД", "ЧТВ", "ПТН", "СБТ", "ВСК" }
};

bool IsLeapYear( int Year )
{
    return ( ( !( Year%4 ) )&&( ( Year%100 )||( !( Year%400 ) ) ) );
}

boost::posix_time::minutes sirena_int_time_to_minutes(int time)
{
    int h = time / 100;
    int m = time % 100;

    return boost::posix_time::minutes(h * 60 + m);
}

boost::posix_time::time_duration sirena_int_time_to_time_duration(int time)
{
    // if(time < 0)
    //    throw std::out_of_range("sirena int time cannot be negative");
    return boost::posix_time::time_duration(time/100, time%100, 0, 0);
}

int NormalizeYear( int yy )
{
    if ( yy >= 0 && yy <= 99 )
        return ( yy > 50 ) ? 1900 + yy : 2000 + yy ;
    else return yy;
}

const char *long_months[2][12] = {{"JANUARY","FEBRUARY","MARCH","APRIL","MAY",
                                   "JUNE","JULY","AUGUST","SEPTEMBER","OCTOBER",
                                   "NOVEMBER","DECEMBER"},
                                  {"ЯНВАРЬ","ФЕВРАЛЬ","МАРТ","АПРЕЛЬ","МАЙ",
                                   "ИЮНЬ","ИЮЛЬ","АВГУСТ","СЕНТЯБРЬ","ОКТЯБРЬ",
                                   "НОЯБРЬ","ДЕКАБРЬ"}};

int maketm_pos(const char *nickname,const char *filename,int line,const char *s,
  struct tm *tm)
{
    time_t ret;
    switch( (s!=NULL && tm!=NULL) ? strlen(s) : -1){
        case 6:   /*RRMMDD*/
            sscanf(s+4,"%2d",&tm->tm_mday);
            sscanf(s+2,"%2d",&tm->tm_mon);tm->tm_mon--;
            sscanf(s,"%2d",&tm->tm_year);
            if(tm->tm_year<91)
                tm->tm_year+=100;
            tm->tm_sec=tm->tm_min=tm->tm_hour=0;
            tm->tm_isdst=-1;
            ret=mktime(tm);
            break;
        case 10:   /*RRMMDDHHMI*/
            sscanf(s+4,"%2d",&tm->tm_mday);
            sscanf(s+2,"%2d",&tm->tm_mon);tm->tm_mon--;
            sscanf(s,"%2d",&tm->tm_year);
            if(tm->tm_year<91)
                tm->tm_year+=100;
            tm->tm_isdst=-1;
            tm->tm_sec=0;
            sscanf(s+8,"%2d",&tm->tm_min);
            sscanf(s+6,"%2d",&tm->tm_hour);
            ret=mktime(tm);
            break;
        case 8:  /*YYYYMMDD*/
            sscanf(s+6,"%2d",&tm->tm_mday);
            sscanf(s+4,"%2d",&tm->tm_mon);tm->tm_mon--;
            sscanf(s,"%4d",&tm->tm_year);
            tm->tm_year-=1900;
            tm->tm_isdst=-1;
            tm->tm_sec=tm->tm_min=tm->tm_hour=0;
            ret=mktime(tm);
            break;
        case 12: /*YYYYMMDDHHMM*/
            sscanf(s+6,"%2d",&tm->tm_mday);
            sscanf(s+4,"%2d",&tm->tm_mon);tm->tm_mon--;
            sscanf(s,"%4d",&tm->tm_year);
            tm->tm_isdst=-1;
            tm->tm_year-=1900;
            tm->tm_sec=0;
            sscanf(s+10,"%2d",&tm->tm_min);
            sscanf(s+8,"%2d",&tm->tm_hour);
            ret=mktime(tm);
            break;
        case 14:/*YYYYMMDDHHMMSS*/
            sscanf(s+6,"%2d",&tm->tm_mday);
            sscanf(s+4,"%2d",&tm->tm_mon);tm->tm_mon--;
            sscanf(s,"%4d",&tm->tm_year);
            tm->tm_isdst=-1;
            tm->tm_year-=1900;
            sscanf(s+12,"%2d",&tm->tm_sec);
            sscanf(s+10,"%2d",&tm->tm_min);
            sscanf(s+8,"%2d",&tm->tm_hour);
            ret=mktime(tm);
            break;
         default:
            ProgError(nickname,filename,line,"maketm, s=<%.15s> tm=%s",SNULL(s),
              tm?"Ok":"(null)");
//            ProgError(STDLOG,"maketm, s=<%.15s> tm=%s",SNULL(s),
//                tm?"Ok":"(null)");
            return -1;
    }
    if( ret == -1 )
    {
      if( tm->tm_year <= 70 )
        ret = 0;
      else if( tm->tm_year >=138 )
        ret = static_cast<time_t>(0x7FFFFFFFL);
      WriteLog(nickname,filename,line,"mktime() failed: s=<%.15s> - return %ld",
        SNULL(s),ret);
//      WriteLog(STDLOG,"mktime() failed: s=<%.15s> - return %ld",SNULL(s),ret);
    }
    return ret;
}

void today_plus(int k, char *snd)
{
    const std::string rrmmdd = Dates::rrmmdd(Dates::currentDate() + Dates::days(k));
    strcpy(snd, rrmmdd.c_str());
}

void today_minus(int k, char *snd)
{
    const std::string rrmmdd = Dates::rrmmdd(Dates::currentDate() - Dates::days(k));
    strcpy(snd, rrmmdd.c_str());
}

int VFDate(char *Bdate,const char *s,int l, TodayPlusFuncPointer today_plus_fp)
{
  int i,dn,mn=0,yn=0,k,p,cyn,cmn,cdn,Mt=0;
  char sub[4],tmp[9],tmpn[9],tmpp[19];

  p=-1; Mt=1;
  for(i=0;i<l;i++)
  if((s[i]>'9')||(s[i]<'0')) { p=i; break; }
  if(p>2) return -1;
  if(p==-1) Mt=0;

  if(Mt==1)
  {
    if(s[0]=='Ц')
    {
      /* if((s[1]=='1')&&(l==2)) { today_plus(14,Bdate); return 1; } */
     if((s[1]=='2')&&(l==2)) { (*today_plus_fp)(31,Bdate); return 1; }
     return -1;
    }
    if((s[0]=='С')||(s[0]=='N')||(s[0]=='З')||(s[0]=='T'))
    {
      if(((s[0]=='С')||(s[0]=='N'))&&(l==1)) { (*today_plus_fp)(0,Bdate); return 1;}
      if(((s[0]=='З')||(s[0]=='T'))&&(l==1)) { (*today_plus_fp)(1,Bdate); return 1;}
      if((l>2)&&((s[1]=='+')||(s[1]=='-')))
      { if((l-2)>8)
          return -1;
        tmp[0]=0; strncat(tmp,s+2,(l-2)); k=atoi(tmp);
        if((s[0]=='З')||(s[0]=='T')) { if(s[1]=='-') k--;  else k++; }
        if(s[1]=='-') { (*today_plus_fp)((-1)*k,Bdate); return 1; }
    if(s[1]=='+') { (*today_plus_fp)(k,Bdate);  return 1; }
      }
      return -1;
    }
     for(i=p;i<l and i<p+3;i++) { if((s[i]<='9')&&(s[i]>='0')) return -1; }
     if( ((l-p-3)!=2)&&((l-p-3)!=0) ) return -1;
     for(i=p+3;i<l;i++) { if((s[i]>'9')||(s[i]<'0')) return -1; }
   }
  if(Mt==0)
  {
    if((l!=6)&&(l!=4)) return -1;
    for(i=0;i<l;i++) { if((s[i]>'9')||(s[i]<'0')) return -1; }
  }

  sub[0]=0;
  if(Mt==1) strncat(sub,s,p); else strncat(sub,s,2);
  dn=atoi(sub);
  if((dn<1)||(dn>31)) return -1;
  if(Mt==1)
  {
   sub[0]=0; strncat(sub,s+p,3);
   mn=0;
    for(i=0;i<12;i++)
    {
     if(strncmp(sub,Mez[i].name,3)==0) { mn=Mez[i].number; k=i; break; }
     if(strncmp(sub,Mon[i].name,3)==0) { mn=Mon[i].number; k=i; break; }
    }
   if(mn==0) return -1;
   if(dn>Mez[k].ndays) return -1;
   if((l-p-3)==2) { sub[0]=0; strncat(sub,s+p+3,2);     yn=atoi(sub); }
   else
   {  (*today_plus_fp)(0,tmp);
      sub[0]=0; strncat(sub,tmp,2); cyn=atoi(sub);
      sub[0]=0; strncat(sub,tmp+2,2); cmn=atoi(sub);
      sub[0]=0; strncat(sub,tmp+4,2); cdn=atoi(sub);
      if(cmn<mn)
      {
        yn=cyn;
        sprintf(tmpn,"%02d%02d%02d",cyn,cmn,cdn);
        sprintf(tmpp,"%02d%02d%02d",cyn,mn ,dn );
        if(DateMinus(tmpp,tmpn)>330)
        {
         sprintf(tmpp,"%02d%02d%02d",cyn-1,mn,dn);
         if(DateMinus(tmpp,tmpn)>=-LOOK_BACK) yn=cyn-1;
        }
      }
      else
      { p=0;
        for(i=mn;i<cmn;i++) p+=Mez[i-1].ndays;
        p+=(cdn-dn);
        if(p>LOOK_BACK) yn=cyn+1;
                   else yn=cyn;
      }
   }
  }

  if(Mt==0)
  {
      sub[0]=0; strncat(sub,s+2,2);
      mn=atoi(sub);
      if((mn<=0)||(mn>12)) return -1;
      if(Mez[mn-1].ndays<dn) return -1;
      sub[0]=0;
      if(l==6) { strncat(sub,s+4,2);  yn=atoi(sub); }
      else
      {
       (*today_plus_fp)(0,tmp);
       sub[0]=0; strncat(sub,tmp,2); cyn=atoi(sub);
       sub[0]=0; strncat(sub,tmp+2,2); cmn=atoi(sub);
       sub[0]=0; strncat(sub,tmp+4,2); cdn=atoi(sub);
       if(cmn<mn)
       {
         tst();
         yn=cyn;
         sprintf(tmpn,"%02u%02u%02u",cyn,cmn,cdn);
         sprintf(tmpp,"%02u%02u%02u",cyn,mn,dn);
         if(DateMinus(tmpp,tmp)>330)
         {
          tst();
          sprintf(tmpp,"%02u%02u%02u",cyn-1,mn,dn);
          if(DateMinus(tmpp,tmpn)>=-LOOK_BACK) yn=cyn-1;
         }
       }
       else
       {
        p=0;
        for(i=mn;i<cmn;i++) p+=Mez[i-1].ndays;
        p+=(cdn-dn);
        if(p>LOOK_BACK) yn=cyn+1;
                   else yn=cyn;
        }
      }
   }

  if(yn>=100) yn=(yn%100);
  if( ((yn%4)!=0)&&(mn==2)&&(dn>28) ) return -1;
  if(yn<0) return -12;
  sprintf(Bdate,"%.2d%.2d%.2d",yn,mn,dn);
  return 1;
}

boost::gregorian::date mon_shift(const boost::gregorian::date& d, const int m)
{
    boost::gregorian::date r = d + boost::gregorian::months(m);
/*    if(r.day() > d.day())
        r -= boost::gregorian::days(r.day() - d.day());*/
    return r;
}

boost::posix_time::ptime mon_shift(const boost::posix_time::ptime& t, const int m)
{
    return boost::posix_time::ptime( mon_shift(t.date(),m), t.time_of_day() );
}


long DateMinus(const boost::gregorian::date& date2, const boost::gregorian::date& date1)
{
    return (date2 - date1).days();
}

int DateMinus(const char *date2,const char *date1)
{
   static struct tm tb2,tb1;
   char y[5],m[3],d[3];
   time_t t1,t2,dt;
   int n;

   d[0]=0;y[0]=0;m[0]=0;
   strncat(y,date1,2); strncat(m,date1+2,2); strncat(d,date1+4,2);

   tb1.tm_sec=0;  tb1.tm_min=0;  tb1.tm_hour=12; tb1.tm_isdst=-1;
   tb1.tm_mday=atoi(d);   tb1.tm_mon =atoi(m)-1;
   tb1.tm_year=atoi(y);
   if(tb1.tm_year<50) tb1.tm_year+=100;

   d[0]=0;y[0]=0;m[0]=0;
   strncat(y,date2,2); strncat(m,date2+2,2); strncat(d,date2+4,2);

   tb2.tm_sec=0;  tb2.tm_min=0;  tb2.tm_hour=12; tb2.tm_isdst=-1;
   tb2.tm_mday=atoi(d);   tb2.tm_mon =atoi(m)-1;
   tb2.tm_year=atoi(y);
   if(tb2.tm_year<50) tb2.tm_year+=100;

/* Неприятность: функция mktime() корректно работает только на датах */
/* с 1-янв-1970 до чего-то-там-2038. Если у нас даты вылезают за этот */
/* интервал, то надо обойти вызовы mktime(). */
   if( tb1.tm_year >= 135 || tb2.tm_year >=135 ||
       tb1.tm_year < 70 || tb2.tm_year < 70 )
   {
     ProgTrace(TRACE4, "DateMinus: work around failing mktime(): "
               "d2=%d/%d/%d, d1=%d/%d/%d",tb2.tm_year,tb2.tm_mon,
               tb2.tm_mday,tb1.tm_year,tb1.tm_mon,tb1.tm_mday);
     if( tb1.tm_year >= 135 && tb2.tm_year >=135 )
       return 0;
     else if( tb1.tm_year < 70 && tb2.tm_year < 70 )
       return 0;
     else if( tb2.tm_year >= 135 )
       return 25000;
     else if( tb1.tm_year >= 135 )
       return -25000;
     else if( tb2.tm_year < 70 )
       return -25000;
     else if( tb1.tm_year < 70 )
       return 25000;
   }

   if( (t1=mktime(&tb1))==((time_t)-1) )
   {
     WriteLog(STDLOG,"mktime() failed! %d/%d/%d",tb1.tm_year,tb1.tm_mon,
               tb1.tm_mday);
     return -25000;
   }
   if( (t2=mktime(&tb2))==((time_t)-1) )
   {
     WriteLog(STDLOG,"mktime() failed! %d/%d/%d",tb2.tm_year,tb2.tm_mon,
               tb2.tm_mday);
     return +25000;
   }

   dt=t2-t1;
   dt+=(tb2.tm_isdst-tb1.tm_isdst)*3600;
   n=dt/86400;
   return n;
 }

unsigned getMonthNum(const std::string &mon_str)
{
    unsigned i;
    for (i=0;i<12;i++){
        if(!strncmp(mon_str.c_str(), Mon[i].name, 3) ||
           !strncmp(mon_str.c_str(), Mez[i].name, 3)){
            return i+1;
        }
    }
    return 0;
}

const char * getMonthName(const int mon_num, char *txt, int lang)
{
    if(mon_num<1 || mon_num>12)
        return strcpy(txt, "");

    if(lang == RUSSIAN)
        return strcpy(txt, Mez[mon_num - 1].name);
    else
        return strcpy(txt, Mon[mon_num - 1].name);
}

int nMonthsDays(const char *rrmmdd)
{
  if(!rrmmdd)
    return -1;
  int month_num=atoinNVL(rrmmdd+2,2,0);
  if(month_num<1 || month_num>12)
    return -2;

  int n_days=Mon[month_num-1].ndays;

  // Отдельно проверим февраль - 28 или 29 дней
  if(month_num==2) // февраль
  {
    if(!IsLeapYear(YYYYfromYY(atoinNVL(rrmmdd,2,0))))
      n_days=28;
  }
  return n_days;
}

size_t localtime_fullformat(char *buff, size_t size, const char *format)
{
    struct timeval tv;
    struct tm *tm;
    gettimeofday(&tv,NULL);
    tm = localtime(&tv.tv_sec);

    ssize_t ret_len = snprintf(buff, size, format,
                              tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour,
                              tm->tm_min, tm->tm_sec, tv.tv_usec);

    if(ret_len < 0 || static_cast<size_t>(ret_len) >= size) {
        ret_len = 0;
    }
    buff[ret_len] = '\0';

    return ret_len;
}

unsigned short day_of_week_ru(const boost::posix_time::ptime& t)
{
    return day_of_week_ru(t.date());
}

unsigned short day_of_week_ru(const boost::gregorian::date& d)
{
    return (d.day_of_week().as_number() + 6) % 7 + 1;
}

const boost::gregorian::date& resolveSpecials(const boost::gregorian::date& d)
{
    static const boost::gregorian::date posInf(2049, 12, 31);
    static const boost::gregorian::date negInf(1949, 01, 01);

    if (d.is_infinity()) {
        if (d.is_pos_infinity())    return posInf;
        if (d.is_neg_infinity())    return negInf;
    }
    return d;
}

std::string rrmmdd(const boost::gregorian::date &d)
{
    const boost::gregorian::date& dt = resolveSpecials(d);
    if (dt.is_not_a_date()) return std::string();

    char buf[7] = {0};
    std::sprintf(buf, "%2.2i%2.2i%2.2i", static_cast<unsigned>(dt.year() % 100), static_cast<unsigned>(dt.month()%100), static_cast<unsigned>(dt.day()%100));
    return buf;
}

std::string ddmmrr(const boost::gregorian::date &d, bool delimeter)
{
    const boost::gregorian::date& dt = resolveSpecials(d);
    if (dt.is_not_a_date()) return std::string();

    char buf[9] = {0};
    std::sprintf(buf, delimeter ? "%2.2i.%2.2i.%2.2i" : "%2.2i%2.2i%2.2i", static_cast<int>(dt.day()), static_cast<int>(dt.month()), static_cast<int>(dt.year() % 100));
    return buf;
}

std::string ddmonrr(const boost::gregorian::date &d, Language lang)
{
    static const char* months[2][12] = {
        { "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" },
        { "ЯНВ", "ФЕВ", "МАР", "АПР", "МАЙ", "ИЮН", "ИЮЛ", "АВГ", "СЕН", "ОКТ", "НОЯ", "ДЕК" }
    };
    const boost::gregorian::date& dt = resolveSpecials(d);
    if (dt.is_not_a_date()) return std::string();

    char buf[8] = {0};
    std::sprintf(buf, "%2.2i%s%2.2i", static_cast<int>(dt.day()), months[lang][dt.month()-1], static_cast<int>(dt.year() % 100));
    return buf;
}

std::string ddmon(const boost::gregorian::date &d, Language lang)
{
    static const char* months[2][12] = {
        { "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" },
        { "ЯНВ", "ФЕВ", "МАР", "АПР", "МАЙ", "ИЮН", "ИЮЛ", "АВГ", "СЕН", "ОКТ", "НОЯ", "ДЕК" }
    };
    const boost::gregorian::date& dt = resolveSpecials(d);
    if (dt.is_not_a_date()) return std::string();
    char buf[6] = {0};
    std::snprintf(buf, 6, "%2.2i%s", static_cast<int>(dt.day()), months[lang][dt.month()-1]);
    return buf;
}

std::string ddmmyyyy(const boost::gregorian::date &d, bool delimeter)
{
    const boost::gregorian::date& dt = resolveSpecials(d);
    if (dt.is_not_a_date()) return std::string();

    char buf[11] = {0};
    std::sprintf(buf, delimeter ? "%2.2i.%2.2i.%4.4i" : "%2.2i%2.2i%4.4i", static_cast<int>(dt.day()), static_cast<int>(dt.month()), static_cast<int>(dt.year()));
    return buf;
}

std::string yyyymmdd(const boost::gregorian::date &d, bool delimeter)
{
    const boost::gregorian::date& dt = resolveSpecials(d);
    if (dt.is_not_a_date()) return std::string();

    char buf[11] = {0};
    std::sprintf(buf,"%4.4i%s%2.2i%s%2.2i", static_cast<unsigned>(dt.year()%10000), delimeter? "." : "" , static_cast<unsigned>(dt.month()%100), delimeter ? "." : "", static_cast<unsigned>(dt.day()%100));
    return buf;
}

std::string mmyy(const boost::gregorian::date& date)
{
    if (date.is_not_a_date()) return string();

    const boost::gregorian::date& dt = Dates::resolveSpecials(date);
    ostringstream os;
    os << setw(2) << setfill('0') << (unsigned)dt.month()
       << setw(2) << setfill('0') << dt.year() % 100;
    return os.str();
}

std::string yymm(const boost::gregorian::date& date)
{
    if (date.is_not_a_date()) return string();

    const boost::gregorian::date& dt = Dates::resolveSpecials(date);
    ostringstream os;
    os << setw(2) << setfill('0') << dt.year() % 100
       << setw(2) << setfill('0') << (unsigned)dt.month();

    return os.str();
}

std::string to_iso_string(const boost::gregorian::date& d)
{
    return boost::gregorian::to_iso_string(d);
}

std::string to_iso_extended_string(const boost::gregorian::date& d)
{
    return boost::gregorian::to_iso_extended_string(d);
}

std::string ddmonyyyy(const boost::gregorian::date& date, Language lang)
{
    char mon[4]={};
    if (date.is_not_a_date()) return string();

    const boost::gregorian::date& dt = Dates::resolveSpecials(date);
    ostringstream os;
    os << setw(2) << setfill('0') << dt.day()
       << Dates::getMonthName(dt.month(), mon, lang)
       << dt.year();
    return os.str();
}

std::string hhmi(const boost::posix_time::time_duration& time)
{
    return hh24mi(time, false);
}

string hhmiss(const time_duration &time)
{
    return hh24miss(time, false);
}

boost::gregorian::date rrmmdd(const char* s)
{
    try {
        return boost::gregorian::date(2000+10*s[0]+s[1]-11*'0', 10*s[2]+s[3]-11*'0', 10*s[4]+s[5]-11*'0');
    } catch (const boost::bad_lexical_cast&) {
        return boost::gregorian::date(boost::gregorian::not_a_date_time);
    } catch (const std::out_of_range& e) {
        return boost::gregorian::date(boost::gregorian::not_a_date_time);
    }
}

boost::gregorian::date rrmmdd(const std::string &d)
{
  if(d.size() != 6)
      return boost::gregorian::date();
  return rrmmdd(d.c_str());
}

boost::gregorian::date ddmmyyyy(const char* input)
{
  if(strlen(input) != 8)
      return boost::gregorian::date();
  int n;
  unsigned int yyyy, mm, dd;
  if(sscanf(input, "%2u%2u%4u%n", &dd, &mm, &yyyy, &n) <= 0)
      return boost::gregorian::date();
  if(n != 8)
      return boost::gregorian::date();

  return boost::gregorian::date(yyyy, mm, dd);
}

boost::gregorian::date ddmmyyyy(const std::string& d)
{
  return ddmmyyyy(d.c_str());
}

boost::posix_time::ptime ptime_from_undelimited_string(const std::string& input)
{
  if(input.size() != 14)
      return boost::posix_time::not_a_date_time;
  return ptime_from_undelimited_string(input.c_str());
}

boost::gregorian::date date_from_iso_string(const std::string& s)
{
    try {
        return boost::gregorian::date_from_iso_string(s);
    } catch (const boost::bad_lexical_cast&) {
        return boost::gregorian::date(boost::gregorian::not_a_date_time);
    } catch (const std::out_of_range& e) {
        return boost::gregorian::date(boost::gregorian::not_a_date_time);
    }
}

boost::posix_time::time_duration duration_from_string(const std::string& s)
{
    try {
        return boost::posix_time::duration_from_string(s);
    } catch (const boost::bad_lexical_cast&) {
        return boost::posix_time::not_a_date_time;
    } catch (const std::out_of_range& e) {
        return boost::posix_time::not_a_date_time;
    }
}

boost::posix_time::ptime time_from_iso_string(const std::string& s)
{
    try {
        return boost::posix_time::from_iso_string(s);
    } catch (const boost::bad_lexical_cast&) {
        return boost::posix_time::not_a_date_time;
    } catch (const std::out_of_range& e) {
        return boost::posix_time::not_a_date_time;
    }
}

boost::posix_time::ptime ptime_from_undelimited_string(const char* s)
{ 
  tm pt_tm;
  pt_tm.tm_year = 1000*s[0]+100*s[1]+10*s[2]+s[3] - 1111*'0' - 1900;
  pt_tm.tm_mon  = 10*s[4]+s[5] - 11*'0' - 1;
  pt_tm.tm_mday = 10*s[6]+s[7] - 11*'0';
  pt_tm.tm_hour = 10*s[8]+s[9] - 11*'0';
  pt_tm.tm_min  = 10*s[10]+s[11] - 11*'0';
  pt_tm.tm_sec  = 10*s[12]+s[13] - 11*'0';
  try {
    return boost::posix_time::ptime_from_tm(pt_tm);
  } catch (const std::out_of_range& e) {
    return boost::posix_time::not_a_date_time;
  }
}

boost::gregorian::date ddmmrr(const std::string &d)
{
    if(d.size() != 6)
        return boost::gregorian::date();
    const std::string dd=d.substr(4,2)+d.substr(2,2)+d.substr(0,2);
    return rrmmdd(dd);
}

bool ptime_to_undelimited_string(const boost::posix_time::ptime& p, char* out)
{
  if(p.is_special())
  {
      *out = '\0';
      return false;
  }

  const boost::gregorian::date d = p.date();
  const boost::posix_time::time_duration t = p.time_of_day();
  unsigned int day=d.day(), month=d.month(), year=d.year();
  unsigned int hours=t.hours(), mins=t.minutes(), secs=t.seconds();

  out[0] = '0' + year/1000%10;
  out[1] = '0' + year/100%10;
  out[2] = '0' + year/10%10;
  out[3] = '0' + year%10;
  out[4] = '0' + month/10%10;
  out[5] = '0' + month%10;
  out[6] = '0' + day/10%10;
  out[7] = '0' + day%10;
  out[8] = '0' + hours/10%10;
  out[9] = '0' + hours%10;
  out[10]= '0' + mins/10%10;
  out[11]= '0' + mins%10;
  out[12]= '0' + secs/10%10;
  out[13]= '0' + secs%10;
  out[14]= '\0';

  return true;
}

std::string ptime_to_undelimited_string(const boost::posix_time::ptime& p)
{
  char buf[15] = {0};
  if(ptime_to_undelimited_string(p,buf))
      return buf;
  return std::string();
}

std::string hh24mi(const boost::posix_time::time_duration& t, bool delimiter)
{
    auto s = hh24miss(t, delimiter);
    if(not s.empty())
        s.resize(s.size() - 2 - delimiter);
    return s;
}

std::string hh24miss(const boost::posix_time::time_duration& t, bool delimiter)
{
    if (t.is_not_a_date_time()) return string();
    auto s = delimiter ? to_simple_string(t) : to_iso_string(t);
    auto p = s.find_first_of(".,");
    if(p != s.npos)
        s.erase(p, s.npos);
    return s;
}

boost::posix_time::time_duration hh24mi(const std::string& s)
{
  if(s.find(':') != std::string::npos)
      return Dates::duration_from_string(s);
  else if(s.length()!=4)
      return boost::posix_time::not_a_date_time;
  else
      return Dates::duration_from_string(s.substr(0,2)+":"+s.substr(2));
}

boost::posix_time::time_duration hh24miss(const std::string& s)
{
  if(s.find(':') != std::string::npos)
      return Dates::duration_from_string(s);
  else if(s.length()!=6)
      return boost::posix_time::not_a_date_time;
  else
      return Dates::duration_from_string(s.substr(0,2)+":"+s.substr(2,2)+":"+s.substr(4));
}

boost::gregorian::date currentDate()
{
    return currentDateTime().date();
}

boost::posix_time::ptime currentDateTime()
{
  boost::posix_time::ptime pt = boost::posix_time::second_clock::local_time();
#ifdef XP_TESTING
  if (TNowOffset::get() != 0)
      pt = ( LocalTime(pt, TNowOffset::get_tz_string()) + boost::posix_time::seconds(TNowOffset::get()) ).getBoostLocalTime();
#endif
  return pt;
}

boost::posix_time::ptime currentDateTime_us()
{
  boost::posix_time::ptime pt = boost::posix_time::microsec_clock::local_time();
#ifdef XP_TESTING
  if (TNowOffset::get() != 0)
  {
      const boost::posix_time::time_duration& td = pt.time_of_day();
      boost::posix_time::time_duration extra_ticks = td - boost::posix_time::seconds(td.total_seconds()) ;
      pt = ( LocalTime(pt, TNowOffset::get_tz_string()) + boost::posix_time::seconds(TNowOffset::get()) ).getBoostLocalTime() + extra_ticks;
  }
#endif
  return pt;
}


int getTotalMinutes(const boost::posix_time::time_duration& time)
{
    if (time.is_special()) return 0;
    return time.total_seconds() / 60;
}

//смещение в днях относительно даты вылета рейда
boost::gregorian::date_duration daysOffset(const boost::posix_time::time_duration& time)
{
    if (time.is_special()) return boost::posix_time::not_a_date_time;
    int offset = getTotalMinutes(time) / minutesInDay;
    if (time.is_negative()) {
        return boost::gregorian::days(offset - 1);
    } else {
        return boost::gregorian::days(offset);
    }
}

//смещение в минутах относительно начала текущих суток
boost::posix_time::time_duration dayTime(const boost::posix_time::time_duration& time)
{
    if (time.is_special()) return boost::posix_time::not_a_date_time;
    return time - hours(24 * daysOffset(time).days());
}

int getdayname(char *stout,int dn,int lan)
{
  if( (lan!=ENG && lan != RUS ) || dn < 1 || dn > 7 )
    return -1;

  strcpy(stout,DofW3[lan][dn-1]);
  return strlen(stout);
}

int day_abb(char *s, int n, int lan)
{
    s[0] = '\0';
    if (n < 1 || n > 7) {
        return -1;
    }
    strcpy(s, Days_Abb[lan ? RUSSIAN : ENGLISH][n - 1]);
    return 0;
}

std::string day_abb(int dayNo, Language lang, bool two_letter)
{
    if (dayNo >= 0 && dayNo < 7) {
        return std::string(Days_Abb[lang][dayNo], (two_letter ? 2 : 3));
    }
    return std::string();
}

std::string day_abb(const boost::gregorian::date& dt, Language lang, bool two_letter)
{
    return day_abb(day_of_week_ru(dt)-1, lang, two_letter);
}

std::string month_name(int monthNo, Language lang)
{
    if (monthNo < 1 || monthNo > 12) {
        return std::string();
    }
    return std::string(lang == RUSSIAN ? Mez[monthNo - 1].name : Mon[monthNo - 1].name);
}

std::string month_name(const boost::gregorian::date& dt, Language lang)
{
    return month_name(dt.month(), lang);
}

int getdayname2(char *stout,int dn,int lan)
{
  if( (lan!=ENG && lan != RUS ) || dn < 1 || dn > 7 )
    return -1;

  strcpy(stout,DofW2[lan][dn-1]);
  return strlen(stout);
}

int IsDayName(const char *stin,int stlen)
{
  int  i;
  int  lang;
  int  res=0;

  if (res==0 && stlen!=3 && stlen!=2)
    res=-1;
  lang=RUS;
  for (i=0; res==0 && i<7; i++)
    if ((stlen==2 && strncmp(stin,DofW2[lang][i],stlen)==0) ||
      (stlen==3 && strncmp(stin,DofW3[lang][i],stlen)==0))
      res=i+1;
  lang=ENG;
  for (i=0; res==0 && i<7; i++)
    if ((stlen==2 && strncmp(stin,DofW2[lang][i],stlen)==0) ||
      (stlen==3 && strncmp(stin,DofW3[lang][i],stlen)==0))
      res=i+1;
  res=(res==0?-1:res);
  ProgTrace(TRACE5,"res=%i",res);
  return res;
}

// adds n days to the rrmmdd data
void DayShift(char* out, const char* in, int n)
{
  if(n == 0)
  {
      strncpy(out, in, 6);
      out[6] = '\0';
      return;
  }

  static int days_in_month[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

  int y = 10*in[0]+in[1]-11*'0';
  int m = 10*in[2]+in[3]-11*'0';
  int d = 10*in[4]+in[5]-11*'0' + n;

  y += (y>50 ? 1900 : 2000);

  if(n > 0)
      while(d > days_in_month[m])
      {
          if(m == 2 and IsLeapYear(y))
          {
              if(d > 29)
                  d -= 1;
              else
                  break;
          }
          d -= days_in_month[m];
          m += 1;
          if(m > 12)
          {
              m = 1;
              y += 1;
          }
      }
  else
      while(d <= 0)
      {
          m -= 1;
          if(m <= 0)
          {
              m = 12;
              y -= 1;
          }
          d += days_in_month[m];
          if(m == 2 and IsLeapYear(y))
              d += 1;
      }

  out[0] = '0' + y%100/10;
  out[1] = '0' + y%10;
  out[2] = '0' + m/10;
  out[3] = '0' + m%10;
  out[4] = '0' + d/10;
  out[5] = '0' + d%10;
  out[6] = '\0';

  // Ненуачо. Спидап офигенный, дух арифметики на строках сохранён.
  // В продакшн. Проблем солвед. Некст.

  /*
  boost::gregorian::date d = Dates::rrmmdd(in) + boost::gregorian::days(n);

  unsigned int day=d.day(), month=d.month(), year=d.year();
  sprintf(out,"%2.2u%2.2u%2.2u",year%100,month,day);
  */
}

int Date2Int(const char *olddate) {
  static struct tm when;
  char y[5],m[3],d[3],h[3],mm[3];

  d[0]=y[0]=m[0]=h[0]=mm[0]=0;

  strncat(y,olddate+0,2);
  strncat(m,olddate+2,2);
  strncat(d,olddate+4,2);

  when.tm_sec=0;
  when.tm_min=0;
  when.tm_hour=0;
  when.tm_isdst=-1;
  when.tm_mday=atoi(d);
  when.tm_mon= atoi(m)-1;
  when.tm_year=atoi(y);

  if(when.tm_year<50) when.tm_year+=100;

  return (int)mktime(&when);
}

void prev_day(char *snd,const char *td)
{
 DayShift(snd,td,-1);
}

void next_day(char *snd,const char *td)
{
 DayShift(snd,td,1);
}

void FreqShift(char *newfreq,const char *oldfreq,int n)
{
    n %= 7;

    int p,kkk,bbb;
    strcpy(newfreq,".......");
    for(p=0;p<7;p++)
    {
        if(oldfreq[p]!='.')
        {
            bbb=((int)(oldfreq[p])) -0x30;
            kkk=bbb+n;
            if((bbb+n)>7)
                kkk=(bbb+n)-7;
            if((bbb+n)<=0)
                kkk=7+(bbb+n);
            newfreq[kkk-1]=kkk+0x30;
        }
    }
}

int fan(const char *freq,char *sf,int lng)
 {
  int i,j,np=0,nd=0;
  int pn[7];
  int dn[7];
  for(i=0;i<7;i++)
   if(freq[i]=='.')
    { pn[np]=i+1; np++; }
   else
    { dn[nd]=i+1; nd++;}
  if(nd==7)
   { if(!lng) { strcpy(sf,"EVRD"); return 4; }
         else{ strcpy(sf,"ЕЖД"); return 3;  }
   }
  if(nd<5)
   {
    strcpy(sf,"");
    for(j=0;j<nd;j++)
     sf[j]=0x30+dn[j];
    return nd;
   }
  if(nd>4)
   {
    if(!lng) strcpy(sf,"EX");else strcpy(sf,"КР");
    for(j=0;j<np;j++)
     sf[j+2]=0x30+pn[j];
    return (np+2);
   }
   return 0;
 }

int TimeSub(int t2,int t1)
{
  int c1,c2,m1,m2,m,c,t=0;
  c1=t1/100;
  c2=t2/100;
  m1=t1-(c1*100);
  m2=t2-(c2*100);

  c=c2-c1;
  m=m2-m1;

  if(c>0)
   {
    if(m>=0)
     { t=c*100;
       if(m<60) t+=m;
       else     t+=(100+m-60);
     }
    else
     {  t=(c-1)*100; m=m+60; t+=m; }
   }

  if(c<0)
   {
    if(m>0) { t=(c+1)*100; m=m-60; t+=m; }
    else /* m<=0 */
     {
      if(m>-60) { t=c*100; t+=m; }
      else { t=(c-1)*100; m+=60; t+=m; }
     }
   }
  if(c==0)
   { if(m>=60) t=100+m-60;
     if(m>-60&&m<60) t=m;
     if(m<=-60) t=-100+m+60;
   }
  return t;
}

int TimeAdd(int t1,int t2)
{
  return TimeSub(t1,-t2);
}

int Is_date(const char* s, int l)
{
    int val = 0;
    char sub[100] = {};
    if ((l == 1) && (s[0] != 'N') && (s[0] != 'T'))
        return 0;
    if (((s[0] == 'N') || (s[0] == 'T')) && (l > 1)) {
        if (s[1] != '+')
            return 0;
        if ((l > 5) || (l < 3))
            return 0;
        memcpy(sub, s + 2, (l - 2));
        sub[l - 2] = 0;
        val = atoi(sub);
        if ((val < 0) || (val > 330))
            return 0;
    }

    if ((s[0] != 'N') && (s[0] != 'T') && (l > 1)) {
        if (DATE6) {
            if ((l != 4) && (l != 6))
                return 0;
        } else {
            if ((l != 4) && (l != 8))
                return 0;
        }
        if (l+1 > (int)sizeof(sub)) {
            ProgError(STDLOG, "invalid date: %s len=%d", s, l);
            return 0;
        }

        memcpy(sub, s, l);
        sub[l] = 0;
        for (int i = 0; i < l; i++)
            if ((sub[i] < '0') || (sub[i] > '9'))
                return 0;

        memcpy(sub, s, 2);
        sub[2] = 0;
        val = atoi(sub);
        if ((val < 0) || (val > 31))
            return 0;
        memcpy(sub, s + 2, 2);
        sub[2] = 0;
        val = atoi(sub);
        if ((val < 1) || (val > 12))
            return 0;
        if (l != 4) {
            if (DATE6) {
                memcpy(sub, s + 4, 2);
                sub[2] = 0;
                val = atoi(sub);
                if ((val < 0) || (val > 99)) {
                    return 0;
                }
            } else {
                memcpy(sub, s + 4, 4);
                sub[4] = 0;
                val = atoi(sub);
                if ((val < 1900) || (val > 2099)) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

int Is_time(const char *s,int l)
{
 int val,i;
 char sub[5];
 if(TIME2) { if((l>2)||(l<2)) return 0; }
 else { if((l>4)||(l<2))  return 0; }

 memcpy(sub,s,l); sub[l]=0;
 for(i=0;i<l;i++) if((sub[i]<'0')||(sub[i]>'9'))  return 0;
 if(TIME2)
 {
  val=atoi(sub);
  if((val<0)||(val>23)) return 0;
 }
 else
 {
  memcpy(sub,s,(l-2)); sub[(l-2)]=0;
  val=atoi(sub);
  if((val<0)||(val>23)) return 0;
  memcpy(sub,s+(l-2),2); sub[2]=0;
  val=atoi(sub);
  if((val<0)||(val>59)) return 0;
 }
 return 1;
}

int GetFreq(char *freq,const char *s,int l)
{
 int i,b,k,l1;
 strcpy(freq,"1234567");
 b=-1;
 for(i=0;i<l;i++) if(s[i]!=' ') { b=i; break;}
 if(b==-1) { tst(); return (-1); }
 l1=l-1;
 for(;s[l1]==' ' && l1>=0; l1--);
 l1++;
 if((l1-b)<=0)
   { tst(); return -1; }
 if(((l1-b)==1)&&(s[0]=='8'||s[0]=='Е')) return 1;

 if( ((l1-b)>2)&&(s[b]=='К')&&(s[b+1]=='Р') )
 {
  for(i=b+2;i<l1;i++)
  {
   if(s[i]!=' '&&s[i]!='.')
   {
    k=(int)(s[i]-0x30);
    if((k<1)||(k>7)) return -1;
    freq[k-1]='.';
   }
  }
  if(strncmp(freq,".......",7)==0) return -1;
  return 1;
 }

 if(((l1-b)>1)&&(s[b]=='-'))
 {
  for(i=b+1;i<l1;i++)
  {
   if(s[i]!=' '&&s[i]!='.')
   {
    k=(int)(s[i]-0x30);
    if((k<1)||(k>7)) return -1;
    freq[k-1]='.';
   }
  }
  if(strncmp(freq,".......",7)==0) return -1;
  return 1;
 }

  if( ((l1-b)>=3)&&(s[b]=='Е')&&(s[b+1]=='Ж')&&(s[b+2]=='Д') )
   return 1;

  strcpy(freq,".......");

  for(i=0;i<l1;i++)
  {
   if(s[i]!=' '&&s[i]!='.')
   {
    k=(int)(s[i]-0x30);
    if((k<1)||(k>7)) return -1;
    freq[k-1]=0x30+k;
   }
  }
  return 1;
}

void date_cat(char *s1,const char *s2)
{
 strncat(s1,s2+4,2);
 strncat(s1,s2+2,2);
 strncat(s1,s2,2);
}

int  day_cpy(char *s2,const char *s1,int lan)
{
  int k;

  s2[0] = s1[4];
  s2[1] = s1[5];
  k = ( s1[2] - '0' ) * 10 + s1[3] - '0';
  if(k>=1 && k<=12) {
   if( lan ) {
    memcpy( &s2[2], Mez[ k - 1 ].name, 3 );
   } else {
    memcpy( &s2[2], Mon[ k - 1 ].name, 3 );
   }
  } else { /* не будем гулять по памяти, просто покажем что месяц кривой */
   memcpy( &s2[2],"XXX",3);
  }
  s2[5] = s1[0];
  s2[6] = s1[1];
  s2[7] = '\0';
  return 0;
}

std::string makeDate6(const boost::gregorian::date& d)
{
    return Dates::rrmmdd(d);
}
void makeDate6(char* dest, const boost::gregorian::date& d)
{
    strcpy(dest, Dates::rrmmdd(d).c_str());
}

std::string makeDate8(const boost::gregorian::date& d)
{
    if (d.is_not_a_date())  return std::string();
    std::string r = Dates::ddmmyyyy(d);
    return r.substr(4, 4) + r.substr(2,2) + r.substr(0,2);
}

void makeDate8(char *dest, const boost::gregorian::date& d)
{
    strcpy(dest, makeDate8(d).c_str());
}

std::string makeDateHuman6(boost::gregorian::date const &d, int lang)
{
    if (d.is_not_a_date())  return std::string();

    const std::string& r = Dates::ddmonrr(d, static_cast<Language>(lang));
    return r.substr(0,2) + "-" + r.substr(2,3) + "-" + r.substr(5, 2);
}

std::string makeDateHuman8(boost::gregorian::date const &d, int lang)
{
    if (d.is_not_a_date())  return std::string();

    const boost::gregorian::date& dt = Dates::resolveSpecials(d);
    const std::string& r = Dates::ddmonrr(dt, static_cast<Language>(lang));
    char buf[12] = {};
    sprintf(buf, "%s-%s-%4.4i", r.substr(0,2).c_str(), r.substr(2,3).c_str(), static_cast<int>(dt.year()));
    return buf;
}

int DatNotCont(const char *s)
{
  char dt[9],yy[9];
  int k,m;
  today_plus(0,dt);
  yy[0]=0; strncat(yy,s,2);
  m=atoi(yy);
  k=DateMinus(s,dt);
  ProgTrace(TRACE3,"^^^^r=%d~~~~~~",k);
  if((k<-LOOK_BACK)||(k>330)) { return 1;}
  if((k==0)&&(m<70)&&(m>=50)) { return 1; }
  return 0;
}

int DatNotCont(const boost::gregorian::date& d)
{
    if(d.is_not_a_date())
        return 0;
    return DatNotCont(makeDate6(d).c_str());
}

int VerDate(char *Bdate,const char *s,int l)
{
  int i,dn,mn,yn,p=0,cyn,cmn,cdn;
  char sub[4],tmp[9];

  for(i=0;i<l;i++)
  if((s[i]>'9')||(s[i]<'0')) { p=i; break; }
  if(p>2) return -1;
  sub[0]=0; strncat(sub,s,p);
  if((l-p)>3) return -1;
  dn=atoi(sub);
  if((dn<1)||(dn>31)) return -1;
  sub[0]=0; strncat(sub,s+p,3);
  mn=0;
  for(i=0;i<12;i++)
  if(strncmp(sub,Mon[i].name,3)==0) { mn=Mon[i].number; p=i; break; }
  if(mn==0) return -1;
  if(dn>Mon[p].ndays) return -1;
  today_plus(0,tmp);
  sub[0]=0; strncat(sub,tmp  ,2); cyn=atoi(sub);
  sub[0]=0; strncat(sub,tmp+2,2); cmn=atoi(sub);
  sub[0]=0; strncat(sub,tmp+4,2); cdn=atoi(sub);
  if(cmn>mn) yn=cyn+1;
  else { if((cmn==mn)&&(cdn>dn)) yn=cyn+1; else yn=cyn;}
  if(yn>=100) yn=(yn%100);
  if( ((yn%4)!=0)&&(mn==2)&&(dn>28) ) return -1;
  sprintf(Bdate,"%.2d%.2d%.2d",yn,mn,dn);
  return 1;
  }

int VecDate(char *Bdate,char *s,int l)
{
  int i,dn,mn,yn,p=0,cyn,cmn,cdn;

  char sub[4],tmp[9];

  for(i=0;i<l;i++)
  if((s[i]>'9')||(s[i]<'0')) { p=i; break; }

  if(p>2) return -1;

  sub[0]=0; strncat(sub,s,p);

  if((l-p)>3) return -1;

  dn=atoi(sub);
  if((dn<1)||(dn>31)) return -1;

  sub[0]=0; strncat(sub,s+p,3);
  mn=0;
  for(i=0;i<12;i++)
  if(strncmp(sub,Mez[i].name,3)==0) { mn=Mez[i].number; p=i; break; }
  if(mn==0) return -1;
  if(dn>Mez[p].ndays) return -1;
  today_plus(0,tmp);
   sub[0]=0; strncat(sub,tmp,2); cyn=atoi(sub);
   sub[0]=0; strncat(sub,tmp+2,2); cmn=atoi(sub);
   sub[0]=0; strncat(sub,tmp+4,2); cdn=atoi(sub);
  if(cmn>mn) yn=cyn+1;
  else { if((cmn==mn)&&(cdn>dn)) yn=cyn+1; else yn=cyn;}
  if(yn>=100) yn=(yn%100);
  if( ((yn%4)!=0)&&(mn==2)&&(dn>28) ) return -1;
  sprintf(Bdate,"%.2d%.2d%.2d",yn,mn,dn);
  return 1;
}

int Is_fulltime(const char *s,int l)
{
 int val,i;
 char sub[7];

 if(l>6) return 0;

 memcpy(sub,s,l); sub[l]=0;
 for(i=0;i<l;i++) if((sub[i]<'0')||(sub[i]>'9'))  return 0;

 memcpy(sub,s,2); sub[2]=0;
 val=atoi(sub);
 if((val<0)||(val>23)) return 0;
 memcpy(sub,s+2,2); sub[2]=0;
 val=atoi(sub);
 if((val<0)||(val>59)) return 0;
 memcpy(sub,s+4,2); sub[2]=0;
 val=atoi(sub);
 if((val<0)||(val>59)) return 0;
 return 1;
}

int D2_D1(const char *date2,const char *date1)
{
   static struct tm tb2,tb1;
   char y[5],m[3],d[3],h[3],mi[3],s[3];
   time_t t1,t2,dt;
   int n;
   ProgTrace(TRACE4,"d1=%s d2=%s =",date1,date2);

   sprintf(y,"%2.2s",date1);
   sprintf(m,"%2.2s",date1+2);
   sprintf(d,"%2.2s",date1+4);
   sprintf(h,"%2.2s",date1+6);
   sprintf(mi,"%2.2s",date1+8);
   sprintf(s,"%2.2s",date1+10);
   ProgTrace(TRACE4,"y=%s m=%s d=%s h=%s mi=%s s=%s",
   y,m,d,h,mi,s);

   tb1.tm_sec=atoi(s);  tb1.tm_min=atoi(mi);  tb1.tm_hour=atoi(h);
   tb1.tm_isdst=-1; tb1.tm_mday=atoi(d);   tb1.tm_mon =atoi(m)-1;
   tb1.tm_year=atoi(y);
   if(tb1.tm_year<50) tb1.tm_year+=100;
   ProgTrace(TRACE4,"y=%d m=%d d=%d h=%d mi=%d s=%d",
   tb1.tm_year,tb1.tm_mon,tb1.tm_mday,tb1.tm_hour,tb1.tm_min,tb1.tm_sec);
   if( (t1=mktime(&tb1))==((time_t)-1) )
   {
    WriteLog(STDLOG,"D2-D1() : mktime failed");
    return -25000;
   }
   d[0]=0;y[0]=0;m[0]=0;
   sprintf(y,"%2.2s",date2);
   sprintf(m,"%2.2s",date2+2);
   sprintf(d,"%2.2s",date2+4);
   sprintf(h,"%2.2s",date2+6);
   sprintf(mi,"%2.2s",date2+8);
   sprintf(s,"%2.2s",date2+10);

   ProgTrace(TRACE4,"y=%s m=%s d=%s h=%s mi=%s s=%s",
   y,m,d,h,mi,s);
   tb2.tm_sec=atoi(s);  tb2.tm_min=atoi(mi);  tb2.tm_hour=atoi(h);
   tb2.tm_isdst=-1;
   tb2.tm_mday=atoi(d);   tb2.tm_mon =atoi(m)-1; tb2.tm_year=atoi(y);
   if(tb2.tm_year<50) tb2.tm_year+=100;
   ProgTrace(TRACE4,"y=%d m=%d d=%d h=%d mi=%d s=%d",
   tb2.tm_year,tb2.tm_mon,tb2.tm_mday,tb2.tm_hour,tb2.tm_min,tb2.tm_sec);

   if( (t2=mktime(&tb2))==((time_t)-1) )
   { WriteLog(STDLOG,"D2-D1(): mktime failed");
     return +25000;
   }

   dt=t2-t1;

   n=dt/84600;
   return n;
   if(dt>0) return 1;
    else if(dt==0) return 0;
          else return -1;
 }


long MonMinus(const boost::gregorian::date& d1, const boost::gregorian::date& d2)
{
    return (d1.year() - d2.year()) * 12 + d1.month().as_number() - d2.month().as_number();
}

int MonMinus(const char *date2,const char *date1)
{
 int y1,y2,m1,m2,d1,d2,res;
 char tmp[10];

 tmp[0]=0; strncat(tmp,date1,2); y1=atoi(tmp); if(y1<50) y1+=100;
 tmp[0]=0; strncat(tmp,date2,2); y2=atoi(tmp); if(y2<50) y2+=100;

 tmp[0]=0; strncat(tmp,date1+2,2); m1=atoi(tmp);
 tmp[0]=0; strncat(tmp,date2+2,2); m2=atoi(tmp);

 tmp[0]=0; strncat(tmp,date1+4,2); d1=atoi(tmp);
 tmp[0]=0; strncat(tmp,date2+4,2); d2=atoi(tmp);

 res=0;
 if(y2!=y1) res+=(12*(y2-y1));
 if(m2!=m1) res+=m2-m1;
 if(res<0) res=(-1)*MonMinus(date1,date2);
 else
 {
  int maxd1=Mon[m1-1].ndays;
  if((m1==2)&&((y1%4)!=0)) maxd1--;
  int maxd2=Mon[m2-1].ndays;
  if((m2==2)&&((y2%4)!=0)) maxd2--;

  if(d2<d1)
  {
   ProgTrace(TRACE4,"d2=%d d1=%d m2=%d maxd1=%d maxd2=%d\n",d2,d1,m2,maxd1,maxd2);
   if( ((d1==maxd1)&&(d2==maxd2))||((d1<maxd1)&&(d1>maxd2)&&(d2==maxd2)) ) tst();
   else if((res>0)&&(res<2)) res--;
  }
  if(d2>d1)
  {
   ProgTrace(TRACE4,"d2=%d d1=%d m2=%d maxd1=%d maxd2=%d\n",d2,d1,m2,maxd1,maxd2);
   if( ((d1==maxd1)&&(d2==maxd2))||((d2<maxd2)&&(d2>maxd1)&&(d1==maxd1)) ) tst();
   else if(res>0) res++;
  }
 }
 ProgTrace(TRACE3,"d1=%d d2=%d m1=%d m2=%d y1=%d y2=%d %.6s-%.6s=%d mes\n",
 d1,d2,m1,m2,y1,y2,date2,date1,res);
 return res;
}

int MonShift(char *newdate,const char *olddate,int n)
{
 int y1,y2,m1,m2,d1,d2,maxd1,maxd2;
 char tmp[10];

 tmp[0]=0; strncat(tmp,olddate+0,2); y1=atoi(tmp);
 if(y1<50) y1+=100;
 tmp[0]=0; strncat(tmp,olddate+2,2); m1=atoi(tmp);
 tmp[0]=0; strncat(tmp,olddate+4,2); d1=atoi(tmp);
 m2=m1+n;
 y2=y1;
 d2=d1;
 if(m2>12)
 {
   y2+=((m2-1)/12);
   m2=((m2-1)%12)+1;
 }
 if(m2<=0)
 {
  y2+=((m2/12)-1);
  m2=((abs(m2/12)+1)*12)+m2;
 }

 maxd1=Mon[m1-1].ndays;
 if((m1==2)&&((y1%4)!=0)) maxd1--;

 maxd2=Mon[m2-1].ndays;
 if((m2==2)&&((y2%4)!=0)) maxd2--;

 ProgTrace(TRACE3,"d1=%d m1=%d y1=%d nm=%d  d2=%d m2=%d y2=%d md=%d",
 d1,m1,y1,n,d2,m2,y2,maxd2);
 if(d2>maxd2) d2=maxd2;
/* if(d1==maxd1) d2=maxd2; */
 sprintf(newdate,"%.2d%.2d%.2d",y2%100,m2,d2);
 return 0;
}

// сравнивает две yyyymmddhh24miss даты и возвращает -1/0/1
short CompareTimes(const char *dt1,const char *dt2)
{
  return std::strncmp(dt1, dt2, 14);
}

// сравнивает две rrmmddhh24miss даты и возвращает -1/0/1
short YYCompareTimes(const char *dt1,const char *dt2)
{
  int d1_50 = std::strncmp(dt1,"50",2); // -1  -> 20??
  int d2_50 = std::strncmp(dt2,"50",2); // 0,1 -> 19??
  if((d1_50<0) == (d2_50<0))
      return std::strncmp(dt1, dt2, 12);
  else
      return (d1_50 < 0 && d2_50 >=0) ? 1 : -1;
}

void fixTimeZone(int source_tz,int result_tz,
      int source_time,char const *source_date,
      int * result_time, char *result_date)
{

    *result_time=TimeSub(source_time, (source_tz -result_tz)*100) ;
    ProgTrace(TRACE3,"sour_time=%d source_tz=%d result_tz=%d res_time=%d",
    source_time,source_tz,result_tz,*result_time);

    if(*result_time<0){
      DayShift(result_date,source_date,-1);
      *result_time=TimeAdd(*result_time,2400);
    }else if(*result_time >2400){
      DayShift(result_date,source_date,1);
      *result_time=TimeAdd(*result_time,-2400);
    }else{
      strcpy(result_date,source_date);
    }
}

std::string get2CMonthNum(const std::string &mon_str)
{
  int i;
  for(i=0;i<12;i++)
  {
    if(mon_str==Mon[i].name)
      break;
    if(mon_str==Mez[i].name)
      break;
  }
  if(i<12)
  {
    char str[3];
    sprintf(str,"%02d",i);
    return str;
  }
  return "";
}

boost::posix_time::time_duration from_sirena_time(std::string const &s)
{
    return from_sirena_time(s.c_str());
}
boost::posix_time::time_duration from_sirena_time(char const *s)
{
    int t=boost::lexical_cast<int>(s);
    return  boost::posix_time::time_duration (t/100,
                        t%100,0,0);
}
boost::gregorian::date from_sirena_date(char const * chardate)
{
    // special speed up
    const size_t len = strnlen(chardate, 9);
    short year = 0, month = 0, day = 0, p = 0;
    switch (len) {
    case 6: {
        year = (boost::gregorian::day_clock::local_day().year() / 100) * 100
            + (chardate[0] - '0') * 10 + (chardate[1] - '0');
        p = 1;
        break;
    }
    case 8: {
        year = (chardate[0] - '0') * 1000 + (chardate[1] - '0') * 100
            + (chardate[2] - '0') * 10 + (chardate[3] - '0');
        p = 3;
        break;
    }
    case 0: {
        return boost::gregorian::date();
    }
    default:
        ProgError(STDLOG, "invalid date str of len %zu: %9.9s", len, chardate);
        abort();
    }
    month = (chardate[p+1] - '0') * 10 + (chardate[p+2] - '0');
    day = (chardate[p+3] - '0') * 10 + (chardate[p+4] - '0');
    return boost::gregorian::date(year, month, day);
}
boost::gregorian::date from_sirena_date(std::string const & chardate)
{
    return from_sirena_date(chardate.c_str());
}
boost::posix_time::ptime from_sirena_date_and_time(const char *dt, const char *tm)
{
  boost::posix_time::ptime t(Dates::from_sirena_date(dt));
  if(!t.is_special() && tm[0] != '\0' && tm[0] != '-')
  {
    t += Dates::from_sirena_time(tm);
  }
  return t;
}
void ChangeTime(const char *dt1, int delta, char *dt2)
/* Adds to dt1 time (YYYYMMDDHH24MISS) delta seconds and write it into
   dt2 (YYYYMMDDHH24MISS)
   dt1 == dt2 - you can do it */
{
  //ProgTrace(TRACE5,"dt1='%s' delta=%i",dt1,delta);

  if(delta!=0)
  {
    struct tm tt;
    time_t dd,dl;

    dd=(time_t)maketm(dt1,&tt);
    dl=dd+delta;
    if( strftime(dt2,15,"%Y%m%d%H%M%S",localtime(&dl))==0)
    {
      ProgError( STDLOG, "...  Error change time ... ");
      strcpy(dt2,D_MIN_DATE);
    };
  }
  else
    strcpy(dt2,dt1);
  //ProgTrace(TRACE5,"dt1='%s' dt2='%s'",dt1,dt2);
}

int CorrectDate(int day, int mon, int year)
{
    if(year>=0 && year < 50)
        year+=2000;
    else if(year>=50 && year < 100)
        year+=1900;

    if(year<1000  || mon<1 || mon >12 || day <1)
        return -1;
    if(mon !=2)
        return day > Mon[mon-1].ndays ? -1 : 0;

    return   day > (IsLeapYear(year) ? 29:28)  ? -1 : 0;
}



int aTime2int( const char *t)
{
    /*ROMAN: add hours*/
    return ((t[0]-'0')*10+(t[1]-'0'))*3600+((t[2]-'0')*10+(t[3]-'0'))*60+(t[4]-'0')*10+(t[5]-'0');
}
void iTime2ascii( int in, char * s)
{
    int h = in/3600, sec=in%3600;
    if(s) {
            sprintf(s, "%.2d%.2d%.2d", h, sec/60, sec%60);
    }
}

/*
 By Roman
 Add to date n seconds
 RRMMDDHH24MISS
 */
void  DayShiftSec(char *newdate,const char *olddate,int n)
{
    int oldsec = aTime2int(olddate+6);
    int adddays;
    ProgTrace(TRACE3,"date in %s, add seconds %d", olddate,n);
    oldsec += n;
    adddays = oldsec/86400;
    if(adddays){
    char date_tmp[6+1];
        strncpy(date_tmp, olddate, 6);
        DayShift(newdate, olddate, adddays);
    }else {
        strncpy(newdate, olddate, 6);
    }

    iTime2ascii(oldsec%86400, newdate+6);
    ProgTrace(TRACE3,"date out %s", newdate);
}

long ADayNum(const char * s)
{
    int d, m, y;
    int l = strlen(s)==8;
    if(l){
        sscanf(s,"%4d",&y);
        s+=4;
    }else{
        sscanf(s,"%2d",&y);
        s+=2;
    }
    sscanf(s,"%2d",&m);
    sscanf(s,"%2d",&d);
    return DayNum(d,m,y);
}
long DayNum(int day, int month, int year)
{
    int d1=0,i,L;
    L=IsLeapYear(year);
    month--;
    if (month!=1){
        if (day > Mon[month].ndays)
            return -1;
    }
    else{
        if (day >  Mon[month].ndays + L )
        return -1;
    }
    for (i=0;i<month;i++){
        d1+=Mon[i].ndays + (i==1 ? L:0);
    }
    d1+=day-1;
    d1+=365*(--year);
    d1+=year/4-year/100+year/400;
    return d1;
}
#define START 1904

/*input:
    src: mmdd
  output:
    dst: ddmmm
*/
void MakeFaceDate(const char *src, char *dst)
{
    int month;
    if(*src)
    {
        strncpy(dst,src+2,2);
        sscanf(src,"%2d",&month);
        getMonthName(month,dst+2,ENGLISH);
    }
    else
        *dst=0;
}

int CompleteDate(char *out, struct tm *ptm,int day,int mon)
{
    char out2[10];
    int rc;

    rc=CompleteDate1(out2,ptm,day,mon);
    if (rc<0)
        return rc;
    strcpy(out,out2+2);
    return rc;
}

int CompleteDate1(char *out, struct tm *ptm,int day,int mon)
{
    long d1,d2;

    int nextyear=0,year1=0;

    if((d1=DayNum(day,mon,ptm->tm_year+1900))-
            (d2=DayNum(ptm->tm_mday,ptm->tm_mon+1,
                ptm->tm_year+1900)) < -30){
        nextyear=1;
    }else if(d1-d2 > 330){
        nextyear=-1;
    }

    year1=1900+ptm->tm_year+nextyear;
    sprintf(out,"%04d%02d%02d",year1,mon,day);
    ProgTrace(TRACE5,"date=%s",out);
    if(CorrectDate(day,mon,year1)<0 ){
        return -1;
    }
    return 0;
}



int YYYYfromYY(int yy)
{
    if (yy < 0 || yy > 99) {
        ProgTrace(TRACE1,"InvalidYY: (%i)", yy);
        throw std::runtime_error("Invalid year in YY format was provided");
    }
    return (yy >= 50) ? (1900 + yy) : (2000 + yy);
}

int YYYYfromYY(const char *yy)
{
    if (!yy || strlen(yy) != 2) {
        ProgTrace(TRACE1, "InvalidYY: '%s'", yy);
        throw std::runtime_error("Invalid year in YY format was provided");
    }
    return YYYYfromYY(atoiNVL(yy, -1));
}

std::string normalizeDate(const std::string &date)
{
    if (date.size() != 6) {
        ProgTrace(TRACE1, "Invalid date: '%s'", date.c_str());
        throw std::runtime_error("Invalid date format instead of yymmdd was provided");
    }
    std::string result = date;
    std::string YY = result.substr(0, 2);
    std::string YYYY = boost::lexical_cast<std::string>(YYYYfromYY(YY.c_str()));
    result.replace(0, 2, YYYY);
    return result;
}


/* В случае ошибки возвращает not-a-date */
boost::gregorian::date MakeGregorianDate(unsigned year, unsigned month, unsigned day)
{
    try {
        return boost::gregorian::date(year, month, day);
    } catch (const boost::gregorian::bad_year&) {
        return boost::gregorian::date();
    } catch (const boost::gregorian::bad_month&) {
        return boost::gregorian::date();
    } catch (const boost::gregorian::bad_day_of_month&) {
        return boost::gregorian::date();
    }
}

/*
 * Преобразование трехбуквенного русского или латинского кода месяца
 * в его номер. Функция нечувствительна к регистру букв.
 */
static unsigned MonthNumber_CaseInsensitive(std::string month)
{
    StrUtils::UpperCase(month);
    return Dates::getMonthNum(month);
}

static string DayMonth(unsigned day, unsigned month, Language lang)
{
    char mon[4] = {};
    ostringstream os;
    os << setw(2) << setfill('0') << day << Dates::getMonthName(month, mon, lang);
    return os.str();
}
/*
 * If current year = 2000:
 * 99 -> 2099,
 * 01 -> 2001
 */
boost::gregorian::date YY2YYYY_UseCurrentCentury(
        unsigned yy, unsigned month, unsigned day,
        boost::gregorian::date currentDate)
{
    if (yy > 99) return boost::gregorian::date();
    unsigned year = (currentDate.year() - currentDate.year() % 100) + yy;
    return MakeGregorianDate(year, month, day);
}

/*
 * If current year = 2000:
 * 99 -> 1999,
 * 01 -> 2001
 */
boost::gregorian::date YY2YYYY_WraparoundFutureDate(
        unsigned yy, unsigned month, unsigned day,
        boost::gregorian::date currentDate)
{
    boost::gregorian::date date = YY2YYYY_UseCurrentCentury(yy, month, day, currentDate);
    if (date.is_not_a_date()) return date;

    return date > currentDate ?
        date - boost::gregorian::years(100) :
        date;
}

/*
 * If current year = 2000:
 * 49 -> 2049,
 * 50 -> 1950,
 * 01 -> 2001,
 */
boost::gregorian::date YY2YYYY_Wraparound50YearsFutureDate(
        unsigned yy, unsigned month, unsigned day,
        boost::gregorian::date currentDate)
{
    boost::gregorian::date date = YY2YYYY_UseCurrentCentury(yy, month, day, currentDate);
    if (date.is_not_a_date()) return date;
    return date >= currentDate + boost::gregorian::years(50) ?
        date - boost::gregorian::years(100) :
        date;
}

/* В случае ошибки возвращает not-a-date */
boost::gregorian::date DateFromDDMONYY(
        const string& dateStr,
        YY_to_YYYY yy2yyyy,
        boost::gregorian::date currentDate)
{
    unsigned day = 0, year = 0;
    char monthStr[8] = { 0 };
    int n = 0;
    if (dateStr.size() != 7
        || sscanf(dateStr.c_str(), "%2u%3s%2u%n", &day, monthStr, &year, &n) != 3
        || n != 7)
    {
        return boost::gregorian::date();
    }
    unsigned month = MonthNumber_CaseInsensitive(dateStr.substr(2, 3));
    if (month == 0) return boost::gregorian::date();
    return yy2yyyy(year, month, day, currentDate);
}

boost::gregorian::date DateFromDDMMYY(
        const string& dateStr,
        YY_to_YYYY yy2yyyy,
        boost::gregorian::date currentDate)
{
    unsigned year = 0, month = 0, day = 0;
    int n = 0;
    if (dateStr.size() != 6
        || sscanf(dateStr.c_str(), "%2u%2u%2u%n", &day, &month, &year, &n) != 3
        || n != 6)
    {
        return boost::gregorian::date();
    }
    return yy2yyyy(year, month, day, currentDate);
}

boost::gregorian::date DateFromDD_MM_YY(
        const std::string& dateStr,
        YY_to_YYYY yy2yyyy,
        boost::gregorian::date currentDate)
{
    unsigned year = 0, month = 0, day = 0;
    int n = 0;
    if (dateStr.size() != 8
        || sscanf(dateStr.c_str(), "%2u.%2u.%2u%n", &day, &month, &year, &n) != 3
        || n != 8)
    {
        return boost::gregorian::date();
    }
    return yy2yyyy(year, month, day, currentDate);
}

boost::gregorian::date DateFromYYMMDD(
        const string& dateStr,
        YY_to_YYYY yy2yyyy,
        boost::gregorian::date currentDate)
{
    unsigned year = 0, month = 0, day = 0;
    int n = 0;
    if (dateStr.size() != 6
        || sscanf(dateStr.c_str(), "%2u%2u%2u%n", &year, &month, &day, &n) != 3
        || n != 6)
    {
        return boost::gregorian::date();
    }
    return yy2yyyy(year, month, day, currentDate);
}

boost::gregorian::date DateFromYYYYMMDD(const char* dateStr, unsigned size)
{
    unsigned day = 0, month = 0, year = 0;
    int n = 0;
    if (8 != size
        || sscanf(dateStr, "%4u%2u%2u%n", &year, &month, &day, &n) != 3
        || n != 8)
    {
        return boost::gregorian::date();
    }
    return MakeGregorianDate(year, month, day);
}

boost::gregorian::date DateFromYYYYMMDD(const std::string& dateStr)
{
    return DateFromYYYYMMDD(dateStr.c_str(), dateStr.size());
}


boost::gregorian::date DateFromDDMONYYYY(const std::string& dateStr)
{
    unsigned day = 0, year = 0;
    char monthStr[8] = { 0 };
    int n = 0;

    if (dateStr.size() != 9
        || sscanf(dateStr.c_str(), "%2u%3s%4u%n", &day, monthStr, &year, &n) != 3
        || n != 9)
    {
        return boost::gregorian::date();
    }
    unsigned month = MonthNumber_CaseInsensitive(dateStr.substr(2, 3));
    if (month == 0) return boost::gregorian::date();
    return MakeGregorianDate(year, month, day);
}

boost::gregorian::date DateFromDD_MM_YYYY(const std::string& dateStr)
{
    unsigned day = 0, month = 0, year = 0;
    int n = 0;

    if (dateStr.size() != 10
        || sscanf(dateStr.c_str(), "%2u.%2u.%4u%n", &day, &month, &year, &n) != 3
        || n != 10)
    {
        return boost::gregorian::date();
    }

    return MakeGregorianDate(year, month, day);
}

 /*
 * Функция допускает время в формате HHMM
 * В случае ошибки возвращает not-a-date-time.
 */
boost::posix_time::time_duration TimeFromHHMM(const char* timeStr, unsigned size)
{
    unsigned h = 0, m = 0;
    int n = 0;
    if (4 != size
        || sscanf(timeStr, "%2u%2u%n", &h, &m, &n) != 2
        || n != 4
        || h > 23
        || m > 59)
    {
        return boost::posix_time::time_duration(boost::posix_time::not_a_date_time);
    }
    return boost::posix_time::time_duration(h, m, 0, 0);
}

boost::posix_time::time_duration TimeFromHHMM(const std::string& timeStr)
{
    return TimeFromHHMM(timeStr.c_str(), timeStr.size());
}

boost::gregorian::date DateFromDDMM(
        const std::string& dateStr,
        const GuessYear& guessYear,
        boost::gregorian::date currentDate)
{
    unsigned month = 0, day = 0;
    int n = 0;

    if (dateStr.size() != 4
        || sscanf(dateStr.c_str(), "%2u%2u%n", &day, &month, &n) != 2
        || n != 4)
    {
        return boost::gregorian::date();
    }

    return guessYear(month, day, currentDate);
}

std::string dateToDDMMYY(const boost::gregorian::date& date)
{
    std::ostringstream out;
    out.imbue(std::locale(std::locale::classic(), new boost::gregorian::date_facet("%d%m%y")));
    out << date;
    return out.str();
}

std::string dateToYYMMDD(const boost::gregorian::date& date)
{
    std::ostringstream out;
    out.imbue(std::locale(std::locale::classic(), new boost::gregorian::date_facet("%y%m%d")));
    out << date;
    return out.str();
}

std::string durationToHHMM(const boost::posix_time::time_duration& dur)
{
    char buf[16];
    const int h = dur.hours(), m = dur.minutes();
    snprintf(buf, sizeof(buf), "%02d%02d", h, m);
    return std::string(buf);
}

/***************************************************************************
 * Дата без указания года
 **************************************************************************/

boost::gregorian::date GuessYear_Current::operator()(unsigned month, unsigned day, boost::gregorian::date currentDate) const
{
    return MakeGregorianDate(currentDate.year(), month, day);
}

boost::gregorian::date GuessYear_Future::operator()(unsigned month, unsigned day, boost::gregorian::date currentDate) const
{
    /* Цикл нужен из-за 29ФЕВ в високосном годе */
    for (int yoffset = 0; yoffset < 4; ++yoffset) {
        const boost::gregorian::date date = MakeGregorianDate(currentDate.year() + yoffset, month, day);
        if (!date.is_not_a_date() && date >= currentDate)
            return date;
    }
    return boost::gregorian::date();
}

boost::gregorian::date GuessYear_Past::operator()(unsigned month, unsigned day, boost::gregorian::date currentDate) const
{
    /* Цикл нужен из-за 29ФЕВ в високосном годе */
    for (int yoffset = 0; yoffset < 4; ++yoffset) {
        const boost::gregorian::date date = MakeGregorianDate(currentDate.year() - yoffset, month, day);
        if (!date.is_not_a_date() && date <= currentDate)
            return date;
    }
    return boost::gregorian::date();
}

boost::gregorian::date GuessYear_Itin::operator()(unsigned month, unsigned day, boost::gregorian::date currentDate) const
{
    const string ddmon = DayMonth(day, month, ENGLISH);
    char vf_out[256];
    if (Dates::VFDate(vf_out, ddmon.c_str(), (int)ddmon.size()) < 0)
        return boost::gregorian::date();

    return DateFromYYMMDD(vf_out, YY2YYYY_Wraparound50YearsFutureDate, Dates::currentDate());
}

/* При ошибке возвращает not-a-date */
boost::gregorian::date DateFromDDMON(
        const string& dateStr,
        const GuessYear& guessYear,
        boost::gregorian::date currentDate)
{
    unsigned day = 0;
    char monthStr[8] = { 0 };
    int n = 0;

    if (dateStr.size() != 5
        || sscanf(dateStr.c_str(), "%2u%3s%n", &day, monthStr, &n) != 2
        || n != 5)
    {
        return boost::gregorian::date();
    }

    unsigned month = MonthNumber_CaseInsensitive(monthStr);
    if (month == 0) return boost::gregorian::date();

    return guessYear(month, day, currentDate);
}

boost::gregorian::date DateFromMMDD(
        const string& dateStr,
        const GuessYear& guessYear,
        boost::gregorian::date currentDate)
{
    unsigned month = 0, day = 0;
    int n = 0;

    if (dateStr.size() != 4
        || sscanf(dateStr.c_str(), "%2u%2u%n", &month, &day, &n) != 2
        || n != 4)
    {
        return boost::gregorian::date();
    }

    return guessYear(month, day, currentDate);
}

std::string hh24mi_ddmmyyyy(const boost::gregorian::date& d, const boost::posix_time::time_duration& t)
{
    return d.is_special() ? "" : hh24mi(t) + " " + ddmmyyyy(d,true);
}

std::string hh24mi_ddmmyyyy(const boost::posix_time::ptime& t)
{
    return t.is_special() ? "" : hh24mi_ddmmyyyy(t.date(), t.time_of_day());
}

std::string to_iso_string(const boost::posix_time::ptime& t)
{
    return boost::posix_time::to_iso_string(t);
}

std::string to_iso_extended_string(const boost::posix_time::ptime& t)
{
    return boost::posix_time::to_iso_extended_string(t);
}

boost::posix_time::ptime midnight(const boost::posix_time::ptime& t)
{
    return midnight(t.date());
}

boost::posix_time::ptime midnight(const boost::gregorian::date& d)
{
    return boost::posix_time::ptime(d, boost::posix_time::hours(0));
}

boost::posix_time::ptime end_of_day(const boost::posix_time::ptime& t)
{
    return end_of_day(t.date());
}

boost::posix_time::ptime end_of_day(const boost::gregorian::date& d)
{
    return boost::posix_time::ptime(d, boost::posix_time::time_duration(23,59,59));
}

std::string timeDurationToStr( boost::posix_time::time_duration const &td )
{
    std::string const tdstr( boost::posix_time::to_simple_string( td ) );
    size_t const pos = tdstr.find_last_of( ':' );
    ASSERT( pos != std::string::npos );
    return tdstr.substr( 0, pos );
}

namespace {


}

int years_between(const boost::gregorian::date &d1, const boost::gregorian::date &d2)
{
  return Dates::months_between(d1,d2) / 12;
}

int months_between(const boost::gregorian::date &d1, const boost::gregorian::date &d2)
{
  return d1 < d2 ? months_between_dates_where_the_first_one_is_lesser(d1,d2)
                 : months_between_dates_where_the_first_one_is_lesser(d2,d1);
}
} // namespace Dates


namespace boost
{

namespace gregorian
{

void fillDateTimeFacet(date_facet *fac, Language lang)
{
    const Dates::M_mass* c_short_months = lang == RUSSIAN ? Dates::Mez : Dates::Mon;
    const char* *c_long_months = lang == RUSSIAN ? Dates::long_months[1] : Dates::long_months[0];
    const char* *c_short_weekdays = lang == RUSSIAN ? Dates::DofW2[1] : Dates::DofW2[0];
    const char* *c_long_weekdays = lang == RUSSIAN ? Dates::DofW3[1] : Dates::DofW3[0];

    date_facet::input_collection_type short_months, long_months,
        short_weekdays, long_weekdays;
    for ( int i = 0 ; i < 12 ; ++i ) {
        short_months.insert( short_months.end() , c_short_months[i].name );
        long_months.insert( long_months.end() , c_long_months[i] );
    }
    for ( int i = 6; i < 13 ; ++i ) {
        /* америкосы блин дни нумеруют с воскресенья */
        short_weekdays.insert( short_weekdays.end() , c_short_weekdays[i%7] );
        long_weekdays.insert( long_weekdays.end() , c_long_weekdays[i%7] );
    }
    fac->short_month_names(short_months);
    fac->long_month_names(long_months);
    fac->short_weekday_names(short_weekdays);
    fac->long_weekday_names(long_weekdays);
}

std::ostream& operator<<(std::ostream& os, const date& d)
{
    boost::io::ios_flags_saver iflags(os);
    typedef boost::date_time::date_facet<date, char> custom_date_facet;
    std::ostreambuf_iterator<char> output_itr(os);
    if (std::has_facet<custom_date_facet>(os.getloc()))
      std::use_facet<custom_date_facet>(os.getloc()).put(output_itr, os, os.fill(), d);
    else {
      //instantiate a custom facet for dealing with dates since the user
      //has not put one in the stream so far.  This is for efficiency 
      //since we would always need to reconstruct for every date
      //if the locale did not already exist.  Of course this will be overridden
      //if the user imbues at some later point.  With the default settings
      //for the facet the resulting format will be the same as the
      //std::time_facet settings.
      custom_date_facet* f = new custom_date_facet();
      std::locale l = std::locale(os.getloc(), f);
      os.imbue(l);
      f->put(output_itr, os, os.fill(), d);
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const date_duration& dd)
{
    boost::io::ios_flags_saver iflags(os);
    typedef boost::date_time::date_facet<date, char> custom_date_facet;
    std::ostreambuf_iterator<char> output_itr(os);
    if (std::has_facet<custom_date_facet>(os.getloc()))
      std::use_facet<custom_date_facet>(os.getloc()).put(output_itr, os, os.fill(), dd);
    else {
      custom_date_facet* f = new custom_date_facet();
      std::locale l = std::locale(os.getloc(), f);
      os.imbue(l);
      f->put(output_itr, os, os.fill(), dd);

    }
    return os;
}

} // gregorian

namespace posix_time
{

std::ostream& operator<<(std::ostream& os, const ptime& p)
{
    boost::io::ios_flags_saver iflags(os);
    typedef boost::date_time::time_facet<ptime, char> custom_ptime_facet;
    typedef std::time_put<char> std_ptime_facet;
    std::ostreambuf_iterator<char> oitr(os);
    if (std::has_facet<custom_ptime_facet>(os.getloc()))
      std::use_facet<custom_ptime_facet>(os.getloc()).put(oitr, os, os.fill(), p);
    else {
      //instantiate a custom facet for dealing with times since the user
      //has not put one in the stream so far.  This is for efficiency 
      //since we would always need to reconstruct for every time period
      //if the locale did not already exist.  Of course this will be overridden
      //if the user imbues as some later point.
      custom_ptime_facet* f = new custom_ptime_facet();
      std::locale l = std::locale(os.getloc(), f);
      os.imbue(l);
      f->put(oitr, os, os.fill(), p);
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const time_duration& td)
{
    boost::io::ios_flags_saver iflags(os);
    typedef boost::date_time::time_facet<ptime, char> custom_ptime_facet;
    typedef std::time_put<char>                  std_ptime_facet;
    std::ostreambuf_iterator<char> oitr(os);
    if (std::has_facet<custom_ptime_facet>(os.getloc()))
      std::use_facet<custom_ptime_facet>(os.getloc()).put(oitr, os, os.fill(), td);
    else {
      //instantiate a custom facet for dealing with times since the user
      //has not put one in the stream so far.  This is for efficiency 
      //since we would always need to reconstruct for every time period
      //if the locale did not already exist.  Of course this will be overridden
      //if the user imbues as some later point.
      custom_ptime_facet* f = new custom_ptime_facet();
      std::locale l = std::locale(os.getloc(), f);
      os.imbue(l);
      f->put(oitr, os, os.fill(), td);
    }
    return os;
}

} // posix_time

} // boost

#ifdef XP_TESTING
#include "checkunit.h"
#include "perfom.h"
#include "timer.h"

#define array_size(a) (sizeof(a) / sizeof(a[0]))







namespace xp_testing
{



bool setDateTimeOffset(long offset, const std::string& tz)
{
    if (!tz.empty())
    {
        TNowOffset::set_tz_string(tz);
    }
    return TNowOffset::set(offset);
}


void fixDate(const boost::gregorian::date& dt)
{
    if (dt.is_not_a_date())
    {
        unfixDate();
    }
    else
    {
        fixDateTime(boost::posix_time::ptime(dt, boost::posix_time::seconds(0)));
    }
    LogTrace(TRACE1) << "Current date changed: " << Dates::ddmmyyyy(Dates::currentDate(), true);
}

void fixDateTime(const boost::posix_time::ptime& tm, const std::string& tz)
{
    if (tm.is_not_a_date_time())
    {
        unfixDate();
    }
    else
    {
        if (!tz.empty())
        {
            TNowOffset::set_tz_string(tz);
        }
        TNowOffset::set(tm);
    }
    LogTrace(TRACE1) << "Current date-time changed: " << Dates::hh24mi_ddmmyyyy(Dates::currentDateTime());
}

void unfixDate()
{
    TNowOffset::set(0);
    LogTrace(TRACE1) << "Current date/time returned to real";
}

} // xp_testing

using namespace Dates;

START_TEST(ptime_from_undelimited_string)
{
    boost::posix_time::ptime p = ptime_from_undelimited_string("20000101010101");
    fail_unless(p.date().year() == 2000);
    fail_unless(p.date().month() == 1);
    fail_unless(p.date().day() == 1);
    fail_unless(p.time_of_day().hours() == 1);
    fail_unless(p.time_of_day().minutes() == 1);
    fail_unless(p.time_of_day().seconds() == 1);

    p = ptime_from_undelimited_string("20491231235959");
    fail_unless(p.date().year() == 2049);
    fail_unless(p.date().month() == 12);
    fail_unless(p.date().day() == 31);
    fail_unless(p.time_of_day().hours() == 23);
    fail_unless(p.time_of_day().minutes() == 59);
    fail_unless(p.time_of_day().seconds() == 59);
}
END_TEST;


START_TEST(date_string)
{
    fail_unless(Dates::rrmmdd("badstr").is_not_a_date() == true);
    fail_unless(Dates::rrmmdd("      ").is_not_a_date() == true);
    fail_unless(Dates::rrmmdd("999999").is_not_a_date() == true);
    fail_unless(Dates::rrmmdd("101112").is_not_a_date() == false);
    boost::gregorian::date d(2006, 4, 15);
    fail_unless("20060415" == HelpCpp::string_cast(d,"%Y%m%d",0), "fail formatting");
    fail_unless(string("2006-АПР-15")
            == HelpCpp::string_cast(d, "%Y-%b-%d",1), "fail formatting");
}
END_TEST

START_TEST(time_string)
{
    boost::posix_time::ptime t(boost::gregorian::date(2006, 4, 15),
            boost::posix_time::time_duration(23,11,15));
    std::string result = HelpCpp::string_cast(t,"%Y%m%d%H%M%S",0);
    //std::cout << "date result: " << result << std::endl; 
    fail_unless("20060415231115" == result, "fail formatting");
    fail_unless(string("2006-АПР-15 23:11:15")
            == HelpCpp::string_cast(t,"%Y-%b-%d %H:%M:%S",1), "fail formatting");
}
END_TEST

START_TEST(check_time_duration)
{
    boost::posix_time::time_duration t = from_sirena_time("1435");
    fail_unless(t.hours() == 14, "wrong hours");
    fail_unless(t.minutes() == 35, "wrong minutes");
    t = from_sirena_time("0100");
    fail_unless(t.hours() == 1, "wrong hours");
    fail_unless(t.minutes() == 0, "wrong minutes");
    t = from_sirena_time("0000");
    fail_unless(t.hours() == 0, "wrong hours");
    fail_unless(t.minutes() == 0, "wrong minutes");
    t = from_sirena_time("2400");
    fail_unless(t.hours() == 24, "wrong hours");
    fail_unless(t.minutes() == 0, "wrong minutes");
    t = sirena_int_time_to_time_duration(0);
    t = sirena_int_time_to_time_duration(2400);
    t = sirena_int_time_to_time_duration(2401);
    t = sirena_int_time_to_time_duration(2501);
    t = sirena_int_time_to_time_duration(361);
}END_TEST

START_TEST(date_cast)
{
    boost::gregorian::date d = boost::gregorian::day_clock::local_day();
    for (size_t i = 0; i < 365; ++i) {
        d += boost::gregorian::days(1);
        std::string ds6 = HelpCpp::string_cast(d, "%y%m%d");
        std::string ds8 = HelpCpp::string_cast(d, "%Y%m%d");
        boost::gregorian::date d6 = HelpCpp::date_cast(ds6.c_str(), "%y%m%d", -1);
        boost::gregorian::date d8 = HelpCpp::date_cast(ds8.c_str(), "%Y%m%d", -1);
        if (d6 != from_sirena_date(ds6)) {
            LogTrace(TRACE5) << ds6 << " " << d6 << " != " << from_sirena_date(ds6);
            fail_unless(0,"from_sirena_date failed");
        }
        if (d8 != from_sirena_date(ds8)) {
            LogTrace(TRACE5) << ds8 << " " << d8 << " != " << from_sirena_date(ds8);
            fail_unless(0,"from_sirena_date failed");
        }
    }
}END_TEST

START_TEST(yycomparetimes)
{
    fail_unless(YYCompareTimes("501231012312", "501231012312") == 0);
    fail_unless(YYCompareTimes("501231012312", "501231012313") < 0);
    fail_unless(YYCompareTimes("501231012312", "481231012313") < 0);
    fail_unless(YYCompareTimes("501231012312", "651231012313") < 0);
    fail_unless(YYCompareTimes("481231012312", "501231012313") > 0);
    fail_unless(YYCompareTimes("481231012312", "351231012313") > 0);
    fail_unless(YYCompareTimes("481231012312", "651231012313") > 0);
    fail_unless(YYCompareTimes("651231012312", "501231012313") > 0);
    fail_unless(YYCompareTimes("651231012312", "481231012313") < 0);
}
END_TEST

START_TEST(check_date_from_mmdd)
{
    boost::gregorian::date currentDate(2008, 1, 1);
    {
        static const char* variants[] = {
            "011",
            "01010",
            "1301",
            "0199"
            "0A01"
            "01BB"
        };
        for (size_t i = 0; i < array_size(variants); ++i)
            fail_unless(Dates::DateFromMMDD(variants[i], Dates::GuessYear_Current(), currentDate).is_not_a_date());
    }
    fail_unless(Dates::DateFromMMDD(
                "0305",
                Dates::GuessYear_Current(),
                boost::gregorian::date(2008, 1, 1)) == MakeGregorianDate(2008, 3, 5));
}
END_TEST;

START_TEST(check_date_from_ddmon)
{
    {
        static const char* variants[] = {
            "1JAN",
            "01JA",
            "01JANN",
            "99JAN",
            "01ERR"
            "B0JAN"
        };
        for (size_t i = 0; i < array_size(variants); ++i)
            fail_unless(Dates::DateFromDDMON(variants[i], Dates::GuessYear_Current(),
                        Dates::currentDate()).is_not_a_date());
    }
    /* current */
    fail_unless(Dates::DateFromDDMON(
                "05MAR",
                Dates::GuessYear_Current(),
                boost::gregorian::date(2008, 1, 1)) == MakeGregorianDate(2008, 3, 5));
    /* past */
    fail_unless(Dates::DateFromDDMON(
                "05MAR",
                Dates::GuessYear_Past(),
                boost::gregorian::date(2008, 6, 1)) == MakeGregorianDate(2008, 3, 5));
    fail_unless(Dates::DateFromDDMON(
                "05MAR",
                Dates::GuessYear_Past(),
                boost::gregorian::date(2008, 1, 1)) == MakeGregorianDate(2007, 3, 5));
    fail_unless(Dates::DateFromDDMON(
                "29FEB", /* Бывает только в високосном году */
                Dates::GuessYear_Past(),
                boost::gregorian::date(2011, 1, 1)) == MakeGregorianDate(2008, 2, 29));

    /* future */
    fail_unless(Dates::DateFromDDMON(
                "05MAR",
                Dates::GuessYear_Future(),
                boost::gregorian::date(2008, 6, 1)) == MakeGregorianDate(2009, 3, 5));
    fail_unless(Dates::DateFromDDMON(
                "05MAR",
                Dates::GuessYear_Future(),
                boost::gregorian::date(2008, 1, 1)) == MakeGregorianDate(2008, 3, 5));
    fail_unless(Dates::DateFromDDMON(
                "29FEB", /* Бывает только в високосном году */
                Dates::GuessYear_Future(),
                boost::gregorian::date(2010, 1, 1)) == MakeGregorianDate(2012, 2, 29));
    const boost::gregorian::date dateFrom = Dates::currentDate() - boost::gregorian::days(364);
    const boost::gregorian::date dateTo   = Dates::currentDate() + boost::gregorian::days(364);
    long numDays = boost::gregorian::date_period(dateFrom, dateTo).length().days();
    size_t errCount = 0;
    for (boost::gregorian::day_iterator dayIt(dateFrom); numDays--; ++dayIt) {
        boost::gregorian::date date = *dayIt;
        const string ddmon = Dates::ddmon(date, ENGLISH);
        if (ddmon == "29FEB") /* VFDate не всегда отработает корректно, т.к. нет возможности указать текущий год */
            continue;

        char vf_out[256];
        LogTrace(TRACE5) << __FUNCTION__ << "ddmon (VFDate param) = \"" << ddmon << "\"";
        int ret = Dates::VFDate(vf_out, ddmon.c_str(), (int)ddmon.size());
        fail_unless(ret >= 0);
        string itinDate = Dates::rrmmdd(DateFromDDMON(ddmon, Dates::GuessYear_Itin(),
                    Dates::currentDate()));
        LogTrace(TRACE5) << __FUNCTION__ << ": date = " << Dates::rrmmdd(date)
            << ", vf_out = " << vf_out
            << ", itinDate = " << itinDate
            << (vf_out != itinDate ? " !!!" : "");

        if (vf_out != itinDate)
            ++errCount;
    }
    fail_unless(errCount == 0);
}
END_TEST

START_TEST(check_hhmi)
{
#define CHECK_HH24MI(hh_, mi_, str) { \
    boost::posix_time::time_duration td(hh_, mi_, 7, 9); \
    ck_assert_str_eq(Dates::hh24mi(td), str); \
}
    CHECK_HH24MI(1, 4, "01:04");
    CHECK_HH24MI(-1, 4, "-01:04");
    CHECK_HH24MI(0, -45, "-00:45");
#undef CHECK_HH24MI
#define CHECK_HH24MISS(hh_, mi_, ss_, str) { \
    boost::posix_time::time_duration td(hh_, mi_, ss_, 9); \
    const std::string s(Dates::hh24miss(td)); \
    fail_unless(str == s, "[%s] != [%s]", str, s.c_str()); \
}
    CHECK_HH24MISS(1, 4, 8, "01:04:08");
    CHECK_HH24MISS(-1, 4, 3, "-01:04:03");
    CHECK_HH24MISS(0, -45, 47, "-00:45:47");
    CHECK_HH24MISS(0, 0, -48, "-00:00:48");
#undef CHECK_HH24MISS
} END_TEST

START_TEST(check_date_conv)
{
    boost::gregorian::date currentDate(2008, 1, 1);

    /* Wraparound test */
    fail_unless(DateFromDDMONYY(
            "05MAR09", Dates::YY2YYYY_UseCurrentCentury, currentDate) ==
        MakeGregorianDate(2009, 3, 5));
    fail_unless(DateFromDDMONYY(
            "05MAR99", Dates::YY2YYYY_WraparoundFutureDate, currentDate) ==
        MakeGregorianDate(1999, 3, 5));
    fail_unless(DateFromDDMONYY(
            "05MAR58", Dates::YY2YYYY_Wraparound50YearsFutureDate, currentDate) ==
        MakeGregorianDate(1958, 3, 5));
    fail_unless(DateFromDDMONYY(
            "05MAR57", Dates::YY2YYYY_Wraparound50YearsFutureDate, currentDate) ==
        MakeGregorianDate(2057, 3, 5));
    fail_unless(DateFromDD_MM_YY(
            "05.03.07", Dates::YY2YYYY_Wraparound50YearsFutureDate, currentDate) ==
        MakeGregorianDate(2007, 3, 5));
    fail_unless(DateFromDD_MM_YY(
            "05.03.09", Dates::YY2YYYY_Wraparound50YearsFutureDate, currentDate) ==
        MakeGregorianDate(2009, 3, 5));
    fail_unless(DateFromDD_MM_YY(
            "05.03.87", Dates::YY2YYYY_Wraparound50YearsFutureDate, currentDate) ==
        MakeGregorianDate(1987, 3, 5));

    fail_unless(DateFromYYYYMMDD("20080305") == boost::gregorian::date(2008, 3, 5));
    fail_unless(DateFromDDMONYYYY("05MAR2008") == boost::gregorian::date(2008, 3, 5));
    fail_unless(DateFromDD_MM_YYYY("05.03.2008") == boost::gregorian::date(2008, 3, 5));

    {
        boost::gregorian::date date(boost::gregorian::pos_infin);
        fail_unless(Dates::rrmmdd(date) == "491231");
        fail_unless(Dates::ddmmrr(date) == "311249");
        fail_unless(Dates::ddmon(date, ENGLISH) == "31DEC");
        fail_unless(Dates::ddmon(date, RUSSIAN) == "31ДЕК");
        fail_unless(Dates::ddmonrr(date, ENGLISH) == "31DEC49");
        fail_unless(Dates::ddmonrr(date, RUSSIAN) == "31ДЕК49");
        fail_unless(Dates::ddmonyyyy(date, ENGLISH) == "31DEC2049");
        fail_unless(Dates::ddmonyyyy(date, RUSSIAN) == "31ДЕК2049");
        fail_unless(Dates::yyyymmdd(date) == "20491231");
    }
    {
        boost::gregorian::date date(boost::gregorian::neg_infin);
        fail_unless(Dates::rrmmdd(date) == "490101");
        fail_unless(Dates::ddmmrr(date) == "010149");
        fail_unless(Dates::ddmon(date, ENGLISH) == "01JAN");
        fail_unless(Dates::ddmon(date, RUSSIAN) == "01ЯНВ");
        fail_unless(Dates::ddmonrr(date, ENGLISH) == "01JAN49");
        fail_unless(Dates::ddmonrr(date, RUSSIAN) == "01ЯНВ49");
        fail_unless(Dates::ddmonyyyy(date, ENGLISH) == "01JAN1949");
        fail_unless(Dates::ddmonyyyy(date, RUSSIAN) == "01ЯНВ1949");
        fail_unless(Dates::yyyymmdd(date) == "19490101");
    }
    {
        boost::gregorian::date date(2008, 3, 5);
        fail_unless(Dates::rrmmdd(date) == "080305");
        fail_unless(Dates::ddmmrr(date) == "050308");
        fail_unless(Dates::ddmon(date, ENGLISH) == "05MAR");
        fail_unless(Dates::ddmon(date, RUSSIAN) == "05МАР");
        fail_unless(Dates::ddmonrr(date, ENGLISH) == "05MAR08");
        fail_unless(Dates::ddmonrr(date, RUSSIAN) == "05МАР08");
        fail_unless(Dates::ddmonyyyy(date, ENGLISH) == "05MAR2008");
        fail_unless(Dates::ddmonyyyy(date, RUSSIAN) == "05МАР2008");
        fail_unless(Dates::yyyymmdd(date) == "20080305");
    }
}
END_TEST;

START_TEST(check_time_from_hhmm)
{
    
    /* Тестируем "плохие" варианты для TimeFromHHMM */
    {
        static const char* variants[] = {
            "1",
            "101",
            "1370",
            "2405",
            "XXXX",
            "120A"
        };
        for (size_t i = 0; i < array_size(variants); ++i) {
            LogTrace(TRACE5) << __FUNCTION__ << ": variants[" << i << "] = " << variants[i];
            fail_unless(Dates::TimeFromHHMM(variants[i]).is_not_a_date_time());
        }
    }

    /* Тестируем "хорошие" варианты для TimeFromHHMM */
    {
        static const char* variants[] = {
            "0100",
            "0001",
            "1200",
            "2359",
            "0000"
        };
        for (size_t i = 0; i < array_size(variants); ++i) {
            LogTrace(TRACE5) << __FUNCTION__ << ": variants[" << i << "] = " << variants[i];
            fail_unless(Dates::hhmi(TimeFromHHMM(variants[i])) == variants[i]);
        }
    }
}
END_TEST;

START_TEST(check_daysOffset)
{
    boost::posix_time::time_duration dep;
    dep = boost::posix_time::time_duration(0, 1450, 0, 0);
    fail_unless(Dates::daysOffset(dep).days() == 1, "Offset: %ld", Dates::daysOffset(dep).days());
    dep = boost::posix_time::time_duration(0, 3000, 0, 0);
    fail_unless(Dates::daysOffset(dep).days() == 2, "Offset: %ld", Dates::daysOffset(dep).days());
    dep = boost::posix_time::time_duration(0, -120, 0, 0);
    fail_unless(Dates::daysOffset(dep).days() == -1, "Offset: %ld", Dates::daysOffset(dep).days());
    dep = boost::posix_time::time_duration(0, -1500, 0, 0);
    fail_unless(Dates::daysOffset(dep).days() == -2, "Offset: %ld", Dates::daysOffset(dep).days());
}
END_TEST

START_TEST(check_dayTime)
{
    const boost::posix_time::time_duration zeroDur(0,0,0,0);
    boost::posix_time::time_duration simpleDur(14,0,0,0);
    fail_unless(simpleDur == Dates::dayTime(simpleDur));
    boost::posix_time::time_duration oneDayDur(24,0,0,0);
    fail_unless(zeroDur == Dates::dayTime(oneDayDur));
    boost::posix_time::time_duration overOneDayPositiveDur(28,0,0,0);
    fail_unless(boost::posix_time::time_duration(4,0,0,0) == Dates::dayTime(overOneDayPositiveDur));
    boost::posix_time::time_duration overOneDayNegativeDur(-4,0,0,0);
    fail_unless(boost::posix_time::time_duration(20,0,0,0) == Dates::dayTime(overOneDayNegativeDur));
}
END_TEST

START_TEST(week_day_ru)
{
    const std::string s("20131020200000");
    const Dates::ptime t = Dates::ptime_from_undelimited_string(s);
    const int wd = week_day(s.c_str()+2);
    const int dw = day_of_week_ru(t);
    fail_unless(wd == dw, "wd=%i, dw=%i", wd, dw);
}
END_TEST


START_TEST(fix_date_time)
{
     boost::posix_time::ptime new_now(
             boost::gregorian::date(2010, 7, 13),
             boost::posix_time::time_duration(13,13,13,0)
             ) ;

     xp_testing::fixDateTime(new_now);

     boost::posix_time::ptime t1 = currentDateTime();
     boost::posix_time::ptime t2 = currentDateTime_us();
     usleep(2);
     boost::posix_time::ptime t3 = currentDateTime_us();

     fail_unless((t1 - new_now) < boost::posix_time::seconds(2));
     fail_unless((t2 - new_now) < boost::posix_time::seconds(2));
     fail_unless((t3 - new_now) < boost::posix_time::seconds(2));
     fail_unless(t3 != t2);
     int i = 0;
     while (t2.time_of_day().total_seconds() != t3.time_of_day().total_seconds())
     {
         t2 = currentDateTime_us();
         usleep(2);
         t3 = currentDateTime_us();
         if (++i > 100) break;
     }
     fail_unless(i < 100);
     fail_unless(t3 != t2);
     LogTrace(TRACE1) << "I:" << i;
     LogTrace(TRACE1) << "T2:" << t2;
     LogTrace(TRACE1) << "T3:" << t3;

     xp_testing::unfixDate();

     t1 = currentDateTime();
     t2 = boost::posix_time::second_clock::local_time();
     fail_unless((t1 - t2) < boost::posix_time::seconds(2));
}
END_TEST


#define SUITENAME "Dates"
TCASEREGISTER(0, 0)
{
    ADD_TEST(ptime_from_undelimited_string);
    ADD_TEST(date_string);
    ADD_TEST(time_string);
    ADD_TEST(week_day_ru);
    ADD_TEST(check_time_duration);
    ADD_TEST(date_cast);
    ADD_TEST(yycomparetimes);
    ADD_TEST(check_hhmi);
    ADD_TEST(check_date_conv);
    ADD_TEST(check_date_from_mmdd);
    ADD_TEST(check_date_from_ddmon);
    ADD_TEST(check_time_from_hhmm);
    ADD_TEST(check_daysOffset);
    ADD_TEST(check_dayTime);
    ADD_TEST(fix_date_time);
}
TCASEFINISH

#endif // XP_TESTING
