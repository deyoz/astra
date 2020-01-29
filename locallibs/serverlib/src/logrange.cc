#include "logrange.h"
#include <vector>
#include <sstream>

#ifdef XP_TESTING
#include "checkunit.h"

void init_logrange_tests() {}

static std::string IntToWord(int n)
{
    switch (n) {
        case 1: return "one";
        case 2: return "two";
        case 3: return "three";
    }
    return std::to_string(n);
}

START_TEST(logrange)
{
    const std::vector<int> empty_v = { };
    const std::vector<int> v = { 1, 2, 3 };

    std::ostringstream s1, s2, s3, s4, s5;
    s1 << LogRange(empty_v.begin(), empty_v.end());
    s2 << LogRange(v.begin(), v.end());
    s3 << LogRange(v.begin(), v.end(), IntToWord);
    s4 << LogRange(v.begin(), v.end(),
            [](int n) -> double {
                return n + 0.5;
            });
    int accumulator = 0;
    s5 << LogRange(v.begin(), v.end(),
            [&accumulator](int n) {
                accumulator += n;
                return accumulator;
            });

    fail_unless(s1.str() == "[]");
    fail_unless(s2.str() == "[ 1 2 3 ]");
    fail_unless(s3.str() == "[ one two three ]");
    fail_unless(s4.str() == "[ 1.5 2.5 3.5 ]");
    fail_unless(s5.str() == "[ 1 3 6 ]");
}
END_TEST;

#define SUITENAME "Serverlib"
TCASEREGISTER(0, 0)
{
    ADD_TEST(logrange);
}
TCASEFINISH;

#endif // #ifdef XP_TESTING
