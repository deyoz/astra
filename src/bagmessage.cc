#include <boost/asio.hpp>
#include <boost/bind.hpp>
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

/**************************************************************************/
/* ��������� ᮮ�饭��, ��ࠢ�塞��� � BagMessage ��� ����砥���� ���㤠 */
/**************************************************************************/
class BM_HEADER
{
  public:
  // �������� ��������
    string appl_id;                // �����䨪��� ������ - 8 ᨬ�����
    unsigned version;              // ����� ��������� - 2-���⮢�� �����������
    unsigned type;                 // ��� ᮮ�饭�� - 2-���⮢�� �����������
    unsigned message_id_number;    // �����䨪�樮��� ����� ᮮ�饭�� - 2-���⮢�� �����������
    unsigned data_length;          // ����� ⥪�⮢�� ������ ��᫥ ��������� - 2-���⮢�� �����������
                                   // � �� 4 ���� ��१�ࢨ஢���
  // ����⠭�� � ��⮤�
    static const int SIZE = 20;    // ����� ��।�������� ��������� � �����
    void loadFrom( char *buf );    // ����㧨�� ��������� �� ���� ��᫥ �ਥ��
    void saveTo( char *buf );      // ������� ��������� � ���� ��। ��।�祩
};

void BM_HEADER::loadFrom( char *buf )
{
  appl_id = string( buf, 8 );
// ��������: �� ��।�� �ਭ�� ���冷� little-endian, � ����७��� ���⥪��� ��襩 ��⥬� ����� �⫨�����.
  version = (unsigned char) buf[8] + ( (unsigned char) buf[9] ) * 256;
  type = (unsigned char) buf[10] + ( (unsigned char) buf[11] ) * 256;
  message_id_number = (unsigned char) buf[12] + ( (unsigned char) buf[13] ) * 256;
  data_length = (unsigned char) buf[14] + ( (unsigned char) buf[15] ) * 256;
}

void BM_HEADER::saveTo( char *buf )
{
  strncpy( buf, appl_id.c_str(), 8 );
  buf[8] = version & 0x00FF;
  buf[9] = ( version >> 8 ) & 0x00FF;
  buf[10] = type & 0x00FF;
  buf[11] = ( type >> 8 ) & 0x00FF;
  buf[12] = message_id_number & 0x00FF;
  buf[13] = ( message_id_number >> 8 ) & 0x00FF;
  buf[14] = data_length & 0x00FF;
  buf[15] = ( data_length >> 8 ) & 0x00FF;
  buf[16] = buf[17] = buf[18] = buf[19] = 0;
}

/*****************************/
/* �室��� ��।� ᮮ�饭�� */
/*****************************/
// ����饭�� ����� � ����������.
typedef struct
{
  BM_HEADER head;
  string text;
} BM_MESSAGE;

std::queue<BM_MESSAGE> mes_queue;

/******************************/
/* ��室��� ��।� ᮮ�饭�� */
/******************************/
class BM_OUTPUT_QUEUE : public TFileQueue
{
  private:
    string canonName;                      // ��� �ࢨ�, �� ���஬� ��ࠢ������ ⥫��ࠬ��
    static const int WAIT_ANSWER_SEC = 30; // ����-��� ����� �⬥⪠�� 'send' � 'done'. �᫨ ��⥪ - ᮮ�饭�� �������� ����୮� ���뫪�
    time_t lastRead;                       // ����� ��᫥���� ࠧ �⠫� ��।� �� ����
  public:
    BM_OUTPUT_QUEUE();                     // ���������
    void get();                            // ������� ��।�
    void sendFile( int tlg_id );           // ���⠢��� �⬥�� "⥫��ࠬ�� ���� �� ���뫪�"
    static void unSendFile( int tlg_id );  // ����� �⬥�� "⥫��ࠬ�� ���� �� ���뫪�"
    static void doneFile( int tlg_id );    // ���⠢��� �⬥�� "⥫��ࠬ�� ���⠢����"
    static void onWriteFinished( int tlg_id, int result ); // ��ࠡ��稪 �����襭�� ��ࠢ�� ⥫��ࠬ��
};

BM_OUTPUT_QUEUE::BM_OUTPUT_QUEUE()
{
  canonName = getTCLParam( "SBM_CANON_NAME", NULL );
  ProgTrace( TRACE0, "SITA BagMessage: canon_name=%s", canonName.c_str() );
  lastRead = 0;
}

void BM_OUTPUT_QUEUE::get()
{
  time_t now = time( NULL );
//  if( now - lastRead > 0 ) // �⮡� �� �����뢠�� ���� �� ��᪮�쪮 ࠧ � ᥪ㭤�
  {
    ProgTrace( TRACE5, "re-reading outgoing queue" );
    TFileQueue::get( TFilterQueue( canonName, WAIT_ANSWER_SEC ) );
    lastRead = now;
  }
}

void BM_OUTPUT_QUEUE::sendFile( int tlg_id )
{
  TFileQueue::sendFile( tlg_id );
  OraSession.Commit();
  lastRead = lastRead - 2; // ��� ������ ����� �� ����୮�� �⥭��
  get();
}

void BM_OUTPUT_QUEUE::unSendFile( int tlg_id )
{
  TQuery Qry(&OraSession);
  Qry.SQLText = "UPDATE file_queue SET status='PUT', time=system.UTCSYSDATE WHERE id= :id";
  Qry.CreateVariable( "id", otInteger, tlg_id );
  Qry.Execute();
  bool res = ( Qry.RowsProcessed() > 0 );
  if ( res ) {
    ProgTrace( TRACE5, "unSendFile id=%d", tlg_id );
  }
  OraSession.Commit();
}

void BM_OUTPUT_QUEUE::doneFile( int tlg_id )
{
  TFileQueue::doneFile( tlg_id );
  OraSession.Commit();
}

void BM_OUTPUT_QUEUE::onWriteFinished( int tlg_id, int error )
{
  if( error == 0 )
  {
    ProgTrace( TRACE5, "Message id=%d sent OK", tlg_id );
    doneFile( tlg_id );
  }
  else
  {
    ProgError( STDLOG, "Failed to send message to SITA BagMessage! tlg_id=%d", tlg_id );
    unSendFile( tlg_id );
  }
}

/*****************************************/
/* ���������� � ��⥬�� SITA BagMessage */
/*****************************************/
// �����஢�� ⨯�� ᮮ�饭��, ����� ����� ���� ��᫠�� � ��⥬� SITA BagMessage
const string messageTypes[] = { "unknown", "LOGIN_RQST", "LOGIN_ACCEPT", "LOGIN_REJECT", "DATA", "ACK_DATA",
                                "ACK_MSG", "NAK_MSG", "STATUS", "DATA_ON", "DATA_OFF", "LOG_OFF" };

class BMConnection
{
  public:
  // ����ﭨ� �ᨭ�஭��� ����樨
    typedef enum
    {
      BM_OP_NONE = 0,      // �� �뫮 ����⪨ ᤥ���� ������
      BM_OP_WAIT = 1,      // ������ ���� - ���� �����襭��
      BM_OP_READY = 2      // �����祭� ��ଠ�쭮
    } BM_OPERATION_STATUS;
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
    int line_number;   // ���浪��� ����� ᮥ������� � ��⥬�
    BM_HOST ip_addr[NUMLINES];  // ���� ��� ����� �裡 - �᭮���� � १�ࢭ� (१�ࢭ��?)
    string login;      // ����� � ��⥬� SITA BagMessage
    string password;   // ��஫� ⠬ ��
    int heartBeat;     // ���ᨬ���� ���ࢠ� �६��� ����� ᮮ�饭�ﬨ
  // ����稥 ��६����
  // ������
    bool configured;   // �ਧ��� ⮣�, �� ���䨣���� ���⠭�
    bool paused;       // �ਧ��� ⮣�, �� ��㣠� ��஭� ����ᨫ� ���� (�뤠�� DATA_OFF)
    BM_OPERATION_STATUS connected;    // �ਧ��� ⮣�, �� ��� ��⠭������
    BM_OPERATION_STATUS loginStatus;  // �ਧ��� ⮣�, �� �� ��諨 � �� ��⥬�
    BM_OPERATION_STATUS writeStatus;  // ����ﭨ� ��᫥���� ����樨 ��।�� ������
  // ���� � ��⮪���� ����
    ip::tcp::socket socket;  // ����� ��� �裡
    int activeIp;        // ����� �� ����� �裡 ᥩ�� �ᯮ������?
    unsigned mes_num;    // ����� ᮮ�饭�� ��� ��⮪��� �裡
    time_t lastAdmin;    // �६� ��᫥����� ����⢨� �� �ࠢ����� ����ᮬ - ��� �࣠����樨 ����থ�
  // �ਥ�
    time_t lastRecvTime; // �६� ��室� �।��饣� ᮮ�饭�� - ��᫥�������� ��� �業�� ࠡ��ᯮᮡ���� �����
    char *rbuf;          // ���� ��� �㭪樨 async_read
    BM_HEADER rheader;   // ��������� �ਭ�⮣� ᮮ�饭��
    int needSendAck;     // �㦭� ��।��� ���⢥ত���� �ਭ�⮣� ᮮ�饭�� � �⨬ ����஬
  // ��।��
    time_t lastSendTime; // �६� ��।�� �।��饣� ᮮ�饭�� - ��� ��।�� ����� �� �����
    BM_HEADER header;    // ��������� ᮮ�饭��, �ନ�㥬��� ��� ��।��
    string text;         // ����� ᮮ�饭��, �ନ�㥬��� ��� ��।��
    char *wbuf;          // ���� ��� �㭪樨 async_write
    int waitForAck;      // ��⮪���� ����� ᮮ�饭��, �� ���뫪� ���ண� ���⠢��� ����� ���⢥ত����,
                         // �⢥� �� ����� �� �� ����祭, ��� -1 �᫨ ��祣� �� ����
    int waitForAckId;    // ID ᮮ⢥�����饩 ⥫��ࠬ�� � ���� ������
    time_t waitForAckTime; // ����� ��᫠�� �� ᮮ�饭��
    void (*writeHandler)( int tlg_id, int status ); // ��ࠡ��稪 �����襭�� ��।�� � ���⢥ত�����
  public:
    BMConnection( int i, io_service& io ) : socket( io ) { line_number = i; configured = false; wbuf = NULL; rbuf = NULL; }
    ~BMConnection();
    void run();          // ������� ��-� � ࠡ�祬 横��
  private:
    bool isInit() { return configured; };      // �஢����, �� ���䨣���� ���⠭�
    void init();                               // ������ ���䨣���� � ��࠭��� � ��ꥪ�
    void connect();                            // ����������� � �ࢥ��
    void onConnect( const boost::system::error_code& err );   // ��ࠡ��稪 �����襭�� ����樨 ������祭��
    void onRead( const boost::system::error_code& error, std::size_t n ); // ��ࠡ��稪 �����襭�� ����樨 �ਥ�� ᮮ�饭��
    void onWrite( const boost::system::error_code& error, std::size_t n ); // ��ࠡ��稪 �����襭�� ����樨 ���뫪� ᮮ�饭��
    bool makeMessage( BM_MESSAGE_TYPE type, string text = "" , int msg_id = -1 );     // �����⮢��� ��������� � ⥪�� � ���뫪�
    void doSendMessage();                                      // ��᫠�� �����⮢������ ᮮ�饭��
    void sendMessage( BM_MESSAGE_TYPE type, string text = "", int msg_id = -1 );  // ��᫠�� � BagMessage ᮮ�饭�� - ��騥 ����
    void sendLogin();                     // ��᫠�� � �� ��⥬� ����� � ��஫�
    void checkInput();                    // �஢���� �室��� ��⮪ � ��-� � ��� ᤥ����
    void checkTimer();                    // �஢���� ⠩���� ᮮ�饭�� - ���� ����� ���㫠��
  public:
    int getNumber() { return line_number; }    // �뤠�� ����� ᮥ������� �����
    void sendTlg( string text );               // ��᫠�� BSM-⥫��ࠬ��
    void sendTlgAck( int id, string text, void (*handler)( int, int) );  // ��᫠�� BSM-⥫��ࠬ�� � ����ᮬ ���⢥ত���� �ਥ��
    bool readyToSend();                        // �஢����, �� ᮥ������� ����㯭� ��� ���뫪� ᮮ�饭��
};

BMConnection::~BMConnection()
{
/*
  if( loginStatus == BM_LOGIN_READY )
    sendMessage( LOG_OFF );
*/
  if( connected != BM_OP_NONE )
    socket.close();
  if( wbuf != NULL )
    delete( wbuf );
  if( rbuf != NULL )
    delete( rbuf );
}

void BMConnection::init()
{
  if( configured )
    return;
// ���� �� �⠥� ����஥�� ��ࠬ���� �� ���䨣��樮����� 䠩��
// � ���饬 �������� �⥭�� �� ��㣮�� ���筨��, �� � ⮬� �� � ����ᨬ��� �� ���祭�� line_number
//  if( line_number == 0 )
//  {
    ip_addr[0].host = getTCLParam( "SBM_HOST1", NULL );
    ip_addr[0].port = getTCLParam( "SBM_PORT1", 1000, 65535, 65535 );
    ip_addr[1].host = getTCLParam( "SBM_HOST2", NULL );
    ip_addr[1].port = getTCLParam( "SBM_PORT2", 1000, 65535, 65535 );
    activeIp = 0;
    login = getTCLParam( "SBM_LOGIN", NULL );
    password = getTCLParam( "SBM_PASSWORD", NULL );
    heartBeat = getTCLParam( "SBM_HEARTBEAT_INTERVAL", 0, 60, 30 );
/* ����� �⫠��� 2-��������� ��ਠ��. ��⮬ ����� �㤥� ����
  }
  else if( line_number == 1 )
  {
    ip_addr[0].host = getTCLParam( "SBM_HOST1", NULL );
    ip_addr[0].port = 8854;
    ip_addr[1].host = getTCLParam( "SBM_HOST2", NULL );
    ip_addr[1].port = 8855;
    activeIp = 0;
    login = getTCLParam( "SBM_LOGIN", NULL );
    password = getTCLParam( "SBM_PASSWORD", NULL );
    heartBeat = getTCLParam( "SBM_HEARTBEAT_INTERVAL", 0, 60, 30 );
  }
*/
  ProgTrace( TRACE0, "BMConnection::init(): line_number=%d,ip[0]=%s:%d,ip[1]=%s:%d,login=%s,password=%s,heartbeat=%d",
             line_number, ip_addr[0].host.c_str(), ip_addr[0].port, ip_addr[1].host.c_str(), ip_addr[1].port,
             login.c_str(), password.c_str(), heartBeat );
  configured = true;
  // �᫨ ���� �������� ����室������ ������� ���䨣���� - � ᪮॥ �ᥣ� �� ���� �裡,
  // � �㦭� ����⠭�������� ������ � ᮥ�������, � �室 � ��⥬�
  connected = BM_OP_NONE;
  loginStatus = BM_OP_NONE;
  writeStatus = BM_OP_NONE;
  paused = false;
  lastAdmin = 0;
  mes_num = 1;
  waitForAck = -1;
  needSendAck = -1;
  if( wbuf != NULL )
    delete( wbuf );
  wbuf = NULL;
  if( rbuf != NULL )
    delete( rbuf );
  rbuf = NULL;
}

void BMConnection::onConnect( const boost::system::error_code& err )
{
  BM_HOST *ip = & ip_addr[ activeIp ];
  if( err.value() == 0 )
  {
    ProgTrace( TRACE0, "Connection %d: connected to SITA BagMessage on %s:%d", line_number, ip->host.c_str(), ip->port );
    connected = BM_OP_READY;
  }
  else
  {
    ProgError( STDLOG, "Connection %d: Cannot connect to SITA BagMessage on %s:%d. error=%d(%s)", line_number,
               ip->host.c_str(), ip->port, err.value(), err.message().c_str() );
    socket.close(); // �� �ᨭ�஭��� ����樨 �� ��⠥��� ������, � ������ async_connect() �� ��室�� - ���� �������
    connected = BM_OP_NONE;
    activeIp = ( activeIp + 1 ) % NUMLINES; // �⮡� �� ��㤠筮� ����⪥ ��⠭����� ��� ᫥����� ����⪠ �뫠 �� ��㣮� �����
  }
}

void BMConnection::connect()
{
  if( connected != BM_OP_NONE ) // ���� �� ��砩���� ����୮�� �맮��
    return;
  time_t now = time( NULL );
  if( now <= lastAdmin + 1 ) // �⮡� �� ��㤠� socket.connect() �� ������� �����뢭�, � ������ ���� � 2 ᥪ㭤�
    return;
  connected = BM_OP_WAIT;
  lastAdmin = now;
  BM_HOST *ip = & ip_addr[ activeIp ];
  ip::tcp::endpoint ep( ip::address::from_string( ip->host ), ip->port );
  if( socket.is_open() ) // ��������: �᫨ ᮪�� ��祬�-� 㦥 ������ - � ��⠭���� ᮥ������� �� ��室��. ��� �� ���� �������.
    socket.close();
  ProgTrace( TRACE5, "connection %d try to connect", line_number );
  socket.async_connect( ep, boost::bind( &BMConnection::onConnect, this, boost::asio::placeholders::error) );
}

bool BMConnection::makeMessage( BM_MESSAGE_TYPE type, string message_text, int msg_id )
{
  ProgTrace( TRACE5, "makeMessage()" );
  if( message_text.length() >= 0xFFFF )
  {
    ProgError( STDLOG, "Too long text to send - %lu bytes", message_text.length() );
    return false;
  }
  header.appl_id = login;
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
  return true;
}

void BMConnection::doSendMessage()
{
  ProgTrace( TRACE5, "connection %d doSendMessage(): try to send: appl_id=%8.8s version=%d type=%d(%s) id=%d data_len=%d",
             line_number, header.appl_id.c_str(), header.version, header.type, messageTypes[ header.type ].c_str(),
             header.message_id_number, header.data_length );
  if( wbuf != NULL )
    delete( wbuf );
  wbuf = new char[ BM_HEADER::SIZE + header.data_length + 1 ];
  header.saveTo( wbuf );
  if( header.data_length > 0 )
    memcpy( wbuf + BM_HEADER::SIZE, text.c_str(), header.data_length );
  writeStatus = BM_OP_WAIT;
  async_write( socket, buffer( wbuf, BM_HEADER::SIZE + header.data_length ),
               boost::bind( &BMConnection::onWrite, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred) );
  lastSendTime = time( NULL );
  if( header.type == ACK_DATA )
  {
    waitForAck = header.message_id_number;
    waitForAckTime = time( NULL );
    ProgTrace( TRACE5, "connection %d set timer for ACK_DATA - id=%d,now=%lu", line_number, waitForAck, waitForAckTime );
  }
}

void BMConnection::onWrite( const boost::system::error_code& error, std::size_t n )
{
  delete( wbuf );
  wbuf = NULL;
  if( error.value() == 0 )
  {
    ProgTrace( TRACE5, "async_write() finished for connection %d. %lu bytes written", line_number, n );
    writeStatus = BM_OP_NONE;
    if( needSendAck > 0 ) // �⮨� ����� �� ���⢥ত���� ᮮ�饭�� - ��᫠�� ��� ��।�
    {
      sendMessage( ACK_MSG, "", needSendAck ); // ���⢥न�� �ਥ�
      needSendAck = -1;
    }
  }
  else
  { // �訡�� � ��।�� ���� ⠪ �� ���������. ���॥ �ᥣ�, �஡���� � ����. � ���� ��⠭�������� �� ������.
    ProgError( STDLOG, "async_write() finished with errors for connection %d. %lu bytes written, err=%d(%s)",
               line_number, n, error.value(), error.message().c_str() );
    connected = BM_OP_NONE;
    if( writeHandler != NULL )
    {
      (*writeHandler)( waitForAckId, error.value() );
      writeHandler = NULL;
    }
  }
}

void BMConnection::sendMessage( BM_MESSAGE_TYPE type, string message_text, int msg_id )
{
  if( ! configured )
  {
    ProgError( STDLOG, "Call to sendMessage() before connection is cofigured" );
    return;
  }
  if( waitForAck >= 0 )
  {
    ProgTrace( TRACE5, "Call to sendMessage() while waiting for data acknowledgment!!!" );
    return;
  }
  if( writeStatus != BM_OP_NONE )
  {
    ProgTrace( TRACE5, "Call to sendMessage() while previous operation is not finished" );
    return;
  }
  if( makeMessage( type, message_text, msg_id ) )
    doSendMessage();
}

void BMConnection::sendLogin()
{
  ProgTrace( TRACE5, "connection %d sendLogin()", line_number );
  sendMessage( LOGIN_RQST, password );
  loginStatus = BM_OP_WAIT;
}

void BMConnection::sendTlg( string text )
{
  ProgTrace( TRACE5, "sendTlg(): text=%s", text.c_str() );
  sendMessage( DATA, text );
}

void BMConnection::sendTlgAck( int id, string text, void (*handler)( int, int ) )
{
  ProgTrace( TRACE5, "sendTlgAck(): id=%d,text=%s", id, text.c_str() );
  waitForAckId = id;
  writeHandler = handler;
  sendMessage( ACK_DATA, text );
}

void BMConnection::onRead( const boost::system::error_code& error, std::size_t n )
{
  if( error.value() == 0 && n == rheader.data_length )
  {
    ProgTrace( TRACE5, "async_read() finished for connection %d. %lu bytes read", line_number, n );
    if( rheader.type == ACK_DATA )
    {
      needSendAck = rheader.message_id_number;
    }
    else
      needSendAck = -1;
    if( rheader.type == DATA || rheader.type == ACK_DATA )
    {
      BM_MESSAGE mes;
      mes.head = rheader;
      mes.text = string( rbuf, rheader.data_length );
      delete( rbuf );
      rbuf = NULL;
      mes_queue.push( mes ); // �������� - �ᯮ��㥬 ��������� ��६����� mes_queue. ���� �� ��।�����
    }
    else
    {
      ProgError( STDLOG, "SITA BagMessage sends a message of incorrect type %d for connection %d", rheader.type, line_number );
    }
  }
  else
  {
    ProgError( STDLOG, "async_read() finished with errors for connection %d. %lu bytes read (%d expected), err=%d(%s)",
               line_number, n, rheader.data_length, error.value(), error.message().c_str() );
  }
}

void BMConnection::checkInput()
{
  if( socket.available() >= BM_HEADER::SIZE ) // ��襫 ��������� - ���� �� ������
  {
    char buf[ BM_HEADER::SIZE + 1 ];
  // ��������: �.�. 㦥 �஢�ਫ� ����稥 �� �室� �㦭��� ������⢠ ���⮢, ᫥����� ������ ��ࠡ�⠥� ���������.
  // � ��⮬� ��� ����室����� ������ �� �ᨭ�஭���
    size_t size = read( socket, buffer(buf), transfer_exactly( BM_HEADER::SIZE ) );

    rheader.loadFrom( buf );
    ProgTrace( TRACE5, "header received on connection %d: size=%lu, head: appl_id=%8.8s version=%d type=%d(%s) id=%d data_len=%d",
               line_number, size, rheader.appl_id.c_str(), rheader.version, rheader.type, messageTypes[ rheader.type ].c_str(),
               rheader.message_id_number, rheader.data_length );
    lastRecvTime = time( NULL );
    if( rheader.data_length > 0 ) // ������ ��������� ���� �����
    {
      if( rbuf != NULL )
        delete( rbuf );
      rbuf = new char[ header.data_length + 1 ];
      async_read( socket, buffer( rbuf, header.data_length ),
                  boost::bind( &BMConnection::onRead, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred) );
    }
    else // ����饭�� ⮫쪮 �� ��������� - �㦥����, ���� ��ࠡ���� �����
    {
      switch( rheader.type ) // ������ ��ࠡ�⪠ ⮣�, �� �易�� � ��⮪����
      {
        case LOGIN_ACCEPT:
          ProgTrace( TRACE5, "SITA BagMessage: login accepted for connection %d", line_number );
          loginStatus = BM_OP_READY;
          break;
        case LOGIN_REJECT:
          ProgError( STDLOG, "SITA BagMessage: login rejected for connection %d!!! Login or password may be incorrect.", line_number );
          loginStatus = BM_OP_NONE;
          break;
        case ACK_MSG:
          ProgTrace( TRACE5, "SITA BagMessage: receive ACK_MSG for message id=%d", rheader.message_id_number );
          if( (int)rheader.message_id_number == waitForAck )
          { // ���⢥ত��� ���⠢�� ᮮ�饭�� - ⮫쪮 ⥯��� ����砥� ��� ��� ���⠢������
            waitForAck = -1;
            if( writeHandler != NULL )
            {
              (*writeHandler)( waitForAckId, 0 );
              writeHandler = NULL;
            }
          }
          else
          {
            ProgTrace( TRACE5, "... but we are waiting for id=%d - resending need!", waitForAck );
            doSendMessage();
          }
          break;
        case NAK_MSG:
          ProgTrace( TRACE5, "SITA BagMessage: receive NAK_MSG for message id=%d", rheader.message_id_number );
          waitForAck = -1;
          doSendMessage();
          break;
        case STATUS:
          ProgTrace( TRACE5, "SITA BagMessage: receive STATUS message for connection %d", line_number );
          break;
        case DATA_ON:
          ProgTrace( TRACE5, "SITA BagMessage: receive DATA_ON message for connection %d", line_number );
          paused = false;
          break;
        case DATA_OFF:
          ProgTrace( TRACE5, "SITA BagMessage: receive DATA_OFF message for connection %d", line_number );
          paused = true;
          break;
// ����饭�� ⨯�� LOGIN_RQST � LOG_OFF �� �� ����砥�, ⮫쪮 ���뫠��
// ����饭�� ⨯�� DATA � ACK_DATA �ᥣ�� ᮤ�ঠ� ����� - �� ��ࠡ��뢠�� ��᫥ ������� ����祭��
        default:
          ProgError( STDLOG, "SITA BagMessage sends a message of incorrect type %d for connection %d", rheader.type, line_number );
          break;
      } // switch
    } // if ᮮ�饭�� ⮫쪮 �� ��������� ... else ...
  } // if . � �᫨ ��襫 ������� ��������� - ���� ����, ����� �� �ਤ�� ���������.
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
    ProgError( STDLOG, "SITA BagMessage: no input messages on connection %d in last %d seconds - connection may be failed",
               line_number, 2 * heartBeat );
    socket.close();
    connected = BM_OP_NONE;
    loginStatus = BM_OP_NONE;
    paused = false;
    // � ��᫥ �⮣� � �᭮���� ࠡ�祬 横�� ������ ��筥��� ��⠭���� �裡 � ��祥
  }
  if( waitForAck >= 0 && now - waitForAckTime > 1 )
  { // ����ᨫ� ���⢥ত���� �ਥ��, � ��� �� ��諮 ���६�
    ProgTrace( TRACE5, "connection %d TIMEOUT! now=%lu, waitForAckTime=%lu - No acknowledgment for message id=%d - resending",
               line_number, now, waitForAckTime, waitForAck );
    waitForAck = -1;
    doSendMessage();
  }
}

void BMConnection::run()
{ // ������� ���� ����⢨� �� ࠡ��. ��᪮��� ��뢠���� � 横��, ����� �ࠧ� ������ ������
  if( ! isInit() )
  {
    init();
    return;
  }
  else if( connected == BM_OP_NONE )
  {
    connect();
    return;
  }
  else if( connected == BM_OP_WAIT )
  {
    return; // ���� ��� �� ��⠭������ - ��祣� ������ �� �����
  }
  else if( connected == BM_OP_READY )
  { // ���� ��⠭������ - ����� ��-� ᤥ����
    if( loginStatus == BM_OP_NONE )
    {
      sendLogin();
      return;
    }
    else if( loginStatus == BM_OP_WAIT )
    {
      checkInput();
      return;
    }
    else if( loginStatus == BM_OP_READY )
    {
      checkInput();
      checkTimer();
      return;
    }
    else
    {
      ProgError( STDLOG, "connection %d: loginStatus=%d - that is not normal!", line_number, loginStatus );
    }
  }
  else
  {
    ProgError( STDLOG, "connection %d: connected=%d - that is not normal!", line_number, connected );
  }
}

bool BMConnection::readyToSend()
{
  bool result = configured && connected == BM_OP_READY && loginStatus == BM_OP_READY && ( ! paused ) && writeStatus == BM_OP_NONE;
  return result;
}

/*****************************************/
/* ��ࠡ�⪠ ��।� ��襤�� ᮮ�饭�� */
/*****************************************/
void processBagMessageMes( BM_MESSAGE& mes )
{ // �᫨ ��� ��襤襣� ᮮ�饭�� �㦭� �����-� ᢮� ��ࠡ�⪠ - � ��⠢��� �� �
  // � ���� �� ���� ����஢��
  ProgTrace( TRACE5, "processBagMessageMes: text=%s", mes.text.c_str() );
}

/****************************/
/* �᭮��� ࠡ�稥 �㭪樨 */
/****************************/
typedef std::shared_ptr<BMConnection> pBMConnection;

void run_bag_msg_process( const std::string &name )
{
  io_service io;
  BM_OUTPUT_QUEUE queue;
  int NUM_CONNECTIONS = 1; // ������⢮ ��ࠫ������ ᮥ�������; � ���饬 ����� ���� 㢥��祭�;
// ����� � ���饬 �������� �⥭�� �⮩ ����稭� �� ���䨣��樮����� 䠩��
//  NUM_CONNECTIONS = getTCLParam( "SBM_NUM_CONNECTIONS", 1, 4, 1 );
  ProgTrace( TRACE0, "SITA BagMessage: NUM_CONNECTIONS=%d", NUM_CONNECTIONS );
  vector<pBMConnection> conn;
  for( int i = 0; i < NUM_CONNECTIONS; ++i )
    conn.push_back( pBMConnection( new BMConnection( i, io ) ) );
  vector<pBMConnection>::iterator lastSend = conn.begin();

  for(;;)
  {
    if( io.stopped() ) // ��易⥫쭮! ���� �ࢨ� ������ �� ���� ᢮�� ��।�, ��⠭������ � ����� ��祣� ������ �� �㤥�.
      io.reset();
  // ������ ࠡ�� ᮥ�������
    for( vector<pBMConnection>::iterator curr = conn.begin(); curr != conn.end(); ++curr )
      (*curr)->run();
  // ������ ࠡ�� ᠬ��� boost'�
    io.poll_one();
  // ��।� �� ��室
    queue.get();
    if( ! queue.empty() )
    {
      TQueueItem tlg = *queue.begin();
      ProgTrace( TRACE5, "first in queue: id=%d,receiver=%s,type=%s,time=%s",
                 tlg.id, tlg.receiver.c_str(), tlg.type.c_str(), DateTimeToStr( tlg.time, "yyyy-mm-dd hh:nn:ss" ).c_str() );
      vector<pBMConnection>::iterator curr = lastSend + 1;
      if( curr == conn.end() )
        curr = conn.begin();
      unsigned n = 0;
      for( ; n < conn.size(); n++ )
      {
        if( (*curr)->readyToSend() )
        {
          ProgTrace( TRACE5, "send message via connection %d", (*curr)->getNumber() );
          queue.sendFile( tlg.id );
#if 1
// ���� �� ࠡ�⠥� � ०��� "� ���⢥ত�����"
          (*curr)->sendTlgAck( tlg.id, tlg.data, BM_OUTPUT_QUEUE::onWriteFinished );
#else
// � �᫨ �㤥� ᫠�� ��� ���⢥ত���� - ⮣�� ⠪:
          (*curr)->sendTlg( tlg.data );
          BM_OUTPUT_QUEUE::doneFile( tlg.id );
#endif
          lastSend = curr;
          break;
        }
        if( curr + 1 == conn.end() )
        {
          curr = conn.begin();
        }
        else
        {
          curr = curr + 1;
        }
      }
      if( n == conn.size() )
      {
        ProgTrace( TRACE5, "No ready connections to send message" );
      }
    }
  // ��।� �� �室
    if( ! mes_queue.empty() )
    {
      BM_MESSAGE mes = mes_queue.front();
      mes_queue.pop();
      processBagMessageMes( mes );
    }
  } // ����� �᭮����� ࠡ�祣� 横��
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