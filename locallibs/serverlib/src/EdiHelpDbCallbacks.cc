#include "serverlib/EdiHelpDbCallbacks.h"
#include "serverlib/EdiHelpDbOraCallbacks.h"
#define NICKNAME "ROMAN"
#include <serverlib/slogger.h>

namespace ServerFramework {

EdiHelpDbCallbacks *EdiHelpDbCallbacks::Instance = 0;
EdiHelpDbCallbacks *EdiHelpDbCallbacks::instance()
{
    if(EdiHelpDbCallbacks::Instance == 0)
        EdiHelpDbCallbacks::Instance = new EdiHelpDbOraCallbacks();
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
