#include <boost/regex.hpp>

#include "rip.h"
#include "rip_validators.h"

#define NICKNAME "NONSTOP"
#include "slogger.h"

namespace rip
{
namespace validators
{
bool byRegex(const std::string& v, const std::string& reStr)
{
    const boost::regex re(reStr);
    return boost::regex_match(v, re);
}
} // validators
} // rip

void init_right_parameters()
{
}

#ifndef ENABLE_PG_TESTS
#ifdef XP_TESTING
#include "rip_oci.h"
#include "oci_row.h"
#include "helpcpp.h"

#include "xp_test_utils.h"
#include "checkunit.h"


namespace rip
{
DECL_RIP(TypeInt, int);
DECL_RIP(TypeStr, std::string);
DECL_RIP_RANGED(TypeRangedInt, int, 10, 100);
DECL_RIP_LENGTH(TypeLenStr, std::string, 2, 4);
DECL_RIP_REGEX(TypeReStr, std::string, "ab.*");
DECL_RIP(TypeBool, bool);

} // rip

namespace {

START_TEST(rip)
{
    rip::TypeInt v1(0);
    rip::TypeStr v2("");

    rip::TypeRangedInt v3(12), v31(13);
    fail_unless(v3.get() == 12);
    fail_unless(v3 < v31);
    fail_unless(v31 > v3);
    v31 = v3;
    fail_unless(v3 == v31);
    fail_unless((bool)rip::TypeRangedInt::create(112) == false);
    try { rip::TypeRangedInt(112); fail_if(1, "must throw");
    } catch (const rip::TypeRangedInt::Exception& e) {
        fail_unless(strcmp(e.what(), "invalid TypeRangedInt val='112'") == 0);
    }
    std::stringstream ss;
    ss << v3;
    fail_unless(ss.str() == "12");

    rip::TypeLenStr v4("12");
    fail_unless(v4.get() == "12");
    fail_unless((bool)rip::TypeLenStr::create("12345") == false);
    try { rip::TypeLenStr("12345"); fail_if(1, "must throw");
    } catch (const rip::TypeLenStr::Exception& e) {
        fail_unless(strcmp(e.what(), "invalid TypeLenStr val='12345'") == 0);
    }

    rip::TypeReStr v5("ab12");
    fail_unless(v5.get() == "ab12");
    fail_unless((bool)rip::TypeReStr::create("12345") == false);
    try { rip::TypeReStr("12345"); fail_if(1, "must throw");
    } catch (const rip::TypeReStr::Exception& e) {
        fail_unless(strcmp(e.what(), "invalid TypeReStr val='12345'") == 0);
    }

    rip::TypeBool v6(true);
    fail_unless(v6.get() == true);
} END_TEST

START_TEST(rip_num_ocicpp)
{
    const rip::TypeInt v1(1);
    rip::TypeInt v2(2);
    rip::TypeRangedInt v3(13);
    
    make_curs("select 33 from dual where :v1 = 1 and :v2 = 2")
        .bind("v1", v1).bind("v2", v2)
        .def(v3)
        .EXfet();
    fail_unless(v3 == rip::TypeRangedInt(33));

    try {
        make_curs("select 133 from dual where :v1 = 1 and :v2 = 2")
            .bind("v1", v1).bind("v2", v2)
            .def(v3)
            .EXfet();
        fail_if(1, "must throw");
    } catch (const rip::TypeRangedInt::Exception& e) {
        fail_unless(strcmp(e.what(), "invalid TypeRangedInt val='133'") == 0);
    }
    fail_unless(v3 == rip::TypeRangedInt(33));

    make_curs("begin :v2 := 22; end;").bindOut(":v2", v2).exec();
    fail_unless(v2 == rip::TypeInt(22));

    try {
        make_curs("begin :v3 := 133; end;").bindOut(":v3", v3).exec();
        fail_if(1, "must throw");
    } catch (const rip::TypeRangedInt::Exception& e) {
        fail_unless(strcmp(e.what(), "invalid TypeRangedInt val='133'") == 0);
    }
    fail_unless(v3 == rip::TypeRangedInt(33));
} END_TEST

START_TEST(rip_str_unstb)
{
    const rip::TypeStr v1("1");
    rip::TypeStr v2("2");
    rip::TypeReStr v3("ab3");

    std::string s;
    make_curs("select 'ab33' from dual where :v1 = '1' and :v2 = '2'")
        .unstb()
        .bind("v1", v1).bind("v2", v2)
        .def(v3)
        .EXfet();
    fail_unless(v3 == rip::TypeReStr("ab33"));

    try {
        make_curs("select '12345' from dual where :v1 = '1' and :v2 = '2'")
            .unstb()
            .bind("v1", v1).bind("v2", v2)
            .def(v3)
            .EXfet();
        fail_if(1, "must throw");
    } catch (const rip::TypeReStr::Exception& e) {
        fail_unless(strcmp(e.what(), "invalid TypeReStr val='12345'") == 0);
    }
    fail_unless(v3 == rip::TypeReStr("ab33"));
} END_TEST

START_TEST(rip_str_stb)
{
    const rip::TypeStr v1("1");
    rip::TypeStr v2("2");
    rip::TypeReStr v3("ab3");

    std::string s;
    make_curs("select 'ab33' from dual where :v1 = '1' and :v2 = '2'")
        .stb()
        .bind("v1", v1).bind("v2", v2)
        .def(v3)
        .EXfet();
    fail_unless(v3 == rip::TypeReStr("ab33"));

    try {
        make_curs("select '12345' from dual where :v1 = '1' and :v2 = '2'")
            .stb()
            .bind("v1", v1).bind("v2", v2)
            .def(v3)
            .EXfet();
        fail_if(1, "must throw");
    } catch (const rip::TypeReStr::Exception& e) {
        fail_unless(strcmp(e.what(), "invalid TypeReStr val='12345'") == 0);
    }
    fail_unless(v3 == rip::TypeReStr("ab33"));
} END_TEST

START_TEST(rip_def_row)
{
    OciCpp::Row<rip::TypeInt, rip::TypeStr, rip::TypeRangedInt, rip::TypeLenStr, boost::optional<rip::TypeInt> > row;
    make_curs("select 1, 'lol', 200, 'invalid', null from dual")
        .defRow(row)
        .EXfet();
    fail_unless(row.get<0>() == rip::TypeInt(1));
    fail_unless(row.get<1>() == rip::TypeStr("lol"));
    fail_unless(static_cast<bool>(row.get<4>()) == false);
    try {
        row.get<2>();
        fail_if(1, "must throw, due invalid value for TypeRangedInt");
    } catch (const rip::TypeRangedInt::Exception&) {
    }
    try {
        row.get<3>();
        fail_if(1, "must throw, due invalid value for TypeLenStr");
    } catch (const rip::TypeLenStr::Exception&) {
    }
} END_TEST

#define SUITENAME "Serverlib"
TCASEREGISTER(0, 0)
    ADD_TEST(rip)
TCASEFINISH
TCASEREGISTER(testInitDB, 0)
    ADD_TEST(rip_num_ocicpp)
    ADD_TEST(rip_str_unstb)
    ADD_TEST(rip_str_stb)
    ADD_TEST(rip_def_row)
TCASEFINISH
} // namespace

#endif /* XP_TESTING */
#endif /*ENABLE_PG_TESTS*/
