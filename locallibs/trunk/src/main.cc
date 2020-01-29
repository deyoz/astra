#include "query_runner.h"

int main(int ac, char **av)
{
#ifdef SERVERLIB_ADDR2LINE
  void init_addr2line(const std::string& filename);
  init_addr2line(av[0]);
#endif // SERVERLIB_ADDR2LINE
  set_signal(term3);
  ServerFramework::setApplicationCallbacks<ServerFramework::ApplicationCallbacks>();
  return ServerFramework::applicationCallbacks()->run(ac, av);
}
