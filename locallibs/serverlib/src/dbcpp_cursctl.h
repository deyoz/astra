#pragma once
#ifdef ENABLE_PG

#include <cstdint>
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
        CERR_INVALID_IDENTIFIER,
        CERR_TRUNC,
        CERR_SELL,
        CERR_TABLE_NOT_EXISTS,
        CERR_SNAPSHOT_TOO_OLD,
        REQUIRED_NEXT_PIECE,
        REQUIRED_NEXT_BUFFER
    };
    std::ostream& operator<<(std::ostream& os, ResultCode);

    class CursCtl
    {
    public:
        CursCtl(Session& sess, const char* n, const char* f, int l, const char* sql,
                bool cacheit = true);
        CursCtl(CursCtl&&);
        ~CursCtl();

        CursCtl& bind(const std::string& n, char*& t, const short* ind = 0) = delete;
        template <typename T, typename = std::enable_if_t<!std::is_lvalue_reference<T>::value>>
        CursCtl& bind(const std::string& n, T&& t, const short* ind = 0)
        {
            if (!isStable())
            {
                throwBadBindException("unstable rvalue bind " + n);
            }
            bind_(n, static_cast<const T&>(t), ind);
            return *this;
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
        CursCtl& bind_array(const std::string& n, const char* const& t, size_t size,
                            const short* ind = 0);
        template <size_t N>
        CursCtl& bind(const std::string& n, const std::array<char, N>& t, const short* ind = 0)
        {
            return bind_array(n, t.data(), N, ind);
        }

        template <typename T> CursCtl& def(T, short* ind = 0) = delete;
        template <typename T> CursCtl& defNull(T, const T)    = delete;
#define defType(X)                                                                                 \
    CursCtl& def(X& t, short* ind = 0);                                                            \
    CursCtl& defNull(X& t, const X& defVal)
        defType(bool);
        defType(char);
        defType(short);
        defType(int);
        defType(long);
        defType(long long);
        defType(unsigned char);
        defType(unsigned short);
        defType(unsigned int);
        defType(unsigned long);
        defType(unsigned long long);
        defType(std::string);
        defType(boost::gregorian::date);
        defType(boost::posix_time::ptime);
#undef defType
        template <size_t N> CursCtl& def(char (&t)[N], short* ind = 0) { return def(t, N, ind); }
        template <size_t N> CursCtl& defNull(char (&t)[N], const char* defVal)
        {
            return defNull(t, N, defVal, std::strlen(defVal) + 1);
        }
        template <size_t N, size_t M> CursCtl& defNull(char (&t)[N], char (&defVal)[M])
        {
            return defNull(t, N, defVal, M);
        }
        CursCtl& def(char* t, size_t size, short* ind = 0);
        CursCtl& defNull(char* t, size_t size, const char* defVal, size_t defSize);

        CursCtl& autoNull();
        CursCtl& noThrowError(ResultCode err);
        CursCtl& stb();
        CursCtl& unstb();
        CursCtl& singleRowMode();
        CursCtl& fetchLen(int count);
        CursCtl& structSize(int data, int indicator = 0);

        ResultCode exfet(int count = 1);
        ResultCode EXfet(int count = 1);
        ResultCode exec();
        ResultCode fen(int count = 1);
        ResultCode err();
        int rowcount();
        int rowcount_now();

        bool isStable() const;

    private:
        CursCtl& bind_(const std::string& n, const char* const& t, size_t size, const short* ind);
        CursCtl& bind_(const std::string& n, const char* const& t, const short* ind);
        CursCtl& bind_(const std::string& n, const bool& t, const short* ind);
        CursCtl& bind_(const std::string& n, const char& t, const short* ind);
        CursCtl& bind_(const std::string& n, const short& t, const short* ind);
        CursCtl& bind_(const std::string& n, const int& t, const short* ind);
        CursCtl& bind_(const std::string& n, const long& t, const short* ind);
        CursCtl& bind_(const std::string& n, const long long& t, const short* ind);
        CursCtl& bind_(const std::string& n, const unsigned char& t, const short* ind);
        CursCtl& bind_(const std::string& n, const unsigned short& t, const short* ind);
        CursCtl& bind_(const std::string& n, const unsigned int& t, const short* ind);
        CursCtl& bind_(const std::string& n, const unsigned long& t, const short* ind);
        CursCtl& bind_(const std::string& n, const unsigned long long& t, const short* ind);
        CursCtl& bind_(const std::string& n, const std::string& t, const short* ind);
        CursCtl& bind_(const std::string& n, const boost::gregorian::date& t, const short* ind);
        CursCtl& bind_(const std::string& n, const boost::posix_time::ptime& t, const short* ind);

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
