#ifndef _POSTHOOKS_
#define _POSTHOOKS_
#ifdef __cplusplus

#include <utility>

namespace  Posthooks
{

class BaseHook
{
public:
    virtual ~BaseHook();
    bool less(const BaseHook *h) const;
    virtual void run() = 0;
    virtual BaseHook* clone() const = 0;
private:
    virtual bool less2(const BaseHook *) const noexcept = 0;
};

template <typename T> class Simple : public BaseHook
{
    T p;
  public:
    template <typename A> explicit Simple(A&& pp) : p(std::forward<A>(pp)) {}
    void run() override final { p(); }
    Simple<T> *clone() const override final { return new Simple<T>(*this); }
    bool less2(const BaseHook*h) const noexcept override final { return p < static_cast<const Simple*>(h)->p; }
};

template <typename T> Simple<T> make_simple(T&& p) { return Simple<T>(std::forward<T>(p)); }

typedef Simple<void(*)()> SimpleFunction;

void sethAfter(const BaseHook& p);
void sethBefore(const BaseHook& p);
void sethAlways(const BaseHook& p);
void sethRollb(const BaseHook& p);
void sethCommit(const BaseHook& p);
void sethCleanCache(const BaseHook& p);
void sethAfterTextProcOk(const BaseHook &p);

struct ReqDefPar
{
    virtual ~ReqDefPar() = default;
    ReqDefPar* Register();
};
template <> void Simple<ReqDefPar * >::run();

template <typename T> struct ReqDefParHolder : public ReqDefPar
{
    T data;
    ReqDefParHolder(T const &t) : data(t) {}
};

template <typename T> ReqDefPar* registerReqDefPar(const T& t)
{
    auto ptr = new ReqDefParHolder<T>(t);
    return ptr->Register();
}

template <typename T> bool getReqDefPar(const ReqDefPar* rdp, T& t)
{
    if(auto ptr = dynamic_cast<ReqDefParHolder<T> const*>(rdp))
    {
        t = ptr->data;
        return true;
    }
    return false;
}

template <typename T> T* getReqDefParInPlace( ReqDefPar* rdp )
{
    if(auto ptr = dynamic_cast<ReqDefParHolder<T> *>(rdp))
        return & ptr->data;
    return nullptr;
}

} // namespace Posthooks

extern "C" {
#endif
void registerHookBefore(void (*p)(void));
void registerHookAfter(void (*p)(void));
void registerHookAlways(void (*p)(void));
void registerHookRollback(void (*p)(void));
void registerHookCommit(void (*p)(void));

void callPostHooksAlways(void);
void callPostHooksBefore(void);
void callPostHooksAfter(void);
void callRollbackPostHooks(void);
void callPostHooksCommit(void);
void callPostHooksAfterTextProcOk(void);
void emptyHookTables(void);
void emptyCommitHooksTables(void);
#ifdef __cplusplus
}
#endif
#endif
