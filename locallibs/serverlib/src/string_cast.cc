#if HAVE_CONFIG_H
#endif

#include "str_utils.h"
#include "string_cast.h"

namespace HelpCpp {
std::string string_cast(int n)
{
    return std::to_string(n);
}

std::string string_cast(unsigned n)
{
    return std::to_string(n);
}

std::string string_cast(unsigned long n)
{
    return std::to_string(n);
}

std::string string_cast(unsigned long long n)
{
    return std::to_string(n);
}

std::string string_cast(long n)
{
    return std::to_string(n);
}

std::string string_cast(long long n)
{
    return std::to_string(n);
}

std::string string_cast(double n)
{
    std::string tmp=std::to_string(n);
    if(tmp.find('.')!=std::string::npos)
    {
        StrUtils::StringRTrimCh(tmp, '0');
        StrUtils::StringRTrimCh(tmp, '.');
    }
    return tmp;
}

std::string string_cast(bool val) { return val ? "true" : "false"; }

std::string string_cast(char val) { return std::string(1, val); }

} // namespace HelpCpp

#include "checkunit.h"
#ifdef XP_TESTING

START_TEST(string_cast_int)
{
    using namespace HelpCpp;
    fail_unless(string_cast(10) == "10",
                "string_cast(10) = %s", string_cast(10).c_str());
    fail_unless(string_cast((long)10) == "10",
                "string_cast((long)10)=%s", string_cast((long)10).c_str());
    fail_unless(string_cast((long long)1222222333222222220LL) == "1222222333222222220");
    fail_unless(string_cast((double)1222222.232222) == "1222222.232222", "for 1222222.232222 got %s",
                string_cast((double)1222222.232222).c_str());
    fail_unless(string_cast((double)1222222.23222) == "1222222.23222", "for 1222222.23222 got %s",
                string_cast((double)1222222.23222).c_str());
    fail_unless(string_cast((double)0.23) == "0.23", "for 0.23 got %s",
                string_cast((double)0.23).c_str());
    fail_unless(string_cast((double)0.00023) == "0.00023", "for 0.00023 got %s",
                string_cast((double)0.00023).c_str());
    fail_unless(string_cast((double)20.000000) == "20", "for (double)20.000000 got %s",
                string_cast((double)20.000000).c_str());
    fail_unless(string_cast((double)15.) == "15", "for (double)15. got %s",
                string_cast((double)15.).c_str());
    fail_unless(string_cast((double)16) == "16", "for (double)16 got %s",
                string_cast((double)16).c_str());
}
END_TEST

#define SUITENAME "Serverlib"
TCASEREGISTER(0,0)
{
    ADD_TEST(string_cast_int);
}
TCASEFINISH

#endif /*XP_TESTING*/

