#pragma once
#ifdef ENABLE_PG

#include <cstring>
#include <memory>
#include <string>

namespace boost
{
    namespace gregorian
    {
        class date;
    }
    namespace posix_time
    {
        class ptime;
    }
}

namespace PgCpp
{
    class CursCtl;
}

namespace OciCpp
{
    class CursCtl;
}

namespace DbCpp
{
    class Session;

    enum class ResultCode
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
        CERR_INVALID_IDENTIFIER,
        CERR_TRUNC,
        CERR_SELL,
        CERR_TABLE_NOT_EXISTS,
        CERR_SNAPSHOT_TOO_OLD,
        REQUIRED_NEXT_PIECE,
        REQUIRED_NEXT_BUFFER
    };
    bool operator!(ResultCode e);
    std::ostream& operator<<(std::ostream& os, ResultCode);

    class CursCtl
    {
    public:
        CursCtl(Session& sess, const char* n, const char* f, int l, const char* sql,
                bool cacheit = true);
        CursCtl(CursCtl&&);
        ~CursCtl();

        CursCtl& autoNull();
        CursCtl& noThrowError(ResultCode err);
        CursCtl& stb();
        CursCtl& unstb();
        CursCtl& singleRowMode();

        ResultCode exfet();
        ResultCode EXfet();
        ResultCode exec();
        ResultCode fen();
        ResultCode err();
        int rowcount();

        bool isStable() const;

        CursCtl& bind(const std::string& n, char*& t, const short* ind = 0) = delete;

        template <typename T, typename = std::enable_if_t<!std::is_lvalue_reference_v<T>>>
        CursCtl& bind(const std::string& n, T&& t, const short* ind = 0)
        {
            if (!isStable())
            {
                throwBadBindException("unstable rvalue bind " + n);
            }
            return bind_(n, static_cast<const T&>(t), ind);
        }
        template <typename T> CursCtl& bind(const std::string& n, const T& t, const short* ind = 0)
        {
            return bind_(n, t, ind);
        }
        CursCtl& bind(const std::string& n, const char* const& t, size_t size, const short* ind = 0)
        {
            return bind_(n, t, size, ind);
        }
        template <size_t N>
        CursCtl& bind(const std::string& n, const char (&t)[N], const short* ind = 0)
        {
            return bind_(n, t, N, ind);
        }
        CursCtl& bindArray(const std::string& n, const char* const& t, size_t size,
                           const short* ind = 0);
        template <size_t N>
        CursCtl& bind(const std::string& n, const std::array<char, N>& t, const short* ind = 0)
        {
            return bindArray(n, t.data(), N, ind);
        }

        template <typename T> CursCtl& def(T& t, short* ind = 0) { return def_(t, ind); }
        template <typename T, typename U> CursCtl& defNull(T& t, const U& defVal)
        {
            return defNull_(t, defVal);
        }
        CursCtl& def(char* t, size_t size, short* ind = 0);
        template <size_t N> CursCtl& def(char (&t)[N], short* ind = 0) { return def(t, N, ind); }

        CursCtl& defNull(char* t, size_t size, const char* defVal, size_t defSize);
        template <size_t N> CursCtl& defNull(char (&t)[N], const char* defVal)
        {
            return defNull(t, N, defVal, std::strlen(defVal) + 1);
        }
        template <size_t N, size_t M> CursCtl& defNull(char (&t)[N], char (&defVal)[M])
        {
            return defNull(t, N, defVal, M);
        }

        CursCtl& defArray(char* t, size_t size, short* ind = 0);

    private:
        CursCtl& bind_(const std::string& n, const char* const& t, size_t size, const short* ind);
        CursCtl& bind_(const std::string& n, const char* const& t, const short* ind);

#define defineType(TYPE)                                                                           \
    CursCtl& def_(TYPE& t, short* ind = 0);                                                        \
    CursCtl& defNull_(TYPE& t, const TYPE& defVal);                                                \
    CursCtl& bind_(const std::string& n, const TYPE& t, const short* ind);

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

        void throwBadBindException(const std::string& msg) const;

        std::unique_ptr<class CursCtlImpl> mImpl;
    };

    CursCtl make_curs_(const char* n, const char* f, int l, const std::string& sql);
    CursCtl make_curs_(const char* n, const char* f, int l, const std::string& sql, Session& sess);
    CursCtl make_curs_no_cache_(const char* n, const char* f, int l, const std::string& sql);
    CursCtl make_curs_no_cache_(const char* n, const char* f, int l, const std::string& sql,
                                Session& sess);
} // DbCpp

#ifndef make_db_curs
#define make_db_curs(...) DbCpp::make_curs_(STDLOG, __VA_ARGS__)
#endif // make_db_curs
#ifndef make_db_curs_no_cache
#define make_db_curs_no_cache(...) DbCpp::make_curs_no_cache_(STDLOG, __VA_ARGS__)
#endif // make_db_curs_no_cache
#endif // ENABLE_PG
