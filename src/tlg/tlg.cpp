#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "tlg.h"
#include <string>
#include "date_time.h"
#include "exceptions.h"
#include "oralib.h"
#include "astra_consts.h"
#include "astra_context.h"
#include "astra_utils.h"
#include "tlg_source_edifact.h"
#include "tlg_source_typeb.h"
#include "qrys.h"
#include <serverlib/tcl_utils.h>
#include <serverlib/str_utils.h>
#include <serverlib/logger.h>
#include <serverlib/testmode.h>
#include <serverlib/TlgLogger.h>
#include <serverlib/posthooks.h>
#include <serverlib/cursctl.h>
#include <edilib/edi_user_func.h>
#include <edilib/edi_tables.h>
#include <libtlg/tlg_outbox.h>
#include <libtlg/hth.h>
#include "xp_testing.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace std;

const char* ETS_CANON_NAME()
{
  static string VAR;
  if ( VAR.empty() )
    VAR=getTCLParam("ETS_CANON_NAME",NULL);
  return VAR.c_str();
}

const char* OWN_CANON_NAME()
{
  static string VAR;
  if ( VAR.empty() )
    VAR=getTCLParam("OWN_CANON_NAME",NULL);
  return VAR.c_str();
}

const char* DEF_CANON_NAME()
{
  static bool init=false;
  static string VAR;
  if ( !init ) {
    VAR=getTCLParam("DEF_CANON_NAME","");
    init=true;
  }
  return VAR.c_str();
}

int HANDLER_PROC_ATTEMPTS()
{
  static int VAR=ASTRA::NoExists;
  if (VAR==ASTRA::NoExists)
    VAR=getTCLParam("HANDLER_PROC_ATTEMPTS",1,9,3);
  return VAR;
};

void sendCmdTlgHttpSnd()
{
  sendCmd("CMD_TLG_HTTP_SND","H");
}

void sendCmdTlgSnd()
{
  sendCmd("CMD_TLG_SND","H");
}

void sendCmdTlgSndStepByStep()
{
  sendCmd("CMD_TLG_SND","S");
}

void sendCmdAppsHandler()
{
  sendCmd("CMD_APPS_HANDLER","H");
}

void sendCmdTypeBHandler()
{
  sendCmd("CMD_TYPEB_HANDLER","H");
}

void sendCmdEdiCommonHandler()
{
  sendCmd("CMD_EDI_HANDLER","H");
}

void sendCmdEdiItciReqHandler()
{
  sendCmd("CMD_ITCI_REQ_HANDLER","H");
}

void sendCmdEdiItciResHandler()
{
  sendCmd("CMD_ITCI_RES_HANDLER","H");
}

void sendCmdEdiIapiHandler()
{
  sendCmd("CMD_IAPI_EDI_HANDLER","H");
}

void sendCmdEdiHandler(TEdiTlgSubtype st)
{
    if(st == stItciReq) {
        sendCmdEdiItciReqHandler();
    } else if(st == stItciRes) {
        sendCmdEdiItciResHandler();
    } else if(st == stIapi) {
        sendCmdEdiIapiHandler();
    } else {
        sendCmdEdiCommonHandler();
    }
}

void sendCmdEdiHandlerAtHook(TEdiTlgSubtype st)
{
    if(st == stItciReq) {
        registerHookAfter(sendCmdEdiItciReqHandler);
    } else if(st == stItciRes) {
        registerHookAfter(sendCmdEdiItciResHandler);
    } else if(st == stIapi) {
        registerHookAfter(sendCmdEdiIapiHandler);
    } else {
        registerHookAfter(sendCmdEdiCommonHandler);
    }
}

TEdiTlgSubtype specifyEdiTlgSubtype(const std::string& ediText)
{
    TEdiTlgSubtype ret = stCommon;

    if(ReadEdiMessage(ediText.c_str()) == EDI_MES_OK)
    {
        switch(GetEdiMesStruct()->pTempMes->Type.type)
        {
        case DCQCKI:
        case DCQCKU:
        case DCQCKX:
        case DCQPLF:
        case DCQBPR:
        case DCQSMF:
        {
            ret = stItciReq;
            break;
        }
        case DCRCKA:
        case DCRSMF:
        {
            ret = stItciRes;
            break;
        }
        case CUSRES:
        case CUSUMS:
        {
            ret = stIapi;
            break;
        }
        default:
            break;
        }
    }

    LogTrace(TRACE3) << "TlgSubtype " << ret << " detected for edi_message: "
                     << "'" << ediText.substr(0, 100) << "...'";

    return ret;
}

std::string getEdiTlgSubtypeName(TEdiTlgSubtype st)
{
    std::string str = "";
    switch(st)
    {
    case stItciReq:
    {
        str = "itci_req";
        break;
    }
    case stItciRes:
    {
        str = "itci_res";
        break;
    }
    case stIapi:
    {
        str = "iapi";
        break;
    }
    case stCommon:
    {
        str = "common";
        break;
    }
    default:
        LogError(STDLOG) << "Unknown edi_handler: " << st;
        break;
    }

    return str;
}

int getNextTlgNum()
{
  int tlg_num = 0;
  TQuery Qry(&OraSession);
  Qry.SQLText = "SELECT tlgs_id.nextval as tlg_num FROM dual";
  Qry.Execute();

  tlg_num = Qry.FieldAsInteger("tlg_num");

  Qry.Close();

  return tlg_num;
}

int saveTlg(const char * receiver,
            const char * sender,
            const char * type,
            const std::string &text,
            int typeb_tlg_id, int typeb_tlg_num)
{
  TDateTime nowUTC=NowUTC();

  int tlg_num = getNextTlgNum();

  TQuery Qry(&OraSession);

  Qry.SQLText=
    "INSERT INTO tlgs(id,sender,tlg_num,receiver,type,time,error,typeb_tlg_id,typeb_tlg_num) "
    "VALUES(:tlg_num,:sender,:tlg_num,:receiver,:type,:time,NULL,:typeb_tlg_id,:typeb_tlg_num)";

  Qry.CreateVariable("sender",otString,sender);
  Qry.CreateVariable("receiver",otString,receiver);
  Qry.CreateVariable("type",otString,type);
  Qry.CreateVariable("time",otDate,nowUTC);
  Qry.CreateVariable("tlg_num",otInteger,tlg_num);

  if (typeb_tlg_id!=ASTRA::NoExists && typeb_tlg_num!=ASTRA::NoExists)
  {
    Qry.CreateVariable("typeb_tlg_id", otInteger, typeb_tlg_id);
    Qry.CreateVariable("typeb_tlg_num", otInteger, typeb_tlg_num);
  }
  else
  {
    Qry.CreateVariable("typeb_tlg_id", otInteger, FNull);
    Qry.CreateVariable("typeb_tlg_num", otInteger, FNull);
  };

  Qry.Execute();
  Qry.Close();

  putTlgText(tlg_num, text);

  return tlg_num;
}

void putTypeBBody(int tlg_id, int tlg_num, const string &tlg_body)
{
  if (tlg_body.empty()) return;
  if (tlg_id==ASTRA::NoExists)
    throw Exception("%s: tlg_id=ASTRA::NoExists", __FUNCTION__);
  if (tlg_num==ASTRA::NoExists)
    throw Exception("%s: tlg_num=ASTRA::NoExists", __FUNCTION__);

  const char* sql=
    "INSERT INTO typeb_in_body(id, num, page_no, text) "
    "VALUES(:id, :num, :page_no, :text)";

  QParams QryParams;
  QryParams << QParam("id", otInteger, tlg_id);
  QryParams << QParam("num", otInteger, tlg_num);
  QryParams << QParam("page_no", otInteger);
  QryParams << QParam("text", otString);
  TCachedQuery TextQry(sql, QryParams);

  longToDB(TextQry.get(), "text", tlg_body);
};

string getTypeBBody(int tlg_id, int tlg_num)
{
  string result;

  const char* sql=
    "SELECT text FROM typeb_in_body WHERE id=:id AND num=:num ORDER BY page_no";
  QParams QryParams;
  QryParams << QParam("id", otInteger, tlg_id);
  QryParams << QParam("num", otInteger, tlg_num);
  TCachedQuery TextQry(sql, QryParams);
  TextQry.get().Execute();
  for(;!TextQry.get().Eof;TextQry.get().Next())
    result+=TextQry.get().FieldAsString("text");
  return result;
};

void putTlgText(int tlg_id, const string &tlg_text)
{
  if (tlg_text.empty()) return;
  if (tlg_id==ASTRA::NoExists)
    throw Exception("%s: tlg_id=ASTRA::NoExists", __FUNCTION__);

  const char* sql=
    "INSERT INTO tlgs_text(id, page_no, text) VALUES(:id, :page_no, :text)";

  QParams QryParams;
  QryParams << QParam("id", otInteger, tlg_id);
  QryParams << QParam("page_no", otInteger);
  QryParams << QParam("text", otString);
  TCachedQuery TextQry(sql, QryParams);

  longToDB(TextQry.get(), "text", tlg_text);
}

std::string getTlgText(int tlg_id)
{
    std::string text;

    const char* sql=
    "SELECT text FROM tlgs_text WHERE id=:id ORDER BY page_no";
    QParams QryParams;
    QryParams << QParam("id", otInteger, tlg_id);
    TCachedQuery TextQry(sql, QryParams);
    TextQry.get().Execute();
    for(;!TextQry.get().Eof;TextQry.get().Next())
        text+=TextQry.get().FieldAsString("text");
    return text;
}

std::string getTlgText2(const tlgnum_t& tnum)
{
    tlgnum_t tlgNum(tnum); // to avoid compiler warning treated as error (used unitialized)
    const int tlg_id = boost::lexical_cast<int>(tlgNum.num);
    std::string tlgText = getTlgText(tlg_id);
    if(tlgText.find("BGM+132") != std::string::npos) {
        ProgTrace(TRACE0, "Force replace message name from CUSRES to CUSUMS");
        tlgText = StrUtils::replaceSubstrCopy(tlgText, "CUSRES", "CUSUMS");
    }
    return tlgText;
}

static void logTlgTypeA(const std::string& text)
{
    LogTlg() << TlgHandling::TlgSourceEdifact(text).text2view();
}

static void logTlgTypeB(const std::string& text)
{
    LogTlg() << text;
}

static void logTlgTypeAPP(const std::string& text)
{
    LogTlg() << text;
}

static void logTlg(const std::string& type, int tlgNum, const std::string& receiver, const std::string& text)
{
    if(type != "OUTA" && type != "OUTB" && type != "OAPP")
        return;

    LogTlg() << "| TNUM: " << tlgNum
             << " | DIR: " << type
             << " | ROUTER: " << receiver
             << " | TSTAMP: " << boost::posix_time::second_clock::local_time();

    if(type == "OUTA")
        logTlgTypeA(text);
    else if(type == "OUTB")
        logTlgTypeB(text);
    else
        logTlgTypeAPP(text);
}

static void logTlg(const TlgHandling::TlgSourceEdifact& tlg)
{
    LogTlg() << "| TNUM: " << *tlg.tlgNum()
             << " | DIR: " << "OUTA"
             << " | ROUTER: " << tlg.toRot()
             << " | TSTAMP: " << boost::posix_time::second_clock::local_time() << "\n"
             << (tlg.h2h() ? hth::toStringOnTerm(*tlg.h2h()) + "\n" : "")
             << tlg.text2view();
}

static void logTlg(const TlgHandling::TlgSourceTypeB& tlg)
{
    LogTlg() << "| TNUM: " << *tlg.tlgNum()
             << " | DIR: " << "OUTB"
             << " | ROUTER: " << tlg.toRot()
             << " | TSTAMP: " << boost::posix_time::second_clock::local_time() << "\n"
             << tlg.text2view();
}

static void putTlg2OutQueue(const std::string& receiver,
                            const std::string& sender,
                            const std::string& type,
                            const std::string& text,
                            int priority,
                            int tlgNum,
                            int ttl)
{
    TDateTime nowUTC=NowUTC();
    TQuery Qry(&OraSession);
    Qry.SQLText=
      "INSERT INTO tlg_queue(id,sender,tlg_num,receiver,type,priority,status,time,ttl,time_msec,last_send) "
      "VALUES(:tlg_num,:sender,:tlg_num,:receiver,:type,:priority,'PUT',:time,:ttl,:time_msec,NULL) ";
    Qry.CreateVariable("sender",otString,sender);
    Qry.CreateVariable("receiver",otString,receiver);
    Qry.CreateVariable("type",otString, type.c_str());
    Qry.CreateVariable("priority",otInteger,priority);
    Qry.CreateVariable("time",otDate,nowUTC);
    if ((priority==qpOutA || priority==qpOutAStepByStep) && ttl>0)
      Qry.CreateVariable("ttl",otInteger,ttl);
    else
      Qry.CreateVariable("ttl",otInteger,FNull);
    Qry.CreateVariable("time_msec",otFloat,nowUTC);
    Qry.CreateVariable("tlg_num",otInteger,tlgNum);
    Qry.Execute();

    ProgTrace(TRACE5,"OUT: PUT (sender=%s, tlg_num=%d, time=%.10f, priority=%d)",
                     Qry.GetVariableAsString("sender"),
                     tlgNum,
                     nowUTC,
                     priority);
    Qry.Close();
#ifdef XP_TESTING
    if (inTestMode()) {
        xp_testing::TlgOutbox::getInstance().push(tlgnum_t("no tlg num"), text, 0);
    }
#endif /* #ifdef XP_TESTING */
}

void putTlg2OutQueue_wrap(const std::string& receiver,
                          const std::string& sender,
                          const std::string& type,
                          const std::string& text,
                          int priority,
                          int tlgNum,
                          int ttl)
{
  putTlg2OutQueue(receiver, sender, type, text, priority, tlgNum, ttl);
}

struct TlgTypePriority
{
    std::string Type;
    int Priority;

    TlgTypePriority(const std::string& t, int p)
        : Type(t), Priority(p)
    {}
};

static TlgTypePriority getTlgTypePriority(TTlgQueuePriority queuePriority)
{
    switch(queuePriority)
    {
    case qpOutA:
        return TlgTypePriority("OUTA", qpOutA);
    case qpOutAStepByStep:
        return TlgTypePriority("OUTA", qpOutAStepByStep);
    case qpOutApp:
        return TlgTypePriority("OAPP", qpOutApp);
    case qpOutB:
        return TlgTypePriority("OUTB", qpOutB);
    default:
        return TlgTypePriority("OUTB", qpOutB);
    }
}

static void putTlg2OutQueue(const TlgHandling::TlgSourceEdifact& tlg,
                            TTlgQueuePriority queuePriority,
                            int ttl)
{
    if(tlg.tlgNum()) {
        TlgTypePriority tp = getTlgTypePriority(queuePriority);
        tlgnum_t tlgNum = *tlg.tlgNum();
        int tnum = boost::lexical_cast<int>(tlgNum.num);
        putTlg2OutQueue(tlg.toRot(), tlg.fromRot(), tp.Type, tlg.text(), tp.Priority, tnum, ttl);
    } else {
        LogError(STDLOG) << "Invalid tlgNum!";
    }
}

int sendTlg(const char* receiver,
            const char* sender,
            TTlgQueuePriority queuePriority,
            int ttl,
            const std::string &text,
            int typeb_tlg_id,
            int typeb_tlg_num)
{
    try
    {
        TlgTypePriority tp = getTlgTypePriority(queuePriority);

        int tlg_num = saveTlg(receiver, sender, tp.Type.c_str(), text,
                              typeb_tlg_id, typeb_tlg_num);

        // кладём тлг в очередь на отправку
        putTlg2OutQueue(receiver, sender, tp.Type, text, tp.Priority, tlg_num, ttl);

        logTlg(tp.Type, tlg_num, receiver, text);

        registerHookAfter(queuePriority==qpOutAStepByStep?
                            sendCmdTlgSndStepByStep:
                            sendCmdTlgSnd);

        return tlg_num;
    }
    catch( std::exception &e)
    {
        ProgError(STDLOG, "%s", e.what());
        throw;
    }
    catch(...)
    {
        ProgError(STDLOG, "sendTlg: Unknown error");
        throw;
    };
}


void sendEdiTlg(TlgHandling::TlgSourceEdifact& tlg,
                TTlgQueuePriority queuePriority,
                int ttl)
{
    try
    {
        tlg.write(); // сохранение телеграммы
        putTlg2OutQueue(tlg, queuePriority, ttl);
        logTlg(tlg);
        registerHookAfter(queuePriority==qpOutAStepByStep?
                            sendCmdTlgSndStepByStep:
                            sendCmdTlgSnd);
    }
    catch( std::exception &e)
    {
        ProgError(STDLOG, "%s", e.what());
        throw;
    }
    catch(...)
    {
        ProgError(STDLOG, "sendTlg: Unknown error");
        throw;
    };
}

void sendTpbTlg(TlgHandling::TlgSourceTypeB& tlg)
{
    try
    {
        tlg.write(); // сохранение телеграммы
        putTlg2OutQueue(tlg, qpOutB, 0/*ttl*/);
        logTlg(tlg);
        registerHookAfter(sendCmdTlgSnd);
    }
    catch( std::exception &e)
    {
        ProgError(STDLOG, "%s", e.what());
        throw;
    }
    catch(...)
    {
        ProgError(STDLOG, "sendTlg: Unknown error");
        throw;
    };
}

int loadTlg(const std::string &text)
{
    bool hist_uniq_error;
    return loadTlg(text, ASTRA::NoExists, hist_uniq_error);
}

int loadTlg(const std::string &text, int prev_typeb_tlg_id, bool &hist_uniq_error)
{
    try
    {
        hist_uniq_error = false;

        TDateTime nowUTC=NowUTC();
        int tlg_id = getNextTlgNum();

        TQuery Qry(&OraSession);
        Qry.SQLText=
          "INSERT INTO tlg_queue(id,sender,tlg_num,receiver,type,priority,status,time,ttl,time_msec,last_send) "
          "VALUES(:tlg_num,:sender,:tlg_num,:receiver,:type,1,'PUT',:time,:ttl,:time_msec,NULL)";
        Qry.CreateVariable("sender",otString,OWN_CANON_NAME());
        Qry.CreateVariable("receiver",otString,OWN_CANON_NAME());
        Qry.CreateVariable("type",otString,"INB");
        Qry.CreateVariable("time",otDate,nowUTC);
        Qry.CreateVariable("ttl",otInteger,FNull);
        Qry.CreateVariable("time_msec",otFloat,nowUTC);
        Qry.CreateVariable("tlg_num",otInteger,tlg_id);
        Qry.Execute();

        Qry.SQLText=
          "INSERT INTO tlgs(id,sender,tlg_num,receiver,type,time,error,typeb_tlg_id,typeb_tlg_num) "
          "VALUES(:tlg_num,:sender,:tlg_num,:receiver,:type,:time,NULL,NULL,NULL)";
        Qry.DeleteVariable("ttl");
        Qry.DeleteVariable("time_msec");
        Qry.Execute();

        putTlgText(tlg_id, text);

        ProgTrace(TRACE5,"IN: PUT (sender=%s, tlg_num=%d, time=%.10f)",
                         Qry.GetVariableAsString("sender"),
                         tlg_id,
                         nowUTC);

        if (prev_typeb_tlg_id != ASTRA::NoExists)
        {
          Qry.Clear();
          Qry.SQLText=
            "INSERT INTO typeb_in_history(prev_tlg_id, tlg_id, id) "
            "VALUES(:prev_tlg_id, NULL, :id)";
          Qry.CreateVariable("prev_tlg_id", otInteger, prev_typeb_tlg_id);
          Qry.CreateVariable("id", otInteger, tlg_id);
          try
          {
            Qry.Execute();
          }
          catch(EOracleError E)
          {
            if (E.Code==1)
            {
              Qry.Clear();
              Qry.SQLText="SELECT prev_tlg_id FROM typeb_in_history WHERE prev_tlg_id=:prev_tlg_id";
              Qry.CreateVariable("prev_tlg_id", otInteger, prev_typeb_tlg_id);
              Qry.Execute();
              if (Qry.Eof) throw;
              else hist_uniq_error=true;
            }
            else
              throw;
          };
        };

        Qry.Close();
        registerHookAfter(sendCmdTypeBHandler);
        return tlg_id;
    }
    catch( std::exception &e)
    {
        ProgError(STDLOG, "%s", e.what());
        throw;
    }
    catch(...)
    {
        ProgError(STDLOG, "loadTlg: Unknown error");
        throw;
    };
};

bool deleteTlg(int tlg_id)
{
    LogTrace(TRACE3) << "del tlg by num: " << tlg_id;
  try
  {
    TQuery TlgQry(&OraSession);
    TlgQry.Clear();
    TlgQry.SQLText=
           "DELETE FROM tlg_queue WHERE id= :id";
    TlgQry.CreateVariable("id",otInteger,tlg_id);
    TlgQry.Execute();
    return TlgQry.RowsProcessed()>0;
  }
  catch( std::exception &e)
  {
      ProgError(STDLOG, "%s", e.what());
      throw;
  }
  catch(...)
  {
      ProgError(STDLOG, "deleteTlg: Unknown error");
      throw;
  };
};

bool errorTlg(int tlg_id, const string &type, const string &msg)
{
  try
  {
    deleteTlg(tlg_id);
    TQuery TlgQry(&OraSession);
    if (!msg.empty())
    {
      TlgQry.Clear();
      TlgQry.SQLText=
        "BEGIN "
        "  UPDATE tlg_error SET msg= :msg WHERE id= :id; "
        "  IF SQL%NOTFOUND THEN "
        "    INSERT INTO tlg_error(id,msg) VALUES(:id,:msg); "
        "  END IF; "
        "END;";
      TlgQry.CreateVariable("msg",otString,msg.substr(0,1000));
      TlgQry.CreateVariable("id",otInteger,tlg_id);
      TlgQry.Execute();
    };
    TlgQry.Clear();
    TlgQry.SQLText="UPDATE tlgs SET error= :error WHERE id= :id";
    TlgQry.CreateVariable("error",otString,type.substr(0,4));
    TlgQry.CreateVariable("id",otInteger,tlg_id);
    TlgQry.Execute();
    return TlgQry.RowsProcessed()>0;
  }
  catch( std::exception &e)
  {
      ProgError(STDLOG, "%s", e.what());
      throw;
  }
  catch(...)
  {
      ProgError(STDLOG, "errorTlg: Unknown error");
      throw;
  };
};

void parseTypeB(int tlg_id)
{
  try
  {
    const char* sql=
      "UPDATE tlgs_in SET time_parse=system.UTCSYSDATE, time_receive_not_parse=NULL "
      "WHERE id=:id AND time_parse IS NULL";

    QParams QryParams;
    QryParams << QParam("id", otInteger, tlg_id);

    TCachedQuery Qry(sql, QryParams);
    Qry.get().Execute();
  }
  catch( std::exception &e)
  {
      ProgError(STDLOG, "%s", e.what());
      throw;
  }
  catch(...)
  {
      ProgError(STDLOG, "parseTypeB: Unknown error");
      throw;
  };
};

void errorTypeB(int tlg_id,
                int part_no,
                int &error_no,
                int error_pos,
                int error_len,
                const string &text)
{
  try
  {
    const char* sql=
      "INSERT INTO typeb_in_errors(tlg_id, part_no, error_no, lang, error_pos, error_len, text) "
      "VALUES(:tlg_id, :part_no, :error_no, :lang, :error_pos, :error_len, :text)";

    QParams QryParams;
    QryParams << QParam("tlg_id", otInteger, tlg_id);
    if (part_no!=ASTRA::NoExists)
      QryParams << QParam("part_no", otInteger, part_no);
    else
      QryParams << QParam("part_no", otInteger, FNull);
    if (error_no==ASTRA::NoExists) error_no=1;
    QryParams << QParam("error_no", otInteger, error_no);
    QryParams << QParam("error_pos", otInteger, error_pos==ASTRA::NoExists?0:error_pos);
    QryParams << QParam("error_len", otInteger, error_len==ASTRA::NoExists?0:error_len);
    QryParams << QParam("lang", otString);
    QryParams << QParam("text", otString, text.substr(0, 250));

    TCachedQuery ErrQry(sql, QryParams);

    for(int pass=0; pass<2; pass++)
    {
      ErrQry.get().SetVariable("lang", pass==0?AstraLocale::LANG_RU:AstraLocale::LANG_EN);
      ErrQry.get().Execute();
    };
  }
  catch( std::exception &e)
  {
      ProgError(STDLOG, "%s", e.what());
      throw;
  }
  catch(...)
  {
      ProgError(STDLOG, "errorTypeB: Unknown error");
      throw;
  };
};

void procTypeB(int tlg_id, int inc)
{
  try
  {
    const char* sql=
      "UPDATE typeb_in SET proc_attempt=NVL(proc_attempt,0)+SIGN(:d) WHERE id=:id ";

    QParams QryParams;
    QryParams << QParam("id", otInteger, tlg_id);
    QryParams << QParam("d", otInteger, inc);

    TCachedQuery Qry(sql, QryParams);
    Qry.get().Execute();
  }
  catch( std::exception &e)
  {
      ProgError(STDLOG, "%s", e.what());
      throw;
  }
  catch(...)
  {
      ProgError(STDLOG, "procTypeB: Unknown error");
      throw;
  };
};

bool procTlg(int tlg_id)
{
  try
  {
    TQuery TlgQry(&OraSession);
    TlgQry.Clear();
    TlgQry.SQLText=
           "UPDATE tlg_queue SET proc_attempt=NVL(proc_attempt,0)+1 WHERE id= :id";
    TlgQry.CreateVariable("id",otInteger,tlg_id);
    TlgQry.Execute();
    return TlgQry.RowsProcessed()>0;
  }
  catch( std::exception &e)
  {
      ProgError(STDLOG, "%s", e.what());
      throw;
  }
  catch(...)
  {
      ProgError(STDLOG, "procTlg: Unknown error");
      throw;
  };
};

void sendCmd(const char* receiver, const char* cmd)
{
  sendCmd(receiver, cmd, strlen(cmd));
};

void sendCmd(const char* receiver, const char* cmd, int cmd_len)
{
  try
  {
    if (receiver==NULL || *receiver==0)
      throw EXCEPTIONS::Exception( "sendCmd: receiver not defined");
    if (cmd==NULL || cmd_len<=0)
      throw EXCEPTIONS::Exception( "sendCmd: cmd not defined");
    if (cmd_len>MAX_CMD_LEN)
      throw EXCEPTIONS::Exception( "sendCmd: cmd too long (%d)", cmd_len);
    static int sockfd=-1;
    static struct sockaddr_un sock_addr;
    static map<string,string> addrs;
    if (sockfd==-1)
    {
      if ((sockfd=socket(AF_UNIX,SOCK_DGRAM,0))==-1)
        throw EXCEPTIONS::Exception("sendCmd: 'socket' error %d: %s",errno,strerror(errno));
      memset(&sock_addr,0,sizeof(sock_addr));
      sock_addr.sun_family=AF_UNIX;
      ProgTrace(TRACE5,"sendCmd: socket opened");
    };

    if (addrs.find(receiver)==addrs.end())
    {
      string sun_path=readStringFromTcl( receiver, "");
      if (sun_path.empty() || sun_path.size()>sizeof (sock_addr.sun_path) - 1)
        throw EXCEPTIONS::Exception( "sendCmd: can't read parameter '%s'", receiver );
      addrs[receiver]=sun_path;
      ProgTrace(TRACE5,"sendCmd: receiver %s added",receiver);
    };
    strcpy(sock_addr.sun_path,addrs[receiver].c_str());

    int len;
    if ((len=sendto(sockfd,cmd,cmd_len,MSG_DONTWAIT,
                    (struct sockaddr*)&sock_addr,sizeof(sock_addr)))==-1)
    {
      if (errno!=EAGAIN)
        throw EXCEPTIONS::Exception("sendCmd: 'sendto' error %d: %s",errno,strerror(errno));
      ProgTrace(TRACE5,"sendCmd: 'sendto' error %d: %s",errno,strerror(errno));
    }
    else
    {
      if (len>10)
        ProgTrace(TRACE5,"sendCmd: %d bytes sended to %s (time=%ld)",len,receiver,time(NULL));
      else
        ProgTrace(TRACE5,"sendCmd: cmd '%s' sended to %s (time=%ld)",cmd,receiver,time(NULL));
    };
  }
  catch(EXCEPTIONS::Exception E)
  {
    ProgTrace(TRACE0,"Exception: %s",E.what());
  };
}

int bindLocalSocket(const string &sun_path)
{
  int sockfd=socket(AF_UNIX,SOCK_DGRAM,0);
  if ((sockfd)==-1)
    throw EXCEPTIONS::Exception("%s: 'socket' error %d: %s",__FUNCTION__,errno,strerror(errno));
  try
  {
    ProgTrace(TRACE5, "%s: sun_path=%s sockfd=%d", __FUNCTION__, sun_path.c_str(), sockfd);
    struct sockaddr_un sock_addr;
    memset(&sock_addr,0,sizeof(sock_addr));
    sock_addr.sun_family=AF_UNIX;
    if (sun_path.empty() || sun_path.size()>sizeof (sock_addr.sun_path) - 1)
      throw EXCEPTIONS::Exception( "%s: wrong sun_path '%s'", __FUNCTION__, sun_path.c_str() );
    unlink(sun_path.c_str());
    strcpy(sock_addr.sun_path,sun_path.c_str());
    if (bind(sockfd,(struct sockaddr*)&sock_addr,sizeof(sock_addr))==-1)
      throw EXCEPTIONS::Exception("%s: 'bind' error %d: %s", __FUNCTION__, errno, strerror(errno));
  }
  catch(...)
  {
    close(sockfd);
    throw;
  };
  return sockfd;
}

int waitCmd(const char* receiver, int msecs, char* buf, int buflen)
{
  if (receiver==NULL || *receiver==0)
    throw EXCEPTIONS::Exception( "waitCmd: receiver not defined");
  if (buf==NULL || buflen <= 1 )
    throw EXCEPTIONS::Exception( "waitCmd: buf not defined");
  static map<string,int> sockfds;

  int sockfd;
  if (sockfds.find(receiver)==sockfds.end())
  {
    sockfds[receiver]=bindLocalSocket(readStringFromTcl( receiver, ""));
    ProgTrace(TRACE5,"waitCmd: receiver %s added",receiver);
  };
  sockfd=sockfds[receiver];

  try
  {
    fd_set rfds;
    struct timeval tv;
    FD_ZERO(&rfds);
    FD_SET(sockfd,&rfds);
    tv.tv_sec=msecs/1000;
    tv.tv_usec=msecs%1000*1000;
    int res;
    if ((res=select(sockfd+1,&rfds,NULL,NULL,&tv))==-1)
    {
      if (errno!=EINTR)
        throw EXCEPTIONS::Exception("waitCmd: 'select' error %d: %s",errno,strerror(errno));
    };
    if (res>0&&FD_ISSET(sockfd,&rfds))
    {
      int len;
      memset((void*)buf,0,buflen);
      if ((len = recv(sockfd,(char*)buf,buflen-1,0))==-1)
      {
        if (errno!=ECONNREFUSED)
          throw EXCEPTIONS::Exception("waitCmd: 'recv' error %d: %s",errno,strerror(errno));
      }
      else
      {
        if (len>10)
          ProgTrace(TRACE5,"waitCmd: %d bytes received from %s (time=%ld)",len,receiver,time(NULL));
        else
          ProgTrace(TRACE5,"waitCmd: cmd '%s' received from %s (time=%ld)",buf,receiver,time(NULL));
        return len;
      };
    };
  }
  catch(EXCEPTIONS::Exception E)
  {
    ProgError(STDLOG,"Exception: %s",E.what());
  };
  return 0;
};

void tlg_info::fromDB(TQuery &Qry)
{
  id           = Qry.FieldAsInteger("id");
  text         = getTlgText(id);
  sender       = Qry.FieldAsString("sender");
  tlg_num      = Qry.FieldAsFloat("tlg_num");
  proc_attempt = Qry.FieldAsInteger("proc_attempt");
  time         = Qry.FieldAsDateTime("time");
  if (!Qry.FieldIsNULL("ttl"))
    ttl=Qry.FieldAsFloat("ttl");
  else
    ttl=boost::none;
}

std::string tlg_info::tlgNumStr() const
{
  if (!tlg_num) return "unknown";
  return FloatToString(tlg_num.get(), 0);
}

bool tlg_info::ttlExpired() const
{
  return ttl && (NowUTC()-time)*SecsPerDay>=ttl.get();
}


