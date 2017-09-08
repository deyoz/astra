#include <serverlib/new_daemon.h>

#include "file_queue.h"
#include "astra_utils.h"

#define NICKNAME "GRIG"
#define NICKTRACE GRIG_TRACE
#include <serverlib/slogger.h>
#include <serverlib/ourtime.h>

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
/* �室��� ��।� ᮮ�饭�� */
/*****************************/

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
  if( now - lastRead > 0 ) // �⮡� �� �����뢠�� ���� �� ��᪮�쪮 ࠧ � ᥪ㭤�
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

void run_bag_msg_process( const char *cmd, const std::string &name )
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
    InitLogTime( cmd ); // �⮡� �६� � ��� �ࠢ��쭮 ��ᠫ���.
  // ������ ࠡ�� ᮥ�������
    for( vector<pBMConnection>::iterator curr = conn.begin(); curr != conn.end(); ++curr )
      (*curr)->run();
  // ������ ࠡ�� ᠬ��� boost'�
    io.poll_one();
  // ��।� �� ��室
    vector<pBMConnection>::iterator curr = lastSend + 1;
    if( curr == conn.end() )
      curr = conn.begin();
    unsigned n = 0;
    for( ; n < conn.size(); n++ )
    {
      if( (*curr)->readyToSend() )
      {
        queue.get();
        if( ! queue.empty() )
        {
          TQueueItem tlg = *queue.begin();
          ProgTrace( TRACE5, "first in queue: id=%d,receiver=%s,type=%s,time=%s",
                     tlg.id, tlg.receiver.c_str(), tlg.type.c_str(), DateTimeToStr( tlg.time, "yyyy-mm-dd hh:nn:ss" ).c_str() );
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
    if( n == conn.size() && ! queue.empty() )
    {
      ProgTrace( TRACE5, "No ready connections to send message" );
    }
  // ��।� �� �室
  // ���砫� ����⠢��� �� ��।�� ᮥ������� � ����� ��।�
    for( vector<pBMConnection>::iterator curr = conn.begin(); curr != conn.end(); ++curr )
    {
      while( (*curr)->readyToReceive() )
      {
        mes_queue.push( (*curr)->getFirst() );
      }
    }
  // ������ ��ࠡ���� ����� ��।�
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
    InitLogTime( argc > 0 ? argv[0] : NULL );
    BagMsgQueueDaemon daemon;
    run_bag_msg_process( argc > 0 ? argv[0] : NULL, argc > 1 ? argv[1] : "" );
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
