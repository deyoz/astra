#include "jxtlib.h"
#include <serverlib/query_runner.h>

int main(int argc, char *argv[])
{
  jxtlib::JXTLibCallbacks jc;
  loclib::LocaleLibCallbacks lc;
  ServerFramework::setApplicationCallbacks<ServerFramework::ApplicationCallbacks>();
  return ServerFramework::applicationCallbacks()->run(argc, argv);
}
