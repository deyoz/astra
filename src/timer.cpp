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
#include "astra_consts.h"
#include "cfgproc.h"
const int sleepsec = 5;

using namespace ASTRA;
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
	    	      	else
	    	      		if ( name == "arx_daily" ) arx_daily( NowUTC()-60 );

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
      	OrigOfRequest org("UT",
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
      //в случае ошибки запишем пустой файл
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
      FilesQry.SetVariable("code","ЛОВД");
    }
    else
      FilesQry.SetVariable("code","ЛОВД-СУТКИ");

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

const int ARX_MIN_DAYS()
{
	static bool init=false;
  static int VAR;
  if (!init) {
    char r[100];
    r[0]=0;
    if ( get_param( "ARX_MIN_DAYS", r, sizeof( r ) ) < 0 )
      throw EXCEPTIONS::Exception( "Can't read param ARX_MIN_DAYS" );
    if (StrToInt(r,VAR)==EOF||r<=0)
      throw EXCEPTIONS::Exception("Wrong param ARX_MIN_DAYS");
    init=true;
    ProgTrace( TRACE5, "ARX_MIN_DAYS=%d", VAR );
  }
  return VAR;
};

const int ARX_MAX_DAYS()
{
	static bool init=false;
  static int VAR;
  if (!init) {
    char r[100];
    r[0]=0;
    if ( get_param( "ARX_MAX_DAYS", r, sizeof( r ) ) < 0 )
      throw EXCEPTIONS::Exception( "Can't read param ARX_MAX_DAYS" );
    if (StrToInt(r,VAR)==EOF||r<=0)
      throw EXCEPTIONS::Exception("Wrong param ARX_MAX_DAYS");
    init=true;
    ProgTrace( TRACE5, "ARX_MAX_DAYS=%d", VAR );
  }
  return VAR;
};

void arx_move(int move_id, TDateTime part_key)
{
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "BEGIN "
    "  arch.move(:move_id,:part_key); "
    "END;";
  Qry.CreateVariable("move_id",otInteger,move_id);
  if (part_key!=NoExists)
  	Qry.CreateVariable("part_key",otDate,part_key);
  else
  	Qry.CreateVariable("part_key",otDate,FNull);
  Qry.Execute();
};

void arx_daily(TDateTime utcdate)
{
	//соберем статистику для тех, кто не вылетел
	TQuery Qry(&OraSession);
	Qry.Clear();
	Qry.SQLText=
	  "BEGIN "
	  "  statist.get_full_stat(:point_id); "
	  "END;";
	Qry.DeclareVariable("point_id",otInteger);


	TQuery PointsQry(&OraSession);
  PointsQry.Clear();
  PointsQry.SQLText =
    "SELECT point_id FROM points,trip_sets "
    "WHERE points.point_id=trip_sets.point_id AND "
    "      points.pr_del=0 AND trip_sets.pr_stat=0 AND "
    "      NVL(act_out,NVL(est_out,scd_out))<:stat_date";
  PointsQry.CreateVariable("stat_date",otDate,utcdate-2); //2 дня
  PointsQry.Execute();
  for(;!PointsQry.Eof;PointsQry.Next())
  {
  	Qry.SetVariable("point_id",PointsQry.FieldAsInteger("point_id"));
  	Qry.Execute();
  };
  OraSession.Commit();


  //сначала ищем рейсы из СПП которые можно переместить в архив

  Qry.Clear();
  Qry.SQLText =
    "SELECT move_id "
    "FROM points "
    "GROUP BY move_id "
    "HAVING MAX(pr_del)=-1 AND MIN(pr_del)=-1 OR "
    "       MAX(NVL(act_out,NVL(est_out,scd_out)))<:arx_date OR "
    "       MAX(NVL(act_in,NVL(est_in,scd_in)))<:arx_date";
  Qry.CreateVariable("arx_date",otDate,utcdate-ARX_MIN_DAYS());
  Qry.Execute();

  PointsQry.Clear();
  PointsQry.SQLText =
    "SELECT act_out,act_in,pr_del, "
    "       NVL(act_out,NVL(est_out,scd_out)) AS time_out, "
    "       NVL(act_in,NVL(est_in,scd_in)) AS time_in, "
    "FROM points "
    "WHERE move_id=:move_id AND pr_del<>-1"
    "ORDER BY point_num";
  PointsQry.DeclareVariable("move_id",otInteger);
  for(;!Qry.Eof;Qry.Next())
  {
    int move_id=Qry.FieldAsInteger("move_id");
    PointsQry.SetVariable("move_id",move_id);
    PointsQry.Execute();

    TDateTime last_date=NoExists;
    TDateTime prior_time_out=NoExists;
    TDateTime final_act_in=NoExists;

    if (!PointsQry.Eof)
    {
      while(!PointsQry.Eof)
      {
        if (!PointsQry.FieldIsNULL("time_out"))
          prior_time_out=PointsQry.FieldAsDateTime("time_out");
        else
          prior_time_out=NoExists;

        Qry.Next();

        if (Qry.Eof) break;

        //анализируем предыдущий time_out,act_out
        if (prior_time_out!=NoExists &&
            (last_date==NoExists || last_date<prior_time_out))
          last_date=prior_time_out;

        if (!Qry.FieldIsNULL("time_in") &&
            (last_date==NoExists || last_date<Qry.FieldAsDateTime("time_in")))
          last_date=Qry.FieldAsDateTime("time_in");

        if (Qry.FieldAsInteger("pr_del")==0)
        {
       	  if (!Qry.FieldIsNULL("act_in"))
            final_act_in=Qry.FieldAsDateTime("act_in");
          else
            final_act_in=NoExists;
        };

      };
      if (last_date!=NoExists)
      {
        if ( final_act_in!=NoExists && last_date<utcdate-ARX_MIN_DAYS() ||
             final_act_in==NoExists && last_date<utcdate-ARX_MAX_DAYS() )
        {
          //переместить в архив
          arx_move(move_id,last_date);
        };
      };
    }
    else
    {
      //переместить в архив удаленный рейс
      arx_move(move_id,NoExists);
    };
  };


  //переместим разные данные, не привязанные к рейсам
  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  arch.move(:arx_date); "
    "END;";
  Qry.CreateVariable("arx_date",otDate,utcdate-ARX_MAX_DAYS());
  Qry.Execute();


  //далее ищем разобранные данные телеграмм которые можно удалить
  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  arch.tlg_trip(:point_id); "
    "END;";
  Qry.DeclareVariable("point_id",otInteger);

  PointsQry.Clear();
  PointsQry.SQLText=
    "SELECT point_id,scd,pr_utc,system.AirpTZRegion(airp_dep,0) AS region "
    "FROM tlg_trips,tlg_binding "
    "WHERE tlg_trips.point_id=tlg_binding.point_id_tlg(+) AND tlg_binding.point_id_tlg IS NULL";
  PointsQry.Execute();
  for(;!PointsQry.Eof;PointsQry.Next())
  {
    int point_id=PointsQry.FieldAsInteger("point_id");
    bool pr_utc=PointsQry.FieldAsInteger("pr_utc")!=0;
    TDateTime scd=PointsQry.FieldAsDateTime("scd"); //NOT NULL всегда
    if (!pr_utc) scd=LocalToUTC(scd+1,PointsQry.FieldAsString("region"));

    if (scd<utcdate-ARX_MAX_DAYS())
    {
      Qry.SetVariable("point_id",point_id);
      Qry.Execute();
    };
  };

  //далее перемещаем в архив телеграммы из tlgs_in
  TQuery TlgQry(&OraSession);
  TlgQry.Clear();
  TlgQry.SQLText=
    "SELECT id "
    "FROM tlgs_in "
    "WHERE time_parse IS NOT NULL AND "
    "      NOT EXISTS(SELECT * FROM tlg_source WHERE tlg_source.tlg_id=tlgs_in.id) "
    "GROUP BY id "
    "HAVING MAX(time_parse)<:arx_date";
  TlgQry.CreateVariable("arx_date",otDate,utcdate-ARX_MAX_DAYS());

  Qry.Clear();
  Qry.SQLText=
    "SELECT id,num,body FROM tlgs_in WHERE id=:id FOR UPDATE";
  Qry.DeclareVariable("id",otInteger);

  TQuery InsQry(&OraSession);
  InsQry.Clear();
  InsQry.SQLText=
    "INSERT INTO arx_tlgs_in "
    "  (id,num,page_no,type,addr,heading,body,ending,merge_key,time_create,time_receive,time_parse,part_key) "
    "SELECT "
    "   id,num,:page_no,type,addr,heading,:body,ending,merge_key,time_create,time_receive,time_parse,time_parse "
    "FROM tlgs_in "
    "WHERE id=:id AND num=:num";
  InsQry.DeclareVariable("id",otInteger);
  InsQry.DeclareVariable("num",otInteger);
  InsQry.DeclareVariable("page_no",otInteger);
  InsQry.DeclareVariable("body",otString);

  int len,bufLen=0;
  char *ph,*buf=NULL;
  try
  {
    TlgQry.Execute();
    for(;!TlgQry.Eof;TlgQry.Next())
    {
      int id=TlgQry.FieldAsInteger("id");
      InsQry.SetVariable("id",id);
      Qry.SetVariable("id",id);
      Qry.Execute();
      for(;!Qry.Eof;Qry.Next())
      {
        len=Qry.GetSizeLongField("body")+1;
        if (len>bufLen)
        {
          if (buf==NULL)
            ph=(char*)malloc(len);
          else
            ph=(char*)realloc(buf,len);
          if (ph==NULL) throw EMemoryError("Out of memory");
          buf=ph;
          bufLen=len;
        };
        Qry.FieldAsLong("body",buf);
        buf[len-1]=0;

        string body=buf;
        int page_no=1;
        InsQry.SetVariable("num",Qry.FieldAsInteger("num"));
        for(;!body.empty();page_no++)
        {
          InsQry.SetVariable("page_no",page_no);
          if (body.size()>2000)
          {
            InsQry.SetVariable("body",body.substr(0,2000).c_str());
            InsQry.Execute();
            body.erase(0,2000);
          }
          else
          {
            InsQry.SetVariable("body",body.c_str());
            InsQry.Execute();
            body.clear();
          };
        };
      };
    };
    if (buf!=NULL) free(buf);
  }
  catch(...)
  {
    if (buf!=NULL) free(buf);
    throw;
  };

  //далее перемещаем в архив нормы, тарифы и т.п.
  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  arch.norms_rates_etc(:arx_date); "
    "END;";
  Qry.CreateVariable("arx_date",otDate,utcdate-ARX_MAX_DAYS()-15);
  Qry.Execute();

  //и наконец чистим tlgs
  Qry.Clear();
  Qry.SQLText=
    "DELETE FROM tlgs WHERE time<:arx_date";
  Qry.CreateVariable("arx_date",otDate,utcdate-30); //30 дней
  Qry.Execute();
};

