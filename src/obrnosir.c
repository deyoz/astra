#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif
/* $Id$ */
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#define NICKNAME "SYSTEM"
#define NICKTRACE SYSTEM_TRACE
#include "edilib/edi_loading.h"
#include "serverlib/test.h"

int LocalIsNosir=0;

int nosir_test(int argc,char **argv);
int nosir_EdiInsert(int argc,char **argv);

struct {
  char *name;
  int (*p)(int argc,char **argv);
  char *description;
} obrnosirnick []={
  {"-test",          nosir_test,           "Sample of 'obrzap -nosir' usage - printing all params" },
  {"-ediinsert",     edi_load_messages_main,      "loading edifact templates"},
};

int nosir_test(int argc,char **argv)
{
  int  i;

  ProgTrace(TRACE5,"argc=%i",argc);
  printf("argc=%i\n",argc);
  for(i=0; i<argc; i++)
  {
    ProgTrace(TRACE5,"argv[%i]='%s'",i,argv[i]);
    fprintf(stdout,"argv[%i]='%s'\n",i,argv[i]);
  };
  return 0;
}

int main_nosir_user(int argc,char **argv)
{
  int  res=0;
  int  i;
  int  found=-1;

  LocalIsNosir=1;
  for (i=0;found<0 && i<sizeof(obrnosirnick)/sizeof(obrnosirnick[0]);i++)
    found=strcmp(argv[0],obrnosirnick[i].name)==0?i:found;
  if (found<0)
  {
    ProgError(STDLOG,"invalid argument %s",argv[0]);
    printf("invalid argument %s\n",argv[0]);
    LocalIsNosir=0;
    return 1;
  };
  ProgTrace(TRACE1,"found nosir func (name='%s' id=%i)",argv[0],found);
  ProgTrace(TRACE5,"argc=%i",argc);
  res=obrnosirnick[found].p(argc,argv);
  ProgTrace(TRACE1,"exit nosir func (name='%s' id=%i) res=%i",argv[0],found,
    res);
#ifdef TEST_150
  ProgTrace(TRACE1,"TEST_150 Malloc check!!!");
#endif

  LocalIsNosir=0;
  return res;
}

void help_nosir_user(void)
{
  int i;

  puts("Usage:");
  puts("  obrzap -help                           ; this page");
  puts("  sirena.tcl [user/password]  -nosir (key) <params>");
  puts("  obrzap sirena.tcl [user/password]  -nosir (key) <params>");
  puts("  obrzap [user/password] -nosir (key) <params>    ;use SIRENA_TCL environnment for sirena.tcl");
  puts("Function key (key) is:");
  for (i=0;i<sizeof(obrnosirnick)/sizeof(obrnosirnick[0]);i++)
    printf("  %-15.15s %s\n",obrnosirnick[i].name,obrnosirnick[i].description);
  return;
}

int IsNosir(void)
{
  return LocalIsNosir==1;
}

int print_nosir_scr(const char* format, ...)
{
  int  res=0;

  if (IsNosir())
  {
    va_list ap;

    va_start(ap,format);
    res=vprintf(format,ap);
    printf("\n");

    va_end(ap);
  }
  return res;
}

