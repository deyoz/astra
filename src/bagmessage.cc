#include <boost/asio.hpp>
#include <serverlib/new_daemon.h>
#include <serverlib/tclmon.h>

#include "exceptions.h"
#include "msg_queue.h"
#include "astra_utils.h"
#include "file_queue.h"
#include "date_time.h"

#define NICKNAME "GRIG"
#define NICKTRACE GRIG_TRACE
#include <serverlib/slogger.h>

using namespace EXCEPTIONS;
using namespace std;
using namespace boost::asio;
using namespace BASIC::date_time;

class BagMsgQueueDaemon : public ServerFramework::NewDaemon
{
public:
    BagMsgQueueDaemon() : ServerFramework::NewDaemon("bag_msg_handler") {}
};

// �����஢�� ⨯�� ᮮ�饭��, ����� ����� ���� ��᫠�� � ��⥬� SITA BagMessage
char messageTypes[][20] = { "unknown", "LOGIN_RQST", "LOGIN_ACCEPT", "LOGIN_REJECT", "DATA", "ACK_DATA", "ACK_MSG", "NAK_MSG",
                          "STATUS", "DATA_ON", "DATA_OFF", "LOG_OFF" };

// ��������� ᮮ�饭��, ��ࠢ�塞��� � BagMessage
typedef struct
{
  char appl_id[8];
  u_short version;
  u_short type;
  u_short message_id_number;
  u_short data_length;
  char reserved[4];
} BM_HEADER;

// ����饭�� ����� � ����������.
typedef struct
{
  BM_HEADER head;
  char text[2048];
} BM_MESSAGE;

std::queue<BM_MESSAGE> mes_queue;

class BMConnection // ���������� � ��⥬�� SITA BagMessage
{
  public:
  // ����� �室� � ��⥬� SITA BagMessage
    typedef enum
    {
      BM_LOGIN_NONE = 0, // �� �뫮 ����⪨ ������
      BM_LOGIN_WAIT = 1, // ��᫠�� �����/��஫� - ���� �⢥�
      BM_LOGIN_READY = 2 // ��� ���⨫� � �� ��⥬�
    } BM_LOGIN_STATUS;
  // ���� ᮮ�饭��, ����� ����� ���� ��᫠�� � ��⥬� SITA BagMessage ��� �ਭ��� �� ���.
    typedef enum
    {
      LOGIN_RQST = 1,      // ����� �室� � 㤠������ ��⥬�
      LOGIN_ACCEPT = 2,    // ������⥫�� �⢥� �� ����� �室�
      LOGIN_REJECT = 3,    // ����⥫�� �⢥� �� ����� �室�
      DATA = 4,            // ��।�� ������
      ACK_DATA = 5,        // ��।�� ������ � ����ᮬ ���⢥ত���� �ਥ��
      ACK_MSG = 6,         // ������⥫�� �⢥� �� ����� ���⢥ত���� �ਥ�� ������
      NAK_MSG = 7,         // ����⥫�� �⢥� �� ����� ���⢥ত���� �ਥ�� ������
      STATUS = 8,          // ����� - ���⢥ত���� ⮣�, �� ��� �� ����襭�
      DATA_ON = 9,         // �⬥�� ࠭�� �뤠����� DATA_OFF
      DATA_OFF = 10,       // ���졠 �ਮ�⠭����� ���뫪� ������
      LOG_OFF = 11         // ����� ������� �裡
    } BM_MESSAGE_TYPE;
  // ���ᨨ ���������� ᮮ�饭��
    typedef enum
    {
      VERSION_2 = 2        // ���� �� ⮫쪮 ⠪��
    } BM_HEADER_VERSIONS;
  private:
    typedef struct
    {
      string host;        // IP-���� ���
      int port;           // ����
    } BM_HOST;
    static const int NUMLINES = 2;
  // ����ன��, ���⠭�� ����� �� ����᪥
    string canonName;  // ��� �ࢨ�, �� ���஬� ��ࠢ������ ⥫��ࠬ��
    BM_HOST ip_addr[NUMLINES];  // ���� ��� ����� �裡 - �᭮���� � १�ࢭ� (१�ࢭ��?)
    string login;      // ����� � ��⥬� SITA BagMessage
    string password;   // ��஫� ⠬ ��
    int heartBeat;     // ���ᨬ���� ���ࢠ� �६��� ����� ᮮ�饭�ﬨ
  // ����稥 ��६����
    bool configured;   // �ਧ��� ⮣�, �� ���䨣���� ���⠭�
    bool connected;    // �ਧ��� ⮣�, �� ��� ��⠭������
    bool paused;       // �ਧ��� ⮣�, �� ��㣠� ��஭� ����ᨫ� ���� (�뤠�� DATA_OFF)
    int activeIp;      // ����� �� ����� �裡 ᥩ�� �ᯮ������?
    BM_LOGIN_STATUS loginStatus;  // �ਧ��� ⮣�, �� �� ��諨 � �� ��⥬�
    io_service *io;          // ��ࢨ�, ���ᯥ稢��騩 ���
    ip::tcp::socket *socket; // ����� ��� �裡
    u_short mes_num;  // ����� ᮮ�饭�� ��� ��⮪��� �裡
    time_t lastRecvTime; // �६� ��室� �।��饣� ᮮ�饭�� - ��᫥�������� ��� �業�� ࠡ��ᯮᮡ���� �����
    time_t lastSendTime; // �६� ��।�� �।��饣� ᮮ�饭�� - ��� ��।�� ����� �� �����
    time_t lastAdmin;    // �६� ��᫥����� ����⢨� �� �ࠢ����� ����ᮬ - ��� �࣠����樨 ����থ�
    BM_HEADER header;    // ��������� ᮮ�饭��, �ନ�㥬��� ��� ��।��
    string text;         // ����� ᮮ�饭��, �ନ�㥬��� ��� ��।��
    int waitForAck;      // ��⮪���� ����� ᮮ�饭��, �� ���뫪� ���ண� ���⠢��� ����� ���⢥ত����,
                         // �⢥� �� ����� �� �� ����祭, ��� -1 �᫨ ��祣� �� ����
    int waitForAckId;    // ID ᮮ⢥�����饩 ⥫��ࠬ�� � ���� ������
    time_t waitForAckTime; // ����� ��᫠�� �� ᮮ�饭��
  public:
    BMConnection();
    ~BMConnection();
    void run();                            // ������� ��-� � ࠡ�祬 横��
    bool isInit() { return configured; };  // �஢����, �� ���䨣���� ���⠭�
    void init();                               // ������ ���䨣���� �� ����᪠�饣� �ਯ� � ��࠭��� � ��ꥪ�
    bool isConnected() { return connected; }   // �஢����, �� ���� ᮥ������� � �ࢥ஬
    void connect( io_service& io );            // ����������� � �ࢥ��
    bool sendTlg( string text );               // ��᫠�� BSM-⥫��ࠬ��
    void sendTlgAck( int id, string text );    // ��᫠�� BSM-⥫��ࠬ�� � ����ᮬ ���⢥ত���� �ਥ��
  private:
    void makeMessage( BM_MESSAGE_TYPE type, string text = "" , int msg_id = -1 );     // �����⮢��� ��������� � ⥪�� � ���뫪�
    bool doSendMessage();                                      // ��᫠�� �����⮢������ ᮮ�饭��
    bool sendMessage( BM_MESSAGE_TYPE type, string text = "", int msg_id = -1 );  // ��᫠�� � BagMessage ᮮ�饭�� - ��騥 ����
    bool sendLogin();                     // ��᫠�� � �� ��⥬� ����� � ��஫�
    void checkInput();                    // �஢���� �室��� ��⮪ � ��-� � ��� ᤥ����
    void checkTimer();                    // �஢���� ⠩���� ᮮ�饭�� - ���� ����� ���㫠��
    void checkQueue();                    // �஢���� ��।� �� ��室, � �᫨ ���� 祣� - � ��᫠��.
};

BMConnection::BMConnection()
{
  configured = false;
  connected = false;
  paused = false;
  loginStatus = BM_LOGIN_NONE;
  io = NULL;
  socket = NULL;
  lastAdmin = 0;
  mes_num = 1;
  waitForAck = -1;
}

BMConnection::~BMConnection()
{
  if( loginStatus == BM_LOGIN_READY )
    sendMessage( LOG_OFF );
  if( connected )
    socket->close();
  if( socket != NULL )
    delete( socket );
}

void BMConnection::init()
{
  if( configured )
    return;
  canonName = getTCLParam( "SBM_CANON_NAME", NULL );
  ip_addr[0].host = getTCLParam( "SBM_HOST1", NULL );
  ip_addr[0].port = getTCLParam( "SBM_PORT1", 1000, 65535, 65535 );
  ip_addr[1].host = getTCLParam( "SBM_HOST2", NULL );
  ip_addr[1].port = getTCLParam( "SBM_PORT2", 1000, 65535, 65535 );
  activeIp = 0;
  login = getTCLParam( "SBM_LOGIN", NULL );
  password = getTCLParam( "SBM_PASSWORD", NULL );
  heartBeat = getTCLParam( "SBM_HEARTBEAT_INTERVAL", 0, 60, 30 );
  ProgTrace( TRACE0, "BMConnection::init(): canon_name=%s,ip[0]=%s:%d,ip[1]=%s:%d,login=%s,password=%s,heartbeat=%d",
             canonName.c_str(), ip_addr[0].host.c_str(), ip_addr[0].port, ip_addr[1].host.c_str(), ip_addr[1].port,
             login.c_str(), password.c_str(), heartBeat );
  configured = true;
  // �᫨ ���� �������� ����室������ ������� ���䨣���� - � ᪮॥ �ᥣ� �� ���� �裡,
  // � �㦭� ����⠭�������� ������ � ᮥ�������, � �室 � ��⥬�
  connected = false;
  loginStatus = BM_LOGIN_NONE;
}

void BMConnection::connect( io_service& service )
{
  boost::system::error_code err;

  if( connected )
    return;
  time_t now = time( NULL );
  if( now <= lastAdmin + 1 ) // �⮡� �� ��㤠� socket->connect() �� ������� �����뢭�, � ������ ���� � 2 ᥪ㭤�
    return;
  lastAdmin = now;
  io = &service;
  BM_HOST *ip = & ip_addr[ activeIp ];
  ip::tcp::endpoint ep( ip::address::from_string( ip->host ), ip->port );
 // ��������: �᫨ ���� �������� ����室������ ������� ���䨣���� - � ������ ��������, �� socket 㦥 �������, ��祬 ������.
 // ���⮬� ���� �������� �� ��������� �����⭮�⥩.
  if( socket == NULL )
    socket = new ip::tcp::socket( *io );
  if( socket->is_open() )
    socket->close();
  ProgTrace( TRACE5, "try to connect" );
  socket->connect( ep, err );
  if( err.value() == 0 )
  {
    ProgTrace( TRACE0, "Connect to SITA BagMessage on %s:%d - success", ip->host.c_str(), ip->port );
    connected = true;
  }
  else
  {
    ProgError( STDLOG, "Cannot connect to SITA BagMessage on %s:%d. error=%d(%s)", ip->host.c_str(), ip->port,
               err.value(), err.message().c_str() );
    connected = false;
    activeIp = ( activeIp + 1 ) % NUMLINES; // �⮡� �� ��㤠筮� ����⪥ ��⠭����� ��� ᫥����� ����⪠ �뫠 �� ��㣮� �����
  }
}

void BMConnection::makeMessage( BM_MESSAGE_TYPE type, string message_text, int msg_id )
{
  ProgTrace( TRACE5, "makeMessage()" );
  strncpy( header.appl_id, login.c_str(), 8 );
  header.version = VERSION_2;
  header.type = type;
  if( msg_id < 0 )
  {
    header.message_id_number = mes_num;
    mes_num = ( mes_num + 1 ) % 0x10000; // ������᪨� 2-���⮢� ���稪
  }
  else
    header.message_id_number = msg_id;
  text = message_text;
  header.data_length = message_text.length();
}

bool BMConnection::doSendMessage()
{
  boost::system::error_code err;
  char buf[2048];

  ProgTrace( TRACE5, "doSendMessage(): try to send: appl_id=%8.8s version=%d type=%d(%s) id=%d data_len=%d",
             header.appl_id, header.version, header.type, messageTypes[ header.type ], header.message_id_number,
             header.data_length );
  memcpy( buf, (char *) &header, sizeof( BM_HEADER ) );
  if( header.data_length > 0 )
    memcpy( buf + sizeof( BM_HEADER ), text.c_str(), header.data_length );
  socket->send( buffer( buf, sizeof( BM_HEADER ) + header.data_length ), 0, err );
  if( err.value() != 0 )
  {
    ProgError( STDLOG, "doSendMessage(): after sending: error=%d(%s)", err.value(), err.message().c_str() );
    connected = false; // - �㦭� ⠪ ��� ���? ���� �� ����
    return false;
  }
  else
  {
    lastSendTime = time( NULL );
    if( header.type == ACK_DATA )
    {
      waitForAck = header.message_id_number;
      waitForAckTime = time( NULL );
      ProgTrace( TRACE5, "set timer for ACK_DATA - id=%d,now=%lu", waitForAck, waitForAckTime );
    }
  }
  return true;
}

bool BMConnection::sendMessage( BM_MESSAGE_TYPE type, string message_text, int msg_id )
{
  if( ! configured )
  {
    ProgError( STDLOG, "Call to sendMessage() before connection is cofigured" );
    return false;
  }
  if( waitForAck >= 0 )
  {
    ProgTrace( TRACE5, "Call to sendMessage() while waiting for data acknowledgment!!!" );
    return false;
  }
  makeMessage( type, message_text, msg_id );
  return doSendMessage();
}

bool BMConnection::sendLogin()
{
  ProgTrace( TRACE5, "sendLogin()" );
  if( sendMessage( LOGIN_RQST, password ) )
  {
    loginStatus = BM_LOGIN_WAIT;
    return true;
  }
  return false;
}

bool BMConnection::sendTlg( string text )
{
  ProgTrace( TRACE5, "sendTlg(): text=%s", text.c_str() );
  return sendMessage( DATA, text );
}

void BMConnection::sendTlgAck( int id, string text )
{
  ProgTrace( TRACE5, "sendTlgAck(): id=%d,text=%s", id, text.c_str() );
  waitForAckId = id;
  sendMessage( ACK_DATA, text );
}

void BMConnection::checkInput()
{
  boost::system::error_code err;
  size_t size, total;
  char buf[2048];
  BM_HEADER *head;
  BM_MESSAGE mes;

  if( ( total = socket->available() ) >= sizeof(BM_HEADER) ) // ���� ��������� - ���� �� ������
  {
    ProgTrace( TRACE5, "RECEIVE: %lu bytes availiable", total );
    size = read( *socket, buffer(buf), transfer_exactly( sizeof(BM_HEADER) ) );
    head = (BM_HEADER *) buf;
    ProgTrace( TRACE5, "size=%lu, head: appl_id=%8.8s version=%d type=%d(%s) id=%d data_len=%d",
               size, head->appl_id, head->version, head->type, messageTypes[ head->type ], head->message_id_number,
               head->data_length );
    memcpy( & mes.head, head, sizeof(BM_HEADER) );
    if( mes.head.data_length > 0 )
    {
      size = read( *socket, buffer(buf), transfer_exactly( mes.head.data_length ) );
      ProgTrace( TRACE5, "data: %s", buf );
      memcpy( mes.text, buf, mes.head.data_length );
    }
    lastRecvTime = time( NULL );
    switch( mes.head.type ) // ������ ��ࠡ�⪠ ⮣�, �� �易�� � ��⮪����
    {
      case LOGIN_ACCEPT:
        ProgTrace( TRACE5, "SITA BagMessage: login accepted" );
        loginStatus = BM_LOGIN_READY;
        break;
      case LOGIN_REJECT:
        ProgError( STDLOG, "SITA BagMessage: login rejected!!! Login or password may be incorrect." );
        loginStatus = BM_LOGIN_NONE;
        break;
      case DATA_OFF:
        ProgTrace( TRACE5, "SITA BagMessage: receive DATA_OFF message" );
        paused = true;
        break;
      case DATA_ON:
        ProgTrace( TRACE5, "SITA BagMessage: receive DATA_ON message" );
        paused = false;
        break;
      case STATUS:
        ProgTrace( TRACE5, "SITA BagMessage: receive STATUS message" );
        break;
      case ACK_MSG:
        ProgTrace( TRACE5, "SITA BagMessage: receive ACK_MSG for message id=%d", mes.head.message_id_number );
        if( mes.head.message_id_number == waitForAck )
        { // ���⢥ত��� ���⠢�� ᮮ�饭�� - ⮫쪮 ⥯��� ����砥� ��� ��� ���⠢������
          waitForAck = -1;
          TFileQueue::sendFile( waitForAckId );
          TFileQueue::doneFile( waitForAckId );
          OraSession.Commit();
        }
        else
        {
          ProgTrace( TRACE5, "... but we are waiting for id=%d - resending need!", waitForAck );
          doSendMessage();
        }
        break;
      case NAK_MSG:
        ProgTrace( TRACE5, "SITA BagMessage: receive NAK_MSG for message id=%d", mes.head.message_id_number );
        waitForAck = -1;
        doSendMessage();
        break;
      case ACK_DATA:
        sendMessage( ACK_MSG, "", mes.head.message_id_number ); // ���⢥न�� �ਥ�
        mes_queue.push( mes );      // ��ࠡ���� �����
        break;
      case DATA: // ��諨 ����� - �� ���� �⤥�쭮 ��ࠡ����
        mes_queue.push( mes );
        break;
      default:
        ProgError( STDLOG, "SITA BagMessage sends a message of incorrect type %d", mes.head.type );
        break;
    } // switch
  } // � �᫨ �ਭ�� ������� ��������� - ���� ���� �த������� � ��祣� �� ������
}

void BMConnection::checkTimer()
{
  time_t now = time( NULL );
  if( now - lastSendTime >= heartBeat )
  { // ����� ��祣� �� ���뫠�� - ���� ��᫠�� �����
    sendMessage( STATUS );
  }
  if( now - lastRecvTime >= 2 * heartBeat )
  { // ����� ��祣� �� ����砫� - ��⠥�, �� ����� ���㫠��
    ProgError( STDLOG, "SITA BagMessage: no input messages in last %d seconds - connection may be failed", 2 * heartBeat );
    connected = false;
    socket->close();
    loginStatus = BM_LOGIN_NONE;
    // � ��᫥ �⮣� � �᭮���� ࠡ�祬 横�� ������ ��筥��� ��⠭���� �裡 � ��祥
  }
  if( waitForAck >= 0 && now - waitForAckTime > 1 )
  { // ����ᨫ� ���⢥ত���� �ਥ��, � ��� �� ��諮 ���६�
    ProgTrace( TRACE5, "TIMEOUT! now=%lu, waitForAckTime=%lu - No acknowledgment for message id=%d - resending",
               now, waitForAckTime, waitForAck );
    waitForAck = -1;
    doSendMessage();
  }
}

void BMConnection::checkQueue()
{
  if( paused ) // ��㣠� ��⥬� ����ᨫ� ��� �������� - ���稬
    return;
  if( waitForAck > 0 ) // ���� ���⢥ত���� �ਥ�� ᮮ�饭�� - ���� ��祣� �� ���뫠��
    return;

  TFileQueue fileQueue;
  const int WAIT_ANSWER_SEC = 30;

  fileQueue.get( TFilterQueue( canonName, WAIT_ANSWER_SEC ) );
  if( ! fileQueue.empty() )
  {
    TQueueItem& tlg = *fileQueue.begin();
    ProgTrace( TRACE5, "first in queue: id=%d,receiver=%s,type=%s,time=%s",
               tlg.id, tlg.receiver.c_str(), tlg.type.c_str(), DateTimeToStr( tlg.time, "yyyy-mm-dd hh:nn:ss" ).c_str() );
    if( sendTlg( tlg.data ) )
    { // �᫨ ���뫠�� ��� ����� ���⢥ত���� - � ����� �ࠧ� �⬥���, �� ��᫠��.
      TFileQueue::sendFile( tlg.id );
      TFileQueue::doneFile( tlg.id );
      OraSession.Commit();
    }
/* � �᫨ ���뫠�� � ����ᮬ ���⢥ত���� - � �⬥⪨ "��᫠��" �ਤ���� �⫮���� �� ����祭�� ���⢥ত���� */
//    sendTlgAck( tlg.id, tlg.data );
  }
//  else
//    ProgTrace( TRACE5, "fileQueue is empty" );
}

void BMConnection::run()
{ // ������� ���� ����⢨� �� ࠡ��. ��᪮��� ��뢠���� � 横��, ����� �ࠧ� ������ ������
  if( loginStatus == BM_LOGIN_NONE )
  {
    sendLogin();
    return;
  }
  else if( loginStatus == BM_LOGIN_WAIT )
  {
    checkInput();
    return;
  }
  else if( loginStatus == BM_LOGIN_READY )
  {
    checkInput();
    checkTimer();
    checkQueue();
    return;
  }
  else
  {
    ProgError( STDLOG, "loginStatus=%d - that is not normal!", loginStatus );
  }
}

void processBagMessageMes( BM_MESSAGE& mes )
{ // �᫨ ��� ��襤襣� ᮮ�饭�� �㦭� �����-� ᢮� ��ࠡ�⪠ - � ��⠢��� �� �
  // � ���� �� ���� ����஢��
  ProgTrace( TRACE5, "processBagMessageMes: text=%s", mes.text );
}

void run_bag_msg_process( const std::string &name )
{
  BMConnection conn;
  io_service io;

#if 0
  {
  // �६���� ���� ��� ᮧ����� ��⮢�� ������
    BMConnection tmp;
    tmp.init();
    fileQueue.get( TFilterQueue( tmp.getCanonName(), "BSM" ) );
    ProgTrace( TRACE5, "fileQueue.get() returns %lu items", fileQueue.size() );
    if( ! fileQueue.empty() )
    {
      for( TFileQueue::iterator item = fileQueue.begin(); item != fileQueue.end(); ++item )
      {
        ProgTrace( TRACE5, "item: id=%d,receiver=%s,type=%s,time=%s,wait_time=%s,data=%s",
                   item->id, item->receiver.c_str(), item->type.c_str(),
                   DateTimeToStr( item->time, "yyyy-mm-dd hh:nn:ss" ).c_str(),
                   DateTimeToStr( item->wait_time, "yyyy-mm-dd hh:nn:ss" ).c_str(),
                   item->data.c_str() );
        fileQueue.putFile( item->receiver, "BETADC", "BSM", item->params, item->data );
        OraSession.Commit();
      }
    }
  }
#endif
  for(;;)
  {
    if( ! conn.isInit() )
    {
      conn.init();
    }
    else if( ! conn.isConnected() )
    {
      conn.connect( io );
    }
    else
    {
      conn.run();
    }
    io.run();
    if( ! mes_queue.empty() )
    {
      BM_MESSAGE mes = mes_queue.front();
      mes_queue.pop();
      processBagMessageMes( mes );
    }
  };
}

int main_bag_msg_handler_tcl( int supervisorSocket, int argc, char *argv[] )
{
  try
  {
    sleep(2); // !!!vlad �� ����室��� �⮡� cores �� �������� ��᪮��� ����࠭�⢮
    BagMsgQueueDaemon daemon;
    run_bag_msg_process( argc > 1 ? argv[1] : "" );
  }
  catch( std::exception &E )
  {
    ProgError( STDLOG, "std::exception: %s", E.what() );
    throw;
  }
  catch( ... )
  {
    ProgError( STDLOG, "Unknown error" );
    throw;
  };

  return 0;
}