//---------------------------------------------------------------------------
#include <stdio.h>
#include <signal.h>
#include <fstream>
#include "timer.h"
#include "oralib.h"
#include "exceptions.h"
#include "etick.h"
#include "astra_ticket.h"
#include "season.h"
#include "stages.h"
#include "tlg/tlg.h"
#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "test.h"
#include <daemon.h>
const int sleepsec = 5;

using namespace BASIC;
using namespace EXCEPTIONS;
using namespace std;
using namespace Ticketing;

int main_timer_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist)
{
  try
  {
    OpenLogFile("log1");
    ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks()
        ->connect_db();
    if (init_edifact()<0) throw Exception("'init_edifact' error");
    for( ;; )
    {
      try
      {
        exec_tasks();
      }
      catch( std::exception E ) {
        ProgError( STDLOG, "Exception: %s", E.what() );
      }
      catch( ... ) {
        ProgError( STDLOG, "Unknown error" );
      };
      sleep( sleepsec );
    };
  }
  catch( std::exception E ) {
    ProgError( STDLOG, "Exception: %s", E.what() );
  }
  catch( ... ) {
    ProgError( STDLOG, "Unknown error" );
  };
  return 0;
}

void exec_tasks( void )
{
	TDateTime VTime = 0.0, utcdate = NowUTC();
	int Hour, Min, Sec;
	DecodeTime( utcdate, Hour, Min, Sec );
	modf( (double)utcdate, &utcdate );
	EncodeTime( Hour, Min, 0, VTime );
	utcdate += VTime;
	TQuery Qry(&OraSession);
	Qry.SQLText =
	 "SELECT name,last_exec,interval FROM tasks "\
	 " WHERE pr_denial=0 AND NVL(next_exec,:utcdate) <= :utcdate ";
	Qry.CreateVariable( "utcdate", otDate, utcdate );
	Qry.Execute();
	TQuery UQry(&OraSession);
	UQry.SQLText =
	 "UPDATE tasks SET last_exec=:utcdate,next_exec=NVL(next_exec,:utcdate)+interval/1440 "\
	 " WHERE name=:name";
	UQry.CreateVariable( "utcdate", otDate, utcdate );
	UQry.DeclareVariable( "name", otString );
	string name;
	while ( !Qry.Eof )
	{
	  try
	  {
	    name = Qry.FieldAsString( "name" );
	    if ( name == "astra_timer" ) astra_timer( utcdate );
	    else
	      if ( name == "createSPP" ) createSPP( utcdate );
	    	else
	    	  if ( name == "ETCheckStatusFlt" ) ETCheckStatusFlt();
	    	    else
	    	      if ( name == "sync_mvd" ) sync_mvd();

	    UQry.SetVariable( "name", name );
	    UQry.Execute();	 //???
	    OraSession.Commit();
	  }
          catch( Exception E )
          {
    	    try { OraSession.Rollback(); } catch(...) {};
            ProgError( STDLOG, "Exception: %s, task name=%s", E.what(), name.c_str() );
          }
          catch( ... )
          {
    	    try { OraSession.Rollback(); } catch(...) {};
            ProgError( STDLOG, "Unknown error, task name=%s", name.c_str() );
          };
	  Qry.Next();
	};
};

void createSPP( TDateTime utcdate )
{
	map<string,string> regions;
	string city = "ŒŽ‚";
	utcdate += 1; //  ­  á«¥¤ãîé¨© ¤¥­ì
	TReqInfo *reqInfo = TReqInfo::Instance();
	reqInfo->clear();
	reqInfo->user.time_form = tfUTC;
	reqInfo->user.user_type = utSupport;
	reqInfo->desk.tz_region = GetTZRegion( city, regions, true );
	CreateSPP( utcdate );
	ProgTrace( TRACE5, "‘ ¯®«ãç¥­ §  %s", DateTimeToStr( utcdate, "dd.mm.yy" ).c_str() );
}

void ETCheckStatusFlt(void)
{
  TQuery Qry(&OraSession);
  try
  {
    TQuery UpdQry(&OraSession);
    UpdQry.SQLText="UPDATE trip_sets SET pr_etstatus=1 WHERE point_id=:point_id";
    UpdQry.DeclareVariable("point_id",otInteger);
    Qry.SQLText=
     "SELECT p.point_id "
     "FROM points, "
     "  (SELECT points.point_id,point_num, "
     "          DECODE(pr_tranzit,0,points.point_id,first_point) AS first_point "
     "   FROM points,trip_sets "
     "   WHERE points.point_id=trip_sets.point_id AND points.pr_del=0 AND "
     "         act_out IS NOT NULL AND pr_etstatus=0) p "
     "WHERE points.first_point=p.first_point AND "
     "      points.point_num>p.point_num AND points.pr_del=0 AND "
     "      points.pr_tranzit=0 AND NVL(act_in,NVL(est_in,scd_in))<system.UTCSYSDATE ";
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next(),OraSession.Rollback())
    {
      try
      {
      	ProgTrace(TRACE5,"ETCheckStatusFlt: point_id=%d",Qry.FieldAsInteger("point_id"));
      	OrigOfRequest org("UT",
      	                  "ŒŽ‚",
      	                  "ŒŽ‚",
                          'Y',
                          "SYSTEM",
                          "",
                          Lang::RUSSIAN);
        if (!ETCheckStatus(org,Qry.FieldAsInteger("point_id"),csaFlt,Qry.FieldAsInteger("point_id")))
        {
          UpdQry.SetVariable("point_id",Qry.FieldAsInteger("point_id"));
          UpdQry.Execute();
        };
        OraSession.Commit();
      }
      catch(...) {};
    };
    Qry.Close();
    UpdQry.Close();
  }
  catch(...)
  {
    try { OraSession.Rollback( ); } catch( ... ) { };
    throw;
  };
};

#define ENDL "\r\n"

void create_mvd_file(TDateTime first_time, TDateTime last_time,
                     const char* airp, const char* tz_region, const char* file_name)
{
  ofstream f;
  f.open(file_name);
  if (!f.is_open()) throw Exception("Can't open file '%s'",file_name);
  try
  {
    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText=
      "SELECT time, "
      "       airline,flt_no,suffix, "
      "       takeoff, "
      "       SUBSTR(term,1,6) AS term, "
      "       seat_no, "
      "       SUBSTR(surname,1,20) AS surname, "
      "       SUBSTR(LTRIM(RTRIM(SUBSTR(name||' ',1,INSTR(name||' ',' ')))),1,20) AS name, "
      "       SUBSTR(LTRIM(RTRIM(SUBSTR(name||' ',INSTR(name||' ',' ')+1))),1,20) AS patronymic, "
      "       SUBSTR(document,1,20) AS document, "
      "       operation, "
      "       tags, "
      "       airp_dep, "
      "       airp_arv, "
      "       bag_weight, "
      "       SUBSTR(pnr,1,12) AS pnr, "
      "       system.AirpTZRegion(airp_dep) AS tz_region "
      "FROM rozysk "
      "WHERE time>=:first_time AND time<:last_time AND (airp_dep=:airp OR :airp IS NULL) "
      "ORDER BY time";
    Qry.CreateVariable("first_time",otDate,first_time);
    Qry.CreateVariable("last_time",otDate,last_time);
    Qry.CreateVariable("airp",otString,airp);
    Qry.Execute();

    if (tz_region!=NULL&&*tz_region!=0)
    {
      first_time = UTCToLocal(first_time, tz_region);
      last_time = UTCToLocal(last_time, tz_region);
    };

    f << DateTimeToStr(first_time,"ddmmyyhhnn") << '-'
      << DateTimeToStr(last_time-1.0/1440,"ddmmyyhhnn") << ENDL;
    for(;!Qry.Eof;Qry.Next())
    {
      const char* tz_region = Qry.FieldAsString("tz_region");

      TDateTime time_local=UTCToLocal(Qry.FieldAsDateTime("time"),tz_region);
      TDateTime takeoff_local;
      if (!Qry.FieldIsNULL("term"))
        takeoff_local=UTCToLocal(Qry.FieldAsDateTime("takeoff"),tz_region);
      else
        takeoff_local=Qry.FieldAsDateTime("takeoff");

      f << DateTimeToStr(time_local,"dd.mm.yyyy") << '|'
        << DateTimeToStr(time_local,"hh:nn") << '|'
        << setw(3) << setfill('0') << Qry.FieldAsInteger("flt_no") << Qry.FieldAsString("suffix") << '|'
        << DateTimeToStr(takeoff_local,"dd.mm.yyyy") << '|'
        << Qry.FieldAsString("term") << '|'
        << Qry.FieldAsString("seat_no") << '|'
        << Qry.FieldAsString("surname") << '|'
        << Qry.FieldAsString("name") << '|'
        << Qry.FieldAsString("patronymic") << '|'
        << Qry.FieldAsString("document") << '|'
        << Qry.FieldAsString("operation") << '|'
        << Qry.FieldAsString("tags") << '|'
        << Qry.FieldAsString("airline") << '|' << '|'
        << Qry.FieldAsString("airp_dep") << '|'
        << Qry.FieldAsString("airp_arv") << '|'
        << Qry.FieldAsString("bag_weight") << '|' << '|'
        << DateTimeToStr(takeoff_local,"hh:nn") << '|'
        << Qry.FieldAsString("pnr") << '|' << ENDL;
    };
    f.close();
  }
  catch(...)
  {
    try { f.close(); } catch( ... ) { };
    try
    {
      //¢ á«ãç ¥ ®è¨¡ª¨ § ¯¨è¥¬ ¯ãáâ®© ä ©«
      f.open(file_name);
      if (f.is_open()) f.close();
    }
    catch( ... ) { };
    throw;
  };
};

void sync_mvd(void)
{
  int Hour,Min,Sec;
  TDateTime now=NowUTC();
  DecodeTime(now,Hour,Min,Sec);
  if (Min%15!=0) return;

  TQuery Qry(&OraSession);
  Qry.SQLText =
    "UPDATE files SET last_create=:now WHERE code=:code AND "
    "                 (airp=:airp OR airp IS NULL AND :airp IS NULL)";
  Qry.DeclareVariable("code",otString);
  Qry.DeclareVariable("airp",otString);
  Qry.DeclareVariable("now",otDate);

  TQuery FilesQry(&OraSession);
  FilesQry.Clear();
  FilesQry.SQLText=
    "SELECT name,dir,last_create,airp, "
    "       DECODE(airp,NULL,NULL,system.AirpTZRegion(airp)) AS tz_region "
    "FROM files "
    "WHERE code=:code AND pr_denial=0";
  FilesQry.DeclareVariable("code",otString);

  for(int i=0;i<=1;i++)
  {
    modf(now,&now);
    if (i==0)
    {
      TDateTime now_time;
      EncodeTime(Hour,Min,0,now_time);
      now+=now_time;
      FilesQry.SetVariable("code","‹Ž‚„");
    }
    else
      FilesQry.SetVariable("code","‹Ž‚„-‘“’Šˆ");

    Qry.SetVariable("code",FilesQry.GetVariableAsString("code"));
    Qry.SetVariable("now",now);

    FilesQry.Execute();
    for(;!FilesQry.Eof;FilesQry.Next())
    {
      if (!FilesQry.FieldIsNULL("last_create")&&
         (now-FilesQry.FieldAsDateTime("last_create"))<1.0/1440) continue;

      TDateTime local;
      if (!FilesQry.FieldIsNULL("tz_region"))
        local=UTCToLocal(now,FilesQry.FieldAsString("tz_region"));
      else
        local=now;
      ostringstream file_name;
      file_name << FilesQry.FieldAsString("dir")
                << DateTimeToStr(local,FilesQry.FieldAsString("name"));

      if (FilesQry.FieldIsNULL("last_create"))
        create_mvd_file(now-1,now,
                        FilesQry.FieldAsString("airp"),
                        FilesQry.FieldAsString("tz_region"),
                        file_name.str().c_str());
      else
        create_mvd_file(FilesQry.FieldAsDateTime("last_create"),now,
                        FilesQry.FieldAsString("airp"),
                        FilesQry.FieldAsString("tz_region"),
                        file_name.str().c_str());

      Qry.SetVariable("airp",FilesQry.FieldAsString("airp"));
      Qry.Execute();
      OraSession.Commit();
    };
  };
};
