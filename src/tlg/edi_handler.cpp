#include <unistd.h>
#include <errno.h>
#include <tcl.h>
#include <math.h>
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

using namespace BASIC;
using namespace EXCEPTIONS;
//using namespace tlg_process;

#define WAIT_INTERVAL           60      //seconds
#define TLG_SCAN_INTERVAL      600   	//seconds
#define SCAN_COUNT             100      //кол-во разбираемых телеграмм за одно сканирование

static void handle_tlg(void);

int main_edi_handler_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist)
{
  try
  {
    sleep(10);
    InitLogTime(NULL);
    OpenLogFile("logairimp");

    ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks()
            ->connect_db();
    if (init_edifact()<0) throw Exception("'init_edifact' error");

    time_t scan_time=0;
    char buf[10];
    for(;;)
    {
      InitLogTime(NULL);
      if (time(NULL)-scan_time>=TLG_SCAN_INTERVAL)
      {
        InitLogTime(NULL);
        base_tables.Invalidate();
        handle_tlg();
        scan_time=time(NULL);
      };
      if (waitCmd("CMD_EDI_HANDLER",WAIT_INTERVAL,buf,sizeof(buf)))
      {
        InitLogTime(NULL);
        base_tables.Invalidate();
        handle_tlg();
        scan_time=time(NULL);
      };
    }; // end of loop
  }
  catch(EOracleError &E)
  {
    ProgError(STDLOG,"EOracleError %d: %s",E.Code,E.what());
  }
  catch(Exception &E)
  {
    ProgError(STDLOG,"Exception: %s",E.what());
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

void handle_tlg(void)
{
  time_t time_start=time(NULL);

  static TQuery TlgQry(&OraSession);
  if (TlgQry.SQLText.IsEmpty())
  {
    //внимание порядок объединения таблиц важен!
    TlgQry.Clear();
    TlgQry.SQLText=
      "SELECT tlg_queue.id,tlgs.tlg_text,tlg_queue.time,ttl, "
      "       tlg_queue.tlg_num,tlg_queue.sender "
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
      for(;!TlgQry.Eof && (count++)<SCAN_COUNT; TlgQry.Next(), OraSession.Rollback())
      {
      	  tlg_id=TlgQry.FieldAsInteger("id");
          ProgTrace(TRACE1,"========= %d TLG: START HANDLE =============",tlg_id);
          ProgTrace(TRACE1,"========= (sender=%s tlg_num=%d) =============",
                    TlgQry.FieldAsString("sender"),
                    TlgQry.FieldAsInteger("tlg_num"));
          try{
              int len = TlgQry.GetSizeLongField("tlg_text");
              boost::shared_ptr< char > tlg (new (char [len+1]));
              TlgQry.FieldAsLong("tlg_text", tlg.get());
              tlg.get()[len]=0;
              ProgTrace(TRACE5,"TLG_IN: <%s>", tlg.get());
              proc_edifact(tlg.get());
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
          catch(Exception &e)
          {
              OraSession.Rollback();
              try
              {
                ProgError(STDLOG, "Exception: %s", e.what());
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
      };
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
}





