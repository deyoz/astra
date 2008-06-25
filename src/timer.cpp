//---------------------------------------------------------------------------
#include <stdio.h>
#include <signal.h>
#include <fstream>
#include "timer.h"
#include "oralib.h"
#include "exceptions.h"
#include "etick.h"
#include "season.h"
#include "stages.h"
#include "tlg/tlg.h"
#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "test.h"
#include <daemon.h>
#include "astra_consts.h"
#include "astra_utils.h"
#include "base_tables.h"
#include "astra_service.h"
#include "cfgproc.h"
#include "posthooks.h"
const int sleepsec = 25;

using namespace ASTRA;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace std;

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
        base_tables.Invalidate();
        exec_tasks();
      }
      catch( std::exception &E ) {
        ProgError( STDLOG, "Exception: %s", E.what() );
      }
      catch( ... ) {
        ProgError( STDLOG, "Unknown error" );
      };
      sleep( sleepsec );
    };
  }
  catch( std::exception &E ) {
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
	 "SELECT name,last_exec,next_exec,interval FROM tasks "\
	 " WHERE pr_denial=0 AND NVL(next_exec,:utcdate) <= :utcdate ";
	Qry.CreateVariable( "utcdate", otDate, utcdate );
	Qry.Execute();
	TQuery UQry(&OraSession);
	UQry.SQLText =
	 "UPDATE tasks SET last_exec=:utcdate,next_exec=:next_exec WHERE name=:name";
	UQry.CreateVariable( "utcdate", otDate, utcdate );
	UQry.DeclareVariable( "next_exec", otDate );
	UQry.DeclareVariable( "name", otString );
	string name;
	TDateTime execTasks = NowUTC();
	while ( !Qry.Eof )
	{
	  TReqInfo::Instance()->clear();
	  emptyHookTables();

	  try {
	  	TDateTime execTask = NowUTC();
	    name = Qry.FieldAsString( "name" );
	    if ( name == "astra_timer" ) astra_timer( utcdate );
	    else
	      if ( name == "createSPP" ) createSPP( utcdate );
	    	else
	    	  if ( name == "ETCheckStatusFlt" ) ETCheckStatusFlt();
	    	    else
	    	      if ( name == "sync_mvd" ) sync_mvd();
	    	      	else
	    	      		if ( name == "arx_daily" ) arx_daily( utcdate );
	    	      			else
	    	      				if ( name == "sync_aodb" ) sync_aodb( );
	    	      				else
	    	      				  if ( name == "sync_sirena_codes" ) sync_sirena_codes( );
	    	      				  else
	    	      				  	if ( name == "sync_sppcek" ) sync_sppcek( );
      TDateTime next_exec;
      if ( Qry.FieldIsNULL( "next_exec" ) )
      	next_exec = utcdate;
      else
      	next_exec = Qry.FieldAsDateTime( "next_exec" );
      while ( next_exec <= utcdate ) {
       next_exec += (double)Qry.FieldAsInteger( "interval" )/1440.0;
      }
      if ( NowUTC() - execTask > 5.0/(1440.0*60.0) )
      	ProgTrace( TRACE5, "Attention execute task time!!!, name=%s, time=%s", name.c_str(), DateTimeToStr( NowUTC() - execTask, "nn:ss" ).c_str() );
      UQry.SetVariable( "next_exec", next_exec );
	    UQry.SetVariable( "name", name );
	    UQry.Execute();
	    OraSession.Commit();
	    callPostHooksAfter();
	  }
    catch( Exception &E )
    {
    try { OraSession.Rollback(); } catch(...) {};
      ProgError( STDLOG, "Exception: %s, task name=%s", E.what(), name.c_str() );
    }
    catch( ... )
    {
    try { OraSession.Rollback(); } catch(...) {};
      ProgError( STDLOG, "Unknown error, task name=%s", name.c_str() );
    };
    callPostHooksAlways();
	  Qry.Next();
	};
	if ( NowUTC() - execTasks > 1.0/1440.0 )
		ProgTrace( TRACE5, "Attention execute all tasks time > 1 min !!!, time=%s", DateTimeToStr( NowUTC() - execTasks, "nn:ss" ).c_str() );
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
    if (StrToInt(r,VAR)==EOF||VAR<2)
      throw EXCEPTIONS::Exception("Wrong param ARX_MIN_DAYS=%s",r);
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
    if (StrToInt(r,VAR)==EOF||VAR<2)
      throw EXCEPTIONS::Exception("Wrong param ARX_MAX_DAYS=%s",r);
    init=true;
    ProgTrace( TRACE5, "ARX_MAX_DAYS=%d", VAR );
  }
  return VAR;
};

const int CREATE_SPP_DAYS()
{
	static bool init=false;
  static int VAR;
  if (!init) {
    char r[100];
    r[0]=0;
    if ( get_param( "CREATE_SPP_DAYS", r, sizeof( r ) ) < 0 )
      throw EXCEPTIONS::Exception( "Can't read param CREATE_SPP_DAYS" );
    if (StrToInt(r,VAR)==EOF||VAR<1||VAR>9)
      throw EXCEPTIONS::Exception("Wrong param CREATE_SPP_DAYS=%s",r);
    init=true;
    ProgTrace( TRACE5, "CREATE_SPP_DAYS=%d", VAR );
  }
  return VAR;
};

void createSPP( TDateTime utcdate )
{
	utcdate += CREATE_SPP_DAYS(); //  на следующий день
	TReqInfo *reqInfo = TReqInfo::Instance();
	reqInfo->Initialize("МОВ");
	reqInfo->user.sets.time = ustTimeUTC;
	CreateSPP( utcdate );
	ProgTrace( TRACE5, "СПП получен за %s", DateTimeToStr( utcdate, "dd.mm.yy" ).c_str() );
}

void ETCheckStatusFlt(void)
{
  TQuery Qry(&OraSession);
  try
  {
    Qry.Clear();
    Qry.SQLText="DELETE FROM edisession WHERE sessdatecr<SYSDATE-1/24";
    Qry.Execute();
    OraSession.Commit();

    TQuery UpdQry(&OraSession);
    UpdQry.SQLText="UPDATE trip_sets SET pr_etstatus=1 WHERE point_id=:point_id";
    UpdQry.DeclareVariable("point_id",otInteger);
    Qry.Clear();
    Qry.SQLText=
     "SELECT p.point_id "
     "FROM points, "
     "  (SELECT points.point_id,point_num, "
     "          DECODE(pr_tranzit,0,points.point_id,first_point) AS first_point "
     "   FROM points,trip_sets "
     "   WHERE points.point_id=trip_sets.point_id AND points.pr_del>=0 AND "
     "         act_out IS NOT NULL AND pr_etstatus=0) p "
     "WHERE points.first_point=p.first_point AND "
     "      points.point_num>p.point_num AND points.pr_del=0 AND "
     "      ckin.get_pr_tranzit(points.point_id)=0 AND "
     "      NVL(act_in,NVL(est_in,scd_in))<system.UTCSYSDATE ";
    Qry.Execute();
    for(;!Qry.Eof;Qry.Next(),OraSession.Rollback())
    {
      try
      {
      	ProgTrace(TRACE5,"ETCheckStatusFlt: point_id=%d",Qry.FieldAsInteger("point_id"));
        if (!ETCheckStatus(Qry.FieldAsInteger("point_id"),csaFlt,Qry.FieldAsInteger("point_id")))
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
      "       SUBSTR(pnr,1,12) AS pnr "
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
      string tz_region = AirpTZRegion(Qry.FieldAsString("airp_dep"));

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

void create_czech_police_file(int point_id)
{
  try
  {
    TQuery FilesQry(&OraSession);
    FilesQry.Clear();
    FilesQry.SQLText=
      "SELECT name,dir "
      "FROM file_sets "
      "WHERE code=:code AND pr_denial=0";
    FilesQry.CreateVariable("code",otString,"ЧЕШСКАЯ ПОЛИЦИЯ");
    FilesQry.Execute();
    if (FilesQry.Eof) return;

  	TQuery Qry(&OraSession);
    Qry.SQLText =
      "SELECT airline,flt_no,airp,scd_out,NVL(act_out,NVL(est_out,scd_out)) AS act_out, "
      "       point_num,DECODE(pr_tranzit,0,points.point_id,first_point) AS first_point, "
      "       country "
      "FROM points,airps,cities "
      "WHERE points.airp=airps.code AND airps.city=cities.code AND "
      "      point_id=:point_id AND points.pr_del=0 AND points.pr_reg<>0 ";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.Execute();
    if (Qry.Eof) return;

    TAirlinesRow &airline = (TAirlinesRow&)base_tables.get("airlines").get_row("code",Qry.FieldAsString("airline"));
    if (airline.code_lat.empty()) throw Exception("airline.code_lat empty (code=%s)",Qry.FieldAsString("airline"));
    TAirpsRow &airp_dep = (TAirpsRow&)base_tables.get("airps").get_row("code",Qry.FieldAsString("airp"));
    if (airp_dep.code_lat.empty()) throw Exception("airp_dep.code_lat empty (code=%s)",Qry.FieldAsString("airp"));
    string tz_region=AirpTZRegion(Qry.FieldAsString("airp"));
    if (Qry.FieldIsNULL("scd_out")) throw Exception("scd_out empty (airp_dep=%s)",Qry.FieldAsString("airp"));
    TDateTime scd_out_local	= UTCToLocal(Qry.FieldAsDateTime("scd_out"),tz_region);
    if (Qry.FieldIsNULL("act_out")) throw Exception("act_out empty (airp_dep=%s)",Qry.FieldAsString("airp"));
    TDateTime act_out_local	= UTCToLocal(Qry.FieldAsDateTime("act_out"),tz_region);

    TQuery PointsQry(&OraSession);
    PointsQry.SQLText=
      "SELECT point_id,airp,scd_in,NVL(act_in,NVL(est_in,scd_in)) AS act_in,country "
      "FROM points,airps,cities "
      "WHERE points.airp=airps.code AND airps.city=cities.code AND "
      "      first_point=:first_point AND point_num>:point_num AND points.pr_del=0";
    PointsQry.CreateVariable("first_point",otInteger,Qry.FieldAsInteger("first_point"));
    PointsQry.CreateVariable("point_num",otInteger,Qry.FieldAsInteger("point_num"));

    TQuery PaxQry(&OraSession);
    PaxQry.SQLText=
      "SELECT pax_doc.pax_id, "
      "       system.transliter(pax.surname,1) AS surname, "
      "       system.transliter(pax.name,1) AS name, "
      "       DECODE(system.is_name(pax.document),0,NULL,pax.document) AS document, "
      "       system.transliter(pax_doc.surname,1) AS doc_surname, "
      "       system.transliter(pax_doc.first_name,1) AS doc_first_name, "
      "       system.transliter(pax_doc.second_name,1) AS doc_second_name, "
      "       birth_date,gender,nationality,pax_doc.type,pax_doc.no, "
      "       expiry_date,issue_country "
      "FROM pax_grp,pax,pax_doc "
      "WHERE pax_grp.grp_id=pax.grp_id AND pax.pax_id=pax_doc.pax_id(+) AND "
      "      pax_grp.point_dep=:point_dep AND pax_grp.point_arv=:point_arv AND "
      "      pr_brd=1";
    PaxQry.CreateVariable("point_dep",otInteger,point_id);
    PaxQry.DeclareVariable("point_arv",otInteger);

    PointsQry.Execute();
    for(;!PointsQry.Eof;PointsQry.Next())
    {
    	if (/*strcmp(Qry.FieldAsString("country"),"ЦЗ")!=0 &&*/
    		  strcmp(PointsQry.FieldAsString("country"),"ЦЗ")!=0) continue;

    	TAirpsRow &airp_arv = (TAirpsRow&)base_tables.get("airps").get_row("code",PointsQry.FieldAsString("airp"));
    	if (airp_arv.code_lat.empty()) throw Exception("airp_arv.code_lat empty (code=%s)",PointsQry.FieldAsString("airp"));
    	tz_region=AirpTZRegion(Qry.FieldAsString("airp"));

      if (PointsQry.FieldIsNULL("act_in")) throw Exception("act_in empty (airp_arv=%s)",PointsQry.FieldAsString("airp"));
      TDateTime act_in_local = UTCToLocal(PointsQry.FieldAsDateTime("act_in"),tz_region);

    	ostringstream file_name;
    	file_name << FilesQry.FieldAsString("dir")
    	          << airline.code_lat
    	          << setw(4) << setfill('0') << Qry.FieldAsInteger("flt_no")
    	          << airp_dep.code_lat << airp_arv.code_lat
    	          << DateTimeToStr(scd_out_local,"yyyymmdd")
    	          << FilesQry.FieldAsString("name");

    	int count=0;
  	  ostringstream body;
  	  PaxQry.SetVariable("point_arv",PointsQry.FieldAsInteger("point_id"));
  	  PaxQry.Execute();
  	  for(;!PaxQry.Eof;PaxQry.Next(),count++)
  	  {
  	  	if (PaxQry.FieldIsNULL("pax_id"))
  	  	{
  	  		body << PaxQry.FieldAsString("surname") << ";"
  	  		     << PaxQry.FieldAsString("name") << ";"
  	  		     << ";;;;;" << PaxQry.FieldAsString("document") << ";;;"
  	  		     << ENDL;
  	    }
  	    else
  	    {
  	      string gender;
  	      if (!PaxQry.FieldIsNULL("gender"))
  	      {
  	    	  TGenderTypesRow &gender_row = (TGenderTypesRow&)base_tables.get("gender_types").get_row("code",PaxQry.FieldAsString("gender"));
  	    	  if (gender_row.code_lat.empty()) throw Exception("gender.code_lat empty (code=%s)",PaxQry.FieldAsString("gender"));
  	    	  gender=gender_row.code_lat;
  	    	};
  	    	string doc_type;
  	    	if (!PaxQry.FieldIsNULL("type"))
  	    	{
  	    	  TPaxDocTypesRow &doc_type_row = (TPaxDocTypesRow&)base_tables.get("pax_doc_types").get_row("code",PaxQry.FieldAsString("type"));
  	    	  if (doc_type_row.code_lat.empty()) throw Exception("doc_type.code_lat empty (code=%s)",PaxQry.FieldAsString("type"));
  	    	  doc_type=doc_type_row.code_lat;
  	    	};
  	    	string nationality;
  	    	if (!PaxQry.FieldIsNULL("nationality"))
  	    	{
  	    	  TCountriesRow &nationality_row = (TCountriesRow&)base_tables.get("countries").get_row("code",PaxQry.FieldAsString("nationality"));
  	    	  if (nationality_row.code_iso.empty()) throw Exception("nationality.code_iso empty (code=%s)",PaxQry.FieldAsString("nationality"));
  	    	  nationality=nationality_row.code_iso;
  	    	};
  	    	string issue_country;
  	    	if (!PaxQry.FieldIsNULL("issue_country"))
  	    	{
  	    	  TCountriesRow &issue_country_row = (TCountriesRow&)base_tables.get("countries").get_row("code",PaxQry.FieldAsString("issue_country"));
  	    	  if (issue_country_row.code_iso.empty()) throw Exception("issue_country.code_iso empty (code=%s)",PaxQry.FieldAsString("issue_country"));
  	    	  issue_country=issue_country_row.code_iso;
  	    	};
  	    	string birth_date;
  	    	if (!PaxQry.FieldIsNULL("birth_date"))
  	    	  birth_date=DateTimeToStr(PaxQry.FieldAsDateTime("birth_date"),"ddmmmyy",true);

  	    	string expiry_date;
  	    	if (!PaxQry.FieldIsNULL("expiry_date"))
  	    	  expiry_date=DateTimeToStr(PaxQry.FieldAsDateTime("expiry_date"),"ddmmmyy",true);

  	    	body << PaxQry.FieldAsString("doc_surname") << ";"
  	  		     << PaxQry.FieldAsString("doc_first_name") << ";"
  	  		     << PaxQry.FieldAsString("doc_second_name") << ";"
  	  		     << birth_date << ";"
  	  		     << gender << ";"
  	  		     << nationality << ";"
  	  		     << doc_type << ";"
  	  		     << PaxQry.FieldAsString("no") << ";"
  	  		     << expiry_date << ";"
  	  		     << issue_country << ";"
  	  		     << ENDL;
  	    };
  	  };

    	ofstream f;
      f.open(file_name.str().c_str());
      if (!f.is_open()) throw Exception("Can't open file '%s'",file_name.str().c_str());
      try
      {
      	f << "csv;ROSSIYA;"
      	  << airline.code_lat << setw(3) << setfill('0') << Qry.FieldAsInteger("flt_no") << ";"
    	    << airp_dep.code_lat << ";" << DateTimeToStr(act_out_local,"yyyy-mm-dd'T'hh:nn:00.0") << ";"
    	    << airp_arv.code_lat << ";" << DateTimeToStr(act_in_local,"yyyy-mm-dd'T'hh:nn:00.0") << ";"
    	    << count << ";" << ENDL
    	    << body.str();

      	f.close();
      }
      catch(...)
      {
        try { f.close(); } catch( ... ) { };
        try
        {
          //в случае ошибки запишем пустой файл
          f.open(file_name.str().c_str());
          if (f.is_open()) f.close();
        }
        catch( ... ) { };
        throw;
      };

    };
  }
  catch(Exception &E)
  {
    throw Exception("create_czech_police_file: %s",E.what());
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
    "UPDATE file_sets SET last_create=:now WHERE code=:code AND "
    "                 (airp=:airp OR airp IS NULL AND :airp IS NULL)";
  Qry.DeclareVariable("code",otString);
  Qry.DeclareVariable("airp",otString);
  Qry.DeclareVariable("now",otDate);

  TQuery FilesQry(&OraSession);
  FilesQry.Clear();
  FilesQry.SQLText=
    "SELECT name,dir,last_create,airp "
    "FROM file_sets "
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
      string tz_region;
      if (!FilesQry.FieldIsNULL("airp"))
      {
        tz_region=AirpTZRegion(FilesQry.FieldAsString("airp"));
        local=UTCToLocal(now,tz_region);
      }
      else
      {
        tz_region.clear();
        local=now;
      };
      ostringstream file_name;
      file_name << FilesQry.FieldAsString("dir")
                << DateTimeToStr(local,FilesQry.FieldAsString("name"));

      if (FilesQry.FieldIsNULL("last_create"))
        create_mvd_file(now-1,now,
                        FilesQry.FieldAsString("airp"),
                        tz_region.c_str(),
                        file_name.str().c_str());
      else
        create_mvd_file(FilesQry.FieldAsDateTime("last_create"),now,
                        FilesQry.FieldAsString("airp"),
                        tz_region.c_str(),
                        file_name.str().c_str());

      Qry.SetVariable("airp",FilesQry.FieldAsString("airp"));
      Qry.Execute();
      OraSession.Commit();
    };
  };
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
  OraSession.Commit();
};

void arx_daily(TDateTime utcdate)
{
	ProgTrace(TRACE5,"arx_daily started");

	//соберем статистику для тех, кто не вылетел
	ProgTrace(TRACE5,"arx_daily: statist.get_full_stat(:point_id)");
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
    "SELECT points.point_id FROM points,trip_sets "
    "WHERE points.point_id=trip_sets.point_id AND "
    "      points.pr_del=0 AND points.pr_reg<>0 AND trip_sets.pr_stat=0 AND "
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
  ProgTrace(TRACE5,"arx_daily: arch.move(:move_id,:part_key)");
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
    "       NVL(act_in,NVL(est_in,scd_in)) AS time_in "
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

        PointsQry.Next();

        if (PointsQry.Eof) break;

        //анализируем предыдущий time_out,act_out
        if (prior_time_out!=NoExists &&
            (last_date==NoExists || last_date<prior_time_out))
          last_date=prior_time_out;

        if (!PointsQry.FieldIsNULL("time_in") &&
            (last_date==NoExists || last_date<PointsQry.FieldAsDateTime("time_in")))
          last_date=PointsQry.FieldAsDateTime("time_in");

        if (PointsQry.FieldAsInteger("pr_del")==0)
        {
       	  if (!PointsQry.FieldIsNULL("act_in"))
            final_act_in=PointsQry.FieldAsDateTime("act_in");
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
  ProgTrace(TRACE5,"arx_daily: arch.move(:arx_date)");
  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  arch.move(:arx_date); "
    "END;";
  Qry.CreateVariable("arx_date",otDate,utcdate-ARX_MAX_DAYS());
  Qry.Execute();
  OraSession.Commit();

  //далее ищем разобранные данные телеграмм которые можно удалить
  ProgTrace(TRACE5,"arx_daily: arch.tlg_trip(:point_id)");
  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  arch.tlg_trip(:point_id); "
    "END;";
  Qry.DeclareVariable("point_id",otInteger);

  PointsQry.Clear();
  PointsQry.SQLText=
    "SELECT point_id,scd,pr_utc,airp_dep "
    "FROM tlg_trips,tlg_binding "
    "WHERE tlg_trips.point_id=tlg_binding.point_id_tlg(+) AND tlg_binding.point_id_tlg IS NULL AND "
    "      tlg_trips.scd<:arx_date";
  PointsQry.CreateVariable("arx_date",otDate,utcdate-ARX_MAX_DAYS());
  PointsQry.Execute();
  for(;!PointsQry.Eof;PointsQry.Next())
  {
    int point_id=PointsQry.FieldAsInteger("point_id");
    bool pr_utc=PointsQry.FieldAsInteger("pr_utc")!=0;
    TDateTime scd=PointsQry.FieldAsDateTime("scd"); //NOT NULL всегда
    if (!pr_utc)
      scd=LocalToUTC(scd+1,AirpTZRegion(PointsQry.FieldAsString("airp_dep")));

    if (scd<utcdate-ARX_MAX_DAYS())
    {
      Qry.SetVariable("point_id",point_id);
      Qry.Execute();
      OraSession.Commit();
    };
  };

  //далее перемещаем в архив телеграммы из tlgs_in
  ProgTrace(TRACE5,"arx_daily: tlgs_in -> arx_tlgs_in");
  TQuery TlgQry(&OraSession);
  TlgQry.Clear();
  TlgQry.SQLText=
    "SELECT id FROM "
    "  (SELECT DISTINCT id "
    "   FROM tlgs_in "
    "   WHERE time_receive<:arx_date) "
    "WHERE NOT EXISTS(SELECT * FROM tlg_source WHERE tlg_source.tlg_id=id) ";
 /*   "SELECT id "
    "FROM tlgs_in "
    "WHERE time_parse IS NOT NULL AND "
    "      NOT EXISTS(SELECT * FROM tlg_source WHERE tlg_source.tlg_id=tlgs_in.id) "
    "GROUP BY id "
    "HAVING MAX(time_parse)<:arx_date";*/
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

  TQuery DelQry(&OraSession);
  DelQry.Clear();
  DelQry.SQLText="DELETE FROM tlgs_in WHERE id=:id";
  DelQry.DeclareVariable("id",otInteger);

  int len,bufLen=0;
  char *ph,*buf=NULL;
  try
  {
    TlgQry.Execute();
    for(;!TlgQry.Eof;TlgQry.Next())
    {
      int id=TlgQry.FieldAsInteger("id");
      InsQry.SetVariable("id",id);
      DelQry.SetVariable("id",id);
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
      DelQry.Execute();
      OraSession.Commit();
    };
    if (buf!=NULL) free(buf);
  }
  catch(...)
  {
    if (buf!=NULL) free(buf);
    throw;
  };
  OraSession.Commit();

  //далее перемещаем в архив нормы, тарифы и т.п.
  ProgTrace(TRACE5,"arx_daily: arch.norms_rates_etc(:arx_date)");
  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  arch.norms_rates_etc(:arx_date); "
    "END;";
  Qry.CreateVariable("arx_date",otDate,utcdate-ARX_MAX_DAYS()-15);
  Qry.Execute();
  OraSession.Commit();

  //и наконец чистим tlgs, files и т.п.
  ProgTrace(TRACE5,"arx_daily: clear tlgs");
  Qry.Clear();
  Qry.SQLText=
    "BEGIN "
    "  arch.tlgs_files_etc(:arx_date); "
    "END;";
  Qry.CreateVariable("arx_date",otDate,utcdate-ARX_MAX_DAYS());
  Qry.Execute();
  OraSession.Commit();

  ProgTrace(TRACE5,"arx_daily stopped");
};

#include <boost/date_time/local_time/local_time.hpp>
using namespace boost::local_time;

void sync_sirena_codes( void )
{
	ProgTrace(TRACE5,"sync_sirena_codes started");

	//вычисляем признак летней/зимней навигации
	bool pr_summer=false;
	string tz_region=CityTZRegion("МОВ");
  tz_database &tz_db = get_tz_database();
  time_zone_ptr tz = tz_db.time_zone_from_region( tz_region );
  if (tz==NULL) throw Exception("Region '%s' not found",tz_region.c_str());
  if (tz->has_dst())
  {
    local_date_time ld(DateTimeToBoost(NowUTC()),tz);
    pr_summer=ld.is_dst();
  };

	TQuery Qry(&OraSession);
	Qry.Clear();
  Qry.SQLText= //04068
    "BEGIN "
    "  utils.sync_sirena_codes(:pr_summer); "
    "END;";
  Qry.CreateVariable("pr_summer",otInteger,(int)pr_summer);
  Qry.Execute();

  OraSession.Commit();
	ProgTrace(TRACE5,"sync_sirena_codes stopped");
};


