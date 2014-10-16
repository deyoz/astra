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
#include "serverlib/query_runner.h"
#include "serverlib/ocilocal.h"
#include "serverlib/testmode.h"
#include "edilib/edi_loading.h"

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
int nosir_tscript(int argc, char** argv);
int tz2db(int argc,char **argv);

const
  struct {
    const char *name;
    int (*p)(int argc,char **argv);     //��楤�� ��ࠡ�⪨
    void (*h)(const char *name);                    //������
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
    {"-rollback096",            rollback096,            NULL,                       NULL},
    {"-mobile_stat",            mobile_stat,            NULL,                       NULL},
    {"-test_astra_locale_adv",  test_astra_locale_adv,  NULL,                       NULL},
    {"-insert_locales",         insert_locales,         NULL,                       NULL},
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
    puts(""); //�⤥��� ���⮩ ��ப�� �뢮� ��ࠡ��稪�
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
  vector<TSTrip> trips;
  try{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT trip_id,move_id,num,first_day,last_day,days,sched_days.tz FROM sched_days, seasons "
    "WHERE seasons.hours = 0 AND seasons.tz=sched_days.tz AND "
    "      first_day BETWEEN seasons.first AND seasons.last "
    "ORDER BY trip_id,move_id,num";
  Qry.Execute();
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
    trips.push_back( t );

    TDateTime prior_first_day = Qry.FieldAsDateTime( "first_day" );
    TDateTime prior_last_day = Qry.FieldAsDateTime( "last_day" );
    TDateTime trunc_pf, trunc_f;
    modf( prior_first_day, &trunc_pf );
    int trip_id = Qry.FieldAsInteger( "trip_id" );
    int sched_num = Qry.FieldAsInteger( "num" );
    string prior_days = Qry.FieldAsString( "days" );
    TDateTime first_day, last_day;
    string days;
    first_day = prior_first_day - 1.0/24.0;
    last_day = prior_last_day - 1.0/24.0;
    ProgTrace( TRACE5, "start, trip_id=%d, move_id=%d, num=%d", trip_id, Qry.FieldAsInteger( "move_id" ), sched_num );
    ProgTrace( TRACE5, "prior_first_day=%f, trunc_pf=%f", prior_first_day, trunc_pf );
    modf( first_day, &trunc_f );
    int diff = (int)trunc_f - (int)trunc_pf;

    if ( move_id != Qry.FieldAsInteger( "move_id" ) ) {
      move_id = Qry.FieldAsInteger( "move_id" );
      RQry.SetVariable( "move_id", move_id );
      RQry.Execute();
      TDateTime scd_in, prior_scd_in, scd_out, prior_scd_out;
      int delta_in, prior_delta_in, delta_out, prior_delta_out;
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

        if (  prior_scd_in != NoExists ) {
          ProgTrace( TRACE5, "before convert move_id=%d, route.num=%d, scd_in=%s, delta_in=%d, airp=%s",
                     move_id, route_num, DateTimeToStr( prior_scd_in, "dd.mm.yyyy hh:nn" ).c_str(),
                     prior_delta_in, RQry.FieldAsString( "airp" ) );
          if ( row.country == "��" || row.country == "��" || row.country == "��" ) {
            scd_in = trunc_f + prior_delta_in + prior_scd_in;
            ProgTrace( TRACE5, "scd_in=%f, scd_in=%s", scd_in, DateTimeToStr( scd_in, "dd.mm.yyyy hh:nn" ).c_str() );
            scd_in -= 1.0/24.0;
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
          }
          else {
            scd_in = prior_scd_in;
            delta_in = prior_delta_in;
          }
          if ( !pr_dest_flight_time && row.country == "��" ) {
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
          if ( row.country == "��" || row.country == "��" || row.country == "��" ) {
            scd_out = trunc_f + prior_delta_out + prior_scd_out;
            ProgTrace( TRACE5, "scd_out=%f, scd_out=%s", scd_out, DateTimeToStr( scd_out, "dd.mm.yyyy hh:nn" ).c_str() );
            scd_out -= 1.0/24.0;
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
          if ( !pr_dest_flight_time && row.country == "��" ) {
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
    if ( diff ) { // ���������� ��� �믮������
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
      " FROM ROUTES,SCHED_DAYS, SEASONS "
      " WHERE "
      " ROUTES.MOVE_ID=SCHED_DAYS.MOVE_ID AND "
      " SEASONS.HOURS = 1 AND SEASONS.TZ=SCHED_DAYS.TZ AND SCD_IN IS NOT NULL AND "
      " TRUNC(FIRST_DAY)+DELTA_IN+(SCD_IN-TRUNC(SCD_IN)) NOT BETWEEN SEASONS.FIRST AND SEASONS.LAST AND "
      " FIRST_DAY BETWEEN SEASONS.FIRST AND SEASONS.LAST "
      " UNION "
      " SELECT SCHED_DAYS.TRIP_ID,SCHED_DAYS.MOVE_ID,SCHED_DAYS.NUM,SCHED_DAYS.FIRST_DAY, airp,scd_in,delta_in,scd_out,delta_out,routes.num route_num "
      " FROM ROUTES,SCHED_DAYS, SEASONS "
      " WHERE "
      " ROUTES.MOVE_ID=SCHED_DAYS.MOVE_ID AND "
      " SEASONS.HOURS = 1 AND SEASONS.TZ=SCHED_DAYS.TZ AND SCD_OUT IS NOT NULL AND "
      " TRUNC(FIRST_DAY)+DELTA_OUT+(SCD_OUT-TRUNC(SCD_OUT)) NOT BETWEEN SEASONS.FIRST AND SEASONS.LAST AND "
      " FIRST_DAY BETWEEN SEASONS.FIRST AND SEASONS.LAST "
      " order by first_day ";
    Qry.Execute();
    while ( !Qry.Eof ) {
      vector<TSTrip>::iterator i=trips.end();
      for ( vector<TSTrip>::iterator i=trips.begin(); i!=trips.end(); i++ ) {
        if ( i->trip_id == Qry.FieldAsInteger( "trip_id" ) &&
             i->move_id == Qry.FieldAsInteger( "move_id" ) &&
             i->num == Qry.FieldAsInteger( "num" )  ) {
          ProgTrace( TRACE5, "trip already convert: trip_id=%d, move_id=%d, num=%d", i->trip_id, i->move_id, i->num );
          break;
        }
      }
      if ( i == trips.end() ) {
        ProgTrace( TRACE5, "trip convert: trip_id=%d, move_id=%d, num=%d", Qry.FieldAsInteger( "trip_id" ), Qry.FieldAsInteger( "move_id" ), Qry.FieldAsInteger( "num" ) );
        string city = ((TAirpsRow&)baseairps.get_row( "code", Qry.FieldAsString( "airp" )  , true )).city;
        TCitiesRow& row=(TCitiesRow&)base_tables.get("cities").get_row("code",city,true);
        if ( row.country == "��" || row.country == "��" || row.country == "��" ) {
            TDateTime trunc_f, scd_in, prior_scd_in, scd_out, prior_scd_out;
            modf( Qry.FieldAsDateTime( "first_day" ), &trunc_f );
            int delta_in, prior_delta_in, delta_out, prior_delta_out;
            if ( Qry.FieldIsNULL( "scd_in" ) )
              prior_scd_in = NoExists;
            else
              prior_scd_in = Qry.FieldAsDateTime( "scd_in" );
            prior_delta_in = Qry.FieldAsInteger( "delta_in" );

            if (  prior_scd_in != NoExists ) {
              scd_in = trunc_f + prior_delta_in + prior_scd_in;
              ProgTrace( TRACE5, "scd_in=%f, scd_in=%s", scd_in, DateTimeToStr( scd_in, "dd.mm.yyyy hh:nn" ).c_str() );
              scd_in -= 1.0/24.0;
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
              scd_out -= 1.0/24.0;
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

