/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

DO NOT EDIT THIS FILE BY HAND

USE autotests.sh INSTEAD

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
#include <iostream>
#include <boost/foreach.hpp>
#include <serverlib/tscript.h>
#include "xp_testing.h"
#ifdef XP_TESTING

#define NICKNAME "DMITRYVM"
#include <serverlib/slogger.h>

#include <serverlib/xp_test_utils.h>
#include <serverlib/checkunit.h>

/*******************************************************************************
 * �����
 ******************************************************************************/

static void RunAllTests(const std::string& fileName)
{
    std::cout << "running " << fileName << std::endl;
    namespace tscript = xp_testing::tscript;
    const std::vector<tscript::Test> tests = tscript::Parse(fileName);
    std::vector<size_t> failed;
    for (size_t i = 0; i < tests.size(); ++i)
        try {
            tscript::RunTest(tests[i]);
        } catch (const std::exception& e) {
            std::cout << fileName << ": test #" << (i + 1) << " failed: " << e.what() << std::endl;
            failed.push_back(i);
        }

    std::ostringstream errText;
    errText << fileName << ": " << failed.size() << " of " << tests.size() << " failed:";
    BOOST_FOREACH (size_t index, failed)
        errText << " #" << (index + 1);

    fail_unless(failed.empty(), errText.str().c_str());
}

#define AUTO_TEST(name, file_name) START_TEST(name) { RunAllTests(file_name); } END_TEST;
AUTO_TEST(ts_test_ts, "./ts/test.ts");

#undef SUITENAME
#define SUITENAME "auto"
TCASEREGISTER(testInitDB, 0)
{
    SET_TIMEOUT(300)
    ADD_TEST(ts_test_ts);
}
TCASEFINISH

#endif /* XP_TESTING */
