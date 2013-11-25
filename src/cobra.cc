/*
Основные праила по передаче информации между системами
1. Передача сообщений осуществляется с использованием протокола TCP-IP
2. Программа синхронизации данных реализована на стороне сервера и представляет собой
процесс - демон (Сирена). Реализуется разработчиками DCS Астра.
3. Процесс выступает в роли сервера (TCP-IP) - слушает порт на предмет входящих сообщений, устанавливает соединение и осуществляет прием и передачу данных (для "Кобра" СПП на прием, результаты регистрации на выдачу). Будем называть этот процесс Сервер.
Программа, которая будет выступать в роли Клиента - разрабатывается сторонними разработчиками
4. В случаях, когда возникает ошибочная ситуация - потеря связи, ошибки в канале, происходит удаления соединения со стороны Сервера.
Клиент должен уметь правильно реагировать на разрыв соединения - восстанавливать соединение.
5. Над протоколом TCP-IP реализован протокол, определящий правила и формат передачи данных.
Соединение со стороны сервера открывается со свойством SO_KEEPALIVE.
Передача данных осуществялется пакетами, имеющими ниже описанный формат.
<Сообщение>=<Заголовок сообщения><Тело сообщения>;
<Заголовок сообщения>=<Длина сообщения><Ид. сообщения><Флаги>;
<Длина сообщения>= LongInt (4 байта);
Указывается длина сообщения без длины заголовка
<Ид. сообщения>= LongInt (4 байта);
Уникальное в рамках клиента или сервера, но возможно совпадение этого идентификатора у клиента с сервером.
<Флаги>= LongInt (4 байта);
Битовая маска:
$01000000
если бит установлен, то сообщение зашифровано (пока не используем)
$02000000
если установлен этот бит, то пришло текстовое сообщение (кодировка CP866), иначе XML-сообщение (кодировка UTF-8)
$04000000
если установлен этот бит, то сообщение сжато zip-архив (пока не используем)
$80000000
если установлен этот бит, то пришел запрос, иначе это ответ
Этот бит + ид. сообщения - уникальный ключ в рамках всех передаваемых/принимаемых сообщений
Каждый раз, когда делается запрос, надо выставлять этот бит.
Каждый раз, когда делается ответ, надо сбрасывать этот бит.
<Тело сообщения> - представляет собой xml-документ, переведенный в текст, в кодировке UTF8.

За один раз может прийти сразу несколько пакетов.

Для проверки канала со стороны как Клиента так и Cервера предусмотрено служебное сообщение,
которое может передаваться в любой момент времени. В случае прихода такого сообщения надо передать
его обратно изменив бит "запрос" на бит "ответ" в кратчайший срок. Формат сообщения:
<?xml version="1.0" encoding="UTF-8"?>
<heartbeat/>
Такое сообщение Клиент будет получать каждые 30 секунд в случае, когда нет активности в TCP канале. Это время задается на стороне сервера и его можно изменить.
Если ответа на сообщение не последует в течении 30*2=60 секунд, то происходит закрытие соединения со стороны Сервера.

Правила передачи сообщений для синхронизации данными с системой "Кобра" (не входит в логику протокола):
1. Передача данных в сообщениях и их обработка должна проходит в порядке в котором происходили изменение на одной из сторон.
Например: Пассажир зарегистрирован - сообщение1, далее пассажир разрегистрирован - сообщение2. Сообщение2 должно прийти и обработаться после сообщения1.
2. Факт приема и обработки сообщения должно быть подтверждено с другой стороны. Только после этого можно считать, что сообщение доставлено. Иначе надо продолжать делать попытки доставить это сообщение.
Формат подтверждения доставки и обработки сообщения:
<?xml version="1.0" encoding="UTF-8"?>
<error code="" message="">
<сommit>
тег <error> будет приходит только в случае, когда возникли логические проблемы с разбором и записью данных. Например пришла информация на добавления рейса, а рейс уже заведен в системе "Астра".
На любой запрос может прийти ответ в формате:
<?xml version="1.0" encoding="UTF-8"?>
<invalid_msg>
Это означает, что формат сообщения неправильный.
3. В сообщениях времена передаются в формате UTC
4. В сообщениях передаются только изменения. Например: если по рейсу нет изменений, то информацию по нему (СПП) не надо передавать, если это конечно не начальное формирование СПП.
Форматы xml-запросов были обсуждены ранее Чичеровым Владом и описаны в приложении к письму
Буду рад, если последуют вопросы, пожелания, предложения.

*/

//---------------------------------------------------------------------------
#define __STDC_FORMAT_MACROS
#include <stdio.h>
#include <signal.h>
#include <inttypes.h>
#include <fstream>

#include <cobra.h>
#include "serverlib/query_runner.h"
#include "oralib.h"
#include "exceptions.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "astra_misc.h"
#include "base_tables.h"
#include "stl_utils.h"
#include "points.h"
#include "season.h"
#include "cent.h"
#include "salons.h"
#include "salonform.h"
#include "file_queue.h"
#include "stl_utils.h"
#include "tlg/tlg.h"
#include "xml_unit.h"
#include "jxtlib/xml_stuff.h"
#include "serverlib/logger.h"
#include "empty_proc.h"
#include "serverlib/posthooks.h"
#include "serverlib/perfom.h"

#define NICKNAME "DJEK"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"
#include "serverlib/ourtime.h"

#define WAIT_PARSER_INTERVAL           30000      //миллисекунды

const
  int MAX_LISTENER_WAIT_CONNECTIONS = 1;
  int TCPSESSION_RECEIVE_BUFFER = 1024*8;
  unsigned int HEADER_SIZE = 12;
  //int WAIT_ANSWER_MSEC = 5000; // врем ожидания ответа
  int HEART_BEAT_MSEC = 30000; // время ожидания активности на прием в сети
  std::string STR_HEARTBEAT = "heartbeat";

  int COBRA_INQUEUE_COUNT_MSGS = 5;
  int COBRA_OUTQUEUE_COUNT_MSGS = 5;
  int WAIT_ANSWER_SEC = 60;
  std::string COBRA_FLIGHTS_COMMAND_TAG = "flights";
  std::string COBRA_STAGES_COMMAND_TAG = "stages";

using namespace ASTRA;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace std;

void ToEvents( const string &client_type, int request_file_id, int answer_file_id )
{
  if ( request_file_id == ASTRA::NoExists && answer_file_id == ASTRA::NoExists )
    return;
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "INSERT INTO tcp_data_events(id,client_type,file_in_id,file_out_id,time) "
    " SELECT tlgs_id.nextval,:client_type,:file_in_id,:file_out_id,system.UTCSYSDATE FROM dual";
  Qry.CreateVariable( "client_type", otString, client_type );
  if ( request_file_id == NoExists)
    Qry.CreateVariable( "file_in_id", otInteger, FNull );
  else
    Qry.CreateVariable( "file_in_id", otInteger, request_file_id );
  if ( answer_file_id == NoExists)
    Qry.CreateVariable( "file_out_id", otInteger, FNull );
  else
    Qry.CreateVariable( "file_out_id", otInteger, answer_file_id );
  Qry.Execute();
}

int main_tcp_cobra_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist)
{
  try
  {
    sleep(1);
    InitLogTime(NULL);
    OpenLogFile("log1");

    int p_count;
    int listener_port;
    if ( TCL_OK != Tcl_ListObjLength( interp, argslist, &p_count ) ) {
    	ProgError( STDLOG,
                 "ERROR:main_tcpserv_tcl wrong parameters:%s",
                 Tcl_GetString(Tcl_GetObjResult(interp)) );
      return 1;
    }
    if ( p_count != 2 ) {
    	ProgError( STDLOG,
                 "ERROR:main_timer_tcl wrong number of parameters:%d",
                 p_count );
    }
    else {
    	 Tcl_Obj *val;
    	 Tcl_ListObjIndex( interp, argslist, 1, &val );
    	 StrToInt( Tcl_GetString( val ), listener_port );
    }

    ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks()
        ->connect_db();
        
    if (init_edifact()<0) throw Exception("'init_edifact' error");
    
    TReqInfo::Instance()->clear();
    base_tables.Invalidate();
    
    ProgTrace( TRACE5, "listener_port=%d", listener_port );
    TTCPListener<CobraSession> cobra_serv(listener_port);
    tst();
    cobra_serv.Execute();
  }
  catch( std::exception &E ) {
    ProgError( STDLOG, "std::exception: %s", E.what() );
  }
  catch( ... ) {
    ProgError( STDLOG, "Unknown error" );
  };
  return 0;
}

bool parseIncommingCobraData();

int main_cobra_handler_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist)
{
  try
  {
    sleep(1);
    InitLogTime(NULL);
    OpenLogFile("log1");

    ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks()
            ->connect_db();

     if (init_edifact()<0) throw Exception("'init_edifact' error");

    TReqInfo::Instance()->clear();
    char buf[10];
    int wait_interval;
    for(;;)
    {
      emptyHookTables();
      TDateTime execTask = NowUTC();
      InitLogTime(NULL);
      base_tables.Invalidate();
      bool queue_empty = parseIncommingCobraData();
      OraSession.Commit();
      if ( NowUTC() - execTask > 5.0/(1440.0*60.0) )
      	ProgTrace( TRACE5, "Attention execute task time!!!, name=%s, time=%s","CMD_PARSE_COBRA", DateTimeToStr( NowUTC() - execTask, "nn:ss" ).c_str() );
      callPostHooksAfter();
      if ( queue_empty )
        wait_interval = WAIT_PARSER_INTERVAL;
      else
        wait_interval = 50;
      waitCmd("CMD_PARSE_COBRA",wait_interval,buf,sizeof(buf));
    }; // end of loop
  }
  catch(EOracleError &E)
  {
    ProgError( STDLOG, "EOracleError %d: %s", E.Code, E.what());
    ProgError( STDLOG, "SQL: %s", E.SQLText());
  }
  catch(EXCEPTIONS::Exception &E)
  {
  tst();
    ProgError( STDLOG, "std::exception: %s", E.what() );
  }
  catch(std::exception &E)
  {
    tst();
    ProgError( STDLOG, "std::exception: %s", E.what() );
  }
  catch(...)
  {
    tst();
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
  tst();
  return 0;
};

//////////////////////////////////TCP////////////////////////////////////////
int GetLastError( int Ret, int errCode )
{
  if ( Ret != errCode )
    return 0;
  else {
    ProgTrace( TRACE5, "GetLastError, ret=%d", Ret );
    return errno;
  }
}

int GetErr( int code, const char *func )
{
  if ( code != 0 ) {
    ProgError( STDLOG, "TCPSession error %s, error=%d, func=%s", strerror( code ), code, func );
  }
  return code;
}

TTCPSession::TTCPSession( int vhandle )
{
  handle = vhandle;
  use_heartBeat = canUseHeartBeat();
  HeartBeatOK();
}

bool TTCPSession::DoAcceptConnect( sockaddr_in &addr_out )
{
  ProgTrace( TRACE5, "TTCPSession::DoAcceptConnect, addr_out=%s", inet_ntoa( addr_out.sin_addr ) );
  return AcceptConnect( addr_out );
}

void TTCPSession::DoRead( void *recvbuf, int msg_len )
{
  ProgTrace( TRACE5, "TTCPSession::DoRead, handle=%d, msg_len=%d", handle, msg_len );
  Read( recvbuf, msg_len );
}

int TTCPSession::DoWriteSize( int &msg_id )
{
  int msg_len = WriteSize( msg_id );
  if ( msg_len )
    ProgTrace( TRACE5, "TTCPSession::DoWriteSize, handle=%d, outcomming msg_id=%d, msg_len=%d", handle, msg_id, msg_len );
  return msg_len;
}

void TTCPSession::DoWrite( int msg_id, void *sendbuf, int msg_len )
{
  ProgTrace( TRACE5, "TTCPSession::DoWrite, handle=%d, outcomming msg_id=%d, msg_len=%d", handle, msg_id, msg_len );
  Write( msg_id, sendbuf, msg_len );
}

void TTCPSession::HeartBeatOK()
{
  heartBeat = BASIC::NowUTC();
  pr_wait_heartBeat = false;
}

void TTCPSession::DoHeartBeat()
{
  if ( !use_heartBeat )
    return;
  ProgTrace( TRACE5, "TTCPSession::DoHeartBeat" );
  HeartBeat( );
}

void TTCPSession::DoException()
{
  ProgTrace( TRACE5, "TTCPSession::DoException, handle=%d", handle );
  Exception();
}

void TTCPSession::DoExecute()
{
  ProcessIncommingMsgs();
}
/////////////////////////////////////////////////////////////////////////////////
template <class T>
TTCPListener<T>::~TTCPListener()
{
  close( handle );
}

template <class T>
void TTCPListener<T>::Execute() {
  T sess( -1 );
  ProgTrace( TRACE5, "TTCPListener<%s> Execute", sess.client_type.c_str() );
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "UPDATE tcp_clients SET pr_active=NULL WHERE client_type=:client_type";
  Qry.CreateVariable( "client_type", otString, sess.client_type );
  Qry.Execute();
  OraSession.Commit();
  handle = socket( /*PF_INET*/AF_INET, SOCK_STREAM, 0 );
  if ( handle == -1 ) {
    tst();
    GetErr( GetLastError( handle, -1 ), "socket()" );
  }
  ProgTrace( TRACE5, "handle=%d", handle );
  fcntl( handle, F_SETFL, O_NONBLOCK );
  sockaddr_in addr_in;
  addr_in.sin_family = AF_INET;
  addr_in.sin_port = htons( listen_port );
  addr_in.sin_addr.s_addr = htonl( INADDR_ANY );
  int val = 1;
  int err = setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, &val, sizeof (val));
  ProgTrace( TRACE5, "setsockopt ret=%d", err );
  err = bind( handle, (struct sockaddr *)&addr_in, sizeof(addr_in) );
  ProgTrace( TRACE5, "bind ret=%d", err );
  GetErr( GetLastError( err, -1 ), "bind" );
  err = listen( handle, MAX_LISTENER_WAIT_CONNECTIONS );
  ProgTrace( TRACE5, "listen ret=%d", err );
  GetErr( GetLastError( err, -1 ), "listen" );
  Terminate = false;
  tst();
  int recvbuf_len = TCPSESSION_RECEIVE_BUFFER;
  void *recvbuf = malloc( recvbuf_len );

  try {
    while ( !Terminate ) {
      InitLogTime(NULL);
      fd_set rsets, wsets, esets;
      FD_ZERO( &rsets );
      FD_ZERO( &wsets );
      FD_ZERO( &esets );
      FD_SET( handle, &rsets ); // прослушку добавили
      int maxh = handle;
      for ( typename map<int,T*>::iterator i=hclients.begin(); i!=hclients.end(); i++ ) {
        FD_SET( i->first, &rsets );
        FD_SET( i->first, &wsets );
        FD_SET( i->first, &esets );
        if ( maxh < i->first )
          maxh = i->first;
      }
      timeval timeout;
      timeout.tv_sec = 0;
      timeout.tv_usec = 100;
      err = select( maxh + 1, &rsets, &wsets, &esets, &timeout );
      if ( !GetErr( GetLastError( err, -1 ), "select" ) ) {
        if ( FD_ISSET( handle, &rsets ) ) {
          sockaddr_in addr_out;
          addr_out.sin_family = AF_INET;
          socklen_t addrsize = sizeof(addr_out);
          int client_handle = accept( handle, (sockaddr*)&addr_out, &addrsize );
          if ( !GetErr( GetLastError( client_handle, -1 ), "accept" ) ) {
            GetErr( GetLastError( fcntl( client_handle, F_SETFL, O_NONBLOCK ), -1 ), "fcntl" );
            int one = 1;
            GetErr( GetLastError( setsockopt( client_handle, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof(one) ), -1 ), "setsockopt" );
            AddClient( client_handle, addr_out );
          }
        }
      }
      vector<int> delhds;
      for ( typename map<int,T*>::iterator i=hclients.begin(); i!=hclients.end(); i++ ) {
        socklen_t err_len;
        int error;
        err_len = sizeof( error );
        if( GetErr( GetLastError( getsockopt( i->first, SOL_SOCKET, SO_ERROR, &error, &err_len ), -1 ), "getsockopt" ) || error != 0 ) {
          delhds.push_back( i->first );
        }
        else {
          if ( FD_ISSET( i->first, &rsets ) ) {
            int msg_len = recv( i->first, recvbuf, recvbuf_len, 0 );
            if ( msg_len <= 0 ) {
              delhds.push_back( i->first );
            }
            else {
              if ( i->second->use_heartBeat ) {
                i->second->heartBeat = BASIC::NowUTC(); // что-то пришло, значит соединение живо
              }
              try {
                i->second->DoRead( recvbuf, msg_len );
              }
              catch(...) {
                ProgError( STDLOG, "TCPSession::DoRead exception, handle=%d", i->first );
              }
            }
          }
          if ( FD_ISSET( i->first, &wsets ) ) {
            int msg_id = -1;
            int msg_len = i->second->DoWriteSize( msg_id ); // размер сообщения, который надо отправить
            if ( msg_len == 0 )
              i->second->getOutCommingMsgs();
            else { // есть что отправить
              buffers[ i->first ].sendbuf = realloc( buffers[ i->first ].sendbuf, buffers[ i->first ].sendbuf_len + msg_len ); // добавляем память до того, чтобы вместилось новое сообщение
              buffers[ i->first ].sendbuf_len += msg_len; //!!!зависит от сессии - для разных разный буфер;
              void *msg_send_buf = (void*)((uintptr_t)buffers[ i->first ].sendbuf + buffers[ i->first ].sendbuf_len - msg_len);
              try {
                i->second->DoWrite( msg_id, msg_send_buf, msg_len ); // заполняем память данными нового сообщения
                msg_len = send( i->first, buffers[ i->first ].sendbuf, buffers[ i->first ].sendbuf_len, 0 ); // отправляем сколько сможем
                for ( int jj=0; jj<msg_len; jj++ ) {
                  char *k = (char*)buffers[ i->first ].sendbuf;
                  ProgTrace( TRACE5, "send byte[%d]=%d, %c", jj, (int)k[jj], k[jj] );
                }
                ProgTrace( TRACE5, "send handle=%d, sendbuf_len=%d, msg_len=%d", i->first, buffers[ i->first ].sendbuf_len, msg_len );
                if ( buffers[ i->first ].sendbuf_len != msg_len )
                  memmove( buffers[ i->first ].sendbuf, (void*)((uintptr_t)buffers[ i->first ].sendbuf + msg_len), buffers[ i->first ].sendbuf_len - msg_len );
                buffers[ i->first ].sendbuf_len -= msg_len; // уменьшаем память на порцию отправленных данных
                buffers[ i->first ].sendbuf = realloc( buffers[ i->first ].sendbuf, buffers[ i->first ].sendbuf_len );
              }
              catch(...) {
                ProgError( STDLOG, "TCPSession::DoWrite exception, handle=%d", i->first );
              }
            }
            if ( i->second->use_heartBeat && i->second->heartBeat + HEART_BEAT_MSEC/(24.0*60.0*60.0*1000) < BASIC::NowUTC() ) {
              if ( i->second->pr_wait_heartBeat ) { // не пришел ответ о корректности сети - удаляем соединение
                delhds.push_back( i->first );
              }
              else {
                i->second->DoHeartBeat(); //послать сообщение для проверки сети
                i->second->heartBeat = BASIC::NowUTC();
                i->second->pr_wait_heartBeat = true;
              }
            }
          }
          if ( FD_ISSET( i->first, &esets ) ) {
            try {
              i->second->DoException();
            }
            catch(...) {
              ProgError( STDLOG, "TCPSession::DoException exception, handle=%d", i->first );
            }
          }
          try {
            i->second->DoExecute();
          }
          catch(...) {
            ProgError( STDLOG, "TCPSession::DoExecute exception, handle=%d", i->first );
          }
        }
        OraSession.Commit();
        i->second->DoHook();
      }
      for ( vector<int>::iterator i=delhds.begin(); i!=delhds.end(); i++ ) {
        ProgTrace( TRACE5, "close socket, handle=%d", *i );
        GetErr( GetLastError( close( *i ), -1 ), "close" );
        RemoveClient( *i );
      }
      OraSession.Commit();
      sleep(1);
    }
  }
  catch(...) {
    try { OraSession.Rollback(); }catch(...){};
    while ( !hclients.empty() ) {
      GetErr( GetLastError( close( hclients.begin()->first ), -1 ), "close" );
      RemoveClient( hclients.begin()->first );
    }
    OraSession.Commit();
    free( recvbuf );
  }
  while ( !hclients.empty() ) {
    GetErr( GetLastError( close( hclients.begin()->first ), -1 ), "close" );
    RemoveClient( hclients.begin()->first );
  }
  free( recvbuf );
  OraSession.Commit();
}

template <class T>
void TTCPListener<T>::AddClient( int vhandle, sockaddr_in &addr_out )
{
  if ( !DoAcceptConnect( addr_out ) )
    return;
  T* sess = new T( vhandle );
  try {
    if ( sess->DoAcceptConnect( addr_out ) ) {
      hclients[ vhandle ] = sess;
      TQuery Qry( &OraSession );
      Qry.SQLText =
        "UPDATE tcp_clients SET pr_active=1 WHERE desk=:desk";
      Qry.CreateVariable( "desk", otString, sess->desk );
      Qry.Execute();
      ProgTrace( TRACE5, "AddClient: handle=%d, addr_out=%s, clisnt_type=%s, desk=%s ",
                 vhandle, inet_ntoa( addr_out.sin_addr ), sess->client_type.c_str(), sess->desk.c_str() );
    }
  }
  catch(...) {
    ProgError( STDLOG, "TCPSession::DoAcceptConnect exception, handle=%d", vhandle );
  }
}

template <class T>
void TTCPListener<T>::RemoveClient( int vhandle )
{
  T* sess = hclients[ vhandle ];
  TQuery Qry( &OraSession );
  Qry.SQLText =
    "UPDATE tcp_clients SET pr_active=NULL WHERE desk=:desk";
  Qry.CreateVariable( "desk", otString, sess->desk );
  Qry.Execute();
  ProgTrace( TRACE5, "RemoveClient: handle=%d, clisnt_type=%s, desk=%s ",
             vhandle, sess->client_type.c_str(), sess->desk.c_str() );
  hclients[ vhandle ] = NULL;
  free( buffers[ vhandle ].sendbuf );
  hclients.erase( vhandle );
  delete sess;
}

template <class T>
bool TTCPListener<T>::DoAcceptConnect( sockaddr_in &addr_out )
{
  return AcceptConnect( addr_out );
}

/////////////////////////////////////////////////////////////////////

//HEADER: <LEN(4)><ID(4)><FLAGS(4)><BODY>
//FLAGS[ 0 ] AND 01 - шифрование
//FLAGS[ 0 ] AND 02 - текстовое сообщение
//FLAGS[ 0 ] AND 04 - сжатие
//FLAGS[ 0 ] AND 80 - запрос от клиента, на который надо ответить с выключенным флагом
// ID - ид. сообщения - уникальная вещь относительно клиента/сервера (ID,FLAGS[ 0 ] AND 80)


bool ServSession::AcceptConnect( sockaddr_in &addr_out )
{
  return true;
}


/* status:
  smDelete - работа закончена с сообщением
  smWaitAnswer - ждем ответа от клиента
  smCommit - сообщение готово к обработке (есть входящее)
  smForSend - сообщение готово к отправке
  flags:
  $01000000 - шифрование
  $02000000 - текст CP866, иначе xml UTF-8
  $04000000 - ZIP
  $80000000 - запрос
*/

bool isHeartBeatMsg( const TAstraServMsg &msg )
{
  bool res = false;
  bool pr_test = ( msg.strbody == "<_connection_test_>" );
  bool pr_text = ( (msg.flags & 0x02000000) == 0x02000000 );
  if ( !pr_test && !pr_text ) {
    xmlDocPtr doc = TextToXMLTree( msg.strbody );
    try {
      res = ( doc != NULL && doc->children != NULL && string( (char *)doc->children->name ) == STR_HEARTBEAT );
    }
    catch(...) {
      xmlFreeDoc( doc );
      throw;
    }
    xmlFreeDoc( doc );
  }
  return res;
}

void ServSession::PutMsg( int msg_id, int msg_flags, const std::string &strbody )
{
  TAstraServMsg msg;
  msg.msg_id = msg_id;
  msg.strbody = strbody;
  msg.flags = msg_flags;
  ProgTrace( TRACE5, "ServSession::parseBuffer body=%s", msg.strbody.c_str() );
/*  for ( int i=0; i< msg.strbody.size(); i++ ) {
    ProgTrace( TRACE5, "char=%s, code=%d", msg.strbody.substr(i,1).c_str(), msg.strbody.c_str()[ i ] );
  }*/
  bool pr_test = ( msg.strbody == "<_connection_test_>" );
  if ( pr_test ) {
    msg.flags = msg.flags | 0x80000000;
  }
  tst();
  ProgTrace( TRACE5, "ServSession::PutMsg msg_id=%d, msg_len=%zu, msg_flags=%d, isClientRequest=%d, pr_crypt=%d, pr_txt=%d, pr_zip=%d, pr_test=%d",
             msg.msg_id, msg.strbody.size(), msg.flags, TAstraServMsg::isClientRequest(msg.flags),
             ( msg.flags & 0x01000000 ), ( msg.flags & 0x02000000 ), ( msg.flags & 0x04000000 ), pr_test );
  tst();
  if ( pr_test ||
       (use_heartBeat && TAstraServMsg::isClientRequest(msg.flags) && isHeartBeatMsg( msg )) ) {
    tst();
    msg.setClientAnswer();
    if ( pr_test || outmsgs.empty() )
      outmsgs.push_back( msg );
    else
      outmsgs.insert( outmsgs.begin(), msg );
  }
  else {   //!!!???
    if ( use_heartBeat && pr_wait_heartBeat && !TAstraServMsg::isClientRequest(msg.flags) && isHeartBeatMsg( msg ) )
      HeartBeatOK();
    else {
      tst();
      msg.processTime = BASIC::NowUTC();
      inmsgs.push_back( msg );
    }
  }
}

void ServSession::parseBuffer( void **parse_buf, unsigned int &parse_len )
{
  void *p = *parse_buf;
  ProgTrace( TRACE5, "ServSession::parseBuffer: parse_len=%d, HEADER_SIZE=%d", parse_len, HEADER_SIZE );
  while ( parse_len >= HEADER_SIZE ) {
    int msg_len = ntohl(*(int*)p);
    ProgTrace( TRACE5, "ServSession::parseBuffer: msg_len=%d", msg_len );
    if ( msg_len + HEADER_SIZE > parse_len )
      return;
    ProgTrace( TRACE5, "msg end" );
    p = (void*)( (uintptr_t)p + sizeof(int));
    int msg_id = ntohl(*(int*)p);
    p = (void*)( (uintptr_t)p + sizeof(int));
    int msg_flags = ntohl(*(int*)p);
    p = (void*)( (uintptr_t)p + sizeof(int));
    std::string strbody( (char*)p, msg_len );
    PutMsg( msg_id, msg_flags, strbody );
    p = (void*)( (uintptr_t)p + msg_len);
    ProgTrace( TRACE5, "parseBuffer: msg_len=%d, msg_len=%" PRIdPTR, msg_len, (uintptr_t)p - (uintptr_t)(*parse_buf) );
    parse_len = parse_len - msg_len - HEADER_SIZE;
    ProgTrace( TRACE5, "parseBuffer: After parse_len=%d", parse_len );
  }
  if ( (uintptr_t)p != (uintptr_t)*parse_buf ) {
    memmove( *parse_buf, p, parse_len );
  }
  *parse_buf = realloc( *parse_buf, parse_len );
}

void ServSession::Read( void *recvbuf, int recvbuf_len )
{
  len += recvbuf_len;
  buf = realloc( buf, len );
  memcpy( (void*)((uintptr_t)buf + len - recvbuf_len ), recvbuf, recvbuf_len );
  parseBuffer( &buf, len );
}

string ServSession::createHeartBeatStr( )
{
  string res;
  xmlDocPtr doc = CreateXMLDoc( "UTF-8", STR_HEARTBEAT.c_str() );
  try {
    SetProp( doc->children, "time", BASIC::DateTimeToStr( BASIC::NowUTC() ) );
    res = XMLTreeToText( doc );
    res = ConvertCodepage( res, "CP866", MsgCodePage );
  }
  catch(...) {
    xmlFreeDoc( doc );
    throw;
  }
  xmlFreeDoc( doc );
  return res;
}

string createInvalidMsg( const string &MsgCodePage )
{
  string res;
  xmlDocPtr resDoc = CreateXMLDoc( "UTF-8", SESS_INVALID_MSG.c_str() );
  try {
    res = XMLTreeToText( resDoc );
    res = ConvertCodepage( res, "CP866", MsgCodePage );
  }
  catch(...) {
    xmlFreeDoc( resDoc );
    throw;
  }
  xmlFreeDoc( resDoc );
  return res;
}

struct TCobraError {
  int code;
  string msg;
  string flight_id;
};

string createCommitStr( const string &MsgCodePage, int code, const vector<TCobraError> &errors )
{
  string res;
  xmlDocPtr doc = CreateXMLDoc( "UTF-8", SESS_COMMIT_MSG.c_str() );
  try {
    for ( vector<TCobraError>::const_iterator i=errors.begin(); i!=errors.end(); i++ ) {
      xmlNodePtr errNode = NewTextChild( doc->children, "error" );
      if ( !i->flight_id.empty() )
        SetProp( errNode, "flight_id", i->flight_id );
      SetProp( errNode, "code", i->code );
      SetProp( errNode, "message", i->msg );
    }
    res = XMLTreeToText( doc );
    ProgTrace( TRACE5, "createCommitStr: res=%s", res.c_str() );
    res = ConvertCodepage( res, "CP866", MsgCodePage );
    ProgTrace( TRACE5, "createCommitStr: res=%s", res.c_str() );
  }
  catch(...) {
    xmlFreeDoc( doc );
    throw;
  }
  xmlFreeDoc( doc );
  return res;
}

//status=teClientRequest - запрос от клиента, status=teClientAsnwer - ответ от клиента, status=teServerRequest - надо делать запрос на клиент
/*void ServSession::Execute( const xmlDocPtr &reqDoc, xmlDocPtr &resDoc, TExecuteStatus status )
{
  switch ( status ) {
    case teClientRequest: // запрос от клиента
        if ( isHeartBeatXmlDoc( reqDoc ) ) {
          tst();
          HeartBeatOK();
          resDoc = createHeartBeatXML();
          return;
        }
        if ( reqDoc == NULL ) { // не смогли разобрать xml-документ запроса
          ProgError( STDLOG, "ServSession::Execute: mode=teClientRequest, reqDoc=NULL => invalid xml-format" );
          return;
        }
        DoProcessClientRequest( reqDoc, resDoc );
        break;
    case teClientAsnwer: // ответ от клиента
        if ( isHeartBeatXmlDoc( reqDoc ) ) {
          tst();
          HeartBeatOK();
          return;
        }
        if ( reqDoc == NULL ) { // не смогли разобрать xml-документ запроса
          ProgError( STDLOG, "ServSession::Execute: mode=teClientRequest, reqDoc=NULL => invalid xml-format" );
          return;
        }
        if ( resDoc == NULL ) { // не смогли разобрать xml-документ ответа
          ProgError( STDLOG, "ServSession::Execute: mode=teClientRequest, resDoc=NULL => invalid xml-format" );
          return;
        }
        DoProcessClientAnswer( reqDoc, resDoc );
        break;

    case teServerRequest:
        DoProcessServerRequest( resDoc );
        break;
  }
  if ( status != teServerRequest ) {
    ProgTrace( TRACE5, "reqDoc=%p, resDoc=%p, status=%d", reqDoc, resDoc, (int)status );
  }
  if ( status == teClientRequest ) { // запрос от клиента
    resDoc = reqDoc; //!!! временно
  }

} */

void ServSession::ProcessIncommingMsgs()
{
  //здесь можно обработать входящие сообщения
  if ( inmsgs.size() == 0 )
    return;
  std::vector<TAstraServMsg>::iterator imsg=inmsgs.begin();
  int icount = getInQueueCountMsg();
  while ( imsg!=inmsgs.end() && ( icount == -1 || icount > 0 ) ) {
    ProcessMsg( *imsg );
    imsg = inmsgs.erase( imsg );
    if ( icount > 0 )
      icount--;
  }
}

/*void ServSession::ExecuteQueueMsgs()
{
  //здесь можно разбирать ответы и готовить запросы
  for ( std::map<int,TServMsg>::iterator i=msgs.begin(); i!=msgs.end(); ) {
    if ( i->second.status == smDelete ) {
      ProgTrace( TRACE5, "ServSession::ExecuteQueueMsgs delete Sessid=%d, count=%d", i->first, 1 );
      msgs.erase( i++ );
      continue;
    }
    if ( i->second.status == smWaitAnswer && i->second.putTime + WAIT_ANSWER_MSEC/(24.0*60.0*60.0*1000) < BASIC::NowUTC() ) {
      //!!!timeout
      ProgTrace( TRACE5, "ServSession::ExecuteQueueMsgs timeout, SessId=%d", i->first );
      i->second.status = smDelete;
      ++i;
      continue;
    }
    if ( i->second.status != smCommit ) {
      ++i;
      continue;
    }
    ProgTrace( TRACE5, "i->second.status=%d, i->second.isClientRequest()=%d",i->second.status,i->second.isClientRequest() );
    TExecuteStatus status;
    if ( i->second.isClientRequest() )
      status = teClientRequest;
    else
      status = teClientAsnwer;
    try {
      Execute( i->second.receiveDoc, i->second.sendDoc, status );
      if ( status == teClientAsnwer ||
           status == teClientRequest && i->second.sendDoc == NULL ) // запрос от  клиента на который мы не даем ответ
        i->second.status = smDelete;
      else {
        i->second.status = smForSend;
        msgs[ FID ].setClientAsnwer(); // будет ответ
      }
    }
    catch(...) {
      ProgError( STDLOG, "ServSession::ExecuteQueueMsgs, SessId=%d, throw, status=%d", i->first, (int)status );
    }
    ++i;
  }
  // а теперь возможно надо самим послать запрос на клиент
  xmlDocPtr sendDoc = NULL;
  try {
    Execute( NULL, sendDoc, teServerRequest );
    if ( sendDoc != NULL ) {
      FID++;
      msgs[ FID ].msg_id = FID;
      msgs[ FID ].setClientRequest(); // будет запрос
      msgs[ FID ].receiveDoc = NULL;
      msgs[ FID ].sendDoc = sendDoc;
      msgs[ FID ].pr_test = false;
      msgs[ FID ].status = smForSend;
      msgs[ FID ].putTime = BASIC::NowUTC();
    }
  }
  catch(...) {
    ProgError( STDLOG, "ServSession::ExecuteQueueMsgs, throw, status=teServerRequest" );
  }
} */

void ServSession::Write( int msg_id, void *sendbuf, int sendbuf_len )
{
  std::vector<TAstraServMsg>::iterator imsg=outmsgs.begin();
  for ( ; imsg!=outmsgs.end(); imsg++ ) {
    if ( imsg->msg_id == msg_id )
      break;
  }
  if ( imsg == outmsgs.end() ) {
    ProgError( STDLOG, "ServSession::Write, msg_id=%d, deleted", msg_id );
    return;
  }
  int *p = (int*)sendbuf;
  *p = htonl(imsg->strbody.size());
  p++;
  *p = htonl(imsg->msg_id);
  p++;
  *p = htonl(imsg->flags);
  ProgTrace( TRACE5, "ServSession::Write msg_id=%d, flags=%d, strbody=%s", imsg->msg_id, imsg->flags, imsg->strbody.c_str() );
  memcpy( (void*)((uintptr_t)sendbuf + HEADER_SIZE), (void*)imsg->strbody.c_str(), imsg->strbody.size() );
  outmsgs.erase( imsg );
}


int ServSession::WriteSize( int &msg_id )
{
  for ( std::vector<TAstraServMsg>::iterator imsg=outmsgs.begin(); imsg!=outmsgs.end(); imsg++ ) {
    if ( imsg->processTime <= 0 ) {
      ProgTrace( TRACE5, "ServSession::WriteSize msg_id=%d, flags=%d", imsg->msg_id, imsg->flags );
      imsg->processTime = BASIC::NowUTC();
      msg_id = imsg->msg_id;
      return HEADER_SIZE + imsg->strbody.size();
    }
  }
  return 0;
}

/*
  smDelete - работа закончена с сообщением
  smWaitAnswer - ждем ответа от клиента
  smCommit - сообщение готово к обработке (есть входящее)
  smForSend - сообщение готово к отправке
*/

/*int ServSession::WriteSize( int &SessId )
{
  for ( std::map<int,TServMsg>::iterator i=msgs.begin(); i!=msgs.end(); i++ ) {
    bool pr_test = ( i->second.isClientRequest() && i->second.pr_test );
    if (  !pr_test &&
          ( i->second.status != smForSend ||
            i->second.sendDoc == NULL ) )
      continue;
    SessId = i->first;
    if ( !i->second.isClientRequest() || i->second.pr_test ) // ответ клиенту
      i->second.status = smDelete;
    else // запрос клиенту
      i->second.status = smWaitAnswer;
    if ( i->second.pr_test )
      return HEADER_SIZE + string("<_connection_test_>").size();
    else
      return HEADER_SIZE + XMLTreeToText( i->second.sendDoc ).size();
  }
  return 0;
}*/

void ServSession::HeartBeat()
{
  FID++;
  TAstraServMsg msg;
  msg.msg_id = FID;
  msg.strbody = createHeartBeatStr();
  msg.setClientRequest(); // будет запрос
  if ( outmsgs.empty() )
    outmsgs.push_back( msg );
  else
    outmsgs.insert( outmsgs.begin(), msg );
}

void ServSession::Exception()
{
}

/* flags:
  $01000000 - шифрование
  $02000000 - текст CP866, иначе xml UTF-8
  $04000000 - ZIP
  $80000000 - запрос
*/


void ServSession::SendAnswer( int msg_id, const std::string &strbody, bool pr_text, bool pr_zip )
{
  TAstraServMsg msg;
  msg.msg_id = msg_id;
  msg.setClientAnswer();
  if ( pr_text )
    msg.flags = (msg.flags | 0x02000000);
  if ( pr_zip )
    msg.flags = (msg.flags | 0x04000000);
  msg.strbody = strbody;
  outmsgs.push_back( msg );
  ProgTrace( TRACE5, "ServSession::SendAnswer msg_id=%d", msg.msg_id );
}

void ServSession::SendRequest( const std::string &strbody, bool pr_text, bool pr_zip )
{
  FID++;
  TAstraServMsg msg;
  msg.msg_id = FID;
  msg.setClientRequest();
  if ( pr_text )
    msg.flags = (msg.flags | 0x02000000);
  if ( pr_zip )
    msg.flags = (msg.flags | 0x04000000);
  outmsgs.push_back( msg );
  ProgTrace( TRACE5, "ServSession::SendRequest msg_id=%d", msg.msg_id );
}

/////////////////////////COBRA////////////////////////////////////////////

bool CobraSession::AcceptConnect( sockaddr_in &addr_out )
{
  /* таблица для определения пульта по адресу, если не нашли, то не принимаем соединение */
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT desk FROM tcp_clients "
    " WHERE client_type=:client_type AND addr=:addr AND pr_active IS NULL AND rownum<2";
  Qry.CreateVariable( "client_type", otString, client_type );
  Qry.CreateVariable( "addr", otString, inet_ntoa( addr_out.sin_addr ) );
  Qry.Execute();
  if ( Qry.Eof ) {
    ProgTrace( TRACE5, "NOT AcceptConnect! client_type=%s, addr=%s",  client_type.c_str(), inet_ntoa( addr_out.sin_addr ) );
    return false;
  }
  desk = Qry.FieldAsString( "desk" );
  return true;
}


void CobraSession::getOutCommingMsgs()
{
  //определяем сообщение, которое необходимо отправить
  //надо учесть:
  //1. Признак того, что сообщения посылаются по-порядку tcp_msg_types.in_order
  //2. Самое старое сообщение
  //3. Перепослать сообщение, для которого не было ответа или возникла ошибка (доставки=tcp_msg_types.in_order=0, любая ошибка tcp_msg_types.in_order=1)
  if ( !Empty_OutCommingQueue() ) { // есть сообщения в очереди, которые не отправлены
    tst();
    return;
  }
  TQuery ParamsQry(&OraSession);
  ParamsQry.SQLText =
    "SELECT name,value FROM file_params WHERE id=:id";
  ParamsQry.DeclareVariable( "id", otInteger );

  TQuery Qry(&OraSession);
  Qry.SQLText =
		"SELECT file_queue.id,"
		"       file_queue.status,file_queue.time,files.time as wait_time,system.UTCSYSDATE AS now, "
		"       files.data, NVL(tcp_msg_types.in_order,0) in_order "
		" FROM file_queue,files,tcp_msg_types "
		" WHERE file_queue.id=files.id AND "
    "       file_queue.sender=:sender AND "
    "       file_queue.receiver=:receiver AND "
    "       file_queue.type=tcp_msg_types.code AND "
    "       tcp_msg_types.code=:msg_out_type "
    " ORDER BY DECODE(in_order,1,files.time,file_queue.time),file_queue.id";
	Qry.CreateVariable( "sender", otString, OWN_POINT_ADDR() );
	Qry.CreateVariable( "receiver", otString, desk );
	Qry.CreateVariable( "msg_out_type", otString, msg_out_type );
	Qry.Execute();
	map<string,string> fileparams;
	string str_body;
	char *p = NULL;
	try {
	  for ( int icount=0; !Qry.Eof && icount<=getOutQueueCountMsg(); Qry.Next() ) {
	    if ( string(Qry.FieldAsString( "status" )) != "PUT" &&
           Qry.FieldAsInteger( "in_order" ) &&
           Qry.FieldAsDateTime( "time" ) + WAIT_ANSWER_SEC/(60.0*60.0*24.0) > Qry.FieldAsDateTime( "now" ) ) {
        tst();
        break; // важен порядок и время ответа еще не вышло
      }
      tst();
      int file_id = Qry.FieldAsInteger( "id" );
      ProgTrace( TRACE5, "CobraSession::getOutCommingMsgs(): file_id=%d", file_id );
      int len = Qry.GetSizeLongField( "data" );
      if ( p )
     		p = (char*)realloc( p, len );
     	else
     	  p = (char*)malloc( len );
     	if ( !p )
     		throw EXCEPTIONS::Exception( string( "Can't malloc " ) + IntToString( len ) + " byte" );
      tst();
      Qry.FieldAsLong( "data", p );
      tst();
      string strbody( p, len );
      ProgTrace( TRACE5, "CobraSession::getOutCommingMsgs(): strbody=%s, len=%d", strbody.c_str(), len );
      if ( string(Qry.FieldAsString( "status" )) != "PUT" )
        ProgError( STDLOG, "CobraSession::getOutCommingMsgs: file_id=%d, status=%s, file_queue.time=%s, files.time=%s, now=%s",
                   file_id, Qry.FieldAsString( "status" ),
                   DateTimeToStr( Qry.FieldAsDateTime( "time" ), ServerFormatDateTimeAsString ).c_str(),
                   DateTimeToStr( Qry.FieldAsDateTime( "wait_time" ), ServerFormatDateTimeAsString ).c_str(),
                   DateTimeToStr( Qry.FieldAsDateTime( "now" ), ServerFormatDateTimeAsString ).c_str() );
      ParamsQry.SetVariable( "id", file_id );
      ParamsQry.Execute();
      fileparams.clear();
      while ( !ParamsQry.Eof ) {
        fileparams[ ParamsQry.FieldAsString( "name" ) ] = ParamsQry.FieldAsString( "value" );
        ParamsQry.Next();
      }
      int msg_param;
      if ( StrToInt( fileparams[ SESS_MSG_FLAGS ].c_str(), msg_param ) == EOF ) {
        ProgError( STDLOG, "CobraSession::getOutCommingMsgs: param %s not found, file_id=%d",
                   SESS_MSG_FLAGS.c_str(), file_id );
        TFileQueue::deleteFile( file_id );
      }
      else {
        if ( TAstraServMsg::isClientRequest( msg_param ) ) {
          SendRequest( strbody, false, false );
          TFileQueue::sendFile( file_id );
        }
        else {
          if ( StrToInt( fileparams[ SESS_MSG_ID ].c_str(), msg_param ) == EOF )
            ProgError( STDLOG, "CobraSession::getOutCommingMsgs: param %s not found, file_id=%d",
                       SESS_MSG_ID.c_str(), file_id );
          else
            SendAnswer( msg_param, strbody, false, false );
          TFileQueue::deleteFile( file_id );
        }
      }
    }
  }
  catch(...) {
    if ( p )
      free( p );
    throw;
  }
  if ( p )
    free( p );
}

int getFileIdFromMsgId( int msg_id, const string &receiver, const string &canon_name )
{
  int res = NoExists;
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT file_queue.id id, file_params.value value "
		" FROM file_queue, file_params "
		" WHERE sender=:sender AND "
    "       receiver=:receiver AND "
    "       type=:msg_type AND "
    "       status=:status AND "
    "       file_queue.id=file_params.id AND "
    "       file_params.name=:param_name ";
	Qry.CreateVariable( "sender", otString, OWN_POINT_ADDR() );
	Qry.CreateVariable( "receiver", otString, receiver );
	Qry.CreateVariable( "msg_type", otString, canon_name );
	Qry.CreateVariable( "status", otString, "SEND" );
	Qry.CreateVariable( "param_name", otString, SESS_MSG_ID );
	Qry.Execute();
	string str_msg_id = IntToString( msg_id );
  while ( !Qry.Eof ) {
    if ( str_msg_id == string(Qry.FieldAsString( "value" )) ) {
      res = Qry.FieldAsInteger( "id" );
      break;
    }
    Qry.Next();
  }
  if ( res == NoExists ) {
    ProgError( STDLOG, "getFileIdFromMsgId: msg_id=%d not found in file_queue", msg_id );
  }
  return res;
}

int CobraSession::getInQueueCountMsg()
{
  return COBRA_INQUEUE_COUNT_MSGS;
}

int CobraSession::getOutQueueCountMsg()
{
  return COBRA_OUTQUEUE_COUNT_MSGS;
}

bool CobraSession::isInvalidAnswer( const xmlDocPtr &doc )
{
  return ( doc == NULL || doc->children == NULL || GetNode( SESS_COMMIT_MSG.c_str(), doc->children ) == NULL );
}

void CobraSession::ProcessMsg( const TAstraServMsg &msg )
{
  ProgTrace( TRACE5, "CobraSession::ProcessMsg, incomming msg_id=%d, client request=%d, strbody=%s",
             msg.msg_id, TAstraServMsg::isClientRequest(msg.flags), msg.strbody.c_str() );
/*  if ( TAstraServMsg::isClientRequest(msg.flags) ) { //!!!заглушка
    SendAnswer( msg.msg_id, msg.strbody, (msg.flags & 0x02000000) == 0x02000000, (msg.flags & 0x04000000) == 0x04000000 );
    return;
  }*/
  isHook = false;
  try {
    xmlDocPtr doc = NULL;
    try {
      tst();
      xmlDocPtr doc = TextToXMLTree( msg.strbody );
      tst();
	    if ( doc ) {
        ProgTrace( TRACE5, "CobraSession::ProcessMsg: TextToXMLTree is good" );
        //xmlFree(const_cast<xmlChar *>(doc->encoding));
        //doc->encoding = 0;
//        strbody = XMLTreeToText( doc );
//        ProgTrace( TRACE5, "CobraSession::ProcessMsg, after xml_decode_nodelist, strbody=%s",  strbody.c_str() );
		  }
		  tst();
      if ( TAstraServMsg::isClientRequest(msg.flags) ) {
        if ( doc == NULL || doc->children == NULL ) {
          ProgError( STDLOG, "CobraSession::ProcessMsg: REQUEST INVALID: msg_id=%d, isClientRequest=%d, msg.strbody=%s",
                     msg.msg_id, TAstraServMsg::isClientRequest(msg.flags), msg.strbody.c_str() );
          SendAnswer( msg.msg_id, createInvalidMsg(MsgCodePage), false, false );
        }
        else { //пришел запрос
          map<string,string> params;
          params[ SESS_MSG_ID ] = IntToString( msg.msg_id );
          params[ SESS_MSG_FLAGS ] = IntToString( msg.flags );
          int fid = TFileQueue::putFile( desk, OWN_POINT_ADDR(), msg_in_type, params, msg.strbody );
          ProgTrace( TRACE5, "CobraSession::ProcessMsg putfile file_id=%d", fid );
          isHook = true;
        }
      }
      else { //ответ
        int file_id = getFileIdFromMsgId( msg.msg_id, desk, msg_out_type );
        ToEvents( COBRA_CLIENT_TYPE, file_id, NoExists );
        if ( isInvalidAnswer( doc ) ) {
          ProgError( STDLOG, "CobraSession::ProcessMsg: ANSWER INVALID: msg_id=%d, isClientRequest=%d, msg.strbody=%s, file_id=%d",
                     msg.msg_id, TAstraServMsg::isClientRequest(msg.flags), msg.strbody.c_str(), file_id );
          if ( file_id != NoExists ) {
            ProgError( STDLOG, "Ошибка формата сообщения: file_id=%d, msg_type=%s, desk=%s, msgbody=%s",
                       file_id, msg_out_type.c_str(), desk.c_str(), msg.strbody.c_str() );
          }
        }
        else { // пришло подтверждение обработки запроса
          if ( file_id != NoExists ) {
            ProgTrace( TRACE5, "CobraSession::ProcessMsg: ANSWER COMMIT: msg_id=%d, isClientRequest=%d, msg.strbody=%s, file_id=%d",
                       msg.msg_id, TAstraServMsg::isClientRequest(msg.flags), msg.strbody.c_str(), file_id );
            xmlNodePtr errNode = GetNode( "error", doc->children );
            if ( errNode != NULL ) {
              string msg_tolog = "Ошибка в сообщении: msg_id="+IntToString(msg.msg_id)+",code=";
              xmlNodePtr pNode = GetNode( "@code", doc->children );
              if ( pNode != NULL )
                msg_tolog += NodeAsString( pNode );
              msg_tolog += ",message=";
              pNode = GetNode( "@message", doc->children );
              if ( pNode != NULL )
                msg_tolog += NodeAsString( pNode );
              ProgError( STDLOG, "%s", msg_tolog.c_str() );
            }
            TFileQueue::deleteFile( file_id ); //???
          }
        }
      }
    }
    catch(...) {
      xmlFreeDoc( doc );
      throw;
    }
    xmlFreeDoc( doc );
  }
  catch(EOracleError &E)
  {
    ProgError( STDLOG, "EOracleError %d: %s", E.Code, E.what());
    ProgError( STDLOG, "SQL: %s", E.SQLText());
    throw;
  }
  catch(std::exception &E)
  {
    ProgError( STDLOG, "std::exception: %s", E.what() );
    throw;
  }
  catch(...)
  {
    ProgError(STDLOG, "Unknown exception");
    throw;
  };
}

void CobraSession::DoHook( )
{
   if ( isHook )
     sendCmd( cmd_hook.c_str(), "P" );
   isHook = false;
}

enum TCobraAction { caInsert, caUpdate, caDelete, caUnknown };

/*
 Есть несколько систем, откуда может приходить рейс на вставку, обновление, удаление (маршрут):
 Если рейс пришел из одной системы, то удаление рейса, изменение маршрута допускается только в этой системе
 (Астра, Сочи, Краснодар)
 Для определения системы, откуда завели рейс необходимо прописать в поле cobra_points.airp:
 СОЧ - из Кобры СОЧ
 КПА - из Кобры КПА
 Если рейс найден в СПП и не найден в таблице - нельзя редактировать маршрут
*/

void ParseFlights( const xmlNodePtr reqNode, vector<TCobraError> &errors )
{
  //разбор входящего сообщения
  string strtree = XMLTreeToText( reqNode->doc );
  ProgTrace( TRACE5, "strtree=%s", strtree.c_str() );
  TQuery Qry(&OraSession);
  Qry.SQLText =
    "SELECT move_id FROM cobra_points WHERE flight_id=:flight_id AND airp=:airp";
  Qry.DeclareVariable( "flight_id", otString );
  Qry.DeclareVariable( "airp", otString );
  
  /*Qry.SQLText =
    "SELECT points.move_id,points.point_id,points.airp,points.point_num, "
    "       points.airline,points.flt_no,points.suffix,points.scd_out,points.scd_in "
    " FROM points,cobra_points "
    " WHERE flight_id=:flight_id AND points.move_id=cobra_points.move_id AND "
    "       pr_del!=-1 "
    "ORDER BY point_num";
  Qry.DeclareVariable( "flight_id", otString );*/
  TQuery UQry(&OraSession);
  UQry.SQLText =
    "BEGIN "
    " IF :what_do = 1 THEN "
    "  UPDATE cobra_points SET move_id=:move_id WHERE flight_id=:flight_id AND airp=:airp; "
    "  IF SQL%NOTFOUND THEN "
    "   INSERT INTO cobra_points(move_id,flight_id,airp) VALUES(:move_id,:flight_id,:airp);"
    "  END IF; "
    " ELSE "
    "  DELETE cobra_points WHERE flight_id=:flight_id AND airp=:airp; "
    " END IF; "
    "END; ";
  UQry.DeclareVariable( "move_id", otInteger );
  UQry.DeclareVariable( "flight_id", otString );
  UQry.DeclareVariable( "airp", otString );
  UQry.DeclareVariable( "what_do", otInteger );

  string local_airp, temp_airp;
  if ( GetNode( "@airp", reqNode ) == NULL )
    local_airp = "СОЧ";
  else
    local_airp = NodeAsString( "@airp", reqNode );  TElemFmt fmt;
  temp_airp = ElemToElemId( etAirp, local_airp, fmt, false );
  if ( fmt == efmtUnknown )
    throw EXCEPTIONS::Exception( "Invalid airp '%s' of 'flights' node", local_airp.c_str() );
  local_airp = temp_airp;
  TReqInfo::Instance()->desk.code = string("КОБРА") + local_airp; //!!! удалить
  Qry.SetVariable( "airp", local_airp );
  UQry.SetVariable( "airp", local_airp );
  TQuery TermQry(&OraSession);
  TermQry.SQLText =
    "SELECT desk FROM stations WHERE airp=:airp AND work_mode=:work_mode AND name=:name";
  TermQry.CreateVariable( "airp", otString, local_airp );
  TermQry.DeclareVariable( "work_mode", otString );
  TermQry.DeclareVariable( "name", otString );
  string flight_id;
  errors.clear();
  string invalid_desks, invalid_gates;
  xmlNodePtr nodeFlight = reqNode->children;
  try {
    if ( nodeFlight == NULL )
      throw EXCEPTIONS::Exception( "Node 'flight' not found" );
    string tmp, tmp1;
    while ( nodeFlight ) {
      invalid_desks.clear();
      invalid_gates.clear();
      //vector<TPointKey> airps;
      flight_id.clear();
      TCobraAction flight_action;
      string key_airline;
      int key_flt_no;
      string key_suffix;
      TElemFmt key_suffix_fmt;
      TDateTime key_scd_out;
      string flight_airline;
      TElemFmt flight_airline_fmt;
      int flight_flt_no;
      string flight_suffix;
      TElemFmt flight_suffix_fmt;
      string flight_trip_type;
      string flight_litera;
      int flight_pr_cancel;
      bool flight_pr_cobra;
      string flight_key;

      try {
        xmlNodePtr n = GetNode( "@action", nodeFlight );
        if ( n == NULL )
          throw EXCEPTIONS::Exception( "Node 'action' not found" );
        string action = NodeAsString( n );
        if ( action == "insert" )
          flight_action = caInsert;
        else
          if ( action == "update" )
            flight_action = caUpdate;
          else
            if ( action == "delete" )
              flight_action = caDelete;
        if ( flight_action == caUnknown )
          throw EXCEPTIONS::Exception( "Invalid value of 'action' node" );
        n = GetNode( "flight_id", nodeFlight );
        if ( n == NULL )
          throw EXCEPTIONS::Exception( "Node 'flight_id' not found" );
        flight_id = NodeAsString( n );
        std::string::size_type i = flight_id.find( "/" );
        ProgTrace( TRACE5, "i=%zu, flight_id=%s", i, flight_id.c_str() );
        if ( i == string::npos )
          throw EXCEPTIONS::Exception( "Invalid value '%s' of 'flight_id' node", flight_id.c_str() );
        try {
          parseFlt( flight_id.substr( 0, i ), key_airline, key_flt_no, key_suffix );
        }
        catch( EXCEPTIONS::Exception &E ) {
          throw EXCEPTIONS::Exception( "Invalid value '%s' of 'flight_id' node", flight_id.c_str() );
        }
        tmp = flight_id;
        tmp.erase( 0, i + 1 );
        ProgTrace( TRACE5, "flight_id=%s, date=%s", flight_id.c_str(), tmp.c_str() );
        if ( BASIC::StrToDateTime( tmp.c_str(), "ddmmyy", key_scd_out ) == EOF )
          throw EXCEPTIONS::Exception( "Invalid value '%s' of 'flight_id' node", flight_id.c_str() );
        ProgTrace( TRACE5, "key_airline=%s, key_flt_no=%d, key_suffix=%s, key_scd_out=%s",
                   key_airline.c_str(), key_flt_no, key_suffix.c_str(), DateTimeToStr( key_scd_out, "dd.mm.yy" ).c_str() );
                   
        tmp = ElemToElemId( etAirline, key_airline, fmt, false );
        if ( fmt == efmtUnknown )
          throw EXCEPTIONS::Exception( "Invalid airline '%s' of 'flight_id' node", flight_id.c_str() );
        key_airline = tmp;
      	if ( !key_suffix.empty() ) {
          tmp = ElemToElemId( etSuffix, key_suffix, key_suffix_fmt, false );
          if ( key_suffix_fmt == efmtUnknown )
            throw EXCEPTIONS::Exception( "Invalid suffix '%s' of 'flight_id' node", flight_id.c_str() );
          key_suffix = tmp;
        }
        n = GetNode( "airline", nodeFlight );
        if ( n == NULL )
          throw EXCEPTIONS::Exception( "Node 'airline' not found" );
        flight_airline = NodeAsString( n );
        tmp = ElemToElemId( etAirline, flight_airline, flight_airline_fmt, false );
        if ( flight_airline_fmt == efmtUnknown )
          throw EXCEPTIONS::Exception( "Invalid value '%s' of 'airline' node", flight_airline.c_str() );
        flight_airline = tmp;
        n = GetNode( "flt_no", nodeFlight );
        if ( n == NULL )
          throw EXCEPTIONS::Exception( "Node 'flt_no' not found" );
        tmp = flight_airline + NodeAsString( n );
        ProgTrace( TRACE5, "trip=%s", tmp.c_str() );
        try {
          parseFlt( tmp, tmp1, flight_flt_no, flight_suffix );
        }
        catch( EXCEPTIONS::Exception &E ) {
          throw EXCEPTIONS::Exception( "Invalid value '%s' of 'flt_no' node", NodeAsString( n ) );
        }
        if ( !flight_suffix.empty() ) {
          tmp = ElemToElemId( etSuffix, flight_suffix, flight_suffix_fmt, false );
          if ( flight_suffix_fmt == efmtUnknown )
            throw EXCEPTIONS::Exception( "Invalid value '%s' of 'flt_no' node", NodeAsString( n ) );
          flight_suffix = tmp;
        }
        n = GetNode( "trip_type", nodeFlight );
        if ( n == NULL )
          throw EXCEPTIONS::Exception( "Node 'trip_type' not found" );
        flight_trip_type = NodeAsString( n );
        tmp = ElemToElemId( etTripType, flight_trip_type, fmt, false );
        if ( fmt == efmtUnknown )
          throw EXCEPTIONS::Exception( "Invalid value '%s' of 'trip_type' node", flight_trip_type.c_str() );
        flight_trip_type = tmp;
        n = GetNode( "litera", nodeFlight );
        if ( n != NULL )
          flight_litera = NodeAsString( n );
        if ( !flight_litera.empty() ) {
          tmp = ElemToElemId( etTripLiter, flight_litera, fmt, false );
          if ( fmt == efmtUnknown )
            throw EXCEPTIONS::Exception( "Invalid value '%s' of 'litera' node", flight_litera.c_str() );
          flight_litera = tmp;
        }
        if ( flight_litera.size() > 3 )
          throw EXCEPTIONS::Exception( "Invalid value '%s' of 'litera' node", flight_litera.c_str() );
        n = GetNode( "canceled", nodeFlight );
        if ( n == NULL )
          throw EXCEPTIONS::Exception( "Node 'canceled' not found" );
        try {
          flight_pr_cancel = NodeAsInteger( n );
        }
        catch( EXMLError ) {
          throw EXCEPTIONS::Exception( "Invalid value '%s' of 'canceled' node", NodeAsString( n ) );
        }
        //конец аттрибутов рейса
        //маршрут
        n = GetNode( "route", nodeFlight );
        if ( n == NULL )
          throw EXCEPTIONS::Exception( "Node 'route' not found" );
        n = n->children;
        int key=0;
        TPoints points;
        TPointDests new_dests;
        TPointDests &old_dests = points.dests;

        while ( n != NULL && string((char*)n->name) == "point" ) {
          TPointsDest dest;
/*          try {
            //dest.point_num = NodeAsInteger( "@num", n ); //???
            if ( dest.point_num < 0 || dest.point_num > 99 )
              throw EXMLError( "Invalid tag num" );
          }
          catch( EXMLError ) {
            throw EXCEPTIONS::Exception( "Invalid value '%s' of 'point/@num' node", NodeAsString( "@num", n ) );
          }*/
          try {
            dest.scd_in = NodeAsDateTime( "scd_in", n, NoExists );
          }
          catch( EXMLError ) {
            throw EXCEPTIONS::Exception( "Invalid value '%s' of 'point/scd_in' node", NodeAsString( "scd_in", n ) );
          }
          try {
            dest.est_in = NodeAsDateTime( "est_in", n, NoExists );
          }
          catch( EXMLError ) {
            throw EXCEPTIONS::Exception( "Invalid value '%s' of 'point/est_in' node", NodeAsString( "est_in", n ) );
          }
          try {
            dest.act_in = NodeAsDateTime( "act_in", n, NoExists );
          }
          catch( EXMLError ) {
            throw EXCEPTIONS::Exception( "Invalid value '%s' of 'point/act_in' node", NodeAsString( "act_in", n ) );
          }
          try {
            dest.scd_out = NodeAsDateTime( "scd_out", n, NoExists );
          }
          catch( EXMLError ) {
            throw EXCEPTIONS::Exception( "Invalid value '%s' of 'point/scd_out' node", NodeAsString( "scd_out", n ) );
          }
          try {
            dest.est_out = NodeAsDateTime( "est_out", n, NoExists );
          }
          catch( EXMLError ) {
            throw EXCEPTIONS::Exception( "Invalid value '%s' of 'point/est_out' node", NodeAsString( "est_out", n ) );
          }
          try {
            dest.act_out = NodeAsDateTime( "act_out", n, NoExists );
          }
          catch( EXMLError ) {
            throw EXCEPTIONS::Exception( "Invalid value '%s' of 'point/act_out' node", NodeAsString( "act_out", n ) );
          }
          dest.craft = NodeAsString( "craft", n, "" );
          if ( !dest.craft.empty() ) {
            tmp = ElemToElemId( etCraft, dest.craft, dest.craft_fmt, false );
            if ( dest.craft_fmt == efmtUnknown )
              throw EXCEPTIONS::Exception( "Invalid value '%s' of 'points/point/craft' node", dest.craft.c_str() );
            dest.craft = tmp;
          }
          dest.bort = NodeAsString( "bort", n, "" );
          if ( dest.bort.size() > 10 )
            throw EXCEPTIONS::Exception( "Invalid value '%s' of 'points/point/bort' node", dest.bort.c_str() );
          xmlNodePtr n2 = GetNode( "airp", n );
          if ( n2 == NULL )
            throw EXCEPTIONS::Exception( "Node 'point/arp' not found" );
          dest.airp = NodeAsString( n2 );
          tmp = ElemToElemId( etAirp, dest.airp, dest.airp_fmt, false );
          if ( dest.airp_fmt == efmtUnknown )
            throw EXCEPTIONS::Exception( "Invalid value '%s' of 'point/airp' node", dest.airp.c_str() );
          dest.airp = tmp;
          ProgTrace( TRACE5, "dest.airp=%s", dest.airp.c_str() );
          //!!!!
          dest.airline = flight_airline;
          dest.airline_fmt = flight_airline_fmt;
          dest.flt_no = flight_flt_no;
          dest.suffix = flight_suffix;
          dest.suffix_fmt = flight_suffix_fmt;
          dest.trip_type = flight_trip_type;
          dest.litera = flight_litera;
          dest.point_id = NoExists;
          if ( new_dests.items.empty() ) {
            dest.scd_in = NoExists;
            dest.est_in = NoExists;
            dest.act_in = NoExists;
          }
          if ( n->next == NULL ) {
            dest.airline.clear();
            dest.airline_fmt = efmtUnknown;
            dest.flt_no = NoExists;
            dest.suffix.clear();
            dest.suffix_fmt = efmtUnknown;
            dest.scd_out = NoExists;
            dest.est_out = NoExists;
            dest.act_out = NoExists;
          }
          ProgTrace( TRACE5, "point: scd_in=%f, scd_out=%f, airp=%s, craft=%s, bort=%s",
                     dest.scd_in, dest.scd_out, dest.airp.c_str(), dest.craft.c_str(), dest.bort.c_str() );
          dest.key = IntToString( key );
          key++;
          if ( dest.airline == flight_airline &&
               dest.flt_no == flight_flt_no &&
               dest.suffix == flight_suffix &&
               dest.airp == local_airp ) { //наш пункт посадки
            ProgTrace( TRACE5, "own dest" );
            Qry.SetVariable( "flight_id", flight_id );
            Qry.Execute();
            flight_pr_cobra = !Qry.Eof; //определение рейса признака собственности Кобры
            if ( flight_pr_cobra ) {
              points.move_id = Qry.FieldAsInteger( "move_id" );
            }
            ProgTrace( TRACE5, "own dest, flight_pr_cobra=%d, points.move_id=%d", flight_pr_cobra, points.move_id );
            int findMove_id, findPoint_id;
            if ( TPoints::isDouble( NoExists, dest, findMove_id, findPoint_id ) ) {
              ProgTrace( TRACE5, "TPoints::isDouble, findMove_id=%d", findMove_id );
              if ( flight_pr_cobra && points.move_id != findMove_id )
                throw EXCEPTIONS::Exception( "flight already exists" );
              points.move_id = findMove_id;
              old_dests.Load( points.move_id ); //зачитаем старый маршрут
              ProgTrace( TRACE5, "old_dests.items.size()=%zu", old_dests.items.size() );
            }
            else {
              if ( flight_action != caInsert )
                throw EXCEPTIONS::Exception( "Flight not found, but action!=insert" );
              points.move_id = NoExists;
              flight_pr_cobra = true;
            }
            flight_key = dest.key;
          }
          new_dests.items.push_back( dest );
          n = n->next;
        } //конец маршрут
        ProgTrace( TRACE5, "flight_key=%s", flight_key.c_str() );
        if ( flight_key.empty() )
          throw EXCEPTIONS::Exception( "airp %s not in routes", local_airp.c_str() );
        tst();
        old_dests.sychDests( new_dests, flight_pr_cobra, true );
        tst();
        //int point_num = 0;
        vector<TPointsDest>::iterator own_dest=old_dests.items.end();
        for ( vector<TPointsDest>::iterator i=old_dests.items.begin(); i!=old_dests.items.end(); i++ ) {
          ProgTrace( TRACE5, "pr_del=%d, i->point_id=%d, i->point_num=%d, i->flt_no=%d, i->airp=%s, i->key=%s, flight_pr_cobra=%d",
                     i->pr_del, i->point_id, i->point_num, i->flt_no, i->airp.c_str(), i->key.c_str(), flight_pr_cobra );
          if ( i->pr_del == -1 ) {
            continue;
          }
          i->UseData.clearFlags();
          if ( flight_pr_cobra && flight_action == caDelete )
            i->pr_del = -1;
          else
            i->pr_del = flight_pr_cancel;
          for ( vector<TPointsDest>::iterator j=new_dests.items.begin(); j!=new_dests.items.end(); j++ ) {
            ProgTrace( TRACE5, "i->point_id=%d, i->key=%s, i->point_num=%d, j->point_id=%d, j->key=%s",
                       i->point_id, i->key.c_str(), i->point_num, j->point_id, j->key.c_str() );
            if ( (i->point_id != NoExists && i->point_id == j->point_id) ||
                 (i->point_id == NoExists && i->key == j->key) ) {
              //i->point_num = point_num;
              i->key = j->key;
              i->scd_in = j->scd_in;
              i->est_in = j->est_in;
              i->act_in = j->act_in;
              i->airline = j->airline;
              i->airline_fmt = j->airline_fmt;
              i->flt_no = j->flt_no;
              i->suffix = j->suffix;
              i->suffix_fmt = j->suffix_fmt;
              i->craft = j->craft;
              i->craft_fmt = j->craft_fmt;
              i->bort = j->bort;
              i->airp = j->airp;
              i->airp_fmt = j->airp_fmt;
              i->scd_out = j->scd_out;
              i->est_out = j->est_out;
              i->act_out = j->act_out;
              i->key = j->key;
              if ( i->key == flight_key ) {
                own_dest = i;
                own_dest->UseData.setFlag( udNoCalcESTTimeStage );
                own_dest->UseData.setFlag( udCargo );
                own_dest->UseData.setFlag( udMaxCommerce );
                own_dest->UseData.setFlag( udStations );
                own_dest->UseData.setFlag( udStages );
                ProgTrace( TRACE5, "own_dest->point_id=%d, key=%s", own_dest->point_id, own_dest->key.c_str() );
              }
              break;
            }
          }
          //point_num++;
        }
        tst();
        ////////////////////////////////////////
        ////////////////////////////////////////
        n = GetNode( "load", nodeFlight );
        if ( n == NULL )
          throw EXCEPTIONS::Exception( "Node 'load' not found" );
        xmlNodePtr n1 = GetNode( "max_commerce", n );
        if ( n1 == NULL )
          own_dest->max_commerce.SetValue( NoExists );
        else {
          int max_commerce;
          try {
            max_commerce = NodeAsInteger( n1 );
          }
          catch( EXMLError ) {
            throw EXCEPTIONS::Exception( "Invalid value '%s' of 'max_commerce' node", NodeAsString( n1 ) );
          }
          ProgTrace( TRACE5, "max_commerce=%d", max_commerce );
          if ( max_commerce < 0 || max_commerce > 999999 ) {
            throw EXCEPTIONS::Exception( "Invalid value '%s' of 'max_commerce' node", NodeAsString( n1 ) );
          }
          tst();
          own_dest->max_commerce.SetValue( max_commerce );
          tst();
        }
        n1 = GetNode( "points", n );
        if ( n1 == NULL )
          throw EXCEPTIONS::Exception( "Node 'points' not found" );
        n1 = n1->children;
        while ( n1 != NULL && string((char*)n1->name) == "point" ) {
          TPointsDestCargo cargo;
          xmlNodePtr n2 = GetNode( "airp_arv", n1 );
          if ( n2 == NULL )
            throw EXCEPTIONS::Exception( "Node 'airp_arv' not found" );
          cargo.airp_arv = NodeAsString( n2 );
          tmp = ElemToElemId( etAirp, cargo.airp_arv, fmt, false );
          if ( fmt == efmtUnknown )
            throw EXCEPTIONS::Exception( "Invalid value '%s' of 'load/points/point/airp_arv' node", cargo.airp_arv.c_str() );
          cargo.airp_arv = tmp;
          cargo.airp_arv_fmt = fmt;
          n2 = GetNode( "cargo", n1 );
          if ( n2 == NULL )
            cargo.cargo = NoExists;
          else {
            try {
              cargo.cargo = NodeAsInteger( n2 );
              if ( cargo.cargo < 0 || cargo.cargo > 999999999 )
                 throw EXMLError( "" );
            }
            catch( EXMLError ) {
              throw EXCEPTIONS::Exception( "Invalid value '%s' of 'cargo' node", NodeAsString( "cargo", n2  ) );
            }
          }
          n2 = GetNode( "mail", n1 );
          if ( n2 == NULL )
            cargo.mail = NoExists;
          else {
            try {
              cargo.mail = NodeAsInteger( n2 );
              if ( cargo.mail < 0 || cargo.mail > 999999999 )
                throw EXMLError( "" );
            }
            catch( EXMLError ) {
              throw EXCEPTIONS::Exception( "Invalid value '%s' of 'mail' node", NodeAsString( "mail", n2  ) );
            }
          }
          ProgTrace( TRACE5, "load: airp_arv=%s, cargo=%d, mail=%d", cargo.airp_arv.c_str(), cargo.cargo, cargo.mail );
          //проверка соответствия пунктов посадки по маршруту и загрузки по маршруту
          for ( vector<TPointsDest>::iterator ipoint=own_dest+1; ipoint!=old_dests.items.end(); ipoint++ ) {
            ProgTrace( TRACE5, "own_dest == old_dests.items.end()=%d", own_dest == old_dests.items.end() );
            ProgTrace( TRACE5, "own_dest + 1 == old_dests.items.end()=%d", own_dest + 1 == old_dests.items.end() );
            ProgTrace( TRACE5, "ipoint->airp=%s, cargo.airp_arv=%s", ipoint->airp.c_str(), cargo.airp_arv.c_str() );
            if ( ipoint->airp == cargo.airp_arv ) {
              ProgTrace( TRACE5, "ipoint->key=%s", ipoint->key.c_str() );
              cargo.key = ipoint->key;
              break;
            }
          }
          tst();
          if ( cargo.key.empty() )
            throw EXCEPTIONS::Exception( "cargo airp %s is not exists in routes", cargo.airp_arv.c_str() );
          tst();
          own_dest->cargos.Add( cargo );
          tst();
          n1 = n1->next;
        }
        tst();
        //stages
        n1 = GetNode( "stages", nodeFlight );
        if ( n1 == NULL )
          throw EXCEPTIONS::Exception( "Node 'stages' not found" );
        n1 = n1->children;
        ProgTrace( TRACE5, "n1!=NULL=%d, n1->name=%s, string((char*)n->name) == 'stage'=%d",
                   n1!=NULL, (char*)n1->name, string((char*)n1->name) == "stage" );
        while ( n1 != NULL && string((char*)n1->name) == "stage" ) {
          xmlNodePtr n2 = GetNode( "@type", n1 );
          if ( n2 == NULL )
            throw EXCEPTIONS::Exception( "Node 'stage/@type' not found" );
          string stage = upperc(NodeAsString( "@type", n1 ));
          TStage stage_id;
          if ( stage == upperc("sOpenCheckIn") )
            stage_id = sOpenCheckIn;
          else
            if ( stage == upperc("sCloseCheckIn") )
              stage_id = sCloseCheckIn;
            else
              if ( stage == upperc("sOpenBoarding") )
                stage_id = sOpenBoarding;
              else
                if ( stage == upperc("sCloseBoarding") )
                  stage_id = sCloseBoarding;
                else
                  throw EXCEPTIONS::Exception( "Invalid value '%s' of 'stage/@type' node", stage.c_str() );
          TTripStage trip_stage = own_dest->stages.GetStage( stage_id );
          TDateTime scd = NodeAsDateTime( "scd", n1, NoExists );
          if ( scd == NoExists ) {
            scd = trip_stage.scd;
          }
          TDateTime est = NodeAsDateTime( "est", n1, NoExists );
          TDateTime act = NodeAsDateTime( "act", n1, NoExists );
          ProgTrace( TRACE5, "stage: type=%s, scd=%f, est=%f, act=%f", stage.c_str(), scd, est, act );
          if ( trip_stage.scd != scd ) {
            if ( est == NoExists )
              trip_stage.est = scd;
            else
              trip_stage.est = est;
          }
          if ( est != NoExists && trip_stage.est != est ) {
            trip_stage.est = est;
          }
          trip_stage.act = act;
          own_dest->stages.SetStage( stage_id, trip_stage );
          n1 = n1->next;
        }
        n1 = GetNode( "stations", nodeFlight );
        if ( n1 == NULL )
          throw EXCEPTIONS::Exception( "Node 'stations' not found" );
        n1 = n1->children;
        while ( n1 != NULL && string((char*)n1->name) == "station" ) {
          TSOPPStation station;
          xmlNodePtr n2 = GetNode( "name", n1 );
          if ( n2 == NULL )
            throw EXCEPTIONS::Exception( "Node 'station/name' not found" );
          station.name = NodeAsString( n2 );
          if ( station.name.empty() || station.name.size() > 6 )
            throw EXCEPTIONS::Exception( "Invalid value '%s' of 'name' node", NodeAsString( n2 ) );
          n2 = GetNode( "work_mode", n1 );
          if ( n2 == NULL )
            throw EXCEPTIONS::Exception( "Node 'station/work_mode' not found" );
          station.work_mode = NodeAsString( n2 );
          if ( upperc(station.work_mode) != upperc("checkin") && upperc(station.work_mode) != upperc("boarding") )
            throw EXCEPTIONS::Exception( "Invalid value '%s' of 'name' node", NodeAsString( n2 ) );
          if ( upperc(station.work_mode) == upperc("checkin") )
            station.work_mode = "Р";
          else
            station.work_mode = "П";
          ProgTrace( TRACE5, "station: name=%s, work_mode=%s", station.name.c_str(), station.work_mode.c_str() );
          TermQry.SetVariable( "name", station.name );
          TermQry.SetVariable( "work_mode", station.work_mode );
          TermQry.Execute();
          if ( TermQry.Eof ) {
            if ( station.work_mode == "Р" )
              invalid_desks += string( " " ) + station.name;
            else
              invalid_gates += string( " " ) + station.name;
          }
          else {
            own_dest->stations.Add( station );
          }
          n1 = n1->next;
        }
        if ( !invalid_desks.empty() )
          invalid_desks = string( "Неправильно заданы стойки регистрации:" ) + invalid_desks;
        if ( !invalid_gates.empty() ) {
          if ( !invalid_desks.empty() )
            invalid_desks += ",";
          invalid_desks += string( "Неправильно заданы выходы:" ) + invalid_gates;
        }
        if ( !invalid_desks.empty() ) {
          TCobraError error;
          error.code = 1;
          error.flight_id = flight_id;
          error.msg = invalid_desks;
          errors.push_back( error );
        }
        ProgTrace( TRACE5, "flight_id=%s OK", flight_id.c_str() );

        //write to BD
        //int move_id;
        {
/*          TPoints points;
          points.move_id = flight.move_id;
          int point_num = 0;
          BitSet<TUseDestData> UseData;
          UseData.clearFlags();
          UseData.setFlag( udNoCalcESTTimeStage );
          for ( vector<TCobraPoint>::iterator i=flight.points.begin(); i!=flight.points.end(); i++ ) {
            TPointsDest dest;
            if ( i->point_id != NoExists ) //+update
              dest.Load( i->point_id, UseData );
            else {
              dest.UseData = UseData;
              dest.airline = i->airline;
              dest.airline_fmt = i->airline_fmt;
              dest.flt_no = i->flt_no;
              dest.suffix = i->suffix;
              dest.suffix_fmt = i->suffix_fmt;
              dest.airp = i->airp;
              dest.airp_fmt = i->airp_fmt;
              dest.scd_out = i->scd_out;
              dest.scd_in = i->scd_in;
            }
            dest.UseData.setFlag( udCargo );
            dest.UseData.setFlag( udMaxCommerce );
            dest.UseData.setFlag( udStations );
            dest.point_num = point_num;
            dest.trip_type = flight.trip_type;
            dest.craft = i->craft;
            dest.craft_fmt = i->craft_fmt;
            dest.bort = i->bort;
            dest.est_in = i->est_in;
            dest.act_in = i->act_in;
            dest.est_out = i->est_out;
            dest.act_out = i->act_out;
            dest.trip_type = flight.trip_type;
            dest.litera = flight.litera;
            dest.pr_del = flight.pr_cancel;
            dest.key = i->key;
            if ( i->airp == local_airp ) { // наш пункт, относительно которого выдаем загрузку по маршруту
              dest.max_commerce.SetValue( flight.max_commerce );
              for ( vector<TCobraStage>::iterator istage=flight.stages.begin(); istage!=flight.stages.end(); istage++ ) {
                TTripStage stage = dest.stages.GetStage( istage->stage_id );
                if ( stage.scd != istage->scd ) {
                  if ( istage->est == NoExists )
                    stage.est = istage->scd;
                  else
                    stage.est = istage->est;
                }
                stage.act = istage->act;
                dest.stages.SetStage( istage->stage_id, stage );
              }
              for ( vector<TCobraStation>::iterator istation=flight.stations.begin(); istation!=flight.stations.end(); istation++ ) {
                TSOPPStation station;
                station.name = istation->name;
                station.work_mode = istation->work_mode;
                dest.stations.Add( station );
              }
            }
            points.dests.push_back( dest );
            point_num++;
          }
          //+удаленные пункты
          for ( vector<TPointsDest>::iterator i=old_dests.begin(); i!=old_points.dests.end(); i++ ) {
            if ( !i->pr_del )
              continue;
            ProgTrace( TRACE5, "deleted: a.airp=%s, a.point_id=%d", i->airp.c_str(), i->point_id );
            TPointsDest dest;
            dest.Load( i->point_id, UseData );
            dest.pr_del = -1;
            points.dests.push_back( dest );
          }*/
          AstraLocale::LexemaData lexemaData;
          tst();
          points.Verify( true, lexemaData );
          tst();
          points.Save( false );
          tst();
          //move_id = points.move_id;
        }
        //
        ProgTrace( TRACE5, "move_id=%d, flight_id=%s", points.move_id, flight_id.c_str() );
        if ( flight_pr_cobra ) {
          UQry.SetVariable( "move_id", points.move_id );
          UQry.SetVariable( "flight_id", flight_id );
          UQry.SetVariable( "what_do", flight_action != caDelete );
          UQry.Execute();
        }
        OraSession.Commit();
      }
      catch( EXCEPTIONS::Exception &E ) {
        OraSession.Rollback();
        TCobraError error;
        error.code = 0;
        error.flight_id = flight_id;
        error.msg = E.what();
        ProgError( STDLOG, "flight_id=%s, error=%s", flight_id.c_str(), error.msg.c_str() );
        errors.push_back( error );
      }
      catch( ... ) {
        OraSession.Rollback();
        TCobraError error;
        error.code = 0;
        error.flight_id = flight_id;
        error.msg = "Unknown error";
        ProgError( STDLOG, "flight_id=%s, error=%s", flight_id.c_str(), error.msg.c_str() );
        errors.push_back( error );
      }
      nodeFlight = nodeFlight->next;
    }
  }
  catch( EXCEPTIONS::Exception &E ) {
    TCobraError error;
    error.code = 0;
    error.flight_id = flight_id;
    error.msg = E.what();
    errors.push_back( error );
  }
  catch( ... ) {
    TCobraError error;
    error.code = 0;
    error.flight_id = flight_id;
    error.msg = "Unknown error";;
    errors.push_back( error );
  }
}

void ParseStages( const xmlNodePtr reqNode, vector<TCobraError> &errors )
{
  //разбор входящего сообщения
  errors.clear();
}

bool parseIncommingCobraData( )
{
  // выбираем входящие сообщения - разбираем и кладем в БД ответ
  ProgTrace( TRACE5, "parseIncommingCobraData" );
  TQuery ParamsQry(&OraSession);
  ParamsQry.SQLText =
    "SELECT name,value FROM file_params WHERE id=:id";
  ParamsQry.DeclareVariable( "id", otInteger );

  TQuery Qry(&OraSession);
  Qry.SQLText =
		"SELECT file_queue.id, files.data, file_queue.receiver "
		" FROM file_queue,files,tcp_msg_types "
		" WHERE file_queue.id=files.id AND "
    "       file_queue.sender=:sender AND "
    "       file_queue.type=tcp_msg_types.code AND "
    "       tcp_msg_types.code=:msg_in_type "
    " ORDER BY DECODE(in_order,1,files.time,file_queue.time),file_queue.id";
	Qry.CreateVariable( "sender", otString, OWN_POINT_ADDR() );
	Qry.CreateVariable( "msg_in_type", otString, CANON_NAME_COBRA_INCOMMING );
	tst();
	Qry.Execute();
	tst();
	map<string,string> fileparams;
	string strbody;
	char *p = NULL;
	tst();
  if ( !Qry.Eof ) {
    tst();
    int file_id = Qry.FieldAsInteger( "id" );
    ProgTrace( TRACE5, "parseIncommingCobraData: file_id=%d", file_id );
    int len = Qry.GetSizeLongField( "data" );
    if ( p )
   		p = (char*)realloc( p, len );
   	else
   	  p = (char*)malloc( len );
   	if ( !p )
   		throw EXCEPTIONS::Exception( string( "Can't malloc " ) + IntToString( len ) + " byte" );
    try {
      Qry.FieldAsLong( "data", p );
      strbody = string( p, len );
      if ( p )
        free( p );
    }
    catch( ... ) {
      if ( p )
        free( p );
      throw;
    }
    ParamsQry.SetVariable( "id", file_id );
    ParamsQry.Execute();
    fileparams.clear();
    while ( !ParamsQry.Eof ) {
      fileparams[ ParamsQry.FieldAsString( "name" ) ] = ParamsQry.FieldAsString( "value" );
      ParamsQry.Next();
    }
    tst();
    xmlDocPtr reqDoc = TextToXMLTree( strbody );
    xml_decode_nodelist( reqDoc->children );
    vector<TCobraError> errors;
    string str_answer;
    try {
      if ( string( (char*)reqDoc->children->name ) != COBRA_FLIGHTS_COMMAND_TAG &&
           string( (char*)reqDoc->children->name ) != COBRA_STAGES_COMMAND_TAG ) {
        ProgError( STDLOG, "parseIncommingCobraData: unknown command tag %s", (char*)reqDoc->children->name );
        str_answer = createInvalidMsg(CobraMsgCodePage);
      }
      else {
        ProgTrace( TRACE5, "parseIncommingCobraData: command tag %s", (char*)reqDoc->children->name );
        if ( string( (char*)reqDoc->children->name ) == COBRA_FLIGHTS_COMMAND_TAG )
          ParseFlights( reqDoc->children, errors );
        else
          if ( string( (char*)reqDoc->children->name ) == COBRA_STAGES_COMMAND_TAG )
            ParseStages( reqDoc->children, errors );
        str_answer = createCommitStr( CobraMsgCodePage, 0, errors );
      }
      TAstraServMsg msg;
      StrToInt( fileparams[ SESS_MSG_FLAGS ].c_str(), msg.flags );
      msg.setClientAnswer();
      fileparams[ SESS_MSG_FLAGS ] = IntToString( msg.flags );
      int fid= TFileQueue::putFile( Qry.FieldAsString("receiver"),
                                    OWN_POINT_ADDR(),
                                    CANON_NAME_COBRA_OUTCOMMING,
                                    fileparams,
                                    str_answer );
      ProgTrace( TRACE5, "parseIncommingCobraData: answer in putFile, file_id=%d, msg_id=%s", fid, fileparams[ SESS_MSG_ID ].c_str() );
      ToEvents( COBRA_CLIENT_TYPE, file_id, fid );
      TFileQueue::deleteFile( file_id );
    }
    catch(...) {
      xmlFreeDoc( reqDoc );
      throw;
    }
    xmlFreeDoc( reqDoc );
    Qry.Next();
    return Qry.Eof;
  }
  return true;
}

/////////////////////////WB_GARANT////////////////////////////////////////////

const
  std::string WB_GARANT_FLIGHT_COMMAND_TAG = "flight";

int main_tcp_wb_garant_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist)
{
  try
  {
    sleep(1);
    InitLogTime(NULL);
    OpenLogFile("log1");

    int p_count;
    int listener_port;
    if ( TCL_OK != Tcl_ListObjLength( interp, argslist, &p_count ) ) {
    	ProgError( STDLOG,
                 "ERROR:main_tcpserv_tcl wrong parameters:%s",
                 Tcl_GetString(Tcl_GetObjResult(interp)) );
      return 1;
    }
    if ( p_count != 2 ) {
    	ProgError( STDLOG,
                 "ERROR:main_timer_tcl wrong number of parameters:%d",
                 p_count );
    }
    else {
    	 Tcl_Obj *val;
    	 Tcl_ListObjIndex( interp, argslist, 1, &val );
    	 StrToInt( Tcl_GetString( val ), listener_port );
    }

    ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks()
        ->connect_db();

    if (init_edifact()<0) throw Exception("'init_edifact' error");

    TReqInfo::Instance()->clear();
    base_tables.Invalidate();

    ProgTrace( TRACE5, "listener_port=%d", listener_port );
    TTCPListener<WBGarantSession> wb_garant_serv(listener_port);
    tst();
    wb_garant_serv.Execute();
  }
  catch( std::exception &E ) {
    ProgError( STDLOG, "std::exception: %s", E.what() );
  }
  catch( ... ) {
    ProgError( STDLOG, "Unknown error" );
  };
  return 0;
}

bool parseIncommingWB_GarantData( );

int main_wb_garant_handler_tcl(Tcl_Interp *interp,int in,int out, Tcl_Obj *argslist)
{
  try
  {
    sleep(1);
    InitLogTime(NULL);
    OpenLogFile("log1");

    ServerFramework::Obrzapnik::getInstance()->getApplicationCallbacks()
            ->connect_db();

     if (init_edifact()<0) throw Exception("'init_edifact' error");

    TReqInfo::Instance()->clear();
    char buf[10];
    int wait_interval;
    for(;;)
    {
      emptyHookTables();
      TDateTime execTask = NowUTC();
      InitLogTime(NULL);
      base_tables.Invalidate();
      bool queue_empty = parseIncommingWB_GarantData( );
      OraSession.Commit();
      if ( NowUTC() - execTask > 5.0/(1440.0*60.0) )
      	ProgTrace( TRACE5, "Attention execute task time!!!, name=%s, time=%s","CMD_PARSE_WB_GARANT", DateTimeToStr( NowUTC() - execTask, "nn:ss" ).c_str() );
      callPostHooksAfter();
      if ( queue_empty )
        wait_interval = WAIT_PARSER_INTERVAL;
      else
        wait_interval = 50;
      waitCmd("CMD_PARSE_WB_GARANT",wait_interval,buf,sizeof(buf));
    }; // end of loop
  }
  catch(EOracleError &E)
  {
    ProgError( STDLOG, "EOracleError %d: %s", E.Code, E.what());
    ProgError( STDLOG, "SQL: %s", E.SQLText());
  }
  catch(std::exception &E)
  {
    ProgError( STDLOG, "std::exception: %s", E.what() );
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
  tst();
  return 0;
};

bool WBGarantSession::isInvalidAnswer( const xmlDocPtr &doc )
{
  return ( doc == NULL || doc->children == NULL || GetNode( SESS_COMMIT_MSG.c_str(), doc->children ) == NULL );
}

void SetAirpProperty( xmlNodePtr &nodeAirp, const string where, const TDestBalance &destbal )
{
  std::map<std::string,TBalance> classbal;
  TBalance bal;
  if ( where == "tranzit" ) {
    classbal = destbal.tranzit_classbal;
    bal = destbal.tranzit;
  }
  else
    if ( where == "checkin" ) {
      classbal = destbal.goshow_classbal;
      bal = destbal.goshow;
    }
    else
      throw Exception( "invalid param 'where'" );
  if ( classbal.empty() && bal.cargo == 0 && bal.mail == 0  )
    return;

  xmlNodePtr n2 = NewTextChild( nodeAirp, where.c_str() );

  if ( !classbal.empty() ) {
    xmlNodePtr maleNode = NULL;
    xmlNodePtr femaleNode = NULL;
    xmlNodePtr chdNode = NULL;
    xmlNodePtr infNode = NULL;
    xmlNodePtr rkNode = NULL;
    xmlNodePtr bagamountNode = NULL;
    xmlNodePtr bagweightNode = NULL;
    xmlNodePtr paybagNode = NULL;
    for ( std::map<std::string,TBalance>::iterator iclass=classbal.begin(); iclass!=classbal.end(); iclass++ ) {
      if ( iclass->second.male > 0 ) {
        if ( !maleNode )
          maleNode = NewTextChild( n2, "male" );
        SetProp( NewTextChild( maleNode, "class", iclass->second.male ), "id", iclass->first );
      }
      if ( iclass->second.female > 0 ) {
        if ( !femaleNode )
          femaleNode = NewTextChild( n2, "female" );
        SetProp( NewTextChild( femaleNode, "class", iclass->second.female ), "id", iclass->first );
      }
      if ( iclass->second.chd > 0 ) {
        if ( !chdNode )
          chdNode = NewTextChild( n2, "chd" );
        SetProp( NewTextChild( chdNode, "class", iclass->second.chd ), "id", iclass->first );
      }
      if ( iclass->second.inf > 0 ) {
        if ( !infNode )
          infNode = NewTextChild( n2, "inf" );
        SetProp( NewTextChild( infNode, "class", iclass->second.inf ), "id", iclass->first );
      }
      if ( iclass->second.rk_weight > 0 ) {
        if ( !rkNode )
          rkNode = NewTextChild( n2, "rk_weight" );
        SetProp( NewTextChild( rkNode, "class", iclass->second.rk_weight ), "id", iclass->first );
      }
      if ( iclass->second.bag_amount > 0 ) {
        if ( !bagamountNode )
          bagamountNode = NewTextChild( n2, "bag_amount" );
        SetProp( NewTextChild( bagamountNode, "class", iclass->second.bag_amount ), "id", iclass->first );
      }
      if ( iclass->second.bag_weight > 0 ) {
        if ( !bagweightNode )
          bagweightNode = NewTextChild( n2, "bag_weight" );
        SetProp( NewTextChild( bagweightNode, "class", iclass->second.bag_weight ), "id", iclass->first );
      }
      if ( iclass->second.paybag_weight > 0 ) {
        if ( !paybagNode )
          paybagNode = NewTextChild( n2, "paybag_weight" );
        SetProp( NewTextChild( paybagNode, "class", iclass->second.paybag_weight ), "id", iclass->first );
      }
    }
  }
  if ( bal.cargo > 0 )
    NewTextChild( n2, "cargo", bal.cargo );
  if ( bal.mail > 0 )
    NewTextChild( n2, "mail", bal.mail );
}

string AnswerFlight( const xmlNodePtr reqNode )
{
  //разбор входящего сообщения
  ProgTrace( TRACE5, "AnswerFlight" );
  string res;
  xmlNodePtr nodeFlight = reqNode->children;
  try {
    TElemFmt fmt;
    xmlNodePtr node = GetNodeFast( "airline", nodeFlight );
    tst();
    if ( node == NULL )
      throw Exception( "node 'airline' not found" );
    tst();
    string airline = NodeAsString( node );
    airline = ElemToElemId( etAirline, airline, fmt, false );
    if ( fmt == efmtUnknown )
      throw Exception( "invalid airline" );
    tst();
    int flt_no;
    node = GetNodeFast( "flt_no", nodeFlight );
    if ( node == NULL )
      throw Exception( "node 'flt_no' not found" );
    tst();
    flt_no = NodeAsInteger( node );
    if ( flt_no > 99999 || flt_no <= 0 )
      throw Exception( "invalid flt_no" );
    tst();
    string suffix;
    node = GetNodeFast( "suffix", nodeFlight );
    tst();
    if ( node ) {
      suffix = NodeAsString( node );
      if ( !suffix.empty() ) {
        suffix = ElemToElemId( etSuffix, suffix, fmt, false );
        if ( fmt == efmtUnknown )
          throw Exception( "invalid suffix" );
      }
    }
    tst();
    TDateTime scd_out;
    node = GetNodeFast( "scd_out", nodeFlight );
    if ( node == NULL )
      throw Exception( "node 'scd_out' not found" );
    tst();
    scd_out = NodeAsDateTime( node );
    modf( scd_out, &scd_out );
    string airp;
    node = GetNodeFast( "airp", nodeFlight );
    if ( node == NULL )
      throw Exception( "node 'airp' not found" );
    tst();
    airp = NodeAsString( node );
    airp = ElemToElemId( etAirp, airp, fmt, false );
    if ( fmt == efmtUnknown )
      throw Exception( "invalid airp" );
    tst();
    TDoubleTrip doubletrip;
    int point_id, move_id = NoExists;
    if ( !doubletrip.IsExists( move_id, airline, flt_no, suffix, airp, NoExists, scd_out, point_id ) ) {
      throw Exception( "flight not found" );
    }
    ProgTrace( TRACE5, "point_id=%d", point_id );
    //////////////////////////////////////////////////////////////////////////////////////////////////
    TQuery FlightPermitQry( &OraSession );
    FlightPermitQry.SQLText =
      "SELECT balance_type, "
      "       DECODE( airp, NULL, 0, 8 ) + "
      "       DECODE( bort, NULL, 0, 4 ) + "
      "       DECODE( airline, NULL, 0, 2 ) + "
      "       DECODE( flt_no, NULL, 0, 1 ) AS priority, "
      "       pr_denial "
      " FROM balance_sets "
      " WHERE airp=:airp AND "
	    "       ( bort IS NULL OR bort=:bort ) AND "
	    "       ( airline IS NULL OR airline=:airline ) AND "
	    "       ( flt_no IS NULL OR flt_no=:flt_no ) "
	    " ORDER BY priority DESC";
    FlightPermitQry.DeclareVariable( "airp", otString );
    FlightPermitQry.DeclareVariable( "bort", otString );
    FlightPermitQry.DeclareVariable( "airline", otString );
    FlightPermitQry.DeclareVariable( "flt_no", otInteger );
    //начитываем данные по СПП для центровки
    TQuery Qry( &OraSession );
    Qry.SQLText =
      "SELECT system.UTCSYSDATE currdate FROM dual";
    Qry.Execute();
    tst();
//    TDateTime currdate = Qry.FieldAsDateTime( "currdate" );
    Qry.Clear();
    string sql;
    sql =
      "SELECT point_id,point_num,first_point,pr_tranzit,airline,flt_no,suffix,"
      "       craft,bort,airp,trip_type,SUBSTR( park_out, 1, 5 ) park_out,"
      "       scd_out,est_out,act_out,time_out FROM points "
      " WHERE pr_del=0 AND pr_reg=1 AND act_out IS NULL AND point_id=:point_id";
    Qry.SQLText = sql;
    Qry.CreateVariable( "point_id", otInteger, point_id );
    Qry.Execute();
    TTripRoute routesB, routesA;
    //экипаж
    TQuery CrewsQry( &OraSession );
    CrewsQry.SQLText =
      "SELECT commander,cockpit,cabin FROM trip_crew WHERE point_id=:point_id";
    CrewsQry.DeclareVariable( "point_id", otInteger );
    TQuery CfgQry( &OraSession );
    CfgQry.SQLText =               //!!!djek переделать на TCFG
       "SELECT class,cfg "
       " FROM trip_classes,classes "
       "WHERE trip_classes.point_id=:point_id AND trip_classes.class=classes.code "
       "ORDER BY priority";
    CfgQry.DeclareVariable( "point_id", otInteger );


    for ( ;!Qry.Eof; Qry.Next() ) {
    /*!!!  if ( fabs( Qry.FieldAsDateTime( "time_out" ) - currdate ) > 1.0 )
        throw Exception( "flight not found" );*/
      tst();
      if ( !getBalanceFlightPermit( FlightPermitQry,
                                    Qry.FieldAsInteger( "point_id" ),
                                    Qry.FieldAsString( "airline" ),
                                    Qry.FieldAsString( "airp" ),
                                    Qry.FieldAsInteger( "flt_no" ),
                                    Qry.FieldAsString( "bort" ),
                                    "BW" ) )
        throw Exception( "permit to flight denial" );
      tst();
      routesA.GetRouteAfter( NoExists,
                             point_id,
                             Qry.FieldAsInteger( "point_num" ),
                             Qry.FieldIsNULL("first_point")?NoExists:Qry.FieldAsInteger("first_point"),
                             Qry.FieldAsInteger( "pr_tranzit" ),
                             trtNotCurrent,
                             trtNotCancelled );
      ProgTrace( TRACE5, "point_id=%d, routes.size()=%zu", point_id, routesA.size() );
      if ( routesA.empty() ) {
        throw Exception( "flight not have takeoff" );
      }
      routesB.GetRouteBefore( NoExists,
                              point_id,
                              Qry.FieldAsInteger( "point_num" ),
                              Qry.FieldIsNULL("first_point")?NoExists:Qry.FieldAsInteger("first_point"),
                              Qry.FieldAsInteger( "pr_tranzit" ),
                              trtWithCurrent,
                              trtNotCancelled );
        vector<SALONS2::TCompSectionLayers> CompSectionsLayers;
        vector<TZoneOccupiedSeats> zones;
        ZoneLoads( point_id, true, true, true, zones, CompSectionsLayers );
        if ( CompSectionsLayers.empty() ) { //нет информации по зонам
          ProgTrace( TRACE5, "CompSectionsLayers.empty(), point_id=%d", point_id );
          throw Exception( "CompSectionsLayers is empty" );
        }
        string bort = Qry.FieldAsString( "bort" );
        if ( !( (bort.size() == 5 && bort.find( "-" ) == string::npos) || (bort.size() == 6 && bort.find( "-" ) == 2) ) ) {
          ProgTrace( TRACE5, "point_id=%d, bort=%s, bort.find=%zu", point_id, bort.c_str(), bort.find( "-" ) );
          throw Exception( "Invalid bort" );
        }
        
        xmlDocPtr doc = CreateXMLDoc( "UTF-8", "flight" );
        xmlNodePtr flightNode = doc->children;
        try {
          NewTextChild( flightNode, "airline", Qry.FieldAsString( "airline" ) );
          NewTextChild( flightNode, "flt_no", Qry.FieldAsInteger( "flt_no" ) );
          if ( !Qry.FieldIsNULL( "suffix" ) )
            NewTextChild( flightNode, "suffix", Qry.FieldAsString( "suffix" ) );
          NewTextChild( flightNode, "tranzit", Qry.FieldAsInteger( "pr_tranzit" ) );
          NewTextChild( flightNode, "bort", bort );
          NewTextChild( flightNode, "scd_out", DateTimeToStr( Qry.FieldAsDateTime( "scd_out" ), ServerFormatDateTimeAsString ) );
          if ( !Qry.FieldIsNULL( "est_out" ) )
            NewTextChild( flightNode, "est_out", DateTimeToStr( Qry.FieldAsDateTime( "est_out" ), ServerFormatDateTimeAsString ) );
          if ( !Qry.FieldIsNULL( "act_out" ) )
            NewTextChild( flightNode, "act_out", DateTimeToStr( Qry.FieldAsDateTime( "act_out" ), ServerFormatDateTimeAsString ) );
          if ( !Qry.FieldIsNULL( "park_out" ) )
            NewTextChild( flightNode, "part", Qry.FieldAsString( "park_out" ) );
          NewTextChild( flightNode, "craft", Qry.FieldAsString( "craft" ) );
          CfgQry.SetVariable( "point_id", point_id );
          CfgQry.Execute();
          xmlNodePtr n = NewTextChild( flightNode, "comp" );
          while ( !CfgQry.Eof ) { //!!!djek переделать на TCFG
             SetProp( NewTextChild( n, "class", CfgQry.FieldAsInteger( "cfg" ) ), "id", CfgQry.FieldAsString( "class" ) );
             CfgQry.Next();
          }
          CrewsQry.SetVariable( "point_id", point_id );
          CrewsQry.Execute();
          if ( !CrewsQry.Eof ) {
            if ( !CrewsQry.FieldIsNULL( "cockpit" ) ) {
              NewTextChild( flightNode, "crew", CrewsQry.FieldAsString( "cockpit" ) );
            }
            if ( !CrewsQry.FieldIsNULL( "cockpit" ) ) {
              NewTextChild( flightNode, "stew", CrewsQry.FieldAsString( "cabin" ) );
            }
            if ( !CrewsQry.FieldIsNULL( "commander" ) )
              NewTextChild( flightNode, "capt", upperc( CrewsQry.FieldAsString( "commander" ) ) );
          }
          xmlNodePtr loadsNode = NewTextChild( flightNode, "loads" );
          n = NewTextChild( loadsNode, "airps" );
          tst();
          TBalanceData balanceData;
          balanceData.get( point_id, Qry.FieldAsInteger( "pr_tranzit" ), routesB, routesA, false );
          tst();
          map<string,TBalance> classbal;
          bool pr_pads = false;
          for ( vector<TDestBalance>::iterator ibal=balanceData.balances.begin(); ibal!=balanceData.balances.end(); ibal++ ) {
            xmlNodePtr n1 = NewTextChild( n, "airp" );
            SetProp( n1, "id", ibal->airp );
            SetAirpProperty( n1, "tranzit", *ibal );
            SetAirpProperty( n1, "checkin", *ibal );
            for ( std::map<std::string,TBalance>::iterator iclass=ibal->total_classbal.begin(); iclass!=ibal->total_classbal.end(); iclass++ ) {
              ProgTrace( TRACE5, "ibal->airp=%s, iclass->first=%s, iclass->second.pad=%d", ibal->airp.c_str(), iclass->first.c_str(), iclass->second.pad );
              classbal[ iclass->first ] += iclass->second;
              if ( iclass->second.pad > 0 )
                pr_pads = true;
            }
          }
          if ( pr_pads  ) {
            n = NewTextChild( loadsNode, "pads" );
            for ( std::map<std::string,TBalance>::iterator iclass=classbal.begin(); iclass!=classbal.end(); iclass++ ) {
              SetProp( NewTextChild( n, "class", iclass->second.pad ), "id", iclass->first );
            }
          }
          n = NewTextChild( loadsNode, "seats" );
          SetProp( n, "type", "sections" );
          vector<TPassenger> passs;
          for ( map<int,TPassenger>::iterator i=balanceData.passengers.begin(); i!=balanceData.passengers.end(); i++ ) {
            trace( i->second.pax_id, i->second.grp_id, i->second.parent_pax_id, i->second.parent_pax_id, i->second.pers_type, i->second.seats );
            ProgTrace( TRACE5, "pax_id=%d, is_female=%d", i->second.pax_id, (int)i->second.is_female );
            passs.push_back( i->second );
          }
          ProgTrace( TRACE5, "passs.size=%zu", passs.size() );
          std::vector<SALONS2::TCompSection> compSections;
          ZonePax<TPassenger>( point_id, passs, compSections );
          ProgTrace( TRACE5, "passs.size=%zu", passs.size() );
          for ( vector<TPassenger>::iterator i=passs.begin(); i!=passs.end(); i++ ) {
            trace( i->pax_id, i->grp_id, i->parent_pax_id, i->parent_pax_id, i->pers_type, i->seats );
            ProgTrace( TRACE5, "pax_id=%d, is_female=%d", i->pax_id, (int)i->is_female );
            balanceData.passengers[ i->pax_id ].zone = i->zone;
          }
          for ( vector<SALONS2::TCompSection>::iterator i=compSections.begin(); i!=compSections.end(); i++ ) {
            xmlNodePtr secNode = NewTextChild( n, "seat" );
            NewTextChild( secNode, "name", i->name );
            NewTextChild( secNode, "capacity", i->seats );
            int tranzit = 0;
            for ( vector<SALONS2::TCompSectionLayers>::iterator icl=CompSectionsLayers.begin(); icl!=CompSectionsLayers.end(); icl++ ) {
              if ( icl->compSection.name != i->name )
                continue;
              tranzit = icl->layersSeats[ cltBlockTrzt ].size() +
                        icl->layersSeats[ cltSOMTrzt ].size() +
                        icl->layersSeats[ cltPRLTrzt ].size() +
                        icl->layersSeats[ cltPRLTrzt ].size();
            }
            int male = 0, male_count = 0, male_pad = 0, male_pad_count = 0;
            int female = 0, female_count = 0, female_pad = 0, female_pad_count = 0;
            int chd = 0, chd_count = 0, chd_pad = 0, chd_pad_count = 0;
            int inf = 0, inf_count = 0, inf_woseats = 0;
            for ( map<int,TPassenger>::iterator p=balanceData.passengers.begin(); p!=balanceData.passengers.end(); p++ ) {
              ProgTrace( TRACE5, "pax_id=%d, seats=%d, zone=%s", p->second.pax_id, p->second.seats, p->second.zone.c_str() );
              if ( p->second.zone != i->name )
                continue;
              int count = ( p->second.pers_type == "ВЗ" )&&( !p->second.is_female );
              male_count += count;
              male += count*p->second.seats;
              male_pad += count*p->second.seats*p->second.pr_pad;
              male_pad_count += count*p->second.pr_pad;
              count = ( p->second.pers_type == "ВЗ" )&&( p->second.is_female );
              female_count += count;
              female += count*p->second.seats;
              female_pad += count*p->second.seats*p->second.pr_pad;
              female_pad_count += count*p->second.pr_pad;
              count = ( p->second.pers_type == "РБ" );
              chd_count += count;
              chd += count*p->second.seats;
              chd_pad += count*p->second.seats*p->second.pr_pad;
              chd_pad_count += count*p->second.pr_pad;
              if ( p->second.pers_type == "РМ" ) {
                inf_count++;
                if ( p->second.seats == 0 )
                  inf_woseats++;
                else
                  inf += p->second.seats;
              }
            }
            if ( male > 0 ) {
              xmlNodePtr occNode = NewTextChild( secNode, "occupied", male ); //кол-во занятых кресел мужчинами
              SetProp( occNode, "pers_type", "male" );
              if ( male_pad > 0 ) // кол-во занятых кресел служебниками
                SetProp( occNode, "pad", male_pad );
              if ( male != male_count )
                SetProp( occNode, "count", male_count ); // кол-во мужчин
              if ( male_pad != male_pad_count )
                SetProp( occNode, "pad_count", male_pad_count ); // кол-во мужчин служебников
            }
            if ( female > 0 ) {
              xmlNodePtr occNode = NewTextChild( secNode, "occupied", female );
              SetProp( occNode, "pers_type", "female" );
              if ( female_pad > 0 )
                SetProp( occNode, "pad", female_pad );
              if ( female != female_count )
                SetProp( occNode, "count", female_count ); // кол-во женщин
              if ( female_pad != female_pad_count )
                SetProp( occNode, "pad_count", female_pad_count ); // кол-во женщин служебников
            }
            if ( chd > 0 ) {
              xmlNodePtr occNode = NewTextChild( secNode, "occupied", chd );
              SetProp( occNode, "pers_type", "chd" );
              if ( chd_pad > 0 )
                SetProp( occNode, "pad", chd_pad );
              if ( chd != chd_count )
                SetProp( occNode, "count", chd_pad_count );
              if ( chd_pad != chd_pad_count )
                SetProp( occNode, "pad_count", chd_pad_count );
            }
            if ( inf > 0 ) {
              xmlNodePtr occNode = NewTextChild( secNode, "occupied", inf );
              SetProp( occNode, "pers_type", "inf" );
            }
            if ( inf_woseats > 0 )
              NewTextChild( secNode, "infant_woseats", inf_woseats );
            if ( tranzit > 0 )
              NewTextChild( secNode, "tranzit", tranzit );
          }
          res = XMLTreeToText( doc );
        }
        catch(...) {
          xmlFreeDoc( doc );
          throw;
        }
        xmlFreeDoc( doc );
//////////////////////
  }
  }
  catch( Exception &e ) {
    ProgTrace( TRACE5, "exception.what()=%s", e.what() );
    xmlDocPtr doc = CreateXMLDoc( "UTF-8", "error" );
    NewTextChild( doc->children, "message", e.what() );
    res = XMLTreeToText( doc );
  }
  return res;
}

bool parseIncommingWB_GarantData( )
{
  // выбираем входящие сообщения - разбираем и кладем в БД ответ
  ProgTrace( TRACE5, "parseIncommingWB_GarantData" );
  TQuery ParamsQry(&OraSession);
  ParamsQry.SQLText =
    "SELECT name,value FROM file_params WHERE id=:id";
  ParamsQry.DeclareVariable( "id", otInteger );

  TQuery Qry(&OraSession);
  Qry.SQLText =
		"SELECT file_queue.id, files.data, file_queue.receiver "
		" FROM file_queue,files,tcp_msg_types "
		" WHERE file_queue.id=files.id AND "
    "       file_queue.sender=:sender AND "
    "       file_queue.type=tcp_msg_types.code AND "
    "       tcp_msg_types.code=:msg_in_type "
    " ORDER BY DECODE(in_order,1,files.time,file_queue.time),file_queue.id";
	Qry.CreateVariable( "sender", otString, OWN_POINT_ADDR() );
	Qry.CreateVariable( "msg_in_type", otString, CANON_NAME_WB_GARANT_INCOMMING );
	tst();
	Qry.Execute();
	tst();
	map<string,string> fileparams;
	string strbody;
	char *p = NULL;
  if ( !Qry.Eof ) {
    int file_id = Qry.FieldAsInteger( "id" );
    ProgTrace( TRACE5, "parseIncommingWB_GarantData: file_id=%d", file_id );
    int len = Qry.GetSizeLongField( "data" );
    if ( p )
   		p = (char*)realloc( p, len );
   	else
   	  p = (char*)malloc( len );
   	if ( !p )
   		throw EXCEPTIONS::Exception( string( "Can't malloc " ) + IntToString( len ) + " byte" );
    try {
      Qry.FieldAsLong( "data", p );
      strbody = string( p, len );
      if ( p )
        free( p );
    }
    catch( ... ) {
      if ( p )
        free( p );
      throw;
    }
    ParamsQry.SetVariable( "id", file_id );
    ParamsQry.Execute();
    fileparams.clear();
    while ( !ParamsQry.Eof ) {
      fileparams[ ParamsQry.FieldAsString( "name" ) ] = ParamsQry.FieldAsString( "value" );
      ParamsQry.Next();
    }

    xmlDocPtr reqDoc = TextToXMLTree( strbody );
    xml_decode_nodelist( reqDoc->children );
    vector<string> errors;
    string str_answer;
    try {
      if ( string( (char*)reqDoc->children->name ) != WB_GARANT_FLIGHT_COMMAND_TAG ) {
        ProgError( STDLOG, "parseIncommingWB_GarantData: unknown command tag %s", (char*)reqDoc->children->name );
        str_answer = createInvalidMsg(WBGarantMsgCodePage);
      }
      else {
        str_answer = AnswerFlight( reqDoc->children );
        ProgTrace( TRACE5, "AnswerFlight: str_answer.size()=%zu", str_answer.size() );
        str_answer = ConvertCodepage( str_answer, "CP866", WBGarantMsgCodePage );
        ProgTrace( TRACE5, "AnswerFlight after convert: str_answer.size()=%zu", str_answer.size() );
      }
      TAstraServMsg msg;
      StrToInt( fileparams[ SESS_MSG_FLAGS ].c_str(), msg.flags );
      msg.setClientAnswer();
      fileparams[ SESS_MSG_FLAGS ] = IntToString( msg.flags );
      int fid= TFileQueue::putFile( Qry.FieldAsString("receiver"),
                                    OWN_POINT_ADDR(),
                                    CANON_NAME_WB_GARANT_OUTCOMMING,
                                    fileparams,
                                    str_answer );
      ProgTrace( TRACE5, "parseIncommingWB_GarantData: answer in putFile, file_id=%d, msg_id=%s", fid, fileparams[ SESS_MSG_ID ].c_str() );
      ToEvents( WB_GARANT_CLIENT_TYPE, file_id, fid );
      TFileQueue::deleteFile( file_id );
    }
    catch(...) {
      xmlFreeDoc( reqDoc );
      throw;
    }
    xmlFreeDoc( reqDoc );
    Qry.Next();
    return Qry.Eof;
  }
  return true;
}
