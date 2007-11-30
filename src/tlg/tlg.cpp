#include "tlg.h"
#include <string>
#include "exceptions.h"
#include "oralib.h"
#include "cfgproc.h"
#include "logger.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "test.h"

using namespace EXCEPTIONS;
using namespace std;

const char* ETS_CANON_NAME()
{
  static string ETSNAME;
  if ( ETSNAME.empty() ) {
    char r[100];
    r[0]=0;
    if ( get_param( "ETS_CANON_NAME", r, sizeof( r ) ) < 0 )
      throw EXCEPTIONS::Exception( "Can't read param ETS_CANON_NAME" );
    ETSNAME = r;
    ProgTrace( TRACE5, "ETS_CANON_NAME=%s", ETSNAME.c_str() );
  }
  return ETSNAME.c_str();
}

const char* OWN_CANON_NAME()
{
  static string OWNNAME;
  if ( OWNNAME.empty() ) {
    char r[100];
    r[0]=0;
    if ( get_param( "OWN_CANON_NAME", r, sizeof( r ) ) < 0 )
      throw EXCEPTIONS::Exception( "Can't read param OWN_CANON_NAME" );
    OWNNAME = r;
    ProgTrace( TRACE5, "OWN_CANON_NAME=%s", OWNNAME.c_str() );
  }
  return OWNNAME.c_str();
}

const char* ERR_CANON_NAME()
{
  static bool init=false;
  static string ERRNAME;
  if ( !init ) {
    char r[100];
    r[0]=0;
    if ( get_param( "ERR_CANON_NAME", r, sizeof( r ) ) >= 0 )
      ERRNAME = r;
    ProgTrace( TRACE5, "ERR_CANON_NAME=%s", ERRNAME.c_str() );
    init=true;
  }
  return ERRNAME.c_str();
}

const char* DEF_CANON_NAME()
{
  static bool init=false;
  static string DEFNAME;
  if ( !init ) {
    char r[100];
    r[0]=0;
    if ( get_param( "DEF_CANON_NAME", r, sizeof( r ) ) >= 0 )
      DEFNAME = r;
    ProgTrace( TRACE5, "DEF_CANON_NAME=%s", DEFNAME.c_str() );
    init=true;
  }
  return DEFNAME.c_str();
}

const char* OWN_SITA_ADDR()
{
  static string OWNADDR;
  if ( OWNADDR.empty() ) {
    char r[100];
    r[0]=0;
    if ( get_param( "OWN_SITA_ADDR", r, sizeof( r ) ) < 0 )
      throw EXCEPTIONS::Exception( "Can't read param OWN_SITA_ADDR" );
    OWNADDR = r;
    ProgTrace( TRACE5, "OWN_SITA_ADDR=%s", OWNADDR.c_str() );
  }
  return OWNADDR.c_str();
}

void sendCmdTlgSnd()
{
  sendCmd("CMD_TLG_SND","H");
}

void sendTlg(const char* receiver,
             const char* sender,
             bool isEdi,
             int ttl,
             const std::string &text)
{
    try
    {
        TQuery Qry(&OraSession);
        Qry.SQLText=
                "INSERT INTO "
                "tlg_queue(id,sender,tlg_num,receiver,type,status,time,ttl) "
                "VALUES"
                "(tlgs_id.nextval,:sender,tlgs_id.nextval,:receiver,"
                ":type,'PUT',system.UTCSYSDATE,:ttl)";
        Qry.CreateVariable("sender",otString,sender);
        Qry.CreateVariable("receiver",otString,receiver);
        Qry.CreateVariable("type",otString,isEdi?"OUTA":"OUTB");
        if (isEdi&&ttl>0)
          Qry.CreateVariable("ttl",otInteger,ttl);
        else
          Qry.CreateVariable("ttl",otInteger,FNull);
        Qry.Execute();
        Qry.SQLText=
                "INSERT INTO "
                "tlgs(id,sender,tlg_num,receiver,type,time,tlg_text,error) "
                "VALUES"
                "(tlgs_id.currval,:sender,tlgs_id.currval,:receiver,"
                ":type,system.UTCSYSDATE,:text,NULL)";
        Qry.DeclareVariable("text",otLong);
        Qry.SetLongVariable("text",(void *)text.c_str(),text.size());
        Qry.DeleteVariable("ttl");
        Qry.Execute();
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
        TQuery Qry(&OraSession);
        Qry.SQLText=
                "INSERT INTO "
                "tlg_queue(id,sender,tlg_num,receiver,type,status,time,ttl) "
                "VALUES"
                "(tlgs_id.nextval,:sender,tlgs_id.nextval,:receiver,"
                ":type,'PUT',system.UTCSYSDATE,:ttl)";
        Qry.CreateVariable("sender",otString,OWN_CANON_NAME());
        Qry.CreateVariable("receiver",otString,OWN_CANON_NAME());
        Qry.CreateVariable("type",otString,"INB");
        Qry.CreateVariable("ttl",otInteger,FNull);
        Qry.Execute();
        Qry.SQLText=
                "INSERT INTO "
                "tlgs(id,sender,tlg_num,receiver,type,time,tlg_text,error) "
                "VALUES"
                "(tlgs_id.currval,:sender,tlgs_id.currval,:receiver,"
                ":type,system.UTCSYSDATE,:text,NULL)";
        Qry.DeclareVariable("text",otLong);
        Qry.SetLongVariable("text",(void *)text.c_str(),text.size());
        Qry.DeleteVariable("ttl");
        Qry.Execute();
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
      if ( get_param( receiver, sock_addr.sun_path, sizeof (sock_addr.sun_path) - 1 ) < 0 )
        throw EXCEPTIONS::Exception( "sendCmd: can't read parameter '%s'", receiver );
      addrs[receiver]=sock_addr.sun_path;
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
    ProgError(STDLOG,"Exception: %s",E.what());
  };
};

bool waitCmd(const char* receiver, int secs, const char* buf, int buflen)
{
  if (receiver==NULL || *receiver==0)
    throw EXCEPTIONS::Exception( "sendCmd: receiver not defined");
  if (buf==NULL || buflen <= 1 )
    throw EXCEPTIONS::Exception( "sendCmd: buf not defined");
  static map<string,int> sockfds;

  int sockfd;
  if (sockfds.find(receiver)==sockfds.end())
  {
    if ((sockfd=socket(AF_UNIX,SOCK_DGRAM,0))==-1)
      throw EXCEPTIONS::Exception("waitCmd: 'socket' error %d: %s",errno,strerror(errno));
    try
    {
      struct sockaddr_un sock_addr;
      memset(&sock_addr,0,sizeof(sock_addr));
      sock_addr.sun_family=AF_UNIX;
      if ( get_param( receiver, sock_addr.sun_path, sizeof (sock_addr.sun_path) - 1 ) < 0 )
        throw EXCEPTIONS::Exception( "waitCmd: can't read parameter '%s'", receiver );
      unlink(sock_addr.sun_path);
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
    tv.tv_sec=secs;
    tv.tv_usec=0;
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


