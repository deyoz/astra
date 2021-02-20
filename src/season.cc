#include "season.h"
#include "xml_unit.h"
#include "exceptions.h"
#include "oralib.h"
#include "cache.h"
#include "misc.h"
#include "stages.h"
#include "date_time.h"
#include "stl_utils.h"
#include "stat/stat_utils.h"
#include "docs/docs_common.h"
#include "pers_weights.h"
#include "base_tables.h"
#include "astra_misc.h"
#include "flt_binding.h"
#include "sopp.h"
#include "trip_tasks.h"
#include "astra_date_time.h"
#include "PgOraConfig.h"
#include <serverlib/timer.h>
#include <serverlib/dbcpp_cursctl.h>
#include "db_tquery.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"
#include <serverlib/slogger.h>

const int SEASON_PERIOD_COUNT = 5;
const int SEASON_PRIOR_PERIOD = 2;

using namespace BASIC::date_time;
using namespace ASTRA::date_time;
using namespace EXCEPTIONS;
using namespace AstraLocale;
using namespace std;
using namespace boost::local_time;
using namespace boost::posix_time;
using namespace SEASON;


bool createAirportTrip( string airp, int trip_id, TFilter filter, TDestList &ds,
                        TDateTime vdate, bool viewOwnPort, bool UTCFilter, string &err_city );
bool createAirportTrip( int trip_id, TFilter filter, TDestList &ds, bool viewOwnPort, string &err_city );
bool createAirlineTrip( int trip_id, TFilter &filter, TDestList &ds, string &err_city );
bool createAirlineTrip(int trip_id, TFilter &filter, TDestList &ds, TDateTime ldt_SppStart, string &err_city );

void GetDests( map<int,TDestList> &mapds, const TFilter &filter, int move_id = NoExists );
string GetCommonDays( string days1, string days2 );
bool CommonDays( string days1, string days2 );
string DeleteDays( string days1, string days2 );
void ClearNotUsedDays( TDateTime first, TDateTime last, string &days );

void PeriodToUTC( TDateTime &first, TDateTime &last, string &days, const string region );

void internalRead( TFilter &filter, xmlNodePtr dataNode, int trip_id = NoExists );
void GetEditData( int trip_id, TFilter &filter, bool buildRanges, xmlNodePtr dataNode, string &err_city );

void createSPP( TDateTime localdate, TSpp &spp, bool createViewer, string &err_city );

int getMaxNumSchedDays(int trip_id)
{
  int num = 0;
  auto cur = make_db_curs("SELECT MAX(num) num FROM sched_days WHERE trip_id=:trip_id",
                          PgOra::getROSession("SCHED_DAYS"));
  cur.bind(":trip_id", trip_id).defNull(num, 0).EXfet();
  return num;
}

static long getNextRoutesMoveId()
{
  return PgOra::getSeqNextVal("ROUTES_MOVE_ID");
}

static long getNextRoutesTripId()
{
  return PgOra::getSeqNextVal("ROUTES_TRIP_ID");
}

static std::list<int> getMoveIdByTripId(int trip_id, boost::posix_time::ptime begin_date = boost::posix_time::not_a_date_time)
{
  int move_id;
  std::list<int> res;
  auto cur = make_db_curs("SELECT move_id FROM sched_days "
                          "WHERE trip_id = :trip_id AND "
                          "(last_day >= :begin_date_season or :begin_date_season is null)",
                          PgOra::getROSession("SCHED_DAYS"));
  cur.
    bind(":trip_id", trip_id).
    bind(":begin_date_season", begin_date).
    def(move_id).
    exec();
  while(!cur.fen())
    res.push_back(move_id);

  return res;
}

static void deleteSchedDays(int trip_id, boost::posix_time::ptime begin_date = boost::posix_time::not_a_date_time)
{
  auto cur = make_db_curs("DELETE FROM sched_days "
                          "WHERE trip_id=:trip_id "
                          "AND (last_day>=:begin_date_season or :begin_date_season is null)",
                          PgOra::getRWSession("SCHED_DAYS"));
  cur.
    bind(":trip_id", trip_id).
    bind(":begin_date_season", begin_date).
    exec();
  LogTrace(TRACE3) << "delete sched_days by trip_id = "
                   << trip_id << "since: " << begin_date <<
                   " [" << cur.rowcount() << "] rows deleted";
}

static void deleteRoutesByMoveid(const std::list<int> &lmove_id)
{
  for(const int move_id: lmove_id) {
    auto cur = make_db_curs("DELETE FROM routes WHERE move_id = :move_id",
                            PgOra::getRWSession("ROUTES"));
    cur.bind(":move_id", move_id).exec();
    LogTrace(TRACE3) << "delete routes by move_id = "
                     << move_id << " [" << cur.rowcount() << "] rows deleted";
  }
}

bool SsmIdExists(int trip_id)
{
  DB::TQuery QryCheck(PgOra::getROSession("SCHED_DAYS"));
  QryCheck.SQLText = "SELECT ssm_id from sched_days where trip_id = :trip_id";
  QryCheck.CreateVariable("trip_id", otInteger, trip_id);
  QryCheck.Execute();
  return (!QryCheck.Eof && !QryCheck.FieldIsNULL("ssm_id") && QryCheck.FieldAsInteger("ssm_id") != 0);
}

bool LOCK_FLIGHT_SCHEDULE()
{
    static int lock = NoExists;
    if (lock == NoExists)
        lock = getTCLParam("LOCK_FLIGHT_SCHEDULE", 0, 1, 0);

    return lock;
}

void throwOnScheduleLock() {
    if(LOCK_FLIGHT_SCHEDULE())
        throw UserException("MSG.FLIGHT_SCHEDULE.LOCKED");
}

string DefaultTripType( bool pr_lang )
{
    string res = "�";
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
  if ( !PDest || ( NDest->trip > NoExists && abs( PDest->trip - NDest->trip ) <= 1 && PDest->airline == NDest->airline ) ) {
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
  filter_tz_region = TReqInfo::Instance()->desk.tz_region;
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
}

void TFilter::GetSeason()
{
    season prev(Now()), next = prev;
    periods.push_back( TSeason(prev) );

    range.first = prev.begin();
    range.last  = prev.end();
    range.days  = AllDays;
    season_idx  = SEASON_PRIOR_PERIOD;

    for(unsigned i = 0; i < SEASON_PRIOR_PERIOD; ++i) {
        periods.push_back(++next);
        periods.push_front(--prev);
    }
}

//! Seasons
bool TFilter::isSummer( TDateTime pfirst )
{
  ptime r( DateTimeToBoost( pfirst ) );
  for ( deque<TSeason>::iterator p=periods.begin(); p!=periods.end(); p++ ) {
    if ( !p->period.contains( r ) ) // �� ���ᥪ�����
      continue;
    return p->summer;
  }
  return false;
}

inline void setDestsDiffTime( TFilter *filter, TDests &dests, TDateTime f1, TDateTime f2 )
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
            f2 = f1 + utcfirst2 + f3; //! ��७�� ��⮪ ��室���� �� � delta, � � date-��� TDateTime
            f1 += utcfirst1 + f3;
        }

        diff = getDatesOffsetsDiff(f1, f2, filter->filter_tz_region);

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

        diff = getDatesOffsetsDiff(f1, f2, filter->filter_tz_region);

        if ( id->scd_out >= 0 )
            id->scd_out += diff;
        else
            id->scd_out -= diff;
    }
  }
}

inline int calcDestsProp( TFilter *filter, map<int,TDestList> &mapds, int old_move_id, TDateTime f1, TDateTime f2 )
{
  TDestList ds = mapds[ old_move_id ];
  setDestsDiffTime( filter, ds.dests, f1, f2 );
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
            //������� ��뫪� �� ���� � �� �� �������, � ��ਮ�� �ਭ������� ࠧ�� ᥧ���� - ࠧ���� � ��⮢�� ����
            p.modify = finsert;
            p.move_id = calcDestsProp( this, mapds, p.move_id, ip->first, p.first );
        }
ProgTrace( TRACE5, "ip first=%s, last=%s",
           DateTimeToStr( ip->first, "dd.mm.yyyy hh:nn:ss" ).c_str(),
           DateTimeToStr( ip->last, "dd.mm.yyyy hh:nn:ss" ).c_str() );
        time_period r( DateTimeToBoost( ip->first ), DateTimeToBoost( ip->last ) );
        if ( !r.intersects( s ) || !CommonDays( ip->days, p.days ) )
            continue;
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
        diff = getDatesOffsetsDiff(ip->first, p.first, filter_tz_region);

        modf( (double)BoostToDateTime( d.begin() ), &f1 );
        np.first = f1 + modf( (double)ip->first, &f2 ) + diff;
        modf( (double)BoostToDateTime( d.end() ), &f2 );
        np.last = f2 + modf( (double)ip->first, &f1 ) + diff;
        np.days = DeleteDays( ip->days, p.days ); // 㤠�塞 �� p.days ��� ip->days
        ClearNotUsedDays( np.first, np.last, np.days );

        if ( np.days != NoDays ) {
            /* ࠧ���� ��ਮ� - ��� ��᮪ ����� �ਭ�������� ��㣮�� �ᯨᠭ�� */
            if ( diff ) {
                /* ���� ��ᬠ�ਢ��� ����� ��ਮ� ��� �⤥��� � �� ���७�� */
                np.modify = finsert;
                np.move_id = calcDestsProp( this, mapds, np.move_id, ip->first, np.first );
            }
            nperiods.push_back( np );
        }

        if ( !n2.is_null() ) {
            np = *ip;
            diff = getDatesOffsetsDiff(ip->first, p.first, filter_tz_region);

            modf( (double)p.last, &f1 );
            np.first = f1 + 1 + modf( (double)ip->first, &f2 ) + diff;
            modf( (double)ip->last, &f2 );
            np.last = f2 + modf( (double)ip->first, &f1 ) + diff;
            ClearNotUsedDays( np.first, np.last, np.days );


            if ( np.days != NoDays ) {
                /* ࠧ���� ��ਮ� - ��� ��᮪ ����� �ਭ�������� ��㣮�� �ᯨᠭ�� */
                if ( diff ) {
                    /* ���� ��ᬠ�ਢ��� ����� ��ਮ� ��� �⤥��� � �� ���७�� */
                    np.modify = finsert;
                    np.move_id = calcDestsProp( this, mapds, np.move_id, ip->first, np.first );
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
}

bool TFilter::isFilteredUTCTime( TDateTime vd, TDateTime first, TDateTime dest_time )
{
  if ( firstTime == NoExists || filter_tz_region.empty() )
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

  // ���� �뤥���� ⮫쪮 �६�, ��� ��� �᫠ � ���室� ��⮪
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
  TDateTime diff = GetTZTimeDiff( vd, first, filter_tz_region );
  f1 += diff;

  return ( f <= f1 && f1 <= l );
}


bool TFilter::isFilteredTime( TDateTime vd, TDateTime first_day, TDateTime scd_in, TDateTime scd_out,
                              const string &flight_tz_region )
{
    ProgTrace( TRACE5, "In func: %s: filter.firsttime=%s, filter.lasttime=%s, first_day=%s, scd_in=%s, scd_out=%s",
                __FUNCTION__,
               DateTimeToStr( firstTime, "dd.mm hh:nn" ).c_str(),
               DateTimeToStr( lastTime, "dd.mm hh:nn" ).c_str(),
               DateTimeToStr( first_day, "dd.mm hh:nn" ).c_str(),
               DateTimeToStr( scd_in, "dd.mm hh:nn" ).c_str(),
               DateTimeToStr( scd_out, "dd.mm hh:nn" ).c_str());
  if ( firstTime == NoExists || filter_tz_region.empty() )
    return true;
  /* ��ॢ���� �६��� � 䨫��� �� �६� UTC �⭮�⥫쭮 ��த� � ������� */
  // ��ॢ���� �६� ��砫� �ᯨᠭ�� � UTC
  // ��ॢ���� �६� �� �६� ������ � ��� ��୮�� 㤠�塞 �६� � ������塞 ����
  double f1,f2,f3;

  modf( first_day, &f1 );
  TDateTime f,l;
  // ��ॢ���� �६� 䨫��� � UTC
  // normilize date
  try {
    f2 = modf( (double)ClientToUTC( f1 + firstTime, flight_tz_region ), &f3 );
  }
  catch( boost::local_time::ambiguous_result& ) {
    f2 = modf( (double)ClientToUTC( f1 + 1 + firstTime, flight_tz_region ), &f3  );
    f3--;
  }
  catch( boost::local_time::time_label_invalid& ) {
    throw AstraLocale::UserException( "MSG.FLIGHT_BEGINNING_TIME_NOT_EXISTS",
            LParams() << LParam("time", DateTimeToStr( f1 + firstTime, "dd.mm hh:nn" )));
  }

  if ( f3 < f1 )
    f = f3 - f1 - f2;
  else
    f = f3 - f1 + f2;
  try {
    f2 = modf( (double)ClientToUTC( f1 + lastTime, flight_tz_region ), &f3 );
  }
  catch( boost::local_time::ambiguous_result& ) {
    f2 = modf( (double)ClientToUTC( f1 + 1 + lastTime, flight_tz_region ), &f3 );
    f3--;
  }
  catch( boost::local_time::time_label_invalid& ) {
    throw AstraLocale::UserException( "MSG.FLIGHT_ENDING_TIME_NOT_EXISTS",
            LParams() << LParam("time", DateTimeToStr( f1 + lastTime, "dd.mm hh:nn" )));
  }
  if ( f3 < f1 )
    l = f3 - f1 - f2;
  else
    l = f3 - f1 + f2;

  // ���� �v������ ⮫쪮 �६�, ��� ��� �᫠ � ���室� ��⮪
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


  //��� ���室� �६���
  TDateTime diff = GetTZTimeDiff( vd, first_day, filter_tz_region );
  f1 += diff;
  f2 += diff;

  ProgTrace( TRACE5, "f=%f,l=%f, f1=%f, f2=%f", f, l, f1, f2 );

  return ( ( f1 >= f && f1 <= l ) || ( f2 >= f && f2 <= l ) );
}

bool TFilter::isFilteredTime( TDateTime first_day, TDateTime scd_in, TDateTime scd_out, const string &flight_tz_region )
{
  return isFilteredTime( BoostToDateTime( periods[ season_idx ].period.begin() ) + 1,
                         first_day, scd_in, scd_out, flight_tz_region );
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

/* ���� 䨫��� � UTC, �६��� ⮦� � UTC, �᪫�祭��, �����
 (int)TReqInfo::Instance()->user.access.airps.size() != 1 */
void TFilter::Parse( xmlNodePtr filterNode )
{
  TBaseTable &baseairps = base_tables.get( "airps" );
  /* ���砫� ������塞 �� 㬮�砭��, � ��⮬ ��९��뢠�� ⥬, �� ��諮 � ������ */
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
    range.first = LocalToUTC(NodeAsDateTime( "first", node ), filter_tz_region);
    range.last = LocalToUTC(NodeAsDateTime( "last", node ), filter_tz_region);

    node = GetNode( "days", node );
    range.days = NodeAsString( node );
  }
  else { /* �������� �� �����, � �ᯮ��㥬 �� 㬮�砭�� UTC */
    range.first = BoostToDateTime( periods[ season_idx ].period.begin() );
    range.last = BoostToDateTime( periods[ season_idx ].period.end() );
    range.days = AllDays;
  }

  node = GetNode( "airp", filterNode );
  if ( node ) {
    try {
        TElemFmt fmt;
      airp = ElemToElemId( etAirp, NodeAsString( node ), fmt ); // ᪮����⨫ � � ��� ����� � ����
    }
    catch( EConvertError &e ) {
        throw AstraLocale::UserException( "MSG.AIRP.INVALID_GIVEN_CODE" );
    }
  }
  node = GetNode( "city", filterNode );
  if ( node ) {
    try {
        TElemFmt fmt;
      city = ElemToElemId( etCity, NodeAsString( node ), fmt ); // ᪮����⨫ � � ��� ����� � ����
    }
    catch( EConvertError &e ) {
        throw AstraLocale::UserException( "MSG.CITY.INVALID_GIVEN_CODE" );
    }
  }
  string sairpcity = city;
  if ( !airp.empty() ) {
    /* �஢�ઠ �� ᮢ������� ��த� � ��ய��⮬ */
    sairpcity = ((const TAirpsRow&)baseairps.get_row( "code", airp )).city;
    if ( !city.empty() && sairpcity != city )
      throw AstraLocale::UserException( "MSG.GIVEN_AIRP_NOT_BELONGS_TO_GIVEN_CITY" );
  }
  node = GetNode( "time", filterNode );
  if ( node ) {
      /* �㤥� ��ॢ����� � UTC �⭮�⥫쭮 ���� � ������� */
      firstTime = NodeAsDateTime( "first", node );
      lastTime = NodeAsDateTime( "last", node );
  }
  node = GetNode( "company", filterNode );
  if ( node ) {
    try {
        TElemFmt fmt;
      airline = ElemToElemId( etAirline, NodeAsString( node ), fmt ); // ᪮����⨫ � � ��� ����� � ����
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

/* ����� �� 㦥 � �������� �६���� */
void TFilter::Build( xmlNodePtr filterNode )
{
  NewTextChild( filterNode, "season_idx", 0 );
  NewTextChild( filterNode, "season_count", SEASON_PERIOD_COUNT );
  filterNode = NewTextChild( filterNode, "seasons" );
  int i=0;
  for ( deque<TSeason>::iterator p=periods.begin(); p!=periods.end(); p++ ) {
    xmlNodePtr node = NewTextChild( filterNode, "season" );
    NewTextChild( node, "index", IntToString( i - SEASON_PRIOR_PERIOD ) );
    NewTextChild( node, "summer", p->summer );

    NewTextChild( node, "first", DateTimeToStr(UTCToLocal(BoostToDateTime(p->period.begin()), filter_tz_region)));
    NewTextChild( node, "last", DateTimeToStr(UTCToLocal(BoostToDateTime(p->period.end()), filter_tz_region)));

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
  // GRISHA ����� ।���஢���� ��ਮ��, ����祭���� �� SSM ⥫��ࠬ��
  const int trip_id = NodeAsInteger("trip_id", reqNode);
  if (SsmIdExists(trip_id))
  {
    AstraLocale::showError("MSG.SSIM.EDIT_SSM_PERIOD_FORBIDDEN");
    return;
  }
  deleteRoutesByMoveid(getMoveIdByTripId(trip_id));
  deleteSchedDays(trip_id);
  TReqInfo::Instance()->LocaleToLog("EVT.SEASON.DELETE_FLIGHT", evtSeason, trip_id);
  AstraLocale::showMessage( getLocaleText("MSG.FLIGHT_DELETED"));
}

struct CreatorSPPLocker {
   bool islock;
   CreatorSPPLocker() {
     islock = false;
   }

   void lock() {
     LogTrace(TRACE3) << "season_spp::lock";
     if ( islock ) {
       return;
     }
     DB::TQuery Qry(PgOra::getRWSession("SEASON_SPP"));
     Qry.SQLText = "SELECT lock_spp FROM season_spp FOR UPDATE";
     Qry.Execute();
     ASSERT(Qry.RowCount() == 1);
     islock = true;
   }
   void commit() {
      #ifndef XP_TESTING
      ASTRA::commit();
      #endif
      islock = false;
      LogTrace(TRACE3) << "season_spp::commit";
   }
};

void CreateSPP( TDateTime localdate )
{
  throwOnScheduleLock();

  TPersWeights persWeights;
  TQuery MIDQry(&OraSession);
  MIDQry.SQLText =
   "BEGIN "\
   " SELECT move_id.nextval INTO :move_id from dual; "\
   " INSERT INTO move_ref(move_id,reference)  SELECT :move_id, NULL FROM dual; "\
   "END;";
  MIDQry.DeclareVariable( "move_id", otInteger );
  /* ����室��� ᤥ���� �஢��� �� �� ����⢮����� ३� */
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

  TSpp spp;
  string err_city;
  createSPP( localdate, spp, false, err_city );
  TDoubleTrip doubletrip;
  CreatorSPPLocker locker;

  for ( TSpp::iterator sp=spp.begin(); sp!=spp.end(); sp++ ) {
    tmapds &mapds = sp->second;
    for ( map<int,TDestList>::iterator im=mapds.begin(); im!=mapds.end(); im++ ) {
      locker.commit();
      TDests::iterator p = im->second.dests.end();
      int point_id = 0,first_point = 0;

      locker.lock(); // ��窠 ��। ���᪮�
      TElemFmt fmt;
      /* �஢�ઠ �� �� ����⢮����� */
      bool exists = false;
      for ( TDests::iterator d=im->second.dests.begin(); d!=im->second.dests.end() - 1; d++ ) {
        TDateTime vscd_in, vscd_out;
        if ( d->scd_in > NoExists )
            vscd_in = d->scd_in + d->diff;//!!!08.04.13im->second.diff;
        else
            vscd_in = NoExists;
        if ( d->scd_out > NoExists )
            vscd_out = d->scd_out + d->diff;//!!!08.04.13im->second.diff;
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
        continue;
      }

      MIDQry.Execute();
      int move_id = MIDQry.GetVariableAsInteger( "move_id" );
      PQry.SetVariable( "move_id", move_id );

      bool pr_tranzit;
      string airline, suffix, airp;
      TDateTime scd_out;
      vector<TTripInfo> flts;
      set<int> points;
      bool isEmptySCD_IN = false;
      for ( TDests::iterator d=im->second.dests.begin(); d!=im->second.dests.end(); d++ ) {
        PQry.SetVariable( "point_num", d->num );
        airp = ElemToElemId( etAirp, d->airp, fmt );
        PQry.SetVariable( "airp", airp );
        PQry.SetVariable( "airp_fmt", (int)d->airp_fmt );

        pr_tranzit=( d != im->second.dests.begin() ) &&
                   ( p->airline + IntToString( p->trip ) + p->suffix ==
                     d->airline + IntToString( d->trip ) + d->suffix );  //!!!������ � ����� ����


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
        if ( d->scd_in == NoExists ) {
          PQry.SetVariable( "scd_in", FNull );
          if ( d != im->second.dests.begin() ) {
            isEmptySCD_IN = true;
          }
        }
        else
          PQry.SetVariable( "scd_in", d->scd_in + d->diff );//!!!08.04.13im->second.diff );
        if ( d->scd_out == NoExists ) {
          PQry.SetVariable( "scd_out", FNull );
          scd_out = NoExists;
        }
        else {
          scd_out = d->scd_out + d->diff;//!!!08.04.13im->second.diff;
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
                       !d->triptype.empty() &&
                       ((const TTripTypesRow&)TripTypes.get_row("code", d->triptype )).pr_reg != 0 &&
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
          set_flight_sets(point_id, d->f, d->c, d->y);
        }
        //���᫥��� ��ᮢ ���ᠦ�஢ �� ३��
        PersWeightRules weights;
        persWeights.getRules( point_id, weights );
        weights.write( point_id );
        points.insert(point_id);
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
      TTlgBinding(true).bind_flt_oper(flts);
      TTrferBinding().bind_flt_oper(flts);
      for ( auto const &i : points) {
        on_change_trip( CALL_POINT, i, ChangeTrip::SeasonCreateSPP );
      };
      if ( isEmptySCD_IN ) {
        changeSCDIN_AtDests( points );
      }
    }  //flights points
  } //spp days
  TReqInfo::Instance()->LocaleToLog("EVT.SEASON.GET_SPP", LEvntPrms()
                                    << PrmDate("time", localdate, "dd.mm.yy"), evtSeason);
}

void SeasonInterface::GetSPP(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
//TReqInfo::Instance()->user.check_access( amWrite );
  TDateTime localdate;
  modf( (double)NodeAsDateTime( "date", reqNode ), &localdate );
  CreateSPP( localdate );
  AstraLocale::showMessage("MSG.DATA_SAVED");
}

bool insert_points( double da, int move_id, TFilter &filter, TDateTime first_day, TDateTime vd, TDestList &ds)
{
  ProgTrace( TRACE5, "da=%s, move_id=%d, first_day=%s, vd=%s",
             DateTimeToStr( da, "dd.mm.yy hh:nn" ).c_str(),
             move_id,
             DateTimeToStr( first_day, "dd.mm.yy hh:nn" ).c_str(),
             DateTimeToStr( vd, "dd.mm.yy hh:nn" ).c_str() );

  TReqInfo *reqInfo = TReqInfo::Instance();
  bool canUseAirline, canUseAirp; /* ����� �� �ᯮ�짮���� ����� ३� */

  canUseAirline = false;
  canUseAirp = false;
  // ����� move_id, vd �� ��ਮ� �믮������
  // ����稬 ������� � �஢�ਬ �� �ࠢ� ����㯠 � �⮬� ��������
  const auto boost_vd = DateTimeToBoost(vd);
  DB::TQuery Qry(PgOra::getROSession("ROUTES"));

  Qry.SQLText =
    "SELECT num, routes.airp, routes.airp_fmt, scd_in, "
    "        airline, airline_fmt, flt_no, craft, craft_fmt, "
    "        scd_out, trip_type, litera, "
    "        routes.pr_del, f, c, y, suffix, suffix_fmt, delta_in, delta_out "
    " FROM ROUTES "
    " WHERE routes.move_id=:vmove_id "
    " ORDER BY move_id,num";

  // Qry.CreateVariable( "vdate", otDate, vd );
  Qry.CreateVariable( "vmove_id", otInteger, move_id );

  Qry.Execute();
  bool candests = false;
  ds.pr_del = true;

  double f1;
  while ( !Qry.Eof ) {
    TDest d;
    d.num = Qry.FieldAsInteger( "num" );
    d.airp = Qry.FieldAsString( "airp" );
    d.airp_fmt = (TElemFmt)Qry.FieldAsInteger( "airp_fmt" );
    d.city = BaseTables::Port(d.airp)->city()->code();
    d.pr_del = Qry.FieldAsInteger( "pr_del" );
    if ( !d.pr_del )
      ds.pr_del = false;
    if ( Qry.FieldIsNULL( "scd_in" ) )
      d.scd_in = NoExists;
    else {
      // scd_in-TRUNC(scd_in)+:vdate+delta_in scd_in
      const boost::gregorian::days delta_in(Qry.FieldAsInteger("delta_in"));
      auto scd_in = DateTimeToBoost(Qry.FieldAsDateTime( "scd_in" ));
      scd_in = (boost_vd + delta_in) + (scd_in - boost::posix_time::ptime(scd_in.date()));
      d.scd_in = BoostToDateTime(scd_in);
      modf( (double)d.scd_in, &f1 );
      if ( f1 == da ) {
        candests = candests || filter.isFilteredUTCTime( da, first_day, d.scd_in );
        ProgTrace( TRACE5, "filter.firsttime=%s, filter.lasttime=%s, d.scd_in=%s, res=%d",
                   DateTimeToStr( filter.firstTime, "dd hh:nn" ).c_str(),
                   DateTimeToStr( filter.lastTime, "dd hh:nn" ).c_str(),
                   DateTimeToStr( d.scd_in, "dd hh:nn" ).c_str(), candests );
      }
    }
    d.airline = Qry.FieldAsString( "airline" );
    d.airline_fmt = (TElemFmt)Qry.FieldAsInteger( "airline_fmt" );

    d.region = AirpTZRegion( Qry.FieldAsString( "airp" ), false );

    //d.diff = ddiff( d.region, first_day, vd );
    //first_day - ���+�६� �믮������ ३�, vd - ��� �� ����
    double vdt = modf( (double)first_day, &f1 ) + vd;
    ProgTrace( TRACE5, "create season time %s", DateTimeToStr( vdt, "dd hh:nn" ).c_str() );
    d.diff = getDatesOffsetsDiff(first_day, vdt, d.region);

    ProgTrace( TRACE5, "dest: region=%s, first_date=%s, curr_date=%s, diff=%f",
                       d.region.c_str(), DateTimeToStr( first_day, "dd.mm.yy hh:nn" ).c_str(),
                       DateTimeToStr( vd, "dd.mm.yy hh:nn" ).c_str(), d.diff );

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
      // scd_out-TRUNC(scd_out)+:vdate+delta_out
      const boost::gregorian::days delta_out(Qry.FieldAsInteger("delta_out"));
      auto scd_out = DateTimeToBoost(Qry.FieldAsDateTime("scd_out"));
      scd_out = (boost_vd + delta_out) + (scd_out - boost::posix_time::ptime(scd_out.date()));
      d.scd_out = BoostToDateTime(scd_out);

      modf((double)d.scd_out, &f1);
      if ( f1 == da ) {
        candests = candests || filter.isFilteredUTCTime( da, first_day, d.scd_out );
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
    if ( reqInfo->user.access.airps().permitted( d.airp ) ) // new
        canUseAirp = true; //new
    if ( reqInfo->user.access.airlines().permitted( d.airline ) ) //new
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

// �६��� � 䨫��� �࠭���� � UTC
void createTrips( TDateTime utc_spp_date, TDateTime ldt_SppStart, TFilter &filter,
                  TDestList &ds, string &err_city )
{
  TDateTime firstTime = filter.firstTime;
  TDateTime lastTime = filter.lastTime;

  TReqInfo *reqInfo = TReqInfo::Instance();

  if ( reqInfo->user.user_type != utAirport ) {
    filter.firstTime = NoExists;
    createAirlineTrip( NoExists, filter, ds, ldt_SppStart, err_city );
  }
  else {
    TStageTimes stagetimes( sRemovalGangWay );
    if (!reqInfo->user.access.airps().elems_permit())
      throw Exception("%s: strange situation access.airps().elems_permit()=false for user_type=utAirport", __FUNCTION__);
    for ( set<string>::iterator s=reqInfo->user.access.airps().elems().begin();
                                s!=reqInfo->user.access.airps().elems().end(); s++ ) {
      int vcount = (int)ds.trips.size();
      // ᮧ���� ३�� �⭮�⥫쭮 ࠧ�襭��� ���⮢ reqInfo->user.access.airps

      createAirportTrip( *s, NoExists, filter, ds, utc_spp_date, false, true, err_city );
      for ( int i=vcount; i<(int)ds.trips.size(); i++ ) {
        ds.trips[ i ].trap = stagetimes.GetTime( ds.trips[ i ].airlineId, ds.trips[ i ].airpId, ds.trips[ i ].craftId, ds.trips[ i ].triptypeId, ds.trips[ i ].scd_out );
      }
    }
  }
  filter.firstTime = firstTime;
  filter.lastTime = lastTime;
}

void createSPP( TDateTime ldt_SPPStart, TSpp &spp, bool createViewer, string &err_city )
{
  map<string,TTimeDiff> v;
  TFilter filter;
  filter.GetSeason();

  DB::TQuery Qry(PgOra::getROSession("SCHED_DAYS"));

  double udt_SppStart, udt_SppEnd;
  udt_SppStart = ClientToUTC( ldt_SPPStart, filter.filter_tz_region, false );
  udt_SppEnd = ClientToUTC( ldt_SPPStart + 1 - 1/1440, filter.filter_tz_region, false ); // 1/1440 - 1 ���.

  const bool pg_enabled = PgOra::supportsPg("SCHED_DAYS");

  ProgTrace( TRACE5, "spp on local date %s, utc date and time begin=%s, end=%s",
             DateTimeToStr( ldt_SPPStart, "dd.mm.yy" ).c_str(),
             DateTimeToStr( udt_SppStart, "dd.mm.yy hh:nn" ).c_str(),
             DateTimeToStr( udt_SppEnd, "dd.mm.yy hh:nn" ).c_str() );
  // ��� ��砫� ���� ������� ᯨ᮪ ��ਮ���, ����� �믮������� � ��� ����, ���� ��� ��� �६���

  if(pg_enabled) {
    Qry.SQLText =
        "SELECT DISTINCT move_id,first_day,last_day,:vd - interval '1 day' * delta AS qdate,pr_del,d.region region "
        "  FROM "
        "  ( SELECT routes.move_id as move_id,"
        "           delta_in as delta,"
        "           sched_days.pr_del as pr_del,"
        "           first_day,last_day,region "
        " FROM sched_days,routes "
        " WHERE routes.move_id = sched_days.move_id AND "
        "       DATE_TRUNC('day', first_day) + interval '1 day' * (delta_in + delta) <= :vd AND "
        "       DATE_TRUNC('day', last_day) + interval '1 day' * (delta_in + delta) >= :vd AND  "
        "       POSITION(TO_CHAR(:vd - interval '1 day' * (delta_in + delta), 'ID') in days) != 0 "
        "   UNION "
        " SELECT routes.move_id as move_id, "
        "        delta_out as delta,"
        "        sched_days.pr_del as pr_del,"
        "        first_day,last_day,region "
        " FROM sched_days,routes "
        "   WHERE routes.move_id = sched_days.move_id AND "
        "         DATE_TRUNC('day', first_day) + interval '1 day' * (delta_out + delta) <= :vd AND "
        "         DATE_TRUNC('day', last_day) + interval '1 day' * (delta_out + delta) >= :vd AND "
        "         POSITION(TO_CHAR(:vd - interval '1 day' * (delta_out + delta), 'ID') in days) != 0 ) as d "
        " ORDER BY move_id, qdate";
  } else {
    Qry.SQLText =
        " SELECT DISTINCT move_id,first_day,last_day,:vd-delta AS qdate,pr_del,d.region region "
        "  FROM "
        "  ( SELECT routes.move_id as move_id,"
        "           delta_in as delta,"
        "           sched_days.pr_del as pr_del,"
        "           first_day,last_day,region "
        " FROM sched_days,routes "
        " WHERE routes.move_id = sched_days.move_id AND "
        "       TRUNC(first_day) + delta_in + delta <= :vd AND "
        "       TRUNC(last_day) + delta_in + delta >= :vd AND  "
        "       INSTR( days, TO_CHAR( :vd - delta_in - delta, 'D' ) ) != 0 "
        "   UNION "
        " SELECT routes.move_id as move_id, "
        "        delta_out as delta,"
        "        sched_days.pr_del as pr_del,"
        "        first_day,last_day,region "
        " FROM sched_days,routes "
        "   WHERE routes.move_id = sched_days.move_id AND "
        "         TRUNC(first_day) + delta_out + delta <= :vd AND "
        "         TRUNC(last_day) + delta_out + delta >= :vd AND "
        "         INSTR( days, TO_CHAR( :vd - delta_out - delta, 'D' ) ) != 0 ) d "
        " ORDER BY move_id, qdate";
  }

   Qry.DeclareVariable( "vd", otDate );
   //! f3 = modf( d1, &f1 );
   //! f4 = modf( d2, &f2 );

   TDateTime ud_SppStart, ud_SppEnd;

   TDateTime ut_SppStart = modf( udt_SppStart, &ud_SppStart );
   TDateTime ut_SppEnd = modf( udt_SppEnd, &ud_SppEnd );

   //! f3 = modf( udt_SppStart, &f1 );
   //! f4 = modf( udt_SppEnd, &f2 );

   TDestList ds;

   for ( double ud_SppCurrDay = ud_SppStart; ud_SppCurrDay <= ud_SppEnd; ++ud_SppCurrDay ) {
     if ( ud_SppCurrDay == ud_SppStart ) {
       filter.firstTime = ut_SppStart;
       filter.lastTime = 1.0 - 1.0/1440.0;
     }
     else {
       filter.firstTime = 0.0;
       filter.lastTime = ut_SppEnd - 1.0/1440.0;
     }
     ProgTrace( TRACE5, "filter{ first: %s, last: %s }", DateTimeToStr(filter.firstTime).c_str() , DateTimeToStr(filter.lastTime).c_str());
     ProgTrace( TRACE5, "date = %s", DateTimeToStr( ud_SppCurrDay, "dd.mm.yy  hh:nn" ).c_str() );

     Qry.SetVariable( "vd", ud_SppCurrDay );
     Qry.Execute();
     vector<TDateTime> days;
     int vmove_id = -1;

//     string flight_tz_region;
     TDateTime udt_scdPeriodStart = ASTRA::NoExists, udt_scdPeriodEnd = ASTRA::NoExists;

     while ( 1 ) {
       if ( vmove_id > 0 && ( Qry.Eof || vmove_id != Qry.FieldAsInteger( "move_id" ) ) ) {
        // 横� �� ����祭�� ��⠬
         for ( vector<TDateTime>::iterator vd = days.begin(); vd != days.end(); ++vd ) {

           ProgTrace( TRACE5, "day = %s, move_id = %d",
                      DateTimeToStr( *vd, "dd.mm.yy hh:nn" ).c_str(), vmove_id );

          if ( insert_points( ud_SppCurrDay, vmove_id, filter, udt_scdPeriodStart, *vd, ds ) ) { // ����� �ࠢ� � ������⮬ ࠡ���� + 䨫��� �� �६����
              ds.flight_time = udt_scdPeriodStart;
              ds.last_day = udt_scdPeriodEnd;
//              ds.flight_tz_region1 = flight_tz_region;

              ProgTrace( TRACE5, "canspp trip d=%s spp[ %s ][ %d ].trips.size()=%zu",
                         DateTimeToStr( ud_SppCurrDay, "dd.mm.yy hh:nn" ).c_str(),
                         DateTimeToStr( *vd, "dd.mm.yy hh:nn" ).c_str(),
                         vmove_id,
                         spp[ *vd ][ vmove_id ].trips.size() );
              if ( createViewer ) {
                vector<trip> trips = spp[ *vd ][ vmove_id ].trips; // ��࠭塞 㦥 ����祭�� ३��
/*                if ( spp[ *vd ][ vmove_id ].trips.empty() ) {*/
                  createTrips( ud_SppCurrDay, ldt_SPPStart, filter, ds, err_city );
                  // 㤠����� �㡫������ ३ᮢ
                  for ( vector<trip>::iterator itr=trips.begin(); itr!=trips.end(); itr++ ) {
                    vector<trip>::iterator jtr=ds.trips.begin();
                    for ( ; jtr!=ds.trips.end(); jtr++ )
                      if ( itr->name == jtr->name && itr->scd_out == jtr->scd_out && itr->scd_in == jtr->scd_in )
                        break;
                    if ( jtr == ds.trips.end() )
                        ds.trips.push_back( *itr );
                  }
                  ProgTrace( TRACE5, "ds.trips.size()=%zu", ds.trips.size() );
              }
              spp[ *vd ][ vmove_id ] = ds;
           } // end insert

           ProgTrace( TRACE5, "first_day=%s, move_id=%d",
                      DateTimeToStr( udt_scdPeriodStart, "dd.mm.yy hh:nn" ).c_str(),
                      vmove_id );

           ds.dests.clear();
           ds.trips.clear();
         } // end for days
         days.clear();
       } // end if

       if ( Qry.Eof )
        break;

       vmove_id = Qry.FieldAsInteger( "move_id" );
//       flight_tz_region = Qry.FieldAsString( "region" );
       udt_scdPeriodStart = Qry.FieldAsDateTime( "first_day" );
       udt_scdPeriodEnd = Qry.FieldAsDateTime( "last_day" );

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

  for ( TSpp::iterator sp = spp.begin(); sp != spp.end(); sp++ )
  {
    tmapds &mapds = sp->second;
    for ( map<int,TDestList>::iterator im = mapds.begin(); im != mapds.end(); im++ ) {

      ProgTrace( TRACE5, "build xml vdate=%s, move_id=%d, trips.size()=%zu",
                 DateTimeToStr( sp->first, "dd.mm.yy" ).c_str(),
                 im->first,
                 im->second.trips.size() );

      for ( vector<trip>::iterator tr = im->second.trips.begin(); tr != im->second.trips.end(); tr++ )
          ViewTrips.push_back( *tr );

      im->second.trips.clear();
    }
  }
  if ( reqInfo->user.user_type != utAirport )
    sort( ViewTrips.begin(), ViewTrips.end(), CompareAirlineTrip );
  else
    sort( ViewTrips.begin(), ViewTrips.end(), CompareAirpTrip );

  //! ��ନ�㥬 XML
  xmlNodePtr tripsSPP = NULL;
  for ( vector<trip>::iterator tr = ViewTrips.begin(); tr != ViewTrips.end(); tr++ )
  {
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
      for ( vector<TDest>::iterator h = tr->vecportsFrom.begin(); h != tr->vecportsFrom.end(); h++ )
      {
         if ( !ports_in.empty() )
           ports_in += "/";
         ports_in += ElemIdToElemCtxt( ecDisp, etAirp, h->airp, h->airp_fmt );
      }
      for ( vector<TDest>::iterator h = tr->vecportsTo.begin(); h != tr->vecportsTo.end(); h++ )
      {
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
        NewTextChild( tripNode, "ports_out", tr->portsForAirline ); /* �祭� ��㤭� �����뢠���� �� ����, ���⮬� ⠪ */
    if ( !tr->vecportsFrom.empty() ) {
      xmlNodePtr psNode = NewTextChild( tripNode, "portsFrom" );

      for ( vector<TDest>::iterator h = tr->vecportsFrom.begin(); h != tr->vecportsFrom.end(); ++h ) {
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
      for ( vector<TDest>::iterator h = tr->vecportsTo.begin(); h != tr->vecportsTo.end(); ++h ) {
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
      NewTextChild( tripNode, "ref", AstraLocale::getLocaleText("�⬥��") );
  }
 if ( !err_city.empty() )
    AstraLocale::showErrorMessage( "MSG.CITY.REGION_NOT_DEFINED.NOT_ALL_FLIGHTS_ARE_SHOWN",
                                     LParams() << LParam("city", ElemIdToCodeNative(etCity,err_city)));
}

void VerifyRangeList( TRangeList &rangeList, map<int,TDestList> &mapds )
{
  vector<string> flg;
  // �஢�ઠ �������
  for ( map<int,TDestList>::iterator im=mapds.begin(); im!=mapds.end(); im++ ) {
    ProgTrace( TRACE5, "im->second.dests.size()=%zu", im->second.dests.size() );
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
      if ( !id->pr_del )
        notpr_del++;
ProgTrace( TRACE5, "airp=%s, scd_in=%f, scd_out=%f", id->airp.c_str(), id->scd_in, id->scd_out );
      if ( id->scd_in > NoExists || id->scd_out > NoExists )
        notime = false;
      pid = id;
    } /* end for */
    if ( notime )
      throw AstraLocale::UserException( "MSG.CHECK_FLIGHT.EMPTY_TIME_IN_ROUTE" );
    /* ���� ३� �⬥���, �.�. ����⨫��� �� ����� ������ �� �⬥������� �㭪� ��ᠤ�� */
    if ( notpr_del <= 1 ) {
     for ( TDests::iterator id=im->second.dests.begin(); id!=im->second.dests.end(); id++ ) {
       id->pr_del = true;
     }
     im->second.pr_del = true;
    }
    else
      im->second.pr_del = false;
  }
  // �஢�ઠ ���������� + ���⠢����� �⬥������ ��ਮ���
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

TDateTime ConvertFlightDate( TDateTime time, TDateTime first, const std::string &airp, bool pr_arr, TConvert convert )
{
  double first_day, f2, f3, utcFirst;
  modf( (double)first, &utcFirst );
  modf( (double)first, &first_day );
  TDateTime val = ASTRA::NoExists;
  std::string region = AirpTZRegion( airp );
  if ( time != NoExists ) {
    f2 = modf( (double)time, &f3 );
    f3 += first_day + fabs( f2 );
    ProgTrace( TRACE5, "scd=%s, region=%s, airp=%s",DateTimeToStr( f3, "dd.mm.yyyy hh:nn:ss" ).c_str(), region.c_str(), airp.c_str() );
    if ( convert == mtoLocal ) {
      try {
        f2 = modf( (double)UTCToClient( f3, region ), &f3 );
      }
      catch( Exception &e ) {
          const TAirpsRow& row=(const TAirpsRow&)base_tables.get("airps").get_row("code",airp,true);
          throw AstraLocale::UserException( "MSG.CITY.REGION_NOT_DEFINED",
                                            LParams() << LParam("city", ElemIdToCodeNative(etCity,row.city )));
      }
    }
    else {
      try {
        f2 = modf( (double)ClientToUTC( f3, region ), &f3 );
      }
      catch( boost::local_time::ambiguous_result& ) {
        f2 = modf( (double)ClientToUTC( f3 + 1, region ) - 1, &f3 );
      }
      catch( boost::local_time::time_label_invalid& ) {
        throw AstraLocale::UserException( pr_arr?"MSG.ARV_TIME_FOR_POINT_NOT_EXISTS":"MSG.DEP_TIME_FOR_POINT_NOT_EXISTS",
                LParams() << LParam("airp", ElemIdToCodeNative(etAirp,airp)) << LParam("time", DateTimeToStr( first, "dd.mm" )));
      }
    }
    ProgTrace( TRACE5, "trunc(scd)=%s, time=%s",
               DateTimeToStr( f3, "dd.mm.yyyy hh:nn:ss" ).c_str(),
               DateTimeToStr( f2, "dd.mm.yyyy hh:nn:ss" ).c_str() );
    if ( f3 < utcFirst )
      val = f3 - utcFirst - f2;
    else
      val = f3 - utcFirst + f2;
    if (convert == mtoUTC)
      ProgTrace( TRACE5, "utc scd=%s",DateTimeToStr( val, "dd.mm.yyyy hh:nn:ss" ).c_str() );
    else
      ProgTrace( TRACE5, "local scd=%s",DateTimeToStr( val, "dd.mm.yyyy hh:nn:ss" ).c_str() );
  }
  return val;
}


// ࠧ��� � ��ॢ�� �६�� � UTC, � ���������� �믮������ �࠭���� �६��� �뫥�
bool ParseRangeList( xmlNodePtr rangelistNode, TRangeList &rangeList, map<int,TDestList> &mapds, const std::string &filter_tz_region )
{
  TBaseTable &baseairps = base_tables.get( "airps" );
  TReqInfo *reqInfo = TReqInfo::Instance();
  bool canUseAirline, canUseAirp; /* ����� �� �ᯮ�짮���� ����� ३� */
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
    double first_day, f2;
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
          dest.airp = ElemCtxtToElemId( ecDisp, etAirp, NodeAsStringFast( "cod", curNode ), dest.airp_fmt, false ); // ᪮����⨫ � � ��� ����� � ����
        }
        catch( EConvertError &e ) {
            throw AstraLocale::UserException( "MSG.AIRP.INVALID_GIVEN_CODE" );
        }
        if ( dest.airp.empty() )
          throw AstraLocale::UserException( "MSG.AIRP.INVALID_GIVEN_CODE" );
        dest.city = ((const TAirpsRow&)baseairps.get_row( "code", dest.airp )).city;
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
        if ( reqInfo->user.access.airps().permitted( dest.airp ) ) // new
            canUseAirp = true; //new
        if ( reqInfo->user.access.airlines().permitted( dest.airline ) ) //new
            canUseAirline = true; //new
        ds.dests.push_back( dest );
        destNode = destNode->next;
      } // while ( destNode )
      if ( !canUseAirline || !canUseAirp )
        throw AstraLocale::UserException( "MSG.INSUFFICIENT_RIGHTS.NOT_ACCESS" );
//      ProgTrace( TRACE5, "first_dest=%d", ds.first_dest );
      if ( mapds.find( period.move_id ) == mapds.end() ) //! ����� ���� ��ਮ� (३�) � �ࠧ� ���ਫ� ��� ����� ��⮩
        mapds.insert(std::make_pair( period.move_id, ds ) );
      else
        newdests = false; // �ᯮ��㥬 ���� �������
    } // if ( node )
    // ��ਮ�� �࠭��� �६� �뫥� �� �.�. ��ॢ���� � UTC
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
    period.delta = ConvertPeriod( period, ds.flight_time, filter_tz_region );
    if ( newdests ) {
      // ��ॢ�� �६�� � ������� � �������
      for ( TDests::iterator id=ds.dests.begin(); id!=ds.dests.end(); id++ ) {
        id->scd_in = ConvertFlightDate( id->scd_in, period.first, id->airp, true, mtoUTC );
        id->scd_out = ConvertFlightDate( id->scd_out, period.first, id->airp, false, mtoUTC );
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

void SEASON::int_write(const TFilter &filter, int ssm_id, vector<TPeriod> &speriods,
                        int &trip_id, map<int, TDestList> &mapds)
{
  LogTrace(TRACE5) << __func__ << " ssm_id=" << ssm_id << " trip_id=" << trip_id;
  vector<TPeriod> oldperiods;
  DB::TQuery SQrySched(PgOra::getROSession("SCHED_DAYS"));
  SQrySched.SQLText =
    "SELECT first_day, last_day, days, pr_del, tlg, reference, trip_id, move_id "
    " FROM sched_days "
    " WHERE trip_id = :trip_id";
  SQrySched.CreateVariable( "trip_id", otInteger, trip_id );
  SQrySched.Execute();
  while ( !SQrySched.Eof ) {
      TPeriod p;
      p.first = SQrySched.FieldAsDateTime( "first_day" );
      p.last = SQrySched.FieldAsDateTime( "last_day" );
      p.days = SQrySched.FieldAsString( "days" );
      p.pr_del = SQrySched.FieldAsInteger( "pr_del" );
      p.tlg = SQrySched.FieldAsString( "tlg" );
      p.ref = SQrySched.FieldAsString( "reference" );
      p.move_id = SQrySched.FieldAsInteger( "move_id" );
      oldperiods.push_back( p );
      SQrySched.Next();
  }

  int num=0;
  if ( trip_id != NoExists ) {
    if ( ssm_id == NoExists ) {
      std::list<int> move_id_list = getMoveIdByTripId(trip_id, filter.periods.begin()->period.begin());
      // ⥯��� ����� 㤠���� �� ��ਮ��, ���.
      //!!! �訡�� �.�. ��ਮ�� ��������� �⭮�⥫쭮 ॣ���� ��ࢮ�� �.�. � ���. delta=0
      ProgTrace( TRACE5, "delete all periods from database" );
      deleteRoutesByMoveid(move_id_list);
      deleteSchedDays(trip_id, filter.periods.begin()->period.begin());
    }
    num = getMaxNumSchedDays(trip_id) + 1;
  }
  else {
    // �� ���� ३�
    ProgTrace(TRACE5, "it is new trip");
    trip_id = getNextRoutesTripId();
    ProgTrace( TRACE5, "new trip_id=%d", trip_id );
    TReqInfo::Instance()->LocaleToLog("EVT.SEASON.NEW_FLIGHT", evtSeason, trip_id);
  }

  LogTrace(TRACE3) << "INSERT INTO sched_days";
  auto InsSched = DB::TQuery(PgOra::getRWSession("SCHED_DAYS"));
  InsSched.SQLText =
      "INSERT INTO sched_days(trip_id,move_id,num,first_day,last_day,days,pr_del,tlg,reference,region,ssm_id,delta) "
      "VALUES(:trip_id,:move_id,:num,:first_day,:last_day,:days,:pr_del,:tlg,:reference,:region,:ssm_id,:delta) ";
  InsSched.DeclareVariable( "trip_id", otInteger );
  InsSched.DeclareVariable( "move_id", otInteger );
  InsSched.DeclareVariable( "num", otInteger );
  InsSched.DeclareVariable( "first_day", otDate );
  InsSched.DeclareVariable( "last_day", otDate );
  InsSched.DeclareVariable( "days", otString );
  InsSched.DeclareVariable( "pr_del", otInteger );
  InsSched.DeclareVariable( "tlg", otString );
  InsSched.DeclareVariable( "reference", otString );
  InsSched.CreateVariable( "region", otString, filter.filter_tz_region );
  InsSched.CreateVariable( "ssm_id", otInteger, ssm_id==ASTRA::NoExists?FNull:ssm_id );
  InsSched.DeclareVariable( "delta", otInteger );

  DB::TQuery RQry(PgOra::getRWSession("ROUTES"));
  RQry.SQLText =
  "INSERT INTO routes(move_id,num,airp,airp_fmt,pr_del,scd_in,airline,airline_fmt,flt_no,craft,craft_fmt,scd_out,litera, "
  "                   trip_type,rbd_order,f,c,y,unitrip,delta_in,delta_out,suffix,suffix_fmt) "
  " VALUES(:move_id,:num,:airp,:airp_fmt,:pr_del,:scd_in,:airline,:airline_fmt,:flt_no,:craft,:craft_fmt,:scd_out,:litera, "
  "        :trip_type,:rbd_order,:f,:c,:y,:unitrip,:delta_in,:delta_out,:suffix,:suffix_fmt) ";
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
  RQry.DeclareVariable( "rbd_order", otString );
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
      new_move_id = getNextRoutesMoveId();
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


    ProgTrace( TRACE5, "trip_id=%d, new_move_id=%d,num=%d", trip_id, new_move_id,num );
    InsSched.SetVariable( "trip_id", trip_id );
    InsSched.SetVariable( "move_id", new_move_id );
    InsSched.SetVariable( "num", num );
    InsSched.SetVariable( "first_day", ip->first );
    InsSched.SetVariable( "last_day", ip->last );
    InsSched.SetVariable( "days", ip->days );
    InsSched.SetVariable( "pr_del", ip->pr_del );
    InsSched.SetVariable( "tlg", ip->tlg );
    InsSched.SetVariable( "reference", ip->ref );
    InsSched.SetVariable( "delta", ip->delta );
    LogTrace(TRACE5) << "InsSched: trip_id=" << trip_id <<
      " move_id=" << new_move_id << " num=" << num <<
      " first_day=" << ip->first <<
      " last_day=" << ip->last <<
      " days=" << ip->days << " pr_del=" << ip->pr_del <<
      " tlg=" << ip->tlg << " reference=" << ip->ref <<
      " region=" << filter.filter_tz_region << " ssm_id=" << ssm_id;

    vector<TPeriod>::iterator ew = oldperiods.end();
    for ( ew=oldperiods.begin(); ew!=oldperiods.end(); ew++ ) {
        if ( ew->first == ip->first && ew->last == ip->last )
            break;
    }
    string lexema_id;
    LEvntPrms params;
    if ( ew == oldperiods.end() ) {
      lexema_id = "EVT.SEASON.NEW_PERIOD";
      params << PrmDate("date_first", ip->first, "dd.mm.yy")
                << PrmDate("date_last", ip->last, "dd.mm.yy")
                << PrmSmpl<string>("days", ip->days);
      if ( ip->pr_del )
        params << PrmLexema("del", "EVT.CANCEL");
      else params << PrmSmpl<string>("del", "");
    }
    else {
      bool empty = true;
      if ( ip->days != ew->days ) {
        PrmLexema lexema("days", "EVT.DAYS");
        lexema.prms << PrmSmpl<string>("days", ip->days);
        params << lexema;
        empty = false;
      }
      else params << PrmSmpl<string>("days", "");
      if ( ew->pr_del != ip->pr_del ) {
        params << PrmLexema("del", "EVT.CANCEL");
        empty = false;
      }
      else params << PrmSmpl<string>("del", "");
      if ( ip->tlg != ew->tlg ) {
        PrmLexema lexema("tlg", "EVT.TLG");
        lexema.prms << PrmSmpl<string>("tlg", ip->tlg);
        params << lexema;
        empty = false;
      }
      else params << PrmSmpl<string>("tlg", "");
      if ( ip->ref != ew->ref ) {
        PrmLexema lexema("ref", "EVT.REFERENCES");
        lexema.prms << PrmSmpl<string>("ref", ip->ref);
        params << lexema;
        empty = false;
      }
      else params << PrmSmpl<string>("ref", "");
      if (!empty) {
        lexema_id = "EVT.SEASON.MODIFY_PERIOD";
      }
      params << PrmDate("date_first", ip->first, "dd.mm.yy")
             << PrmDate("date_last", ip->last, "dd.mm.yy");
      ew->modify = fdelete;
    }
    if (!lexema_id.empty()) {
      params << PrmSmpl<int>("trip_id", trip_id) << PrmSmpl<int>("route_id", new_move_id);
      TReqInfo::Instance()->LocaleToLog( lexema_id, params, evtSeason, trip_id, new_move_id );
    }
    // GRISHA
    InsSched.Execute()  ;
    num++;
    TDestList ds = mapds[ ip->move_id ];
    int dnum = 0;
    double fl, ff;
    tst();
    if ( !ds.dests.empty() ) {
      tst();
      PrmEnum prmenum("route", "-");
      for ( TDests::iterator id=ds.dests.begin(); id!=ds.dests.end(); id++ ) {
        ProgTrace(TRACE5, "airp=%s, move_id=%d, num=%d, airp_fmt=%d, id->pr_del=%d, id->airline=%s, id->scd_in=%f, id->scd_out=%f, id->trip=%d, id->craft=%s, id->f=%d, id->c=%d, id->y=%d",
                  id->airp.c_str(), new_move_id, dnum, (int)id->airp_fmt, id->pr_del, id->airline.c_str(), id->scd_in, id->scd_out, id->trip, id->craft.c_str(), id->f, id->c, id->y );
        RQry.SetVariable( "move_id", new_move_id );
        RQry.SetVariable( "num", dnum );
        RQry.SetVariable( "airp", id->airp );
        RQry.SetVariable( "airp_fmt", (int)id->airp_fmt );
        PrmEnum prmenum2("", "");
        bool pr_add = false;
        if ( !id->airline.empty() ) {
          prmenum2.prms << PrmElem<string>("", etAirline, id->airline);
          pr_add = true;
        }
        if ( id->trip != NoExists ) {
        prmenum2.prms << PrmSmpl<int>("", id->trip);
          pr_add = true;
        }
        if ( !id->suffix.empty() ) {
          prmenum2.prms << PrmElem<string>("", etSuffix, id->suffix);
          pr_add = true;
        }
        if ( !id->craft.empty() ) {
          prmenum2.prms << PrmSmpl<string>("", ",");
          prmenum2.prms << PrmElem<string>("", etCraft, id->craft);
          pr_add = true;
        }
        if ( pr_add )
          prmenum2.prms << PrmSmpl<string>("", ",");
        RQry.SetVariable( "pr_del", id->pr_del );
        if ( id->scd_in > NoExists ) {
          RQry.SetVariable( "scd_in", modf( (double)id->scd_in, &fl ) ); // 㤠�塞 delta_in
          prmenum2.prms << PrmDate("", id->scd_in, "hh:nn(UTC)");
        }
        else {
          fl = 0.0;
          RQry.SetVariable( "scd_in", FNull );
        }
        prmenum2.prms << PrmElem<string>("", etAirp, id->airp);
        if ( id->airline.empty() ) {
          RQry.SetVariable( "airline", FNull );
          RQry.SetVariable( "airline_fmt", FNull );
        }
        else {
          RQry.SetVariable( "airline", id->airline );
          RQry.SetVariable( "airline_fmt", (int)id->airline_fmt );
        }
        if ( id->trip != NoExists ) {
          RQry.SetVariable( "flt_no", id->trip );
        }
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
          RQry.SetVariable( "scd_out", modf( (double)id->scd_out, &ff ) ); // 㤠�塞 delta_out
          prmenum2.prms << PrmDate("", id->scd_out, "hh:nn(UTC)");
        }
        else {
          ff = 0.0;
          RQry.SetVariable( "scd_out", FNull );
        }
        if ( id->pr_del )
          prmenum2.prms << PrmLexema("", "EVT.CANCEL");
        if ( id->triptype.empty() )
          RQry.SetVariable( "trip_type", FNull );
        else
          RQry.SetVariable( "trip_type", id->triptype );
        if ( id->rbd_order.empty() )
          RQry.SetVariable( "rbd_order", FNull );
        else
          RQry.SetVariable( "rbd_order", id->rbd_order);
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
        prmenum.prms << prmenum2;
      }
      mapds[ ip->move_id ].dests.clear();
      ip->move_id = new_move_id;
      TReqInfo::Instance()->LocaleToLog("EVT.SEASON.ROUTE", LEvntPrms() << prmenum, evtSeason, trip_id, new_move_id );
    }
  }
  for ( vector<TPeriod>::iterator ew=oldperiods.begin(); ew!=oldperiods.end(); ew++ ) {
    if ( ew->modify == fdelete )
        continue;
    LEvntPrms params;
    params << PrmDate("date_first", ew->first, "dd.mm.yy")
              << PrmDate("date_last", ew->last, "dd.mm.yy")
              << PrmSmpl<string>("days", ew->days);
    if ( ew->pr_del )
      params << PrmLexema("del", "EVT.CANCEL");
    else
        params << PrmSmpl<string>("del", "");
    params << PrmSmpl<int>("trip_id", trip_id) << PrmSmpl<int>("route_id", ew->move_id);
    TReqInfo::Instance()->LocaleToLog("EVT.SEASON.DELETE_PERIOD", params, evtSeason, trip_id, ew->move_id);
  }
}

void SeasonInterface::Write(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  xmlNodePtr dataNode = NewTextChild( resNode, "data" );
  TFilter filter;
  xmlNodePtr filterNode = GetNode( "filter", reqNode );
  filter.Parse( filterNode );
  TRangeList rangeList;
  map<int,TDestList> mapds;
  xmlNodePtr rangelistNode = GetNode( "SubrangeList", reqNode );
  ParseRangeList( rangelistNode, rangeList, mapds, filter.filter_tz_region );

  VerifyRangeList( rangeList, mapds );
  vector<TPeriod> nperiods, speriods;
  xmlNodePtr node = GetNode( "trip_id", reqNode );
  int trip_id = ASTRA::NoExists;
  if ( node ) {
    trip_id = NodeAsInteger( node );
  }

  // GRISHA ����� ।���஢���� ��ਮ��, ����祭���� �� SSM ⥫��ࠬ��
  if (SsmIdExists(trip_id))
  {
    AstraLocale::showError("MSG.SSIM.EDIT_SSM_PERIOD_FORBIDDEN");
    return;
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
  int_write( filter, NoExists, speriods, trip_id, mapds );
  // ���� ������� ���ଠ�� �� �࠭� ।���஢����
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
  if ( OwnDest->trip == NoExists ) /* ३� �� �ਫ�� */
    res = ElemIdToElemCtxt( ecDisp, etAirline, airline, airline_fmt ) +
          IntToString( flt_no ) +
          ElemIdToElemCtxt( ecDisp, etSuffix, suffix, suffix_fmt );
  else
    if ( flt_no == NoExists ||
         ( airline == OwnDest->airline && flt_no == OwnDest->trip && suffix == OwnDest->suffix ) )
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

TDateTime TDateTimeToClientICU(TDateTime ud_first_day, TDateTime ut_time, TDateTime ud_target_day, const string& region) {

    modf(ud_first_day, &ud_first_day);
    modf(ud_target_day, &ud_target_day);

    double stub, f2, f3;

    if ( ut_time > NoExists ) {
        ut_time = modf(ut_time, &stub);
        TDateTime diff = getDatesOffsetsDiff(ud_first_day + ut_time, ud_target_day + ut_time, region);

        ProgTrace(TRACE5, "TDTICU diff: %s", DateTimeToStr(diff).c_str());

        f2 = modf(UTCToClient(ud_target_day + ut_time + diff, region), &f3);

        if(f3 < ud_target_day)
            return f3 - ud_target_day - f2;
        else
            return f3 - ud_target_day + f2;
    }
    else
        return NoExists;

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
bool createAirportTrip( string airp, int trip_id, TFilter filter, TDestList &ds,
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
  int i = 0;

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
            d.scd_in = TDateTimeToClientICU( ds.flight_time, d.scd_in, utc_spp_date, d.region );
            d.scd_out = TDateTimeToClientICU(ds.flight_time, d.scd_out, utc_spp_date, d.region );
            vecportsFrom.push_back( d );
        }
      }
      else { /* ��� ���� � ������� �� ���� �⮡ࠦ��� */
        TDest d = *NDest;
        d.scd_in = TDateTimeToClientICU( ds.flight_time, d.scd_in, utc_spp_date, d.region );
        d.scd_out = TDateTimeToClientICU(ds.flight_time, d.scd_out, utc_spp_date, d.region );

        ( !OwnDest ? vecportsFrom : vecportsTo ).push_back( d );

        createTrip = ( OwnDest && ( PDest->trip != NDest->trip || PDest->airline != NDest->airline ) );
      }
    }
    catch( Exception &e ) {
        if ( err_city.empty() )
            err_city = NDest->city;
        return false;
    }
    /* ����� �������� ��᪮�쪮 ३ᮢ. */
    if ( createTrip || ( OwnDest != NULL && NDest == &ds.dests.back() ) ) {
     /* �᫨ �� �����, � ����砥��� ३�,
       � ���� ���� �६� �ਫ��/�뫥� � ��� ����� �� 㤮���⢮���� �᫮��� 䨫���
       ����� �� �।�⠢�⥫� ������ ���� => ����� ������ �६��� �뫥� � �ਫ�� � ���� */
       // error view filter time normilize
       bool cantrip = false;
       if ( utc_spp_date > NoExists ) {
         double f2;
         if ( OwnDest->scd_in > NoExists ) {
           modf( (double)OwnDest->scd_in, &f2 );
           ProgTrace( TRACE5, "scd_in f2=%f, utc_spp_date=%f", f2, utc_spp_date );
           if ( f2 == utc_spp_date ) {
             cantrip = true;
             ProgTrace(TRACE5, "scd_in satisfy, cantrip -> true");
           }
         }
         if ( !cantrip && OwnDest->scd_out > NoExists ) {
           modf( (double)OwnDest->scd_out, &f2 );
           ProgTrace( TRACE5, "scd_out f2=%f, utc_spp_date=%f", f2, utc_spp_date );
           if ( f2 == utc_spp_date ) {
             cantrip = true;
             ProgTrace(TRACE5, "scd_out satisfy, cantrip -> true");
           }
         }
       }
       else
         cantrip = true;

      if ( cantrip &&
           ( ( UTCFilter &&
              ( filter.isFilteredUTCTime( utc_spp_date, ds.flight_time, OwnDest->scd_in ) ||
                filter.isFilteredUTCTime( utc_spp_date, ds.flight_time, OwnDest->scd_out ) ) ) ||
             ( !UTCFilter && filter.isFilteredTime( ds.flight_time, OwnDest->scd_in, OwnDest->scd_out, OwnDest->region ) ) )
         ) {
        /* ३� ���室�� ��� ��� �᫮��� */
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
        tr.pr_del = OwnDest->pr_del; //!!! ���ࠢ��쭮 ⠪, ���� ����뢠��
        /* ��ॢ���� �६��� �뫥� �ਫ�� � ������� */ //!!! error tz

        tr.scd_in = TDateTimeToClientICU( ds.flight_time, OwnDest->scd_in, utc_spp_date, OwnDest->region );
        tr.scd_out = TDateTimeToClientICU(ds.flight_time, OwnDest->scd_out, utc_spp_date, OwnDest->region );
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
  ProgTrace( TRACE5, "trips.size()=%zu", ds.trips.size() );
  return !ds.trips.empty();
}


/* UTCTIME */
bool createAirportTrip( int trip_id, TFilter filter, TDestList &ds, bool viewOwnPort, string &err_city )
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  bool res = false;
  if (!reqInfo->user.access.airps().elems_permit())
    throw Exception("%s: strange situation access.airps().elems_permit()=false for user_type=utAirport", __FUNCTION__);
  for ( set<string>::iterator s=reqInfo->user.access.airps().elems().begin();
                              s!=reqInfo->user.access.airps().elems().end(); s++ ) {
    res = res || createAirportTrip( *s, trip_id, filter, ds, NoExists, viewOwnPort, false, err_city );
  }
  return res;
}

/* UTCTIME */
bool createAirlineTrip( int trip_id, TFilter &filter, TDestList &ds, string &err_city )
{
  return createAirlineTrip( trip_id, filter, ds, NoExists, err_city );
}

/* UTCTIME to client  */
bool createAirlineTrip( int trip_id, TFilter &filter, TDestList &ds, TDateTime ldt_SppStart, string &err_city )
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
      timeKey = timeKey || filter.isFilteredTime( ds.flight_time, NDest->scd_in, NDest->scd_out, NDest->region );
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
         tr.portsForAirline += "/"; // "���/"
         str_dests += "/";
      }

      double ud_PeriodStartDay, // ��� ��ࢮ�� ३�
              f2, f3,
              ud_scd_in,
              ut_scd_in,
              ud_scd_out;

      modf( (double)ds.flight_time, &ud_PeriodStartDay );

      // �᫨ ���� ��ᠤ��
      if ( ldt_SppStart > NoExists && NDest->scd_in > NoExists ) {
          ProgTrace( TRACE5, "airp=%s,localdate=%s, utcscd_in=%s, first_day=%s",
                     NDest->airp.c_str(),
                     DateTimeToStr( ldt_SppStart, "dd.mm.yy hh:nn" ).c_str(),
                     DateTimeToStr( NDest->scd_in, "dd.mm.yy hh:nn" ).c_str(),
                     DateTimeToStr( ud_PeriodStartDay, "dd.mm.yy hh:nn" ).c_str() );

        //! double f2;
        TDateTime scd_in;
        TDateTime lt_Landig, ld_Landing;

        //! f3 = modf( (double)NDest->scd_in, &utcDate_scd_in );
        ut_scd_in = modf( (double)NDest->scd_in, &ud_scd_in );
        try {
          //! TDateTime fd_f3 = localDateTime = UTCToClient( utcDateFirstDay + fabs( f3 ), NDest->region);
          TDateTime ldt_Landing = UTCToClient( ud_PeriodStartDay + fabs( ut_scd_in ), NDest->region);
          ProgTrace(TRACE5, "region: %s, first_day = %s, ut_scd_in = %s, LOCAL = %s",
                    NDest->region.c_str(),
                    DateTimeToStr(ud_PeriodStartDay).c_str(),
                    DateTimeToStr(ut_scd_in).c_str(),
                    DateTimeToStr(ldt_Landing).c_str());

          lt_Landig = modf( (double)ldt_Landing, &ld_Landing );
          //! f2 = modf( (double)fd_f3, &f3 );
        }
        catch( Exception &e ) {
            if ( err_city.empty() )
                err_city = NDest->city;
            return false;
        }

        // ����砥� �६�
        if ( ld_Landing < ud_PeriodStartDay )
          scd_in = ld_Landing - ud_PeriodStartDay - lt_Landig;
          //! scd_in = f3 - first_day - f2;
        else
          scd_in = ld_Landing - ud_PeriodStartDay + lt_Landig;
          //! scd_in = f3 - first_day + f2;

        ProgTrace( TRACE5, "trip_name=%s, own_date=%s, localdate=%s, utc_date_scd_in=%s, localDate=%f, first_day=%f",
                   tr.name.c_str(),
                   DateTimeToStr( own_date, "dd.mm.yy hh:nn" ).c_str(),
                   DateTimeToStr( ldt_SppStart, "dd hh:nn" ).c_str(),
                   DateTimeToStr( ud_scd_in, "dd hh:nn" ).c_str(),
                   ld_Landing, ud_PeriodStartDay );

        //if ( utcDate_scd_in + f3 - utcDateFirstDay == localdate ) {
        if ( ud_scd_in + ld_Landing - ud_PeriodStartDay == ldt_SppStart ) {
          ProgTrace( TRACE5, "localdate=%s, localscd_in=%s",
                     DateTimeToStr( ldt_SppStart, "dd hh:nn" ).c_str(),
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
      } // if ( ldt_SppStart > NoExists && NDest->scd_in > NoExists )

      tr.portsForAirline += ElemIdToElemCtxt( ecDisp, etAirp, NDest->airp, NDest->airp_fmt );
      tr.vecportsTo.push_back( *NDest ); // �㦭� ��� �뢮�� �� 1 �࠭ � ᥧ����
      str_dests += ElemIdToElemCtxt( ecDisp, etAirp, NDest->airp, NDest->airp_fmt );
      if ( ldt_SppStart > NoExists && NDest->scd_out > NoExists ) {
          ProgTrace( TRACE5, "airp=%s,localdate=%s, utcscd_out=%s",
                     NDest->airp.c_str(),DateTimeToStr( ldt_SppStart, "dd hh:nn" ).c_str(),
                     DateTimeToStr( NDest->scd_out, "dd hh:nn" ).c_str() );
        TDateTime scd_out;
        f3 = modf( (double)NDest->scd_out, &ud_scd_out );
        try {
          f2 = modf( (double)UTCToClient( ud_PeriodStartDay + fabs( f3 ), NDest->region ), &f3 );
        }
        catch( Exception &e ) {
            if ( err_city.empty() )
                err_city = NDest->city;
            return false;
        }
        if ( f3 < ud_PeriodStartDay )
          scd_out = f3 - ud_PeriodStartDay - f2;
        else
          scd_out = f3 - ud_PeriodStartDay + f2;
        if ( ud_scd_out + f3 - ud_PeriodStartDay == ldt_SppStart ) {
          ProgTrace( TRACE5, "localdate=%s, localscd_out=%s",
                     DateTimeToStr( ldt_SppStart, "dd hh:nn" ).c_str(),
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
      if  ( !PDest || ( PDest->airline != NDest->airline && !NDest->airline.empty() ) ||
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
//ProgTrace( TRACE5, "ds.trips.size()=%zu", trips.size() );
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

bool ConvertPeriodToLocal( TDateTime &first, TDateTime &last, string &days, const string &tz_region, string &err_tz_region )
{
  TDateTime f;
  TDateTime l;

  try {
    f = UTCToClient( first, tz_region );
    l = UTCToClient( last, tz_region );
  }
  catch( Exception &e ) {
    if ( err_tz_region.empty() ) {
        err_tz_region = tz_region;
    }
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

  return true;
}

void GetDests( map<int,TDestList> &mapds, const TFilter &filter, int vmove_id )
{
  LogTrace(TRACE3) << __FUNCTION__;
  HelpCpp::Timer tm1;
  TReqInfo *reqInfo = TReqInfo::Instance();
  DB::TQuery RQry(PgOra::getROSession("ROUTES"));
  string sql =
  "SELECT move_id,num,routes.airp airp,airp_fmt, "
  "       routes.pr_del,scd_in, delta_in, airline,airline_fmt,"
  "       flt_no,craft,craft_fmt,litera,trip_type,scd_out, delta_out,f,c,y,unitrip,suffix,suffix_fmt "
  " FROM routes ";
  if ( vmove_id > NoExists ) {
    RQry.CreateVariable( "move_id", otInteger, vmove_id );
    sql += "WHERE move_id=:move_id ";
  }
  sql += " ORDER BY move_id,num";

  RQry.SQLText = sql;
  RQry.Execute();
  int idx_rmove_id = RQry.FieldIndex("move_id");
  int idx_num = RQry.FieldIndex("num");
  int idx_airp = RQry.FieldIndex("airp");
  int idx_airp_fmt = RQry.FieldIndex("airp_fmt");
  int idx_rpr_del = RQry.FieldIndex("pr_del");
  int idx_scd_in = RQry.FieldIndex("scd_in");
  int idx_delta_in = RQry.FieldIndex("delta_in");
  int idx_airline = RQry.FieldIndex("airline");
  int idx_airline_fmt = RQry.FieldIndex("airline_fmt");
  int idx_trip = RQry.FieldIndex("flt_no");
  int idx_craft = RQry.FieldIndex("craft");
  int idx_craft_fmt = RQry.FieldIndex("craft_fmt");
  int idx_scd_out = RQry.FieldIndex("scd_out");
  int idx_delta_out = RQry.FieldIndex("delta_out");
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
  bool canUseAirline = false, canUseAirp = false; /* ����� �� �ᯮ�짮���� ����� */
  bool compKey = false, cityKey = false, airpKey = false, triptypeKey = false, timeKey = false;
  double f1 = 0.;
  while ( !RQry.Eof ) {
    if ( move_id != RQry.FieldAsInteger( idx_rmove_id ) ) {
      if ( move_id >= 0 ) {
        if ( canUseAirline && canUseAirp &&
             cityKey && airpKey && compKey && triptypeKey && timeKey ) {
            mapds.insert(std::make_pair( move_id, ds ) );
        }
      }

      ds.dests.clear();
      compKey = filter.airline.empty();
      cityKey = filter.city.empty();
      airpKey = filter.airp.empty();
      triptypeKey = filter.triptype.empty();
      timeKey = filter.firstTime == NoExists;

      move_id = RQry.FieldAsInteger( idx_rmove_id );
      canUseAirline = false;
      canUseAirp = false;
    }
    d.num = RQry.FieldAsInteger( idx_num );
    d.airp = RQry.FieldAsString( idx_airp );
    d.airp_fmt = (TElemFmt)RQry.FieldAsInteger( idx_airp_fmt );
    airpKey = airpKey || d.airp == filter.airp;
    d.airline = RQry.FieldAsString( idx_airline );
    d.airline_fmt = (TElemFmt)RQry.FieldAsInteger( idx_airline_fmt );
    compKey = compKey  || d.airline == filter.airline;
    if ( reqInfo->user.access.airps().permitted( d.airp ) )
        canUseAirp = true;
    if ( reqInfo->user.access.airlines().permitted( d.airline ) )
        canUseAirline = true;
    d.city = ((const TAirpsRow&)base_tables.get("airps").get_row( "code", d.airp, true )).city;
    cityKey = cityKey || d.city == filter.city;
    d.region = AirpTZRegion( RQry.FieldAsString( idx_airp ), false );
    d.pr_del = RQry.FieldAsInteger( idx_rpr_del );
    if (RQry.FieldIsNULL(idx_scd_in) or RQry.FieldIsNULL(idx_delta_in))
      d.scd_in = NoExists;
    else {
      const auto scd_in = DateTimeToBoost(RQry.FieldAsDateTime(idx_scd_in)) + \
                          boost::gregorian::days(RQry.FieldAsInteger(idx_delta_in));
      d.scd_in = BoostToDateTime(scd_in);
      modf((double)d.scd_in, &f1);
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
    if ( RQry.FieldIsNULL( idx_scd_out ) or RQry.FieldIsNULL( idx_delta_out ))
      d.scd_out = NoExists;
    else {
      const auto scd_out = DateTimeToBoost(RQry.FieldAsDateTime(idx_scd_out)) +
                           boost::gregorian::days(RQry.FieldAsInteger(idx_delta_out));
      d.scd_out = BoostToDateTime(scd_out);
    }
    d.f = RQry.FieldAsInteger( idx_f );
    d.c = RQry.FieldAsInteger( idx_c );
    d.y = RQry.FieldAsInteger( idx_y );
    d.unitrip = RQry.FieldAsString( idx_unitrip );
    d.suffix = RQry.FieldAsString( idx_suffix );
    d.suffix_fmt = (TElemFmt)RQry.FieldAsInteger( idx_suffix_fmt );
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
       timeKey = timeKey || reqInfo->user.access.airps().permitted(d.airp);
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
    mapds.insert(std::make_pair( move_id, ds ) );
  }
  LogTrace(TRACE5) << __func__ << " " << tm1;
}

TDateTime TFilter::GetTZTimeDiff( TDateTime utcnow, TDateTime first, const string &tz_region )
{
    return fabs(getDatesOffsetsDiff(first, utcnow, tz_region));
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
  string err_tz_region;
  string err_city;
  TDateTime last_date_season = BoostToDateTime( filter.periods.begin()->period.begin() );
  map<int,string> mapreg;
  map<string,TTimeDiff> v;
  map<int,TDestList> mapds;
  DB::TQuery SQry(PgOra::getROSession("SCHED_DAYS"));
  string sql =
  "SELECT trip_id,move_id,first_day,last_day,days,pr_del,tlg,region "
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
  int idx_first_day = SQry.FieldIndex("first_day");
  int idx_last_day = SQry.FieldIndex("last_day");
  int idx_days = SQry.FieldIndex("days");
  int idx_spr_del = SQry.FieldIndex("pr_del");
  int idx_tlg = SQry.FieldIndex("tlg");
  int idx_region = SQry.FieldIndex("region");

  if ( !SQry.RowCount() )
    AstraLocale::showErrorMessage( "MSG.NO_FLIGHTS_IN_SCHED" );
  // ����� ��� ���� ������� �� �ࠧ� ��������
  GetDests( mapds, filter );
  /* ⥯��� ��३��� � �롮થ � 䨫���樨 ���������� */
  TViewPeriod viewperiod;
  viewperiod.trip_id = NoExists;
  int move_id = NoExists;
  TDestList ds;
  string s;

  bool canRange = false;
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
      /* �� ࠡ�⠥� � ३ᠬ� � ���. ��� delta = 0 */
      canRange = !ds.dests.empty();
    }
    if ( canRange ) {
        TDateTime first = SQry.FieldAsDateTime( idx_first_day );
        TDateTime last = SQry.FieldAsDateTime( idx_last_day );
        string days = SQry.FieldAsString( idx_days );
        bool pr_del = SQry.FieldAsInteger( idx_spr_del );
        string tlg = SQry.FieldAsString( idx_tlg );
        TDateTime utc_first = first;
        /* ����稬 �ࠢ��� ���室�(�뢮��) �६�� � ३� */

        string flight_tz_region;
        flight_tz_region = SQry.FieldAsString( idx_region );
        /* �த������ 䨫��஢��� */
        time_period p( DateTimeToBoost( first ), DateTimeToBoost( last ) );
        time_period df( DateTimeToBoost( filter.range.first ), DateTimeToBoost( filter.range.last ) );
        /* 䨫��� �� ����������, ��� � �६���� �뫥�, �᫨ ���짮��⥫� ���⮢�� */
  /* ??? ���� �� ��ॢ����� � 䨫��� ��� �믮������ � UTC */
        ds.flight_time = utc_first;
        //ds.flight_tz_region = flight_tz_region;
//        ProgTrace( TRACE5, "move_id=%d, pregion=%s", move_id, pregion.c_str() );
        if ( df.intersects( p ) &&
             /* ��ॢ���� �������� �믮������ � ������� �ଠ� - ����� ���� ᤢ�� */
             ConvertPeriodToLocal( first, last, days, flight_tz_region, err_tz_region ) &&
             CommonDays( days, filter.range.days ) && /* ??? � df.intersects ���� ��ᬮ���� ���� �� ��� �믮������ */
            ( ds.dests.empty() ||
              ( TReqInfo::Instance()->user.user_type == utAirport &&
                createAirportTrip( viewperiod.trip_id, filter,  ds, true, err_city ) ) ||
              ( TReqInfo::Instance()->user.user_type != utAirport &&
                createAirlineTrip( viewperiod.trip_id, filter, ds, err_city ) ) ) ) {
          rangeListEmpty = false;
          TDateTime delta_out = NoExists; // ���室 �१ ��⪨ �� �뫥��
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
            if ( TReqInfo::Instance()->user.user_type == utAirport && // ⮫쪮 ��� ࠡ�⭨��� ��ய���
                   vt.scd_out > NoExists ) { // ��� ���� ����㧪� ᫮⮢
                vt.first = first;
                vt.last = last;
                vt.days = days;
                vt.pr_del = pr_del;
            }
            viewperiod.trips.push_back( vt );
          }
          mapds[ move_id ].dests.clear(); /* 㦥 �ᯮ�짮���� ������� */
          mapds[ move_id ].trips.clear();
          ds.dests.clear();
          ds.trips.clear();
        } /* ����� �᫮��� 䨫��� �� ��������� */
    } /* end if canRange */
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
 if ( !err_tz_region.empty() ) {
    ProgError( STDLOG, "%s: err_tz_region=%s!", __FUNCTION__, err_tz_region.c_str() );
 }
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
    int move_id = NoExists;
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
      if ( j->scd_out == NoExists || j->pr_del ) { // ⮫쪮 �� �뫥�
        continue;
      }
      if ( !tripsNode ) {
        tripsNode = NewTextChild( dataNode, "trips" );
      }
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
int ConvertPeriod( TPeriod &period, const TDateTime &flight_time, const std::string &filter_tz_region )
{
  double first_day, f3;
  first_day = period.first;
  period.first += flight_time;
  ProgTrace( TRACE5, "period.first=%s, period.last=%s, period.days=%s",
             DateTimeToStr( period.first, "dd.mm.yyyy hh:nn:ss" ).c_str(),
             DateTimeToStr( period.last, "dd.mm.yyyy hh:nn:ss" ).c_str(),
             period.days.c_str() );
  try {
    period.first = ClientToUTC( (double)period.first, filter_tz_region );
  }
  catch( boost::local_time::ambiguous_result& ) {
      period.first = ClientToUTC( (double)period.first + 1, filter_tz_region ) - 1;
  }
  catch( boost::local_time::time_label_invalid& ) {
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
  return (int)first_day - (int)utcFirst;
}
}

void SeasonInterface::Read(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  throwOnScheduleLock();
//  GRISHA
  if ( not get_test_server() &&
       TReqInfo::Instance()->user.access.airlines().totally_permitted() &&
       TReqInfo::Instance()->user.access.airps().totally_permitted() ) {
     throw UserException( "MSG.SET_LEVEL_PERMIT" );
  }

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

struct SchedDays {
    int trip_id;
    int move_id;
    boost::posix_time::ptime first_day;
    boost::posix_time::ptime last_day;
    std::string days;
    int pr_del;
    std::string tlg;
    std::string reference;
    std::string region;
};

std::list<SchedDays> getSchedDays(const boost::posix_time::ptime &begin_date)
{
    auto cur = make_db_curs(
        "SELECT trip_id,move_id,first_day,last_day,days,pr_del,tlg,reference,region "
        " FROM sched_days "
        " WHERE last_day>=:begin_date_season "
        "ORDER BY trip_id,move_id,num",
        PgOra::getROSession("SCHED_DAYS"));

    std::list<SchedDays> reslist;

    SchedDays schd = {};
    cur.bind(":begin_date_season", begin_date)
        .autoNull()
        .def(schd.trip_id)
        .def(schd.move_id)
        .def(schd.first_day)
        .def(schd.last_day)
        .def(schd.days)
        .def(schd.pr_del)
        .def(schd.tlg)
        .def(schd.reference)
        .def(schd.region)
        .exec();

    while(!cur.fen()) {
        reslist.push_back(schd);
    }
    return reslist;
}

void GetEditData( int trip_id, TFilter &filter, bool buildRanges, xmlNodePtr dataNode, string &err_city )
{
    throwOnScheduleLock();

    string err_tz_region;

    const auto lschd = getSchedDays(filter.periods.begin()->period.begin());
    // �롨ࠥ� ��� ।���஢���� �� ��ਮ��, ����� ����� ��� ࠢ�� ⥪�饩 ���
    // ����� ��� ���� ������� �� �ࠧ� ��������
    map<int,string> mapreg;
    map<string,TTimeDiff> v;
    map<int,TDestList> mapds;
    GetDests( mapds, filter );
    xmlNodePtr node = NULL;
    if ( trip_id > NoExists ) {
        NewTextChild( dataNode, "trip_id", trip_id );
        node = NewTextChild( dataNode, "ranges" );
    }
    int move_id = -1;
    int vtrip_id = -1;
    bool canRange = false;
    bool canTrips = false;
    bool DestsExists = false;
    for( const auto schedd: lschd ) {
        TDateTime first = BoostToDateTime(schedd.first_day);
        TDateTime last = BoostToDateTime(schedd.last_day);
        string flight_tz_region;
        flight_tz_region = schedd.region;
        if ( vtrip_id != schedd.trip_id ) {
            canTrips = true;
            vtrip_id = schedd.trip_id;
        }
        if ( move_id != schedd.move_id ) {
            move_id = schedd.move_id;
            if ( canTrips && !mapds[ move_id ].dests.empty() ) {
                mapds[ move_id ].flight_time = first;

                if ( TReqInfo::Instance()->user.user_type == utAirport )
                    canTrips = !createAirportTrip( vtrip_id, filter, mapds[ move_id ], false, err_city );
                else
                    canTrips = !createAirlineTrip( vtrip_id, filter, mapds[ move_id ], err_city );
            }
            canRange = ( !mapds[ move_id ].dests.empty() && schedd.trip_id == trip_id );
        }
        if ( canRange && buildRanges ) {
            DestsExists = true;
            ProgTrace( TRACE5, "edit canrange move_id=%d", move_id );
            string days = schedd.days;

            double utcf;
            modf( (double)first, &utcf );

            double first_day;
            modf( (double)UTCToClient( first, flight_tz_region ), &first_day );
            ProgTrace( TRACE5, "local first_day=%s",DateTimeToStr( first_day, "dd.mm.yyyy hh:nn:ss" ).c_str() );

            /* 䨫��� �� ����������, ��� � �६���� �뫥�, �᫨ ���짮��⥫� ���⮢�� */
            /* ��ॢ���� �������� �믮������ � ������� �ଠ� - ����� ���� ᤢ�� */
            if ( ConvertPeriodToLocal( first, last, days, flight_tz_region, err_tz_region ) ) { // ptz
                xmlNodePtr range = NewTextChild( node, "range" );
                NewTextChild( range, "move_id", move_id );
                NewTextChild( range, "first", DateTimeToStr( (int)first ) );
                NewTextChild( range, "last", DateTimeToStr( (int)last ) );
                NewTextChild( range, "days", days );
                if ( schedd.pr_del )
                    NewTextChild( range, "cancel", 1 );
                if ( !schedd.tlg.empty() )
                    NewTextChild( range, "tlg", schedd.tlg );
                if ( !schedd.reference.empty() )
                    NewTextChild( range, "ref", schedd.reference );
                /* ��।��� ����� ��� �࠭� ।���஢���� */
                if ( !mapds[ move_id ].dests.empty() ) {
                    xmlNodePtr destsNode = NewTextChild( range, "dests" );
                    for ( TDests::iterator id=mapds[ move_id ].dests.begin(); id!=mapds[ move_id ].dests.end(); id++ ) {
                    xmlNodePtr destNode = NewTextChild( destsNode, "dest" );
                    NewTextChild( destNode, "cod", ElemIdToElemCtxt( ecDisp, etAirp, id->airp, id->airp_fmt ) );
                    if ( id->airp != id->city )
                        NewTextChild( destNode, "city", id->city );
                    if ( id->pr_del )
                        NewTextChild( destNode, "cancel", id->pr_del );
                    // � �᫨ � �⮬ ����� ��㣨� �ࠢ��� ���室� �� ��⭥�/������ �ᯨᠭ�� ???
                    // issummer( TDAteTime, region ) != issummer( utcf, pult.region );
                    id->scd_in = ConvertFlightDate( id->scd_in, utcf, id->airp, true, mtoLocal );
                    if ( id->scd_in > NoExists ) {
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
                    id->scd_out = ConvertFlightDate( id->scd_out, utcf, id->airp, true, mtoLocal );
                    if ( id->scd_out > NoExists ) {
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
                    mapds[ move_id ].dests.clear(); /* 㦥 �ᯮ�짮���� ������� */
                } // end if
        /*        } // end else */
            }
        }
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
  string err_city;
  GetEditData( trip_id, filter, trip_id > NoExists, dataNode, err_city );
  ProgTrace(TRACE5, "getdata %ld", tm.Print());

}

void SeasonInterface::convert(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  throw Exception("Unimplemented SeasonInterface::convert");
#if 0
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
  xmlNodePtr node = NULL;
  while ( !Qry.Eof ) {
    if ( trip_id != Qry.FieldAsInteger( "trip_id" ) ) {
        if ( trip_id > NoExists ) {
            // ��।����� ⥪�騩 ᥧ��
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
              if ( !DQry.FieldIsNULL( "suffix" ) ) {
                NewTextChild( d, "suffix", DQry.FieldAsString( "suffix" ) );
              }
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
#endif
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

//������:
// Period.biweekly; - �� �� ⠪��?
// ��� �㤥� ��室��� ࠧ�����, �᫨ ��諨 ��������� �� ⥬ ��ਡ�⠬, ����� ���� �� �࠭�� � �� ��।��� � �-�� ࠧ�����?
// ssim::ScdPeriods AstraSsimCallbacks::getSchedules(const ct::Flight& flight, const Period& period) const - ������� period ��।����� �ᥣ�� � LT (�����쭮� �६���)?
//�ଠ� ������������, ⨯� ��, ��ய��� IATA |ICAO ?



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

           //! ���� �v������ ⮫쪮 �६�, ��� ��� �᫠ � ���室� ��⮪
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

#if HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef XP_TESTING

#include "xp_testing.h"
START_TEST(check_delete_routes)
{
  std::list<int> lmove_id;
  lmove_id.push_back(1);
  deleteRoutesByMoveid(lmove_id);
}
END_TEST;

START_TEST(check_delete_sched_days)
{
  deleteSchedDays(1);
}
END_TEST;

START_TEST(check_int_write_simple) {
  TFilter filter;
  int ssm_id = 10;
  std::vector<TPeriod> speriods;
  int trip_id = 10;
  std::map<int, TDestList> mapds;
  int_write(filter, ssm_id, speriods, trip_id, mapds);
} END_TEST;

START_TEST(check_getMaxNumSchedDays) {
  getMaxNumSchedDays(10);
} END_TEST;

START_TEST(check_boost_datetime_logic)
{
  const auto dt = Dates::DateTime_t(Dates::Date_t(1899,12,30), Dates::time_duration(22, 0, 0));
  const auto dt2 = dt + boost::gregorian::days(-1);
  LogTrace(TRACE3) << "dt=" << dt << " dt2=" << dt2;
  fail_unless(dt2 == Dates::DateTime_t(Dates::Date_t(1899, 12, 29), Dates::time_duration(22, 0, 0)));
  auto cur = make_db_curs(
    "insert into routes (move_id, num, pr_del, scd_out, delta_out) "
    "values "
    "(1, 0, 0, :dt, -1)", PgOra::getRWSession("ROUTES"));
  cur.bind("dt", dt).exec();

  DB::TQuery RQry(PgOra::getROSession("ROUTES"));
  string sql =
      "SELECT scd_out, delta_out "
      " FROM routes where move_id=1";
  RQry.SQLText = sql;
  RQry.Execute();

  int idx_scd_out = RQry.FieldIndex("scd_out");
  int idx_delta_out = RQry.FieldIndex("delta_out");

  const auto scd_out = DateTimeToBoost(RQry.FieldAsDateTime(idx_scd_out)) +
                       boost::gregorian::days(RQry.FieldAsInteger(idx_delta_out));

  fail_unless(scd_out == dt2);
  LogTrace(TRACE3) << "scd_out = " << scd_out << " d: " << RQry.FieldAsDateTime(idx_scd_out);
  auto scd_out2 = BoostToDateTime(scd_out);
  LogTrace(TRACE3) << "scd_out2 = " << DateTimeToBoost(scd_out2) << " d: " << scd_out2;
  fail_unless(DateTimeToBoost(scd_out2) == dt2);

}END_TEST;

START_TEST(check_boost_dt2) {
  const double scd_out = 0.25;
  const int delta = 1;
  const double day_before = -scd_out - delta; //= -1.25

  const auto b_scd_out = DateTimeToBoost(scd_out);
  const auto b_day_before = b_scd_out - boost::gregorian::days(1);

  LogTrace(TRACE3) << "scd_out=" << scd_out <<
              " day_before: " << day_before <<
              " b_scd_out: " << b_scd_out <<
              " b_day_before: " << b_day_before <<
              " BoostToDateTime(b_day_before) = " << BoostToDateTime(b_day_before);
  LogTrace(TRACE3) << "DateTimeToBoost(-1.25) = " << DateTimeToBoost(-1.25);
  fail_unless(BoostToDateTime(b_day_before) == day_before);
} END_TEST;

#define SUITENAME "season"
TCASEREGISTER(testInitDB, testShutDBConnection)
{
  ADD_TEST(check_delete_routes);
  ADD_TEST(check_delete_sched_days);
  ADD_TEST(check_int_write_simple);
  ADD_TEST(check_getMaxNumSchedDays);
  ADD_TEST(check_boost_datetime_logic);
  ADD_TEST(check_boost_dt2);
}
TCASEFINISH;
#undef SUITENAME
#endif /*XP_TESTING*/
