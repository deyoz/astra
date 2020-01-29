#include "dbcpp_cursctl.h"
#ifdef ENABLE_PG

#include <boost/date_time/posix_time/posix_time.hpp>
#include <map>

#include "cursctl.h"
#include "dates_oci.h"
#include "dbcpp_db_type.h"
#include "dbcpp_session.h"
#include "enum.h"
#include "oci_err.h"
#include "pg_cursctl.h"
#include "test.h"
#include "testmode.h"

#define NICKNAME "ASM"
#include "slogger.h"

using PgOid = PgCpp::details::PgOid;

struct CharBuffer
{
    char* data;
    size_t size;
    bool deepCopy;
    PgOid oid;

    CharBuffer(const char* data_, size_t size_, bool deepCopy_, PgOid oid_ = PgOid::Varchar)
        : data(const_cast<char*>(data_))
        , size(size_)
        , deepCopy(deepCopy_)
        , oid(oid_)
    {
        if (deepCopy)
        {
            data = new char[size];
            memcpy(data, data_, size);
        }
    }
    CharBuffer(const CharBuffer& other)
    {
        size     = other.size;
        deepCopy = other.deepCopy;
        oid      = other.oid;
        if (deepCopy)
        {
            data = new char[size];
            memcpy(data, other.data, size);
        } else
        {
            data = other.data;
        }
    }

    CharBuffer& operator=(const CharBuffer& other)
    {
        ASSERT(size >= other.size);
        memcpy(data, other.data, size);
        return *this;
    }
    ~CharBuffer()
    {
        if (deepCopy)
        {
            delete[] data;
        }
    }
};

namespace PgCpp
{
    namespace details
    {
        template <> class DefaultValueHolder<CharBuffer, CharBuffer> : public IDefaultValueHolder
        {
        public:
            static constexpr bool isConstructibe = true;
            DefaultValueHolder(const CharBuffer& d)
                : defaultValue_(d)
            {
            }
            void setValue(void* v) override
            {
                CharBuffer* p = reinterpret_cast<CharBuffer*>(v);
                memcpy(p->data, defaultValue_.data, defaultValue_.size);
            }

        private:
            CharBuffer defaultValue_;
        };

        template <> struct PgTraits<CharBuffer>
        {
            static const int format = 0;

            using this_type = CharBuffer;

            static int length(const this_type& b) { return 0; }
            static bool setNull(char* value)
            {
                auto p     = reinterpret_cast<CharBuffer*>(value);
                p->data[0] = '\0';
                return true;
            }
            static void fillBindData(std::vector<char>& dst, const CharBuffer& s)
            {
                if (s.oid == PgOid::Varchar)
                {
                    const int sz = strnlen(s.data, s.size);
                    if (sz == 0)
                    {
                        dst.clear();
                        return;
                    }
                    dst.resize(sz + 1);
                    memcpy(dst.data(), s.data, sz);
                    dst[sz] = 0;
                } else
                {
                    dst.resize(s.size + 1);
                    memcpy(dst.data(), s.data, s.size);
                }
            }
            static bool setValue(char* value, const char* data, int len)
            {
                CharBuffer* p = reinterpret_cast<CharBuffer*>(value);
                if (static_cast<size_t>(len) > p->size)
                {
                    return false;
                }
                memcpy(p->data, data, len);
                if (static_cast<size_t>(len) < p->size)
                {
                    p->data[len] = '\0';
                }
                return true;
            }
        };
        template <> Argument makeTypeDesc(const CharBuffer& t)
        {
            return Argument{ static_cast<PgOid_t>(t.oid), PgTraits<CharBuffer>::format,
                             PgTraits<CharBuffer>::format ? PgTraits<CharBuffer>::length(t) : 0 };
        }
    }
}
namespace OciCpp
{
    template <> void default_ptr_t_fill(void* out, CharBuffer const& m)
    {
        memcpy(out, m.data, m.size);
    }
    static void add_sel_buf(bool debug, DefList_t& sel_bufs, unsigned& i_, CharBuffer& t,
                            short* ind, default_ptr_t def)
    {
        if (debug)
            Logger::getTracer().ProgTrace(TRACESYS, "%d type=%d", i_, SQLT_STR);
        sel_bufs.emplace_back(++i_, 0, t.data, t.size, SQLT_STR, ind, nullptr, External::pod,
                              std::move(def), false, t.size, 0, nullptr);
    }
    template <>
    CursCtl& CursCtl::def_<CharBuffer, std::nullptr_t>(CharBuffer& t, short* ind, std::nullptr_t&&)
    {
        add_sel_buf(isDebug(), sel_bufs, i_, t, ind,
                    ind ? default_ptr_t::none() : default_ptr_t(char(0)));
        return *this;
    }
    template <> CursCtl& CursCtl::def_<CharBuffer>(CharBuffer& t, short* ind, CharBuffer& defval)
    {
        add_sel_buf(isDebug(), sel_bufs, i_, t, ind,
                    ind ? default_ptr_t::none() : default_ptr_t(defval));
        return *this;
    }
}

namespace DbCpp
{
    ENUM_NAMES_DECL(ResultCode);
    ENUM_NAMES_BEGIN(ResultCode)
        (ResultCode::Ok, "Ok")
        (ResultCode::Fatal, "Fatal")
        (ResultCode::BadSqlText, "BadSqlText")
        (ResultCode::BadBind, "BadBind")
        (ResultCode::NoDataFound, "NoDataFound")
        (ResultCode::TooManyRows, "TooManyRows")
        (ResultCode::ConstraintFail, "ConstraintFail")
        (ResultCode::FetchNull, "FetchNull")
        (ResultCode::FetchFail, "FetchFail")
        (ResultCode::Busy, "Busy")
        (ResultCode::Deadlock, "Deadlock")
        (ResultCode::WaitExpired, "WaitExpired")
        (ResultCode::BadConnection, "BadConnection")
        (ResultCode::CERR_INVALID_IDENTIFIER, "CERR_INVALID_IDENTIFIER")
        (ResultCode::CERR_TRUNC, "CERR_TRUNC")
        (ResultCode::CERR_SELL, "CERR_SELL")
        (ResultCode::CERR_TABLE_NOT_EXISTS, "CERR_TABLE_NOT_EXISTS")
        (ResultCode::CERR_SNAPSHOT_TOO_OLD, "CERR_SNAPSHOT_TOO_OLD")
        (ResultCode::REQUIRED_NEXT_PIECE, "REQUIRED_NEXT_PIECE")
        (ResultCode::REQUIRED_NEXT_BUFFER, "REQUIRED_NEXT_BUFFER")
    ENUM_NAMES_END(ResultCode)

    std::ostream& operator<<(std::ostream& os, ResultCode code)
    {
        return os << enumToStr(code);
    }

    static const std::multimap<ResultCode, OciCppErrs> sOraErrorCodeMapper = {
        { ResultCode::Ok, CERR_OK },
        { ResultCode::BadSqlText, CERR_INVALID_IDENTIFIER },
        { ResultCode::BadBind, CERR_BIND },
        { ResultCode::NoDataFound, NO_DATA_FOUND },
        { ResultCode::NoDataFound, CERR_NODAT },
        { ResultCode::TooManyRows, CERR_TOO_MANY_ROWS },
        { ResultCode::ConstraintFail, CERR_U_CONSTRAINT },
        { ResultCode::FetchNull, CERR_NULL },
        { ResultCode::FetchFail, CERR_INVALID_NUMBER },
        { ResultCode::Busy, CERR_BUSY },
        { ResultCode::Deadlock, CERR_DEADLOCK },
        { ResultCode::WaitExpired, WAIT_TIMEOUT_EXPIRED },
        { ResultCode::BadConnection, CERR_NOT_CONNECTED },
        { ResultCode::BadConnection, CERR_NOT_LOGGED_ON },
        { ResultCode::ConstraintFail, CERR_DUPK },
        { ResultCode::ConstraintFail, CERR_I_CONSTRAINT },
        { ResultCode::CERR_TRUNC, CERR_TRUNC },
        { ResultCode::CERR_SELL, CERR_SELL },
        { ResultCode::TooManyRows, CERR_EXACT },
        { ResultCode::CERR_TABLE_NOT_EXISTS, CERR_TABLE_NOT_EXISTS },
        { ResultCode::CERR_SNAPSHOT_TOO_OLD, CERR_SNAPSHOT_TOO_OLD },
        { ResultCode::REQUIRED_NEXT_PIECE, REQUIRED_NEXT_PIECE },
        { ResultCode::REQUIRED_NEXT_BUFFER, REQUIRED_NEXT_BUFFER },
    };

    static const std::map<ResultCode, PgCpp::ResultCode> sPgErrorCodeMapper = {
        { ResultCode::Ok, PgCpp::Ok },
        { ResultCode::Fatal, PgCpp::Fatal },
        { ResultCode::BadSqlText, PgCpp::BadSqlText },
        { ResultCode::BadBind, PgCpp::BadBind },
        { ResultCode::NoDataFound, PgCpp::NoDataFound },
        { ResultCode::TooManyRows, PgCpp::TooManyRows },
        { ResultCode::ConstraintFail, PgCpp::ConstraintFail },
        { ResultCode::FetchNull, PgCpp::FetchNull },
        { ResultCode::FetchFail, PgCpp::FetchFail },
        { ResultCode::Busy, PgCpp::Busy },
        { ResultCode::Deadlock, PgCpp::Deadlock },
        { ResultCode::WaitExpired, PgCpp::WaitExpired },
        { ResultCode::BadConnection, PgCpp::BadConnection },
    };

    static ResultCode pgErrorCodeMapper(int code)
    {
        for (const auto& p : sPgErrorCodeMapper)
        {
            if (p.second == code)
            {
                return p.first;
            }
        }
        return ResultCode::Fatal;
    }

    static PgCpp::ResultCode pgErrorCodeMapper(ResultCode code)
    {
        auto it = sPgErrorCodeMapper.find(code);
        if (it != sPgErrorCodeMapper.end())
        {
            return it->second;
        }
        throw comtech::Exception(STDLOG, __func__, "Unknown DbCpp::ResultCode");
    }

    static ResultCode oraErrorCodeMapper(int code)
    {
        for (const auto& p : sOraErrorCodeMapper)
        {
            if (p.second == code)
            {
                return p.first;
            }
        }
        return ResultCode::Fatal;
    }

    static std::vector<OciCppErrs> oraErrorCodeMapper(ResultCode code)
    {
        std::vector<OciCppErrs> errors;
        auto range = sOraErrorCodeMapper.equal_range(code);
        for (auto it = range.first; it != range.second; ++it)
        {
            errors.push_back(it->second);
        }
        return errors;
    }

    class CursCtlImpl
    {
    public:
        virtual ~CursCtlImpl() {}

#define supportType(X)                                                                             \
    virtual void def(X t, short* ind)                                    = 0;                      \
    virtual void defNull(X t, const X defVal)                            = 0;                      \
    virtual void bind(const std::string& n, const X t, const short* ind) = 0
        supportType(bool&);
        supportType(char&);
        supportType(short&);
        supportType(int&);
        supportType(long&);
        supportType(long long&);
        supportType(unsigned char&);
        supportType(unsigned short&);
        supportType(unsigned int&);
        supportType(unsigned long&);
        supportType(unsigned long long&);
        supportType(std::string&);
        supportType(boost::gregorian::date&);
        supportType(boost::posix_time::ptime&);
#undef supportType
        virtual void def(char* t, size_t size, short* ind)                             = 0;
        virtual void defNull(char* t, size_t size, const char* defVal, size_t defSize) = 0;
        virtual void bind(const std::string& n, const char* const& t, size_t size, const short* ind)
            = 0;
        virtual void bind_array(const std::string& n, const char* const& t, size_t size,
                                const short* ind)
            = 0;
        virtual void bind(const std::string& n, const char* const& t, const short* ind) = 0;

        virtual void autoNull()                          = 0;
        virtual void noThrowError(ResultCode err)        = 0;
        virtual void stb()                               = 0;
        virtual void unstb()                             = 0;
        virtual void singleRowMode()                     = 0;
        virtual void fetchLen(int count)                 = 0;
        virtual void structSize(int data, int indicator) = 0;

        virtual ResultCode exfet(int count = 1) = 0;
        virtual ResultCode EXfet(int count = 1) = 0;
        virtual ResultCode exec()               = 0;
        virtual ResultCode fen(int count = 1)   = 0;
        virtual ResultCode err()                = 0;
        virtual int rowcount()                  = 0;
        virtual int rowcount_now()              = 0;
        virtual bool isStable() const           = 0;
    };

#define supportType(X)                                                                             \
    virtual void def(X t, short* ind) override                                                     \
    {                                                                                              \
        mNeedFetch = true;                                                                         \
        mCursCtl.def(t, ind);                                                                      \
    }                                                                                              \
    virtual void defNull(X t, const X defVal) override                                             \
    {                                                                                              \
        mNeedFetch = true;                                                                         \
        mCursCtl.defNull(t, defVal);                                                               \
    }                                                                                              \
    virtual void bind(const std::string& n, const X t, const short* ind) override                  \
    {                                                                                              \
        mCursCtl.bind(n, t, ind);                                                                  \
    }

    static bool defaultStableBind()
    {
        static const bool val = readIntFromTcl("CURSCTL_DEFAULT_STABLE_BIND", 0);
        return val;
    }

    class PostgresImpl final : public CursCtlImpl
    {
    public:
        PostgresImpl(Session& sess, const char* n, const char* f, int l, const char* sql,
                     bool cacheit)
            : mSession(static_cast<PgSession&>(sess))
            , mCursCtl(mSession.createPgCursor(n, f, l, sql, cacheit))
            , mIsStable(defaultStableBind())
            , mExecuted(false)
            , mNeedFetch(false)
            , mFetched(false)
        {
            mCursCtl.noThrowError(PgCpp::ResultCode::BadConnection);
            if (mIsStable)
            {
                stb();
            } else
            {
                unstb();
            }
        }

        ~PostgresImpl()
        {
#ifdef XP_TESTING
            if (!mExecuted)
            {
                LogWarning(STDLOG) << "DbCpp::CursCtl has been destructed without being executed";
            }
            if (mNeedFetch && !mFetched)
            {
                LogWarning(STDLOG) << "mNeedFetch && !mFetched";
            }
#endif
        }

        supportType(bool&);
        supportType(char&);
        supportType(short&);
        supportType(int&);
        supportType(long&);
        supportType(long long&);
        supportType(unsigned short&);
        supportType(unsigned int&);
        supportType(unsigned long&);
        supportType(unsigned long long&);
        supportType(std::string&);
        supportType(boost::gregorian::date&);
        supportType(boost::posix_time::ptime&);

        virtual void bind(const std::string& n, const char* const& t, const short* ind) override
        {
            mCursCtl.bind(n, t, ind);
        }
        virtual void def(unsigned char& t, short* ind) override
        {
            throw comtech::Exception(STDLOG, __func__, "Not supported");
        }
        virtual void defNull(unsigned char& t, const unsigned char& defVal) override
        {
            throw comtech::Exception(STDLOG, __func__, "Not supported");
        }
        virtual void bind(const std::string& n, const unsigned char& t, const short* ind) override
        {
            throw comtech::Exception(STDLOG, __func__, "Not supported");
        }

        virtual void def(char* t, size_t size, short* ind = 0) override
        {
            mNeedFetch = true;
            mCharBufferCache.emplace_back(t, size, false);
            mCursCtl.def(mCharBufferCache.back(), ind);
        }
        virtual void defNull(char* t, size_t size, const char* defVal, size_t defSize) override
        {
            if (defSize > size)
            {
                throw comtech::Exception("default value is longer than output buffer");
            }
            mNeedFetch = true;
            CharBuffer def(defVal, defSize, true);

            mCharBufferCache.emplace_back(t, size, false);
            mCursCtl.defNull(mCharBufferCache.back(), def);
        }
        virtual void bind(const std::string& n, const char* const& t, size_t size,
                          const short* ind) override
        {
            mCharBufferCache.emplace_back(t, size, false);
            mCursCtl.bind(n, mCharBufferCache.back(), ind);
        }

        virtual void bind_array(const std::string& n, const char* const& t, size_t size,
                                const short* ind) override
        {
            mCharBufferCache.emplace_back(t, size, false, PgOid::ByteArray);
            mCursCtl.bind(n, mCharBufferCache.back(), ind);
        }

        virtual void autoNull() override { mCursCtl.autoNull(); }
        virtual void noThrowError(ResultCode err) override
        {
            mCursCtl.noThrowError(pgErrorCodeMapper(err));
        }
        virtual void stb() override
        {
            mIsStable = true;
            mCursCtl.stb();
        }
        virtual void unstb() override
        {
            mIsStable = false;
            mCursCtl.unstb();
        }
        virtual void singleRowMode() override { mCursCtl.singleRowMode(); }
        virtual void fetchLen(int count) override
        {
            throw comtech::Exception(STDLOG, __func__, "Call of unsupported PostgresImpl method");
        }
        virtual void structSize(int data, int indicator) override
        {
            throw comtech::Exception(STDLOG, __func__, "Call of unsupported PostgresImpl method");
        }

        virtual ResultCode exfet(int count) override
        {
            mExecuted = true;
            mFetched  = true;
            if (count > 1)
            {
                throw comtech::Exception(STDLOG, __func__,
                                         "PostgresImpl: 'count' should not exceed 1");
            }
            mSession.activateSession();
            DbCpp::ResultCode ret = pgErrorCodeMapper(mCursCtl.exfet());
            if (ret == DbCpp::ResultCode::BadConnection)
            {
                LogError(STDLOG) << "PgException: BadConnection on exfet(). exit";
                exit(1);
            }
            return ret;
        }
        virtual ResultCode EXfet(int count) override
        {
            mExecuted = true;
            mFetched  = true;
            if (count > 1)
            {
                throw comtech::Exception(STDLOG, __func__,
                                         "PostgresImpl: 'count' should not exceed 1");
            }
            mSession.activateSession();
            DbCpp::ResultCode ret = pgErrorCodeMapper(mCursCtl.EXfet());
            if (ret == DbCpp::ResultCode::BadConnection)
            {
                LogError(STDLOG) << "PgException: BadConnection on EXfet(). exit";
                exit(1);
            }
            return ret;
        }
        virtual ResultCode exec() override
        {
            mExecuted = true;
            mSession.activateSession();
            DbCpp::ResultCode ret = pgErrorCodeMapper(mCursCtl.exec());
            if (ret == DbCpp::ResultCode::BadConnection)
            {
                LogError(STDLOG) << "PgException: BadConnection on exec(). exit";
                exit(1);
            }
            return ret;
        }
        virtual ResultCode fen(int count) override
        {
#ifdef XP_TESTING
            if (!mExecuted && inTestMode())
            {
                LogError(STDLOG) << "DbCpp::CursCtl has to be executed before calling fen()";
                exit(1);
            }
#endif
            mFetched = true;
            if (count > 1)
            {
                throw comtech::Exception(STDLOG, __func__,
                                         "PostgresImpl: 'count' should not exceed 1");
            }
            return pgErrorCodeMapper(mCursCtl.fen());
        }
        virtual ResultCode err() override { return pgErrorCodeMapper(mCursCtl.err()); }
        virtual int rowcount() override { return mCursCtl.rowcount(); }
        virtual int rowcount_now() override
        {
            throw comtech::Exception(STDLOG, __func__, "Call of unsupported PostgresImpl method");
        }
        virtual bool isStable() const override { return mIsStable; }

    private:
        std::list<CharBuffer> mCharBufferCache;
        DbCpp::PgSession& mSession;
        PgCpp::CursCtl mCursCtl;
        bool mIsStable;
        bool mExecuted;
        bool mNeedFetch;
        bool mFetched;
    };
#undef supportType

#define supportType(X)                                                                             \
    virtual void def(X t, short* ind) override                                                     \
    {                                                                                              \
        mNeedFetch = true;                                                                         \
        mCursCtl.def(t, ind);                                                                      \
    }                                                                                              \
    virtual void defNull(X t, const X defVal) override                                             \
    {                                                                                              \
        mNeedFetch = true;                                                                         \
        mCursCtl.defNull(t, defVal);                                                               \
    }                                                                                              \
    virtual void bind(const std::string& n, const X t, const short* ind) override                  \
    {                                                                                              \
        mCursCtl.bind(n, t, const_cast<short*>(ind));                                              \
    }
    class OracleImpl final : public CursCtlImpl
    {
    public:
        OracleImpl(Session& sess, const char* n, const char* f, int l, const char* sql,
                   bool cacheit)
            : mCursCtl(static_cast<OraSession&>(sess).createOraCursor(n, f, l, sql, cacheit))
            , mIsStable(defaultStableBind())
            , mExecuted(false)
            , mNeedFetch(false)
            , mFetched(false)
        {
            if (mIsStable)
            {
                stb();
            } else
            {
                unstb();
            }
        }

        ~OracleImpl()
        {
#ifdef XP_TESTING
            if (!mExecuted)
            {
                LogWarning(STDLOG) << "DbCpp::CursCtl has been destructed without being executed";
            }
            if (mNeedFetch && !mFetched)
            {
                LogWarning(STDLOG) << "mNeedFetch && !mFetched";
            }
#endif
        }

        supportType(bool&);
        supportType(char&);
        supportType(short&);
        supportType(int&);
        supportType(long long&);
        supportType(unsigned char&);
        supportType(unsigned short&);
        supportType(unsigned int&);
        supportType(unsigned long long&);
        supportType(long&);
        supportType(unsigned long&);
        supportType(std::string&);
        supportType(boost::gregorian::date&);
        supportType(boost::posix_time::ptime&);

        virtual void bind(const std::string& n, const char* const& t, const short* ind) override
        {
            mCursCtl.bind(n, t, const_cast<short*>(ind));
        }
        virtual void def(char* t, size_t size, short* ind) override
        {
            mNeedFetch = true;
            mCharBufferCache.emplace_back(t, size, false);
            mCursCtl.def(mCharBufferCache.back(), ind);
        }
        virtual void defNull(char* t, size_t size, const char* defVal, size_t defSize) override
        {
            if (defSize > size)
            {
                throw OciCpp::ociexception("default value is longer than output buffer");
            }
            mNeedFetch = true;
            mCharBufferCache.emplace_back(t, size, false);
            CharBuffer def(defVal, defSize, true);

            mCursCtl.defNull(mCharBufferCache.back(), def);
        }
        virtual void bind(const std::string& n, const char* const& t, size_t size,
                          const short* ind) override
        {
            mCursCtl.bindFull(n, const_cast<char*>(t), size, const_cast<short*>(ind), 0, SQLT_STR);
        }

        virtual void bind_array(const std::string& n, const char* const& t, size_t size,
                                const short* ind) override
        {
            mCursCtl.bindFull(n, const_cast<char*>(t), size, const_cast<short*>(ind), 0, SQLT_CHR);
        }

        virtual void autoNull() override { mCursCtl.autoNull(); }
        virtual void noThrowError(ResultCode err) override
        {
            for (OciCppErrs ociErr : oraErrorCodeMapper(err))
            {
                mCursCtl.noThrowError(ociErr);
            }
        }
        virtual void stb() override
        {
            mIsStable = true;
            mCursCtl.stb();
        }
        virtual void unstb() override
        {
            mIsStable = false;
            mCursCtl.unstb();
        }
        virtual void singleRowMode() override
        {
            throw comtech::Exception(STDLOG, __func__, "Call of unsupported OracleImpl method");
        }
        virtual void fetchLen(int count) override
        {
            mFetched = true;
            mCursCtl.fetchLen(count);
        }
        virtual void structSize(int data, int indicator) override
        {
            mCursCtl.structSize(data, indicator);
        }

        virtual ResultCode exfet(int count) override
        {
            mExecuted = true;
            mFetched  = true;
            return oraErrorCodeMapper(mCursCtl.exfet(count));
        }
        virtual ResultCode EXfet(int count) override
        {
            mExecuted = true;
            mFetched  = true;
            return oraErrorCodeMapper(mCursCtl.EXfet(count));
        }
        virtual ResultCode exec() override
        {
            mExecuted = true;
            mCursCtl.exec();
            return ResultCode::Ok;
        }
        virtual ResultCode fen(int count) override
        {
#ifdef XP_TESTING
            if (!mExecuted && inTestMode())
            {
                LogError(STDLOG) << "DbCpp::CursCtl has to be executed before calling fen()";
                exit(1);
            }
#endif
            mFetched = true;
            return oraErrorCodeMapper(mCursCtl.fen(count));
        }
        virtual ResultCode err() override { return oraErrorCodeMapper(mCursCtl.err()); }
        virtual int rowcount() override { return mCursCtl.rowcount(); }
        virtual int rowcount_now() override { return mCursCtl.rowcount_now(); }
        virtual bool isStable() const override { return mIsStable; }

    private:
        std::list<CharBuffer> mCharBufferCache;
        OciCpp::CursCtl mCursCtl;
        bool mIsStable;
        bool mExecuted;
        bool mNeedFetch;
        bool mFetched;
    };
#undef supportType

#define supportType(X)                                                                             \
    CursCtl& CursCtl::def(X& t, short* ind)                                                        \
    {                                                                                              \
        mImpl->def(t, ind);                                                                        \
        return *this;                                                                              \
    }                                                                                              \
    CursCtl& CursCtl::defNull(X& t, const X& defVal)                                               \
    {                                                                                              \
        mImpl->defNull(t, defVal);                                                                 \
        return *this;                                                                              \
    }                                                                                              \
    CursCtl& CursCtl::bind_(const std::string& n, const X& t, const short* ind)                    \
    {                                                                                              \
        mImpl->bind(n, t, ind);                                                                    \
        return *this;                                                                              \
    }
    supportType(bool);
    supportType(char);
    supportType(short);
    supportType(int);
    supportType(long);
    supportType(long long);
    supportType(unsigned char);
    supportType(unsigned short);
    supportType(unsigned int);
    supportType(unsigned long);
    supportType(unsigned long long);
    supportType(std::string);
    supportType(boost::gregorian::date);
    supportType(boost::posix_time::ptime);
#undef supportType

    CursCtl& CursCtl::def(char* t, size_t size, short* ind)
    {
        mImpl->def(t, size, ind);
        return *this;
    }
    CursCtl& CursCtl::defNull(char* t, size_t size, const char* defVal, size_t defSize)
    {
        mImpl->defNull(t, size, defVal, defSize);
        return *this;
    }
    CursCtl& CursCtl::bind_(const std::string& n, const char* const& t, size_t size,
                            const short* ind)
    {
        mImpl->bind(n, t, size, ind);
        return *this;
    }
    CursCtl& CursCtl::bind_(const std::string& n, const char* const& t, const short* ind)
    {
        mImpl->bind(n, t, ind);
        return *this;
    }

    CursCtl& CursCtl::bind_array(const std::string& n, const char* const& t, size_t size,
                                 const short* ind)
    {
        mImpl->bind_array(n, t, size, ind);
        return *this;
    }

    CursCtl::CursCtl(Session& sess, const char* n, const char* f, int l, const char* sql,
                     bool cacheit)
    {
        if (sess.getType() == DbType::Oracle)
        {
            mImpl.reset(new OracleImpl(sess, n, f, l, sql, cacheit));
        } else
        {
            mImpl.reset(new PostgresImpl(sess, n, f, l, sql, cacheit));
        }
    }

    CursCtl::CursCtl(CursCtl&& ) = default;

    CursCtl::~CursCtl() = default;

    CursCtl& CursCtl::autoNull()
    {
        mImpl->autoNull();
        return *this;
    }
    CursCtl& CursCtl::noThrowError(ResultCode err)
    {
        mImpl->noThrowError(err);
        return *this;
    }
    CursCtl& CursCtl::stb()
    {
        mImpl->stb();
        return *this;
    }
    CursCtl& CursCtl::unstb()
    {
        mImpl->unstb();
        return *this;
    }
    CursCtl& CursCtl::singleRowMode()
    {
        mImpl->singleRowMode();
        return *this;
    }
    CursCtl& CursCtl::fetchLen(int count)
    {
        mImpl->fetchLen(count);
        return *this;
    }
    CursCtl& CursCtl::structSize(int data, int indicator)
    {
        mImpl->structSize(data, indicator);
        return *this;
    }

    ResultCode CursCtl::exfet(int count) { return mImpl->exfet(count); }
    ResultCode CursCtl::EXfet(int count) { return mImpl->EXfet(count); }
    ResultCode CursCtl::exec() { return mImpl->exec(); }
    ResultCode CursCtl::fen(int count) { return mImpl->fen(count); }
    ResultCode CursCtl::err() { return mImpl->err(); }
    int CursCtl::rowcount() { return mImpl->rowcount(); }
    int CursCtl::rowcount_now() { return mImpl->rowcount_now(); }

    bool CursCtl::isStable() const { return mImpl->isStable(); }
    void CursCtl::throwBadBindException(const std::string& msg) const
    {
        throw comtech::Exception(msg);
    }

    CursCtl make_curs_(const char* n, const char* f, int l, const std::string& sql)
    {
        return mainOraSession(n, f, l).createCursor(n, f, l, sql);
    }
    CursCtl make_curs_(const char* n, const char* f, int l, const std::string& sql, Session& sess)
    {
        return sess.createCursor(n, f, l, sql);
    }
    CursCtl make_curs_no_cache_(const char* n, const char* f, int l, const std::string& sql)
    {
        return mainOraSession(n, f, l).createCursorNoCache(n, f, l, sql);
    }
    CursCtl make_curs_no_cache_(const char* n, const char* f, int l, const std::string& sql,
                                Session& sess)
    {
        return sess.createCursorNoCache(n, f, l, sql);
    }
}
#endif // ENABLE_PG
