#include <serverlib/query_runner.h>

int main(int argc, char *argv[])
{
  void initTimaticTests();
  initTimaticTests();
  ServerFramework::setApplicationCallbacks<ServerFramework::ApplicationCallbacks>();
  return ServerFramework::applicationCallbacks()->run(argc, argv);
}
