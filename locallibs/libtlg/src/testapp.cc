#include <serverlib/query_runner.h>


int main(int ac, char **av)
{
    void initAirsrvTests();
    initAirsrvTests();
    void initHthTests();
    initHthTests();
    void initExpressTests();
    initExpressTests();
    ServerFramework::setApplicationCallbacks<ServerFramework::ApplicationCallbacks>();
    return ServerFramework::applicationCallbacks()->run(ac, av);
}

