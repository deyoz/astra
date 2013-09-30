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
#include "misc.h"
#include "astra_misc.h"
#include "astra_context.h"
#include "base_tables.h"
#include "astra_service.h"
#include "apis_edi_file.h"
#include "telegram.h"
#include "arx_daily.h"
#include "base_tables.h"
#include "stl_utils.h"
#include "empty_proc.h"
#include "basel_aero.h"
#include "rozysk.h"
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
    ProgError( STDLOG, "std::exception: %s", E.what() );
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
	    else
      if ( name == "sync_checkin_data" ) sync_checkin_data( );
      else
      if ( name == "sych_basel_aero_stat" ) sych_basel_aero_stat( utcdate );
      else
      if ( name == "sync_sirena_rozysk" ) sync_sirena_rozysk( utcdate );
      else
      if ( name == "mintrans" ) save_mintrans_files();
      else
      if ( name == "utg" ) utg();
      else
      if ( name == "den" ) utg_prl_tst();
/*	  else
      if ( name == "cobra" ) cobra();*/

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
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("CREATE_SPP_DAYS",1,9,NoExists);
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

#include <boost/filesystem.hpp>

void utg_prl_tst(void)
{
    static bool processed = false;
    static bool passed = false;
    if(processed) {
        if(not passed) {
            passed = true;
            ProgTrace(TRACE5, "utg_prl_tst passed");
        }
        return;
    }
    processed = true;

    static TQuery pointsQry(&OraSession);
    if (pointsQry.SQLText.IsEmpty()) {
        pointsQry.Clear();
        pointsQry.SQLText =
            "SELECT ckin_open.point_id "
            "FROM trip_stages ckin_open, "
            "     trip_stages ckin_close "
            "WHERE ckin_open.point_id=ckin_close.point_id AND "
//            "      ckin_open.point_id = 2758830 and " //!!! test
            "      ckin_open.stage_id=20 AND "
            "      ckin_close.stage_id=30 AND "
            "      NVL(ckin_open.act, NVL(ckin_open.est, ckin_open.scd))<:time AND "
            "      NVL(ckin_close.act, NVL(ckin_close.est, ckin_close.scd))>=:time ";
        pointsQry.DeclareVariable("time", otDate);
    }

    static TQuery TlgQry(&OraSession);
    if(TlgQry.SQLText.IsEmpty()) {
        TlgQry.Clear();
        TlgQry.SQLText=
            "SELECT id, num, type, point_id, addr AS addrs, heading, body, ending, "
            "       completed, has_errors, time_create, originator_id, airline_mark "
            "FROM tlg_out "
            "WHERE id=:id FOR UPDATE";
        TlgQry.DeclareVariable( "id", otInteger);
    }

    static TQuery updQry(&OraSession);
    if(updQry.SQLText.IsEmpty()) {
        updQry.Clear();
        updQry.SQLText=
            "UPDATE tlg_out SET time_send_act=system.UTCSYSDATE WHERE id=:id";
        updQry.DeclareVariable( "id", otInteger);
    }

    static TQuery Qry(&OraSession);
    if(Qry.SQLText.IsEmpty()) {
        Qry.Clear();
        Qry.SQLText="SELECT airline, flt_no, suffix, airp, scd_out FROM points WHERE point_id=:point_id";
        Qry.DeclareVariable("point_id", otInteger);
    }

    TDateTime time;
    StrToDateTime("01.08.2013 00:00:00", time);
    pointsQry.SetVariable("time", time);
    pointsQry.Execute();
    TPerfTimer tm_many("many tlgs");
    tm_many.Init();
    int count = 0;
    TStats stats;
    for(; not pointsQry.Eof; pointsQry.Next(), count++) {
        //        if(count == 10) break;
        TPerfTimer tm("one tlg");
        tm.Init();
        TypeB::TCreateInfo info("PRL");
        info.point_id = pointsQry.FieldAsInteger("point_id");
        TTypeBTypesRow tlgTypeInfo;
        try {

            int tlg_id = TelegramInterface::create_tlg(info, tlgTypeInfo, stats);
            OraSession.Rollback(); //!!!
            /*
            TlgQry.SetVariable("id", tlg_id);
            TlgQry.Execute();

            string tlg_text=(string)TlgQry.FieldAsString("heading")+
            TlgQry.FieldAsString("body")+
            TlgQry.FieldAsString("ending");

            TTripInfo fltInfo;
            Qry.SetVariable("point_id", info.point_id);
            Qry.Execute();
            fltInfo.Init(Qry);
            putUTG(tlg_id, "PRL", fltInfo, tlg_text);
            updQry.SetVariable("id", tlg_id);
            updQry.Execute();
            */
            ProgTrace(TRACE5, "utg_prl_tst %s", tm.PrintWithMessage().c_str());
            ProgTrace(TRACE5, "utg_prl_tst: sending %d", tlg_id);

        }
        catch( std::exception &E )
        {
            OraSession.Rollback();
            ProgTrace(TRACE5, "utg_prl_tst: failed for point_id %d: %s", info.point_id, E.what());
        }
        ProgTrace(TRACE5, "utg_prl_tst: count %d", count);
    }
    OraSession.Rollback(); //!!!
    ProgTrace(TRACE5, "utg_prl_tst count: %d, %s", count, tm_many.PrintWithMessage().c_str());
    stats.dump();
}

void utg(void)
{
    static TQuery paramQry(&OraSession);
    if(paramQry.SQLText.IsEmpty()) {
        paramQry.SQLText = "select name, value from file_params where id = :id";
        paramQry.DeclareVariable("id", otInteger);
    }
    static TQuery completeQry(&OraSession);
    if(completeQry.SQLText.IsEmpty()) {
        completeQry.SQLText="UPDATE tlg_out SET completed=1 WHERE id=:id";
        completeQry.DeclareVariable("id",otInteger);
    }
    static TQuery TlgQry(&OraSession);
    if (TlgQry.SQLText.IsEmpty()) {
        TlgQry.Clear();
        TlgQry.SQLText =
            "select file_queue.id, files.data from file_queue, files where "
            "   file_queue.type = :type and "
            "   file_queue.sender = :sender and "
            "   file_queue.receiver = :receiver and "
            "   file_queue.status = :status and "
            "   file_queue.id = files.id "
            "order by file_queue.id ";
        TlgQry.CreateVariable("type", otString, FILE_UTG_TYPE);
        TlgQry.CreateVariable("sender", otString, OWN_POINT_ADDR());
        TlgQry.CreateVariable("receiver", otString, OWN_POINT_ADDR());
        TlgQry.CreateVariable("status", otString, "PUT");
    }
    TlgQry.Execute();
    int trace_count=0;
    static TMemoryManager mem(STDLOG);
    static char *p = NULL;
    static int p_len = 0;
    for(; not TlgQry.Eof; trace_count++, TlgQry.Next(), OraSession.Commit()) {
        int id = TlgQry.FieldAsInteger("id");
        try {
            int len = TlgQry.GetSizeLongField( "data" );
            if (len > p_len)
            {
                char *ph = NULL;
                if (p==NULL) {
                    ph=(char*)mem.malloc(len, STDLOG);
                } else {
                    ph=(char*)mem.realloc(p,len, STDLOG);
                }
                if (ph==NULL) throw EMemoryError("Out of memory");
                p=ph;
                p_len=len;
            };
            TlgQry.FieldAsLong( "data", p );
            string data( (char*)p, len );

            map<string, string> fileparams;
            paramQry.SetVariable("id", id);
            paramQry.Execute();
            string work_dir, file_name;
            for(; not paramQry.Eof; paramQry.Next()) {
                if(paramQry.FieldAsString("name") == PARAM_WORK_DIR)
                    work_dir = paramQry.FieldAsString("value");
                if(paramQry.FieldAsString("name") == PARAM_FILE_NAME)
                    file_name = paramQry.FieldAsString("value");
            }
            if(work_dir.empty())
                throw Exception("work_dir not specified");
            if(file_name.empty())
                throw Exception("file_name not specified");
            boost::filesystem::path apath(work_dir);
            if(not boost::filesystem::exists(apath))
                throw Exception("utg: directory '%s' not exists", work_dir.c_str());
            ofstream f;
            file_name = work_dir + "/" + file_name;
            apath = file_name;
            if(boost::filesystem::exists(apath))
                throw Exception("utg: file '%s' already exists");
            f.open(file_name.c_str());
            if(!f.is_open()) throw Exception("Can't open file '%s'", file_name.c_str());
            f << data;
            f.close();
            deleteFile(id);
        } catch(Exception &E) {
            OraSession.Rollback();
            try
            {
                EOracleError *orae=dynamic_cast<EOracleError*>(&E);
                if (orae!=NULL&&
                        (orae->Code==4061||orae->Code==4068)) continue;
                ProgError(STDLOG,"Exception: %s (file id=%d)",E.what(),id);
            }
            catch(...) {};

        } catch(...) {
            OraSession.Rollback();
            ProgError(STDLOG, "Something goes wrong");
        }
    }
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


    TQuery ETQry(&OraSession);
    ETQry.Clear();
    ETQry.SQLText=
      "SELECT points.point_id,point_num, "
      "       DECODE(pr_tranzit,0,points.point_id,first_point) AS first_point, "
      "       pr_etstatus "
      "FROM points,trip_sets "
      "WHERE points.point_id=trip_sets.point_id AND points.pr_del>=0 AND "
      "      (act_out IS NOT NULL AND pr_etstatus=0 OR pr_etstatus<0)";
      

    Qry.Clear();
    Qry.SQLText=
      "SELECT 1 "
      "FROM points "
      "WHERE points.first_point=:first_point AND "
      "      points.point_num>:point_num AND points.pr_del=0 AND "
      "      ckin.get_pr_tranzit(points.point_id)=0 AND "
      "      (NVL(act_in,NVL(est_in,scd_in))<:now AND :pr_etstatus=0 OR :pr_etstatus<0) ";
    Qry.DeclareVariable("first_point", otInteger);
    Qry.DeclareVariable("point_num", otInteger);
    Qry.DeclareVariable("pr_etstatus", otInteger);
    Qry.CreateVariable("now",otDate,now);
    
    ETQry.Execute();
    for(;!ETQry.Eof;ETQry.Next(),OraSession.Rollback())
    {
      Qry.SetVariable("first_point", ETQry.FieldAsInteger("first_point"));
      Qry.SetVariable("point_num", ETQry.FieldAsInteger("point_num"));
      Qry.SetVariable("pr_etstatus", ETQry.FieldAsInteger("pr_etstatus"));
      Qry.Execute();

      if (Qry.Eof) continue;

      int point_id=ETQry.FieldAsInteger("point_id");
      try
      {
        ETStatusInterface::TFltParams fltParams;
        fltParams.get(point_id);

        if (fltParams.in_final_status &&
            (fltParams.et_final_attempt>=5 || //не менее 5 попыток подтвердить статусы интерактивом
             fltParams.etl_only))                //либо выставлен признак запрета интерактива
        {
          //Работа с сервером эл. билетов в интерактивном режиме запрещена
          //либо же никак не хотят подтверждаться конечные статусы
          //Отправляем ETL если настроена автоотправка
          try
          {
            ProgTrace(TRACE5,"ETCheckStatusFlt.SendTlg: point_id=%d",point_id);
            vector<TypeB::TCreateInfo> createInfo;
            TypeB::TETLCreator(point_id).getInfo(createInfo);
            TelegramInterface::SendTlg(createInfo);
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
            TChangeStatusList mtick;
            ETStatusInterface::ETCheckStatus(point_id,csaFlt,point_id,true,mtick);
            if (!ETStatusInterface::ETChangeStatus(ASTRA::NoExists,mtick))
            {
              if (fltParams.in_final_status)
              {
                UpdQry.SetVariable("point_id",point_id);
                UpdQry.SetVariable("pr_etstatus",1);
                UpdQry.Execute();
              };
            }
            else
            {
              if (fltParams.in_final_status)
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
    ETQry.Close();
    UpdQry.Close();
  }
  catch(...)
  {
    try { OraSession.Rollback( ); } catch( ... ) { };
    throw;
  };
};

#define ENDL "\r\n"

const char* APIS_PARTY_INFO()
{
  static string VAR;
  if ( VAR.empty() )
    VAR=getTCLParam("APIS_PARTY_INFO","");
  return VAR.c_str();
};

#define MAX_PAX_PER_EDI_PART 15
#define MAX_LEN_OF_EDI_PART 3000

void create_apis_nosir_help(const char *name)
{
  printf("  %-15.15s ", name);
  puts("<points.point_id>  ");
};

int create_apis_nosir(int argc,char **argv)
{
  TQuery Qry(&OraSession);
  int point_id=ASTRA::NoExists;
  try
  {
    //проверяем параметры
    if (argc<2) throw EConvertError("wrong parameters");
    point_id = ToInt(argv[1]);
    Qry.Clear();
    Qry.SQLText="SELECT point_id FROM points WHERE point_id=:point_id";
    Qry.CreateVariable("point_id", otInteger, point_id);
    Qry.Execute();
    if (Qry.Eof) throw EConvertError("point_id not found");
  }
  catch(EConvertError &E)
  {
    printf("Error: %s\n", E.what());
    if (argc>0)
    {
      puts("Usage:");
      create_apis_nosir_help(argv[0]);
      puts("Example:");
      printf("  %s 1234567\n",argv[0]);
    };
    return 1;
  };

  if (init_edifact()<0) throw Exception("'init_edifact' error");
  create_apis_file(point_id);

  puts("create_apis successfully completed");
  return 0;
};

void create_apis_file(int point_id)
{
  try
  {
  	TQuery Qry(&OraSession);
    Qry.SQLText =
      "SELECT airline,flt_no,suffix,airp,scd_out,NVL(act_out,NVL(est_out,scd_out)) AS act_out, "
      "       point_num, first_point, pr_tranzit, "
      "       country "
      "FROM points,airps,cities "
      "WHERE points.airp=airps.code AND airps.city=cities.code AND "
      "      point_id=:point_id AND points.pr_del=0 AND points.pr_reg<>0 ";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.Execute();
    if (Qry.Eof) return;
    
    TAirlinesRow &airline = (TAirlinesRow&)base_tables.get("airlines").get_row("code",Qry.FieldAsString("airline"));
    string country_dep = Qry.FieldAsString("country");

    TTripRoute route;
    route.GetRouteAfter(NoExists,
                        point_id,
                        Qry.FieldAsInteger("point_num"),
                        Qry.FieldIsNULL("first_point")?NoExists:Qry.FieldAsInteger("first_point"),
                        Qry.FieldAsInteger("pr_tranzit")!=0,
                        trtNotCurrent, trtNotCancelled);

    TQuery RouteQry(&OraSession);
    RouteQry.SQLText=
      "SELECT airp,scd_in,NVL(act_in,NVL(est_in,scd_in)) AS act_in,country "
      "FROM points,airps,cities "
      "WHERE points.airp=airps.code AND airps.city=cities.code AND point_id=:point_id";
    RouteQry.DeclareVariable("point_id",otInteger);
    
    TQuery ApisSetsQry(&OraSession);
    ApisSetsQry.Clear();
    ApisSetsQry.SQLText=
      "SELECT dir,edi_addr,edi_own_addr,format "
      "FROM apis_sets "
      "WHERE airline=:airline AND country_dep=:country_dep AND country_arv=:country_arv AND pr_denial=0";
    ApisSetsQry.CreateVariable("airline", otString, airline.code);
    ApisSetsQry.CreateVariable("country_dep", otString, country_dep);
    ApisSetsQry.DeclareVariable("country_arv", otString);
    
    TQuery PaxQry(&OraSession);
    PaxQry.SQLText=
      "SELECT pax_doc.pax_id AS doc_pax_id, pax_doco.pax_id AS doco_pax_id, "
      "       system.transliter(pax.surname,1,1) AS surname, "
      "       system.transliter(pax.name,1,1) AS name, "
      "       system.transliter(pax_doc.surname,1,1) AS doc_surname, "
      "       system.transliter(pax_doc.first_name,1,1) AS doc_first_name, "
      "       system.transliter(pax_doc.second_name,1,1) AS doc_second_name, "
      "       pax_doc.type AS doc_type, pax_doc.issue_country, pax_doc.no AS doc_no, "
      "       pax_doc.nationality, pax_doc.birth_date, pax_doc.gender, pax_doc.expiry_date, pax_doc.pr_multi, "
      "       pax_doco.birth_place, pax_doco.type AS doco_type, pax_doco.no AS doco_no, "
      "       pax_doco.issue_place, pax_doco.issue_date, pax_doco.applic_country, pax_doco.pr_inf, "
      "       tckin_segments.airp_arv AS airp_final "
      "FROM pax_grp,pax,pax_doc,pax_doco,tckin_segments "
      "WHERE pax_grp.grp_id=pax.grp_id AND "
      "      pax.pax_id=pax_doc.pax_id(+) AND "
      "      pax.pax_id=pax_doco.pax_id(+) AND "
      "      pax_grp.grp_id=tckin_segments.grp_id(+) AND tckin_segments.pr_final(+)<>0 AND "
      "      pax_grp.point_dep=:point_dep AND pax_grp.point_arv=:point_arv AND "
      "      pr_brd=1";
    PaxQry.CreateVariable("point_dep",otInteger,point_id);
    PaxQry.DeclareVariable("point_arv",otInteger);
    
    for(TTripRoute::const_iterator r=route.begin(); r!=route.end(); r++)
    {
      //получим информацию по пункту маршрута
      RouteQry.SetVariable("point_id",r->point_id);
      RouteQry.Execute();
      if (RouteQry.Eof) continue;

      string country_arv=RouteQry.FieldAsString("country");
      //получим информацию по настройке APIS
      ApisSetsQry.SetVariable("country_arv",country_arv);
      ApisSetsQry.Execute();
      if (!ApisSetsQry.Eof)
      {
        if (airline.code_lat.empty()) throw Exception("airline.code_lat empty (code=%s)",airline.code.c_str());
        int flt_no=Qry.FieldAsInteger("flt_no");
        string suffix;
        if (!Qry.FieldIsNULL("suffix"))
        {
          TTripSuffixesRow &suffixRow = (TTripSuffixesRow&)base_tables.get("trip_suffixes").get_row("code",Qry.FieldAsString("suffix"));
          if (suffixRow.code_lat.empty()) throw Exception("suffixRow.code_lat empty (code=%s)",suffixRow.code.c_str());
          suffix=suffixRow.code_lat;
        };
        TAirpsRow &airp_dep = (TAirpsRow&)base_tables.get("airps").get_row("code",Qry.FieldAsString("airp"));
        if (airp_dep.code_lat.empty()) throw Exception("airp_dep.code_lat empty (code=%s)",airp_dep.code.c_str());
        string tz_region=AirpTZRegion(airp_dep.code);
        if (Qry.FieldIsNULL("scd_out")) throw Exception("scd_out empty (airp_dep=%s)",airp_dep.code.c_str());
        TDateTime scd_out_local	= UTCToLocal(Qry.FieldAsDateTime("scd_out"),tz_region);
        if (Qry.FieldIsNULL("act_out")) throw Exception("act_out empty (airp_dep=%s)",airp_dep.code.c_str());
        TDateTime act_out_local	= UTCToLocal(Qry.FieldAsDateTime("act_out"),tz_region);
      
        TAirpsRow &airp_arv = (TAirpsRow&)base_tables.get("airps").get_row("code",RouteQry.FieldAsString("airp"));
      	if (airp_arv.code_lat.empty()) throw Exception("airp_arv.code_lat empty (code=%s)",airp_arv.code.c_str());
      	tz_region=AirpTZRegion(airp_arv.code);
        if (RouteQry.FieldIsNULL("scd_in")) throw Exception("scd_in empty (airp_arv=%s)",airp_arv.code.c_str());
        TDateTime act_in_local = UTCToLocal(RouteQry.FieldAsDateTime("scd_in"),tz_region);

        for(;!ApisSetsQry.Eof;ApisSetsQry.Next())
        {
          string fmt=ApisSetsQry.FieldAsString("format");
          
          string airline_country;
          if (fmt=="TXT_EE")
          {
            if (airline.city.empty()) throw Exception("airline.city empty (code=%s)",airline.code.c_str());
            TCitiesRow &airlineCityRow = (TCitiesRow&)base_tables.get("cities").get_row("code",airline.city);
            TCountriesRow &airlineCountryRow = (TCountriesRow&)base_tables.get("countries").get_row("code",airlineCityRow.country);
   	    	  if (airlineCountryRow.code_iso.empty()) throw Exception("airlineCountryRow.code_iso empty (code=%s)",airlineCityRow.country.c_str());
   	    	  airline_country = airlineCountryRow.code_iso;
          };

        	Paxlst::PaxlstInfo paxlstInfo;

          if (fmt=="EDI_CZ" || fmt=="EDI_CN" || fmt=="EDI_IN")
          {
            if (fmt=="EDI_CN" || fmt=="EDI_IN") paxlstInfo.settings().setRespAgnCode("ZZZ");
            //информация о том, кто формирует сообщение
            vector<string> strs;
            SeparateString(string(APIS_PARTY_INFO()), ':', strs);
            vector<string>::const_iterator i;
            i=strs.begin();
            if (i!=strs.end()) paxlstInfo.setPartyName(*i++);
            if (i!=strs.end()) paxlstInfo.setPhone(*i++);
            if (i!=strs.end()) paxlstInfo.setFax(*i++);

            SeparateString(string(ApisSetsQry.FieldAsString("edi_own_addr")), ':', strs);
            i=strs.begin();
            if (i!=strs.end()) paxlstInfo.setSenderName(*i++);
            if (i!=strs.end()) paxlstInfo.setSenderCarrierCode(*i++);

            SeparateString(string(ApisSetsQry.FieldAsString("edi_addr")), ':', strs);
            i=strs.begin();
            if (i!=strs.end()) paxlstInfo.setRecipientName(*i++);
            if (i!=strs.end()) paxlstInfo.setRecipientCarrierCode(*i++);

            ostringstream flight;
            flight << airline.code_lat << flt_no << suffix;

            string iataCode = Paxlst::createIataCode(flight.str(),act_in_local);
            paxlstInfo.setIataCode( iataCode );
            paxlstInfo.setFlight(flight.str());
            paxlstInfo.setDepPort(airp_dep.code_lat);
            paxlstInfo.setDepDateTime(act_out_local);
            paxlstInfo.setArrPort(airp_arv.code_lat);
            paxlstInfo.setArrDateTime(act_in_local);
          };

        	int count=0;
      	  ostringstream body;
      	  PaxQry.SetVariable("point_arv",r->point_id);
      	  PaxQry.Execute();
      	  for(;!PaxQry.Eof;PaxQry.Next(),count++)
      	  {
      	    Paxlst::PassengerInfo paxInfo;
      	    string airp_final_lat;
      	    if (fmt=="CSV_DE")
        	  {
        	    if (!PaxQry.FieldIsNULL("airp_final"))
        	    {
        	      TAirpsRow &airp_final = (TAirpsRow&)base_tables.get("airps").get_row("code",PaxQry.FieldAsString("airp_final"));
                if (airp_final.code_lat.empty()) throw Exception("airp_final.code_lat empty (code=%s)",airp_final.code.c_str());
                airp_final_lat=airp_final.code_lat;
              }
              else
                airp_final_lat=airp_arv.code_lat;
            };

      	    if (PaxQry.FieldIsNULL("doc_pax_id"))
      	  	{
      	  	  //документ пассажира не найден
              if (fmt=="EDI_CZ" || fmt=="EDI_CN" || fmt=="EDI_IN")
              {
        	      paxInfo.setName(PaxQry.FieldAsString("name"));
        	      paxInfo.setSurname(PaxQry.FieldAsString("surname"));
        	      paxInfo.setDocNumber("");
        	    };
              if (fmt=="CSV_CZ")
        	    {
      	        body << PaxQry.FieldAsString("surname") << ";"
      	  		       << PaxQry.FieldAsString("name") << ";"
      	  		       << ";;;;;;;;";
      	  		};
      	  		if (fmt=="CSV_DE")
        	    {
      	        body << PaxQry.FieldAsString("surname") << ";"
      	  		       << TruncNameTitles(PaxQry.FieldAsString("name")) << ";"
      	  		       << ";;;"
                     << airp_arv.code_lat << ";"
      	  		       << airp_dep.code_lat << ";"
      	  		       << airp_final_lat << ";;;";
      	  		};
      	  		if (fmt=="TXT_EE")
      	  		{
      	  		  body << "1# " << count+1 << ENDL
      	  		       << "2# " << PaxQry.FieldAsString("surname") << ENDL
      	  		       << "3# " << TruncNameTitles(PaxQry.FieldAsString("name")) << ENDL
      	  		       << "4# " << ENDL
      	  		       << "5# " << ENDL
      	  		       << "6# " << ENDL
      	  		       << "7# " << ENDL
      	  		       << "8# " << ENDL
      	  		       << "9# " << ENDL
                     << "10# " << ENDL
                     << "11# " << ENDL;
              };
      	    }
      	    else
      	    {
      	      int pax_id=PaxQry.FieldAsInteger("doc_pax_id");
      	      
      	      string doc_surname, doc_first_name, doc_second_name;
              if (!PaxQry.FieldIsNULL("doc_surname"))
              {
                doc_surname=PaxQry.FieldAsString("doc_surname");
        	  		doc_first_name=PaxQry.FieldAsString("doc_first_name");
        	  		doc_second_name=PaxQry.FieldAsString("doc_second_name");
              }
              else
              {
                //в терминалах до версии 201107-0126021 невозможен контроль и ввод фамилии из документа
                doc_surname=PaxQry.FieldAsString("surname");
                doc_first_name=PaxQry.FieldAsString("name");
              };
              if (fmt=="CSV_DE" || fmt=="TXT_EE")
              {
                doc_first_name=TruncNameTitles(doc_first_name.c_str());
                doc_second_name=TruncNameTitles(doc_second_name.c_str());
              };
              if (fmt=="EDI_CZ" || fmt=="EDI_CN" || fmt=="EDI_IN" || fmt=="CSV_DE" || fmt=="TXT_EE")
              {
                if (!doc_second_name.empty())
                {
                  doc_first_name+=" "+doc_second_name;
                  doc_second_name.clear();
                };
              };

      	      string gender;
      	      if (!PaxQry.FieldIsNULL("gender"))
      	      {
      	    	  TGenderTypesRow &gender_row = (TGenderTypesRow&)base_tables.get("gender_types").get_row("code",PaxQry.FieldAsString("gender"));
      	    	  if (gender_row.code_lat.empty()) throw Exception("gender.code_lat empty (code=%s)",PaxQry.FieldAsString("gender"));
      	    	  gender=gender_row.code_lat;
      	    	  if (fmt=="EDI_CZ" || fmt=="EDI_CN" || fmt=="EDI_IN")
                {
                  gender = gender.substr(0,1);
                  if (gender!="M" &&
                      gender!="F")
        	          gender = "M";
                };
      	    	  if (fmt=="CSV_DE")
      	    	  {
      	    	    gender = gender.substr(0,1);
                  if (gender!="M" &&
                      gender!="F")
        	          gender = "U";
        	      };
        	      if (fmt=="TXT_EE")
      	    	  {
      	    	    gender = gender.substr(0,1);
                  if (gender!="M" &&
                      gender!="F")
        	          gender = "N";
        	      };
      	    	};
      	    	string doc_type;
      	    	if (!PaxQry.FieldIsNULL("doc_type"))
      	    	{
      	    	  TPaxDocTypesRow &doc_type_row = (TPaxDocTypesRow&)base_tables.get("pax_doc_types").get_row("code",PaxQry.FieldAsString("doc_type"));
      	    	  if (doc_type_row.code_lat.empty()) throw Exception("doc_type.code_lat empty (code=%s)",PaxQry.FieldAsString("doc_type"));
      	    	  doc_type=doc_type_row.code_lat;
      	    	  if (fmt=="EDI_CZ" || fmt=="EDI_CN" || fmt=="EDI_IN")
                {
                  if (doc_type!="P") doc_type.clear();
                };
      	    	  if (fmt=="CSV_DE")
      	    	  {
      	    	    if (doc_type!="P" && doc_type!="I") doc_type="P";
                };
                if (fmt=="TXT_EE")
      	    	  {
      	    	    if (doc_type!="P") doc_type.clear(); else doc_type="2";
      	    	  };
      	    	};
      	    	string nationality;
      	    	if (!PaxQry.FieldIsNULL("nationality"))
      	    	{
      	    	  nationality=GetPaxDocCountryCode(PaxQry.FieldAsString("nationality"));
      	    	};
      	    	string issue_country;
      	    	if (!PaxQry.FieldIsNULL("issue_country"))
      	    	{
      	    	  issue_country=GetPaxDocCountryCode(PaxQry.FieldAsString("issue_country"));
      	    	};
      	    	string birth_date;
      	    	if (!PaxQry.FieldIsNULL("birth_date"))
      	    	{
                if (fmt=="CSV_CZ")
      	    	    birth_date=DateTimeToStr(PaxQry.FieldAsDateTime("birth_date"),"ddmmmyy",true);
      	    	  if (fmt=="CSV_DE")
      	    	    birth_date=DateTimeToStr(PaxQry.FieldAsDateTime("birth_date"),"yymmdd",true);
      	    	  if (fmt=="TXT_EE")
      	    	    birth_date=DateTimeToStr(PaxQry.FieldAsDateTime("birth_date"),"dd.mm.yyyy",true);
      	    	};

      	    	string expiry_date;
      	    	if (!PaxQry.FieldIsNULL("expiry_date"))
      	    	{
      	    	  if (fmt=="CSV_CZ")
      	    	    expiry_date=DateTimeToStr(PaxQry.FieldAsDateTime("expiry_date"),"ddmmmyy",true);
      	    	};

              string doc_no=PaxQry.FieldAsString("doc_no");
              if (fmt=="EDI_IN")
                doc_no=CheckIn::NormalizeDocNoForAPIS(doc_no);

              if (fmt=="EDI_CZ" || fmt=="EDI_CN" || fmt=="EDI_IN")
              {
                paxInfo.setName(doc_first_name);
        	      paxInfo.setSurname(doc_surname);
                paxInfo.setSex(gender);

        	      if (!PaxQry.FieldIsNULL("birth_date"))
        	        paxInfo.setBirthDate( PaxQry.FieldAsDateTime("birth_date"));

        	      paxInfo.setDepPort(airp_dep.code_lat);
                paxInfo.setArrPort(airp_arv.code_lat);
                paxInfo.setNationality(nationality);
                //PNR
                vector<TPnrAddrItem> pnrs;
                GetPaxPnrAddr(pax_id,pnrs);
                if (!pnrs.empty())
                  paxInfo.setReservNum(convert_pnr_addr(pnrs.begin()->addr, 1));

                paxInfo.setDocType(doc_type);
                paxInfo.setDocNumber(doc_no);
                if (!PaxQry.FieldIsNULL("expiry_date"))
                  paxInfo.setDocExpirateDate(PaxQry.FieldAsDateTime("expiry_date"));
                paxInfo.setDocCountry(issue_country);
              };

              if (fmt=="CSV_CZ")
        	    {
        	    	body << doc_surname << ";"
        	  		     << doc_first_name << ";"
        	  		     << doc_second_name << ";"
        	  		     << birth_date << ";"
        	  		     << gender << ";"
        	  		     << nationality << ";"
        	  		     << doc_type << ";"
        	  		     << doc_no << ";"
        	  		     << expiry_date << ";"
        	  		     << issue_country << ";";
        	  	};
        	  	if (fmt=="CSV_DE")
        	    {
        	      body << doc_surname << ";"
        	           << doc_first_name << ";"
        	           << gender << ";"
        	           << birth_date << ";"
        	           << nationality << ";"
                     << airp_arv.code_lat << ";"
      	  		       << airp_dep.code_lat << ";"
      	  		       << airp_final_lat << ";"
      	  		       << doc_type << ";"
      	  		       << convert_char_view(doc_no,true) << ";"
      	  		       << issue_country;
        	    };
        	    if (fmt=="TXT_EE")
        	    {
                body << "1# " << count+1 << ENDL
      	  		       << "2# " << doc_surname << ENDL
      	  		       << "3# " << doc_first_name << ENDL
      	  		       << "4# " << birth_date << ENDL
      	  		       << "5# " << nationality << ENDL
      	  		       << "6# " << doc_type << ENDL
      	  		       << "7# " << convert_char_view(doc_no,true) << ENDL
      	  		       << "8# " << issue_country << ENDL
      	  		       << "9# " << gender << ENDL;
        	    };
      	    };
      	    if (!PaxQry.FieldIsNULL("doco_pax_id") && (fmt=="CSV_DE" || fmt=="TXT_EE"))
      	  	{
      	  	  //виза пассажира найдена
        	    string doco_type;
      	    	if (!PaxQry.FieldIsNULL("doco_type"))
      	    	{
      	    	  TPaxDocTypesRow &doco_type_row = (TPaxDocTypesRow&)base_tables.get("pax_doc_types").get_row("code",PaxQry.FieldAsString("doco_type"));
      	    	  if (doco_type_row.code_lat.empty()) throw Exception("doco_type.code_lat empty (code=%s)",PaxQry.FieldAsString("doco_type"));
      	    	  doco_type=doco_type_row.code_lat;
      	    	};
      	    	string applic_country;
      	    	if (!PaxQry.FieldIsNULL("applic_country"))
      	    	{
      	    	  applic_country=GetPaxDocCountryCode(PaxQry.FieldAsString("applic_country"));
      	    	};
      	    
      	      if (fmt=="CSV_DE")
        	    {
        	      body << ";"
                     << doco_type << ";"
                     << convert_char_view(PaxQry.FieldAsString("doco_no"),true) << ";"
                     << applic_country;
              };
              
              if (fmt=="TXT_EE")
              {
                body << "10# " << ENDL
                     << "11# " << (doco_type=="V"?convert_char_view(PaxQry.FieldAsString("doco_no"),true):"") << ENDL;
              };
            }
            else
            {
              if (fmt=="TXT_EE")
              {
                body << "10# " << ENDL
                     << "11# " << ENDL;
              };
            };
            
            if (fmt=="CSV_CZ" || fmt=="CSV_DE")
              body << ENDL;
      	    
            if (fmt=="EDI_CZ" || fmt=="EDI_CN" || fmt=="EDI_IN")
      	      paxlstInfo.addPassenger( paxInfo );
      	  }; //цикл по пассажирам

          vector< pair<string, string> > files;

          if (fmt=="EDI_CZ" || fmt=="EDI_CN" || fmt=="EDI_IN")
          {
            if (!paxlstInfo.passengersList().empty())
            {
              vector<string> parts;
              if (fmt=="EDI_CZ")
              {
                parts.push_back(paxlstInfo.toEdiString());
              };
              if (fmt=="EDI_CN" || fmt=="EDI_IN")
              {
                for(unsigned maxPaxPerString=MAX_PAX_PER_EDI_PART;maxPaxPerString>0;maxPaxPerString--)
                {
                  parts=paxlstInfo.toEdiStrings(maxPaxPerString);
                  vector<string>::const_iterator p=parts.begin();
                  for(; p!=parts.end(); ++p)
                    if (p->size()>MAX_LEN_OF_EDI_PART) break;
                  if (p==parts.end()) break;
                };
              };

              int part_num=parts.size()>1?1:0;
              for(vector<string>::const_iterator p=parts.begin(); p!=parts.end(); ++p, part_num++)
              {
                ostringstream file_name;
                file_name << ApisSetsQry.FieldAsString("dir")
                          << Paxlst::createEdiPaxlstFileName(airline.code_lat,
                                                             flt_no,
                                                             suffix,
                                                             airp_dep.code_lat,
                                                             airp_arv.code_lat,
                                                             scd_out_local,
                                                             "TXT",
                                                             part_num);
                files.push_back( make_pair(file_name.str(), *p) );
              };
            };
      	  }
      	  else
          {
      	    ostringstream file_name;
      	    if (fmt=="CSV_CZ" || fmt=="CSV_DE")
            {
              ostringstream f;
              f << airline.code_lat << flt_no << suffix;
            	file_name << ApisSetsQry.FieldAsString("dir")
                        << airline.code_lat
                        << (f.str().size()<6?string(6-f.str().size(),'0'):"") << flt_no << suffix  //по стандарту поле не должно превышать 6 символов
            	          << airp_dep.code_lat
            	          << airp_arv.code_lat
            	          << DateTimeToStr(scd_out_local,"yyyymmdd") << ".CSV";
            };
            	          
            if (fmt=="TXT_EE")
              file_name << ApisSetsQry.FieldAsString("dir")
                        << "LL-" << airline.code_lat << setw(3) << setfill('0') << flt_no << suffix
                        << "-" << DateTimeToStr(act_in_local,"ddmmyyyy-hhnn") << "-S.TXT";

            //доклеиваем заголовочную часть
            ostringstream header;
            if (fmt=="CSV_CZ")
            	header << "csv;ROSSIYA;"
            	    	 << airline.code_lat << setw(3) << setfill('0') << flt_no << suffix << ";"
          	      	 << airp_dep.code_lat << ";" << DateTimeToStr(act_out_local,"yyyy-mm-dd'T'hh:nn:00.0") << ";"
          	      	 << airp_arv.code_lat << ";" << DateTimeToStr(act_in_local,"yyyy-mm-dd'T'hh:nn:00.0") << ";"
          	      	 << count << ";" << ENDL;
          	if (fmt=="CSV_DE")
              header << airline.code_lat << ";"
                  	 << airline.code_lat << setw(3) << setfill('0') << flt_no << ";"
                  	 << airp_dep.code_lat << ";" << DateTimeToStr(act_out_local,"yymmddhhnn") << ";"
                  	 << airp_arv.code_lat << ";" << DateTimeToStr(act_in_local,"yymmddhhnn") << ";"
                  	 << count << ENDL;
            if (fmt=="TXT_EE")
            {
              string airline_name=airline.short_name_lat;
              if (airline_name.empty())
                airline_name=airline.name_lat;
              if (airline_name.empty())
                airline_name=airline.code_lat;

              header << "1$ " << airline_name << ENDL
                  	 << "2$ " << ENDL
                  	 << "3$ " << airline_country << ENDL
                  	 << "4$ " << ENDL
                  	 << "5$ " << ENDL
                  	 << "6$ " << ENDL
                  	 << "7$ " << ENDL
                  	 << "8$ " << ENDL
                  	 << "9$ " << DateTimeToStr(act_in_local,"dd.mm.yy hh:nn") << ENDL
                  	 << "10$ " << (airp_arv.code=="TLL"?"Tallinna Lennujaama piiripunkt":
                       	          airp_arv.code=="TAY"?"Tartu piiripunkt":
                           	      airp_arv.code=="URE"?"Kuressaare-2 piiripunkt":
                               	  airp_arv.code=="KDL"?"Kardla Lennujaama piiripunkt":"") << ENDL
                  	 << "11$ " << ENDL
                  	 << "1$ " << airline.code_lat << setw(3) << setfill('0') << flt_no << suffix << ENDL
                  	 << "2$ " << ENDL
                  	 << "3$ " << count << ENDL;
            };
            files.push_back( make_pair(file_name.str(), string(header.str()).append(body.str())) );
      	  };
      	  
          for(vector< pair<string, string> >::const_iterator iFile=files.begin();iFile!=files.end();++iFile)
          {
          	ofstream f;
            f.open(iFile->first.c_str());
            if (!f.is_open()) throw Exception("Can't open file '%s'",iFile->first.c_str());
            try
            {
            	f << iFile->second;
            	f.close();
            }
            catch(...)
            {
              try { f.close(); } catch( ... ) { };
              try
              {
                //в случае ошибки запишем пустой файл
                f.open(iFile->first.c_str());
                if (f.is_open()) f.close();
              }
              catch( ... ) { };
              throw;
            };
          };
        };
      };

    };
  }
  catch(Exception &E)
  {
    throw Exception("create_apis_file: %s",E.what());
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

