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

// Расшифровки типов сообщений, которые могут быть посланы в систему SITA BagMessage
char messageTypes[][20] = { "unknown", "LOGIN_RQST", "LOGIN_ACCEPT", "LOGIN_REJECT", "DATA", "ACK_DATA", "ACK_MSG", "NAK_MSG",
                          "STATUS", "DATA_ON", "DATA_OFF", "LOG_OFF" };

/**************************************************************************/
/* Заголовок сообщения, отправляемого в BagMessage или получаемого оттуда */
/**************************************************************************/
class BM_HEADER
{
  public:
  // Элементы структуры
    string appl_id;                // Идентификация клиента - 8 символов
    unsigned version;              // Версия заголовка - 2-байтовое беззнаковое
    unsigned type;                 // Тип сообщения - 2-байтовое беззнаковое
    unsigned message_id_number;    // Идентификационный номер сообщения - 2-байтовое беззнаковое
    unsigned data_length;          // Длина текстовых данных после заголовка - 2-байтовое беззнаковое
                                   // И еще 4 байта зарезервированы
  // Константы и методы
    static const int SIZE = 20;    // Длина передаваемого заголовка в байтах
    void loadFrom( char *buf );    // Загрузить заголовок из буфера после приема
    void saveTo( char *buf );      // Записать заголовок в буфер перед передачей
};

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

/*****************************/
/* Входная очередь сообщений */
/*****************************/
// Сообщение вместе с заголовком.
typedef struct
{
  BM_HEADER head;
  string text;
} BM_MESSAGE;

std::queue<BM_MESSAGE> mes_queue;

/******************************/
/* Выходная очередь сообщений */
/******************************/
class BM_OUTPUT_QUEUE
{
  private:
    string canonName;                      // Имя сервиса, по которому отправляются телеграммы
    TFileQueue fileQueue;                  // Сообщения в очереди
    static const int WAIT_ANSWER_SEC = 30; // Тайм-аут между отметками 'send' и 'done'. Если истек - сообщение подлежит повторной отсылке
    time_t lastRead;                       // Когда последний раз читали очередь из базы
  public:
    BM_OUTPUT_QUEUE();                                   // Конструктор
    bool empty() { return fileQueue.empty(); }           // Очередь пустая?
    TQueueItem& begin() { return * fileQueue.begin(); }  // Взять первый элемент из очереди
    void get();                                          // Перечитать очередь
    void sendFile( int tlg_id );                         // Поставить отметку "телеграмма взята на отсылку"
    static void doneFile( int tlg_id );                  // Поставить отметку "телеграмма доставлена"
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
  if( now - lastRead > 0 ) // Чтобы не перечитывать базу по несколько раз в секунду
  {
    ProgTrace( TRACE5, "re-reading outgoing queue" );
    fileQueue.get( TFilterQueue( canonName, WAIT_ANSWER_SEC ) );
    lastRead = now;
  }
}

void BM_OUTPUT_QUEUE::sendFile( int tlg_id )
{
  TFileQueue::sendFile( tlg_id );
  OraSession.Commit();
  lastRead = lastRead - 2; // Для обмана защиты от повторного чтения
  get();
}

void BM_OUTPUT_QUEUE::doneFile( int tlg_id )
{
  TFileQueue::doneFile( tlg_id );
  OraSession.Commit();
}

/*****************************************/
/* Соединение с системой SITA BagMessage */
/*****************************************/
class BMConnection
{
  public:
  // Состояние асинхронной операции
    typedef enum
    {
      BM_OP_NONE = 0,      // Не было попытки сделать операцию
      BM_OP_WAIT = 1,      // Операция начата - ждем завершения
      BM_OP_READY = 2      // Закончено
    } BM_OPERATION_STATUS;
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
    typedef struct
    {
      string host;        // IP-адрес хоста
      int port;           // Порт
    } BM_HOST;
    static const int NUMLINES = 2;
  // Настройки, прочитанные извне при запуске
    int line_number;   // Порядковый номер соединения в системе
    BM_HOST ip_addr[NUMLINES];  // Адреса для линий связи - основной и резервный (резервнЫЕ?)
    string login;      // Логин в системе SITA BagMessage
    string password;   // Пароль там же
    int heartBeat;     // Максимальный интервал времени между сообщениями
  // Рабочие переменные
  // Статусы
    bool configured;   // Признак того, что конфигурация прочитана
    bool paused;       // Признак того, что другая сторона запросила паузу (выдала DATA_OFF)
    BM_OPERATION_STATUS connected;    // Признак того, что связь установлена
    BM_OPERATION_STATUS loginStatus;  // Признак того, что мы вошли в ту систему
    BM_OPERATION_STATUS writeStatus;  // Состояние последней операции передачи данных
  // Связь и протокольные дела
    ip::tcp::socket socket;  // Сокет для связи
    int activeIp;        // Какая из линий связи сейчас используется?
    unsigned mes_num;    // Номер сообщения для протокола связи
    time_t lastAdmin;    // Время последнего действия по управлению процессом - для организации задержек
  // Передача
    time_t lastRecvTime; // Время прихода предыдущего сообщения - отслеживается для оценки работоспособности линии
    char *rbuf;          // Буфер для функции async_read
    BM_HEADER rheader;   // Заголовок принятого сообщения
    int needSendAck;     // Нужно передать подтверждение принятого сообщения с этим номером
  // Прием
    time_t lastSendTime; // Время передачи предыдущего сообщения - для передачи статуса при затишье
    BM_HEADER header;    // Заголовок сообщения, формируемого для передачи
    string text;         // Текст сообщения, формируемого для передачи
    char *wbuf;          // Буфер для функции async_write
    int waitForAck;      // Протокольный номер сообщения, при отсылке которого выставлен запрос подтверждения,
                         // ответ на который еще не получен, или -1 если ничего не ждем
    int waitForAckId;    // ID соответствующей телеграммы в базе данных
    time_t waitForAckTime; // Когда послали это сообщение
  public:
    BMConnection( int i, io_service& io ) : socket( io ) { line_number = i; configured = false; wbuf = NULL; rbuf = NULL; }
    ~BMConnection();
    void run();          // Сделать что-то в рабочем цикле
  private:
    bool isInit() { return configured; };      // Проверить, что конфигурация прочитана
    void init();                               // Прочитать конфигурацию и сохранить в объекте
    void connect();                            // Подключиться к серверу
    void onConnect( const boost::system::error_code& err );   // Обработчик завершения операции подключения
    void onRead( const boost::system::error_code& error, std::size_t n ); // Обработчик завершения операции приема сообщения
    void onWrite( const boost::system::error_code& error, std::size_t n ); // Обработчик завершения операции посылки сообщения
    bool makeMessage( BM_MESSAGE_TYPE type, string text = "" , int msg_id = -1 );     // Подготовить заголовок и текст к отсылке
    void doSendMessage();                                      // Отослать подготовленное сообщение
    void sendMessage( BM_MESSAGE_TYPE type, string text = "", int msg_id = -1 );  // Послать в BagMessage сообщение - общие дела
    void sendLogin();                     // Послать в ту систему логин и пароль
    void checkInput();                    // Проверить входной поток и что-то с ним сделать
    void checkTimer();                    // Проверить таймеры сообщений - вдруг линия грохнулась
  public:
    int getNumber() { return line_number; }    // Выдать номер соединения наружу
    void sendTlg( string text );               // Послать BSM-телеграмму
    void sendTlgAck( int id, string text );    // Послать BSM-телеграмму с запросом подтверждения приема
    bool readyToSend() { return configured && connected == BM_OP_READY && loginStatus == BM_OP_READY && ( ! paused )
                             && writeStatus == BM_OP_NONE; } // Проверить, что соединение доступно для отсылки сообщений
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
// Пока что читаем настроечные параметры из конфигурационного файла
// В будущем возможно чтение из другого источника, да к тому же в зависимости от значения line_number
  ip_addr[0].host = getTCLParam( "SBM_HOST1", NULL );
  ip_addr[0].port = getTCLParam( "SBM_PORT1", 1000, 65535, 65535 );
  ip_addr[1].host = getTCLParam( "SBM_HOST2", NULL );
  ip_addr[1].port = getTCLParam( "SBM_PORT2", 1000, 65535, 65535 );
  activeIp = 0;
  login = getTCLParam( "SBM_LOGIN", NULL );
  password = getTCLParam( "SBM_PASSWORD", NULL );
  heartBeat = getTCLParam( "SBM_HEARTBEAT_INTERVAL", 0, 60, 30 );
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
             line_number, header.appl_id.c_str(), header.version, header.type, messageTypes[ header.type ], header.message_id_number,
             header.data_length );
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
  ProgTrace( TRACE5, "async_write() finished for connection %d. n=%lu bytes written, err=%d(%s)",
             line_number, n, error.value(), error.message().c_str() );
  delete( wbuf );
  wbuf = NULL;
  if( error.value() == 0 )
  {
    writeStatus = BM_OP_NONE;
    if( needSendAck > 0 ) // Стоит запрос на подтверждение сообщения - отослать вне очереди
    {
      sendMessage( ACK_MSG, "", needSendAck ); // подтвердить прием
      needSendAck = -1;
    }
  }
  else
  { // Ошибки в передаче просто так не возникают. Скорее всего, проблемы со связью. И надо устанавливать ее заново.
    connected = BM_OP_NONE;
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

void BMConnection::sendTlgAck( int id, string text )
{
  ProgTrace( TRACE5, "sendTlgAck(): id=%d,text=%s", id, text.c_str() );
  waitForAckId = id;
  sendMessage( ACK_DATA, text );
}

void BMConnection::onRead( const boost::system::error_code& error, std::size_t n )
{
  ProgTrace( TRACE5, "async_read() finished for connection %d. n=%lu bytes read, err=%d(%s)",
             line_number, n, error.value(), error.message().c_str() );

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
    mes_queue.push( mes );
  }
  else
  {
    ProgError( STDLOG, "SITA BagMessage sends a message of incorrect type %d for connection %d", rheader.type, line_number );
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
               line_number, size, rheader.appl_id.c_str(), rheader.version, rheader.type, messageTypes[ rheader.type ],
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
      switch( rheader.type ) // Быстрая обработка того, что связано с протоколом
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
          { // Подтверждена доставка сообщения - только теперь помечаем его как доставленное
            waitForAck = -1;
            BM_OUTPUT_QUEUE::doneFile( waitForAckId );
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

void processBagMessageMes( BM_MESSAGE& mes )
{ // Если для пришедшего сообщения нужна какая-то своя обработка - то вставить это сюда
  // А пока что просто трассировка
  ProgTrace( TRACE5, "processBagMessageMes: text=%s", mes.text.c_str() );
}

/****************************/
/* Основные рабочие функции */
/****************************/
typedef std::shared_ptr<BMConnection> pBMConnection;
static const int NUM_CONNECTIONS = 1; // Количество параллельных соединений; в будущем может быть увеличено;
                                      // Также в будущем возможно чтение этой величины из конфигурационного файла

void run_bag_msg_process( const std::string &name )
{
  io_service io;
  BM_OUTPUT_QUEUE queue;
/* Это на будущее, когда будет несколько параллельных соединений */
  vector<pBMConnection> conn;
  for( int i = 0; i < NUM_CONNECTIONS; ++i )
    conn.push_back( pBMConnection( new BMConnection( i, io ) ) );

  for(;;)
  {
    if( io.stopped() ) // Обязательно! Иначе сервис дойдет до конца своей очереди, остановится и больше ничего делать не будет.
      io.reset();
  // Текущая работа соединений
    for( vector<pBMConnection>::iterator curr = conn.begin(); curr != conn.end(); ++curr )
      (*curr)->run();
  // Текущая работа самого boost'а
    io.poll_one();
  // Очередь на выход
    queue.get();
    if( ! queue.empty() )
    {
      TQueueItem tlg = queue.begin();
      ProgTrace( TRACE5, "first in queue: id=%d,receiver=%s,type=%s,time=%s",
                 tlg.id, tlg.receiver.c_str(), tlg.type.c_str(), DateTimeToStr( tlg.time, "yyyy-mm-dd hh:nn:ss" ).c_str() );
      vector<pBMConnection>::iterator curr = conn.begin();
      for( ; curr != conn.end(); ++curr )
      {
        if( (*curr)->readyToSend() )
        {
          ProgTrace( TRACE5, "send it via connection %d", (*curr)->getNumber() );
          queue.sendFile( tlg.id );
#if 1
// Пока что работаем в режиме "с подтверждением"
          (*curr)->sendTlgAck( tlg.id, tlg.data );
#else
// А если будем слать без подтверждения - тогда так:
          (*curr)->sendTlg( tlg.data );
          BM_OUTPUT_QUEUE::doneFile( tlg.id );
#endif
          break;
        }
      }
      if( curr == conn.end() )
      {
        ProgTrace( TRACE5, "No ready connections to send message" );
      }
    }
  // Очередь на вход
    if( ! mes_queue.empty() )
    {
      BM_MESSAGE mes = mes_queue.front();
      mes_queue.pop();
      processBagMessageMes( mes );
    }
  } // Конец основного рабочего цикла
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