#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "astra_utils.h"

#define NICKNAME "GRIG"
#define NICKTRACE GRIG_TRACE
#include <serverlib/slogger.h>

using namespace std;
using namespace boost::asio;
using namespace boost::posix_time;

#include "bagmessage.h"

/**************************************************************************/
/* Заголовок сообщения, отправляемого в BagMessage или получаемого оттуда */
/**************************************************************************/
void BM_HEADER::loadFrom( char *buf )
{
  appl_id = string( buf, 8 );
// ТОНКОСТЬ: при передаче принят порядок little-endian, а внутренняя архитектура нашей системы может отличаться.
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

/*****************************************/
/* Соединение с системой SITA BagMessage */
/*****************************************/
// Расшифровки типов сообщений, которые могут быть посланы в систему SITA BagMessage

const string BMConnection::messageTypes[12] = { "unknown", "LOGIN_RQST", "LOGIN_ACCEPT", "LOGIN_REJECT", "DATA", "ACK_DATA",
                                                "ACK_MSG", "NAK_MSG", "STATUS", "DATA_ON", "DATA_OFF", "LOG_OFF" };

BMConnection::BMConnection( int i, boost::asio::io_service& io ) : socket( io )
{
  line_number = i;
  configured = false;
  wbuf = NULL;
  rbuf = NULL;
}

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
// Пока что читаем настроечные параметры из конфигурационного файла
// В будущем возможно чтение из другого источника, да к тому же в зависимости от значения line_number
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
/* Следы отладки 2-линейного варианта. Потом можно будет убрать
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
  // Если вдруг возникла необходимость перечитать конфигурацию - то скорее всего был обрыв связи,
  // и нужно восстанавливать заново и соединение, и вход в систему
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
  time_duration cd = microsec_clock::local_time() - adminStartTime;
  BM_HOST *ip = & ip_addr[ activeIp ];
  if( err.value() == 0 )
  {
    ProgTrace( TRACE0, "Connection %d: connected to SITA BagMessage on %s:%d (after wait for %ld ms)",
               line_number, ip->host.c_str(), ip->port, cd.total_milliseconds() );
    connected = BM_OP_READY;
  }
  else
  {
    ProgError( STDLOG, "Connection %d: Cannot connect to SITA BagMessage on %s:%d. error=%d(%s) wait=%ld ms", line_number,
               ip->host.c_str(), ip->port, err.value(), err.message().c_str(), cd.total_milliseconds() );
    socket.close(); // При асинхронной операции он остается открытым, и повторный async_connect() не проходит - надо закрыть
    connected = BM_OP_NONE;
    activeIp = ( activeIp + 1 ) % NUMLINES; // Чтобы при неудачной попытке установить связь следующая попытка была по другой линии
  }
}

void BMConnection::connect()
{
  if( connected != BM_OP_NONE ) // Защита от случайного повторного вызова
    return;
  time_t now = time( NULL );
  if( now <= lastAdmin + 1 ) // Чтобы при неудаче socket.connect() не долбить непрерывно, а делать паузу в 2 секунды
    return;
  connected = BM_OP_WAIT;
  lastAdmin = now;
  BM_HOST *ip = & ip_addr[ activeIp ];
  ip::tcp::endpoint ep( ip::address::from_string( ip->host ), ip->port );
  if( socket.is_open() ) // Тонкость: если сокет почему-то уже открытый - то установка соединения не проходит. Так что надо закрыть.
    socket.close();
  ProgTrace( TRACE5, "connection %d try to connect", line_number );
  adminStartTime = microsec_clock::local_time();
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
    mes_num = ( mes_num + 1 ) % 0x10000; // Циклический 2-байтовый счетчик
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
  lastSendTime = time( NULL );
  sendStartTime = microsec_clock::local_time();
  async_write( socket, buffer( wbuf, BM_HEADER::SIZE + header.data_length ),
               boost::bind( &BMConnection::onWrite, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred) );
  if( header.type == ACK_DATA )
  {
    waitForAck = header.message_id_number;
    waitForAckTime = time( NULL );
    ProgTrace( TRACE5, "connection %d set timer for ACK_DATA - id=%d,now=%lu", line_number, waitForAck, waitForAckTime );
  }
}

void BMConnection::onWrite( const boost::system::error_code& error, std::size_t n )
{
  time_duration cd = microsec_clock::local_time() - sendStartTime;
  delete( wbuf );
  wbuf = NULL;
  if( error.value() == 0 )
  {
    ProgTrace( TRACE5, "async_write() finished for connection %d. %lu bytes written, %ld ms spent",
               line_number, n, cd.total_milliseconds() );
    writeStatus = BM_OP_NONE;
    if( needSendAck > 0 ) // Стоит запрос на подтверждение сообщения - отослать вне очереди
    {
      sendMessage( ACK_MSG, "", needSendAck ); // подтвердить прием
      needSendAck = -1;
    }
  }
  else
  { // Ошибки в передаче просто так не возникают. Скорее всего, проблемы со связью. И надо устанавливать ее заново.
    ProgError( STDLOG, "async_write() finished with errors for connection %d. %lu bytes written, err=%d(%s). %ld ms spent ",
               line_number, n, error.value(), error.message().c_str(), cd.total_milliseconds() );
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
      inputQueue.push( mes );
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
  if( socket.available() >= BM_HEADER::SIZE ) // Пришел заголовок - есть что делать
  {
    char buf[ BM_HEADER::SIZE + 1 ];
  // Тонкость: т.к. уже проверили наличие на входе нужного количества байтов, следующая операция отработает мгновенно.
  // И потому нет необходимости делать ее асинхронной
    size_t size = read( socket, buffer(buf), transfer_exactly( BM_HEADER::SIZE ) );

    rheader.loadFrom( buf );
    ProgTrace( TRACE5, "header received on connection %d: size=%lu, head: appl_id=%8.8s version=%d type=%d(%s) id=%d data_len=%d",
               line_number, size, rheader.appl_id.c_str(), rheader.version, rheader.type, messageTypes[ rheader.type ].c_str(),
               rheader.message_id_number, rheader.data_length );
    lastRecvTime = time( NULL );
    if( rheader.data_length > 0 ) // Помимо заголовка есть данные
    {
      if( rbuf != NULL )
        delete( rbuf );
      rbuf = new char[ header.data_length + 1 ];
      async_read( socket, buffer( rbuf, header.data_length ),
                  boost::bind( &BMConnection::onRead, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred) );
    }
    else // Сообщение только из заголовка - служебное, надо обработать здесь
    {
      time_duration cd = microsec_clock::local_time() - sendStartTime;
      switch( rheader.type ) // Быстрая обработка того, что связано с протоколом
      {
        case LOGIN_ACCEPT:
          ProgTrace( TRACE5, "SITA BagMessage: login accepted for connection %d after %ld ms waiting",
                     line_number, cd.total_milliseconds() );
          loginStatus = BM_OP_READY;
          break;
        case LOGIN_REJECT:
          ProgError( STDLOG, "SITA BagMessage: login rejected for connection %d!!! Login or password may be incorrect.", line_number );
          loginStatus = BM_OP_NONE;
          break;
        case ACK_MSG:
          ProgTrace( TRACE5, "SITA BagMessage: receive ACK_MSG for message id=%d after %ld ms waiting",
                     rheader.message_id_number, cd.total_milliseconds() );
          if( (int)rheader.message_id_number == waitForAck )
          { // Подтверждена доставка сообщения - только теперь помечаем его как доставленное
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
          ProgTrace( TRACE5, "SITA BagMessage: receive NAK_MSG for message id=%d after %ld ms waiting",
                     rheader.message_id_number, cd.total_milliseconds() );
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
// Сообщения типов LOGIN_RQST и LOG_OFF мы не получаем, только посылаем
// Сообщения типов DATA и ACK_DATA всегда содержат данные - их обрабатываем после полного получения
        default:
          ProgError( STDLOG, "SITA BagMessage sends a message of incorrect type %d for connection %d", rheader.type, line_number );
          break;
      } // switch
    } // if сообщение только из заголовка ... else ...
  } // if . А если пришел неполный заголовок - просто ждем, когда он придет полностью.
}

void BMConnection::checkTimer()
{
  time_t now = time( NULL );
  if( now - lastSendTime >= heartBeat )
  { // давно ничего не посылали - надо послать статус
    sendMessage( STATUS );
  }
  if( now - lastRecvTime >= 2 * heartBeat )
  { // давно ничего не получали - считаем, что линия грохнулась
    ProgError( STDLOG, "SITA BagMessage: no input messages on connection %d in last %d seconds - connection may be failed",
               line_number, 2 * heartBeat );
    socket.close();
    connected = BM_OP_NONE;
    loginStatus = BM_OP_NONE;
    paused = false;
    // И после этого в основном рабочем цикле заново начнется установка связи и прочее
  }
  if( waitForAck >= 0 && now - waitForAckTime > 1 )
  { // Запросили подтверждение приема, а оно не пришло вовремя
    ProgTrace( TRACE5, "connection %d TIMEOUT! now=%lu, waitForAckTime=%lu - No acknowledgment for message id=%d - resending",
               line_number, now, waitForAckTime, waitForAck );
    waitForAck = -1;
    doSendMessage();
  }
}

void BMConnection::run()
{ // Сделать одно действие по работе. Поскольку вызывается в цикле, можно сразу делать возврат
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
    return; // Пока связь не установлена - ничего делать не можем
  }
  else if( connected == BM_OP_READY )
  { // Связь установлена - можно что-то сделать
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

bool BMConnection::readyToReceive()
{
  return ! inputQueue.empty();
}

BM_MESSAGE BMConnection::getFirst()
{
  BM_MESSAGE mes = inputQueue.front();
  inputQueue.pop();
  return mes;
}
