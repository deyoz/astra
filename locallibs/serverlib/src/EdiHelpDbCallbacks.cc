#include "serverlib/EdiHelpDbCallbacks.h"
#include "serverlib/EdiHelpDbOraCallbacks.h"
#include "serverlib/EdiHelpDbPgCallbacks.h"

#include "serverlib/tcl_utils.h"
#include "serverlib/pg_cursctl.h"

#define NICKNAME "ROMAN"
#include <serverlib/slogger.h>

namespace ServerFramework {

static std::string getPgConnectString()
{
    static std::string cs = readStringFromTcl("PG_CONNECT_STRING", "");
    return cs;
}

static bool usePg()
{
    static bool use_pg = getVariableStaticBool("EDI_HELP_PG", NULL, 1);
    return use_pg;
}

static PgCpp::SessionDescriptor& getPgManaged()
{
    static PgCpp::SessionDescriptor sd = PgCpp::getManagedSession(getPgConnectString());
    return sd;
}


EdiHelpDbCallbacks *EdiHelpDbCallbacks::Instance = 0;
EdiHelpDbCallbacks *EdiHelpDbCallbacks::instance()
{
    if(EdiHelpDbCallbacks::Instance == 0) {
        if(usePg()) {
            EdiHelpDbCallbacks::Instance = new EdiHelpDbPgCallbacks(getPgManaged());
        } else {
            EdiHelpDbCallbacks::Instance = new EdiHelpDbOraCallbacks();
        }
    }
    return Instance;
}

void EdiHelpDbCallbacks::setEdiHelpDbCallbacks(EdiHelpDbCallbacks *cb)
{
    if(EdiHelpDbCallbacks::Instance)
      delete EdiHelpDbCallbacks::Instance;
    EdiHelpDbCallbacks::Instance = cb;
}

EdiHelpDbCallbacks::~EdiHelpDbCallbacks()
{
}

} // namespace ServerFramework
