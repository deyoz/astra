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
	string city = "МОВ";
	utcdate += 1; //  на следующий день
	TReqInfo *reqInfo = TReqInfo::Instance();
	reqInfo->clear();
	reqInfo->user.time_form = tfUTC;
	reqInfo->user.user_type = utSupport;
	reqInfo->desk.tz_region = GetTZRegion( city, regions, true );
	CreateSPP( utcdate );
	ProgTrace( TRACE5, "СПП получен за %s", DateTimeToStr( utcdate, "dd.mm.yy" ).c_str() );
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
      	OrigOfRequest org("P2",
      	                  "МОВ",
      	                  "МОВ",
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

void sync_mvd(void)
{
  int Hour,Min,Sec;
  TDateTime now=NowUTC();
  DecodeTime(now,Hour,Min,Sec);
  if (Min%15!=0) return;

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT time, "
    "       airline||LPAD(flt_no,GREATEST(3,LENGTH(flt_no)),'0')||suffix AS trip, "
    "       takeoff, "
    "       SUBSTR(term,1,6) AS term, "
    "       seat_no, "
    "       SUBSTR(surname,1,20) AS surname, "
    "       SUBSTR(LTRIM(RTRIM(SUBSTR(name||' ',1,INSTR(name||' ',' ')))),1,20) AS name, "
    "       SUBSTR(LTRIM(RTRIM(SUBSTR(name||' ',INSTR(name||' ',' ')+1))),1,20) AS patronymic, "
    "       SUBSTR(document,1,20) AS document, "
    "       operation, "
    "       tags, "
    "       airline, "
    "       airp_dep, "
    "       airp_arv, "
    "       bag_weight, "
    "       SUBSTR(pnr,1,12) AS pnr "
    "FROM rozysk WHERE time>=:now-1 AND time<:now AND airp_dep=:airp ORDER BY time";
  Qry.CreateVariable("now",otDate,now);
  Qry.DeclareVariable("airp",otString);

  TQuery UpdQry(&OraSession);
  UpdQry.Clear();
  UpdQry.SQLText=
    "BEGIN "
    "  UPDATE files SET last_create=:now WHERE name='ЛОВД' AND airp=:airp; "
    "  DELETE FROM rozysk WHERE time<:now AND airp_dep=:airp; "
    "END;";
  UpdQry.CreateVariable("now",otDate,now);
  UpdQry.DeclareVariable("airp",otString);


  TQuery FilesQry(&OraSession);
  FilesQry.Clear();
  FilesQry.SQLText=
    "SELECT files.dir,files.last_create,files.airp,airps.code_lat AS airp_lat, "
    "       system.AirpTZRegion(files.airp) AS tz_region "
    "FROM files,airps "
    "WHERE airps.code=files.airp AND files.name='ЛОВД' AND pr_denial=0";
  FilesQry.Execute();
  for(;!FilesQry.Eof;FilesQry.Next())
  {
    if (!FilesQry.FieldIsNULL("last_create")&&
       (now-FilesQry.FieldAsDateTime("last_create"))<1.0/1440) continue;

    const char *tz_region=FilesQry.FieldAsString("tz_region");
    const char *airp=FilesQry.FieldAsString("airp");

    TDateTime local=UTCToLocal(now,tz_region);
    DecodeTime(local,Hour,Min,Sec);
    fstream f;
    char file_name[64];
    sprintf(file_name,"%s%02d%02d.%s",
            FilesQry.FieldAsString("dir"),Hour,Min,FilesQry.FieldAsString("airp_lat"));
    f.open( file_name , ios_base::out ); //открыть и залочить на чтение и запись !!!
    if (!f.is_open()) throw Exception((string)"Can't open file '"+file_name+"'");
    try
    {
      Qry.SetVariable("airp",airp);
      Qry.Execute();
      for(;!Qry.Eof;Qry.Next())
      {
        TDateTime time_local=UTCToLocal(Qry.FieldAsDateTime("time"),tz_region);
        TDateTime takeoff_local;
        if (!Qry.FieldIsNULL("term"))
          takeoff_local=UTCToLocal(Qry.FieldAsDateTime("takeoff"),tz_region);
        else
          takeoff_local=Qry.FieldAsDateTime("takeoff");

        f << DateTimeToStr(time_local,"dd.mm.yyyy") << '|'
          << DateTimeToStr(time_local,"hh:nn") << '|'
          << Qry.FieldAsString("trip") << '|'
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
          << Qry.FieldAsString("pnr") << '|' << endl;
      };
      f.close();
      UpdQry.SetVariable("airp",airp);
      UpdQry.Execute();
      OraSession.Commit();
    }
    catch(...)
    {
      try { OraSession.Rollback( ); } catch( ... ) { };
      try { f.close(); } catch( ... ) { };
      try
      {
        f.open( file_name , ios_base::out );
        f.close();
      }
      catch( ... ) { };
      throw;
    };


  };

};
