#include "exceptions.h"
#include "msg_queue.h"
#include "Depeches.hpp"

#include <serverlib/new_daemon.h>

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

void f(depeche_id_t id, Depeche::depeche_status_t status)
{

}

typedef boost::shared_ptr<Bagmessage> BagmessagePtr;

void run_msg_process(const std::string &name)
{
  using namespace AstraMessages;

  if (name.empty()) throw Exception("%s: empty process name", __FUNCTION__);

  TProcess process(name);

  map< string, BagmessagePtr > BMHandlers;
  boost::asio::io_service io;

  for(std::set<THandler>::const_iterator h=process.getHandlers().begin();
                                         h!=process.getHandlers().end(); ++h)
    if (h->getType()==THandler::BagMessage)
    {
      BagmessageSettings settings(paramValue(*h, "addr"),
                                  ToInt(paramValue(*h, "port")),
                                  paramValue(*h, "application_id"),
                                  paramValue(*h, "passwd"),
                                  true,
                                  ToInt(paramValue(*h, "heartbeat")));
      BMHandlers[h->getCode()]=BagmessagePtr(new Bagmessage(io, settings, f, ""));  //!!!vlad "" завенить на msg_handlers.name_lat
    };

  io.run();
}

int main_msg_handler_tcl(int supervisorSocket, int argc, char *argv[])
{
  try
  {
    sleep(5); //это необходимо чтобы cores не забивали дисковое пространство
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
