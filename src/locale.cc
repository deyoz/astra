#include "oralib.h"
#include "tlg/tlg.h"
#include <jxtlib/JxtInterface.h>
#include <serverlib/ocilocal.h>
#include <serverlib/TlgLogger.h>

#define NICKNAME "KONST"
#define NICKTRACE KONST_TRACE
#include <serverlib/test.h>

int init_locale(void)
{
    ProgTrace(TRACE1,"init_locale");
    //  jxtlib::JXTLib::Instance()->SetCallbacks(new AstraCallbacks());
    JxtInterfaceMng::Instance();
    if(edifact::init_edifact() < 0)
        throw EXCEPTIONS::Exception("'init_edifact' error!");
    TlgLogger::setLogging();
    return 0;
}
