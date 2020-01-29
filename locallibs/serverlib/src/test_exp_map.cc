#ifdef XP_TESTING
#include <string>
#include "exp_map.h"

#define NICKNAME "DMITRYVM"
#define NICKTRACE DMITRYVM_TRACE
#include "slogger.h"
#include "checkunit.h"

void init_exp_map_tests()
{
}

namespace {
    bool IsExpired(const int& t1, const int& t2)
    {
        return t2 - t1 > 10;
    }
}

typedef MapWithExpiration<char, std::string, int, IsExpired> Map_t;

static std::string Str(const Map_t& m)
{
    std::ostringstream os;
    for (Map_t::const_iterator it = m.begin(); it != m.end(); ++it) {
        if (it != m.begin())
            os << ' ';
        os << it->first << ':' << it->second.v << ',' << it->second.t;
    }
    return os.str();
}

START_TEST(CheckExpMap1)
{
    Map_t m;
    m.update('a', "A", 0);
    m.update('b', "B", 0);
    m.update('c', "C", 1);
    m.update('d', "D", 1);
    m.update('e', "E", 2);
    m.update('f', "F", 3);
    fail_unless(Str(m) == "a:A,0 b:B,0 c:C,1 d:D,1 e:E,2 f:F,3");

    m.update('b', "BB", 4);
    m.update('c', "CC", 4);
    m.update('e', "EE", 5);
    fail_unless(Str(m) == "a:A,0 b:BB,4 c:CC,4 d:D,1 e:EE,5 f:F,3");

    m.update('a', "AA", 13);
    fail_unless(Str(m) == "a:AA,13 b:BB,4 c:CC,4 e:EE,5 f:F,3");

    m.update('g', "G", 15);
    fail_unless(Str(m) == "a:AA,13 e:EE,5 g:G,15");
}
END_TEST;

START_TEST(CheckExpMap2)
{
    Map_t m;
    for (unsigned i = 0; i < 1000000; ++i)
        m.update(
                (char)('a' + i % 26),
                std::string(1, (char)('A' + i % 26)),
                i);
    fail_unless(Str(m) == "d:D,999989 e:E,999990 f:F,999991 g:G,999992 h:H,999993"
            " i:I,999994 j:J,999995 k:K,999996 l:L,999997 m:M,999998 n:N,999999");
}
END_TEST;

#define SUITENAME "exp_map"
TCASEREGISTER(0, 0)
{
    ADD_TEST(CheckExpMap1);
    ADD_TEST(CheckExpMap2);
}
TCASEFINISH

#endif /* #ifdef XP_TESTING */
