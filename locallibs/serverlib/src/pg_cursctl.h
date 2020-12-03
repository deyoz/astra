#pragma once
#ifdef ENABLE_PG
#include <memory>
#include <array>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <string.h>
#include "exception.h"

namespace boost { namespace gregorian { class date; } }
namespace boost { namespace posix_time { class ptime; } }

namespace dbcpp { template <typename C> class BaseRow; }

namespace PgCpp
{
const short* nullInd(bool isNull = true) noexcept;

class CursCtl;
class Session;

namespace details {
class SessionManager;
struct SessionDescription;
class PgExecutor;
class PgFetcher;
}

using PgOid_t = unsigned int; // typedef Oid in libpq-fe.h
using PgResult = void*;
using SessionDescriptor = details::SessionDescription*;

enum ResultCode
{
    Ok = 0,
    Fatal,
    BadSqlText,
    BadBind,
    NoDataFound,
    TooManyRows,
    ConstraintFail,
    FetchNull,
    FetchFail,
    Busy,
    Deadlock,
    WaitExpired,
    BadConnection,
    ReadOnly,
};

SessionDescriptor getManagedSession(const std::string&);
SessionDescriptor getReadOnlySession(const std::string&);
SessionDescriptor getAutoCommitSession(const std::string&);

void commit();
void rollback();
void makeSavepoint(const std::string& name);
void rollbackSavepoint(const std::string& name);
void resetSavepoint(const std::string& name);
#ifdef XP_TESTING
void commitInTestMode();
void rollbackInTestMode();
#endif // XP_TESTING

class PgException : public comtech::Exception
{
public:
    PgException(SessionDescriptor, ResultCode, const std::string&);
    PgException(const char* n, const char* f, int l, SessionDescriptor, ResultCode, const std::string&);
    virtual ~PgException() {}

    ResultCode errCode() const;
private:
    ResultCode erc_;
};

// Binary data
struct BinaryBindHelper
{
    const char* data;
    size_t size;
};

template<typename Container>
struct BinaryDefHelper
{
    Container& c;
};

namespace details
{

struct Argument
{
    PgOid_t type;
    int format;
    int length;
};

struct DefArg
{
    Argument arg;
    char* value;
    short* ind;
    int defaultValue;
    bool (*setNull) (char*);
    bool (*setValue) (char*, const char* data, int len);
    void (*setDefault) (char*);
};

struct BindArg
{
    Argument arg;
    std::vector<char> value;
    // unstable mode only:
    const short* ind;
    const void* data;
    void (*setValue) (std::vector<char>& dst, const void* data);
};

enum class PgOid : PgOid_t { // values from pg_type.h
    Float4 = 700,
    Float8 = 701,
    Int8 = 20,
    Boolean = 16,
    ByteArray = 17,
    Int4 = 23,
    Text = 25,
    Json = 114,
    Xml = 142,
    AbsDateTime = 702,
    RelDateTime = 703,
    BpChar = 1042, // blank-padded char
    Varchar = 1043,
    Date = 1082,
    TimeOfDay = 1083,
    Timestamp = 1114,
    TimestampZ = 1184,
    TimeOfDayZ = 1266,
    Uuid = 3220,
};

template<typename T>
struct PgTraits
{
};

template<>
struct PgTraits<float>
{
    static const PgOid oid = PgOid::Float4;
    static const int format = 1;

    static int length(float) { return sizeof(float); }
    static bool setNull(char* value);
    static void fillBindData(std::vector<char>& dst, float v);
    static bool setValue(char* value, const char* data, int);
};

template<>
struct PgTraits<double>
{
    static const PgOid oid = PgOid::Float8;
    static const int format = 1;

    static int length(double) { return sizeof(double); }
    static bool setNull(char* value);
    static void fillBindData(std::vector<char>& dst, double v);
    static bool setValue(char* value, const char* data, int);
};

template<>
struct PgTraits<int>
{
    static const PgOid oid = PgOid::Int4;
    static const int format = 1;

    static int length(int) { return sizeof(int); }
    static bool setNull(char* value);
    static void fillBindData(std::vector<char>& dst, int v);
    static bool setValue(char* value, const char* data, int);
};

template<>
struct PgTraits<short>
{
    static const PgOid oid = PgOid::Int4;
    static const int format = 1;

    static int length(int) { return sizeof(short); }
    static bool setNull(char* value);
    static void fillBindData(std::vector<char>& dst, short v);
    static bool setValue(char* value, const char* data, int);
};

template<>
struct PgTraits<unsigned short> : PgTraits<short> {};

template<>
struct PgTraits<unsigned int> : PgTraits<int> {};

template<>
struct PgTraits<bool>
{
    static const PgOid oid = PgOid::Boolean;
    static const int format = 1;

    static int length(bool) { return sizeof(bool); }
    static bool setNull(char* value);
    static void fillBindData(std::vector<char>& dst, bool v);
    static bool setValue(char* value, const char* data, int);
};

template<>
struct PgTraits<long long>
{
    static const PgOid oid = PgOid::Int8;
    static const int format = 1;

    static int length(long long) { return sizeof(long long); }
    static bool setNull(char* value);
    static void fillBindData(std::vector<char>& dst, long long v);
    static bool setValue(char* value, const char* data, int);
};

template<>
struct PgTraits<unsigned long long> : PgTraits<long long> {};

template<>
struct PgTraits<long> : PgTraits<typename std::conditional<sizeof(long) == sizeof(long long),
    long long, int>::type> {};

template<>
struct PgTraits<unsigned long> : PgTraits<typename std::conditional<sizeof(long) == sizeof(long long),
    long long, int>::type> {};

template<>
struct PgTraits<boost::gregorian::date>
{
    static const PgOid oid = PgOid::Date;
    static const int format = 0;

    static int length(const boost::gregorian::date&) { return 0;; }
    static bool setNull(char* value);
    static void fillBindData(std::vector<char>& dst, const boost::gregorian::date&);
    static bool setValue(char* value, const char* data, int);
};

template<>
struct PgTraits<boost::posix_time::ptime>
{
    static const PgOid oid = PgOid::Timestamp;
    static const int format = 0;

    static int length(const boost::posix_time::ptime&) { return 0; }
    static bool setNull(char* value);
    static void fillBindData(std::vector<char>& dst, const boost::posix_time::ptime&);
    static bool setValue(char* value, const char* data, int);
};

template<>
struct PgTraits<std::string>
{
    static const PgOid oid = PgOid::Varchar;
    static const int format = 0;

    static int length(const std::string& ) { return 0; }
    static bool setNull(char* value);
    static void fillBindData(std::vector<char>& dst, const std::string& s);
    static bool setValue(char* value, const char* data, int len);
};

template<int N>
struct PgTraits<char[N]>
{
    static const PgOid oid = PgOid::Varchar;
    static const int format = 0;

    static int length(const char[N]) { return 0; }
    static bool setNull(char* value) { *value = 0; return true; }
    static void fillBindData(std::vector<char>& dst, const char* s) {
        const int sz = strnlen(s, N);
        if (sz == 0) {
            dst.clear();
            return;
        }
        dst.resize(sz + 1);
        memcpy(dst.data(), s, sz);
        dst[sz] = 0;
    }
    static bool setValue(char* value, const char* data, int len) {
        if (len > N) {
            return false;
        }
        memcpy(value, data, len);
        if (len < N) {
            value[len] = 0;
        }
        return true;
    }
};

template<>
struct PgTraits<const char*>
{
    static const PgOid oid = PgOid::Varchar;
    static const int format = 0;

    static int length(const char* ) {
        return 0;
    }
    // static bool setNull - def is not allowed
    static void fillBindData(std::vector<char>& dst, const char* s);
    // static bool setValue - def is not allowed
};

template<>
struct PgTraits<char>
{
    static const PgOid oid = PgOid::Varchar;
    static const int format = 0;

    static int length(char) { return 0; }
    static bool setNull(char* value);
    static void fillBindData(std::vector<char>& dst, char v);
    static bool setValue(char* value, const char* data, int);
};

bool fillCharBuffFromHex(char*, int, const char*);

template<std::size_t N>
struct PgTraits<std::array<char, N>>
{
    static const PgOid oid = PgOid::ByteArray;
    static const int format = 1;

    static int length(const std::array<char, N>&) { return N; }
    static bool setNull(char* value) {
        auto p = reinterpret_cast<std::array<char, N>*>(value);
        p->fill(0);
        return true;
    }
    static void fillBindData(std::vector<char>& dst, const std::array<char, N>& v) {
        dst.resize(N);
        memcpy(dst.data(), v.data(), N);
    }
    static bool setValue(char* value, const char* data, int /*l*/) {
        auto p = reinterpret_cast<std::array<char, N>*>(value);
        p->fill(0);
        return fillCharBuffFromHex(p->data(), N, data);
    }
};

// bind only
template<>
struct PgTraits<BinaryBindHelper>
{
    static const PgOid oid = PgOid::ByteArray;
    static const int format = 1;

    static int length(const BinaryBindHelper& b) { return b.size; }
    static void fillBindData(std::vector<char>& dst, const BinaryBindHelper& b) {
        dst.resize(b.size);
        memcpy(dst.data(), b.data, b.size);
    }
};

// def only
template<typename Container>
struct PgTraits<BinaryDefHelper<Container>>
{
    static const PgOid oid = PgOid::ByteArray;
    static const int format = 1;

    using this_type = BinaryDefHelper<Container>;

    static int length(const this_type& b) { return b.c.size(); }
    static bool setNull(char* value) {
        reinterpret_cast<this_type*>(value)->c.clear();
        return true;
    }
    static bool setValue(char* value, const char* data, int l) {
        auto c = reinterpret_cast<this_type*>(value);
        c->c.resize((l - 2) / 2);
        return fillCharBuffFromHex(&(c->c[0]), l, data);
    }
};

template<typename T>
Argument makeTypeDesc(const T& t)
{
    return Argument{
        static_cast<PgOid_t>(PgTraits<T>::oid),
        PgTraits<T>::format,
        PgTraits<T>::format ? PgTraits<T>::length(t) : 0};
}

class IDefaultValueHolder
{
public:
    virtual ~IDefaultValueHolder() {}
    virtual void setValue(void*) = 0;
};

template<typename T1, typename T2>
class DefaultValueHolder : public IDefaultValueHolder
{
public:
    static constexpr bool isConstructibe = std::is_same<T1, T2>::value || std::is_constructible<T1, T2>::value;
    DefaultValueHolder(const T2& d)
        : defaultValue_(d)
    {}
    void setValue(void* v) override {
        T1* p = reinterpret_cast<T1*>(v);
        *p = T1(defaultValue_);
    }
private:
    const T2 defaultValue_;
};

template<>
class DefaultValueHolder<std::string, const char*> : public IDefaultValueHolder
{
public:
    static constexpr bool isConstructibe = true;
    DefaultValueHolder(const char* d)
        : defaultValue_(d)
    {}
    void setValue(void* v) override {
        std::string* p = reinterpret_cast<std::string*>(v);
        *p = defaultValue_;
    }
private:
    const std::string defaultValue_;
};

template<int N>
class DefaultValueHolder<std::string, char[N]> : public IDefaultValueHolder
{
public:
    static constexpr bool isConstructibe = true;
    DefaultValueHolder(const char* d)
    {
        strncpy(defaultValue_, d, N); // stop at first NULL-char
        defaultValue_[N] = 0;
    }
    void setValue(void* v) override {
        std::string* p = reinterpret_cast<std::string*>(v);
        *p = defaultValue_;
    }
private:
    char defaultValue_[N + 1];
};

template<int N1, int N2>
class DefaultValueHolder<char[N1], char[N2]> : public IDefaultValueHolder
{
public:
    static constexpr bool isConstructibe = N1 >= N2;
    DefaultValueHolder(const char* d)
    {
        memcpy(defaultValue_, d, N2);
    }
    void setValue(void* v) override {
        char* p = reinterpret_cast<char*>(v);
        memcpy(p, defaultValue_, N2);
    }
private:
    char defaultValue_[N2];
};

void handleError(const CursCtl&, SessionDescriptor, ResultCode, const char*, const char* = nullptr);
bool checkResult(CursCtl& cur, SessionDescriptor, PgResult result);
void dumpCursor(const CursCtl&);
} // details

class CursCtl
{
    friend class Session;
    friend class details::PgFetcher;
private:
    CursCtl(std::shared_ptr<Session> sess, const char* n, const char* f, int l, std::shared_ptr<details::PgExecutor> executor);
public:
    ~CursCtl();

    CursCtl(CursCtl&&); // does not need implementation due to RVO

    CursCtl(const CursCtl&) = delete;
    CursCtl& operator=(const CursCtl&) = delete;
    CursCtl& operator=(CursCtl&&) = delete;

    template<typename T>
    CursCtl& def(T& t, short* ind = 0)
    {
        using details::PgTraits;
        details::DefArg a{details::makeTypeDesc(t),
            reinterpret_cast<char*>(&t),
            ind ? ind : (autoNull_ ? &defIndPlaceholder_ : nullptr),
            -1,
            &PgTraits<T>::setNull,
            &PgTraits<T>::setValue
        };
        defs_.push_back(a);
        return *this;
    }

    template<typename T1, typename T2>
    CursCtl& defNull(T1& t, const T2& defVal)
    {
        using details::PgTraits;
        static_assert(details::DefaultValueHolder<T1, T2>::isConstructibe,
                "invalid type for default value");
        defaults_.push_back(new details::DefaultValueHolder<T1, T2>(defVal));
        details::DefArg a{details::makeTypeDesc(t),
            reinterpret_cast<char*>(&t),
            &defIndPlaceholder_,
            static_cast<int>(defaults_.size() - 1),
            &PgTraits<T1>::setNull,
            &PgTraits<T1>::setValue,
        };
        defs_.push_back(a);
        return *this;
    }

    CursCtl& defRow(dbcpp::BaseRow<CursCtl>&);

    template <typename T, typename = std::enable_if_t<!std::is_lvalue_reference<T>::value>>
    CursCtl& bind(const std::string& pl, T&& t, const short* ind = 0)
    {
        if (!stable_) {
            throwBadBindException(__FILE__, __LINE__, std::string("unstable rvalue bind: ") + pl);
        }
        bind_(pl, t, ind);
        return *this;
    }

    template <typename T>
    CursCtl& bind(const std::string& pl, const T& t, const short* ind = 0)
    {
        bind_(pl, t, ind);
        return *this;
    }

    CursCtl& autoNull()
    {
        autoNull_ = true;
        return *this;
    }

    CursCtl& noThrowError(ResultCode err)
    {
        noThrowErrors_.insert(err);
        return *this;
    }

    CursCtl& stb() {
        if (!binds_.empty()) {
            throwBadBindException(__FILE__, __LINE__, "can't change bind mode with existing binds");
        }
        stable_ = true;
        return *this;
    }

    CursCtl& unstb() {
        if (!binds_.empty()) {
            throwBadBindException(__FILE__, __LINE__, "can't change bind mode with existing binds");
        }
        stable_ = false;
        return *this;
    }

    CursCtl& singleRowMode() {
        singleRowMode_ = true;
        return *this;
    }

    int exfet();
    int EXfet();
    int exec();
    int fen();

    ResultCode err() const { return erc_; }
    int rowcount();

#ifdef XP_TESTING
public:
#else
private:
#endif // !XP_TESTING
    static short defIndPlaceholder_;
    std::shared_ptr<Session> sess_;
    std::shared_ptr<details::PgExecutor> exectr_;
    std::unique_ptr<details::PgFetcher> fetcher_;
    ServerFramework::NickFileLine pos_;
    ResultCode erc_;
    bool autoNull_;
    bool stable_;
    bool singleRowMode_;
    std::map<std::string, details::BindArg> binds_;
    std::vector<details::DefArg> defs_;
    std::set<ResultCode> noThrowErrors_;
    std::vector<details::IDefaultValueHolder*> defaults_;

    void appendBinding(const std::string& pl, const details::BindArg&);
    bool hasBinding(const std::string& pl);
    PgResult execSql();

    template <typename T>
    void bind_(const std::string& pl_, const T& t, const short* ind = nullptr)
    {
        using details::PgTraits;

        std::string pl;
        if (pl_[0] == ':')
            pl = pl_.substr(1);
        else
            pl = pl_;

        if (!hasBinding(pl)) {
            throwBadBindException(__FILE__, __LINE__, std::string("no such bind: ") + pl);
        }

        details::BindArg ba{details::makeTypeDesc(t)};
        if (stable_) {
            if (!ind || *ind != -1) {
                PgTraits<T>::fillBindData(ba.value, t);
            }
        } else {
            ba.ind = ind;
            ba.data = &t;
            ba.setValue = [](std::vector<char>& dst, const void* data){
                PgTraits<T>::fillBindData(dst, *static_cast<const T*>(data));
            };
        }

        appendBinding(pl, ba);
    }

    void throwBadBindException(const char* f, int l, const std::string& err);

    friend void details::handleError(const CursCtl&, SessionDescriptor, ResultCode, const char*, const char*);
    friend bool details::checkResult(CursCtl&, SessionDescriptor, PgResult result);
    friend void details::dumpCursor(const CursCtl&);
};

CursCtl make_curs_(const char* n, const char* f, int l, SessionDescriptor, const char* sql);
CursCtl make_curs_(const char* n, const char* f, int l, SessionDescriptor, const std::string& sql);
CursCtl make_curs_autonomous_(const char* n, const char* f, int l, SessionDescriptor, const char* sql);
CursCtl make_curs_autonomous_(const char* n, const char* f, int l, SessionDescriptor, const std::string& sql);
CursCtl make_curs_nocache_(const char* n, const char* f, int l, SessionDescriptor, const char* sql);
CursCtl make_curs_nocache_(const char* n, const char* f, int l, SessionDescriptor, const std::string& sql);

bool copyDataFrom(Session& sess, const std::string& sql, const char* data, size_t size);

} // PgCpp

#ifdef make_pg_curs
#error make_pg_curs is already defined
#endif // make_curs
#define make_pg_curs(sd, x) PgCpp::make_curs_(STDLOG, sd, (x))
#define make_pg_curs_autonomous(sd, x) PgCpp::make_curs_autonomous_(STDLOG, sd, (x))
#define make_pg_curs_nocache(sd, x) PgCpp::make_curs_nocache_(STDLOG, sd, (x))
#endif // ENABLE_PG
