#ifndef __XP_TESTING_H__
#define __XP_TESTING_H__
#include "config.h"

#ifdef XP_TESTING
#include "serverlib/xp_test_utils.h"
#include "serverlib/checkunit.h"

namespace xp_testing
{

class TestStringsImpl;
class TestStrings
{
public:
    TestStrings();
    TestStrings(TestStrings const &);
    TestStrings(const std::string& tst);
    TestStrings& operator=(TestStrings const &);
    ~TestStrings();
    TestStrings& add(std::string const &);
    std::string check(std::string const &answer) const;
    std::string find_all(std::string const &answer) const;
    std::string show_mismatch(std::string const &answer) const;
private:
    TestStringsImpl *pl;
};
inline TestStrings& operator<<(TestStrings &t, std::string const & s)
{
    t.add(s);
    return t;
}
inline TestStrings& operator<<(TestStrings &t, char const * s)
{
    t.add(s);
    return t;
}

}//namespace xp_testing

void testClearShutDB();
#endif /*XP_TESTING*/

#endif /* __XP_TEST_UTILS_H__ */
