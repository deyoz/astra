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
#include "astra_main.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "astra_date_time.h"
#include "apis_utils.h"
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
#include "basel_aero.h"
#include "rozysk.h"
#include "serverlib/posthooks.h"
#include "serverlib/perfom.h"
#include "serverlib/EdiHelpManager.h"
#include "qrys.h"
#include "points.h"
#include "trip_tasks.h"
#include "stat.h"
#include "edi_utils.h"
#include "http_io.h"
#include "httpClient.h"
#include "TypeBHelpMng.h"
#include "sirena_exchange.h"
#include "baggage_tags.h"
#include <thread>
#include <chrono>

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/slogger.h"
#include "serverlib/ourtime.h"

const int sleepsec = 25;
static const int keep_alive_minuts = 30;

using namespace ASTRA;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace std;

void exec_tasks( const char *proc_name, int argc, char *argv[] );

static void watchDog(TDateTime* ptimer)
{
  while (true) {
    std::this_thread::sleep_for(std::chrono::minutes(keep_alive_minuts));
    if (*ptimer == 0.0) continue;
    double minutes = (NowUTC() - *ptimer)*1440.0;
    if(minutes > keep_alive_minuts) {
      ProgError( STDLOG, "Timer aborted" );
      abort();
    }
  }
}

int main_timer_tcl(int supervisorSocket, int argc, char *argv[])
{
  try
  {
    sleep(10);
    InitLogTime(argc>0?argv[0]:NULL);

    string num;
    if ( argc != 2 ) {
        ProgError( STDLOG,
                 "ERROR:main_timer_tcl wrong number of parameters:%d",
                 argc );
    }
    else {
         num = argv[1];
    }

    ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks()
        ->connect_db();
    init_locale();
    TDateTime watchdog_timer;
    std::thread watchdog_thread(watchDog, &watchdog_timer);
    for( ;; )
    {
      InitLogTime(argc>0?argv[0]:NULL);
      PerfomInit();
      base_tables.Invalidate();
      watchdog_timer = NowUTC();
      exec_tasks( num.c_str(), argc, argv );
      watchdog_timer = 0.0;
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

void exec_tasks( const char *proc_name, int argc, char *argv[] )
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
      InitLogTime(argc>0?argv[0]:NULL);
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
      if ( name == "send_sirena_rozysk" ) sirena_rozysk_send();
      else
      if ( name == "mintrans" ) save_mintrans_files();
      else
      if ( name == "stat_orders_synchro" ) stat_orders_synchro();
      else
      if ( name == "stat_orders_collect" ) stat_orders_collect();
      else
      if ( name == "utg" ) utg();
      else
      if ( name == "utg_prl" ) utg_prl();
      else
      if ( name == "check_trip_tasks" ) check_trip_tasks();
      else
      if ( name == "sync_fids" ) sync_fids_data();
/*	  else
      if ( name == "cobra" ) cobra();*/
      else
      if ( name == "send_apis_tr" ) send_apis_tr();
      else
      if ( name == "send_apis_lt" ) send_apis_lt();
      else
      if ( name == "clean_typeb_help" ) TypeBHelpMng::clean_typeb_help();
      else
      if ( name == "test_watch_dog" ) sleep(keep_alive_minuts * 60 * 2);

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
        callPostHooksBefore();
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

int CREATE_SPP_DAYS()
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
         throw Exception("utg: file '%s' already exists", file_name.c_str());
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
}

using namespace AstraEdifact;

void ETCheckStatusFlt(void)
{
  TReqInfo *reqInfo = TReqInfo::Instance();
  reqInfo->user.sets.time = ustTimeUTC;

  try
  {
    SirenaExchange::TLastExchangeInfo::cleanOldRecords();//!!!��⮬ ��� ���⪨ 祣� �� � �� �뫮 �뤥���� �⤥���� ��楤���
    ServerFramework::EdiHelpManager::cleanOldRecords();
    AstraEdifact::cleanOldRecords(120);
    TGeneratedTags::cleanOldRecords(2*24*60); //2 ��⮪
    OraSession.Commit();

    list<TAdvTripInfo> flts;

    TQuery ETQry(&OraSession);
    ETQry.Clear();
    ETQry.SQLText=
      "SELECT " + TAdvTripInfo::selectedFields("points") + ", pr_etstatus "
      "FROM points,trip_sets "
      "WHERE points.point_id=trip_sets.point_id AND points.pr_del>=0 AND "
      "      (act_out IS NOT NULL AND pr_etstatus=0 OR pr_etstatus<0)";
    ETQry.Execute();
    for(;!ETQry.Eof;ETQry.Next())
    {
      TAdvTripInfo fltInfo(ETQry);
      ETSExchangeStatus::Enum ets_exchange_status=ETSExchangeStatus::fromDB(ETQry);
      if (ets_exchange_status==ETSExchangeStatus::Finalized) continue;
      if (ets_exchange_status==ETSExchangeStatus::Online)
      {
        bool control_method;
        bool in_final_status;
        if (!TFltParams::get(fltInfo, control_method, in_final_status)) continue;
        if (!in_final_status) continue;
      }
      flts.push_back(fltInfo);
      ProgTrace(TRACE5, "%s: flight=%s point_id=%d", __FUNCTION__, fltInfo.flight_view().c_str(), fltInfo.point_id);
    }
    ETQry.Close();

    for(const TAdvTripInfo &flt : flts)
    {
      OraSession.Rollback();
      try
      {
        TFltParams fltParams;
        fltParams.get(flt);

        if (fltParams.in_final_status &&
            (fltParams.et_final_attempt>=5 || //�� ����� 5 ����⮪ ���⢥न�� ������ ���ࠪ⨢��
             fltParams.ets_no_exchange))                //���� ���⠢��� �ਧ��� ����� ���ࠪ⨢�
        {
          //����� � �ࢥ஬ �. ����⮢ � ���ࠪ⨢��� ०��� ����饭�
          //���� �� ����� �� ���� ���⢥ত����� ������ ������
          //��ࠢ�塞 ETL �᫨ ����஥�� ��⮮�ࠢ��
          try
          {
            ProgTrace(TRACE5,"ETCheckStatusFlt.SendTlg: point_id=%d",flt.point_id);
            vector<TypeB::TCreateInfo> createInfo;
            TypeB::TETLCreator(flt.point_id).getInfo(createInfo);
            TelegramInterface::SendTlg(createInfo);
            TFltParams::finishFinalAttempts(flt.point_id);
            OraSession.Commit();
          }
          catch(std::exception &E)
          {
            ProgError(STDLOG,"ETCheckStatusFlt.SendTlg (point_id=%d): %s",flt.point_id,E.what());
          };
        }
        else
        {
          //��ࠢ�� ���ࠪ⨢�� ������
          try
          {
            ProgTrace(TRACE5,"ETCheckStatusFlt.ETCheckStatus: point_id=%d",flt.point_id);
            TETChangeStatusList mtick;
            ETStatusInterface::ETCheckStatus(flt.point_id,csaFlt,flt.point_id,true,mtick);
            if (!ETStatusInterface::ETChangeStatus(NULL,mtick))
            {
              if (fltParams.in_final_status)
                TFltParams::finishFinalAttempts(flt.point_id);
              else
              {
                if (fltParams.ets_exchange_status==ETSExchangeStatus::NotConnected)
                  TFltParams::setETSExchangeStatus(flt.point_id, ETSExchangeStatus::Online);
              };
            }
            else
            {
              if (fltParams.in_final_status)
                TFltParams::incFinalAttempts(flt.point_id);
            };
            OraSession.Commit();
          }
          catch(std::exception &E)
          {
            ProgError(STDLOG,"ETCheckStatusFlt.ETCheckStatus (point_id=%d): %s",flt.point_id,E.what());
          };
        };
      }
      catch(...)
      {
        ProgError(STDLOG,"ETCheckStatusFlt (point_id=%d): unknown error",flt.point_id);
      };
    };
    OraSession.Rollback();
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

    TQuery PointsQry(&OraSession);
  PointsQry.Clear();
  PointsQry.SQLText =
    "SELECT points.point_id FROM points,trip_sets "
    "WHERE points.point_id=trip_sets.point_id AND "
    "      points.pr_del=0 AND points.pr_reg<>0 AND trip_sets.pr_stat=0 AND "
    "      time_out<:stat_date AND time_out>TO_DATE('01.01.0001','DD.MM.YYYY')";
  PointsQry.CreateVariable("stat_date",otDate,utcdate-2); //2 ���
  PointsQry.Execute();
  map<string, long> stat_times;
  for(;!PointsQry.Eof;PointsQry.Next())
  {
    get_flight_stat(stat_times, PointsQry.FieldAsInteger("point_id"), true);
    OraSession.Commit();
  };
  for(map<string, long>::iterator i = stat_times.begin(); i != stat_times.end(); i++)
      LogTrace(TRACE5) << i->first << ": " << i->second;
};


void sync_sirena_codes( void )
{
    ProgTrace(TRACE5,"sync_sirena_codes started");

    TQuery Qry(&OraSession);
    Qry.Clear();
    Qry.SQLText= //04068
        "BEGIN "
        "  utils.sync_sirena_codes(:pr_summer); "
        "END;";
    Qry.CreateVariable("pr_summer",otInteger,
       ASTRA::date_time::season( Now(CityTZRegion("���")) ).isSummer());

    Qry.Execute();

    OraSession.Commit();
    ProgTrace(TRACE5,"sync_sirena_codes stopped");
}

