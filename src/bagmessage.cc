#include <boost/asio.hpp>
#include <serverlib/new_daemon.h>
#include <serverlib/tclmon.h>

#include "exceptions.h"
#include "msg_queue.h"
#include "astra_utils.h"
#include "file_queue.h"
#include "date_time.h"
//#include "bagmessage.h"

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

// Расшифровки типов сообщений, которые могут быть посланы в систему SITA BagMessage
char messageTypes[][20] = { "unknown", "LOGIN_RQST", "LOGIN_ACCEPT", "LOGIN_REJECT", "DATA", "ACK_DATA", "ACK_MSG", "NAK_MSG",
                          "STATUS", "DATA_ON", "DATA_OFF", "LOG_OFF" };

// Заголовок сообщения, отправляемого в BagMessage
typedef struct
{
  char appl_id[8];
  u_short version;
  u_short type;
  u_short message_id_number;
  u_short data_length;
  char reserved[4];
} BM_HEADER;

// Сообщение вместе с заголовком.
typedef struct
{
  BM_HEADER head;
  char text[2048];
} BM_MESSAGE;

std::queue<BM_MESSAGE> mes_queue;

class BMConnection // Соединение с системой SITA BagMessage
{
  public:
  // Статус входа в систему SITA BagMessage
    typedef enum
    {
      BM_LOGIN_NONE = 0, // Не было попытки логина
      BM_LOGIN_WAIT = 1, // Послали логин/пароль - ждем ответа
      BM_LOGIN_READY = 2 // Нас пустили в ту систему
    } BM_LOGIN_STATUS;
  // Типы сообщений, которые могут быть посланы в систему SITA BagMessage или приняты из нее.
    typedef enum
    {
      LOGIN_RQST = 1,      // Запрос входа в удаленную систему
      LOGIN_ACCEPT = 2,    // Положительный ответ на запрос входа
      LOGIN_REJECT = 3,    // Отрицательный ответ на запрос входа
      DATA = 4,            // Передача данных
      ACK_DATA = 5,        // Передача данных с запросом подтверждения приема
      ACK_MSG = 6,         // Положительный ответ на запрос подтверждения приема данных
      NAK_MSG = 7,         // Отрицательный ответ на запрос подтверждения приема данных
      STATUS = 8,          // Статус - подтверждение того, что связь не нарушена
      DATA_ON = 9,         // Отмена ранее выданного DATA_OFF
      DATA_OFF = 10,       // Просьба приостановить посылку данных
      LOG_OFF = 11         // Запрос закрытия связи
    } BM_MESSAGE_TYPE;
  // Версии заголовков сообщения
    typedef enum
    {
      VERSION_2 = 2        // Пока что только такое
    } BM_HEADER_VERSIONS;
  private:
  // Настройки, прочитанные извне при запуске
    string canonName;  // Имя сервиса, по которому отправляются телеграммы
    string host1;      // Адрес хоста для основной линии
    int port1;         // Порт для основной линии
    string host2;      // Адрес хоста для резервной линии
    int port2;         // Порт для резервной линии
    string login;      // Логин в системе SITA BagMessage
    string password;   // Пароль там же
    int heartBeat;     // Максимальный интервал времени между сообщениями
  // Рабочие переменные
    bool configured;   // Признак того, что конфигурация прочитана
    bool connected;    // Признак того, что связь установлена
    bool paused;       // Признак того, что другая сторона запросила паузу (выдала DATA_OFF)
    BM_LOGIN_STATUS loginStatus;  // Признак того, что мы вошли в ту систему
    io_service *io;          // Сервис, обеспечивающий связь
    ip::tcp::socket *socket; // Сокет для связи
    u_short mes_num;  // Номер сообщения для протокола связи
    time_t lastRecvTime; // Время прихода предыдущего сообщения - отслеживается для оценки работоспособности линии
    time_t lastSendTime; // Время передачи предыдущего сообщения - для передачи статуса при затишье
    time_t lastAdmin;    // Время последнего действия по управлению процессом - для организации задержек
    BM_HEADER header;    // Заголовок сообщения, формируемого для передачи
    string text;         // Текст сообщения, формируемого для передачи
    int waitForAck;      // Протокольный номер сообщения, при отсылке которого выставлен запрос подтверждения,
                         // ответ на который еще не получен, или -1 если ничего не ждем
    int waitForAckId;    // ID соответствующей телеграммы в базе данных
    time_t waitForAckTime; // Когда послали это сообщение
  public:
    BMConnection();
    ~BMConnection();
    void run();                            // Сделать что-то в рабочем цикле
    bool isInit() { return configured; };  // Проверить, что конфигурация прочитана
    void init();                               // Прочитать конфигурацию из запускающего скрипта и сохранить в объекте
    bool isConnected() { return connected; }   // Проверить, что есть соединение с сервером
    void connect( io_service& io );            // Подключиться к серверу
    bool sendTlg( string text );               // Послать BSM-телеграмму
    void sendTlgAck( int id, string text );    // Послать BSM-телеграмму с запросом подтверждения приема
  private:
    void makeMessage( BM_MESSAGE_TYPE type, string text = "" , int msg_id = -1 );     // Подготовить заголовок и текст к отсылке
    bool doSendMessage();                                      // Отослать подготовленное сообщение
    bool sendMessage( BM_MESSAGE_TYPE type, string text = "", int msg_id = -1 );  // Послать в BagMessage сообщение - общие дела
    bool sendLogin();                     // Послать в ту систему логин и пароль
    void checkInput();                    // Проверить входной поток и что-то с ним сделать
    void checkTimer();                    // Проверить таймеры сообщений - вдруг линия грохнулась
    void checkQueue();                    // Проверить очередь на выход, и если есть чего - то отослать.
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
  host1 = getTCLParam( "SBM_HOST1", NULL );
  port1 = getTCLParam( "SBM_PORT1", 1000, 65535, 65535 );
  host2 = getTCLParam( "SBM_HOST2", "" );
  port2 = getTCLParam( "SBM_PORT2", 1000, 65535, 65535 );
  login = getTCLParam( "SBM_LOGIN", NULL );
  password = getTCLParam( "SBM_PASSWORD", NULL );
  heartBeat = getTCLParam( "SBM_HEARTBEAT_INTERVAL", 0, 60, 30 );
  ProgTrace( TRACE0, "BMConnection::init(): canon_name=%s,host1=%s,port1=%d,host2=%s,port2=%d,login=%s,password=%s,heartbeat=%d",
             canonName.c_str(), host1.c_str(), port1, host2.c_str(), port2, login.c_str(), password.c_str(), heartBeat );
  configured = true;
  // Если вдруг возникла необходимость перечитать конфигурацию - то скорее всего был обрыв связи,
  // и нужно восстанавливать заново и соединение, и вход в систему
  connected = false;
  loginStatus = BM_LOGIN_NONE;
}

void BMConnection::connect( io_service& service )
{
  boost::system::error_code err;

  if( connected )
    return;
  time_t now = time( NULL );
  if( now <= lastAdmin + 1 ) // Чтобы при неудаче socket->connect() не долбить непрерывно, а делать паузу в 2 секунды
    return;
  lastAdmin = now;
  io = &service;
  ip::tcp::endpoint ep( ip::address::from_string( host1 ), port1 );
 // Тонкость: если вдруг возникла необходимость перечитать конфигурацию - то вполне возможно, что socket уже существует, причем открытый.
 // Поэтому надо защититься от возможных неприятностей.
  if( socket == NULL )
    socket = new ip::tcp::socket( *io );
  if( socket->is_open() )
    socket->close();
  ProgTrace( TRACE5, "try to connect" );
  socket->connect( ep, err );
  if( err.value() == 0 )
  {
    ProgTrace( TRACE5, "after connect(): error=%d(%s)", err.value(), err.message().c_str() );
    connected = true;
  }
  else
  {
    ProgError( STDLOG, "Cannot connect to SITA BagMessage. error=%d(%s)", err.value(), err.message().c_str() );
    connected = false;
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
    mes_num = ( mes_num + 1 ) % 0x10000; // Циклический 2-байтовый счетчик
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
    connected = false; // - Нужно так или нет? Пока не знаю
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

  if( ( total = socket->available() ) >= sizeof(BM_HEADER) ) // Есть заголовок - есть что делать
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
    switch( mes.head.type ) // Быстрая обработка того, что связано с протоколом
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
        { // Подтверждена доставка сообщения - только теперь помечаем его как доставленное
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
        sendMessage( ACK_MSG, "", mes.head.message_id_number ); // подтвердить прием
        mes_queue.push( mes );      // обработать данные
        break;
      case DATA: // пришли данные - их надо отдельно обработать
        mes_queue.push( mes );
        break;
      default:
        ProgError( STDLOG, "SITA BagMessage sends a message of incorrect type %d", mes.head.type );
        break;
    } // switch
  } // А если принят неполный заголовок - просто ждем продолжения и ничего не делаем
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
    ProgError( STDLOG, "SITA BagMessage: no input messages in last %d seconds - connection may be failed", 2 * heartBeat );
    connected = false;
    socket->close();
    loginStatus = BM_LOGIN_NONE;
    // И после этого в основном рабочем цикле заново начнется установка связи и прочее
  }
  if( waitForAck >= 0 && now - waitForAckTime > 1 )
  { // Запросили подтверждение приема, а оно не пришло вовремя
    ProgTrace( TRACE5, "TIMEOUT! now=%lu, waitForAckTime=%lu - No acknowledgment for message id=%d - resending",
               now, waitForAckTime, waitForAck );
    waitForAck = -1;
    doSendMessage();
  }
}

void BMConnection::checkQueue()
{
  if( paused ) // Другая система попросила нас помолчать - молчим
    return;
  if( waitForAck > 0 ) // Ждем подтверждения приема сообщения - пока ничего не посылаем
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
    { // Если посылаем без запроса подтверждения - то можно сразу отмечать, что отослано.
      TFileQueue::sendFile( tlg.id );
      TFileQueue::doneFile( tlg.id );
      OraSession.Commit();
    }
/* А если посылаем с запросом подтверждения - то отметки "отослано" придется отложить до получения подтверждения */
//    sendTlgAck( tlg.id, tlg.data );
  }
//  else
//    ProgTrace( TRACE5, "fileQueue is empty" );
}

void BMConnection::run()
{ // Сделать одно действие по работе. Поскольку вызывается в цикле, можно сразу делать возврат
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
{ // Если для пришедшего сообщения нужна какая-то своя обработка - то вставить это сюда
  // А пока что просто трассировка
  ProgTrace( TRACE5, "processBagMessageMes: text=%s", mes.text );
}

void run_bag_msg_process( const std::string &name )
{
  BMConnection conn;
  io_service io;

#if 0
  {
  // Временный блок для создания тестовых данных
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
    sleep(2); // !!!vlad это необходимо чтобы cores не забивали дисковое пространство
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