#include <cstdlib>
#include "logcout.h"


CoutLogger::CoutLogger(const CoutPriority level): level_(level)
{}

bool CoutLogger::can_write() const
{
    const char* cutlog = getenv("STDOUT_CUTLOGGING");
    return level_ <= (cutlog and *cutlog ? std::atoi(cutlog) : COUT_INFO);
}


//хак для std::endl
CoutLogger& CoutLogger::operator<<(std::ostream& (*manip)(std::ostream&))
{
    if(can_write()) std::cout << manip;
    return *this;
}


CoutLogger LogCout(const CoutPriority level)
{
    return CoutLogger(level);
}


#ifdef XP_TESTING
#include "tcl_utils.h"
#include "xp_test_utils.h"

#include "checkunit.h"
#include <sstream>

namespace
{
    START_TEST(coutlogger)
    {
        std::ostringstream out_str_buf;
        std::streambuf* cout_backup = std::cout.rdbuf(); // Сохраняем rdbuf std::cout
        std::cout.rdbuf(out_str_buf.rdbuf()); // Подменяем rdbuf std::cout

        const std::string test_str = "test proverka 123 ";

        char c_[] = "STDOUT_CUTLOGGING";
        putenv(c_);
        LogCout(COUT_ERROR) << test_str;
        LogCout(COUT_WARNING) << test_str;
        LogCout(COUT_INFO) << test_str;
        fail_unless(out_str_buf.str() == (test_str + test_str + test_str));
        out_str_buf.str("");
        //
        char c3[] = "STDOUT_CUTLOGGING=3";
        putenv(c3);
        LogCout(COUT_ERROR) << test_str;
        LogCout(COUT_WARNING) << test_str;
        LogCout(COUT_INFO) << test_str;
        fail_unless(out_str_buf.str() == (test_str + test_str + test_str));
        out_str_buf.str("");
        //
        char c2[] = "STDOUT_CUTLOGGING=2";
        putenv(c2);
        LogCout(COUT_ERROR) << test_str;
        LogCout(COUT_WARNING) << test_str;
        LogCout(COUT_INFO) << test_str;
        fail_unless(out_str_buf.str() == (test_str + test_str));
        out_str_buf.str("");
        //
        char c1[] = "STDOUT_CUTLOGGING=1";
        putenv(c1);
        LogCout(COUT_ERROR) << test_str;
        LogCout(COUT_WARNING) << test_str;
        LogCout(COUT_INFO) << test_str;
        fail_unless(out_str_buf.str() == test_str);
        out_str_buf.str("");
        //
        char c0[] = "STDOUT_CUTLOGGING=0";
        putenv(c0);
        LogCout(COUT_ERROR) << test_str;
        LogCout(COUT_WARNING) << test_str;
        LogCout(COUT_INFO) << test_str;
        fail_unless(out_str_buf.str() == "");

        std::cout.rdbuf(cout_backup); // Восстанавливаем rdbuf std::cout
    }
    END_TEST


    #define SUITENAME "Serverlib"

    TCASEREGISTER(0, 0)
    {
        ADD_TEST(coutlogger);
    }
    TCASEFINISH

}


#endif // XP_TESTING
