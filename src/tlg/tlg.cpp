#include "tlg.h"
#include <string>
#include "basic.h"
#include "exceptions.h"
#include "oralib.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "tlg_source_edifact.h"
#include "qrys.h"
#include <serverlib/tcl_utils.h>
#include <serverlib/logger.h>
#include <serverlib/testmode.h>
#include <serverlib/TlgLogger.h>
#include <serverlib/posthooks.h>
#include <edilib/edi_user_func.h>
#include <libtlg/tlg_outbox.h>
#include "xp_testing.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

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

void sendCmdTypeBHandler()
{
  sendCmd("CMD_TYPEB_HANDLER","H");
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
            int ttl,
            const std::string &text,
            int typeb_tlg_id, int typeb_tlg_num)
{
  BASIC::TDateTime nowUTC=BASIC::NowUTC();

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

  std::string::const_iterator ib,ie;
  ib=tlg_body.begin();
  for(int page_no=1;ib<tlg_body.end();page_no++)
  {
    ie=ib+4000;
    if (ie>tlg_body.end()) ie=tlg_body.end();
    TextQry.get().SetVariable("page_no", page_no);
    TextQry.get().SetVariable("text", std::string(ib,ie));
    TextQry.get().Execute();
    ib=ie;
  };
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

  std::string::const_iterator ib,ie;
  ib=tlg_text.begin();
  for(int page_no=1;ib<tlg_text.end();page_no++)
  {
    ie=ib+4000;
    if (ie>tlg_text.end()) ie=tlg_text.end();
    TextQry.get().SetVariable("page_no", page_no);
    TextQry.get().SetVariable("text", std::string(ib,ie));
    TextQry.get().Execute();
    ib=ie;
  };
};

string getTlgText(int tlg_id)
{
  string result;

  const char* sql=
    "SELECT text FROM tlgs_text WHERE id=:id ORDER BY page_no";
  QParams QryParams;
  QryParams << QParam("id", otInteger, tlg_id);
  TCachedQuery TextQry(sql, QryParams);
  TextQry.get().Execute();
  for(;!TextQry.get().Eof;TextQry.get().Next())
    result+=TextQry.get().FieldAsString("text");  
  return result;
};

static void logTlgTypeA(const std::string& text)
{
    LogTlg() << TlgHandling::TlgSourceEdifact(text).text2view();
}

static void logTlgTypeB(const std::string& text)
{
    LogTlg() << text;
}

static void logTlg(const std::string& type, int tlgNum, const std::string& receiver, const std::string& text)
{
    if(type != "OUTA" && type != "OUTB")
        return;

    LogTlg() << "| TNUM: " << tlgNum
             << " | DIR: " << "OUT"
             << " | ROUTER: " << receiver
             << " | TSTAMP: " << boost::posix_time::second_clock::local_time();

    if(type == "OUTA")
        logTlgTypeA(text);
    else
        logTlgTypeB(text);
}

static void putTlg2OutQueue(const std::string& receiver,
                            const std::string& sender,
                            const std::string& type,
                            const std::string& text,
                            int priority,
                            int tlgNum,
                            int ttl)
{
    BASIC::TDateTime nowUTC=BASIC::NowUTC();
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

    // дадим работу TlgLogger'у
    logTlg(type, tlgNum, receiver, text);
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
        std::string type;
        int priority;
        switch(queuePriority)
        {
          case qpOutA:
            type = "OUTA";
            priority = qpOutA;
            break;
          case qpOutAStepByStep:
            type = "OUTA";
            priority = qpOutAStepByStep;
            break;
          default:
            type = "OUTB";
            priority = qpOutB;
            break;
        };

        int tlg_num = saveTlg(receiver, sender, type.c_str(), ttl, text,
                              typeb_tlg_id, typeb_tlg_num);

        // кладём тлг в очередь на отправку
        putTlg2OutQueue(receiver, sender, type, text, priority, tlg_num, ttl);

        registerHookAfter(sendCmdTlgSnd);

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

        BASIC::TDateTime nowUTC=BASIC::NowUTC();
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
    QryParams << QParam("text", otString, text);

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
};

int waitCmd(const char* receiver, int msecs, const char* buf, int buflen)
{
  if (receiver==NULL || *receiver==0)
    throw EXCEPTIONS::Exception( "waitCmd: receiver not defined");
  if (buf==NULL || buflen <= 1 )
    throw EXCEPTIONS::Exception( "waitCmd: buf not defined");
  static map<string,int> sockfds;

  int sockfd;
  if (sockfds.find(receiver)==sockfds.end())
  {
    if ((sockfd=socket(AF_UNIX,SOCK_DGRAM,0))==-1)
      throw EXCEPTIONS::Exception("waitCmd: 'socket' error %d: %s",errno,strerror(errno));
    try
    {
      ProgTrace(TRACE5, "waitCmd: receiver=%s sockfd=%d",receiver,sockfd);
      struct sockaddr_un sock_addr;
      memset(&sock_addr,0,sizeof(sock_addr));
      sock_addr.sun_family=AF_UNIX;
      string sun_path=readStringFromTcl( receiver, "");
      if (sun_path.empty() || sun_path.size()>sizeof (sock_addr.sun_path) - 1)
        throw EXCEPTIONS::Exception( "waitCmd: can't read parameter '%s'", receiver );
      unlink(sun_path.c_str());
      strcpy(sock_addr.sun_path,sun_path.c_str());
      if (bind(sockfd,(struct sockaddr*)&sock_addr,sizeof(sock_addr))==-1)
        throw EXCEPTIONS::Exception("waitCmd: 'bind' error %d: %s",errno,strerror(errno));
      sockfds[receiver]=sockfd;
    }
    catch(...)
    {
      close(sockfd);
      throw;
    };
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


