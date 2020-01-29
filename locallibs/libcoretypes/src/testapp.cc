#include <serverlib/query_runner.h>


#define NICKNAME "NONSTOP"
#include <serverlib/slogger.h>

static void init_coretypes_tests()
{
    void init_seats_tests();
    init_seats_tests();
    void init_rbd_tests();
    init_rbd_tests();
    void init_pnr_tests();
    init_pnr_tests();
    void init_flight_tests();
    init_flight_tests();
    void init_packer_tests();
    init_packer_tests();
    void init_connection_tests();
    init_connection_tests();
}

int main(int ac, char **av)
{
    init_coretypes_tests();
    ServerFramework::setApplicationCallbacks<ServerFramework::ApplicationCallbacks>();
    return ServerFramework::applicationCallbacks()->run(ac, av);
}

