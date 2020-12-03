#ifndef SERVERLIB_CHECKUNIT_NEW_H 
#define SERVERLIB_CHECKUNIT_NEW_H

#ifdef XP_TESTING

#include <string>

namespace TestCases
{
typedef void(*TestFuncPtr)(int);
typedef void(*SetupFuncPtr)();
typedef void(*TeardownFuncPtr)();
struct SirenaSuite;

SirenaSuite* createSuite(const char*, SetupFuncPtr, TeardownFuncPtr);
void setTimeout(struct SirenaSuite*, int);
void addTest(struct SirenaSuite*, const char*, const char*, TestFuncPtr);
int GetJobIdx();
} // namespace TestCases

#define _TCASES_CONCAT(x,y) x##y
#define _TCASES_CONCAT2(x,y) _TCASES_CONCAT(x,y)
#define _TCASES_STR(x) #x
#define _TCASES_NAME(x,y) x ":" _TCASES_STR(y)

#define TCASEREGISTER(setup_, teardown_) \
namespace  { \
    struct _TCASES_CONCAT2(tcu,__LINE__) { \
        _TCASES_CONCAT2(tcu,__LINE__)() { \
            TestCases::SirenaSuite* ss = \
                TestCases::createSuite(SUITENAME, setup_, teardown_);

#define SET_TIMEOUT(x) TestCases::setTimeout(ss, x);

#define ADD_TEST(x) TestCases::addTest(ss, #x , SUITENAME "." #x , x);

#define TCASEFINISH \
        } \
    } _TCASES_CONCAT2(tcu,__LINE__); \
}

#include <check.h>
// clashes with std::iostream::fail so often that _IT_ REALLY_ _HURTS_ _TO_ _THE_ _BONE_
#ifdef fail
  #undef fail
#endif
#ifdef ck_abort_msg
  #undef ck_abort_msg
#endif
#ifdef ck_abort
  #undef ck_abort
#endif

// since check.h ver. 0.9.10 there does not exist _fail_unless
#if (CHECK_MAJOR_VERSION*10000+CHECK_MINOR_VERSION*100+CHECK_MICRO_VERSION >= 910)
 #define _fail_unless ck_assert_msg
#endif

// since check.h ver. 0.9.12 there does not exist _ck_assert_msg
#if (CHECK_MAJOR_VERSION*10000+CHECK_MINOR_VERSION*100+CHECK_MICRO_VERSION >= 912)
 #define _ck_assert_msg ck_assert_msg
#endif

template <typename S> auto __wtfailed_c_str(S&& s) -> decltype(s.c_str()) {  return s.c_str();  }
const char* __wtfailed_c_str(const char* s); // {  return s;  }

#define __wtfailed_vargs(e,f,l,format,...) do { if(e) { _ck_assert_failed((f),(l),"failure",(format),__VA_ARGS__); } _mark_point(f,l); } while(false)
#define __wtfailed(e,f,l,r) __wtfailed_vargs((e),(f),(l),"%s",__wtfailed_c_str(r))

#define CHECK_EQUAL_STRINGS(s1, s2)  ck_assert_str_eq((s1), (s2))

#ifdef _ck_assert_str
#undef _ck_assert_str
#define _ck_assert_str(X, OP, Y, NULLEQ, NULLNE) \
    __local_ck_assert_str(__FILE__,__LINE__, __wtfailed_c_str(X), __wtfailed_c_str(Y), \
                          [](const char* l, const char* r){ return l and r and 0 OP strcmp(l,r); }, \
                          "Assertion '"#X" "#OP" "#Y"' failed: left is \"%s\", right is \"%s\"", \
                          NULLEQ, NULLNE);
#endif

#define ck_assert_strstr(HAYSTACK, NEEDLE) \
    __local_ck_assert_str(__FILE__,__LINE__, __wtfailed_c_str(HAYSTACK), __wtfailed_c_str(NEEDLE), \
                          [](const char* h, const char* n){ return std::strstr(h,n) != nullptr; }, \
                          "Assertion '"#NEEDLE" `in` "#HAYSTACK"' failed: looked in '%s' for '%s'", false, false)

template <class Cmp> void __local_ck_assert_str(const char* f, int l, const char* s1, const char* s2, Cmp&& cmp, const char* failmsg, bool nulleq, bool nullne)
{
    if(nulleq and not s1 and not s2) {
        _mark_point(f,l);
        return;
    }
    if(nullne and (bool(s1) xor bool(s2))) {
        _mark_point(f,l);
        return;
    }
    __wtfailed_vargs(not cmp(s1, s2), f, l, failmsg, s1, s2);
}

namespace xp_testing {

std::string maybe_recode_answer(std::string&& answer);

}

#endif // XP_TESTING

#endif /* SERVERLIB_CHECKUNIT_NEW_H */

