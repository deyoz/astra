#include <unistd.h>
#include <errno.h>
#include <tcl.h>
#include "astra_main.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "base_tables.h"
#include "exceptions.h"
#include "oralib.h"
#include "tlg.h"
#include "tlg_parser.h"
#include "apps_interaction.h"

#include <serverlib/query_runner.h>
#include <serverlib/posthooks.h>
#include <serverlib/ourtime.h>
#include <serverlib/TlgLogger.h>
#include <edilib/edi_func_cpp.h>

#define NICKNAME "ANNA"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/test.h>

using namespace std;

static int WAIT_INTERVAL()       //миллисекунды
{
  static int VAR=ASTRA::NoExists;
  if (VAR==ASTRA::NoExists)
    VAR=getTCLParam("APPS_ANSWER_EMUL_WAIT_INTERVAL",1,ASTRA::NoExists,3000);
  return VAR;
};

static int PROC_INTERVAL()       //миллисекунды
{
  static int VAR=ASTRA::NoExists;
  if (VAR==ASTRA::NoExists)
    VAR=getTCLParam("APPS_ANSWER_EMUL_PROC_INTERVAL",0,10000,1000);
  return VAR;
};

static int PROC_COUNT()          //кол-во разбираемых телеграмм за одну итерацию
{
  static int VAR=ASTRA::NoExists;
  if (VAR==ASTRA::NoExists)
    VAR=getTCLParam("APPS_ANSWER_EMUL_PROC_COUNT",1,ASTRA::NoExists,100);
  return VAR;
};

static bool handle_tlg( void );

int main_apps_answer_emul_tcl(int supervisorSocket, int argc, char *argv[])
{
  try
  {
    sleep(2);
    InitLogTime(argc>0?argv[0]:NULL);

    ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks()
            ->connect_db();
    init_locale();

    char buf[10];
    for(;;)
    {
      InitLogTime(argc>0?argv[0]:NULL);
      base_tables.Invalidate();
      bool queue_not_empty=handle_tlg();

      waitCmd("CMD_APPS_ANSWER_EMUL",queue_not_empty?PROC_INTERVAL():WAIT_INTERVAL(),buf,sizeof(buf));
    };
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
    ASTRA::rollback();
    OraSession.LogOff();
  }
  catch(...)
  {
    ProgError(STDLOG, "Unknown exception");
  };
  return 0;
}

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
      "SELECT tlg_queue.id,tlg_queue.time,ttl, "
      "       tlg_queue.tlg_num,tlg_queue.sender, "
      "       NVL(tlg_queue.proc_attempt,0) AS proc_attempt "
      "FROM tlg_queue "
      "WHERE tlg_queue.receiver=:receiver AND "
      "      tlg_queue.type='OAPP' "
      "ORDER BY tlg_queue.time,tlg_queue.id";
    TlgQry.CreateVariable("receiver",otString,getAPPSAddr());
  };

  int count;

  count=0;
  TlgQry.Execute();
  try
  {
      for(;!TlgQry.Eof && (count++)<PROC_COUNT(); TlgQry.Next(), ASTRA::rollback())
      {
        int id = TlgQry.FieldAsInteger("id");
        string text = getTlgText(id);
        ProgTrace(TRACE5,"TLG_IN: <%s>", text.c_str());

        // составим ответ
        string answer = emulateAnswer(text);

        if (!answer.empty()) {
          // отправим телеграмму
          sendTlg(OWN_CANON_NAME(), getAPPSAddr().c_str(), qpOutB, 20, answer,
                  ASTRA::NoExists, ASTRA::NoExists);

          sendCmd("CMD_APPS_HANDLER","H");
        }
        else
          ProgTrace(TRACE5,"No answer from remote system");

        // удалим обработанную телеграмму
        deleteTlg(id);
        callPostHooksBefore();
        ASTRA::commit();
        callPostHooksAfter();
        emptyHookTables();
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
