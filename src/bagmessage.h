#ifndef _BAGMESSAGE_H_
#define _BAGMESSAGE_H_

#include <string>
#include <queue>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

/**************************************************************************/
/* Заголовок сообщения, отправляемого в BagMessage или получаемого оттуда */
/**************************************************************************/
class BM_HEADER
{
  public:
  // Элементы структуры
    std::string appl_id;           // Идентификация клиента - 8 символов
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

// Сообщение вместе с заголовком.
typedef struct
{
  BM_HEADER head;
  std::string text;
} BM_MESSAGE;

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
      BM_OP_READY = 2      // Закончено нормально
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
  private:
    static const std::string messageTypes[12];
  public:
  // Версии заголовков сообщения
    typedef enum
    {
      VERSION_2 = 2        // Пока что только такое
    } BM_HEADER_VERSIONS;
  private:
    typedef struct
    {
      std::string host;    // IP-адрес хоста
      int port;            // Порт
    } BM_HOST;
    static const int NUMLINES = 2;
  // Настройки, прочитанные извне при запуске
    int line_number;            // Порядковый номер соединения в системе
    BM_HOST ip_addr[NUMLINES];  // Адреса для линий связи - основной и резервный (резервнЫЕ?)
    std::string login;          // Логин в системе SITA BagMessage
    std::string password;       // Пароль там же
    int heartBeat;              // Максимальный интервал времени между сообщениями
  // Рабочие переменные
  // Статусы
    bool configured;                  // Признак того, что конфигурация прочитана
    bool paused;                      // Признак того, что другая сторона запросила паузу (выдала DATA_OFF)
    BM_OPERATION_STATUS connected;    // Признак того, что связь установлена
    BM_OPERATION_STATUS loginStatus;  // Признак того, что мы вошли в ту систему
    BM_OPERATION_STATUS writeStatus;  // Состояние последней операции передачи данных
  // Связь и протокольные дела
    boost::asio::ip::tcp::socket socket;  // Сокет для связи
    int activeIp;                         // Какая из линий связи сейчас используется?
    unsigned mes_num;                     // Номер сообщения для протокола связи
    time_t lastAdmin;                     // Время последнего действия по управлению процессом - для организации задержек
    boost::posix_time::ptime adminStartTime; // Точное время последней операции по управлению - для оценки времени ее выполнения
  // Прием
    time_t lastRecvTime; // Время прихода предыдущего сообщения - отслеживается для оценки работоспособности линии
    char *rbuf;          // Буфер для функции async_read
    BM_HEADER rheader;   // Заголовок принятого сообщения
    int needSendAck;     // Нужно передать подтверждение принятого сообщения с этим номером
    std::queue<BM_MESSAGE> inputQueue; // Очередь принятых сообщений, еще не переднанных на обработку
  // Передача
    time_t lastSendTime; // Время передачи предыдущего сообщения - для передачи статуса при затишье
    BM_HEADER header;    // Заголовок сообщения, формируемого для передачи
    std::string text;         // Текст сообщения, формируемого для передачи
    char *wbuf;          // Буфер для функции async_write
    int waitForAck;      // Протокольный номер сообщения, при отсылке которого выставлен запрос подтверждения,
                         // ответ на который еще не получен, или -1 если ничего не ждем
    int waitForAckId;    // ID соответствующей телеграммы в базе данных
    time_t waitForAckTime; // Когда послали это сообщение
    void (*writeHandler)( int tlg_id, int status ); // Обработчик завершения передачи с подтверждением
    boost::posix_time::ptime sendStartTime; // Время начала операции передачи - для точного вычисления времени ее выполнения
  public:
    BMConnection( int i, boost::asio::io_service& io );
    ~BMConnection();
    void run();          // Сделать что-то в рабочем цикле
  private:
    bool isInit() { return configured; };      // Проверить, что конфигурация прочитана
    void init();                               // Прочитать конфигурацию и сохранить в объекте
    void connect();                            // Подключиться к серверу
    void onConnect( const boost::system::error_code& err );                // Обработчик завершения операции подключения
    void onRead( const boost::system::error_code& error, std::size_t n );  // Обработчик завершения операции приема сообщения
    void onWrite( const boost::system::error_code& error, std::size_t n ); // Обработчик завершения операции посылки сообщения
    bool makeMessage( BM_MESSAGE_TYPE type, std::string text = "" , int msg_id = -1 );  // Подготовить заголовок и текст к отсылке
    void doSendMessage();                                                               // Отослать подготовленное сообщение
    void sendMessage( BM_MESSAGE_TYPE type, std::string text = "", int msg_id = -1 );   // Послать в BagMessage сообщение - общие дела
    void sendLogin();                     // Послать в ту систему логин и пароль
    void checkInput();                    // Проверить входной поток и что-то с ним сделать
    void checkTimer();                    // Проверить таймеры сообщений - вдруг линия грохнулась
  public:
    int getNumber() { return line_number; }       // Выдать номер соединения наружу
    void sendTlg( std::string text );             // Послать BSM-телеграмму
    void sendTlgAck( int id, std::string text, void (*handler)( int, int) );  // Послать BSM-телеграмму с запросом подтверждения приема
    bool readyToSend();                           // Проверить, что соединение доступно для отсылки сообщений
    bool readyToReceive();                        // Проверить, что есть полностью принятые сообщения
    BM_MESSAGE getFirst();                        // Взять первое принятое сообщение
};

#endif // _BAGMESSAGE_H_