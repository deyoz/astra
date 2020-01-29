#if HAVE_CONFIG_H
#endif

#ifdef XP_TESTING

#include <string.h>
#include <string>

#include "optional.h"
#include "xp_test_utils.h"
#include "checkunit.h"

#define NICKNAME "NONSTOP"
#include "slogger.h"

namespace {

struct Foo
{
    int fld1, fld2;
    bool operator==(const Foo& v) const {
        return (fld1 == v.fld1) && (fld2 == v.fld2);
    }
};

void bar(comtech::optional<int> val) // copy construction/destruction
{
    ProgTrace(TRACE5, "val=%d", val.get());
}

START_TEST(check_optional)
{
    using comtech::optional;
    optional<int> val, val2(321);
    bar(val2);
    if (val)
        fail_unless(0,"default false failed");
    if (!val2)
        fail_unless(0,"default init true failed");
    val = 123;
    if (!val)
        fail_unless(0,"init true failed");
    fail_unless(val == 123, "init 123 failed");
    val.reset();
    fail_unless(not val, "reset false failed");
    val = val2;
    fail_unless(val == val2, "equal failed");

    optional<std::string> str;
    {
        optional<std::string> str2(std::string("lol"));
        fail_unless(str2->length() == 3, "bad copy");
        if (str)
            fail_unless(0,"default false failed");
        if (!str2)
            fail_unless(0,"default init true failed");
        str = str2;
    }
    fail_unless(str->length() == 3, "bad copy");
    fail_unless(strcmp(str->c_str(), "lol") == 0, "bad copy");

    fail_unless(optional<int>(123) != optional<int>(), "invalid comparison");
    fail_unless(123 != optional<int>(), "invalid comparison");

    Foo foo = {};
    foo.fld1 = 1; foo.fld2 = 2;
    //optional<Foo const&> optFoo(foo), opt;
    optional<Foo const&> optFoo(foo), opt;
    //fail_unless(optFoo == foo, "invalid comparison");
}END_TEST


#define SUITENAME "Serverlib"
TCASEREGISTER(0, 0)
    ADD_TEST(check_optional)
TCASEFINISH

}

#endif /* XP_TESTING */

