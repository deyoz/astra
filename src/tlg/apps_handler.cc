#include <unistd.h>
#include <errno.h>
#include <tcl.h>
#include "astra_main.h"
#include "astra_misc.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "base_tables.h"
#include "exceptions.h"
#include "oralib.h"
#include "tlg.h"
#include "tlg_parser.h"
#include "apps_interaction.h"
#include "alarms.h"
#include "date_time.h"

#include <serverlib/query_runner.h>
#include <serverlib/posthooks.h>
#include <serverlib/ourtime.h>
#include <serverlib/TlgLogger.h>
#include <edilib/edi_func_cpp.h>
#include <libtlg/telegrams.h>

#define NICKNAME "ANNA"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/test.h>

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC::date_time;

static int WAIT_INTERVAL()       //миллисекунды
{
  static int VAR=ASTRA::NoExists;
  if (VAR==ASTRA::NoExists)
    VAR=getTCLParam("APPS_HANDLER_WAIT_INTERVAL",1,ASTRA::NoExists,5000);
  return VAR;
};

static int PROC_INTERVAL()       //миллисекунды
{
  static int VAR=ASTRA::NoExists;
  if (VAR==ASTRA::NoExists)
    VAR=getTCLParam("APPS_HANDLER_PROC_INTERVAL",0,10000,1000);
  return VAR;
};

static int PROC_COUNT()          //кол-во разбираемых телеграмм за одну итерацию
{
  static int VAR=ASTRA::NoExists;
  if (VAR==ASTRA::NoExists)
    VAR=getTCLParam("APPS_HANDLER_PROC_COUNT",1,ASTRA::NoExists,100);
  return VAR;
};

static bool handle_tlg(void);
static void resend_tlg(void);

int main_apps_handler_tcl(int supervisorSocket, int argc, char *argv[])
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
      resend_tlg();

      waitCmd("CMD_APPS_HANDLER",queue_not_empty?PROC_INTERVAL():WAIT_INTERVAL(),buf,sizeof(buf));
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

void handle_apps_tlg(const tlg_info &tlg)
{
    hth::HthInfo h2h = {};
    bool isH2h = false;

    tlgnum_t tlgNum(std::to_string(tlg.id));
    if (!telegrams::callbacks()->readHthInfo(tlgNum, h2h)) {
        isH2h = true;
    }

    LogTlg() << "| TNUM: " << tlg.id
             << " | GATEWAYNUM: " << tlg.tlgNumStr()
             << " | DIR: " << "IAPP"
             << " | ROUTER: " << tlg.sender
             << " | TSTAMP: " << boost::posix_time::second_clock::local_time() << "\n"
             << (isH2h ? hth::toStringOnTerm(h2h) + "\n" : "")
             << appsTextAsHumanReadable(tlg.text);

    if(!processReply(tlg.text))
      ProgTrace(TRACE1, "Message was not processed!");
    deleteTlg(tlg.id);
    callPostHooksBefore();
    ASTRA::commit();
    callPostHooksAfter();
    emptyHookTables();
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
      "      tlg_queue.type='IAPP' AND tlg_queue.status='PUT' "
      "ORDER BY tlg_queue.time,tlg_queue.id";
    TlgQry.CreateVariable( "receiver", otString, OWN_CANON_NAME() );
  };

  int count;

  count=0;
  TlgQry.Execute();
  try
  {
      for(;!TlgQry.Eof && (count++)<PROC_COUNT(); TlgQry.Next(), ASTRA::rollback())
      {
          tlg_info tlgi = {};
          tlgi.fromDB(TlgQry);

          ProgTrace(TRACE5,"TLG_IN: <%s>", tlgi.text.c_str());

          handle_apps_tlg(tlgi);
      }
      queue_not_empty=!TlgQry.Eof;
  }
  catch(...)
  {
      ProgError(STDLOG, "Unknown error");
      throw;
  }
  time_t time_end=time(NULL);
  if (time_end-time_start>1)
    ProgTrace(TRACE5,"Attention! handle_tlg execute time: %ld secs, count=%d",
                     time_end-time_start,count);
  return queue_not_empty;
}

void resend_tlg(void)
{
  time_t time_start=time(NULL);

  // проверим, есть ли сообщения без ответа или нуждающиеся в повторной отправке
  TQuery Qry(&OraSession);
  Qry.SQLText = "SELECT send_time, msg_id, send_attempts, msg_text, point_id "
                "FROM apps_messages ";
  Qry.Execute();

  int count = 0;
  for(;!Qry.Eof && (count++)<PROC_COUNT(); Qry.Next(), ASTRA::rollback())
  {
    int point_id = Qry.FieldAsInteger("point_id");
    int send_attempts = Qry.FieldAsInteger("send_attempts");
    bool apps_down = get_alarm(point_id, Alarm::APPSOutage);
    TDateTime send_time = Qry.FieldAsDateTime("send_time");
    int msg_id = Qry.FieldAsInteger("msg_id");
    if (!checkTime(point_id) || send_attempts == MaxSendAttempts) {
      /* More than 2 days has passed after flight scheduled departure time or max value of send attempts has been reached.
         It is useless to continue send. */
      deleteMsg(msg_id);
    }
    // maximum time to wait for a response from APPS is 4 sec
    TDateTime now = NowUTC();
    double ttw_sec = apps_down?600.0:10.0;
    if (now - send_time < ttw_sec/(24.0*60.0*60.0)) {
      continue;
    }
    if( ( send_attempts >= NumSendAttempts ) && !apps_down ) {
      // включим тревогу "Нет связи с APPS"
      set_alarm(point_id, Alarm::APPSOutage, true);
    }
    ProgTrace(TRACE5, "resend_tlg: elapsed time %s", DateTimeToStr( (NowUTC() - send_time), "hh:nn:ss" ).c_str());
    reSendMsg(send_attempts, Qry.FieldAsString("msg_text"), msg_id);
    callPostHooksBefore();
    ASTRA::commit();
    callPostHooksAfter();
    emptyHookTables();
  }
  time_t time_end=time(NULL);
  if (time_end-time_start>1)
    ProgTrace(TRACE5,"Attention! resend_tlg execute time: %ld secs, count=%d",
                     time_end-time_start,count);
}
