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
#include "astra_consts.h"
#include "astra_utils.h"
#include "astra_misc.h"
#include "astra_context.h"
#include "base_tables.h"
#include "astra_service.h"
#include "czech_police_edi_file.h"
#include "telegram.h"
#include "arx_daily.h"
#include "serverlib/daemon.h"
#include "serverlib/cfgproc.h"
#include "serverlib/posthooks.h"
#include "serverlib/perfom.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"
#include "serverlib/ourtime.h"

const int sleepsec = 25;

using namespace ASTRA;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace std;

int main_timer_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist)
{
  try
  {
    sleep(10);
    InitLogTime(NULL);
    OpenLogFile("log1");

    int p_count;
    string num;
    if ( TCL_OK != Tcl_ListObjLength( interp, argslist, &p_count ) ) {
    	ProgError( STDLOG,
                 "ERROR:main_timer_tcl wrong parameters:%s",
                 Tcl_GetString(Tcl_GetObjResult(interp)) );
      return 1;
    }
    if ( p_count != 2 ) {
    	ProgError( STDLOG,
                 "ERROR:main_timer_tcl wrong number of parameters:%d",
                 p_count );
    }
    else {
    	 Tcl_Obj *val;
    	 Tcl_ListObjIndex( interp, argslist, 1, &val );
    	 num = Tcl_GetString( val );
    }

    ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks()
        ->connect_db();
    if (init_edifact()<0) throw Exception("'init_edifact' error");
    for( ;; )
    {
      InitLogTime(NULL);
      PerfomInit();
      base_tables.Invalidate();
      exec_tasks( num.c_str() );
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

void exec_tasks( const char *proc_name )
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
	 " WHERE pr_denial=0 AND NVL(next_exec,:utcdate) <= :utcdate AND proc_name=:proc_name ";
	Qry.CreateVariable( "utcdate", otDate, utcdate );
	Qry.CreateVariable( "proc_name", otString, proc_name );
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
	  InitLogTime(NULL);
	  bool Result=true;

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
	    	   		if ( name == "arx_daily" ) Result = arx_daily( utcdate );
	    	  		else
	    	  			if ( name == "sync_aodb" ) sync_aodb( );
	    	  			else
	    	  			  if ( name == "sync_sirena_codes" ) sync_sirena_codes( );
	    	  			  else
	    	  			  	if ( name == "sync_sppcek" ) sync_sppcek( );
	    	  			  	else
	    	  			  		if ( name == "get_full_stat" ) get_full_stat( utcdate );
	    	  			  		else
	    	  			  			if ( name == "sync_1ccek" ) sync_1ccek();
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
      if (Result)
      {
        //если ф-ция возвратила true то вычислить время следующего выполнения
        UQry.SetVariable( "next_exec", next_exec );
  	    UQry.SetVariable( "name", name );
  	    UQry.Execute();
  	  };
	    OraSession.Commit();
	    callPostHooksAfter();
	  }
	  catch( EOracleError &E )
    {
      try { OraSession.Rollback(); } catch(...) {};
      ProgError( STDLOG, "EOracleError %d: %s", E.Code, E.what());
      ProgError( STDLOG, "SQL: %s", E.SQLText());
      ProgError( STDLOG, "task name=%s", name.c_str() );
    }
    catch( std::exception &E )
    {
      try { OraSession.Rollback(); } catch(...) {};
      ProgError( STDLOG, "std::exception: %s, task name=%s", E.what(), name.c_str() );
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
  TReqInfo *reqInfo = TReqInfo::Instance();
  reqInfo->user.sets.time = ustTimeUTC;

  TQuery Qry(&OraSession);
  try
  {
    TDateTime now=NowUTC();

    AstraContext::ClearContext("EDI_SESSION",now-1.0/48);
    AstraContext::ClearContext("TERM_REQUEST",now-1.0/48);
    AstraContext::ClearContext("EDI_HELP_INTMSGID",now-1.0/48);
    AstraContext::ClearContext("EDI_RESPONSE",now-1.0/48);

    Qry.Clear();
    Qry.SQLText="DELETE FROM edisession WHERE sessdatecr<SYSDATE-1/48";
    Qry.Execute();
    OraSession.Commit();

    TQuery UpdQry(&OraSession);
    UpdQry.SQLText=
      "BEGIN "
      "  IF :pr_etstatus IS NOT NULL THEN "
      "    UPDATE trip_sets SET pr_etstatus=:pr_etstatus WHERE point_id=:point_id; "
      "  ELSE "
      "    UPDATE trip_sets SET et_final_attempt=et_final_attempt+1 WHERE point_id=:point_id; "
      "  END IF; "
      "END;";
    UpdQry.DeclareVariable("point_id",otInteger);
    UpdQry.DeclareVariable("pr_etstatus",otInteger);

    Qry.Clear();
    Qry.SQLText=
     "SELECT p.point_id, p.airline, p.flt_no, p.airp, p.act_out, "
     "       NVL(act_in,NVL(est_in,scd_in)) AS real_in, "
     "       p.pr_etstatus,p.et_final_attempt "
     "FROM points, "
     "  (SELECT points.point_id,point_num,airline,flt_no,airp,act_out, "
     "          DECODE(pr_tranzit,0,points.point_id,first_point) AS first_point, "
     "          pr_etstatus,et_final_attempt "
     "   FROM points,trip_sets "
     "   WHERE points.point_id=trip_sets.point_id AND points.pr_del>=0 AND "
     "         (act_out IS NOT NULL AND pr_etstatus=0 OR pr_etstatus<0)) p "
     "WHERE points.first_point=p.first_point AND "
     "      points.point_num>p.point_num AND points.pr_del=0 AND "
     "      ckin.get_pr_tranzit(points.point_id)=0 AND "
     "      (NVL(act_in,NVL(est_in,scd_in))<:now AND pr_etstatus=0 OR pr_etstatus<0)";
    Qry.CreateVariable("now",otDate,now);
    Qry.Execute();

    for(;!Qry.Eof;Qry.Next(),OraSession.Rollback())
    {
      int point_id=Qry.FieldAsInteger("point_id");
      try
      {
        TTripInfo info;
        info.airline=Qry.FieldAsString("airline");
        info.flt_no=Qry.FieldAsInteger("flt_no");
        info.airp=Qry.FieldAsString("airp");
        bool pr_final=!Qry.FieldIsNULL("act_out") &&
                      !Qry.FieldIsNULL("real_in") &&
                      Qry.FieldAsDateTime("real_in")<now;


        if (pr_final &&
            (Qry.FieldAsInteger("et_final_attempt")>=5 || //не менее 5 попыток подтвердить статусы интерактивом
             GetTripSets(tsETLOnly,info)))                //либо выставлен признак запрета интерактива
          {
          //Работа с сервером эл. билетов в интерактивном режиме запрещена
          //либо же никак не хотят подтверждаться конечные статусы
          //Отправляем ETL если настроена автоотправка
          try
          {
            ProgTrace(TRACE5,"ETCheckStatusFlt.SendTlg: point_id=%d",point_id);
            vector<string>  tlg_types;
            tlg_types.push_back("ETL");
            TelegramInterface::SendTlg(point_id,tlg_types);
            UpdQry.SetVariable("point_id",point_id);
            UpdQry.SetVariable("pr_etstatus",1);
            UpdQry.Execute();
            OraSession.Commit();
          }
          catch(std::exception &E)
          {
            ProgError(STDLOG,"ETCheckStatusFlt.SendTlg (point_id=%d): %s",point_id,E.what());
          };
        }
        else
        {
          //отправим интерактивно статусы
          try
          {
          	ProgTrace(TRACE5,"ETCheckStatusFlt.ETCheckStatus: point_id=%d",point_id);
            map<TTicketListKey,TTicketListCtxt> mtick;
            ETStatusInterface::ETCheckStatus(point_id,csaFlt,point_id,true,mtick);
            if (!ETStatusInterface::ETChangeStatus(ASTRA::NoExists,mtick))
            {
              if (pr_final)
              {
                UpdQry.SetVariable("point_id",point_id);
                UpdQry.SetVariable("pr_etstatus",1);
                UpdQry.Execute();
              };
            }
            else
            {
              if (pr_final)
              {
                UpdQry.SetVariable("point_id",point_id);
                UpdQry.SetVariable("pr_etstatus",FNull); //увеличим et_final_attempt
                UpdQry.Execute();
              };
            };
            OraSession.Commit();
          }
          catch(std::exception &E)
          {
            ProgError(STDLOG,"ETCheckStatusFlt.ETCheckStatus (point_id=%d): %s",point_id,E.what());
          };
        };
      }
      catch(...)
      {
        ProgError(STDLOG,"ETCheckStatusFlt (point_id=%d): unknown error",point_id);
      };
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

void create_czech_police_file(int point_id, bool is_edi)
{
  try
  {
    TQuery FilesQry(&OraSession);
    FilesQry.Clear();
    FilesQry.SQLText=
      "SELECT name,dir "
      "FROM file_sets "
      "WHERE code=:code AND pr_denial=0";
    if (is_edi)
      FilesQry.CreateVariable("code",otString,"ЧЕШСКАЯ ПОЛИЦИЯ EDI");
    else
      FilesQry.CreateVariable("code",otString,"ЧЕШСКАЯ ПОЛИЦИЯ CSV");
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

      ostringstream flight;

      flight << airline.code_lat
    	       << setw(4) << setfill('0') << Qry.FieldAsInteger("flt_no");

      ostringstream file_name;

      if (is_edi)
        file_name << FilesQry.FieldAsString("dir") <<
           Paxlst::CreateEdiPaxlstFileName(flight.str(),
                                           airp_dep.code_lat,
                                           airp_arv.code_lat,
                                           DateTimeToStr(scd_out_local,"yyyymmdd"),
                                           FilesQry.FieldAsString("name"));
      else
      	file_name << FilesQry.FieldAsString("dir")
      	          << flight.str()
      	          << airp_dep.code_lat
      	          << airp_arv.code_lat
      	          << DateTimeToStr(scd_out_local,"yyyymmdd") << "."
      	          << FilesQry.FieldAsString("name");

    	Paxlst::PaxlstInfo paxlstInfo;

      if (is_edi)
      {
      	//информация о том, кто формирует сообщение
      	paxlstInfo.setPartyName("SIRENA-TRAVEL");
        paxlstInfo.setPhone("4959504991");
        paxlstInfo.setFax("4959504973");

        //информация об авиакомпании
        if (airline.name_lat.empty()) throw Exception("airline.name_lat empty (code=%s)",airline.code.c_str());
        paxlstInfo.setSenderName(airline.name_lat);
        paxlstInfo.setSenderCarrierCode(airline.code_lat);
        paxlstInfo.setRecipientCarrierCode("ZZ");
        string iataCode;
        if (!Paxlst::CreateIATACode(iataCode,flight.str(),act_in_local))
          throw Exception("CreateIATACode error");
        paxlstInfo.setIATAcode( iataCode );
        paxlstInfo.setFlight(flight.str());
        paxlstInfo.setDepartureAirport(airp_dep.code_lat);
        paxlstInfo.setDepartureDate(act_out_local);
        paxlstInfo.setArrivalAirport(airp_arv.code_lat);
        paxlstInfo.setArrivalDate(act_in_local);
      };

    	int count=0;
  	  ostringstream body;
  	  PaxQry.SetVariable("point_arv",PointsQry.FieldAsInteger("point_id"));
  	  PaxQry.Execute();
  	  for(;!PaxQry.Eof;PaxQry.Next(),count++)
  	  {
  	    Paxlst::PassengerInfo paxInfo;
  	    if (PaxQry.FieldIsNULL("pax_id"))
  	  	{
  	  	  if (is_edi)
          {
    	      paxInfo.setPassengerName(PaxQry.FieldAsString("name"));
    	      paxInfo.setPassengerSurname(PaxQry.FieldAsString("surname"));
    	      paxInfo.setIdNumber(PaxQry.FieldAsString("document"));
    	    }
    	    else
    	    {
  	        body << PaxQry.FieldAsString("surname") << ";"
  	  		       << PaxQry.FieldAsString("name") << ";"
  	  		       << ";;;;;" << PaxQry.FieldAsString("document") << ";;;"
  	  		       << ENDL;
  	  		};
  	    }
  	    else
  	    {
  	      int pax_id=PaxQry.FieldAsInteger("pax_id");

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

          if (is_edi)
          {
    	    	paxInfo.setPassengerName(PaxQry.FieldAsString("doc_first_name"));
    	    	if (!PaxQry.FieldIsNULL("doc_second_name"))
    	    	{
              string passengerName = paxInfo.getPassengerName();
              if (!passengerName.empty()) passengerName += " ";
              passengerName += PaxQry.FieldAsString("doc_second_name");
              paxInfo.setPassengerName( passengerName );
    	    	};
    	    	paxInfo.setPassengerSurname( PaxQry.FieldAsString("doc_surname") );

    	      string passengerSex = gender.substr(0,1);
            if (passengerSex!="M" &&
                passengerSex!="F")
    	        passengerSex = "M";
            paxInfo.setPassengerSex( passengerSex );

    	      if (!PaxQry.FieldIsNULL("birth_date"))
    	        paxInfo.setBirthDate( PaxQry.FieldAsDateTime("birth_date"));

    	      paxInfo.setDeparturePassenger(airp_dep.code_lat);
            paxInfo.setArrivalPassenger(airp_arv.code_lat);
            paxInfo.setPassengerCountry(nationality);
            //PNR
            vector<TPnrAddrItem> pnrs;
            GetPaxPnrAddr(pax_id,pnrs);
            if (!pnrs.empty())
              paxInfo.setPassengerNumber(convert_pnr_addr(pnrs.begin()->addr, 1));

            if (doc_type=="P")
              paxInfo.setPassengerType(doc_type);
            paxInfo.setIdNumber(PaxQry.FieldAsString("no"));
            if (!PaxQry.FieldIsNULL("expiry_date"))
              paxInfo.setExpirateDate(PaxQry.FieldAsDateTime("expiry_date"));
            paxInfo.setDocCountry(issue_country);
          }
          else
          {
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
  	    if (is_edi)
  	      paxlstInfo.addPassenger( paxInfo );
  	  };

      if (is_edi && !paxlstInfo.getPassengersList().empty())
      {
        string tlg,err;
  	    if (!paxlstInfo.toEdiString(tlg,err)) throw Exception(err);
  	    body << tlg;
  	  };

    	ofstream f;
      f.open(file_name.str().c_str());
      if (!f.is_open()) throw Exception("Can't open file '%s'",file_name.str().c_str());
      try
      {
        if (!is_edi)
        	f << "csv;ROSSIYA;"
        	  << airline.code_lat << setw(3) << setfill('0') << Qry.FieldAsInteger("flt_no") << ";"
      	    << airp_dep.code_lat << ";" << DateTimeToStr(act_out_local,"yyyy-mm-dd'T'hh:nn:00.0") << ";"
      	    << airp_arv.code_lat << ";" << DateTimeToStr(act_in_local,"yyyy-mm-dd'T'hh:nn:00.0") << ";"
      	    << count << ";" << ENDL;

      	f << body.str();
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


void get_full_stat(TDateTime utcdate)
{
	//соберем статистику по истечении двух дней от вылета,
	//если не проставлен признак окончательного сбора статистики pr_stat
	ProgTrace(TRACE5,"arx_daily: statist.get_full_stat(:point_id)");
	TQuery Qry(&OraSession);
	Qry.Clear();
	Qry.SQLText=
	  "BEGIN "
	  "  statist.get_full_stat(:point_id, 1); "
	  "END;";
	Qry.DeclareVariable("point_id",otInteger);


	TQuery PointsQry(&OraSession);
  PointsQry.Clear();
  PointsQry.SQLText =
    "SELECT points.point_id FROM points,trip_sets "
    "WHERE points.point_id=trip_sets.point_id AND "
    "      points.pr_del=0 AND points.pr_reg<>0 AND trip_sets.pr_stat=0 AND "
    "      time_out<:stat_date AND time_out>TO_DATE('01.01.0001','DD.MM.YYYY')";
  PointsQry.CreateVariable("stat_date",otDate,utcdate-2); //2 дня
  PointsQry.Execute();
  for(;!PointsQry.Eof;PointsQry.Next())
  {
  	Qry.SetVariable("point_id",PointsQry.FieldAsInteger("point_id"));
  	Qry.Execute();
  	OraSession.Commit();
  };
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



