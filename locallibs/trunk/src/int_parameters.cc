#ifdef XP_TESTING

#include "int_parameters.h"
#include "int_parameters_oci.h"

#include "int_parameters_serialization.h"
#include <sstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>

#include "xp_test_utils.h"
#include "checkunit.h"

#define NICKNAME "NONSTOP"
#include "slogger.h"

void init_int_parameters()
{
}

namespace {

MakeIntParamType(Type1, int);
MakeIntParamType(Type2, int);
MakeIntParamType(Type3, int);


struct Type4Traits {
    Type4Traits() {
        name_ = "Type4";
    };
    static constexpr bool can_compare_to_base_t = true;
    //static constexpr bool allow_arithmetics = true;
    const char* name_;
};
typedef ParInt::BaseIntParam<Type4Traits, int> Type4;

START_TEST(parint_compile)
{
    Type1 t1(1);
    Type1 t11(1);
    if (t1 == t11)
        LogTrace(TRACE5) << "t1 == t11";

    LogTrace(TRACE5) << "t1.name = " << t1.name();

    Type2 t2(2);
    Type3 t3(3);
    LogTrace(TRACE5) << "t2.name = " << t2.name();
    LogTrace(TRACE5) << "t3.name = " << t3.name();
    
// this doesn't compile
//    int tmp = t2;
//    t2 = t1;
//    if (t1 == 4) return 1;
// this throws
}END_TEST

MakeArithmeticsIntParamType(Type5, int);

START_TEST(parint_math)
{
    Type2 t2(2);
    Type3 t3(3);
    fail_unless(t2 == Type2(2), "failed t2 == 2");
    fail_unless(t3 == Type3(3), "failed t3 == 3");

    Type4 t4(0);
    fail_unless(t4 == 0, "t4 != 0");
    fail_unless(t4 != 1, "t4 == 1");
    
    Type5 t51(2), t52(3);
    t52 += t51;
    fail_unless(t52 == 5, "t52 != 5");
    t52 -= t51;
    fail_unless(t52 == 3, "t52 != 3");
}END_TEST

START_TEST(parint_init)
{
    Type2 t2;
    try {
        int tmp = t2.get();
        fail_unless(0,"failed get when not init: %d", tmp);
    } catch (const Type2::Exception& e) {
        // all ok
    }
    
    if (t2.valid()) {
        fail_unless(0,"not init must be false");
    }
}END_TEST

MakeIntParamType(TypeShort, short);
MakeIntParamType(TypeUShort, unsigned short);
MakeIntParamType(TypeUInt, unsigned int);
MakeIntParamType(TypeDouble, double);
MakeIntParamType(TypeFloat, float);

MakeStrParamType(StrType1, 1);
MakeStrParamType(StrType2, 2);

#ifndef ENABLE_PG_TESTS
START_TEST(parint_ocicpp)
{
    Type4 t1;
    Type4 t2;
    Type4 t3, t3_null(10);
    int t2_int = 4;
    OciCpp::CursCtl cr = make_curs("select 1, :t2, NULL from dual");
    cr.setDebug();
    cr.bind(":t2", t2_int);
    cr.def(t1).defNull(t2, t3_null).defNull(t3, t3_null);
    cr.EXfet();
    fail_unless(t1 == 1, "bad t1");
    fail_unless(t2 == 4, "bad t2");
    fail_unless(t3 == t3_null, "bad t3");
}END_TEST

START_TEST(parint_types)
{
    TypeShort tShort;
    TypeUShort tUShort;
    TypeUInt tUInt;
    TypeDouble tDouble;
    TypeFloat tFloat;

    OciCpp::CursCtl cr = make_curs("select 20, 10, 123, 12.3, 24.6 from dual");
    cr.setDebug();
    cr.def(tShort).def(tUShort).def(tUInt).def(tDouble).def(tFloat);
    cr.EXfet();
    fail_unless(tShort == TypeShort(20), "bad tShort: %d", tShort.get());
    fail_unless(tUShort == TypeUShort(10), "bad tUShort: %u", tUShort.get());
    fail_unless(tUInt == TypeUInt(123), "bad tUInt: %u", tUInt.get());
    fail_unless(tDouble == TypeDouble(12.3), "bad tDouble: %f", tDouble.get());
    fail_unless(tFloat == TypeFloat(24.6), "bad tFloat: %f", tFloat.get());
}END_TEST

START_TEST(parstr_ocicpp)
{
    StrType1 t1;
    StrType2 t2, t3;
    make_curs("select 't', 'tt', :v from dual")
        .def(t1).def(t2).def(t3)
        .bind(":v", StrType2("ss"))
        .EXfet();
    fail_unless(t1 == StrType1("t"));
    fail_unless(t2 == StrType2("tt"));
    fail_unless(t3 == StrType2("ss"));
} END_TEST

START_TEST(parint_def_null_changes)
{
    TypeShort tsh(5);
    OciCpp::indicator ind;
    make_curs("select null from dual").def(tsh, &ind).EXfet();
    fail_unless(tsh.get() == 5, "parint_def_null_changes failed");
    fail_unless(ind == -1);
}END_TEST

START_TEST(parint_bind_null)
{
    TypeShort tsh_in(5), tsh_out(123);
    OciCpp::indicator ind = -1;
    int e = make_curs("select :t from dual").noThrowError(CERR_NULL).bind(":t",tsh_in,&ind).def(tsh_out).EXfet();
    fail_unless(e == CERR_NULL);

    make_curs("select :t from dual").autoNull().bind(":t",tsh_in,&ind).def(tsh_out).EXfet();
    fail_if(tsh_out.valid());
    fail_unless(ind == -1);

    tsh_in = TypeShort();
    tsh_out = TypeShort(123);
    make_curs("select :t from dual").autoNull().bind(":t",tsh_in,&ind).def(tsh_out).EXfet();
    fail_if(tsh_out.valid());
    fail_unless(ind == -1);

    tsh_out = TypeShort(123);
    make_curs("select :t from dual").autoNull().bind(":t",tsh_in).def(tsh_out).EXfet();
    fail_if(tsh_out.valid());
}
END_TEST
START_TEST(parint_bindOut)
{
    TypeShort p;
    make_curs("begin select 3 into :i from dual; end;").unstb().bindOut(":i",p).exec();
    ck_assert_int_eq(p.get(), 3);
}
END_TEST
#endif /*ENABLE_PG_TESTS*/

START_TEST(parint_compare)
{
//    using namespace int_parameters;
    TypeShort tShort;
    TypeUShort tUShort;
    fail_unless(!tShort.valid(), "bad tShort");
    fail_unless(!tUShort.valid(), "bad tUShort");
    tShort = TypeShort(30);
    tUShort = TypeUShort(20);
    fail_unless((tShort < TypeShort(10)) == false, "bad tShort: %d", tShort.get());
    fail_unless((tUShort < TypeUShort(10)) == false, "bad tUShort: %d", tUShort.get());
    fail_unless((tShort > TypeShort(40)) == false, "bad tShort: %d", tShort.get());
    fail_unless((tUShort > TypeUShort(40)) == false, "bad tUShort: %d", tUShort.get());
    fail_unless((tShort <= TypeShort(10)) == false, "bad tShort: %d", tShort.get());
    fail_unless((tUShort <= TypeUShort(10)) == false, "bad tUShort: %d", tUShort.get());
    fail_unless((tShort >= TypeShort(40)) == false, "bad tShort: %d", tShort.get());
    fail_unless((tUShort >= TypeUShort(40)) == false, "bad tUShort: %d", tUShort.get());
}END_TEST

START_TEST(parint_serialize)
{
    TypeShort tShort(2), tShort2, tShort3;
    TypeUShort tUShort(3), tUShort2, tUShort3;
    TypeUInt tUInt(123), tUInt2, tUInt3;
    TypeDouble tDouble(4.5), tDouble2, tDouble3;
    TypeFloat tFloat(2.3), tFloat2, tFloat3;
    
    std::stringstream str;
    {
        boost::archive::text_oarchive oa(str);
        oa << tShort << tUShort << tUInt << tDouble << tFloat;
    }
    LogTrace(TRACE5) << "serialized stream: " << str.str();
    {
        boost::archive::text_iarchive ia(str);
        ia >> tShort2 >> tUShort2 >> tUInt2 >> tDouble2 >> tFloat2;
    }
#define CHECK_FAIL(val, val2) LogTrace(TRACE5) << #val << "=" << val << " " << #val2 "=" << val2; \
    fail_unless(val == val2, #val" failed");
    CHECK_FAIL(tShort, tShort2);
    CHECK_FAIL(tUShort, tUShort2);
    CHECK_FAIL(tUInt, tUInt2);
    CHECK_FAIL(tDouble, tDouble2);
    CHECK_FAIL(tFloat, tFloat2);

    // check XML serialize
    std::stringstream xmlstr;
    {
        boost::archive::xml_oarchive oa(xmlstr);
        oa << BOOST_SERIALIZATION_NVP(tShort)
            << BOOST_SERIALIZATION_NVP(tUShort)
            << BOOST_SERIALIZATION_NVP(tUInt)
            << BOOST_SERIALIZATION_NVP(tDouble)
            << BOOST_SERIALIZATION_NVP(tFloat);
    }
    LogTrace(TRACE5) << "serialized stream: " << xmlstr.str();

    {
        boost::archive::xml_iarchive ia(xmlstr);
        ia >> BOOST_SERIALIZATION_NVP(tShort3)
            >> BOOST_SERIALIZATION_NVP(tUShort3)
            >> BOOST_SERIALIZATION_NVP(tUInt3)
            >> BOOST_SERIALIZATION_NVP(tDouble3)
            >> BOOST_SERIALIZATION_NVP(tFloat3);
    }
    CHECK_FAIL(tShort, tShort3);
    CHECK_FAIL(tUShort, tUShort3);
    CHECK_FAIL(tUInt, tUInt3);
    CHECK_FAIL(tDouble, tDouble3);
    CHECK_FAIL(tFloat, tFloat3);
}END_TEST

START_TEST(parstr)
{
    StrType1 t1("v"), t2;
    fail_unless(t1.valid() == true);
    fail_unless(t2.valid() == false);
    LogTrace(TRACE5) << t1;
    LogTrace(TRACE5) << t2;
    try {
        const bool eq = StrType1("a") == StrType1();
        fail_if(eq);
    } catch (const StrType1::Exception& e) {
        // ok
    }
    try {
        StrType2("q");
    } catch (const StrType2::Exception& e) {
        // ok
    }
} END_TEST


#define SUITENAME "Serverlib"
TCASEREGISTER(0, 0)
    ADD_TEST(parint_compile)
    ADD_TEST(parint_init)
    ADD_TEST(parint_math)
    ADD_TEST(parint_compare)
    ADD_TEST(parint_serialize)
    ADD_TEST(parstr)
TCASEFINISH
#undef SUITENAME

#ifndef ENABLE_PG_TESTS
#define SUITENAME "SqlUtil"
TCASEREGISTER(testInitDB, 0)
    ADD_TEST(parint_types)
    ADD_TEST(parint_ocicpp)
    ADD_TEST(parint_def_null_changes)
    ADD_TEST(parint_bind_null)
    ADD_TEST(parint_bindOut)
    ADD_TEST(parstr_ocicpp)
TCASEFINISH
#undef SUITENAME
#endif /*ENABLE_PG_TESTS*/
} // namespace

#endif /* XP_TESTING */

