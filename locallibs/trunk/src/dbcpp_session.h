#pragma once
#ifdef ENABLE_PG

#include <memory>
#include <string>
#include <vector>

#include "dbcpp_db_type.h"

namespace OciCpp
{
    class OciSession;
    class CursCtl;
}
namespace PgCpp
{
    class Session;
    class CursCtl;
}
struct cda_text;

namespace DbCpp
{
    class CursCtl;
    enum class CopyResult
    {
        Ok,
        PrepareError,
        CopyDataError
    };

    class Session
    {
    public:
        virtual ~Session() = default;
        virtual CursCtl createCursor(const char* n, const char* f, int l, const char* sql) = 0;
        virtual CursCtl createCursor(const char* n, const char* f, int l, const std::string& sql)
            = 0;
        virtual CursCtl createCursorNoCache(const char* n, const char* f, int l, const char* sql)
            = 0;
        virtual CursCtl createCursorNoCache(const char* n, const char* f, int l,
                                            const std::string& sql)
            = 0;

        virtual DbType getType() const = 0;
        virtual void commit()          = 0;
        virtual void rollback()        = 0;

        virtual std::string getConnectString() const = 0;

        virtual CopyResult copyDataFrom(const std::string& sql, const char* data, size_t size) = 0;

        bool isOracle() const { return getType() == DbType::Oracle; }
    };

    class OraSession final : public Session
    {
        friend class OracleImpl;

    public:
        OraSession(const char* nick, const char* file, int line, const std::string& connStr);
        OraSession(const char* nick, const char* file, int line, OciCpp::OciSession* session,
                   bool releaseOnDestruction);
        ~OraSession();

        virtual CursCtl createCursor(const char* n, const char* f, int l, const char* sql) override;
        virtual CursCtl createCursor(const char* n, const char* f, int l,
                                     const std::string& sql) override;
        virtual CursCtl createCursorNoCache(const char* n, const char* f, int l,
                                            const char* sql) override;
        virtual CursCtl createCursorNoCache(const char* n, const char* f, int l,
                                            const std::string& sql) override;

        OciCpp::OciSession& get_sess4oci8() const;
        cda_text* mk_cda__(const char* nick, const char* file, int line,
                           const std::string& query) const;
        bool is_alive() const;
        void reconnect();

        virtual DbType getType() const override { return DbType::Oracle; }
        virtual void commit() override;
        virtual void rollback() override;
        virtual std::string getConnectString() const override;
        virtual CopyResult copyDataFrom(const std::string& sql, const char* data,
                                        size_t size) override;

    private:
        OciCpp::CursCtl createOraCursor(const char* n, const char* f, int l, const char* sql,
                                        bool cacheit);

        std::unique_ptr<OciCpp::OciSession> mSession;
        bool mReleaseOnDestruction;
    };

    class PgSession final : public Session
    {
        friend class PostgresImpl;

    public:
        PgSession(const char* nick, const char* file, int line, const std::string& connStr,
                  DbSessionType type);
        PgSession(const std::string& connStr, DbSessionType type);
        PgSession(const char* connStr, DbSessionType type);

        ~PgSession();
        PgSession(const PgSession&) = delete;
        PgSession(PgSession&&)      = delete;
        PgSession& operator=(const PgSession&) = delete;
        PgSession& operator=(PgSession&&) = delete;

        virtual CursCtl createCursor(const char* n, const char* f, int l, const char* sql) override;
        virtual CursCtl createCursor(const char* n, const char* f, int l,
                                     const std::string& sql) override;
        virtual CursCtl createCursorNoCache(const char* n, const char* f, int l,
                                            const char* sql) override;
        virtual CursCtl createCursorNoCache(const char* n, const char* f, int l,
                                            const std::string& sql) override;

        virtual DbType getType() const override { return DbType::Postgres; }
        virtual void commit() override;
        virtual void rollback() override;
#ifdef XP_TESTING
        void rollbackInTestMode();
#endif // XP_TESTING
        virtual std::string getConnectString() const override { return mConnectString; }
        void setClientInfo(const std::string& clientInfo);

        virtual CopyResult copyDataFrom(const std::string& sql, const char* data,
                                        size_t size) override;

    private:
        PgCpp::CursCtl createPgCursor(const char* n, const char* f, int l, const char* sql,
                                      bool cacheit);
        void activateSession();

        DbSessionType mType;
        bool mIsActive;
        std::string mConnectString;
        std::shared_ptr<PgCpp::Session> mSession;
    };

    OraSession& mainOraSession(const char* nick, const char* file, int line);
    OraSession* mainOraSessionPtr(const char* nick, const char* file, int line);
    PgSession& mainPgSession(const char* nick, const char* file, int line);
    PgSession* mainPgSessionPtr(const char* nick, const char* file, int line,
                                bool createIfNotExist = true);

} // DbCpp
#endif // ENABLE_PG
