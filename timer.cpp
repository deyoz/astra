//---------------------------------------------------------------------------
#include <stdio.h>
#ifdef __WIN32__
 #include <dos.h>
 #define sleep(x) _sleep(x)
#endif
#include <fstream>
#include "timer.h"
#include "oralib.h"
#include "exceptions.h"
#define NICKNAME "VLAD"
#include "test.h"

const int sleepsec = 30;

using namespace BASIC;
using namespace EXCEPTIONS;
using namespace std;

#ifndef __WIN32__

int main_timer_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist)
{
  TDateTime now;
  int PrevMin=-1;
  for( ;; )
  {
    ProgTrace(TRACE5,"astra_timer - ok");	
    try
    {
      now=Now();
      int Hour,Min,Sec;
      DecodeTime(now,Hour,Min,Sec);
      if (Min!=PrevMin)
      {
        PrevMin=Min;
        if (Min%2==0)
        {         	         
          astra_timer();
          ProgTrace(TRACE5,"astra_timer - ok");
        };
        if (Min%15==0)
        {         
          sync_mvd(now);
        };
      };
    }
    catch( Exception E ) {
      ProgError( STDLOG, "Exception: %s", E.Message );
    }
    catch( ... ) {
      ProgError( STDLOG, "Unknown error" );
    };    
    sleep( sleepsec );    
  };
}
#endif

void astra_timer(void)
{
  TQuery Qry(&OraSession);
  try
  {
    Qry.SQLText=
      "BEGIN\
         timer.astratimer;\
       END;";
    Qry.Execute();
    Qry.Close();
    OraSession.Commit();
  }
  catch(...)
  {
    try { OraSession.Rollback( ); } catch( ... ) { };
    throw;
  };
};

#ifdef __WIN32__
#define ENDL "\n"
#else
#define ENDL "\r\n"
#endif

void sync_mvd(TDateTime now)
{
  int Hour,Min,Sec;
  DecodeTime(now,Hour,Min,Sec);

  TQuery Qry(&OraSession);
  Qry.Clear();
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
  if (!f.is_open()) throw Exception("Can't open file '%s'",file_name);
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
