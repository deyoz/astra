#include <unistd.h>
#include <errno.h>
#include <tcl.h>
#include <math.h>
#include "astra_consts.h"
#include "astra_utils.h"
#include "base_tables.h"
#include "exceptions.h"
#include "oralib.h"
#include "tlg.h"
#include "tlg_parser.h"
#include "edi_tlg.h"
#include "edi_msg.h"

#include "serverlib/query_runner.h"
#include "serverlib/posthooks.h"
#include "serverlib/ourtime.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace ASTRA;
using namespace BASIC;
using namespace EXCEPTIONS;
//using namespace tlg_process;

static const int WAIT_INTERVAL()       //миллисекунды
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("EDI_HANDLER_WAIT_INTERVAL",1,NoExists,60000);
  return VAR;
};

static const int PROC_INTERVAL()       //миллисекунды
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("EDI_HANDLER_PROC_INTERVAL",0,10000,1000);
  return VAR;
};

static const int PROC_COUNT()          //кол-во разбираемых телеграмм за одну итерацию
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("EDI_HANDLER_PROC_COUNT",1,NoExists,100);
  return VAR;
};

static bool handle_tlg(void);

int main_edi_handler_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist)
{
  try
  {
    sleep(2);
    InitLogTime(NULL);
    OpenLogFile("logairimp");

    ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks()
            ->connect_db();
    if (init_edifact()<0) throw Exception("'init_edifact' error");

    char buf[10];
    for(;;)
    {
      InitLogTime(NULL);
      base_tables.Invalidate();
      bool queue_not_empty=handle_tlg();
      
      waitCmd("CMD_EDI_HANDLER",queue_not_empty?PROC_INTERVAL():WAIT_INTERVAL(),buf,sizeof(buf));
    }; // end of loop
  }
  catch(EOracleError &E)
  {
    ProgError(STDLOG,"EOracleError %d: %s",E.Code,E.what());
  }
  catch(std::exception &E)
  {
    ProgError(STDLOG,"std::exception: %s",E.what());
  }
  catch(...)
  {
    ProgError(STDLOG, "Unknown exception");
  };
  try
  {
    OraSession.Rollback();
    OraSession.LogOff();
  }
  catch(...)
  {
    ProgError(STDLOG, "Unknown exception");
  };
  return 0;
};

bool handle_tlg(void)
{
  bool queue_not_empty=false;

  time_t time_start=time(NULL);

  static TQuery TlgQry(&OraSession);
  if (TlgQry.SQLText.IsEmpty())
  {
    //внимание порядок объединения таблиц важен!
    TlgQry.Clear();
    TlgQry.SQLText=
      "SELECT tlg_queue.id,tlgs.tlg_text,tlg_queue.time,ttl, "
      "       tlg_queue.tlg_num,tlg_queue.sender, "
      "       NVL(tlg_queue.proc_attempt,0) AS proc_attempt "
      "FROM tlgs,tlg_queue "
      "WHERE tlg_queue.id=tlgs.id AND tlg_queue.receiver=:receiver AND "
      "      tlg_queue.type='INA' AND tlg_queue.status='PUT' "
      "ORDER BY tlg_queue.time,tlg_queue.id";
    TlgQry.CreateVariable("receiver",otString,OWN_CANON_NAME());
  };

  int count,tlg_id;

  count=0;
  TlgQry.Execute();
//  obr_tlg_queue tlg_obr(1); // Класс - обработчик телеграмм
  try
  {
      for(;!TlgQry.Eof && (count++)<PROC_COUNT(); TlgQry.Next(), OraSession.Rollback())
      {
      	  tlg_id=TlgQry.FieldAsInteger("id");
          ProgTrace(TRACE1,"========= %d TLG: START HANDLE =============",tlg_id);
          ProgTrace(TRACE1,"========= (sender=%s tlg_num=%d) =============",
                    TlgQry.FieldAsString("sender"),
                    TlgQry.FieldAsInteger("tlg_num"));
          if (TlgQry.FieldAsInteger("proc_attempt")>=HANDLER_PROC_ATTEMPTS())
          {
            ProgTrace(TRACE5, "handle_tlg: tlg_id=%d proc_attempt=%d", tlg_id, TlgQry.FieldAsInteger("proc_attempt"));
            errorTlg(tlg_id,"PROC");
            OraSession.Commit();
          }
          else
          try
          {
              procTlg(tlg_id);
              OraSession.Commit();
              
              std::string text=getTlgText(tlg_id, TlgQry);

              ProgTrace(TRACE5,"TLG_IN: <%s>", text.c_str());
              proc_edifact(text);
              deleteTlg(tlg_id);
              callPostHooksBefore();
              OraSession.Commit();
              callPostHooksAfter();
              emptyHookTables();
          }
          catch(edi_exception &e)
          {
              OraSession.Rollback();
              try
              {
                ProgTrace(TRACE0,"EdiExcept: %s:%s", e.errCode().c_str(), e.what());
                errorTlg(tlg_id,"PARS",e.what());
                OraSession.Commit();
              }
              catch(...) {};
          }
          catch(std::exception &e)
          {
              OraSession.Rollback();
              try
              {
                ProgError(STDLOG, "std::exception: %s", e.what());
                errorTlg(tlg_id,"PARS",e.what());
                OraSession.Commit();
              }
              catch(...) {};
          }
          catch(...)
          {
              OraSession.Rollback();
              try
              {
                ProgError(STDLOG, "Unknown error");
                errorTlg(tlg_id,"UNKN");
                OraSession.Commit();
              }
              catch(...) {};
          }
          ProgTrace(TRACE1,"========= %d TLG: DONE HANDLE =============",tlg_id);
          ProgTrace(TRACE5, "IN: PUT->DONE (sender=%s, tlg_num=%ld, time=%.10f)",
                            TlgQry.FieldAsString("sender"),
                            (unsigned long)TlgQry.FieldAsInteger("tlg_num"),
                            NowUTC());
          monitor_idle_zapr_type(1, QUEPOT_TLG_EDI);
      };
      queue_not_empty=!TlgQry.Eof;
  }
  catch(...)
  {
    ProgError(STDLOG, "Unknown error");
    throw;
  };
  time_t time_end=time(NULL);
  if (time_end-time_start>1)
    ProgTrace(TRACE5,"Attention! handle_tlg execute time: %ld secs, count=%d",
                     time_end-time_start,count);
                     
  return queue_not_empty;
}





