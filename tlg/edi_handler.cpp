#ifndef __WIN32__
 #include <unistd.h>
 #include <errno.h>
 #include <tcl.h>
 #include <math.h>
#endif
#include "astra_utils.h"
#include "exceptions.h"
#include "oralib.h"
#include "tlg.h"
#include "tlg_parser.h"
#include "daemon.h"
#include "edi_tlg.h"
#include "edi_msg.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "test.h"

using namespace BASIC;
using namespace EXCEPTIONS;
//using namespace tlg_process;

#define WAIT_INTERVAL           1      //seconds
#define TLG_SCAN_INTERVAL	1   	//seconds
#define SCAN_COUNT              10      //кол-во разбираемых телеграмм за одно сканирование

static const char* OWN_CANON_NAME=NULL;
static const char* ERR_CANON_NAME=NULL;

//bool obr_tlg_queue::has_removed;

static void handle_tlg(void);

#ifdef __WIN32__
int main(int argc, char* argv[])
#else
int main_edi_handler_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist)
#endif
{
  try
  {
    try
    {
      OpenLogFile("logairimp");
      if ((OWN_CANON_NAME=Tcl_GetVar(interp,"OWN_CANON_NAME",TCL_GLOBAL_ONLY))==NULL||
          strlen(OWN_CANON_NAME)!=5)
        throw Exception("Unknown or wrong OWN_CANON_NAME");

      ERR_CANON_NAME=Tcl_GetVar(interp,"ERR_CANON_NAME",TCL_GLOBAL_ONLY);

      ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks()
              ->connect_db();
      if (init_edifact()<0) throw Exception("'init_edifact' error");

      time_t scan_time=0;

      for(;;)
      {
      	sleep(WAIT_INTERVAL);
        if (time(NULL)-scan_time>=TLG_SCAN_INTERVAL)
        {
          handle_tlg();
          scan_time=time(NULL);
        };
      }; // end of loop
    }
    catch(EOracleError E)
    {
#ifndef __WIN32__
      ProgError(STDLOG,"EOracleError %d: %s",E.Code,E.what());
#endif
      throw;
    }
    catch(Exception E)
    {
#ifndef __WIN32__
      ProgError(STDLOG,"Exception: %s",E.what());
#endif
      throw;
    };
  }
  catch(...) {};
  try
  {
    OraSession.Rollback();
    OraSession.LogOff();
  }
  catch(...) {};
  return 0;
};

void handle_tlg(void)
{
  static TQuery TlgQry(&OraSession);
  if (TlgQry.SQLText.IsEmpty())
  {
    //внимание порядок объединения таблиц важен!
    TlgQry.Clear();
    TlgQry.SQLText=
      "SELECT tlgs.id,tlgs.tlg_text,tlg_queue.time,ttl\
       FROM tlgs,tlg_queue\
       WHERE tlg_queue.id=tlgs.id AND tlg_queue.type='INA' AND tlg_queue.status='PUT'\
       ORDER BY tlg_queue.time,tlg_queue.id";
  };

  int count;

  count=0;
  TlgQry.Execute();
//  obr_tlg_queue tlg_obr(1); // Класс - обработчик телеграмм
  try
  {
      for(;!TlgQry.Eof && (count++)<SCAN_COUNT; TlgQry.Next(), OraSession.Rollback())
      {
          try{
              char *tlg;
              int len = TlgQry.GetSizeLongField("tlg_text");
              tlg = new (char [len]);
              tlg[len]=0;
              TlgQry.FieldAsLong("tlg_text", tlg);
              proc_edifact(tlg);
          }
          catch(edi_exception &e)
          {
              ProgTrace(TRACE0,"EdiExcept: %s:%s", e.errCode().c_str(), e.what());
          }
          catch(std::exception &e)
          {
              ProgError(STDLOG, "std::exception: %s", e.what());
          }
      };
  }
  catch(...)
  {
    throw;
  };
}

