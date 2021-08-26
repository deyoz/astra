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
#include "pg_session.h"

#include <serverlib/query_runner.h>
#include <serverlib/testmode.h>
#include <edilib/edilib_dbpg_callbacks.h>
#include <edilib/edi_loading.h>

#define NICKNAME "VLAD"
#include <serverlib/slogger.h>

#include "tlg/lci_parser.h"
#include "html_pages.h"
#include "web_main.h"
#include "kiosk/kiosk_config.h"
#include "db_pkg.h"
#include "nosir_create_tlg.h"
#include "nosir_crstxt.h"
#include "stat/stat_rfisc.h"
#include "stat/stat_self_ckin.h"
#include "stat/stat_limited_capab.h"
#include "stat/stat_seDCSAddReport.h"
#include "stat/stat_departed.h"
#include "stat/stat_ovb.h"
#include "dbcpp_nosir_check.h"
#include "tripinfo.h"

int nosir_test(int argc,char **argv);
void nosir_test_help(const char *name);

int edi_load_messages_local(int argc, char** argv);
int nosir_tscript(int argc, char** argv);
int verifyHTTP(int argc,char **argv);
int test_reprint(int argc,char **argv);
int season_to_schedules(int argc,char **argv);
int test_typeb_utils(int argc,char **argv);
//int test_typeb_utils2(int argc,char **argv);
int test_sopp_sql(int argc,char **argv);
int ego_stat(int argc,char **argv);
int tst_vo(int, char**);
int prn_tags(int argc, char **argv);
int stat_belgorod(int argc, char **argv);
int rbd_test(int argc, char **argv);
int tzdump(int argc, char **argv);
int tzdiff(int argc, char **argv);
int print_pg_tables(int argc, char **argv);
int seat_no_test_single(int argc, char **argv);

#ifdef XP_TESTING

void NosirTrace(int l, const char *nickname, const char *filename,int line, const char *format,  ...)
{
  LogTrace(l,nickname,filename,line)<<"NosirTrace";
  va_list ap;
  va_start(ap, format);
  if(inTestMode()) {
    //LogTrace(l,nickname,filename,line)<<"inTestMode";
  }
  else {
    //LogTrace(l,nickname,filename,line)<<"not inTestMode";
    vfprintf(stdout,format,ap);
    fputc('\n',stdout);
  }
  va_end(ap);
}

int pg_sessions_check(int argc,char **argv)
{
  NosirTrace(TRACE1,"\npg_sessions_check started..." );
  {
    NosirTrace(TRACE1,"check_autonomous_sessions_load_save_consistency started..." );
    std::vector<DbCpp::TCheckSessionsLoadSaveConsistency> res
          =DbCpp::check_autonomous_sessions_load_save_consistency();

    if(!res.empty())
    {
      for(auto const& i : res)
      {
        NosirTrace(TRACE1,"  %s:%zi %s",i.file,i.line,i.text_error.c_str());
      }
      NosirTrace(TRACE1,"pg_autonomous_session_check process finished with error(s)...\n" );
      return 1;
    }
    NosirTrace(TRACE1,"check_autonomous_sessions_load_save_consistency finished successfully" );
  }

  NosirTrace(TRACE1,"pg_sessions_check process finished successfully\n" );

  return 0;
}
#endif

void init_edilib_callbacks();

const
  struct {
    const char *name;
    int (*p)(int argc,char **argv);     //��楤�� ��ࠡ�⪨
    void (*h)(const char *name);                    //������
    const char *description;
  } obrnosirnick []={
    {"-test",                   nosir_test,             nosir_test_help,            NULL},
    {"-ediinsert",              edi_load_messages_local, NULL,                       "loading edifact templates"},
    {"-testbm",                 testbm,                 NULL,                       NULL},
    {"-load_fr",                load_fr,                NULL,                       "loading FR files to database"},
    {"-get_fr",                 get_fr,                 NULL,                       "getting FR files from database to local path"},
    {"-termversion",            SetTermVersionNotice,   SetTermVersionNoticeHelp,   NULL},
    {"-create_apis",            create_apis_nosir,      create_apis_nosir_help,     NULL},
    {"-send_tlg",               send_tlg,               send_tlg_help,              NULL},
    {"-lci",                    TypeB::lci,             NULL,                       NULL},
    {"-basel_stat",             basel_stat,             NULL,                       NULL},
    {"-testsalons",             testsalons,             NULL,                       NULL},
    {"-test_typeb_utils",       test_typeb_utils,       NULL,                       NULL},
    {"-test_sopp_sql",          test_sopp_sql,          NULL,                       NULL},
#ifdef XP_TESTING
    {"-tscript",                nosir_tscript,          NULL,                       NULL},
#endif//XP_TESTING
    {"-file_by_id",             file_by_id,             NULL,                       NULL},
    // {"-dst_points",             points_dst_format,      NULL,                       NULL},
    {"-ovb",                    STAT::ovb,              NULL,                       NULL},
    {"-http",                   verifyHTTP,             NULL,                       NULL},
    {"-bcbp",                   AstraWeb::bcbp_test,    NULL,                       NULL},
    {"-ego_stat",               ego_stat,               NULL,                       NULL},
    {"-rfisc_stat",             nosir_rfisc_stat,       NULL,                       NULL},
    {"-rfisc_all",              nosir_rfisc_all,        NULL,                       NULL},
    {"-test_reprint",           test_reprint,    NULL,                       NULL},
    {"-ffp",                    ffp,                    ffp_help,                   "getting FFP card status"},
    {"-parse_bcbp",             nosir_parse_bcbp,       NULL,                       NULL},
    {"-departed_pax",           nosir_departed_pax,     NULL,                       NULL},
    {"-departed",               nosir_departed,         NULL,                       NULL},
    {"-sql",                    nosir_departed_sql,     NULL,                       NULL},
    {"-seDCSAddReport",         nosir_seDCSAddReport,   NULL,                       NULL},
    {"-lim_capab_stat",         nosir_lim_capab_stat,   NULL,                       NULL},
//    {"-convert_tz",             tz_conversion,          NULL,                       NULL},
//    {"-test_cnv",               test_conversion,        NULL,                       NULL},
    {"-vo",                     tst_vo,                 NULL,                       NULL},
    {"-html_to_db",             html_to_db,             NULL,                       "loading html files to database"},
    {"-html_from_db",           html_from_db,           NULL,                       "getting html files from database to local path"},
    {"-prn_tags",               prn_tags,               NULL,                       NULL},
    {"-kuf_fix",                KUF_STAT::fix,          NULL,                       NULL},
    {"-stat_belgorod",          stat_belgorod,          NULL,                       NULL},
    {"-apis_test",              apis_test_single,              NULL,                       NULL},
    {"-seat_no_test",           seat_no_test_single,    NULL,                       NULL},
    {"-alias_to_db",            KIOSKCONFIG::alias_to_db, NULL,                       NULL},
    {"-db_pkg",                 db_pkg,                 NULL,                       NULL},
    {"-rbd_test",               rbd_test,               NULL,                       NULL},
    {"-lci_data",               TypeB::lci_data,               NULL,                       NULL},
    {"-asvc_list_print_sql",    PaxASVCList::print_sql, NULL,                       NULL},
    {"-tzdump",                 tzdump,                 NULL,                       NULL},
    {"-tzdiff",                 tzdiff,                 NULL,                       NULL},
    {"-create_tlg",             nosir_create_tlg,       NULL,                       NULL},
    {"-crstxt",                 nosir_crstxt,           NULL,                       NULL},
    {"-dump_typeb_out",         nosirDumpTypeBOut,      nosirDumpTypeBOutUsage,     NULL},
    {"-comp_elem_types_to_db",  comp_elem_types_to_db,  NULL,                       NULL},
    {"-comp_elem_types_from_db",comp_elem_types_from_db,NULL,                       NULL},
    {"-pg-tables",              print_pg_tables,        NULL,                       NULL},
  #ifdef XP_TESTING
     {"-pg_sessions_check", pg_sessions_check, NULL, "Check main PG session consistency for different methods of usage PG" },
 #endif

  };

int nosir_test(int argc,char **argv)
{
  printf("argc=%i\n",argc);
  for(int i=0; i<argc; i++)
    printf("argv[%i]='%s'\n",i,argv[i]);
  return 0;
}

void nosir_test_help(const char *name)
{
  printf("  %-15.15s ", name);
  puts("Sample of 'astra -nosir' usage: printing all params");
}

int edi_load_messages_local(int argc, char** argv)
{
  init_edilib_callbacks();
  return edi_load_messages_main(argc, argv);
}

int main_nosir_user(int argc,char **argv)
{
  int res = -1;
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
        res=obrnosirnick[i].p(argc,argv);
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
        catch(...) {  ProgError(STDLOG, "Rollback or OraSession.LogOff error"); };
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
    E.showProgError();
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
  return res;
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
    if (commit_before_sleep) ASTRA::commit();
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

int seat_no_test_single(int argc, char **argv)
{
  tst();
    TQuery PointIdQry(&OraSession);
    //scd_out IS NOT NULL AND scd_out > sysdate-2
    PointIdQry.SQLText=
      //"SELECT point_id FROM points WHERE airline is NOT NULL AND pr_del=0 AND scd_out IS NOT NULL ORDER BY point_id";
       "SELECT 4911653 point_id FROM dual ";
  int c = 0;
  for ( int i=0; i<10; i++ ) {
      PointIdQry.Execute();
  for (;!PointIdQry.Eof;PointIdQry.Next()) {
    tst();
/*    XMLDoc reqDoc;
    std::string reqText=
    "<term>"
    "<query handle=\"0\" id=\"prepreg\" ver=\"1\" opr=\"DJEK\" screen=\"AIR.EXE\" mode=\"STAND\" lang=\"RU\" term_id=\"1215111772\">"
    "<ViewCRSList>"
    "<dev_model/>"
    "<fmt_type/>"
    "<prnParams>"
    "<pr_lat>0</pr_lat>"
    "<encoding>UTF-16LE</encoding>"
    "<offset>20</offset>"
    "<top>0</top>"
    "</prnParams>"
    "<point_id>4915579</point_id>"
    "</ViewCRSList>"
    "</query>"
    "</term>";
    reqDoc.set(reqText);
    //xmlNodePtr node = reqDoc.docPtr()->children->children;
    ReplaceTextChild(reqDoc.docPtr()->children->children->children,"point_id",PointIdQry.FieldAsInteger("point_id"));*/
    XMLDoc resDoc1("data");
    XMLDoc resDoc2("data");
    c++;
    LogTrace(TRACE5) << c << " " << PointIdQry.FieldAsInteger("point_id");
    try {
      boost::posix_time::ptime mcsTime = boost::posix_time::microsec_clock::universal_time();
      /*viewCRSList( PointIdQry.FieldAsInteger("point_id"),
                   {}, resDoc1.docPtr()->children, false );*/
      int exec_time = (boost::posix_time::microsec_clock::universal_time() - mcsTime).total_microseconds();
      LogTrace(TRACE5) << "old algo exec time=" <<exec_time;
      mcsTime = boost::posix_time::microsec_clock::universal_time();
      /*viewCRSList( PointIdQry.FieldAsInteger("point_id"),
                   {}, resDoc2.docPtr()->children, true );*/
      exec_time = (boost::posix_time::microsec_clock::universal_time() - mcsTime).total_microseconds();
      LogTrace(TRACE5) << "new algo exec time=" <<exec_time;
    }
    catch(const EXCEPTIONS::Exception &e) {
      LogError(STDLOG) << e.what();
      LogError(STDLOG) << PointIdQry.FieldAsInteger("point_id");
      continue;
    }
    if ( resDoc1.text() != resDoc2.text() ) {
      LogError(STDLOG) << PointIdQry.FieldAsInteger("point_id");
      //LogError(STDLOG) << resDoc1.text();
      //LogError(STDLOG) << resDoc2.text();
    }
  }
    }
  tst();
  return 0;
}


