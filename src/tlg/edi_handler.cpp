#include <unistd.h>
#include <errno.h>
#include <tcl.h>
#include <math.h>
#include "date_time.h"
#include "astra_main.h"
#include "astra_consts.h"
#include "astra_context.h"
#include "astra_utils.h"
#include "base_tables.h"
#include "exceptions.h"
#include "oralib.h"
#include "tlg.h"
#include "tlg_parser.h"
#include "edi_tlg.h"
#include "edi_msg.h"
#include "edi_handler.h"
#include "postpone_edifact.h"
#include "tlg_source_edifact.h"
#include "remote_system_context.h"

#include <serverlib/query_runner.h>
#include <serverlib/posthooks.h>
#include <serverlib/ourtime.h>
#include <serverlib/TlgLogger.h>
#include <serverlib/testmode.h>
#include <edilib/edi_func_cpp.h>
#include <libtlg/telegrams.h>

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

using namespace ASTRA;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;
//using namespace tlg_process;

static int WAIT_INTERVAL()       //миллисекунды
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("EDI_HANDLER_WAIT_INTERVAL",1,NoExists,60000);
  return VAR;
};

static int PROC_INTERVAL()       //миллисекунды
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("EDI_HANDLER_PROC_INTERVAL",0,10000,1000);
  return VAR;
};

static int PROC_COUNT()          //кол-во разбираемых телеграмм за одну итерацию
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("EDI_HANDLER_PROC_COUNT",1,NoExists,100);
  return VAR;
};

static bool handle_tlg(void);

int base_edi_handler_tcl(const char* cmd, int argc, char *argv[])
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

        waitCmd(cmd,queue_not_empty?PROC_INTERVAL():WAIT_INTERVAL(),buf,sizeof(buf));
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
      ASTRA::rollback();
      OraSession.LogOff();
    }
    catch(...)
    {
      ProgError(STDLOG, "Unknown exception");
    };
    return 0;
}

int main_edi_handler_tcl(int supervisorSocket, int argc, char *argv[])
{
    return base_edi_handler_tcl("CMD_EDI_HANDLER", argc, argv);
}

int main_itci_req_handler_tcl(int supervisorSocket, int argc, char *argv[])
{
    return base_edi_handler_tcl("CMD_ITCI_REQ_HANDLER", argc, argv);
}

int main_itci_res_handler_tcl(int supervisorSocket, int argc, char *argv[])
{
    return base_edi_handler_tcl("CMD_ITCI_RES_HANDLER", argc, argv);
}

static bool isTlgPostponed(const tlg_info& tlg)
{
    // в идеале признак postponed должен уже храниться в структуре (tlg_info, tlg_source...)
    return TlgHandling::isTlgPostponed(ASTRA::make_tlgnum(tlg.id));
}


void handle_edi_tlg(const tlg_info &tlg)
{
    hth::HthInfo h2h = {};
    bool isH2h = false;

    tlgnum_t tlgNum(boost::lexical_cast<std::string>(tlg.id));
    if (!telegrams::callbacks()->readHthInfo(tlgNum, h2h)) {
        isH2h = true;
    }

    LogTlg() << "| TNUM: " << tlg.id
             << " | GATEWAYNUM: " << tlg.tlgNumStr()
             << " | DIR: " << "IN"
             << " | ROUTER: " << tlg.sender
             << " | TSTAMP: " << boost::posix_time::second_clock::local_time()
             << (isTlgPostponed(tlg) ? " | POSTPONED" : "") << "\n"
             << (isH2h ? hth::toStringOnTerm(h2h) + "\n" : "")
             << TlgHandling::TlgSourceEdifact(tlg.text).text2view();

    const int tlg_id = tlg.id;
    ProgTrace(TRACE1,"========= %d TLG: START HANDLE =============",tlg_id);
    ProgTrace(TRACE1,"========= (sender=%s tlg_num=%s) =============",
      tlg.sender.c_str(), tlg.tlgNumStr().c_str());
    if (tlg.proc_attempt>=HANDLER_PROC_ATTEMPTS())
    {
      ProgTrace(TRACE5, "handle_tlg: tlg_id=%d proc_attempt=%d", tlg_id, tlg.proc_attempt);
      errorTlg(tlg_id,"PROC");
      ASTRA::commit();
    }
    else
    try
    {
        boost::optional<TlgHandling::TlgSourceEdifact> answTlg;
        procTlg(tlg_id);
        ASTRA::commit();

        boost::shared_ptr<TlgHandling::TlgSourceEdifact> tlgSrc;
        tlgSrc.reset(new TlgHandling::TlgSourceEdifact(TlgHandling::TlgSource::readFromDb(ASTRA::make_tlgnum(tlg_id))));
        answTlg = proc_new_edifact(tlgSrc);

        // если сформировался ответ, отправим его
        if(answTlg)
        {
            answTlg->setToRot(tlg.sender);
            answTlg->setFromRot(OWN_CANON_NAME());
            sendEdiTlg(*answTlg, qpOutA);
        }
        deleteTlg(tlg_id);
        callPostHooksBefore();
        ASTRA::commit();
        callPostHooksAfter();
        emptyHookTables();
    }
    catch(TlgHandling::TlgToBePostponed& e)
    {
        LogTrace(TRACE1) << "Tlg " << e.tlgNum() << " to be postponed";
        deleteTlg(e.tlgnum());
        callPostHooksBefore();
        ASTRA::commit();
        callPostHooksAfter();
        emptyHookTables();
    }
    catch(edifact::edi_exception &e)
    {
        ASTRA::rollback();
        try
        {
          ProgTrace(TRACE0,"EdiExcept: %s:%s", e.errCode().c_str(), e.what());
          errorTlg(tlg_id,"PARS",e.what());
          ASTRA::commit();
        }
        catch(...) {};
    }
    catch(std::exception &e)
    {
        ASTRA::rollback();
        try
        {
          ProgError(STDLOG, "std::exception: %s", e.what());
          errorTlg(tlg_id,"PARS",e.what());
          ASTRA::commit();
        }
        catch(...) {};
    }
    catch(Ticketing::RemoteSystemContext::system_not_found &e)
    {
        ASTRA::rollback();
        try
        {
          ProgError(STDLOG, "system_not_found");
          errorTlg(tlg_id,"PARS", "bad system");
          ASTRA::commit();
        }
        catch(...) {};
    }
    catch(...)
    {
        ASTRA::rollback();
        try
        {
          ProgError(STDLOG, "Unknown error");
          errorTlg(tlg_id,"UNKN");
          ASTRA::commit();
        }
        catch(...) {};
    }
    ProgTrace(TRACE1,"========= %d TLG: DONE HANDLE =============",tlg_id);
    ProgTrace(TRACE5, "IN: PUT->DONE (sender=%s, tlg_num=%d, time=%.10f)",
                      tlg.sender.c_str(),
                      tlg_id,
                      NowUTC());
    monitor_idle_zapr_type(1, QUEPOT_TLG_EDI);
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
      "      tlg_queue.type='INA' AND tlg_queue.status='PUT' "
      "ORDER BY tlg_queue.time,tlg_queue.id";
    TlgQry.CreateVariable("receiver",otString,OWN_CANON_NAME());
  };

  int count;

  count=0;
  TlgQry.Execute();
//  obr_tlg_queue tlg_obr(1); // Класс - обработчик телеграмм
  try
  {
      for(;!TlgQry.Eof && (count++)<PROC_COUNT(); TlgQry.Next(), ASTRA::rollback())
      {
        tlg_info tlgi = {};
        tlgi.fromDB(TlgQry);

        ProgTrace(TRACE5,"TLG_IN: <%s>", tlgi.text.c_str());

        handle_edi_tlg(tlgi);
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
