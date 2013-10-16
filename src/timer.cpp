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
#include "file_queue.h"
#include "telegram.h"
#include "arx_daily.h"
#include "base_tables.h"
#include "stl_utils.h"
#include "empty_proc.h"
#include "basel_aero.h"
#include "rozysk.h"
#include "serverlib/posthooks.h"
#include "serverlib/perfom.h"
#include "qrys.h"
#include "points.h"

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
      if ( name == "den" ) utg_prl();
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
        //�᫨ �-�� �����⨫� true � ���᫨�� �६� ᫥���饣� �믮������
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
	utcdate += CREATE_SPP_DAYS(); //  �� ᫥���騩 ����
	TReqInfo *reqInfo = TReqInfo::Instance();
	reqInfo->Initialize("���");
	reqInfo->user.sets.time = ustTimeUTC;
	CreateSPP( utcdate );
	ProgTrace( TRACE5, "��� ����祭 �� %s", DateTimeToStr( utcdate, "dd.mm.yy" ).c_str() );
}

#include <boost/filesystem.hpp>

void utg_prl(void)
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

    TDateTime low_time = NowUTC() - 1;
    TDateTime high_time = low_time + 3;

    QParams QryParams;
    QryParams
        << QParam("low_time", otDate, low_time)
        << QParam("high_time", otDate, high_time)
        << QParam("stage_id", otInteger, sOpenCheckIn)
        << QParam("stage_type", otInteger, stCheckIn);
    TQuery &pointsQry = TQrys::Instance()->get(
            "SELECT point_id, airp, airline, flt_no, suffix, scd_out "
            "FROM trip_final_stages, points "
            "WHERE trip_final_stages.point_id=points.point_id AND "
            "      trip_final_stages.stage_id=:stage_id AND "
            "      trip_final_stages.stage_type=:stage_type AND "
            "      points.time_out>=:low_time AND points.time_out<=:high_time AND act_out IS NULL ",
            QryParams
            );

    QryParams.clear();
    QryParams << QParam("id", otInteger);
    TQuery &TlgQry = TQrys::Instance()->get(
            "SELECT heading, body, ending "
            "FROM tlg_out "
            "WHERE id=:id",
            QryParams
            );

    QryParams.clear();
    QryParams << QParam("point_id", otInteger) << QParam("last_flt_change_tid", otInteger);
    TQuery &updQry = TQrys::Instance()->get(
            "UPDATE utg_prl set last_tlg_create_tid = :last_flt_change_tid where point_id = :point_id",
            QryParams
            );

    QryParams.clear();
    QryParams << QParam("point_id", otInteger);
    TQuery &utgQry = TQrys::Instance()->get(
            "select last_flt_change_tid from utg_prl where point_id = :point_id and "
            "(last_tlg_create_tid is null or last_tlg_create_tid <> last_flt_change_tid)",
            QryParams
            );

    pointsQry.Execute();
    TTripInfo flt;
    for(; not pointsQry.Eof; pointsQry.Next(), OraSession.Commit()) {
        int point_id = pointsQry.FieldAsInteger("point_id");
        try {
            flt.Init(pointsQry);
            map<string, string> file_params;
            TFileQueue::add_sets_params( flt.airp,
                    flt.airline,
                    IntToString(flt.flt_no),
                    OWN_POINT_ADDR(),
                    FILE_UTG_TYPE,
                    1,
                    file_params );
            if(not file_params.empty() and (file_params[PARAM_TLG_TYPE].find("PRL") != string::npos)) {
                TFlights().Get(point_id, ftTranzit);
                utgQry.SetVariable("point_id", point_id);
                utgQry.Execute();
                if(utgQry.Eof) continue;
                int last_flt_change_tid = utgQry.FieldAsInteger("last_flt_change_tid");
                TypeB::TCreateInfo info("PRL");
                info.point_id = point_id;
                TTypeBTypesRow tlgTypeInfo;
                int tlg_id = TelegramInterface::create_tlg(info, tlgTypeInfo);
                TlgQry.SetVariable("id", tlg_id);
                TlgQry.Execute();
                string tlg_text=(string)
                    TlgQry.FieldAsString("heading")+
                    TlgQry.FieldAsString("body")+
                    TlgQry.FieldAsString("ending");
                OraSession.Rollback();
                putUTG(tlg_id, "PRL", flt, tlg_text, file_params);
                updQry.SetVariable("point_id", point_id);
                updQry.SetVariable("last_flt_change_tid", last_flt_change_tid);
                updQry.Execute();
            }
        } catch(Exception &E) {
            OraSession.Rollback();
            ProgError(STDLOG,"utg_prl: Exception: %s (point_id=%d)",E.what(), point_id);
        } catch(...) {
            OraSession.Rollback();
            ProgError(STDLOG,"utg_prl: unknown error (point_id=%d)", point_id);
        }
    }
#ifdef SQL_COUNTERS
    for(map<string, int>::iterator im = sqlCounters.begin(); im != sqlCounters.end(); im++) {
        ProgTrace(TRACE5, "sqlCounters[%s] = %d", im->first.c_str(), im->second);
    }
#endif
}

void utg(void)
{
  TFileQueue file_queue;
  file_queue.get( TFilterQueue( OWN_POINT_ADDR(), FILE_UTG_TYPE ) );
  for ( TFileQueue::iterator item=file_queue.begin();
        item!=file_queue.end();
        item++, OraSession.Commit() ) {
     try {
       if ( item->params.find( PARAM_WORK_DIR ) == item->params.end() ||
            item->params[ PARAM_WORK_DIR ].empty() ) {
         throw Exception("work_dir not specified");
       }
       if ( item->params.find( PARAM_FILE_NAME ) == item->params.end() ||
            item->params[ PARAM_FILE_NAME ].empty() ) {
         throw Exception("file_name not specified");
       }
       boost::filesystem::path apath(item->params[ PARAM_WORK_DIR ]);
       if(not boost::filesystem::exists(apath))
          throw Exception("utg: directory '%s' not exists", item->params[ PARAM_WORK_DIR ].c_str());
       ofstream f;
       string file_name = item->params[ PARAM_WORK_DIR ] + "/" +item->params[ PARAM_FILE_NAME ];
       apath = file_name;
       if(boost::filesystem::exists(apath))
         throw Exception("utg: file '%s' already exists");
       f.open(file_name.c_str());
       if(!f.is_open()) throw Exception("Can't open file '%s'", file_name.c_str());
       f << item->data;
       f.close();
       TFileQueue::deleteFile(item->id);
     }
     catch(Exception &E) {
        OraSession.Rollback();
        try
        {
            EOracleError *orae=dynamic_cast<EOracleError*>(&E);
            if (orae!=NULL&&
                    (orae->Code==4061||orae->Code==4068)) continue;
            ProgError(STDLOG,"Exception: %s (file id=%d)",E.what(),item->id);
        }
        catch(...) {};

    }
    catch(...) {
       OraSession.Rollback();
       ProgError(STDLOG, "Something goes wrong");
    }
  }
/*  den!!!  static TQuery paramQry(&OraSession);
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
    }*/
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
            (fltParams.et_final_attempt>=5 || //�� ����� 5 ����⮪ ���⢥न�� ������ ���ࠪ⨢��
             fltParams.etl_only))                //���� ���⠢��� �ਧ��� ����� ���ࠪ⨢�
        {
          //����� � �ࢥ஬ ��. ����⮢ � ���ࠪ⨢��� ०��� ����饭�
          //���� �� ����� �� ���� ���⢥ত����� ������ ������
          //��ࠢ�塞 ETL �᫨ ����஥�� ��⮮�ࠢ��
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
          //��ࠢ�� ���ࠪ⨢�� ������
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
                UpdQry.SetVariable("pr_etstatus",FNull); //㢥��稬 et_final_attempt
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

void get_full_stat(TDateTime utcdate)
{
	//ᮡ�६ ����⨪� �� ���祭�� ���� ���� �� �뫥�,
	//�᫨ �� ���⠢��� �ਧ��� �����⥫쭮�� ᡮ� ����⨪� pr_stat
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
  PointsQry.CreateVariable("stat_date",otDate,utcdate-2); //2 ���
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

	//����塞 �ਧ��� ��⭥�/������ ������樨
	bool pr_summer=false;
	string tz_region=CityTZRegion("���");
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

