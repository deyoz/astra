#include "tlg.h"
#include <string>
#include "basic.h"
#include "exceptions.h"
#include "oralib.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "serverlib/tcl_utils.h"
#include "serverlib/logger.h"

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

const char* ERR_CANON_NAME()
{
  static bool init=false;
  static string VAR;
  if ( !init ) {
    VAR=getTCLParam("ERR_CANON_NAME","");
    init=true;
  }
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

const char* OWN_SITA_ADDR()
{
  static string VAR;
  if ( VAR.empty() )
    VAR=getTCLParam("OWN_SITA_ADDR",NULL);
  return VAR.c_str();
}

const int HANDLER_PROC_ATTEMPTS()
{
  static int VAR=ASTRA::NoExists;
  if (VAR==ASTRA::NoExists)
    VAR=getTCLParam("HANDLER_PROC_ATTEMPTS",1,9,3);
  return VAR;
};

void sendCmdTlgSnd()
{
  sendCmd("CMD_TLG_SND","H");
}

void sendCmdTypeBHandler()
{
  sendCmd("CMD_TYPEB_HANDLER","H");
}

void sendTlg(const char* receiver,
             const char* sender,
             bool isEdi,
             int ttl,
             const std::string &text)
{
    try
    {
        BASIC::TDateTime nowUTC=BASIC::NowUTC();
        TQuery Qry(&OraSession);
        Qry.SQLText=
          "BEGIN "
          "  SELECT tlgs_id.nextval INTO :tlg_num FROM dual; "
          "  INSERT INTO tlg_queue(id,sender,tlg_num,receiver,type,status,time,ttl,time_msec,last_send) "
          "  VALUES(:tlg_num,:sender,:tlg_num,:receiver,:type,'PUT',:time,:ttl,:time_msec,NULL); "
          "END;";
        Qry.CreateVariable("sender",otString,sender);
        Qry.CreateVariable("receiver",otString,receiver);
        Qry.CreateVariable("type",otString,isEdi?"OUTA":"OUTB");
        Qry.CreateVariable("time",otDate,nowUTC);
        if (isEdi&&ttl>0)
          Qry.CreateVariable("ttl",otInteger,ttl);
        else
          Qry.CreateVariable("ttl",otInteger,FNull);
        Qry.CreateVariable("time_msec",otFloat,nowUTC);
        Qry.CreateVariable("tlg_num",otInteger,FNull);
        Qry.Execute();
        Qry.SQLText=
          "INSERT INTO tlgs(id,sender,tlg_num,receiver,type,time,tlg_text,error) "
          "VALUES(:tlg_num,:sender,:tlg_num,:receiver,:type,:time,:text,NULL)";
        Qry.DeclareVariable("text",otLong);
        Qry.SetLongVariable("text",(void *)text.c_str(),text.size());
        Qry.DeleteVariable("ttl");
        Qry.DeleteVariable("time_msec");
        Qry.Execute();
        ProgTrace(TRACE5,"OUT: PUT (sender=%s, tlg_num=%ld, time=%.10f)",
                         Qry.GetVariableAsString("sender"),
                         (long int)Qry.GetVariableAsInteger("tlg_num"),
                         nowUTC);
        Qry.Close();
    }
    catch( std::exception &e)
    {
        ProgError(STDLOG, e.what());
        throw;
    }
    catch(...)
    {
        ProgError(STDLOG, "sendTlg: Unknown error");
        throw;
    };
}

void loadTlg(const std::string &text)
{
    try
    {
        BASIC::TDateTime nowUTC=BASIC::NowUTC();
        TQuery Qry(&OraSession);
        Qry.SQLText=
          "BEGIN "
          "  SELECT tlgs_id.nextval INTO :tlg_num FROM dual; "
          "  INSERT INTO tlg_queue(id,sender,tlg_num,receiver,type,status,time,ttl,time_msec,last_send) "
          "  VALUES(:tlg_num,:sender,:tlg_num,:receiver,:type,'PUT',:time,:ttl,:time_msec,NULL); "
          "END;";
        Qry.CreateVariable("sender",otString,OWN_CANON_NAME());
        Qry.CreateVariable("receiver",otString,OWN_CANON_NAME());
        Qry.CreateVariable("type",otString,"INB");
        Qry.CreateVariable("time",otDate,nowUTC);
        Qry.CreateVariable("ttl",otInteger,FNull);
        Qry.CreateVariable("time_msec",otFloat,nowUTC);
        Qry.CreateVariable("tlg_num",otInteger,FNull);
        Qry.Execute();
        Qry.SQLText=
          "INSERT INTO tlgs(id,sender,tlg_num,receiver,type,time,tlg_text,error) "
          "VALUES(:tlg_num,:sender,:tlg_num,:receiver,:type,:time,:text,NULL)";
        Qry.DeclareVariable("text",otLong);
        Qry.SetLongVariable("text",(void *)text.c_str(),text.size());
        Qry.DeleteVariable("ttl");
        Qry.DeleteVariable("time_msec");
        Qry.Execute();
        ProgTrace(TRACE5,"IN: PUT (sender=%s, tlg_num=%ld, time=%.10f)",
                         Qry.GetVariableAsString("sender"),
                         (long int)Qry.GetVariableAsInteger("tlg_num"),
                         nowUTC);
        Qry.Close();
    }
    catch( std::exception &e)
    {
        ProgError(STDLOG, e.what());
        throw;
    }
    catch(...)
    {
        ProgError(STDLOG, "loadTlg: Unknown error");
        throw;
    };
};
/*
void sendErrorTlg(const char *format, ...)
{
  try
  {
    const char *sender=OWN_CANON_NAME();
    const char *receiver=ERR_CANON_NAME();

    if (*receiver==0) return;
    char Message[500];
    if (format==NULL) return;
    va_list ap;
    va_start(ap, format);
    sprintf(Message,"Sender: %s\n",sender);
    int len=strlen(Message);
    vsnprintf(Message+len, sizeof(Message)-len, format, ap);
    Message[sizeof(Message)-1]=0;
    va_end(ap);

    sendTlg(receiver,sender,false,0,Message);
  }
  catch( std::exception &e)
  {
      ProgError(STDLOG, e.what());
  }
  catch(...)
  {
      ProgError(STDLOG, "sendErrorTlg: Unknown error");
  };
};*/

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
      ProgError(STDLOG, e.what());
      throw;
  }
  catch(...)
  {
      ProgError(STDLOG, "deleteTlg: Unknown error");
      throw;
  };
};

bool errorTlg(int tlg_id, string type, string msg)
{
  try
  {
    if (deleteTlg(tlg_id))
    {
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
    else return false;
  }
  catch( std::exception &e)
  {
      ProgError(STDLOG, e.what());
      throw;
  }
  catch(...)
  {
      ProgError(STDLOG, "errorTlg: Unknown error");
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
      ProgError(STDLOG, e.what());
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
  try
  {
    if (receiver==NULL || *receiver==0)
      throw EXCEPTIONS::Exception( "sendCmd: receiver not defined");
    if (cmd==NULL || *cmd==0)
      throw EXCEPTIONS::Exception( "sendCmd: cmd not defined");
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

    if (sendto(sockfd,cmd,strlen(cmd),MSG_DONTWAIT,
               (struct sockaddr*)&sock_addr,sizeof(sock_addr))==-1)
    {
      if (errno!=EAGAIN)
        throw EXCEPTIONS::Exception("sendCmd: 'sendto' error %d: %s",errno,strerror(errno));
      ProgTrace(TRACE5,"sendCmd: 'sendto' error %d: %s",errno,strerror(errno));
    }
    else
      ProgTrace(TRACE5,"sendCmd: cmd '%s' sended to %s (time=%ld)",cmd,receiver,time(NULL));
  }
  catch(EXCEPTIONS::Exception E)
  {
    ProgTrace(TRACE0,"Exception: %s",E.what());
  };
};

bool waitCmd(const char* receiver, int msecs, const char* buf, int buflen)
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
        ProgTrace(TRACE5,"waitCmd: cmd '%s' received from %s (time=%ld)",buf,receiver,time(NULL));
        return true;
      };
    };
  }
  catch(EXCEPTIONS::Exception E)
  {
    ProgError(STDLOG,"Exception: %s",E.what());
  };
  return false;
};


