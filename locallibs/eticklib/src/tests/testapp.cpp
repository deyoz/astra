#include <serverlib/query_runner.h>


int main(int ac, char **av)
{
    void init_tick_data_cpp();
    init_tick_data_cpp();
    ServerFramework::setApplicationCallbacks<ServerFramework::ApplicationCallbacks>();
    return ServerFramework::applicationCallbacks()->run(ac, av);
}

