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
#include "oci8.h"

void commit_base()
{
    ServerFramework::applicationCallbacks()->
        commit_db();
}

void rollback_base()
{
    ServerFramework::applicationCallbacks()->
        rollback_db();
    OciCpp::Oci8Session::instance(STDLOG).rollback();
    //memcache::callbacks()->flushAll();
}

#endif /* XP_TESTING */
