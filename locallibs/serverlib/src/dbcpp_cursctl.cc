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
            using this_type = CharBuffer;

            static int length(const this_type& b) { return b.size; }
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
                    dst.resize(s.size);
                    memcpy(dst.data(), s.data, s.size);
                }
            }
            static bool setValue(char* value, const char* data, int len)
            {
                CharBuffer* p = reinterpret_cast<CharBuffer*>(value);
                if (p->oid == PgOid::Varchar)
                {
                    if (static_cast<size_t>(len) > p->size)
                    {
                        return false;
                    }
                    memcpy(p->data, data, len);
                    if (static_cast<size_t>(len) < p->size)
                    {
                        p->data[len] = '\0';
                    }
                } else
                {
                    fillCharBuffFromHex(p->data, p->size, data);
                }
                return true;
            }
        };
        template <> Argument makeTypeDesc(const CharBuffer& t)
        {
            int format = t.oid == PgOid::Varchar ? 0 : 1;
            return Argument { static_cast<PgOid_t>(t.oid), format,
                              format ? PgTraits<CharBuffer>::length(t) : 0 };
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
        (ResultCode::ReadOnly, "ReadOnly")
        (ResultCode::CERR_INVALID_IDENTIFIER, "CERR_INVALID_IDENTIFIER")
        (ResultCode::CERR_TRUNC, "CERR_TRUNC")
        (ResultCode::CERR_SELL, "CERR_SELL")
        (ResultCode::CERR_TABLE_NOT_EXISTS, "CERR_TABLE_NOT_EXISTS")
        (ResultCode::CERR_SNAPSHOT_TOO_OLD, "CERR_SNAPSHOT_TOO_OLD")
        (ResultCode::REQUIRED_NEXT_PIECE, "REQUIRED_NEXT_PIECE")
        (ResultCode::REQUIRED_NEXT_BUFFER, "REQUIRED_NEXT_BUFFER")
    ENUM_NAMES_END(ResultCode)

    bool operator!(ResultCode e)
    {
        return e == ResultCode::Ok;
    }

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
        { ResultCode::ReadOnly, PgCpp::ReadOnly },
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
        virtual ~CursCtlImpl() { }

#define defineType(TYPE)                                                                           \
    virtual void def(TYPE& t, short* ind)                                    = 0;                  \
    virtual void defNull(TYPE& t, const TYPE& defVal)                        = 0;                  \
    virtual void bind(const std::string& n, const TYPE& t, const short* ind) = 0

        defineType(bool);
        defineType(char);
        defineType(short);
        defineType(int);
        defineType(long);
        defineType(long long);
        defineType(unsigned char);
        defineType(unsigned short);
        defineType(unsigned int);
        defineType(unsigned long);
        defineType(unsigned long long);
        defineType(float);
        defineType(double);
        defineType(std::string);
        defineType(boost::gregorian::date);
        defineType(boost::posix_time::ptime);

#undef defineType

        virtual void def(char* t, size_t size, short* ind)                             = 0;
        virtual void defNull(char* t, size_t size, const char* defVal, size_t defSize) = 0;
        virtual void defArray(char* t, size_t size, short* ind)                        = 0;
        virtual void bind(const std::string& n, const char* const& t, size_t size, const short* ind)
            = 0;
        virtual void bindArray(const std::string& n, const char* const& t, size_t size,
                               const short* ind)
            = 0;
        virtual void bind(const std::string& n, const char* const& t, const short* ind) = 0;

        virtual void autoNull()                   = 0;
        virtual void noThrowError(ResultCode err) = 0;
        virtual void stb()                        = 0;
        virtual void unstb()                      = 0;
        virtual void singleRowMode()              = 0;

        virtual ResultCode exfet()    = 0;
        virtual ResultCode EXfet()    = 0;
        virtual ResultCode exec()     = 0;
        virtual ResultCode fen()      = 0;
        virtual ResultCode nefen()    = 0;
        virtual ResultCode err()      = 0;
        virtual int rowcount()        = 0;
        virtual bool isStable() const = 0;

        virtual int currentRow() const = 0;
        virtual int fieldsCount() const = 0;

        virtual bool        fieldIsNull(const std::string& fn) const = 0;
        virtual std::string fieldValue(const std::string& fn) const = 0;
        virtual int         fieldIndex(const std::string& fn) const = 0;

        virtual bool        fieldIsNull(int fi) const = 0;
        virtual std::string fieldValue(int fi) const = 0;
        virtual std::string fieldName(int fi) const = 0;
    };

#define defineType(TYPE)                                                                           \
    virtual void def(TYPE& t, short* ind) override { mCursCtl.def(t, ind); }                       \
    virtual void defNull(TYPE& t, const TYPE& defVal) override { mCursCtl.defNull(t, defVal); }    \
    virtual void bind(const std::string& n, const TYPE& t, const short* ind) override              \
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

        ~PostgresImpl() { }

        defineType(bool);
        defineType(char);
        defineType(short);
        defineType(int);
        defineType(long);
        defineType(long long);
        defineType(unsigned short);
        defineType(unsigned int);
        defineType(unsigned long);
        defineType(unsigned long long);
        defineType(float);
        defineType(double);
        defineType(std::string);
        defineType(boost::gregorian::date);
        defineType(boost::posix_time::ptime);

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
            mCharBufferCache.emplace_back(t, size, false);
            mCursCtl.def(mCharBufferCache.back(), ind);
        }
        virtual void defNull(char* t, size_t size, const char* defVal, size_t defSize) override
        {
            if (defSize > size)
            {
                throw comtech::Exception("default value is longer than output buffer");
            }
            CharBuffer def(defVal, defSize, true);

            mCharBufferCache.emplace_back(t, size, false);
            mCursCtl.defNull(mCharBufferCache.back(), def);
        }

        virtual void defArray(char* t, size_t size, short* ind)
        {
            mCharBufferCache.emplace_back(t, size, false, PgOid::ByteArray);
            mCursCtl.def(mCharBufferCache.back(), ind);
        }

        virtual void bind(const std::string& n, const char* const& t, size_t size,
                          const short* ind) override
        {
            mCharBufferCache.emplace_back(t, size, false);
            mCursCtl.bind(n, mCharBufferCache.back(), ind);
        }

        virtual void bindArray(const std::string& n, const char* const& t, size_t size,
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

        virtual ResultCode exfet() override
        {
            mSession.activateSession();
            DbCpp::ResultCode ret = pgErrorCodeMapper(mCursCtl.exfet());
            if (ret == DbCpp::ResultCode::BadConnection)
            {
                LogError(STDLOG) << "PgException: BadConnection on exfet(). exit";
                exit(1);
            }
            return ret;
        }
        virtual ResultCode EXfet() override
        {
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
            mSession.activateSession();
            DbCpp::ResultCode ret = pgErrorCodeMapper(mCursCtl.exec());
            if (ret == DbCpp::ResultCode::BadConnection)
            {
                LogError(STDLOG) << "PgException: BadConnection on exec(). exit";
                exit(1);
            }
            return ret;
        }
        virtual ResultCode fen() override { return pgErrorCodeMapper(mCursCtl.fen()); }
        virtual ResultCode nefen() override { return pgErrorCodeMapper(mCursCtl.nefen()); }
        virtual ResultCode err() override { return pgErrorCodeMapper(mCursCtl.err()); }
        virtual int rowcount() override { return mCursCtl.rowcount(); }
        virtual bool isStable() const override { return mIsStable; }

        virtual int currentRow() const override { return mCursCtl.currentRow(); }
        virtual int fieldsCount() const override { return mCursCtl.fieldsCount(); }

        virtual bool        fieldIsNull(const std::string& fn) const override { return mCursCtl.fieldIsNull(fn); }
        virtual std::string  fieldValue(const std::string& fn) const override { return mCursCtl.fieldValue(fn); }
        virtual int          fieldIndex(const std::string& fn) const override { return mCursCtl.fieldIndex(fn); }

        virtual bool        fieldIsNull(int fi) const override { return mCursCtl.fieldIsNull(fi); }
        virtual std::string  fieldValue(int fi) const override { return mCursCtl.fieldValue(fi); }
        virtual std::string   fieldName(int fi) const override { return mCursCtl.fieldName(fi); }

    private:
        std::list<CharBuffer> mCharBufferCache;
        DbCpp::PgSession& mSession;
        PgCpp::CursCtl mCursCtl;
        bool mIsStable;
    };
#undef defineType

#define defineType(TYPE)                                                                           \
    virtual void def(TYPE& t, short* ind) override { mCursCtl.def(t, ind); }                       \
    virtual void defNull(TYPE& t, const TYPE& defVal) override { mCursCtl.defNull(t, defVal); }    \
    virtual void bind(const std::string& n, const TYPE& t, const short* ind) override              \
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
        {
            if (mIsStable)
            {
                stb();
            } else
            {
                unstb();
            }
        }

        ~OracleImpl() { }

        defineType(bool);
        defineType(char);
        defineType(short);
        defineType(int);
        defineType(long long);
        defineType(unsigned char);
        defineType(unsigned short);
        defineType(unsigned int);
        defineType(unsigned long long);
        defineType(long);
        defineType(unsigned long);
        defineType(float);
        defineType(double);
        defineType(std::string);
        defineType(boost::gregorian::date);
        defineType(boost::posix_time::ptime);

        virtual void bind(const std::string& n, const char* const& t, const short* ind) override
        {
            mCursCtl.bind(n, t, const_cast<short*>(ind));
        }
        virtual void def(char* t, size_t size, short* ind) override
        {
            mCharBufferCache.emplace_back(t, size, false);
            mCursCtl.def(mCharBufferCache.back(), ind);
        }
        virtual void defNull(char* t, size_t size, const char* defVal, size_t defSize) override
        {
            if (defSize > size)
            {
                throw OciCpp::ociexception("default value is longer than output buffer");
            }
            mCharBufferCache.emplace_back(t, size, false);
            CharBuffer def(defVal, defSize, true);

            mCursCtl.defNull(mCharBufferCache.back(), def);
        }

        virtual void defArray(char* t, size_t size, short* ind)
        {
            mCursCtl.defFull(const_cast<char*>(t), size, const_cast<short*>(ind), 0, SQLT_BIN);
        }

        virtual void bind(const std::string& n, const char* const& t, size_t size,
                          const short* ind) override
        {
            mCursCtl.bindFull(n, const_cast<char*>(t), size, const_cast<short*>(ind), 0, SQLT_STR);
        }

        virtual void bindArray(const std::string& n, const char* const& t, size_t size,
                               const short* ind) override
        {
            mCursCtl.bindFull(n, const_cast<char*>(t), size, const_cast<short*>(ind), 0, SQLT_BIN);
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

        virtual ResultCode exfet() override { return oraErrorCodeMapper(mCursCtl.exfet()); }
        virtual ResultCode EXfet() override { return oraErrorCodeMapper(mCursCtl.EXfet()); }
        virtual ResultCode exec() override
        {
            mCursCtl.exec();
            return ResultCode::Ok;
        }
        virtual ResultCode fen() override { return oraErrorCodeMapper(mCursCtl.fen()); }
        virtual ResultCode nefen() override
        {
            throw comtech::Exception(STDLOG, __func__, "Call of unsupported OracleImpl method");
        }
        virtual ResultCode err() override { return oraErrorCodeMapper(mCursCtl.err()); }
        virtual int rowcount() override { return mCursCtl.rowcount(); }
        virtual bool isStable() const override { return mIsStable; }

        virtual int fieldsCount() const override
        {
            throw comtech::Exception(STDLOG, __func__, "Call of unsupported OracleImpl method");
        }

        virtual int currentRow() const override
        {
            throw comtech::Exception(STDLOG, __func__, "Call of unsupported OracleImpl method");
        }

        virtual bool fieldIsNull(const std::string& fn) const override
        {
            throw comtech::Exception(STDLOG, __func__, "Call of unsupported OracleImpl method");
        }
        virtual std::string fieldValue(const std::string& fn) const override
        {
            throw comtech::Exception(STDLOG, __func__, "Call of unsupported OracleImpl method");
        }
        virtual int fieldIndex(const std::string& fn) const override
        {
            throw comtech::Exception(STDLOG, __func__, "Call of unsupported OracleImpl method");
        }

        virtual bool fieldIsNull(int fi) const override
        {
            throw comtech::Exception(STDLOG, __func__, "Call of unsupported OracleImpl method");
        }
        virtual std::string fieldValue(int fi) const override
        {
            throw comtech::Exception(STDLOG, __func__, "Call of unsupported OracleImpl method");
        }
        virtual std::string fieldName(int fi) const override
        {
            throw comtech::Exception(STDLOG, __func__, "Call of unsupported OracleImpl method");
        }

    private:
        std::list<CharBuffer> mCharBufferCache;
        OciCpp::CursCtl mCursCtl;
        bool mIsStable;
    };
#undef defineType

#define defineType(TYPE)                                                                           \
    CursCtl& CursCtl::def_(TYPE& t, short* ind)                                                    \
    {                                                                                              \
        mImpl->def(t, ind);                                                                        \
        return *this;                                                                              \
    }                                                                                              \
    CursCtl& CursCtl::defNull_(TYPE& t, const TYPE& defVal)                                        \
    {                                                                                              \
        mImpl->defNull(t, defVal);                                                                 \
        return *this;                                                                              \
    }                                                                                              \
    CursCtl& CursCtl::bind_(const std::string& n, const TYPE& t, const short* ind)                 \
    {                                                                                              \
        mImpl->bind(n, t, ind);                                                                    \
        return *this;                                                                              \
    }

    defineType(bool);
    defineType(char);
    defineType(short);
    defineType(int);
    defineType(long);
    defineType(long long);
    defineType(unsigned char);
    defineType(unsigned short);
    defineType(unsigned int);
    defineType(unsigned long);
    defineType(unsigned long long);
    defineType(float);
    defineType(double);
    defineType(std::string);
    defineType(boost::gregorian::date);
    defineType(boost::posix_time::ptime);

#undef defineType

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
    CursCtl& CursCtl::defArray(char* t, size_t size, short* ind)
    {
        mImpl->defArray(t, size, ind);
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

    CursCtl& CursCtl::bindArray(const std::string& n, const char* const& t, size_t size,
                                const short* ind)
    {
        mImpl->bindArray(n, t, size, ind);
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

    CursCtl::CursCtl(CursCtl&&) = default;

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

    ResultCode CursCtl::exfet() { return mImpl->exfet(); }
    ResultCode CursCtl::EXfet() { return mImpl->EXfet(); }
    ResultCode CursCtl::exec() { return mImpl->exec(); }
    ResultCode CursCtl::fen() { return mImpl->fen(); }
    ResultCode CursCtl::nefen() { return mImpl->nefen(); }
    ResultCode CursCtl::err() { return mImpl->err(); }
    int CursCtl::rowcount() { return mImpl->rowcount(); }

    int CursCtl::currentRow() const { return mImpl->currentRow(); }
    int CursCtl::fieldsCount() const { return mImpl->fieldsCount(); }

    bool CursCtl::fieldIsNull(const std::string& fn) const { return mImpl->fieldIsNull(fn); }
    std::string CursCtl::fieldValue(const std::string& fn) const { return mImpl->fieldValue(fn); }
    int CursCtl::fieldIndex(const std::string& fn) const { return mImpl->fieldIndex(fn); }

    bool CursCtl::fieldIsNull(int fi) const { return mImpl->fieldIsNull(fi); }
    std::string CursCtl::fieldValue(int fi) const { return mImpl->fieldValue(fi); }
    std::string CursCtl::fieldName(int fi) const { return mImpl->fieldName(fi); }

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
