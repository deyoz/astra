#ifndef _HELPCPP_H_
#define _HELPCPP_H_
#ifdef __cplusplus
#define MY_BAD_CAST(x,y) reinterpret_cast<x>(y)
#else
#define MY_BAD_CAST(x,y) (x)(y)
#endif

#ifdef __cplusplus
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <list>
#include <vector>
#include <set>
#include <map>
#include <cmath>
#include <limits>
#include <algorithm>
#include "object.h"
#include "exception.h"

#define FIELD_LESS(lv, rv) \
      if (lv < rv) return true; \
      else if (lv > rv) return false



std::vector<std::string> getFilenamesByMask(std::string const &mask);
class BoolHolder {
    bool val;
    public:
    explicit BoolHolder(bool v):val(v){}
    bool getValue() const { return val;}
};

namespace OciCpp { class OciSession; }

namespace HelpCpp {

template<typename Type, size_t sz>
constexpr size_t array_size(const Type(&)[sz]) noexcept { return sz; }

#define ARRAY_SIZE(a) HelpCpp::array_size(a)

#define __regstack__concat(a,b) a##b
#define MarkStack HelpCpp::RegStack  __regstack__concat (mark,__LINE__) (BIGLOG)




class CallStackRegisterManager
{
    CallStackRegisterManager() {}
    CallStackRegisterManager(CallStackRegisterManager const&);
    CallStackRegisterManager& operator=(CallStackRegisterManager const&);
    struct point {
        point(char const* n, char const* f, int l, char const* fu)
            : nick(n), file(f), line(l), func(fu ? fu : "unknown function")  {
            }
        std::string nick;
        std::string file;
        int line;
        std::string func;
        std::list<std::string> messages;
    };
    std::list<point> l;
public:
    static const size_t pointMsgMaxNumber = 50; //magic number
    static CallStackRegisterManager& getInstance();
    void Register(char const* nick, char const* file, int line , char const* func) {
        l.push_back(point(nick, file, line, func));
    }
    void log(std::string const& s) {
        if (!l.empty()) {
            l.rbegin()->messages.push_back(s);
            //if maximum number of messages is exceeded, drop the oldest one
            while (l.rbegin()->messages.size() > pointMsgMaxNumber) {
                l.rbegin()->messages.pop_front();
            }
        }

    }
    void deregister() {
        if (l.empty()) {
            abort();
        }
        l.pop_back();
    }
    std::string dump();
};

class RegStack
{
public:
    RegStack(char const* nick, char const* file, int line , char const* func = 0) {
        CallStackRegisterManager::getInstance().Register(nick, file, line, func);
    }
    ~RegStack() {
        CallStackRegisterManager::getInstance().deregister();
    }
};

/**
 * Convert memory into printable hex representation
 * @param aBuf - mem start address
 * @param aBufLen - count of bytes to convert
 * @param markIndex - (optional) mark certain byte by index, e.g: DO [FE] DO
 */
std::string memdump(const void *aBuf, size_t aBufLen, int markIndex = -1);

template<typename T>
std::string bitdump(T val)
{
    const size_t bitlen = sizeof(T) / sizeof(uint8_t) * 8;
    std::string result(bitlen,'0');
    for(size_t i = bitlen; i > 0; --i)
        if(val & (T(1) << (i-1)))
            result[bitlen-i] = '1';
    return result;
}


template< typename Cont >
unsigned int list_to_mask(const Cont& cnt)
{
    unsigned int mask = 0;
    typedef typename Cont::const_iterator Iter;
    for (Iter i = cnt.begin(); i != cnt.end(); ++i) {
        mask |= (1 << i->get());
    }
    return mask;
}

template<typename Cont>
void mask_to_list(Cont& c, int mask, size_t len)
{
    typedef typename Cont::value_type value_type;
    for (size_t i = 0, j = 1; i < len; ++i, j <<= 1) {
        if (mask & j) {
            c.push_back(value_type(i));
        }
    }
}

/*
 * PtrLock - шаблон для хранения и автоматического
 * освобождения указателей
 * сделан по мотивам auto_ptr.
 * В качестве параметра шаблона используется
 * тип указателя, а не тип на который указывает указатель
 * Для реальной работы необходимо указать специализацию
 * функции PtrFree
*  example usage
* template <> void HelpCpp::PtrLock<xmlNodePtr>::PtrFree()
* {
*   xmlFreeNode(p);
* }
 *
 *
 * */
template <typename T> class PtrLock
{
    T p;
    void PtrFree();
    template <typename T2> struct PtrLockRef {
        T2 p;
        PtrLockRef(T2 pp): p(pp) {}
    };
public:
    PtrLock(PtrLock& t): p(t.release()) {
    }
    template <typename T2> PtrLock(PtrLock<T2> &t): p(t.release()) {
    }
    PtrLock& operator= (PtrLock& t) {
        if (&t != this && p != 0) {
            PtrFree();
        }
        p = t.release();
        return *this;
    }
    template <typename T2> PtrLock& operator= (PtrLock<T2> &t) {
        if (p != 0) {
            PtrFree();
        }
        p = t.release();
        return *this;
    }
    T release() {
        T p1 = get() ;
        p = 0;
        return p1;
    }
    T get() {
        return p;
    }

    explicit PtrLock(T pp): p(pp) {}
    explicit PtrLock(void* pp): p((T)pp) {
        (void)p->voidptr;
    }
    template <typename T2>  PtrLock(T2* pp): p(pp) {}
    void reset(T pp = 0) {
        if (p) PtrFree();
        p = pp;
    }
    ~PtrLock() {
        if (p) PtrFree();
    }
    T operator->() {
        return get();
    }
    template <typename T2> operator PtrLockRef<T2>() {
        return PtrLockRef<T2>(this->release());
    }
    template <typename T2> operator PtrLock<T2>() {
        return PtrLock<T2>(this->release());
    }
    PtrLock(PtrLockRef<T> pp): p(pp.p) {}
};

struct CAlloc {
    enum {voidptr};
};
template <>
inline void PtrLock<CAlloc*>::PtrFree()
{
   free(p);
}

typedef PtrLock<CAlloc*> MallocLock_base ;
class MallocLock : private MallocLock_base
{
public:
    MallocLock(void* pp): MallocLock_base(pp) {};
    void* get() {
        return MallocLock_base::get();
    }
    void* release() {
        return MallocLock_base::release();
    }
    void reset() {
        return MallocLock_base::reset();
    }
};

template <typename InputIterator, typename OutputIterator, typename Predicate>
OutputIterator copy_if(InputIterator begin, InputIterator end,
        OutputIterator destBegin, Predicate p)
{
    while (begin != end) {
        if (p(*begin)) *destBegin++ = *begin;
        ++begin;
    }
    return destBegin;
}

template<typename T, typename Predicate>
void move_if(std::vector<T>& srcs, std::vector<T>& dsts, Predicate pred)
{
    HelpCpp::copy_if(srcs.begin(), srcs.end(), std::back_inserter(dsts), pred);
    srcs.erase(std::remove_if(srcs.begin(), srcs.end(), pred), srcs.end());
}

struct cstring_eq {
bool operator()(const char *a, const char *b) const;
};
struct cstring_not_eq {
bool operator()(const char *a, const char *b) const;
};
struct cstring_less {
bool operator()(const char *a,const char *b) const;
};

template <int v>
struct Int2Type
{
    enum { value =v };
};

template<typename T,int L>
void ZeroArr(T t[L])
{
    memset(t,0,sizeof(t));
}

template<typename T>
void Zero(T &t)
{
    memset(&t,0,sizeof(T));
}
template<typename T>
void Memcopy(T &to,T const & from)
{
    memcpy(&to,&from,sizeof(T));
}

template<typename T>
int Memcmp(T const *t,T const *t2)
{
    return memcmp(t,t2,sizeof(T));
}

template<typename T>
int Memcmp(T const &t,T const &t2)
{
    return Memcmp(&t,&t2);
}

template<typename T>
struct StructComp
{
    bool operator()(T const &a,T const &b)
    {
        return Memcmp(a,b);
    }
};

template <typename T>
T *checkNull(T *m)
{
    if(!m)
        abort();
    return m;
}

const char *checkLen(const char *s, size_t len);

void abortError();

template <typename T, typename Comp=std::not_equal_to<T>, void E()=abortError>
class VarChecker
{
    std::pair<T, T const*> state;
public:
    bool bad() {
        if (Comp()(state.first, (*state.second))) {
            return true;
        }
        return false;
    }
    VarChecker(const T& t): state(t, &t) {};
    ~VarChecker() {
        if (bad()) {
            E();
        }
    }
};
std::string vsprintf_string_ap(char const *str , va_list ap);
std::string vsprintf_string(char const *str , ...);

template<typename T>
typename T::mapped_type mapAt(const T& m, typename T::key_type i) 
{
    typename T::const_iterator it(m.find(i));
    if (it == m.end()) {
        throw comtech::Exception("cannot find element in map");
    }
    return it->second;
}

template<typename T>
typename T::mapped_type findInMap(const T& m, typename T::key_type k, typename T::mapped_type def)
{
    typename T::const_iterator iter(m.find(k));
    return iter != m.end() ? iter->second : def;
}

template<typename T>
void setInsert(T &m, const typename T::value_type& v) 
{
    typename T::iterator it(m.find(v));
    if (it != m.end()) {
        throw comtech::Exception("duplicate element in set");
    }
    m.insert(it, v);
}

template<typename T>
void mapInsert(T  &m, const typename T::value_type& v)
{
    typename T::iterator it(m.find(v.first));
    if (it != m.end()) {
        throw comtech::Exception("duplicate element in map");
    }
    m.insert(it, v);
}

class pItemLock
{
    pItem p;
public:
    pItemLock(pItem p_) : p(p_) {}
    ~pItemLock() {
        deleteItem(p);
    }
};

// same as in Python v[start:end]
// startPos - start index, endPos - stop index
// if endPos is negative it counts backward
template<typename T>
T slice(const T& t, size_t startPos, int endPos = 0)
{
    typename T::const_iterator beg(t.begin()), end(t.begin());
    const size_t sz(t.size());
    if (startPos >= sz) {
        return T();
    }
    if (endPos < 0) {
        endPos = std::max(0, static_cast<int>(sz + endPos));
    }
    size_t __endPos = endPos;
    if (__endPos > sz || endPos == 0) {
        __endPos = sz;
    }
    if (__endPos == startPos && __endPos < sz) {
        ++__endPos;
    }
    if (__endPos <= startPos) {
        return T();
    }
    std::advance(beg, startPos);
    std::advance(end, __endPos);
    return T(beg, end);
}

template<class InputIterator, class Distance>
InputIterator advance(const InputIterator& i, Distance n)
{
    InputIterator it(i);
    std::advance(it, n);
    return it;
}

template<typename T1, typename T2>
bool is_subseq(const T1& beg, const T1& end, const T2& b_, const T2& e_)
{
    for (T2 i = b_; i != e_; ++i)
        if (std::find(beg, end, *i) == end)
            return false;
    return true;
}

template<typename T1, typename T2>
bool is_subseq(const T1& l, const T2& r)
{
    return is_subseq(l.begin(), l.end(), r.begin(), r.end());
}

// deprecated; use algo::contains
template<class C, typename E>
bool contains(const C& container, const E& element)
{
    return std::find(std::begin(container), std::end(container), element) != std::end(container);
}

// deprecated; use algo::contains
template<class T, typename E>
bool contains(const std::set<T>& container, const E& element)
{
    return container.find(element) != container.end();
}

// deprecated; use algo::contains
template<class K, class V, typename E>
bool contains(const std::map<K,V>& container, const E& element)
{
    return container.find(element) != container.end();
}

template<typename T1, typename T2> bool has_intersection(const T1& a, const T2& b)
{
    return std::any_of(a.begin(), a.end(), [&b](auto& e){ return contains(b,e); });
}

template<typename T1, typename T2>
T1 intersect(const T1& a, const T2& b)
{
    T1 c;
    std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), std::inserter(c, c.begin()));
    return c;
}

template <class T>
bool rough_eq(T lhs, T rhs, T epsilon = std::numeric_limits<T>::epsilon()) // operator==
{
  return std::abs(lhs - rhs) < epsilon;
}

template <class T>
bool rough_lt(T lhs, T rhs, T epsilon = std::numeric_limits<T>::epsilon()) // operator<
{
  return rhs - lhs >= epsilon;
       // tricky >= because if the difference is equal to epsilon
       // then they are not equal per the rough_eq method
}

template <class T>
bool rough_lte(T lhs, T rhs, T epsilon = std::numeric_limits<T>::epsilon()) // operator<=
{
  return rhs - lhs > -epsilon;
}

typedef uint64_t ObjIdType;

ObjIdType objId();
ObjIdType objId(const std::string& sequence, OciCpp::OciSession* sess = nullptr);

std::string convertToId(const std::string& src, unsigned len, const std::string& prefix, const std::string& dict);
std::string convertToId(const std::string& src, unsigned len, const std::string& prefix);
std::string convertToId(const uint64_t& src, unsigned len, const std::string& prefix);
uint64_t convertFromId(const std::string& src, unsigned prefixLength = 0);

} // namespace HelpCpp
#endif
#endif
