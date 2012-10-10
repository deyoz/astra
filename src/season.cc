#include "season.h"
#include "xml_unit.h"
#include "exceptions.h"
#include "oralib.h"
#include "cache.h"
#include "misc.h"
#include <map>
#include <vector>
#include <string>
#include "stages.h"
#include "basic.h"
#include "stl_utils.h"
#include "stat.h"
#include "docs.h"
#include "pers_weights.h"
#include "base_tables.h"
#include "astra_misc.h"
#include "tlg/tlg_binding.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

const int SEASON_PERIOD_COUNT = 5;
const int SEASON_PRIOR_PERIOD = 2;

using namespace BASIC;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace std;
using namespace boost::local_time;
using namespace boost::posix_time;
using namespace SEASON;


struct TRange {
  TDateTime first;
  TDateTime last;
  string days;
  TRange() {
    first = NoExists;
    last = NoExists;
  }
};


struct TPeriod {
  int move_id;
//фы  тvўшёыхэшх яхЁхїюфр  int first_dest;
  TDateTime first;
  TDateTime last;
  string days;
  string tlg;
  string ref;
  tmodify modify;
  bool pr_del;
  int hours;
  TPeriod() {
    modify = fnochange;
    pr_del = false;
  }
};

struct TSeason {
  time_period period;
  bool summer;
  string name;
  TSeason( ptime start_time, ptime end_time, bool asummer, string aname ):
           period( start_time, end_time), summer(asummer), name(aname) {};
};

struct timeDiff {
  TDateTime first;
  TDateTime last;
  int hours;
};

typedef vector<timeDiff> TTimeDiff;

struct TDest {
  int num;
  string airp;
  TElemFmt airp_fmt;
  string city;
  TElemFmt city_fmt;
  int pr_del;
  TDateTime scd_in;
  string airline;
  TElemFmt airline_fmt;
  string region;
  int trip;
  string craft;
  TElemFmt craft_fmt;
  string litera;
  string triptype;
  TDateTime scd_out;
  int f;
  int c;
  int y;
  string unitrip;
  string suffix;
  TElemFmt suffix_fmt;
};

struct trip {
  int trip_id;
  string name;
  string print_name;
  string crafts;
  string airlineId;
  string airpId;
  string craftId;
  string triptypeId;
  string owncraft;
  string ownport;
  string portsForAirline;
  vector<TDest> vecportsFrom, vecportsTo;
  string bold_ports;
  TDateTime scd_in;
  TDateTime scd_out;
  TDateTime trap;
  string triptype;
  int pr_del;
  trip() {
    scd_in = NoExists;
    scd_out = NoExists;
    trap = NoExists;
  }
};

typedef vector<TDest> TDests;

struct TDestList {
  bool pr_del;
  TDateTime flight_time;
  TDateTime last_day;
  int tz;
  string region;
  TDests dests;
  TDateTime diff;
  vector<trip> trips;
  TDestList() {
  	diff = 0;
  }
};

typedef map<int,TDestList> tmapds;

typedef map<double,tmapds> TSpp;

struct TRangeList {
  int trip_id;
  vector<TPeriod> periods;
};

class TFilter {
  private:
  	map<int,TTimeDiff> offsets;
  public:
    int dst_offset;
    int tz;
    vector<TSeason> periods; //периоды летнего и зимнего расписания
    string region; // регион относительно которого расчитvвается периодv расписания
    int season_idx; // текущее расписание
    TRange range; // диапазон дат в фильтре, когда не задан - диапазон расписания с временами
    TDateTime firstTime;
    TDateTime lastTime;
    string airline;
    string city;
    string airp;
    string triptype;
    void Clear();
    void Parse( xmlNodePtr filterNode );
    void Build( xmlNodePtr filterNode );
    void GetSeason();
    bool isSummer( TDateTime pfirst );
    void InsertSectsPeriods( map<int,TDestList> &mapds,
                             vector<TPeriod> &speriods, vector<TPeriod> &nperiods, TPeriod p );
    bool isFilteredTime( TDateTime first_day, TDateTime scd_in, TDateTime scd_out,
                         int dst_offset, string region );
    bool isFilteredUTCTime( TDateTime vd, TDateTime first, TDateTime dest_time, int dst_offset );
    bool isFilteredTime( TDateTime vd, TDateTime first_day, TDateTime scd_in, TDateTime scd_out,
                         int dst_offset, string vregion );
    TDateTime GetTZTimeDiff( TDateTime utcnow, TDateTime first, int tz );
    TFilter();
};

bool createAirportTrip( string airp, int trip_id, TFilter filter, int offset, TDestList &ds,
                        TDateTime vdate, bool viewOwnPort, bool UTCFilter, string &err_city );
bool createAirportTrip( int trip_id, TFilter filter, int offset, TDestList &ds, bool viewOwnPort, string &err_city );
bool createAirlineTrip( int trip_id, TFilter &filter, int offset, TDestList &ds, string &err_city );
bool createAirlineTrip( int trip_id, TFilter &filter, int offset, TDestList &ds, TDateTime localdate, string &err_city );
int GetTZOffSet( TDateTime first, int tz, map<int,TTimeDiff> &v );
void GetDests( map<int,TDestList> &mapds, const TFilter &filter, int move_id = NoExists );
string GetCommonDays( string days1, string days2 );
bool CommonDays( string days1, string days2 );
string DeleteDays( string days1, string days2 );
void ClearNotUsedDays( TDateTime first, TDateTime last, string &days );

void PeriodToUTC( TDateTime &first, TDateTime &last, string &days, const string region );

void internalRead( TFilter &filter, xmlNodePtr dataNode, int trip_id = NoExists );
void GetEditData( int trip_id, TFilter &filter, bool buildRanges, xmlNodePtr dataNode, string &err_city );

void createSPP( TDateTime localdate, TSpp &spp, bool createViewer, string &err_city );

string DefaultTripType( bool pr_lang = true )
{
	string res = "п";
	if (pr_lang)
		res = ElemIdToCodeNative(etTripType,res);
	return res;
}

bool isDefaultTripType( const string &triptype )
{
	TElemFmt fmt;
  return ElemToElemId(etTripType,triptype,fmt) == DefaultTripType(false);
}

string GetPrintName( TDest *PDest, TDest *NDest )
{
	string res;
  if ( !PDest || NDest->trip > NoExists && abs( PDest->trip - NDest->trip ) <= 1 && PDest->airline == NDest->airline ) {
    res = ElemIdToElemCtxt( ecDisp, etAirline, NDest->airline, NDest->airline_fmt );
    while ( res.size() < 3 ) {
    	res += " ";
    }
    	res += IntToString( NDest->trip );
  }
  else {
  	res = ElemIdToElemCtxt( ecDisp, etAirline, PDest->airline, PDest->airline_fmt );
    while ( res.size() < 3 ) {
    	res += " ";
    }
    res += IntToString( PDest->trip );
    if ( NDest->trip > NoExists ) {
    	res += "/";
    	if ( PDest->airline != NDest->airline ) {
    		string comp = ElemIdToElemCtxt( ecDisp, etAirline, NDest->airline, NDest->airline_fmt );
      	while ( comp.size() < 3 ) {
      	  comp += " ";
       	}
       	res += comp;
      }
    	res += IntToString( NDest->trip );
    }
  }
	return res;
}

TFilter::TFilter()
{
  Clear();
  region = TReqInfo::Instance()->desk.tz_region;
  TQuery GQry( &OraSession );
  GQry.SQLText = "SELECT tz FROM tz_regions WHERE region=:region AND pr_del=0 ORDER BY tz DESC";
  GQry.CreateVariable( "region", otString, region );
  GQry.Execute();
  tz = GQry.FieldAsInteger( "tz" );
}

void TFilter::Clear()
{
  season_idx = 0;
  firstTime = NoExists;
  lastTime = NoExists;
  airline.clear();
  city.clear();
  airp.clear();
  triptype.clear();
  periods.clear();
  dst_offset = 0;
}

void TFilter::GetSeason()
{
  ptime utcd = second_clock::universal_time();
  int year = utcd.date().year();
  tz_database &tz_db = get_tz_database();
  time_zone_ptr tz = tz_db.time_zone_from_region( region );
  local_date_time ld( utcd, tz ); /* определяем текущее время локальное */
  bool summer = true;
  /* устанавливаем первый год и признак периода */
  for ( int i=0; i<SEASON_PRIOR_PERIOD; i++ ) {
    if ( tz->has_dst() ) {  // если есть переход на зимнее/летнее расписание
    	if ( i == 0 ) {
        dst_offset = tz->dst_offset().hours();
        if ( ld.is_dst() ) {  // если сейчас лето
          year--;
          summer = false;
        }
        else {  // если сейчас зима
          ptime start_time = tz->dst_local_start_time( year );
          ProgTrace( TRACE5, "start_time=%s", DateTimeToStr( BoostToDateTime(start_time),"dd.mm.yy hh:nn:ss" ).c_str() );
          if ( ld.local_time() < start_time ) {
          	tst();
            year--;
          }
          summer = true;
        }
      }
      else {
      	summer = !summer;
        if ( !summer ) {  // если сейчас лето
          year--;
        }
      }
    }
    else {
     year--;
     dst_offset = 0;
    }

  }
  for ( int i=0; i<SEASON_PERIOD_COUNT; i++ ) {
    ptime s_time, e_time;
    string name;
    if ( tz->has_dst() ) {
      if ( summer ) {
        s_time = tz->dst_local_start_time( year ) - tz->base_utc_offset();
        e_time = tz->dst_local_end_time( year ) - tz->base_utc_offset() - seconds(1);
        name = getLocaleText( string( "Лето" ) ) + " " + IntToString( year );
      }
      else {
        s_time = tz->dst_local_end_time( year ) - tz->base_utc_offset() - tz->dst_offset();
        year++;
        e_time = tz->dst_local_start_time( year ) - tz->base_utc_offset() - seconds(1);
        name = getLocaleText( string( "Зима" ) ) + " " + IntToString( year - 1 ) + "-" + IntToString( year );
      }
      summer = !summer;
    }
    else {
     /* период - это целый год */
     s_time = ptime( boost::gregorian::date(year,1,1) );
     year++;
     e_time = ptime( boost::gregorian::date(year,1,1) );
     name = IntToString( year - 1 ) + " " + getLocaleText("год");
    }
    ProgTrace( TRACE5, "s_time=%s, e_time=%s, summer=%d, i=%d",
               DateTimeToStr( UTCToLocal( BoostToDateTime(s_time), region ),"dd.mm.yy hh:nn:ss" ).c_str(),
               DateTimeToStr( UTCToLocal( BoostToDateTime(e_time), region ), "dd.mm.yy hh:nn:ss" ).c_str(), !summer, i );
    periods.push_back( TSeason( s_time, e_time, !summer, name ) );
    if ( i == SEASON_PRIOR_PERIOD ) {
      range.first = BoostToDateTime( periods[ i ].period.begin() );
      range.last = BoostToDateTime( periods[ i ].period.end() );
      range.days = AllDays;
      season_idx = i;
    }
  }
}

bool TFilter::isSummer( TDateTime pfirst )
{
  ptime r( DateTimeToBoost( pfirst ) );
  for ( vector<TSeason>::iterator p=periods.begin(); p!=periods.end(); p++ ) {
    if ( !p->period.contains( r ) ) // не пересекаются
      continue;
    return p->summer;
  }
  return false;
}

inline TDateTime getDiff( int dst_offset, bool ssummer, bool psummer )
{
  TDateTime diff;
  if ( ssummer == psummer )
    diff = 0.0;
  else
    if ( ssummer )
      diff = (double)dst_offset*3600000/(double)MSecsPerDay;
    else {
      diff = 0.0 - (double)dst_offset*3600000/(double)MSecsPerDay;
    }
  return diff;
}

inline void setDestsDiffTime( TFilter *filter, TDests &dests, int dst_offset, TDateTime f1, TDateTime f2 )
{
	TDateTime diff;
  double utcfirst1, utcfirst2;
  modf((double)f1, &utcfirst1 );
  modf((double)f2, &utcfirst2 );

  for ( TDests::iterator id=dests.begin(); id!=dests.end(); id++ ) {
    if ( id->scd_in > NoExists ) {
    	if ( id->scd_in >= 0 ) {
    	  f1 = utcfirst1 + id->scd_in;
    	  f2 = utcfirst2 + id->scd_in;
      }
      else {
      	double f3 = fabs( modf( (double)id->scd_in, &f1 ) );
      	f2 = f1 + utcfirst2 + f3;
      	f1 += utcfirst1 + f3;
      }
    	diff = getDiff( dst_offset, filter->isSummer( f1 ), filter->isSummer( f2 ) );
      if ( id->scd_in >= 0 )
        id->scd_in += diff;
      else
        id->scd_in -= diff;
    }
    if ( id->scd_out > NoExists ) {
    	if ( id->scd_out >= 0 ) {
    	  f1 = utcfirst1 + id->scd_out;
    	  f2 = utcfirst2 + id->scd_out;
      }
      else {
      	double f3 = fabs( modf( (double)id->scd_out, &f1 ) );
      	f2 = f1 + utcfirst2 + f3;
      	f1 += utcfirst1 + f3;
      }

    	diff = getDiff( dst_offset, filter->isSummer( f1 ), filter->isSummer( f2 ) );

      if ( id->scd_out >= 0 )
        id->scd_out += diff;
      else
      	id->scd_out -= diff;

    }
  }
}

inline int calcDestsProp( TFilter *filter, map<int,TDestList> &mapds, int dst_offset, int old_move_id, TDateTime f1, TDateTime f2 )
{
  TDestList ds = mapds[ old_move_id ];
  setDestsDiffTime( filter, ds.dests, dst_offset, f1, f2 );
  int new_move_id=0;
  for ( map<int,TDestList>::iterator im=mapds.begin(); im!=mapds.end(); im++ ) {
    if ( im->first < new_move_id )
      new_move_id = im->first;
  }
  new_move_id--;
  mapds.insert(std::make_pair( new_move_id, ds ) );
  return new_move_id;
}

/* на все неизмененные диапазоны накладываем измененный и пересечения сохраняем */
/* speriods содержит все периоды с которыми надо пересекать */
/* nperiods - содержит множество пересеченных периодов */
void TFilter::InsertSectsPeriods( map<int,TDestList> &mapds,
                                  vector<TPeriod> &speriods, vector<TPeriod> &nperiods, TPeriod p )
{
	TDateTime diff;
  TPeriod np;
  double f1, f2;
  bool issummer;
  bool psummer = isSummer( p.first );
  time_period s( DateTimeToBoost( p.first ), DateTimeToBoost( p.last ) );
  //разбитие периодов periods периодом p
  for ( vector<TPeriod>::iterator ip=speriods.begin(); ip!=speriods.end(); ip++ ) {
    // периоды хранять время вылета из п.п.
    if ( ip->modify == fdelete )
      continue;
    issummer = isSummer( ip->first );

ProgTrace( TRACE5, "p.move_id=%d, ip->move_id=%d, psummer=%d, issummer=%d",
           p.move_id, ip->move_id, psummer, issummer );
    if ( p.move_id == ip->move_id && psummer != issummer ) {
        //имеется ссылка на один и тот же маршрут, а периоды принадлежат разным сезонам - разбить к чертовой матери
      diff = getDiff( dst_offset, issummer, psummer );
      ProgTrace( TRACE5, "IsSummer(p.first)=%d, issummer=%d, diff=%f",
                 isSummer( p.first ), issummer, diff );
      p.modify = finsert;
      p.move_id = calcDestsProp( this, mapds, dst_offset, p.move_id, ip->first, p.first );
    }

ProgTrace( TRACE5, "ip first=%s, last=%s",
           DateTimeToStr( ip->first, "dd.mm.yyyy hh:nn:ss" ).c_str(),
           DateTimeToStr( ip->last, "dd.mm.yyyy hh:nn:ss" ).c_str() );
    time_period r( DateTimeToBoost( ip->first ), DateTimeToBoost( ip->last ) );
    if ( !r.intersects( s ) || !CommonDays( ip->days, p.days ) ) {
      continue;
    }
    // есть пересечение
    time_period d( r.intersection( s ) );
    time_period n1( r.begin(), d.begin() );
    time_period n2( d.end(), r.end() );
    if ( !n1.is_null() ) {
      np = *ip;
      modf( (double)p.first, &f1 );
      np.last = f1 - 1 + modf( (double)ip->last, &f2 );
      ClearNotUsedDays( np.first, np.last, np.days );
    ProgTrace( TRACE5, "result np->first=%s, np->last=%s, np->days=%s",
               DateTimeToStr( np.first,"dd.mm.yy hh:nn:ss" ).c_str(),
               DateTimeToStr( np.last,"dd.mm.yy hh:nn:ss" ).c_str(),
               np.days.c_str() );

      if ( np.days != NoDays )
        nperiods.push_back( np );
    }
    np = *ip;
    // этот кусок периода может быть в другом расписании, а следовательно иметь другие времена
    diff = getDiff( dst_offset, issummer, psummer );
    modf( (double)BoostToDateTime( d.begin() ), &f1 );
    np.first = f1 + modf( (double)ip->first, &f2 ) + diff;
    modf( (double)BoostToDateTime( d.end() ), &f2 );
    np.last = f2 + modf( (double)ip->first, &f1 ) + diff;
    np.days = DeleteDays( ip->days, p.days ); // удаляем из p.days дни ip->days
    ClearNotUsedDays( np.first, np.last, np.days );
/*    ProgTrace( TRACE5, "result np->first=%s, np->last=%s, np->days=%s, ip->days=%s",
               DateTimeToStr( np.first,"dd.mm.yy hh:nn:ss" ).c_str(),
               DateTimeToStr( np.last,"dd.mm.yy hh:nn:ss" ).c_str(),
               np.days.c_str(), ip->days.c_str() );*/

    if ( np.days != NoDays ) {
      /* разбили период - этот кусок может принадлежать другому расписанию */
      if ( diff ) {
        /* надо рассматривать данный период как отдельный а не расширение */
        np.modify = finsert;
        np.move_id = calcDestsProp( this, mapds, dst_offset, np.move_id, ip->first, np.first );
      }
      nperiods.push_back( np );
    }

    if ( !n2.is_null() ) {
      np = *ip;
      diff = getDiff( dst_offset, isSummer( p.last + 1 ), issummer );
      modf( (double)p.last, &f1 );
      np.first = f1 + 1 + modf( (double)ip->first, &f2 ) + diff;
      modf( (double)ip->last, &f2 );
      np.last = f2 + modf( (double)ip->first, &f1 ) + diff;
      ClearNotUsedDays( np.first, np.last, np.days );
/*    ProgTrace( TRACE5, "result np->first=%s, np->last=%s, np->days=%s",
               DateTimeToStr( np.first,"dd.mm.yy hh:nn" ).c_str(),
               DateTimeToStr( np.last,"dd.mm.yy hh:nn" ).c_str(),
               np.days.c_str() );*/

      if ( np.days != NoDays ) {
        /* разбили период - этот кусок может принадлежать другому расписанию */
        if ( diff ) {
          /* надо рассматривать данный период как отдельный а не расширение */
          np.modify = finsert;
          np.move_id = calcDestsProp( this, mapds, dst_offset, np.move_id, ip->first, np.first );
        }
        nperiods.push_back( np );
      }
    }
    ip->modify = fdelete;
  }
//  p.modify = fnochange;
ProgTrace( TRACE5, "first=%s, last=%s, modified=%d",
           DateTimeToStr( p.first, "dd.mm.yyyy hh:nn:ss" ).c_str(),
           DateTimeToStr( p.last, "dd.mm.yyyy hh:nn:ss" ).c_str(),
           p.modify );

  nperiods.push_back( p );
};

bool TFilter::isFilteredUTCTime( TDateTime vd, TDateTime first, TDateTime dest_time, int dst_offset )
{
  if ( firstTime == NoExists || region.empty() )
    return true;
  double f1,f2,f3;
  modf( vd, &f1 );
  TDateTime f,l;
  f2 = modf( (double)(f1 + firstTime), &f3 );
  if ( f3 < f1 )
    f = f3 - f1 - f2;
  else
    f = f3 - f1 + f2;

  f2 = modf( (double)(f1 + lastTime), &f3 );
  if ( f3 < f1 )
    l = f3 - f1 - f2;
  else
    l = f3 - f1 + f2;

  // надо вvделять только время, без учета числа и перехода суток
  f = modf( (double)f, &f1 );
  l = modf( (double)l, &f1 );

  if ( f < 0 )
    f = fabs( f );
  else
    f += 1.0;
  if ( l < 0 )
    l = fabs( l );
  else
    l += 1.0;

  f -= (double)1000/(double)MSecsPerDay;
  l +=  (double)1000/(double)MSecsPerDay;
  if ( dest_time == NoExists )
    f1 = NoExists;
  else {
    f1 = modf( (double)dest_time, &f2 );
    if ( f1 < 0 )
      f1 = fabs( f1 );
    else
      f1 += 1.0;
  }

  //учет перехода времени
  TDateTime diff = GetTZTimeDiff( vd, first, tz );
  f1 += diff;

  return ( f1 >= f && f1 <= l );
}


bool TFilter::isFilteredTime( TDateTime vd, TDateTime first_day, TDateTime scd_in, TDateTime scd_out,
                              int dst_offset, string vregion )
{
	ProgTrace( TRACE5, "filter.firsttime=%s, filter.lasttime=%s, first_day=%s, scd_in=%s, scd_out=%s",
	           DateTimeToStr( firstTime, "dd.mm hh:nn" ).c_str(),
	           DateTimeToStr( lastTime, "dd.mm hh:nn" ).c_str(),
	           DateTimeToStr( first_day, "dd.mm hh:nn" ).c_str(),
	           DateTimeToStr( scd_in, "dd.mm hh:nn" ).c_str(),
	           DateTimeToStr( scd_out, "dd.mm hh:nn" ).c_str());
  if ( firstTime == NoExists || region.empty() )
    return true;
  /* переводим времена в фильтре во время UTC относительно города в маршруте */
  // переводим время начала расписания в UTC
  // переводим время во время клиента и для верности удаляем время и добавляем день
  double f1,f2,f3;
  //???modf( t, &f1 );
  modf( first_day, &f1 );
  TDateTime f,l;
  // переводим время фильтра в UTC
  // normilize date
  try {
    f2 = modf( (double)ClientToUTC( f1 + firstTime, vregion ), &f3 );
  }
  catch( boost::local_time::ambiguous_result ) {
  	f2 = modf( (double)ClientToUTC( f1 + 1 + firstTime, vregion ), &f3  );
  	f3--;
  }
  catch( boost::local_time::time_label_invalid ) {
    throw AstraLocale::UserException( "MSG.FLIGHT_BEGINNING_TIME_NOT_EXISTS",
            LParams() << LParam("time", DateTimeToStr( f1 + firstTime, "dd.mm hh:nn" )));
  }

  if ( f3 < f1 )
    f = f3 - f1 - f2;
  else
    f = f3 - f1 + f2;
  try {
    f2 = modf( (double)ClientToUTC( f1 + lastTime, vregion ), &f3 );
  }
  catch( boost::local_time::ambiguous_result ) {
  	f2 = modf( (double)ClientToUTC( f1 + 1 + lastTime, vregion ), &f3 );
  	f3--;
  }
  catch( boost::local_time::time_label_invalid ) {
    throw AstraLocale::UserException( "MSG.FLIGHT_ENDING_TIME_NOT_EXISTS",
            LParams() << LParam("time", DateTimeToStr( f1 + lastTime, "dd.mm hh:nn" )));
  }
  if ( f3 < f1 )
    l = f3 - f1 - f2;
  else
    l = f3 - f1 + f2;

  // надо вvделять только время, без учета числа и перехода суток
  f = modf( (double)f, &f1 );
  l = modf( (double)l, &f1 );

  if ( f < 0 )
    f = fabs( f );
/*  if ( f < 1.0 )
  	f += 1.0;  */
  else
    f += 1.0;
  if ( l < 0 )
    l = fabs( l );
/*  if ( l < 1.0 )
  	l += 1.0;*/
  else
    l += 1.0;

  f -= (double)1000/(double)MSecsPerDay;
  l +=  (double)1000/(double)MSecsPerDay;
  if ( scd_in == NoExists )
    f1 = NoExists;
  else {
    f1 = modf( (double)scd_in, &f2 );
    if ( f1 < 0 )
      f1 = fabs( f1 );
/*    if ( f1 < 1.0 )
    	f1 += 1.0;  */
    else
      f1 += 1.0;
  }
  if ( scd_out == NoExists )
    f2 = NoExists;
  else {
    f2 = modf( (double)scd_out, &f3 );
    if ( f2 < 0 )
      f2 = fabs( f2 );
/*    if ( f2 < 1.0 )
    	f2 += 1.0;*/
    else
      f2 += 1.0;
  }


  //учет перехода времени
  //01.04.2008 TDateTime diff = getDiff( dst_offset, isSummer( first_day ), isSummer( vd ) );
  TDateTime diff = GetTZTimeDiff( vd, first_day, tz );
  f1 += diff;
  f2 += diff;


  ProgTrace( TRACE5, "f=%f,l=%f, f1=%f, f2=%f", f, l, f1, f2 );
/*	ProgTrace( TRACE5, "filter.firsttime=%s, filter.lasttime=%s, scd_in=%s, scd_out=%s",
	           DateTimeToStr( f, "dd.mm hh:nn" ).c_str(),
	           DateTimeToStr( l, "dd.mm hh:nn" ).c_str(),
	           DateTimeToStr( f1, "dd.mm hh:nn" ).c_str(),
	           DateTimeToStr( f2, "dd.mm hh:nn" ).c_str());*/

  return ( f1 >= f && f1 <= l || f2 >= f && f2 <= l );
}

bool TFilter::isFilteredTime( TDateTime first_day, TDateTime scd_in, TDateTime scd_out, int dst_offset, string vregion )
{


/*, DateTimeToStr( BoostToDateTime( periods[ season_idx ].period.begin() ) + 10, "dd.mm.yy hh:nn" ).c_str()  */

  return isFilteredTime( BoostToDateTime( periods[ season_idx ].period.begin() ) + 1,
                         first_day, scd_in, scd_out, dst_offset, vregion );
}


string DeleteDays( string days1, string days2 )
{
  string res = days1;
  for ( string::iterator s=days2.begin(); s!=days2.end(); s++ ) {
    string::size_type n;
    if ( *s != '.' && ( n = res.find( *s ) ) != string::npos )
      res[ n ] = '.';
  }
  ProgTrace( TRACE5, "res=%s, days1=%s, days2=%s", res.c_str(), days1.c_str(), days2.c_str() );
  return res;
}

string GetCommonDays( string days1, string days2 )
{
  string res;
  for ( string::iterator s=days1.begin(); s!=days1.end(); s++ ) {
   if ( *s != '.' && days2.find( *s ) != string::npos )
    res += *s;
   else
    res += ".";
  }
  return res;
}

bool CommonDays( string days1, string days2 )
{
  return GetCommonDays( days1, days2 ) != NoDays;
}

string GetWOPointDays( string days )
{
  string res;
  for ( string::iterator s=days.begin(); s!=days.end(); s++ ) {
    if ( *s != '.' )
     res += *s;
  }
  return res;
}

string AddDays( string days, int delta )
{
  string res = NoDays;
  for ( int i=0; i<7; i++ ) {
    if ( days[ i ] == '.' )
      continue;
    int day = ToInt( days.substr(i,1) ) + delta;
    if ( day > 7 )
      day -= 7;
    else
      if ( day <= 0 )
        day += 7;
    res[ day - 1 ] = *IntToString( day ).c_str();
  }
  return res;
}

string GetNextDays( string days, int delta )
{
  string res = "(";
  res += GetWOPointDays( AddDays( days, delta ) );
  res += ")";
  return res;
}

void ClearNotUsedDays( TDateTime first, TDateTime last, string &days )
{
  string res = NoDays;
  int year, month, day;
  DecodeDate( first, year, month, day );
  boost::gregorian::date d( year, month, day );
  for ( int i=(int)first; i<=(int)last && i-(int)first<7; i++ ) {
    int wday = d.day_of_week();
    if ( wday == 0 )
      wday = 7;
    d += boost::gregorian::date_duration( 1 );
    ProgTrace( TRACE5, "wday=%d, year=%d, month=%d, day=%d", wday, year, month, day + i - (int)first );
    res[ wday - 1 ] = IntToString( wday ).c_str()[0];
  };
ProgTrace( TRACE5, "res=%s, days=%s", res.c_str(), days.c_str() );
  days = GetCommonDays( days, res );
  ProgTrace( TRACE5, "common days=%s", days.c_str() );
}

/* даты фильтра в UTC, времена тоже в UTC, исключение, когда
 (int)TReqInfo::Instance()->user.access.airps.size() != 1 */
void TFilter::Parse( xmlNodePtr filterNode )
{
  TBaseTable &baseairps = base_tables.get( "airps" );
  /* вначале заполняем по умолчанию, а потом переписываем тем, что пришло с клиента */
  Clear();
  GetSeason();
  if ( !filterNode ) {
/*    ProgTrace( TRACE5, "filter parse: season_idx=%d,range.first=%s,range.last=%s,airp=%s,city=%s,time.first=%s,time.last=%s, airline=%s, triptype=%s",
               season_idx, DateTimeToStr( range.first, "dd.mm.yy hh:nn" ).c_str(), DateTimeToStr( range.last, "dd.mm.yy hh:nn" ).c_str(),
               airp.c_str(),city.c_str(),DateTimeToStr( firstTime, "dd.mm.yy hh:nn" ).c_str(),
               DateTimeToStr( lastTime, "dd.mm.yy hh:nn" ).c_str(), airline.c_str(), triptype.c_str() );*/
    return;
  }
  xmlNodePtr node;
  node = NodeAsNode( "season", filterNode );
  if ( node ) {
    season_idx = NodeAsInteger( node );
    ProgTrace( TRACE5, "season_idx=%d", season_idx );
  }
  node = GetNode( "range", filterNode );
  if ( node ) {
    range.first = NodeAsDateTime( "first", node );
    range.last = NodeAsDateTime( "last", node );
    node = GetNode( "days", node );
    range.days = NodeAsString( node );
    tz_database &tz_db = get_tz_database();
    time_zone_ptr tz = tz_db.time_zone_from_region( region );
    if ( tz->has_dst() ) {
      TDateTime f = range.first;
      ptime p = DateTimeToBoost( f ) - tz->base_utc_offset();
      if ( periods[ season_idx ].summer )
        p = p - hours( dst_offset );
      range.first = BoostToDateTime( p );
      if ( (int)range.first != (int)f ) {
        range.last += (int)f - (int)range.first;
        //01.04.08range.days = AddDays( range.days, (int)f - (int)range.first );
      }
    }
  }
  else { /* диапазон не задан, то используем по умолчанию UTC */
    range.first = BoostToDateTime( periods[ season_idx ].period.begin() );
    range.last = BoostToDateTime( periods[ season_idx ].period.end() );
    range.days = AllDays;
  }
//  ClearNotUsedDays( range.first, range.last, range.days );
  node = GetNode( "airp", filterNode );
  if ( node ) {
  	try {
  		TElemFmt fmt;
      airp = ElemToElemId( etAirp, NodeAsString( node ), fmt ); // сконвертил в то как лежит в базе
    }
    catch( EConvertError &e ) {
    	throw AstraLocale::UserException( "MSG.AIRP.INVALID_GIVEN_CODE" );
    }
  }
  node = GetNode( "city", filterNode );
  if ( node ) {
  	try {
  		TElemFmt fmt;
      city = ElemToElemId( etCity, NodeAsString( node ), fmt ); // сконвертил в то как лежит в базе
    }
    catch( EConvertError &e ) {
    	throw AstraLocale::UserException( "MSG.CITY.INVALID_GIVEN_CODE" );
    }
  }
  string sairpcity = city;
  if ( !airp.empty() ) {
    /* проверка на совпадение города с аэропортом */
    sairpcity = ((TAirpsRow&)baseairps.get_row( "code", airp )).city;
    if ( !city.empty() && sairpcity != city )
      throw AstraLocale::UserException( "MSG.GIVEN_AIRP_NOT_BELONGS_TO_GIVEN_CITY" );
  }
  node = GetNode( "time", filterNode );
  if ( node ) {
      /* будем переводить в UTC относительно порта в маршруте */
      firstTime = NodeAsDateTime( "first", node );
      lastTime = NodeAsDateTime( "last", node );
  }
  node = GetNode( "company", filterNode );
  if ( node ) {
  	try {
  		TElemFmt fmt;
      airline = ElemToElemId( etAirline, NodeAsString( node ), fmt ); // сконвертил в то как лежит в базе
    }
    catch( EConvertError &e ) {
    	throw AstraLocale::UserException( "MSG.AIRLINE.INVALID_GIVEN_CODE" );
    }
  }

  node = GetNode( "triptype", filterNode );
  if ( node ) {
  	TElemFmt fmt;
    triptype = ElemToElemId( etTripType, NodeAsString( node ), fmt );
		if ( fmt == efmtUnknown )
    	throw AstraLocale::UserException( "MSG.CHECK_FLIGHT.INVALID_TYPE" );
  }

  ProgTrace( TRACE5, "filter parse: season_idx=%d,range.first=%s,range.last=%s,days=%s,airp=%s,city=%s,time.first=%s,time.last=%s, airline=%s, triptype=%s",
             season_idx, DateTimeToStr( range.first, "dd.mm.yy hh:nn" ).c_str(), DateTimeToStr( range.last, "dd.mm.yy hh:nn" ).c_str(),
             range.days.c_str(),airp.c_str(),city.c_str(),DateTimeToStr( firstTime, "dd.mm.yy hh:nn" ).c_str(),
             DateTimeToStr( lastTime, "dd.mm.yy hh:nn" ).c_str(), airline.c_str(), triptype.c_str() );
}

/* здесь все уже в локальных временах */
void TFilter::Build( xmlNodePtr filterNode )
{
  tz_database &tz_db = get_tz_database();
  time_zone_ptr tz = tz_db.time_zone_from_region( region );
  NewTextChild( filterNode, "season_idx", 0 );
  NewTextChild( filterNode, "season_count", SEASON_PERIOD_COUNT );
  filterNode = NewTextChild( filterNode, "seasons" );
  int i=0;
  for ( vector<TSeason>::iterator p=periods.begin(); p!=periods.end(); p++ ) {
    xmlNodePtr node = NewTextChild( filterNode, "season" );
    NewTextChild( node, "index", IntToString( i - SEASON_PRIOR_PERIOD ) );
    NewTextChild( node, "summer", p->summer );
    NewTextChild( node, "first", DateTimeToStr( BoostToDateTime( local_date_time( p->period.begin(), tz ).local_time() ) ) );
    NewTextChild( node, "last", DateTimeToStr( BoostToDateTime( local_date_time( p->period.end(), tz ).local_time() ) ) );
    NewTextChild( node, "name", p->name );
    i++;
  }
}

void SeasonInterface::Filter(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
//  TReqInfo::Instance()->user.check_access( amWrite );
  TFilter filter;
  filter.GetSeason();
  xmlNodePtr dataNode = NewTextChild(resNode, "data");
  xmlNodePtr filterNode = NewTextChild(dataNode, "filter");
  filter.Build( filterNode );
}

void SeasonInterface::DelRangeList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
//  TReqInfo::Instance()->user.check_access( amWrite );
    TQuery Qry(&OraSession);
    Qry.SQLText =
    "BEGIN "
    "DELETE routes WHERE move_id IN (SELECT move_id FROM sched_days WHERE trip_id=:trip_id ); "
    "DELETE sched_days WHERE trip_id=:trip_id; END; ";
    Qry.CreateVariable( "trip_id", otInteger, NodeAsInteger( "trip_id", reqNode ) );
    Qry.Execute();
    TReqInfo::Instance()->MsgToLog("Удаление рейса ", evtSeason, NodeAsInteger("trip_id", reqNode));
  AstraLocale::showMessage( getLocaleText("MSG.FLIGHT_DELETED"));
}


void CreateSPP( BASIC::TDateTime localdate )
{
  //throw UserException( "Работа с экраном 'Сезонное расписание' временно остановлено. Идет обновление" );
  TPersWeights persWeights;
  TQuery MIDQry(&OraSession);
  MIDQry.SQLText =
   "BEGIN "\
   " SELECT move_id.nextval INTO :move_id from dual; "\
   " INSERT INTO move_ref(move_id,reference)  SELECT :move_id, NULL FROM dual; "\
   "END;";
  MIDQry.DeclareVariable( "move_id", otInteger );
  /* необходимо сделать проверку на не существование рейса */
  TBaseTable &TripTypes = base_tables.get("TRIP_TYPES");

  TQuery PQry(&OraSession);
  PQry.SQLText =
   "BEGIN "\
   " SELECT point_id.nextval INTO :point_id FROM dual; "
   " INSERT INTO points(point_id,move_id,point_num,airp,airp_fmt,pr_tranzit,first_point,airline,airline_fmt,"
   "                    flt_no,suffix,suffix_fmt,craft,craft_fmt,scd_in,scd_out,trip_type,litera,pr_del,tid,pr_reg) "\
   " SELECT :point_id,:move_id,:point_num,:airp,:airp_fmt,:pr_tranzit,:first_point,:airline,:airline_fmt,"\
   "        :flt_no,:suffix,:suffix_fmt,:craft,:craft_fmt,:scd_in,:scd_out,:trip_type,:litera,:pr_del,cycle_tid__seq.nextval,:pr_reg FROM dual; "\
   "END;";
  PQry.DeclareVariable( "point_id", otInteger );
  PQry.DeclareVariable( "move_id", otInteger );
  PQry.DeclareVariable( "point_num", otInteger );
  PQry.DeclareVariable( "airp", otString );
  PQry.DeclareVariable( "airp_fmt", otInteger );
  PQry.DeclareVariable( "pr_tranzit", otInteger );
  PQry.DeclareVariable( "first_point", otInteger );
  PQry.DeclareVariable( "airline", otString );
  PQry.DeclareVariable( "airline_fmt", otInteger );
  PQry.DeclareVariable( "flt_no", otInteger );
  PQry.DeclareVariable( "suffix", otString );
  PQry.DeclareVariable( "suffix_fmt", otInteger );
  PQry.DeclareVariable( "craft", otString );
  PQry.DeclareVariable( "craft_fmt", otInteger );
  PQry.DeclareVariable( "scd_in", otDate );
  PQry.DeclareVariable( "scd_out", otDate );
  PQry.DeclareVariable( "trip_type", otString );
  PQry.DeclareVariable( "litera", otString );
  PQry.DeclareVariable( "pr_del", otInteger );
  PQry.DeclareVariable( "pr_reg", otInteger );

  TQuery TQry(&OraSession);
  TQry.SQLText =
   "BEGIN "
   " sopp.set_flight_sets(:point_id,:use_seances,:f,:c,:y);"
   "END;";
  TQry.CreateVariable( "use_seances", otInteger, (int)USE_SEANCES() );
  TQry.DeclareVariable( "point_id", otInteger );
  TQry.DeclareVariable( "f", otInteger );
  TQry.DeclareVariable( "c", otInteger );
  TQry.DeclareVariable( "y", otInteger );
  TSpp spp;
  string err_city;
  createSPP( localdate, spp, false, err_city );
  TDoubleTrip doubletrip;

  for ( TSpp::iterator sp=spp.begin(); sp!=spp.end(); sp++ ) {
    tmapds &mapds = sp->second;
    for ( map<int,TDestList>::iterator im=mapds.begin(); im!=mapds.end(); im++ ) {
      TDests::iterator p = im->second.dests.end();
      int point_id,first_point;

      TElemFmt fmt;
      /* проверка на не существование */
      bool exists = false;
      string name;
      for ( TDests::iterator d=im->second.dests.begin(); d!=im->second.dests.end() - 1; d++ ) {
      	TDateTime vscd_in, vscd_out;
      	if ( d->scd_in > NoExists )
      		vscd_in = d->scd_in + im->second.diff;
      	else
      		vscd_in = NoExists;
      	if ( d->scd_out > NoExists )
      		vscd_out = d->scd_out + im->second.diff;
      	else
      		vscd_out = NoExists;

        int point_id;
      	if ( doubletrip.IsExists( NoExists, d->airline,
      		                        d->trip, d->suffix,
      		                        d->airp,
      		                        vscd_in, vscd_out,
                                  point_id ) ) {
      		exists = true;
      		break;
      	}
      }
      if ( exists ) {
 	      // А строка уже вставлена в таблицу move_ref
        continue;
      }

      MIDQry.Execute();
      int move_id = MIDQry.GetVariableAsInteger( "move_id" );
      PQry.SetVariable( "move_id", move_id );


      bool pr_tranzit;
      string airline, suffix, airp;
      TDateTime scd_out;
      vector<TTripInfo> flts;
      for ( TDests::iterator d=im->second.dests.begin(); d!=im->second.dests.end(); d++ ) {
        PQry.SetVariable( "point_num", d->num );
        airp = ElemToElemId( etAirp, d->airp, fmt );
        PQry.SetVariable( "airp", airp );
        PQry.SetVariable( "airp_fmt", (int)d->airp_fmt );

        pr_tranzit=( d != im->second.dests.begin() ) &&
                   ( p->airline + IntToString( p->trip ) + p->suffix ==
                     d->airline + IntToString( d->trip ) + d->suffix );  //!!!запись в одном месте


        PQry.SetVariable( "pr_tranzit", pr_tranzit );

        if (d != im->second.dests.begin() )
          PQry.SetVariable( "first_point", first_point );
        else
          PQry.SetVariable( "first_point",FNull );

        if ( d->airline.empty() ) {
          PQry.SetVariable( "airline", FNull );
          PQry.SetVariable( "airline_fmt", FNull );
          airline.clear();
        }
        else {
          airline = ElemToElemId( etAirline, d->airline, fmt );
          PQry.SetVariable( "airline", airline );
          PQry.SetVariable( "airline_fmt", (int)d->airline_fmt );
        }
        if ( d->trip == NoExists )
          PQry.SetVariable( "flt_no", FNull );
        else
          PQry.SetVariable( "flt_no", d->trip );
        if ( d->suffix.empty() ) {
          PQry.SetVariable( "suffix", FNull );
          PQry.SetVariable( "suffix_fmt", FNull );
          suffix.clear();
        }
        else {
          suffix = ElemToElemId( etSuffix, d->suffix, fmt );
          PQry.SetVariable( "suffix", suffix );
          PQry.SetVariable( "suffix_fmt", (int)d->suffix_fmt );
        }
        if ( d->craft.empty() ) {
          PQry.SetVariable( "craft", FNull );
          PQry.SetVariable( "craft_fmt", FNull );
        }
        else {
          PQry.SetVariable( "craft", ElemToElemId( etCraft, d->craft, fmt ) );
          PQry.SetVariable( "craft_fmt", (int)d->craft_fmt );
        }
        if ( d->scd_in == NoExists )
          PQry.SetVariable( "scd_in", FNull );
        else
          PQry.SetVariable( "scd_in", d->scd_in + im->second.diff );
        if ( d->scd_out == NoExists ) {
          PQry.SetVariable( "scd_out", FNull );
          scd_out = NoExists;
        }
        else {
          scd_out = d->scd_out + im->second.diff;
          PQry.SetVariable( "scd_out", scd_out );
        }
        if ( d->triptype.empty() )
          PQry.SetVariable( "trip_type", FNull );
        else
          PQry.SetVariable( "trip_type", d->triptype );
        if ( d->litera.empty() )
          PQry.SetVariable( "litera", FNull );
        else
          PQry.SetVariable( "litera", d->litera );
        PQry.SetVariable( "pr_del", d->pr_del );

        int pr_reg = ( d->scd_out > NoExists &&
                       ((TTripTypesRow&)TripTypes.get_row("code", d->triptype )).pr_reg != 0 &&
                       d->pr_del == 0 && d != im->second.dests.end() - 1 );
        if ( pr_reg ) {
          TDests::iterator r=d;
          r++;
          for ( ;r!=im->second.dests.end(); r++ ) {
            if ( !r->pr_del )
              break;
          }
          if ( r == im->second.dests.end() )
            pr_reg = 0;
        }
        PQry.SetVariable( "pr_reg", pr_reg );
        PQry.Execute();
        point_id = PQry.GetVariableAsInteger( "point_id" );
        if (!pr_tranzit)
          first_point=point_id;
        ProgTrace( TRACE5, "new line into points with point_id=%d", point_id );
        if ( pr_reg ) {
          TQry.SetVariable( "point_id", point_id );
          TQry.SetVariable( "f", d->f );
          TQry.SetVariable( "c", d->c );
          TQry.SetVariable( "y", d->y );
          TQry.Execute();
        }
        //вычисление весов пассажиров по рейсу
        PersWeightRules weights;
        persWeights.getRules( point_id, weights );
        weights.write( point_id );
        p = d;
        if ( !airline.empty() &&
              d->trip != NoExists &&
              scd_out != NoExists ) {
          TTripInfo tripInfo;
          tripInfo.airline = airline;
          tripInfo.flt_no = d->trip;
          tripInfo.suffix = suffix;
          tripInfo.airp = airp;
          tripInfo.scd_out = scd_out;
          flts.push_back( tripInfo );
        }
      } // end for dests
      bind_tlg_oper(flts, true);
    }
  }
  TReqInfo::Instance()->MsgToLog( string( "Получение СПП за " ) + DateTimeToStr( localdate, "dd.mm.yy" ), evtSeason );
}

void SeasonInterface::GetSPP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
//TReqInfo::Instance()->user.check_access( amWrite );
  TDateTime localdate;
  modf( (double)NodeAsDateTime( "date", reqNode ), &localdate );
  CreateSPP( localdate );
  AstraLocale::showMessage("MSG.DATA_SAVED");
}

bool insert_points( double da, int move_id, TFilter &filter, TDateTime first_day, int offset,
                    TDateTime vd, TDestList &ds, int ptz )
{
  ProgTrace( TRACE5, "first_day=%s", DateTimeToStr( first_day, "dd.mm.yy hh:nn" ).c_str() );

  TReqInfo *reqInfo = TReqInfo::Instance();
  bool canUseAirline, canUseAirp; /* можно ли использовать данный рейс */
/*  if ( reqInfo->user.user_type == utSupport ) {*/
    /* все права - все рейсы доступны если не указаны конкретные ак и ап*/
/*    canUseAirline = reqInfo->user.access.airlines.empty();
    canUseAirp = reqInfo->user.access.airps.empty();
  }
  else {
    canUseAirline = ( reqInfo->user.user_type == utAirport && reqInfo->user.access.airlines.empty() );
    canUseAirp = ( reqInfo->user.user_type == utAirline && reqInfo->user.access.airps.empty() );
  }*/
  canUseAirline = false; // new
  canUseAirp = false; //new
  // имеем move_id, vd на период выполнения
  // получим маршрут и проверим на права доступа к этому маршруту
  TQuery Qry(&OraSession);
  Qry.SQLText =
  " SELECT num, routes.airp, routes.airp_fmt, scd_in-TRUNC(scd_in)+:vdate+delta_in scd_in, "
  "        airline, airline_fmt, flt_no, craft, craft_fmt, "
  "        scd_out-TRUNC(scd_out)+:vdate+delta_out scd_out, trip_type, litera, "
  "        airps.city city, routes.pr_del, f, c, y, suffix, suffix_fmt "
  "  FROM routes, airps "
  " WHERE routes.move_id=:vmove_id AND routes.airp=airps.code  "
  " ORDER BY move_id,num";

  Qry.CreateVariable( "vdate", otDate, vd );
  Qry.CreateVariable( "vmove_id", otInteger, move_id );
  Qry.Execute();
  bool candests = false;
  ds.pr_del = true;
  ds.diff = filter.GetTZTimeDiff( da, first_day, ptz );
  ds.tz = ptz;
  double f1;
  while ( !Qry.Eof ) {
    TDest d;
    d.num = Qry.FieldAsInteger( "num" );
    d.airp = Qry.FieldAsString( "airp" );
    d.airp_fmt = (TElemFmt)Qry.FieldAsInteger( "airp_fmt" );
    d.city = Qry.FieldAsString( "city" );
    d.pr_del = Qry.FieldAsInteger( "pr_del" );
    if ( !d.pr_del )
      ds.pr_del = false;
    if ( Qry.FieldIsNULL( "scd_in" ) )
      d.scd_in = NoExists;
    else {
      d.scd_in = Qry.FieldAsDateTime( "scd_in" );
      modf( (double)d.scd_in, &f1 );
      if ( f1 == da ) {
        candests = candests || filter.isFilteredUTCTime( da, first_day, d.scd_in, offset );
      	ProgTrace( TRACE5, "filter.firsttime=%s, filter.lasttime=%s, d,scd_in=%s	, res=%d",
      	           DateTimeToStr( filter.firstTime, "dd hh:nn" ).c_str(),
      	           DateTimeToStr( filter.lastTime, "dd hh:nn" ).c_str(),
      	           DateTimeToStr( d.scd_in, "dd hh:nn" ).c_str(), candests );
      }
    }
    d.airline = Qry.FieldAsString( "airline" );
    d.airline_fmt = (TElemFmt)Qry.FieldAsInteger( "airline_fmt" );

    d.region = AirpTZRegion( Qry.FieldAsString( "airp" ), false );
    if ( Qry.FieldIsNULL( "flt_no" ) )
      d.trip = NoExists;
    else
    d.trip = Qry.FieldAsInteger( "flt_no" );
    d.craft = Qry.FieldAsString( "craft" );
    d.craft_fmt = (TElemFmt)Qry.FieldAsInteger( "craft_fmt" );
    d.litera = Qry.FieldAsString( "litera" );
    d.triptype = Qry.FieldAsString( "trip_type" );
    if ( Qry.FieldIsNULL( "scd_out" ) )
      d.scd_out = NoExists;
    else {
      d.scd_out = Qry.FieldAsDateTime( "scd_out" );
      modf( (double)d.scd_out, &f1 );
      if ( f1 == da ) {
        candests = candests || filter.isFilteredUTCTime( da, first_day, d.scd_out, offset );
      	ProgTrace( TRACE5, "filter.firsttime=%s, filter.lasttime=%s, d,scd_out=%s, res=%d",
      	           DateTimeToStr( filter.firstTime, "dd hh:nn" ).c_str(),
      	           DateTimeToStr( filter.lastTime, "dd hh:nn" ).c_str(),
      	           DateTimeToStr( d.scd_out, "dd hh:nn" ).c_str(), candests );
      }
    }
    d.f = Qry.FieldAsInteger( "f" );
    d.c = Qry.FieldAsInteger( "c" );
    d.y = Qry.FieldAsInteger( "y" );
    d.suffix = Qry.FieldAsString( "suffix" );
    d.suffix_fmt = (TElemFmt)Qry.FieldAsInteger( "suffix_fmt" );
    if ( reqInfo->CheckAirp( d.airp ) ) // new
    	canUseAirp = true; //new
    if ( reqInfo->CheckAirline( d.airline ) ) //new
    	canUseAirline = true; //new
    // фильтр по временам прилета/вылета в каждом п.п.
    ds.dests.push_back( d );
    Qry.Next();
  } // end while
  if ( !canUseAirline || !canUseAirp || !candests ) {
    ds.dests.clear();
    ProgTrace( TRACE5, "clear dests move_id=%d, date=%s", move_id, DateTimeToStr( da, "dd.hh:nn" ).c_str() );
  }
  return !ds.dests.empty();
}

// времена в фильтре хранятся в UTC
void createTrips( TDateTime utc_spp_date, TDateTime localdate, TFilter &filter, int offset,
                  TDestList &ds, string &err_city )
{
  TDateTime firstTime = filter.firstTime;
  TDateTime lastTime = filter.lastTime;

  TReqInfo *reqInfo = TReqInfo::Instance();

  if ( reqInfo->user.user_type != utAirport ) {
    filter.firstTime = NoExists;
    createAirlineTrip( NoExists, filter, offset, ds, localdate, err_city );
  }
  else {
  	TStageTimes stagetimes( sRemovalGangWay );
    for ( vector<string>::iterator s=reqInfo->user.access.airps.begin();
          s!=reqInfo->user.access.airps.end(); s++ ) {
      int vcount = (int)ds.trips.size();
      // создаем рейсы относительно разрешенных портов reqInfo->user.access.airps

      createAirportTrip( *s, NoExists, filter, offset, ds, utc_spp_date, false, true, err_city );
      for ( int i=vcount; i<(int)ds.trips.size(); i++ ) {
      	ds.trips[ i ].trap = stagetimes.GetTime( ds.trips[ i ].airlineId, ds.trips[ i ].airpId, ds.trips[ i ].craftId, ds.trips[ i ].triptypeId, ds.trips[ i ].scd_out );
      }
    }
  }
  filter.firstTime = firstTime;
  filter.lastTime = lastTime;
}

string GetRegionFromTZ( int ptz, map<int,string> &mapreg )
{
  string res;
  if ( mapreg.find( ptz ) != mapreg.end() )
    return mapreg[ ptz ];
  TQuery Qry( &OraSession );
  Qry.SQLText = "SELECT region FROM tz_regions WHERE tz=:ptz AND country='РФ' AND pr_del=0";
  Qry.CreateVariable( "ptz", otInteger, ptz );
  Qry.Execute();
  if ( !Qry.Eof ) {
    res = Qry.FieldAsString( "region" );
    mapreg[ ptz ] = res;
  }
  return res;
}

void createSPP( TDateTime localdate, TSpp &spp, bool createViewer, string &err_city )
{
  //throw UserException( "Работа с экраном 'Сезонное расписание' временно остановлено. Идет обновление" );
	map<int,string> mapreg;
  map<int,TTimeDiff> v;
  TFilter filter;
  filter.GetSeason();

  TQuery Qry(&OraSession);
  double d1, d2, f1, f2, f3, f4;
  d1 = ClientToUTC( localdate, filter.region );
  d2 = ClientToUTC( localdate + 1 - 1/1440, filter.region );
  ProgTrace( TRACE5, "spp on local date %s, utc date and time begin=%s, end=%s",
             DateTimeToStr( localdate, "dd.mm.yy" ).c_str(),
             DateTimeToStr( d1, "dd.mm.yy hh:nn" ).c_str(),
             DateTimeToStr( d2, "dd.mm.yy hh:nn" ).c_str() );
  // для начала надо получить список периодов, которые выполняются в эту дату, пока без учета времени
  Qry.SQLText =
  " SELECT DISTINCT move_id,first_day,last_day,:vd-delta AS qdate,pr_del,d.tz tz "
  "  FROM "
  "  ( SELECT routes.move_id as move_id,"
  "           TO_NUMBER(delta_in) as delta,"
  "           sched_days.pr_del as pr_del,"
  "           first_day,last_day,tz "
  " FROM sched_days,routes "
  " WHERE routes.move_id = sched_days.move_id AND "
  "       TRUNC(first_day) + delta_in <= :vd AND "
  "       TRUNC(last_day) + delta_in >= :vd AND  "
  "       INSTR( days, TO_CHAR( :vd - delta_in, 'D' ) ) != 0 "
  "   UNION "
  " SELECT routes.move_id as move_id, "
  "        TO_NUMBER(delta_out) as delta,"
  "        sched_days.pr_del as pr_del,"
  "        first_day,last_day,tz "
  " FROM sched_days,routes "
  "   WHERE routes.move_id = sched_days.move_id AND "
  "         TRUNC(first_day) + delta_out <= :vd AND "
  "         TRUNC(last_day) + delta_out >= :vd AND "
  "         INSTR( days, TO_CHAR( :vd - delta_out, 'D' ) ) != 0 ) d "
  " ORDER BY move_id, qdate";
   Qry.DeclareVariable( "vd", otDate );
   f3 = modf( d1, &f1 );
   f4 = modf( d2, &f2 );
   TDestList ds;

   for ( double d=f1; d<=f2; d++ ) {
     if ( d == f1 ) {
       filter.firstTime = f3;
       filter.lastTime = 1.0 - 1.0/1440.0;
     }
     else {
       filter.firstTime = 0.0;
       filter.lastTime = f4 - 1.0/1440.0;
     }
     ProgTrace( TRACE5, "date=%s",
                DateTimeToStr( d, "dd.mm.yy  hh:nn" ).c_str() );
     Qry.SetVariable( "vd", d );
     Qry.Execute();
     vector<TDateTime> days;
     int vmove_id = -1;
     int ptz;
     string pregion;
     TDateTime first_day, last_day;
     while ( 1 ) {
       if ( vmove_id > 0 && ( Qry.Eof || vmove_id != Qry.FieldAsInteger( "move_id" ) ) ) {
        // цикл по полученным датам
         for ( vector<TDateTime>::iterator vd=days.begin(); vd!=days.end(); vd++ ) {

           int offset = GetTZOffSet( first_day, ptz, v );

           ProgTrace( TRACE5, "day=%s, move_id=%d",
                      DateTimeToStr( *vd, "dd.mm.yy hh:nn" ).c_str(), vmove_id );

           if ( insert_points( d, vmove_id, filter, first_day, offset, *vd, ds, ptz ) ) { // имеем права с маршрутом работать + фильтр по временам
              ds.flight_time = first_day;
              ds.last_day = last_day;
              ds.tz = ptz;
              ds.region = pregion;

              ProgTrace( TRACE5, "canspp trip d=%s spp[ %s ][ %d ].trips.size()=%d",
                         DateTimeToStr( d, "dd.mm.yy hh:nn" ).c_str(),
                         DateTimeToStr( *vd, "dd.mm.yy hh:nn" ).c_str(),
                         vmove_id,
                         (int)spp[ *vd ][ vmove_id ].trips.size() );
              if ( createViewer ) {
              	vector<trip> trips = spp[ *vd ][ vmove_id ].trips; // сохраняем уже полученные рейсы
/*                if ( spp[ *vd ][ vmove_id ].trips.empty() ) {*/
                  createTrips( d, localdate, filter, offset, ds, err_city );
                  // удаление дублирующих роейсов
                  for ( vector<trip>::iterator itr=trips.begin(); itr!=trips.end(); itr++ ) {
                  	vector<trip>::iterator jtr=ds.trips.begin();
                  	for ( ; jtr!=ds.trips.end(); jtr++ )
                  	  if ( itr->name == jtr->name && itr->scd_out == jtr->scd_out && itr->scd_in == jtr->scd_in )
                  	  	break;
                  	if ( jtr == ds.trips.end() )
                  		ds.trips.push_back( *itr );
                  }
                  ProgTrace( TRACE5, "ds.trips.size()=%d", (int)ds.trips.size() );
                /*}
                else
                  ds.trips = spp[ *vd ][ vmove_id ].trips;*/
              }
              spp[ *vd ][ vmove_id ] = ds;
/*              ProgTrace( TRACE5, "vmove_id=%d, vd=%f, spp[ *vd ][ vmove_id ].dests.size()=%d", vmove_id, *vd, spp[ *vd ][ vmove_id ].dests.size() );
              tst();*/
           } // end insert
           ProgTrace( TRACE5, "first_day=%s, move_id=%d",
                      DateTimeToStr( first_day, "dd.mm.yy hh:nn" ).c_str(),
                      vmove_id );
           ds.dests.clear();
           ds.trips.clear();
         } // end for days
         days.clear();
       } // end if
       if ( Qry.Eof )
        break;
       vmove_id = Qry.FieldAsInteger( "move_id" );
       ptz = Qry.FieldAsInteger( "tz" );
//       pregion = Qry.FieldAsString( "region" );
       pregion = GetRegionFromTZ( ptz, mapreg );
       first_day = Qry.FieldAsDateTime( "first_day" );
       last_day = Qry.FieldAsDateTime( "last_day" );

       if ( find( days.begin(), days.end(), Qry.FieldAsDateTime( "qdate" ) ) == days.end() )
         days.push_back( Qry.FieldAsDateTime( "qdate" ) );
       Qry.Next();
     }
   }
}

bool CompareAirlineTrip( trip t1, trip t2 )
{
  if ( t1.name.size() < t2.name.size() )
	  return true;
	else
		if ( t1.name.size() > t2.name.size() )
			return false;
		else
		  if ( t1.name < t2.name )
		    return true;
      else
        if ( t1.name > t2.name )
        	return false;
        else
   		    if ( t1.trip_id < t2.trip_id )
  	  		  return true;
  	  		else
  	  		  return false;
};

bool CompareAirpTrip( trip t1, trip t2 )
{
	TDateTime f1, f2;
	double d;
	if ( t1.scd_out > NoExists )
		f1 = modf( t1.scd_out, &d );
	else
		f1 = modf( t1.scd_in, &d );
	if ( t2.scd_out > NoExists )
		f2 = modf( t2.scd_out, &d );
	else
		f2 = modf( t2.scd_in, &d );
	if ( f1 < f2 )
		return true;
	else
		if ( f1 > f2 )
			return false;
	  else
	  	return CompareAirlineTrip( t1, t2 );

}

void SeasonInterface::ViewSPP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
	string err_city;
  TReqInfo *reqInfo = TReqInfo::Instance();
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  if ( reqInfo->user.user_type == utAirport  ) {
    NewTextChild( dataNode, "mode", "port" );
  }
  else {
    NewTextChild( dataNode, "mode", "airline" );
  }
  TSpp spp;
  vector<trip> ViewTrips;
  map<int,TTimeDiff> v;
  TDateTime vdate;
  modf( (double)NodeAsDateTime( "date", reqNode ), &vdate );
  createSPP( vdate, spp, true, err_city );
  for ( TSpp::iterator sp=spp.begin(); sp!=spp.end(); sp++ ) {
    tmapds &mapds = sp->second;
    for ( map<int,TDestList>::iterator im=mapds.begin(); im!=mapds.end(); im++ ) {
      ProgTrace( TRACE5, "build xml vdate=%s, move_id=%d, trips.size()=%d",
                 DateTimeToStr( sp->first, "dd.mm.yy" ).c_str(),
                 im->first,
                 (int)im->second.trips.size() );
      for ( vector<trip>::iterator tr=im->second.trips.begin(); tr!=im->second.trips.end(); tr++ ) {
        ViewTrips.push_back( *tr );
      }
      im->second.trips.clear();
    }
  }
  if ( reqInfo->user.user_type != utAirport )
  	sort( ViewTrips.begin(), ViewTrips.end(), CompareAirlineTrip );
  else
  	sort( ViewTrips.begin(), ViewTrips.end(), CompareAirpTrip );

  xmlNodePtr tripsSPP = NULL;
  for ( vector<trip>::iterator tr=ViewTrips.begin(); tr!=ViewTrips.end(); tr++ ) {
    if ( !tripsSPP )
      tripsSPP = NewTextChild( dataNode, "tripsSPP" );
    xmlNodePtr tripNode = NewTextChild( tripsSPP, "trip" );
    NewTextChild( tripNode, "trip", tr->name );
    NewTextChild( tripNode, "print_name", tr->name ); //tr->print_name???
    if ( reqInfo->user.user_type != utAirport  )
      NewTextChild( tripNode, "craft", tr->crafts );
    else {
      NewTextChild( tripNode, "craft", tr->owncraft );
      NewTextChild( tripNode, "ownport", tr->ownport );
    }
    NewTextChild( tripNode, "triptype", tr->triptype );
    if ( reqInfo->user.user_type == utAirport  ) {
      /* only for prior version */
  	  string ports_in, ports_out;
     	for ( vector<TDest>::iterator h=tr->vecportsFrom.begin(); h!=tr->vecportsFrom.end(); h++ ) {
         if ( !ports_in.empty() )
           ports_in += "/";
         ports_in += ElemIdToElemCtxt( ecDisp, etAirp, h->airp, h->airp_fmt );
  	  }
  	  for ( vector<TDest>::iterator h=tr->vecportsTo.begin(); h!=tr->vecportsTo.end(); h++ ) {
         if ( !ports_out.empty() )
           ports_out += "/";
         ports_out += ElemIdToElemCtxt( ecDisp, etAirp, h->airp, h->airp_fmt );
   	  }
      NewTextChild( tripNode, "ports", ports_in );
      NewTextChild( tripNode, "ports_out", ports_out );
    }
    else {
      /*old if ( !ports_in.empty() && !ports_out.empty() )
        NewTextChild( tripNode, "ports_out", ports_in + "/" + ports_out );
      else
        NewTextChild( tripNode, "ports_out", ports_in + ports_out );*/
    }
   	/* end old version */

   	/* new version */
   	if ( reqInfo->user.user_type != utAirport )
   		NewTextChild( tripNode, "ports_out", tr->portsForAirline ); /* очень трудно рассчитывается это поле, поэтому так */
   	if ( !tr->vecportsFrom.empty() ) {
   	  xmlNodePtr psNode = NewTextChild( tripNode, "portsFrom" );
   	  for ( vector<TDest>::iterator h=tr->vecportsFrom.begin(); h!=tr->vecportsFrom.end(); h++ ) {
   	  	xmlNodePtr pNode = NewTextChild( psNode, "port" );
   	  	NewTextChild( pNode, "airp", ElemIdToElemCtxt( ecDisp, etAirp, h->airp, h->airp_fmt ) );
   	  	if ( h->scd_in > NoExists )
   	  	  NewTextChild( pNode, "land", DateTimeToStr( h->scd_in ) );
   	  	if ( h->scd_out > NoExists )
   	  		NewTextChild( pNode, "takeoff", DateTimeToStr( h->scd_out ) );
   	  	if( h->pr_del )
   	  		NewTextChild( pNode, "pr_cancel", h->pr_del );
      }
    }
   	if ( !tr->vecportsTo.empty() ) {
   	  xmlNodePtr psNode = NewTextChild( tripNode, "portsTo" );
   	  for ( vector<TDest>::iterator h=tr->vecportsTo.begin(); h!=tr->vecportsTo.end(); h++ ) {
   	  	xmlNodePtr pNode = NewTextChild( psNode, "port" );
   	  	NewTextChild( pNode, "airp", ElemIdToElemCtxt( ecDisp, etAirp, h->airp, h->airp_fmt ) );
   	  	if ( h->scd_in > NoExists )
   	  	  NewTextChild( pNode, "land", DateTimeToStr( h->scd_in ) );
   	  	if ( h->scd_out > NoExists )
   	  		NewTextChild( pNode, "takeoff", DateTimeToStr( h->scd_out ) );
   	  	if( h->pr_del )
   	  		NewTextChild( pNode, "pr_cancel", h->pr_del );
      }
    }
   	/* end new version */

    if ( !tr->bold_ports.empty() )
      NewTextChild( tripNode, "bold_ports", tr->bold_ports );
    if ( tr->scd_in > NoExists )
      NewTextChild( tripNode, "land", DateTimeToStr( tr->scd_in ) );
    if ( tr->scd_out > NoExists ) {
      NewTextChild( tripNode, "takeoff", DateTimeToStr( tr->scd_out ) );
      NewTextChild( tripNode, "trap", DateTimeToStr( tr->trap ) );
    }
    if ( tr->pr_del )
      NewTextChild( tripNode, "ref", AstraLocale::getLocaleText("Отмена") );
  }
 if ( !err_city.empty() )
    AstraLocale::showErrorMessage( "MSG.CITY.REGION_NOT_DEFINED.NOT_ALL_FLIGHTS_ARE_SHOWN",
    	                             LParams() << LParam("city", ElemIdToCodeNative(etCity,err_city)));
}

void VerifyRangeList( TRangeList &rangeList, map<int,TDestList> &mapds )
{
  vector<string> flg;
  // проверка маршрута
  for ( map<int,TDestList>::iterator im=mapds.begin(); im!=mapds.end(); im++ ) {
ProgTrace( TRACE5, "(int)im->second.dests.size()=%d", (int)im->second.dests.size() );
    if ( (int)im->second.dests.size() < 2 )
      throw AstraLocale::UserException( "MSG.CHECK_FLIGHT.ROUTE_LEAST_TWO_POINTS" );
    im->second.dests.begin()->scd_in = NoExists;
    TDests::iterator enddest = im->second.dests.end() - 1;
    enddest->airline.clear();
    enddest->trip = NoExists;
    enddest->craft.clear();
    enddest->f = 0;
    enddest->c = 0;
    enddest->y = 0;
    enddest->scd_out = NoExists;
    enddest->litera.clear();
    enddest->triptype.clear();
    enddest->unitrip.clear();
    enddest->suffix.clear();
    string fairline = im->second.dests.begin()->airline;
    bool notime = true;
    int notpr_del = 0;
    flg.clear();
    TDests::iterator pid;
    for ( TDests::iterator id=im->second.dests.begin(); id!=im->second.dests.end(); id++ ) {
ProgTrace( TRACE5, "id->airline=%s", id->airline.c_str() );
      if ( id->airp.empty() )
        throw AstraLocale::UserException( "MSG.CHECK_FLIGHT.NOT_SET_DEST_CODE" );

      if ( id < enddest ) {
        if ( id->airline.empty() )
          throw AstraLocale::UserException( "MSG.AIRLINE.NOT_SET" );
        if ( fairline != id->airline )
          throw AstraLocale::UserException( "MSG.CHECK_FLIGHT.ROUTE_CANNOT_BELONG_TO_DIFFERENT_AIRLINES" );
        if ( id->trip == NoExists )
          throw AstraLocale::UserException( "MSG.CHECK_FLIGHT.NOT_SET_FLT_NO" );
        if ( id->craft.empty() )
          throw AstraLocale::UserException( "MSG.CRAFT.NOT_SET" );
        string f = IntToString( id->trip ) + id->airp;
        if ( find( flg.begin(), flg.end(), f ) != flg.end() )
          throw AstraLocale::UserException( "MSG.CHECK_FLIGHT.ROUTE_HAVE_DEST_EQUAL_FLT_NO" );
        else
          flg.push_back( f );
      }
/*29.10 for chelb      if ( id != im->second.dests.begin() && id->airp == pid->airp )
        throw UserException( "Маршрут не может содержать два одинаковых подряд идущих п.п." );*/
      if ( !id->pr_del )
        notpr_del++;
ProgTrace( TRACE5, "airp=%s, scd_in=%f, scd_out=%f", id->airp.c_str(), id->scd_in, id->scd_out );
      if ( id->scd_in > NoExists || id->scd_out > NoExists )
        notime = false;
      pid = id;
    } /* end for */
    if ( notime )
      throw AstraLocale::UserException( "MSG.CHECK_FLIGHT.EMPTY_TIME_IN_ROUTE" );
    /* весь рейс отменен, т.к. встретилось не более одного не отмененного пункта посадки */
    if ( notpr_del <= 1 ) {
     for ( TDests::iterator id=im->second.dests.begin(); id!=im->second.dests.end(); id++ ) {
       id->pr_del = true;
     }
     im->second.pr_del = true;
    }
    else
      im->second.pr_del = false;
  }
  // проверка диапазонов + проставление отмененных периодов
  for ( vector<TPeriod>::iterator ip=rangeList.periods.begin(); ip!=rangeList.periods.end(); ip++) {
    map<int,TDestList>::iterator im = mapds.find( ip->move_id );
    if ( im == mapds.end() || im->second.dests.empty( ) )
      throw AstraLocale::UserException( "MSG.CHECK_FLIGHT.ROUTE_NOT_SPECIFIED_FOR_RANGE" );
    ip->pr_del = im->second.pr_del;
    if ( ip->first > ip->last )
    	throw AstraLocale::UserException( "MSG.CHECK_FLIGHT.FIRST_DATE_BIGGER_THAN_LAST_ONE",
                LParams()
                << LParam("first", DateTimeToStr( ip->first, "dd.mm.yy" ))
                <<LParam("last", DateTimeToStr( ip->last, "dd.mm.yy" )));
  }
}

// разбор и перевод времен в UTC, в диапазонах выполнения хранятся времена вылета
bool ParseRangeList( xmlNodePtr rangelistNode, TRangeList &rangeList, map<int,TDestList> &mapds, string &filter_region )
{
  TBaseTable &baseairps = base_tables.get( "airps" );
  TReqInfo *reqInfo = TReqInfo::Instance();
  bool canUseAirline, canUseAirp; /* можно ли использовать данный рейс */
  canUseAirline = false; // new
  canUseAirp = false; //new
  mapds.clear();
  string code;
  rangeList.periods.clear();
  if ( !rangelistNode )
   return true;
  xmlNodePtr node = GetNode( "trip_id", rangelistNode );
  if ( !node )
    rangeList.trip_id = NoExists;
  else
    rangeList.trip_id = NodeAsInteger( node );
  xmlNodePtr rangeNode = rangelistNode->children;
  TDestList ds;
  while ( rangeNode ) {
    TPeriod period;
    xmlNodePtr curNode = rangeNode->children;
    string modify = NodeAsStringFast( "modify", curNode );
//   	ambiguous_timeNode = GetNodeFast( "ambiguous_time", curNode );

    if ( modify == "delete" ) {
      rangeNode = rangeNode->next;
      continue;
    }
    if ( modify == "change" )
      period.modify = fchange;
    else
      if ( modify == "nochange" )
        period.modify = fnochange;
      else
        period.modify = finsert;
    period.move_id = NodeAsIntegerFast( "move_id", curNode );
    modf( (double)NodeAsDateTimeFast( "first", curNode ), &period.first );
    modf( (double)NodeAsDateTimeFast( "last", curNode ), &period.last );
    ProgTrace( TRACE5, "first=%s, last=%s",
               DateTimeToStr( period.first, "dd.mm.yyyy hh:nn:ss" ).c_str(),
               DateTimeToStr( period.last, "dd.mm.yyyy hh:nn:ss" ).c_str() );
    period.days = NodeAsStringFast( "days", curNode );
    node = GetNodeFast( "tlg", curNode );
    if ( node )
      period.tlg = NodeAsString( node );
    node = GetNodeFast( "ref", curNode );
    if ( node )
      period.ref = NodeAsString( node );
    node = GetNodeFast( "dests", curNode );
    double first_day, f2, f3;
    modf( (double)period.first, &first_day );
    bool newdests = node;
    if ( newdests ) {
      ds.dests.clear();
      ds.flight_time = NoExists;
//      ds.first_dest = NoExists;
      xmlNodePtr destNode = node->children;
      while ( destNode ) {
        TDest dest;
        curNode = destNode->children;
       	try {
          dest.airp = ElemCtxtToElemId( ecDisp, etAirp, NodeAsStringFast( "cod", curNode ), dest.airp_fmt, false ); // сконвертил в то как лежит в базе
        }
        catch( EConvertError &e ) {
    	    throw AstraLocale::UserException( "MSG.AIRP.INVALID_GIVEN_CODE" );
        }
        if ( dest.airp.empty() )
          throw AstraLocale::UserException( "MSG.AIRP.INVALID_GIVEN_CODE" );
        dest.city = ((TAirpsRow&)baseairps.get_row( "code", dest.airp )).city;
        dest.region = CityTZRegion( dest.city );
        node = GetNodeFast( "cancel", curNode );
        if ( node )
          dest.pr_del = NodeAsInteger( node );
        else
          dest.pr_del = 0;
        node = GetNodeFast( "land", curNode );
        if ( node ) {
          dest.scd_in = NodeAsDateTime( node );
          modf( (double)dest.scd_in, &f2 );
          if ( ds.flight_time == NoExists && f2 == 0 ) {
            ds.flight_time = dest.scd_in;
            ds.region = dest.region;
          }
        }
        else
          dest.scd_in = NoExists;
        node = GetNodeFast( "company", curNode );
        if ( node ) {
         	try {
            dest.airline = ElemCtxtToElemId( ecDisp, etAirline, NodeAsString( node ), dest.airline_fmt, false );
          }
          catch( EConvertError &e ) {
    	      throw AstraLocale::UserException( "MSG.AIRLINE.INVALID_GIVEN_CODE" );
          }
        }
        node = GetNodeFast( "trip", curNode );
        if ( node )
          dest.trip = NodeAsInteger( node );
        else
          dest.trip = NoExists;
        node = GetNodeFast( "bc", curNode );
        if ( node ) {
         	try {
            dest.craft = ElemCtxtToElemId( ecDisp, etCraft, NodeAsString( node ), dest.craft_fmt, false );
          }
          catch( EConvertError &e ) {
    	      throw AstraLocale::UserException( "MSG.CRAFT.WRONG_SPECIFIED" );
          }
        }
        node = GetNodeFast( "litera", curNode );
        if ( node )
          dest.litera = NodeAsString( node );
        node = GetNodeFast( "triptype", curNode );
        if ( node ) {
        	TElemFmt fmt;
          dest.triptype = NodeAsString( node );
          if ( !dest.triptype.empty() ) {
            dest.triptype = ElemToElemId( etTripType, dest.triptype, fmt );
        		if ( fmt == efmtUnknown )
            	throw AstraLocale::UserException( "MSG.CHECK_FLIGHT.INVALID_TYPE" );
          }
        }
        else
          dest.triptype = DefaultTripType(false);
        node = GetNodeFast( "takeoff", curNode );
        if ( node ) {
        	dest.scd_out = NodeAsDateTime( node );
        	modf( (double)dest.scd_out, &f2 );
        	if ( ds.flight_time == NoExists && f2 == 0 ) {
              ds.flight_time = dest.scd_out;
              ds.region = dest.region;
          }
        }
        else
          dest.scd_out = NoExists;
        node = GetNodeFast( "f", curNode );
        if ( node )
          dest.f = NodeAsInteger( node );
        else
          dest.f = 0;
        node = GetNodeFast( "c", curNode );
        if ( node )
          dest.c = NodeAsInteger( node );
        else
          dest.c = 0;
        node = GetNodeFast( "y", curNode );
        if ( node )
          dest.y = NodeAsInteger( node );
        else
          dest.y = 0;
        node = GetNodeFast( "unitrip", curNode );
        if ( node )
          dest.unitrip = NodeAsString( node );
        node = GetNodeFast( "suffix", curNode );
        if ( node ) {
         	try {
            dest.suffix = ElemCtxtToElemId( ecDisp, etSuffix, NodeAsString( node ), dest.suffix_fmt, false );
          }
          catch( EConvertError &e ) {
    	      throw AstraLocale::UserException( "MSG.SUFFIX.INVALID.NO_PARAM" );
          }
        }
        if ( reqInfo->CheckAirp( dest.airp ) ) // new
        	canUseAirp = true; //new
        if ( reqInfo->CheckAirline( dest.airline ) ) //new
    	    canUseAirline = true; //new
        ds.dests.push_back( dest );
        destNode = destNode->next;
      } // while ( destNode )
      if ( !canUseAirline || !canUseAirp )
        throw AstraLocale::UserException( "MSG.INSUFFICIENT_RIGHTS.NOT_ACCESS" );
//      ProgTrace( TRACE5, "first_dest=%d", ds.first_dest );
      if ( mapds.find( period.move_id ) == mapds.end() ) //! ввели новый период (рейс) и сразу расширили его новой датой
        mapds.insert(std::make_pair( period.move_id, ds ) );
      else
      	newdests = false; // используем старый маршрут
    } // if ( node )
    // периоды хранять время вылета из п.п. переводим в UTC
    ds = mapds[ period.move_id ];
    if ( ds.dests.empty() )
      throw AstraLocale::UserException( "MSG.ROUTE_NOT_SPECIFIED_FOR_PERIOD" );
    if ( ds.flight_time == NoExists )
      throw AstraLocale::UserException( "MSG.CHECK_FLIGHT.TIMES_SHIFT_BY_DATE" );
    ProgTrace( TRACE5, "first=%s, last=%s, flight_time=%s, flight_time=%f",
               DateTimeToStr( period.first, "dd.mm.yyyy hh:nn:ss" ).c_str(),
               DateTimeToStr( period.last, "dd.mm.yyyy hh:nn:ss" ).c_str(),
               DateTimeToStr( ds.flight_time, "dd.mm.yyyy hh:nn:ss" ).c_str(),
               ds.flight_time );
    first_day = period.first;
    period.first += ds.flight_time;
    ProgTrace( TRACE5, "period.first=%s, period.last=%s, period.days=%s",
               DateTimeToStr( period.first, "dd.mm.yyyy hh:nn:ss" ).c_str(),
               DateTimeToStr( period.last, "dd.mm.yyyy hh:nn:ss" ).c_str(),
               period.days.c_str() );
 		try {
      period.first = ClientToUTC( (double)period.first, filter_region );
 	  }
    catch( boost::local_time::ambiguous_result ) {
    	period.first = ClientToUTC( (double)period.first + 1, filter_region ) - 1;
    }
    catch( boost::local_time::time_label_invalid ) {
      throw AstraLocale::UserException( "MSG.FLIGHT_TIME_NOT_EXISTS",
              LParams() << LParam("time", DateTimeToStr( period.first, "dd.mm hh:nn" )));
    }
    double utcFirst;
    f3 = modf( (double)period.first, &utcFirst );
    if ( first_day != utcFirst ) {
      period.days = AddDays( period.days, (int)utcFirst - (int)first_day );
    }
    period.last += utcFirst - first_day + f3;
    ProgTrace( TRACE5, "local first=%s",DateTimeToStr( first_day, "dd.mm.yyyy hh:nn:ss" ).c_str() );
    ProgTrace( TRACE5, "utc first=%s",DateTimeToStr( utcFirst, "dd.mm.yyyy hh:nn:ss" ).c_str() );

    if ( newdests ) {
      // перевод времен в маршруте в локальные
      for ( TDests::iterator id=ds.dests.begin(); id!=ds.dests.end(); id++ ) {
      	if ( id->scd_in > NoExists ) {
      		f2 = modf( (double)id->scd_in, &f3 );
      		f3 += first_day + fabs( f2 );
          ProgTrace( TRACE5, "local scd_in=%s",DateTimeToStr( f3, "dd.mm.yyyy hh:nn:ss" ).c_str() );

      		try {
      	    f2 = modf( (double)ClientToUTC( f3, id->region ), &f3 );
      	  }
          catch( boost::local_time::ambiguous_result ) {
          	f2 = modf( (double)ClientToUTC( f3 + 1, id->region ) - 1, &f3 );
          }
          catch( boost::local_time::time_label_invalid ) {
            throw AstraLocale::UserException( "MSG.ARV_TIME_FOR_POINT_NOT_EXISTS",
                    LParams() << LParam("airp", ElemIdToCodeNative(etAirp,id->airp)) << LParam("time", DateTimeToStr( period.first, "dd.mm" )));
          }
          ProgTrace( TRACE5, "trunc(scd_in)=%s, time=%s",
                     DateTimeToStr( f3, "dd.mm.yyyy hh:nn:ss" ).c_str(),
                     DateTimeToStr( f2, "dd.mm.yyyy hh:nn:ss" ).c_str() );

    	    if ( f3 < utcFirst )
            id->scd_in = f3 - utcFirst - f2;
          else
            id->scd_in = f3 - utcFirst + f2;
          ProgTrace( TRACE5, "utc scd_in=%s", DateTimeToStr( id->scd_in, "dd.mm.yyyy hh:nn:ss" ).c_str() );
        }
    	  if ( id->scd_out > NoExists ) {
      		f2 = modf( (double)id->scd_out, &f3 );
      		f3 += first_day + fabs( f2 );
          ProgTrace( TRACE5, "local scd_out=%s",DateTimeToStr( f3, "dd.mm.yyyy hh:nn:ss" ).c_str() );
    	  	try {
    	      f2 = modf( (double)ClientToUTC( f3, id->region ), &f3 );
    	    }
          catch( boost::local_time::ambiguous_result ) {
          	f2 = modf( (double)ClientToUTC( f3 + 1, id->region ) - 1, &f3 );
/*            throw UserException( "Время вылета рейса в пункте %s не определено однозначно %s",
                                 id->airp.c_str(),
                                 DateTimeToStr( first_day, "dd.mm" ).c_str() );*/
          }
          catch( boost::local_time::time_label_invalid ) {
            throw AstraLocale::UserException( "MSG.DEP_TIME_FOR_POINT_NOT_EXISTS",
                    LParams() << LParam("airp", ElemIdToCodeNative(etAirp,id->airp)) << LParam("time", DateTimeToStr( period.first, "dd.mm" )));
          }
          ProgTrace( TRACE5, "trunc(scd_out)=%s, time=%s",
                     DateTimeToStr( f3, "dd.mm.yyyy hh:nn:ss" ).c_str(),
                     DateTimeToStr( f2, "dd.mm.yyyy hh:nn:ss" ).c_str() );
    	    if ( f3 < utcFirst )
            id->scd_out = f3 - utcFirst - f2;
          else
            id->scd_out = f3 - utcFirst + f2;
          ProgTrace( TRACE5, "utc scd_out=%s",DateTimeToStr( id->scd_out, "dd.mm.yyyy hh:nn:ss" ).c_str() );
        }
      } // end for
      mapds[ period.move_id ] = ds;
    }
    ProgTrace( TRACE5, "period.first=%s, period.last=%s, period.days=%s, move_id=%d",
               DateTimeToStr( period.first, "dd.mm.yyyy hh:nn:ss" ).c_str(),
               DateTimeToStr( period.last, "dd.mm.yyyy hh:nn:ss" ).c_str(),
               period.days.c_str(),
               period.move_id );
    rangeList.periods.push_back( period );
    rangeNode = rangeNode->next;
  } // END WHILE
  return true;
}

void SeasonInterface::Write(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  //TReqInfo::Instance()->user.check_access( amWrite );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  vector<TPeriod> oldperiods;
  TFilter filter;
  xmlNodePtr filterNode = GetNode( "filter", reqNode );
  filter.Parse( filterNode );
  TRangeList rangeList;
  map<int,TDestList> mapds;
  xmlNodePtr rangelistNode = GetNode( "SubrangeList", reqNode );
  ParseRangeList( rangelistNode, rangeList, mapds, filter.region );
/*  if ( !ParseRangeList( rangelistNode, rangeList, mapds, filter.region ) ) {
  	NewTextChild( dataNode, "ambiguous_time" );
  	return;
  }*/
  VerifyRangeList( rangeList, mapds );
  vector<TPeriod> nperiods, speriods;

  string log;
  string sql;
  TQuery SQry( &OraSession );
  xmlNodePtr node = GetNode( "trip_id", reqNode );
  int trip_id;
  int num=0;
  if ( node ) {
    trip_id = NodeAsInteger( node );
    SQry.Clear();
    SQry.SQLText = "SELECT first_day, last_day, days, pr_del, tlg, reference "
                   " FROM sched_days "
                   " WHERE trip_id=:trip_id";
    SQry.CreateVariable( "trip_id", otInteger, trip_id );
    SQry.Execute();
    while ( !SQry.Eof ) {
    	TPeriod p;
    	p.first = SQry.FieldAsDateTime( "first_day" );
    	p.last = SQry.FieldAsDateTime( "last_day" );
    	p.days = SQry.FieldAsString( "days" );
    	p.pr_del = SQry.FieldAsInteger( "pr_del" );
    	p.tlg = SQry.FieldAsString( "tlg" );
    	p.ref = SQry.FieldAsString( "reference" );
    	oldperiods.push_back( p );
    	SQry.Next();
    }
    TDateTime begin_date_season = BoostToDateTime( filter.periods.begin()->period.begin() );
    // теперь можно удалить все периоды, кот.
    //!!! ошибка т.к. периоды заводятся относительно региона первого п.п. у кот. delta=0
    ProgTrace( TRACE5, "delete all periods from database" );
    SQry.Clear();
    SQry.SQLText =
    "BEGIN "
    "DELETE routes WHERE move_id IN "
    "(SELECT move_id FROM sched_days WHERE trip_id=:trip_id AND last_day>=:begin_date_season ); "
    "DELETE sched_days WHERE trip_id=:trip_id AND last_day>=:begin_date_season; END; ";
    SQry.CreateVariable( "trip_id", otInteger, trip_id );
    SQry.CreateVariable( "begin_date_season", otDate, begin_date_season );
    SQry.Execute();
    SQry.Clear();
    SQry.SQLText = "SELECT MAX(num) num FROM sched_days WHERE trip_id=:trip_id";
    SQry.CreateVariable( "trip_id", otInteger, trip_id );
    SQry.Execute();
    if ( SQry.Eof )
    	num = 0;
    else
      num = SQry.FieldAsInteger( "num" ) + 1;
  }
  else {
    // это новый рейс
    ProgTrace( TRACE5, "it is new trip" );
    SQry.SQLText = "SELECT routes_trip_id.nextval AS trip_id FROM dual";
    SQry.Execute();
    trip_id = SQry.FieldAsInteger(0);
    ProgTrace( TRACE5, "new trip_id=%d", trip_id );
    TReqInfo::Instance()->MsgToLog( "Ввод нового рейса ", evtSeason, trip_id );
  }
  // все диапазоны уже в UTC
  // пробегаем по всем полученным с клиента и накладываем их на все из БД

  /* вначале добавляем неизмененные */
  for ( vector<TPeriod>::iterator ip=rangeList.periods.begin(); ip!=rangeList.periods.end(); ip++ ) {
    if ( ip->modify == fnochange )
      speriods.push_back( *ip );
  }

  for ( vector<TPeriod>::iterator ip=rangeList.periods.begin(); ip!=rangeList.periods.end(); ip++ ) {
    nperiods.clear();
    if ( ip->modify != fnochange ) {
    	ProgTrace( TRACE5, "before InsertSectsPeriods ip->move_id=%d", ip->move_id );
      filter.InsertSectsPeriods( mapds, speriods, nperiods, *ip );
      for ( vector<TPeriod>::iterator yp=nperiods.begin(); yp!=nperiods.end(); yp++ ) {
        if ( yp->modify == fdelete )
          continue;
        speriods.push_back( *yp );
      }
    }
  }

 /* for ( vector<TPeriod>::iterator yp=speriods.begin(); yp!=speriods.end(); yp++ ) {
    if ( yp->modify == fdelete )
      continue;
    nperiods.push_back( *yp );
  }*/
  ProgTrace( TRACE5, "Result of Insersect" );
  for ( vector<TPeriod>::iterator yp=speriods.begin(); yp!=speriods.end(); yp++ ) {
    if ( yp->modify == fdelete )
      continue;

   ProgTrace( TRACE5, "result first=%s, last=%s, days=%s, move_id=%d, modified=%d",
              DateTimeToStr( yp->first,"dd.mm.yy hh:nn:ss" ).c_str(),
              DateTimeToStr( yp->last,"dd.mm.yy hh:nn:ss" ).c_str(),
              yp->days.c_str(),
              yp->move_id,
              yp->modify );
  }

  // теперь внимание среди периодов есть, те которые удалены
  TQuery GQry( &OraSession );
  GQry.Clear();
  GQry.SQLText =
  "DECLARE i NUMBER;"
  "BEGIN "
  "SELECT COUNT(*) INTO i FROM seasons "
  " WHERE tz=:tz AND :first=first AND :last=last; "
  "IF i = 0 THEN "
  " INSERT INTO seasons(tz,first,last,hours) VALUES(:tz,:first,:last,:hours); "
  "END IF; "
  "END;";
  GQry.CreateVariable( "tz", otInteger, filter.tz );
  GQry.DeclareVariable( "first", otDate );
  GQry.DeclareVariable( "last", otDate );
  GQry.DeclareVariable( "hours", otInteger );
  TQuery NQry( &OraSession );
  NQry.SQLText = "SELECT routes_move_id.nextval AS move_id FROM dual";
  TQuery RQry( &OraSession );
  SQry.Clear();
  SQry.SQLText =
  "INSERT INTO sched_days(trip_id,move_id,num,first_day,last_day,days,pr_del,tlg,reference,tz) "
  "VALUES(:trip_id,:move_id,:num,:first_day,:last_day,:days,:pr_del,:tlg,:reference,:tz) ";
  SQry.DeclareVariable( "trip_id", otInteger );
  SQry.DeclareVariable( "move_id", otInteger );
  SQry.DeclareVariable( "num", otInteger );
  SQry.DeclareVariable( "first_day", otDate );
  SQry.DeclareVariable( "last_day", otDate );
  SQry.DeclareVariable( "days", otString );
  SQry.DeclareVariable( "pr_del", otInteger );
  SQry.DeclareVariable( "tlg", otString );
  SQry.DeclareVariable( "reference", otString );
  SQry.CreateVariable( "tz", otInteger, filter.tz );
  RQry.Clear();
  RQry.SQLText =
  "INSERT INTO routes(move_id,num,airp,airp_fmt,pr_del,scd_in,airline,airline_fmt,flt_no,craft,craft_fmt,scd_out,litera, "
  "                   trip_type,f,c,y,unitrip,delta_in,delta_out,suffix,suffix_fmt) "
  " VALUES(:move_id,:num,:airp,:airp_fmt,:pr_del,:scd_in,:airline,:airline_fmt,:flt_no,:craft,:craft_fmt,:scd_out,:litera, "
  "        :trip_type,:f,:c,:y,:unitrip,:delta_in,:delta_out,:suffix,:suffix_fmt) ";
  RQry.DeclareVariable( "move_id", otInteger );
  RQry.DeclareVariable( "num", otInteger );
  RQry.DeclareVariable( "airp", otString );
  RQry.DeclareVariable( "airp_fmt", otInteger );
  RQry.DeclareVariable( "pr_del", otInteger );
  RQry.DeclareVariable( "scd_in", otDate );
  RQry.DeclareVariable( "airline", otString );
  RQry.DeclareVariable( "airline_fmt", otInteger );
  RQry.DeclareVariable( "flt_no", otInteger );
  RQry.DeclareVariable( "craft", otString );
  RQry.DeclareVariable( "craft_fmt", otInteger );
  RQry.DeclareVariable( "scd_out", otDate );
  RQry.DeclareVariable( "litera", otString );
  RQry.DeclareVariable( "trip_type", otString );
  RQry.DeclareVariable( "f", otInteger );
  RQry.DeclareVariable( "c", otInteger );
  RQry.DeclareVariable( "y", otInteger );
  RQry.DeclareVariable( "unitrip", otString );
  RQry.DeclareVariable( "delta_in", otInteger );
  RQry.DeclareVariable( "delta_out", otInteger );
  RQry.DeclareVariable( "suffix", otString );
  RQry.DeclareVariable( "suffix_fmt", otInteger );
  //int num = 0;
  int new_move_id;
  for ( vector<TPeriod>::iterator ip=speriods.begin(); ip!=speriods.end(); ip++ ) {
    ProgTrace( TRACE5, "ip->modify=%d, ip->move_id=%d", ip->modify, ip->move_id );
    if ( ip->modify == fdelete ) {
      continue;
    }
    if ( ip->modify == finsert ) {
      NQry.Execute();
      new_move_id = NQry.FieldAsInteger( 0 );
      for ( vector<TPeriod>::iterator yp=ip+1; yp!=speriods.end(); yp++ ) {
        ProgTrace( TRACE5, "yp->move_id=%d, yp->modify=%d, ip->move_id=%d, ip->modify=%d",
                   yp->move_id, yp->modify, ip->move_id, ip->modify );
        if ( yp->move_id == ip->move_id ) {
          yp->move_id = new_move_id;
          yp->modify = fchange;
        }
      }

    } // end finsert
    else { //????
      new_move_id = ip->move_id;
      ProgTrace( TRACE5, "ip move_id=%d", new_move_id );
    }
    // проверяем есть ли все правила вывода (перехода) времен в маршруте в записываемом периоде
   // необходимо добавить правила перехода
   // смотрим в каком расписании лежит текущий период.
   time_period r( DateTimeToBoost( ip->first ), DateTimeToBoost( ip->last ) );
   // пробег по расписаниям
   bool inSeason = false;
   int hours;
   for ( vector<TSeason>::iterator p=filter.periods.begin(); p!=filter.periods.end(); p++ ) {
     if ( !p->period.intersects( r ) ) // не пересекаются
       continue;
     if ( p->summer )
       hours = filter.dst_offset;
     else
       hours = 0;
     GQry.SetVariable( "first", BoostToDateTime( p->period.begin() ) );
     GQry.SetVariable( "last", BoostToDateTime( p->period.end() ) );
     GQry.SetVariable( "hours", hours );
     GQry.Execute();
     inSeason = true;
   }
   if ( !inSeason )
     throw AstraLocale::UserException( "MSG.DERIVED_FLIGHT_PERIOD_NOT_IN_SCHED",
             LParams()
             << LParam("first", DateTimeToStr( ip->first, "dd.mm.yy" ))
             << LParam("last", DateTimeToStr( ip->last, "dd.mm.yy" ))
             << LParam("days", ip->days)
             );
    ProgTrace( TRACE5, "trip_id=%d, new_move_id=%d,num=%d", trip_id, new_move_id,num );
    SQry.SetVariable( "trip_id", trip_id );
    SQry.SetVariable( "move_id", new_move_id );
    SQry.SetVariable( "num", num );
//    SQry.SetVariable( "first_dest", ip->first_dest );
    SQry.SetVariable( "first_day", ip->first );
    SQry.SetVariable( "last_day", ip->last );
    SQry.SetVariable( "days", ip->days );
    SQry.SetVariable( "pr_del", ip->pr_del );
    SQry.SetVariable( "tlg", ip->tlg );
    SQry.SetVariable( "reference", ip->ref );
    vector<TPeriod>::iterator ew = oldperiods.end();
    for ( ew=oldperiods.begin(); ew!=oldperiods.end(); ew++ ) {
    	if ( ew->first == ip->first && ew->last == ip->last )
    		break;
    }
    if ( ew == oldperiods.end() ) {
      log = "Ввод нового периода " +
            DateTimeToStr( ip->first, "dd.mm.yy" ) +
            "-" +
            DateTimeToStr( ip->last, "dd.mm.yy" ) + " " +
            ip->days;
      if ( ip->pr_del )
    	  log += " отм.";
    }
    else {
    	log.clear();
    	if ( ip->days != ew->days )
    		log += " дни " + ip->days;
      if ( ew->pr_del != ip->pr_del )
      	log += " отм.";
    	if ( ip->tlg != ew->tlg )
    		log += " кратко " + ip->tlg;
    	if ( ip->ref != ew->ref )
    		log += " источники " + ip->ref;
    	if ( !log.empty() )
    	  log = "Изменение периода " +
    	        DateTimeToStr( ip->first, "dd.mm.yy" ) +
    	        "-" +
              DateTimeToStr( ip->last, "dd.mm.yy" ) + log;
    	ProgTrace( TRACE5, "log=%s, move_id=%d", log.c_str(), ew->move_id );
    	ew->modify = fdelete;
    }
    if ( !log.empty() )
      TReqInfo::Instance()->MsgToLog( log, evtSeason, trip_id, new_move_id );
    SQry.Execute()  ;
    num++;
    TDestList ds = mapds[ ip->move_id ];
    int dnum = 0;
    double fl, ff;
    if ( !ds.dests.empty() ) {
    	log.clear();
      for ( TDests::iterator id=ds.dests.begin(); id!=ds.dests.end(); id++ ) {
        RQry.SetVariable( "move_id", new_move_id );
        RQry.SetVariable( "num", dnum );
        RQry.SetVariable( "airp", id->airp );
        RQry.SetVariable( "airp_fmt", (int)id->airp_fmt );
        if ( !log.empty() )
        	log += "-";
        else
        	log = "Маршрут: ";
        RQry.SetVariable( "pr_del", id->pr_del );
        if ( id->scd_in > NoExists ) {
          RQry.SetVariable( "scd_in", modf( (double)id->scd_in, &fl ) ); // удаляем delta_in
          log += DateTimeToStr( id->scd_in, "hh:nn(UTC)" );

        }
        else {
          fl = 0.0;
          RQry.SetVariable( "scd_in", FNull );
        }
        log += id->airp;
        if ( id->airline.empty() ) {
          RQry.SetVariable( "airline", FNull );
          RQry.SetVariable( "airline_fmt", FNull );
        }
        else {
          RQry.SetVariable( "airline", id->airline );
          RQry.SetVariable( "airline_fmt", (int)id->airline_fmt );
        }
        if ( id->trip > NoExists )
          RQry.SetVariable( "flt_no", id->trip );
        else
          RQry.SetVariable( "flt_no", FNull );
        if ( id->craft.empty() ) {
          RQry.SetVariable( "craft", FNull );
          RQry.SetVariable( "craft_fmt", FNull );
        }
        else {
          RQry.SetVariable( "craft", id->craft );
          RQry.SetVariable( "craft_fmt", (int)id->craft_fmt );
        }
        if ( id->scd_out > NoExists ) {
          RQry.SetVariable( "scd_out", modf( (double)id->scd_out, &ff ) ); // удаляем delta_out
          log += DateTimeToStr( id->scd_out, "hh:nn(UTC)" );
        }
        else {
          ff = 0.0;
          RQry.SetVariable( "scd_out", FNull );
        }
        if ( id->pr_del )
        	log += " отм.";
        if ( id->triptype.empty() )
          RQry.SetVariable( "trip_type", FNull );
        else
          RQry.SetVariable( "trip_type", id->triptype );
        RQry.SetVariable( "f", id->f );
        RQry.SetVariable( "c", id->c );
        RQry.SetVariable( "y", id->y );
        if ( id->litera.empty() )
          RQry.SetVariable( "litera", FNull );
        else
          RQry.SetVariable( "litera", id->litera );
        if ( id->unitrip.empty() )
          RQry.SetVariable( "unitrip", FNull );
        else
          RQry.SetVariable( "unitrip", id->unitrip );
        if ( id->scd_in > NoExists )
          RQry.SetVariable( "delta_in", fl );
        else
          RQry.SetVariable( "delta_in", FNull ); //???
        if ( id->scd_out > NoExists )
          RQry.SetVariable( "delta_out", ff );
        else
          RQry.SetVariable( "delta_out", FNull ); //???
        if ( id->suffix.empty() ) {
          RQry.SetVariable( "suffix", FNull );
          RQry.SetVariable( "suffix_fmt", FNull );
        }
        else {
          RQry.SetVariable( "suffix", id->suffix );
          RQry.SetVariable( "suffix_fmt", (int)id->suffix_fmt );
        }
        RQry.Execute();
        dnum++;
      }
      mapds[ ip->move_id ].dests.clear();
      ip->move_id = new_move_id;
      TReqInfo::Instance()->MsgToLog( log, evtSeason, trip_id, new_move_id );
    }
  }
  for ( vector<TPeriod>::const_iterator ew=oldperiods.begin(); ew!=oldperiods.end(); ew++ ) {
  	if ( ew->modify == fdelete )
  		continue;
      log = "Удаление периода " +
            DateTimeToStr( ew->first, "dd.mm.yy" ) +
            "-" +
            DateTimeToStr( ew->last, "dd.mm.yy" ) + " " +
            ew->days;
      if ( ew->pr_del )
    	  log += " отм.";
    	TReqInfo::Instance()->MsgToLog( log, evtSeason, trip_id, new_move_id );
  }

  // надо перечитать информацию по экрану редактирования
  string err_city;
  GetEditData( trip_id, filter, true, dataNode, err_city );
  AstraLocale::showMessage( "MSG.DATA_SAVED" );
}

string GetTrip( TDest *PriorDest, TDest *OwnDest )
{
  string res;
  int flt_no;
  string suffix;
  string airline;
  TElemFmt airline_fmt, suffix_fmt;
  if ( !PriorDest ) {
    flt_no = NoExists;
    airline_fmt = efmtCodeNative;
    suffix_fmt = efmtCodeNative;
  }
  else {
    flt_no = PriorDest->trip;
    airline = PriorDest->airline;
    suffix = PriorDest->suffix;
    airline_fmt = PriorDest->airline_fmt;
    suffix_fmt = PriorDest->suffix_fmt;
  }
  if ( OwnDest->trip == NoExists ) /* рейс на прилет */
    res = ElemIdToElemCtxt( ecDisp, etAirline, airline, airline_fmt ) +
          IntToString( flt_no ) +
          ElemIdToElemCtxt( ecDisp, etSuffix, suffix, suffix_fmt );
  else
    if ( flt_no == NoExists ||
         airline == OwnDest->airline && flt_no == OwnDest->trip && suffix == OwnDest->suffix )
      res = ElemIdToElemCtxt( ecDisp, etAirline, OwnDest->airline, OwnDest->airline_fmt ) +
            IntToString( OwnDest->trip ) +
            ElemIdToElemCtxt( ecDisp, etSuffix, OwnDest->suffix, OwnDest->suffix_fmt );
    else
      if ( airline != OwnDest->airline )
        res = ElemIdToElemCtxt( ecDisp, etAirline, airline, airline_fmt ) +
              IntToString( flt_no ) +
              ElemIdToElemCtxt( ecDisp, etSuffix, suffix, suffix_fmt ) +
              "/" +
              ElemIdToElemCtxt( ecDisp, etAirline, OwnDest->airline, OwnDest->airline_fmt ) +
              IntToString( OwnDest->trip ) +
              ElemIdToElemCtxt( ecDisp, etSuffix, OwnDest->suffix, OwnDest->suffix_fmt );
      else
        res = ElemIdToElemCtxt( ecDisp, etAirline, airline, airline_fmt ) +
              IntToString( flt_no ) +
              ElemIdToElemCtxt( ecDisp, etSuffix, suffix, suffix_fmt ) +
              "/" +
              IntToString( OwnDest->trip ) +
              ElemIdToElemCtxt( ecDisp, etSuffix, OwnDest->suffix, OwnDest->suffix_fmt );
  return res;
}

string GetTextTime( TDateTime Fact, TDateTime VDate )
{
  string res;
  char l;
  int Hour, Min, Sec;
  DecodeTime( Fact, Hour, Min, Sec );
  if ( Fact > NoExists ) {
    res = IntToString( Hour );
    switch ( (int)Fact - (int)VDate ) {
      case -1: l = '-';
               break;
      case -2: l = '=';
               break;
      case 0: l = ' ';
              break;
      case 1: l = '+';
              break;
      case 2: l = '*';
              break;
      default: l = '#';
    }
    res = DateTimeToStr( Fact, string( "hh" ) + l + "nn" );
  }
  return res;
}

TDateTime TDateTimeToClient( TDateTime flight_time, TDateTime dest_time, const string &dest_region )
{
  double f1, f2, f3;
  modf( (double)flight_time, &f1 );
  if ( dest_time > NoExists ) {
    f3 = modf( (double)dest_time, &f2 );
    f2 = modf( (double)UTCToClient( f1 + fabs( f3 ), dest_region ), &f3 );
    if ( f3 < f1 )
      return f3 - f1 - f2;
    else
      return f3 - f1 + f2;
  }
  else
    return NoExists;
}

/* UTCTIME */
bool createAirportTrip( string airp, int trip_id, TFilter filter, int offset, TDestList &ds,
                        TDateTime utc_spp_date, bool viewOwnPort, bool UTCFilter, string &err_city )
{
  if ( ds.dests.empty() )
    return false;
  bool createTrip = false;
  TDest *OwnDest = NULL;
  TDest *PriorDest = NULL;
  TDest *PDest = NULL;
  TDest *NDest;
  string crafts, craft_format;
  vector<TDest> vecportsFrom, vecportsTo;
  int i=0;
//  ProgTrace( TRACE5, "createAirporttrip trip_id=%d, trips.size()=%d", trip_id, (int)ds.trips.size() );
  do {
    NDest = &ds.dests[ i ];
    craft_format = ElemIdToElemCtxt( ecDisp, etCraft, NDest->craft, NDest->craft_fmt );
    if ( crafts.find( craft_format ) == string::npos ) {
      if ( !crafts.empty() )
        crafts += "/";
      crafts += craft_format;
    }
    try {
      if ( OwnDest == NULL && NDest->airp == airp ) {
        PriorDest = PDest;
        OwnDest = NDest;
        if ( viewOwnPort ) {
        	TDest d = *NDest;
        	d.scd_in = TDateTimeToClient( ds.flight_time, d.scd_in, d.region );
        	d.scd_out = TDateTimeToClient( ds.flight_time, d.scd_out, d.region );
          vecportsFrom.push_back( d );
        }
      }
      else { /* наш порт в маршруте не надо отображать */
          if ( !OwnDest ) {
          	TDest d = *NDest;
        	  d.scd_in = TDateTimeToClient( ds.flight_time, d.scd_in, d.region );
        	  d.scd_out = TDateTimeToClient( ds.flight_time, d.scd_out, d.region );
            vecportsFrom.push_back( d );
          }
          else {
          	TDest d = *NDest;
          	d.scd_in = TDateTimeToClient( ds.flight_time, d.scd_in, d.region );
          	d.scd_out = TDateTimeToClient( ds.flight_time, d.scd_out, d.region );
            vecportsTo.push_back( d );
          }
        createTrip = ( OwnDest && ( PDest->trip != NDest->trip || PDest->airline != NDest->airline ) );
      }
    }
    catch( Exception &e ) {
    	if ( err_city.empty() )
    		err_city = NDest->city;
    	return false;
    }
    /* может получится несколько рейсов. */
    if ( createTrip || OwnDest != NULL && NDest == &ds.dests.back() ) {
     /* если мы здесь, то получается рейс,
       у него есть время прилета/вылета и оно может не удовлетворять условиям фильтра
       Когда это представитель одного порта => можно понять времена вылета и прилета в порт */
       // error view filter time normilize
       bool cantrip = false;
       if ( utc_spp_date > NoExists ) {
         double f2;
         if ( OwnDest->scd_in > NoExists ) {
           modf( (double)OwnDest->scd_in, &f2 );
           ProgTrace( TRACE5, "scd_in f2=%f, utc_spp_date=%f", f2, utc_spp_date );
           if ( f2 == utc_spp_date )
             cantrip = true;
         }
         if ( !cantrip && OwnDest->scd_out > NoExists ) {
           modf( (double)OwnDest->scd_out, &f2 );
           ProgTrace( TRACE5, "scd_out f2=%f, utc_spp_date=%f", f2, utc_spp_date );
           if ( f2 == utc_spp_date )
             cantrip = true;
         }
       }
       else
         cantrip = true;

      if ( cantrip &&
      	   ( UTCFilter &&
      	      ( filter.isFilteredUTCTime( utc_spp_date, ds.flight_time, OwnDest->scd_in, offset ) ||
      	        filter.isFilteredUTCTime( utc_spp_date, ds.flight_time, OwnDest->scd_out, offset ) ) ||
      	     !UTCFilter && filter.isFilteredTime( ds.flight_time, OwnDest->scd_in, OwnDest->scd_out, offset, OwnDest->region ) )
      	 ) {
        /* рейс подходит под наши условия */
        ProgTrace( TRACE5, "createAirporttrip trip_id=%d, OwnDest->scd_in=%s, OwnDest.scd_out=%s",
                   trip_id,
                   DateTimeToStr( OwnDest->scd_in, "dd.mm.yy hh:nn" ).c_str(),
                   DateTimeToStr( OwnDest->scd_out, "dd.mm.yy hh:nn" ).c_str() );
        trip tr;
        tr.trip_id = trip_id;
        tr.name = GetTrip( PriorDest, OwnDest ); // local format
        tr.print_name = GetPrintName( PriorDest, OwnDest ); // local format
        tr.ownport = ElemIdToElemCtxt( ecDisp, etAirp, OwnDest->airp, OwnDest->airp_fmt ); // local format
        tr.airlineId = OwnDest->airline;
        tr.airpId = OwnDest->airp;
        tr.crafts = crafts; // local format
        tr.vecportsFrom = vecportsFrom;
        tr.vecportsTo = vecportsTo;

        if ( OwnDest == NDest ) {
          tr.owncraft = ElemIdToElemCtxt( ecDisp, etCraft, PriorDest->craft, PriorDest->craft_fmt ); // local format
          tr.triptype = ElemIdToCodeNative(etTripType,PriorDest->triptype);
          tr.craftId = PriorDest->craft;
          tr.triptypeId = PriorDest->triptype;
        }
        else {
          tr.owncraft = ElemIdToElemCtxt( ecDisp, etCraft, OwnDest->craft, OwnDest->craft_fmt );
          tr.triptype = ElemIdToCodeNative(etTripType,OwnDest->triptype);
          tr.craftId = OwnDest->craft;
          tr.triptypeId = OwnDest->triptype;
        }
        tr.pr_del = OwnDest->pr_del; //!!! неправильно так, надо расчитывать
        /* переводим времена вылета прилета в локальные */ //!!! error tz
        tr.scd_in = TDateTimeToClient( ds.flight_time, OwnDest->scd_in, OwnDest->region );
        tr.scd_out = TDateTimeToClient( ds.flight_time, OwnDest->scd_out, OwnDest->region );

        ds.trips.push_back( tr );
      }
      createTrip = false;
      PriorDest = NULL;
      OwnDest = NULL;
      vecportsFrom.clear();
      vecportsTo.clear();
    }
    else
      i++;
    PDest = NDest;
  }
  while ( NDest != &ds.dests.back() );
  ProgTrace( TRACE5, "trips.size()=%d", (int)ds.trips.size() );
  return !ds.trips.empty();
}


/* UTCTIME */
bool createAirportTrip( int trip_id, TFilter filter, int offset, TDestList &ds, bool viewOwnPort, string &err_city )
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  bool res = false;
  for ( vector<string>::iterator s=reqInfo->user.access.airps.begin();
        s!=reqInfo->user.access.airps.end(); s++ ) {
    res = res || createAirportTrip( *s, trip_id, filter, offset, ds, NoExists, viewOwnPort, false, err_city );
  }
  return res;
}

/* UTCTIME */
bool createAirlineTrip( int trip_id, TFilter &filter, int offset, TDestList &ds, string &err_city )
{
  return createAirlineTrip( trip_id, filter, offset, ds, NoExists, err_city );
}

/* UTCTIME to client  */
bool createAirlineTrip( int trip_id, TFilter &filter, int offset, TDestList &ds, TDateTime localdate, string &err_city )
{
  if ( ds.dests.empty() )
    return false;
  TDest *PDest = NULL;
  TDest *NDest;
  int i = 0;
  trip tr;
  tr.trip_id = trip_id;
  tr.scd_in = NoExists;
  tr.scd_out = NoExists;
  bool timeKey = filter.firstTime == NoExists;
  bool pr_del = true;
  int own_date = 0;
  string ptime;
  string::size_type p = 0, bold_begin = 0, bold_end = 0;
  string str_dests;
  string craft_format;
  string str_trip_type;

  do {
    NDest = &ds.dests[ i ];
    if ( !NDest->pr_del )
      pr_del = false;
    timeKey = timeKey || filter.isFilteredTime( ds.flight_time, NDest->scd_in, NDest->scd_out, offset, NDest->region );
    craft_format = ElemIdToElemCtxt( ecDisp, etCraft, NDest->craft, NDest->craft_fmt );
    if ( tr.crafts.find( craft_format ) == string::npos ) {
      if ( !tr.crafts.empty() )
        tr.crafts += "/";
      tr.crafts += craft_format;
    }

    str_trip_type = ElemIdToCodeNative(etTripType,NDest->triptype);
    if ( tr.triptype.find( str_trip_type ) == string::npos ) {
      if ( !tr.triptype.empty() )
        tr.triptype += "/";
      tr.triptype += str_trip_type;
    }
      if ( !tr.portsForAirline.empty() ) {
         tr.portsForAirline += "/";
         str_dests += "/";
      }

      double first_day, f2, f3, utc_date_scd_in, utc_date_scd_out;
      modf( (double)ds.flight_time, &first_day );

      if ( localdate > NoExists && NDest->scd_in > NoExists ) {
          ProgTrace( TRACE5, "airp=%s,localdate=%s, utcscd_in=%s, first_day=%s",
                     NDest->airp.c_str(),
                     DateTimeToStr( localdate, "dd.mm.yy hh:nn" ).c_str(),
                     DateTimeToStr( NDest->scd_in, "dd.mm.yy hh:nn" ).c_str(),
                     DateTimeToStr( first_day, "dd.mm.yy hh:nn" ).c_str() );

        double f2;
        TDateTime scd_in;

        f3 = modf( (double)NDest->scd_in, &utc_date_scd_in );
        try {
          f2 = modf( (double)UTCToClient( first_day + fabs( f3 ), NDest->region ), &f3 );
        }
        catch( Exception &e ) {
        	if ( err_city.empty() )
    		    err_city = NDest->city;
    	    return false;
        }
        // получаем время
        if ( f3 < first_day )
          scd_in = f3 - first_day - f2;
        else
          scd_in = f3 - first_day + f2;
        ProgTrace( TRACE5, "localdate=%s, utc_date_scd_in=%s, f3=%f, first_day=%f",
                   DateTimeToStr( localdate, "dd hh:nn" ).c_str(),
                   DateTimeToStr( utc_date_scd_in, "dd hh:nn" ).c_str(),
                   f3, first_day );

        if ( utc_date_scd_in + f3 - first_day == localdate ) {
          ProgTrace( TRACE5, "localdate=%s, localscd_in=%s",
                     DateTimeToStr( localdate, "dd hh:nn" ).c_str(),
                     DateTimeToStr( scd_in, "dd hh:nn" ).c_str() );
          ptime = DateTimeToStr( scd_in, "(hh:nn)" );
          if ( own_date == 0 ) {
            bold_begin = tr.portsForAirline.size();
            tr.portsForAirline += ptime;
            own_date = 1;
          }
          else
            p = tr.portsForAirline.size();
          bold_end = tr.portsForAirline.size();
        }
        else
          if ( own_date == 1 ) {
            if ( p > 0 ) {
              tr.portsForAirline.insert( p, ptime );
              p += ptime.size();
              bold_end = p;
            }
            own_date = 2;
          }
      }
      tr.portsForAirline += ElemIdToElemCtxt( ecDisp, etAirp, NDest->airp, NDest->airp_fmt );
      tr.vecportsTo.push_back( *NDest ); // нужно для вывода на 1 экран в сезонке
      str_dests += ElemIdToElemCtxt( ecDisp, etAirp, NDest->airp, NDest->airp_fmt );
      if ( localdate > NoExists && NDest->scd_out > NoExists ) {
          ProgTrace( TRACE5, "airp=%s,localdate=%s, utcscd_out=%s",
                     NDest->airp.c_str(),DateTimeToStr( localdate, "dd hh:nn" ).c_str(),
                     DateTimeToStr( NDest->scd_out, "dd hh:nn" ).c_str() );
        TDateTime scd_out;
        f3 = modf( (double)NDest->scd_out, &utc_date_scd_out );
        try {
          f2 = modf( (double)UTCToClient( first_day + fabs( f3 ), NDest->region ), &f3 );
        }
        catch( Exception &e ) {
        	if ( err_city.empty() )
    		    err_city = NDest->city;
    	    return false;
        }
        if ( f3 < first_day )
          scd_out = f3 - first_day - f2;
        else
          scd_out = f3 - first_day + f2;
        if ( utc_date_scd_out + f3 - first_day == localdate ) {
          ProgTrace( TRACE5, "localdate=%s, localscd_out=%s",
                     DateTimeToStr( localdate, "dd hh:nn" ).c_str(),
                     DateTimeToStr( scd_out, "dd hh:nn" ).c_str() );

          ptime = DateTimeToStr( scd_out, "(hh:nn)" );
          if ( own_date == 0 ) {
            if ( i )
              bold_begin = tr.portsForAirline.size();
            tr.portsForAirline += ptime;
            own_date = 1;
          }
          else
            p = tr.portsForAirline.size();
          bold_end = tr.portsForAirline.size();
        }
        else
          if ( own_date == 1 ) {
            if ( p > 0 ) {
              tr.portsForAirline.insert( p, ptime );
              p += ptime.size();
              bold_end = p;
            }
            own_date = 2;
          }
      }


//      ProgTrace( TRACE5, "tr.ports_out=%s, bold ports=%s", tr.ports_out.c_str(), tr.bold_ports.c_str() );
   // }
    if ( NDest->trip > NoExists ) {
      if  ( !PDest || PDest->airline != NDest->airline && !NDest->airline.empty() ||
            PDest->trip != NDest->trip ||
            PDest->suffix != NDest->suffix ) {
        if ( !PDest )
         tr.name = ElemIdToElemCtxt( ecDisp, etAirline, NDest->airline, NDest->airline_fmt ) +
                   IntToString( NDest->trip ) +
                   ElemIdToElemCtxt( ecDisp, etSuffix, NDest->suffix, NDest->suffix_fmt );
        else {
          if ( PDest->airline != NDest->airline )
            tr.name += string( "/" ) +
                       ElemIdToElemCtxt( ecDisp, etAirline, NDest->airline, NDest->airline_fmt ) +
                       IntToString( NDest->trip ) +
                       ElemIdToElemCtxt( ecDisp, etSuffix, NDest->suffix, NDest->suffix_fmt );
          else
            tr.name += string( "/" ) + IntToString( NDest->trip ) +
                       ElemIdToElemCtxt( ecDisp, etSuffix, NDest->suffix, NDest->suffix_fmt );
        }
      }
    }
    else
      if ( NDest == &ds.dests.back() && tr.name.empty() ) {
        tr.name = ElemIdToElemCtxt(ecDisp, etAirline, PDest->airline, PDest->airline_fmt ) +
                  IntToString( PDest->trip ) +
                  ElemIdToElemCtxt( ecDisp, etSuffix, PDest->suffix, PDest->suffix_fmt );
      }
    tr.print_name = GetPrintName( PDest, NDest );
    i++;
    PDest = NDest;
  }
  while ( NDest != &ds.dests.back() );

  if ( !bold_end || own_date == 1 ) {
  	if ( !bold_begin )
      tr.portsForAirline = str_dests;
    bold_end = tr.portsForAirline.size();
  }

  tr.bold_ports.assign( tr.portsForAirline, bold_begin, bold_end - bold_begin );


  if ( timeKey ) {
    tr.pr_del = pr_del;
    ds.trips.push_back( tr );
  }
//ProgTrace( TRACE5, "ds.trips.size()=%d", (int)trips.size() );
  return !ds.trips.empty();
}

void AddRefPeriod( string &exec, TDateTime first, TDateTime last, int delta_out,
                   string days, string tlg )
{
  if ( !exec.empty() )
  exec += "; ";

  if ( !days.empty() && days != AllDays ) {
   exec += GetWOPointDays( days );
   if ( delta_out )
     exec += GetNextDays( days, delta_out );
   exec += " ";
  }
  if ( first != last )
    exec += DateTimeToStr( first, "dd.mm" ) + "-" + DateTimeToStr( last, "dd.mm" );
  else
    exec += DateTimeToStr( first, "dd.mm" );
  if ( !tlg.empty() )
    exec += " " + TrimString( tlg );
}

bool ConvertPeriodToLocal( TDateTime &first, TDateTime &last, string &days, string region, int ptz, int &errtz )
{
  TDateTime f;
  TDateTime l;

  try {
    f = UTCToClient( first, region );
    l = UTCToClient( last, region );
  }
  catch( Exception &e ) {
  	if ( errtz == NoExists )
  		errtz = ptz;
  	return false;
  }
  int m = (int)f - (int)first;
  first = f;
  last = l;
  /* нет сдвига или указаны все дни */
  if ( !m || days == AllDays )
   return true;
  /* сдвиг даты произошел и у нас не все дни выполнения */
  days = AddDays( days, m );
//  ProgTrace( TRACE5, "ConvertPeriodToLocal have move range" );
  return true;
}

void GetDests( map<int,TDestList> &mapds, const TFilter &filter, int vmove_id )
{
//  ProgTrace( TRACE5, "GetDests vmove_id=%d", vmove_id );
  TPerfTimer tm;
  tm.Init();
  TReqInfo *reqInfo = TReqInfo::Instance();
  TQuery RQry( &OraSession );
  string sql =
  "SELECT move_id,num,routes.airp airp,airp_fmt,airps.city city, "
  "       routes.pr_del,scd_in+delta_in scd_in,airline,airline_fmt,"
  "       flt_no,craft,craft_fmt,litera,trip_type,scd_out+delta_out scd_out,f,c,y,unitrip,suffix,suffix_fmt "
  " FROM routes, airps "
  "WHERE ";
  if ( vmove_id > NoExists ) {
    RQry.CreateVariable( "move_id", otInteger, vmove_id );
    sql += "move_id=:move_id AND ";
  }
  sql += "routes.airp=airps.code ORDER BY move_id,num";

  RQry.SQLText = sql;
  RQry.Execute();
  int idx_rmove_id = RQry.FieldIndex("move_id");
  int idx_num = RQry.FieldIndex("num");
  int idx_airp = RQry.FieldIndex("airp");
  int idx_airp_fmt = RQry.FieldIndex("airp_fmt");
  int idx_city = RQry.FieldIndex("city");
  int idx_rpr_del = RQry.FieldIndex("pr_del");
  int idx_scd_in = RQry.FieldIndex("scd_in");
  int idx_airline = RQry.FieldIndex("airline");
  int idx_airline_fmt = RQry.FieldIndex("airline_fmt");
  int idx_trip = RQry.FieldIndex("flt_no");
  int idx_craft = RQry.FieldIndex("craft");
  int idx_craft_fmt = RQry.FieldIndex("craft_fmt");
  int idx_scd_out = RQry.FieldIndex("scd_out");
  int idx_litera = RQry.FieldIndex("litera");
  int idx_triptype = RQry.FieldIndex("trip_type");
  int idx_f = RQry.FieldIndex("f");
  int idx_c = RQry.FieldIndex("c");
  int idx_y = RQry.FieldIndex("y");
  int idx_unitrip = RQry.FieldIndex("unitrip");
  int idx_suffix = RQry.FieldIndex("suffix");
  int idx_suffix_fmt = RQry.FieldIndex("suffix_fmt");

  int move_id = NoExists;
  TDestList ds;
  TDest d;
  bool canUseAirline, canUseAirp; /* можно ли использовать данный */
  bool compKey, cityKey, airpKey, triptypeKey, timeKey;
  double f1;
  while ( !RQry.Eof ) {
    if ( move_id != RQry.FieldAsInteger( idx_rmove_id ) ) {
      if ( move_id >= 0 ) {
      	if ( canUseAirline && canUseAirp &&
      	     cityKey && airpKey && compKey && triptypeKey && timeKey ) {
//            ProgTrace( TRACE5, "canuse move_id=%d", move_id );
            mapds.insert(std::make_pair( move_id, ds ) );
        }
      }
        ds.dests.clear();
//        ds.region.clear();

        compKey = filter.airline.empty();
        cityKey = filter.city.empty();
        airpKey = filter.airp.empty();
        triptypeKey = filter.triptype.empty();
        timeKey = filter.firstTime == NoExists;
//      }
      move_id = RQry.FieldAsInteger( idx_rmove_id );
      canUseAirline = false; // new
      canUseAirp = false; //new
    }
    d.num = RQry.FieldAsInteger( idx_num );
    d.airp = RQry.FieldAsString( idx_airp );
    d.airp_fmt = (TElemFmt)RQry.FieldAsInteger( idx_airp_fmt );
    airpKey = airpKey || d.airp == filter.airp;
    d.airline = RQry.FieldAsString( idx_airline );
    d.airline_fmt = (TElemFmt)RQry.FieldAsInteger( idx_airline_fmt );
    compKey = compKey  || d.airline == filter.airline;
    if ( reqInfo->CheckAirp( d.airp ) )
     	canUseAirp = true;
    if ( reqInfo->CheckAirline( d.airline ) )
 	    canUseAirline = true;
    d.city = RQry.FieldAsString( idx_city );
    cityKey = cityKey || d.city == filter.city;
    d.region = AirpTZRegion( RQry.FieldAsString( idx_airp ), false );
    d.pr_del = RQry.FieldAsInteger( idx_rpr_del );
    if ( RQry.FieldIsNULL( idx_scd_in ) )
      d.scd_in = NoExists;
    else {
      d.scd_in = RQry.FieldAsDateTime( idx_scd_in );
      modf( (double)d.scd_in, &f1 );
    }
    if ( RQry.FieldIsNULL( idx_trip ) )
      d.trip = NoExists;
    else
      d.trip = RQry.FieldAsInteger( idx_trip );
    d.craft = RQry.FieldAsString( idx_craft );
    d.craft_fmt = (TElemFmt)RQry.FieldAsInteger( idx_craft_fmt );
    d.litera = RQry.FieldAsString( idx_litera );
    d.triptype = RQry.FieldAsString( idx_triptype );
    triptypeKey = triptypeKey || d.triptype == filter.triptype;
    if ( RQry.FieldIsNULL( idx_scd_out ) )
      d.scd_out = NoExists;
    else {
      d.scd_out = RQry.FieldAsDateTime( idx_scd_out );
    }
    d.f = RQry.FieldAsInteger( idx_f );
    d.c = RQry.FieldAsInteger( idx_c );
    d.y = RQry.FieldAsInteger( idx_y );
    d.unitrip = RQry.FieldAsString( idx_unitrip );
    d.suffix = RQry.FieldAsString( idx_suffix );
    d.suffix_fmt = (TElemFmt)RQry.FieldAsInteger( idx_suffix_fmt );
    //!!!! неправильно фильтровать по времени UTC && LOCAL - зависит от настройки пульта
  /* фильтр по времени работает в случаях:
     1. Представитель порта (ед. порт) фильтр по времени прилета/вылета по этому порту
        если задан в фильре порт, то выбор рейсов у которых встречается этот порт в маршруте
     Иначе
        Фильтр по времени на прилет/вылет по порту, кот. задан в фильтре
        если в фильтре не задан порт и надо фильтровать по времени, то проверка времен по всему маршруту */
    if ( reqInfo->user.user_type == utAirport ) {
      /* Когда это представитель одного порта => можно понять времена вылета и прилета в порт
         проверка на времена будет производится непосредственно в процедуре формирования рейса,
         т.к. в маршруте может быть более одного рейса относительно нашего порта */
       timeKey = timeKey ||
                 find( reqInfo->user.access.airps.begin(),
                       reqInfo->user.access.airps.end(), d.airp ) != reqInfo->user.access.airps.end();
    }
    else {
      timeKey = true;
      // отсюда убрали фильтр по временам, т.к. надо знать начало выполнения рейса (сдвиг по времени зима/лето)
    }
    ds.dests.push_back( d );
    RQry.Next();
  }
  if ( canUseAirline && canUseAirp &&
       cityKey && airpKey && compKey && triptypeKey && timeKey ) {
//    ProgTrace( TRACE5, "canuse move_id=%d", move_id );
    mapds.insert(std::make_pair( move_id, ds ) );
  }
}

int GetTZOffSet( TDateTime first, int tz, map<int,TTimeDiff> &v )
{
  map<int,TTimeDiff>::iterator mt = v.find( tz );
  TTimeDiff vt;
  if ( mt == v.end() ) {
    TQuery Qry( &OraSession );
    Qry.SQLText = "SELECT tz,first,last, hours FROM seasons WHERE tz=:tz";
    Qry.CreateVariable( "tz", otInteger, tz );
    Qry.Execute();
    while ( !Qry.Eof ) {
      timeDiff t;
      t.first = Qry.FieldAsDateTime( "first" );
      t.last = Qry.FieldAsDateTime( "last" );
      t.hours = Qry.FieldAsInteger( "hours" );
      vt.push_back( t );
      Qry.Next();
    }
    v.insert( make_pair(tz,vt));
  }
  else vt = v[ tz ];

  for ( vector<timeDiff>::iterator i=vt.begin(); i!=vt.end(); i++ ) {
    if ( i->first <= first && i->last >= first ) {
      return i->hours;
    }
  }
  return 0;
}

TDateTime TFilter::GetTZTimeDiff( TDateTime utcnow, TDateTime first, int tz )
{
  map<int,TTimeDiff>::iterator mt = offsets.find( tz );
  TTimeDiff vt;
  if ( mt == offsets.end() ) {
    TQuery Qry( &OraSession );
    Qry.SQLText = "SELECT tz,first,last, hours FROM seasons WHERE tz=:tz";
    Qry.CreateVariable( "tz", otInteger, tz );
    Qry.Execute();
    while ( !Qry.Eof ) {
      timeDiff t;
      t.first = Qry.FieldAsDateTime( "first" );
      t.last = Qry.FieldAsDateTime( "last" );
      t.hours = Qry.FieldAsInteger( "hours" );
      vt.push_back( t );
      Qry.Next();
    }
    offsets.insert( make_pair(tz,vt));
  }
  else vt = offsets[ tz ];

  int periodDiff = NoExists, seasonDiff = NoExists;

  for ( vector<timeDiff>::iterator i=vt.begin(); i!=vt.end(); i++ ) {
/*    ProgTrace( TRACE5, "i->first=%s, i->last=%s, i->hours=%d",
               DateTimeToStr( i->first, "dd.mm.yy hh:nn" ).c_str(),
               DateTimeToStr( i->last, "dd.mm.yy hh:nn" ).c_str(),
               i->hours );*/
    if ( i->first <= first && i->last >= first ) {
      periodDiff = i->hours; // сдвиг времени выполнения рейса
/*      ProgTrace( TRACE5, "period first=%s, periofDiff=%d",
                 DateTimeToStr( first, "dd.mm.yy hh:nn" ).c_str(),
                 periodDiff );*/
    }
    /* для перевода времени необходимо, чтобы текущее время было внутри диапазона,
       тогда и отображать надо соответствеюше */
/*    if ( first <= utcnow && last >= utcnow &&
         i->first <= utcnow && i->last >= utcnow ) {
      seasonDiff = i->hours;*/
/*      ProgTrace( TRACE5, "seasonDiff=%d", seasonDiff );*/
     if ( i->first <= utcnow && i->last >= utcnow ) {
     	 seasonDiff = i->hours; // сдвиг тек. времени
     }
    if ( periodDiff > NoExists && seasonDiff > NoExists )
      break;
  }
  // если периоды совпали, то никакого сдвига времени нет или же не смогли найти правила
  if ( periodDiff == seasonDiff || periodDiff == NoExists ||  seasonDiff == NoExists )
    return 0.0;
  else {
    ProgTrace( TRACE5, "periodDiff - seasonDiff =%d", periodDiff - seasonDiff );
    return (double)( periodDiff - seasonDiff )*3600000/(double)MSecsPerDay;
  }
 /* ПРАВИЛО: ПЕРЕВОДИТ ОСУЩЕСТВЛЯЕТСЯ ОТНОСИТЕЛЬНО ПЕРВОГО ДНЯ ВЫПОЛНЕНИЯ ДИАПАЗОНА
   период ЗИМА (3) = 0 сегодня ЛЕТО(4) = 1 =>  1
   период ЛЕТО(4) = 1 сегодня ЗИМА(3) = -1 => -1
 */

//  ProgError( STDLOG, ">>>> error GetTZTimeDiff not found" );
}

bool ComparePeriod1( TViewPeriod t1, TViewPeriod t2 )
{
	double f;
    if ( !t1.trips.empty() && !t2.trips.empty() ) {
        bool result;
        if ( t1.trips.begin()->scd_out == t2.trips.begin()->scd_out ) {
        	if ( t1.trips.begin()->scd_in != t2.trips.begin()->scd_in )
        		result = modf((double)t1.trips.begin()->scd_in, &f) < modf((double)t2.trips.begin()->scd_in, &f);
        	else
            if(t1.trips.begin()->name.size() == t2.trips.begin()->name.size()) {
                if ( t1.trips.begin()->name == t2.trips.begin()->name ) {
                    result = t1.trips.begin()->move_id < t2.trips.begin()->move_id;
                } else
                    result = t1.trips.begin()->name < t2.trips.begin()->name;
            } else
                result = t1.trips.begin()->name.size() < t2.trips.begin()->name.size();
        }
        else
            result = modf((double)t1.trips.begin()->scd_out, &f) < modf((double)t2.trips.begin()->scd_out, &f);
        return result;
    }
    return false;
}

bool ComparePeriod( TViewPeriod t1, TViewPeriod t2 )
{
    if ( !t1.trips.empty() && !t2.trips.empty() ) {
            if ( t1.trips.begin()->name.size() < t2.trips.begin()->name.size() )
                return true;
            else
                if ( t1.trips.begin()->name.size() > t2.trips.begin()->name.size() )
                    return false;
                else
                    if ( t1.trips.begin()->name < t2.trips.begin()->name )
                        return true;
                    else
                        if ( t1.trips.begin()->name > t2.trips.begin()->name )
                            return false;
                        else
                            if ( t1.trips.begin()->move_id < t2.trips.begin()->move_id )
                                return true;
    }
    return false;
}

void internalRead( TFilter &filter, vector<TViewPeriod> &viewp, int trip_id = NoExists )
{
  int errtz = NoExists;
  string err_city;
  TDateTime last_date_season = BoostToDateTime( filter.periods.begin()->period.begin() );
  map<int,string> mapreg;
  map<int,TTimeDiff> v;
  map<int,TDestList> mapds;
  TQuery SQry( &OraSession );
  string sql =
  "SELECT trip_id,move_id,first_day,last_day,days,pr_del,tlg,tz "
  " FROM sched_days WHERE last_day>=:begin_date_season ";
  if ( trip_id > NoExists )
  	sql += " AND trip_id=:trip_id ";
  sql += "ORDER BY trip_id,move_id,num";
  SQry.SQLText = sql;
  SQry.CreateVariable( "begin_date_season", otDate, last_date_season );
  if ( trip_id > NoExists )
  	SQry.CreateVariable( "trip_id", otInteger, trip_id );
  SQry.Execute();
  int idx_trip_id = SQry.FieldIndex("trip_id");
  int idx_smove_id = SQry.FieldIndex("move_id");
//  int idx_first_dest = SQry.FieldIndex( "first_dest" );
  int idx_first_day = SQry.FieldIndex("first_day");
  int idx_last_day = SQry.FieldIndex("last_day");
  int idx_days = SQry.FieldIndex("days");
  int idx_spr_del = SQry.FieldIndex("pr_del");
  int idx_tlg = SQry.FieldIndex("tlg");
  int idx_ptz = SQry.FieldIndex("tz");

  if ( !SQry.RowCount() )
    AstraLocale::showErrorMessage( "MSG.NO_FLIGHTS_IN_SCHED" );
  // может нам надо получить все сразу маршруты
  GetDests( mapds, filter );
  /* теперь перейдем к выборке и фильтрации диапазонов */
  TViewPeriod viewperiod;
  viewperiod.trip_id = NoExists;
  int move_id = NoExists;
  TDestList ds;
  string s;
//  string exec, noexec;
  bool canRange;
  bool rangeListEmpty = false;
  while ( !SQry.Eof ) {
    if ( viewperiod.trip_id != SQry.FieldAsInteger( idx_trip_id ) ) {
      if ( !rangeListEmpty ) {
        if ( viewperiod.trip_id > NoExists ) {
        	viewp.push_back( viewperiod );
        }
      }
      viewperiod.trips.clear();
      viewperiod.exec.clear();
      viewperiod.noexec.clear();
      viewperiod.trip_id = SQry.FieldAsInteger( idx_trip_id );
      rangeListEmpty = true;
    }
    if ( move_id != SQry.FieldAsInteger( idx_smove_id ) ) {
      move_id = SQry.FieldAsInteger( idx_smove_id );
      ds = mapds[ move_id ];
      /* не работаем с рейсами у кот. нет delta = 0 */
      canRange = !ds.dests.empty();
    }
    if ( canRange ) {
        TDateTime first = SQry.FieldAsDateTime( idx_first_day );
        TDateTime last = SQry.FieldAsDateTime( idx_last_day );
        string days = SQry.FieldAsString( idx_days );
        bool pr_del = SQry.FieldAsInteger( idx_spr_del );
        string tlg = SQry.FieldAsString( idx_tlg );
        TDateTime utc_first = first;
        /* получим правила перехода(вывода) времен в рейсе */

        int ptz = SQry.FieldAsInteger( idx_ptz );
        string pregion = GetRegionFromTZ( ptz, mapreg );
        /* продолжаем фильтровать */
        time_period p( DateTimeToBoost( first ), DateTimeToBoost( last ) );
        time_period df( DateTimeToBoost( filter.range.first ), DateTimeToBoost( filter.range.last ) );
        /* фильтр по диапазонам, дням и временам вылета, если пользователь портовой */
  /* ??? надо ли переводить у фильтра дни выполнения в UTC */
        ds.flight_time = utc_first;
        ds.region = pregion;
//        ProgTrace( TRACE5, "move_id=%d, pregion=%s", move_id, pregion.c_str() );
        if ( df.intersects( p ) &&
             /* переводим диапазон выполнения в локальный формат - может быть сдвиг */
             ConvertPeriodToLocal( first, last, days, pregion, ptz, errtz ) &&
             CommonDays( days, filter.range.days ) && /* ??? в df.intersects надо посмотреть есть ли дни выполнения */
            ( ds.dests.empty() ||
              TReqInfo::Instance()->user.user_type == utAirport &&
              createAirportTrip( viewperiod.trip_id, filter, GetTZOffSet( first, ptz, v ), ds, true, err_city ) /*??? isfiltered */ ||
              TReqInfo::Instance()->user.user_type != utAirport &&
      	      createAirlineTrip( viewperiod.trip_id, filter, GetTZOffSet( utc_first, ptz, v ), ds, err_city ) ) ) {
          rangeListEmpty = false;
          TDateTime delta_out = NoExists; // переход через сутки по вылету
          delta_out = 0.0;
          if ( pr_del )
            AddRefPeriod( viewperiod.noexec, first, last, (int)delta_out, days, tlg );
          else
            AddRefPeriod( viewperiod.exec, first, last, (int)delta_out, days, tlg );
          for ( vector<trip>::iterator tr=ds.trips.begin(); tr!=ds.trips.end(); tr++ ) {
            TViewTrip vt;
            vt.trip_id = viewperiod.trip_id;
            vt.move_id = move_id;
            vt.name = tr->name;
            vt.crafts = tr->crafts;
   	        string ports_in, ports_out;
   	        for ( vector<TDest>::iterator h=tr->vecportsFrom.begin(); h!=tr->vecportsFrom.end(); h++ ) {
              if ( !ports_in.empty() )
                ports_in += "/";
              ports_in += ElemIdToElemCtxt( ecDisp, etAirp, h->airp, h->airp_fmt );
  	        }
  	        for ( vector<TDest>::iterator h=tr->vecportsTo.begin(); h!=tr->vecportsTo.end(); h++ ) {
              if ( !ports_out.empty() )
                ports_out += "/";
              ports_out += ElemIdToElemCtxt( ecDisp, etAirp, h->airp, h->airp_fmt );
   	        }
            if ( !ports_in.empty() && !ports_out.empty() )
              vt.ports = ports_in + "/" + ports_out;
            else
              vt.ports = ports_in + ports_out;
            vt.scd_in = tr->scd_in;
            vt.scd_out = tr->scd_out;
            if ( TReqInfo::Instance()->user.user_type == utAirport && // только для работников аэропорта
            	   vt.scd_out > NoExists ) { // для расчета загрузки слотов
            	vt.first = first;
            	vt.last = last;
            	vt.days = days;
            	vt.pr_del = pr_del;
            }
            viewperiod.trips.push_back( vt );
          }
          mapds[ move_id ].dests.clear(); /* уже использовали маршрут */
          mapds[ move_id ].trips.clear();
          ds.dests.clear();
          ds.trips.clear();
        } /* конец условия фильтра по диапазону */
    } /* end if canRange */
    SQry.Next();
  }
  if ( rangeListEmpty ) {
   ProgTrace( TRACE5, "drop rangelist trip_id=%d, move_id=%d", viewperiod.trip_id,move_id );
   /* удаляем ноду */
  }
  else {
    if ( viewperiod.trip_id > NoExists ) {
    	viewp.push_back( viewperiod );
    }
  }
 if ( errtz != NoExists )
    AstraLocale::showErrorMessage( "MSG.REGION_NOT_SPECIFIED_FOR_COUNTRY_WITH_ZONE.NOT_ALL_FLIGHTS_ARE_SHOWN",
    	                             LParams() << LParam("country", ElemIdToCodeNative(etCountry,"РФ")) << LParam("zone", errtz));
 if ( !err_city.empty() )
    showErrorMessage( "MSG.CITY.REGION_NOT_DEFINED.NOT_ALL_FLIGHTS_ARE_SHOWN",
                      LParams() << LParam("city", ElemIdToCodeNative(etCity,err_city)));
}

void buildViewTrips( const vector<TViewPeriod> viewp, xmlNodePtr dataNode )
{
  xmlNodePtr rangeListNode;
  double f;
  for ( vector<TViewPeriod>::const_iterator i=viewp.begin(); i!=viewp.end(); i++ ) {
  	rangeListNode = NewTextChild(dataNode, "rangeList");
    NewTextChild( rangeListNode, "trip_id", i->trip_id );
    NewTextChild( rangeListNode, "exec", i->exec );
    NewTextChild( rangeListNode, "noexec", i->noexec );
    xmlNodePtr tripsNode = NULL;
    int move_id;
    if ( i->trips.size() )
    	move_id = i->trips.begin()->move_id; // беру первый маршрут он можетсодержать несколько рейсов, их надо отображать 1019, 1020
    for ( vector<TViewTrip>::const_iterator j=i->trips.begin(); j!=i->trips.end() && move_id==j->move_id; j++ ) {
    	if ( !tripsNode )
        tripsNode = NewTextChild( rangeListNode, "trips" );
      xmlNodePtr tripNode = NewTextChild( tripsNode, "trip" );
      NewTextChild( tripNode, "move_id", j->move_id );
      NewTextChild( tripNode, "name", j->name );
      NewTextChild( tripNode, "crafts", j->crafts );
      NewTextChild( tripNode, "ports", j->ports );
      if ( j->scd_in > NoExists ) {
        NewTextChild( tripNode, "land", DateTimeToStr( modf((double)j->scd_in, &f ) ) );
      }
      if ( j->scd_out > NoExists )
        NewTextChild( tripNode, "takeoff", DateTimeToStr( modf((double)j->scd_out, &f ) ) );
    }
  }
}

void buildViewSlots( const vector<TViewPeriod> viewp, xmlNodePtr dataNode )
{
  double f;
  xmlNodePtr tripsNode = NULL;
  for ( vector<TViewPeriod>::const_iterator i=viewp.begin(); i!=viewp.end(); i++ ) {
    for ( vector<TViewTrip>::const_iterator j=i->trips.begin(); j!=i->trips.end(); j++ ) {
      if ( j->scd_out == NoExists || j->pr_del ) // только на вылет
      	continue;
    	if ( !tripsNode )
        tripsNode = NewTextChild( dataNode, "trips" );
      xmlNodePtr tripNode = NewTextChild( tripsNode, "trip" );
      NewTextChild( tripNode, "move_id", j->move_id );
      NewTextChild( tripNode, "name", j->name );
      NewTextChild( tripNode, "takeoff", DateTimeToStr( modf((double)j->scd_out, &f ) ) );
      NewTextChild( tripNode, "first", DateTimeToStr( j->first ) );
      NewTextChild( tripNode, "last", DateTimeToStr( j->last ) );
      NewTextChild( tripNode, "days", j->days );
    }
  }
}

namespace SEASON {
void ReadTripInfo( int trip_id, vector<TViewPeriod> &viewp, xmlNodePtr reqNode )
{
  xmlNodePtr filterNode = GetNode( "filter", reqNode );
  TFilter filter;
  filter.Parse( filterNode );
  internalRead( filter, viewp, trip_id );
}

}

void SeasonInterface::Read(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  //throw UserException( "Работа с экраном 'Сезонное расписание' временно остановлено. Идет обновление" );
  map<int,TDestList> mapds;
  TReqInfo *reqInfo = TReqInfo::Instance();
//  ri->user.check_access( amRead );
  xmlNodePtr filterNode = GetNode( "filter", reqNode );

  TFilter filter;
  bool init = filterNode;
  filter.Parse( filterNode );

  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  if ( !init ) {
    filterNode = NewTextChild( dataNode, "filter" );
    filter.Build( filterNode );
  }
  string mode;
  if ( reqInfo->user.user_type == utAirport  )
      mode = "port";
  else
      mode = "airline";
  NewTextChild( dataNode, "mode", mode );

  vector<TViewPeriod> viewp;
  internalRead( filter, viewp );
  sort( viewp.begin(), viewp.end(), ComparePeriod1 );
  buildViewTrips( viewp, dataNode );
  get_new_report_form("SeasonList", reqNode, resNode);
  STAT::set_variables(resNode);
  xmlNodePtr variablesNode = GetNode("form_data/variables", resNode);
  NewTextChild(variablesNode, "mode", mode);
}

void SeasonInterface::Slots(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  if ( reqInfo->user.user_type != utAirport  ) // only for airport
  	return;
  map<int,TDestList> mapds;
//  ri->user.check_access( amRead );
  xmlNodePtr filterNode = GetNode( "filter", reqNode );
  TFilter filter;
  bool init = filterNode;
  filter.Parse( filterNode );

  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  if ( !init ) {
    filterNode = NewTextChild( dataNode, "filter" );
    filter.Build( filterNode );
  }
  string mode;
  vector<TViewPeriod> viewp;
  internalRead( filter, viewp );
  buildViewSlots( viewp, dataNode );
}

void GetEditData( int trip_id, TFilter &filter, bool buildRanges, xmlNodePtr dataNode, string &err_city )
{
  //throw UserException( "Работа с экраном 'Сезонное расписание' временно остановлено. Идет обновление" );
	int errtz = NoExists;
  TQuery SQry( &OraSession );
  TDateTime begin_date_season = BoostToDateTime( filter.periods.begin()->period.begin() );
  // выбираем для редактирования все периоды, которые больше или равны текущей дате
  SQry.SQLText =
  "SELECT trip_id,move_id,first_day,last_day,days,pr_del,tlg,reference,tz "
  " FROM sched_days "
  " WHERE last_day>=:begin_date_season "
  "ORDER BY trip_id,move_id,num";
  SQry.CreateVariable( "begin_date_season", otDate, begin_date_season );
  SQry.Execute();
  int idx_trip_id = SQry.FieldIndex("trip_id");
  int idx_move_id = SQry.FieldIndex("move_id");
//  int idx_first_dest = SQry.FieldIndex( "first_dest" );
  int idx_first_day = SQry.FieldIndex("first_day");
  int idx_last_day = SQry.FieldIndex("last_day");
  int idx_days = SQry.FieldIndex("days");
  int idx_pr_del = SQry.FieldIndex("pr_del");
  int idx_tlg = SQry.FieldIndex("tlg");
  int idx_reference = SQry.FieldIndex( "reference" );
//  int idx_region = SQry.FieldIndex("region");
  int idx_tz = SQry.FieldIndex("tz");

  // может нам надо получить все сразу маршруты
  map<int,string> mapreg;
  map<int,TTimeDiff> v;
  map<int,TDestList> mapds;
  GetDests( mapds, filter );
  xmlNodePtr node;
  if ( trip_id > NoExists ) {
    NewTextChild( dataNode, "trip_id", trip_id );
    node = NewTextChild( dataNode, "ranges" );
  }
  int move_id = -1;
  int vtrip_id = -1;
  bool canRange;
  bool canTrips;
  bool DestsExists = false;
  while ( !SQry.Eof ) {
    TDateTime first = SQry.FieldAsDateTime( idx_first_day );
    TDateTime last = SQry.FieldAsDateTime( idx_last_day );
    int ptz = SQry.FieldAsInteger( idx_tz );
//    string pregion = SQry.FieldAsString( idx_region );
    string pregion = GetRegionFromTZ( ptz, mapreg );
    if ( pregion.empty() )
    	throw AstraLocale::UserException( "MSG.REGION_NOT_SPECIFIED_FOR_COUNTRY_WITH_ZONE",
    		                                LParams() << LParam("country", ElemIdToCodeNative(etCountry,"РФ")) << LParam("zone", ptz));
/*    TDateTime hours = GetTZTimeDiff( NowUTC(), first, last, ptz, v );
    first += hours; //???
    last += hours;*/
    if ( vtrip_id != SQry.FieldAsInteger( idx_trip_id ) ) {
      canTrips = true;
      vtrip_id = SQry.FieldAsInteger( idx_trip_id );
    }
    if ( move_id != SQry.FieldAsInteger( idx_move_id ) ) {
      move_id = SQry.FieldAsInteger( idx_move_id );
      if ( canTrips && !mapds[ move_id ].dests.empty() ) {
/*        if ( SQry.FieldIsNULL( idx_first_dest ) ) {
          ProgError( STDLOG, "first_dest is null, trip_id=%d, move_id=%d", trip_id, move_id );
          canTrips = false;
        }
        else {*/
//          ProgTrace( TRACE5, "create trip with trip_id=%d, move_id=%d", trip_id, move_id );
          mapds[ move_id ].flight_time = first;
          mapds[ move_id ].tz = ptz;
          mapds[ move_id ].region = pregion;
          if ( TReqInfo::Instance()->user.user_type == utAirport )
            canTrips = !createAirportTrip( vtrip_id, filter, GetTZOffSet( first, ptz, v ), mapds[ move_id ], false, err_city );
          else
            canTrips = !createAirlineTrip( vtrip_id, filter, GetTZOffSet( first, ptz, v ), mapds[ move_id ], err_city );
/*        } */
      }
      canRange = ( !mapds[ move_id ].dests.empty() && SQry.FieldAsInteger( idx_trip_id ) == trip_id );
    }
    if ( canRange && buildRanges ) {
    	DestsExists = true;
ProgTrace( TRACE5, "edit canrange move_id=%d", move_id );
      string days = SQry.FieldAsString( idx_days );

      double utcf;
      double f2, f3;
      modf( (double)first, &utcf );

      double first_day;
      modf( (double)UTCToClient( first, pregion ), &first_day );
      ProgTrace( TRACE5, "local first_day=%s",DateTimeToStr( first_day, "dd.mm.yyyy hh:nn:ss" ).c_str() );

      /* фильтр по диапазонам, дням и временам вылета, если пользователь портовой */
      /* переводим диапазон выполнения в локальный формат - может быть сдвиг */
      if ( ConvertPeriodToLocal( first, last, days, pregion, ptz, errtz ) ) { // ptz
        xmlNodePtr range = NewTextChild( node, "range" );
        NewTextChild( range, "move_id", move_id );
        NewTextChild( range, "first", DateTimeToStr( (int)first ) );
        NewTextChild( range, "last", DateTimeToStr( (int)last ) );
        NewTextChild( range, "days", days );
        if ( SQry.FieldAsInteger( idx_pr_del ) )
          NewTextChild( range, "cancel", 1 );
        if ( !SQry.FieldIsNULL( idx_tlg ) )
          NewTextChild( range, "tlg", SQry.FieldAsString( idx_tlg ) );
        if ( !SQry.FieldIsNULL( idx_reference ) )
          NewTextChild( range, "ref", SQry.FieldAsString( idx_reference ) );
/*        if ( SQry.FieldIsNULL( idx_first_dest ) ) {
          ProgError( STDLOG, "first_dest is null, trip_id=%d, move_id=%d", trip_id, move_id );
        }
        else {*/
          /* передаем данные для экрана редактирования */
          if ( !mapds[ move_id ].dests.empty() ) {
            xmlNodePtr destsNode = NewTextChild( range, "dests" );
            for ( TDests::iterator id=mapds[ move_id ].dests.begin(); id!=mapds[ move_id ].dests.end(); id++ ) {
              xmlNodePtr destNode = NewTextChild( destsNode, "dest" );
              NewTextChild( destNode, "cod", ElemIdToElemCtxt( ecDisp, etAirp, id->airp, id->airp_fmt ) );
      	      if ( id->airp != id->city )
                NewTextChild( destNode, "city", id->city );
              if ( id->pr_del )
      	        NewTextChild( destNode, "cancel", id->pr_del );
              // а если в этом порту другие правила перехода гп летнее/зимнее расписание ???
              // issummer( TDAteTime, region ) != issummer( utcf, pult.region );
      	      if ( id->scd_in > NoExists ) {
                f2 = modf( (double)id->scd_in, &f3 );
    		        f3 += utcf + fabs( f2 );
                ProgTrace( TRACE5, "utc scd_in=%s", DateTimeToStr( f3, "dd.mm.yyyy hh:nn:ss" ).c_str() );
                try {
                  f2 = modf( (double)UTCToClient( f3, id->region ), &f3 );
                }
                catch( Exception &e ) {
                	throw AstraLocale::UserException( "MSG.CITY.REGION_NOT_DEFINED",
                		                                LParams() << LParam("city", ElemIdToCodeNative(etCity,id->city)));
                }
                ProgTrace( TRACE5, "local date scd_in=%s, time scd_in=%s",
                           DateTimeToStr( f3, "dd.mm.yy" ).c_str(),
                           DateTimeToStr( f2, "dd.mm.yy hh:nn" ).c_str() );
                if ( f3 < first_day )
                  id->scd_in = f3 - first_day - f2;
                else
                  id->scd_in = f3 - first_day + f2;

      	        NewTextChild( destNode, "land", DateTimeToStr( id->scd_in ) ); //???
              }
      	      if ( !id->airline.empty() )
      	        NewTextChild( destNode, "company", ElemIdToElemCtxt( ecDisp, etAirline, id->airline, id->airline_fmt ) );
      	      if ( id->trip > NoExists )
      	        NewTextChild( destNode, "trip", id->trip );
      	      if ( !id->craft.empty() )
      	        NewTextChild( destNode, "bc", ElemIdToElemCtxt( ecDisp, etCraft, id->craft, id->craft_fmt ) );
      	      if ( !id->litera.empty() )
                NewTextChild( destNode, "litera", id->litera );
      	      if ( !isDefaultTripType(id->triptype) )
      	        NewTextChild( destNode, "triptype", ElemIdToCodeNative(etTripType,id->triptype) );
      	      if ( id->scd_out > NoExists ) {
                f2 = modf( (double)id->scd_out, &f3 );
                f3 += utcf + fabs( f2 );
                ProgTrace( TRACE5, "utc scd_out=%s",DateTimeToStr( f3, "dd.mm.yyyy hh:nn:ss" ).c_str() );
                try {
                  f2 = modf( (double)UTCToClient( f3, id->region ), &f3 );
                }
                catch( Exception &e ) {
                	throw AstraLocale::UserException( "MSG.CITY.REGION_NOT_DEFINED",
                		                                LParams() << LParam("city", ElemIdToCodeNative(etCity,id->city)));
                }
                ProgTrace( TRACE5, "local date scd_out=%s, time scd_out=%s",
                           DateTimeToStr( f3, "dd.mm.yy" ).c_str(),
                           DateTimeToStr( f2, "dd.mm.yy hh:nn" ).c_str() );
                if ( f3 < first_day )
                  id->scd_out = f3 - first_day - f2;
                else
                  id->scd_out = f3 - first_day + f2;
      	        NewTextChild( destNode, "takeoff", DateTimeToStr( id->scd_out ) );
              }
      	      if ( id->f )
      	        NewTextChild( destNode, "f", id->f );
      	      if ( id->c )
      	        NewTextChild( destNode, "c", id->c );
      	      if ( id->y )
      	        NewTextChild( destNode, "y", id->y );
      	      if ( !id->unitrip.empty() )
      	        NewTextChild( destNode, "unitrip", id->unitrip );
      	      if ( !id->suffix.empty() )
      	        NewTextChild( destNode, "suffix", ElemIdToElemCtxt( ecDisp, etSuffix, id->suffix, id->suffix_fmt ) );
      	    } // end for
            mapds[ move_id ].dests.clear(); /* уже использовали маршрут */
          } // end if
/*        } // end else */
      }
    }
    SQry.Next();
  }

  if ( !DestsExists && trip_id > NoExists )
  	throw AstraLocale::UserException( "MSG.FLIGHT_DELETED.REFRESH_DATA" );

  vector<TViewPeriod> viewp;
  TViewPeriod p;
  for ( map<int,TDestList>::iterator im=mapds.begin(); im!=mapds.end(); im++ ) {
    for ( vector<trip>::iterator t=im->second.trips.begin(); t!=im->second.trips.end(); t++ ) {
    	p.trips.clear();
    	TViewTrip vt;
    	vt.trip_id = t->trip_id;
    	vt.move_id = im->first;
    	vt.name = t->name;
    	p.trips.push_back( vt );
    	viewp.push_back( p );
    }
  }
  sort( viewp.begin(), viewp.end(), ComparePeriod );

  xmlNodePtr tripsNode = NULL;
  for ( vector<TViewPeriod>::iterator p=viewp.begin(); p!=viewp.end(); p++ ) {
    for ( vector<TViewTrip>::iterator t=p->trips.begin(); t!=p->trips.end(); t++ ) {
      if ( !tripsNode )
        tripsNode = NewTextChild( dataNode, "trips" );
      xmlNodePtr tripNode = NewTextChild( tripsNode, "trip" );
      NewTextChild( tripNode, "trip_id", t->trip_id );
      NewTextChild( tripNode, "name", t->name );
    }
  }
}

// список периодов + маршруты к периодам + список рейсов для быстрого перехода
void SeasonInterface::Edit(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TPerfTimer tm;
  tm.Init();
//  TReqInfo::Instance()->user.check_access( amRead );
  int trip_id = NodeAsInteger( "trip_id", reqNode );
  ProgTrace( TRACE5, "edit trip_id=%d", trip_id );
  xmlNodePtr filterNode = GetNode( "filter", reqNode );
  TFilter filter;
  filter.Parse( filterNode );
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  string err_city;
  GetEditData( trip_id, filter, trip_id > NoExists, dataNode, err_city );
  ProgTrace(TRACE5, "getdata %ld", tm.Print());

}

void SeasonInterface::convert(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TQuery Qry( &OraSession );
  Qry.SQLText =
   "SELECT trip_id,num,move_id,first_day,last_day,days,pr_cancel pr_del,tlg,reference "
   " FROM drop_sched_days WHERE first_day >=to_date('24.03.07','DD.MM.YY') ORDER BY trip_id,move_id,num ";
  Qry.Execute();

  TQuery DQry( &OraSession );
  DQry.SQLText =
   "SELECT move_id,num,cod airp,pr_cancel pr_del,land+delta_in scd_in,company airline,trip flt_no,bc craft,"
   "       litera,triptype trip_type,takeoff+delta_out scd_out,f,c,y,unitrip,suffix "\
   " FROM drop_routes WHERE move_id=:move_id ORDER BY num";
  DQry.DeclareVariable( "move_id", otInteger );

  int trip_id = NoExists, move_id = NoExists;
  xmlNodePtr reqn = NewTextChild( resNode, "data" );
  xmlNodePtr node;
  while ( !Qry.Eof ) {
  	if ( trip_id != Qry.FieldAsInteger( "trip_id" ) ) {
  		if ( trip_id > NoExists ) {
  			// определить текущий сезон
  			//xmlNodePtr fnode = NewTextChild( reqn, "filter" );
    	  try {
  	      Write( ctxt, reqn, resNode );
        }
        catch( std::exception &E ) {
          ProgError( STDLOG, "std::exception: %s, trip_id=%d", E.what(), trip_id );
        }
        catch( ... ) {
          ProgError( STDLOG, "Unknown error, trip_id=%d", trip_id );
        };
        xmlUnlinkNode( node );
        xmlFreeNode( node );

  		}
  		trip_id = Qry.FieldAsInteger( "trip_id" );
  		node = NewTextChild( reqn, "SubrangeList" );
  	}
  	xmlNodePtr rangeNode = NewTextChild( node, "range" );
  		NewTextChild( rangeNode, "modify", "finsert" );
  		NewTextChild( rangeNode, "move_id", Qry.FieldAsInteger( "move_id" ) );
  		NewTextChild( rangeNode, "first", DateTimeToStr( Qry.FieldAsDateTime( "first_day" ) ) );
  		NewTextChild( rangeNode, "last", DateTimeToStr( Qry.FieldAsDateTime( "last_day" ) ) );
  		NewTextChild( rangeNode, "days", Qry.FieldAsString( "days" ) );
  		NewTextChild( rangeNode, "tlg", Qry.FieldAsString( "tlg" ) );
  		NewTextChild( rangeNode, "ref", Qry.FieldAsString( "reference" ) );
  		if ( move_id != Qry.FieldAsInteger( "move_id" ) ) {
  			move_id = Qry.FieldAsInteger( "move_id" );
  			xmlNodePtr dnode = NewTextChild( rangeNode, "dests" );
  			DQry.SetVariable( "move_id", move_id );
  			DQry.Execute();
  			while (!DQry.Eof) {
  			 xmlNodePtr d = NewTextChild( dnode, "dest" );
  			 NewTextChild( d, "cod", DQry.FieldAsString( "airp" ) );
  			 NewTextChild( d, "cancel", DQry.FieldAsInteger( "pr_del" ) );
  			 if ( !DQry.FieldIsNULL( "scd_in" ) )
  			 	NewTextChild( d, "land", DateTimeToStr( DQry.FieldAsDateTime( "scd_in" ) ) );
  			 if ( !DQry.FieldIsNULL( "airline" ) )
  			 	NewTextChild( d, "company", DQry.FieldAsString( "airline" ) );
  			 if ( !DQry.FieldIsNULL( "flt_no" ) )
  			 	NewTextChild( d, "trip", DQry.FieldAsInteger( "flt_no" ) );
  			 if ( !DQry.FieldIsNULL( "craft" ) )
  			 	NewTextChild( d, "bc", DQry.FieldAsString( "craft" ) );
  			 if ( !DQry.FieldIsNULL( "litera" ) )
  			 	NewTextChild( d, "litera", DQry.FieldAsString( "litera" ) );
  			 if ( !DQry.FieldIsNULL( "trip_type" ) )
  			 	NewTextChild( d, "triptype", ElemIdToCodeNative(etTripType,DQry.FieldAsString( "trip_type" )) );
  			 if ( !DQry.FieldIsNULL( "scd_out" ) )
  			 	NewTextChild( d, "takeoff", DateTimeToStr( DQry.FieldAsDateTime( "scd_out" ) ) );
  			 if ( DQry.FieldAsInteger( "f" ) )
  			 	NewTextChild( d, "f", DQry.FieldAsInteger( "f" ) );
  			 if ( DQry.FieldAsInteger( "c" ) )
  			 	NewTextChild( d, "c", DQry.FieldAsInteger( "c" ) );
  			 if ( DQry.FieldAsInteger( "y" ) )
  			 	NewTextChild( d, "y", DQry.FieldAsInteger( "y" ) );
  			 if ( !DQry.FieldIsNULL( "unitrip" ) )
  			 	NewTextChild( d, "unitrip", DQry.FieldAsString( "unitrip" ) );
  			 if ( !DQry.FieldIsNULL( "suffix" ) )
  			 	NewTextChild( d, "suffix", DQry.FieldAsString( "suffix" ) );
  				DQry.Next();
  			}
  		}

  	Qry.Next();
  }
 if ( trip_id > NoExists ) {
 	try {
  	Write( ctxt, reqn, resNode );
  }
  catch( std::exception &E ) {
    ProgError( STDLOG, "std::exception: %s, trip_id=%d", E.what(), trip_id );
  }
  catch( ... ) {
    ProgError( STDLOG, "Unknown error, trip_id=%d", trip_id );
  };
 }

}


void SeasonInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
}

TDoubleTrip::TDoubleTrip( )
{
	Qry = new TQuery( &OraSession );
	Qry->SQLText =
     "SELECT point_id, scd_in, scd_out FROM points "
     " WHERE airline=:airline AND flt_no=:flt_no AND NVL(suffix,' ')=NVL(:suffix,' ') AND "
     "       move_id!=:move_id AND airp=:airp AND pr_del!=-1 AND "
     "       ( scd_in BETWEEN :scd_in-2 AND :scd_in+2 OR "
     "         scd_out BETWEEN :scd_out-2 AND :scd_out+2 )";
  Qry->DeclareVariable( "move_id", otInteger );
  Qry->DeclareVariable( "airp", otString );
  Qry->DeclareVariable( "airline", otString );
  Qry->DeclareVariable( "flt_no", otInteger );
  Qry->DeclareVariable( "suffix", otString );
  Qry->DeclareVariable( "scd_in", otDate );
  Qry->DeclareVariable( "scd_out", otDate );
}

bool TDoubleTrip::IsExists( int move_id, string airline, int flt_no,
	                          string suffix, string airp,
	                          TDateTime scd_in, TDateTime scd_out,
                            int &point_id )
{
  point_id = NoExists;
	TElemFmt fmt;
	airp = ElemToElemId( etAirp, airp, fmt );
	suffix = ElemToElemId( etSuffix, suffix, fmt );
	airline = ElemToElemId( etAirline, airline, fmt );
	Qry->SetVariable( "move_id", move_id );
	Qry->SetVariable( "flt_no", flt_no );
	Qry->SetVariable( "suffix", suffix );
	Qry->SetVariable( "airline", airline );
	Qry->SetVariable( "airp", airp );
	if ( scd_in > NoExists )
	  Qry->SetVariable( "scd_in", scd_in );
	else
		Qry->SetVariable( "scd_in", FNull );
	if ( scd_out > NoExists )
		Qry->SetVariable( "scd_out", scd_out );
	else
		Qry->SetVariable( "scd_out", FNull );
	Qry->Execute();
	bool res = false;
  double local_scd_in,local_scd_out,d1;
  string region;
  if ( scd_in > NoExists ) {
    region = AirpTZRegion( airp );
    d1 = UTCToLocal( scd_in, region );
    modf( d1, &local_scd_in );
  }
  else local_scd_in = NoExists;
  if ( scd_out > NoExists ) {
    if ( region.empty () )
      region = AirpTZRegion( airp );
    d1 = UTCToLocal( scd_out, region );
    modf( d1, &local_scd_out );
  }
  else local_scd_out = NoExists;
  while ( !Qry->Eof ) {
  	if ( !Qry->FieldIsNULL( "scd_in" ) && local_scd_in > NoExists ) {
      modf( (double)UTCToLocal( Qry->FieldAsDateTime( "scd_in" ), region ), &d1 );
      if ( d1 == local_scd_in ) {
   			res = true;
   			point_id = Qry->FieldAsInteger( "point_id" );
        break;
      }
    }
    if ( !Qry->FieldIsNULL( "scd_out" ) && local_scd_out > NoExists ) {
      modf( (double)UTCToLocal( Qry->FieldAsDateTime( "scd_out" ), region ), &d1 );
      if ( d1 == local_scd_out ) {
   			res = true;
   			point_id = Qry->FieldAsInteger( "point_id" );
        break;
      }
    }
    Qry->Next();
  }
	return res;
}

TDoubleTrip::~TDoubleTrip()
{
	delete Qry;
}




/* проверка времен по всему маршруту */
       /* переводим времена в фильтре во время UTC относительно города в маршруте */
/*       if ( !timeKey ) {
       	 if ( !d.region.empty() ) {
           // переводим время начала расписания в UTC
           TDateTime t = BoostToDateTime( filter.periods[ filter.season_idx ].period.begin() );
           // переводим время во время клиента и для верности удаляем время и добавляем день
           double f1,f2,f3;
           modf( t + 10, &f1 );
           TDateTime f,l;
           // переводим время в UTC
           // normilize date
           f2 = modf( (double)ClientToUTC( f1 + filter.firstTime, d.region ), &f3 );
           ProgTrace( TRACE5, "f1=%s,f2=%s,f3=%s",
                      DateTimeToStr( f1, "dd.mm.yyyy hh:nn" ).c_str(),
                      DateTimeToStr( f2, "dd.mm.yyyy hh:nn" ).c_str(),
                      DateTimeToStr( f3, "dd.mm.yyyy hh:nn" ).c_str() );

           if ( f3 < f1 )
            f = f3 - f1 - f2;
          else
            f = f3 - f1 + f2;

           f2 = modf( (double)ClientToUTC( f1 + filter.lastTime, d.region ), &f3 );
           if ( f3 < f1 )
            l = f3 - f1 - f2;
           else
            l = f3 - f1 + f2;

           //! надо вvделять только время, без учета числа и перехода суток
           f = modf( (double)f, &f1 );
           l = modf( (double)l, &f1 );

           if ( f < 0 )
             f = fabs( f );
           else
             f += 1.0;
           if ( l < 0 )
             l = fabs( l );
           else
             l += 1.0;

           f -= (double)1000/(double)MSecsPerDay;
           l +=  (double)1000/(double)MSecsPerDay;
           if ( d.scd_in == NoExists )
             f1 = NoExists;
           else {
             f1 = modf( (double)d.scd_in, &f2 );
             if ( f1 < 0 )
               f1 = fabs( f1 );
             else
              f1 += 1.0;
           }
           if ( d.scd_out == NoExists )
             f2 = NoExists;
           else {
             f2 = modf( (double)d.scd_out, &f3 );
             if ( f2 < 0 )
               f2 = fabs( f2 );
             else
               f2 += 1.0;
           }

           ProgTrace( TRACE5, "f=%f,l=%f, f1=%f, f2=%f", f, l, f1, f2 );

           timeKey = f1 >= f1, f2 );

           timeKey = f1 >= f1, f2 );

           timeKey = f1 >= f1, f2 );

           timeKey = f1 >= f && f1 <= l ||
                     f2 >= f && f2 <= l;
         }
       } // end !timeKey
*/
/*        }
       } // end !timeKey
*/

