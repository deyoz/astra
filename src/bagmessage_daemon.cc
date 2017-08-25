#include <serverlib/new_daemon.h>

#include "file_queue.h"
#include "astra_utils.h"

#define NICKNAME "GRIG"
#define NICKTRACE GRIG_TRACE
#include <serverlib/slogger.h>

#include "bagmessage.h"

using namespace std;
using namespace boost::asio;
using namespace BASIC::date_time;

class BagMsgQueueDaemon : public ServerFramework::NewDaemon
{
public:
    BagMsgQueueDaemon() : ServerFramework::NewDaemon("bag_msg_handler") {}
};

/*****************************/
/* Входная очередь сообщений */
/*****************************/

std::queue<BM_MESSAGE> mes_queue;

/******************************/
/* Выходная очередь сообщений */
/******************************/
class BM_OUTPUT_QUEUE : public TFileQueue
{
  private:
    string canonName;                      // Имя сервиса, по которому отправляются телеграммы
    static const int WAIT_ANSWER_SEC = 30; // Тайм-аут между отметками 'send' и 'done'. Если истек - сообщение подлежит повторной отсылке
    time_t lastRead;                       // Когда последний раз читали очередь из базы
  public:
    BM_OUTPUT_QUEUE();                     // Конструктор
    void get();                            // Перечитать очередь
    void sendFile( int tlg_id );           // Поставить отметку "телеграмма взята на отсылку"
    static void unSendFile( int tlg_id );  // Снять отметку "телеграмма взята на отсылку"
    static void doneFile( int tlg_id );    // Поставить отметку "телеграмма доставлена"
    static void onWriteFinished( int tlg_id, int result ); // Обработчик завершения отправки телеграммы
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
//  if( now - lastRead > 0 ) // Чтобы не перечитывать базу по несколько раз в секунду
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
  lastRead = lastRead - 2; // Для обмана защиты от повторного чтения
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
/* Обработка очереди пришедших сообщений */
/*****************************************/
void processBagMessageMes( BM_MESSAGE& mes )
{ // Если для пришедшего сообщения нужна какая-то своя обработка - то вставить это сюда
  // А пока что просто трассировка
  ProgTrace( TRACE5, "processBagMessageMes: text=%s", mes.text.c_str() );
}

/****************************/
/* Основные рабочие функции */
/****************************/
typedef std::shared_ptr<BMConnection> pBMConnection;

void run_bag_msg_process( const std::string &name )
{
  io_service io;
  BM_OUTPUT_QUEUE queue;
  int NUM_CONNECTIONS = 1; // Количество параллельных соединений; в будущем может быть увеличено;
// Также в будущем возможно чтение этой величины из конфигурационного файла
//  NUM_CONNECTIONS = getTCLParam( "SBM_NUM_CONNECTIONS", 1, 4, 1 );
  ProgTrace( TRACE0, "SITA BagMessage: NUM_CONNECTIONS=%d", NUM_CONNECTIONS );
  vector<pBMConnection> conn;
  for( int i = 0; i < NUM_CONNECTIONS; ++i )
    conn.push_back( pBMConnection( new BMConnection( i, io ) ) );
  vector<pBMConnection>::iterator lastSend = conn.begin();

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
// Пока что работаем в режиме "с подтверждением"
          (*curr)->sendTlgAck( tlg.id, tlg.data, BM_OUTPUT_QUEUE::onWriteFinished );
#else
// А если будем слать без подтверждения - тогда так:
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
  // Очередь на вход
  // Сначала переставить из очередей соединений в общую очередь
    for( vector<pBMConnection>::iterator curr = conn.begin(); curr != conn.end(); ++curr )
    {
      while( (*curr)->readyToReceive() )
      {
        mes_queue.push( (*curr)->getFirst() );
      }
    }
  // Теперь обработать общую очередь
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
