#include <stdio.h>
#include <string.h>
#include <map>
#include <vector>
#include <string>

#include "basic.h"
#include "stl_utils.h"
#include "base_tables.h"
#include "astra_misc.h"
#include "telegram.h"
#include "oralib.h"
#include "maindcs.h"
#include "docs.h"
#include "season.h"
#include "load_fr.h"
#include "stat.h"
#include "apis.h"
#include "salons.h"
#include "file_queue.h"
#include "empty_proc.h"
#include "pers_weights.h"
#include "serverlib/query_runner.h"
#include "serverlib/ocilocal.h"
#include "serverlib/testmode.h"
#include "edilib/edi_loading.h"
#include "app_interaction.h"

#define NICKNAME "VLAD"
#include "serverlib/test.h"

#include "tlg/ssm_parser.h"
#include "tlg/lci_parser.h"

/*
int LocalIsNosir=0;
*/
int nosir_test(int argc,char **argv);
void nosir_test_help(const char *name);
int seasons_dst_format(int argc,char **argv);
int points_dst_format(int argc,char **argv);
int nosir_tscript(int argc, char** argv);
int tz2db(int argc,char **argv);

const
  struct {
    const char *name;
    int (*p)(int argc,char **argv);     //процедура обработки
    void (*h)(const char *name);                    //помощь
    const char *description;
  } obrnosirnick []={
    {"-test",                   nosir_test,             nosir_test_help,            NULL},
    {"-ediinsert",              edi_load_messages_main, NULL,                       "loading edifact templates"},
    {"-testbm",                 testbm,                 NULL,                       NULL},
    {"-load_fr",                load_fr,                NULL,                       "loading FR files to database"},
    {"-get_fr",                 get_fr,                 NULL,                       "getting FR files from database to local path"},
    {"-termversion",            SetTermVersionNotice,   SetTermVersionNoticeHelp,   NULL},
    {"-create_apis",            create_apis_nosir,      create_apis_nosir_help,     NULL},
    {"-send_tlg",               send_tlg,               send_tlg_help,              NULL},
    {"-create_tlg",             create_tlg,             NULL,                       NULL},
    {"-dst_seasons",            seasons_dst_format,     NULL,                       NULL},
    {"-agent_stat_delta",       STAT::agent_stat_delta, NULL,                       NULL},
    {"-lci",                    TypeB::lci,             NULL,                       NULL},
    {"-ssm",                    TypeB::ssm,             NULL,                       NULL},
    {"-tz2db",                  tz2db,                  NULL,                       "reload date_time_zonespec.csv content to db"},
    {"-get_sirena_rozysk_stat", get_sirena_rozysk_stat, NULL,                       NULL},
    {"-get_events_stat",        get_events_stat2,       NULL,                       NULL},
    {"-get_basel_aero_stat",    get_basel_aero_stat,    NULL,                       NULL},
    {"-testsalons",             testsalons,             NULL,                       NULL},
    {"-test_trfer_exists",      test_trfer_exists,      NULL,                       NULL},
    {"-bind_trfer_trips",       bind_trfer_trips,       NULL,                       NULL},
    {"-unbind_trfer_trips",     unbind_trfer_trips,     NULL,                       NULL},
    {"-test_typeb_utils",       test_typeb_utils,       NULL,                       NULL},
    {"-compare_apis",           compare_apis,           NULL,                       NULL},
    {"-test_sopp_sql",          test_sopp_sql,          NULL,                       NULL},
    {"-test_file_queue",        test_file_queue,        NULL,                       NULL},
    {"-tscript",                nosir_tscript,          NULL,                       NULL},    
    {"-mobile_stat",            mobile_stat,            NULL,                       NULL},
    {"-test_astra_locale_adv",  test_astra_locale_adv,  NULL,                       NULL},
    {"-insert_locales",         insert_locales,         NULL,                       NULL},
    {"-file_by_id",             file_by_id,             NULL,                       NULL},
    {"-dst_points",             points_dst_format,      NULL,                       NULL},
    {"-fill_counters_by_subcls",fill_counters_by_subcls,NULL,                       NULL},
    {"-check_counters_by_subcls",check_counters_by_subcls,NULL,                     NULL},
    {"-convert_codeshare",      convert_codeshare,      NULL,                       NULL},
    {"-test_app_interaction",   test_app_interaction,   NULL,                       NULL},
  };

int nosir_test(int argc,char **argv)
{
  printf("argc=%i\n",argc);
  for(int i=0; i<argc; i++)
    printf("argv[%i]='%s'\n",i,argv[i]);
  return 0;
};

void nosir_test_help(const char *name)
{
  printf("  %-15.15s ", name);
  puts("Sample of 'astra -nosir' usage: printing all params");
};

int main_nosir_user(int argc,char **argv)
{
  try
  {
    puts(""); //отделим пустой строкой вывод обработчика
    //LocalIsNosir=1;
    int i=sizeof(obrnosirnick)/sizeof(obrnosirnick[0])-1;
    for(;i>=0;i--)
      if (strcmp(argv[0], obrnosirnick[i].name)==0) break;
    if (i>=0)
    {
      ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks()->connect_db();
      try
      {

        ProgTrace(TRACE1,"nosir func started: name='%s, id=%i, argc=%i", argv[0], i, argc);
        int res=obrnosirnick[i].p(argc,argv);
        ProgTrace(TRACE1,"nosir func finished: name='%s, id=%i, res=%i", argv[0], i, res);

        if(res != 0)
            ASTRA::rollback();
        else
            ASTRA::commit();
        OraSession.LogOff();
      }
      catch(const std::exception &e)
      {
        LogError(STDLOG) << e.what();
        try
        {
          ASTRA::rollback();
          OraSession.LogOff();
        }
        catch(...) {  ProgError(STDLOG, "OraSession.Rollback or OraSession.LogOff error"); };
        throw;
      };
    }
    else
    {
      ProgError(STDLOG,"invalid parameter %s",argv[0]);
      printf("invalid parameter %s\n",argv[0]);
    };
  }
  catch(EOracleError &E)
  {
    ProgError(STDLOG,"EOracleError %d: %s",E.Code,E.what());
    puts("Bad error! Contact with developers!");
  }
  catch(std::exception &E)
  {
    ProgError(STDLOG,"std::exception: %s",E.what());
    puts("Bad error! Contact with developers!");
  }
  catch(...)
  {
    ProgError(STDLOG, "Unknown exception");
    puts("Bad error! Contact with developers!");
  };
  //LocalIsNosir=0;
  return 0;
}

void help_nosir_user(void)
{
  puts("Usage:");
  puts("  astra -help                           ; this page");
  puts("  astra.tcl [user/password]  -nosir (key) <params>");
  puts("  astra astra.tcl [user/password]  -nosir (key) <params>");
  //puts("  obrzap [user/password] -nosir (key) <params>    ;use SIRENA_TCL environnment for sirena.tcl");
  puts("Function key (key) is:");
  int len=sizeof(obrnosirnick)/sizeof(obrnosirnick[0]);
  for (int i=0;i<len;i++)
  {
    if (obrnosirnick[i].h!=NULL) obrnosirnick[i].h(obrnosirnick[i].name);
    else
    {
      if (obrnosirnick[i].description!=NULL)
        printf("  %-15.15s %s\n",obrnosirnick[i].name, obrnosirnick[i].description);
    };
  };
  return;
}
/*
int IsNosir(void)
{
  return LocalIsNosir==1;
}
*/

struct TSTrip {
  int trip_id;
  int move_id;
  int num;
};

using namespace std;
using namespace BASIC;
using namespace SEASON;
using namespace ASTRA;

TDateTime getdiffhours( const std::string &region )
{
/*'Asia/Magadan' +2
'Asia/Anadyr') +0
'Asia/Kamchatka' +0
'Asia/Novokuznetsk' +0
'Europe/Samara' +0
'Europe/Kaliningrad' +1
'Europe/Moscow' +1
'Europe/Simferopol' +1
'Europe/Volgograd' +1
'Asia/Yekaterinburg' +1
'Asia/Omsk' +1
'Asia/Novosibirsk' +1
'Asia/Krasnoyarsk' +1
'Asia/Irkutsk' +1
'Asia/Yakutsk' +1
'Asia/Khandyga' +1
'Asia/Vladivostok' +1
'Asia/Sakhalin' +1
'Asia/Ust-Nera' +1
'Asia/Chita' +2
'Asia/Srednekolymsk' +1
*/
  if ( region == "Asia/Anadyr" ||
       region == "Asia/Kamchatka" ||
       region == "Asia/Novokuznetsk" ||
       region == "Europe/Samara" ) {
    return 0.0;
  }

  if ( region == "Asia/Magadan" ||
       region == "Asia/Chita" ) {
    return 2.0;
  }
  return 1.0;
}


using namespace boost::local_time;
using namespace boost::posix_time;


void gettime( const TDateTime &old_utc, TDateTime &new_utc,
              TDateTime &old_local, TDateTime &new_local, const std::string &region )
{
   new_utc = old_utc;
   old_local = NoExists;
   new_local = NoExists;
   if ( old_utc == NoExists ) {
     return;
   }
   tz_database &tz_db = get_tz_database();
   old_local = UTCToLocal( old_utc, region );
   ptime vtime( DateTimeToBoost( old_utc ) );
   time_zone_ptr tz = tz_db.time_zone_from_region( region );
   local_date_time ltime( vtime, tz ); /* определяем текущее время локальное */
   if ( ltime.is_dst() ) { // лето
     new_utc = new_utc + getdiffhours( region ) /24.0;
   }
   new_local = UTCToLocal( new_utc, region );
}

/*

update points set pr_del=-1 WHERE move_id in
( select move_id from points
where (scd_in >= to_date('24.10.14','DD.MM.YY') or scd_out >= to_date('24.10.14','DD.MM.YY')) AND pr_del <> -1 )
*/


const int SEASON_PERIOD_COUNT = 16;
const int SEASON_PRIOR_PERIOD = 13;

struct P {
    ptime b;
    ptime s;
    boolean summer;
    int dst;
};

void getPeriods( string filter_tz_region, vector<P> &periods ) {
  ptime utcd = second_clock::universal_time();
  int year = utcd.date().year();
  tz_database &tz_db = get_tz_database();
  time_zone_ptr tz = tz_db.time_zone_from_region( filter_tz_region );
  if (tz==NULL) throw EXCEPTIONS::Exception("Region '%s' not found",filter_tz_region.c_str());
  local_date_time ld( utcd, tz ); /* определяем текущее время локальное */
  bool summer = true;
  /* устанавливаем первый год и признак периода */
  for ( int i=0; i<SEASON_PRIOR_PERIOD; i++ ) {
    if ( tz->has_dst() ) {  // если есть переход на зимнее/летнее расписание
        if ( i == 0 ) {
        //dst_offset = tz->dst_offset().hours();
        if ( ld.is_dst() ) {  // если сейчас лето
          year--;
          summer = false;
        }
        else {  // если сейчас зима
          ptime start_time = tz->dst_local_start_time( year );
          ProgTrace( TRACE5, "start_time=%s", DateTimeToStr( BoostToDateTime(start_time),"dd.mm.yy hh:nn:ss" ).c_str() );
          if ( ld.local_time() < start_time ) {
            tst();
            year--;
          }
          summer = true;
        }
      }
      else {
        summer = !summer;
        if ( !summer ) {  // если сейчас лето
          year--;
        }
      }
    }
    else {
     year--;
     //dst_offset = 0;
    }

  }
  for ( int i=0; i<SEASON_PERIOD_COUNT; i++ ) {
    ptime s_time, e_time;
    string name;
    if ( tz->has_dst() ) {
      if ( summer ) {
        s_time = tz->dst_local_start_time( year ) - tz->base_utc_offset();
        e_time = tz->dst_local_end_time( year ) - tz->base_utc_offset() - seconds(1);
      }
      else {
        s_time = tz->dst_local_end_time( year ) - tz->base_utc_offset() - tz->dst_offset();
        year++;
        e_time = tz->dst_local_start_time( year ) - tz->base_utc_offset() - seconds(1);
      }
      summer = !summer;
    }
    else {
     /* период - это целый год */
     s_time = ptime( boost::gregorian::date(year,1,1) );
     year++;
     e_time = ptime( boost::gregorian::date(year,1,1) );
    }
    ProgTrace( TRACE5, "s_time=%s, e_time=%s, summer=%d, i=%d, dst=%i",
               DateTimeToStr( UTCToLocal( BoostToDateTime(s_time), filter_tz_region ),"dd.mm.yy hh:nn:ss" ).c_str(),
               DateTimeToStr( UTCToLocal( BoostToDateTime(e_time), filter_tz_region ), "dd.mm.yy hh:nn:ss" ).c_str(), !summer, i,
               tz->dst_offset().hours());
    P p;
    p.b = s_time;
    p.s = e_time;
    p.summer = !summer;
    p.dst =  tz->dst_offset().hours();
    periods.push_back( p );
  }
}


int points_dst_format(int argc,char **argv)
{
  bool prior = false;
  printf("argc=%i\n",argc);
  for(int i=0; i<argc; i++) {
    printf("argv[%i]='%s'\n",i,argv[i]);
    if ( string( argv[i] ) == "prior" ) {
      prior = true;
      tst();
      break;
    }
  }

  TQuery Qry(&OraSession);
  if ( prior ) {
    Qry.Clear();
    Qry.SQLText = "select distinct s.region region from seasons s, DATE_TIME_ZONESPEC d where s.region=d.id AND id IN ("
        "'Europe/Kaliningrad',"
        "'Europe/Moscow',"
        "'Europe/Samara',"
        "'Asia/Yekaterinburg',"
        "'Asia/Novosibirsk',"
        "'Asia/Krasnoyarsk',"
        "'Asia/Irkutsk',"
        "'Asia/Yakutsk',"
        "'Asia/Sakhalin',"
        "'Asia/Srednekolymsk',"
        "'Asia/Anadyr',"
        "'Asia/Novokuznetsk',"
        "'Asia/Chita',"
        "'Asia/Magadan' )";
  tst();
    Qry.Execute();
    tst();
    vector<string> regions;
    for ( ; !Qry.Eof; Qry.Next() ) {
      regions.push_back( Qry.FieldAsString("region") );
    }
    Qry.SQLText = "DELETE FROM seasons where region in (  "
        "select distinct s.region from seasons s, DATE_TIME_ZONESPEC d "
        " where s.region=d.id AND d.id IN ( "
                  "'Europe/Kaliningrad',"
                  "'Europe/Moscow',"
                  "'Europe/Samara',"
                  "'Asia/Yekaterinburg',"
                  "'Asia/Yekaterinburg',"
                  "'Asia/Novosibirsk',"
                  "'Asia/Krasnoyarsk',"
                  "'Asia/Irkutsk',"
                  "'Asia/Yakutsk',"
                  "'Asia/Sakhalin',"
                  "'Asia/Srednekolymsk',"
                  "'Asia/Anadyr',"
                  "'Asia/Novokuznetsk',"
                  "'Asia/Chita',"
                  "'Asia/Magadan' )"
        ")";
    tst();
    Qry.Execute();
    tst();
    Qry.Clear();

    TQuery GQry( &OraSession );
    GQry.Clear();
    GQry.SQLText =
    "DECLARE i NUMBER;"
    "BEGIN "
    "SELECT COUNT(*) INTO i FROM seasons "
    " WHERE region=:region AND :first=first AND :last=last; "
    "IF i = 0 THEN "
    " INSERT INTO seasons(region,first,last,hours) VALUES(:region,:first,:last,:hours); "
    "END IF; "
    "END;";
    GQry.DeclareVariable( "region", otString );
    GQry.DeclareVariable( "first", otDate );
    GQry.DeclareVariable( "last", otDate );
    GQry.DeclareVariable( "hours", otInteger );

    for ( vector<string>::iterator ireg=regions.begin(); ireg!=regions.end(); ireg++) {
      vector<P> periods;
      getPeriods( *ireg, periods );
      GQry.SetVariable( "region", *ireg );
      for ( vector<P>::iterator p=periods.begin(); p!=periods.end(); p++ ) {
        int hours;
         if ( p->summer )
           hours = p->dst;
         else
           hours = 0;
         GQry.SetVariable( "first", BoostToDateTime( p->b ) );
         GQry.SetVariable( "last", BoostToDateTime( p->s) );
         GQry.SetVariable( "hours", hours );
         GQry.Execute();
       }
    }
  }
  Qry.Clear();
  Qry.SQLText =
    "select point_id,airp,scd_in,scd_out,est_in,est_out,act_in,act_out, c.tz_region region from airps a, cities c,"
    " ( select point_id,airp,scd_in,scd_out,est_in,est_out,act_in,act_out from points p "
    " where (scd_in >= to_date('27.03.15','DD.MM.YY') or scd_out >= to_date('27.03.15','DD.MM.YY')) AND pr_del <> -1 ) p "
    " WHERE "
    " p.airp=a.code AND a.city=c.code AND c.country='РФ' ";
  Qry.Execute();
  TQuery UQry(&OraSession);
  UQry.SQLText =
    "UPDATE points SET scd_in=:scd_in,scd_out=:scd_out,est_in=:est_in,est_out=:est_out,act_in=:act_in,act_out=:act_out WHERE point_id=:point_id";
  UQry.DeclareVariable( "point_id", otInteger );
  UQry.DeclareVariable( "scd_in", otDate );
  UQry.DeclareVariable( "scd_out", otDate );
  UQry.DeclareVariable( "est_in", otDate );
  UQry.DeclareVariable( "est_out", otDate );
  UQry.DeclareVariable( "act_in", otDate );
  UQry.DeclareVariable( "act_out", otDate );
  TQuery CQry(&OraSession);
  if ( prior ) {
    CQry.SQLText =
    "INSERT INTO dpoints( point_id,scd_in,scd_out,est_in,est_out,act_in,act_out,pscd_in,pscd_out,pest_in,pest_out,pact_in,pact_out )"
    "VALUES(:point_id,:scd_in,:scd_out,:est_in,:est_out,:act_in,:act_out,:pscd_in,:pscd_out,:pest_in,:pest_out,:pact_in,:pact_out)";
  }
  else {
    CQry.SQLText =
    "UPDATE dpoints SET nscd_in=:nscd_in,nscd_out=:nscd_out,nest_in=:nest_in,nest_out=:nest_out,nact_in=:nact_in,nact_out=:nact_out WHERE point_id=:point_id";
  }
  CQry.DeclareVariable( "point_id", otInteger );
  if ( prior ) {
    CQry.DeclareVariable( "scd_in", otDate );
    CQry.DeclareVariable( "scd_out", otDate );
    CQry.DeclareVariable( "est_in", otDate );
    CQry.DeclareVariable( "est_out", otDate );
    CQry.DeclareVariable( "act_in", otDate );
    CQry.DeclareVariable( "act_out", otDate );
    CQry.DeclareVariable( "pscd_in", otDate );
    CQry.DeclareVariable( "pscd_out", otDate );
    CQry.DeclareVariable( "pest_in", otDate );
    CQry.DeclareVariable( "pest_out", otDate );
    CQry.DeclareVariable( "pact_in", otDate );
    CQry.DeclareVariable( "pact_out", otDate );
  }
  else {
    CQry.DeclareVariable( "nscd_in", otDate );
    CQry.DeclareVariable( "nscd_out", otDate );
    CQry.DeclareVariable( "nest_in", otDate );
    CQry.DeclareVariable( "nest_out", otDate );
    CQry.DeclareVariable( "nact_in", otDate );
    CQry.DeclareVariable( "nact_out", otDate );
  }

  for ( ;!Qry.Eof; Qry.Next() ) {
  TDateTime putc_scd_in = NoExists,
            putc_scd_out = NoExists,
            putc_est_in  = NoExists,
            putc_est_out  = NoExists,
            putc_act_in  = NoExists,
            putc_act_out = NoExists;
  TDateTime nutc_scd_in = NoExists,
            nutc_scd_out = NoExists,
            nutc_est_in  = NoExists,
            nutc_est_out  = NoExists,
            nutc_act_in  = NoExists,
            nutc_act_out = NoExists;
  TDateTime plocal_scd_in = NoExists,
            plocal_scd_out = NoExists,
            plocal_est_in  = NoExists,
            plocal_est_out  = NoExists,
            plocal_act_in  = NoExists,
            plocal_act_out = NoExists;
  TDateTime nlocal_scd_in = NoExists,
            nlocal_scd_out = NoExists,
            nlocal_est_in  = NoExists,
            nlocal_est_out  = NoExists,
            nlocal_act_in  = NoExists,
            nlocal_act_out = NoExists;
    if ( !Qry.FieldIsNULL( "scd_in" ) ) {
      putc_scd_in = Qry.FieldAsDateTime("scd_in");
      gettime( putc_scd_in, nutc_scd_in,
               plocal_scd_in, nlocal_scd_in, Qry.FieldAsString( "region" ) );
    }
    if ( !Qry.FieldIsNULL( "scd_out" ) ) {
      putc_scd_out = Qry.FieldAsDateTime("scd_out");
      gettime( putc_scd_out, nutc_scd_out,
               plocal_scd_out, nlocal_scd_out, Qry.FieldAsString( "region" ) );
    }
    if ( !Qry.FieldIsNULL( "est_in" ) ) {
      putc_est_in = Qry.FieldAsDateTime("est_in");
      gettime( putc_est_in, nutc_est_in,
               plocal_est_in, nlocal_est_in, Qry.FieldAsString( "region" ) );
    }
    if ( !Qry.FieldIsNULL( "est_out" ) ) {
      putc_est_out = Qry.FieldAsDateTime("est_out");
      gettime( putc_est_out, nutc_est_out,
               plocal_est_out, nlocal_est_out, Qry.FieldAsString( "region" ) );
    }
    if ( !Qry.FieldIsNULL( "act_in" ) ) {
      putc_act_in = Qry.FieldAsDateTime("act_in");
      gettime( putc_act_in, nutc_act_in,
               plocal_act_in, nlocal_act_in, Qry.FieldAsString( "region" ) );
    }
    if ( !Qry.FieldIsNULL( "act_out" ) ) {
      putc_act_out = Qry.FieldAsDateTime("act_out");
      gettime( putc_act_out, nutc_act_out,
               plocal_act_out, nlocal_act_out, Qry.FieldAsString( "region" ) );
    }
    UQry.SetVariable( "point_id", Qry.FieldAsInteger("point_id") );
    CQry.SetVariable( "point_id", Qry.FieldAsInteger( "point_id" ) );
    if ( nutc_scd_in != NoExists ) {
      UQry.SetVariable( "scd_in", nutc_scd_in );
      if ( prior ) {
        CQry.SetVariable( "scd_in", putc_scd_in );
        CQry.SetVariable( "pscd_in", plocal_scd_in );
      }
      else {
        CQry.SetVariable( "nscd_in", nlocal_scd_in );
      }
    }
    else {
      UQry.SetVariable( "scd_in", FNull );
      if ( prior ) {
        CQry.SetVariable( "scd_in", FNull );
        CQry.SetVariable( "pscd_in", FNull );
      }
      else {
        CQry.SetVariable( "nscd_in", FNull );
      }
    }
    if ( nutc_scd_out != NoExists ) {
      UQry.SetVariable( "scd_out", nutc_scd_out );
      if ( prior ) {
        CQry.SetVariable( "scd_out", putc_scd_out );
        CQry.SetVariable( "pscd_out", plocal_scd_out );
      }
      else {
        CQry.SetVariable( "nscd_out", nlocal_scd_out );
      }
    }
    else {
      UQry.SetVariable( "scd_out", FNull );
      if ( prior ) {
        CQry.SetVariable( "scd_out", FNull );
        CQry.SetVariable( "pscd_out", FNull );
      }
      else {
        CQry.SetVariable( "nscd_out", FNull );
      }
    }
    if ( nutc_est_in != NoExists ) {
      UQry.SetVariable( "est_in", nutc_est_in );
      if ( prior ) {
        CQry.SetVariable( "est_in", putc_est_in );
        CQry.SetVariable( "pest_in", plocal_est_in );
      }
      else {
        CQry.SetVariable( "nest_in", nlocal_scd_in );
      }
    }
    else {
      UQry.SetVariable( "est_in", FNull );
      if ( prior ) {
        CQry.SetVariable( "est_in", FNull );
        CQry.SetVariable( "pest_in", FNull );
      }
      else {
        CQry.SetVariable( "nest_in", FNull );
      }
    }
    if ( nutc_est_out != NoExists ) {
      UQry.SetVariable( "est_out", nutc_est_out );
      if ( prior ) {
        CQry.SetVariable( "est_out", putc_est_out );
        CQry.SetVariable( "pest_out", plocal_est_out );
      }
      else {
        CQry.SetVariable( "nest_out", nlocal_est_out );
      }
    }
    else {
      UQry.SetVariable( "est_out", FNull );
      if ( prior ) {
        CQry.SetVariable( "est_out", FNull );
        CQry.SetVariable( "pest_out", FNull );
      }
      else {
        CQry.SetVariable( "nest_out", FNull );
      }
    }
    if ( nutc_act_in != NoExists ) {
      UQry.SetVariable( "act_in", nutc_act_in );
      if ( prior ) {
        CQry.SetVariable( "act_in", putc_act_in );
        CQry.SetVariable( "pact_in", plocal_act_in );
      }
      else {
        CQry.SetVariable( "nact_in", nlocal_act_in );
      }
    }
    else {
      UQry.SetVariable( "act_in", FNull );
      if ( prior ) {
        CQry.SetVariable( "act_in", FNull );
        CQry.SetVariable( "pact_in", FNull );
      }
      else {
        CQry.SetVariable( "nact_in", FNull );
      }
    }
    if ( nutc_act_out != NoExists ) {
      UQry.SetVariable( "act_out", nutc_act_out );
      if ( prior ) {
        CQry.SetVariable( "act_out", putc_act_out );
        CQry.SetVariable( "pact_out", plocal_act_out );
      }
      else {
        CQry.SetVariable( "nact_out", nlocal_act_out );
      }
    }
    else {
      UQry.SetVariable( "act_out", FNull );
      if ( prior ) {
        CQry.SetVariable( "act_out", FNull );
        CQry.SetVariable( "pact_out", FNull );
      }
      else {
        CQry.SetVariable( "nact_out", FNull );
      }
    }
    tst();
    CQry.Execute();
    if ( !prior ) {
      UQry.Execute();
    }
  }
  OraSession.Commit();
  if ( !prior ) {
    Qry.Clear();
    Qry.SQLText =
      "SELECT point_id from dpoints "
      " WHERE NVL(pscd_in,to_date('27.03.15','DD.MM.YY'))!=NVL(nscd_in,to_date('27.03.15','DD.MM.YY')) OR "
      "        NVL(pscd_out,to_date('27.03.15','DD.MM.YY'))!=NVL(nscd_out,to_date('27.03.15','DD.MM.YY')) OR "
      "        NVL(pest_in,to_date('27.03.15','DD.MM.YY'))!=NVL(nest_in,to_date('27.03.15','DD.MM.YY')) OR "
      "        NVL(pest_out,to_date('27.03.15','DD.MM.YY'))!=NVL(nest_out,to_date('27.03.15','DD.MM.YY')) OR "
      "        NVL(pact_in,to_date('27.03.15','DD.MM.YY'))!=NVL(nact_in,to_date('27.03.15','DD.MM.YY')) OR "
      "        NVL(pact_out,to_date('27.03.15','DD.MM.YY'))!=NVL(nact_out,to_date('27.03.15','DD.MM.YY')) ";
    Qry.Execute();
    for ( ; !Qry.Eof; Qry.Next() ) {
      ProgError( STDLOG, "point_id=%d", Qry.FieldAsInteger( "point_id" ) );
    }
  }
  return 0;
}

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>

namespace TZ_2_DB {
    void from_file(vector<vector<string> > &data)
    {
        ifstream input("date_time_zonespec.csv");
        bool pr_first = true;
        for(string line; getline(input, line);) {
            if(pr_first) {
                pr_first = not pr_first; //skip first row
                continue;
            }
            vector<string> values;
            vector<string> row;
            boost::algorithm::split(values, line, boost::is_any_of(","));
            for(vector<string>::const_iterator i = values.begin(); i != values.end(); i++) {
                string val = *i;
                boost::algorithm::erase_all(val, "\"");
                row.push_back(val);
            }
            if(row.size() < 11) throw logic_error((string)"not enough fields " + IntToString(row.size()) + ": " + line);
            if(row.size() > 11) throw logic_error((string)"too many fields " + IntToString(row.size()) + ": " + line);
            data.push_back(row);
        }
    }

    string rowToString(const vector<string> &val)
    {
        string result;
        for(vector<string>::const_iterator i = val.begin(); i != val.end(); i++) {
            if(not result.empty()) result += ", ";
            result += *i;
        }
        return result;
    }

    void to_db(vector<vector<string> > &data)
    {
        TQuery Qry(&OraSession);
        Qry.SQLText =
            "begin "
            "   execute immediate 'drop table new_date_time_zonespec'; "
            "   execute immediate 'create table new_date_time_zonespec  "
            "(   id varchar2(50) not null,  "
            "    std_abbr varchar2(5),  "
            "    std_name varchar2(50),  "
            "    dst_abbr varchar2(5),  "
            "    dst_name varchar2(50),  "
            "    gmt_offset varchar2(9),  "
            "    dst_adjustment varchar2(9),  "
            "    dst_start_date_rule varchar2(10),  "
            "    start_time varchar2(9),  "
            "    dst_end_date_rule varchar2(10),  "
            "    end_time varchar2(9) "
            ")'; "
            "end; ";
        Qry.Execute();

        Qry.SQLText =
            "insert into new_date_time_zonespec( "
            " id, "
            " std_abbr, "
            " std_name, "
            " dst_abbr, "
            " dst_name, "
            " gmt_offset, "
            " dst_adjustment, "
            " dst_start_date_rule, "
            " start_time, "
            " dst_end_date_rule, "
            " end_time "
            ") values ( "
            " :id, "
            " :std_abbr, "
            " :std_name, "
            " :dst_abbr, "
            " :dst_name, "
            " :gmt_offset, "
            " :dst_adjustment, "
            " :dst_start_date_rule, "
            " :start_time, "
            " :dst_end_date_rule, "
            " :end_time "
            ")";

        Qry.DeclareVariable("id", otString);
        Qry.DeclareVariable("std_abbr", otString);
        Qry.DeclareVariable("std_name", otString);
        Qry.DeclareVariable("dst_abbr", otString);
        Qry.DeclareVariable("dst_name", otString);
        Qry.DeclareVariable("gmt_offset", otString);
        Qry.DeclareVariable("dst_adjustment", otString);
        Qry.DeclareVariable("dst_start_date_rule", otString);
        Qry.DeclareVariable("start_time", otString);
        Qry.DeclareVariable("dst_end_date_rule", otString);
        Qry.DeclareVariable("end_time", otString);

        for(vector<vector<string> >::const_iterator row = data.begin(); row != data.end(); row++) {
            int idx = 0;
            if(row->size() < 11) throw logic_error((string)"not enough fields in row " + IntToString(row->size()) + ": " + rowToString(*row));
            if(row->size() > 11) throw logic_error((string)"too many fields in row " + IntToString(row->size()) + ": " + rowToString(*row));
            for(vector<string>::const_iterator val = row->begin(); val != row->end(); val++, idx++) {
                switch(idx) {
                    case 0: Qry.SetVariable("id", *val); break;
                    case 1: Qry.SetVariable("std_abbr", *val); break;
                    case 2: Qry.SetVariable("std_name", *val); break;
                    case 3: Qry.SetVariable("dst_abbr", *val); break;
                    case 4: Qry.SetVariable("dst_name", *val); break;
                    case 5: Qry.SetVariable("gmt_offset", *val); break;
                    case 6: Qry.SetVariable("dst_adjustment", *val); break;
                    case 7: Qry.SetVariable("dst_start_date_rule", *val); break;
                    case 8: Qry.SetVariable("start_time", *val); break;
                    case 9: Qry.SetVariable("dst_end_date_rule", *val); break;
                    case 10: Qry.SetVariable("end_time", *val); break;
                }
            }
            Qry.Execute();
        }
        cout << data.size() << " rows inserted." << endl;
    }
}



int tz2db(int argc,char **argv)
{
    vector<vector<string> > data;
    TZ_2_DB::from_file(data);
    TZ_2_DB::to_db(data);
    return 0;
}

int seasons_dst_format(int argc,char **argv)
{
  tst();
  vector<TSTrip> trips;
  try{
  TQuery Qry(&OraSession);
  //1 - зима
  Qry.Clear();
  Qry.SQLText =
    "SELECT trip_id,move_id,num,first_day,last_day,days,sched_days.region FROM sched_days, seasons,"
    "( SELECT id region FROM DATE_TIME_ZONESPEC "
    " WHERE id IN ("
    "'Europe/Kaliningrad',"
    "'Europe/Moscow',"
    "'Europe/Samara',"
    "'Asia/Yekaterinburg',"
    "'Asia/Novosibirsk',"
    "'Asia/Krasnoyarsk',"
    "'Asia/Irkutsk',"
    "'Asia/Yakutsk',"
    "'Asia/Sakhalin',"
    "'Asia/Srednekolymsk',"
    "'Asia/Anadyr',"
    "'Asia/Novokuznetsk',"
    "'Asia/Chita',"
    "'Asia/Magadan' ) ) d "
    "WHERE seasons.region=sched_days.region AND seasons.region=d.region AND seasons.hours=1 AND "
    "      first_day BETWEEN seasons.first AND seasons.last "
    "ORDER BY trip_id,move_id,num";
  tst();
  Qry.Execute();
  tst();

  TQuery VQry(&OraSession);
  VQry.SQLText = "SELECT move_id FROM routes where move_id=:move_id AND airline='7К' AND rownum<2";
  VQry.DeclareVariable( "move_id", otInteger );


  TQuery VSQry(&OraSession);
  VSQry.SQLText = "SELECT move_id from drop_prior_sched_days WHERE trip_id=:trip_id AND move_id=:move_id";
  VSQry.DeclareVariable( "trip_id", otInteger);
  VSQry.DeclareVariable( "move_id", otInteger);

  TQuery UQry(&OraSession);
  UQry.SQLText =
    "BEGIN "
    " INSERT INTO drop_prior_sched_days(trip_id,move_id,num,prior_first_day,prior_last_day,prior_days,"
    "                                   first_day,last_day,days) "
    "  SELECT trip_id,move_id,num,first_day,last_day,days,:new_first_day,:new_last_day,:new_days "
    "   FROM sched_days WHERE trip_id=:trip_id AND move_id=:move_id AND num=:num;"
    "UPDATE sched_days SET first_day=:new_first_day, last_day=:new_last_day, days=:new_days "
    " WHERE trip_id=:trip_id AND move_id=:move_id AND num=:num; "
    "END;";
  UQry.DeclareVariable( "trip_id", otInteger );
  UQry.DeclareVariable( "move_id", otInteger );
  UQry.DeclareVariable( "num", otInteger );
  UQry.DeclareVariable( "new_first_day", otDate );
  UQry.DeclareVariable( "new_last_day", otDate );
  UQry.DeclareVariable( "new_days", otString );
  TQuery RQry(&OraSession);
  RQry.SQLText =
    "SELECT num,airp,airline,flt_no,suffix,craft,scd_in,delta_in,scd_out,delta_out,trip_type,litera,pr_del,f,c,y FROM routes "
    " WHERE move_id=:move_id "
    "ORDER BY num";
  RQry.DeclareVariable( "move_id", otInteger );
  TQuery RUQry(&OraSession);
  RUQry.SQLText =
    "BEGIN "
    "INSERT INTO drop_prior_routes(move_id,num,prior_scd_in,prior_delta_in,"
    "                              scd_in,delta_in,prior_scd_out,prior_delta_out,"
    "                              scd_out,delta_out) "
    "SELECT move_id,num,scd_in,delta_in,:new_scd_in,:new_delta_in,scd_out,delta_out,:new_scd_out,:new_delta_out "
    " FROM routes WHERE move_id=:move_id AND num=:num; "
    " UPDATE routes SET scd_in=:new_scd_in,delta_in=:new_delta_in,scd_out=:new_scd_out,delta_out=:new_delta_out "
    "  WHERE move_id=:move_id AND num=:num;"
    "END;";
  RUQry.DeclareVariable( "move_id", otInteger );
  RUQry.DeclareVariable( "num", otInteger );
  RUQry.DeclareVariable( "new_scd_in", otDate );
  RUQry.DeclareVariable( "new_delta_in", otInteger );
  RUQry.DeclareVariable( "new_scd_out", otDate );
  RUQry.DeclareVariable( "new_delta_out", otInteger );
  int move_id = NoExists;
  TBaseTable &baseairps = base_tables.get( "airps" );
  while ( !Qry.Eof ) {
    TSTrip t;
    t.trip_id = Qry.FieldAsInteger( "trip_id" );
    t.move_id = Qry.FieldAsInteger( "move_id" );
    t.num = Qry.FieldAsInteger( "num" );
    VQry.SetVariable( "move_id", t.move_id );
    VQry.Execute();
    if ( !VQry.Eof ) {
      Qry.Next();
      continue;
    }
    VSQry.SetVariable( "trip_id", t.trip_id );
    VSQry.SetVariable( "move_id", t.move_id );
    VSQry.Execute();
    if ( !VSQry.Eof ) {
      Qry.Next();
      continue;
    }
    trips.push_back( t );

    TDateTime prior_first_day = Qry.FieldAsDateTime( "first_day" );
    TDateTime prior_last_day = Qry.FieldAsDateTime( "last_day" );
    string region = Qry.FieldAsString( "region" );
    TDateTime trunc_pf, trunc_f;
    modf( prior_first_day, &trunc_pf );
    int trip_id = Qry.FieldAsInteger( "trip_id" );
    int sched_num = Qry.FieldAsInteger( "num" );
    string prior_days = Qry.FieldAsString( "days" );
    TDateTime first_day, last_day;
    string days;
    first_day = prior_first_day + getdiffhours( region )/24.0;
    last_day = prior_last_day + getdiffhours( region)/24.0;
    ProgTrace( TRACE5, "start, trip_id=%d, move_id=%d, num=%d", trip_id, Qry.FieldAsInteger( "move_id" ), sched_num );
    ProgTrace( TRACE5, "prior_first_day=%f, trunc_pf=%f", prior_first_day, trunc_pf );
    modf( first_day, &trunc_f );
    int diff = (int)trunc_f - (int)trunc_pf; // переход на зиму

    if ( move_id != Qry.FieldAsInteger( "move_id" ) ) {
      move_id = Qry.FieldAsInteger( "move_id" );
      RQry.SetVariable( "move_id", move_id );
      RQry.Execute();
      TDateTime scd_in = ASTRA::NoExists, prior_scd_in = ASTRA::NoExists,
                scd_out = ASTRA::NoExists, prior_scd_out = ASTRA::NoExists;
      int delta_in = 0, prior_delta_in = 0, delta_out = 0, prior_delta_out = 0;
      bool pr_dest_flight_time = false;
      int date_diff = 0;
      while ( !RQry.Eof ) {
        if ( RQry.FieldIsNULL( "scd_in" ) )
          prior_scd_in = NoExists;
        else
          prior_scd_in = RQry.FieldAsDateTime( "scd_in" );
        prior_delta_in = RQry.FieldAsInteger( "delta_in" );

        int route_num = RQry.FieldAsInteger( "num" );
        string city = ((TAirpsRow&)baseairps.get_row( "code", RQry.FieldAsString( "airp" )  , true )).city;
        TCitiesRow& row=(TCitiesRow&)base_tables.get("cities").get_row("code",city,true);
        string city_region;
        try {
          city_region = CityTZRegion( city );
          if ( city_region.empty() )
            throw EXCEPTIONS::Exception("region empty");
        }
        catch(...) {
         ProgError(STDLOG,"city=%s, region empty move_id=%d", city.c_str(), move_id );
         break;
        }
        if (  prior_scd_in != NoExists ) {
          ProgTrace( TRACE5, "before convert move_id=%d, route.num=%d, scd_in=%s, delta_in=%d, airp=%s",
                     move_id, route_num, DateTimeToStr( prior_scd_in, "dd.mm.yyyy hh:nn" ).c_str(),
                     prior_delta_in, RQry.FieldAsString( "airp" ) );
          if ( row.country == "РФ" ) {
            scd_in = trunc_f + prior_delta_in + prior_scd_in;
            ProgTrace( TRACE5, "scd_in=%f, scd_in=%s", scd_in, DateTimeToStr( scd_in, "dd.mm.yyyy hh:nn" ).c_str() );
            scd_in += getdiffhours(city_region)/24.0;
            ProgTrace( TRACE5, "scd_in=%f, scd_in=%s", scd_in, DateTimeToStr( scd_in, "dd.mm.yyyy hh:nn" ).c_str() );
            TDateTime f2, f3;
            f2 = modf( scd_in, &f3 );
            if ( f3 < trunc_f )
              scd_in = f3 - trunc_f - f2;
            else
              scd_in = f3 - trunc_f + f2;
            double df = 0.;
            scd_in = modf( scd_in, &df );
            delta_in = (int)df;
          }
          else {
            scd_in = prior_scd_in;
            delta_in = prior_delta_in;
          }
          if ( !pr_dest_flight_time && row.country == "РФ" ) {
            pr_dest_flight_time = ( prior_delta_in == 0 );
            if ( pr_dest_flight_time && delta_in != 0 ) {
              date_diff = delta_in;
              delta_in = 0;
              ProgError( STDLOG, "move_id=%d, route_num=%d, delta_in set 0", move_id, route_num );
            }
          }
          else {
            delta_in -= date_diff;
            ProgError( STDLOG, "move_id=%d, route_num=%d, delta_in set -date_diff=%d", move_id, route_num, date_diff );
          }
          ProgTrace( TRACE5, "after convert scd_in=%s, delta_in=%d",
                     DateTimeToStr( scd_in, "dd.mm.yyyy hh:nn" ).c_str(), delta_in );
        }
        if ( RQry.FieldIsNULL( "scd_out" ) )
          prior_scd_out = NoExists;
        else
          prior_scd_out = RQry.FieldAsDateTime( "scd_out" );
        prior_delta_out = RQry.FieldAsInteger( "delta_out" );
        if ( prior_scd_out != NoExists ) {
          ProgTrace( TRACE5, "before convert move_id=%d,route.num=%d, scd_out=%s, delta_out=%d, airp=%s",
                     move_id, route_num, DateTimeToStr( prior_scd_out, "dd.mm.yyyy hh:nn" ).c_str(),
                     prior_delta_out, RQry.FieldAsString( "airp" ) );
          if ( row.country == "РФ" ) {
            scd_out = trunc_f + prior_delta_out + prior_scd_out;
            ProgTrace( TRACE5, "scd_out=%f, scd_out=%s", scd_out, DateTimeToStr( scd_out, "dd.mm.yyyy hh:nn" ).c_str() );
            scd_out += getdiffhours(city_region)/24.0;
            ProgTrace( TRACE5, "scd_out=%f, scd_out=%s", scd_out, DateTimeToStr( scd_out, "dd.mm.yyyy hh:nn" ).c_str() );
            TDateTime f2, f3;
            f2 = modf( scd_out, &f3 );
            if ( f3 < trunc_f )
              scd_out = f3 - trunc_f - f2;
            else
              scd_out = f3 - trunc_f + f2;
            ProgTrace( TRACE5, "scd_out=%f, scd_out=%s", scd_out, DateTimeToStr( scd_out, "dd.mm.yyyy hh:nn" ).c_str() );
            double df;
            scd_out = modf( scd_out, &df );
            delta_out = (int)df;
          }
          else {
            scd_out = prior_scd_out;
            delta_out = prior_delta_out;
          }
          if ( !pr_dest_flight_time && row.country == "РФ" ) {
            pr_dest_flight_time = ( prior_delta_out == 0 );
            if ( pr_dest_flight_time && delta_out != 0 ) {
              date_diff = delta_out;
              delta_out = 0;
              ProgError( STDLOG, "move_id=%d, route_num=%d, delta_out set 0", move_id, route_num );
            }
          }
          else {
            delta_out -= date_diff;
            ProgError( STDLOG, "move_id=%d, route_num=%d, delta_out set -date_diff=%d", move_id, route_num, date_diff );
          }
          ProgTrace( TRACE5, "after convert scd_out=%s, delta_out=%d",
                     DateTimeToStr( scd_out, "dd.mm.yyyy hh:nn" ).c_str(), delta_out );
        }
        RUQry.SetVariable( "move_id", move_id );
        RUQry.SetVariable( "num", route_num );
        if ( prior_scd_in != NoExists ) {
          RUQry.SetVariable( "new_scd_in", scd_in );
          RUQry.SetVariable( "new_delta_in", delta_in );
        }
        else {
          RUQry.SetVariable( "new_scd_in", FNull );
          RUQry.SetVariable( "new_delta_in", FNull );
        }
        if ( prior_scd_out != NoExists ) {
          RUQry.SetVariable( "new_scd_out", scd_out );
          RUQry.SetVariable( "new_delta_out", delta_out );
        }
        else {
          RUQry.SetVariable( "new_scd_out", FNull );
          RUQry.SetVariable( "new_delta_out", FNull );
        }
        RUQry.Execute();
        RQry.Next();
      }
      if ( !pr_dest_flight_time )
        ProgError( STDLOG, "!pr_dest_flight_time, move_id=%d", move_id );
    }
    days = prior_days;
    ProgTrace( TRACE5, "days=%s", days.c_str() );
    if ( diff ) { // изменились дни выполнения
      ProgTrace( TRACE5, "diff days=%d", diff );
      days = AddDays( days, diff );
      ProgTrace( TRACE5, "days=%s", days.c_str() );
    }
    ProgTrace( TRACE5, "trip_id=%d, move_id=%d, num=%d, prior_first_day=%s, prior_last_day=%s, prior_days=%s, "
               "first_day=%s, last_day=%s, days=%s",
               trip_id, move_id, sched_num,
               DateTimeToStr( prior_first_day, "dd.mm.yyyy hh:nn" ).c_str(),
               DateTimeToStr( prior_last_day, "dd.mm.yyyy hh:nn" ).c_str(),
               prior_days.c_str(),
               DateTimeToStr( first_day, "dd.mm.yyyy hh:nn" ).c_str(),
               DateTimeToStr( last_day, "dd.mm.yyyy hh:nn" ).c_str(),
               days.c_str() );
    UQry.SetVariable( "trip_id", trip_id );
    UQry.SetVariable( "move_id", move_id );
    UQry.SetVariable( "num", sched_num );
    UQry.SetVariable( "new_first_day", first_day );
    UQry.SetVariable( "new_last_day", last_day );
    UQry.SetVariable( "new_days", days );
    UQry.Execute();
    Qry.Next();
    OraSession.Commit();
  }
    Qry.Clear();
    Qry.SQLText =
      "SELECT SCHED_DAYS.TRIP_ID,SCHED_DAYS.MOVE_ID,SCHED_DAYS.NUM,SCHED_DAYS.FIRST_DAY, airp,scd_in,delta_in,scd_out,delta_out,routes.num route_num "
      " FROM ROUTES,SCHED_DAYS, SEASONS, "
      "( SELECT id region FROM DATE_TIME_ZONESPEC "
      " WHERE id IN ("
      "'Europe/Kaliningrad',"
      "'Europe/Moscow',"
      "'Europe/Samara',"
      "'Asia/Yekaterinburg',"
      "'Asia/Novosibirsk',"
      "'Asia/Krasnoyarsk',"
      "'Asia/Irkutsk',"
      "'Asia/Yakutsk',"
      "'Asia/Sakhalin',"
      "'Asia/Srednekolymsk',"
      "'Asia/Anadyr',"
      "'Asia/Novokuznetsk',"
      "'Asia/Chita',"
      "'Asia/Magadan' ) ) d "
      " WHERE "
      " ROUTES.MOVE_ID=SCHED_DAYS.MOVE_ID AND "
      " seasons.region=d.region AND seasons.hours=1 AND SEASONS.REGION=SCHED_DAYS.REGION AND SCD_IN IS NOT NULL AND "
      " TRUNC(FIRST_DAY)+DELTA_IN+(SCD_IN-TRUNC(SCD_IN)) NOT BETWEEN SEASONS.FIRST AND SEASONS.LAST AND "
      " FIRST_DAY BETWEEN SEASONS.FIRST AND SEASONS.LAST "
      " UNION "
      " SELECT SCHED_DAYS.TRIP_ID,SCHED_DAYS.MOVE_ID,SCHED_DAYS.NUM,SCHED_DAYS.FIRST_DAY, airp,scd_in,delta_in,scd_out,delta_out,routes.num route_num "
      " FROM ROUTES,SCHED_DAYS, SEASONS, "
      "( SELECT id region FROM DATE_TIME_ZONESPEC "
      " WHERE id IN ("
      "'Europe/Kaliningrad',"
      "'Europe/Moscow',"
      "'Europe/Samara',"
      "'Asia/Yekaterinburg',"
      "'Asia/Novosibirsk',"
      "'Asia/Krasnoyarsk',"
      "'Asia/Irkutsk',"
      "'Asia/Yakutsk',"
      "'Asia/Sakhalin',"
      "'Asia/Srednekolymsk',"
      "'Asia/Anadyr',"
      "'Asia/Novokuznetsk',"
      "'Asia/Chita',"
      "'Asia/Magadan' ) ) d "
      " WHERE "
      " ROUTES.MOVE_ID=SCHED_DAYS.MOVE_ID AND "
      " seasons.region=d.region AND seasons.hours=1 AND SEASONS.REGION=SCHED_DAYS.REGION AND SCD_OUT IS NOT NULL AND "
      " TRUNC(FIRST_DAY)+DELTA_OUT+(SCD_OUT-TRUNC(SCD_OUT)) NOT BETWEEN SEASONS.FIRST AND SEASONS.LAST AND "
      " FIRST_DAY BETWEEN SEASONS.FIRST AND SEASONS.LAST "
      " order by first_day ";
    tst();
    Qry.Execute();
    tst();
    while ( !Qry.Eof ) {
      vector<TSTrip>::iterator i=trips.end();
      for ( vector<TSTrip>::iterator i=trips.begin(); i!=trips.end(); i++ ) {
        if ( i->trip_id == Qry.FieldAsInteger( "trip_id" ) &&
             i->move_id == Qry.FieldAsInteger( "move_id" ) ) {
          ProgTrace( TRACE5, "trip already convert: trip_id=%d, move_id=%d, num=%d", i->trip_id, i->move_id, i->num );
          break;
        }
      }
      if ( i == trips.end() ) {
        ProgTrace( TRACE5, "trip convert: trip_id=%d, move_id=%d, num=%d", Qry.FieldAsInteger( "trip_id" ), Qry.FieldAsInteger( "move_id" ), Qry.FieldAsInteger( "num" ) );
        string city = ((TAirpsRow&)baseairps.get_row( "code", Qry.FieldAsString( "airp" )  , true )).city;
        TCitiesRow& row=(TCitiesRow&)base_tables.get("cities").get_row("code",city,true);
        string city_region;
        try {
           city_region = CityTZRegion( city );
           if ( city_region.empty() )
             throw EXCEPTIONS::Exception("region empty");
        }
        catch(...) {
          ProgError(STDLOG,"city=%s, region empty move_id=%d", city.c_str(), move_id );
          Qry.Next();
          continue;
       }
        if ( row.country == "РФ" ) {
            TDateTime trunc_f = ASTRA::NoExists, scd_in = ASTRA::NoExists, prior_scd_in = ASTRA::NoExists,
                      scd_out = ASTRA::NoExists, prior_scd_out = ASTRA::NoExists;
            modf( Qry.FieldAsDateTime( "first_day" ), &trunc_f );
            int delta_in = 0, prior_delta_in = 0, delta_out = 0, prior_delta_out = 0;
            if ( Qry.FieldIsNULL( "scd_in" ) )
              prior_scd_in = NoExists;
            else
              prior_scd_in = Qry.FieldAsDateTime( "scd_in" );
            prior_delta_in = Qry.FieldAsInteger( "delta_in" );

            if (  prior_scd_in != NoExists ) {
              scd_in = trunc_f + prior_delta_in + prior_scd_in;
              ProgTrace( TRACE5, "scd_in=%f, scd_in=%s", scd_in, DateTimeToStr( scd_in, "dd.mm.yyyy hh:nn" ).c_str() );
              scd_in += getdiffhours( city_region ) /24.0;
              ProgTrace( TRACE5, "scd_in=%f, scd_in=%s", scd_in, DateTimeToStr( scd_in, "dd.mm.yyyy hh:nn" ).c_str() );
              TDateTime f2, f3;
              f2 = modf( scd_in, &f3 );
              if ( f3 < trunc_f )
                scd_in = f3 - trunc_f - f2;
              else
                scd_in = f3 - trunc_f + f2;
              double df;
              scd_in = modf( scd_in, &df );
              delta_in = (int)df;
/*              if ( prior_delta_in != 0 || delta_in < 0 ) {
                delta_in = prior_delta_in;
                scd_in = prior_scd_in;
                tst();
              }*/
            }

            if ( Qry.FieldIsNULL( "scd_out" ) )
              prior_scd_out = NoExists;
            else
              prior_scd_out = Qry.FieldAsDateTime( "scd_out" );
            prior_delta_out = Qry.FieldAsInteger( "delta_out" );
            if ( prior_scd_out != NoExists ) {
              scd_out = trunc_f + prior_delta_out + prior_scd_out;
              ProgTrace( TRACE5, "scd_out=%f, scd_out=%s", scd_out, DateTimeToStr( scd_out, "dd.mm.yyyy hh:nn" ).c_str() );
              scd_out += getdiffhours( city_region )/24.0;
              ProgTrace( TRACE5, "scd_out=%f, scd_out=%s", scd_out, DateTimeToStr( scd_out, "dd.mm.yyyy hh:nn" ).c_str() );
              TDateTime f2, f3;
              f2 = modf( scd_out, &f3 );
              if ( f3 < trunc_f )
                scd_out = f3 - trunc_f - f2;
              else
                scd_out = f3 - trunc_f + f2;
              ProgTrace( TRACE5, "scd_out=%f, scd_out=%s", scd_out, DateTimeToStr( scd_out, "dd.mm.yyyy hh:nn" ).c_str() );
              double df;
              scd_out = modf( scd_out, &df );
              delta_out = (int)df;
/*              if ( prior_delta_out != 0 || delta_out < 0 ) {
                delta_out = prior_delta_out;
                scd_out = prior_scd_out;
                tst();
              }*/
            }
            RUQry.SetVariable( "move_id", Qry.FieldAsInteger( "move_id" ) );
            RUQry.SetVariable( "num", Qry.FieldAsInteger( "route_num" ) );
            if ( prior_scd_in != NoExists ) {
              RUQry.SetVariable( "new_scd_in", scd_in );
              RUQry.SetVariable( "new_delta_in", delta_in );
            }
            else {
              RUQry.SetVariable( "new_scd_in", FNull );
              RUQry.SetVariable( "new_delta_in", FNull );
            }
            if ( prior_scd_out != NoExists ) {
              RUQry.SetVariable( "new_scd_out", scd_out );
              RUQry.SetVariable( "new_delta_out", delta_out );
            }
            else {
              RUQry.SetVariable( "new_scd_out", FNull );
              RUQry.SetVariable( "new_delta_out", FNull );
            }
            RUQry.Execute();
        }
      }
      Qry.Next();
      OraSession.Commit();
    }


  }
  catch(...){
    throw;
  };
  return 0;
}

