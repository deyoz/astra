/* This file contains utilities to be used within XP tests */

/* pragma cplusplus */

#ifdef XP_TESTING
#define NICKNAME "ILYA"
#include <iostream>
#include <string>
#include <stdarg.h>
#include "test.h"
#include "query_runner.h"
#include "xp_test_utils.h"
#ifdef ENABLE_ORACLE
#include "oci8.h"
#endif
void commit_base()
{
    ServerFramework::applicationCallbacks()->
        commit_db();
}

void rollback_base()
{
    ServerFramework::applicationCallbacks()->
        rollback_db();
#ifdef ENABLE_ORACLE
    OciCpp::Oci8Session::instance(STDLOG).rollback();
#endif
    //memcache::callbacks()->flushAll();
}

#endif /* XP_TESTING */
