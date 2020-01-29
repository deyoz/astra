#if HAVE_CONFIG_H
#endif

/*pragma cplusplus*/
#include <string>
#include <set>
#include <typeinfo>
#include <algorithm>

#include "posthooks.h"

#define NICKNAME "SYSTEM"
#include "test.h"

namespace Posthooks {

BaseHook::~BaseHook()
{}

bool BaseHook::less(const BaseHook *h) const
{
    if(typeid(*this).before(typeid(*h)))
        return 1;
    if(typeid(*h).before(typeid(*this)))
        return 0;
    return less2(h);
}

class HookHolder
{
    BaseHook *hook;
    public:
        void run(void) const
        {
            hook->run();
        }
        HookHolder(const BaseHook& p):hook(p.clone()){}
        ~HookHolder()
        {
            delete hook;
        }
        HookHolder(const HookHolder& h)
        {
            hook=h.hook->clone();
        }
        void swap(HookHolder& h)
        {
            std::swap(hook, h.hook);
        }
        HookHolder& operator=(const HookHolder &h)
        {
            HookHolder tmp(h);
            swap(tmp);
            return *this;
        }
        bool operator < (const HookHolder& h) const
        {
            return hook->less(h.hook);
        }
};

ReqDefPar* ReqDefPar::Register() {
    Posthooks::sethAlways(Posthooks::Simple<ReqDefPar*>(this));
    return this;
}

template <> void Simple<ReqDefPar * >::run()
{
   delete p;
   p=0;
}

enum which { AFTER,BEFORE,ALWAYS,ROLLBACK,COMMIT, BASETABLES, AFTER_TEXT_PROC_OK };

inline const char* which2str(int w)
{
  return (w == AFTER ? "AFTER" :
          w == BEFORE ? "BEFORE" :
          w == ALWAYS ? "ALWAYS" :
          w == ROLLBACK ? "ROLLBACK" :
          w == COMMIT ? "COMMIT" :
          w == BASETABLES ? "BASETABLES" :
          w == AFTER_TEXT_PROC_OK ? "AFTER_TEXT_PROC_OK" : "\?\?\?");
}

using std::set;
set <HookHolder> *hbefore=0;
set <HookHolder> *hafter=0;
set <HookHolder> *halways=0;
set <HookHolder> *hrollb=0;
set <HookHolder> *hcommit=0;
set <HookHolder> *basetables_cache=0;
set <HookHolder> *hafterTextProcOk=0;

set <HookHolder> &getHook(which w)
{
    set <HookHolder> ** s = NULL;
    switch (w)
    {
        case AFTER:
            s=&hafter;
            break;
        case BEFORE:
            s=&hbefore;
            break;
        case ALWAYS:
            s=&halways;
            break;
        case ROLLBACK:
            s=&hrollb;
            break;
        case COMMIT:
            s=&hcommit;
            break;
        case BASETABLES:
            s=&basetables_cache;
            break;
        case AFTER_TEXT_PROC_OK:
            s=&hafterTextProcOk;
            break;
    }
    if(*s==0){
        *s=new set<HookHolder>;
    }
    return **s;
}

void seth(const BaseHook& p, which w)
{
    getHook(w).insert(HookHolder(p));
}
void seth(void (*p)(void), which w)
{
    seth(SimpleFunction(p),w);
}
void sethAfter(const BaseHook& p)
{
    seth(p, AFTER);
}
void sethAfterTextProcOk(const BaseHook &p)
{
    seth(p, AFTER_TEXT_PROC_OK);
}
void sethBefore(const BaseHook& p)
{
    seth(p, BEFORE);
}
void sethAlways(const BaseHook& p)
{
    seth(p, ALWAYS);
}
void sethRollb(const BaseHook& p)
{
    seth(p, ROLLBACK);
}
void sethCommit(const BaseHook& p)
{
    seth(p, COMMIT);
}
void sethCleanCache(const BaseHook& p)
{
    seth(p, BASETABLES);
}

void callPostHooks(which w)
{
    set<HookHolder>& s = getHook(w);
    if (!s.empty()) {
        ProgTrace(TRACE5, "%s %s(%d) size=%zd",
                __FUNCTION__, which2str(w), w, s.size());
        std::for_each(s.begin(), s.end(), [](auto&hook){hook.run();});
    }
}

}
using namespace Posthooks;

extern "C"
void registerHookBefore(void (*p)(void))
{
    seth(p,BEFORE);
}
extern "C"
void registerHookAlways(void (*p)(void))
{
    seth(p,ALWAYS);
}
extern "C"
void registerHookAfter(void (*p)(void))
{
    seth(p,AFTER);
}
extern "C"
void registerHookRollback(void (*p)(void))
{
    seth(p,ROLLBACK);
}
extern "C"
void registerHookCommit(void (*p)(void))
{
    seth(p,COMMIT);
}
extern "C"
void emptyHookTables(void)
{
    getHook(AFTER).clear();
    getHook(BEFORE).clear();
    getHook(ALWAYS).clear();
    getHook(ROLLBACK).clear();
    getHook(COMMIT).clear();
    getHook(AFTER_TEXT_PROC_OK).clear();
    callPostHooks(BASETABLES);

}
extern "C"
void emptyCommitHooksTables(void)
{
  getHook(COMMIT).clear();
}

extern "C"
void callPostHooksBefore(void)
{
    callPostHooks(BEFORE);
}

extern "C"
void callPostHooksAfter(void)
{
    callPostHooks(AFTER);
}
extern "C"
void callPostHooksAlways(void)
{
    callPostHooks(ALWAYS);
}
extern "C"
void callRollbackPostHooks(void)
{
    callPostHooks(ROLLBACK);
}
extern "C"
void callPostHooksCommit(void)
{
    callPostHooks(COMMIT);
}
extern "C"
void callPostHooksAfterTextProcOk(void)
{
    callPostHooks(AFTER_TEXT_PROC_OK);
}
#include "checkunit.h"
#ifdef XP_TESTING
namespace {
int i;
void setup (void)
{

    emptyHookTables();
    i=10;
}

void teardown (void)
{
}
class test_del : public ReqDefPar {
    public:
        ~test_del(){
            i=5;
         }
};
void test_post_hook1()
{
    i=1000;
}
void test_post_hook2()
{
    i=2000;
}
void test_post_hook3()
{
    i=3000;
}
void test_post_hook4()
{
    i++;
}

START_TEST(ReqDefPar_test)
{
    (new test_del)->Register();
    fail_unless(i==10,"bred");
    callPostHooksAlways();
    fail_unless(i==5,"destructor not called by callPostHooksAlways");
}
END_TEST
START_TEST(test_add_hook)
{
    fail_unless(i==10,"initially i must be 10");
    registerHookBefore(test_post_hook1);
    registerHookAfter(test_post_hook2);
    registerHookAlways(test_post_hook3);
    callPostHooksBefore();
    fail_unless(i==1000,"test_post_hook1 must set i to 1000");
    callPostHooksAfter();
    fail_unless(i==2000,"test_post_hook2 must set i to 2000");
    callPostHooksAlways();
    fail_unless(i==3000,"test_post_hook2 must set i to 2000");
}
END_TEST
START_TEST(test_empty_hook)
{
    fail_unless(i==10,"initially i must be 10");
    registerHookAfter(test_post_hook1);
    registerHookAfter(test_post_hook2);
    emptyHookTables();
    callPostHooksAfter();
fail_unless(i==10,"test_post_hook1 should not be called");
}
END_TEST
START_TEST(test_add_many)
{
    fail_unless(i==10,"initially i must be 10");
    registerHookAfter(test_post_hook4);
    registerHookAfter(test_post_hook4);
    registerHookAfter(test_post_hook4);
    registerHookAfter(test_post_hook4);
    callPostHooksAfter();
    fail_unless(i==11,"test_post_hook1 must be called only once");
}
END_TEST

#define SUITENAME "Serverlib"
TCASEREGISTER(setup,teardown)
  ADD_TEST(test_add_hook);
  ADD_TEST(test_empty_hook);
  ADD_TEST(test_add_many);
  ADD_TEST(ReqDefPar_test);
TCASEFINISH
}
#endif //XP_TESTING
