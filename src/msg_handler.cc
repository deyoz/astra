#include "exceptions.h"
#include "msg_queue.h"
#include "Depeches.hpp"
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <serverlib/new_daemon.h>
#include <serverlib/tclmon.h>

#define NICKNAME "ROMAN"
#define NICKTRACE ROMAN_TRACE
#include <serverlib/slogger.h>

using namespace EXCEPTIONS;
using namespace std;

class MsgQueueDaemon : public ServerFramework::NewDaemon
{

public:
    MsgQueueDaemon() : ServerFramework::NewDaemon("msg_handler") {}
};

using namespace depeches;

BagmessageSettings createBagmessageSettings(const AstraMessages::THandler &handler)
{
  return BagmessageSettings(paramValue(handler, "addr"),
                            ToInt(paramValue(handler, "port")),
                            paramValue(handler, "application_id"),
                            paramValue(handler, "passwd"),
                            true,
                            ToInt(paramValue(handler, "heartbeat")));
}

typedef shared_ptr<Bagmessage> BagmessagePtr;

class BagmessageProcess : public AstraMessages::TProcess
{
  private:
    map< string, BagmessagePtr > BMHandlers;
    boost::asio::deadline_timer timer_;
  public:
    BagmessageProcess(boost::asio::io_service &io,
                      const std::string &p) : AstraMessages::TProcess(p), timer_(io)
    {
      using namespace AstraMessages;
      for(std::set<THandler>::const_iterator h=getHandlers().begin();
                                             h!=getHandlers().end(); ++h)
        if (h->getType()==THandler::BagMessage)
        {
          BMHandlers[h->getCode()]=BagmessagePtr(new Bagmessage(io,
                                                                createBagmessageSettings(*h),
                                                                boost::bind(&BagmessageProcess::finish_depeche, this, _1, _2),
                                                                ""));  //!!!vlad "" заменить на msg_handlers.name_lat
        };
    }
    void check_and_send_depeche()
    {
      using namespace AstraMessages;

      bool use_timer=true;
      while(boost::optional<TQueueId> next=nextMsg())
      {
        boost::optional<TQueueMsg> msg;
        TQueue::get(next->id, msg);
        if (!msg) throw Exception("strange situation!!!");
        map< string, BagmessagePtr >::const_iterator BMH=BMHandlers.find(msg->handler.getCode());
        if (BMH==BMHandlers.end()) throw Exception("strange situation!!!");

        BMH->second->send_depeche(msg->content,
                                  createBagmessageSettings(msg->handler),
                                  next->id,
                                  ToInt(paramValue(msg->handler, "timeout_secs", string("10"))));
        use_timer=false;
      }
      if (use_timer)
      {
        timer_.expires_from_now(boost::posix_time::seconds(5)); //!!!vlad 5?
        timer_.async_wait(boost::bind(&BagmessageProcess::finish_timer, this, _1));
      }
    }

    void finish_depeche(depeche_id_t id, Depeche::depeche_status_t status)
    {
      using namespace AstraMessages;
      switch(status)
      {
        case Depeche::OK: TQueue::complete_attempt(id); break;
        case Depeche::FAIL: TQueue::complete_attempt(id, "FAIL!"); break;
        case Depeche::EXPIRED: TQueue::complete_attempt(id, "EXPIRED!"); break;
        case Depeche::NO_FREE_SLOT: TQueue::complete_attempt(id, "NO_FREE_SLOT!"); break;
      };

      monitor_idle_zapr_type(1, QUEPOT_NULL);

      check_and_send_depeche();
    }

    void finish_timer(const boost::system::error_code &e)
    {
      if (!e) check_and_send_depeche();
    }
};



typedef shared_ptr<AstraMessages::TProcess> ProcessPtr;

void put_test_msg()
{
  using namespace AstraMessages;

  TQuery Qry(&OraSession);
  Qry.SQLText="SELECT count(*) AS num FROM msg_queue";

  list<string> handlers;
  handlers.push_back("BAG_MESSAGE");
  handlers.push_back("BAG_MESSAGE2");
  TBagMessageSetDetails setDetails(handlers);

  int i=1;
  for(;;)
  {
    if (i>500)
    {
      sleep(5);
      continue;
    };
    //sleep(3);
    Qry.Execute();
    if (Qry.FieldAsInteger("num")>50) continue;
    ostringstream s;
    s << "test telegram " << i;
    i++;
    AstraMessages::TQueue::put(setDetails, "BSM;point_id=1234565", s.str());
    OraSession.Commit();
    monitor_idle_zapr_type(1, QUEPOT_NULL);
  }
}

void run_msg_process(const std::string &name)
{
  using namespace AstraMessages;

  if (name.empty()) throw Exception("%s: empty process name", __FUNCTION__);

  if (name=="put_test_msg")
  {
    put_test_msg();
    return;
  }

  boost::asio::io_service io;

  BagmessageProcess BMProcess(io, name);
  for(;;)
  {
    BMProcess.check_and_send_depeche();
    io.run();
  };

/*
  for(;;)
  {
    while(boost::optional<TQueueId> next=process->nextMsg())
    {
      boost::optional<TQueueMsg> msg;
      TQueue::get(next->id, msg);
      if (!msg) throw Exception("strange situation!!!");
      ProgTrace(TRACE5, "%s: msg.content=%s", __FUNCTION__, msg->content.c_str());
      TQueue::complete_attempt(next->id);
      monitor_idle_zapr_type(1, QUEPOT_NULL);
    };
    sleep(1);
  }
*/
}

int main_msg_handler_tcl(int supervisorSocket, int argc, char *argv[])
{
  try
  {
    sleep(2); //!!!vlad это необходимо чтобы cores не забивали дисковое пространство
    MsgQueueDaemon daemon;
    run_msg_process(argc>1?argv[1]:"");
  }
  catch( std::exception &E ) {
    ProgError( STDLOG, "std::exception: %s", E.what() );
    throw;
  }
  catch( ... ) {
    ProgError( STDLOG, "Unknown error" );
    throw;
  };

  return 0;
}

int test_msg_queue(int argc,char **argv)
{
  using namespace AstraMessages;
//  list<string> handlers;
//  handlers.push_back("BAG_MESSAGE");
//  handlers.push_back("BAG_MESSAGE2");
//  TBagMessageSetDetails setDetails(handlers);

//  AstraMessages::TQueue::put(setDetails, "", "ЭТО ТЕЛО ТЕЛЕГРАММЫ");
//  OraSession.Commit();

  boost::optional<TQueueMsg> msg;
  AstraMessages::TQueue::get(12728411, msg);
  if (msg)
  {
    ProgError(STDLOG,
              "handler=%s format=%s type=%s content=%s",
              msg->handler.getCode().c_str(),
              msg->format.getCode().c_str(),
              msg->type.c_str(),
              msg->content.c_str());
  }
  else ProgError(STDLOG, "msg=boost::none");
  AstraMessages::TQueue::complete_attempt(12728411);



 //  AstraMessages::TQueue::put(TSetDetails(), "", "");

  return 0;
}

