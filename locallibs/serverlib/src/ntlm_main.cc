#include "ntlm_server.h"

int main(int argc, char **argv)
{
    return httpsrv::ntlm::runTestServer(argc, argv);
}
