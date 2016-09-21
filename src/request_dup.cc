//---------------------------------------------------------------------------
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "request_dup.h"
#include "basic.h"
#include "xml_unit.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "tlg/tlg.h"
#include "serverlib/ourtime.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

const int SOCKET_ERROR = -1;

using namespace std;

bool SEND_REQUEST_DUP()
{
  static int VAR=ASTRA::NoExists;
  if (VAR==ASTRA::NoExists)
    VAR=getTCLParam("SEND_REQUEST_DUP",0,1,0);
  return VAR!=0;
}

bool RECEIVE_REQUEST_DUP()
{
  static int VAR=ASTRA::NoExists;
  if (VAR==ASTRA::NoExists)
    VAR=getTCLParam("RECEIVE_REQUEST_DUP",0,1,0);
  return VAR!=0;
}

void throw_if_request_dup(const std::string &where)
{
  if (RECEIVE_REQUEST_DUP())
    throw EXCEPTIONS::Exception("%s: forbidden operation when request duplicate mode", where.c_str());
}


void TestInterface::TestRequestDup(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode)
{
  BASIC::TDateTime start_time = BASIC::NowUTC();
  for(;;)
  {
    if ((BASIC::NowUTC()-start_time)*86400000>200) break;  //задержка на 50 мсек
  };
  NewTextChild(resNode, "iteration", NodeAsInteger("iteration", reqNode) );
};

int GetLastError1( int Ret, int errCode )
{
  if ( Ret != errCode )
    return 0;
  else {
    ProgTrace( TRACE5, "GetLastError, ret=%d", Ret );
    return errno;
  }
}

int GetErr1( int code, const char *func )
{
  if ( code != 0 ) {
    throw EXCEPTIONS::Exception("TCPSession error:%s, error=%d, func=%s", strerror( code ), code, func);
  }
  return code;
}

int init_socket_grp( const string &var_addr_grp, const string &var_port_grp )
{
  string addr_grp = getTCLParam( var_addr_grp.c_str(), "" );
  string port_grp = getTCLParam( var_port_grp.c_str(), "" );
  ProgTrace( TRACE5, "%s, %s, %s:%s", var_addr_grp.c_str(), var_port_grp.c_str(),
             addr_grp.c_str(), port_grp.c_str() );

  int handle = socket( AF_INET, SOCK_STREAM, 0 );
  GetErr1( GetLastError1( handle, SOCKET_ERROR ), "socket()" );
  GetErr1( GetLastError1( fcntl( handle, F_SETFL, O_NONBLOCK ), SOCKET_ERROR ), "fcntl" );
  sockaddr_in addr_in;
  addr_in.sin_family = AF_INET;
  int port;
  if ( BASIC::StrToInt( port_grp.c_str(), port ) == EOF )
    port = -1;
  addr_in.sin_port = htons( port );
  addr_in.sin_addr.s_addr = inet_addr( addr_grp.c_str() );
  int val = 1;
  GetErr1( GetLastError1( setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, &val, sizeof (val)), SOCKET_ERROR ), "setsockopt" );
  int one = 1;
  GetErr1( GetLastError1( setsockopt( handle, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof(one) ), SOCKET_ERROR ), "setsockopt" );
  int err = connect( handle, (struct sockaddr *)&addr_in, sizeof(addr_in) );
  err = GetLastError1( err, SOCKET_ERROR );
  if ( err != 0 && err != EINPROGRESS )
    GetErr1( err, "connect" );
  return handle;
}

int exec_grp( int handle, const char *buf, int len )
{
  fd_set rsets, wsets, esets;
  FD_ZERO( &rsets );
  FD_ZERO( &wsets );
  FD_ZERO( &esets );
  FD_SET( handle, &rsets );
  FD_SET( handle, &wsets );
  FD_SET( handle, &esets );

  timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
  GetErr1( GetLastError1( select( handle + 1, &rsets, &wsets, &esets, &timeout ), SOCKET_ERROR ), "select" );
  socklen_t err_len;
  int error, received = 0, sended = 0;
  err_len = sizeof( error );
  if ( FD_ISSET( handle, &esets ) ||
       GetErr1( GetLastError1( getsockopt( handle, SOL_SOCKET, SO_ERROR, &error, &err_len ), -1 ), "getsockopt" ) || error != 0 ) {
    throw EXCEPTIONS::Exception("exec_grp: invalid socket stat");
    return 0;
  }
  if ( FD_ISSET( handle, &rsets ) ) {
    int BLen = 0;
    GetErr1( GetLastError1( ioctl( handle, FIONREAD, &BLen ), SOCKET_ERROR ), "ioctl" );
    if ( BLen == 0 )
      throw EXCEPTIONS::Exception("Connect aborted");
    char recbuf[1000000];
    received = recv( handle, recbuf, sizeof(recbuf), 0 );
    GetErr1( GetLastError1( received, SOCKET_ERROR ), "recv" );
  }
  if ( !FD_ISSET( handle, &wsets ) ) {
    ProgError( STDLOG, "send_grp - cannot write to socket" );
  }
  else
    if ( len > 0 ) {
      sended = send( handle, buf, len, 0 ); // отправляем сколько сможем
      GetErr1( GetLastError1( sended, SOCKET_ERROR ), "send" );
    }
  if ( received + sended > 0 )
    ProgTrace( TRACE5, "received=%d, sended=%d", received, sended );
  return sended;
}

int main_request_dup_tcl(int supervisorSocket, int argc, char *argv[])
{
  try {
    sleep(5);
    InitLogTime(argc>0?argv[0]:NULL);

    int handle_grp2 = init_socket_grp( "DUP_ADDR_GRP2", "DUP_PORT_GRP2" );
    int handle_grp3 = init_socket_grp( "DUP_ADDR_GRP3", "DUP_PORT_GRP3" );

    char buf[100000];
    vector<string> bufs;
    for( ;; )
    {
      InitLogTime(argc>0?argv[0]:NULL);
      int len = waitCmd("REQUEST_DUP",10,buf,sizeof(buf));
      if ( len > 0 ) {
        ProgTrace( TRACE5, "incomming msg, size()=%d", len - 1 );
        if ( bufs.size() < 1000 )
          bufs.push_back( string( buf, len ) );
        else
          ProgError( STDLOG, "incomming buffers more then 1000 msgs" );
      }
      int handle;
      for ( int igrp=2; igrp<4; igrp++ ) {
        if ( igrp == 2  )
          handle = handle_grp2;
         else
          handle = handle_grp3;
        for ( vector<string>::iterator i=bufs.begin(); i!=bufs.end();  ) {
          if ( i->c_str()[ 0 ] != igrp  ) {
            i++;
            continue;
          }
          int sended = exec_grp( handle, i->data() + 1, i->size() - 1 );
          if ( sended ) {
            i->erase( 1, sended );
          }
          if ( i->size() == 1 ) {
            i = bufs.erase( i );
          }
          else
            break;
        }
        exec_grp( handle, NULL, 0 );
      }
      //sleep(30); расскоментарив эту строку можно создать ситуацию переполнения sendto,
      //           которая однако не приводит к плохим последствиям для сервера, перенаправляющего поток запросов
    }
  }
  catch( EXCEPTIONS:: Exception &e ) {
    ProgError( STDLOG, "Exception: %s", e.what() );
  }
  catch( ... ) {
    ProgError( STDLOG, "Unknown error" );
  };
  return 0;
}

bool BuildMsgForTermRequestDup(const std::string &pult,
                               const std::string &opr,
                               const std::string &body,
                               std::string &msg)
{
  msg.clear();
  char head[100];
  memset( &head, 0, sizeof(head) );
  char *p=head+4*3+4*8;
  strncpy(p,pult.c_str(),6);
  strcpy(p+6,"  ");
  strcpy(p+6+2,opr.c_str());
  head[84]=char(0x10);
  head[99]='D'; //признах что REQUEST_DUP
  *(long int*)head=htonl(body.size());
  msg+=char(3);
  msg+=std::string(head,sizeof(head));
  msg+=body;
  return true;
};

bool BuildMsgForWebRequestDup(short int client_id,
                              const std::string &body,
                              std::string &msg)
{
  msg.clear();
  char head[100];
  memset( &head, 0, sizeof(head) );
  char *p=head+4*3+4*8;
  *(short int*)p=htons(client_id);
  *(long int*)head=htonl(body.size());
  head[99]='D'; //признах что REQUEST_DUP
  msg+=char(2);
  msg+=std::string(head,sizeof(head));
  msg+=body;
  return true;
};

