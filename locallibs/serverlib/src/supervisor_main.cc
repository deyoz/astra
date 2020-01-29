#define  NICKNAME "MIXA"

#include <cstdlib>
#include <cstring>
#include "monitor_ctl.h"
#include "supervisor.h"

int main(int argc, char** argv)
{
    const char* xp_testing = getenv("XP_TESTING");

    if (!xp_testing || !(*xp_testing)) {
        fprintf(stderr, "argv : [");
        for (int i = 0; i < argc; ++i) {
            if(i > 0)  fprintf(stderr, ", ");
            fprintf(stderr, "'%s'", argv[i]);
        }
        fprintf(stderr, "]\n");
    }
    set_signal(term3);
#ifdef SERVERLIB_ADDR2LINE
    void init_addr2line(const std::string & filename);
    init_addr2line(argv[0]);
#endif //SERVERLIB_ADDR2LINE

    return Supervisor::run(argc, argv);
}

