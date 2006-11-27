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
  TDateTime now;
  int PrevMin=-1;
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
	while ( !Qry.Eof ) {
		try {
			name = Qry.FieldAsString( "name" );
	    if ( name == "astra_timer" )
	    	astra_timer( utcdate );
	    else
	    	if ( name == "createSPP" )
	    		createSPP( utcdate );
	    	else
	    		if ( name == "ETCheckStatusFlt" )
	    			ETCheckStatusFlt();
	    			
			UQry.SetVariable( "name", name );
			UQry.Execute();	 //???
			OraSession.Commit();
		}
    catch( Exception E ) {
    	try { OraSession.Rollback(); } catch(...){};
      ProgError( STDLOG, "Exception: %s, task name=%s", E.what(), name.c_str() );
    }
    catch( ... ) {
    	try { OraSession.Rollback(); } catch(...){};
      ProgError( STDLOG, "Unknown error, task name=%s", name.c_str() );
    };
		Qry.Next();
	}
}

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
    ProgTrace(TRACE5,"ETCheckStatusFlt intrance");
    TQuery UpdQry(&OraSession);
    UpdQry.SQLText="UPDATE trip_sets SET pr_etstatus=1 WHERE point_id=:point_id";
    UpdQry.DeclareVariable("point_id",otInteger);
    Qry.SQLText=
      "SELECT points.point_id FROM points,trip_sets "
      "WHERE points.point_id=trip_sets.point_id AND "
      "      act_out IS NOT NULL AND pr_etstatus=0 ";
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next(),OraSession.Rollback())
    {
      try
      {
      	ProgTrace(TRACE5,"ETCheckStatusFlt: point_id=%d",Qry.FieldAsInteger("point_id"));
      	OrigOfRequest org("Y1",
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

#define ENDL "\r\n"

void sync_mvd(TDateTime now)
{
  int Hour,Min,Sec;
  DecodeTime(now,Hour,Min,Sec);

  TQuery Qry(&OraSession);
  Qry.Clear(); /*!!!*/
  Qry.SQLText="SELECT files.dir,files.last_create,airps.lat AS airp_lat\
               FROM files,options,airps\
               WHERE airps.cod=options.cod AND files.name='ЛОВД' AND pr_denial=0";
  Qry.Execute();
  if (Qry.Eof) return;
  if (!Qry.FieldIsNULL("last_create")&&
      (now-Qry.FieldAsDateTime("last_create"))<1.0/1440) return;

  fstream f;
  char file_name[64];

  sprintf(file_name,"%s%02d%02d.%s",
          Qry.FieldAsString("dir"),Hour,Min,Qry.FieldAsString("airp_lat"));
  f.open( file_name , ios_base::out ); //открыть и залочить на чтение и запись !!!
  if (!f.is_open()) throw Exception((string)"Can't open file '"+file_name+"'");
  try
  {
    Qry.Clear();
    Qry.SQLText=
      "SELECT TO_CHAR(time,'DD.MM.YYYY') AS op_date,\
              TO_CHAR(time,'HH24:MI') AS op_time,\
              airline||LPAD(flt_no,GREATEST(3,LENGTH(flt_no)),'0')||suffix AS trip,\
              TO_CHAR(takeoff,'DD.MM.YYYY') AS takeoff_date,\
              SUBSTR(term,1,6) AS term,\
              seat_no,\
              SUBSTR(surname,1,20) AS surname,\
              SUBSTR(LTRIM(RTRIM(SUBSTR(name||' ',1,INSTR(name||' ',' ')))),1,20) AS name,\
              SUBSTR(LTRIM(RTRIM(SUBSTR(name||' ',INSTR(name||' ',' ')+1))),1,20) AS patronymic,\
              SUBSTR(document,1,20) AS document,\
              operation,\
              tags,\
              airline,\
              airp_dep,\
              airp_arv,\
              bag_weight,\
              TO_CHAR(takeoff,'HH24:MI') AS takeoff_time,\
              SUBSTR(pnr,1,12) AS pnr\
       FROM rozysk WHERE time>=:now-1 AND time<:now ORDER BY time";
    Qry.CreateVariable("now",otDate,now);
    Qry.Execute();
    while(!Qry.Eof)
    {
      f << Qry.FieldAsString("op_date") << '|'
        << Qry.FieldAsString("op_time") << '|'
        << Qry.FieldAsString("trip") << '|'
        << Qry.FieldAsString("takeoff_date") << '|'
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
        << Qry.FieldAsString("takeoff_time") << '|'
        << Qry.FieldAsString("pnr") << '|' << ENDL;
      Qry.Next();
    };
    f.close();
    Qry.Clear();
    Qry.SQLText=
      "BEGIN\
         UPDATE files SET last_create=:now WHERE name='ЛОВД';\
         DELETE FROM rozysk WHERE time<:now;\
       END;";
    Qry.CreateVariable("now",otDate,now);
    Qry.Execute();
    Qry.Close();
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
