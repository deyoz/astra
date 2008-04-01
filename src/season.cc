#include "season.h"
#include "xml_unit.h"
#include "exceptions.h"
#define NICKNAME "DJEK"
#include "test.h"
#include "oralib.h"
#include "cache.h"
#include "misc.h"
#include <map>
#include <vector>
#include <string>
#include "stages.h"
#include "boost/date_time/local_time/local_time.hpp"
#include "basic.h"
#include "stl_utils.h"
#include "stat.h"
#include "docs.h"
#include "base_tables.h"


const int SEASON_PERIOD_COUNT = 4;
const int SEASON_PRIOR_PERIOD = 1;

using namespace BASIC;
using namespace EXCEPTIONS;
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
//��� �v�������� ��������  int first_dest;
  TDateTime first;
  TDateTime last;
  string days;
  string tlg;
  string ref;
  tmodify modify;
  bool cancel;
  int hours;
  TPeriod() {
    modify = fnochange;
    cancel = false;
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
  string cod;
  string city;
  int pr_cancel;
  TDateTime Land;
  string company;
  string region;
  int trip;
  string bc;
  string litera;
  string triptype;
  TDateTime Takeoff;
  int f;
  int c;
  int y;
  string unitrip;
  string suffix;
};

struct trip {
  int trip_id;
  string name;
  string print_name;
  string crafts;
  string ownbc;
  string ownport;
  string portsForAirline;
  vector<TDest> vecportsFrom, vecportsTo;
  string bold_ports;
  TDateTime land;
  TDateTime takeoff;
  TDateTime trap;
  string triptype;
  int cancel;
  trip() {
    land = NoExists;
    takeoff = NoExists;
    trap = NoExists;
  }
};

typedef vector<TDest> TDests;

struct TDestList {
  bool cancel;
  TDateTime flight_time;
  TDateTime last_day;
  int tz;
  string region;
  TDests dests;
  vector<trip> trips;
};

typedef map<int,TDestList> tmapds;

typedef map<double,tmapds> TSpp;

struct TRangeList {
  int trip_id;
  vector<TPeriod> periods;
};

class TFilter {
  private:
  public:
    int dst_offset;
    int tz;
    vector<TSeason> periods; //��ਮ�� ��⭥�� � ������� �ᯨᠭ��
    string region; // ॣ��� �⭮�⥫쭮 ���ண� ����v������ ��ਮ�v �ᯨᠭ��
    int season_idx; // ⥪�饥 �ᯨᠭ��
    TRange range; // �������� ��� � 䨫���, ����� �� ����� - �������� �ᯨᠭ�� � �६�����
    TDateTime firstTime;
    TDateTime lastTime;
    string company;
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
    bool isFilteredTime( TDateTime first_day, TDateTime Land, TDateTime Takeoff,
                         int dst_offset, string region );
    bool isFilteredUTCTime( TDateTime vd, TDateTime first, TDateTime dest_time, int dst_offset );
    bool isFilteredTime( TDateTime vd, TDateTime first_day, TDateTime Land, TDateTime Takeoff,
                         int dst_offset, string vregion );

    TFilter();
};

bool createAirportTrip( string airp, int trip_id, TFilter filter, int offset, TDestList &ds, 
                        TDateTime vdate, bool viewOwnPort, bool UTCFilter );
bool createAirportTrip( int trip_id, TFilter filter, int offset, TDestList &ds, bool viewOwnPort );
bool createAirlineTrip( int trip_id, TFilter &filter, int offset, TDestList &ds );
bool createAirlineTrip( int trip_id, TFilter &filter, int offset, TDestList &ds, TDateTime localdate );
TDateTime GetTZTimeDiff( TDateTime utcnow, TDateTime first, TDateTime last, int tz, map<int,TTimeDiff> &v );
int GetTZOffSet( TDateTime first, int tz, map<int,TTimeDiff> &v );
void GetDests( map<int,TDestList> &mapds, const TFilter &filter, int move_id = NoExists );
string GetCommonDays( string days1, string days2 );
bool CommonDays( string days1, string days2 );
string DeleteDays( string days1, string days2 );
void ClearNotUsedDays( TDateTime first, TDateTime last, string &days );

void PeriodToUTC( TDateTime &first, TDateTime &last, string &days, const string region );

void internalRead( TFilter &filter, xmlNodePtr dataNode, int trip_id = NoExists );
void GetEditData( int trip_id, TFilter &filter, bool buildRanges, xmlNodePtr dataNode );

void createSPP( TDateTime localdate, TSpp &spp, vector<TStageTimes> &stagetimes, bool createViewer );

string GetPrintName( TDest *PDest, TDest *NDest )
{
	string res;
  if ( !PDest || NDest->trip > NoExists && abs( PDest->trip - NDest->trip ) <= 1 && PDest->company == NDest->company ) {
    res = NDest->company;
    while ( res.size() < 3 ) {
    	res += " ";
    }
    	res += IntToString( NDest->trip );
  }
  else {
    res = PDest->company;
    while ( res.size() < 3 ) {
    	res += " ";
    }
    res += IntToString( PDest->trip );
    if ( NDest->trip > NoExists ) {
    	res += "/";
    	if ( PDest->company != NDest->company ) {
    		string comp = NDest->company;
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
  GQry.SQLText = "SELECT tz FROM tz_regions WHERE region=:region AND pr_del=0";
  GQry.CreateVariable( "region", otString, region );
  GQry.Execute();
  tz = GQry.FieldAsInteger( "tz" );
}

void TFilter::Clear()
{
  season_idx = 0;
  firstTime = NoExists;
  lastTime = NoExists;
  company.clear();
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
  local_date_time ld( utcd, tz ); /* ��।��塞 ⥪�饥 �६� �����쭮� */
  bool summer = true;
  /* ��⠭�������� ���� ��� � �ਧ��� ��ਮ�� */
  if ( tz->has_dst() ) {  // �᫨ ���� ���室 �� ������/��⭥� �ᯨᠭ��
    dst_offset = tz->dst_offset().hours();
    if ( ld.is_dst() ) {  // �᫨ ᥩ�� ���
      year--;
      summer = false;
    }
    else {  // �᫨ ᥩ�� ����
      ptime start_time = tz->dst_local_start_time( year );
      if ( ld.local_time() < start_time )
        year--;
      summer = true;
    }
  }
  else {
   year--;
   dst_offset = 0;
  }
  for ( int i=0; i<SEASON_PERIOD_COUNT; i++ ) {
    ptime s_time, e_time;
    string name;
    if ( tz->has_dst() ) {
      if ( summer ) {
        s_time = tz->dst_local_start_time( year ) - tz->base_utc_offset();
        e_time = tz->dst_local_end_time( year ) - tz->base_utc_offset() - seconds(1);
        name = string( "��� " ) + IntToString( year );
      }
      else {
        s_time = tz->dst_local_end_time( year ) - tz->base_utc_offset() - tz->dst_offset();
        year++;
        e_time = tz->dst_local_start_time( year ) - tz->base_utc_offset() - seconds(1);
        name = string( "���� " ) + IntToString( year - 1 ) + "-" + IntToString( year );
      }
      summer = !summer;
    }
    else {
     /* ��ਮ� - �� 楫� ��� */
     s_time = ptime( boost::gregorian::date(year,1,1) );
     year++;
     e_time = ptime( boost::gregorian::date(year,1,1) );
     name = IntToString( year - 1 ) + " ���";
    }
    ProgTrace( TRACE5, "s_time=%s, e_time=%s, summer=%d",
               DateTimeToStr( UTCToLocal( BoostToDateTime(s_time), region ),"dd.mm.yy hh:nn:ss" ).c_str(),
               DateTimeToStr( UTCToLocal( BoostToDateTime(e_time), region ), "dd.mm.yy hh:nn:ss" ).c_str(), !summer );
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
    if ( !p->period.contains( r ) ) // �� ���ᥪ�����
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
    if ( id->Land > NoExists ) {
    	if ( id->Land >= 0 ) {
    	  f1 = utcfirst1 + id->Land;
    	  f2 = utcfirst2 + id->Land;
      }
      else {
      	double f3 = fabs( modf( (double)id->Land, &f1 ) );
      	f2 = f1 + utcfirst2 + f3;
      	f1 += utcfirst1 + f3;
      }
    	diff = getDiff( dst_offset, filter->isSummer( f1 ), filter->isSummer( f2 ) );
      if ( id->Land >= 0 )
        id->Land += diff;
      else
        id->Land -= diff;
    }
    if ( id->Takeoff > NoExists ) {
    	if ( id->Takeoff >= 0 ) {
    	  f1 = utcfirst1 + id->Takeoff;
    	  f2 = utcfirst2 + id->Takeoff;
      }
      else {
      	double f3 = fabs( modf( (double)id->Takeoff, &f1 ) );
      	f2 = f1 + utcfirst2 + f3;
      	f1 += utcfirst1 + f3;
      }

    	diff = getDiff( dst_offset, filter->isSummer( f1 ), filter->isSummer( f2 ) );

      if ( id->Takeoff >= 0 )
        id->Takeoff += diff;
      else
      	id->Takeoff -= diff;

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

/* �� �� ����������� ��������� ������뢠�� ��������� � ����祭�� ��࠭塞 */
/* speriods ᮤ�ন� �� ��ਮ�� � ����묨 ���� ���ᥪ��� */
/* nperiods - ᮤ�ন� ������⢮ ����祭��� ��ਮ��� */
void TFilter::InsertSectsPeriods( map<int,TDestList> &mapds,
                                  vector<TPeriod> &speriods, vector<TPeriod> &nperiods, TPeriod p )
{
	TDateTime diff;
  TPeriod np;
  double f1, f2;
  bool issummer;
  bool psummer = isSummer( p.first );
  time_period s( DateTimeToBoost( p.first ), DateTimeToBoost( p.last ) );
  //ࠧ��⨥ ��ਮ��� periods ��ਮ��� p
  for ( vector<TPeriod>::iterator ip=speriods.begin(); ip!=speriods.end(); ip++ ) {
    // ��ਮ�� �࠭��� �६� �뫥� �� �.�.
    if ( ip->modify == fdelete )
      continue;
    issummer = isSummer( ip->first );

ProgTrace( TRACE5, "p.move_id=%d, ip->move_id=%d, psummer=%d, issummer=%d",
           p.move_id, ip->move_id, psummer, issummer );
    if ( p.move_id == ip->move_id && psummer != issummer ) {
        //������� ��뫪� �� ���� � �� �� �������, � ��ਮ�� �ਭ������� ࠧ�� ᥧ���� - ࠧ���� � ��⮢�� ����!!!
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
    // ���� ����祭��
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
    // ��� ��᮪ ��ਮ�� ����� ���� � ��㣮� �ᯨᠭ��, � ᫥����⥫쭮 ����� ��㣨� �६���
    diff = getDiff( dst_offset, issummer, psummer );
    modf( (double)BoostToDateTime( d.begin() ), &f1 );
    np.first = f1 + modf( (double)ip->first, &f2 ) + diff;
    modf( (double)BoostToDateTime( d.end() ), &f2 );
    np.last = f2 + modf( (double)ip->first, &f1 ) + diff;
    np.days = DeleteDays( ip->days, p.days ); // 㤠�塞 �� p.days ��� ip->days
    ClearNotUsedDays( np.first, np.last, np.days );
    ProgTrace( TRACE5, "result np->first=%s, np->last=%s, np->days=%s, ip->days=%s",
               DateTimeToStr( np.first,"dd.mm.yy hh:nn:ss" ).c_str(),
               DateTimeToStr( np.last,"dd.mm.yy hh:nn:ss" ).c_str(),
               np.days.c_str(), ip->days.c_str() );

    if ( np.days != NoDays ) {
      /* ࠧ���� ��ਮ� - ��� ��᮪ ����� �ਭ�������� ��㣮�� �ᯨᠭ�� */
      if ( diff ) {
        /* ���� ��ᬠ�ਢ��� ����� ��ਮ� ��� �⤥��� � �� ���७�� */
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
    ProgTrace( TRACE5, "result np->first=%s, np->last=%s, np->days=%s",
               DateTimeToStr( np.first,"dd.mm.yy hh:nn" ).c_str(),
               DateTimeToStr( np.last,"dd.mm.yy hh:nn" ).c_str(),
               np.days.c_str() );

      if ( np.days != NoDays ) {
        /* ࠧ���� ��ਮ� - ��� ��᮪ ����� �ਭ�������� ��㣮�� �ᯨᠭ�� */
        if ( diff ) {
          /* ���� ��ᬠ�ਢ��� ����� ��ਮ� ��� �⤥��� � �� ���७�� */
          np.modify = finsert;
          np.move_id = calcDestsProp( this, mapds, dst_offset, np.move_id, ip->first, np.first );
        }
        nperiods.push_back( np );
      }
    }
    ip->modify = fdelete;
  }
//  p.modify = fnochange;
ProgTrace( TRACE5, "first=%s, last=%s",
           DateTimeToStr( p.first, "dd.mm.yyyy hh:nn:ss" ).c_str(),
           DateTimeToStr( p.last, "dd.mm.yyyy hh:nn:ss" ).c_str() );

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

  //!!!!!!!! ���� �v������ ⮫쪮 �६�, ��� ��� �᫠ � ���室� ��⮪
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

  //��� ���室� �६���
  TDateTime diff = getDiff( dst_offset, isSummer( first ), isSummer( vd ) );
  f1 += diff;

  return ( f1 >= f && f1 <= l );
}


bool TFilter::isFilteredTime( TDateTime vd, TDateTime first_day, TDateTime Land, TDateTime Takeoff,
                              int dst_offset, string vregion )
{
	ProgTrace( TRACE5, "filter.firsttime=%s, filter.lasttime=%s, first_day=%s, Land=%s, Takeoff=%s", 
	           DateTimeToStr( firstTime, "dd.mm hh:nn" ).c_str(),
	           DateTimeToStr( lastTime, "dd.mm hh:nn" ).c_str(),
	           DateTimeToStr( first_day, "dd.mm hh:nn" ).c_str(),
	           DateTimeToStr( Land, "dd.mm hh:nn" ).c_str(),
	           DateTimeToStr( Takeoff, "dd.mm hh:nn" ).c_str());
  if ( firstTime == NoExists || region.empty() )
    return true;
  /* ��ॢ���� �६��� � 䨫��� �� �६� UTC �⭮�⥫쭮 ��த� � ������� */
  // ��ॢ���� �६� ��砫� �ᯨᠭ�� � UTC
  // ��ॢ���� �६� �� �६� ������ � ��� ��୮�� 㤠�塞 �६� � ������塞 ����
  double f1,f2,f3;
  //???modf( t, &f1 );
  modf( first_day, &f1 );
  TDateTime f,l;
  // ��ॢ���� �६� 䨫��� � UTC
  // normilize date
  try {
    f2 = modf( (double)ClientToUTC( f1 + firstTime, vregion ), &f3 );
  }
  catch( boost::local_time::ambiguous_result ) {
  	f2 = modf( (double)ClientToUTC( f1 + 1 + firstTime, vregion ), &f3  );
  	f3--;
/*!!! 	  throw UserException( string("�६� ��砫� �믮������ ३� �� ��।����� �������筮 ") + DateTimeToStr( f1 + firstTime, "dd.mm hh:nn" ).c_str() );*/
  }
  catch( boost::local_time::time_label_invalid ) {
    throw UserException( "�६� ��砫� �믮������ ३� �� ������� %s",
                         DateTimeToStr( f1 + firstTime, "dd.mm hh:nn" ).c_str() );
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
 	  /*!!!throw UserException( string("�६� ����砭�� �믮������ ३� �� ��।����� �������筮 ") + DateTimeToStr( f1 + lastTime, "dd.mm hh:nn" ).c_str() );*/
  }
  catch( boost::local_time::time_label_invalid ) {
    throw UserException( "�६� ����砭�� �믮������ ३� �� ������� %s",
                         DateTimeToStr( f1 + lastTime, "dd.mm hh:nn" ).c_str() );
  }  
  if ( f3 < f1 )
    l = f3 - f1 - f2;
  else
    l = f3 - f1 + f2;

  //!!!!!!!! ���� �v������ ⮫쪮 �६�, ��� ��� �᫠ � ���室� ��⮪
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
  if ( Land == NoExists )
    f1 = NoExists;
  else {
    f1 = modf( (double)Land, &f2 );
    if ( f1 < 0 )
      f1 = fabs( f1 );
/*    if ( f1 < 1.0 )
    	f1 += 1.0;  */
    else
      f1 += 1.0;
  }
  if ( Takeoff == NoExists )
    f2 = NoExists;
  else {
    f2 = modf( (double)Takeoff, &f3 );
    if ( f2 < 0 )
      f2 = fabs( f2 );
/*    if ( f2 < 1.0 )
    	f2 += 1.0;*/
    else
      f2 += 1.0;
  }


  //��� ���室� �६���
  TDateTime diff = getDiff( dst_offset, isSummer( first_day ), isSummer( vd ) );
  f1 += diff;
  f2 += diff;


  ProgTrace( TRACE5, "f=%f,l=%f, f1=%f, f2=%f", f, l, f1, f2 );
	ProgTrace( TRACE5, "filter.firsttime=%s, filter.lasttime=%s, Land=%s, Takeoff=%s", 
	           DateTimeToStr( f, "dd.mm hh:nn" ).c_str(),
	           DateTimeToStr( l, "dd.mm hh:nn" ).c_str(),
	           DateTimeToStr( f1, "dd.mm hh:nn" ).c_str(),
	           DateTimeToStr( f2, "dd.mm hh:nn" ).c_str());
  
  return ( f1 >= f && f1 <= l || f2 >= f && f2 <= l );
}

bool TFilter::isFilteredTime( TDateTime first_day, TDateTime Land, TDateTime Takeoff, int dst_offset, string vregion )
{


/*, DateTimeToStr( BoostToDateTime( periods[ season_idx ].period.begin() ) + 10, "dd.mm.yy hh:nn" ).c_str()  */

  return isFilteredTime( BoostToDateTime( periods[ season_idx ].period.begin() ) + 1,
                         first_day, Land, Takeoff, dst_offset, vregion );
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
    int day = StrToInt( days[ i ] ) + delta;
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

/* ���� 䨫��� � UTC, �६��� ⮦� � UTC, �᪫�祭��, �����
 (int)TReqInfo::Instance()->user.access.airps.size() != 1 */
void TFilter::Parse( xmlNodePtr filterNode )
{
  TBaseTable &baseairps = base_tables.get( "airps" );	
  /* ���砫� ������塞 �� 㬮�砭��, � ��⮬ ��९��뢠�� ⥬, �� ��諮 � ������ */
  Clear();
  GetSeason();
ProgTrace( TRACE5, "range.first=%f, range.last=%f, range.days=%s",
           range.first, range.last, range.days.c_str() );
  if ( !filterNode ) {
    ProgTrace( TRACE5, "filter parse: season_idx=%d,range.first=%s,range.last=%s,airp=%s,city=%s,time.first=%s,time.last=%s, company=%s, triptype=%s",
               season_idx, DateTimeToStr( range.first, "dd.mm.yy hh:nn" ).c_str(), DateTimeToStr( range.last, "dd.mm.yy hh:nn" ).c_str(),
               airp.c_str(),city.c_str(),DateTimeToStr( firstTime, "dd.mm.yy hh:nn" ).c_str(),
               DateTimeToStr( lastTime, "dd.mm.yy hh:nn" ).c_str(), company.c_str(), triptype.c_str() );
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
        range.days = AddDays( range.days, (int)f - (int)range.first );
      }
    }
  }
  else { /* �������� �� �����, � �ᯮ��㥬 �� 㬮�砭�� UTC */
    range.first = BoostToDateTime( periods[ season_idx ].period.begin() );
    range.last = BoostToDateTime( periods[ season_idx ].period.end() );
    range.days = AllDays;
  }
ProgTrace( TRACE5, "range.first=%f, range.last=%f, range.days=%s",
           range.first, range.last, range.days.c_str() );
//  ClearNotUsedDays( range.first, range.last, range.days );
  node = GetNode( "airp", filterNode );
  if ( node )
    airp = NodeAsString( node );
  node = GetNode( "city", filterNode );
  if ( node )
    city = NodeAsString( node );
  string sairpcity = city;
  if ( !airp.empty() ) {
    /* �஢�ઠ �� ᮢ������� ��த� � ��ய��⮬ */
    sairpcity = ((TAirpsRow&)baseairps.get_row( "code", airp )).city; 
    if ( !city.empty() && sairpcity != city )
      throw UserException( "������� ��� ��ய��� �� �ਭ������� ��������� ���� ��த�" );
  }
  node = GetNode( "time", filterNode );
  if ( node ) {
      /* �㤥� ��ॢ����� � UTC �⭮�⥫쭮 ���� � ������� !!!! */
      firstTime = NodeAsDateTime( "first", node );
      lastTime = NodeAsDateTime( "last", node );
  }
  node = GetNode( "company", filterNode );
  if ( node )
    company = NodeAsString( node );
  node = GetNode( "triptype", filterNode );
  if ( node )
    triptype = NodeAsString( node );
  ProgTrace( TRACE5, "filter parse: season_idx=%d,range.first=%s,range.last=%s,airp=%s,city=%s,time.first=%s,time.last=%s, company=%s, triptype=%s",
             season_idx, DateTimeToStr( range.first, "dd.mm.yy hh:nn" ).c_str(), DateTimeToStr( range.last, "dd.mm.yy hh:nn" ).c_str(),
             airp.c_str(),city.c_str(),DateTimeToStr( firstTime, "dd.mm.yy hh:nn" ).c_str(),
             DateTimeToStr( lastTime, "dd.mm.yy hh:nn" ).c_str(), company.c_str(), triptype.c_str() );
}

/* ����� �� 㦥 � �������� �६���� */
void TFilter::Build( xmlNodePtr filterNode )
{
  tz_database &tz_db = get_tz_database();
  time_zone_ptr tz = tz_db.time_zone_from_region( region );
  NewTextChild( filterNode, "season_idx", SEASON_PRIOR_PERIOD - 1 );
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
    TReqInfo::Instance()->MsgToLog("�������� ३� ", evtSeason, NodeAsInteger("trip_id", reqNode));
  showMessage( "���� 㤠���");
}


void CreateSPP( BASIC::TDateTime localdate )
{
  map<int,TTimeDiff> v;
  TQuery MIDQry(&OraSession);
  MIDQry.SQLText =
   "BEGIN "\
   " SELECT move_id.nextval INTO :move_id from dual; "\
   " INSERT INTO move_ref(move_id,reference)  SELECT :move_id, NULL FROM dual; "\
   "END;";
  MIDQry.DeclareVariable( "move_id", otInteger );
  /* ����室��� ᤥ���� �஢��� �� �� ����⢮����� ३� */
  TQuery VQry(&OraSession);
  VQry.SQLText =
   "SELECT COUNT(*) c FROM points "\
   " WHERE airline||flt_no||suffix||trip_type=:name AND "\
   "       ( TRUNC(scd_in)=TRUNC(:scd_in) OR TRUNC(scd_out)=TRUNC(:scd_out) )";
  VQry.DeclareVariable( "name", otString );
  VQry.DeclareVariable( "scd_in", otDate );
  VQry.DeclareVariable( "scd_out", otDate );

  TQuery PRREG(&OraSession);
  PRREG.SQLText = "SELECT code FROM trip_types WHERE pr_reg=1";
  PRREG.Execute();
  vector<string> triptypes;
  while ( !PRREG.Eof ) {
    triptypes.push_back( PRREG.FieldAsString( "code" ) );
    PRREG.Next();
  }

  TQuery PQry(&OraSession);
  PQry.SQLText =
   "BEGIN "\
   " SELECT point_id.nextval INTO :point_id FROM dual; "\
   " INSERT INTO points(point_id,move_id,point_num,airp,pr_tranzit,first_point,airline,flt_no,suffix,"\
   "                    craft,scd_in,scd_out,trip_type,litera,pr_del,tid,pr_reg) "\
   " SELECT :point_id,:move_id,:point_num,:airp,:pr_tranzit,:first_point,:airline,"\
   "        :flt_no,:suffix,:craft,:scd_in,:scd_out,:trip_type,:litera,:pr_del,tid__seq.nextval,:pr_reg FROM dual; "\
   "END;";
  PQry.DeclareVariable( "point_id", otInteger );
  PQry.DeclareVariable( "move_id", otInteger );
  PQry.DeclareVariable( "point_num", otInteger );
  PQry.DeclareVariable( "airp", otString );
  PQry.DeclareVariable( "pr_tranzit", otInteger );
  PQry.DeclareVariable( "first_point", otInteger );
  PQry.DeclareVariable( "airline", otString );
  PQry.DeclareVariable( "flt_no", otInteger );
  PQry.DeclareVariable( "suffix", otString );
  PQry.DeclareVariable( "craft", otString );
  PQry.DeclareVariable( "scd_in", otDate );
  PQry.DeclareVariable( "scd_out", otDate );
  PQry.DeclareVariable( "trip_type", otString );
  PQry.DeclareVariable( "litera", otString );
  PQry.DeclareVariable( "pr_del", otInteger );
  PQry.DeclareVariable( "pr_reg", otInteger );

  TQuery TQry(&OraSession);
  TQry.SQLText =
   "BEGIN "
   " INSERT INTO trip_sets(point_id,f,c,y,max_commerce,overload_alarm,pr_etstatus,pr_stat, "
   "    pr_tranz_reg,pr_check_load,pr_overload_reg,pr_exam,pr_check_pay,pr_trfer_reg) "
   "  VALUES(:point_id,:f,:c,:y, NULL, 0, 0, 0, "
   "    NULL, 0, 1, 0, 0, 0); "
   " ckin.set_trip_sets(:point_id); "
   " gtimer.puttrip_stages(:point_id); "
   "END;";
  TQry.DeclareVariable( "point_id", otInteger );
  TQry.DeclareVariable( "f", otInteger );
  TQry.DeclareVariable( "c", otInteger );
  TQry.DeclareVariable( "y", otInteger );
  vector<TStageTimes> stagetimes;
  TSpp spp;
  createSPP( localdate, spp, stagetimes, false );

  for ( TSpp::iterator sp=spp.begin(); sp!=spp.end(); sp++ ) {
    tmapds &mapds = sp->second;
    for ( map<int,TDestList>::iterator im=mapds.begin(); im!=mapds.end(); im++ ) {
      MIDQry.Execute();
      int move_id = MIDQry.GetVariableAsInteger( "move_id" );
      TDests::iterator p = im->second.dests.end();
      int point_id,first_point;

      PQry.SetVariable( "move_id", move_id );

      /* �஢�ઠ �� �� ����⢮����� */
      bool exists = false;
      for ( TDests::iterator d=im->second.dests.begin(); d!=im->second.dests.end() - 1; d++ ) {
        VQry.SetVariable( "name", d->company + IntToString( d->trip ) + d->suffix + d->triptype );
        if ( d->Land > NoExists )
          VQry.SetVariable( "scd_in", d->Land );
        else
          VQry.SetVariable( "scd_in", FNull );
        if ( d->Takeoff > NoExists )
          VQry.SetVariable( "scd_out", d->Takeoff );
        else
          VQry.SetVariable( "scd_out", FNull );
        VQry.Execute();
        if ( VQry.FieldAsInteger( "c" ) ) {
          exists = true;
          break;
        }
      }
      if ( exists )
        continue;

      bool pr_tranzit;
      for ( TDests::iterator d=im->second.dests.begin(); d!=im->second.dests.end(); d++ ) {
        PQry.SetVariable( "point_num", d->num );
        PQry.SetVariable( "airp", d->cod );

        pr_tranzit=( d != im->second.dests.begin() ) &&
                   ( p->company + IntToString( p->trip ) + p->suffix + p->triptype ==
                     d->company + IntToString( d->trip ) + d->suffix + d->triptype );


        PQry.SetVariable( "pr_tranzit", pr_tranzit );

        if (d != im->second.dests.begin() )
          PQry.SetVariable( "first_point", first_point );
        else
          PQry.SetVariable( "first_point",FNull );

        if ( d->company.empty() )
          PQry.SetVariable( "airline", FNull );
        else
          PQry.SetVariable( "airline", d->company );
        if ( d->trip == NoExists )
          PQry.SetVariable( "flt_no", FNull );
        else
          PQry.SetVariable( "flt_no", d->trip );
        if ( d->suffix.empty() )
          PQry.SetVariable( "suffix", FNull );
        else
          PQry.SetVariable( "suffix", d->suffix );
        if ( d->bc.empty() )
          PQry.SetVariable( "craft", FNull );
        else
          PQry.SetVariable( "craft", d->bc );
        if ( d->Land == NoExists )
          PQry.SetVariable( "scd_in", FNull );
        else
          PQry.SetVariable( "scd_in", d->Land );
        if ( d->Takeoff == NoExists )
          PQry.SetVariable( "scd_out", FNull );
        else
          PQry.SetVariable( "scd_out", d->Takeoff );
        if ( d->triptype.empty() )
          PQry.SetVariable( "trip_type", FNull );
        else
          PQry.SetVariable( "trip_type", d->triptype );
        if ( d->litera.empty() )
          PQry.SetVariable( "litera", FNull );
        else
          PQry.SetVariable( "litera", d->litera );
        PQry.SetVariable( "pr_del", d->pr_cancel );

        int pr_reg = ( d->Takeoff > NoExists &&
                       find( triptypes.begin(), triptypes.end(), d->triptype ) != triptypes.end() &&
                       d->pr_cancel == 0 && d != im->second.dests.end() - 1 );
        if ( pr_reg ) {
          TDests::iterator r=d;
          r++;
          for ( ;r!=im->second.dests.end(); r++ ) {
            if ( !r->pr_cancel )
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
        p = d;
      }
    }
  }
  TReqInfo::Instance()->MsgToLog( string( "����祭�� ��� �� " ) + DateTimeToStr( localdate, "dd.mm.yy" ), evtSeason );
}

void SeasonInterface::GetSPP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
//TReqInfo::Instance()->user.check_access( amWrite );
  TDateTime localdate;
  modf( (double)NodeAsDateTime( "date", reqNode ), &localdate );
  CreateSPP( localdate );
  showMessage("����� �ᯥ譮 ��࠭���");
}

bool insert_points( double da, int move_id, TFilter &filter, TDateTime first_day, int offset,
                    TDateTime vd, TDestList &ds )
{
  ProgTrace( TRACE5, "first_day=%s", DateTimeToStr( first_day, "dd.mm.yy hh:nn" ).c_str() );

  TReqInfo *reqInfo = TReqInfo::Instance();
  bool canUseAirline, canUseAirp; /* ����� �� �ᯮ�짮���� ����� ३� */
/*  if ( reqInfo->user.user_type == utSupport ) {*/
    /* �� �ࠢ� - �� ३�� ����㯭� �᫨ �� 㪠���� ������� �� � ��*/
/*    canUseAirline = reqInfo->user.access.airlines.empty();
    canUseAirp = reqInfo->user.access.airps.empty();
  }
  else {
    canUseAirline = ( reqInfo->user.user_type == utAirport && reqInfo->user.access.airlines.empty() );
    canUseAirp = ( reqInfo->user.user_type == utAirline && reqInfo->user.access.airps.empty() );
  }*/
  canUseAirline = false; // new
  canUseAirp = false; //new
  // ����� move_id, vd �� ��ਮ� �믮������
  // ����稬 ������� � �஢�ਬ �� �ࠢ� ����㯠 � �⮬� ��������
  TQuery Qry(&OraSession);
  Qry.SQLText = 
  " SELECT num, routes.cod, land-TRUNC(land)+:vdate+delta_in land, company, trip, bc, "
  "        takeoff-TRUNC(takeoff)+:vdate+delta_out takeoff, triptype, litera, "
  "        airps.city city, pr_cancel, f, c, y, suffix "
  "  FROM routes, airps "
  " WHERE routes.move_id=:vmove_id AND routes.cod=airps.code  "
  " ORDER BY move_id,num";

  Qry.CreateVariable( "vdate", otDate, vd );
  Qry.CreateVariable( "vmove_id", otInteger, move_id );
  Qry.Execute();
  bool candests = false;
  ds.cancel = true;
  double f1;
  while ( !Qry.Eof ) {
    TDest d;
    d.num = Qry.FieldAsInteger( "num" );
    d.cod = Qry.FieldAsString( "cod" );
    d.city = Qry.FieldAsString( "city" );
    d.pr_cancel = Qry.FieldAsInteger( "pr_cancel" );
    if ( !d.pr_cancel )
      ds.cancel = false;
    if ( Qry.FieldIsNULL( "land" ) )
      d.Land = NoExists;
    else {
      d.Land = Qry.FieldAsDateTime( "land" );
      modf( (double)d.Land, &f1 );
      if ( f1 == da ) {
        candests = candests || filter.isFilteredUTCTime( da, first_day, d.Land, offset );
      	ProgTrace( TRACE5, "filter.firsttime=%s, filter.lasttime=%s, d,land=%s	, res=%d",
      	           DateTimeToStr( filter.firstTime, "dd hh:nn" ).c_str(),
      	           DateTimeToStr( filter.lastTime, "dd hh:nn" ).c_str(),
      	           DateTimeToStr( d.Land, "dd hh:nn" ).c_str(), candests );
      }
    }
    d.company = Qry.FieldAsString( "company" );

    d.region = AirpTZRegion( Qry.FieldAsString( "cod" ), false );
    if ( Qry.FieldIsNULL( "trip" ) )
      d.trip = NoExists;
    else
    d.trip = Qry.FieldAsInteger( "trip" );
    d.bc = Qry.FieldAsString( "bc" );
    d.litera = Qry.FieldAsString( "litera" );
    d.triptype = Qry.FieldAsString( "triptype" );
    if ( Qry.FieldIsNULL( "takeoff" ) )
      d.Takeoff = NoExists;
    else {
      d.Takeoff = Qry.FieldAsDateTime( "takeoff" );
      modf( (double)d.Takeoff, &f1 );
      if ( f1 == da ) {
        candests = candests || filter.isFilteredUTCTime( da, first_day, d.Takeoff, offset );
      	ProgTrace( TRACE5, "filter.firsttime=%s, filter.lasttime=%s, d,takeoff=%s, res=%d",
      	           DateTimeToStr( filter.firstTime, "dd hh:nn" ).c_str(),
      	           DateTimeToStr( filter.lastTime, "dd hh:nn" ).c_str(),
      	           DateTimeToStr( d.Takeoff, "dd hh:nn" ).c_str(), candests );
      }
    }
    d.f = Qry.FieldAsInteger( "f" );
    d.c = Qry.FieldAsInteger( "c" );
    d.y = Qry.FieldAsInteger( "y" );
    d.suffix = Qry.FieldAsString( "suffix" );
/*    if ( !canUseAirp && // �᫨ �� �� ����� �।�⠢�⥫� ��ய��� 
         find( reqInfo->user.access.airps.begin(),
               reqInfo->user.access.airps.end(),
               d.cod
             ) != reqInfo->user.access.airps.end() ) { // ��⠥��� ���� � ���� � �ࠢ�� ��ᬮ�� ���� 
      canUseAirp = true;
    }
    if ( !canUseAirline &&
         find( reqInfo->user.access.airlines.begin(),
               reqInfo->user.access.airlines.end(),
               d.company
             ) != reqInfo->user.access.airlines.end() )
      canUseAirline = true;*/
    if ( reqInfo->CheckAirp( d.cod ) ) // new
    	canUseAirp = true; //new
    if ( reqInfo->CheckAirline( d.company ) ) //new
    	canUseAirline = true; //new
    // 䨫��� �� �६���� �ਫ��/�뫥� � ������ �.�.
    ds.dests.push_back( d );
    Qry.Next();
  } // end while
  if ( !canUseAirline || !canUseAirp || !candests ) {
    ds.dests.clear();
    ProgTrace( TRACE5, "clear dests move_id=%d, date=%s", move_id, DateTimeToStr( da, "dd.hh:nn" ).c_str() );
  }
  return !ds.dests.empty();
}

// �६��� � 䨫��� �࠭���� � UTC!!!
void createTrips( TDateTime utc_spp_date, TDateTime localdate, TFilter &filter, int offset,
                  vector<TStageTimes> &stagetimes, TDestList &ds )
{
  TDateTime firstTime = filter.firstTime;
  TDateTime lastTime = filter.lastTime;

  TReqInfo *reqInfo = TReqInfo::Instance();


  if ( reqInfo->user.user_type != utAirport ) {
    filter.firstTime = NoExists;
    createAirlineTrip( NoExists, filter, offset, ds, localdate );
  }
  else {
    for ( vector<string>::iterator s=reqInfo->user.access.airps.begin();
          s!=reqInfo->user.access.airps.end(); s++ ) {
      int vcount = (int)ds.trips.size();
      // ᮧ���� ३�� �⭮�⥫쭮 ࠧ�襭��� ���⮢ reqInfo->user.access.airps
      
      createAirportTrip( *s, NoExists, filter, offset, ds, utc_spp_date, false, true ); 
      for ( int i=vcount; i<(int)ds.trips.size(); i++ ) {
        ds.trips[ i ].trap = NoExists;
        if ( ds.trips[ i ].takeoff > NoExists ) {
          for ( vector<TStageTimes>::iterator st=stagetimes.begin(); st!=stagetimes.end(); st++ ) {
          	if ( ( st->airp == *s || st->airp.empty() ) &&
          		   ( st->craft == ds.trips[ i ].ownbc || st->craft.empty() ) &&
          		   ( st->trip_type == ds.trips[ i ].triptype || st->trip_type.empty() ) ) {
              ds.trips[ i ].trap = ds.trips[ i ].takeoff - (double)st->time/1440.0;
              break;
            }
          }
        }
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
  Qry.SQLText = "SELECT region FROM tz_regions WHERE tz=:ptz AND country='��' AND pr_del=0";
  Qry.CreateVariable( "ptz", otInteger, ptz );
  Qry.Execute();
  if ( !Qry.Eof ) {
    res = Qry.FieldAsString( "region" );
    mapreg[ ptz ] = res;
  }
  return res;
}

void createSPP( TDateTime localdate, TSpp &spp, vector<TStageTimes> &stagetimes, bool createViewer )
{
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
  // ��� ��砫� ���� ������� ᯨ᮪ ��ਮ���, ����� �믮������� � ��� ����, ���� ��� ��� �६���
  //!!!
  Qry.SQLText = 
  " SELECT DISTINCT move_id,first_day,last_day,:vd-delta AS qdate,pr_cancel,d.tz tz "
  "  FROM "\
  "  ( SELECT routes.move_id as move_id,"
  "           TO_NUMBER(delta_in) as delta,"
  "           sched_days.pr_cancel as pr_cancel,"
  "           first_day,last_day,tz "
  " FROM sched_days,routes "
  " WHERE routes.move_id = sched_days.move_id AND "
  "       TRUNC(first_day) + delta_in <= :vd AND "
  "       TRUNC(last_day) + delta_in >= :vd AND  "
  "       INSTR( days, TO_CHAR( :vd - delta_in, 'D' ) ) != 0 "
  "   UNION "
  " SELECT routes.move_id as move_id, "
  "        TO_NUMBER(delta_out) as delta,"
  "        sched_days.pr_cancel as pr_cancel,"
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
        // 横� �� ����祭�� ��⠬
         for ( vector<TDateTime>::iterator vd=days.begin(); vd!=days.end(); vd++ ) {

           int offset = GetTZOffSet( first_day, ptz, v );

           ProgTrace( TRACE5, "day=%s, move_id=%d",
                      DateTimeToStr( *vd, "dd.mm.yy hh:nn" ).c_str(),
                      vmove_id );

           if ( insert_points( d, vmove_id, filter, first_day, offset,
                                 *vd, ds ) ) { // ����� �ࠢ� � ������⮬ ࠡ���� + 䨫��� �� �६����
              ds.flight_time = first_day;
              ds.last_day = last_day;
              ds.tz = ptz;
              ds.region = pregion;

             ProgTrace( TRACE5, "canspp trip vmove_id=%d,vd=%s,d=%s spp[ *vd ][ vold_move_id ].trips.size()=%d",
                        vmove_id,
                        DateTimeToStr( *vd, "dd.mm.yy hh:nn" ).c_str(),
                        DateTimeToStr( d, "dd.mm.yy hh:nn" ).c_str(),
                        (int)spp[ *vd ][ vmove_id ].trips.size() );
             if ( createViewer )
               if ( spp[ *vd ][ vmove_id ].trips.empty() ) {
  
                 createTrips( d, localdate, filter, offset, stagetimes, ds );

                 ProgTrace( TRACE5, "ds.trips.size()=%d", (int)ds.trips.size() );
               }
               else
                 ds.trips = spp[ *vd ][ vmove_id ].trips;
               spp[ *vd ][ vmove_id ] = ds;
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
	if ( t1.takeoff > NoExists )
		f1 = modf( t1.takeoff, &d );
	else
		f1 = modf( t1.land, &d );
	if ( t2.takeoff > NoExists )
		f2 = modf( t2.takeoff, &d );
	else
		f2 = modf( t2.land, &d );
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
  vector<TStageTimes> stagetimes;
  TReqInfo *reqInfo = TReqInfo::Instance();
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  if ( reqInfo->user.user_type == utAirport  ) {
    NewTextChild( dataNode, "mode", "port" );
    GetStageTimes( stagetimes, sRemovalGangWay );
  }
  else {
    NewTextChild( dataNode, "mode", "airline" );
  }
  TSpp spp;
  vector<trip> ViewTrips;
  map<int,TTimeDiff> v;
  TDateTime vdate;
  modf( (double)NodeAsDateTime( "date", reqNode ), &vdate );
  createSPP( vdate, spp, stagetimes, true );
  for ( TSpp::iterator sp=spp.begin(); sp!=spp.end(); sp++ ) {
    tmapds &mapds = sp->second;
    for ( map<int,TDestList>::iterator im=mapds.begin(); im!=mapds.end(); im++ ) {
      ProgTrace( TRACE5, "build xml vdate=%s, move_id=%d, trips.size()=%d",
                 DateTimeToStr( sp->first, "dd.mm.yy" ).c_str(),
                 im->first,
                 (int)im->second.trips.size() );
      for ( vector<trip>::iterator tr=im->second.trips.begin(); tr!=im->second.trips.end(); tr++ ) {
        TDateTime diffTime = GetTZTimeDiff( sp->first, im->second.flight_time, im->second.last_day, im->second.tz, v );
      	if ( tr->land > NoExists )
      	  tr->land += diffTime;
      	if ( tr->takeoff > NoExists )
      		tr->takeoff += diffTime;

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
    NewTextChild( tripNode, "print_name", tr->print_name );
    if ( reqInfo->user.user_type != utAirport  )
      NewTextChild( tripNode, "craft", tr->crafts );
    else {
      NewTextChild( tripNode, "craft", tr->ownbc );
      NewTextChild( tripNode, "ownport", tr->ownport );
    }
    NewTextChild( tripNode, "triptype", tr->triptype );
    if ( reqInfo->user.user_type == utAirport  ) {
      /* only for prior version */
  	  string ports_in, ports_out;
     	for ( vector<TDest>::iterator h=tr->vecportsFrom.begin(); h!=tr->vecportsFrom.end(); h++ ) {
         if ( !ports_in.empty() )
           ports_in += "/";
         ports_in += h->cod;    			
  	  }
  	  for ( vector<TDest>::iterator h=tr->vecportsTo.begin(); h!=tr->vecportsTo.end(); h++ ) {
         if ( !ports_out.empty() )
           ports_out += "/";
         ports_out += h->cod;    			
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
   		NewTextChild( tripNode, "ports_out", tr->portsForAirline ); /* �祭� ��㤭� �����뢠���� �� ����, ���⮬� ⠪ */
   	if ( !tr->vecportsFrom.empty() ) {
   	  xmlNodePtr psNode = NewTextChild( tripNode, "portsFrom" );
   	  for ( vector<TDest>::iterator h=tr->vecportsFrom.begin(); h!=tr->vecportsFrom.end(); h++ ) {
   	  	xmlNodePtr pNode = NewTextChild( psNode, "port" );
   	  	NewTextChild( pNode, "airp", h->cod );
   	  	if ( h->Land > NoExists )
   	  	  NewTextChild( pNode, "land", DateTimeToStr( h->Land ) );
   	  	if ( h->Takeoff > NoExists )
   	  		NewTextChild( pNode, "takeoff", DateTimeToStr( h->Takeoff ) );
   	  	if( h->pr_cancel )
   	  		NewTextChild( pNode, "pr_cancel", h->pr_cancel );   	  		
      }
    }
   	if ( !tr->vecportsTo.empty() ) {
   	  xmlNodePtr psNode = NewTextChild( tripNode, "portsTo" );
   	  for ( vector<TDest>::iterator h=tr->vecportsTo.begin(); h!=tr->vecportsTo.end(); h++ ) {
   	  	xmlNodePtr pNode = NewTextChild( psNode, "port" );
   	  	NewTextChild( pNode, "airp", h->cod );
   	  	if ( h->Land > NoExists )
   	  	  NewTextChild( pNode, "land", DateTimeToStr( h->Land ) );
   	  	if ( h->Takeoff > NoExists )
   	  		NewTextChild( pNode, "takeoff", DateTimeToStr( h->Takeoff ) );
   	  	if( h->pr_cancel )
   	  		NewTextChild( pNode, "pr_cancel", h->pr_cancel );
      }
    } 
   	/* end new version */
   	
    if ( !tr->bold_ports.empty() )
      NewTextChild( tripNode, "bold_ports", tr->bold_ports );
    if ( tr->land > NoExists )
      NewTextChild( tripNode, "land", DateTimeToStr( tr->land ) );
    if ( tr->takeoff > NoExists ) {
      NewTextChild( tripNode, "takeoff", DateTimeToStr( tr->takeoff ) );
      NewTextChild( tripNode, "trap", DateTimeToStr( tr->trap ) );
    }
    if ( tr->cancel )
      NewTextChild( tripNode, "ref", "�⬥��" );
  }
}

void VerifyRangeList( TRangeList &rangeList, map<int,TDestList> &mapds )
{
  vector<string> flg;
  // �஢�ઠ �������
  for ( map<int,TDestList>::iterator im=mapds.begin(); im!=mapds.end(); im++ ) {
ProgTrace( TRACE5, "(int)im->second.dests.size()=%d", (int)im->second.dests.size() );
    if ( (int)im->second.dests.size() < 2 )
      throw UserException( "������� ������ ᮤ�ঠ�� �� ����� ���� �㭪⮢ ��ᠤ��" );
    im->second.dests.begin()->Land = NoExists;
    TDests::iterator enddest = im->second.dests.end() - 1;
    enddest->company.clear();
    enddest->trip = NoExists;
    enddest->bc.clear();
    enddest->f = 0;
    enddest->c = 0;
    enddest->y = 0;
    enddest->Takeoff = NoExists;
    enddest->litera.clear();
    enddest->triptype.clear();
    enddest->unitrip.clear();
    enddest->suffix.clear();
    string fcompany = im->second.dests.begin()->company;
    bool notime = true;
    int notcancel = 0;
    flg.clear();
    TDests::iterator pid;
    for ( TDests::iterator id=im->second.dests.begin(); id!=im->second.dests.end(); id++ ) {
ProgTrace( TRACE5, "id->company=%s", id->company.c_str() );
      if ( id->cod.empty() )
        throw UserException( "�� ����� �㭪� ��ᠤ��" );

      if ( id < enddest ) {
        if ( id->company.empty() )
          throw UserException( "�� ������ ������������" );
        if ( fcompany != id->company )
          throw UserException( "������� �� ����� �ਭ�������� ࠧ�� ������������" );
        if ( id->trip == NoExists )
          throw UserException( "�� ����� ����� ३�" );
        if ( id->bc.empty() )
          throw UserException( "�� ����� ⨯ ��" );
        string f = IntToString( id->trip ) + id->cod;
        if ( find( flg.begin(), flg.end(), f ) != flg.end() )
          throw UserException( "� ������� �������騥 �㭪�� � ��������� ����஬ ३�" );
        else
          flg.push_back( f );
      }
      if ( id != im->second.dests.begin() && id->cod == pid->cod )
        throw UserException( "������� �� ����� ᮤ�ঠ�� ��� ���������� ����� ����� �.�." );
      if ( !id->pr_cancel )
        notcancel++;
ProgTrace( TRACE5, "cod=%s, land=%f, takeoff=%f", id->cod.c_str(), id->Land, id->Takeoff );
      if ( id->Land > NoExists || id->Takeoff > NoExists )
        notime = false;
      pid = id;
    } /* end for */
    if ( notime )
      throw UserException( "�६��� � ������� �� ������" );
    /* ���� ३� �⬥���, �.�. ����⨫��� �� ����� ������ �� �⬥������� �㭪� ��ᠤ�� */
    if ( notcancel <= 1 ) {
     for ( TDests::iterator id=im->second.dests.begin(); id!=im->second.dests.end(); id++ ) {
       id->pr_cancel = true;
     }
     im->second.cancel = true;
    }
    else
      im->second.cancel = false;
  }
  // �஢�ઠ ���������� + ���⠢����� �⬥������ ��ਮ���
  for ( vector<TPeriod>::iterator ip=rangeList.periods.begin(); ip!=rangeList.periods.end(); ip++) {
    map<int,TDestList>::iterator im = mapds.find( ip->move_id );
    if ( im == mapds.end() || im->second.dests.empty( ) )
      throw UserException( "��� ��������� �� ����� �������" );
    ip->cancel = im->second.cancel;
    if ( ip->first > ip->last ) {
    	string errstr = "��砫쭠� ��� ��������� �믮������ ���� ����筮� ";
    	errstr +=DateTimeToStr( ip->first, "dd.mm.yy" );
    	errstr += "-";
    	errstr +=DateTimeToStr( ip->last, "dd.mm.yy" );
    	throw UserException( errstr );
    }
  }
}

// ࠧ��� � ��ॢ�� �६�� � UTC, � ���������� �믮������ �࠭���� �६��� �뫥�
bool ParseRangeList( xmlNodePtr rangelistNode, TRangeList &rangeList, map<int,TDestList> &mapds, string &filter_region )
{
  TBaseTable &baseairps = base_tables.get( "airps" );		
  TReqInfo *reqInfo = TReqInfo::Instance();
  bool canUseAirline, canUseAirp; /* ����� �� �ᯮ�짮���� ����� ३� */
/*  if ( reqInfo->user.user_type == utSupport ) {
   // �� �ࠢ� - �� ३�� ����㯭� �᫨ �� 㪠���� ������� �� � ��
    canUseAirline = reqInfo->user.access.airlines.empty();
    canUseAirp = reqInfo->user.access.airps.empty();
  }
  else {
    canUseAirline = ( reqInfo->user.user_type == utAirport && reqInfo->user.access.airlines.empty() );
    canUseAirp = ( reqInfo->user.user_type == utAirline && reqInfo->user.access.airps.empty() );
  } new*/
  canUseAirline = false; // new
  canUseAirp = false; //new
  mapds.clear();
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
        dest.cod = NodeAsStringFast( "cod", curNode );
        dest.city = ((TAirpsRow&)baseairps.get_row( "code", dest.cod )).city; 
        dest.region = CityTZRegion( dest.city );
        node = GetNodeFast( "cancel", curNode );
        if ( node )
          dest.pr_cancel = NodeAsInteger( node );
        else
          dest.pr_cancel = 0;
        node = GetNodeFast( "land", curNode );
        if ( node ) {
          dest.Land = NodeAsDateTime( node );
          modf( (double)dest.Land, &f2 );
          if ( ds.flight_time == NoExists && f2 == 0 ) {
            ds.flight_time = dest.Land;
            ds.region = dest.region;
          }
        }
        else
          dest.Land = NoExists;
        node = GetNodeFast( "company", curNode );
        if ( node ) {
          dest.company = NodeAsString( node );
          ProgTrace( TRACE5, "dest.company=%s, period.move_id=%d", dest.company.c_str(), period.move_id );
        }
        node = GetNodeFast( "trip", curNode );
        if ( node )
          dest.trip = NodeAsInteger( node );
        else
          dest.trip = NoExists;
        node = GetNodeFast( "bc", curNode );
        if ( node )
          dest.bc = NodeAsString( node );
        node = GetNodeFast( "litera", curNode );
        if ( node )
          dest.litera = NodeAsString( node );
        node = GetNodeFast( "triptype", curNode );
        if ( node )
          dest.triptype = NodeAsString( node );
        else
          dest.triptype = "�";
        node = GetNodeFast( "takeoff", curNode );
        if ( node ) {
        	dest.Takeoff = NodeAsDateTime( node );
        	modf( (double)dest.Takeoff, &f2 );
        	if ( ds.flight_time == NoExists && f2 == 0 ) {
              ds.flight_time = dest.Takeoff;
              ds.region = dest.region;
          }
        }
        else
          dest.Takeoff = NoExists;
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
        if ( node )
          dest.suffix = NodeAsString( node );

/*        if ( !canUseAirp && // �᫨ �� �� ����� �।�⠢�⥫� ��ய��� 
           find( reqInfo->user.access.airps.begin(),
                 reqInfo->user.access.airps.end(),
                 dest.cod
                ) != reqInfo->user.access.airps.end() ) { // ��⠥��� ���� � ���� � �ࠢ�� ��ᬮ�� ���� 
          canUseAirp = true;
        }
        if ( !canUseAirline &&
             find( reqInfo->user.access.airlines.begin(),
                   reqInfo->user.access.airlines.end(),
                   dest.company
                  ) != reqInfo->user.access.airlines.end() )
          canUseAirline = true;*/
        if ( reqInfo->CheckAirp( dest.cod ) ) // new
        	canUseAirp = true; //new
        if ( reqInfo->CheckAirline( dest.company ) ) //new
    	    canUseAirline = true; //new         
        ds.dests.push_back( dest );
        destNode = destNode->next;
      } // while ( destNode )
      if ( !canUseAirline || !canUseAirp )
        throw UserException( "�������筮 �ࠢ. ����� � ���ଠ樨 ����������" );
//      ProgTrace( TRACE5, "first_dest=%d", ds.first_dest );
      mapds.insert(std::make_pair( period.move_id, ds ) );
    } // if ( node )
    // ��ਮ�� �࠭��� �६� �뫥� �� �.�. ��ॢ���� � UTC
    ds = mapds[ period.move_id ];
    if ( ds.dests.empty() )
      throw UserException( "��� ��ਮ�� �� ����� �������" );
    if ( ds.flight_time == NoExists )
      throw UserException( "�६��� � ������� ������ � ᤢ���� �� ���" );
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
    	period.first = ClientToUTC( (double)period.first + 1, filter_region ) - 1; //!!!djek
/*    	if ( !ambiguous_timeNode ) {
    	  showErrorMessage( string("�६� �믮������ ३� �� ��।����� �������筮 ") + DateTimeToStr( period.first, "dd.mm hh:nn" ).c_str() );
     	  return fale;   	 
     	}
     	else {
     		period.first = ClientToUTC( (double)period.first + 1, filter_region ) - 1; //!!!djek
     	}*/
/*      throw UserException( "�६� �믮������ ३� �� ��।����� �������筮 %s",
                            DateTimeToStr( period.first, "dd.mm hh:nn" ).c_str() );!!!*/
    }
    catch( boost::local_time::time_label_invalid ) {
      throw UserException( "�६� �믮������ ३� �� ������� %s",
                           DateTimeToStr( period.first, "dd.mm hh:nn" ).c_str() );
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
      // ��ॢ�� �६�� � ������� � �������
      for ( TDests::iterator id=ds.dests.begin(); id!=ds.dests.end(); id++ ) {
      	if ( id->Land > NoExists ) {
      		f2 = modf( (double)id->Land, &f3 );
      		f3 += first_day + fabs( f2 );
          ProgTrace( TRACE5, "local land=%s",DateTimeToStr( f3, "dd.mm.yyyy hh:nn:ss" ).c_str() );
          
      		try {
      	    f2 = modf( (double)ClientToUTC( f3, id->region ), &f3 );
      	  }
          catch( boost::local_time::ambiguous_result ) {
          	f2 = modf( (double)ClientToUTC( f3 + 1, id->region ) - 1, &f3 );
/*!!!            throw UserException( "�६� �ਫ�� ३� � �㭪� %s �� ��।����� �������筮 %s",
                                 id->cod.c_str(),
                                 DateTimeToStr( first_day, "dd.mm" ).c_str() );*/
          }
          catch( boost::local_time::time_label_invalid ) {
            throw UserException( "�६� �ਫ�� ३� � �㭪� %s �� ������� %s",
                                 id->cod.c_str(),
                                 DateTimeToStr( period.first, "dd.mm" ).c_str() );
          }
          ProgTrace( TRACE5, "trunc(land)=%s, time=%s",
                     DateTimeToStr( f3, "dd.mm.yyyy hh:nn:ss" ).c_str(),
                     DateTimeToStr( f2, "dd.mm.yyyy hh:nn:ss" ).c_str() );

    	    if ( f3 < utcFirst )
            id->Land = f3 - utcFirst - f2;
          else
            id->Land = f3 - utcFirst + f2;
          ProgTrace( TRACE5, "utc land=%s", DateTimeToStr( id->Land, "dd.mm.yyyy hh:nn:ss" ).c_str() );
        }
    	  if ( id->Takeoff > NoExists ) {
      		f2 = modf( (double)id->Takeoff, &f3 );
      		f3 += first_day + fabs( f2 );
          ProgTrace( TRACE5, "local takeoff=%s",DateTimeToStr( f3, "dd.mm.yyyy hh:nn:ss" ).c_str() );
    	  	try {
    	      f2 = modf( (double)ClientToUTC( f3, id->region ), &f3 );
    	    }
          catch( boost::local_time::ambiguous_result ) {
          	f2 = modf( (double)ClientToUTC( f3 + 1, id->region ) - 1, &f3 );
/*            throw UserException( "�६� �뫥� ३� � �㭪� %s �� ��।����� �������筮 %s",
                                 id->cod.c_str(),
                                 DateTimeToStr( first_day, "dd.mm" ).c_str() );*/
          }
          catch( boost::local_time::time_label_invalid ) {
            throw UserException( "�६� �뫥� ३� � �㭪� %s �� ������� %s",
                                 id->cod.c_str(),
                                 DateTimeToStr( period.first, "dd.mm" ).c_str() );
          }
          ProgTrace( TRACE5, "trunc(takeoff)=%s, time=%s",
                     DateTimeToStr( f3, "dd.mm.yyyy hh:nn:ss" ).c_str(),
                     DateTimeToStr( f2, "dd.mm.yyyy hh:nn:ss" ).c_str() );
    	    if ( f3 < utcFirst )
            id->Takeoff = f3 - utcFirst - f2;
          else
            id->Takeoff = f3 - utcFirst + f2;
          ProgTrace( TRACE5, "utc takeoff=%s",DateTimeToStr( id->Takeoff, "dd.mm.yyyy hh:nn:ss" ).c_str() );
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
  if ( node ) {
    trip_id = NodeAsInteger( node );
    SQry.Clear();
    SQry.SQLText = "SELECT first_day, last_day, days, pr_cancel, tlg, reference "
                   " FROM sched_days "
                   " WHERE trip_id=:trip_id";
    SQry.CreateVariable( "trip_id", otInteger, trip_id );
    SQry.Execute();
    while ( !SQry.Eof ) {
    	TPeriod p;
    	p.first = SQry.FieldAsDateTime( "first_day" );
    	p.last = SQry.FieldAsDateTime( "last_day" );
    	p.days = SQry.FieldAsString( "days" );
    	p.cancel = SQry.FieldAsInteger( "pr_cancel" );
    	p.tlg = SQry.FieldAsString( "tlg" );
    	p.ref = SQry.FieldAsString( "reference" );
    	oldperiods.push_back( p );
    	SQry.Next();
    }
    TDateTime begin_date_season = BoostToDateTime( filter.periods.begin()->period.begin() );
    // ⥯��� ����� 㤠���� �� ��ਮ��, ���.
    //!!! �訡�� �.�. ��ਮ�� ��������� �⭮�⥫쭮 ॣ���� ��ࢮ�� �.�. � ���. delta=0
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
  }
  else {
    // �� ���� ३�
    ProgTrace( TRACE5, "it is new trip" );
    SQry.SQLText = "SELECT routes_trip_id.nextval AS trip_id FROM dual";
    SQry.Execute();
    trip_id = SQry.FieldAsInteger(0);
    ProgTrace( TRACE5, "new trip_id=%d", trip_id );
    TReqInfo::Instance()->MsgToLog( "���� ������ ३� ", evtSeason, trip_id );
  }
  // �� ��������� 㦥 � UTC
  // �஡����� �� �ᥬ ����祭�� � ������ � ������뢠�� �� �� �� �� ��

  /* ���砫� ������塞 ����������� */
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

   ProgTrace( TRACE5, "result first=%s, last=%s, days=%s, move_id=%d",
              DateTimeToStr( yp->first,"dd.mm.yy hh:nn:ss" ).c_str(),
              DateTimeToStr( yp->last,"dd.mm.yy hh:nn:ss" ).c_str(),
              yp->days.c_str(),
              yp->move_id );
  }

  // ⥯��� �������� �।� ��ਮ��� ����, � ����� 㤠����
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
  "INSERT INTO sched_days(trip_id,move_id,num,first_day,last_day,days,pr_cancel,tlg,reference,tz) "
  "VALUES(:trip_id,:move_id,:num,:first_day,:last_day,:days,:pr_cancel,:tlg,:reference,:tz) ";
  SQry.DeclareVariable( "trip_id", otInteger );
  SQry.DeclareVariable( "move_id", otInteger );
  SQry.DeclareVariable( "num", otInteger );
  SQry.DeclareVariable( "first_day", otDate );
  SQry.DeclareVariable( "last_day", otDate );
  SQry.DeclareVariable( "days", otString );
  SQry.DeclareVariable( "pr_cancel", otInteger );
  SQry.DeclareVariable( "tlg", otString );
  SQry.DeclareVariable( "reference", otString );
  SQry.CreateVariable( "tz", otInteger, filter.tz );
  RQry.Clear();
  RQry.SQLText =
  "INSERT INTO routes(move_id,num,cod,pr_cancel,land,company,trip,bc,takeoff,litera, "
  "                   triptype,f,c,y,unitrip,delta_in,delta_out,suffix) "
  " VALUES(:move_id,:num,:cod,:pr_cancel,:land, :company,:trip,:bc,:takeoff,:litera, "
  "        :triptype,:f,:c,:y,:unitrip,:delta_in,:delta_out,:suffix) ";
  RQry.DeclareVariable( "move_id", otInteger );
  RQry.DeclareVariable( "num", otInteger );
  RQry.DeclareVariable( "cod", otString );
  RQry.DeclareVariable( "pr_cancel", otInteger );
  RQry.DeclareVariable( "land", otDate );
  RQry.DeclareVariable( "company", otString );
  RQry.DeclareVariable( "trip", otInteger );
  RQry.DeclareVariable( "bc", otString );
  RQry.DeclareVariable( "takeoff", otDate );
  RQry.DeclareVariable( "litera", otString );
  RQry.DeclareVariable( "triptype", otString );
  RQry.DeclareVariable( "f", otInteger );
  RQry.DeclareVariable( "c", otInteger );
  RQry.DeclareVariable( "y", otInteger );
  RQry.DeclareVariable( "unitrip", otString );
  RQry.DeclareVariable( "delta_in", otInteger );
  RQry.DeclareVariable( "delta_out", otInteger );
  RQry.DeclareVariable( "suffix", otString );
  int num = 0;
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
          tst();
          yp->move_id = new_move_id;
          yp->modify = fchange;
        }
      }

    } // end finsert
    else {
      new_move_id = ip->move_id;
      ProgTrace( TRACE5, "ip move_id=%d", new_move_id );
    }
    // �஢��塞 ���� �� �� �ࠢ��� �뢮�� (���室�) �६�� � ������� � �����뢠���� ��ਮ��
tst();
   // ����室��� �������� �ࠢ��� ���室�
   // ᬮ�ਬ � ����� �ᯨᠭ�� ����� ⥪�騩 ��ਮ�.
   time_period r( DateTimeToBoost( ip->first ), DateTimeToBoost( ip->last ) );
   // �஡�� �� �ᯨᠭ��
   bool inSeason = false;
   int hours;
   for ( vector<TSeason>::iterator p=filter.periods.begin(); p!=filter.periods.end(); p++ ) {
     if ( !p->period.intersects( r ) ) // �� ���ᥪ�����
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
     throw UserException( "����祭�� ��ਮ� �믮������ ३� %s-%s %s �� �ਭ������� �ᯨᠭ��",
                          DateTimeToStr( ip->first, "dd.mm.yy" ).c_str(),
                          DateTimeToStr( ip->last, "dd.mm.yy" ).c_str(),
                          ip->days.c_str() );
tst();
    SQry.SetVariable( "trip_id", trip_id );
    SQry.SetVariable( "move_id", new_move_id );
    SQry.SetVariable( "num", num );
//    SQry.SetVariable( "first_dest", ip->first_dest );
    SQry.SetVariable( "first_day", ip->first );
    SQry.SetVariable( "last_day", ip->last );
    SQry.SetVariable( "days", ip->days );
    SQry.SetVariable( "pr_cancel", ip->cancel );
    SQry.SetVariable( "tlg", ip->tlg );
    SQry.SetVariable( "reference", ip->ref );
    vector<TPeriod>::iterator ew = oldperiods.end();
    for ( ew=oldperiods.begin(); ew!=oldperiods.end(); ew++ ) {
    	if ( ew->first == ip->first && ew->last == ip->last )
    		break;
    }
    if ( ew == oldperiods.end() ) {
      log = "���� ������ ��ਮ�� " +
            DateTimeToStr( ip->first, "dd.mm.yy" ) +
            "-" +
            DateTimeToStr( ip->last, "dd.mm.yy" ) + " " +
            ip->days;
      if ( ip->cancel )
    	  log += " ��.";
    }
    else {
    	log.clear();
    	if ( ip->days != ew->days )
    		log += " ��� " + ip->days;
      if ( ew->cancel != ip->cancel )
      	log += " ��.";
    	if ( ip->tlg != ew->tlg )
    		log += " ��⪮ " + ip->tlg;
    	if ( ip->ref != ew->ref )
    		log += " ���筨�� " + ip->ref;
    	if ( !log.empty() )
    	  log = "��������� ��ਮ�� " +
    	        DateTimeToStr( ip->first, "dd.mm.yy" ) +
    	        "-" +
              DateTimeToStr( ip->last, "dd.mm.yy" ) + log;
    	ProgTrace( TRACE5, "log=%s", log.c_str() );
    	ew->modify = fdelete;
    }
    if ( !log.empty() )
      TReqInfo::Instance()->MsgToLog( log, evtSeason, trip_id, new_move_id );
    SQry.Execute()  ;
    num++;
    TDestList ds = mapds[ ip->move_id ];
ProgTrace( TRACE5, "ds.dests.size=%d", (int)ds.dests.size() );
    int dnum = 0;
    double fl, ff;
    if ( !ds.dests.empty() ) {
    	log.clear();
      for ( TDests::iterator id=ds.dests.begin(); id!=ds.dests.end(); id++ ) {
        RQry.SetVariable( "move_id", new_move_id );
        RQry.SetVariable( "num", dnum );
        RQry.SetVariable( "cod", id->cod );
        if ( !log.empty() )
        	log += "-";
        else
        	log = "�������: ";
        RQry.SetVariable( "pr_cancel", id->pr_cancel );
        if ( id->Land > NoExists ) {
          RQry.SetVariable( "land", modf( (double)id->Land, &fl ) ); // 㤠�塞 delta_in
          log += DateTimeToStr( id->Land, "hh:nn(UTC)" );

        }
        else {
          fl = 0.0;
          RQry.SetVariable( "land", FNull );
        }
        log += id->cod;
        if ( id->company.empty() )
          RQry.SetVariable( "company", FNull );
        else
          RQry.SetVariable( "company", id->company );
        if ( id->trip > NoExists )
          RQry.SetVariable( "trip", id->trip );
        else
          RQry.SetVariable( "trip", FNull );
        if ( id->bc.empty() )
          RQry.SetVariable( "bc", FNull );
        else
          RQry.SetVariable( "bc", id->bc );
        if ( id->Takeoff > NoExists ) {
          RQry.SetVariable( "takeoff", modf( (double)id->Takeoff, &ff ) ); // 㤠�塞 delta_out
          log += DateTimeToStr( id->Takeoff, "hh:nn(UTC)" );
        }
        else {
          ff = 0.0;
          RQry.SetVariable( "takeoff", FNull );
        }
        if ( id->pr_cancel )
        	log += " ��.";
        if ( id->triptype.empty() )
          RQry.SetVariable( "triptype", FNull );
        else
          RQry.SetVariable( "triptype", id->triptype );
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
        if ( id->Land > NoExists )
          RQry.SetVariable( "delta_in", fl );
        else
          RQry.SetVariable( "delta_in", FNull ); //???
        if ( id->Takeoff > NoExists )
          RQry.SetVariable( "delta_out", ff );
        else
          RQry.SetVariable( "delta_out", FNull ); //???
        if ( id->suffix.empty() )
          RQry.SetVariable( "suffix", FNull );
        else
          RQry.SetVariable( "suffix", id->suffix );
        RQry.Execute();
        tst();
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
      log = "�������� ��ਮ�� " +
            DateTimeToStr( ew->first, "dd.mm.yy" ) +
            "-" +
            DateTimeToStr( ew->last, "dd.mm.yy" ) + " " +
            ew->days;
      if ( ew->cancel )
    	  log += " ��.";
    	TReqInfo::Instance()->MsgToLog( log, evtSeason, trip_id, new_move_id );
  }

  // ���� ������� ���ଠ�� �� ��࠭� ।���஢����
  GetEditData( trip_id, filter, true, dataNode );
  showMessage( "����� �ᯥ譮 ��࠭���" );
}

string GetTrip( TDest *PriorDest, TDest *OwnDest )
{
  string res;
  int flt_no;
  string suffix;
  string company;
  if ( !PriorDest ) {
    flt_no = NoExists;
    tst();
  }
  else {
    flt_no = PriorDest->trip;
    company = PriorDest->company;
    suffix = PriorDest->suffix;
  }
  ProgTrace( TRACE5, "OwnDest->trip=%d", OwnDest->trip );
  if ( OwnDest->trip == NoExists ) /* ३� �� �ਫ�� */
    res = company + IntToString( flt_no ) + suffix;
  else
    if ( flt_no == NoExists ||
         company == OwnDest->company && flt_no == OwnDest->trip && suffix == OwnDest->suffix )
      res = OwnDest->company + IntToString( OwnDest->trip ) + OwnDest->suffix;
    else
      if ( company != OwnDest->company )
        res = company + IntToString( flt_no ) + suffix + "/" +
              OwnDest->company + IntToString( OwnDest->trip ) + OwnDest->suffix;
      else
        res = company + IntToString( flt_no ) + suffix + "/" +
                        IntToString( OwnDest->trip ) + OwnDest->suffix;
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
      case 2: l += '*';
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
    f2 = modf( (double)UTCToClient( f1 + fabs( f3 ), dest_region ), &f3 ); //!!!
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
                        TDateTime utc_spp_date, bool viewOwnPort, bool UTCFilter )
{
  if ( ds.dests.empty() )
    return false;
  bool createTrip = false;
  TDest *OwnDest = NULL;
  TDest *PriorDest = NULL;
  TDest *PDest = NULL;
  TDest *NDest;
  string crafts;
  vector<TDest> vecportsFrom, vecportsTo;
  int i=0;
  ProgTrace( TRACE5, "createAirporttrip trip_id=%d", trip_id );
  do {
    NDest = &ds.dests[ i ];
    if ( crafts.find( NDest->bc ) == string::npos ) {
      if ( !crafts.empty() )
        crafts += "/";
      crafts += NDest->bc;
    }
    if ( OwnDest == NULL && NDest->cod == airp ) {
      PriorDest = PDest;
      OwnDest = NDest;

/*      // Den was here!!!*/
      if ( viewOwnPort ) {
      	TDest d = *NDest;
      	d.Land = TDateTimeToClient( ds.flight_time, d.Land, d.region );
      	d.Takeoff = TDateTimeToClient( ds.flight_time, d.Takeoff, d.region );
        vecportsFrom.push_back( d );
      }  
/*      // end of Den was here*/
    }
    else { /* ��� ���� � ������� �� ���� �⮡ࠦ��� */
//!!!      if ( ports.find( NDest->cod ) == string::npos ) {
        if ( !OwnDest ) {
        	TDest d = *NDest;
      	  ProgTrace( TRACE5, "d.Land=%f", d.Land );        	
      	  d.Land = TDateTimeToClient( ds.flight_time, d.Land, d.region );
        	ProgTrace( TRACE5, "d.Land=%f", d.Land );
      	  d.Takeoff = TDateTimeToClient( ds.flight_time, d.Takeoff, d.region );        	
          vecportsFrom.push_back( d );
        }
        else {
        	TDest d = *NDest;
        	d.Land = TDateTimeToClient( ds.flight_time, d.Land, d.region );
        	d.Takeoff = TDateTimeToClient( ds.flight_time, d.Takeoff, d.region );        	
          vecportsTo.push_back( d );
        }
//      }
      createTrip = ( OwnDest && ( PDest->trip != NDest->trip || PDest->company != NDest->company ) );
    }
    /* ����� �������� ��᪮�쪮 ३ᮢ. */
    if ( createTrip || OwnDest != NULL && NDest == &ds.dests.back() ) {
     /* �᫨ �� �����, � ����砥��� ३�,
       � ���� ���� �६� �ਫ��/�뫥� � ��� ����� �� 㤮���⢮���� �᫮��� 䨫���
       ����� �� �।�⠢�⥫� ������ ���� => ����� ������ �६��� �뫥� � �ਫ�� � ���� */
       // error view filter time normilize
       bool cantrip = false;
       if ( utc_spp_date > NoExists ) {
         double f2;
         if ( OwnDest->Land > NoExists ) {
           modf( (double)OwnDest->Land, &f2 );
           ProgTrace( TRACE5, "land f2=%f, utc_spp_date=%f", f2, utc_spp_date );
           if ( f2 == utc_spp_date )
             cantrip = true;
         }
         if ( !cantrip && OwnDest->Takeoff > NoExists ) {
           modf( (double)OwnDest->Takeoff, &f2 );
           ProgTrace( TRACE5, "takeoff f2=%f, utc_spp_date=%f", f2, utc_spp_date );
           if ( f2 == utc_spp_date )
             cantrip = true;
         }
       }
       else
         cantrip = true;

/*      double land, takeoff, f3;
      if ( OwnDest->Land > NoExists ) {
        land = modf( (double)OwnDest->Land, &f3 );
      }
      else
        land = NoExists;
      if ( OwnDest->Takeoff > NoExists ) {
        takeoff = modf( (double)OwnDest->Takeoff, &f3 );
      }
      else
        takeoff = NoExists;*/

      if ( cantrip && 
      	   ( UTCFilter &&
      	      ( filter.isFilteredUTCTime( utc_spp_date, ds.flight_time, OwnDest->Land, offset ) ||
      	        filter.isFilteredUTCTime( utc_spp_date, ds.flight_time, OwnDest->Takeoff, offset ) ) ||
      	     !UTCFilter && filter.isFilteredTime( ds.flight_time, OwnDest->Land, OwnDest->Takeoff, offset, OwnDest->region ) )
      	 ) {
/*           ( filter.firstTime == NoExists ||
             land >= filter.firstTime && land <= filter.lastTime ||
             takeoff >= filter.firstTime && takeoff <= filter.lastTime ) ) {*/
        /* ३� ���室�� ��� ��� �᫮��� */
        ProgTrace( TRACE5, "createAirporttrip trip_id=%d, OwnDest->Land=%s, OwnDest.takeoff=%s",
                   trip_id,
                   DateTimeToStr( OwnDest->Land, "dd.mm.yy hh:nn" ).c_str(),
                   DateTimeToStr( OwnDest->Takeoff, "dd.mm.yy hh:nn" ).c_str() );
        trip tr;
        tr.trip_id = trip_id;
        tr.name = GetTrip( PriorDest, OwnDest );
        tr.print_name = GetPrintName( PriorDest, OwnDest );
        tr.ownport = airp;
        tr.crafts = crafts;
        tr.vecportsFrom = vecportsFrom;
        tr.vecportsTo = vecportsTo;
        
        if ( OwnDest == NDest ) {
          tr.ownbc = PriorDest->bc;
          tr.triptype = PriorDest->triptype;
        }
        else {
          tr.ownbc = OwnDest->bc;
          tr.triptype = OwnDest->triptype;
        }
        tr.cancel = OwnDest->pr_cancel; //!!! ���ࠢ��쭮 ⠪, ���� ����뢠��
        /* ��ॢ���� �६��� �뫥� �ਫ�� � ������� */ //!!! error tz
        tr.land = TDateTimeToClient( ds.flight_time, OwnDest->Land, OwnDest->region ); 
        tr.takeoff = TDateTimeToClient( ds.flight_time, OwnDest->Takeoff, OwnDest->region ); 

        ds.trips.push_back( tr );
        tst();
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
bool createAirportTrip( int trip_id, TFilter filter, int offset, TDestList &ds, bool viewOwnPort )
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  bool res = false;
  for ( vector<string>::iterator s=reqInfo->user.access.airps.begin();
        s!=reqInfo->user.access.airps.end(); s++ ) {
    res = res || createAirportTrip( *s, trip_id, filter, offset, ds, NoExists, viewOwnPort, false );
  }
  return res;
}

/* UTCTIME */
bool createAirlineTrip( int trip_id, TFilter &filter, int offset, TDestList &ds )
{
  return createAirlineTrip( trip_id, filter, offset, ds, NoExists );
}

/* UTCTIME to client  */
bool createAirlineTrip( int trip_id, TFilter &filter, int offset, TDestList &ds, TDateTime localdate )
{
  if ( ds.dests.empty() )
    return false;
  TDest *PDest = NULL;
  TDest *NDest;
  int i = 0;
  trip tr;
  tr.trip_id = trip_id;
  tr.land = NoExists;
  tr.takeoff = NoExists;
  bool timeKey = filter.firstTime == NoExists;
  bool cancel = true;
  int own_date = 0;
  string ptime;
  string::size_type p = 0, bold_begin = 0, bold_end = 0;
  string str_dests;

  do {
    NDest = &ds.dests[ i ];
    if ( !NDest->pr_cancel )
      cancel = false;
    timeKey = timeKey || filter.isFilteredTime( ds.flight_time, NDest->Land, NDest->Takeoff, offset, NDest->region );
    if ( tr.crafts.find( NDest->bc ) == string::npos ) {
      if ( !tr.crafts.empty() )
        tr.crafts += "/";
      tr.crafts += NDest->bc;
    }
    if ( tr.triptype.find( NDest->triptype ) == string::npos ) {
      if ( !tr.triptype.empty() )
        tr.triptype += "/";
      tr.triptype += NDest->triptype;
    }
    //!!!if ( tr.ports.find( NDest->cod ) == string::npos ) {
      if ( !tr.portsForAirline.empty() ) {
         tr.portsForAirline += "/";
         str_dests += "/";
      }

      double first_day, f2, f3, utc_date_land, utc_date_takeoff;
      modf( (double)ds.flight_time, &first_day );

      if ( localdate > NoExists && NDest->Land > NoExists ) {
          ProgTrace( TRACE5, "cod=%s,localdate=%s, utcland=%s, first_day=%s",
                     NDest->cod.c_str(),
                     DateTimeToStr( localdate, "dd.mm.yy hh:nn" ).c_str(),
                     DateTimeToStr( NDest->Land, "dd.mm.yy hh:nn" ).c_str(),
                     DateTimeToStr( first_day, "dd.mm.yy hh:nn" ).c_str() );

        double f2;
        TDateTime land;

        f3 = modf( (double)NDest->Land, &utc_date_land );
        f2 = modf( (double)UTCToClient( first_day + fabs( f3 ), NDest->region ), &f3 ); //!!!
        // ����砥� �६�
        if ( f3 < first_day )
          land = f3 - first_day - f2;
        else
          land = f3 - first_day + f2;
        ProgTrace( TRACE5, "localdate=%s, utc_date_land=%s, f3=%f, first_day=%f",
                   DateTimeToStr( localdate, "dd hh:nn" ).c_str(),
                   DateTimeToStr( utc_date_land, "dd hh:nn" ).c_str(),
                   f3, first_day );

        if ( utc_date_land + f3 - first_day == localdate ) {
          ProgTrace( TRACE5, "localdate=%s, localland=%s",
                     DateTimeToStr( localdate, "dd hh:nn" ).c_str(),
                     DateTimeToStr( land, "dd hh:nn" ).c_str() );
          ptime = DateTimeToStr( land, "(hh:nn)" );
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
      tr.portsForAirline += NDest->cod;
      tr.vecportsTo.push_back( *NDest ); // �㦭� ��� �뢮�� �� 1 ��࠭ � ᥧ����
      str_dests += NDest->cod;
      if ( localdate > NoExists && NDest->Takeoff > NoExists ) {
          ProgTrace( TRACE5, "cod=%s,localdate=%s, utctakeoff=%s",
                     NDest->cod.c_str(),DateTimeToStr( localdate, "dd hh:nn" ).c_str(),
                     DateTimeToStr( NDest->Takeoff, "dd hh:nn" ).c_str() );
        TDateTime takeoff;
        f3 = modf( (double)NDest->Takeoff, &utc_date_takeoff );
        f2 = modf( (double)UTCToClient( first_day + fabs( f3 ), NDest->region ), &f3 ); //!!!
        if ( f3 < first_day )
          takeoff = f3 - first_day - f2;
        else
          takeoff = f3 - first_day + f2;
        if ( utc_date_takeoff + f3 - first_day == localdate ) {
          ProgTrace( TRACE5, "localdate=%s, localtakeoff=%s",
                     DateTimeToStr( localdate, "dd hh:nn" ).c_str(),
                     DateTimeToStr( takeoff, "dd hh:nn" ).c_str() );

          ptime = DateTimeToStr( takeoff, "(hh:nn)" );
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
      if  ( !PDest || PDest->company != NDest->company && !NDest->company.empty() ||
            PDest->trip != NDest->trip ||
            PDest->suffix != NDest->suffix ) {
        if ( !PDest )
         tr.name = NDest->company + IntToString( NDest->trip ) + NDest->suffix;
        else {
          if ( PDest->company != NDest->company )
            tr.name += string( "/" ) + NDest->company + IntToString( NDest->trip ) + NDest->suffix;
          else
            tr.name += string( "/" ) + IntToString( NDest->trip ) + NDest->suffix;
        }
      }
    }
    else
      if ( NDest == &ds.dests.back() && tr.name.empty() ) {
        tr.name = PDest->company + IntToString( PDest->trip ) + PDest->suffix;
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
    tr.cancel = cancel;
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
    f = UTCToLocal( first, region );
    l = UTCToLocal( last, region );
  }
  catch( Exception &e ) {
  	if ( errtz == NoExists )
  		errtz = ptz;
  	return false;
  }
  int m = (int)f - (int)first;
  first = f;
  last = l;
  /* ��� ᤢ��� ��� 㪠���� �� ��� */
  if ( !m || days == AllDays )
   return true;
  /* ᤢ�� ���� �ந��襫 � � ��� �� �� ��� �믮������ */
  days = AddDays( days, m );
  ProgTrace( TRACE5, "ConvertPeriodToLocal have move range" );
  return true;
}

void GetDests( map<int,TDestList> &mapds, const TFilter &filter, int vmove_id )
{
  ProgTrace( TRACE5, "GetDests vmove_id=%d", vmove_id );
  TPerfTimer tm;
  tm.Init();
  TReqInfo *reqInfo = TReqInfo::Instance();
  TQuery RQry( &OraSession );
  string sql =
  "SELECT move_id,num,routes.cod cod,airps.city city, "
  "       pr_cancel,land+delta_in land,company,"
  "       trip,bc,litera,triptype,takeoff+delta_out takeoff,f,c,y,unitrip,suffix "
  " FROM routes, airps "
  "WHERE ";
  if ( vmove_id > NoExists ) {
    RQry.CreateVariable( "move_id", otInteger, vmove_id );
    sql += "move_id=:move_id AND ";
  }
  sql += "routes.cod=airps.code ORDER BY move_id,num";

  RQry.SQLText = sql;
  RQry.Execute();
  int idx_rmove_id = RQry.FieldIndex("move_id");
  int idx_num = RQry.FieldIndex("num");
  int idx_cod = RQry.FieldIndex("cod");
  int idx_city = RQry.FieldIndex("city");
  int idx_rcancel = RQry.FieldIndex("pr_cancel");
  int idx_land = RQry.FieldIndex("land");
  int idx_company = RQry.FieldIndex("company");
  int idx_trip = RQry.FieldIndex("trip");
  int idx_bc = RQry.FieldIndex("bc");
  int idx_takeoff = RQry.FieldIndex("takeoff");
  int idx_litera = RQry.FieldIndex("litera");
  int idx_triptype = RQry.FieldIndex("triptype");
  int idx_f = RQry.FieldIndex("f");
  int idx_c = RQry.FieldIndex("c");
  int idx_y = RQry.FieldIndex("y");
  int idx_unitrip = RQry.FieldIndex("unitrip");
  int idx_suffix = RQry.FieldIndex("suffix");

  int move_id = NoExists;
  TDestList ds;
  TDest d;
  bool canUseAirline, canUseAirp; /* ����� �� �ᯮ�짮���� ����� */
  bool compKey, cityKey, airpKey, triptypeKey, timeKey;
tst();
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

        compKey = filter.company.empty();
        cityKey = filter.city.empty();
        airpKey = filter.airp.empty();
        triptypeKey = filter.triptype.empty();
        timeKey = filter.firstTime == NoExists;
//      }
      move_id = RQry.FieldAsInteger( idx_rmove_id );
/*      if ( reqInfo->user.user_type == utSupport ) {
      	// �� �ࠢ� - �� ३�� ����㯭� �᫨ �� 㪠���� ������� �� � ��
      	canUseAirline = reqInfo->user.access.airlines.empty();
      	canUseAirp = reqInfo->user.access.airps.empty();
      }
      else {
        canUseAirline = ( reqInfo->user.user_type == utAirport && reqInfo->user.access.airlines.empty() );
        canUseAirp = ( reqInfo->user.user_type == utAirline && reqInfo->user.access.airps.empty() );
      }new*/
      canUseAirline = false; // new
      canUseAirp = false; //new      
    }
    d.num = RQry.FieldAsInteger( idx_num );
    d.cod = RQry.FieldAsString( idx_cod );
    airpKey = airpKey || d.cod == filter.airp;
    d.company = RQry.FieldAsString( idx_company );
    compKey = compKey  || d.company == filter.company;
/*    if ( !canUseAirp && // �᫨ �� �� ����� �।�⠢�⥫� ��ய��� 
         find( reqInfo->user.access.airps.begin(),
               reqInfo->user.access.airps.end(),
               d.cod
              ) != reqInfo->user.access.airps.end() ) { // ��⠥��� ���� � ���� � �ࠢ�� ��ᬮ�� ���� 
      canUseAirp = true;
    }
    if ( !canUseAirline &&
         find( reqInfo->user.access.airlines.begin(),
               reqInfo->user.access.airlines.end(),
               d.company
             ) != reqInfo->user.access.airlines.end() )
      canUseAirline = true;new*/
    if ( reqInfo->CheckAirp( d.cod ) ) // new
     	canUseAirp = true; //new
    if ( reqInfo->CheckAirline( d.company ) ) //new
 	    canUseAirline = true; //new               
    d.city = RQry.FieldAsString( idx_city );
    cityKey = cityKey || d.city == filter.city;
    d.region = AirpTZRegion( RQry.FieldAsString( idx_cod ), false );
    d.pr_cancel = RQry.FieldAsInteger( idx_rcancel );
    if ( RQry.FieldIsNULL( idx_land ) )
      d.Land = NoExists;
    else {
      d.Land = RQry.FieldAsDateTime( idx_land );
      modf( (double)d.Land, &f1 );
    }
    if ( RQry.FieldIsNULL( idx_trip ) )
      d.trip = NoExists;
    else
      d.trip = RQry.FieldAsInteger( idx_trip );
    d.bc = RQry.FieldAsString( idx_bc );
    d.litera = RQry.FieldAsString( idx_litera );
    d.triptype = RQry.FieldAsString( idx_triptype );
    triptypeKey = triptypeKey || d.triptype == filter.triptype;
    if ( RQry.FieldIsNULL( idx_takeoff ) )
      d.Takeoff = NoExists;
    else {
      d.Takeoff = RQry.FieldAsDateTime( idx_takeoff );
    }
    d.f = RQry.FieldAsInteger( idx_f );
    d.c = RQry.FieldAsInteger( idx_c );
    d.y = RQry.FieldAsInteger( idx_y );
    d.unitrip = RQry.FieldAsString( idx_unitrip );
    d.suffix = RQry.FieldAsString( idx_suffix );
    //!!!! ���ࠢ��쭮 䨫��஢��� �� �६��� UTC && LOCAL - ������ �� ����ன�� ����
  /* 䨫��� �� �६��� ࠡ�⠥� � �����:
     1. �।�⠢�⥫� ���� (��. ����) 䨫��� �� �६��� �ਫ��/�뫥� �� �⮬� �����
        �᫨ ����� � 䨫�� ����, � �롮� ३ᮢ � ������ ����砥��� ��� ���� � �������
     ����
        ������ �� �६��� �� �ਫ��/�뫥� �� �����, ���. ����� � 䨫���
        �᫨ � 䨫��� �� ����� ���� � ���� 䨫��஢��� �� �६���, � �஢�ઠ �६�� �� �ᥬ� �������� */
    if ( reqInfo->user.user_type == utAirport ) {
      /* ����� �� �।�⠢�⥫� ������ ���� => ����� ������ �६��� �뫥� � �ਫ�� � ����
         �஢�ઠ �� �६��� �㤥� �ந�������� �����।�⢥��� � ��楤�� �ନ஢���� ३�,
         �.�. � ������� ����� ���� ����� ������ ३� �⭮�⥫쭮 ��襣� ���� */
       timeKey = timeKey ||
                 find( reqInfo->user.access.airps.begin(),
                       reqInfo->user.access.airps.end(), d.cod ) != reqInfo->user.access.airps.end();
    }
    else {
      timeKey = true;
      // ��� �ࠫ� 䨫��� �� �६����, �.�. ���� ����� ��砫� �믮������ ३� (ᤢ�� �� �६��� ����/���)
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

TDateTime GetTZTimeDiff( TDateTime utcnow, TDateTime first, TDateTime last, int tz, map<int,TTimeDiff> &v )
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

  int periodDiff = NoExists, seasonDiff = NoExists;

  for ( vector<timeDiff>::iterator i=vt.begin(); i!=vt.end(); i++ ) {
/*    ProgTrace( TRACE5, "i->first=%s, i->last=%s, i->hours=%d",
               DateTimeToStr( i->first, "dd.mm.yy hh:nn" ).c_str(),
               DateTimeToStr( i->last, "dd.mm.yy hh:nn" ).c_str(),
               i->hours );*/
    if ( i->first <= first && i->last >= first ) {
      periodDiff = i->hours;
/*      ProgTrace( TRACE5, "period first=%s, periofDiff=%d",
                 DateTimeToStr( first, "dd.mm.yy hh:nn" ).c_str(),
                 periodDiff );*/
    }
    /* ��� ��ॢ��� �६��� ����室���, �⮡� ⥪�饥 �६� �뫮 ����� ���������,
       ⮣�� � �⮡ࠦ��� ���� ᮮ⢥��⢥�� */
    if ( first <= utcnow && last >= utcnow &&
         i->first <= utcnow && i->last >= utcnow ) {
      seasonDiff = i->hours;
/*      ProgTrace( TRACE5, "seasonDiff=%d", seasonDiff );*/
    }
    if ( periodDiff > NoExists && seasonDiff > NoExists )
      break;
  }
  // �᫨ ��ਮ�� ᮢ����, � �������� ᤢ��� �६��� ��� ��� �� �� ᬮ��� ���� �ࠢ���
  if ( periodDiff == seasonDiff || periodDiff == NoExists ||  seasonDiff == NoExists )
    return 0.0;
  else {
    ProgTrace( TRACE5, "periodDiff - seasonDiff =%d", periodDiff - seasonDiff );
    return (double)( seasonDiff - periodDiff )*3600000/(double)MSecsPerDay;
  }
 /* �������!!! ��������� �������������� ������������ ������� ��� ���������� ���������
   ��ਮ� ���� (3) = 0 ᥣ���� ����(4) = 1 =>  1
   ��ਮ� ����(4) = 1 ᥣ���� ����(3) = -1 => -1

 */

//  ProgError( STDLOG, ">>>> error GetTZTimeDiff not found" );
}

bool ComparePeriod1( TViewPeriod t1, TViewPeriod t2 )
{
	double f;
    if ( !t1.trips.empty() && !t2.trips.empty() ) {
        bool result;
        if ( t1.trips.begin()->takeoff == t2.trips.begin()->takeoff ) {
        	if ( t1.trips.begin()->land != t2.trips.begin()->land )
        		result = modf((double)t1.trips.begin()->land, &f) < modf((double)t2.trips.begin()->land, &f);
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
            result = modf((double)t1.trips.begin()->takeoff, &f) < modf((double)t2.trips.begin()->takeoff, &f);
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
  TDateTime last_date_season = BoostToDateTime( filter.periods.begin()->period.begin() );
  map<int,string> mapreg;
  map<int,TTimeDiff> v;
  map<int,TDestList> mapds;
  TQuery SQry( &OraSession );
  string sql =
  "SELECT trip_id,move_id,first_day,last_day,days,pr_cancel,tlg,tz "
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
  int idx_scancel = SQry.FieldIndex("pr_cancel");
  int idx_tlg = SQry.FieldIndex("tlg");
  int idx_ptz = SQry.FieldIndex("tz");

  if ( !SQry.RowCount() )
    showErrorMessage( "� �ᯨᠭ�� ���������� ३��" );
  // ����� ��� ���� ������� �� �ࠧ� ��������
  GetDests( mapds, filter );
  /* ⥯��� ��३��� � �롮થ � 䨫���樨 ���������� */
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
          tst();
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
      /* �� ࠡ�⠥� � ३ᠬ� � ���. ��� delta = 0 */
      canRange = !ds.dests.empty();
    }
    if ( canRange ) {
        TDateTime first = SQry.FieldAsDateTime( idx_first_day );
        TDateTime last = SQry.FieldAsDateTime( idx_last_day );
        string days = SQry.FieldAsString( idx_days );
        bool pr_cancel = SQry.FieldAsInteger( idx_scancel );
        string tlg = SQry.FieldAsString( idx_tlg );
        TDateTime utc_first = first;
        /* ����稬 �ࠢ��� ���室�(�뢮��) �६�� � ३� */

        int ptz = SQry.FieldAsInteger( idx_ptz );
        string pregion = GetRegionFromTZ( ptz, mapreg );
        /* �த������ 䨫��஢��� */
        time_period p( DateTimeToBoost( first ), DateTimeToBoost( last ) );
        time_period df( DateTimeToBoost( filter.range.first ), DateTimeToBoost( filter.range.last ) );
        /* 䨫��� �� ����������, ��� � �६���� �뫥�, �᫨ ���짮��⥫� ���⮢�� */
  /* !!! ���� �� ��ॢ����� � 䨫��� ��� �믮������ � UTC */
        ds.flight_time = utc_first;
        ds.region = pregion;
        ProgTrace( TRACE5, "move_id=%d, pregion=%s", move_id, pregion.c_str() );
        if ( df.intersects( p ) &&
             /* ��ॢ���� �������� �믮������ � ������� �ଠ� - ����� ���� ᤢ�� */
             ConvertPeriodToLocal( first, last, days, pregion, ptz, errtz ) &&
             CommonDays( days, filter.range.days ) && /* !!! � df.intersects ���� ��ᬮ���� ���� �� ��� �믮������ */
            ( ds.dests.empty() ||
              TReqInfo::Instance()->user.user_type == utAirport &&
              createAirportTrip( viewperiod.trip_id, filter, GetTZOffSet( first, ptz, v ), ds, true ) /*??? isfiltered */ ||
              TReqInfo::Instance()->user.user_type != utAirport &&
      	      createAirlineTrip( viewperiod.trip_id, filter, GetTZOffSet( utc_first, ptz, v ), ds ) ) ) {
          rangeListEmpty = false;
          TDateTime delta_out = NoExists; // ���室 �१ ��⪨ �� �뫥��
          delta_out = 0.0;
          if ( pr_cancel )
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
              ports_in += h->cod;    			
  	        }
  	        for ( vector<TDest>::iterator h=tr->vecportsTo.begin(); h!=tr->vecportsTo.end(); h++ ) {
              if ( !ports_out.empty() )
                ports_out += "/";
              ports_out += h->cod;    			
   	        }    	   
            if ( !ports_in.empty() && !ports_out.empty() )
              vt.ports = ports_in + "/" + ports_out;
            else
              vt.ports = ports_in + ports_out;
            vt.land = tr->land;
            vt.takeoff = tr->takeoff;            
            if ( TReqInfo::Instance()->user.user_type == utAirport && // ⮫쪮 ��� ࠡ�⭨��� ��ய���
            	   vt.takeoff > NoExists ) { // ��� ���� ����㧪� ᫮⮢
            	vt.first = first;
            	vt.last = last;
            	vt.days = days;
            	vt.pr_cancel = pr_cancel;
            }
            viewperiod.trips.push_back( vt );            
          }
          mapds[ move_id ].dests.clear(); /* 㦥 �ᯮ�짮���� ������� */
          mapds[ move_id ].trips.clear();
          ds.dests.clear();
          ds.trips.clear();
        } /* ����� �᫮��� 䨫��� �� ��������� */
    } /* end if canRange */
    tst();
    SQry.Next();
  }
  if ( rangeListEmpty ) {
   ProgTrace( TRACE5, "drop rangelist trip_id=%d, move_id=%d", viewperiod.trip_id,move_id );
   /* 㤠�塞 ���� */
  }
  else {
    if ( viewperiod.trip_id > NoExists ) {
    	viewp.push_back( viewperiod );
    }
  }
 if ( errtz != NoExists )
    showErrorMessage( string("��� ���㤠��⢠ �� � �ᮢ� ���ᮬ ") + IntToString( errtz ) + " �� ����� ॣ���. ������� ३�� �� �⮡ࠦ�����" );
 	
  tst();
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
    	move_id = i->trips.begin()->move_id; // ���� ���� ������� �� �����ᮤ�ঠ�� ��᪮�쪮 ३ᮢ, �� ���� �⮡ࠦ��� 1019, 1020
    for ( vector<TViewTrip>::const_iterator j=i->trips.begin(); j!=i->trips.end() && move_id==j->move_id; j++ ) {
    	if ( !tripsNode )
        tripsNode = NewTextChild( rangeListNode, "trips" );
      xmlNodePtr tripNode = NewTextChild( tripsNode, "trip" );
      NewTextChild( tripNode, "move_id", j->move_id );
      NewTextChild( tripNode, "name", j->name );
      NewTextChild( tripNode, "crafts", j->crafts );
      NewTextChild( tripNode, "ports", j->ports );
      if ( j->land > NoExists ) {
        NewTextChild( tripNode, "land", DateTimeToStr( modf((double)j->land, &f ) ) );
      }
      if ( j->takeoff > NoExists )
        NewTextChild( tripNode, "takeoff", DateTimeToStr( modf((double)j->takeoff, &f ) ) );
    }
  }
}

void buildViewSlots( const vector<TViewPeriod> viewp, xmlNodePtr dataNode )
{
  double f;
  xmlNodePtr tripsNode = NULL;      
  for ( vector<TViewPeriod>::const_iterator i=viewp.begin(); i!=viewp.end(); i++ ) {
    for ( vector<TViewTrip>::const_iterator j=i->trips.begin(); j!=i->trips.end(); j++ ) {
      if ( j->takeoff == NoExists || j->pr_cancel ) // ⮫쪮 �� �뫥�
      	continue;
    	if ( !tripsNode )
        tripsNode = NewTextChild( dataNode, "trips" );
      xmlNodePtr tripNode = NewTextChild( tripsNode, "trip" );
      NewTextChild( tripNode, "move_id", j->move_id );
      NewTextChild( tripNode, "name", j->name );
      NewTextChild( tripNode, "takeoff", DateTimeToStr( modf((double)j->takeoff, &f ) ) );
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
  if ( GetNode( "LoadForm", reqNode ) ) {
    get_report_form("SeasonList", resNode);
    STAT::set_variables(resNode);
    xmlNodePtr variablesNode = GetNode("form_data/variables", resNode);
    NewTextChild(variablesNode, "mode", mode);
  }
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

void GetEditData( int trip_id, TFilter &filter, bool buildRanges, xmlNodePtr dataNode )
{
	int errtz = NoExists;
  TQuery SQry( &OraSession );
  TDateTime begin_date_season = BoostToDateTime( filter.periods.begin()->period.begin() );
  // �롨ࠥ� ��� ।���஢���� �� ��ਮ��, ����� ����� ��� ࠢ�� ⥪�饩 ���
  SQry.SQLText = 
  "SELECT trip_id,move_id,first_day,last_day,days,pr_cancel,tlg,reference,tz "
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
  int idx_cancel = SQry.FieldIndex("pr_cancel");
  int idx_tlg = SQry.FieldIndex("tlg");
  int idx_reference = SQry.FieldIndex( "reference" );
//  int idx_region = SQry.FieldIndex("region");
  int idx_tz = SQry.FieldIndex("tz");

  // ����� ��� ���� ������� �� �ࠧ� ��������
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
  while ( !SQry.Eof ) {
    TDateTime first = SQry.FieldAsDateTime( idx_first_day );
    TDateTime last = SQry.FieldAsDateTime( idx_last_day );
    int ptz = SQry.FieldAsInteger( idx_tz );
//    string pregion = SQry.FieldAsString( idx_region );
    string pregion = GetRegionFromTZ( ptz, mapreg );
    if ( pregion.empty() )
    	throw UserException( string("��� ���㤠��⢠ �� � �ᮢ� ���ᮬ ") + IntToString( errtz ) + " �� ����� ॣ���" );
/*    TDateTime hours = GetTZTimeDiff( NowUTC(), first, last, ptz, v );
    first += hours; //???
    last += hours;*/
    tst();
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
          ProgTrace( TRACE5, "create trip with trip_id=%d, move_id=%d", trip_id, move_id );
          mapds[ move_id ].flight_time = first;
          mapds[ move_id ].tz = ptz;
          mapds[ move_id ].region = pregion;
          tst();
          if ( TReqInfo::Instance()->user.user_type == utAirport )
            canTrips = !createAirportTrip( vtrip_id, filter, GetTZOffSet( first, ptz, v ), mapds[ move_id ], false );
          else
            canTrips = !createAirlineTrip( vtrip_id, filter, GetTZOffSet( first, ptz, v ), mapds[ move_id ] );
/*        } */
      }
      canRange = ( !mapds[ move_id ].dests.empty() && SQry.FieldAsInteger( idx_trip_id ) == trip_id );
    }
    if ( canRange && buildRanges ) {
ProgTrace( TRACE5, "edit canrange move_id=%d", move_id );
      string days = SQry.FieldAsString( idx_days );

      double utcf;
      double f2, f3;
      modf( (double)first, &utcf );


  ProgTrace( TRACE5, "first=%s, last=%s",
             DateTimeToStr( first ).c_str(),
             DateTimeToStr( last ).c_str() );

      double first_day;
      modf( (double)UTCToClient( first, pregion ), &first_day );
      ProgTrace( TRACE5, "local first_day=%s",DateTimeToStr( first_day, "dd.mm.yyyy hh:nn:ss" ).c_str() );

      /* 䨫��� �� ����������, ��� � �६���� �뫥�, �᫨ ���짮��⥫� ���⮢�� */
      /* ��ॢ���� �������� �믮������ � ������� �ଠ� - ����� ���� ᤢ�� */
      if ( ConvertPeriodToLocal( first, last, days, pregion, ptz, errtz ) ) { // ptz
        xmlNodePtr range = NewTextChild( node, "range" );
        NewTextChild( range, "move_id", move_id );
        NewTextChild( range, "first", DateTimeToStr( (int)first ) );
        NewTextChild( range, "last", DateTimeToStr( (int)last ) );
        NewTextChild( range, "days", days );
        if ( SQry.FieldAsInteger( idx_cancel ) )
          NewTextChild( range, "cancel", 1 );
        if ( !SQry.FieldIsNULL( idx_tlg ) )
          NewTextChild( range, "tlg", SQry.FieldAsString( idx_tlg ) );
        if ( !SQry.FieldIsNULL( idx_reference ) )
          NewTextChild( range, "ref", SQry.FieldAsString( idx_reference ) );
/*        if ( SQry.FieldIsNULL( idx_first_dest ) ) {
          ProgError( STDLOG, "first_dest is null, trip_id=%d, move_id=%d", trip_id, move_id );
        }
        else {*/
          /* ��।��� ����� ��� ��࠭� ।���஢���� */
          if ( !mapds[ move_id ].dests.empty() ) {
            xmlNodePtr destsNode = NewTextChild( range, "dests" );
            for ( TDests::iterator id=mapds[ move_id ].dests.begin(); id!=mapds[ move_id ].dests.end(); id++ ) {
              xmlNodePtr destNode = NewTextChild( destsNode, "dest" );
              NewTextChild( destNode, "cod", id->cod );
      	      if ( id->cod != id->city )
                NewTextChild( destNode, "city", id->city );
              if ( id->pr_cancel )
      	        NewTextChild( destNode, "cancel", id->pr_cancel );
              // � �᫨ � �⮬ ����� ��㣨� �ࠢ��� ���室� �� ��⭥�/������ �ᯨᠭ�� ???
              // issummer( TDAteTime, region ) != issummer( utcf, pult.region );
      	      if ( id->Land > NoExists ) {
                f2 = modf( (double)id->Land, &f3 );
    		        f3 += utcf + fabs( f2 );
                ProgTrace( TRACE5, "utc land=%s",DateTimeToStr( f3, "dd.mm.yyyy hh:nn:ss" ).c_str() );
                try {
                  f2 = modf( (double)UTCToClient( f3, id->region ), &f3 );
                }
                catch( Exception &e ) {
                	throw UserException( string("��� ��த� ") + id->city + " �� ����� ॣ���" );
                }
                ProgTrace( TRACE5, "local date land=%s, time land=%s",
                           DateTimeToStr( f3, "dd.mm.yy" ).c_str(),
                           DateTimeToStr( f2, "dd.mm.yy hh:nn" ).c_str() );
                if ( f3 < first_day )
                  id->Land = f3 - first_day - f2;
                else
                  id->Land = f3 - first_day + f2;

      	        NewTextChild( destNode, "land", DateTimeToStr( id->Land ) ); //???
              }
      	      if ( !id->company.empty() )
      	        NewTextChild( destNode, "company", id->company );
      	      if ( id->trip > NoExists )
      	        NewTextChild( destNode, "trip", id->trip );
      	      if ( !id->bc.empty() )
      	        NewTextChild( destNode, "bc", id->bc );
      	      if ( !id->litera.empty() )
                NewTextChild( destNode, "litera", id->litera );
      	      if ( id->triptype != "�" )
      	        NewTextChild( destNode, "triptype", id->triptype );
      	      if ( id->Takeoff > NoExists ) {
                f2 = modf( (double)id->Takeoff, &f3 );
                f3 += utcf + fabs( f2 );
                ProgTrace( TRACE5, "utc takeoff=%s",DateTimeToStr( f3, "dd.mm.yyyy hh:nn:ss" ).c_str() );
                try {
                  f2 = modf( (double)UTCToClient( f3, id->region ), &f3 );
                }
                catch( Exception &e ) {
                	throw UserException( string("��� ��த� ") + id->city + " �� ����� ॣ���" );
                }                
                ProgTrace( TRACE5, "local date takeoff=%s, time takeoff=%s",
                           DateTimeToStr( f3, "dd.mm.yy" ).c_str(),
                           DateTimeToStr( f2, "dd.mm.yy hh:nn" ).c_str() );
                if ( f3 < first_day )
                  id->Takeoff = f3 - first_day - f2;
                else
                  id->Takeoff = f3 - first_day + f2;
      	        NewTextChild( destNode, "takeoff", DateTimeToStr( id->Takeoff ) );
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
      	        NewTextChild( destNode, "suffix", id->suffix );
      	    } // end for
            mapds[ move_id ].dests.clear(); /* 㦥 �ᯮ�짮���� ������� */
          } // end if
/*        } // end else */
      }
    }
    SQry.Next();
  }

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

// ᯨ᮪ ��ਮ��� + �������� � ��ਮ��� + ᯨ᮪ ३ᮢ ��� ����ண� ���室�
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
  GetEditData( trip_id, filter, trip_id > NoExists, dataNode );
  ProgTrace(TRACE5, "getdata %ld", tm.Print());

}

void SeasonInterface::convert(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  TQuery Qry( &OraSession );
  Qry.SQLText =
   "SELECT * FROM drop_sched_days ORDER BY trip_id,move_id,num";
  Qry.Execute();
  tst();

  TQuery DQry( &OraSession );
  DQry.SQLText =
   "SELECT move_id,num,cod,pr_cancel,land+delta_in land,company,trip,bc,litera,triptype,takeoff+delta_out takeoff,f,c,y,unitrip,suffix "\
   " FROM drop_routes WHERE move_id=:move_id ORDER BY num";
  DQry.DeclareVariable( "move_id", otInteger );

  int trip_id = NoExists, move_id = NoExists;
  xmlNodePtr reqn = NewTextChild( resNode, "data" );
  xmlNodePtr node;
  while ( !Qry.Eof ) {
  	if ( trip_id != Qry.FieldAsInteger( "trip_id" ) ) {
  		if ( trip_id > NoExists ) {
  			// ��।����� ⥪�騩 ᥧ��
  			//xmlNodePtr fnode = NewTextChild( reqn, "filter" );
    	  try {
  	      Write( ctxt, reqn, resNode );
        }
        catch( std::exception &E ) {
          ProgError( STDLOG, "Exception: %s", E.what() );
        }
        catch( ... ) {
          ProgError( STDLOG, "Unknown error" );
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
  			tst();
  			while (!DQry.Eof) {
  			 xmlNodePtr d = NewTextChild( dnode, "dest" );
  			 NewTextChild( d, "cod", DQry.FieldAsString( "cod" ) );
  			 NewTextChild( d, "cancel", DQry.FieldAsInteger( "pr_cancel" ) );
  			 if ( !DQry.FieldIsNULL( "land" ) )
  			 	NewTextChild( d, "land", DateTimeToStr( DQry.FieldAsDateTime( "land" ) ) );
  			 if ( !DQry.FieldIsNULL( "company" ) )
  			 	NewTextChild( d, "company", DQry.FieldAsString( "company" ) );
  			 if ( !DQry.FieldIsNULL( "trip" ) )
  			 	NewTextChild( d, "trip", DQry.FieldAsInteger( "trip" ) );
  			 if ( !DQry.FieldIsNULL( "bc" ) )
  			 	NewTextChild( d, "bc", DQry.FieldAsString( "bc" ) );
  			 if ( !DQry.FieldIsNULL( "litera" ) )
  			 	NewTextChild( d, "litera", DQry.FieldAsString( "litera" ) );
  			 if ( !DQry.FieldIsNULL( "triptype" ) )
  			 	NewTextChild( d, "triptype", DQry.FieldAsString( "triptype" ) );
  			 if ( !DQry.FieldIsNULL( "takeoff" ) )
  			 	NewTextChild( d, "takeoff", DateTimeToStr( DQry.FieldAsDateTime( "takeoff" ) ) );
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
    ProgError( STDLOG, "Exception: %s", E.what() );
  }
  catch( ... ) {
    ProgError( STDLOG, "Unknown error" );
  };
 }

}


void SeasonInterface::Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
}



/* �஢�ઠ �६�� �� �ᥬ� �������� */
       /* ��ॢ���� �६��� � 䨫��� �� �६� UTC �⭮�⥫쭮 ��த� � ������� */
/*       if ( !timeKey ) {
       	 if ( !d.region.empty() ) {
           // ��ॢ���� �६� ��砫� �ᯨᠭ�� � UTC
           TDateTime t = BoostToDateTime( filter.periods[ filter.season_idx ].period.begin() );
           // ��ॢ���� �६� �� �६� ������ � ��� ��୮�� 㤠�塞 �६� � ������塞 ����
           double f1,f2,f3;
           modf( t + 10, &f1 );
           TDateTime f,l;
           // ��ॢ���� �६� � UTC
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

           //!!!!!!!! ���� �v������ ⮫쪮 �६�, ��� ��� �᫠ � ���室� ��⮪
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
           if ( d.Land == NoExists )
             f1 = NoExists;
           else {
             f1 = modf( (double)d.Land, &f2 );
             if ( f1 < 0 )
               f1 = fabs( f1 );
             else
              f1 += 1.0;
           }
           if ( d.Takeoff == NoExists )
             f2 = NoExists;
           else {
             f2 = modf( (double)d.Takeoff, &f3 );
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
