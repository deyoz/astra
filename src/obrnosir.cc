#include "telegram.h"
#include "maindcs.h"
#include "docs/docs_btm.h"
#include "load_fr.h"
#include "apis_creator.h"
#include "salons.h"
#include "file_queue.h"
#include "basel_aero.h"
#include "ffp_sirena.h"
#include "baggage_calc.h"
#include "serverlib/query_runner.h"
#include "edilib/edi_loading.h"

#define NICKNAME "VLAD"
#include "serverlib/slogger.h"

#include "tlg/lci_parser.h"
#include "img.h"
#include "collect.h"
#include "html_pages.h"
#include "web_main.h"
#include "kiosk/kiosk_config.h"
#include "db_pkg.h"
#include "nosir_create_tlg.h"
#include "stat/stat_rfisc.h"
#include "stat/stat_self_ckin.h"
#include "stat/stat_limited_capab.h"
#include "stat/stat_seDCSAddReport.h"
#include "stat/stat_departed.h"
#include "stat/stat_ovb.h"
#include "nosir_prepare_flt.h"

int nosir_test(int argc,char **argv);
void nosir_test_help(const char *name);

int seasons_dst_format(int argc,char **argv);
int points_dst_format(int argc,char **argv);
int nosir_tscript(int argc, char** argv);
int tz2db(int argc,char **argv);
int verifyHTTP(int argc,char **argv);
int pc_wt_stat(int argc,char **argv);
int test_reprint(int argc,char **argv);
int get_events_stat(int argc,char **argv);
int get_events_stat2(int argc,char **argv);
int season_to_schedules(int argc,char **argv);
int test_typeb_utils(int argc,char **argv);
int test_typeb_utils2(int argc,char **argv);
int test_sopp_sql(int argc,char **argv);
int test_file_queue(int argc,char **argv);
namespace NatStat { int nat_stat(int argc,char **argv); }
int ego_stat(int argc,char **argv);
int tst_vo(int, char**);
int prn_tags(int argc, char **argv);
int stat_belgorod(int argc, char **argv);
int rbd_test(int argc, char **argv);
int tzdump(int argc, char **argv);
int tzdiff(int argc, char **argv);

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
    {"-load_img",               img::load_img,          NULL,                       "loading img files to database"},
    {"-get_img",                img::get_img,           NULL,                       "getting img files from database to local path"},
    {"-termversion",            SetTermVersionNotice,   SetTermVersionNoticeHelp,   NULL},
    {"-create_apis",            create_apis_nosir,      create_apis_nosir_help,     NULL},
    {"-send_tlg",               send_tlg,               send_tlg_help,              NULL},
    {"-dst_seasons",            seasons_dst_format,     NULL,                       NULL},
    {"-agent_stat_delta",       STAT::agent_stat_delta, NULL,                       NULL},
    {"-lci",                    TypeB::lci,             NULL,                       NULL},
    {"-tz2db",                  tz2db,                  NULL,                       "reload date_time_zonespec.csv content to db"},
    {"-get_events_stat",        get_events_stat2,       NULL,                       NULL},
    {"-basel_stat",             basel_stat,             NULL,                       NULL},
    {"-testsalons",             testsalons,             NULL,                       NULL},
    {"-test_typeb_utils",       test_typeb_utils,       NULL,                       NULL},
    {"-test_sopp_sql",          test_sopp_sql,          NULL,                       NULL},
    {"-test_file_queue",        test_file_queue,        NULL,                       NULL},
#ifdef XP_TESTING
    {"-tscript",                nosir_tscript,          NULL,                       NULL},
#endif//XP_TESTING
    {"-file_by_id",             file_by_id,             NULL,                       NULL},
    {"-dst_points",             points_dst_format,      NULL,                       NULL},
    {"-ovb",                    STAT::ovb,              NULL,                       NULL},
    {"-http",                   verifyHTTP,             NULL,                       NULL},
    {"-bcbp",                   AstraWeb::bcbp_test,    NULL,                       NULL},
    {"-nat_stat",               NatStat::nat_stat,      NULL,                       NULL},
    {"-ego_stat",               ego_stat,               NULL,                       NULL},
    {"-pc_wt_stat",             pc_wt_stat,             NULL,                       NULL},
    {"-rfisc_stat",             nosir_rfisc_stat,       NULL,                       NULL},
    {"-test_reprint",           test_reprint,    NULL,                       NULL},
    {"-ffp",                    ffp,                    ffp_help,                   "getting FFP card status"},
    {"-departed_pax",           nosir_departed_pax,     NULL,                       NULL},
    {"-departed",               nosir_departed,         NULL,                       NULL},
    {"-sql",                    nosir_departed_sql,     NULL,                       NULL},
    {"-seDCSAddReport",         nosir_seDCSAddReport,   NULL,                       NULL},
//    {"-convert_tz",             tz_conversion,          NULL,                       NULL},
//    {"-test_cnv",               test_conversion,        NULL,                       NULL},
    {"-bp",                     bp_tst,                 NULL,                       NULL},
    {"-vo",                     tst_vo,                 NULL,                       NULL},
    {"-annul_bag",              nosir_annul_bag,        NULL,                       NULL},
    {"-html_to_db",             html_to_db,             NULL,                       "loading html files to database"},
    {"-html_from_db",           html_from_db,           NULL,                       "getting html files from database to local path"},
    {"-test_norms",             WeightConcept::test_norms,             NULL,                       NULL},
    {"-prn_tags",               prn_tags,               NULL,                       NULL},
    {"-kuf_fix",                KUF_STAT::fix,          NULL,                       NULL},
    {"-stat_belgorod",          stat_belgorod,          NULL,                       NULL},
    {"-apis_test",              apis_test_single,              NULL,                       NULL},
    {"-alias_to_db",            KIOSKCONFIG::alias_to_db, NULL,                       NULL},
    {"-db_pkg",                 db_pkg,                 NULL,                       NULL},
    {"-rbd_test",               rbd_test,               NULL,                       NULL},
    {"-lci_data",               TypeB::lci_data,               NULL,                       NULL},
    {"-asvc_list_print_sql",    PaxASVCList::print_sql, NULL,                       NULL},
    {"-tzdump",                 tzdump,                 NULL,                       NULL},
    {"-tzdiff",                 tzdiff,                 NULL,                       NULL},
    {"-create_tlg",             nosir_create_tlg,       NULL,                       NULL},
    {"-load_comp_elem_types",   load_comp_elem_types,   NULL,                       NULL},
    {"-prepare_flt",            nosir_prepare_flt,      NULL,                       NULL},
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

        callPostHooksBefore();
        if(res != 0)
            ASTRA::rollback();
        else
            ASTRA::commit();
        callPostHooksAfter();
        emptyHookTables();
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

void nosir_wait(int processed, bool commit_before_sleep=false, int work_secs=5, int sleep_secs=5)
{
  static time_t start_time=time(NULL);
  if (time(NULL)-start_time>=work_secs)
  {
    if (commit_before_sleep) OraSession.Commit();
    printf("%d iterations processed. sleep...", processed);
    fflush(stdout);
    sleep(sleep_secs);
    printf("go!\n");
    start_time=time(NULL);
  };
}

bool getDateRangeFromArgs(int argc, char **argv,
                          TDateTime& firstDate,
                          TDateTime& lastDate)
{
  firstDate=ASTRA::NoExists;
  lastDate=ASTRA::NoExists;

  if(argc < 3) {
      cout << "usage: " << argv[0] << " yyyymmdd yyyymmdd" << endl;
      return false;
  }

  if(StrToDateTime(argv[1], "yyyymmdd", firstDate) == EOF) {
      cout << "wrong first date: " << argv[1] << endl;
      return false;
  }

  if(StrToDateTime(argv[2], "yyyymmdd", lastDate) == EOF) {
      cout << "wrong last date: " << argv[2] << endl;
      return false;
  }

  if (lastDate<=firstDate) {
      cout << "wrong range: [" << argv[1] << ", " << argv[2] << ")" << endl;
      return false;
  }

  return true;
}

