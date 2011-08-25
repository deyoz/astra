#include <stdio.h>
#include <string.h>

#include "telegram.h"
#include "oralib.h"
#include "maindcs.h"
#include "docs.h"
#include "load_fr.h"
#include "empty_proc.h"
#include "serverlib/query_runner.h"
#include "edilib/edi_loading.h"

#define NICKNAME "VLAD"
#include "serverlib/test.h"

/*
int LocalIsNosir=0;
*/
int nosir_test(int argc,char **argv);
void nosir_test_help(const char *name);

const
  struct {
    const char *name;
    int (*p)(int argc,char **argv);     //процедура обработки
    void (*h)(const char *name);                    //помощь
    const char *description;
  } obrnosirnick []={
    {"-test",          nosir_test,              nosir_test_help,          NULL},
    {"-ediinsert",     edi_load_messages_main,  NULL,                     "loading edifact templates"},
    {"-testbm",        testbm,                  NULL,                     NULL},
    {"-load_fr",       load_fr,                 NULL,                     "loading FR files to database"},
    {"-get_fr",        get_fr,                  NULL,                     "getting FR files from database to local path"},
    {"-termversion",   SetTermVersionNotice,    SetTermVersionNoticeHelp, NULL},
    {"-alter_db",      alter_db,                NULL,                     NULL},
    {"-send_tlg",      send_tlg,                send_tlg_help,            NULL}
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
          OraSession.Rollback();
        else
          OraSession.Commit();
        OraSession.LogOff();
      }
      catch(...)
      {
        try
        {
          OraSession.Rollback();
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


